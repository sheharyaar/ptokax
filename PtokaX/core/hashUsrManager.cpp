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
#include "hashUsrManager.h"
//---------------------------------------------------------------------------
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
HashManager * HashManager::m_Ptr = NULL;
//---------------------------------------------------------------------------

HashManager::HashManager() {
    memset(m_pNickTable, 0, sizeof(m_pNickTable));
    memset(m_pIpTable, 0, sizeof(m_pIpTable));

    //Memo("HashManager created");
}
//---------------------------------------------------------------------------

HashManager::~HashManager() {
    //Memo("HashManager destroyed");
    for(uint32_t ui32i = 0; ui32i < 65536; ui32i++) {
		IpTableItem * cur = NULL,
            * next = m_pIpTable[ui32i];
        
        while(next != NULL) {
            cur = next;
            next = cur->m_pNext;
        
            delete cur;
		}
    }
}
//---------------------------------------------------------------------------

bool HashManager::Add(User * pUser) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &pUser->m_ui32NickHash, sizeof(uint16_t));

    if(m_pNickTable[ui16dx] != NULL) {
		m_pNickTable[ui16dx]->m_pHashTablePrev = pUser;
        pUser->m_pHashTableNext = m_pNickTable[ui16dx];
    }

	m_pNickTable[ui16dx] = pUser;

    if(m_pIpTable[pUser->m_ui16IpTableIdx] == NULL) {
		m_pIpTable[pUser->m_ui16IpTableIdx] = new (std::nothrow) IpTableItem();

        if(m_pIpTable[pUser->m_ui16IpTableIdx] == NULL) {
			pUser->m_ui32BoolBits |= User::BIT_ERROR;
			pUser->Close();

            AppendDebugLog("%s - [MEM] Cannot allocate IpTableItem in HashManager::Add\n");
            return false;
        }

		m_pIpTable[pUser->m_ui16IpTableIdx]->m_pNext = NULL;
		m_pIpTable[pUser->m_ui16IpTableIdx]->m_pPrev = NULL;

		m_pIpTable[pUser->m_ui16IpTableIdx]->m_pFirstUser = pUser;
		m_pIpTable[pUser->m_ui16IpTableIdx]->m_ui16Count = 1;

        return true;
    }

    IpTableItem * cur = NULL,
        * next = m_pIpTable[pUser->m_ui16IpTableIdx];

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(memcmp(cur->m_pFirstUser->m_ui128IpHash, pUser->m_ui128IpHash, 16) == 0) {
            cur->m_pFirstUser->m_pHashIpTablePrev = pUser;
			pUser->m_pHashIpTableNext = cur->m_pFirstUser;
            cur->m_pFirstUser = pUser;
			cur->m_ui16Count++;

            return true;
        }
    }

    cur = new (std::nothrow) IpTableItem();

    if(cur == NULL) {
		pUser->m_ui32BoolBits |= User::BIT_ERROR;
		pUser->Close();

		AppendDebugLog("%s - [MEM] Cannot allocate IpTableItem2 in HashManager::Add\n");
		return false;
    }

    cur->m_pFirstUser = pUser;
	cur->m_ui16Count = 1;

    cur->m_pNext = m_pIpTable[pUser->m_ui16IpTableIdx];
    cur->m_pPrev = NULL;

	m_pIpTable[pUser->m_ui16IpTableIdx]->m_pPrev = cur;
	m_pIpTable[pUser->m_ui16IpTableIdx] = cur;

    return true;
}
//---------------------------------------------------------------------------

