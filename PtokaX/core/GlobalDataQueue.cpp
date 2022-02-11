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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "stdinc.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "GlobalDataQueue.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "colUsers.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
#include "ZlibUtility.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
GlobalDataQueue * GlobalDataQueue::m_Ptr = NULL;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

GlobalDataQueue::GlobalDataQueue() : m_pCreatedGlobalQueues(NULL), m_pQueueItems(NULL), m_pSingleItems(NULL), m_bHaveItems(false) {
    // OpList buffer
#ifdef _WIN32
	m_OpListQueue.m_pBuffer = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, 256);
#else
	m_OpListQueue.m_pBuffer = (char *)calloc(256, 1);
#endif
	if(m_OpListQueue.m_pBuffer == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create m_OpListQueue\n");
		exit(EXIT_FAILURE);
	}
	m_OpListQueue.m_szLen = 0;
	m_OpListQueue.m_szSize = 255;
    
    // UserIP buffer
#ifdef _WIN32
	m_UserIPQueue.m_pBuffer = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, 256);
#else
	m_UserIPQueue.m_pBuffer = (char *)calloc(256, 1);
#endif
	if(m_UserIPQueue.m_pBuffer == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create m_UserIPQueue\n");
		exit(EXIT_FAILURE);
	}
	m_UserIPQueue.m_szLen = 0;
	m_UserIPQueue.m_szSize = 255;
	m_UserIPQueue.m_bHaveDollars = false;

	m_pNewQueueItems[0] = NULL;
	m_pNewQueueItems[1] = NULL;
    
	m_pNewSingleItems[0] = NULL;
	m_pNewSingleItems[1] = NULL;

    for(uint8_t ui8i = 0; ui8i < 144; ui8i++) {
		m_GlobalQueues[ui8i].m_szLen = 0;
		m_GlobalQueues[ui8i].m_szSize = 255;
		m_GlobalQueues[ui8i].m_szZlen = 0;
		m_GlobalQueues[ui8i].m_szZsize = 255;

#ifdef _WIN32
		m_GlobalQueues[ui8i].m_pBuffer = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, 256);
		m_GlobalQueues[ui8i].m_pZbuffer = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, 256);
#else
		m_GlobalQueues[ui8i].m_pBuffer = (char *)calloc(256, 1);
		m_GlobalQueues[ui8i].m_pZbuffer = (char *)calloc(256, 1);
#endif
        if(m_GlobalQueues[ui8i].m_pBuffer == NULL || m_GlobalQueues[ui8i].m_pZbuffer == NULL) {
            AppendDebugLog("%s - [MEM] Cannot create m_GlobalQueues[ui8i]\n");
            exit(EXIT_FAILURE);
        }

		m_GlobalQueues[ui8i].m_pNext = NULL;
		m_GlobalQueues[ui8i].m_bCreated = false;
		m_GlobalQueues[ui8i].m_bZlined = false;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

GlobalDataQueue::~GlobalDataQueue() {
#ifdef _WIN32
    if(m_OpListQueue.m_pBuffer != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_OpListQueue.m_pBuffer) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_OpListQueue.m_pBuffer in GlobalDataQueue::~GlobalDataQueue\n");
        }
    }
#else
	free(m_OpListQueue.m_pBuffer);
#endif

#ifdef _WIN32
    if(m_UserIPQueue.m_pBuffer != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_UserIPQueue.m_pBuffer) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_UserIPQueue.m_pBuffer in GlobalDataQueue::~GlobalDataQueue\n");
        }
    }
#else
	free(m_UserIPQueue.m_pBuffer);
#endif

    if(m_pNewSingleItems[0] != NULL) {
        SingleDataItem * pCur = NULL,
            * pNext = m_pNewSingleItems[0];

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->m_pNext;

#ifdef _WIN32
            if(pCur->m_pData != NULL) {
                if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->m_pData) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->m_pData in GlobalDataQueue::~GlobalDataQueue\n");
                }
            }
#else
			free(pCur->m_pData);
#endif
            delete pCur;
		}
    }

    if(m_pSingleItems != NULL) {
        SingleDataItem * pCur = NULL,
            * pNext = m_pSingleItems;

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->m_pNext;

#ifdef _WIN32
            if(pCur->m_pData != NULL) {
                if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->m_pData) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->m_pData in GlobalDataQueue::~GlobalDataQueue\n");
                }
            }
#else
			free(pCur->m_pData);
#endif
            delete pCur;
		}
    }

    if(m_pNewQueueItems[0] != NULL) {
        QueueItem * pCur = NULL,
            * pNext = m_pNewQueueItems[0];

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->m_pNext;

#ifdef _WIN32
            if(pCur->m_pCommand1 != NULL) {
                if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->m_pCommand1) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->m_pCommand1 in GlobalDataQueue::~GlobalDataQueue\n");
                }
            }
            if(pCur->m_pCommand2 != NULL) {
                if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->m_pCommand2) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->m_pCommand2 in GlobalDataQueue::~GlobalDataQueue\n");
                }
            }
#else
			free(pCur->m_pCommand1);
			free(pCur->m_pCommand2);
#endif
            delete pCur;
		}
    }

    if(m_pQueueItems != NULL) {
        QueueItem * pCur = NULL,
            * pNext = m_pQueueItems;

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->m_pNext;

#ifdef _WIN32
            if(pCur->m_pCommand1 != NULL) {
                if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->m_pCommand1) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->m_pCommand1 in GlobalDataQueue::~GlobalDataQueue\n");
                }
            }
            if(pCur->m_pCommand2 != NULL) {
                if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->m_pCommand2) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->m_pCommand2 in GlobalDataQueue::~GlobalDataQueue\n");
                }
            }
#else
			free(pCur->m_pCommand1);
			free(pCur->m_pCommand2);
