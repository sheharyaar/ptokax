/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
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
#include "LanguageManager.h"
#include "ServerManager.h"
#include "serviceLoop.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "ServerThread.h"
//---------------------------------------------------------------------------

ServerThread::AntiConFlood::AntiConFlood(const uint8_t * pIpHash) : m_ui64Time(ServerManager::m_ui64ActualTick), m_pPrev(NULL), m_pNext(NULL), m_ui16Hits(1) {
    memcpy(m_ui128IpHash, pIpHash, 16);
};
//---------------------------------------------------------------------------

ServerThread::ServerThread(const int iAddrFamily, const uint16_t ui16PortNumber) : m_pAntiFloodList(NULL), 
#ifdef _WIN32
    m_Server(INVALID_SOCKET),
#else
    m_ThreadId(0), m_Server(-1),
#endif
	m_ui32SuspendTime(0), m_iAdressFamily(iAddrFamily), m_bTerminated(false), m_pPrev(NULL), m_pNext(NULL), m_ui16Port(ui16PortNumber), 
	m_bActive(false), m_bSuspended(false) {

#ifdef _WIN32
    m_hThreadHandle = NULL;

	InitializeCriticalSection(&m_csServerThread);
#else
	pthread_mutex_init(&m_mtxServerThread, NULL);
#endif
}
//---------------------------------------------------------------------------

ServerThread::~ServerThread() {
#ifdef _WIN32
    DeleteCriticalSection(&m_csServerThread);
#else
    if(m_ThreadId != 0) {
        Close();
        WaitFor();
    }

    pthread_mutex_destroy(&m_mtxServerThread);
#endif
        
    AntiConFlood * acfcur = NULL,
        * acfnext = m_pAntiFloodList;
        
    while(acfnext != NULL) {
        acfcur = acfnext;
        acfnext = acfcur->m_pNext;
		delete acfcur;
    }

#ifdef _WIN32
    if(m_hThreadHandle != NULL) {
        CloseHandle(m_hThreadHandle);
    }
#endif
}
//---------------------------------------------------------------------------

