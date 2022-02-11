/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2017  Petr Kozelka, PPK at PtokaX dot org

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * as published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//---------------------------------------------------------------------------
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "eventqueue.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "UDPThread.h"
//---------------------------------------------------------------------------
UDPThread * UDPThread::m_PtrIPv4 = NULL;
UDPThread * UDPThread::m_PtrIPv6 = NULL;
//---------------------------------------------------------------------------

UDPThread::UDPThread() :
#ifdef _WIN32
    m_hThreadHandle(NULL), m_Socket(INVALID_SOCKET), 
#else
    m_ThreadId(0), m_Socket(-1),
#endif
    m_bTerminated(false) {
    m_RecvBuf[0] = '\0';
}

bool UDPThread::Listen(const int iAddressFamily) {
    m_Socket = socket(iAddressFamily, SOCK_DGRAM, IPPROTO_UDP);

#ifdef _WIN32
	if(m_Socket == INVALID_SOCKET) {
#else
	if(m_Socket == -1) {
#endif
		AppendLog("[ERR] UDP Socket creation error.");
		return false;
    }

#ifndef _WIN32
    int on = 1;
    setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
#endif

    sockaddr_storage sas;
    memset(&sas, 0, sizeof(sockaddr_storage));
    socklen_t sas_len;

    if(iAddressFamily == AF_INET6) {
        ((struct sockaddr_in6 *)&sas)->sin6_family = AF_INET6;
        ((struct sockaddr_in6 *)&sas)->sin6_port = htons((unsigned short)atoi(SettingManager::m_Ptr->m_sTexts[SETTXT_UDP_PORT]));
        sas_len = sizeof(struct sockaddr_in6);

        if(SettingManager::m_Ptr->m_bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true && ServerManager::m_sHubIP6[0] != '\0') {
#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
            win_inet_pton(ServerManager::m_sHubIP6, &((struct sockaddr_in6 *)&sas)->sin6_addr);
#else
            inet_pton(AF_INET6, ServerManager::m_sHubIP6, &((struct sockaddr_in6 *)&sas)->sin6_addr);
#endif
        } else {
            ((struct sockaddr_in6 *)&sas)->sin6_addr = in6addr_any;

            if(ServerManager::m_bIPv6DualStack == true && SettingManager::m_Ptr->m_bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == false) {
#ifdef _WIN32
                DWORD dwIPv6 = 0;
                setsockopt(m_Socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&dwIPv6, sizeof(dwIPv6));
#else
                int iIPv6 = 0;
                setsockopt(m_Socket, IPPROTO_IPV6, IPV6_V6ONLY, &iIPv6, sizeof(iIPv6));
#endif
            }
        }
    } else {
        ((struct sockaddr_in *)&sas)->sin_family = AF_INET;
        ((struct sockaddr_in *)&sas)->sin_port = htons((unsigned short)atoi(SettingManager::m_Ptr->m_sTexts[SETTXT_UDP_PORT]));
        sas_len = sizeof(struct sockaddr_in);

        if(SettingManager::m_Ptr->m_bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true && ServerManager::m_sHubIP[0] != '\0') {
            ((struct sockaddr_in *)&sas)->sin_addr.s_addr = inet_addr(ServerManager::m_sHubIP);
        } else {
            ((struct sockaddr_in *)&sas)->sin_addr.s_addr = INADDR_ANY;
        }
    }

#ifdef _WIN32
    if(bind(m_Socket, (struct sockaddr *)&sas, sas_len) == SOCKET_ERROR) {
		AppendLog(("[ERR] UDP Socket bind error: "+string(WSAGetLastError())).c_str());
#else
    if(bind(m_Socket, (struct sockaddr *)&sas, sas_len) == -1) {
		AppendLog(("[ERR] UDP Socket bind error: "+string(ErrnoStr(errno))+" ("+string(errno)+")").c_str());
#endif
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------

UDPThread::~UDPThread() {
#ifdef _WIN32
    if(m_hThreadHandle != NULL) {
        CloseHandle(m_hThreadHandle);
#else
    if(m_ThreadId != 0) {
        Close();
        WaitFor();
#endif
	}
}
//---------------------------------------------------------------------------

#ifdef _WIN32
unsigned __stdcall ExecuteUDP(void * pThread) {
#else
static void* ExecuteUDP(void * pThread) {
#endif
	(reinterpret_cast<UDPThread *>(pThread))->Run();

	return 0;
}
//---------------------------------------------------------------------------

void UDPThread::Resume() {
#ifdef _WIN32
	m_hThreadHandle = (HANDLE)_beginthreadex(NULL, 0, ExecuteUDP, this, 0, NULL);
	if(m_hThreadHandle == 0) {
#else
	int iRet = pthread_create(&m_ThreadId, NULL, ExecuteUDP, this);
	if(iRet != 0) {
#endif
		AppendDebugLog("%s - [ERR] Failed to create new UDPThread\n");
    }
}
//---------------------------------------------------------------------------

void UDPThread::Run() {
    sockaddr_storage sas;
	socklen_t sas_len = sizeof(sockaddr_storage);
	int len = 0;

	while(m_bTerminated == false) {
		len = recvfrom(m_Socket, m_RecvBuf, 4095, 0, (struct sockaddr *)&sas, &sas_len);

		if(len < 5 || strncmp(m_RecvBuf, "$SR ", 4) != 0) {
			continue;
		}

		m_RecvBuf[len] = '\0';

		// added ip check, we don't want fake $SR causing kick of innocent user...
        EventQueue::m_Ptr->AddThread(EventQueue::EVENT_UDP_SR, m_RecvBuf, &sas);
    }
}
//---------------------------------------------------------------------------

void UDPThread::Close() {
	m_bTerminated = true;
#ifdef _WIN32
	closesocket(m_Socket);
#else
	shutdown(m_Socket, SHUT_RDWR);
	close(m_Socket);
#endif
}
//---------------------------------------------------------------------------

void UDPThread::WaitFor() {
#ifdef _WIN32
    WaitForSingleObject(m_hThreadHandle, INFINITE);
#else
	if(m_ThreadId != 0) {
		pthread_join(m_ThreadId, NULL);
        m_ThreadId = 0;
	}
#endif
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

UDPThread * UDPThread::Create(const int iAddressFamily) {
    UDPThread * pUDPThread = new (std::nothrow) UDPThread();
    if(pUDPThread == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate pUDPThread in UDPThread::Create\n");
        return NULL;
    }

    if(pUDPThread->Listen(iAddressFamily) == true) {
        pUDPThread->Resume();
        return pUDPThread;
    } else {
        delete pUDPThread;
        return NULL;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void UDPThread::Destroy(UDPThread * pUDPThread) {
    if(pUDPThread != NULL) {
        pUDPThread->Close();
        pUDPThread->WaitFor();
        delete pUDPThread;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