#endif
            delete pCur;
		}
    }

    for(uint8_t ui8i = 0; ui8i < 144; ui8i++) {
#ifdef _WIN32
        if(m_GlobalQueues[ui8i].m_pBuffer != NULL) {
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_GlobalQueues[ui8i].m_pBuffer) == 0) {
				AppendDebugLog("%s - [MEM] Cannot deallocate m_GlobalQueues[ui8i].m_pBuffer in GlobalDataQueue::~GlobalDataQueue\n");
            }
        }

        if(m_GlobalQueues[ui8i].m_pZbuffer != NULL) {
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_GlobalQueues[ui8i].m_pZbuffer) == 0) {
				AppendDebugLog("%s - [MEM] Cannot deallocate m_GlobalQueues[ui8i].m_pZbuffer in GlobalDataQueue::~GlobalDataQueue\n");
            }
        }
#else
		free(m_GlobalQueues[ui8i].m_pBuffer);
		free(m_GlobalQueues[ui8i].m_pZbuffer);
#endif
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::AddQueueItem(char * sCommand1, const size_t szLen1, char * sCommand2, const size_t szLen2, const uint8_t ui8CmdType) {
    QueueItem * pNewItem = new (std::nothrow) QueueItem();
    if(pNewItem == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewItem in GlobalDataQueue::AddQueueItem\n");
    	return;
    }

#ifdef _WIN32
    pNewItem->m_pCommand1 = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szLen1+1);
#else
	pNewItem->m_pCommand1 = (char *)malloc(szLen1+1);
#endif
    if(pNewItem->m_pCommand1 == NULL) {
        delete pNewItem;

		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for pNewItem->m_pCommand1 in GlobalDataQueue::AddQueueItem\n", szLen1+1);

        return;
    }

    memcpy(pNewItem->m_pCommand1, sCommand1, szLen1);
    pNewItem->m_pCommand1[szLen1] = '\0';

	pNewItem->m_szLen1 = szLen1;

    if(sCommand2 != NULL) {
#ifdef _WIN32
        pNewItem->m_pCommand2 = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szLen2+1);
#else
        pNewItem->m_pCommand2 = (char *)malloc(szLen2+1);
#endif
        if(pNewItem->m_pCommand2 == NULL) {
#ifdef _WIN32
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pNewItem->m_pCommand1) == 0) {
				AppendDebugLog("%s - [MEM] Cannot deallocate pNewItem->m_pCommand1 in GlobalDataQueue::AddQueueItem\n");
            }
#else
            free(pNewItem->m_pCommand1);
