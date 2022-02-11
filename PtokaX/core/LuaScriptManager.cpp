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

//------------------------------------------------------------------------------
#include "stdinc.h"
//------------------------------------------------------------------------------
#include "LuaInc.h"
//------------------------------------------------------------------------------
#include "LuaScriptManager.h"
//------------------------------------------------------------------------------
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//------------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//------------------------------------------------------------------------------
#include "LuaScript.h"
//------------------------------------------------------------------------------
#ifdef _BUILD_GUI
    #include "../gui.win/MainWindowPageScripts.h"
#endif
//------------------------------------------------------------------------------
ScriptManager * ScriptManager::m_Ptr = NULL;
//------------------------------------------------------------------------------

void ScriptManager::LoadXML() {
	// PPK ... first start all script in order from xml file
#ifdef _WIN32
	TiXmlDocument doc((ServerManager::m_sPath+"\\cfg\\Scripts.xml").c_str());
#else
	TiXmlDocument doc((ServerManager::m_sPath+"/cfg/Scripts.xml").c_str());
#endif
	if(doc.LoadFile() == false) {
        if(doc.ErrorId() != TiXmlBase::TIXML_ERROR_OPENING_FILE && doc.ErrorId() != TiXmlBase::TIXML_ERROR_DOCUMENT_EMPTY) {
            int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "Error loading file Scripts.xml. %s (Col: %d, Row: %d)", doc.ErrorDesc(), doc.Column(), doc.Row());
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
		TiXmlNode *scripts = cfg.FirstChild("Scripts").Node();
		if(scripts != NULL) {
			TiXmlNode *child = NULL;
			while((child = scripts->IterateChildren(child)) != NULL) {
				TiXmlNode *script = child->FirstChild("Name");
    
				if(script == NULL || (script = script->FirstChild()) == NULL) {
					continue;
				}
    
				char *name = (char *)script->Value();

				if(FileExist((ServerManager::m_sScriptPath+string(name)).c_str()) == false) {
					continue;
				}

				if((script = child->FirstChild("Enabled")) == NULL ||
					(script = script->FirstChild()) == NULL) {
					continue;
				}
    
				bool enabled = atoi(script->Value()) == 0 ? false : true;

				if(FindScript(name) != NULL) {
					continue;
				}

				AddScript(name, enabled, false);
            }
        }
    }
}
//------------------------------------------------------------------------------

ScriptManager::ScriptManager() : m_pRunningScriptE(NULL), m_pRunningScriptS(NULL), m_ppScriptTable(NULL), m_pActualUser(NULL), m_pTimerListS(NULL), m_pTimerListE(NULL), m_ui8ScriptCount(0), m_ui8BotsCount(0), m_bMoved(false) {
#ifdef _WIN32
    ServerManager::m_hLuaHeap = ::HeapCreate(HEAP_NO_SERIALIZE, 0x80000, 0);
#endif

#ifdef _WIN32
    if(FileExist((ServerManager::m_sPath + "\\cfg\\Scripts.pxt").c_str()) == false) {
#else
    if(FileExist((ServerManager::m_sPath + "/cfg/Scripts.pxt").c_str()) == false) {
#endif
        LoadXML();

        return;
    }

#ifdef _WIN32
	FILE * fScriptsFile = fopen((ServerManager::m_sPath + "\\cfg\\Scripts.pxt").c_str(), "rt");
#else
	FILE * fScriptsFile = fopen((ServerManager::m_sPath + "/cfg/Scripts.pxt").c_str(), "rt");
#endif
    if(fScriptsFile == NULL) {
    	int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "Error loading file Scripts.pxt %s (%d)",
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

	while(fgets(ServerManager::m_pGlobalBuffer, (int)ServerManager::m_szGlobalBufferSize, fScriptsFile) != NULL) {
		if(ServerManager::m_pGlobalBuffer[0] == '#' || ServerManager::m_pGlobalBuffer[0] == '\n') {
			continue;
		}

		szLen = strlen(ServerManager::m_pGlobalBuffer)-1;

		if(szLen < 7) {
			continue;
		}

		ServerManager::m_pGlobalBuffer[szLen] = '\0';

		for(size_t szi = szLen-1; szi != 0; szi--) {
			if(isspace(ServerManager::m_pGlobalBuffer[szi-1]) != 0 || ServerManager::m_pGlobalBuffer[szi-1] == '=') {
				ServerManager::m_pGlobalBuffer[szi-1] = '\0';
				continue;
			}

			break;
		}

		if(ServerManager::m_pGlobalBuffer[0] == '\0' || (ServerManager::m_pGlobalBuffer[szLen-1] != '1' && ServerManager::m_pGlobalBuffer[szLen-1] != '0')) {
			continue;
		}

		if(FileExist((ServerManager::m_sScriptPath+string(ServerManager::m_pGlobalBuffer)).c_str()) == false || FindScript(ServerManager::m_pGlobalBuffer) != NULL) {
			continue;
		}

		AddScript(ServerManager::m_pGlobalBuffer, ServerManager::m_pGlobalBuffer[szLen-1] == '1' ? true : false, false);
	}

    fclose(fScriptsFile);
}
//------------------------------------------------------------------------------

ScriptManager::~ScriptManager() {
    m_pRunningScriptS = NULL;
    m_pRunningScriptE = NULL;

    for(uint8_t ui8i = 0; ui8i < m_ui8ScriptCount; ui8i++) {
		delete m_ppScriptTable[ui8i];
    }

#ifdef _WIN32
	if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_ppScriptTable) == 0) {
		AppendDebugLog("%s - [MEM] Cannot deallocate m_ppScriptTable in ScriptManager::~ScriptManager\n");
	}
#else
	free(m_ppScriptTable);
#endif

	m_ppScriptTable = NULL;

	m_ui8ScriptCount = 0;

    m_pActualUser = NULL;

    m_ui8BotsCount = 0;

#ifdef _WIN32
    ::HeapDestroy(ServerManager::m_hLuaHeap);
#endif
}
//------------------------------------------------------------------------------

