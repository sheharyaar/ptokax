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
#include "UdpDebug.h"
//---------------------------------------------------------------------------
#include "LanguageManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
UdpDebug * UdpDebug::m_Ptr = NULL;
//---------------------------------------------------------------------------

UdpDebug::UdpDbgItem::UdpDbgItem() : m_pPrev(NULL), m_pNext(NULL), m_sNick(NULL), 
#ifdef _WIN32
    m_Socket(INVALID_SOCKET),
#else
    m_Socket(-1),
#endif
	m_sasLen(0), m_ui32Hash(0), m_bIsScript(false), m_bAllData(true) {
    memset(&m_sasTo, 0, sizeof(sockaddr_storage));
}
//---------------------------------------------------------------------------

UdpDebug::UdpDbgItem::~UdpDbgItem() {
#ifdef _WIN32
    if(m_sNick != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sNick) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate m_sNick in UdpDebug::UdpDbgItem::~UdpDbgItem\n");
        }
    }
#else
	free(m_sNick);
#endif

#ifdef _WIN32
	closesocket(m_Socket);
#else
	close(m_Socket);
#endif
}
//---------------------------------------------------------------------------

UdpDebug::UdpDebug() : m_sDebugBuffer(NULL), m_sDebugHead(NULL), m_pDbgItemList(NULL) {
	// ...
}
//---------------------------------------------------------------------------

UdpDebug::~UdpDebug() {
#ifdef _WIN32
	if(m_sDebugBuffer != NULL) {
	    if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sDebugBuffer) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_sDebugBuffer in UdpDebug::~UdpDebug\n");
		}
	}
#else
	free(m_sDebugBuffer);
#endif

    UdpDbgItem * cur = NULL,
        * next = m_pDbgItemList;

	while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

    	delete cur;
    }
}
//---------------------------------------------------------------------------

void UdpDebug::Broadcast(const char * sMsg, const size_t szMsgLen) const {
    if(m_pDbgItemList == NULL) {
        return;
    }

    ((uint16_t *)m_sDebugBuffer)[1] = (uint16_t)szMsgLen;
    memcpy(m_sDebugHead, sMsg, szMsgLen);
    size_t szLen = (m_sDebugHead-m_sDebugBuffer)+szMsgLen;
     
    UdpDbgItem * pCur = NULL,
        * pNext = m_pDbgItemList;

	while(pNext != NULL && pNext->m_bAllData == true) {
        pCur = pNext;
        pNext = pCur->m_pNext;
#ifdef _WIN32
    	sendto(pCur->m_Socket, m_sDebugBuffer, (int)szLen, 0, (struct sockaddr *)&pCur->m_sasTo, pCur->m_sasLen);
#else
		sendto(pCur->m_Socket, m_sDebugBuffer, szLen, 0, (struct sockaddr *)&pCur->m_sasTo, pCur->m_sasLen);
#endif
        ServerManager::m_ui64BytesSent += szLen;
    }
}
//---------------------------------------------------------------------------

void UdpDebug::BroadcastFormat(const char * sFormatMsg, ...) const {
	if(m_pDbgItemList == NULL) {
		return;
	}

	va_list vlArgs;
	va_start(vlArgs, sFormatMsg);

	int iRet = vsnprintf(m_sDebugHead, 65535, sFormatMsg, vlArgs);

	va_end(vlArgs);

	if(iRet <= 0) {
		AppendDebugLogFormat("[ERR] vsnprintf wrong value %d in UdpDebug::Broadcast\n", iRet);

		return;
	}

	((uint16_t *)m_sDebugBuffer)[1] = (uint16_t)iRet;
	size_t szLen = (m_sDebugHead-m_sDebugBuffer)+iRet;

    UdpDbgItem * pCur = NULL,
        * pNext = m_pDbgItemList;

	while(pNext != NULL && pNext->m_bAllData == true) {
        pCur = pNext;
        pNext = pCur->m_pNext;
#ifdef _WIN32
    	sendto(pCur->m_Socket, m_sDebugBuffer, (int)szLen, 0, (struct sockaddr *)&pCur->m_sasTo, pCur->m_sasLen);
#else
		sendto(pCur->m_Socket, m_sDebugBuffer, szLen, 0, (struct sockaddr *)&pCur->m_sasTo, pCur->m_sasLen);
#endif
        ServerManager::m_ui64BytesSent += szLen;
    }
}
//---------------------------------------------------------------------------

