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
#include "serviceLoop.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
#include "ZlibUtility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#ifdef _WITH_SQLITE
	#include "DB-SQLite.h"
#elif _WITH_POSTGRES
	#include "DB-PostgreSQL.h"
#elif _WITH_MYSQL
	#include "DB-MySQL.h"
#endif
#include "LuaScript.h"
#include "RegThread.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
    #include "../gui.win/MainWindowPageUsersChat.h"
#endif
//---------------------------------------------------------------------------
ServiceLoop * ServiceLoop::m_Ptr = NULL;
#if defined(_WIN32) && !defined(_WIN_IOT)
	HANDLE ServiceLoop::m_hLoopEvents[2] = { NULL, NULL };
    DWORD ServiceLoop::m_dwMainThreadId = 0;
#endif
//---------------------------------------------------------------------------

#if defined(_WIN32) && !defined(_WIN_IOT)
unsigned __stdcall ExecuteLoop(void *) {
	DWORD dwRet = 0;

	while(true) {
		dwRet = ::WaitForMultipleObjects(2, ServiceLoop::m_hLoopEvents, FALSE, INFINITE);
		if(dwRet == WAIT_OBJECT_0 || dwRet == WAIT_TIMEOUT) {
			::Sleep(100);

#ifdef _BUILD_GUI
			::PostMessage(ServerManager::m_hMainWindow, WM_PX_DO_LOOP, 0, 0);
#else
			::PostThreadMessage(ServiceLoop::m_dwMainThreadId, WM_PX_DO_LOOP, 0, 0);
#endif
		} else if(dwRet == (WAIT_OBJECT_0 + 1)) {
			break;
		}
	}

	return 0;
}
#endif
//---------------------------------------------------------------------------

ServiceLoop::AcceptedSocket::AcceptedSocket() : m_pNext(NULL),
#ifdef _WIN32
	m_Socket(INVALID_SOCKET)
#else
	m_Socket(-1)
#endif
    {
    // ...
};
//---------------------------------------------------------------------------

void ServiceLoop::Looper() {
	// PPK ... two loop stategy for saving badwith
	if(m_bRecv == true) {
		ReceiveLoop();
	} else {
		SendLoop();
		EventQueue::m_Ptr->ProcessEvents();
	}

	if(ServerManager::m_bServerTerminated == false) {
		m_bRecv = !m_bRecv;

#if defined(_WIN32) && !defined(_WIN_IOT)
		if(::SetEvent(m_hLoopEvents[0]) == 0) {
	        AppendDebugLog("%s - [ERR] Cannot set m_hLoopEvent in ServiceLoop::Looper\n");
	        exit(EXIT_FAILURE);
	    }
#endif
	} else {
#if defined(_WIN32) && !defined(_WIN_IOT)
		if(::SetEvent(m_hLoopEvents[1]) == 0) {
	        AppendDebugLog("%s - [ERR] Cannot set m_hLoopEvent in ServiceLoop::Looper\n");
	        exit(EXIT_FAILURE);
	    }
#endif

	    // tell the scripts about the end
	    ScriptManager::m_Ptr->OnExit();
	    
	    // send last possible global data
	    GlobalDataQueue::m_Ptr->SendFinalQueue();

#if defined(_WIN32) && !defined(_WIN_IOT)
		::WaitForSingleObject(m_hThreadHandle, INFINITE);
#endif

	    ServerManager::FinalStop(true);
	}
}
//---------------------------------------------------------------------------

ServiceLoop::ServiceLoop() : m_ui64LstUptmTck(ServerManager::m_ui64ActualTick), m_pAcceptedSocketsS(NULL), m_pAcceptedSocketsE(NULL), m_dLoggedUsers(0), m_dActualSrvLoopLogins(0), m_ui32LastSendRest(0), m_ui32SendRestsPeak(0), m_ui32LastRecvRest(0), m_ui32RecvRestsPeak(0), m_ui32LoopsForLogins(0), m_bRecv(true) {
#ifdef _WIN32
    InitializeCriticalSection(&m_csAcceptQueue);
#else
	pthread_mutex_init(&m_mtxAcceptQueue, NULL);
#endif

	ServerManager::m_bServerTerminated = false;
	
#ifdef _WIN32
	#ifdef _WIN_IOT
		ui64LastSecond = ::GetTickCount64() / 1000;
		ui64LastRegToHublist = ::GetTickCount64() / 1000;
	#else
		m_dwMainThreadId = ::GetCurrentThreadId();
		
		m_hLoopEvents[0] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	
	    if(m_hLoopEvents[0] == NULL) {
			AppendDebugLog("%s - [ERR] Cannot create m_hLoopEvent in ServiceLoop::ServiceLoop\n");
	    	exit(EXIT_FAILURE);
	    }

		m_hLoopEvents[1] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	
	    if(m_hLoopEvents[1] == NULL) {
			AppendDebugLog("%s - [ERR] Cannot create m_hTerminateEvent in ServiceLoop::ServiceLoop\n");
	    	exit(EXIT_FAILURE);
	    }

		m_hThreadHandle = (HANDLE)::_beginthreadex(NULL, 0, ExecuteLoop, NULL, 0, NULL);
		if(m_hThreadHandle == 0) {
			AppendDebugLog("%s - [ERR] Failed to create hThreadHandle in ServiceLoop::ServiceLoop\n");
			exit(EXIT_FAILURE);
		}
	#endif
#else
	#ifdef __MACH__
		mach_timespec_t mts;
		clock_get_time(ServerManager::m_csMachClock, &mts);
		m_ui64LastSecond = mts.tv_sec;
		m_ui64LastRegToHublist =  mts.tv_sec;
	#else
		timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		m_ui64LastSecond = ts.tv_sec;
		m_ui64LastRegToHublist = ts.tv_sec;
	#endif
#endif
}
//---------------------------------------------------------------------------

ServiceLoop::~ServiceLoop() {
    AcceptedSocket * cursck = NULL,
        * nextsck = m_pAcceptedSocketsS;
        
    while(nextsck != NULL) {
        cursck = nextsck;
		nextsck = cursck->m_pNext;
#ifdef _WIN32
		shutdown(cursck->m_Socket, SD_SEND);
        closesocket(cursck->m_Socket);
#else
		shutdown(cursck->m_Socket, SHUT_RDWR);
        close(cursck->m_Socket);
#endif
        delete cursck;
	}

#ifdef _WIN32
	DeleteCriticalSection(&m_csAcceptQueue);

	#ifndef _WIN_IOT
		if(m_hThreadHandle != 0) {
	        ::CloseHandle(m_hThreadHandle);
	    }

	    if(m_hLoopEvents[0] != NULL) {
	    	::CloseHandle(m_hLoopEvents[0]);
		}

		if(m_hLoopEvents[1] != NULL) {
	    	::CloseHandle(m_hLoopEvents[1]);
		}
    #endif
#else
	pthread_mutex_destroy(&m_mtxAcceptQueue);
#endif

	Cout(string("MainLoop terminated."));
}
//---------------------------------------------------------------------------