void ScriptManager::Start() {
	m_ui8BotsCount = 0;
	m_pActualUser = NULL;

    
    // PPK ... first look for deleted and new scripts
    CheckForDeletedScripts();
    CheckForNewScripts();

    // PPK ... second start all enabled scripts
    for(uint8_t ui8i = 0; ui8i < m_ui8ScriptCount; ui8i++) {
		if(m_ppScriptTable[ui8i]->m_bEnabled == true) {
        	if(ScriptStart(m_ppScriptTable[ui8i]) == true) {
				AddRunningScript(m_ppScriptTable[ui8i]);
			} else {
				m_ppScriptTable[ui8i]->m_bEnabled = false;
            }
		}
	}

#ifdef _BUILD_GUI
    MainWindowPageScripts::m_Ptr->AddScriptsToList(true);
#endif
}
//------------------------------------------------------------------------------

#ifdef _BUILD_GUI
bool ScriptManager::AddScript(char * sName, const bool bEnabled, const bool bNew) {
#else
bool ScriptManager::AddScript(char * sName, const bool bEnabled, const bool /*bNew*/) {
#endif
	if(m_ui8ScriptCount == 254) {
        return false;
    }

    Script ** oldbuf = m_ppScriptTable;
#ifdef _WIN32
    if(m_ppScriptTable == NULL) {
		m_ppScriptTable = (Script **)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, (m_ui8ScriptCount+1)*sizeof(Script *));
    } else {
		m_ppScriptTable = (Script **)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, (m_ui8ScriptCount+1)*sizeof(Script *));
    }
#else
	m_ppScriptTable = (Script **)realloc(oldbuf, (m_ui8ScriptCount+1)*sizeof(Script *));
#endif
	if(m_ppScriptTable == NULL) {
		m_ppScriptTable = oldbuf;

		AppendDebugLog("%s - [MEM] Cannot (re)allocate m_ppScriptTable in ScriptManager::AddScript\n");
        return false;
    }

	m_ppScriptTable[m_ui8ScriptCount] = Script::CreateScript(sName, bEnabled);

    if(m_ppScriptTable[m_ui8ScriptCount] == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate new Script in ScriptManager::AddScript\n");

        return false;
    }

	m_ui8ScriptCount++;

#ifdef _BUILD_GUI
    if(bNew == true) {
        MainWindowPageScripts::m_Ptr->ScriptToList(m_ui8ScriptCount-1, true, false);
    }
#endif

    return true;
}
//------------------------------------------------------------------------------

