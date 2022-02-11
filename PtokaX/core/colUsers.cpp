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
#include "colUsers.h"
//---------------------------------------------------------------------------
#include "GlobalDataQueue.h"
#include "LanguageManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
static const uint32_t MYINFOLISTSIZE = 1024*256;
static const uint32_t IPLISTSIZE = 1024*64;
static const uint32_t ZLISTSIZE = 1024*16;
static const uint32_t ZMYINFOLISTSIZE = 1024*128;
//---------------------------------------------------------------------------
Users * Users::m_Ptr = NULL;
//---------------------------------------------------------------------------

Users::RecTime::RecTime(const uint8_t * pIpHash) : m_ui64DisConnTick(0), m_pPrev(NULL), m_pNext(NULL), m_sNick(NULL), m_ui32NickHash(0) {
    memcpy(m_ui128IpHash, pIpHash, 16);
};
//---------------------------------------------------------------------------

Users::Users() : m_ui64ChatMsgsTick(0), m_ui64ChatLockFromTick(0), m_pRecTimeList(NULL), m_pUserListE(NULL), m_ui16ChatMsgs(0), m_bChatLocked(false), m_pUserListS(NULL), m_pNickList(NULL), m_pZNickList(NULL), m_pOpList(NULL), m_pZOpList(NULL), m_pUserIPList(NULL), m_pZUserIPList(NULL),
	m_pMyInfos(NULL), m_pZMyInfos(NULL), m_pMyInfosTag(NULL), m_pZMyInfosTag(NULL), m_ui32MyInfosLen(0), m_ui32MyInfosSize(0), m_ui32ZMyInfosLen(0), m_ui32ZMyInfosSize(0), m_ui32MyInfosTagLen(0), m_ui32MyInfosTagSize(0), m_ui32ZMyInfosTagLen(0), m_ui32ZMyInfosTagSize(0),
	m_ui32NickListLen(0), m_ui32NickListSize(0), m_ui32ZNickListLen(0), m_ui32ZNickListSize(0), m_ui32OpListLen(0), m_ui32OpListSize(0), m_ui32ZOpListLen(0), m_ui32ZOpListSize(0), m_ui32UserIPListSize(0), m_ui32UserIPListLen(0), m_ui32ZUserIPListSize(0), m_ui32ZUserIPListLen(0),
	m_ui16ActSearchs(0), m_ui16PasSearchs(0) {
#ifdef _WIN32
    ServerManager::m_hRecvHeap = HeapCreate(HEAP_NO_SERIALIZE, 0x20000, 0);
    ServerManager::m_hSendHeap = HeapCreate(HEAP_NO_SERIALIZE, 0x40000, 0);

	m_pNickList = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, NICKLISTSIZE);
#else
	m_pNickList = (char *)calloc(NICKLISTSIZE, 1);
#endif
    if(m_pNickList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create m_pNickList\n");
		exit(EXIT_FAILURE);
    }
    memcpy(m_pNickList, "$NickList |", 11);
	m_pNickList[11] = '\0';
	m_ui32NickListLen = 11;
	m_ui32NickListSize = NICKLISTSIZE - 1;
#ifdef _WIN32
	m_pZNickList = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZLISTSIZE);
#else
	m_pZNickList = (char *)calloc(ZLISTSIZE, 1);
#endif
    if(m_pZNickList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create m_pZNickList\n");
		exit(EXIT_FAILURE);
    }
	m_ui32ZNickListLen = 0;
	m_ui32ZNickListSize = ZLISTSIZE - 1;
#ifdef _WIN32
	m_pOpList = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, OPLISTSIZE);
#else
	m_pOpList = (char *)calloc(OPLISTSIZE, 1);
#endif
    if(m_pOpList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create m_pOpList\n");
		exit(EXIT_FAILURE);
    }
    memcpy(m_pOpList, "$OpList |", 9);
	m_pOpList[9] = '\0';
	m_ui32OpListLen = 9;
	m_ui32OpListSize = OPLISTSIZE - 1;
#ifdef _WIN32
	m_pZOpList = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZLISTSIZE);
#else
	m_pZOpList = (char *)calloc(ZLISTSIZE, 1);
#endif
    if(m_pZOpList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create m_pZOpList\n");
		exit(EXIT_FAILURE);
    }
	m_ui32ZOpListLen = 0;
	m_ui32ZOpListSize = ZLISTSIZE - 1;
    
    if(SettingManager::m_Ptr->m_ui8FullMyINFOOption != 0) {
#ifdef _WIN32
		m_pMyInfos = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, MYINFOLISTSIZE);
#else
		m_pMyInfos = (char *)calloc(MYINFOLISTSIZE, 1);
