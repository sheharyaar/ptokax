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
#include "ServerManager.h"
#include "SettingManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "RegThread.h"
//---------------------------------------------------------------------------
RegisterThread * RegisterThread::m_Ptr = NULL;
//---------------------------------------------------------------------------

RegisterThread::RegSocket::RegSocket() : m_ui64TotalShare(0), m_pPrev(NULL), m_pNext(NULL), m_sAddress(NULL), m_pRecvBuf(NULL), m_pSendBuf(NULL), m_pSendBufHead(NULL),
#ifdef _WIN32
    m_Sock(INVALID_SOCKET), 
#else
    m_Sock(-1),
#endif
	m_ui32RecvBufLen(0), m_ui32RecvBufSize(0), m_ui32SendBufLen(0), m_ui32TotalUsers(0), m_ui32AddrLen(0)
	{
	// ...
}
//---------------------------------------------------------------------------

RegisterThread::RegSocket::~RegSocket() {
    free(m_sAddress);
    free(m_pRecvBuf);
    free(m_pSendBuf);

#ifdef _WIN32
    shutdown(m_Sock, SD_SEND);
    closesocket(m_Sock);
#else
    shutdown(m_Sock, SHUT_RDWR);
    close(m_Sock);
#endif
}
//---------------------------------------------------------------------------

RegisterThread::RegisterThread() : m_pRegSockListS(NULL), m_pRegSockListE(NULL), m_bTerminated(false), m_ui32BytesRead(0), m_ui32BytesSent(0) {
    m_sMsg[0] = '\0';

#ifdef _WIN32
    m_hThreadHandle = NULL;
#else
	m_ThreadId = 0;
#endif
}
//---------------------------------------------------------------------------

RegisterThread::~RegisterThread() {
#ifndef _WIN32
    if(m_ThreadId != 0) {
        Close();
        WaitFor();
    }
#endif

	ServerManager::m_ui64BytesRead += (uint64_t)m_ui32BytesRead;
    ServerManager::m_ui64BytesSent += (uint64_t)m_ui32BytesSent;
    
    RegSocket * cur = NULL,
        * next = m_pRegSockListS;
        
    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;
                       
        delete cur;
    }

#ifdef _WIN32
    if(m_hThreadHandle != NULL) {
        CloseHandle(m_hThreadHandle);
    }
#endif
}
//---------------------------------------------------------------------------

void RegisterThread::Setup(char * sListAddresses, const uint16_t ui16AddrsLen) {
    // parse all addresses and create individul RegSockets
    char *sAddresses = (char *)malloc(ui16AddrsLen+1);
    if(sAddresses == NULL) {
        AppendDebugLogFormat("[MEM] Cannot allocate %hu bytes for sAddresses in RegisterThread::Setup\n", ui16AddrsLen+1);

        return;
    }

    memcpy(sAddresses, sListAddresses, ui16AddrsLen);
    sAddresses[ui16AddrsLen] = '\0';

    char *sAddress = sAddresses;

    for(uint16_t ui16i = 0; ui16i < ui16AddrsLen; ui16i++) {
        if(sAddresses[ui16i] == ';') {
            sAddresses[ui16i] = '\0';
        } else if(ui16i != ui16AddrsLen-1) {
            continue;
        } else {
            ui16i++;
        }

        AddSock(sAddress, (sAddresses+ui16i)-sAddress);
        sAddress = sAddresses+ui16i+1;
    }

	free(sAddresses);
}
//---------------------------------------------------------------------------