void ScriptManager::Stop() {
	Script * S = NULL,
        * next = m_pRunningScriptS;

	m_pRunningScriptS = NULL;
	m_pRunningScriptE = NULL;

    while(next != NULL) {
		S = next;
		next = S->m_pNext;

		ScriptStop(S);
	}

	m_pActualUser = NULL;

#ifdef _BUILD_GUI
    MainWindowPageScripts::m_Ptr->ClearMemUsageAll();
#endif
}
//------------------------------------------------------------------------------

void ScriptManager::AddRunningScript(Script * pScript) {
	if(m_pRunningScriptS == NULL) {
		m_pRunningScriptS = pScript;
		m_pRunningScriptE = pScript;
		return;
	}

   	pScript->m_pPrev = m_pRunningScriptE;
	m_pRunningScriptE->m_pNext = pScript;
	m_pRunningScriptE = pScript;
}
//------------------------------------------------------------------------------

void ScriptManager::RemoveRunningScript(Script * pScript) {
	// single entry
	if(pScript->m_pPrev == NULL && pScript->m_pNext == NULL) {
		m_pRunningScriptS = NULL;
		m_pRunningScriptE = NULL;
        return;
    }

    // first in list
	if(pScript->m_pPrev == NULL) {
		m_pRunningScriptS = pScript->m_pNext;
		m_pRunningScriptS->m_pPrev = NULL;
        return;
    }

    // last in list
    if(pScript->m_pNext == NULL) {
		m_pRunningScriptE = pScript->m_pPrev;
		m_pRunningScriptE->m_pNext = NULL;
        return;
    }

    // in the middle
    pScript->m_pPrev->m_pNext = pScript->m_pNext;
    pScript->m_pNext->m_pPrev = pScript->m_pPrev;
}
//------------------------------------------------------------------------------

void ScriptManager::SaveScripts() {
#ifdef _WIN32
    FILE * fScriptsFile = fopen((ServerManager::m_sPath + "\\cfg\\Scripts.pxt").c_str(), "wb");
#else
	FILE * fScriptsFile = fopen((ServerManager::m_sPath + "/cfg/Scripts.pxt").c_str(), "wb");
#endif
    if(fScriptsFile == NULL) {
    	return;
    }

	static const char sPtokaXScriptsFile[] = "#\n# PtokaX scripts settings file\n#\n\n";
    fwrite(sPtokaXScriptsFile, 1, sizeof(sPtokaXScriptsFile)-1, fScriptsFile);

	for(uint8_t ui8i = 0; ui8i < m_ui8ScriptCount; ui8i++) {
		if(FileExist((ServerManager::m_sScriptPath+string(m_ppScriptTable[ui8i]->m_sName)).c_str()) == false) {
			continue;
        }

        fprintf(fScriptsFile, "%s\t=\t%c\n", m_ppScriptTable[ui8i]->m_sName, m_ppScriptTable[ui8i]->m_bEnabled == true ? '1' : '0');
    }

    fclose(fScriptsFile);
}
//------------------------------------------------------------------------------

void ScriptManager::CheckForDeletedScripts() {
    uint8_t ui8i = 0;

	while(ui8i < m_ui8ScriptCount) {
		if(FileExist((ServerManager::m_sScriptPath+string(m_ppScriptTable[ui8i]->m_sName)).c_str()) == true || m_ppScriptTable[ui8i]->m_pLua != NULL) {
			ui8i++;
			continue;
        }

		delete m_ppScriptTable[ui8i];

		for(uint8_t ui8j = ui8i; ui8j+1 < m_ui8ScriptCount; ui8j++) {
			m_ppScriptTable[ui8j] = m_ppScriptTable[ui8j+1];
        }

		m_ppScriptTable[m_ui8ScriptCount-1] = NULL;
		m_ui8ScriptCount--;
    }
}
//------------------------------------------------------------------------------