#endif
            delete pNewItem;

            AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for pNewItem->m_pCommand2 in GlobalDataQueue::AddQueueItem\n", szLen2+1);

            return;
        }

        memcpy(pNewItem->m_pCommand2, sCommand2, szLen2);
        pNewItem->m_pCommand2[szLen2] = '\0';

        pNewItem->m_szLen2 = szLen2;
    } else {
        pNewItem->m_pCommand2 = NULL;
        pNewItem->m_szLen2 = 0;
    }

	pNewItem->m_ui8CommandType = ui8CmdType;

	pNewItem->m_pNext = NULL;

    if(m_pNewQueueItems[0] == NULL) {
		m_pNewQueueItems[0] = pNewItem;
		m_pNewQueueItems[1] = pNewItem;
    } else {
		m_pNewQueueItems[1]->m_pNext = pNewItem;
		m_pNewQueueItems[1] = pNewItem;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// appends data to the OpListQueue
void GlobalDataQueue::OpListStore(char * sNick) {
    if(m_OpListQueue.m_szLen == 0) {
		int iLen = snprintf(m_OpListQueue.m_pBuffer, m_OpListQueue.m_szSize, "$OpList %s$$|", sNick);
		if(iLen <= 0) {
			m_OpListQueue.m_szLen = 0;
		} else {
			m_OpListQueue.m_szLen = iLen;
		}
    } else {
    	size_t szNickLen = strlen(sNick) + 3;
		if(m_OpListQueue.m_szSize < m_OpListQueue.m_szLen+szNickLen) {
            size_t szAllignLen = Allign256(m_OpListQueue.m_szLen+szNickLen);
            char * pOldBuf = m_OpListQueue.m_pBuffer;
#ifdef _WIN32
			m_OpListQueue.m_pBuffer = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, szAllignLen);
#else
			m_OpListQueue.m_pBuffer = (char *)realloc(pOldBuf, szAllignLen);
#endif
            if(m_OpListQueue.m_pBuffer == NULL) {
				m_OpListQueue.m_pBuffer = pOldBuf;

                AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes in GlobalDataQueue::OpListStore\n", szAllignLen);

                return;
            }
			m_OpListQueue.m_szSize = (uint32_t)(szAllignLen-1);
        }

        int iDataLen = snprintf(m_OpListQueue.m_pBuffer+m_OpListQueue.m_szLen-1, m_OpListQueue.m_szSize-(m_OpListQueue.m_szLen-1), "%s$$|", sNick);
		if(iDataLen <= 0) {
			m_OpListQueue.m_pBuffer[m_OpListQueue.m_szLen-1] = '|';
			m_OpListQueue.m_pBuffer[m_OpListQueue.m_szLen] = '\0';
		} else {
			m_OpListQueue.m_szLen += iDataLen-1;
		}
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// appends data to the UserIPQueue
void GlobalDataQueue::UserIPStore(User * pUser) {
    if(m_UserIPQueue.m_szLen == 0) {
		m_UserIPQueue.m_szLen = snprintf(m_UserIPQueue.m_pBuffer, m_UserIPQueue.m_szSize, "$UserIP %s %s|", pUser->m_sNick, pUser->m_sIP);
		if(m_UserIPQueue.m_szLen > 0) {
			m_UserIPQueue.m_bHaveDollars = false;
		} else {
			m_UserIPQueue.m_szLen = 0;
		}
    } else {
		size_t szLen = pUser->m_ui8NickLen + pUser->m_ui8IpLen + 4;
        if(m_UserIPQueue.m_szSize < m_UserIPQueue.m_szLen+szLen) {
            size_t szAllignLen = Allign256(m_UserIPQueue.m_szLen+szLen);
            char * pOldBuf = m_UserIPQueue.m_pBuffer;
#ifdef _WIN32
			m_UserIPQueue.m_pBuffer = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, szAllignLen);
#else
			m_UserIPQueue.m_pBuffer = (char *)realloc(pOldBuf, szAllignLen);
#endif
            if(m_UserIPQueue.m_pBuffer == NULL) {
				m_UserIPQueue.m_pBuffer = pOldBuf;

				AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes in GlobalDataQueue::UserIPStore\n", szAllignLen);

                return;
            }
			m_UserIPQueue.m_szSize = (uint32_t)(szAllignLen-1);
        }

        if(m_UserIPQueue.m_bHaveDollars == false) {
			m_UserIPQueue.m_pBuffer[m_UserIPQueue.m_szLen-1] = '$';
			m_UserIPQueue.m_pBuffer[m_UserIPQueue.m_szLen] = '$';
			m_UserIPQueue.m_bHaveDollars = true;
			m_UserIPQueue.m_szLen += 2;
        }

		int iDataLen = snprintf(m_UserIPQueue.m_pBuffer+m_UserIPQueue.m_szLen-1, m_UserIPQueue.m_szSize-(m_UserIPQueue.m_szLen-1), "%s %s$$|", pUser->m_sNick, pUser->m_sIP);
		if(iDataLen <= 0) {
			m_UserIPQueue.m_pBuffer[m_UserIPQueue.m_szLen-1] = '|';
			m_UserIPQueue.m_pBuffer[m_UserIPQueue.m_szLen] = '\0';
		} else {
			m_UserIPQueue.m_szLen += iDataLen-1;
		}
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::PrepareQueueItems() {
	m_pQueueItems = m_pNewQueueItems[0];

	m_pNewQueueItems[0] = NULL;
	m_pNewQueueItems[1] = NULL;

    if(m_pQueueItems != NULL || m_OpListQueue.m_szLen != 0 || m_UserIPQueue.m_szLen != 0) {
		m_bHaveItems = true;
    }

	m_pSingleItems = m_pNewSingleItems[0];

	m_pNewSingleItems[0] = NULL;
	m_pNewSingleItems[1] = NULL;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::ClearQueues() {
	m_bHaveItems = false;

    if(m_pCreatedGlobalQueues != NULL) {
        GlobalQueue * pCur = NULL,
            * pNext = m_pCreatedGlobalQueues;

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->m_pNext;

            pCur->m_szLen = 0;
            pCur->m_szZlen = 0;
            pCur->m_pNext = NULL;
            pCur->m_bCreated = false;
            pCur->m_bZlined = false;
        }
    }

	m_pCreatedGlobalQueues = NULL;

	m_OpListQueue.m_pBuffer[0] = '\0';
	m_OpListQueue.m_szLen = 0;

	m_UserIPQueue.m_pBuffer[0] = '\0';
	m_UserIPQueue.m_szLen = 0;

    if(m_pQueueItems != NULL) {
        QueueItem * pCur = NULL,
            * pNext = m_pQueueItems;

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->m_pNext;

#ifdef _WIN32
            if(pCur->m_pCommand1 != NULL) {
                if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->m_pCommand1) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->m_pCommand1 in GlobalDataQueue::ClearQueues\n");
                }
            }
            if(pCur->m_pCommand2 != NULL) {
                if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->m_pCommand2) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->m_pCommand2 in GlobalDataQueue::ClearQueues\n");
                }
            }
#else
			free(pCur->m_pCommand1);
			free(pCur->m_pCommand2);
#endif
            delete pCur;
		}
    }

	m_pQueueItems = NULL;

    if(m_pSingleItems != NULL) {
        SingleDataItem * pCur = NULL,
            * pNext = m_pSingleItems;

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->m_pNext;

#ifdef _WIN32
            if(pCur->m_pData != NULL) {
                if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pCur->m_pData) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate pCur->m_pData in GlobalDataQueue::ClearQueues\n");
                }
            }
#else
			free(pCur->m_pData);