#endif
        if(m_pMyInfos == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot create m_pMyInfos\n");
    		exit(EXIT_FAILURE);
        }
		m_ui32MyInfosSize = MYINFOLISTSIZE - 1;
        
#ifdef _WIN32
		m_pZMyInfos = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZMYINFOLISTSIZE);
#else
		m_pZMyInfos = (char *)calloc(ZMYINFOLISTSIZE, 1);
#endif
        if(m_pZMyInfos == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot create m_pZMyInfos\n");
    		exit(EXIT_FAILURE);
        }
		m_ui32ZMyInfosSize = ZMYINFOLISTSIZE - 1;
    } else {
		m_pMyInfos = NULL;
		m_ui32MyInfosSize = 0;
		m_pZMyInfos = NULL;
		m_ui32ZMyInfosSize = 0;
    }
	m_ui32MyInfosLen = 0;
	m_ui32ZMyInfosLen = 0;
    
    if(SettingManager::m_Ptr->m_ui8FullMyINFOOption != 2) {
#ifdef _WIN32
		m_pMyInfosTag = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, MYINFOLISTSIZE);
#else
		m_pMyInfosTag = (char *)calloc(MYINFOLISTSIZE, 1);
#endif
        if(m_pMyInfosTag == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot create m_pMyInfosTag\n");
    		exit(EXIT_FAILURE);
        }
		m_ui32MyInfosTagSize = MYINFOLISTSIZE - 1;

#ifdef _WIN32
		m_pZMyInfosTag = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZMYINFOLISTSIZE);
#else
		m_pZMyInfosTag = (char *)calloc(ZMYINFOLISTSIZE, 1);
#endif
        if(m_pZMyInfosTag == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot create m_pZMyInfosTag\n");
    		exit(EXIT_FAILURE);
        }
		m_ui32ZMyInfosTagSize = ZMYINFOLISTSIZE - 1;
    } else {
		m_pMyInfosTag = NULL;
		m_ui32MyInfosTagSize = 0;
		m_pZMyInfosTag = NULL;
		m_ui32ZMyInfosTagSize = 0;
    }
	m_ui32MyInfosTagLen = 0;
	m_ui32ZMyInfosTagLen = 0;

#ifdef _WIN32
	m_pUserIPList = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, IPLISTSIZE);
#else
	m_pUserIPList = (char *)calloc(IPLISTSIZE, 1);
#endif
    if(m_pUserIPList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create m_pUserIPList\n");
		exit(EXIT_FAILURE);
    }
    memcpy(m_pUserIPList, "$UserIP |", 9);
	m_pUserIPList[9] = '\0';
	m_ui32UserIPListLen = 9;
	m_ui32UserIPListSize = IPLISTSIZE - 1;

#ifdef _WIN32
	m_pZUserIPList = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZLISTSIZE);
#else
	m_pZUserIPList = (char *)calloc(ZLISTSIZE, 1);
#endif
    if(m_pZUserIPList == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create m_pZUserIPList\n");
		exit(EXIT_FAILURE);
    }
	m_ui32ZUserIPListLen = 0;
	m_ui32ZUserIPListSize = ZLISTSIZE - 1;
}
//---------------------------------------------------------------------------

Users::~Users() {
    RecTime * cur = NULL,
        * next = m_pRecTimeList;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

#ifdef _WIN32
        if(cur->m_sNick != NULL) {
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->m_sNick) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate cur->m_sNick in Users::~Users\n");
            }
        }
#else
		free(cur->m_sNick);
#endif

        delete cur;
    }

#ifdef _WIN32
    if(m_pNickList != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_pNickList) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate m_pNickList in Users::~Users\n");
        }
    }
#else
	free(m_pNickList);
#endif

#ifdef _WIN32
    if(m_pZNickList != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_pZNickList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_pZNickList in Users::~Users\n");
        }
    }
#else
	free(m_pZNickList);
#endif

#ifdef _WIN32
    if(m_pOpList != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_pOpList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_pOpList in Users::~Users\n");
        }
    }
#else
	free(m_pOpList);
#endif

#ifdef _WIN32
    if(m_pZOpList != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_pZOpList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_pZOpList in Users::~Users\n");
        }
    }
#else
	free(m_pZOpList);
#endif

#ifdef _WIN32
    if(m_pMyInfos != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_pMyInfos) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_pMyInfos in Users::~Users\n");
        }
    }
#else
	free(m_pMyInfos);
#endif

#ifdef _WIN32
    if(m_pZMyInfos != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_pZMyInfos) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_pZMyInfos in Users::~Users\n");
        }
    }
#else
	free(m_pZMyInfos);
#endif

#ifdef _WIN32
    if(m_pMyInfosTag != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_pMyInfosTag) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_pMyInfosTag in Users::~Users\n");
        }
    }