void ServiceLoop::AcceptUser(AcceptedSocket * pAccptSocket) {
    bool bIPv6 = false;

    char sIP[40];

    uint8_t ui128IpHash[16];
    memset(ui128IpHash, 0, 16);

    uint16_t ui16IpTableIdx = 0;

    if(pAccptSocket->m_Addr.ss_family == AF_INET6) {
        memcpy(ui128IpHash, &((struct sockaddr_in6 *)&pAccptSocket->m_Addr)->sin6_addr.s6_addr, 16);

        if(IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)&pAccptSocket->m_Addr)->sin6_addr)) {
			in_addr ipv4addr;
			memcpy(&ipv4addr, ((struct sockaddr_in6 *)&pAccptSocket->m_Addr)->sin6_addr.s6_addr + 12, 4);
			strcpy(sIP, inet_ntoa(ipv4addr));

            ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
        } else {
            bIPv6 = true;
#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
            win_inet_ntop(&((struct sockaddr_in6 *)&pAccptSocket->m_Addr)->sin6_addr, sIP, 40);
#else
            inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&pAccptSocket->m_Addr)->sin6_addr, sIP, 40);
#endif
            ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
        }
    } else {
        strcpy(sIP, inet_ntoa(((struct sockaddr_in *)&pAccptSocket->m_Addr)->sin_addr));

        ui128IpHash[10] = 255;
        ui128IpHash[11] = 255;
        memcpy(ui128IpHash+12, &((struct sockaddr_in *)&pAccptSocket->m_Addr)->sin_addr.s_addr, 4);

        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    }

    // set the recv buffer
#ifdef _WIN32
    int32_t bufsize = 8192;
    if(setsockopt(pAccptSocket->m_Socket, SOL_SOCKET, SO_RCVBUF, (char *) &bufsize, sizeof(bufsize)) == SOCKET_ERROR) {
    	int iError = WSAGetLastError();
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] setsockopt failed on attempt to set SO_RCVBUF. IP: %s Err: %s (%d)",  sIP, WSErrorStr(iError), iError);
#else
    int bufsize = 8192;
    if(setsockopt(pAccptSocket->m_Socket, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) == -1) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] setsockopt failed on attempt to set SO_RCVBUF. IP: %s Err: %s (%d)", sIP, ErrnoStr(errno), errno);
#endif
#ifdef _WIN32
    	shutdown(pAccptSocket->m_Socket, SD_SEND);
        closesocket(pAccptSocket->m_Socket);
#else
    	shutdown(pAccptSocket->m_Socket, SHUT_RDWR);
        close(pAccptSocket->m_Socket);
#endif
        return;
    }

    // set the send buffer
    bufsize = 32768;
#ifdef _WIN32
    if(setsockopt(pAccptSocket->m_Socket, SOL_SOCKET, SO_SNDBUF, (char *) &bufsize, sizeof(bufsize)) == SOCKET_ERROR) {
        int iError = WSAGetLastError();
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] setsockopt failed on attempt to set SO_SNDBUF. IP: %s Err: %s (%d)", sIP, WSErrorStr(iError), iError);
#else
    if(setsockopt(pAccptSocket->m_Socket, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) == -1) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] setsockopt failed on attempt to set SO_SNDBUF. IP: %s Err: %s (%d)", sIP, ErrnoStr(errno), errno);
#endif
#ifdef _WIN32
	    shutdown(pAccptSocket->m_Socket, SD_SEND);
        closesocket(pAccptSocket->m_Socket);
#else
	    shutdown(pAccptSocket->m_Socket, SHUT_RDWR);
        close(pAccptSocket->m_Socket);
#endif
        return;
    }

    // set sending of keepalive packets
#ifdef _WIN32
    bool bKeepalive = true;
    setsockopt(pAccptSocket->m_Socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&bKeepalive, sizeof(bKeepalive));
#else
    int iKeepAlive = 1;
    if(setsockopt(pAccptSocket->m_Socket, SOL_SOCKET, SO_KEEPALIVE, &iKeepAlive, sizeof(iKeepAlive)) == -1) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] setsockopt failed on attempt to set SO_KEEPALIVE. IP: %s Err: %s (%d)", sIP, ErrnoStr(errno), errno);

	    shutdown(pAccptSocket->m_Socket, SHUT_RDWR);
        close(pAccptSocket->m_Socket);

        return;
    }
#endif

    // set non-blocking mode
#ifdef _WIN32
    uint32_t block = 1;
	if(ioctlsocket(pAccptSocket->m_Socket, FIONBIO, (unsigned long *)&block) == SOCKET_ERROR) {
    	int iError = WSAGetLastError();
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] ioctlsocket failed on attempt to set FIONBIO. IP: %s Err: %s (%d)", sIP, WSErrorStr(iError), iError);
#else
    int oldFlag = fcntl(pAccptSocket->m_Socket, F_GETFL, 0);
    if(fcntl(pAccptSocket->m_Socket, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] fcntl failed on attempt to set O_NONBLOCK. IP: %s Err: %s (%d)", sIP, ErrnoStr(errno), errno);
#endif
#ifdef _WIN32
		shutdown(pAccptSocket->m_Socket, SD_SEND);
		closesocket(pAccptSocket->m_Socket);
#else
		shutdown(pAccptSocket->m_Socket, SHUT_RDWR);
		close(pAccptSocket->m_Socket);
#endif
		return;
    }
    
    if(SettingManager::m_Ptr->m_bBools[SETBOOL_REDIRECT_ALL] == true) {
        if(SettingManager::m_Ptr->m_sTexts[SETTXT_REDIRECT_ADDRESS] != NULL) {
       	    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> %s %s|%s", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_REDIR_TO], SettingManager::m_Ptr->m_sTexts[SETTXT_REDIRECT_ADDRESS], 
			   SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_REDIRECT_ADDRESS]);
            if(iMsgLen > 0) {
                send(pAccptSocket->m_Socket, ServerManager::m_pGlobalBuffer, iMsgLen, 0);
                ServerManager::m_ui64BytesSent += iMsgLen;
            }
        }