#ifdef _WIN32
	unsigned __stdcall ExecuteServerThread(void * pThread) {
#else
	static void* ExecuteServerThread(void * pThread) {
#endif
	(reinterpret_cast<ServerThread *>(pThread))->Run();

	return 0;
}
//---------------------------------------------------------------------------

void ServerThread::Resume() {
#ifdef _WIN32
	m_hThreadHandle = (HANDLE)_beginthreadex(NULL, 0, ExecuteServerThread, this, 0, NULL);
    if(m_hThreadHandle == 0) {
#else
    int iRet = pthread_create(&m_ThreadId, NULL, ExecuteServerThread, this);
    if(iRet != 0) {
#endif
		AppendDebugLog("%s - [ERR] Failed to create new ServerThread\n");
    }
}
//---------------------------------------------------------------------------

void ServerThread::Run() {
	m_bActive = true;
#ifdef _WIN32
    SOCKET s = INVALID_SOCKET;
#else
	int s = -1;
#endif
    sockaddr_storage addr;
	socklen_t len = sizeof(addr);

#ifndef _WIN32
    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 1000000;
#endif

	while(m_bTerminated == false) {
		s = accept(m_Server, (struct sockaddr *)&addr, &len);

		if(m_ui32SuspendTime == 0) {
			if(m_bTerminated == true) {
#ifdef _WIN32
				if(s != INVALID_SOCKET) {
					shutdown(s, SD_SEND);
					closesocket(s);
				}
#else
                if(s != -1) {
                    shutdown(s, SHUT_RDWR); 
					close(s);
				}
#endif
				continue;
			}

#ifdef _WIN32
			if(s == INVALID_SOCKET) {
				if(WSAEWOULDBLOCK != WSAGetLastError()) {
#else
            if(s == -1) {
                if(errno != EWOULDBLOCK) {
                    if(errno == EMFILE) { // max opened file descriptors limit reached
                        sleep(1); // longer sleep give us better chance to have free file descriptor available on next accept call
                    } else {
#endif
						EventQueue::m_Ptr->AddThread(EventQueue::EVENT_SRVTHREAD_MSG, 
                            ("[ERR] accept() for port "+string(m_ui16Port)+" has returned error.").c_str());
                    }
#ifndef _WIN32
				}
#endif
			} else {
				if(isFlooder(s, addr) == true) {
#ifdef _WIN32
					shutdown(s, SD_SEND);
					closesocket(s);
#else
                    shutdown(s, SHUT_RDWR);
                    close(s);
#endif
				}

#ifdef _WIN32
				::Sleep(1);
#else
                nanosleep(&sleeptime, NULL);
#endif
			}
		} else {
			uint32_t iSec = 0;
			while(m_bTerminated == false) {
				if(m_ui32SuspendTime > iSec) {
#ifdef _WIN32
					::Sleep(1000);
#else
					sleep(1);
#endif
					if(m_bSuspended == false) {
						iSec++;
					}
					continue;
				}

#ifdef _WIN32
				EnterCriticalSection(&m_csServerThread);
#else
				pthread_mutex_lock(&m_mtxServerThread);
#endif
				m_ui32SuspendTime = 0;
#ifdef _WIN32
				LeaveCriticalSection(&m_csServerThread);
#else
				pthread_mutex_unlock(&m_mtxServerThread);
#endif

				if(Listen(true) == true) {
					EventQueue::m_Ptr->AddThread(EventQueue::EVENT_SRVTHREAD_MSG, 
						("[SYS] Server socket for port "+string(m_ui16Port)+" sucessfully recovered from suspend state.").c_str());
				} else {
					Close();
				}
				break;
			}
		}
	}

	m_bActive = false;
}
//---------------------------------------------------------------------------

void ServerThread::Close() {
	m_bTerminated = true;
#ifdef _WIN32
	closesocket(m_Server);
#else
    shutdown(m_Server, SHUT_RDWR);
	close(m_Server);
#endif
}
//---------------------------------------------------------------------------

void ServerThread::WaitFor() {
#ifdef _WIN32
    WaitForSingleObject(m_hThreadHandle, INFINITE);
#else
	if(m_ThreadId != 0) {
		pthread_join(m_ThreadId, NULL);
		m_ThreadId = 0;
	}
#endif
}
//---------------------------------------------------------------------------

bool ServerThread::Listen(const bool bSilent/* = false*/) {
	m_Server = socket(m_iAdressFamily, SOCK_STREAM, IPPROTO_TCP);
#ifdef _WIN32
    if(m_Server == INVALID_SOCKET) {
#else
    if(m_Server == -1) {
#endif
        if(bSilent == true) {
            EventQueue::m_Ptr->AddThread(EventQueue::EVENT_SRVTHREAD_MSG, 
#ifdef _WIN32
                ("[ERR] Unable to create server socket for port "+string(m_ui16Port)+" ! ErrorCode "+string(WSAGetLastError())).c_str());
#else
				("[ERR] Unable to create server socket for port "+string(m_ui16Port)+" ! ErrorCode "+string(errno)).c_str());
#endif
		} else {
#ifdef _BUILD_GUI
            ::MessageBox(NULL, (string(LanguageManager::m_Ptr->m_sTexts[LAN_UNB_CRT_SRVR_SCK], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_UNB_CRT_SRVR_SCK]) + " " + string(m_ui16Port) + " ! " + LanguageManager::m_Ptr->m_sTexts[LAN_ERROR_CODE] + " " + string(WSAGetLastError())).c_str(), g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
            AppendLog((string(LanguageManager::m_Ptr->m_sTexts[LAN_UNB_CRT_SRVR_SCK], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_UNB_CRT_SRVR_SCK]) + " " + string(m_ui16Port) + " ! " + LanguageManager::m_Ptr->m_sTexts[LAN_ERROR_CODE] + " " + string(errno)).c_str());
#endif
        }
        return false;
    }

#ifndef _WIN32
    int on = 1;
    if(setsockopt(m_Server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
        if(bSilent == true) {
            EventQueue::m_Ptr->AddThread(EventQueue::EVENT_SRVTHREAD_MSG,
                ("[ERR] Server socket setsockopt error: " + string(errno)+" for port: "+string(m_ui16Port)).c_str());
        } else {
            AppendLog((string(LanguageManager::m_Ptr->m_sTexts[LAN_SRV_SCKOPT_ERR], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SRV_SCKOPT_ERR])+
                ": " + string(ErrnoStr(errno))+" (" + string(errno)+") "+
                string(LanguageManager::m_Ptr->m_sTexts[LAN_FOR_PORT_LWR], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_FOR_PORT_LWR])+": "+string(m_ui16Port)).c_str());
        }
        close(m_Server);
        return false;
    }
#endif

    // set the socket properties
    sockaddr_storage sas;
    memset(&sas, 0, sizeof(sockaddr_storage));
    socklen_t sas_len;

    if(m_iAdressFamily == AF_INET6) {
        ((struct sockaddr_in6 *)&sas)->sin6_family = AF_INET6;
        ((struct sockaddr_in6 *)&sas)->sin6_port = htons(m_ui16Port);
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
                setsockopt(m_Server, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&dwIPv6, sizeof(dwIPv6));
#else
                int iIPv6 = 0;
                setsockopt(m_Server, IPPROTO_IPV6, IPV6_V6ONLY, &iIPv6, sizeof(iIPv6));
#endif
            }
        }
    } else {
        ((struct sockaddr_in *)&sas)->sin_family = AF_INET;
        ((struct sockaddr_in *)&sas)->sin_port = htons(m_ui16Port);
        sas_len = sizeof(struct sockaddr_in);

        if(SettingManager::m_Ptr->m_bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true && ServerManager::m_sHubIP[0] != '\0') {
            ((struct sockaddr_in *)&sas)->sin_addr.s_addr = inet_addr(ServerManager::m_sHubIP);
        } else {
            ((struct sockaddr_in *)&sas)->sin_addr.s_addr = INADDR_ANY;
        }
    }

    // bind it
#ifdef _WIN32
    if(bind(m_Server, (struct sockaddr *)&sas, sas_len) == SOCKET_ERROR) {
    	int err = WSAGetLastError();
#else
	if(bind(m_Server, (struct sockaddr *)&sas, sas_len) == -1) {
#endif
        if(bSilent == true) {
            EventQueue::m_Ptr->AddThread(EventQueue::EVENT_SRVTHREAD_MSG, 
#ifdef _WIN32
                ("[ERR] Server socket bind error: " + string(WSErrorStr(err))+" ("+string(err)+") for port: "+string(m_ui16Port)).c_str());
#else
				("[ERR] Server socket bind error: " + string(ErrnoStr(errno))+" ("+string(errno)+") for port: "+string(m_ui16Port)).c_str());
#endif
		} else {
#ifdef _BUILD_GUI
			::MessageBox(NULL, (string(LanguageManager::m_Ptr->m_sTexts[LAN_SRV_BIND_ERR], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SRV_BIND_ERR]) + ": " + string(WSErrorStr(err)) + " (" + string(err) + ") " + LanguageManager::m_Ptr->m_sTexts[LAN_FOR_PORT_LWR] + ": " + string(m_ui16Port)).c_str(), 
				g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
            AppendLog((string(LanguageManager::m_Ptr->m_sTexts[LAN_SRV_BIND_ERR], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SRV_BIND_ERR])+
#ifdef _WIN32
				": " + string(WSErrorStr(err))+" (" + string(err)+") "+
#else
				": " + string(ErrnoStr(errno))+" (" + string(errno)+") "+
#endif
				string(LanguageManager::m_Ptr->m_sTexts[LAN_FOR_PORT_LWR], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_FOR_PORT_LWR])+": "+string(m_ui16Port)).c_str());
#endif
        }