#else
	free(m_pMyInfosTag);
#endif

#ifdef _WIN32
    if(m_pZMyInfosTag != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_pZMyInfosTag) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_pZMyInfosTag in Users::~Users\n");
        }
    }
#else
	free(m_pZMyInfosTag);
#endif

#ifdef _WIN32
    if(m_pUserIPList != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_pUserIPList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_pUserIPList in Users::~Users\n");
        }
    }
#else
	free(m_pUserIPList);
#endif

#ifdef _WIN32
    if(m_pZUserIPList != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_pZUserIPList) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_pZUserIPList in Users::~Users\n");
        }
    }
#else
	free(m_pZUserIPList);
#endif

#ifdef _WIN32
    HeapDestroy(ServerManager::m_hRecvHeap);
    HeapDestroy(ServerManager::m_hSendHeap);
#endif
}
//---------------------------------------------------------------------------

void Users::AddUser(User * pUser) {
    if(m_pUserListS == NULL) {
		m_pUserListS = pUser;
		m_pUserListE = pUser;
    } else {
        pUser->m_pPrev = m_pUserListE;
		m_pUserListE->m_pNext = pUser;
		m_pUserListE = pUser;
    }
}
//---------------------------------------------------------------------------

void Users::RemUser(User * pUser) {
    if(pUser->m_pPrev == NULL) {
        if(pUser->m_pNext == NULL) {
			m_pUserListS = NULL;
			m_pUserListE = NULL;
        } else {
            pUser->m_pNext->m_pPrev = NULL;
			m_pUserListS = pUser->m_pNext;
        }
    } else if(pUser->m_pNext == NULL) {
        pUser->m_pPrev->m_pNext = NULL;
		m_pUserListE = pUser->m_pPrev;
    } else {
        pUser->m_pPrev->m_pNext = pUser->m_pNext;
        pUser->m_pNext->m_pPrev = pUser->m_pPrev;
    }
}
//---------------------------------------------------------------------------

void Users::DisconnectAll() {
    uint32_t iCloseLoops = 0;

#ifndef _WIN32
    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 50000000;
#endif

    User * u = NULL, * next = NULL;

    while(m_pUserListS != NULL && iCloseLoops <= 100) {
        next = m_pUserListS;

        while(next != NULL) {
            u = next;
    		next = u->m_pNext;

            if(((u->m_ui32BoolBits & User::BIT_ERROR) == User::BIT_ERROR) == true || u->m_ui32SendBufDataLen == 0) {
//              Memo("*** User " + string(u->Nick, u->NickLen) + " closed...");
                if(u->m_pPrev == NULL) {
                    if(u->m_pNext == NULL) {
						m_pUserListS = NULL;
                    } else {
                        u->m_pNext->m_pPrev = NULL;
						m_pUserListS = u->m_pNext;
                    }
                } else if(u->m_pNext == NULL) {
                    u->m_pPrev->m_pNext = NULL;
                } else {
                    u->m_pPrev->m_pNext = u->m_pNext;
                    u->m_pNext->m_pPrev = u->m_pPrev;
                }

#ifdef _WIN32
                shutdown(u->m_Socket, SD_SEND);
				closesocket(u->m_Socket);
#else
                shutdown(u->m_Socket, SHUT_RD);
				close(u->m_Socket);
#endif

				delete u;
            } else {
                u->Try2Send();
            }
        }
        iCloseLoops++;
#ifdef _WIN32
        ::Sleep(50);
#else
        nanosleep(&sleeptime, NULL);
#endif
    }

    next = m_pUserListS;

    while(next != NULL) {
        u = next;
    	next = u->m_pNext;

#ifdef _WIN32
    	shutdown(u->m_Socket, SD_SEND);
		closesocket(u->m_Socket);
#else
    	shutdown(u->m_Socket, SHUT_RDWR);
		close(u->m_Socket);
#endif

		delete u;
	}
}
//---------------------------------------------------------------------------