void HashManager::Remove(User * pUser) {
    if(pUser->m_pHashTablePrev == NULL) {
        uint16_t ui16dx = 0;
        memcpy(&ui16dx, &pUser->m_ui32NickHash, sizeof(uint16_t));

        if(pUser->m_pHashTableNext == NULL) {
			m_pNickTable[ui16dx] = NULL;
        } else {
			pUser->m_pHashTableNext->m_pHashTablePrev = NULL;
			m_pNickTable[ui16dx] = pUser->m_pHashTableNext;
        }
    } else if(pUser->m_pHashTableNext == NULL) {
		pUser->m_pHashTablePrev->m_pHashTableNext = NULL;
    } else {
		pUser->m_pHashTablePrev->m_pHashTableNext = pUser->m_pHashTableNext;
		pUser->m_pHashTableNext->m_pHashTablePrev = pUser->m_pHashTablePrev;
    }

	pUser->m_pHashTablePrev = NULL;
	pUser->m_pHashTableNext = NULL;

	if(pUser->m_pHashIpTablePrev == NULL) {
        IpTableItem * cur = NULL,
            * next = m_pIpTable[pUser->m_ui16IpTableIdx];
    
        while(next != NULL) {
            cur = next;
            next = cur->m_pNext;

            if(memcmp(cur->m_pFirstUser->m_ui128IpHash, pUser->m_ui128IpHash, 16) == 0) {
				cur->m_ui16Count--;

                if(pUser->m_pHashIpTableNext == NULL) {
                    if(cur->m_pPrev == NULL) {
                        if(cur->m_pNext == NULL) {
                            m_pIpTable[pUser->m_ui16IpTableIdx] = NULL;
                        } else {
                            cur->m_pNext->m_pPrev = NULL;
							m_pIpTable[pUser->m_ui16IpTableIdx] = cur->m_pNext;
                        }
                    } else if(cur->m_pNext == NULL) {
                        cur->m_pPrev->m_pNext = NULL;
                    } else {
                        cur->m_pPrev->m_pNext = cur->m_pNext;
                        cur->m_pNext->m_pPrev = cur->m_pPrev;
                    }

                    delete cur;
                } else {
					pUser->m_pHashIpTableNext->m_pHashIpTablePrev = NULL;
                    cur->m_pFirstUser = pUser->m_pHashIpTableNext;
                }

				pUser->m_pHashIpTablePrev = NULL;
				pUser->m_pHashIpTableNext = NULL;

                return;
            }
        }
    } else if(pUser->m_pHashIpTableNext == NULL) {
		pUser->m_pHashIpTablePrev->m_pHashIpTableNext = NULL;
    } else {
		pUser->m_pHashIpTablePrev->m_pHashIpTableNext = pUser->m_pHashIpTableNext;
		pUser->m_pHashIpTableNext->m_pHashIpTablePrev = pUser->m_pHashIpTablePrev;
    }

	pUser->m_pHashIpTablePrev = NULL;
	pUser->m_pHashIpTableNext = NULL;

    IpTableItem * cur = NULL,
        * next = m_pIpTable[pUser->m_ui16IpTableIdx];

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(memcmp(cur->m_pFirstUser->m_ui128IpHash, pUser->m_ui128IpHash, 16) == 0) {
			cur->m_ui16Count--;

            return;
        }
    }
}
//---------------------------------------------------------------------------

User * HashManager::FindUser(char * sNick, const size_t szNickLen) {
    uint32_t ui32Hash = HashNick(sNick, szNickLen);

    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &ui32Hash, sizeof(uint16_t));

    User * next = m_pNickTable[ui16dx];

    // pointer exists ? Then we need look for nick 
    if(next != NULL) {
        User * cur = NULL;

        while(next != NULL) {
            cur = next;
            next = cur->m_pHashTableNext;

            // we are looking for duplicate string
			if(cur->m_ui32NickHash == ui32Hash && cur->m_ui8NickLen == szNickLen && strcasecmp(cur->m_sNick, sNick) == 0) {
                return cur;
            }
        }            
    }
    
    // no equal hash found, we dont have the nick in list
    return NULL;
}
//---------------------------------------------------------------------------

User * HashManager::FindUser(User * pUser) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &pUser->m_ui32NickHash, sizeof(uint16_t));

    User * next = m_pNickTable[ui16dx];

    // pointer exists ? Then we need look for nick
    if(next != NULL) { 
        User * cur = NULL;

        while(next != NULL) {
            cur = next;
            next = cur->m_pHashTableNext;

            // we are looking for duplicate string
			if(cur->m_ui32NickHash == pUser->m_ui32NickHash && cur->m_ui8NickLen == pUser->m_ui8NickLen && strcasecmp(cur->m_sNick, pUser->m_sNick) == 0) {
                return cur;
            }
        }            
    }
    
    // no equal hash found, we dont have the nick in list
    return NULL;
}
//---------------------------------------------------------------------------

User * HashManager::FindUser(const uint8_t * ui128IpHash) {
    uint16_t ui16IpTableIdx = 0;

    if(IN6_IS_ADDR_V4MAPPED((const in6_addr *)ui128IpHash)) {
        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
    }

	IpTableItem * cur = NULL,
        * next = m_pIpTable[ui16IpTableIdx];

    while(next != NULL) {
		cur = next;
        next = cur->m_pNext;

        if(memcmp(cur->m_pFirstUser->m_ui128IpHash, ui128IpHash, 16) == 0) {
            return cur->m_pFirstUser;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

uint32_t HashManager::GetUserIpCount(User * pUser) const {
	IpTableItem * cur = NULL,
        * next = m_pIpTable[pUser->m_ui16IpTableIdx];

	while(next != NULL) {
		cur = next;
		next = cur->m_pNext;

        if(memcmp(cur->m_pFirstUser->m_ui128IpHash, pUser->m_ui128IpHash, 16) == 0) {
            return cur->m_ui16Count;
        }
	}

	return 0;
}
//---------------------------------------------------------------------------