void UdpDebug::CreateBuffer() {
	if(m_sDebugBuffer != NULL) {
		return;
	}

#ifdef _WIN32
	m_sDebugBuffer = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, 4+256+65535);
#else
	m_sDebugBuffer = (char *)malloc(4+256+65535);
#endif
    if(m_sDebugBuffer == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate 4+256+65535 bytes for m_sDebugBuffer in UdpDebug::CreateBuffer\n");

        exit(EXIT_FAILURE);
    }

	UpdateHubName();
}
//---------------------------------------------------------------------------

bool UdpDebug::New(User * pUser, const uint16_t ui16Port) {
	UdpDbgItem * pNewDbg = new (std::nothrow) UdpDbgItem();
    if(pNewDbg == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewDbg in UdpDebug::New\n");
    	return false;
    }

    // initialize dbg item
#ifdef _WIN32
    pNewDbg->m_sNick = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, pUser->m_ui8NickLen+1);
#else
	pNewDbg->m_sNick = (char *)malloc(pUser->m_ui8NickLen+1);
#endif
    if(pNewDbg->m_sNick == NULL) {
		AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu8 " bytes for sNick in UdpDebug::New\n", pUser->m_ui8NickLen+1);

		delete pNewDbg;
        return false;
    }

    memcpy(pNewDbg->m_sNick, pUser->m_sNick, pUser->m_ui8NickLen);
    pNewDbg->m_sNick[pUser->m_ui8NickLen] = '\0';

    pNewDbg->m_ui32Hash = pUser->m_ui32NickHash;

    struct in6_addr i6addr;
    memcpy(&i6addr, &pUser->m_ui128IpHash, 16);

    bool bIPv6 = (IN6_IS_ADDR_V4MAPPED(&i6addr) == 0);

    if(bIPv6 == true) {
        ((struct sockaddr_in6 *)&pNewDbg->m_sasTo)->sin6_family = AF_INET6;
        ((struct sockaddr_in6 *)&pNewDbg->m_sasTo)->sin6_port = htons(ui16Port);
        memcpy(((struct sockaddr_in6 *)&pNewDbg->m_sasTo)->sin6_addr.s6_addr, pUser->m_ui128IpHash, 16);
        pNewDbg->m_sasLen = sizeof(struct sockaddr_in6);
    } else {
        ((struct sockaddr_in *)&pNewDbg->m_sasTo)->sin_family = AF_INET;
        ((struct sockaddr_in *)&pNewDbg->m_sasTo)->sin_port = htons(ui16Port);
        ((struct sockaddr_in *)&pNewDbg->m_sasTo)->sin_addr.s_addr = inet_addr(pUser->m_sIP);
        pNewDbg->m_sasLen = sizeof(struct sockaddr_in);
    }

    pNewDbg->m_Socket = socket((bIPv6 == true ? AF_INET6 : AF_INET), SOCK_DGRAM, IPPROTO_UDP);
#ifdef _WIN32
    if(pNewDbg->m_Socket == INVALID_SOCKET) {
        int iErr = WSAGetLastError();
#else
    if(pNewDbg->m_Socket == -1) {
#endif
        pUser->SendFormat("UdpDebug::New1", true, "*** [ERR] %s: %s (%d).|", LanguageManager::m_Ptr->m_sTexts[LAN_UDP_SCK_CREATE_ERR],
#ifdef _WIN32
			WSErrorStr(iErr), iErr);
#else
			ErrnoStr(errno), errno);
#endif
        delete pNewDbg;
        return false;
    }

    // set non-blocking