void ScriptManager::CheckForNewScripts() {
#ifdef _WIN32
    struct _finddata_t luafile;
    intptr_t hFile = _findfirst((ServerManager::m_sScriptPath+"\\*.lua").c_str(), &luafile);

    if(hFile != -1) {
        do {
			if((luafile.attrib & _A_SUBDIR) != 0 ||
				stricmp(luafile.name+(strlen(luafile.name)-4), ".lua") != 0) {
				continue;
			}

			if(FindScript(luafile.name) != NULL) {
                continue;
            }

			AddScript(luafile.name, false, false);
        } while(_findnext(hFile, &luafile) == 0);

		_findclose(hFile);
	}
#else
    DIR * p_scriptdir = opendir(ServerManager::m_sScriptPath.c_str());

    if(p_scriptdir == NULL) {
        return;
    }

    struct dirent * p_dirent;
    struct stat s_buf;

    while((p_dirent = readdir(p_scriptdir)) != NULL) {
        if(stat((ServerManager::m_sScriptPath + p_dirent->d_name).c_str(), &s_buf) != 0 ||
            (s_buf.st_mode & S_IFDIR) != 0 || 
            strcasecmp(p_dirent->d_name + (strlen(p_dirent->d_name)-4), ".lua") != 0) {
            continue;
        }

		if(FindScript(p_dirent->d_name) != NULL) {
            continue;
        }

		AddScript(p_dirent->d_name, false, false);
    }

    closedir(p_scriptdir);
#endif
}
//------------------------------------------------------------------------------

void ScriptManager::Restart() {
	OnExit();
	Stop();

    CheckForDeletedScripts();

	Start();
	OnStartup();

#ifdef _BUILD_GUI
    MainWindowPageScripts::m_Ptr->AddScriptsToList(true);
#endif
}
//------------------------------------------------------------------------------

Script * ScriptManager::FindScript(char * sName) {
    for(uint8_t ui8i = 0; ui8i < m_ui8ScriptCount; ui8i++) {
		if(strcasecmp(m_ppScriptTable[ui8i]->m_sName, sName) == 0) {
            return m_ppScriptTable[ui8i];
        }
    }

    return NULL;
}
//------------------------------------------------------------------------------

Script * ScriptManager::FindScript(lua_State * pLua) {
    Script * cur = NULL,
        * next = m_pRunningScriptS;

    while(next != NULL) {
    	cur = next;
        next = cur->m_pNext;

        if(cur->m_pLua == pLua) {
            return cur;
        }
    }

    return NULL;
}
//------------------------------------------------------------------------------

uint8_t ScriptManager::FindScriptIdx(char * sName) {
    for(uint8_t ui8i = 0; ui8i < m_ui8ScriptCount; ui8i++) {
		if(strcasecmp(m_ppScriptTable[ui8i]->m_sName, sName) == 0) {
            return ui8i;
        }
    }

    return m_ui8ScriptCount;
}
//------------------------------------------------------------------------------

bool ScriptManager::StartScript(Script * pScript, const bool bEnable) {
    uint8_t ui8dx = 255;
    for(uint8_t ui8i = 0; ui8i < m_ui8ScriptCount; ui8i++) {
        if(pScript == m_ppScriptTable[ui8i]) {
            ui8dx = ui8i;
            break;
        }
    }

    if(ui8dx == 255) {
        return false;
	}

    if(bEnable == true) {
        pScript->m_bEnabled = true;
#ifdef _BUILD_GUI
        MainWindowPageScripts::m_Ptr->UpdateCheck(ui8dx);
#endif
    }

    if(ScriptStart(pScript) == false) {
        pScript->m_bEnabled = false;
#ifdef _BUILD_GUI
        MainWindowPageScripts::m_Ptr->UpdateCheck(ui8dx);
#endif
        return false;
    }

	if(m_pRunningScriptS == NULL) {
		m_pRunningScriptS = pScript;
		m_pRunningScriptE = pScript;
	} else {
		// previous script
		if(ui8dx != 0) {
			for(int16_t i16i = (int16_t)(ui8dx-1); i16i > -1; i16i--) {
				if(m_ppScriptTable[i16i]->m_bEnabled == true) {
					m_ppScriptTable[i16i]->m_pNext = pScript;
					pScript->m_pPrev = m_ppScriptTable[i16i];
					break;
				}
			}

			if(pScript->m_pPrev == NULL) {
				m_pRunningScriptS = pScript;
			}
		} else {
			pScript->m_pNext = m_pRunningScriptS;
			m_pRunningScriptS->m_pPrev = pScript;
			m_pRunningScriptS = pScript;
        }

		// next script
		if(ui8dx != m_ui8ScriptCount-1) {
			for(uint8_t ui8i = ui8dx+1; ui8i < m_ui8ScriptCount; ui8i++) {
				if(m_ppScriptTable[ui8i]->m_bEnabled == true) {
					m_ppScriptTable[ui8i]->m_pPrev = pScript;
					pScript->m_pNext = m_ppScriptTable[ui8i];
					break;
				}
			}

			if(pScript->m_pNext == NULL) {
				m_pRunningScriptE = pScript;
			}
		} else {
			pScript->m_pPrev = m_pRunningScriptE;
			m_pRunningScriptE->m_pNext = pScript;
			m_pRunningScriptE = pScript;
        }
	}


	if(ServerManager::m_bServerRunning == true) {
        ScriptOnStartup(pScript);
    }

    return true;
}
//------------------------------------------------------------------------------