void RegisterThread::AddSock(char * sAddress, const size_t szLen) {
    RegSocket * pNewSock = new (std::nothrow) RegSocket();
    if(pNewSock == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewSock in RegisterThread::AddSock\n");
    	return;
    }

    pNewSock->m_pPrev = NULL;
    pNewSock->m_pNext = NULL;

    pNewSock->m_sAddress = (char *)malloc(szLen+1);
    if(pNewSock->m_sAddress == NULL) {
		delete pNewSock;

		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for sAddress in RegisterThread::AddSock\n", szLen+1);

        return;
    }

    pNewSock->m_ui32AddrLen = (uint32_t)szLen;

    memcpy(pNewSock->m_sAddress, sAddress, pNewSock->m_ui32AddrLen);
    pNewSock->m_sAddress[pNewSock->m_ui32AddrLen] = '\0';
    
    pNewSock->m_pRecvBuf = (char *)malloc(256);
    if(pNewSock->m_pRecvBuf == NULL) {
        delete pNewSock;

		AppendDebugLog("%s - [MEM] Cannot allocate 256 bytes for sRecvBuf in RegisterThread::AddSock\n");
        return;
    }

    pNewSock->m_pSendBuf = NULL;
    pNewSock->m_pSendBufHead = NULL;

    pNewSock->m_ui32RecvBufLen = 0;
    pNewSock->m_ui32RecvBufSize = 255;
    pNewSock->m_ui32SendBufLen = 0;

    pNewSock->m_ui32TotalUsers = 0;
    pNewSock->m_ui64TotalShare = 0;

    if(m_pRegSockListS == NULL) {
        pNewSock->m_pPrev = NULL;
        pNewSock->m_pNext = NULL;
		m_pRegSockListS = pNewSock;
		m_pRegSockListE = pNewSock;
    } else {
        pNewSock->m_pPrev = m_pRegSockListE;
        pNewSock->m_pNext = NULL;
		m_pRegSockListE->m_pNext = pNewSock;
		m_pRegSockListE = pNewSock;
    }
}
//---------------------------------------------------------------------------