#ifdef _WIN32
		closesocket(m_Server);
#else
        close(m_Server);
#endif
        return false;
    }

    // set listen mode
#ifdef _WIN32
    if(listen(m_Server, 512) == SOCKET_ERROR) {
	    int err = WSAGetLastError();
#else
	if(listen(m_Server, 512) == -1) {
#endif
        if(bSilent == true) {
            EventQueue::m_Ptr->AddThread(EventQueue::EVENT_SRVTHREAD_MSG, 
#ifdef _WIN32
                ("[ERR] Server socket listen() error: " + string(WSErrorStr(err))+" ("+string(err)+") for port: "+string(m_ui16Port)).c_str());
#else
				("[ERR] Server socket listen() error: " + string(errno)+" for port: "+string(m_ui16Port)).c_str());
#endif
        } else {
#ifdef _BUILD_GUI
            ::MessageBox(NULL, (string(LanguageManager::m_Ptr->m_sTexts[LAN_SRV_LISTEN_ERR], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SRV_LISTEN_ERR]) + ": " + string(WSErrorStr(err)) + " (" + string(err) + ") " + LanguageManager::m_Ptr->m_sTexts[LAN_FOR_PORT_LWR] + ": " + string(m_ui16Port)).c_str(), 
				g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
            AppendLog((string(LanguageManager::m_Ptr->m_sTexts[LAN_SRV_LISTEN_ERR], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SRV_LISTEN_ERR])+": " + string(errno)+" "+string(LanguageManager::m_Ptr->m_sTexts[LAN_FOR_PORT_LWR], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_FOR_PORT_LWR])+": "+string(m_ui16Port)).c_str());