void ScriptManager::StopScript(Script * pScript, const bool bDisable) {
    if(bDisable == true) {
		pScript->m_bEnabled = false;

#ifdef _BUILD_GUI
	    for(uint8_t ui8i = 0; ui8i < m_ui8ScriptCount; ui8i++) {
            if(pScript == m_ppScriptTable[ui8i]) {
                MainWindowPageScripts::m_Ptr->UpdateCheck(ui8i);
                break;
            }
        }
#endif
    }

	RemoveRunningScript(pScript);

    if(ServerManager::m_bServerRunning == true) {
        ScriptOnExit(pScript);
    }

	ScriptStop(pScript);
}
//------------------------------------------------------------------------------

void ScriptManager::MoveScript(const uint8_t ui8ScriptPosInTbl, const bool bUp) {
    if(bUp == true) {
		if(ui8ScriptPosInTbl == 0) {
            return;
        }

        Script * pScript = m_ppScriptTable[ui8ScriptPosInTbl];
		m_ppScriptTable[ui8ScriptPosInTbl] = m_ppScriptTable[ui8ScriptPosInTbl-1];
		m_ppScriptTable[ui8ScriptPosInTbl-1] = pScript;

		// if one of moved scripts not running then return
		if(pScript->m_pLua == NULL || m_ppScriptTable[ui8ScriptPosInTbl]->m_pLua == NULL) {
			return;
		}

		if(pScript->m_pPrev == NULL) { // first running script, nothing to move
			return;
		} else if(pScript->m_pNext == NULL) { // last running script
			// set prev script as last
			m_pRunningScriptE = pScript->m_pPrev;

			// change prev prev script next
			if(m_pRunningScriptE->m_pPrev != NULL) {
				m_pRunningScriptE->m_pPrev->m_pNext = pScript;
			} else {
				m_pRunningScriptS = pScript;
			}

			// change current script prev and next
			pScript->m_pPrev = m_pRunningScriptE->m_pPrev;
			pScript->m_pNext = m_pRunningScriptE;

			// change prev script prev to current and his next to NULL
			m_pRunningScriptE->m_pPrev = pScript;
			m_pRunningScriptE->m_pNext = NULL;
		} else { // not first, not last ...
			// remember original prev and next
			Script * prev = pScript->m_pPrev;
			Script * next = pScript->m_pNext;

			// change current script prev
			pScript->m_pPrev = prev->m_pPrev;

			// change prev script next
			prev->m_pNext = next;

			// change current script next
			pScript->m_pNext = prev;

			// change next script prev
			next->m_pPrev = prev;

			// change prev prev script next
			if(prev->m_pPrev != NULL) {
				prev->m_pPrev->m_pNext = pScript;
			} else {
				m_pRunningScriptS = pScript;
			}

			// change prev script prev
			prev->m_pPrev = pScript;
		}
    } else {
		if(ui8ScriptPosInTbl == m_ui8ScriptCount-1) {
            return;
		}

        Script * pScript = m_ppScriptTable[ui8ScriptPosInTbl];
		m_ppScriptTable[ui8ScriptPosInTbl] = m_ppScriptTable[ui8ScriptPosInTbl+1];
		m_ppScriptTable[ui8ScriptPosInTbl+1] = pScript;

		// if one of moved scripts not running then return
		if(pScript->m_pLua == NULL || m_ppScriptTable[ui8ScriptPosInTbl]->m_pLua == NULL) {
			return;
		}

        if(pScript->m_pNext == NULL) { // last running script, nothing to move
            return;
        } else if(pScript->m_pPrev == NULL) { // first running script
            //set next running script as first
			m_pRunningScriptS = pScript->m_pNext;

            // change next next script prev
			if(m_pRunningScriptS->m_pNext != NULL) {
				m_pRunningScriptS->m_pNext->m_pPrev = pScript;
			} else {
				m_pRunningScriptE = pScript;
			}

			// change current script prev and next
			pScript->m_pPrev = m_pRunningScriptS;
			pScript->m_pNext = m_pRunningScriptS->m_pNext;

            // change next script next to current and his prev to NULL
			m_pRunningScriptS->m_pPrev = NULL;
			m_pRunningScriptS->m_pNext = pScript;
		} else { // not first, not last ...
            // remember original prev and next
            Script * prev = pScript->m_pPrev;
            Script * next = pScript->m_pNext;

			// change current script next
			pScript->m_pNext = next->m_pNext;

			// change next script prev
			next->m_pPrev = prev;

			// change current script prev
			pScript->m_pPrev = next;

			// change prev script next
			prev->m_pNext = next;

			// change next next script prev
            if(next->m_pNext != NULL) {
                next->m_pNext->m_pPrev = pScript;
			} else {
				m_pRunningScriptE = pScript;
			}

			// change next script next
			next->m_pNext = pScript;
		}
	}
}
//------------------------------------------------------------------------------

