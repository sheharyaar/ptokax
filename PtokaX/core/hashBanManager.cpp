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
#include "hashBanManager.h"
//---------------------------------------------------------------------------
#include "PXBReader.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
	#include "../gui.win/BansDialog.h"
    #include "../gui.win/RangeBansDialog.h"
#endif
//---------------------------------------------------------------------------
BanManager * BanManager::m_Ptr = NULL;
//---------------------------------------------------------------------------
static const char sPtokaXBans[] = "PtokaX Bans";
static const size_t szPtokaXBansLen = sizeof(sPtokaXBans)-1;
static const char sPtokaXRangeBans[] = "PtokaX RangeBans";
static const size_t szPtokaXRangeBansLen = sizeof(sPtokaXRangeBans)-1;
static const char sBanIds[] = "BT" "NI" "NB" "IP" "IB" "FB" "RE" "BY" "EX";
static const size_t szBanIdsLen = sizeof(sBanIds)-1;
static const char sRangeBanIds[] = "BT" "RF" "RT" "FB" "RE" "BY" "EX";
static const size_t szRangeBanIdsLen = sizeof(sRangeBanIds)-1;
//---------------------------------------------------------------------------

BanItem::BanItem(void) : m_tTempBanExpire(0), m_sNick(NULL), m_sReason(NULL), m_sBy(NULL), m_pPrev(NULL), m_pNext(NULL), m_pHashNickTablePrev(NULL), m_pHashNickTableNext(NULL), m_pHashIpTablePrev(NULL), m_pHashIpTableNext(NULL), m_ui32NickHash(0), m_ui8Bits(0) {
	m_sIp[0] = '\0';

    memset(m_ui128IpHash, 0, 16);
}
//---------------------------------------------------------------------------

BanItem::~BanItem(void) {
#ifdef _WIN32
    if(m_sNick != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sNick) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate m_sNick in BanItem::~BanItem\n");
        }
    }
#else
	free(m_sNick);
#endif

#ifdef _WIN32
    if(m_sReason != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sReason) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_sReason in BanItem::~BanItem\n");
        }
    }
#else
	free(m_sReason);
#endif

#ifdef _WIN32
    if(m_sBy != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sBy) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_sBy in BanItem::~BanItem\n");
        }
    }
#else
	free(m_sBy);
#endif
}
//---------------------------------------------------------------------------

RangeBanItem::RangeBanItem(void) : m_tTempBanExpire(0), m_sReason(NULL), m_sBy(NULL), m_pPrev(NULL), m_pNext(NULL), m_ui8Bits(0) {
	m_sIpFrom[0] = '\0';
	m_sIpTo[0] = '\0';

    memset(m_ui128FromIpHash, 0, 16);
    memset(m_ui128ToIpHash, 0, 16);
}
//---------------------------------------------------------------------------

RangeBanItem::~RangeBanItem(void) {
#ifdef _WIN32
    if(m_sReason != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sReason) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_sReason in RangeBanItem::~RangeBanItem\n");
        }
    }
#else
	free(m_sReason);
#endif

#ifdef _WIN32
    if(m_sBy != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sBy) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_sBy in RangeBanItem::~RangeBanItem\n");
        }
    }
#else
	free(m_sBy);
#endif
}
//---------------------------------------------------------------------------

BanManager::BanManager(void) : m_ui32SaveCalled(0), m_pTempBanListS(NULL), m_pTempBanListE(NULL), m_pPermBanListS(NULL), m_pPermBanListE(NULL),
	m_pRangeBanListS(NULL), m_pRangeBanListE(NULL) {
    memset(m_pNickTable, 0, sizeof(m_pNickTable));
    memset(m_pIpTable, 0, sizeof(m_pIpTable));
}
//---------------------------------------------------------------------------

BanManager::~BanManager(void) {
    BanItem * curBan = NULL,
        * nextBan = m_pPermBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
		nextBan = curBan->m_pNext;

		delete curBan;
	}

    nextBan = m_pTempBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
		nextBan = curBan->m_pNext;

		delete curBan;
	}
    
    RangeBanItem * curRangeBan = NULL,
        * nextRangeBan = m_pRangeBanListS;

    while(nextRangeBan != NULL) {
        curRangeBan = nextRangeBan;
		nextRangeBan = curRangeBan->m_pNext;

		delete curRangeBan;
	}

    IpTableItem * cur = NULL, * next = NULL;

    for(uint32_t ui32i = 0; ui32i < 65536; ui32i++) {
        next = m_pIpTable[ui32i];
        
        while(next != NULL) {
            cur = next;
            next = cur->m_pNext;
        
            delete cur;
		}
    }
}
//---------------------------------------------------------------------------

bool BanManager::Add(BanItem * pBan) {
	if(Add2Table(pBan) == false) {
        return false;
    }

    if(((pBan->m_ui8Bits & PERM) == PERM) == true) {
		if(m_pPermBanListE == NULL) {
			m_pPermBanListS = pBan;
			m_pPermBanListE = pBan;
		} else {
			m_pPermBanListE->m_pNext = pBan;
			pBan->m_pPrev = m_pPermBanListE;
			m_pPermBanListE = pBan;
		}
    } else {
		if(m_pTempBanListE == NULL) {
			m_pTempBanListS = pBan;
			m_pTempBanListE = pBan;
		} else {
			m_pTempBanListE->m_pNext = pBan;
			pBan->m_pPrev = m_pTempBanListE;
			m_pTempBanListE = pBan;
		}
    }

#ifdef _BUILD_GUI
	if(BansDialog::m_Ptr != NULL) {
        BansDialog::m_Ptr->AddBan(pBan);
    }
#endif

	return true;
}
//---------------------------------------------------------------------------

bool BanManager::Add2Table(BanItem * pBan) {
	if(((pBan->m_ui8Bits & IP) == IP) == true) {
		if(Add2IpTable(pBan) == false) {
            return false;
        }
    }

    if(((pBan->m_ui8Bits & NICK) == NICK) == true) {
		Add2NickTable(pBan);
    }

    return true;
}
//---------------------------------------------------------------------------

void BanManager::Add2NickTable(BanItem * pBan) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &pBan->m_ui32NickHash, sizeof(uint16_t));

    if(m_pNickTable[ui16dx] != NULL) {
		m_pNickTable[ui16dx]->m_pHashNickTablePrev = pBan;
        pBan->m_pHashNickTableNext = m_pNickTable[ui16dx];
    }

	m_pNickTable[ui16dx] = pBan;
}
//---------------------------------------------------------------------------

bool BanManager::Add2IpTable(BanItem * pBan) {
    uint16_t ui16IpTableIdx = 0;

    if(IN6_IS_ADDR_V4MAPPED((const in6_addr *)pBan->m_ui128IpHash)) {
        ui16IpTableIdx = pBan->m_ui128IpHash[14] * pBan->m_ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(pBan->m_ui128IpHash);
    }
    
    if(m_pIpTable[ui16IpTableIdx] == NULL) {
		m_pIpTable[ui16IpTableIdx] = new (std::nothrow) IpTableItem();

        if(m_pIpTable[ui16IpTableIdx] == NULL) {
			AppendDebugLog("%s - [MEM] Cannot allocate IpTableItem in BanManager::Add2IpTable\n");
            return false;
        }

		m_pIpTable[ui16IpTableIdx]->m_pNext = NULL;
		m_pIpTable[ui16IpTableIdx]->m_pPrev = NULL;

		m_pIpTable[ui16IpTableIdx]->m_pFirstBan = pBan;

        return true;
    }

    IpTableItem * cur = NULL,
        * next = m_pIpTable[ui16IpTableIdx];

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

		if(memcmp(cur->m_pFirstBan->m_ui128IpHash, pBan->m_ui128IpHash, 16) == 0) {
			cur->m_pFirstBan->m_pHashIpTablePrev = pBan;
			pBan->m_pHashIpTableNext = cur->m_pFirstBan;
            cur->m_pFirstBan = pBan;

            return true;
        }
    }

    cur = new (std::nothrow) IpTableItem();

    if(cur == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate IpTableBans2 in BanManager::Add2IpTable\n");
        return false;
    }

    cur->m_pFirstBan = pBan;

    cur->m_pNext = m_pIpTable[ui16IpTableIdx];
    cur->m_pPrev = NULL;

	m_pIpTable[ui16IpTableIdx]->m_pPrev = cur;
	m_pIpTable[ui16IpTableIdx] = cur;

    return true;
}
//---------------------------------------------------------------------------