#ifdef _WIN32
	unsigned __stdcall ExecuteRegisterThread(void * pThread) {
#else
	static void* ExecuteRegisterThread(void * pThread) {
#endif
	(reinterpret_cast<RegisterThread *>(pThread))->Run();

	return 0;
}
//---------------------------------------------------------------------------

void RegisterThread::Resume() {
#ifdef _WIN32
    m_hThreadHandle = (HANDLE)_beginthreadex(NULL, 0, ExecuteRegisterThread, this, 0, NULL);
    if(m_hThreadHandle == 0) {
#else
    int iRet = pthread_create(&m_ThreadId, NULL, ExecuteRegisterThread, this);
    if(iRet != 0) {
#endif
		AppendDebugLog("%s - [ERR] Failed to create new RegisterThread\n");
    }
}
//---------------------------------------------------------------------------

void RegisterThread::Run() {
#ifndef _WIN32
    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 1000000;
#endif

    RegSocket * cur = NULL,
        * next = m_pRegSockListS;

    while(m_bTerminated == false && next != NULL) {
        cur = next;
        next = cur->m_pNext;

        char *port = strchr(cur->m_sAddress, ':');
        if(port != NULL) {
            port[0] = '\0';
            int32_t iPort = atoi(port+1);

            if(iPort >= 0 && iPort <= 65535) {
                port[0] = ':';
                port = NULL;
            }
        }

        struct addrinfo hints;
        memset(&hints, 0, sizeof(addrinfo));

        hints.ai_socktype = SOCK_STREAM;

        if(ServerManager::m_bUseIPv6 == true) {
            hints.ai_family = AF_UNSPEC;
        } else {
            hints.ai_family = AF_INET;
        }

        struct addrinfo * pResult = NULL;

        if(::getaddrinfo(cur->m_sAddress, port != NULL ? port+1 : "2501", &hints, &pResult) != 0 || (pResult->ai_family != AF_INET && pResult->ai_family != AF_INET6)) {
#ifdef _WIN32
			int iError = WSAGetLastError();
			int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock resolve error %s (%d). (%s)", WSErrorStr(iError), iError, cur->m_sAddress);
#else
			int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock resolve error (%s)", cur->m_sAddress);
#endif
            if(iMsgLen > 0) {
                EventQueue::m_Ptr->AddThread(EventQueue::EVENT_REGSOCK_MSG, m_sMsg);
            }

            RemoveSock(cur);
            delete cur;

            if(pResult != NULL) {
                freeaddrinfo(pResult);
            }

            continue;
        }

        if(port != NULL) {
            port[0] = ':';
        }

        // Initialise socket we want to use for connect
#ifdef _WIN32
        if((cur->m_Sock = socket(pResult->ai_family, pResult->ai_socktype, pResult->ai_protocol)) == INVALID_SOCKET) {
            int iError = WSAGetLastError();
            int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock create error %s (%d). (%s)", WSErrorStr(iError), iError, cur->m_sAddress);
#else
        if((cur->m_Sock = socket(pResult->ai_family, pResult->ai_socktype, pResult->ai_protocol)) == -1) {
            int iMsgLen = snprintf(m_sMsg, 2048,  "[REG] RegSock create error %d. (%s)", errno, cur->m_sAddress);
#endif
            if(iMsgLen > 0) {
                EventQueue::m_Ptr->AddThread(EventQueue::EVENT_REGSOCK_MSG, m_sMsg);
            }

            RemoveSock(cur);
            delete cur;
            freeaddrinfo(pResult);

            return;
        }

        // Set the receive buffer
#ifdef _WIN32
        int32_t bufsize = 1024;
        if(setsockopt(cur->m_Sock, SOL_SOCKET, SO_RCVBUF, (char *) &bufsize, sizeof(bufsize)) == SOCKET_ERROR) {
            int iError = WSAGetLastError();
            int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock recv buff error %s (%d). (%s)", WSErrorStr(iError), iError, cur->m_sAddress);
#else
        int bufsize = 1024;
        if(setsockopt(cur->m_Sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) == -1) {
            int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock recv buff error %d. (%s)", errno, cur->m_sAddress);
#endif
            if(iMsgLen > 0) {
                EventQueue::m_Ptr->AddThread(EventQueue::EVENT_REGSOCK_MSG, m_sMsg);
            }

            RemoveSock(cur);
            delete cur;
            freeaddrinfo(pResult);

            return;
        }

        // Set the send buffer
        bufsize = 2048;
#ifdef _WIN32
        if(setsockopt(cur->m_Sock, SOL_SOCKET, SO_SNDBUF, (char *) &bufsize, sizeof(bufsize)) == SOCKET_ERROR) {
            int iError = WSAGetLastError();
            int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock send buff error %s (%d). (%s)", WSErrorStr(iError), iError, cur->m_sAddress);
#else
        if(setsockopt(cur->m_Sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) == -1) {
            int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock send buff error %d. (%s)", errno, cur->m_sAddress);
#endif
            if(iMsgLen > 0) {
                EventQueue::m_Ptr->AddThread(EventQueue::EVENT_REGSOCK_MSG, m_sMsg);
            }

            RemoveSock(cur);
            delete cur;
            freeaddrinfo(pResult);

            return;
        }

        // Set non-blocking mode
#ifdef _WIN32
        uint32_t block = 1;
        if(ioctlsocket(cur->m_Sock, FIONBIO, (unsigned long *)&block) == SOCKET_ERROR) {
            int iError = WSAGetLastError();
            int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock non-block error %s (%d). (%s)", WSErrorStr(iError), iError, cur->m_sAddress);
#else
        int oldFlag = fcntl(cur->m_Sock, F_GETFL, 0);
        if(fcntl(cur->m_Sock, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
            int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock non-block error %d. (%s)", errno, cur->m_sAddress);
#endif
            if(iMsgLen > 0) {
                EventQueue::m_Ptr->AddThread(EventQueue::EVENT_REGSOCK_MSG, m_sMsg);
            }

            RemoveSock(cur);
            delete cur;
            freeaddrinfo(pResult);

            return;
        }

        // Finally, time to connect ! ;)
#ifdef _WIN32
		if(connect(cur->m_Sock, pResult->ai_addr, (int)pResult->ai_addrlen) == SOCKET_ERROR) {
			int iError = WSAGetLastError();
			if(iError != WSAEWOULDBLOCK) {
				int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock connect error %s (%d). (%s)", WSErrorStr(iError), iError, cur->m_sAddress);
#else  
		if(connect(cur->m_Sock, pResult->ai_addr, (int)pResult->ai_addrlen) == -1) {
			if(errno != EINPROGRESS) {
				int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock connect error %s (%d). (%s)", ErrnoStr(errno), errno, cur->m_sAddress);
#endif
                if(iMsgLen > 0) {
                    EventQueue::m_Ptr->AddThread(EventQueue::EVENT_REGSOCK_MSG, m_sMsg);
                }

                RemoveSock(cur);
                delete cur;
                ::freeaddrinfo(pResult);

                continue;
            }
        }

        ::freeaddrinfo(pResult);

#ifdef _WIN32
		::Sleep(1);
#else
        nanosleep(&sleeptime, NULL);
#endif
	}

	uint16_t iLoops = 0;

#ifndef _WIN32
    sleeptime.tv_nsec = 75000000;
#endif

    while(m_pRegSockListS != NULL && m_bTerminated == false && iLoops < 4000) {   
        iLoops++;
  
        next = m_pRegSockListS;
     
        while(next != NULL) {
            cur = next;
            next = cur->m_pNext;

            if(Receive(cur) == false) {
                RemoveSock(cur);
                delete cur;
                continue;
            }

            if(cur->m_ui32SendBufLen > 0) {
                if(Send(cur) == false) {
                    RemoveSock(cur);
                    delete cur;
                    continue;
                }
            }
        }
#ifdef _WIN32
		::Sleep(75);
#else
        nanosleep(&sleeptime, NULL);
#endif
	}

    next = m_pRegSockListS;
    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(m_bTerminated == false) {
            int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock timeout. (%s)", cur->m_sAddress);
            if(iMsgLen > 0) {
                EventQueue::m_Ptr->AddThread(EventQueue::EVENT_REGSOCK_MSG, m_sMsg);
            }
        }

        delete cur;
    }

	m_pRegSockListS = NULL;
	m_pRegSockListE = NULL;
}
//---------------------------------------------------------------------------

void RegisterThread::Close() {
	m_bTerminated = true;
}
//---------------------------------------------------------------------------

void RegisterThread::WaitFor() {
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

bool RegisterThread::Receive(RegSocket * pSock) {
    if(pSock->m_pRecvBuf == NULL) {
        return true;
    }

#ifdef _WIN32
	u_long iAvailBytes = 0;
	if(ioctlsocket(pSock->m_Sock, FIONREAD, &iAvailBytes) == SOCKET_ERROR) {
		int iError = WSAGetLastError();
	    int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock ioctlsocket(FIONREAD) error %s (%d). (%s)", WSErrorStr(iError), iError, pSock->m_sAddress);
#else
	int iAvailBytes = 0;
	if(ioctl(pSock->m_Sock, FIONREAD, &iAvailBytes) == -1) {
	    int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock ioctl(FIONREAD) error %s (%d). (%s)", ErrnoStr(errno), errno, pSock->m_sAddress);
#endif
        if(iMsgLen > 0) {
            EventQueue::m_Ptr->AddThread(EventQueue::EVENT_REGSOCK_MSG, m_sMsg);
        }

        return false;
    }

    if(iAvailBytes == 0) {
        // we need to ry receive to catch connection error, or if user closed connection
        iAvailBytes = 16;
    } else if(iAvailBytes > 1024) {
        // receive max. 1024 bytes to receive buffer
        iAvailBytes = 1024;
    }

    if(pSock->m_ui32RecvBufSize < pSock->m_ui32RecvBufLen+iAvailBytes) {
        size_t szAllignLen = ((pSock->m_ui32RecvBufLen+iAvailBytes+1) & 0xFFFFFE00) + 0x200;
        if(szAllignLen > 2048) {
            int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock receive buffer overflow. (%s)", pSock->m_sAddress);
            if(iMsgLen > 0) {
                EventQueue::m_Ptr->AddThread(EventQueue::EVENT_REGSOCK_MSG, m_sMsg);
            }

            return false;
        }

        char * oldbuf = pSock->m_pRecvBuf;

        pSock->m_pRecvBuf = (char *)realloc(oldbuf, szAllignLen);
        if(pSock->m_pRecvBuf == NULL) {
            free(oldbuf);

			AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes for sRecvBuf in RegisterThread::Receive\n", szAllignLen);

            return false;
        }

		pSock->m_ui32RecvBufSize = (uint32_t)(szAllignLen-1);
    }

    int iBytes = recv(pSock->m_Sock, pSock->m_pRecvBuf+pSock->m_ui32RecvBufLen, pSock->m_ui32RecvBufSize-pSock->m_ui32RecvBufLen, 0);
    
#ifdef _WIN32
    if(iBytes == SOCKET_ERROR) {
        int iError = WSAGetLastError();
        if(iError != WSAEWOULDBLOCK) {                  
			if(iError == WSAENOTCONN) {
#else
    if(iBytes == -1) {
        if(errno != EAGAIN) {
			if(errno == ENOTCONN) {
#endif
				int iErr = 0;
				socklen_t iErrLen = sizeof(iErr);

            	int iRet = getsockopt(pSock->m_Sock, SOL_SOCKET, SO_ERROR, (char *) &iErr, &iErrLen);
#ifdef _WIN32
				if(iRet == SOCKET_ERROR) {
					int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock getsockopt error %s (%d). (%s)", WSErrorStr(iRet), iRet, pSock->m_sAddress);
#else
				if(iRet == -1) {
					int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock getsockopt error %d. (%s)", iRet, pSock->m_sAddress);
#endif
					if(iMsgLen > 0) {
						EventQueue::m_Ptr->AddThread(EventQueue::EVENT_REGSOCK_MSG, m_sMsg);
					}

					return false;
				} else if(iErr != 0) {
#ifdef _WIN32
					int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock connect error %s (%d). (%s)", WSErrorStr(iErr), iErr, pSock->m_sAddress);
#else
					int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock connect error %d. (%s)", iErr, pSock->m_sAddress);
#endif
					if(iMsgLen > 0) {
						EventQueue::m_Ptr->AddThread(EventQueue::EVENT_REGSOCK_MSG, m_sMsg);
					}

					return false;
				}
				return true;
			}

#ifdef _WIN32
            int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock recv error %s (%d). (%s)", WSErrorStr(iError), iError, pSock->m_sAddress);
#else
			int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock recv error %s (%d). (%s)", ErrnoStr(errno), errno, pSock->m_sAddress);
#endif
            if(iMsgLen > 0) {
                EventQueue::m_Ptr->AddThread(EventQueue::EVENT_REGSOCK_MSG, m_sMsg);
            }

            return false;
        } else {
            return true;
        }
    } else if(iBytes == 0) {
        int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock closed connection by server. (%s)", pSock->m_sAddress);
        if(iMsgLen > 0) {
            EventQueue::m_Ptr->AddThread(EventQueue::EVENT_REGSOCK_MSG, m_sMsg);
        }

        return false;
    }
    
	m_ui32BytesRead += iBytes;

    pSock->m_ui32RecvBufLen += iBytes;
    pSock->m_pRecvBuf[pSock->m_ui32RecvBufLen] = '\0';
    char *sBuffer = pSock->m_pRecvBuf;

    for(uint32_t ui32i = 0; ui32i < pSock->m_ui32RecvBufLen; ui32i++) {
        if(pSock->m_pRecvBuf[ui32i] == '|') {
            uint32_t ui32CommandLen = (uint32_t)(((pSock->m_pRecvBuf+ui32i)-sBuffer)+1);
            if(strncmp(sBuffer, "$Lock ", 6) == 0) {
                sockaddr_in addr;
				socklen_t addrlen = sizeof(addr);
#ifdef _WIN32
                if(getsockname(pSock->m_Sock, (struct sockaddr *) &addr, &addrlen) == SOCKET_ERROR) {
                    int iError = WSAGetLastError();
                    int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock local port error %s (%d). (%s)", WSErrorStr(iError), iError, pSock->m_sAddress);
#else
                if(getsockname(pSock->m_Sock, (struct sockaddr *) &addr, &addrlen) == -1) {
                    int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock local port error %d. (%s)", errno, pSock->m_sAddress);
#endif
                    if(iMsgLen > 0) {
                        EventQueue::m_Ptr->AddThread(EventQueue::EVENT_REGSOCK_MSG, m_sMsg);
                    }

                    return false;
                }

                uint16_t ui16port =  (uint16_t)ntohs(addr.sin_port);
                char cMagic = (char) ((ui16port&0xFF)+((ui16port>>8)&0xFF));
				size_t szLen = ui32CommandLen;

                // strip the Lock data
                pSock->m_pRecvBuf[ui32i] = '\0';
                char * pTemp = strchr(sBuffer+6, ' ');
                if(pTemp != NULL) {
                    pTemp[0] = '\0';
                    szLen = pTemp-sBuffer;
                }
                pSock->m_pRecvBuf[ui32i] = '|';

                // Compute the key
                memcpy(m_sMsg, "$Key ", 5);
				m_sMsg[5] = '\0';
                char v;

                // first make the crypting stuff
                for(size_t szi = 6; szi < szLen; szi++) {
                    if(szi == 6) {
                        v = sBuffer[szi] ^ sBuffer[szLen] ^ sBuffer[szLen-1] ^ cMagic;
                    } else {
                        v = sBuffer[szi] ^ sBuffer[szi-1];
                    }

                    // Swap nibbles (0xF0 = 11110000, 0x0F = 00001111)
                    v = (char)(((v << 4) & 0xF0) | ((v >> 4) & 0x0F));

                    switch(v) {
                        case 0:
                            strcat(m_sMsg, "/%DCN000%/");
                            break;
                        case 5:
                            strcat(m_sMsg, "/%DCN005%/");
                            break;
                        case 36:
                            strcat(m_sMsg, "/%DCN036%/");
                            break;
                        case 96:
                            strcat(m_sMsg, "/%DCN096%/");
                            break;
                        case 124:
                            strcat(m_sMsg, "/%DCN124%/");
                            break;
                        case 126:
                            strcat(m_sMsg, "/%DCN126%/");
                            break;
                        default:
                            strncat(m_sMsg, (char *)&v, 1);
                            break;
                    }
                }

                pSock->m_ui32TotalUsers = ServerManager::m_ui32Logged;
                pSock->m_ui64TotalShare = ServerManager::m_ui64TotalShare;

                strcat(m_sMsg, "|");
                SettingManager::m_Ptr->GetText(SETTXT_HUB_NAME, m_sMsg);
                strcat(m_sMsg, "|");
                SettingManager::m_Ptr->GetText(SETTXT_HUB_ADDRESS, m_sMsg);
                uint16_t ui16FirstPort = SettingManager::m_Ptr->GetFirstPort();
                if(ui16FirstPort != 411) {
                    strcat(m_sMsg, ":");
                    strcat(m_sMsg, string(ui16FirstPort).c_str());
                }
                strcat(m_sMsg, "|");
                SettingManager::m_Ptr->GetText(SETTXT_HUB_DESCRIPTION, m_sMsg);
                strcat(m_sMsg, "     |");
                szLen = strlen(m_sMsg);
                *(m_sMsg+szLen-4) = 'x';
                *(m_sMsg+szLen-3) = '.';
                *(m_sMsg+szLen-5) = 'p';
                *(m_sMsg+szLen-6) = '.';
                if(SettingManager::m_Ptr->GetBool(SETBOOL_ANTI_MOGLO) == true) {
                    *(m_sMsg+szLen-2) = (char)160;
                }
                strcat(m_sMsg, string(pSock->m_ui32TotalUsers).c_str());
                strcat(m_sMsg, "|");
                strcat(m_sMsg, string(pSock->m_ui64TotalShare).c_str());
                strcat(m_sMsg, "|");
                Add2SendBuf(pSock, m_sMsg);

                free(pSock->m_pRecvBuf);
                pSock->m_pRecvBuf = NULL;
                pSock->m_ui32RecvBufLen = 0;
                pSock->m_ui32RecvBufSize = 0;
                return true;
            }
            sBuffer += ui32CommandLen;
        }
    }

    pSock->m_ui32RecvBufLen -= (uint32_t)(sBuffer-pSock->m_pRecvBuf);

    if(pSock->m_ui32RecvBufLen == 0) {
        pSock->m_pRecvBuf[0] = '\0';
    } else if(pSock->m_ui32RecvBufLen != 1) {
        memmove(pSock->m_pRecvBuf, sBuffer, pSock->m_ui32RecvBufLen);
        pSock->m_pRecvBuf[pSock->m_ui32RecvBufLen] = '\0';
    } else {
        if(sBuffer[0] == '|') {
            pSock->m_pRecvBuf[0] = '\0';
            pSock->m_ui32RecvBufLen = 0;
        } else {
            pSock->m_pRecvBuf[0] = sBuffer[0];
            pSock->m_pRecvBuf[1] = '\0';
        }
    }
    return true;
}
//---------------------------------------------------------------------------

void RegisterThread::Add2SendBuf(RegSocket * pSock, char * sData) {
    size_t szLen = strlen(sData);
    
    pSock->m_pSendBuf = (char *)malloc(szLen+1);
    if(pSock->m_pSendBuf == NULL) {
        AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for sSendBuf in RegisterThread::Add2SendBuf\n", szLen+1);

        return;
    }

    pSock->m_pSendBufHead = pSock->m_pSendBuf;

    memcpy(pSock->m_pSendBuf, sData, szLen);
    pSock->m_ui32SendBufLen = (uint32_t)szLen;
    pSock->m_pSendBuf[pSock->m_ui32SendBufLen] = '\0';
}
//---------------------------------------------------------------------------

bool RegisterThread::Send(RegSocket * pSock) {
    // compute length of unsent data
    uint32_t iOffset = (uint32_t)(pSock->m_pSendBufHead - pSock->m_pSendBuf);
#ifdef _WIN32
    int32_t iLen = pSock->m_ui32SendBufLen - iOffset;
#else
	int iLen = pSock->m_ui32SendBufLen - iOffset;
#endif

    int iBytes = send(pSock->m_Sock, pSock->m_pSendBufHead, iLen, 0);

#ifdef _WIN32
    if(iBytes == SOCKET_ERROR) {
    	int iError = WSAGetLastError();
        if(iError != WSAEWOULDBLOCK) {
            int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock send error %s (%d). (%s)", WSErrorStr(iError), iError, pSock->m_sAddress);
#else
    if(iBytes == -1) {
        if(errno != EAGAIN) {
            int iMsgLen = snprintf(m_sMsg, 2048, "[REG] RegSock send error %s (%d). (%s)", ErrnoStr(errno), errno, pSock->m_sAddress);
#endif
            if(iMsgLen > 0) {
                EventQueue::m_Ptr->AddThread(EventQueue::EVENT_REGSOCK_MSG, m_sMsg);
            }
            return false;
        } else {
            return true;
        }
    }

	m_ui32BytesSent += iBytes;

	if(iBytes < iLen) {
        pSock->m_pSendBufHead += iBytes;
		return true;
	} else {
		int iMsgLen = snprintf(m_sMsg, 2048, "[REG] Hub is registered on %s hublist (Users: %u, Share: %" PRIu64 ")", pSock->m_sAddress, ServerManager::m_ui32Logged, ServerManager::m_ui64TotalShare);
        if(iMsgLen > 0) {
            EventQueue::m_Ptr->AddThread(EventQueue::EVENT_REGSOCK_MSG, m_sMsg);
        }

        // Here is only one send, and when is all send then is time to delete this sock... false do it for us ;)
		return false;
	}
}
//---------------------------------------------------------------------------

void RegisterThread::RemoveSock(RegSocket * pSock) {
    if(pSock->m_pPrev == NULL) {
        if(pSock->m_pNext == NULL) {
			m_pRegSockListS = NULL;
			m_pRegSockListE = NULL;
        } else {
            pSock->m_pNext->m_pPrev = NULL;
			m_pRegSockListS = pSock->m_pNext;
        }
    } else if(pSock->m_pNext == NULL) {
        pSock->m_pPrev->m_pNext = NULL;
		m_pRegSockListE = pSock->m_pPrev;
	} else {
		pSock->m_pPrev->m_pNext = pSock->m_pNext;
		pSock->m_pNext->m_pPrev = pSock->m_pPrev;
	}
}
//---------------------------------------------------------------------------