#ifdef _WIN32
        shutdown(pAccptSocket->m_Socket, SD_SEND);
        closesocket(pAccptSocket->m_Socket);
#else
        shutdown(pAccptSocket->m_Socket, SHUT_RDWR);
        close(pAccptSocket->m_Socket);
#endif
		return;
    }

    time_t acc_time;
    time(&acc_time);

	BanItem * Ban = BanManager::m_Ptr->FindFull(ui128IpHash, acc_time);

	if(Ban != NULL) {
        if(((Ban->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true) {
            int iMsgLen = GenerateBanMessage(Ban, acc_time);
            if(iMsgLen != 0) {
            	send(pAccptSocket->m_Socket, ServerManager::m_pGlobalBuffer, iMsgLen, 0);
            }
#ifdef _WIN32
            shutdown(pAccptSocket->m_Socket, SD_SEND);
            closesocket(pAccptSocket->m_Socket);
#else
            shutdown(pAccptSocket->m_Socket, SHUT_RD);
            close(pAccptSocket->m_Socket);
#endif
//			UdpDebug::m_Ptr->BroadcastFormat("[SYS] Banned ip %s - connection closed.", sIP);

            return;
        }
    }

	RangeBanItem * RangeBan = BanManager::m_Ptr->FindFullRange(ui128IpHash, acc_time);

	if(RangeBan != NULL) {
        if(((RangeBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true) {
            int iMsgLen = GenerateRangeBanMessage(RangeBan, acc_time);
            if(iMsgLen != 0) {
            	send(pAccptSocket->m_Socket, ServerManager::m_pGlobalBuffer, iMsgLen, 0);
            }
#ifdef _WIN32
            shutdown(pAccptSocket->m_Socket, SD_SEND);
            closesocket(pAccptSocket->m_Socket);
#else
            shutdown(pAccptSocket->m_Socket, SHUT_RD);
            close(pAccptSocket->m_Socket);
#endif
//            UdpDebug::m_Ptr->BroadcastFormat("[SYS] Range Banned ip %s - connection closed.", sIP);

            return;
        }
    }

    ServerManager::m_ui32Joins++;

    // set properties of the new user object
	User * pUser = new (std::nothrow) User();

	if(pUser == NULL) {
#ifdef _WIN32
		shutdown(pAccptSocket->m_Socket, SD_SEND);
		closesocket(pAccptSocket->m_Socket);
#else
		shutdown(pAccptSocket->m_Socket, SHUT_RDWR);
		close(pAccptSocket->m_Socket);
#endif
        AppendDebugLog("%s - [MEM] Cannot allocate pUser in ServiceLoop::AcceptUser\n");
		return;
	}

	pUser->m_pLogInOut = new (std::nothrow) LoginLogout();

    if(pUser->m_pLogInOut == NULL) {
#ifdef _WIN32
		shutdown(pAccptSocket->m_Socket, SD_SEND);
		closesocket(pAccptSocket->m_Socket);
#else
		shutdown(pAccptSocket->m_Socket, SHUT_RDWR);
		close(pAccptSocket->m_Socket);
#endif
        delete pUser;

        AppendDebugLog("%s - [MEM] Cannot allocate pLogInOut in ServiceLoop::AcceptUser\n");
		return;
    }

    pUser->m_pLogInOut->m_ui64LogonTick = ServerManager::m_ui64ActualTick;
    pUser->m_Socket = pAccptSocket->m_Socket;

    memcpy(pUser->m_ui128IpHash, ui128IpHash, 16);
    pUser->m_ui16IpTableIdx = ui16IpTableIdx;

    pUser->SetIP(sIP);

    if(bIPv6 == true) {
        pUser->m_ui32BoolBits |= User::BIT_IPV6;
    } else {
        pUser->m_ui32BoolBits |= User::BIT_IPV4;
    }
    
    if(Ban != NULL) {
        uint32_t hash = 0;
        if(((Ban->m_ui8Bits & BanManager::NICK) == BanManager::NICK) == true) {
            hash = Ban->m_ui32NickHash;
        }
        int iMsglen = GenerateBanMessage(Ban, acc_time);
        pUser->m_pLogInOut->m_pBan = UserBan::CreateUserBan(ServerManager::m_pGlobalBuffer, iMsglen, hash);
        if(pUser->m_pLogInOut->m_pBan == NULL) {
#ifdef _WIN32
            shutdown(pAccptSocket->m_Socket, SD_SEND);
            closesocket(pAccptSocket->m_Socket);
#else
            shutdown(pAccptSocket->m_Socket, SHUT_RDWR);
            close(pAccptSocket->m_Socket);
#endif

            AppendDebugLog("%s - [MEM] Cannot allocate new uBan in ServiceLoop::AcceptUser\n");

            delete pUser;

            return;
        }
    } else if(RangeBan != NULL) {
        int iMsgLen = GenerateRangeBanMessage(RangeBan, acc_time);
        pUser->m_pLogInOut->m_pBan = UserBan::CreateUserBan(ServerManager::m_pGlobalBuffer, iMsgLen, 0);
        if(pUser->m_pLogInOut->m_pBan == NULL) {
#ifdef _WIN32
            shutdown(pAccptSocket->m_Socket, SD_SEND);
            closesocket(pAccptSocket->m_Socket);
#else
            shutdown(pAccptSocket->m_Socket, SHUT_RDWR);
            close(pAccptSocket->m_Socket);
#endif

        	AppendDebugLog("%s - [MEM] Cannot allocate new uBan in ServiceLoop::AcceptUser1\n");

            delete pUser;

        	return;
        }
    }
        
    // Everything is ok, now add to users...
    Users::m_Ptr->AddUser(pUser);
}
//---------------------------------------------------------------------------

void ServiceLoop::ReceiveLoop() {
#ifndef _WIN32
	timespec ts;
	#ifdef __MACH__
		mach_timespec_t mts;
		clock_get_time(ServerManager::m_csMachClock, &mts);
		ts.tv_sec = mts.tv_sec;
		ts.tv_nsec = mts.tv_nsec;
	#else
		clock_gettime(CLOCK_MONOTONIC, &ts);
	#endif

	if((uint64_t)ts.tv_sec != m_ui64LastSecond) {
		m_ui64LastSecond = ts.tv_sec;

		ServerManager::OnSecTimer();
	}

	if(SettingManager::m_Ptr->m_bBools[SETBOOL_AUTO_REG] == true) {
		if(((uint64_t)ts.tv_sec - m_ui64LastRegToHublist) >= 901) { // 15 min 1 sec is hublist register interval
			m_ui64LastRegToHublist = ts.tv_sec;

			ServerManager::OnRegTimer();
		}
	}

	ScriptOnTimer((uint64_t(ts.tv_sec) * 1000) + (uint64_t(ts.tv_nsec) / 1000000));
#elif defined(_WIN32) && defined(_WIN_IOT)
	uint64_t ui64Millis = (uint64_t)::GetTickCount64();
	uint64_t ui64Secs = ui64Millis / 1000;

	if(ui64Secs != ui64LastSecond) {
		ui64LastSecond = ui64Secs;

		ServerManager::OnSecTimer();
	}

	if(SettingManager::m_Ptr->m_bBools[SETBOOL_AUTO_REG] == true) {
		if((ui64Secs - ui64LastRegToHublist) >= 901) { // 15 min 1 sec is hublist register interval
			ui64LastRegToHublist = ui64Secs;

			ServerManager::OnRegTimer();
		}
	}

	ScriptOnTimer(ui64Millis);
#endif

    // Receiving loop for process all incoming data and store in queues
    uint32_t iRecvRests = 0;
    
    ServerManager::m_ui8SrCntr++;
    
    if(ServerManager::m_ui8SrCntr >= 7 || (Users::m_Ptr->m_ui16ActSearchs + Users::m_Ptr->m_ui16PasSearchs) > 8 || 
        Users::m_Ptr->m_ui16ActSearchs > 5) {
        ServerManager::m_ui8SrCntr = 0;
    }

    if(ServerManager::m_ui64ActualTick-m_ui64LstUptmTck > 60) {
        time_t acctime;
        time(&acctime);
        acctime -= ServerManager::m_tStartTime;
    
        uint64_t iValue = acctime / 86400;
		acctime -= (time_t)(86400*iValue);
        ServerManager::m_ui64Days = iValue;
    
        iValue = acctime / 3600;
    	acctime -= (time_t)(3600*iValue);
        ServerManager::m_ui64Hours = iValue;
    
    	iValue = acctime / 60;
        ServerManager::m_ui64Mins = iValue;

        if(ServerManager::m_ui64Mins == 0 || ServerManager::m_ui64Mins == 15 || ServerManager::m_ui64Mins == 30 || ServerManager::m_ui64Mins == 45) {
            RegManager::m_Ptr->Save(false, true);
#ifdef _WIN32
            if(HeapValidate(GetProcessHeap(), 0, 0) == 0) {
                AppendDebugLog("%s - [ERR] Process memory heap corrupted\n");
            }
            HeapCompact(GetProcessHeap(), 0);

            if(HeapValidate(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, 0) == 0) {
                AppendDebugLog("%s - [ERR] PtokaX memory heap corrupted\n");
            }
            HeapCompact(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE);

            if(HeapValidate(ServerManager::m_hRecvHeap, HEAP_NO_SERIALIZE, 0) == 0) {
                AppendDebugLog("%s - [ERR] Recv memory heap corrupted\n");
            }
            HeapCompact(ServerManager::m_hRecvHeap, HEAP_NO_SERIALIZE);

            if(HeapValidate(ServerManager::m_hSendHeap, HEAP_NO_SERIALIZE, 0) == 0) {
                AppendDebugLog("%s - [ERR] Send memory heap corrupted\n");
            }
            HeapCompact(ServerManager::m_hSendHeap, HEAP_NO_SERIALIZE);

            if(HeapValidate(ServerManager::m_hLuaHeap, 0, 0) == 0) {
                AppendDebugLog("%s - [ERR] Lua memory heap corrupted\n");
            }
            HeapCompact(ServerManager::m_hLuaHeap, 0);
#endif
        }

		m_ui64LstUptmTck = ServerManager::m_ui64ActualTick;
    }

    AcceptedSocket * CurSck = NULL,
        * NextSck = NULL;

#ifdef _WIN32
    EnterCriticalSection(&m_csAcceptQueue);
#else
	pthread_mutex_lock(&m_mtxAcceptQueue);
#endif

    if(m_pAcceptedSocketsS != NULL) {
        NextSck = m_pAcceptedSocketsS;
		m_pAcceptedSocketsS = NULL;
		m_pAcceptedSocketsE = NULL;
    }

#ifdef _WIN32
    LeaveCriticalSection(&m_csAcceptQueue);
#else
	pthread_mutex_unlock(&m_mtxAcceptQueue);
#endif

    while(NextSck != NULL) {
        CurSck = NextSck;
        NextSck = CurSck->m_pNext;
        AcceptUser(CurSck);
        delete CurSck;
    }

    User * curUser = NULL,
        * nextUser = Users::m_Ptr->m_pUserListS;

    while(nextUser != 0 && ServerManager::m_bServerTerminated == false) {
        curUser = nextUser;
        nextUser = curUser->m_pNext;

        // PPK ... true == we have rest ;)
        if(curUser->DoRecv() == true) {
            iRecvRests++;
            //Memo("Rest " + string(curUser->Nick, curUser->NickLen) + ": '" + string(curUser->pRecvBuf) + "'");
        }

        //    curUser->ProcessLines();
        //}

        switch(curUser->m_ui8State) {
        	case User::STATE_SOCKET_ACCEPTED: {
        		if(ServerManager::m_ui64ActualTick != curUser->m_pLogInOut->m_ui64LogonTick) {
    				if(curUser->MakeLock() == false) {
                    	curUser->Close();
                    	continue;
    				}

        			curUser->m_ui8State = User::STATE_KEY_OR_SUP;
        		}

				break;
			}
            case User::STATE_KEY_OR_SUP:{
                // check logon timeout for iState 1
                if(ServerManager::m_ui64ActualTick - curUser->m_pLogInOut->m_ui64LogonTick > 20) {
					UdpDebug::m_Ptr->BroadcastFormat("[SYS] Login timeout 1 for %s - user disconnected.", curUser->m_sIP);

                    curUser->Close();
                    continue;
                }
                break;
            }
            case User::STATE_IPV4_CHECK: {
                // check IPv4Check timeout
                if((ServerManager::m_ui64ActualTick - curUser->m_pLogInOut->m_ui64IPv4CheckTick) > 10) {
					UdpDebug::m_Ptr->BroadcastFormat("[SYS] IPv4Check timeout for %s (%s).", curUser->m_sNick, curUser->m_sIP);

                    curUser->m_ui8State = User::STATE_ADDME;
                    continue;
                }
                break;
            }
            case User::STATE_ADDME: {
                // PPK ... Add user, but only if send $GetNickList (or have quicklist supports) <- important, used by flooders !!!
                if(((curUser->m_ui32BoolBits & User::BIT_GETNICKLIST) == User::BIT_GETNICKLIST) == false &&
                    ((curUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false &&
                    ((curUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == true)
                    continue;

                curUser->SendFormat("ServiceLoop::ReceiveLoop->User::STATE_ADDME", true, "%s%" PRIu64 " %s, %" PRIu64 " %s, %" PRIu64 " %s / %s: %u)|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_NAME_WLCM], ServerManager::m_ui64Days, LanguageManager::m_Ptr->m_sTexts[LAN_DAYS_LWR], 
					ServerManager::m_ui64Hours, LanguageManager::m_Ptr->m_sTexts[LAN_HOURS_LWR], ServerManager::m_ui64Mins, LanguageManager::m_Ptr->m_sTexts[LAN_MINUTES_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_USERS], ServerManager::m_ui32Logged);
                curUser->m_ui8State = User::STATE_ADDME_1LOOP;
                continue;
            }
            case User::STATE_ADDME_1LOOP: {
                // PPK ... added login delay.
                if(m_dLoggedUsers >= m_dActualSrvLoopLogins && ((curUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                    if(ServerManager::m_ui64ActualTick - curUser->m_pLogInOut->m_ui64LogonTick > 300) {
						UdpDebug::m_Ptr->BroadcastFormat("[SYS] Login timeout (%d) 3 for %s (%s) - user disconnected.", (int)curUser->m_ui8State, curUser->m_sNick, curUser->m_sIP);

                        curUser->Close();
                    }
                    continue;
                }

                // PPK ... is not more needed, free mem ;)
                curUser->FreeBuffer();

                if((curUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6 && ((curUser->m_ui32BoolBits & User::SUPPORTBIT_IPV4) == User::SUPPORTBIT_IPV4) == false) {
                    in_addr ipv4addr;
					ipv4addr.s_addr = INADDR_NONE;

                    if(curUser->m_ui128IpHash[0] == 32 && curUser->m_ui128IpHash[1] == 2) { // 6to4 tunnel
                        memcpy(&ipv4addr, curUser->m_ui128IpHash + 2, 4);
                    } else if(curUser->m_ui128IpHash[0] == 32 && curUser->m_ui128IpHash[1] == 1 && curUser->m_ui128IpHash[2] == 0 && curUser->m_ui128IpHash[3] == 0) { // teredo tunnel
                        uint32_t ui32Ip = 0;
                        memcpy(&ui32Ip, curUser->m_ui128IpHash + 12, 4);
                        ui32Ip ^= 0xffffffff;
                        memcpy(&ipv4addr, &ui32Ip, 4);
                    }

                    if(ipv4addr.s_addr != INADDR_NONE) {
                        strcpy(curUser->m_sIPv4, inet_ntoa(ipv4addr));
                        curUser->m_ui8IPv4Len = (uint8_t)strlen(curUser->m_sIPv4);
                        curUser->m_ui32BoolBits |= User::BIT_IPV4;
                    }
                }

                //New User Connected ... the user is operator ? invoke lua User/OpConnected
                uint32_t iBeforeLuaLen = curUser->m_ui32SendBufDataLen;

				bool bRet = ScriptManager::m_Ptr->UserConnected(curUser);
				if(curUser->m_ui8State >= User::STATE_CLOSING) {// connection closed by script?
                    if(bRet == false) { // only when all scripts process userconnected
                        ScriptManager::m_Ptr->UserDisconnected(curUser);
                    }

					continue;
				}

                if(iBeforeLuaLen < curUser->m_ui32SendBufDataLen) {
                    size_t szNeededLen = curUser->m_ui32SendBufDataLen-iBeforeLuaLen;

					void * sOldBuf = curUser->m_pLogInOut->m_pBuffer;
#ifdef _WIN32
					if(curUser->m_pLogInOut->m_pBuffer == NULL) {
						curUser->m_pLogInOut->m_pBuffer = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededLen+1);
					} else {
						curUser->m_pLogInOut->m_pBuffer = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, sOldBuf, szNeededLen+1);
					}
#else
					curUser->m_pLogInOut->m_pBuffer = (char *)realloc(sOldBuf, szNeededLen+1);
#endif
                    if(curUser->m_pLogInOut->m_pBuffer == NULL) {
                        curUser->m_ui32BoolBits |= User::BIT_ERROR;
                        curUser->Close();

						AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for sLockUsrConn in ServiceLoop::ReceiveLoop\n", szNeededLen+1);

                		continue;
                    }
                    memcpy(curUser->m_pLogInOut->m_pBuffer, curUser->m_pSendBuf+iBeforeLuaLen, szNeededLen);
                	curUser->m_pLogInOut->m_ui32UserConnectedLen = (uint32_t)szNeededLen;
                	curUser->m_pLogInOut->m_pBuffer[curUser->m_pLogInOut->m_ui32UserConnectedLen] = '\0';
                	curUser->m_ui32SendBufDataLen = iBeforeLuaLen;
                	curUser->m_pSendBuf[curUser->m_ui32SendBufDataLen] = '\0';
                }

                // PPK ... wow user is accepted, now add it :)
                if(((curUser->m_ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG) == true) {
                    curUser->HasSuspiciousTag();
                }
                    
                curUser->Add2Userlist();
                
				m_dLoggedUsers++;
                curUser->m_ui8State = User::STATE_ADDME_2LOOP;
                ServerManager::m_ui64TotalShare += curUser->m_ui64SharedSize;
                curUser->m_ui32BoolBits |= User::BIT_HAVE_SHARECOUNTED;
                
#ifdef _BUILD_GUI
                if(::SendMessage(MainWindowPageUsersChat::m_Ptr->m_hWndPageItems[MainWindowPageUsersChat::BTN_AUTO_UPDATE_USERLIST], BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    MainWindowPageUsersChat::m_Ptr->AddUser(curUser);
                }
#endif
//                if(sqldb) sqldb->AddVisit(curUser);
#ifdef _WITH_SQLITE
				DBSQLite::m_Ptr->UpdateRecord(curUser);
#elif _WITH_POSTGRES
				DBPostgreSQL::m_Ptr->UpdateRecord(curUser);
#elif _WITH_MYSQL
				DBMySQL::m_Ptr->UpdateRecord(curUser);
#endif

                // PPK ... change to NoHello supports
            	int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Hello %s|", curUser->m_sNick);
            	if(iMsgLen > 0) {
                    GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, iMsgLen, NULL, 0, GlobalDataQueue::CMD_HELLO);
                }

                GlobalDataQueue::m_Ptr->UserIPStore(curUser);

                switch(SettingManager::m_Ptr->m_ui8FullMyINFOOption) {
                    case 0:
                        GlobalDataQueue::m_Ptr->AddQueueItem(curUser->m_sMyInfoLong, curUser->m_ui16MyInfoLongLen, NULL, 0, GlobalDataQueue::CMD_MYINFO);
                        break;
                    case 1:
                        GlobalDataQueue::m_Ptr->AddQueueItem(curUser->m_sMyInfoShort, curUser->m_ui16MyInfoShortLen, curUser->m_sMyInfoLong, curUser->m_ui16MyInfoLongLen, GlobalDataQueue::CMD_MYINFO);
                        break;
                    case 2:
                        GlobalDataQueue::m_Ptr->AddQueueItem(curUser->m_sMyInfoShort, curUser->m_ui16MyInfoShortLen, NULL, 0, GlobalDataQueue::CMD_MYINFO);
                        break;
                    default:
                        break;
                }
                
                if(((curUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                    GlobalDataQueue::m_Ptr->OpListStore(curUser->m_sNick);
                }
                
                curUser->m_ui64LastMyINFOSendTick = ServerManager::m_ui64ActualTick;
                break;
            }
            case User::STATE_ADDED:
                if(curUser->m_pCmdToUserStrt != NULL) {
                    PrcsdToUsrCmd * cur = NULL,
                        * next = curUser->m_pCmdToUserStrt;
                        
                    curUser->m_pCmdToUserStrt = NULL;
                    curUser->m_pCmdToUserEnd = NULL;
            
                    while(next != NULL) {
                        cur = next;
                        next = cur->m_pNext;
                                               
                        if(cur->m_ui32Loops >= 2) {
                            User * pToUser = HashManager::m_Ptr->FindUser(cur->m_sToNick, cur->m_ui32ToNickLen);
                            if(pToUser == cur->m_pToUser) {
                                if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_PM_COUNT_TO_USER] == 0 || cur->m_ui32PmCount == 0) {
                                    cur->m_pToUser->SendCharDelayed(cur->m_sCommand, cur->m_ui32Len);
                                } else {
                                    if(cur->m_pToUser->m_ui32ReceivedPmCount == 0) {
                                        cur->m_pToUser->m_ui64ReceivedPmTick = ServerManager::m_ui64ActualTick;
                                    } else if(cur->m_pToUser->m_ui32ReceivedPmCount >= (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_PM_COUNT_TO_USER]) {
                                        if(cur->m_pToUser->m_ui64ReceivedPmTick+60 < ServerManager::m_ui64ActualTick) {
                                            cur->m_pToUser->m_ui64ReceivedPmTick = ServerManager::m_ui64ActualTick;
                                            cur->m_pToUser->m_ui32ReceivedPmCount = 0;
                                        } else {
                                            if(cur->m_ui32PmCount == 1) {
                                                curUser->SendFormat("ServiceLoop::ReceiveLoop->User::STATE_ADDED1", true, "$To: %s From: %s $<%s> %s %s %s!|", curUser->m_sNick, cur->m_sToNick, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SRY_LST_MSG_BCS], 
													cur->m_sToNick, LanguageManager::m_Ptr->m_sTexts[LAN_EXC_MSG_LIMIT]);
                                            } else {
                                                curUser->SendFormat("ServiceLoop::ReceiveLoop->User::STATE_ADDED2", true, "$To: %s From: %s $<%s> %s %u %s %s %s!|", curUser->m_sNick, cur->m_sToNick, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SORRY_LAST], 
													cur->m_ui32PmCount, LanguageManager::m_Ptr->m_sTexts[LAN_MSGS_NOT_SENT], cur->m_sToNick, LanguageManager::m_Ptr->m_sTexts[LAN_EXC_MSG_LIMIT]);
                                            }
#ifdef _WIN32
                                            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->m_sCommand) == 0) {
												AppendDebugLog("%s - [MEM] Cannot deallocate cur->sCommand in ServiceLoop::ReceiveLoop\n");
                                            }
#else
											free(cur->m_sCommand);
#endif
                                            cur->m_sCommand = NULL;

#ifdef _WIN32
                                            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->m_sToNick) == 0) {
												AppendDebugLog("%s - [MEM] Cannot deallocate cur->m_sToNick in ServiceLoop::ReceiveLoop\n");
                                            }
#else
											free(cur->m_sToNick);
#endif
                                            cur->m_sToNick = NULL;
        
                                            delete cur;

                                            continue;
                                        }
                                    }  
                                    cur->m_pToUser->SendCharDelayed(cur->m_sCommand, cur->m_ui32Len);
                                    cur->m_pToUser->m_ui32ReceivedPmCount += cur->m_ui32PmCount;
                                }
                            }
                        } else {
                            cur->m_ui32Loops++;
                            if(curUser->m_pCmdToUserStrt == NULL) {
                                cur->m_pNext = NULL;
                                curUser->m_pCmdToUserStrt = cur;
                                curUser->m_pCmdToUserEnd = cur;
                            } else {
                                curUser->m_pCmdToUserEnd->m_pNext = cur;
                                curUser->m_pCmdToUserEnd = cur;
                            }
                            continue;
                        }

#ifdef _WIN32
                        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->m_sCommand) == 0) {
							AppendDebugLog("%s - [MEM] Cannot deallocate cur->m_sCommand in ServiceLoop::ReceiveLoop\n");
                        }
#else
						free(cur->m_sCommand);
#endif
                        cur->m_sCommand = NULL;

#ifdef _WIN32
                        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->m_sToNick) == 0) {
							AppendDebugLog("%s - [MEM] Cannot deallocate cur->m_ToNick in ServiceLoop::ReceiveLoop\n");
                        }
#else
						free(cur->m_sToNick);
#endif
                        cur->m_sToNick = NULL;
        
                        delete cur;
					}
                }
        
                if(ServerManager::m_ui8SrCntr == 0) {
                    if(curUser->m_pCmdActive6Search != NULL) {
						if(curUser->m_pCmdActive4Search != NULL) {
							GlobalDataQueue::m_Ptr->AddQueueItem(curUser->m_pCmdActive6Search->m_sCommand, curUser->m_pCmdActive6Search->m_ui32Len,
								curUser->m_pCmdActive4Search->m_sCommand, curUser->m_pCmdActive4Search->m_ui32Len, GlobalDataQueue::CMD_ACTIVE_SEARCH_V64);
						} else {
							GlobalDataQueue::m_Ptr->AddQueueItem(curUser->m_pCmdActive6Search->m_sCommand, curUser->m_pCmdActive6Search->m_ui32Len, NULL, 0, GlobalDataQueue::CMD_ACTIVE_SEARCH_V6);
						}
                    } else if(curUser->m_pCmdActive4Search != NULL) {
						GlobalDataQueue::m_Ptr->AddQueueItem(curUser->m_pCmdActive4Search->m_sCommand, curUser->m_pCmdActive4Search->m_ui32Len, NULL, 0, GlobalDataQueue::CMD_ACTIVE_SEARCH_V4);
					}

                    if(curUser->m_pCmdPassiveSearch != NULL) {
						uint8_t ui8CmdType = GlobalDataQueue::CMD_PASSIVE_SEARCH_V4;
						if((curUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
							if((curUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) {
                                if((curUser->m_ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE) {
                                    ui8CmdType = GlobalDataQueue::CMD_PASSIVE_SEARCH_V4_ONLY;
                                } else if((curUser->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE) {
                                    ui8CmdType = GlobalDataQueue::CMD_PASSIVE_SEARCH_V6_ONLY;
                                } else {
                                    ui8CmdType = GlobalDataQueue::CMD_PASSIVE_SEARCH_V64;
                                }
                            } else {
								ui8CmdType = GlobalDataQueue::CMD_PASSIVE_SEARCH_V6;
                            }
						}

						GlobalDataQueue::m_Ptr->AddQueueItem(curUser->m_pCmdPassiveSearch->m_sCommand, curUser->m_pCmdPassiveSearch->m_ui32Len, NULL, 0, ui8CmdType);

                        User::DeletePrcsdUsrCmd(curUser->m_pCmdPassiveSearch);
                        curUser->m_pCmdPassiveSearch = NULL;
                    }
                }

                // PPK ... deflood memory cleanup, if is not needed anymore
				if(ServerManager::m_ui8SrCntr == 0) {
					if(curUser->m_sLastChat != NULL && curUser->m_ui16LastChatLines < 2 && 
                        (curUser->m_ui64SameChatsTick+SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MAIN_CHAT_TIME]) < ServerManager::m_ui64ActualTick) {
#ifdef _WIN32
                        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curUser->m_sLastChat) == 0) {
                            AppendDebugLog("%s - [MEM] Cannot deallocate curUser->m_sLastChat in ServiceLoop::ReceiveLoop\n");
                        }
#else
						free(curUser->m_sLastChat);
#endif
                        curUser->m_sLastChat = NULL;
						curUser->m_ui16LastChatLen = 0;
						curUser->m_ui16SameMultiChats = 0;
						curUser->m_ui16LastChatLines = 0;
                    }

					if(curUser->m_sLastPM != NULL && curUser->m_ui16LastPmLines < 2 && 
                        (curUser->m_ui64SamePMsTick+SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_PM_TIME]) < ServerManager::m_ui64ActualTick) {
#ifdef _WIN32
						if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curUser->m_sLastPM) == 0) {
							AppendDebugLog("%s - [MEM] Cannot deallocate curUser->m_sLastPM in ServiceLoop::ReceiveLoop\n");
                        }
#else
						free(curUser->m_sLastPM);
#endif
                        curUser->m_sLastPM = NULL;
                        curUser->m_ui16LastPMLen = 0;
						curUser->m_ui16SameMultiPms = 0;
                        curUser->m_ui16LastPmLines = 0;
                    }
                    
					if(curUser->m_sLastSearch != NULL && (curUser->m_ui64SameSearchsTick+SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_SEARCH_TIME]) < ServerManager::m_ui64ActualTick) {
#ifdef _WIN32
                        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)curUser->m_sLastSearch) == 0) {
							AppendDebugLog("%s - [MEM] Cannot deallocate curUser->m_sLastSearch in ServiceLoop::ReceiveLoop\n");
                        }