void Users::Add2NickList(User * pUser) {
    // $NickList nick$$nick2$$|

    if(m_ui32NickListSize < m_ui32NickListLen+pUser->m_ui8NickLen+2) {
        char * pOldBuf = m_pNickList;
#ifdef _WIN32
		m_pNickList = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, m_ui32NickListSize+NICKLISTSIZE+1);
#else
		m_pNickList = (char *)realloc(pOldBuf, m_ui32NickListSize+NICKLISTSIZE+1);
#endif
        if(m_pNickList == NULL) {
			m_pNickList = pOldBuf;
            pUser->m_ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

			AppendDebugLogFormat("Cannot reallocate %u bytes in Users::Add2NickList for m_pNickList\n", m_ui32NickListSize+NICKLISTSIZE+1);

            return;
		}
		m_ui32NickListSize += NICKLISTSIZE;
    }

    memcpy(m_pNickList+m_ui32NickListLen-1, pUser->m_sNick, pUser->m_ui8NickLen);
	m_ui32NickListLen += (uint32_t)(pUser->m_ui8NickLen+2);

	m_pNickList[m_ui32NickListLen-3] = '$';
	m_pNickList[m_ui32NickListLen-2] = '$';
	m_pNickList[m_ui32NickListLen-1] = '|';
	m_pNickList[m_ui32NickListLen] = '\0';

	m_ui32ZNickListLen = 0;

    if(((pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        return;
    }

    if(m_ui32OpListSize < m_ui32OpListLen+pUser->m_ui8NickLen+2) {
        char * pOldBuf = m_pOpList;
#ifdef _WIN32
		m_pOpList = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, m_ui32OpListSize+OPLISTSIZE+1);
#else
		m_pOpList = (char *)realloc(pOldBuf, m_ui32OpListSize+OPLISTSIZE+1);
#endif
        if(m_pOpList == NULL) {
			m_pOpList = pOldBuf;
            pUser->m_ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

            AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in Users::Add2NickList for m_pOpList\n", m_ui32OpListSize+OPLISTSIZE+1);

            return;
        }
		m_ui32OpListSize += OPLISTSIZE;
    }

    memcpy(m_pOpList+m_ui32OpListLen-1, pUser->m_sNick, pUser->m_ui8NickLen);
	m_ui32OpListLen += (uint32_t)(pUser->m_ui8NickLen+2);

	m_pOpList[m_ui32OpListLen-3] = '$';
	m_pOpList[m_ui32OpListLen-2] = '$';
	m_pOpList[m_ui32OpListLen-1] = '|';
	m_pOpList[m_ui32OpListLen] = '\0';

	m_ui32ZOpListLen = 0;
}
//---------------------------------------------------------------------------

void Users::AddBot2NickList(char * sNick, const size_t szNickLen, const bool bIsOp) {
    // $NickList nick$$nick2$$|

    if(m_ui32NickListSize < m_ui32NickListLen+szNickLen+2) {
        char * pOldBuf = m_pNickList;
#ifdef _WIN32
		m_pNickList = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, m_ui32NickListSize+NICKLISTSIZE+1);
#else
		m_pNickList = (char *)realloc(pOldBuf, m_ui32NickListSize+NICKLISTSIZE+1);
#endif
        if(m_pNickList == NULL) {
			m_pNickList = pOldBuf;

			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in Users::AddBot2NickList for m_pNickList\n", m_ui32NickListSize+NICKLISTSIZE+1);

            return;
		}
		m_ui32NickListSize += NICKLISTSIZE;
    }

    memcpy(m_pNickList+m_ui32NickListLen-1, sNick, szNickLen);
	m_ui32NickListLen += (uint32_t)(szNickLen+2);

	m_pNickList[m_ui32NickListLen-3] = '$';
	m_pNickList[m_ui32NickListLen-2] = '$';
	m_pNickList[m_ui32NickListLen-1] = '|';
	m_pNickList[m_ui32NickListLen] = '\0';

	m_ui32ZNickListLen = 0;

    if(bIsOp == false)
        return;

    if(m_ui32OpListSize < m_ui32OpListLen+szNickLen+2) {
        char * pOldBuf = m_pOpList;
#ifdef _WIN32
		m_pOpList = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, m_ui32OpListSize+OPLISTSIZE+1);
#else
		m_pOpList = (char *)realloc(pOldBuf, m_ui32OpListSize+OPLISTSIZE+1);
#endif
        if(m_pOpList == NULL) {
			m_pOpList = pOldBuf;

            AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in Users::AddBot2NickList for m_pOpList\n", m_ui32OpListSize+OPLISTSIZE+1);

            return;
        }
		m_ui32OpListSize += OPLISTSIZE;
    }

    memcpy(m_pOpList+m_ui32OpListLen-1, sNick, szNickLen);
	m_ui32OpListLen += (uint32_t)(szNickLen+2);

	m_pOpList[m_ui32OpListLen-3] = '$';
	m_pOpList[m_ui32OpListLen-2] = '$';
	m_pOpList[m_ui32OpListLen-1] = '|';
	m_pOpList[m_ui32OpListLen] = '\0';

	m_ui32ZOpListLen = 0;
}
//---------------------------------------------------------------------------

