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
#include "ServerManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "UpdateCheckThread.h"
//---------------------------------------------------------------------------
#include "../gui.win/MainWindow.h"
//---------------------------------------------------------------------------
UpdateCheckThread * UpdateCheckThread::m_Ptr = NULL;
//---------------------------------------------------------------------------

UpdateCheckThread::UpdateCheckThread() : m_hThread(NULL), m_sRecvBuf(NULL), m_Socket(INVALID_SOCKET), m_ui32FileLen(0), m_ui32RecvBufLen(0), m_ui32RecvBufSize(0), m_ui32BytesRead(0), m_ui32BytesSent(0), m_bOk(false), m_bData(false), m_bTerminated(false) {
    m_sMsg[0] = '\0';
}
//---------------------------------------------------------------------------

UpdateCheckThread::~UpdateCheckThread() {
	ServerManager::m_ui64BytesRead += (uint64_t)m_ui32BytesRead;
    ServerManager::m_ui64BytesSent += (uint64_t)m_ui32BytesSent;

    if(m_Socket != INVALID_SOCKET) {
#ifdef _WIN32
        shutdown(m_Socket, SD_SEND);
		closesocket(m_Socket);
#else
        shutdown(m_Socket, 1);
        close(m_Socket);
#endif
    }

    free(m_sRecvBuf);

    if(m_hThread != NULL) {
        ::CloseHandle(m_hThread);
    }
}
//---------------------------------------------------------------------------

unsigned __stdcall ExecuteUpdateCheck(void * /*pArguments*/) {
	UpdateCheckThread::m_Ptr->Run();

	return 0;
}
//---------------------------------------------------------------------------

void UpdateCheckThread::Resume() {
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, ExecuteUpdateCheck, NULL, 0, NULL);
	if(m_hThread == 0) {
		AppendDebugLog("%s - [ERR] Failed to create new UpdateCheckThread\n");
    }
}
//---------------------------------------------------------------------------