void ScriptManager::DeleteScript(const uint8_t ui8ScriptPosInTbl) {
    Script * pScript = m_ppScriptTable[ui8ScriptPosInTbl];

	if(pScript->m_pLua != NULL) {
		StopScript(pScript, false);
	}

	if(FileExist((ServerManager::m_sScriptPath+string(pScript->m_sName)).c_str()) == true) {
#ifdef _WIN32
        DeleteFile((ServerManager::m_sScriptPath+string(pScript->m_sName)).c_str());
#else
		unlink((ServerManager::m_sScriptPath+string(pScript->m_sName)).c_str());
#endif
    }

    delete pScript;

	for(uint8_t ui8i = ui8ScriptPosInTbl; ui8i+1 < m_ui8ScriptCount; ui8i++) {
		m_ppScriptTable[ui8i] = m_ppScriptTable[ui8i+1];
    }

	m_ppScriptTable[m_ui8ScriptCount-1] = NULL;
	m_ui8ScriptCount--;
}
//------------------------------------------------------------------------------

void ScriptManager::OnStartup() {
    if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
        return;
    }

	m_pActualUser = NULL;
	m_bMoved = false;

    Script * pScript = NULL,
        * next = m_pRunningScriptS;
        
    while(next != NULL) {
		pScript = next;
        next = pScript->m_pNext;

		if(((pScript->m_ui16Functions & Script::ONSTARTUP) == Script::ONSTARTUP) == true && (m_bMoved == false || pScript->m_bProcessed == false)) {
			pScript->m_bProcessed = true;
            ScriptOnStartup(pScript);
        }
	}
}
//------------------------------------------------------------------------------

void ScriptManager::OnExit(const bool bForce/* = false*/) {
    if(bForce == false && SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
        return;
    }

    m_pActualUser = NULL;
	m_bMoved = false;

    Script * pScript = NULL,
        * next = m_pRunningScriptS;
        
    while(next != NULL) {
		pScript = next;
        next = pScript->m_pNext;

		if(((pScript->m_ui16Functions & Script::ONEXIT) == Script::ONEXIT) == true && (m_bMoved == false || pScript->m_bProcessed == false)) {
			pScript->m_bProcessed = true;
            ScriptOnExit(pScript);
        }
    }
}
//------------------------------------------------------------------------------