void Users::Add2OpList(User * pUser) {
    if(m_ui32OpListSize < m_ui32OpListLen+pUser->m_ui8NickLen+2) {
        char * pOldBuf = m_pOpList;
#ifdef _WIN32
		m_pOpList = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, m_ui32OpListSize+OPLISTSIZE+1);
#else
		m_pOpList = (char *)realloc(pOldBuf, m_ui32OpListSize+OPLISTSIZE+1);
#endif
        if(m_pOpList == NULL) {
			m_pOpList = pOldBuf;
            pUser->m_ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

            AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in Users::Add2OpList for m_pOpList\n", m_ui32OpListSize+OPLISTSIZE+1);

            return;
        }
		m_ui32OpListSize += OPLISTSIZE;
    }

    memcpy(m_pOpList+m_ui32OpListLen-1, pUser->m_sNick, pUser->m_ui8NickLen);
	m_ui32OpListLen += (uint32_t)(pUser->m_ui8NickLen+2);

	m_pOpList[m_ui32OpListLen-3] = '$';
	m_pOpList[m_ui32OpListLen-2] = '$';
	m_pOpList[m_ui32OpListLen-1] = '|';
	m_pOpList[m_ui32OpListLen] = '\0';

	m_ui32ZOpListLen = 0;
}
//---------------------------------------------------------------------------

void Users::DelFromNickList(char * sNick, const bool bIsOp) {
    int iRet = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$%s$", sNick);
    if(iRet <= 0) {
        return;
    }

	m_pNickList[9] = '$';
    char * sFound = strstr(m_pNickList, ServerManager::m_pGlobalBuffer);
	m_pNickList[9] = ' ';

    if(sFound != NULL) {
        memmove(sFound+1, sFound+(iRet+1), m_ui32NickListLen-((sFound+iRet)-m_pNickList));
		m_ui32NickListLen -= iRet;
		m_ui32ZNickListLen = 0;
    }

    if(!bIsOp)
		return;

	m_pOpList[7] = '$';
    sFound = strstr(m_pOpList, ServerManager::m_pGlobalBuffer);
	m_pOpList[7] = ' ';

    if(sFound != NULL) {
        memmove(sFound+1, sFound+(iRet+1), m_ui32OpListLen-((sFound+iRet)-m_pOpList));
		m_ui32OpListLen -= iRet;
		m_ui32ZOpListLen = 0;
    }
}
//---------------------------------------------------------------------------

void Users::DelFromOpList(char * sNick) {
    int iRet = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$%s$", sNick);
    if(iRet <= 0) {
        return;
    }

	m_pOpList[7] = '$';
    char * sFound = strstr(m_pOpList, ServerManager::m_pGlobalBuffer);
	m_pOpList[7] = ' ';

    if(sFound != NULL) {
        memmove(sFound+1, sFound+(iRet+1), m_ui32OpListLen-((sFound+iRet)-m_pOpList));
		m_ui32OpListLen -= iRet;
		m_ui32ZOpListLen = 0;
    }
}
//---------------------------------------------------------------------------

// PPK ... check global mainchat flood and add to global queue
void Users::SendChat2All(User * pUser, char * sData, const size_t szChatLen, void * pQueueItem) {
    UdpDebug::m_Ptr->Broadcast(sData, szChatLen);

    if(ProfileManager::m_Ptr->IsAllowed(pUser, ProfileManager::NODEFLOODMAINCHAT) == false && SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] != 0) {
        if(m_ui16ChatMsgs == 0) {
			m_ui64ChatMsgsTick = ServerManager::m_ui64ActualTick;
			m_ui64ChatLockFromTick = ServerManager::m_ui64ActualTick;
			m_ui16ChatMsgs = 0;
			m_bChatLocked = false;
		} else if((m_ui64ChatMsgsTick+SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_TIME]) < ServerManager::m_ui64ActualTick) {
			m_ui64ChatMsgsTick = ServerManager::m_ui64ActualTick;
			m_ui16ChatMsgs = 0;
        }

		m_ui16ChatMsgs++;

        if(m_ui16ChatMsgs > (uint16_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES]) {
			m_ui64ChatLockFromTick = ServerManager::m_ui64ActualTick;
            if(m_bChatLocked == false) {
                if(SettingManager::m_Ptr->m_bBools[SETBOOL_DEFLOOD_REPORT] == true) {
                    GlobalDataQueue::m_Ptr->StatusMessageFormat("Users::SendChat2All", "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_GLOBAL_CHAT_FLOOD_DETECTED]);
                }

				m_bChatLocked = true;
            }
        }

        if(m_bChatLocked == true) {
            if((m_ui64ChatLockFromTick+SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT]) > ServerManager::m_ui64ActualTick) {
                if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 1) {
                    return;
                } else if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GLOBAL_MAIN_CHAT_ACTION] == 2) {
                	memcpy(ServerManager::m_pGlobalBuffer, pUser->m_sIP, pUser->m_ui8IpLen);
                	ServerManager::m_pGlobalBuffer[pUser->m_ui8IpLen] = ' ';
					size_t szLen = pUser->m_ui8IpLen+1;
                    memcpy(ServerManager::m_pGlobalBuffer+szLen, sData, szChatLen);
                    szLen += szChatLen;
                    ServerManager::m_pGlobalBuffer[szLen] = '\0';
                    GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, szLen, NULL, 0, GlobalDataQueue::CMD_OPS);

                    return;
                }
            } else {
				m_bChatLocked = false;
            }
        }
    }

    if(pQueueItem == NULL) {
        GlobalDataQueue::m_Ptr->AddQueueItem(sData, szChatLen, NULL, 0, GlobalDataQueue::CMD_CHAT);
    } else {
        GlobalDataQueue::m_Ptr->FillBlankQueueItem(sData, szChatLen, pQueueItem);
    }
}
//---------------------------------------------------------------------------

