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
#include "ResNickManager.h"
//---------------------------------------------------------------------------
#include "ServerManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
ReservedNicksManager * ReservedNicksManager::m_Ptr = NULL;
//---------------------------------------------------------------------------

ReservedNicksManager::ReservedNick::ReservedNick() : m_pPrev(NULL), m_pNext(NULL), m_sNick(NULL), m_ui32Hash(0), m_bFromScript(false) {
	// ...
}
//---------------------------------------------------------------------------

ReservedNicksManager::ReservedNick::~ReservedNick() {
#ifdef _WIN32
	if(m_sNick != NULL && HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sNick) == 0) {
        AppendDebugLog("%s - [MEM] Cannot deallocate m_sNick in ReservedNicksManager::ReservedNick::~ReservedNick\n");
    }
#else
	free(m_sNick);
#endif
}
//---------------------------------------------------------------------------

ReservedNicksManager::ReservedNick * ReservedNicksManager::ReservedNick::CreateReservedNick(const char * sNewNick, const uint32_t ui32NickHash) {
    ReservedNick * pReservedNick = new (std::nothrow) ReservedNick();

    if(pReservedNick == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate new pReservedNick in ReservedNick::CreateReservedNick\n");

        return NULL;
    }

    size_t szNickLen = strlen(sNewNick);
#ifdef _WIN32
    pReservedNick->m_sNick = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNickLen+1);
#else
	pReservedNick->m_sNick = (char *)malloc(szNickLen+1);
#endif
    if(pReservedNick->m_sNick == NULL) {
        AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes in ReservedNick::CreateReservedNick\n", szNickLen+1);

        delete pReservedNick;
        return NULL;
    }
    memcpy(pReservedNick->m_sNick, sNewNick, szNickLen);
    pReservedNick->m_sNick[szNickLen] = '\0';

	pReservedNick->m_ui32Hash = ui32NickHash;

    return pReservedNick;
}
//---------------------------------------------------------------------------

void ReservedNicksManager::Load() {
#ifdef _WIN32
	FILE * fReservedNicks = fopen((ServerManager::m_sPath + "\\cfg\\ReservedNicks.pxt").c_str(), "rt");
#else
	FILE * fReservedNicks = fopen((ServerManager::m_sPath + "/cfg/ReservedNicks.pxt").c_str(), "rt");
#endif
    if(fReservedNicks == NULL) {
    	int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "Error loading file ReservedNicks.pxt %s (%d)", 
#ifdef _WIN32
        	WSErrorStr(errno), errno);
#else
			ErrnoStr(errno), errno);
#endif
		if(iMsgLen > 0) {
#ifdef _BUILD_GUI
			::MessageBox(NULL, ServerManager::m_pGlobalBuffer, g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
			AppendLog(ServerManager::m_pGlobalBuffer);
#endif
		}

        exit(EXIT_FAILURE);
    }

	size_t szLen = 0;

	while(fgets(ServerManager::m_pGlobalBuffer, (int)ServerManager::m_szGlobalBufferSize, fReservedNicks) != NULL) {
		if(ServerManager::m_pGlobalBuffer[0] == '#' || ServerManager::m_pGlobalBuffer[0] == '\n') {
			continue;
		}

		szLen = strlen(ServerManager::m_pGlobalBuffer)-1;

		ServerManager::m_pGlobalBuffer[szLen] = '\0';

		if(ServerManager::m_pGlobalBuffer[0] == '\0') {
			continue;
		}

		AddReservedNick(ServerManager::m_pGlobalBuffer);
	}

    fclose(fReservedNicks);
}
//---------------------------------------------------------------------------

void ReservedNicksManager::Save() const {
#ifdef _WIN32
    FILE * fReservedNicks = fopen((ServerManager::m_sPath + "\\cfg\\ReservedNicks.pxt").c_str(), "wb");
#else
	FILE * fReservedNicks = fopen((ServerManager::m_sPath + "/cfg/ReservedNicks.pxt").c_str(), "wb");
#endif
    if(fReservedNicks == NULL) {
    	return;
    }

	static const char sPtokaXResNickFile[] = "#\n# PtokaX reserved nicks file\n#\n\n";
    fwrite(sPtokaXResNickFile, 1, sizeof(sPtokaXResNickFile)-1, fReservedNicks);

    ReservedNick * pCur = NULL,
        * pNext = m_pReservedNicks;

    while(pNext != NULL) {
        pCur = pNext;
        pNext = pCur->m_pNext;

		fprintf(fReservedNicks, "%s\n", pCur->m_sNick);
    }

	fclose(fReservedNicks);
}
//---------------------------------------------------------------------------