bool ScriptManager::Arrival(DcCommand * pDcCommand, const uint8_t ui8Type) {
	if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
		return false;
	}

	static const uint32_t iLuaArrivalBits[] = {
        0x1, 
        0x2, 
        0x4, 
        0x8, 
        0x10, 
        0x20, 
        0x40, 
        0x80, 
        0x100, 
        0x200, 
        0x400, 
        0x800, 
        0x1000, 
        0x2000, 
        0x4000, 
        0x8000, 
        0x10000, 
        0x20000, 
        0x40000, 
        0x80000, 
        0x100000
	};

	m_bMoved = false;

    int iTop = 0, iTraceback = 0;

    Script * cur = NULL,
        * next = m_pRunningScriptS;
        
    while(next != NULL) {
    	cur = next;
        next = cur->m_pNext;

        // if any of the scripts returns a nonzero value,
		// then stop for all other scripts
        if(((cur->m_ui32DataArrivals & iLuaArrivalBits[ui8Type]) == iLuaArrivalBits[ui8Type]) == true && (m_bMoved == false || cur->m_bProcessed == false)) {
            cur->m_bProcessed = true;

            lua_pushcfunction(cur->m_pLua, ScriptTraceback);
            iTraceback = lua_gettop(cur->m_pLua);

            // PPK ... table of arrivals
            static const char* arrival[] = { "ChatArrival", "KeyArrival", "ValidateNickArrival", "PasswordArrival",
            "VersionArrival", "GetNickListArrival", "MyINFOArrival", "GetINFOArrival", "SearchArrival",
            "ToArrival", "ConnectToMeArrival", "MultiConnectToMeArrival", "RevConnectToMeArrival", "SRArrival",
            "UDPSRArrival", "KickArrival", "OpForceMoveArrival", "SupportsArrival", "BotINFOArrival", 
            "CloseArrival", "UnknownArrival" };

            lua_getglobal(cur->m_pLua, arrival[ui8Type]);
            iTop = lua_gettop(cur->m_pLua);
            if(lua_isfunction(cur->m_pLua, iTop) == 0) {
                cur->m_ui32DataArrivals &= ~iLuaArrivalBits[ui8Type];

                lua_settop(cur->m_pLua, 0);
                continue;
            }

			m_pActualUser = pDcCommand->m_pUser;

            lua_checkstack(cur->m_pLua, 2); // we need 2 empty slots in stack, check it to be sure

			ScriptPushUser(cur->m_pLua, pDcCommand->m_pUser); // usertable
            lua_pushlstring(cur->m_pLua, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen); // sData

            // two passed parameters, zero returned
            if(lua_pcall(cur->m_pLua, 2, LUA_MULTRET, iTraceback) != 0) {
                ScriptError(cur);

                lua_settop(cur->m_pLua, 0);
                continue;
            }

			m_pActualUser = NULL;
        
            // check the return value
            // if no return value specified, continue
            // if non-boolean value returned, continue
            // if a boolean true value dwels on the stack, return it

            iTop = lua_gettop(cur->m_pLua);
        
            // no return value
            if(iTop == 0) {
                continue;
            }

			if(lua_type(cur->m_pLua, iTop) != LUA_TBOOLEAN || lua_toboolean(cur->m_pLua, iTop) == 0) {
                lua_settop(cur->m_pLua, 0);
                continue;
            }

            // clear the stack for sure
            lua_settop(cur->m_pLua, 0);

			return true; // true means DO NOT process data by the hub's core
        }
    }

    return false;
}
//------------------------------------------------------------------------------