#else
						free(curUser->m_sLastSearch);
#endif
                        curUser->m_sLastSearch = NULL;
                        curUser->m_ui16LastSearchLen = 0;
                    }
                }
                continue;
            case User::STATE_CLOSING: {
                if(((curUser->m_ui32BoolBits & User::BIT_ERROR) == User::BIT_ERROR) == false && curUser->m_ui32SendBufDataLen != 0) {
                  	if(curUser->m_pLogInOut->m_ui32ToCloseLoops != 0 || ((curUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == true) {
                		curUser->Try2Send();
                		curUser->m_pLogInOut->m_ui32ToCloseLoops--;
                		continue;
                	}
                }
            	curUser->m_ui8State = User::STATE_REMME;
            	continue;
            }
            // if user is marked as dead, remove him
            case User::STATE_REMME: {
#ifdef _WIN32
                shutdown(curUser->m_Socket, SD_SEND);
                closesocket(curUser->m_Socket);
#else
                shutdown(curUser->m_Socket, SHUT_RD);
                close(curUser->m_Socket);
#endif

                // linked list
                Users::m_Ptr->RemUser(curUser);

                delete curUser;
                continue;
            }
            default: {
                // check logon timeout
                if(ServerManager::m_ui64ActualTick - curUser->m_pLogInOut->m_ui64LogonTick > 60) {
					UdpDebug::m_Ptr->BroadcastFormat("[SYS] Login timeout (%d) 2 for %s (%s) - user disconnected.", (int)curUser->m_ui8State, curUser->m_sNick, curUser->m_sIP);

                    curUser->Close();
                    continue;
                }
                break;
            }
        }
    }
    
    if(ServerManager::m_ui8SrCntr == 0) {
        Users::m_Ptr->m_ui16ActSearchs = 0;
        Users::m_Ptr->m_ui16PasSearchs = 0;
    }

	m_ui32LastRecvRest = iRecvRests;
	m_ui32RecvRestsPeak = iRecvRests > m_ui32RecvRestsPeak ? iRecvRests : m_ui32RecvRestsPeak;
}
//---------------------------------------------------------------------------