void Users::Add2MyInfos(User * pUser) {
    if(m_ui32MyInfosSize < m_ui32MyInfosLen+pUser->m_ui16MyInfoShortLen) {
        char * pOldBuf = m_pMyInfos;
#ifdef _WIN32
		m_pMyInfos = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, m_ui32MyInfosSize+MYINFOLISTSIZE+1);
#else
		m_pMyInfos = (char *)realloc(pOldBuf, m_ui32MyInfosSize+MYINFOLISTSIZE+1);
#endif
        if(m_pMyInfos == NULL) {
			m_pMyInfos = pOldBuf;
            pUser->m_ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in Users::Add2MyInfos\n", m_ui32MyInfosSize+MYINFOLISTSIZE+1);

            return;
        }
		m_ui32MyInfosSize += MYINFOLISTSIZE;
    }

    memcpy(m_pMyInfos+m_ui32MyInfosLen, pUser->m_sMyInfoShort, pUser->m_ui16MyInfoShortLen);
	m_ui32MyInfosLen += pUser->m_ui16MyInfoShortLen;

	m_pMyInfos[m_ui32MyInfosLen] = '\0';

	m_ui32ZMyInfosLen = 0;
}
//---------------------------------------------------------------------------

void Users::DelFromMyInfos(User * pUser) {
	char * sMatch = strstr(m_pMyInfos, pUser->m_sMyInfoShort+8);
    if(sMatch != NULL) {
		sMatch -= 8;
		memmove(sMatch, sMatch+pUser->m_ui16MyInfoShortLen, m_ui32MyInfosLen-((sMatch+(pUser->m_ui16MyInfoShortLen-1))-m_pMyInfos));
		m_ui32MyInfosLen -= pUser->m_ui16MyInfoShortLen;
		m_ui32ZMyInfosLen = 0;
    }
}
//---------------------------------------------------------------------------

void Users::Add2MyInfosTag(User * pUser) {
    if(m_ui32MyInfosTagSize < m_ui32MyInfosTagLen+pUser->m_ui16MyInfoLongLen) {
        char * pOldBuf = m_pMyInfosTag;
#ifdef _WIN32
		m_pMyInfosTag = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, m_ui32MyInfosTagSize+MYINFOLISTSIZE+1);
#else
		m_pMyInfosTag = (char *)realloc(pOldBuf, m_ui32MyInfosTagSize+MYINFOLISTSIZE+1);
#endif
        if(m_pMyInfosTag == NULL) {
			m_pMyInfosTag = pOldBuf;
            pUser->m_ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in Users::Add2MyInfosTag\n", m_ui32MyInfosTagSize+MYINFOLISTSIZE+1);

            return;
        }
		m_ui32MyInfosTagSize += MYINFOLISTSIZE;
    }

    memcpy(m_pMyInfosTag+m_ui32MyInfosTagLen, pUser->m_sMyInfoLong, pUser->m_ui16MyInfoLongLen);
	m_ui32MyInfosTagLen += pUser->m_ui16MyInfoLongLen;

	m_pMyInfosTag[m_ui32MyInfosTagLen] = '\0';

	m_ui32ZMyInfosTagLen = 0;
}
//---------------------------------------------------------------------------

void Users::DelFromMyInfosTag(User * pUser) {
	char * sMatch = strstr(m_pMyInfosTag, pUser->m_sMyInfoLong+8);
    if(sMatch != NULL) {
		sMatch -= 8;
        memmove(sMatch, sMatch+pUser->m_ui16MyInfoLongLen, m_ui32MyInfosTagLen-((sMatch+(pUser->m_ui16MyInfoLongLen-1))-m_pMyInfosTag));
		m_ui32MyInfosTagLen -= pUser->m_ui16MyInfoLongLen;
		m_ui32ZMyInfosTagLen = 0;
    }
}
//---------------------------------------------------------------------------