bool ScriptManager::UserConnected(User * pUser) {
	if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
        return false;
    }

    uint8_t ui8Type = 0; // User
    if(pUser->m_i32Profile != -1) {
        if(((pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            ui8Type = 1; // Reg
		} else {
			ui8Type = 2; // OP
		}
    }

	m_bMoved = false;

    int iTop = 0, iTraceback = 0;

    Script * cur = NULL,
        * next = m_pRunningScriptS;
        
    while(next != NULL) {
    	cur = next;
        next = cur->m_pNext;

		static const uint32_t iConnectedBits[] = { Script::USERCONNECTED, Script::REGCONNECTED, Script::OPCONNECTED };

		if(((cur->m_ui16Functions & iConnectedBits[ui8Type]) == iConnectedBits[ui8Type]) == true && (m_bMoved == false || cur->m_bProcessed == false)) {
            cur->m_bProcessed = true;

            lua_pushcfunction(cur->m_pLua, ScriptTraceback);
            iTraceback = lua_gettop(cur->m_pLua);

            // PPK ... table of connected functions
            static const char* ConnectedFunction[] = { "UserConnected", "RegConnected", "OpConnected" };

            lua_getglobal(cur->m_pLua, ConnectedFunction[ui8Type]);
            iTop = lua_gettop(cur->m_pLua);
			if(lua_isfunction(cur->m_pLua, iTop) == 0) {
				switch(ui8Type) {
					case 0:
						cur->m_ui16Functions &= ~Script::USERCONNECTED;
						break;
					case 1:
						cur->m_ui16Functions &= ~Script::REGCONNECTED;
						break;
					case 2:
						cur->m_ui16Functions &= ~Script::OPCONNECTED;
						break;
				}

                lua_settop(cur->m_pLua, 0);
                continue;
            }

			m_pActualUser = pUser;

            lua_checkstack(cur->m_pLua, 1); // we need 1 empty slots in stack, check it to be sure

			ScriptPushUser(cur->m_pLua, pUser); // usertable

            // 1 passed parameters, zero returned
			if(lua_pcall(cur->m_pLua, 1, LUA_MULTRET, iTraceback) != 0) {
                ScriptError(cur);

                lua_settop(cur->m_pLua, 0);
                continue;
            }

			m_pActualUser = NULL;
            
            // check the return value
            // if no return value specified, continue
            // if non-boolean value returned, continue
            // if a boolean true value dwels on the stack, return
        
            iTop = lua_gettop(cur->m_pLua);
        
            // no return value
            if(iTop == 0) {
                continue;
            }
        
			if(lua_type(cur->m_pLua, iTop) != LUA_TBOOLEAN || lua_toboolean(cur->m_pLua, iTop) == 0) {
                lua_settop(cur->m_pLua, 0);
                continue;
            }
       
            // clear the stack for sure
            lua_settop(cur->m_pLua, 0);

            UserDisconnected(pUser, cur);

            return true; // means DO NOT process by next scripts
        }
    }

    return false;
}
//------------------------------------------------------------------------------

void ScriptManager::UserDisconnected(User * pUser, Script * pScript/* = NULL*/) {
	if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
        return;
    }

    uint8_t ui8Type = 0; // User
    if(pUser->m_i32Profile != -1) {
        if(((pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            ui8Type = 1; // Reg
		} else {
			ui8Type = 2; // OP
		}
    }

	m_bMoved = false;

    int iTop = 0, iTraceback = 0;

    Script * cur = NULL,
        * next = m_pRunningScriptS;
        
    while(next != NULL) {
    	cur = next;
        next = cur->m_pNext;

        if(cur == pScript) {
            return;
        }

		static const uint32_t iDisconnectedBits[] = { Script::USERDISCONNECTED, Script::REGDISCONNECTED, Script::OPDISCONNECTED };

        if(((cur->m_ui16Functions & iDisconnectedBits[ui8Type]) == iDisconnectedBits[ui8Type]) == true && (m_bMoved == false || cur->m_bProcessed == false)) {
            cur->m_bProcessed = true;

            lua_pushcfunction(cur->m_pLua, ScriptTraceback);
            iTraceback = lua_gettop(cur->m_pLua);

            // PPK ... table of disconnected functions
            static const char* DisconnectedFunction[] = { "UserDisconnected", "RegDisconnected", "OpDisconnected" };

            lua_getglobal(cur->m_pLua, DisconnectedFunction[ui8Type]);
            iTop = lua_gettop(cur->m_pLua);
            if(lua_isfunction(cur->m_pLua, iTop) == 0) {
				switch(ui8Type) {
					case 0:
						cur->m_ui16Functions &= ~Script::USERDISCONNECTED;
						break;
					case 1:
						cur->m_ui16Functions &= ~Script::REGDISCONNECTED;
						break;
					case 2:
						cur->m_ui16Functions &= ~Script::OPDISCONNECTED;
						break;
				}

                lua_settop(cur->m_pLua, 0);
                continue;
            }

			m_pActualUser = pUser;

            lua_checkstack(cur->m_pLua, 1); // we need 1 empty slots in stack, check it to be sure

			ScriptPushUser(cur->m_pLua, pUser); // usertable

            // 1 passed parameters, zero returned
			if(lua_pcall(cur->m_pLua, 1, 0, iTraceback) != 0) {
                ScriptError(cur);

                lua_settop(cur->m_pLua, 0);
                continue;
            }

			m_pActualUser = NULL;

            // clear the stack for sure
            lua_settop(cur->m_pLua, 0);
        }
    }
}
//------------------------------------------------------------------------------

void ScriptManager::PrepareMove(lua_State * pLua) {
    if(m_bMoved == true) {
        return;
    }

    bool bBefore = true;

	m_bMoved = true;

    Script * cur = NULL,
        * next = m_pRunningScriptS;
        
    while(next != NULL) {
    	cur = next;
        next = cur->m_pNext;

        if(bBefore == true) {
            cur->m_bProcessed = true;
        } else {
            cur->m_bProcessed = false;
        }

        if(cur->m_pLua == pLua) {
            bBefore = false;
        }
    }
}
//------------------------------------------------------------------------------