void ServiceLoop::SendLoop() {
    GlobalDataQueue::m_Ptr->PrepareQueueItems();

    // PPK ... send loop
    // now logging users get changed myinfo with myinfos
    // old users get it in this loop from queue -> badwith saving !!! no more twice myinfo =)
    // Sending Loop
    uint32_t iSendRests = 0;

    User * curUser = NULL,
        * nextUser = Users::m_Ptr->m_pUserListS;

    while(nextUser != 0 && ServerManager::m_bServerTerminated == false) {
        curUser = nextUser;
        nextUser = curUser->m_pNext;

        switch(curUser->m_ui8State) {
            case User::STATE_ADDME_2LOOP: {
                ServerManager::m_ui32Logged++;

                if(ServerManager::m_ui32Peak < ServerManager::m_ui32Logged) {
                    ServerManager::m_ui32Peak = ServerManager::m_ui32Logged;
                    if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_USERS_PEAK] < (int16_t)ServerManager::m_ui32Peak)
                        SettingManager::m_Ptr->SetShort(SETSHORT_MAX_USERS_PEAK, (int16_t)ServerManager::m_ui32Peak);
                }

            	curUser->m_ui8State = User::STATE_ADDED;

            	// finaly send the nicklist/myinfos/oplist
                curUser->AddUserList();
                
                // PPK ... UserIP2 supports
                if(((curUser->m_ui32SupportBits & User::SUPPORTBIT_USERIP2) == User::SUPPORTBIT_USERIP2) == true && ((curUser->m_ui32BoolBits & User::BIT_QUACK_SUPPORTS) == User::BIT_QUACK_SUPPORTS) == false && ProfileManager::m_Ptr->IsAllowed(curUser, ProfileManager::SENDALLUSERIP) == false) {
                    curUser->SendFormat("ServiceLoop::SendLoop->User::STATE_ADDME_2LOOP1", true, "$UserIP %s %s|", curUser->m_sNick, (curUser->m_sIPv4[0] == '\0' ? curUser->m_sIP : curUser->m_sIPv4));
                }
                
                curUser->m_ui32BoolBits &= ~User::BIT_GETNICKLIST;

                // PPK ... send motd ???
                if(SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_MOTD] != 0) {
                    if(SettingManager::m_Ptr->m_bBools[SETBOOL_MOTD_AS_PM] == true) {
                        curUser->SendFormat("ServiceLoop::SendLoop->User::STATE_ADDME_2LOOP2", true, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_MOTD], curUser->m_sNick);
                    } else {
                        curUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_MOTD], SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_MOTD]);
                    }
                }

                // check for Debug subscription
                if(((curUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true)
                    UdpDebug::m_Ptr->CheckUdpSub(curUser, true);

                if(curUser->m_pLogInOut->m_ui32UserConnectedLen != 0) {
                    curUser->PutInSendBuf(curUser->m_pLogInOut->m_pBuffer, curUser->m_pLogInOut->m_ui32UserConnectedLen);

                    curUser->FreeBuffer();
                }

                // Login struct no more needed, free mem ! ;)
                delete curUser->m_pLogInOut;
                curUser->m_pLogInOut = NULL;

            	// PPK ... send all login data from buffer !
            	if(curUser->m_ui32SendBufDataLen != 0) {
                    curUser->Try2Send();
                }
                    
                // apply one loop delay to avoid double sending of hello and oplist
            	continue;
            }
            case User::STATE_ADDED: {
                if(((curUser->m_ui32BoolBits & User::BIT_GETNICKLIST) == User::BIT_GETNICKLIST) == true) {
                    curUser->AddUserList();
                    curUser->m_ui32BoolBits &= ~User::BIT_GETNICKLIST;
                }

                // process global data queues
                if(GlobalDataQueue::m_Ptr->m_bHaveItems == true) {
                    GlobalDataQueue::m_Ptr->ProcessQueues(curUser);
                }
                
            	if(GlobalDataQueue::m_Ptr->m_pSingleItems != NULL) {
                    GlobalDataQueue::m_Ptr->ProcessSingleItems(curUser);
            	}

                // send data acumulated by queues above
                // if sending caused error, close the user
                if(curUser->m_ui32SendBufDataLen != 0) {
                    // PPK ... true = we have rest ;)
                	if(curUser->Try2Send() == true) {
                	    iSendRests++;
                	}
                }
                break;
            }
            case User::STATE_CLOSING:
            case User::STATE_REMME:
                continue;
            default:
                if(curUser->m_ui32SendBufDataLen != 0) {
                    curUser->Try2Send();
                }
                break;
        }
    }

    GlobalDataQueue::m_Ptr->ClearQueues();

    if(m_ui32LoopsForLogins >= 40) {
		m_dLoggedUsers = 0;
		m_ui32LoopsForLogins = 0;
		m_dActualSrvLoopLogins = 0;
    }

	m_dActualSrvLoopLogins += (double)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SIMULTANEOUS_LOGINS]/40;
	m_ui32LoopsForLogins++;

	m_ui32LastSendRest = iSendRests;
	m_ui32SendRestsPeak = iSendRests > m_ui32SendRestsPeak ? iSendRests : m_ui32SendRestsPeak;
}
//---------------------------------------------------------------------------