#endif
            delete pCur;
		}
    }

	m_pSingleItems = NULL;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::ProcessQueues(User * pUser) {
    uint32_t ui32QueueType = 0; // short myinfos
    uint16_t ui16QueueBits = 0;

    if(SettingManager::m_Ptr->m_ui8FullMyINFOOption == 1 && ProfileManager::m_Ptr->IsAllowed(pUser, ProfileManager::SENDFULLMYINFOS)) {
        ui32QueueType = 1; // full myinfos
        ui16QueueBits |= BIT_LONG_MYINFO;
    }

    if(pUser->m_ui64SharedSize != 0) {
        if(((pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) == true) {
            if(((pUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) == true) {
                if(((pUser->m_ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE) == true) {
                    if(((pUser->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE) == true) {
                        ui32QueueType += 6; // all searches both ipv4 and ipv6
                        ui16QueueBits |= BIT_ALL_SEARCHES_IPV64;
                    } else {
                        ui32QueueType += 14; // all searches ipv6 + active searches ipv4
                        ui16QueueBits |= BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4;
                    }
                } else {
                    if(((pUser->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE) == true) {
                        ui32QueueType += 16; // active searches ipv6 + all searches ipv4
                        ui16QueueBits |= BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4;
                    } else {
                        ui32QueueType += 12; // active searches both ipv4 and ipv6
                        ui16QueueBits |= BIT_ACTIVE_SEARCHES_IPV64;
                    }
                }
            } else {
                if(((pUser->m_ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE) == true) {
                    ui32QueueType += 4; // all searches ipv6 only
                    ui16QueueBits |= BIT_ALL_SEARCHES_IPV6;
                } else {
                    ui32QueueType += 10; // active searches ipv6 only
                    ui16QueueBits |= BIT_ACTIVE_SEARCHES_IPV6;
                }
            }
        } else {
            if(((pUser->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE) == true) {
                ui32QueueType += 2; // all searches ipv4 only
                ui16QueueBits |= BIT_ALL_SEARCHES_IPV4;
            } else {
                ui32QueueType += 8; // active searches ipv4 only
                ui16QueueBits |= BIT_ACTIVE_SEARCHES_IPV4;
            }
        }
    }

    if(((pUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false) {
        ui32QueueType += 14; // send hellos
        ui16QueueBits |= BIT_HELLO;
    }

    if(((pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
        ui32QueueType += 28; // send operator data
        ui16QueueBits |= BIT_OPERATOR;
    }

    if(pUser->m_i32Profile != -1 && ((pUser->m_ui32SupportBits & User::SUPPORTBIT_USERIP2) == User::SUPPORTBIT_USERIP2) == true &&
        ProfileManager::m_Ptr->IsAllowed(pUser, ProfileManager::SENDALLUSERIP) == true) {
        ui32QueueType += 56; // send userips
        ui16QueueBits |= BIT_USERIP;
    }

    if(m_GlobalQueues[ui32QueueType].m_bCreated == false) {
        if(m_pQueueItems != NULL) {
            QueueItem * pCur = NULL,
                * pNext = m_pQueueItems;

            while(pNext != NULL) {
                pCur = pNext;
                pNext = pCur->m_pNext;

                switch(pCur->m_ui8CommandType) {
                    case CMD_HELLO:
                        if((ui16QueueBits & BIT_HELLO) == BIT_HELLO) {
                            AddDataToQueue(m_GlobalQueues[ui32QueueType], pCur->m_pCommand1, pCur->m_szLen1);
                        }
                        break;
                    case CMD_MYINFO:
                        if((ui16QueueBits & BIT_LONG_MYINFO) == BIT_LONG_MYINFO) {
                            if(pCur->m_pCommand2 != NULL) {
                                AddDataToQueue(m_GlobalQueues[ui32QueueType], pCur->m_pCommand2, pCur->m_szLen2);
                            }
                            break;
                        }

                        if(pCur->m_pCommand1 != NULL) {
                            AddDataToQueue(m_GlobalQueues[ui32QueueType], pCur->m_pCommand1, pCur->m_szLen1);
                        }

                        break;
                    case CMD_OPS:
                        if((ui16QueueBits & BIT_OPERATOR) == BIT_OPERATOR) {
                            AddDataToQueue(m_GlobalQueues[ui32QueueType], pCur->m_pCommand1, pCur->m_szLen1);
                        }
                        break;
                    case CMD_ACTIVE_SEARCH_V6:
                        if(pUser->m_ui64SharedSize != 0 && (((ui16QueueBits & BIT_ALL_SEARCHES_IPV4) == BIT_ALL_SEARCHES_IPV4) == false && ((ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV4) == BIT_ACTIVE_SEARCHES_IPV4) == false)) {
                            AddDataToQueue(m_GlobalQueues[ui32QueueType], pCur->m_pCommand1, pCur->m_szLen1);
                        }
                        break;
                    case CMD_ACTIVE_SEARCH_V64:
                    	if(pUser->m_ui64SharedSize != 0) {
	                        if(((ui16QueueBits & BIT_ALL_SEARCHES_IPV4) == BIT_ALL_SEARCHES_IPV4) == false && ((ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV4) == BIT_ACTIVE_SEARCHES_IPV4) == false) {
	                            AddDataToQueue(m_GlobalQueues[ui32QueueType], pCur->m_pCommand1, pCur->m_szLen1);
	                        } else if(((ui16QueueBits & BIT_ALL_SEARCHES_IPV6) == BIT_ALL_SEARCHES_IPV6) == false && ((ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV6) == BIT_ACTIVE_SEARCHES_IPV6) == false) {
	                            AddDataToQueue(m_GlobalQueues[ui32QueueType], pCur->m_pCommand2, pCur->m_szLen2);
	                        }
	                    }
                        break;
                    case CMD_ACTIVE_SEARCH_V4:
                        if(pUser->m_ui64SharedSize != 0 && (((ui16QueueBits & BIT_ALL_SEARCHES_IPV6) == BIT_ALL_SEARCHES_IPV6) == false && ((ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV6) == BIT_ACTIVE_SEARCHES_IPV6) == false)) {
                            AddDataToQueue(m_GlobalQueues[ui32QueueType], pCur->m_pCommand1, pCur->m_szLen1);
                        }
                        break;
                    case CMD_PASSIVE_SEARCH_V6:
                        if(pUser->m_ui64SharedSize != 0 && ((ui16QueueBits & BIT_ALL_SEARCHES_IPV6) == BIT_ALL_SEARCHES_IPV6 || (ui16QueueBits & BIT_ALL_SEARCHES_IPV64) == BIT_ALL_SEARCHES_IPV64 || (ui16QueueBits & BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4) == BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4)) {
                            AddDataToQueue(m_GlobalQueues[ui32QueueType], pCur->m_pCommand1, pCur->m_szLen1);
                        }
                        break;
                    case CMD_PASSIVE_SEARCH_V64:
                        if(pUser->m_ui64SharedSize != 0 && ((ui16QueueBits & BIT_ALL_SEARCHES_IPV64) == BIT_ALL_SEARCHES_IPV64 || (ui16QueueBits & BIT_ALL_SEARCHES_IPV6) == BIT_ALL_SEARCHES_IPV6 || (ui16QueueBits & BIT_ALL_SEARCHES_IPV4) == BIT_ALL_SEARCHES_IPV4 || 
							(ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4) == BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4 || (ui16QueueBits & BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4) == BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4)) {
                            AddDataToQueue(m_GlobalQueues[ui32QueueType], pCur->m_pCommand1, pCur->m_szLen1);
                        }
                        break;
                    case CMD_PASSIVE_SEARCH_V4:
                        if(pUser->m_ui64SharedSize != 0 && ((ui16QueueBits & BIT_ALL_SEARCHES_IPV4) == BIT_ALL_SEARCHES_IPV4 || (ui16QueueBits & BIT_ALL_SEARCHES_IPV64) == BIT_ALL_SEARCHES_IPV64 || (ui16QueueBits & BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4) == BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4)) {
                            AddDataToQueue(m_GlobalQueues[ui32QueueType], pCur->m_pCommand1, pCur->m_szLen1);
                        }
                        break;
                    case CMD_PASSIVE_SEARCH_V4_ONLY:
                        if(pUser->m_ui64SharedSize != 0 && ((ui16QueueBits & BIT_ALL_SEARCHES_IPV4) == BIT_ALL_SEARCHES_IPV4)) {
                            AddDataToQueue(m_GlobalQueues[ui32QueueType], pCur->m_pCommand1, pCur->m_szLen1);
                        }
                        break;
                    case CMD_PASSIVE_SEARCH_V6_ONLY:
                        if(pUser->m_ui64SharedSize != 0 && ((ui16QueueBits & BIT_ALL_SEARCHES_IPV6) == BIT_ALL_SEARCHES_IPV6)) {
                            AddDataToQueue(m_GlobalQueues[ui32QueueType], pCur->m_pCommand1, pCur->m_szLen1);
                        }
                        break;
                    case CMD_CHAT:
                        if(pCur->m_pCommand1 != NULL) {
                            AddDataToQueue(m_GlobalQueues[ui32QueueType], pCur->m_pCommand1, pCur->m_szLen1);
                        }
                        break;
                    case CMD_HUBNAME:
                    case CMD_QUIT:
                    case CMD_LUA:
                        AddDataToQueue(m_GlobalQueues[ui32QueueType], pCur->m_pCommand1, pCur->m_szLen1);
                        break;
                    default:
                        break; // should not happen ;)
                }
            }
        }

        if(m_OpListQueue.m_szLen != 0) {
            AddDataToQueue(m_GlobalQueues[ui32QueueType], m_OpListQueue.m_pBuffer, m_OpListQueue.m_szLen);
        }

        if(m_UserIPQueue.m_szLen != 0 && (ui16QueueBits & BIT_USERIP) == BIT_USERIP) {
            AddDataToQueue(m_GlobalQueues[ui32QueueType], m_UserIPQueue.m_pBuffer, m_UserIPQueue.m_szLen);
        }

		m_GlobalQueues[ui32QueueType].m_bCreated = true;
		m_GlobalQueues[ui32QueueType].m_pNext = m_pCreatedGlobalQueues;
		m_pCreatedGlobalQueues = &m_GlobalQueues[ui32QueueType];
    }

    if(m_GlobalQueues[ui32QueueType].m_szLen == 0) {
        if(ServerManager::m_ui8SrCntr == 0) {
            if(pUser->m_pCmdActive6Search != NULL) {
				User::DeletePrcsdUsrCmd(pUser->m_pCmdActive6Search);
                pUser->m_pCmdActive6Search = NULL;
            }

            if(pUser->m_pCmdActive4Search != NULL) {
				User::DeletePrcsdUsrCmd(pUser->m_pCmdActive4Search);
                pUser->m_pCmdActive4Search = NULL;
            }
        }

        return;
    }

    if(ServerManager::m_ui8SrCntr == 0) {
    	if(pUser->m_ui64SharedSize == 0) {
    		if(pUser->m_pCmdActive6Search != NULL) {
				User::DeletePrcsdUsrCmd(pUser->m_pCmdActive6Search);
				pUser->m_pCmdActive6Search = NULL;
			}

    		if(pUser->m_pCmdActive4Search != NULL) {
				User::DeletePrcsdUsrCmd(pUser->m_pCmdActive4Search);
				pUser->m_pCmdActive4Search = NULL;
			}
    	} else {
			if((pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
				if(((pUser->m_ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == true) {
					if(m_GlobalQueues[ui32QueueType].m_bZlined == false) {
						m_GlobalQueues[ui32QueueType].m_bZlined = true;
						m_GlobalQueues[ui32QueueType].m_pZbuffer = ZlibUtility::m_Ptr->CreateZPipe(m_GlobalQueues[ui32QueueType].m_pBuffer, m_GlobalQueues[ui32QueueType].m_szLen,
							m_GlobalQueues[ui32QueueType].m_pZbuffer, m_GlobalQueues[ui32QueueType].m_szZlen, m_GlobalQueues[ui32QueueType].m_szZsize);
					}
		
					size_t szSearchLens = 0;
					if(pUser->m_pCmdActive6Search != NULL) {
						szSearchLens += pUser->m_pCmdActive6Search->m_ui32Len;
					}
					if(pUser->m_pCmdActive4Search != NULL) {
						szSearchLens += pUser->m_pCmdActive4Search->m_ui32Len;
					}
							
					if(m_GlobalQueues[ui32QueueType].m_szZlen != 0 && (m_GlobalQueues[ui32QueueType].m_szZlen <= (m_GlobalQueues[ui32QueueType].m_szLen-szSearchLens))) {
						pUser->PutInSendBuf(m_GlobalQueues[ui32QueueType].m_pZbuffer, m_GlobalQueues[ui32QueueType].m_szZlen);
						ServerManager::m_ui64BytesSentSaved += (m_GlobalQueues[ui32QueueType].m_szLen - m_GlobalQueues[ui32QueueType].m_szZlen);
		
						if(pUser->m_pCmdActive6Search != NULL) {
							User::DeletePrcsdUsrCmd(pUser->m_pCmdActive6Search);
							pUser->m_pCmdActive6Search = NULL;
						}
		
						if(pUser->m_pCmdActive4Search != NULL) {
							User::DeletePrcsdUsrCmd(pUser->m_pCmdActive4Search);
							pUser->m_pCmdActive4Search = NULL;
						}
		
						return;
					}
				}

				uint32_t ui32SbLen = pUser->m_ui32SendBufDataLen;
				pUser->PutInSendBuf(m_GlobalQueues[ui32QueueType].m_pBuffer, m_GlobalQueues[ui32QueueType].m_szLen);

				// PPK ... check if adding of searchs not cause buffer overflow !
				if(pUser->m_ui32SendBufDataLen <= ui32SbLen) {
					ui32SbLen = 0;
				}

				if(pUser->m_pCmdActive6Search != NULL) {
					pUser->RemFromSendBuf(pUser->m_pCmdActive6Search->m_sCommand, pUser->m_pCmdActive6Search->m_ui32Len, ui32SbLen);
	
					User::DeletePrcsdUsrCmd(pUser->m_pCmdActive6Search);
					pUser->m_pCmdActive6Search = NULL;
				}

				if(pUser->m_pCmdActive4Search != NULL) {
					pUser->RemFromSendBuf(pUser->m_pCmdActive4Search->m_sCommand, pUser->m_pCmdActive4Search->m_ui32Len, ui32SbLen);
	
					User::DeletePrcsdUsrCmd(pUser->m_pCmdActive4Search);
					pUser->m_pCmdActive4Search = NULL;
				}

				return;
			} else {
				if(pUser->m_pCmdActive4Search != NULL) {
					if(((pUser->m_ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == true) {
						if(m_GlobalQueues[ui32QueueType].m_bZlined == false) {
							m_GlobalQueues[ui32QueueType].m_bZlined = true;
							m_GlobalQueues[ui32QueueType].m_pZbuffer = ZlibUtility::m_Ptr->CreateZPipe(m_GlobalQueues[ui32QueueType].m_pBuffer, m_GlobalQueues[ui32QueueType].m_szLen,
								m_GlobalQueues[ui32QueueType].m_pZbuffer, m_GlobalQueues[ui32QueueType].m_szZlen, m_GlobalQueues[ui32QueueType].m_szZsize);
						}
	
						if(m_GlobalQueues[ui32QueueType].m_szZlen != 0 && (m_GlobalQueues[ui32QueueType].m_szZlen <= (m_GlobalQueues[ui32QueueType].m_szLen-pUser->m_pCmdActive4Search->m_ui32Len))) {
							pUser->PutInSendBuf(m_GlobalQueues[ui32QueueType].m_pZbuffer, m_GlobalQueues[ui32QueueType].m_szZlen);
							ServerManager::m_ui64BytesSentSaved += (m_GlobalQueues[ui32QueueType].m_szLen - m_GlobalQueues[ui32QueueType].m_szZlen);
	
							User::DeletePrcsdUsrCmd(pUser->m_pCmdActive4Search);
							pUser->m_pCmdActive4Search = NULL;
	
							return;
						}
					}
	
					uint32_t ui32SbLen = pUser->m_ui32SendBufDataLen;
					pUser->PutInSendBuf(m_GlobalQueues[ui32QueueType].m_pBuffer, m_GlobalQueues[ui32QueueType].m_szLen);
	
					// PPK ... check if adding of searchs not cause buffer overflow !
					if(pUser->m_ui32SendBufDataLen <= ui32SbLen) {
						ui32SbLen = 0;
					}
	
					pUser->RemFromSendBuf(pUser->m_pCmdActive4Search->m_sCommand, pUser->m_pCmdActive4Search->m_ui32Len, ui32SbLen);
	
					User::DeletePrcsdUsrCmd(pUser->m_pCmdActive4Search);
					pUser->m_pCmdActive4Search = NULL;
	
					return;
				}
			}
		}
	}

    if(((pUser->m_ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == true) {
        if(m_GlobalQueues[ui32QueueType].m_bZlined == false) {
			m_GlobalQueues[ui32QueueType].m_bZlined = true;
			m_GlobalQueues[ui32QueueType].m_pZbuffer = ZlibUtility::m_Ptr->CreateZPipe(m_GlobalQueues[ui32QueueType].m_pBuffer, m_GlobalQueues[ui32QueueType].m_szLen, m_GlobalQueues[ui32QueueType].m_pZbuffer,
				m_GlobalQueues[ui32QueueType].m_szZlen, m_GlobalQueues[ui32QueueType].m_szZsize);
        }

        if(m_GlobalQueues[ui32QueueType].m_szZlen != 0) {
            pUser->PutInSendBuf(m_GlobalQueues[ui32QueueType].m_pZbuffer, m_GlobalQueues[ui32QueueType].m_szZlen);
            ServerManager::m_ui64BytesSentSaved += (m_GlobalQueues[ui32QueueType].m_szLen - m_GlobalQueues[ui32QueueType].m_szZlen);

            return;
        }
    } 

    pUser->PutInSendBuf(m_GlobalQueues[ui32QueueType].m_pBuffer, m_GlobalQueues[ui32QueueType].m_szLen);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::ProcessSingleItems(User * pUser) const {
    size_t szLen = 0, szWanted = 0;
    int iRet = 0;

    SingleDataItem * pCur = NULL,
        * pNext = m_pSingleItems;

    while(pNext != NULL) {
        pCur = pNext;
        pNext = pCur->m_pNext;

        if(pCur->m_pFromUser != pUser) {
            switch(pCur->m_ui8Type) {
                case SI_PM2ALL: { // send PM to ALL
                    szWanted = szLen+pCur->m_szDataLen+pUser->m_ui8NickLen+13;
                    if(ServerManager::m_szGlobalBufferSize < szWanted) {
                        if(CheckAndResizeGlobalBuffer(szWanted) == false) {
							AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes in GlobalDataQueue::ProcessSingleItems\n", Allign128K(szWanted));
                            break;
                        }
                    }
                    iRet = snprintf(ServerManager::m_pGlobalBuffer+szLen, ServerManager::m_szGlobalBufferSize-szLen, "$To: %s From: ", pUser->m_sNick);
                    if(iRet > 0) {
	                    szLen += iRet;

	                    memcpy(ServerManager::m_pGlobalBuffer+szLen, pCur->m_pData, pCur->m_szDataLen);
	                    szLen += pCur->m_szDataLen;
	                    ServerManager::m_pGlobalBuffer[szLen] = '\0';
	                }

                    break;
                }
                case SI_PM2OPS: { // send PM only to operators
                    if(((pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                        szWanted = szLen+pCur->m_szDataLen+pUser->m_ui8NickLen+13;
                        if(ServerManager::m_szGlobalBufferSize < szWanted) {
                            if(CheckAndResizeGlobalBuffer(szWanted) == false) {
								AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes in GlobalDataQueue::ProcessSingleItems1\n", Allign128K(szWanted));
								break;
                            }
                        }
                        iRet = snprintf(ServerManager::m_pGlobalBuffer+szLen, ServerManager::m_szGlobalBufferSize-szLen, "$To: %s From: ", pUser->m_sNick);
                        if(iRet > 0) {
	                        szLen += iRet;

	                        memcpy(ServerManager::m_pGlobalBuffer+szLen, pCur->m_pData, pCur->m_szDataLen);
	                        szLen += pCur->m_szDataLen;
	                        ServerManager::m_pGlobalBuffer[szLen] = '\0';
	                    }
                    }
                    break;
                }
                case SI_OPCHAT: { // send OpChat only to allowed users...
                    if(ProfileManager::m_Ptr->IsAllowed(pUser, ProfileManager::ALLOWEDOPCHAT) == true) {
                        szWanted = szLen+pCur->m_szDataLen+pUser->m_ui8NickLen+13;
                        if(ServerManager::m_szGlobalBufferSize < szWanted) {
                            if(CheckAndResizeGlobalBuffer(szWanted) == false) {
								AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes in GlobalDataQueue::ProcessSingleItems2\n", Allign128K(szWanted));
                                break;
                            }
                        }
                        iRet = snprintf(ServerManager::m_pGlobalBuffer+szLen, ServerManager::m_szGlobalBufferSize-szLen, "$To: %s From: ", pUser->m_sNick);
                        if(iRet > 0) {
	                        szLen += iRet;
	
	                        memcpy(ServerManager::m_pGlobalBuffer+szLen, pCur->m_pData, pCur->m_szDataLen);
	                        szLen += pCur->m_szDataLen;
	                        ServerManager::m_pGlobalBuffer[szLen] = '\0';
	                    }
                    }
                    break;
                }
                case SI_TOPROFILE: { // send data only to given profile...
                    if(pUser->m_i32Profile == pCur->m_i32Profile) {
                        szWanted = szLen+pCur->m_szDataLen;
                        if(ServerManager::m_szGlobalBufferSize < szWanted) {
                            if(CheckAndResizeGlobalBuffer(szWanted) == false) {
								AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes in GlobalDataQueue::ProcessSingleItems3\n", Allign128K(szWanted));
                                break;
                            }
                        }
                        memcpy(ServerManager::m_pGlobalBuffer+szLen, pCur->m_pData, pCur->m_szDataLen);
                        szLen += pCur->m_szDataLen;
                        ServerManager::m_pGlobalBuffer[szLen] = '\0';
                    }
                    break;
                }
                case SI_PM2PROFILE: { // send pm only to given profile...
                    if(pUser->m_i32Profile == pCur->m_i32Profile) {
                        szWanted = szLen+pCur->m_szDataLen+pUser->m_ui8NickLen+13;
                        if(ServerManager::m_szGlobalBufferSize < szWanted) {
                            if(CheckAndResizeGlobalBuffer(szWanted) == false) {
								AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes in GlobalDataQueue::ProcessSingleItems4\n", Allign128K(szWanted));
                                break;
                            }
                        }
                        iRet = snprintf(ServerManager::m_pGlobalBuffer+szLen, ServerManager::m_szGlobalBufferSize-szLen, "$To: %s From: ", pUser->m_sNick);
                        if(iRet > 0) {
	                        szLen += iRet;

	                        memcpy(ServerManager::m_pGlobalBuffer+szLen, pCur->m_pData, pCur->m_szDataLen);
	                        szLen += pCur->m_szDataLen;
	                        ServerManager::m_pGlobalBuffer[szLen] = '\0';
	                	}
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    if(szLen != 0) {
        pUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, szLen);
    }

    ReduceGlobalBuffer();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::SingleItemStore(char * sData, const size_t szDataLen, User * pFromUser, const int32_t i32Profile, const uint8_t ui8Type) {
    SingleDataItem * pNewItem = new (std::nothrow) SingleDataItem();
    if(pNewItem == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewItem in GlobalDataQueue::SingleItemStore\n");
    	return;
    }

    if(sData != NULL) {
#ifdef _WIN32
        pNewItem->m_pData = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szDataLen+1);
#else
		pNewItem->m_pData = (char *)malloc(szDataLen+1);
#endif
        if(pNewItem->m_pData == NULL) {
            delete pNewItem;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes in GlobalDataQueue::SingleItemStore\n", szDataLen+1);

            return;
        }

        memcpy(pNewItem->m_pData, sData, szDataLen);
        pNewItem->m_pData[szDataLen] = '\0';
    } else {
        pNewItem->m_pData = NULL;
    }

	pNewItem->m_szDataLen = szDataLen;

	pNewItem->m_pFromUser = pFromUser;

	pNewItem->m_ui8Type = ui8Type;

	pNewItem->m_pPrev = NULL;
	pNewItem->m_pNext = NULL;

    pNewItem->m_i32Profile = i32Profile;

    if(m_pNewSingleItems[0] == NULL) {
		m_pNewSingleItems[0] = pNewItem;
		m_pNewSingleItems[1] = pNewItem;
    } else {
        pNewItem->m_pPrev = m_pNewSingleItems[1];
		m_pNewSingleItems[1]->m_pNext = pNewItem;
		m_pNewSingleItems[1] = pNewItem;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::SendFinalQueue() {
    if(m_pQueueItems != NULL) {
        QueueItem * pCur = NULL,
            * pNext = m_pQueueItems;

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->m_pNext;

            switch(pCur->m_ui8CommandType) {
                case CMD_OPS:
                case CMD_CHAT:
                case CMD_LUA:
                    AddDataToQueue(m_GlobalQueues[0], pCur->m_pCommand1, pCur->m_szLen1);
                    break;
                default:
                    break;
            }
        }
    }

    if(m_pNewQueueItems[0] != NULL) {
        QueueItem * pCur = NULL,
            * pNext = m_pNewQueueItems[0];

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->m_pNext;

            switch(pCur->m_ui8CommandType) {
                case CMD_OPS:
                case CMD_CHAT:
                case CMD_LUA:
                    AddDataToQueue(m_GlobalQueues[0], pCur->m_pCommand1, pCur->m_szLen1);
                    break;
                default:
                    break;
            }
        }
    }

    if(m_GlobalQueues[0].m_szLen == 0) {
        return;
    }

	m_GlobalQueues[0].m_pZbuffer = ZlibUtility::m_Ptr->CreateZPipe(m_GlobalQueues[0].m_pBuffer, m_GlobalQueues[0].m_szLen, m_GlobalQueues[0].m_pZbuffer, m_GlobalQueues[0].m_szZlen, m_GlobalQueues[0].m_szZsize);

	User * pCur = NULL,
        * pNext = Users::m_Ptr->m_pUserListS;

    while(pNext != NULL) {
        pCur = pNext;
        pNext = pCur->m_pNext;

        if(m_GlobalQueues[0].m_szZlen != 0) {
            pCur->PutInSendBuf(m_GlobalQueues[0].m_pZbuffer, m_GlobalQueues[0].m_szZlen);
            ServerManager::m_ui64BytesSentSaved += (m_GlobalQueues[0].m_szLen - m_GlobalQueues[0].m_szZlen);
        } else {
            pCur->PutInSendBuf(m_GlobalQueues[0].m_pBuffer, m_GlobalQueues[0].m_szLen);
        }
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::AddDataToQueue(GlobalQueue & rQueue, char * sData, const size_t szLen) {
    if(rQueue.m_szSize < (rQueue.m_szLen + szLen)) {
        size_t szAllignLen = Allign256(rQueue.m_szLen + szLen);
        char * pOldBuf = rQueue.m_pBuffer;

#ifdef _WIN32
        rQueue.m_pBuffer = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, szAllignLen);
#else
		rQueue.m_pBuffer = (char *)realloc(pOldBuf, szAllignLen);
#endif
        if(rQueue.m_pBuffer == NULL) {
            rQueue.m_pBuffer = pOldBuf;

            AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes in GlobalDataQueue::AddDataToQueue\n", szAllignLen);
            return;
        }

        rQueue.m_szSize = szAllignLen-1;
    }

    memcpy(rQueue.m_pBuffer+rQueue.m_szLen, sData, szLen);
    rQueue.m_szLen += szLen;
    rQueue.m_pBuffer[rQueue.m_szLen] = '\0';
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void * GlobalDataQueue::GetLastQueueItem() {
    return m_pNewQueueItems[1];
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void * GlobalDataQueue::GetFirstQueueItem() {
    return m_pNewQueueItems[0];
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void * GlobalDataQueue::InsertBlankQueueItem(void * pAfterItem, const uint8_t ui8CmdType) {
    QueueItem * pNewItem = new (std::nothrow) QueueItem();
    if(pNewItem == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewItem in GlobalDataQueue::InsertBlankQueueItem\n");
    	return NULL;
    }

    pNewItem->m_pCommand1 = NULL;
	pNewItem->m_szLen1 = 0;

    pNewItem->m_pCommand2 = NULL;
    pNewItem->m_szLen2 = 0;

	pNewItem->m_ui8CommandType = ui8CmdType;

    if(pAfterItem == m_pNewQueueItems[0]) {
        pNewItem->m_pNext = m_pNewQueueItems[0];
		m_pNewQueueItems[0] = pNewItem;
        return pNewItem;
    }

    QueueItem * pCur = NULL,
        * pNext = m_pNewQueueItems[0];

    while(pNext != NULL) {
        pCur = pNext;
        pNext = pCur->m_pNext;

        if(pCur == pAfterItem) {
            if(pCur->m_pNext == NULL) {
				m_pNewQueueItems[1] = pNewItem;
            }

            pNewItem->m_pNext = pCur->m_pNext;
            pCur->m_pNext = pNewItem;
            return pNewItem;
        }
    }

	pNewItem->m_pNext = NULL;

    if(m_pNewQueueItems[0] == NULL) {
		m_pNewQueueItems[0] = pNewItem;
		m_pNewQueueItems[1] = pNewItem;
    } else {
		m_pNewQueueItems[1]->m_pNext = pNewItem;
		m_pNewQueueItems[1] = pNewItem;
    }

    return pNewItem;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::FillBlankQueueItem(char * sCommand, const size_t szLen, void * pVoidQueueItem) {
	QueueItem * pQueueItem = reinterpret_cast<QueueItem *>(pVoidQueueItem);

#ifdef _WIN32
    pQueueItem->m_pCommand1 = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
	pQueueItem->m_pCommand1 = (char *)malloc(szLen+1);
#endif
    if(pQueueItem->m_pCommand1 == NULL) {
		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for pNewItem->m_pCommand1 in GlobalDataQueue::FillBlankQueueItem\n", szLen+1);

        return;
    }

    memcpy(pQueueItem->m_pCommand1, sCommand, szLen);
    pQueueItem->m_pCommand1[szLen] = '\0';

	pQueueItem->m_szLen1 = szLen;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void GlobalDataQueue::StatusMessageFormat(const char * sFrom, const char * sFormatMsg, ...) {
	int iMsgLen = 0;

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s $", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
		if(iMsgLen <= 0) {
			AppendDebugLogFormat("[ERR] snprintf wrong value %d in GlobalDataQueue::StatusMessageFormat from: %s\n", iMsgLen, sFrom);
			return;
		}
	}

	va_list vlArgs;
	va_start(vlArgs, sFormatMsg);

	int iRet = vsnprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, sFormatMsg, vlArgs);

	va_end(vlArgs);

	if(iRet <= 0) {
		AppendDebugLogFormat("[ERR] vsnprintf wrong value %d in GlobalDataQueue::StatusMessageFormat from: %s\n", iRet, sFrom);
		return;
	}

	iMsgLen += iRet;

	if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
		SingleItemStore(ServerManager::m_pGlobalBuffer, iMsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
	} else {
		AddQueueItem(ServerManager::m_pGlobalBuffer, iMsgLen, NULL, 0, GlobalDataQueue::CMD_OPS);
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