#ifdef _WIN32
    uint32_t block = 1;
	if(SOCKET_ERROR == ioctlsocket(pNewDbg->m_Socket, FIONBIO, (unsigned long *)&block)) {
        int iErr = WSAGetLastError();
#else
    int oldFlag = fcntl(pNewDbg->m_Socket, F_GETFL, 0);
    if(fcntl(pNewDbg->m_Socket, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
#endif
		pUser->SendFormat("UdpDebug::New2", true, "*** [ERR] %s: %s (%d).|", LanguageManager::m_Ptr->m_sTexts[LAN_UDP_NON_BLOCK_FAIL],
#ifdef _WIN32
			WSErrorStr(iErr), iErr);
#else
			ErrnoStr(errno), errno);
#endif
        delete pNewDbg;
        return false;
    }

    pNewDbg->m_pPrev = NULL;
    pNewDbg->m_pNext = NULL;

    if(m_pDbgItemList == NULL){
    	CreateBuffer();
		m_pDbgItemList = pNewDbg;
    } else {
		m_pDbgItemList->m_pPrev = pNewDbg;
        pNewDbg->m_pNext = m_pDbgItemList;
		m_pDbgItemList = pNewDbg;
    }

	pNewDbg->m_bIsScript = false;

	int iLen = snprintf(m_sDebugHead, 65535, "[HUB] Subscribed, users online: %u", ServerManager::m_ui32Logged);
	if(iLen <= 0) {
		AppendDebugLogFormat("[ERR] snprintf wrong value %d in UdpDebug::New\n", iLen);

		return true;
	}

	// create packet
	((uint16_t *)m_sDebugBuffer)[1] = (uint16_t)iLen;
	size_t szLen = (m_sDebugHead-m_sDebugBuffer)+iLen;
#ifdef _WIN32
	sendto(pNewDbg->m_Socket, m_sDebugBuffer, (int)szLen, 0, (struct sockaddr *)&pNewDbg->m_sasTo, pNewDbg->m_sasLen);
#else
	sendto(pNewDbg->m_Socket, m_sDebugBuffer, szLen, 0, (struct sockaddr *)&pNewDbg->m_sasTo, pNewDbg->m_sasLen);
#endif
	ServerManager::m_ui64BytesSent += szLen;

    return true;
}
//---------------------------------------------------------------------------

bool UdpDebug::New(char * sIP, const uint16_t ui16Port, const bool bAllData, char * sScriptName) {
	UdpDbgItem * pNewDbg = new (std::nothrow) UdpDbgItem();
    if(pNewDbg == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewDbg in UdpDebug::New\n");
    	return false;
    }

    // initialize dbg item
    size_t szNameLen = strlen(sScriptName);
#ifdef _WIN32
    pNewDbg->m_sNick = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNameLen+1);
#else
	pNewDbg->m_sNick = (char *)malloc(szNameLen+1);
#endif
    if(pNewDbg->m_sNick == NULL) {
		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sNick in UdpDebug::New\n", szNameLen+1);

        delete pNewDbg;
        return false;
    }

	memcpy(pNewDbg->m_sNick, sScriptName, szNameLen);
    pNewDbg->m_sNick[szNameLen] = '\0';

    pNewDbg->m_ui32Hash = 0;

    uint8_t ui128IP[16];
    HashIP(sIP, ui128IP);

    struct in6_addr i6addr;
    memcpy(&i6addr, &ui128IP, 16);

    bool bIPv6 = (IN6_IS_ADDR_V4MAPPED(&i6addr) == 0);

    if(bIPv6 == true) {
        ((struct sockaddr_in6 *)&pNewDbg->m_sasTo)->sin6_family = AF_INET6;
        ((struct sockaddr_in6 *)&pNewDbg->m_sasTo)->sin6_port = htons(ui16Port);
        memcpy(((struct sockaddr_in6 *)&pNewDbg->m_sasTo)->sin6_addr.s6_addr, ui128IP, 16);
        pNewDbg->m_sasLen = sizeof(struct sockaddr_in6);
    } else {
        ((struct sockaddr_in *)&pNewDbg->m_sasTo)->sin_family = AF_INET;
        ((struct sockaddr_in *)&pNewDbg->m_sasTo)->sin_port = htons(ui16Port);
        memcpy(&((struct sockaddr_in *)&pNewDbg->m_sasTo)->sin_addr.s_addr, ui128IP+12, 4);
        pNewDbg->m_sasLen = sizeof(struct sockaddr_in);
    }

    pNewDbg->m_Socket = socket((bIPv6 == true ? AF_INET6 : AF_INET), SOCK_DGRAM, IPPROTO_UDP);