#ifdef _WIN32
void ServiceLoop::AcceptSocket(const SOCKET s, const sockaddr_storage &addr) {
#else
void ServiceLoop::AcceptSocket(const int s, const sockaddr_storage &addr) {
#endif
    AcceptedSocket * pNewSocket = new (std::nothrow) AcceptedSocket();
    if(pNewSocket == NULL) {
#ifdef _WIN32
		shutdown(s, SD_SEND);
		closesocket(s);
#else
		shutdown(s, SHUT_RDWR);
		close(s);
#endif

		AppendDebugLog("%s - [MEM] Cannot allocate pNewSocket in ServiceLoop::AcceptSocket\n");
    	return;
    }

    pNewSocket->m_Socket = s;

    memcpy(&pNewSocket->m_Addr, &addr, sizeof(sockaddr_storage));

    pNewSocket->m_pNext = NULL;

#ifdef _WIN32
    EnterCriticalSection(&m_csAcceptQueue);
#else
    pthread_mutex_lock(&m_mtxAcceptQueue);
#endif

    if(m_pAcceptedSocketsS == NULL) {
		m_pAcceptedSocketsS = pNewSocket;
		m_pAcceptedSocketsE = pNewSocket;
    } else {
		m_pAcceptedSocketsE->m_pNext = pNewSocket;
		m_pAcceptedSocketsE = pNewSocket;
    }

#ifdef _WIN32
    LeaveCriticalSection(&m_csAcceptQueue);
#else
	pthread_mutex_unlock(&m_mtxAcceptQueue);
#endif
}
//---------------------------------------------------------------------------