void Users::AddBot2MyInfos(char * sMyInfo) {
	size_t szLen = strlen(sMyInfo);
	if(m_pMyInfosTag != NULL) {
	    if(strstr(m_pMyInfosTag, sMyInfo) == NULL ) {
            if(m_ui32MyInfosTagSize < m_ui32MyInfosTagLen+szLen) {
                char * pOldBuf = m_pMyInfosTag;
#ifdef _WIN32
				m_pMyInfosTag = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, m_ui32MyInfosTagSize+MYINFOLISTSIZE+1);
#else
				m_pMyInfosTag = (char *)realloc(pOldBuf, m_ui32MyInfosTagSize+MYINFOLISTSIZE+1);
#endif
                if(m_pMyInfosTag == NULL) {
					m_pMyInfosTag = pOldBuf;

					AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes for m_pMyInfosTag in Users::AddBot2MyInfos\n", m_ui32MyInfosTagSize+MYINFOLISTSIZE+1);

                    return;
                }
				m_ui32MyInfosTagSize += MYINFOLISTSIZE;
            }
        	memcpy(m_pMyInfosTag+m_ui32MyInfosTagLen, sMyInfo, szLen);
			m_ui32MyInfosTagLen += (uint32_t)szLen;
			m_pMyInfosTag[m_ui32MyInfosTagLen] = '\0';
			m_ui32ZMyInfosLen = 0;
        }
    }

    if(m_pMyInfos != NULL) {
    	if(strstr(m_pMyInfos, sMyInfo) == NULL ) {
            if(m_ui32MyInfosSize < m_ui32MyInfosLen+szLen) {
                char * pOldBuf = m_pMyInfos;
#ifdef _WIN32
				m_pMyInfos = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, m_ui32MyInfosSize+MYINFOLISTSIZE+1);
#else
				m_pMyInfos = (char *)realloc(pOldBuf, m_ui32MyInfosSize+MYINFOLISTSIZE+1);
#endif
                if(m_pMyInfos == NULL) {
					m_pMyInfos = pOldBuf;

					AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes for m_pMyInfos in Users::AddBot2MyInfos\n", m_ui32MyInfosSize+MYINFOLISTSIZE+1);

                    return;
                }
				m_ui32MyInfosSize += MYINFOLISTSIZE;
            }
        	memcpy(m_pMyInfos+m_ui32MyInfosLen, sMyInfo, szLen);
			m_ui32MyInfosLen += (uint32_t)szLen;
			m_pMyInfos[m_ui32MyInfosLen] = '\0';
			m_ui32ZMyInfosTagLen = 0;
         }
    }
}
//---------------------------------------------------------------------------

void Users::DelBotFromMyInfos(char * sMyInfo) {
	size_t szLen = strlen(sMyInfo);
	if(m_pMyInfosTag) {
		char * sMatch = strstr(m_pMyInfosTag,  sMyInfo);
	    if(sMatch) {
    		memmove(sMatch, sMatch+szLen, m_ui32MyInfosTagLen-((sMatch+(szLen-1))-m_pMyInfosTag));
			m_ui32MyInfosTagLen -= (uint32_t)szLen;
			m_ui32ZMyInfosTagLen = 0;
         }
    }

	if(m_pMyInfos) {
		char * sMatch = strstr(m_pMyInfos,  sMyInfo);
	    if(sMatch) {
    		memmove(sMatch, sMatch+szLen, m_ui32MyInfosLen-((sMatch+(szLen-1))-m_pMyInfos));
			m_ui32MyInfosLen -= (uint32_t)szLen;
			m_ui32ZMyInfosLen = 0;
         }
    }
}
//---------------------------------------------------------------------------

void Users::Add2UserIP(User * pUser) {
    int iRet = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$%s %s$", pUser->m_sNick, pUser->m_sIP);
    if(iRet <= 0) {
        return;
    }

    if(m_ui32UserIPListSize < m_ui32UserIPListLen+iRet) {
        char * pOldBuf = m_pUserIPList;
#ifdef _WIN32
		m_pUserIPList = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, m_ui32UserIPListSize+IPLISTSIZE+1);
#else
		m_pUserIPList = (char *)realloc(pOldBuf, m_ui32UserIPListSize+IPLISTSIZE+1);
#endif
        if(m_pUserIPList == NULL) {
			m_pUserIPList = pOldBuf;
            pUser->m_ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in Users::Add2UserIP\n", m_ui32UserIPListSize+IPLISTSIZE+1);

            return;
        }
		m_ui32UserIPListSize += IPLISTSIZE;
    }

    memcpy(m_pUserIPList+m_ui32UserIPListLen-1, ServerManager::m_pGlobalBuffer+1, iRet-1);
	m_ui32UserIPListLen += iRet;

	m_pUserIPList[m_ui32UserIPListLen-2] = '$';
	m_pUserIPList[m_ui32UserIPListLen-1] = '|';
	m_pUserIPList[m_ui32UserIPListLen] = '\0';

	m_ui32ZUserIPListLen = 0;
}
//---------------------------------------------------------------------------