#ifdef _WIN32
    if(pNewDbg->m_Socket == INVALID_SOCKET) {
#else
    if(pNewDbg->m_Socket == -1) {
#endif
        delete pNewDbg;
        return false;
    }

    // set non-blocking
#ifdef _WIN32
    uint32_t block = 1;
	if(SOCKET_ERROR == ioctlsocket(pNewDbg->m_Socket, FIONBIO, (unsigned long *)&block)) {
#else
    int oldFlag = fcntl(pNewDbg->m_Socket, F_GETFL, 0);
    if(fcntl(pNewDbg->m_Socket, F_SETFL, oldFlag | O_NONBLOCK) == -1) {
#endif
        delete pNewDbg;
        return false;
    }
        
    pNewDbg->m_pPrev = NULL;
    pNewDbg->m_pNext = NULL;

    if(m_pDbgItemList == NULL) {
    	CreateBuffer();
		m_pDbgItemList = pNewDbg;
    } else {
		m_pDbgItemList->m_pPrev = pNewDbg;
        pNewDbg->m_pNext = m_pDbgItemList;
		m_pDbgItemList = pNewDbg;
    }

    pNewDbg->m_bIsScript = true;
    pNewDbg->m_bAllData = bAllData;

	int iLen = snprintf(m_sDebugHead, 65535, "[HUB] Subscribed, users online: %u", ServerManager::m_ui32Logged);
	if(iLen <= 0) {
		AppendDebugLogFormat("[ERR] snprintf wrong value %d in UdpDebug::New2\n", iLen);

		return true;
	}

	// create packet
	((uint16_t *)m_sDebugBuffer)[1] = (uint16_t)iLen;
	size_t szLen = (m_sDebugHead-m_sDebugBuffer)+iLen;
#ifdef _WIN32
	sendto(pNewDbg->m_Socket, m_sDebugBuffer, (int)szLen, 0, (struct sockaddr *)&pNewDbg->m_sasTo, pNewDbg->m_sasLen);
#else
	sendto(pNewDbg->m_Socket, m_sDebugBuffer, szLen, 0, (struct sockaddr *)&pNewDbg->m_sasTo, pNewDbg->m_sasLen);
#endif
	ServerManager::m_ui64BytesSent += szLen;

    return true;
}
//---------------------------------------------------------------------------

void UdpDebug::DeleteBuffer() {
#ifdef _WIN32
	if(m_sDebugBuffer != NULL) {
	    if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sDebugBuffer) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_sDebugBuffer in UdpDebug::DeleteBuffer\n");
		}
	}
#else
	free(m_sDebugBuffer);
#endif
	m_sDebugBuffer = NULL;
	m_sDebugHead = NULL;
}
//---------------------------------------------------------------------------

bool UdpDebug::Remove(User * pUser) {
    UdpDbgItem * pCur = NULL,
        * pNext = m_pDbgItemList;

	while(pNext != NULL) {
        pCur = pNext;
        pNext = pCur->m_pNext;

		if(pCur->m_bIsScript == false && pCur->m_ui32Hash == pUser->m_ui32NickHash && strcasecmp(pCur->m_sNick, pUser->m_sNick) == 0) {
            if(pCur->m_pPrev == NULL) {
                if(pCur->m_pNext == NULL) {
					m_pDbgItemList = NULL;
                    DeleteBuffer();
                } else {
                    pCur->m_pNext->m_pPrev = NULL;
					m_pDbgItemList = pCur->m_pNext;
                }
            } else if(pCur->m_pNext == NULL) {
                pCur->m_pPrev->m_pNext = NULL;
            } else {
                pCur->m_pPrev->m_pNext = pCur->m_pNext;
                pCur->m_pNext->m_pPrev = pCur->m_pPrev;
            }
            
	        delete pCur;
            return true;
        }
    }
    return false;
}
//---------------------------------------------------------------------------