#endif
        }
#ifdef _WIN32
		closesocket(m_Server);
#else
		close(m_Server);
#endif
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------

#ifdef _WIN32
bool ServerThread::isFlooder(const SOCKET s, const sockaddr_storage &addr) {
#else
bool ServerThread::isFlooder(const int s, const sockaddr_storage &addr) {
#endif
    uint8_t ui128IpHash[16];
    memset(ui128IpHash, 0, 16);

    if(addr.ss_family == AF_INET6) {
        memcpy(ui128IpHash, &((struct sockaddr_in6 *)&addr)->sin6_addr, 16);
    } else {
        ui128IpHash[10] = 255;
        ui128IpHash[11] = 255;
        memcpy(ui128IpHash, &((struct sockaddr_in *)&addr)->sin_addr.s_addr, 4);
    }

    int16_t iConDefloodCount = SettingManager::m_Ptr->GetShort(SETSHORT_NEW_CONNECTIONS_COUNT);
    int16_t iConDefloodTime = SettingManager::m_Ptr->GetShort(SETSHORT_NEW_CONNECTIONS_TIME);
   
    AntiConFlood * cur = NULL,
        * nxt = m_pAntiFloodList;

	while(nxt != NULL) {
		cur = nxt;
		nxt = cur->m_pNext;

    	if(memcmp(ui128IpHash, cur->m_ui128IpHash, 16) == 0) {
            if(cur->m_ui64Time+((uint64_t)iConDefloodTime) >= ServerManager::m_ui64ActualTick) {
                cur->m_ui16Hits++;
                if(cur->m_ui16Hits > iConDefloodCount) {
                    return true;
                } else {
                    ServiceLoop::m_Ptr->AcceptSocket(s, addr);
                    return false;
                }
            } else {
                RemoveConFlood(cur);
                delete cur;
            }
        } else if(cur->m_ui64Time+((uint64_t)iConDefloodTime) < ServerManager::m_ui64ActualTick) {
            RemoveConFlood(cur);
            delete cur;
        }
    }

    AntiConFlood * pNewItem = new (std::nothrow) AntiConFlood(ui128IpHash);
    if(pNewItem == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewItem  in theLoop::isFlooder\n");
    	return true;
    }

    pNewItem->m_pNext = m_pAntiFloodList;

    if(m_pAntiFloodList != NULL) {
		m_pAntiFloodList->m_pPrev = pNewItem;
    }

	m_pAntiFloodList = pNewItem;

    ServiceLoop::m_Ptr->AcceptSocket(s, addr);

    return false;
}
//---------------------------------------------------------------------------

void ServerThread::RemoveConFlood(AntiConFlood * pACF) {
    if(pACF->m_pPrev == NULL) {
        if(pACF->m_pNext == NULL) {
			m_pAntiFloodList = NULL;
        } else {
			pACF->m_pNext->m_pPrev = NULL;
			m_pAntiFloodList = pACF->m_pNext;
        }
    } else if(pACF->m_pNext == NULL) {
		pACF->m_pPrev->m_pNext = NULL;
    } else {
		pACF->m_pPrev->m_pNext = pACF->m_pNext;
		pACF->m_pNext->m_pPrev = pACF->m_pPrev;
    }
}
//---------------------------------------------------------------------------

void ServerThread::ResumeSck() {
    if(m_bActive == true) {
#ifdef _WIN32
        EnterCriticalSection(&m_csServerThread);
#else
		pthread_mutex_lock(&m_mtxServerThread);
#endif
		m_bSuspended = false;
		m_ui32SuspendTime = 0;
#ifdef _WIN32
        LeaveCriticalSection(&m_csServerThread);
#else
		pthread_mutex_unlock(&m_mtxServerThread);
#endif
    }
}
//---------------------------------------------------------------------------

void ServerThread::SuspendSck(const uint32_t ui32Time) {
    if(m_bActive == true) {
#ifdef _WIN32
        EnterCriticalSection(&m_csServerThread);
#else
		pthread_mutex_lock(&m_mtxServerThread);
#endif
        if(ui32Time != 0) {
			m_ui32SuspendTime = ui32Time;
        } else {
			m_bSuspended = true;
			m_ui32SuspendTime = 1;            
        }
#ifdef _WIN32
        LeaveCriticalSection(&m_csServerThread);
    	closesocket(m_Server);
#else
        pthread_mutex_unlock(&m_mtxServerThread);
    	close(m_Server);
#endif
    }
}
//---------------------------------------------------------------------------