void ReservedNicksManager::LoadXML() {
	TiXmlDocument doc;

#ifdef _WIN32
	if(doc.LoadFile((ServerManager::m_sPath+"\\cfg\\ReservedNicks.xml").c_str()) == true) {
#else
	if(doc.LoadFile((ServerManager::m_sPath+"/cfg/ReservedNicks.xml").c_str()) == true) {
#endif
		TiXmlHandle cfg(&doc);
		TiXmlNode * reservednicks = cfg.FirstChild("ReservedNicks").Node();
		if(reservednicks != NULL) {
			TiXmlNode *child = NULL;
			while((child = reservednicks->IterateChildren(child)) != NULL) {
				TiXmlNode *reservednick = child->FirstChild();
                    
				if(reservednick == NULL) {
					continue;
				}

				char *sNick = (char *)reservednick->Value();
                    
				AddReservedNick(sNick);
			}
        }
    }
}
//---------------------------------------------------------------------------

ReservedNicksManager::ReservedNicksManager() : m_pReservedNicks(NULL) {
#ifdef _WIN32
    if(FileExist((ServerManager::m_sPath + "\\cfg\\ReservedNicks.pxt").c_str()) == true) {
#else
    if(FileExist((ServerManager::m_sPath + "/cfg/ReservedNicks.pxt").c_str()) == true) {
#endif
        Load();

        return;
#ifdef _WIN32
    } else if(FileExist((ServerManager::m_sPath + "\\cfg\\ReservedNicks.xml").c_str()) == true) {
#else
    } else if(FileExist((ServerManager::m_sPath + "/cfg/ReservedNicks.xml").c_str()) == true) {
#endif
        LoadXML();

        return;
    } else {
		const char * sNicks[] = { "Hub-Security", "Admin", "Client", "PtokaX", "OpChat" };
		for(uint8_t ui8i = 0; ui8i < 5; ui8i++) {
			AddReservedNick(sNicks[ui8i]);
		}

		Save();
	}
}
//---------------------------------------------------------------------------
	
ReservedNicksManager::~ReservedNicksManager() {
	Save();

    ReservedNick * cur = NULL,
        * next = m_pReservedNicks;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        delete cur;
    }
}
//---------------------------------------------------------------------------

// Check for reserved nicks true = reserved
bool ReservedNicksManager::CheckReserved(const char * sNick, const uint32_t ui32Hash) const {
    ReservedNick * cur = NULL,
        * next = m_pReservedNicks;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

		if(cur->m_ui32Hash == ui32Hash && strcasecmp(cur->m_sNick, sNick) == 0) {
            return true;
        }
    }

    return false;
}
//---------------------------------------------------------------------------

void ReservedNicksManager::AddReservedNick(const char * sNick, const bool bFromScript/* = false*/) {
    uint32_t ui32Hash = HashNick(sNick, strlen(sNick));

    if(CheckReserved(sNick, ui32Hash) == false) {
        ReservedNick * pNewNick = ReservedNick::CreateReservedNick(sNick, ui32Hash);
        if(pNewNick == NULL) {
			AppendDebugLog("%s - [MEM] Cannot allocate pNewNick in ReservedNicksManager::AddReservedNick\n");
        	return;
        }

        if(m_pReservedNicks == NULL) {
			m_pReservedNicks = pNewNick;
        } else {
			m_pReservedNicks->m_pPrev = pNewNick;
            pNewNick->m_pNext = m_pReservedNicks;
			m_pReservedNicks = pNewNick;
        }

        pNewNick->m_bFromScript = bFromScript;
    }
}
//---------------------------------------------------------------------------

void ReservedNicksManager::DelReservedNick(char * sNick, const bool bFromScript/* = false*/) {
    uint32_t ui32Hash = HashNick(sNick, strlen(sNick));

    ReservedNick * cur = NULL,
        * next = m_pReservedNicks;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(cur->m_ui32Hash == ui32Hash && strcmp(cur->m_sNick, sNick) == 0) {
            if(bFromScript == true && cur->m_bFromScript == false) {
                continue;
            }

            if(cur->m_pPrev == NULL) {
                if(cur->m_pNext == NULL) {
					m_pReservedNicks = NULL;
                } else {
                    cur->m_pNext->m_pPrev = NULL;
					m_pReservedNicks = cur->m_pNext;
                }
            } else if(cur->m_pNext == NULL) {
                cur->m_pPrev->m_pNext = NULL;
            } else {
                cur->m_pPrev->m_pNext = cur->m_pNext;
                cur->m_pNext->m_pPrev = cur->m_pPrev;
            }

            delete cur;
            return;
        }
    }
}
//---------------------------------------------------------------------------