void UpdateCheckThread::Run() {
    struct addrinfo * pResult = NULL;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_socktype = SOCK_STREAM;

    if(ServerManager::m_bUseIPv6 == true) {
        hints.ai_family = AF_UNSPEC;
    } else {
        hints.ai_family = AF_INET;
    }

    if(::getaddrinfo("www.PtokaX.org", "80", &hints, &pResult) != 0 || (pResult->ai_family != AF_INET && pResult->ai_family != AF_INET6)) {
        int iError = WSAGetLastError();
        int iMsgLen = snprintf(m_sMsg, 2048, "Update check resolve error %s (%d).", WSErrorStr(iError), iError);
        if(iMsgLen > 0) {
            Message(m_sMsg, iMsgLen);
        }

        ::PostMessage(MainWindow::m_Ptr->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

        if(pResult != NULL) {
            ::freeaddrinfo(pResult);
        }

        return;
    }

    // Initialise socket we want to use for connect
#ifdef _WIN32
    if((m_Socket = socket(pResult->ai_family, pResult->ai_socktype, pResult->ai_protocol)) == INVALID_SOCKET) {
        int iError = WSAGetLastError();
        int iMsgLen = snprintf(m_sMsg, 2048, "Update check create error %s (%d).", WSErrorStr(iError), iError);
#else
    if((m_Socket = socket(pResult->ai_family, pResult->ai_socktype, pResult->ai_protocol)) == -1) {
        int iMsgLen = snprintf(m_sMsg, 2048, "Update check create error %s (%d).", WSErrorStr(errno), errno);
#endif
        if(iMsgLen > 0) {
            Message(m_sMsg, iMsgLen);
        }

        ::PostMessage(MainWindow::m_Ptr->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

        ::freeaddrinfo(pResult);

        return;
    }

    // Set the receive buffer
    int32_t bufsize = 8192;
#ifdef _WIN32
    if(setsockopt(m_Socket, SOL_SOCKET, SO_RCVBUF, (char *) &bufsize, sizeof(bufsize)) == SOCKET_ERROR) {
        int iError = WSAGetLastError();
        int iMsgLen = snprintf(m_sMsg, 2048, "Update check recv buff error %s (%d).", WSErrorStr(iError), iError);
#else
    if(setsockopt(m_Socket, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) == -1) {
        int iMsgLen = snprintf(m_sMsg, 2048, "Update check recv buff error %s (%d).", WSErrorStr(errno), errno);
#endif
		if(iMsgLen > 0) {
			Message(m_sMsg, iMsgLen);
		}

		::PostMessage(MainWindow::m_Ptr->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

        ::freeaddrinfo(pResult);

        return;
	}

	// Set the send buffer
	bufsize = 2048;
#ifdef _WIN32
	if(setsockopt(m_Socket, SOL_SOCKET, SO_SNDBUF, (char *) &bufsize, sizeof(bufsize)) == SOCKET_ERROR) {
		int iError = WSAGetLastError();
		int iMsgLen = snprintf(m_sMsg, 2048, "Update check send buff error %s (%d).", WSErrorStr(iError), iError);
#else
	if(setsockopt(m_Socket, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) == -1) {
        int iMsgLen = snprintf(m_sMsg, 2048, "Update check buff error %s (%d).", WSErrorStr(errno), errno);
#endif
        if(iMsgLen > 0) {
            Message(m_sMsg, iMsgLen);
		}
			
		::PostMessage(MainWindow::m_Ptr->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

        ::freeaddrinfo(pResult);

        return;
    }

	Message("Connecting to PtokaX.org ...", 28);

    // Finally, time to connect ! ;)
#ifdef _WIN32
    if(connect(m_Socket, pResult->ai_addr, (int)pResult->ai_addrlen) == SOCKET_ERROR) {
        int iError = WSAGetLastError();
        if(iError != WSAEWOULDBLOCK) {
            int iMsgLen = snprintf(m_sMsg, 2048, "Update check connect error %s (%d).", WSErrorStr(iError), iError);
#else
    if(connect(m_Socket, pResult->ai_addr, (int)pResult->ai_addrlen) == -1) {
        if(errno != EAGAIN) {
            int iMsgLen = snprintf(m_sMsg, 2048, "Update check connect error %s (%d).", WSErrorStr(errno), errno);
#endif
            if(iMsgLen > 0) {
                Message(m_sMsg, iMsgLen);
            }

			::PostMessage(MainWindow::m_Ptr->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

            ::freeaddrinfo(pResult);

            return;
        }
    }

    ::freeaddrinfo(pResult);

	Message("Connected to PtokaX.org, sending request...", 43);

    if(SendHeader() == false) {
        ::PostMessage(MainWindow::m_Ptr->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

        return;
    }

	Message("Request to PtokaX.org sent, receiving data...", 45);

    // Set non-blocking mode
#ifdef _WIN32
    uint32_t block = 1;
    if(ioctlsocket(m_Socket, FIONBIO, (unsigned long *)&block) == SOCKET_ERROR) {
        int iError = WSAGetLastError();
        int iMsgLen = snprintf(m_sMsg, 2048, "Update check non-block error %s (%d).", WSErrorStr(iError), iError);
#else
    int32_t oldFlag = fcntl(u->s, F_GETFL, 0);
    if(fcntl(m_Socket, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
        int iMsgLen = snprintf(m_sMsg, 2048, "Update check non-block error %s (%d).", WSErrorStr(errno), errno);
#endif
        if(iMsgLen > 0) {
            Message(m_sMsg, iMsgLen);
		}

		::PostMessage(MainWindow::m_Ptr->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

        return;
    }

	m_sRecvBuf = (char *)malloc(512);
    if(m_sRecvBuf == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate 512 bytes for sRecvBuf in UpdateCheckThread::Run\n");

		::PostMessage(MainWindow::m_Ptr->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

        return;
    }

    uint16_t iLoops = 0;

    while(m_bTerminated == false && iLoops < 4000) {
        iLoops++;

		if(Receive() == false) {
			::PostMessage(MainWindow::m_Ptr->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

			return;
        }

        ::Sleep(75);
    }

    if(m_bTerminated == false) {
        Message("Update check timeout.", 21);

		::PostMessage(MainWindow::m_Ptr->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);
    }
}
//---------------------------------------------------------------------------

void UpdateCheckThread::Close() {
	m_bTerminated = true;
}
//---------------------------------------------------------------------------

void UpdateCheckThread::WaitFor() {
    ::WaitForSingleObject(m_hThread, INFINITE);
}
//---------------------------------------------------------------------------

void UpdateCheckThread::Message(char * sMessage, const size_t szLen) {
	char * sMess = (char *)malloc(szLen + 1);
	if(sMess == NULL) {
		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for sMess in UpdateCheckThread::Message\n", szLen+1);

		return;
	}

	memcpy(sMess, sMessage, szLen);
	sMess[szLen] = '\0';

	::PostMessage(MainWindow::m_Ptr->m_hWnd, WM_UPDATE_CHECK_MSG, 0, (LPARAM)sMess);
}
//---------------------------------------------------------------------------

bool UpdateCheckThread::SendHeader() {
	char * sDataToSend = "GET /version HTTP/1.1\r\nUser-Agent: PtokaX " PtokaXVersionString " [" BUILD_NUMBER "]"
		"\r\nHost: www.PtokaX.org\r\nConnection: close\r\nCache-Control: no-cache\r\nAccept: */*\r\nAccept-Language: en\r\n\r\n";

	int iBytes = send(m_Socket, sDataToSend, (int)strlen(sDataToSend), 0);

#ifdef _WIN32
    if(iBytes == SOCKET_ERROR) {
    	int iError = WSAGetLastError();
        int iMsgLen = snprintf(m_sMsg, 2048, "Update check send error %s (%d).", WSErrorStr(iError), iError);
#else
    if(iBytes == -1) {
        int iMsgLen = snprintf(m_sMsg, 2048, "Update check send error %s (%d).)", WSErrorStr(errno), errno);
#endif
        if(iMsgLen > 0) {
            Message(m_sMsg, iMsgLen);
        }

        return false;
    }

	m_ui32BytesSent += iBytes;
    
    return true;
}
//---------------------------------------------------------------------------

bool UpdateCheckThread::Receive() {
    u_long ui32bytes = 0;

	if(ioctlsocket(m_Socket, FIONREAD, &ui32bytes) == SOCKET_ERROR) {
        int iError = WSAGetLastError();
	    int iMsgLen = snprintf(m_sMsg, 2048, "Update check ioctlsocket(FIONREAD) error %s (%d).", WSErrorStr(iError), iError);
        if(iMsgLen > 0) {
			Message(m_sMsg, iMsgLen);
        }

        return false;
    }
    
    if(ui32bytes == 0) {
        // we need to try receive to catch connection error, or if server closed connection
        ui32bytes = 16;
    } else if(ui32bytes > 8192) {
        // receive max. 8192 bytes to receive buffer
        ui32bytes = 8192;
    }

    if(m_ui32RecvBufSize < m_ui32RecvBufLen + ui32bytes) {
        size_t szAllignLen = ((m_ui32RecvBufLen + ui32bytes + 1) & 0xFFFFFE00) + 0x200;

        char * pOldBuf = m_sRecvBuf;

		m_sRecvBuf = (char *)realloc(m_sRecvBuf, szAllignLen);
        if(m_sRecvBuf == NULL) {
			m_sRecvBuf = pOldBuf;

            AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes for sRecvBuf in UpdateCheckThread::Receive\n", szAllignLen);

            return false;
        }

		m_ui32RecvBufSize = (int)szAllignLen - 1;
    }

    int iBytes = recv(m_Socket, m_sRecvBuf + m_ui32RecvBufLen, m_ui32RecvBufSize - m_ui32RecvBufLen, 0);

#ifdef _WIN32
    if(iBytes == SOCKET_ERROR) {
        int iError = WSAGetLastError();
        if(iError != WSAEWOULDBLOCK) {                  
			int iMsgLen = snprintf(m_sMsg, 2048, "Update check recv error %s (%d).", WSErrorStr(iError), iError);
#else
    if(iBytes == -1) {
        if(errno != EAGAIN) {
			int iMsgLen = snprintf(m_sMsg, 2048, "Update check recv error %s (%d).", WSErrorStr(errno), errno);
#endif
            if(iMsgLen > 0) {
                Message(m_sMsg, iMsgLen);
            }

            return false;
        } else {

            return true;
        }
    } else if(iBytes == 0) {
		Message("Update check closed connection by server.", 41);

		return false;
    }
    
	m_ui32BytesRead += iBytes;

	m_ui32RecvBufLen += iBytes;
	m_sRecvBuf[m_ui32RecvBufLen] = '\0';

	if(m_bData == false) {
		char *sBuffer = m_sRecvBuf;

		for(uint32_t ui32i = 0; ui32i < m_ui32RecvBufLen; ui32i++) {
			if(m_sRecvBuf[ui32i] == '\n') {
				m_sRecvBuf[ui32i] = '\0';
				uint32_t ui32iCommandLen = (uint32_t)((m_sRecvBuf+ui32i) - sBuffer) + 1;

				if(ui32iCommandLen > 7 && strncmp(sBuffer, "HTTP", 4) == NULL && strstr(sBuffer, "200") != NULL) {
					m_bOk = true;
				} else if(ui32iCommandLen == 2 && sBuffer[0] == '\r') {
					if(m_bOk == true && m_ui32FileLen != 0) {
						m_bData = true;
					} else {
						Message("Update check failed.", 20);
						::PostMessage(MainWindow::m_Ptr->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);

						return false;
                    }
				} else if(ui32iCommandLen > 16 && strncmp(sBuffer, "Content-Length: ", 16) == NULL) {
					m_ui32FileLen = atoi(sBuffer+16);
				}

				sBuffer += ui32iCommandLen;

				if(m_bData == true) {
					break;
                }
			}
		}

		m_ui32RecvBufLen -= (uint32_t)(sBuffer - m_sRecvBuf);

		if(m_ui32RecvBufLen == 0) {
			m_sRecvBuf[0] = '\0';
		} else if(m_ui32RecvBufLen != 1) {
			memmove(m_sRecvBuf, sBuffer, m_ui32RecvBufLen);
			m_sRecvBuf[m_ui32RecvBufLen] = '\0';
		} else {
			if(sBuffer[0] == '\n') {
				m_sRecvBuf[0] = '\0';
				m_ui32RecvBufLen = 0;
			} else {
				m_sRecvBuf[0] = sBuffer[0];
				m_sRecvBuf[1] = '\0';
			}
		}
	}

	if(m_bData == true) {
		if(m_ui32RecvBufLen == (uint32_t)m_ui32FileLen) {
			char *sMess = (char *)malloc(m_ui32RecvBufLen + 1);
			if(sMess == NULL) {
				AppendDebugLogFormat("[MEM] Cannot allocate %u bytes for sMess in UpdateCheckThread::Receive\n", m_ui32RecvBufLen+1);

				return false;
			}

			memcpy(sMess, m_sRecvBuf, m_ui32RecvBufLen);
			sMess[m_ui32RecvBufLen] = '\0';

			::PostMessage(MainWindow::m_Ptr->m_hWnd, WM_UPDATE_CHECK_DATA, 0, (LPARAM)sMess);

			::PostMessage(MainWindow::m_Ptr->m_hWnd, WM_UPDATE_CHECK_TERMINATE, 0, 0);
        }
    }

	return true;
}
//---------------------------------------------------------------------------