#ifdef _BUILD_GUI
void BanManager::Rem(BanItem * pBan, const bool bFromGui/* = false*/) {
#else
void BanManager::Rem(BanItem * pBan, const bool /*bFromGui = false*/) {
#endif
	RemFromTable(pBan);

    if(((pBan->m_ui8Bits & PERM) == PERM) == true) {
		if(pBan->m_pPrev == NULL) {
			if(pBan->m_pNext == NULL) {
				m_pPermBanListS = NULL;
				m_pPermBanListE = NULL;
			} else {
				pBan->m_pNext->m_pPrev = NULL;
				m_pPermBanListS = pBan->m_pNext;
			}
		} else if(pBan->m_pNext == NULL) {
			pBan->m_pPrev->m_pNext = NULL;
			m_pPermBanListE = pBan->m_pPrev;
		} else {
			pBan->m_pPrev->m_pNext = pBan->m_pNext;
			pBan->m_pNext->m_pPrev = pBan->m_pPrev;
		}
    } else {
        if(pBan->m_pPrev == NULL) {
			if(pBan->m_pNext == NULL) {
				m_pTempBanListS = NULL;
				m_pTempBanListE = NULL;
			} else {
				pBan->m_pNext->m_pPrev = NULL;
				m_pTempBanListS = pBan->m_pNext;
			   }
		} else if(pBan->m_pNext == NULL) {
			pBan->m_pPrev->m_pNext = NULL;
			m_pTempBanListE = pBan->m_pPrev;
		} else {
			pBan->m_pPrev->m_pNext = pBan->m_pNext;
			pBan->m_pNext->m_pPrev = pBan->m_pPrev;
		}
    }

#ifdef _BUILD_GUI
    if(bFromGui == false && BansDialog::m_Ptr != NULL) {
        BansDialog::m_Ptr->RemoveBan(pBan);
    }
#endif
}
//---------------------------------------------------------------------------

void BanManager::RemFromTable(BanItem * pBan) {
    if(((pBan->m_ui8Bits & IP) == IP) == true) {
		RemFromIpTable(pBan);
    }

    if(((pBan->m_ui8Bits & NICK) == NICK) == true) {
		RemFromNickTable(pBan);
    }
}
//---------------------------------------------------------------------------

void BanManager::RemFromNickTable(BanItem * pBan) {
    if(pBan->m_pHashNickTablePrev == NULL) {
        uint16_t ui16dx = 0;
        memcpy(&ui16dx, &pBan->m_ui32NickHash, sizeof(uint16_t));

        if(pBan->m_pHashNickTableNext == NULL) {
			m_pNickTable[ui16dx] = NULL;
        } else {
            pBan->m_pHashNickTableNext->m_pHashNickTablePrev = NULL;
			m_pNickTable[ui16dx] = pBan->m_pHashNickTableNext;
        }
    } else if(pBan->m_pHashNickTableNext == NULL) {
        pBan->m_pHashNickTablePrev->m_pHashNickTableNext = NULL;
    } else {
        pBan->m_pHashNickTablePrev->m_pHashNickTableNext = pBan->m_pHashNickTableNext;
        pBan->m_pHashNickTableNext->m_pHashNickTablePrev = pBan->m_pHashNickTablePrev;
    }

    pBan->m_pHashNickTablePrev = NULL;
    pBan->m_pHashNickTableNext = NULL;
}
//---------------------------------------------------------------------------

void BanManager::RemFromIpTable(BanItem * pBan) {
    uint16_t ui16IpTableIdx = 0;

    if(IN6_IS_ADDR_V4MAPPED((const in6_addr *)pBan->m_ui128IpHash)) {
        ui16IpTableIdx = pBan->m_ui128IpHash[14] * pBan->m_ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(pBan->m_ui128IpHash);
    }

	if(pBan->m_pHashIpTablePrev == NULL) {
        IpTableItem * cur = NULL,
            * next = m_pIpTable[ui16IpTableIdx];

        while(next != NULL) {
            cur = next;
            next = cur->m_pNext;

			if(memcmp(cur->m_pFirstBan->m_ui128IpHash, pBan->m_ui128IpHash, 16) == 0) {
				if(pBan->m_pHashIpTableNext == NULL) {
					if(cur->m_pPrev == NULL) {
						if(cur->m_pNext == NULL) {
							m_pIpTable[ui16IpTableIdx] = NULL;
						} else {
							cur->m_pNext->m_pPrev = NULL;
							m_pIpTable[ui16IpTableIdx] = cur->m_pNext;
                        }
					} else if(cur->m_pNext == NULL) {
						cur->m_pPrev->m_pNext = NULL;
					} else {
						cur->m_pPrev->m_pNext = cur->m_pNext;
                        cur->m_pNext->m_pPrev = cur->m_pPrev;
                    }

                    delete cur;
				} else {
					pBan->m_pHashIpTableNext->m_pHashIpTablePrev = NULL;
                    cur->m_pFirstBan = pBan->m_pHashIpTableNext;
                }

                break;
            }
        }
	} else if(pBan->m_pHashIpTableNext == NULL) {
		pBan->m_pHashIpTablePrev->m_pHashIpTableNext = NULL;
	} else {
        pBan->m_pHashIpTablePrev->m_pHashIpTableNext = pBan->m_pHashIpTableNext;
        pBan->m_pHashIpTableNext->m_pHashIpTablePrev = pBan->m_pHashIpTablePrev;
    }

    pBan->m_pHashIpTablePrev = NULL;
    pBan->m_pHashIpTableNext = NULL;
}
//---------------------------------------------------------------------------

BanItem* BanManager::Find(BanItem * pBan) {
	if(m_pTempBanListS != NULL) {
        time_t acc_time;
        time(&acc_time);

		BanItem * curBan = NULL,
            * nextBan = m_pTempBanListS;

        while(nextBan != NULL) {
            curBan = nextBan;
    		nextBan = curBan->m_pNext;

            if(acc_time > curBan->m_tTempBanExpire) {
				Rem(curBan);
				delete curBan;

                continue;
            }

			if(curBan == pBan) {
				return curBan;
			}
		}
    }
    
	if(m_pPermBanListS != NULL) {
		BanItem * curBan = NULL,
            * nextBan = m_pPermBanListS;

        while(nextBan != NULL) {
            curBan = nextBan;
    		nextBan = curBan->m_pNext;

			if(curBan == pBan) {
				return curBan;
			}
        }
	}

	return NULL;
}
//---------------------------------------------------------------------------

void BanManager::Remove(BanItem * pBan) {
	if(m_pTempBanListS != NULL) {
        time_t acc_time;
        time(&acc_time);

		BanItem * curBan = NULL,
            * nextBan = m_pTempBanListS;

        while(nextBan != NULL) {
            curBan = nextBan;
    		nextBan = curBan->m_pNext;

            if(acc_time > curBan->m_tTempBanExpire) {
				Rem(curBan);
				delete curBan;

                continue;
            }

			if(curBan == pBan) {
				Rem(pBan);
				delete pBan;

				return;
			}
		}
    }
    
	if(m_pPermBanListS != NULL) {
		BanItem * curBan = NULL,
            * nextBan = m_pPermBanListS;

        while(nextBan != NULL) {
            curBan = nextBan;
    		nextBan = curBan->m_pNext;

			if(curBan == pBan) {
				Rem(pBan);
				delete pBan;

				return;
			}
        }
	}
}
//---------------------------------------------------------------------------

void BanManager::AddRange(RangeBanItem * pRangeBan) {
    if(m_pRangeBanListE == NULL) {
		m_pRangeBanListS = pRangeBan;
		m_pRangeBanListE = pRangeBan;
    } else {
		m_pRangeBanListE->m_pNext = pRangeBan;
        pRangeBan->m_pPrev = m_pRangeBanListE;
		m_pRangeBanListE = pRangeBan;
    }

#ifdef _BUILD_GUI
    if(RangeBansDialog::m_Ptr != NULL) {
        RangeBansDialog::m_Ptr->AddRangeBan(pRangeBan);
    }
#endif
}
//---------------------------------------------------------------------------

#ifdef _BUILD_GUI
void BanManager::RemRange(RangeBanItem * pRangeBan, const bool bFromGui/* = false*/) {
#else
void BanManager::RemRange(RangeBanItem * pRangeBan, const bool /*bFromGui = false*/) {
#endif
    if(pRangeBan->m_pPrev == NULL) {
        if(pRangeBan->m_pNext == NULL) {
			m_pRangeBanListS = NULL;
			m_pRangeBanListE = NULL;
        } else {
            pRangeBan->m_pNext->m_pPrev = NULL;
			m_pRangeBanListS = pRangeBan->m_pNext;
        }
    } else if(pRangeBan->m_pNext == NULL) {
        pRangeBan->m_pPrev->m_pNext = NULL;
		m_pRangeBanListE = pRangeBan->m_pPrev;
    } else {
        pRangeBan->m_pPrev->m_pNext = pRangeBan->m_pNext;
        pRangeBan->m_pNext->m_pPrev = pRangeBan->m_pPrev;
    }

#ifdef _BUILD_GUI
    if(bFromGui == false && RangeBansDialog::m_Ptr != NULL) {
        RangeBansDialog::m_Ptr->RemoveRangeBan(pRangeBan);
    }
#endif
}
//---------------------------------------------------------------------------

RangeBanItem* BanManager::FindRange(RangeBanItem * pRangeBan) {
	if(m_pRangeBanListS != NULL) {
        time_t acc_time;
        time(&acc_time);

		RangeBanItem * curBan = NULL,
            * nextBan = m_pRangeBanListS;

		while(nextBan != NULL) {
            curBan = nextBan;
			nextBan = curBan->m_pNext;

			if(((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true && acc_time > curBan->m_tTempBanExpire) {
				RemRange(curBan);
				delete curBan;

                continue;
			}

			if(curBan == pRangeBan) {
				return curBan;
			}
		}
	}

	return NULL;
}
//---------------------------------------------------------------------------

void BanManager::RemoveRange(RangeBanItem * pRangeBan) {
	if(m_pRangeBanListS != NULL) {
        time_t acc_time;
        time(&acc_time);

		RangeBanItem * curBan = NULL,
            * nextBan = m_pRangeBanListS;

		while(nextBan != NULL) {
            curBan = nextBan;
    		nextBan = curBan->m_pNext;

			if(((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true && acc_time > curBan->m_tTempBanExpire) {
				RemRange(curBan);
                delete curBan;

                continue;
			}

			if(curBan == pRangeBan) {
				RemRange(pRangeBan);
				delete pRangeBan;

				return;
			}
		}
	}
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindNick(User * pUser) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &pUser->m_ui32NickHash, sizeof(uint16_t));

    time_t acc_time;
    time(&acc_time);

    BanItem * cur = NULL,
        * next = m_pNickTable[ui16dx];

    while(next != NULL) {
        cur = next;
        next = cur->m_pHashNickTableNext;

		if(cur->m_ui32NickHash == pUser->m_ui32NickHash && strcasecmp(cur->m_sNick, pUser->m_sNick) == 0) {
            // PPK ... check if temban expired
			if(((cur->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
                if(acc_time >= cur->m_tTempBanExpire) {
					Rem(cur);
                    delete cur;

					continue;
                }
            }
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindIP(User * pUser) {
    time_t acc_time;
    time(&acc_time);

    IpTableItem * cur = NULL,
        * next = m_pIpTable[pUser->m_ui16IpTableIdx];

	BanItem * curBan = NULL, * nextBan = NULL;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

		if(memcmp(cur->m_pFirstBan->m_ui128IpHash, pUser->m_ui128IpHash, 16) == 0) {
			nextBan = cur->m_pFirstBan;

			while(nextBan != NULL) {
                curBan = nextBan;
                nextBan = curBan->m_pHashIpTableNext;
                
                // PPK ... check if temban expired
				if(((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
                    if(acc_time >= curBan->m_tTempBanExpire) {
						Rem(curBan);
                        delete curBan;
    
    					continue;
                    }
                }

                return curBan;
            }
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

RangeBanItem* BanManager::FindRange(User * pUser) {
    time_t acc_time;
    time(&acc_time);

    RangeBanItem * cur = NULL,
        * next = m_pRangeBanListS;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(memcmp(cur->m_ui128FromIpHash, pUser->m_ui128IpHash, 16) <= 0 && memcmp(cur->m_ui128ToIpHash, pUser->m_ui128IpHash, 16) >= 0) {
            // PPK ... check if temban expired
            if(((cur->m_ui8Bits & TEMP) == TEMP) == true) {
                if(acc_time >= cur->m_tTempBanExpire) {
                    RemRange(cur);
                    delete cur;

					continue;
                }
            }
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindFull(const uint8_t * ui128IpHash) {
    time_t acc_time;
    time(&acc_time);

    return FindFull(ui128IpHash, acc_time);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindFull(const uint8_t * ui128IpHash, const time_t &tAccTime) {
    uint16_t ui16IpTableIdx = 0;

    if(IN6_IS_ADDR_V4MAPPED((const in6_addr *)ui128IpHash)) {
        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
    }

	IpTableItem * cur = NULL,
        * next = m_pIpTable[ui16IpTableIdx];

	BanItem * curBan = NULL, * nextBan = NULL, * fnd = NULL;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(memcmp(cur->m_pFirstBan->m_ui128IpHash, ui128IpHash, 16) == 0) {
			nextBan = cur->m_pFirstBan;

            while(nextBan != NULL) {
                curBan = nextBan;
                nextBan = curBan->m_pHashIpTableNext;
        
                // PPK ... check if temban expired
				if(((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
                    if(tAccTime >= curBan->m_tTempBanExpire) {
						Rem(curBan);
                        delete curBan;
    
    					continue;
                    }
                }
                    
				if(((curBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true) {
                    return curBan;
                } else if(fnd == NULL) {
                    fnd = curBan;
                }
            }
        }
    }

    return fnd;
}
//---------------------------------------------------------------------------

RangeBanItem* BanManager::FindFullRange(const uint8_t * ui128IpHash, const time_t &tAccTime) {
    RangeBanItem * cur = NULL, * fnd = NULL,
        * next = m_pRangeBanListS;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(memcmp(cur->m_ui128FromIpHash, ui128IpHash, 16) <= 0 && memcmp(cur->m_ui128ToIpHash, ui128IpHash, 16) >= 0) {
            // PPK ... check if temban expired
            if(((cur->m_ui8Bits & TEMP) == TEMP) == true) {
                if(tAccTime >= cur->m_tTempBanExpire) {
                    RemRange(cur);
                    delete cur;

					continue;
                }
            }
            
            if(((cur->m_ui8Bits & FULL) == FULL) == true) {
                return cur;
            } else if(fnd == NULL) {
                fnd = cur;
            }
        }
    }

    return fnd;
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindNick(char * sNick, const size_t szNickLen) {
    uint32_t hash = HashNick(sNick, szNickLen);

    time_t acc_time;
    time(&acc_time);

    return FindNick(hash, acc_time, sNick);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindNick(const uint32_t ui32Hash, const time_t &tAccTime, char * sNick) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &ui32Hash, sizeof(uint16_t));

	BanItem * cur = NULL,
        * next = m_pNickTable[ui16dx];

    while(next != NULL) {
        cur = next;
        next = cur->m_pHashNickTableNext;

		if(cur->m_ui32NickHash == ui32Hash && strcasecmp(cur->m_sNick, sNick) == 0) {
            // PPK ... check if temban expired
			if(((cur->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
                if(tAccTime >= cur->m_tTempBanExpire) {
					Rem(cur);
                    delete cur;

					continue;
                }
            }
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindIP(const uint8_t * ui128IpHash, const time_t &tAccTime) {
    uint16_t ui16IpTableIdx = 0;

    if(IN6_IS_ADDR_V4MAPPED((const in6_addr *)ui128IpHash)) {
        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
    }

    IpTableItem * cur = NULL,
        * next = m_pIpTable[ui16IpTableIdx];

	BanItem * curBan = NULL, * nextBan = NULL;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(memcmp(cur->m_pFirstBan->m_ui128IpHash, ui128IpHash, 16) == 0) {
			nextBan = cur->m_pFirstBan;

            while(nextBan != NULL) {
                curBan = nextBan;
                nextBan = curBan->m_pHashIpTableNext;

                // PPK ... check if temban expired
				if(((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
                    if(tAccTime >= curBan->m_tTempBanExpire) {
						Rem(curBan);
                        delete curBan;
    
    					continue;
                    }
                }

                return curBan;
            }
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

RangeBanItem* BanManager::FindRange(const uint8_t * ui128IpHash, const time_t &tAccTime) {
    RangeBanItem * cur = NULL,
        * next = m_pRangeBanListS;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        // PPK ... check if temban expired
        if(((cur->m_ui8Bits & TEMP) == TEMP) == true) {
            if(tAccTime >= cur->m_tTempBanExpire) {
                RemRange(cur);
                delete cur;

				continue;
            }
        }

        if(memcmp(cur->m_ui128FromIpHash, ui128IpHash, 16) <= 0 && memcmp(cur->m_ui128ToIpHash, ui128IpHash, 16) >= 0) {
            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

RangeBanItem* BanManager::FindRange(const uint8_t * ui128FromHash, const uint8_t * ui128ToHash, const time_t &tAccTime) {
    RangeBanItem * cur = NULL,
        * next = m_pRangeBanListS;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(memcmp(cur->m_ui128FromIpHash, ui128FromHash, 16) == 0 && memcmp(cur->m_ui128ToIpHash, ui128ToHash, 16) == 0) {
            // PPK ... check if temban expired
            if(((cur->m_ui8Bits & TEMP) == TEMP) == true) {
                if(tAccTime >= cur->m_tTempBanExpire) {
                    RemRange(cur);
                    delete cur;

					continue;
                }
            }

            return cur;
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindTempNick(char * sNick, const size_t szNickLen) {
    uint32_t hash = HashNick(sNick, szNickLen);

    time_t acc_time;
    time(&acc_time);

    return FindTempNick(hash, acc_time, sNick);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindTempNick(const uint32_t ui32Hash,  const time_t &tAccTime, char * sNick) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &ui32Hash, sizeof(uint16_t));

	BanItem * cur = NULL,
        * next = m_pNickTable[ui16dx];

    while(next != NULL) {
        cur = next;
        next = cur->m_pHashNickTableNext;

		if(cur->m_ui32NickHash == ui32Hash && strcasecmp(cur->m_sNick, sNick) == 0) {
            // PPK ... check if temban expired
			if(((cur->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
                if(tAccTime >= cur->m_tTempBanExpire) {
                    Rem(cur);
                    delete cur;

					continue;
                }
                return cur;
            }
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindTempIP(const uint8_t * ui128IpHash, const time_t &tAccTime) {
    uint16_t ui16IpTableIdx = 0;

    if(IN6_IS_ADDR_V4MAPPED((const in6_addr *)ui128IpHash)) {
        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
    }

    IpTableItem * cur = NULL,
        * next = m_pIpTable[ui16IpTableIdx];

    BanItem * curBan = NULL, * nextBan = NULL;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(memcmp(cur->m_pFirstBan->m_ui128IpHash, ui128IpHash, 16) == 0) {
			nextBan = cur->m_pFirstBan;

            while(nextBan != NULL) {
                curBan = nextBan;
                nextBan = curBan->m_pHashIpTableNext;
                
                // PPK ... check if temban expired
				if(((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
                    if(tAccTime >= curBan->m_tTempBanExpire) {
						Rem(curBan);
                        delete curBan;
    
    					continue;
                    }

                    return curBan;
                }
            }
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindPermNick(char * sNick, const size_t szNickLen) {
    uint32_t hash = HashNick(sNick, szNickLen);
    
	return FindPermNick(hash, sNick);
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindPermNick(const uint32_t ui32Hash, char * sNick) {
    uint16_t ui16dx = 0;
    memcpy(&ui16dx, &ui32Hash, sizeof(uint16_t));

    BanItem * cur = NULL,
        * next = m_pNickTable[ui16dx];

    while(next != NULL) {
        cur = next;
        next = cur->m_pHashNickTableNext;

		if(cur->m_ui32NickHash == ui32Hash && strcasecmp(cur->m_sNick, sNick) == 0) {
            if(((cur->m_ui8Bits & BanManager::PERM) == BanManager::PERM) == true) {
                return cur;
            }
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

BanItem* BanManager::FindPermIP(const uint8_t * ui128IpHash) {
    uint16_t ui16IpTableIdx = 0;

    if(IN6_IS_ADDR_V4MAPPED((const in6_addr *)ui128IpHash)) {
        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
    }

	IpTableItem * cur = NULL,
        * next = m_pIpTable[ui16IpTableIdx];

    BanItem * curBan = NULL, * nextBan = NULL;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(memcmp(cur->m_pFirstBan->m_ui128IpHash, ui128IpHash, 16) == 0) {
            nextBan = cur->m_pFirstBan;

            while(nextBan != NULL) {
                curBan = nextBan;
                nextBan = curBan->m_pHashIpTableNext;
                
                if(((curBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM) == true) {
                    return curBan;
                }
            }
        }
    }

    return NULL;
}
//---------------------------------------------------------------------------

void BanManager::Load() {
#ifdef _WIN32
    if(FileExist((ServerManager::m_sPath + "\\cfg\\Bans.pxb").c_str()) == false) {
#else
    if(FileExist((ServerManager::m_sPath + "/cfg/Bans.pxb").c_str()) == false) {
#endif
        LoadXML();
        return;
    }

    PXBReader pxbBans;

    // Open regs file
#ifdef _WIN32
    if(pxbBans.OpenFileRead((ServerManager::m_sPath + "\\cfg\\Bans.pxb").c_str(), 9) == false) {
#else
    if(pxbBans.OpenFileRead((ServerManager::m_sPath + "/cfg/Bans.pxb").c_str(), 9) == false) {
#endif
        return;
    }

    // Read file header
    uint16_t ui16Identificators[9];
    ui16Identificators[0] = *((uint16_t *)"FI");
    ui16Identificators[1] = *((uint16_t *)"FV");

    if(pxbBans.ReadNextItem(ui16Identificators, 2) == false) {
        return;
    }

    // Check header if we have correct file
    if(pxbBans.m_ui16ItemLengths[0] != szPtokaXBansLen || strncmp((char *)pxbBans.m_pItemDatas[0], sPtokaXBans, szPtokaXBansLen) != 0) {
        return;
    }

    {
        uint32_t ui32FileVersion = ntohl(*((uint32_t *)(pxbBans.m_pItemDatas[1])));

        if(ui32FileVersion < 1) {
            return;
        }
    }

    time_t tmAccTime;
    time(&tmAccTime);

	// "BT" "NI" "NB" "IP" "IB" "FB" "RE" "BY" "EX"
    memcpy(ui16Identificators, sBanIds, szBanIdsLen);

    bool bSuccess = pxbBans.ReadNextItem(ui16Identificators, 9);

    while(bSuccess == true) {
        BanItem * pBan = new (std::nothrow) BanItem();
        if(pBan == NULL) {
			AppendDebugLog("%s - [MEM] Cannot allocate pBan in BanManager::Load\n");
            exit(EXIT_FAILURE);
        }

		// Permanent or temporary ban?
		pBan->m_ui8Bits |= (((char *)pxbBans.m_pItemDatas[0])[0] == '0' ? PERM : TEMP);

		// Do we have some nick?
		if(pxbBans.m_ui16ItemLengths[1] != 0) {
        	if(pxbBans.m_ui16ItemLengths[1] > 64) {
				AppendDebugLogFormat("[ERR] sNick too long %hu in BanManager::Load\n", pxbBans.m_ui16ItemLengths[1]);

                exit(EXIT_FAILURE);
            }
 #ifdef _WIN32
            pBan->m_sNick = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, pxbBans.m_ui16ItemLengths[1]+1);
#else
			pBan->m_sNick = (char *)malloc(pxbBans.m_ui16ItemLengths[1]+1);
#endif
            if(pBan->m_sNick == NULL) {
				AppendDebugLogFormat("[MEM] Cannot allocate %hu bytes for sNick in BanManager::Load\n",pxbBans.m_ui16ItemLengths[1]+1);

                exit(EXIT_FAILURE);
            }

            memcpy(pBan->m_sNick, pxbBans.m_pItemDatas[1], pxbBans.m_ui16ItemLengths[1]);
            pBan->m_sNick[pxbBans.m_ui16ItemLengths[1]] = '\0';
            pBan->m_ui32NickHash = HashNick(pBan->m_sNick, pxbBans.m_ui16ItemLengths[1]);

			// it is nick ban?
			if(((char *)pxbBans.m_pItemDatas[2])[0] != '0') {
				pBan->m_ui8Bits |= NICK;
			}
		}

		// Do we have some IP address?
		if(pxbBans.m_ui16ItemLengths[3] != 0 && IN6_IS_ADDR_UNSPECIFIED((const in6_addr *)pxbBans.m_pItemDatas[3]) == 0) {
        	if(pxbBans.m_ui16ItemLengths[3] != 16) {
                AppendDebugLogFormat("[ERR] Ban IP address have incorrect length %hu in BanManager::Load\n", pxbBans.m_ui16ItemLengths[3]);

                exit(EXIT_FAILURE);
            }

			memcpy(pBan->m_ui128IpHash, pxbBans.m_pItemDatas[3], 16);

    		if(IN6_IS_ADDR_V4MAPPED((const in6_addr *)pBan->m_ui128IpHash)) {
				in_addr ipv4addr;
				memcpy(&ipv4addr, pBan->m_ui128IpHash + 12, 4);
				strcpy(pBan->m_sIp, inet_ntoa(ipv4addr));
    		} else {
#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
            	win_inet_ntop(pBan->m_ui128IpHash, pBan->m_sIp, 40);
#else
            	inet_ntop(AF_INET6, pBan->m_ui128IpHash, pBan->m_sIp, 40);
#endif
			}

			// it is IP ban?
			if(((char *)pxbBans.m_pItemDatas[4])[0] != '0') {
				pBan->m_ui8Bits |= IP;
			}

			// it is full ban ?
			if(((char *)pxbBans.m_pItemDatas[5])[0] != '0') {
				pBan->m_ui8Bits |= FULL;
			}
		}

		// Do we have reason?
		if(pxbBans.m_ui16ItemLengths[6] != 0) {
        	if(pxbBans.m_ui16ItemLengths[6] > 511) {
                pxbBans.m_ui16ItemLengths[6] = 511;
            }

#ifdef _WIN32
            pBan->m_sReason = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, pxbBans.m_ui16ItemLengths[6]+1);
#else
			pBan->m_sReason = (char *)malloc(pxbBans.m_ui16ItemLengths[6]+1);
#endif
            if(pBan->m_sReason == NULL) {
				AppendDebugLogFormat("[MEM] Cannot allocate %hu bytes for m_sReason in BanManager::Load\n", pxbBans.m_ui16ItemLengths[6]+1);

                exit(EXIT_FAILURE);
            }

            memcpy(pBan->m_sReason, pxbBans.m_pItemDatas[6], pxbBans.m_ui16ItemLengths[6]);
            pBan->m_sReason[pxbBans.m_ui16ItemLengths[6]] = '\0';
		}

		// Do we have who created ban?
		if(pxbBans.m_ui16ItemLengths[7] != 0) {
        	if(pxbBans.m_ui16ItemLengths[7] > 64) {
                pxbBans.m_ui16ItemLengths[7] = 64;
            }

#ifdef _WIN32
            pBan->m_sBy = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, pxbBans.m_ui16ItemLengths[7]+1);
#else
			pBan->m_sBy = (char *)malloc(pxbBans.m_ui16ItemLengths[7]+1);
#endif
            if(pBan->m_sBy == NULL) {
                AppendDebugLogFormat("[MEM] Cannot allocate %hu bytes for m_sBy in BanManager::Load\n", pxbBans.m_ui16ItemLengths[7]+1);

                exit(EXIT_FAILURE);
            }

            memcpy(pBan->m_sBy, pxbBans.m_pItemDatas[7], pxbBans.m_ui16ItemLengths[7]);
            pBan->m_sBy[pxbBans.m_ui16ItemLengths[7]] = '\0';
		}

		// it is temporary ban?
        if(((pBan->m_ui8Bits & TEMP) == TEMP) == true) {
        	if(pxbBans.m_ui16ItemLengths[8] != 8) {
                AppendDebugLogFormat("[ERR] Temp ban expire time have incorrect length %hu in BanManager::Load\n", pxbBans.m_ui16ItemLengths[8]);

                exit(EXIT_FAILURE);
            } else {
            	// Temporary ban expiration datetime
            	pBan->m_tTempBanExpire = (time_t)be64toh(*((uint64_t *)(pxbBans.m_pItemDatas[8])));

            	if(tmAccTime >= pBan->m_tTempBanExpire) {
                	delete pBan;
                } else {
		            if(Add(pBan) == false) {
		            	AppendDebugLog("%s [ERR] Add ban failed in BanManager::Load\n");
		
		                exit(EXIT_FAILURE);
		            }
				}
            }
        } else {
		    if(Add(pBan) == false) {
		        AppendDebugLog("%s [ERR] Add2 ban failed in BanManager::Load\n");
		
		        exit(EXIT_FAILURE);
		    }
		}

        bSuccess = pxbBans.ReadNextItem(ui16Identificators, 9);
    }

    PXBReader pxbRangeBans;

    // Open regs file
#ifdef _WIN32
    if(pxbRangeBans.OpenFileRead((ServerManager::m_sPath + "\\cfg\\RangeBans.pxb").c_str(), 7) == false) {
#else
    if(pxbRangeBans.OpenFileRead((ServerManager::m_sPath + "/cfg/RangeBans.pxb").c_str(), 7) == false) {
#endif
        return;
    }

    // Read file header
    ui16Identificators[0] = *((uint16_t *)"FI");
    ui16Identificators[1] = *((uint16_t *)"FV");

    if(pxbRangeBans.ReadNextItem(ui16Identificators, 2) == false) {
        return;
    }

    // Check header if we have correct file
    if(pxbRangeBans.m_ui16ItemLengths[0] != szPtokaXRangeBansLen || strncmp((char *)pxbRangeBans.m_pItemDatas[0], sPtokaXRangeBans, szPtokaXRangeBansLen) != 0) {
        return;
    }

    {
        uint32_t ui32FileVersion = ntohl(*((uint32_t *)(pxbRangeBans.m_pItemDatas[1])));

        if(ui32FileVersion < 1) {
            return;
        }
    }

	// // "BT" "RF" "RT" "FB" "RE" "BY" "EX";
    memcpy(ui16Identificators, sRangeBanIds, szRangeBanIdsLen);

    bSuccess = pxbRangeBans.ReadNextItem(ui16Identificators, 7);

    while(bSuccess == true) {
        RangeBanItem * pRangeBan = new (std::nothrow) RangeBanItem();
        if(pRangeBan == NULL) {
			AppendDebugLog("%s - [MEM] Cannot allocate pRangeBan in BanManager::Load\n");
            exit(EXIT_FAILURE);
        }

		// Permanent or temporary ban?
		pRangeBan->m_ui8Bits |= (((char *)pxbRangeBans.m_pItemDatas[0])[0] == '0' ? PERM : TEMP);

		// Do we have first IP address?
		if(pxbRangeBans.m_ui16ItemLengths[1] != 16) {
            AppendDebugLogFormat("[ERR] Range Ban first IP address have incorrect length %hu in BanManager::Load\n", pxbBans.m_ui16ItemLengths[1]);

            exit(EXIT_FAILURE);
		}

		memcpy(pRangeBan->m_ui128FromIpHash, pxbRangeBans.m_pItemDatas[1], 16);

    	if(IN6_IS_ADDR_V4MAPPED((const in6_addr *)pRangeBan->m_ui128FromIpHash)) {
			in_addr ipv4addr;
			memcpy(&ipv4addr, pRangeBan->m_ui128FromIpHash + 12, 4);
			strcpy(pRangeBan->m_sIpFrom, inet_ntoa(ipv4addr));
    	} else {
#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
            win_inet_ntop(pRangeBan->m_ui128FromIpHash, pRangeBan->m_sIpFrom, 40);
#else
            inet_ntop(AF_INET6, pRangeBan->m_ui128FromIpHash, pRangeBan->m_sIpFrom, 40);
#endif
		}

		// Do we have second IP address?
		if(pxbRangeBans.m_ui16ItemLengths[2] != 16) {
            AppendDebugLogFormat("[ERR] Range Ban second IP address have incorrect length %hu in BanManager::Load\n", pxbBans.m_ui16ItemLengths[2]);

            exit(EXIT_FAILURE);
		}

		memcpy(pRangeBan->m_ui128ToIpHash, pxbRangeBans.m_pItemDatas[2], 16);

    	if(IN6_IS_ADDR_V4MAPPED((const in6_addr *)pRangeBan->m_ui128ToIpHash)) {
			in_addr ipv4addr;
			memcpy(&ipv4addr, pRangeBan->m_ui128ToIpHash + 12, 4);
			strcpy(pRangeBan->m_sIpTo, inet_ntoa(ipv4addr));
    	} else {
#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
            win_inet_ntop(pRangeBan->m_ui128ToIpHash, pRangeBan->m_sIpTo, 40);
#else
            inet_ntop(AF_INET6, pRangeBan->m_ui128ToIpHash, pRangeBan->m_sIpTo, 40);
#endif
		}

		// it is full ban ?
		if(((char *)pxbRangeBans.m_pItemDatas[3])[0] != '0') {
			pRangeBan->m_ui8Bits |= FULL;
		}

		// Do we have reason?
		if(pxbRangeBans.m_ui16ItemLengths[4] != 0) {
        	if(pxbRangeBans.m_ui16ItemLengths[4] > 511) {
                pxbRangeBans.m_ui16ItemLengths[4] = 511;
            }

#ifdef _WIN32
            pRangeBan->m_sReason = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, pxbRangeBans.m_ui16ItemLengths[4]+1);
#else
			pRangeBan->m_sReason = (char *)malloc(pxbRangeBans.m_ui16ItemLengths[4]+1);
#endif
            if(pRangeBan->m_sReason == NULL) {
				AppendDebugLogFormat("[MEM] Cannot allocate %hu bytes for m_sReason2 in BanManager::Load\n", pxbRangeBans.m_ui16ItemLengths[4]+1);

                exit(EXIT_FAILURE);
            }

            memcpy(pRangeBan->m_sReason, pxbRangeBans.m_pItemDatas[4], pxbRangeBans.m_ui16ItemLengths[4]);
            pRangeBan->m_sReason[pxbRangeBans.m_ui16ItemLengths[4]] = '\0';
		}

		// Do we have who created ban?
		if(pxbRangeBans.m_ui16ItemLengths[5] != 0) {
        	if(pxbRangeBans.m_ui16ItemLengths[5] > 64) {
                pxbRangeBans.m_ui16ItemLengths[5] = 64;
            }

#ifdef _WIN32
            pRangeBan->m_sBy = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, pxbRangeBans.m_ui16ItemLengths[5]+1);
#else
			pRangeBan->m_sBy = (char *)malloc(pxbRangeBans.m_ui16ItemLengths[5]+1);
#endif
            if(pRangeBan->m_sBy == NULL) {
                AppendDebugLogFormat("[MEM] Cannot allocate %hu bytes for m_sBy2 in BanManager::Load\n", pxbRangeBans.m_ui16ItemLengths[5]+1);

                exit(EXIT_FAILURE);
            }

            memcpy(pRangeBan->m_sBy, pxbRangeBans.m_pItemDatas[5], pxbRangeBans.m_ui16ItemLengths[5]);
            pRangeBan->m_sBy[pxbRangeBans.m_ui16ItemLengths[5]] = '\0';
		}

		// it is temporary ban?
        if(((pRangeBan->m_ui8Bits & TEMP) == TEMP) == true) {
        	if(pxbRangeBans.m_ui16ItemLengths[6] != 8) {
                AppendDebugLogFormat("[ERR] Temp range ban expire time have incorrect lenght %hu in BanManager::Load\n", pxbRangeBans.m_ui16ItemLengths[6]);

                exit(EXIT_FAILURE);
            } else {
            	// Temporary ban expiration datetime
            	pRangeBan->m_tTempBanExpire = (time_t)be64toh(*((uint64_t *)(pxbRangeBans.m_pItemDatas[6])));

            	if(tmAccTime >= pRangeBan->m_tTempBanExpire) {
                	delete pRangeBan;
                } else {
		            AddRange(pRangeBan);
				}
            }
        } else {
        	AddRange(pRangeBan);
		}

        bSuccess = pxbRangeBans.ReadNextItem(ui16Identificators, 7);
    }

}
//---------------------------------------------------------------------------

void BanManager::LoadXML() {
    double dVer;

#ifdef _WIN32
    TiXmlDocument doc((ServerManager::m_sPath+"\\cfg\\BanList.xml").c_str());
#else
	TiXmlDocument doc((ServerManager::m_sPath+"/cfg/BanList.xml").c_str());
#endif

    if(doc.LoadFile() == false) {
        if(doc.ErrorId() != TiXmlBase::TIXML_ERROR_OPENING_FILE && doc.ErrorId() != TiXmlBase::TIXML_ERROR_DOCUMENT_EMPTY) {
            int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "Error loading file BanList.xml. %s (Col: %d, Row: %d)", doc.ErrorDesc(), doc.Column(), doc.Row());
			if(iMsgLen > 0) {
#ifdef _BUILD_GUI
				::MessageBox(NULL, ServerManager::m_pGlobalBuffer, g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
				AppendLog(ServerManager::m_pGlobalBuffer);
#endif
			}
            exit(EXIT_FAILURE);
        }
    } else {
        TiXmlHandle cfg(&doc);
        TiXmlElement *banlist = cfg.FirstChild("BanList").Element();
        if(banlist != NULL) {
            if(banlist->Attribute("version", &dVer) == NULL) {
                return;
            }

            time_t acc_time;
            time(&acc_time);

            TiXmlNode *bans = banlist->FirstChild("Bans");
            if(bans != NULL) {
                TiXmlNode *child = NULL;
                while((child = bans->IterateChildren(child)) != NULL) {
                	const char *ip = NULL, *nick = NULL, *reason = NULL, *by = NULL;
					TiXmlNode *ban = child->FirstChild("Type");

					if(ban == NULL || (ban = ban->FirstChild()) == NULL) {
						continue;
					}

                    char type = atoi(ban->Value()) == 0 ? (char)0 : (char)1;

					if((ban = child->FirstChild("IP")) != NULL &&
                        (ban = ban->FirstChild()) != NULL) {
						ip = ban->Value();
					}

					if((ban = child->FirstChild("Nick")) != NULL &&
                        (ban = ban->FirstChild()) != NULL) {
						nick = ban->Value();
					}

					if((ban = child->FirstChild("Reason")) != NULL &&
                        (ban = ban->FirstChild()) != NULL) {
						reason = ban->Value();
					}

					if((ban = child->FirstChild("By")) != NULL &&
                        (ban = ban->FirstChild()) != NULL) {
						by = ban->Value();
					}

					if((ban = child->FirstChild("NickBan")) == NULL ||
                        (ban = ban->FirstChild()) == NULL) {
						continue;
					}

					bool nickban = (atoi(ban->Value()) == 0 ? false : true);

					if((ban = child->FirstChild("IpBan")) == NULL ||
                        (ban = ban->FirstChild()) == NULL) {
						continue;
					}

                    bool ipban = (atoi(ban->Value()) == 0 ? false : true);

					if((ban = child->FirstChild("FullIpBan")) == NULL ||
                        (ban = ban->FirstChild()) == NULL) {
						continue;
					}

                    bool fullipban = (atoi(ban->Value()) == 0 ? false : true);

                    BanItem * Ban = new (std::nothrow) BanItem();
                    if(Ban == NULL) {
						AppendDebugLog("%s - [MEM] Cannot allocate Ban in BanManager::LoadXML\n");
                    	exit(EXIT_FAILURE);
                    }

                    if(type == 0) {
                        Ban->m_ui8Bits |= PERM;
                    } else {
                        Ban->m_ui8Bits |= TEMP;
                    }

                    // PPK ... ipban
                    if(ipban == true) {
                        if(ip != NULL && HashIP(ip, Ban->m_ui128IpHash) == true) {
                            strcpy(Ban->m_sIp, ip);
                            Ban->m_ui8Bits |= IP;

                            if(fullipban == true) {
                                Ban->m_ui8Bits |= FULL;
							}
                        } else {
							delete Ban;

                            continue;
                        }
                    }

                    // PPK ... nickban
                    if(nickban == true) {
                        if(nick != NULL) {
                            size_t szNickLen = strlen(nick);
#ifdef _WIN32
                            Ban->m_sNick = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNickLen+1);
#else
							Ban->m_sNick = (char *)malloc(szNickLen+1);
#endif
                            if(Ban->m_sNick == NULL) {
								AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sNick in BanManager::LoadXML\n", szNickLen+1);

                                exit(EXIT_FAILURE);
                            }

                            memcpy(Ban->m_sNick, nick, szNickLen);
                            Ban->m_sNick[szNickLen] = '\0';
                            Ban->m_ui32NickHash = HashNick(Ban->m_sNick, strlen(Ban->m_sNick));
                            Ban->m_ui8Bits |= NICK;
                        } else {
                            delete Ban;

                            continue;
                        }
                    }

                    if(reason != NULL) {
                        size_t szReasonLen = strlen(reason);
                        if(szReasonLen > 255) {
                            szReasonLen = 255;
                        }
#ifdef _WIN32
                        Ban->m_sReason = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen+1);
#else
						Ban->m_sReason = (char *)malloc(szReasonLen+1);
#endif
                        if(Ban->m_sReason == NULL) {
							AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sReason in BanManager::LoadXML\n", szReasonLen+1);

                            exit(EXIT_FAILURE);
                        }

                        memcpy(Ban->m_sReason, reason, szReasonLen);
                        Ban->m_sReason[szReasonLen] = '\0';
                    }

                    if(by != NULL) {
                        size_t szByLen = strlen(by);
                        if(szByLen > 63) {
                            szByLen = 63;
                        }
#ifdef _WIN32
                        Ban->m_sBy = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
						Ban->m_sBy = (char *)malloc(szByLen+1);
#endif
                        if(Ban->m_sBy == NULL) {
                            AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sBy1 in BanManager::LoadXML\n", szByLen+1);
                            exit(EXIT_FAILURE);
                        }

                        memcpy(Ban->m_sBy, by, szByLen);
                        Ban->m_sBy[szByLen] = '\0';
                    }

                    // PPK ... temp ban
                    if(((Ban->m_ui8Bits & TEMP) == TEMP) == true) {
                        if((ban = child->FirstChild("Expire")) == NULL ||
                            (ban = ban->FirstChild()) == NULL) {
                            delete Ban;

                            continue;
                        }
                        time_t expire = strtoul(ban->Value(), NULL, 10);

                        if(acc_time > expire) {
                            delete Ban;

							continue;
                        }

                        // PPK ... temp ban expiration
                        Ban->m_tTempBanExpire = expire;
                    }
                    
                    if(fullipban == true) {
                        Ban->m_ui8Bits |= FULL;
                    }

                    if(Add(Ban) == false) {
                        exit(EXIT_FAILURE);
                    }
                }
            }

            TiXmlNode *rangebans = banlist->FirstChild("RangeBans");
            if(rangebans != NULL) {
                TiXmlNode *child = NULL;
                while((child = rangebans->IterateChildren(child)) != NULL) {
                	const char *reason = NULL, *by = NULL;
					TiXmlNode *rangeban = child->FirstChild("Type");

					if(rangeban == NULL ||
                        (rangeban = rangeban->FirstChild()) == NULL) {
						continue;
					}

                    char type = atoi(rangeban->Value()) == 0 ? (char)0 : (char)1;

					if((rangeban = child->FirstChild("IpFrom")) == NULL ||
                        (rangeban = rangeban->FirstChild()) == NULL) {
						continue;
					}

                    const char *ipfrom = rangeban->Value();

					if((rangeban = child->FirstChild("IpTo")) == NULL ||
                        (rangeban = rangeban->FirstChild()) == NULL) {
						continue;
					}

                    const char *ipto = rangeban->Value();

					if((rangeban = child->FirstChild("Reason")) != NULL &&
                        (rangeban = rangeban->FirstChild()) != NULL) {
						reason = rangeban->Value();
					}

					if((rangeban = child->FirstChild("By")) != NULL &&
                        (rangeban = rangeban->FirstChild()) != NULL) {
						by = rangeban->Value();
					}

					if((rangeban = child->FirstChild("FullIpBan")) == NULL ||
                        (rangeban = rangeban->FirstChild()) == NULL) {
						continue;
					}

                    bool fullipban = (atoi(rangeban->Value()) == 0 ? false : true);

                    RangeBanItem * RangeBan = new (std::nothrow) RangeBanItem();
                    if(RangeBan == NULL) {
						AppendDebugLog("%s - [MEM] Cannot allocate RangeBan in BanManager::LoadXML\n");
                    	exit(EXIT_FAILURE);
                    }

                    if(type == 0) {
                        RangeBan->m_ui8Bits |= PERM;
                    } else {
                        RangeBan->m_ui8Bits |= TEMP;
                    }

                    // PPK ... fromip
                    if(HashIP(ipfrom, RangeBan->m_ui128FromIpHash) == true) {
                        strcpy(RangeBan->m_sIpFrom, ipfrom);
                    } else {
                        delete RangeBan;

                        continue;
                    }

                    // PPK ... toip
                    if(HashIP(ipto, RangeBan->m_ui128ToIpHash) == true && memcmp(RangeBan->m_ui128ToIpHash, RangeBan->m_ui128FromIpHash, 16) > 0) {
                        strcpy(RangeBan->m_sIpTo, ipto);
                    } else {
                        delete RangeBan;

                        continue;
                    }

                    if(reason != NULL) {
                        size_t szReasonLen = strlen(reason);
                        if(szReasonLen > 255) {
                            szReasonLen = 255;
                        }
#ifdef _WIN32
                        RangeBan->m_sReason = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen+1);
#else
						RangeBan->m_sReason = (char *)malloc(szReasonLen+1);
#endif
                        if(RangeBan->m_sReason == NULL) {
							AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sReason3 in BanManager::LoadXML\n", szReasonLen+1);
                            exit(EXIT_FAILURE);
                        }

                        memcpy(RangeBan->m_sReason, reason, szReasonLen);
                        RangeBan->m_sReason[szReasonLen] = '\0';
                    }

                    if(by != NULL) {
                        size_t szByLen = strlen(by);
                        if(szByLen > 63) {
                            szByLen = 63;
                        }
#ifdef _WIN32
                        RangeBan->m_sBy = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
						RangeBan->m_sBy = (char *)malloc(szByLen+1);
#endif
                        if(RangeBan->m_sBy == NULL) {
							AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sBy3 in BanManager::LoadXML\n", szByLen+1);
                            exit(EXIT_FAILURE);
                        }

                        memcpy(RangeBan->m_sBy, by, szByLen);
                        RangeBan->m_sBy[szByLen] = '\0';
                    }

                    // PPK ... temp ban
                    if(((RangeBan->m_ui8Bits & TEMP) == TEMP) == true) {
                        if((rangeban = child->FirstChild("Expire")) == NULL ||
                            (rangeban = rangeban->FirstChild()) == NULL) {
                            delete RangeBan;

                            continue;
                        }
                        time_t expire = strtoul(rangeban->Value(), NULL, 10);

                        if(acc_time > expire) {
                            delete RangeBan;
							continue;
                        }

                        // PPK ... temp ban expiration
                        RangeBan->m_tTempBanExpire = expire;
                    }

                    if(fullipban == true) {
                        RangeBan->m_ui8Bits |= FULL;
                    }

                    AddRange(RangeBan);
                }
            }
        }
    }
}
//---------------------------------------------------------------------------

void BanManager::Save(const bool bForce/* = false*/) {
    if(bForce == false) {
        // we don't want waste resources with save after every change in bans
        if(m_ui32SaveCalled < 100) {
			m_ui32SaveCalled++;
            return;
        }
    }
    
	m_ui32SaveCalled = 0;

    PXBReader pxbBans;

    // Open bans file
#ifdef _WIN32
    if(pxbBans.OpenFileSave((ServerManager::m_sPath + "\\cfg\\Bans.pxb").c_str(), 9) == false) {
#else
    if(pxbBans.OpenFileSave((ServerManager::m_sPath + "/cfg/Bans.pxb").c_str(), 9) == false) {
#endif
        return;
    }

    // Write file header
    pxbBans.m_sItemIdentifiers[0] = 'F';
    pxbBans.m_sItemIdentifiers[1] = 'I';
    pxbBans.m_ui16ItemLengths[0] = (uint16_t)szPtokaXBansLen;
    pxbBans.m_pItemDatas[0] = (void *)sPtokaXBans;
    pxbBans.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

    pxbBans.m_sItemIdentifiers[2] = 'F';
    pxbBans.m_sItemIdentifiers[3] = 'V';
    pxbBans.m_ui16ItemLengths[1] = 4;
    uint32_t ui32Version = 1;
    pxbBans.m_pItemDatas[1] = (void *)&ui32Version;
    pxbBans.m_ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;

    if(pxbBans.WriteNextItem(szPtokaXBansLen+4, 2) == false) {
        return;
    }

	// "BT" "NI" "NB" "IP" "IB" "FB" "RE" "BY" "EX"
    memcpy(pxbBans.m_sItemIdentifiers, sBanIds, szBanIdsLen);

	pxbBans.m_ui8ItemValues[0] = PXBReader::PXB_BYTE;
    pxbBans.m_ui8ItemValues[1] = PXBReader::PXB_STRING;
    pxbBans.m_ui8ItemValues[2] = PXBReader::PXB_BYTE;
    pxbBans.m_ui8ItemValues[3] = PXBReader::PXB_STRING;
    pxbBans.m_ui8ItemValues[4] = PXBReader::PXB_BYTE;
    pxbBans.m_ui8ItemValues[5] = PXBReader::PXB_BYTE;
    pxbBans.m_ui8ItemValues[6] = PXBReader::PXB_STRING;
    pxbBans.m_ui8ItemValues[7] = PXBReader::PXB_STRING;
	pxbBans.m_ui8ItemValues[8] = PXBReader::PXB_EIGHT_BYTES;

	uint64_t ui64TempBanExpire = 0;

    if(m_pTempBanListS != NULL) {
        BanItem * pCur = NULL,
            * pNext = m_pTempBanListS;

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->m_pNext;

	        pxbBans.m_ui16ItemLengths[0] = 1;
	        pxbBans.m_pItemDatas[0] = (((pCur->m_ui8Bits & PERM) == PERM) == true ? 0 : (void *)1);

	        pxbBans.m_ui16ItemLengths[1] = pCur->m_sNick == NULL ? 0 : (uint16_t)strlen(pCur->m_sNick);
	        pxbBans.m_pItemDatas[1] = pCur->m_sNick == NULL ? (void *)"" : (void *)pCur->m_sNick;

	        pxbBans.m_ui16ItemLengths[2] = 1;
	        pxbBans.m_pItemDatas[2] = (((pCur->m_ui8Bits & NICK) == NICK) == true ? (void *)1 : 0);

	        pxbBans.m_ui16ItemLengths[3] = 16;
	        pxbBans.m_pItemDatas[3] = (void *)pCur->m_ui128IpHash;

	        pxbBans.m_ui16ItemLengths[4] = 1;
	        pxbBans.m_pItemDatas[4] = (((pCur->m_ui8Bits & IP) == IP) == true ? (void *)1 : 0);

	        pxbBans.m_ui16ItemLengths[5] = 1;
	        pxbBans.m_pItemDatas[5] = (((pCur->m_ui8Bits & FULL) == FULL) == true ? (void *)1 : 0);

	        pxbBans.m_ui16ItemLengths[6] = pCur->m_sReason == NULL ? 0 : (uint16_t)strlen(pCur->m_sReason);
	        pxbBans.m_pItemDatas[6] = pCur->m_sReason == NULL ? (void *)"" : (void *)pCur->m_sReason;

	        pxbBans.m_ui16ItemLengths[7] = pCur->m_sBy == NULL ? 0 : (uint16_t)strlen(pCur->m_sBy);
	        pxbBans.m_pItemDatas[7] = pCur->m_sBy == NULL ? (void *)"" : (void *)pCur->m_sBy;

	        pxbBans.m_ui16ItemLengths[8] = 8;
	        ui64TempBanExpire = (uint64_t)pCur->m_tTempBanExpire;
	        pxbBans.m_pItemDatas[8] = (void *)&ui64TempBanExpire;

	        if(pxbBans.WriteNextItem(pxbBans.m_ui16ItemLengths[0] + pxbBans.m_ui16ItemLengths[1] + pxbBans.m_ui16ItemLengths[2] + pxbBans.m_ui16ItemLengths[3] + pxbBans.m_ui16ItemLengths[4] + pxbBans.m_ui16ItemLengths[5] + pxbBans.m_ui16ItemLengths[6] + 
				pxbBans.m_ui16ItemLengths[7] + pxbBans.m_ui16ItemLengths[8], 9) == false) {
	            break;
	        }
	    }
    }

    if(m_pPermBanListS != NULL) {
        BanItem * pCur = NULL,
            * pNext = m_pPermBanListS;

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->m_pNext;

	        pxbBans.m_ui16ItemLengths[0] = 1;
	        pxbBans.m_pItemDatas[0] = (((pCur->m_ui8Bits & PERM) == PERM) == true ? 0 : (void *)1);

	        pxbBans.m_ui16ItemLengths[1] = pCur->m_sNick == NULL ? 0 : (uint16_t)strlen(pCur->m_sNick);
	        pxbBans.m_pItemDatas[1] = pCur->m_sNick == NULL ? (void *)"" : (void *)pCur->m_sNick;

	        pxbBans.m_ui16ItemLengths[2] = 1;
	        pxbBans.m_pItemDatas[2] = (((pCur->m_ui8Bits & NICK) == NICK) == true ? (void *)1 : 0);

	        pxbBans.m_ui16ItemLengths[3] = 16;
	        pxbBans.m_pItemDatas[3] = (void *)pCur->m_ui128IpHash;

	        pxbBans.m_ui16ItemLengths[4] = 1;
	        pxbBans.m_pItemDatas[4] = (((pCur->m_ui8Bits & IP) == IP) == true ? (void *)1 : 0);

	        pxbBans.m_ui16ItemLengths[5] = 1;
	        pxbBans.m_pItemDatas[5] = (((pCur->m_ui8Bits & FULL) == FULL) == true ? (void *)1 : 0);

	        pxbBans.m_ui16ItemLengths[6] = pCur->m_sReason == NULL ? 0 : (uint16_t)strlen(pCur->m_sReason);
	        pxbBans.m_pItemDatas[6] = pCur->m_sReason == NULL ? (void *)"" : (void *)pCur->m_sReason;

	        pxbBans.m_ui16ItemLengths[7] = pCur->m_sBy == NULL ? 0 : (uint16_t)strlen(pCur->m_sBy);
	        pxbBans.m_pItemDatas[7] = pCur->m_sBy == NULL ? (void *)"" : (void *)pCur->m_sBy;

	        pxbBans.m_ui16ItemLengths[8] = 8;
	        ui64TempBanExpire = (uint64_t)pCur->m_tTempBanExpire;
	        pxbBans.m_pItemDatas[8] = (void *)&ui64TempBanExpire;

	        if(pxbBans.WriteNextItem(pxbBans.m_ui16ItemLengths[0] + pxbBans.m_ui16ItemLengths[1] + pxbBans.m_ui16ItemLengths[2] + pxbBans.m_ui16ItemLengths[3] + pxbBans.m_ui16ItemLengths[4] + pxbBans.m_ui16ItemLengths[5] + pxbBans.m_ui16ItemLengths[6] + 
				pxbBans.m_ui16ItemLengths[7] + pxbBans.m_ui16ItemLengths[8], 9) == false) {
	            break;
	        }
	    }
    }

    pxbBans.WriteRemaining();

    PXBReader pxbRangeBans;

    // Open range bans file
#ifdef _WIN32
    if(pxbRangeBans.OpenFileSave((ServerManager::m_sPath + "\\cfg\\RangeBans.pxb").c_str(), 7) == false) {
#else
    if(pxbRangeBans.OpenFileSave((ServerManager::m_sPath + "/cfg/RangeBans.pxb").c_str(), 7) == false) {
#endif
        return;
    }

    // Write file header
    pxbRangeBans.m_sItemIdentifiers[0] = 'F';
    pxbRangeBans.m_sItemIdentifiers[1] = 'I';
    pxbRangeBans.m_ui16ItemLengths[0] = (uint16_t)szPtokaXRangeBansLen;
    pxbRangeBans.m_pItemDatas[0] = (void *)sPtokaXRangeBans;
    pxbRangeBans.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

    pxbRangeBans.m_sItemIdentifiers[2] = 'F';
    pxbRangeBans.m_sItemIdentifiers[3] = 'V';
    pxbRangeBans.m_ui16ItemLengths[1] = 4;
    pxbRangeBans.m_pItemDatas[1] = (void *)&ui32Version;
    pxbRangeBans.m_ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;

    if(pxbRangeBans.WriteNextItem(szPtokaXRangeBansLen+4, 2) == false) {
        return;
    }

	// "BT" "RF" "RT" "FB" "RE" "BY" "EX"
    memcpy(pxbRangeBans.m_sItemIdentifiers, sRangeBanIds, szRangeBanIdsLen);

	pxbRangeBans.m_ui8ItemValues[0] = PXBReader::PXB_BYTE;
    pxbRangeBans.m_ui8ItemValues[1] = PXBReader::PXB_STRING;
    pxbRangeBans.m_ui8ItemValues[2] = PXBReader::PXB_STRING;
    pxbRangeBans.m_ui8ItemValues[3] = PXBReader::PXB_BYTE;
    pxbRangeBans.m_ui8ItemValues[4] = PXBReader::PXB_STRING;
    pxbRangeBans.m_ui8ItemValues[5] = PXBReader::PXB_STRING;
	pxbRangeBans.m_ui8ItemValues[6] = PXBReader::PXB_EIGHT_BYTES;

    if(m_pRangeBanListS != NULL) {
        RangeBanItem * pCur = NULL,
            * pNext = m_pRangeBanListS;

        while(pNext != NULL) {
            pCur = pNext;
            pNext = pCur->m_pNext;

	        pxbRangeBans.m_ui16ItemLengths[0] = 1;
	        pxbRangeBans.m_pItemDatas[0] = (((pCur->m_ui8Bits & PERM) == PERM) == true ? 0 : (void *)1);

	        pxbRangeBans.m_ui16ItemLengths[1] = 16;
	        pxbRangeBans.m_pItemDatas[1] = (void *)pCur->m_ui128FromIpHash;

	        pxbRangeBans.m_ui16ItemLengths[2] = 16;
	        pxbRangeBans.m_pItemDatas[2] = (void *)pCur->m_ui128ToIpHash;

	        pxbRangeBans.m_ui16ItemLengths[3] = 1;
	        pxbRangeBans.m_pItemDatas[3] = (((pCur->m_ui8Bits & FULL) == FULL) == true ? (void *)1 : 0);

	        pxbRangeBans.m_ui16ItemLengths[4] = pCur->m_sReason == NULL ? 0 : (uint16_t)strlen(pCur->m_sReason);
	        pxbRangeBans.m_pItemDatas[4] = pCur->m_sReason == NULL ? (void *)"" : (void *)pCur->m_sReason;

	        pxbRangeBans.m_ui16ItemLengths[5] = pCur->m_sBy == NULL ? 0 : (uint16_t)strlen(pCur->m_sBy);
	        pxbRangeBans.m_pItemDatas[5] = pCur->m_sBy == NULL ? (void *)"" : (void *)pCur->m_sBy;

	        pxbRangeBans.m_ui16ItemLengths[6] = 8;
	        ui64TempBanExpire = (uint64_t)pCur->m_tTempBanExpire;
	        pxbRangeBans.m_pItemDatas[6] = (void *)&ui64TempBanExpire;

	        if(pxbRangeBans.WriteNextItem(pxbRangeBans.m_ui16ItemLengths[0] + pxbRangeBans.m_ui16ItemLengths[1] + pxbRangeBans.m_ui16ItemLengths[2] + pxbRangeBans.m_ui16ItemLengths[3] + pxbRangeBans.m_ui16ItemLengths[4] + pxbRangeBans.m_ui16ItemLengths[5] + 
				pxbRangeBans.m_ui16ItemLengths[6], 7) == false) {
	            break;
	        }
	    }
    }

    pxbRangeBans.WriteRemaining();
}
//---------------------------------------------------------------------------

void BanManager::ClearTemp(void) {
    BanItem * curBan = NULL,
        * nextBan = m_pTempBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
        nextBan = curBan->m_pNext;

        Rem(curBan);
        delete curBan;
	}
    
	Save();
}
//---------------------------------------------------------------------------

void BanManager::ClearPerm(void) {
    BanItem * curBan = NULL,
        * nextBan = m_pPermBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
        nextBan = curBan->m_pNext;

        Rem(curBan);
        delete curBan;
	}
    
	Save();
}
//---------------------------------------------------------------------------

void BanManager::ClearRange(void) {
    RangeBanItem * curBan = NULL,
        * nextBan = m_pRangeBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
        nextBan = curBan->m_pNext;

        RemRange(curBan);
        delete curBan;
	}

	Save();
}
//---------------------------------------------------------------------------

void BanManager::ClearTempRange(void) {
    RangeBanItem * curBan = NULL,
        * nextBan = m_pRangeBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
        nextBan = curBan->m_pNext;

        if(((curBan->m_ui8Bits & TEMP) == TEMP) == true) {
            RemRange(curBan);
            delete curBan;
		}
    }
    
	Save();
}
//---------------------------------------------------------------------------

void BanManager::ClearPermRange(void) {
    RangeBanItem * curBan = NULL,
        * nextBan = m_pRangeBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
        nextBan = curBan->m_pNext;

        if(((curBan->m_ui8Bits & PERM) == PERM) == true) {
            RemRange(curBan);
            delete curBan;
		}
    }

	Save();
}
//---------------------------------------------------------------------------

void BanManager::Ban(User * pUser, const char * sReason, char * sBy, const bool bFull) {
    BanItem * pBan = new (std::nothrow) BanItem();
    if(pBan == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pBan in BanManager::Ban\n");
		return;
    }

    pBan->m_ui8Bits |= PERM;

    strcpy(pBan->m_sIp, pUser->m_sIP);
    memcpy(pBan->m_ui128IpHash, pUser->m_ui128IpHash, 16);
    pBan->m_ui8Bits |= IP;
    
    if(bFull == true) {
        pBan->m_ui8Bits |= FULL;
    }

    time_t acc_time;
    time(&acc_time);

    // PPK ... check for <unknown> nick -> bad ban from script
    if(pUser->m_sNick[0] != '<') {
#ifdef _WIN32
        pBan->m_sNick = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, pUser->m_ui8NickLen+1);
#else
		pBan->m_sNick = (char *)malloc(pUser->m_ui8NickLen+1);
#endif
		if(pBan->m_sNick == NULL) {
            delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu8 " bytes for m_sNick in BanManager::Ban\n", pUser->m_ui8NickLen+1);

			return;
		}

		memcpy(pBan->m_sNick, pUser->m_sNick, pUser->m_ui8NickLen);
		pBan->m_sNick[pUser->m_ui8NickLen] = '\0';
		pBan->m_ui32NickHash = pUser->m_ui32NickHash;
		pBan->m_ui8Bits |= NICK;
        
        // PPK ... not allow same nickbans ! i don't want this check here, but lame scripter find way to ban same nick/ip multiple times :(
		BanItem * nxtBan = FindNick(pBan->m_ui32NickHash, acc_time, pBan->m_sNick);

        if(nxtBan != NULL) {
            if(((nxtBan->m_ui8Bits & PERM) == PERM) == true) {
                if(((nxtBan->m_ui8Bits & IP) == IP) == true) {
                    if(memcmp(pBan->m_ui128IpHash, nxtBan->m_ui128IpHash, 16) == 0) {
                        if(((pBan->m_ui8Bits & FULL) == FULL) == false) {
                            // PPK ... same ban and new is not full, delete new
                            delete pBan;

                            return;
                        } else {
                            if(((nxtBan->m_ui8Bits & FULL) == FULL) == true) {
                                // PPK ... same ban and both full, delete new
                                delete pBan;

                                return;
                            } else {
                                // PPK ... same ban but only new is full, delete old
                                Rem(nxtBan);
                                delete nxtBan;
							}
                        }
                    } else {
                        pBan->m_ui8Bits &= ~NICK;
                    }
                } else {
                    // PPK ... old ban is only nickban, remove it
                    Rem(nxtBan);
                    delete nxtBan;
				}
            } else {
                if(((nxtBan->m_ui8Bits & IP) == IP) == true) {
                    if(memcmp(pBan->m_ui128IpHash, nxtBan->m_ui128IpHash, 16) == 0) {
                        if(((nxtBan->m_ui8Bits & FULL) == FULL) == false) {
                            // PPK ... same ban and old is only temp, delete old
                            Rem(nxtBan);
                            delete nxtBan;
						} else {
                            if(((pBan->m_ui8Bits & FULL) == FULL) == true) {
                                // PPK ... same full ban and old is only temp, delete old
                                Rem(nxtBan);
                                delete nxtBan;
							} else {
                                // PPK ... old ban is full, new not... set old ban to only ipban
								RemFromNickTable(nxtBan);
                                nxtBan->m_ui8Bits &= ~NICK;
                            }
                        }
                    } else {
                        // PPK ... set old ban to ip ban only
						RemFromNickTable(nxtBan);
                        nxtBan->m_ui8Bits &= ~NICK;
                    }
                } else {
                    // PPK ... old ban is only nickban, remove it
                    Rem(nxtBan);
                    delete nxtBan;
				}
            }
        }
    }
    
    // PPK ... clear bans with same ip without nickban and fullban if new ban is fullban
	BanItem * curBan = NULL,
        * nxtBan = FindIP(pBan->m_ui128IpHash, acc_time);

    while(nxtBan != NULL) {
        curBan = nxtBan;
        nxtBan = curBan->m_pHashIpTableNext;

        if(((curBan->m_ui8Bits & NICK) == NICK) == true) {
            continue;
        }
        
        if(((curBan->m_ui8Bits & FULL) == FULL) == true && ((pBan->m_ui8Bits & FULL) == FULL) == false) {
            continue;
        }

        Rem(curBan);
        delete curBan;
	}
        
    if(sReason != NULL) {
        size_t szReasonLen = strlen(sReason);
#ifdef _WIN32
        pBan->m_sReason = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen > 511 ? 512 : szReasonLen+1);
#else
		pBan->m_sReason = (char *)malloc(szReasonLen > 511 ? 512 : szReasonLen+1);
#endif
        if(pBan->m_sReason == NULL) {
            delete pBan;

            AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for sReason in BanManager::Ban\n", szReasonLen > 511 ? 512 : szReasonLen+1);

            return;
        }

        if(szReasonLen > 511) {
            memcpy(pBan->m_sReason, sReason, 508);
			pBan->m_sReason[510] = '.';
			pBan->m_sReason[509] = '.';
            pBan->m_sReason[508] = '.';
            szReasonLen = 511;
        } else {
            memcpy(pBan->m_sReason, sReason, szReasonLen);
        }
        pBan->m_sReason[szReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t szByLen = strlen(sBy);
        if(szByLen > 63) {
            szByLen = 63;
        }
#ifdef _WIN32
        pBan->m_sBy = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
		pBan->m_sBy = (char *)malloc(szByLen+1);
#endif
        if(pBan->m_sBy == NULL) {
            delete pBan;

            AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sBy in BanManager::Ban\n", szByLen+1);

			return;
        }   
        memcpy(pBan->m_sBy, sBy, szByLen);
        pBan->m_sBy[szByLen] = '\0';
    }

	if(Add(pBan) == false) {
        delete pBan;
        return;
    }

	Save();
}
//---------------------------------------------------------------------------

char BanManager::BanIp(User * pUser, char * sIp, char * sReason, char * sBy, const bool bFull) {
    BanItem * pBan = new (std::nothrow) BanItem();
    if(pBan == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate pBan in BanManager::BanIp\n");
    	return 1;
    }

    pBan->m_ui8Bits |= PERM;

    if(pUser != NULL) {
        strcpy(pBan->m_sIp, pUser->m_sIP);
        memcpy(pBan->m_ui128IpHash, pUser->m_ui128IpHash, 16);
    } else {
        if(sIp != NULL && HashIP(sIp, pBan->m_ui128IpHash) == true) {
            strcpy(pBan->m_sIp, sIp);
        } else {
			delete pBan;

            return 1;
        }
    }

    pBan->m_ui8Bits |= IP;
    
    if(bFull == true) {
        pBan->m_ui8Bits |= FULL;
    }
    
    time_t acc_time;
    time(&acc_time);

	BanItem * curBan = NULL,
        * nxtBan = FindIP(pBan->m_ui128IpHash, acc_time);
        
    // PPK ... don't add ban if is already here perm (full) ban for same ip
    while(nxtBan != NULL) {
        curBan = nxtBan;
        nxtBan = curBan->m_pHashIpTableNext;
        
        if(((curBan->m_ui8Bits & TEMP) == TEMP) == true) {
            if(((curBan->m_ui8Bits & FULL) == FULL) == false || ((pBan->m_ui8Bits & FULL) == FULL) == true) {
                if(((curBan->m_ui8Bits & NICK) == NICK) == false) {
                    Rem(curBan);
                    delete curBan;
				}

                continue;
            }

            continue;
        }

        if(((curBan->m_ui8Bits & FULL) == FULL) == false && ((pBan->m_ui8Bits & FULL) == FULL) == true) {
            if(((curBan->m_ui8Bits & NICK) == NICK) == false) {
                Rem(curBan);
                delete curBan;
			}
            continue;
        }

        delete pBan;

        return 2;
    }

    if(sReason != NULL) {
        size_t szReasonLen = strlen(sReason);
#ifdef _WIN32
        pBan->m_sReason = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen > 511 ? 512 : szReasonLen+1);
#else
		pBan->m_sReason = (char *)malloc(szReasonLen > 511 ? 512 : szReasonLen+1);
#endif
        if(pBan->m_sReason == NULL) {
            delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sReason in BanManager::BanIp\n", szReasonLen > 511 ? 512 : szReasonLen+1);

            return 1;
        }

        if(szReasonLen > 511) {
            memcpy(pBan->m_sReason, sReason, 508);
			pBan->m_sReason[510] = '.';
			pBan->m_sReason[509] = '.';
            pBan->m_sReason[508] = '.';
            szReasonLen = 511;
        } else {
            memcpy(pBan->m_sReason, sReason, szReasonLen);
        }
        pBan->m_sReason[szReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t szByLen = strlen(sBy);
        if(szByLen > 63) {
            szByLen = 63;
        }
#ifdef _WIN32
        pBan->m_sBy = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
		pBan->m_sBy = (char *)malloc(szByLen+1);
#endif
        if(pBan->m_sBy == NULL) {
            delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sBy in BanManager::BanIp\n", szByLen+1);

            return 1;
        }   
        memcpy(pBan->m_sBy, sBy, szByLen);
        pBan->m_sBy[szByLen] = '\0';
    }

    if(Add(pBan) == false) {
        delete pBan;
        return 1;
    }

	Save();

    return 0;
}
//---------------------------------------------------------------------------

bool BanManager::NickBan(User * pUser, char * sNick, char * sReason, char * sBy) {
    BanItem * pBan = new (std::nothrow) BanItem();
    if(pBan == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pBan in BanManager::NickBan\n");
    	return false;
    }

    pBan->m_ui8Bits |= PERM;

    if(pUser == NULL) {
        // PPK ... this should never happen, but to be sure ;)
        if(sNick == NULL) {
            delete pBan;

            return false;
        }

        // PPK ... bad script ban check
        if(sNick[0] == '<') {
            delete pBan;

            return false;
        }
        
        size_t szNickLen = strlen(sNick);
#ifdef _WIN32
        pBan->m_sNick = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNickLen+1);
#else
		pBan->m_sNick = (char *)malloc(szNickLen+1);
#endif
        if(pBan->m_sNick == NULL) {
            delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sNick in BanManager::NickBan\n", szNickLen+1);

            return false;
        }

        memcpy(pBan->m_sNick, sNick, szNickLen);
        pBan->m_sNick[szNickLen] = '\0';
        pBan->m_ui32NickHash = HashNick(sNick, szNickLen);
    } else {
        // PPK ... bad script ban check
        if(pUser->m_sNick[0] == '<') {
            delete pBan;

            return false;
        }

#ifdef _WIN32
        pBan->m_sNick = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, pUser->m_ui8NickLen+1);
#else
		pBan->m_sNick = (char *)malloc(pUser->m_ui8NickLen+1);
#endif
        if(pBan->m_sNick == NULL) {
            delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu8 " bytes for sNick1 in BanManager::NickBan\n", pUser->m_ui8NickLen+1);

            return false;
        }   

        memcpy(pBan->m_sNick, pUser->m_sNick, pUser->m_ui8NickLen);
        pBan->m_sNick[pUser->m_ui8NickLen] = '\0';
        pBan->m_ui32NickHash = pUser->m_ui32NickHash;

        strcpy(pBan->m_sIp, pUser->m_sIP);
        memcpy(pBan->m_ui128IpHash, pUser->m_ui128IpHash, 16);
    }

    pBan->m_ui8Bits |= NICK;

    time_t acc_time;
    time(&acc_time);

	BanItem *nxtBan = FindNick(pBan->m_ui32NickHash, acc_time, pBan->m_sNick);
    
    // PPK ... not allow same nickbans !
    if(nxtBan != NULL) {
        if(((nxtBan->m_ui8Bits & PERM) == PERM) == true) {
            delete pBan;

            return false;
        } else {
            if(((nxtBan->m_ui8Bits & IP) == IP) == true) {
                // PPK ... set old ban to ip ban only
				RemFromNickTable(nxtBan);
                nxtBan->m_ui8Bits &= ~NICK;
            } else {
                // PPK ... old ban is only nickban, remove it
                Rem(nxtBan);
                delete nxtBan;
			}
        }
    }

    if(sReason != NULL) {
        size_t szReasonLen = strlen(sReason);
#ifdef _WIN32
        pBan->m_sReason = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen > 511 ? 512 : szReasonLen+1);
#else
		pBan->m_sReason = (char *)malloc(szReasonLen > 511 ? 512 : szReasonLen+1);
#endif
        if(pBan->m_sReason == NULL) {
            delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sReason in BanManager::NickBan\n", szReasonLen > 511 ? 512 : szReasonLen+1);

            return false;
        }   

        if(szReasonLen > 511) {
            memcpy(pBan->m_sReason, sReason, 508);
			pBan->m_sReason[510] = '.';
			pBan->m_sReason[509] = '.';
            pBan->m_sReason[508] = '.';
            szReasonLen = 511;
        } else {
            memcpy(pBan->m_sReason, sReason, szReasonLen);
        }
        pBan->m_sReason[szReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t szByLen = strlen(sBy);
        if(szByLen > 63) {
            szByLen = 63;
        }
#ifdef _WIN32
        pBan->m_sBy = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
		pBan->m_sBy = (char *)malloc(szByLen+1);
#endif
        if(pBan->m_sBy == NULL) {
            delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sBy in BanManager::NickBan\n", szByLen+1);

            return false;
        }   
        memcpy(pBan->m_sBy, sBy, szByLen);
        pBan->m_sBy[szByLen] = '\0';
    }

    if(Add(pBan) == false) {
        delete pBan;
        return false;
    }

	Save();

    return true;
}
//---------------------------------------------------------------------------

void BanManager::TempBan(User * pUser, const char * sReason, char * sBy, const uint32_t ui32Minutes, const time_t &tExpireTime, const bool bFull) {
    BanItem * pBan = new (std::nothrow) BanItem();
    if(pBan == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate pBan in BanManager::TempBan\n");
    	return;
    }

    pBan->m_ui8Bits |= TEMP;

    strcpy(pBan->m_sIp, pUser->m_sIP);
    memcpy(pBan->m_ui128IpHash, pUser->m_ui128IpHash, 16);
    pBan->m_ui8Bits |= IP;
    
    if(bFull == true) {
        pBan->m_ui8Bits |= FULL;
    }

    time_t acc_time;
    time(&acc_time);

    if(tExpireTime > 0) {
        pBan->m_tTempBanExpire = tExpireTime;
    } else {
        if(ui32Minutes > 0) {
            pBan->m_tTempBanExpire = acc_time+(ui32Minutes*60);
        } else {
    	    pBan->m_tTempBanExpire = acc_time+(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFAULT_TEMP_BAN_TIME]*60);
        }
    }

    // PPK ... check for <unknown> nick -> bad ban from script
    if(pUser->m_sNick[0] != '<') {
        size_t szNickLen = strlen(pUser->m_sNick);
#ifdef _WIN32
        pBan->m_sNick = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNickLen+1);
#else
		pBan->m_sNick = (char *)malloc(szNickLen+1);
#endif
        if(pBan->m_sNick == NULL) {
            delete pBan;

            AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sNick in BanManager::TempBan\n", szNickLen+1);

            return;
        }

        memcpy(pBan->m_sNick, pUser->m_sNick, szNickLen);
        pBan->m_sNick[szNickLen] = '\0';
        pBan->m_ui32NickHash = pUser->m_ui32NickHash;
        pBan->m_ui8Bits |= NICK;

        // PPK ... not allow same nickbans ! i don't want this check here, but lame scripter find way to ban same nick multiple times :(
		BanItem *nxtBan = FindNick(pBan->m_ui32NickHash, acc_time, pBan->m_sNick);

        if(nxtBan != NULL) {
            if(((nxtBan->m_ui8Bits & PERM) == PERM) == true) {
                if(((nxtBan->m_ui8Bits & IP) == IP) == true) {
                    if(memcmp(pBan->m_ui128IpHash, nxtBan->m_ui128IpHash, 16) == 0) {
                        if(((pBan->m_ui8Bits & FULL) == FULL) == false) {
                            // PPK ... same ban and old is perm, delete new
                            delete pBan;

                            return;
                        } else {
                            if(((nxtBan->m_ui8Bits & FULL) == FULL) == true) {
                                // PPK ... same ban and old is full perm, delete new
                                delete pBan;

                                return;
                            } else {
                                // PPK ... same ban and only new is full, set new to ipban only
                                pBan->m_ui8Bits &= ~NICK;
                            }
                        }
                    }
                } else {
                    // PPK ... perm ban to same nick already exist, set new to ipban only
                    pBan->m_ui8Bits &= ~NICK;
                }
            } else {
                if(nxtBan->m_tTempBanExpire < pBan->m_tTempBanExpire) {
                    if(((nxtBan->m_ui8Bits & IP) == IP) == true) {
                        if(memcmp(pBan->m_ui128IpHash, nxtBan->m_ui128IpHash, 16) == 0) {
                            if(((nxtBan->m_ui8Bits & FULL) == FULL) == false) {
                                // PPK ... same bans, but old with lower expiration -> delete old
                                Rem(nxtBan);
                                delete nxtBan;
							} else {
                                if(((pBan->m_ui8Bits & FULL) == FULL) == false) {
                                    // PPK ... old ban with lower expiration is full ban, set old to ipban only
									RemFromNickTable(nxtBan);
                                    nxtBan->m_ui8Bits &= ~NICK;
                                } else {
                                    // PPK ... same bans, old have lower expiration -> delete old
                                    Rem(nxtBan);
                                    delete nxtBan;
								}
                            }
                        } else {
                            // PPK ... set old ban to ipban only
							RemFromNickTable(nxtBan);
                            nxtBan->m_ui8Bits &= ~NICK;
                        }
                    } else {
                        // PPK ... old ban is only nickban with lower bantime, remove it
                        Rem(nxtBan);
                        delete nxtBan;
					}
                } else {
                    if(((nxtBan->m_ui8Bits & IP) == IP) == true) {
                        if(memcmp(pBan->m_ui128IpHash, nxtBan->m_ui128IpHash, 16) == 0) {
                            if(((pBan->m_ui8Bits & FULL) == FULL) == false) {
                                // PPK ... same bans, but new with lower expiration -> delete new
                                delete pBan;

								return;
                            } else {
                                if(((nxtBan->m_ui8Bits & FULL) == FULL) == false) {
                                    // PPK ... new ban with lower expiration is full ban, set new to ipban only
                                    pBan->m_ui8Bits &= ~NICK;
                                } else {
                                    // PPK ... same bans, new have lower expiration -> delete new
                                    delete pBan;

                                    return;
                                }
                            }
                        } else {
                            // PPK ... set new ban to ipban only
                            pBan->m_ui8Bits &= ~NICK;
                        }
                    } else {
                        // PPK ... old ban is only nickban with higher bantime, set new to ipban only
                        pBan->m_ui8Bits &= ~NICK;
                    }
                }
            }
        }
    }

    // PPK ... clear bans with lower timeban and same ip without nickban and fullban if new ban is fullban
	BanItem * curBan = NULL,
        * nxtBan = FindIP(pBan->m_ui128IpHash, acc_time);

    while(nxtBan != NULL) {
        curBan = nxtBan;
        nxtBan = curBan->m_pHashIpTableNext;

        if(((curBan->m_ui8Bits & PERM) == PERM) == true) {
            continue;
        }
        
        if(((curBan->m_ui8Bits & NICK) == NICK) == true) {
            continue;
        }

        if(((curBan->m_ui8Bits & FULL) == FULL) == true && ((pBan->m_ui8Bits & FULL) == FULL) == false) {
            continue;
        }

        if(curBan->m_tTempBanExpire > pBan->m_tTempBanExpire) {
            continue;
        }
        
        Rem(curBan);
        delete curBan;
	}

    if(sReason != NULL) {
        size_t szReasonLen = strlen(sReason);
#ifdef _WIN32
        pBan->m_sReason = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen > 511 ? 512 : szReasonLen+1);
#else
		pBan->m_sReason = (char *)malloc(szReasonLen > 511 ? 512 : szReasonLen+1);
#endif
        if(pBan->m_sReason == NULL) {
            delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sReason in BanManager::TempBan\n", szReasonLen > 511 ? 512 : szReasonLen+1);

            return;
        }   

        if(szReasonLen > 511) {
            memcpy(pBan->m_sReason, sReason, 508);
			pBan->m_sReason[510] = '.';
			pBan->m_sReason[509] = '.';
            pBan->m_sReason[508] = '.';
            szReasonLen = 511;
        } else {
            memcpy(pBan->m_sReason, sReason, szReasonLen);
        }
        pBan->m_sReason[szReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t szByLen = strlen(sBy);
        if(szByLen > 63) {
            szByLen = 63;
        }
#ifdef _WIN32
        pBan->m_sBy = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
		pBan->m_sBy = (char *)malloc(szByLen+1);
#endif
        if(pBan->m_sBy == NULL) {
            delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sBy in BanManager::TempBan\n", szByLen+1);

            return;
        }   
        memcpy(pBan->m_sBy, sBy, szByLen);
        pBan->m_sBy[szByLen] = '\0';
    }

    if(Add(pBan) == false) {
        delete pBan;
        return;
    }

	Save();
}
//---------------------------------------------------------------------------

char BanManager::TempBanIp(User * pUser, char * sIp, char * sReason, char * sBy, const uint32_t ui32Minutes, const time_t &tExpireTime, const bool bFull) {
    BanItem * pBan = new (std::nothrow) BanItem();
    if(pBan == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pBan in BanManager::TempBanIp\n");
    	return 1;
    }

    pBan->m_ui8Bits |= TEMP;

    if(pUser != NULL) {
        strcpy(pBan->m_sIp, pUser->m_sIP);
        memcpy(pBan->m_ui128IpHash, pUser->m_ui128IpHash, 16);
    } else {
        if(sIp != NULL && HashIP(sIp, pBan->m_ui128IpHash) == true) {
            strcpy(pBan->m_sIp, sIp);
        } else {
            delete pBan;

            return 1;
        }
    }

    pBan->m_ui8Bits |= IP;
    
    if(bFull == true) {
        pBan->m_ui8Bits |= FULL;
    }

    time_t acc_time;
    time(&acc_time);

    if(tExpireTime > 0) {
        pBan->m_tTempBanExpire = tExpireTime;
    } else {
        if(ui32Minutes == 0) {
    	    pBan->m_tTempBanExpire = acc_time+(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFAULT_TEMP_BAN_TIME]*60);
        } else {
            pBan->m_tTempBanExpire = acc_time+(ui32Minutes*60);
        }
    }
    
	BanItem * curBan = NULL,
        * nxtBan = FindIP(pBan->m_ui128IpHash, acc_time);

    // PPK ... don't add ban if is already here perm (full) ban or longer temp ban for same ip
    while(nxtBan != NULL) {
        curBan = nxtBan;
        nxtBan = curBan->m_pHashIpTableNext;

        if(((curBan->m_ui8Bits & TEMP) == TEMP) == true && curBan->m_tTempBanExpire < pBan->m_tTempBanExpire) {
            if(((curBan->m_ui8Bits & FULL) == FULL) == false || ((pBan->m_ui8Bits & FULL) == FULL) == true) {
                if(((curBan->m_ui8Bits & NICK) == NICK) == false) {
                    Rem(curBan);
                    delete curBan;
				}

                continue;
            }

            continue;
        }

        if(((curBan->m_ui8Bits & FULL) == FULL) == false && ((pBan->m_ui8Bits & FULL) == FULL) == true) continue;

        delete pBan;

        return 2;
    }

    if(sReason != NULL) {
        size_t szReasonLen = strlen(sReason);
#ifdef _WIN32
        pBan->m_sReason = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen > 511 ? 512 : szReasonLen+1);
#else
		pBan->m_sReason = (char *)malloc(szReasonLen > 511 ? 512 : szReasonLen+1);
#endif
        if(pBan->m_sReason == NULL) {
            delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sReason in BanManager::TempBanIp\n", szReasonLen > 511 ? 512 : szReasonLen+1);

            return 1;
        }

        if(szReasonLen > 511) {
            memcpy(pBan->m_sReason, sReason, 508);
			pBan->m_sReason[510] = '.';
			pBan->m_sReason[509] = '.';
            pBan->m_sReason[508] = '.';
            szReasonLen = 511;
        } else {
            memcpy(pBan->m_sReason, sReason, szReasonLen);
        }
        pBan->m_sReason[szReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t szByLen = strlen(sBy);
        if(szByLen > 63) {
            szByLen = 63;
        }
#ifdef _WIN32
        pBan->m_sBy = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
		pBan->m_sBy = (char *)malloc(szByLen+1);
#endif
        if(pBan->m_sBy == NULL) {
            delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sBy in BanManager::TempBanIp\n", szByLen+1);

            return 1;
        }   
        memcpy(pBan->m_sBy, sBy, szByLen);
        pBan->m_sBy[szByLen] = '\0';
    }

    if(Add(pBan) == false) {
        delete pBan;
        return 1;
    }

	Save();

    return 0;
}
//---------------------------------------------------------------------------

bool BanManager::NickTempBan(User * pUser, char * sNick, char * sReason, char * sBy, const uint32_t ui32Minutes, const time_t &tExpireTime) {
    BanItem * pBan = new (std::nothrow) BanItem();
    if(pBan == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pBan in BanManager::NickTempBan\n");
    	return false;
    }

    pBan->m_ui8Bits |= TEMP;

    if(pUser == NULL) {
        // PPK ... this should never happen, but to be sure ;)
        if(sNick == NULL) {
            delete pBan;

            return false;
        }

        // PPK ... bad script ban check
        if(sNick[0] == '<') {
            delete pBan;

            return false;
        }
        
        size_t szNickLen = strlen(sNick);
#ifdef _WIN32
        pBan->m_sNick = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNickLen+1);
#else
		pBan->m_sNick = (char *)malloc(szNickLen+1);
#endif
        if(pBan->m_sNick == NULL) {
            delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sNick in BanManager::NickTempBan\n", szNickLen+1);

            return false;
        }   
        memcpy(pBan->m_sNick, sNick, szNickLen);
        pBan->m_sNick[szNickLen] = '\0';
        pBan->m_ui32NickHash = HashNick(sNick, szNickLen);
    } else {
        // PPK ... bad script ban check
        if(pUser->m_sNick[0] == '<') {
            delete pBan;

            return false;
        }

#ifdef _WIN32
        pBan->m_sNick = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, pUser->m_ui8NickLen+1);
#else
		pBan->m_sNick = (char *)malloc(pUser->m_ui8NickLen+1);
#endif
        if(pBan->m_sNick == NULL) {
            delete pBan;

            AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu8 " bytes for m_sNick1 in BanManager::NickTempBan\n", pUser->m_ui8NickLen+1);

            return false;
        }   
        memcpy(pBan->m_sNick, pUser->m_sNick, pUser->m_ui8NickLen);
        pBan->m_sNick[pUser->m_ui8NickLen] = '\0';
        pBan->m_ui32NickHash = pUser->m_ui32NickHash;

        strcpy(pBan->m_sIp, pUser->m_sIP);
        memcpy(pBan->m_ui128IpHash, pUser->m_ui128IpHash, 16);
    }

    pBan->m_ui8Bits |= NICK;

    time_t acc_time;
    time(&acc_time);

    if(tExpireTime > 0) {
        pBan->m_tTempBanExpire = tExpireTime;
    } else {
        if(ui32Minutes > 0) {
            pBan->m_tTempBanExpire = acc_time+(ui32Minutes*60);
        } else {
    	    pBan->m_tTempBanExpire = acc_time+(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFAULT_TEMP_BAN_TIME]*60);
        }
    }
    
	BanItem *nxtBan = FindNick(pBan->m_ui32NickHash, acc_time, pBan->m_sNick);

    // PPK ... not allow same nickbans !
    if(nxtBan != NULL) {
        if(((nxtBan->m_ui8Bits & PERM) == PERM) == true) {
            delete pBan;

            return false;
        } else {
            if(nxtBan->m_tTempBanExpire < pBan->m_tTempBanExpire) {
                if(((nxtBan->m_ui8Bits & IP) == IP) == true) {
                    // PPK ... set old ban to ip ban only
                    RemFromNickTable(nxtBan);
                    nxtBan->m_ui8Bits &= ~NICK;
                } else {
                    // PPK ... old ban is only nickban, remove it
                    Rem(nxtBan);
                    delete nxtBan;
				}
            } else {
                delete pBan;

                return false;
            }
        }
    }

    if(sReason != NULL) {
        size_t szReasonLen = strlen(sReason);
#ifdef _WIN32
        pBan->m_sReason = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen > 511 ? 512 : szReasonLen+1);
#else
		pBan->m_sReason = (char *)malloc(szReasonLen > 511 ? 512 : szReasonLen+1);
#endif
        if(pBan->m_sReason == NULL) {
            delete pBan;

            AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sReason in BanManager::NickTempBan\n", szReasonLen > 511 ? 512 : szReasonLen+1);

            return false;
        }   

        if(szReasonLen > 511) {
            memcpy(pBan->m_sReason, sReason, 508);
			pBan->m_sReason[510] = '.';
			pBan->m_sReason[509] = '.';
            pBan->m_sReason[508] = '.';
            szReasonLen = 511;
        } else {
            memcpy(pBan->m_sReason, sReason, szReasonLen);
        }
        pBan->m_sReason[szReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t szByLen = strlen(sBy);
        if(szByLen > 63) {
            szByLen = 63;
        }
#ifdef _WIN32
        pBan->m_sBy = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
		pBan->m_sBy = (char *)malloc(szByLen+1);
#endif
        if(pBan->m_sBy == NULL) {
            delete pBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sBy in BanManager::NickTempBan\n", szByLen+1);

            return false;
        }   
        memcpy(pBan->m_sBy, sBy, szByLen);
        pBan->m_sBy[szByLen] = '\0';
    }

    if(Add(pBan) == false) {
        delete pBan;
        return false;
    }

	Save();

    return true;
}
//---------------------------------------------------------------------------

bool BanManager::Unban(char * sWhat) {
    uint32_t hash = HashNick(sWhat, strlen(sWhat));

    time_t acc_time;
    time(&acc_time);

	BanItem *Ban = FindNick(hash, acc_time, sWhat);

    if(Ban == NULL) {
		uint8_t ui128Hash[16];
		memset(ui128Hash, 0, 16);

		if(HashIP(sWhat, ui128Hash) == true && (Ban = FindIP(ui128Hash, acc_time)) != NULL) {
            Rem(Ban);
            delete Ban;
		} else {
            return false;
        }
    } else {
        Rem(Ban);
        delete Ban;
	}

	Save();
    return true;
}
//---------------------------------------------------------------------------

bool BanManager::PermUnban(char * sWhat) {
    uint32_t hash = HashNick(sWhat, strlen(sWhat));

	BanItem *Ban = FindPermNick(hash, sWhat);

    if(Ban == NULL) {
		uint8_t ui128Hash[16];
		memset(ui128Hash, 0, 16);

		if(HashIP(sWhat, ui128Hash) == true && (Ban = FindPermIP(ui128Hash)) != NULL) {
            Rem(Ban);
            delete Ban;
		} else {
            return false;
        }
    } else {
        Rem(Ban);
        delete Ban;
	}

	Save();
    return true;
}
//---------------------------------------------------------------------------

bool BanManager::TempUnban(char * sWhat) {
    uint32_t hash = HashNick(sWhat, strlen(sWhat));

    time_t acc_time;
    time(&acc_time);

    BanItem *Ban = FindTempNick(hash, acc_time, sWhat);

    if(Ban == NULL) {
		uint8_t ui128Hash[16];
		memset(ui128Hash, 0, 16);

        if(HashIP(sWhat, ui128Hash) == true && (Ban = FindTempIP(ui128Hash, acc_time)) != NULL) {
            Rem(Ban);
            delete Ban;
		} else {
            return false;
        }
    } else {
        Rem(Ban);
        delete Ban;
	}

	Save();
    return true;
}
//---------------------------------------------------------------------------

void BanManager::RemoveAllIP(const uint8_t * ui128IpHash) {
    uint16_t ui16IpTableIdx = 0;

    if(IN6_IS_ADDR_V4MAPPED((const in6_addr *)ui128IpHash)) {
        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
    }

    IpTableItem * cur = NULL,
        * next = m_pIpTable[ui16IpTableIdx];

    BanItem * curBan = NULL, * nextBan = NULL;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(memcmp(cur->m_pFirstBan->m_ui128IpHash, ui128IpHash, 16) == 0) {
			nextBan = cur->m_pFirstBan;

            while(nextBan != NULL) {
                curBan = nextBan;
                nextBan = curBan->m_pHashIpTableNext;
                
				Rem(curBan);
                delete curBan;
            }

            return;
        }
    }
}
//---------------------------------------------------------------------------

void BanManager::RemovePermAllIP(const uint8_t * ui128IpHash) {
    uint16_t ui16IpTableIdx = 0;

    if(IN6_IS_ADDR_V4MAPPED((const in6_addr *)ui128IpHash)) {
        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
    }

    IpTableItem * cur = NULL,
        * next = m_pIpTable[ui16IpTableIdx];

    BanItem * curBan = NULL, * nextBan = NULL;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(memcmp(cur->m_pFirstBan->m_ui128IpHash, ui128IpHash, 16) == 0) {
			nextBan = cur->m_pFirstBan;

            while(nextBan != NULL) {
                curBan = nextBan;
                nextBan = curBan->m_pHashIpTableNext;

                if(((curBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM) == true) {
					Rem(curBan);
                    delete curBan;
                }
            }

            return;
        }
    }
}
//---------------------------------------------------------------------------

void BanManager::RemoveTempAllIP(const uint8_t * ui128IpHash) {
    uint16_t ui16IpTableIdx = 0;

    if(IN6_IS_ADDR_V4MAPPED((const in6_addr *)ui128IpHash)) {
        ui16IpTableIdx = ui128IpHash[14] * ui128IpHash[15];
    } else {
        ui16IpTableIdx = GetIpTableIdx(ui128IpHash);
    }

    IpTableItem * cur = NULL,
        * next = m_pIpTable[ui16IpTableIdx];

    BanItem * curBan = NULL, * nextBan = NULL;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(memcmp(cur->m_pFirstBan->m_ui128IpHash, ui128IpHash, 16) == 0) {
			nextBan = cur->m_pFirstBan;

            while(nextBan != NULL) {
                curBan = nextBan;
                nextBan = curBan->m_pHashIpTableNext;

                if(((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
					Rem(curBan);
                    delete curBan;
                }
            }

            return;
        }
    }
}
//---------------------------------------------------------------------------

bool BanManager::RangeBan(char * sIpFrom, const uint8_t * ui128FromIpHash, char * sIpTo, const uint8_t * ui128ToIpHash, char * sReason, char * sBy, const bool bFull) {
    RangeBanItem * pRangeBan = new (std::nothrow) RangeBanItem();
    if(pRangeBan == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pRangeBan in BanManager::RangeBan\n");
    	return false;
    }

    pRangeBan->m_ui8Bits |= PERM;

    strcpy(pRangeBan->m_sIpFrom, sIpFrom);
    memcpy(pRangeBan->m_ui128FromIpHash, ui128FromIpHash, 16);

    strcpy(pRangeBan->m_sIpTo, sIpTo);
    memcpy(pRangeBan->m_ui128ToIpHash, ui128ToIpHash, 16);

    if(bFull == true) {
        pRangeBan->m_ui8Bits |= FULL;
    }

    RangeBanItem * curBan = NULL,
        * nxtBan = m_pRangeBanListS;

    // PPK ... don't add range ban if is already here same perm (full) range ban
    while(nxtBan != NULL) {
        curBan = nxtBan;
        nxtBan = curBan->m_pNext;

        if(memcmp(curBan->m_ui128FromIpHash, pRangeBan->m_ui128FromIpHash, 16) != 0 || memcmp(curBan->m_ui128ToIpHash, pRangeBan->m_ui128ToIpHash, 16) != 0) {
            continue;
        }

        if((curBan->m_ui8Bits & TEMP) == TEMP) {
            continue;
        }
        
        if(((curBan->m_ui8Bits & FULL) == FULL) == false && ((pRangeBan->m_ui8Bits & FULL) == FULL) == true) {
            RemRange(curBan);
            delete curBan;

            continue;
        }
        
        delete pRangeBan;

        return false;
    }

    if(sReason != NULL) {
        size_t szReasonLen = strlen(sReason);
#ifdef _WIN32
        pRangeBan->m_sReason = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen > 511 ? 512 : szReasonLen+1);
#else
		pRangeBan->m_sReason = (char *)malloc(szReasonLen > 511 ? 512 : szReasonLen+1);
#endif
        if(pRangeBan->m_sReason == NULL) {
            delete pRangeBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sReason in BanManager::RangeBan\n", szReasonLen > 511 ? 512 : szReasonLen+1);

            return false;
        }   

        if(szReasonLen > 511) {
            memcpy(pRangeBan->m_sReason, sReason, 508);
            pRangeBan->m_sReason[510] = '.';
            pRangeBan->m_sReason[509] = '.';
            pRangeBan->m_sReason[508] = '.';
            szReasonLen = 511;
        } else {
            memcpy(pRangeBan->m_sReason, sReason, szReasonLen);
        }
        pRangeBan->m_sReason[szReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t szByLen = strlen(sBy);
        if(szByLen > 63) {
            szByLen = 63;
        }
#ifdef _WIN32
        pRangeBan->m_sBy = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
		pRangeBan->m_sBy = (char *)malloc(szByLen+1);
#endif
        if(pRangeBan->m_sBy == NULL) {
            delete pRangeBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sBy in BanManager::RangeBan\n", szByLen+1);

            return false;
        }   
        memcpy(pRangeBan->m_sBy, sBy, szByLen);
        pRangeBan->m_sBy[szByLen] = '\0';
    }

    AddRange(pRangeBan);
	Save();

    return true;
}
//---------------------------------------------------------------------------

bool BanManager::RangeTempBan(char * sIpFrom, const uint8_t * ui128FromIpHash, char * sIpTo, const uint8_t * ui128ToIpHash, char * sReason, char * sBy, const uint32_t ui32Minutes,
    const time_t &tExpireTime, const bool bFull) {
    RangeBanItem * pRangeBan = new (std::nothrow) RangeBanItem();
    if(pRangeBan == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pRangeBan in BanManager::RangeTempBan\n");
    	return false;
    }

    pRangeBan->m_ui8Bits |= TEMP;

    strcpy(pRangeBan->m_sIpFrom, sIpFrom);
    memcpy(pRangeBan->m_ui128FromIpHash, ui128FromIpHash, 16);

    strcpy(pRangeBan->m_sIpTo, sIpTo);
    memcpy(pRangeBan->m_ui128ToIpHash, ui128ToIpHash, 16);

    if(bFull == true) {
        pRangeBan->m_ui8Bits |= FULL;
    }

    time_t acc_time;
    time(&acc_time);

    if(tExpireTime > 0) {
        pRangeBan->m_tTempBanExpire = tExpireTime;
    } else {
        if(ui32Minutes > 0) {
            pRangeBan->m_tTempBanExpire = acc_time+(ui32Minutes*60);
        } else {
    	    pRangeBan->m_tTempBanExpire = acc_time+(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFAULT_TEMP_BAN_TIME]*60);
        }
    }
    
    RangeBanItem * curBan = NULL,
        * nxtBan = m_pRangeBanListS;

    // PPK ... don't add range ban if is already here same perm (full) range ban or longer temp ban for same range
    while(nxtBan != NULL) {
        curBan = nxtBan;
        nxtBan = curBan->m_pNext;

        if(memcmp(curBan->m_ui128FromIpHash, pRangeBan->m_ui128FromIpHash, 16) != 0 || memcmp(curBan->m_ui128ToIpHash, pRangeBan->m_ui128ToIpHash, 16) != 0) {
            continue;
        }

        if(((curBan->m_ui8Bits & TEMP) == TEMP) == true && curBan->m_tTempBanExpire < pRangeBan->m_tTempBanExpire) {
            if(((curBan->m_ui8Bits & FULL) == FULL) == false || ((pRangeBan->m_ui8Bits & FULL) == FULL) == true) {
                RemRange(curBan);
                delete curBan;

                continue;
            }

            continue;
        }
        
        if(((curBan->m_ui8Bits & FULL) == FULL) == false && ((pRangeBan->m_ui8Bits & FULL) == FULL) == true) {
            continue;
        }

        delete pRangeBan;

        return false;
    }

    if(sReason != NULL) {
        size_t szReasonLen = strlen(sReason);
#ifdef _WIN32
        pRangeBan->m_sReason = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szReasonLen > 511 ? 512 : szReasonLen+1);
#else
		pRangeBan->m_sReason = (char *)malloc(szReasonLen > 511 ? 512 : szReasonLen+1);
#endif
        if(pRangeBan->m_sReason == NULL) {
            delete pRangeBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sReason in BanManager::RangeTempBan\n", szReasonLen > 511 ? 512 : szReasonLen+1);

            return false;
        }   

        if(szReasonLen > 511) {
            memcpy(pRangeBan->m_sReason, sReason, 508);
            pRangeBan->m_sReason[510] = '.';
            pRangeBan->m_sReason[509] = '.';
            pRangeBan->m_sReason[508] = '.';
            szReasonLen = 511;
        } else {
            memcpy(pRangeBan->m_sReason, sReason, szReasonLen);
        }
        pRangeBan->m_sReason[szReasonLen] = '\0';
    }

    if(sBy != NULL) {
        size_t szByLen = strlen(sBy);
        if(szByLen > 63) {
            szByLen = 63;
        }
#ifdef _WIN32
        pRangeBan->m_sBy = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szByLen+1);
#else
		pRangeBan->m_sBy = (char *)malloc(szByLen+1);
#endif
        if(pRangeBan->m_sBy == NULL) {
            delete pRangeBan;

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sBy in BanManager::RangeTempBan\n", szByLen+1);

            return false;
        }   
        memcpy(pRangeBan->m_sBy, sBy, szByLen);
        pRangeBan->m_sBy[szByLen] = '\0';
    }

    AddRange(pRangeBan);
	Save();

    return true;
}
//---------------------------------------------------------------------------

bool BanManager::RangeUnban(const uint8_t * ui128FromIpHash, const uint8_t * ui128ToIpHash) {
    RangeBanItem * cur = NULL,
        * next = m_pRangeBanListS;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(memcmp(cur->m_ui128FromIpHash, ui128FromIpHash, 16) == 0 && memcmp(cur->m_ui128ToIpHash, ui128ToIpHash, 16) == 0) {
            RemRange(cur);
            delete cur;

            return true;
        }
    }

	Save();
    return false;
}
//---------------------------------------------------------------------------

bool BanManager::RangeUnban(const uint8_t * ui128FromIpHash, const uint8_t * ui128ToIpHash, const unsigned char cType) {
    RangeBanItem * cur = NULL,
        * next = m_pRangeBanListS;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if((cur->m_ui8Bits & cType) == cType && memcmp(cur->m_ui128FromIpHash, ui128FromIpHash, 16) == 0 && memcmp(cur->m_ui128ToIpHash, ui128ToIpHash, 16) == 0) {
            RemRange(cur);
            delete cur;

            return true;
        }
    }

    Save();
    return false;
}
//---------------------------------------------------------------------------