void Users::DelFromUserIP(User * pUser) {
    int iRet = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$%s %s$", pUser->m_sNick, pUser->m_sIP);
    if(iRet <= 0) {
        return;
    }

	m_pUserIPList[7] = '$';
    char * sFound = strstr(m_pUserIPList, ServerManager::m_pGlobalBuffer);
	m_pUserIPList[7] = ' ';
    
	if(sFound != NULL) {
        memmove(sFound+1, sFound+(iRet+1), m_ui32UserIPListLen-((sFound+iRet)-m_pUserIPList));
		m_ui32UserIPListLen -= iRet;
		m_ui32ZUserIPListLen = 0;
    }
}
//---------------------------------------------------------------------------

void Users::Add2RecTimes(User * pUser) {
    time_t tmAccTime;
    time(&tmAccTime);

    if(ProfileManager::m_Ptr->IsAllowed(pUser, ProfileManager::NOUSRSAMEIP) == true || (tmAccTime-pUser->m_tLoginTime) >= SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_RECONN_TIME]) {
        return;
    }

    RecTime * pNewRecTime = new (std::nothrow) RecTime(pUser->m_ui128IpHash);

	if(pNewRecTime == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewRecTime in Users::Add2RecTimes\n");
        return;
	}

#ifdef _WIN32
    pNewRecTime->m_sNick = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, pUser->m_ui8NickLen+1);
#else
	pNewRecTime->m_sNick = (char *)malloc(pUser->m_ui8NickLen+1);
#endif
	if(pNewRecTime->m_sNick == NULL) {
        delete pNewRecTime;

        AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu8 " bytes in Users::Add2RecTimes\n", pUser->m_ui8NickLen+1);

        return;
    }

    memcpy(pNewRecTime->m_sNick, pUser->m_sNick, pUser->m_ui8NickLen);
    pNewRecTime->m_sNick[pUser->m_ui8NickLen] = '\0';

	pNewRecTime->m_ui64DisConnTick = ServerManager::m_ui64ActualTick-(tmAccTime-pUser->m_tLoginTime);
    pNewRecTime->m_ui32NickHash = pUser->m_ui32NickHash;

    pNewRecTime->m_pNext = m_pRecTimeList;

	if(m_pRecTimeList != NULL) {
		m_pRecTimeList->m_pPrev = pNewRecTime;
	}

	m_pRecTimeList = pNewRecTime;
}
//---------------------------------------------------------------------------

bool Users::CheckRecTime(User * pUser) {
    RecTime * pCur = NULL,
        * pNext = m_pRecTimeList;

    while(pNext != NULL) {
        pCur = pNext;
        pNext = pCur->m_pNext;

        // check expires...
        if(pCur->m_ui64DisConnTick+SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_RECONN_TIME] <= ServerManager::m_ui64ActualTick) {
#ifdef _WIN32
            if(pCur->m_sNick != NULL) {
                if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->m_sNick) == 0) {
                    AppendDebugLog("%s - [MEM] Cannot deallocate pCur->m_sNick in Users::CheckRecTime\n");
                }
            }
#else
			free(pCur->m_sNick);
#endif

            if(pCur->m_pPrev == NULL) {
                if(pCur->m_pNext == NULL) {
					m_pRecTimeList = NULL;
                } else {
                    pCur->m_pNext->m_pPrev = NULL;
					m_pRecTimeList = pCur->m_pNext;
                }
            } else if(pCur->m_pNext == NULL) {
                pCur->m_pPrev->m_pNext = NULL;
            } else {
                pCur->m_pPrev->m_pNext = pCur->m_pNext;
                pCur->m_pNext->m_pPrev = pCur->m_pPrev;
            }

            delete pCur;
            continue;
        }

        if(pCur->m_ui32NickHash == pUser->m_ui32NickHash && memcmp(pCur->m_ui128IpHash, pUser->m_ui128IpHash, 16) == 0 && strcasecmp(pCur->m_sNick, pUser->m_sNick) == 0) {
            pUser->SendFormat("Users::CheckRecTime", false, "<%s> %s %" PRIu64 " %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_PLEASE_WAIT],
                (pCur->m_ui64DisConnTick+SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_RECONN_TIME])-ServerManager::m_ui64ActualTick, LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_BEFORE_RECONN]);

            return true;
        }
    }

    return false;
}
//---------------------------------------------------------------------------