void UdpDebug::Remove(char * sScriptName) {
    UdpDbgItem * pCur = NULL,
        * pNext = m_pDbgItemList;

	while(pNext != NULL) {
        pCur = pNext;
        pNext = pCur->m_pNext;

		if(pCur->m_bIsScript == true && strcasecmp(pCur->m_sNick, sScriptName) == 0) {
            if(pCur->m_pPrev == NULL) {
                if(pCur->m_pNext == NULL) {
					m_pDbgItemList = NULL;
                    DeleteBuffer();
                } else {
                    pCur->m_pNext->m_pPrev = NULL;
					m_pDbgItemList = pCur->m_pNext;
                }
            } else if(pCur->m_pNext == NULL) {
                pCur->m_pPrev->m_pNext = NULL;
            } else {
                pCur->m_pPrev->m_pNext = pCur->m_pNext;
                pCur->m_pNext->m_pPrev = pCur->m_pPrev;
            }
            
	        delete pCur;
            return;
        }
    }
}
//---------------------------------------------------------------------------

bool UdpDebug::CheckUdpSub(User * pUser, const bool bSendMsg/* = false*/) const {
    UdpDbgItem * pCur = NULL,
        * pNext = m_pDbgItemList;

	while(pNext != NULL) {
        pCur = pNext;
        pNext = pCur->m_pNext;

		if(pCur->m_bIsScript == false && pCur->m_ui32Hash == pUser->m_ui32NickHash && strcasecmp(pCur->m_sNick, pUser->m_sNick) == 0) {
            if(bSendMsg == true) {
				pUser->SendFormat("UdpDebug::CheckUdpSub", true, "<%s> *** %s %hu. %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_SUBSCRIBED_UDP_DBG], 
					ntohs(pCur->m_sasTo.ss_family == AF_INET6 ? ((struct sockaddr_in6 *)&pCur->m_sasTo)->sin6_port : ((struct sockaddr_in *)&pCur->m_sasTo)->sin_port), LanguageManager::m_Ptr->m_sTexts[LAN_TO_UNSUB_UDP_DBG]);
            }
                
            return true;
        }
    }
    return false;
}
//---------------------------------------------------------------------------

void UdpDebug::Send(const char * sScriptName, char * sMessage, const size_t szMsgLen) const {
	if(m_pDbgItemList == NULL) {
		return;
	}

    UdpDbgItem * pCur = NULL,
        * pNext = m_pDbgItemList;

	while(pNext != NULL) {
        pCur = pNext;
        pNext = pCur->m_pNext;

		if(pCur->m_bIsScript == true && strcasecmp(pCur->m_sNick, sScriptName) == 0) {      
            // create packet
            ((uint16_t *)m_sDebugBuffer)[1] = (uint16_t)szMsgLen;
            memcpy(m_sDebugHead, sMessage, szMsgLen);
            size_t szLen = (m_sDebugHead-m_sDebugBuffer)+szMsgLen;

#ifdef _WIN32
            sendto(pCur->m_Socket, m_sDebugBuffer, (int)szLen, 0, (struct sockaddr *)&pCur->m_sasTo, pCur->m_sasLen);
#else
			sendto(pCur->m_Socket, m_sDebugBuffer, szLen, 0, (struct sockaddr *)&pCur->m_sasTo, pCur->m_sasLen);
#endif
            ServerManager::m_ui64BytesSent += szLen;

        	return;
        }
    }
}
//---------------------------------------------------------------------------

void UdpDebug::Cleanup() {
    UdpDbgItem * pCur = NULL,
        * pNext = m_pDbgItemList;

	while(pNext != NULL) {
        pCur = pNext;
        pNext = pCur->m_pNext;

    	delete pCur;
    }

	m_pDbgItemList = NULL;
}
//---------------------------------------------------------------------------

void UdpDebug::UpdateHubName() {
	if(m_sDebugBuffer == NULL) {
		return;
	}

	((uint16_t *)m_sDebugBuffer)[0] = (uint16_t)SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_HUB_NAME];
	memcpy(m_sDebugBuffer+4, SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_NAME], SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_HUB_NAME]);

	m_sDebugHead = m_sDebugBuffer+4+SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_HUB_NAME];
}
//---------------------------------------------------------------------------
