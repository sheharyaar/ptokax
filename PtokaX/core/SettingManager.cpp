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
#include "SettingCom.h"
#include "SettingDefaults.h"
#include "SettingManager.h"
#include "SettingStr.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "GlobalDataQueue.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "ResNickManager.h"
#include "ServerThread.h"
#include "TextFileManager.h"
#include "UDPThread.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
	#include "../gui.win/MainWindow.h"
#endif
#ifdef _WITH_SQLITE
	#include "DB-SQLite.h"
#elif _WITH_POSTGRES
	#include "DB-PostgreSQL.h"
#elif _WITH_MYSQL
	#include "DB-MySQL.h"
#endif
//---------------------------------------------------------------------------
static const char* sMin = "[min]";
static const char* sMax = "[max]";
static const char * sHubSec = "Hub-Security";
//---------------------------------------------------------------------------
SettingManager * SettingManager::m_Ptr = NULL;
//---------------------------------------------------------------------------

SettingManager::SettingManager(void) : m_ui64MinShare(0), m_ui64MaxShare(0), m_sMOTD(NULL), m_ui16MOTDLen(0), m_bBotsSameNick(false), m_bUpdateLocked(true), m_bFirstRun(false),
	m_ui8FullMyINFOOption(0) {
#ifdef _WIN32
    InitializeCriticalSection(&m_csSetting);
#else
	pthread_mutex_init(&m_mtxSetting, NULL);
#endif

    memset(m_sPreTexts, 0 , sizeof(m_sPreTexts));
    memset(m_ui16PreTextsLens, 0 , sizeof(m_ui16PreTextsLens));

    memset(m_sTexts, 0 , sizeof(m_sTexts));
    memset(m_ui16TextsLens, 0 , sizeof(m_ui16TextsLens));

    memset(m_i16Shorts, 0 , sizeof(m_i16Shorts));

    memset(m_ui16PortNumbers, 0 , sizeof(m_ui16PortNumbers));
    
    memset(m_bBools, 0 , sizeof(m_bBools));

    // Read default bools
    for(size_t szi = 0; szi < SETBOOL_IDS_END; szi++) {
        SetBool(szi, SetBoolDef[szi]);
	}

    // Read default shorts
    for(size_t szi = 0; szi < SETSHORT_IDS_END; szi++) {
        SetShort(szi, SetShortDef[szi]);
	}

    // Read default texts
	for(size_t szi = 0; szi < SETTXT_IDS_END; szi++) {
		SetText(szi, SetTxtDef[szi]);
	}

    // Load settings
	Load();
}
//---------------------------------------------------------------------------

SettingManager::~SettingManager(void) {
    Save();

    if(m_sMOTD != NULL) {
#ifdef _WIN32
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sMOTD) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_sMOTD in SettingManager::~SettingManager\n");
        }
#else
		free(m_sMOTD);
#endif
    }

    for(size_t szi = 0; szi < SETTXT_IDS_END; szi++) {
        if(m_sTexts[szi] == NULL) {
            continue;
        }

#ifdef _WIN32
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sTexts[szi]) == 0) {
			AppendDebugLogFormat("[MEM] Cannot deallocate m_sTexts[%zu] in SettingManager::~SettingManager\n", szi);
        }
#else
		free(m_sTexts[szi]);
#endif
    }

    for(size_t szi = 0; szi < SETPRETXT_IDS_END; szi++) {
        if(m_sPreTexts[szi] == NULL || (szi == SETPRETXT_HUB_SEC && m_sPreTexts[szi] == sHubSec)) {
            continue;
        }

#ifdef _WIN32
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sPreTexts[szi]) == 0) {
			AppendDebugLogFormat("[MEM] Cannot deallocate m_sPreTexts[%zu] in SettingManager::~SettingManager\n", szi);
        }
#else
		free(m_sPreTexts[szi]);
#endif
    }

#ifdef _WIN32
    DeleteCriticalSection(&m_csSetting);
#else
	pthread_mutex_destroy(&m_mtxSetting);
#endif
}
//---------------------------------------------------------------------------

void SettingManager::CreateDefaultMOTD() {
#ifdef _WIN32
	m_sMOTD = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, 18);
#else
	m_sMOTD = (char *)malloc(18);
#endif
    if(m_sMOTD == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate 18 bytes for sMOTD in SettingManager::CreateDefaultMOTD\n");
        exit(EXIT_FAILURE);
    }
    memcpy(m_sMOTD, "Welcome to PtokaX", 17);
	m_sMOTD[17] = '\0';
	m_ui16MOTDLen = 17;
}
//---------------------------------------------------------------------------

void SettingManager::LoadMOTD() {
    if(m_sMOTD != NULL) {
#ifdef _WIN32
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sMOTD) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_sMOTD in SettingManager::LoadMOTD\n");
        }
#else
		free(m_sMOTD);
#endif
		m_sMOTD = NULL;
		m_ui16MOTDLen = 0;
    }

#ifdef _WIN32
	FILE *fr = fopen((ServerManager::m_sPath + "\\cfg\\Motd.txt").c_str(), "rb");
#else
	FILE * fr = fopen((ServerManager::m_sPath + "/cfg/Motd.txt").c_str(), "rb");
#endif
    if(fr != NULL) {
        fseek(fr, 0, SEEK_END);
        uint32_t ulflen = ftell(fr);
        if(ulflen > 0) {
            fseek(fr, 0, SEEK_SET);
			m_ui16MOTDLen = (uint16_t)(ulflen < 65024 ? ulflen : 65024);

            // allocate memory for sMOTD
#ifdef _WIN32
			m_sMOTD = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, m_ui16MOTDLen+1);
#else
			m_sMOTD = (char *)malloc(m_ui16MOTDLen+1);
#endif
            if(m_sMOTD == NULL) {
				AppendDebugLogFormat("[MEM] Cannot allocate %hu bytes for sMOTD in SettingManager::LoadMOTD\n", m_ui16MOTDLen+1);
                exit(EXIT_FAILURE);
            }

            // read from file to sMOTD, if it failed then free sMOTD and create default one
            if(fread(m_sMOTD, 1, (size_t)m_ui16MOTDLen, fr) == (size_t)m_ui16MOTDLen) {
				m_sMOTD[m_ui16MOTDLen] = '\0';
            } else {
#ifdef _WIN32
                if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sMOTD) == 0) {
					AppendDebugLog("%s - [MEM] Cannot deallocate m_sMOTD in SettingManager::LoadMOTD\n");
                }
#else
				free(m_sMOTD);
#endif
				m_sMOTD = NULL;
				m_ui16MOTDLen = 0;

                // motd loading failed ? create default one...
                CreateDefaultMOTD();
            }
        }
        fclose(fr);
    } else {
        // no motd to load ? create default one...
        CreateDefaultMOTD();
    }

    CheckMOTD();
}
//---------------------------------------------------------------------------

void SettingManager::SaveMOTD() {
#ifdef _WIN32
    FILE *fw = fopen((ServerManager::m_sPath + "\\cfg\\Motd.txt").c_str(), "wb");
#else
	FILE * fw = fopen((ServerManager::m_sPath + "/cfg/Motd.txt").c_str(), "wb");
#endif
    if(fw != NULL) {
        if(m_ui16MOTDLen != 0) {
            fwrite(m_sMOTD, 1, (size_t)m_ui16MOTDLen, fw);
        }
        fclose(fw);
    }
}
//---------------------------------------------------------------------------

void SettingManager::CheckMOTD() {
    for(uint16_t ui16i = 0; ui16i < m_ui16MOTDLen; ui16i++) {
        if(m_sMOTD[ui16i] == '|') {
			m_sMOTD[ui16i] = '0';
        }
    }
}
//---------------------------------------------------------------------------

void SettingManager::CheckAndSet(char * sName, char * sValue) {
	// Booleans
    for(size_t szi = 0; szi < SETBOOL_IDS_END; szi++) {
        if(strcmp(SetBoolStr[szi], sName) == 0) {
            SetBool(szi, sValue[0] == '1' ? true : false);
            return;
        }
    }

	// Integers
    for(size_t szi = 0; szi < SETSHORT_IDS_END; szi++) {
        if(strcmp(SetShortStr[szi], sName) == 0) {
            int32_t iValue = atoi(sValue);

            // Check if is valid value
            if(sValue[0] == '\0' || iValue < 0 || iValue > 32767) {
                return;
            }

            SetShort(szi, (int16_t)iValue);
			return;
        }
    }

	// Strings
    for(size_t szi = 0; szi < SETTXT_IDS_END; szi++) {
        if(strcmp(SetTxtStr[szi], sName) == 0) {
            SetText(szi, sValue);
            return;
        }
    }
}
//---------------------------------------------------------------------------

void SettingManager::Load() {
	m_bUpdateLocked = true;

    // Load MOTD
    LoadMOTD();

#ifdef _WIN32
    if(FileExist((ServerManager::m_sPath + "\\cfg\\Settings.pxt").c_str()) == false) {
#else
    if(FileExist((ServerManager::m_sPath + "/cfg/Settings.pxt").c_str()) == false) {
#endif
        LoadXML();

		m_bUpdateLocked = false;

        return;
    }

#ifdef _WIN32
	FILE * fSettingsFile = fopen((ServerManager::m_sPath + "\\cfg\\Settings.pxt").c_str(), "rt");
#else
	FILE * fSettingsFile = fopen((ServerManager::m_sPath + "/cfg/Settings.pxt").c_str(), "rt");
#endif
    if(fSettingsFile == NULL) {
    	int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "Error loading file Settings.pxt %s (%d)", 
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

	char * sValue = NULL;
	size_t szLen = 0;

	while(fgets(ServerManager::m_pGlobalBuffer, (int)ServerManager::m_szGlobalBufferSize, fSettingsFile) != NULL) {
		if(ServerManager::m_pGlobalBuffer[0] == '#' || ServerManager::m_pGlobalBuffer[0] == '\n') {
			continue;
		}

		sValue = NULL;

		szLen = strlen(ServerManager::m_pGlobalBuffer)-1;

		ServerManager::m_pGlobalBuffer[szLen] = '\0';

		for(size_t szi = 0; szi < szLen; szi++) {
			if(isspace(ServerManager::m_pGlobalBuffer[szi]) != 0) {
				ServerManager::m_pGlobalBuffer[szi] = '\0';
				continue;
			}

			if(ServerManager::m_pGlobalBuffer[szi] == '=') {
				if(isspace(ServerManager::m_pGlobalBuffer[szi+1]) != 0) {
					sValue = ServerManager::m_pGlobalBuffer+szi+2;
				} else {
					sValue = ServerManager::m_pGlobalBuffer+szi+1;
				}

				break;
			}
		}

		if(sValue == NULL || ServerManager::m_pGlobalBuffer[0] == '\0') {
			continue;
		}

		CheckAndSet(ServerManager::m_pGlobalBuffer, sValue);
	}

    fclose(fSettingsFile);

	m_bUpdateLocked = false;
}
//---------------------------------------------------------------------------

void SettingManager::LoadXML() {
#ifdef _WIN32
    TiXmlDocument doc((ServerManager::m_sPath + "\\cfg\\Settings.xml").c_str());
#else
	TiXmlDocument doc((ServerManager::m_sPath + "/cfg/Settings.xml").c_str());
#endif
    if(doc.LoadFile() == false) {
        if(doc.ErrorId() != TiXmlBase::TIXML_ERROR_OPENING_FILE && doc.ErrorId() != TiXmlBase::TIXML_ERROR_DOCUMENT_EMPTY) {
            int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "Error loading file Settings.xml. %s (Col: %d, Row: %d)", doc.ErrorDesc(), doc.Column(), doc.Row());
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

        TiXmlElement *settings = cfg.FirstChild("PtokaX").Element();
        if(settings == NULL) {
            return;
        }

        // version first run
        const char * sVersion;
        if(settings->ToElement() == NULL || (sVersion = settings->ToElement()->Attribute("Version")) == NULL || 
            strcmp(sVersion, PtokaXVersionString) != 0) {
			m_bFirstRun = true;
        } else {
			m_bFirstRun = false;
        }

        // Read bools
        TiXmlNode *SettingNode = settings->FirstChild("Booleans");
        if(SettingNode != NULL) {
            TiXmlNode *SettingValue = NULL;
            while((SettingValue = SettingNode->IterateChildren(SettingValue)) != NULL) {
                const char * sName;

                if(SettingValue->ToElement() == NULL || (sName = SettingValue->ToElement()->Attribute("Name")) == NULL) {
                    continue;
                }

                bool bValue = atoi(SettingValue->ToElement()->GetText()) == 0 ? false : true;

                for(size_t szi = 0; szi < SETBOOL_IDS_END; szi++) {
                    if(strcmp(SetBoolStr[szi], sName) == 0) {
                        SetBool(szi, bValue);
                    }
                }
            }
        }

        // Read integers
        SettingNode = settings->FirstChild("Integers");
        if(SettingNode != NULL) {
            TiXmlNode *SettingValue = NULL;
            while((SettingValue = SettingNode->IterateChildren(SettingValue)) != NULL) {
                const char * sName;

                if(SettingValue->ToElement() == NULL || (sName = SettingValue->ToElement()->Attribute("Name")) == NULL) {
                    continue;
                }

                int32_t iValue = atoi(SettingValue->ToElement()->GetText());
                // Check if is valid value
                if(iValue < 0 || iValue > 32767) {
                    continue;
                }

                for(size_t szi = 0; szi < SETSHORT_IDS_END; szi++) {
                    if(strcmp(SetShortStr[szi], sName) == 0) {
                        SetShort(szi, (int16_t)iValue);
                    }
                }
            }
        }

        // Read strings
        SettingNode = settings->FirstChild("Strings");
        if(SettingNode != NULL) {
            TiXmlNode *SettingValue = NULL;
            while((SettingValue = SettingNode->IterateChildren(SettingValue)) != NULL) {
                const char * sName;

                if(SettingValue->ToElement() == NULL || (sName = SettingValue->ToElement()->Attribute("Name")) == NULL) {
                    continue;
                }

                const char * sText = SettingValue->ToElement()->GetText();

                if(sText == NULL) {
                    continue;
                }

                for(size_t szi = 0; szi < SETTXT_IDS_END; szi++) {
                    if(strcmp(SetTxtStr[szi], sName) == 0) {
                        SetText(szi, sText);
                    }
                }
            }
        }
    }
}
//---------------------------------------------------------------------------

void SettingManager::Save() {
    SaveMOTD();

#ifdef _WIN32
    FILE * fSettingsFile = fopen((ServerManager::m_sPath + "\\cfg\\Settings.pxt").c_str(), "wb");
#else
	FILE * fSettingsFile = fopen((ServerManager::m_sPath + "/cfg/Settings.pxt").c_str(), "wb");
#endif
    if(fSettingsFile == NULL) {
    	return;
    }

	static const char sPtokaXSettingsFile[] = "#\n# PtokaX settings file\n#\n";
    fwrite(sPtokaXSettingsFile, 1, sizeof(sPtokaXSettingsFile)-1, fSettingsFile);

	// Save booleans
	static const char sPtokaXSettingsFileBooleans[] = "\n#\n# Boolean settings\n#\n\n";
    fwrite(sPtokaXSettingsFileBooleans, 1, sizeof(sPtokaXSettingsFileBooleans)-1, fSettingsFile);

    for(size_t szi = 0; szi < SETBOOL_IDS_END; szi++) {
    	// Don't save empty hint
    	if(SetBoolCom[szi][0] != '\0') {
			fputs(SetBoolCom[szi], fSettingsFile);
		}

        // Don't save setting with empty id
        if(SetBoolStr[szi][0] == '\0') {
            continue;
        }

		// Save setting with default value as comment
        if(m_bBools[szi] == SetBoolDef[szi]) {
        	fprintf(fSettingsFile, "#%s\t=\t%c\n", SetBoolStr[szi], m_bBools[szi] == 0 ? '0' : '1');
        } else {
        	fprintf(fSettingsFile, "%s\t=\t%c\n", SetBoolStr[szi], m_bBools[szi] == 0 ? '0' : '1');
		}
    }

	// Save integers
	static const char sPtokaXSettingsFileIntegers[] = "\n#\n# Integer settings\n#\n\n";
    fwrite(sPtokaXSettingsFileIntegers, 1, sizeof(sPtokaXSettingsFileIntegers)-1, fSettingsFile);

    for(size_t szi = 0; szi < SETSHORT_IDS_END; szi++) {
    	// Don't save empty hint
    	if(SetShortCom[szi][0] != '\0') {
			fputs(SetShortCom[szi], fSettingsFile);
		}

        // Don't save setting with empty id
        if(SetShortStr[szi][0] == '\0') {
            continue;
        }

        // Save setting with default value as comment
        if(m_i16Shorts[szi] == SetShortDef[szi]) {
        	fprintf(fSettingsFile, "#%s\t=\t%hd\n", SetShortStr[szi], m_i16Shorts[szi]);
        } else {
        	fprintf(fSettingsFile, "%s\t=\t%hd\n", SetShortStr[szi], m_i16Shorts[szi]);
		}
    }

	// Save strings
	static const char sPtokaXSettingsFileStrings[] = "\n#\n# String settings\n#\n\n";
    fwrite(sPtokaXSettingsFileStrings, 1, sizeof(sPtokaXSettingsFileStrings)-1, fSettingsFile);

    for(size_t szi = 0; szi < SETTXT_IDS_END; szi++) {
    	// Don't save empty hint
    	if(SetTxtCom[szi][0] != '\0') {
			fputs(SetTxtCom[szi], fSettingsFile);
		}

        // Don't save setting with empty id
        if(SetTxtStr[szi][0] == '\0') {
            continue;
        }
    
        // Save setting with default value as comment
        if((m_sTexts[szi] == NULL && SetTxtDef[szi][0] == '\0') || (m_sTexts[szi] != NULL && strcmp(m_sTexts[szi], SetTxtDef[szi]) == 0)) {
			fprintf(fSettingsFile, "#%s\t=\t%s\n", SetTxtStr[szi], m_sTexts[szi] != NULL ? m_sTexts[szi] : "");
        } else {
        	fprintf(fSettingsFile, "%s\t=\t%s\n", SetTxtStr[szi], m_sTexts[szi] != NULL ? m_sTexts[szi] : "");
		}
    }

    fclose(fSettingsFile);
}
//---------------------------------------------------------------------------

bool SettingManager::GetBool(const size_t szBoolId) {
#ifdef _WIN32
    EnterCriticalSection(&m_csSetting);
#else
	pthread_mutex_lock(&m_mtxSetting);
#endif

    bool bValue = m_bBools[szBoolId];

#ifdef _WIN32
    LeaveCriticalSection(&m_csSetting);
#else
	pthread_mutex_unlock(&m_mtxSetting);
#endif
    
    return bValue;
}
//---------------------------------------------------------------------------
uint16_t SettingManager::GetFirstPort() {
#ifdef _WIN32
    EnterCriticalSection(&m_csSetting);
#else
	pthread_mutex_lock(&m_mtxSetting);
#endif

    uint16_t iValue = m_ui16PortNumbers[0];

#ifdef _WIN32
    LeaveCriticalSection(&m_csSetting);
#else
	pthread_mutex_unlock(&m_mtxSetting);
#endif
    
    return iValue;
}
//---------------------------------------------------------------------------

int16_t SettingManager::GetShort(const size_t szShortId) {
#ifdef _WIN32
    EnterCriticalSection(&m_csSetting);
#else
	pthread_mutex_lock(&m_mtxSetting);
#endif

    int16_t iValue = m_i16Shorts[szShortId];

#ifdef _WIN32
    LeaveCriticalSection(&m_csSetting);
#else
	pthread_mutex_unlock(&m_mtxSetting);
#endif
    
    return iValue;
}
//---------------------------------------------------------------------------

void SettingManager::GetText(const size_t szTxtId, char * sMsg) {
#ifdef _WIN32
    EnterCriticalSection(&m_csSetting);
#else
	pthread_mutex_lock(&m_mtxSetting);
#endif

    if(m_sTexts[szTxtId] != NULL) {
        strcat(sMsg, m_sTexts[szTxtId]);
    } else {
    	sMsg[0] = '\0';
	}

#ifdef _WIN32
    LeaveCriticalSection(&m_csSetting);
#else
	pthread_mutex_unlock(&m_mtxSetting);
#endif
}
//---------------------------------------------------------------------------

void SettingManager::SetBool(const size_t szBoolId, const bool bValue) {
    if(m_bBools[szBoolId] == bValue) {
        return;
    }
    
    if(szBoolId == SETBOOL_ANTI_MOGLO) {
#ifdef _WIN32
        EnterCriticalSection(&m_csSetting);
#else
		pthread_mutex_lock(&m_mtxSetting);
#endif

		m_bBools[szBoolId] = bValue;

#ifdef _WIN32
        LeaveCriticalSection(&m_csSetting);
#else
		pthread_mutex_unlock(&m_mtxSetting);
#endif

        return;
    }

	m_bBools[szBoolId] = bValue;

    switch(szBoolId) {
        case SETBOOL_REG_BOT:
            UpdateBotsSameNick();
            if(bValue == false) {
                DisableBot();
            }
            UpdateBot();
            break;
        case SETBOOL_REG_OP_CHAT:
            UpdateBotsSameNick();
            if(bValue == false) {
                DisableOpChat();
            }
            UpdateOpChat();
            break;
        case SETBOOL_USE_BOT_NICK_AS_HUB_SEC:
            UpdateHubSec();
            UpdateMOTD();
            UpdateHubNameWelcome();
            UpdateRegOnlyMessage();
            UpdateShareLimitMessage();
            UpdateSlotsLimitMessage();
            UpdateHubSlotRatioMessage();
            UpdateMaxHubsLimitMessage();
            UpdateNoTagMessage();
            UpdateNickLimitMessage();
            break;
        case SETBOOL_DISABLE_MOTD:
        case SETBOOL_MOTD_AS_PM:
            UpdateMOTD();
            break;
        case SETBOOL_REG_ONLY_REDIR:
            UpdateRegOnlyMessage();
            break;
        case SETBOOL_SHARE_LIMIT_REDIR:
            UpdateShareLimitMessage();
            break;
        case SETBOOL_SLOTS_LIMIT_REDIR:
            UpdateSlotsLimitMessage();
            break;
        case SETBOOL_HUB_SLOT_RATIO_REDIR:
            UpdateHubSlotRatioMessage();
            break;
        case SETBOOL_MAX_HUBS_LIMIT_REDIR:
            UpdateMaxHubsLimitMessage();
            break;
        case SETBOOL_NICK_LIMIT_REDIR:
            UpdateNickLimitMessage();
            break;
        case SETBOOL_ENABLE_TEXT_FILES:
            if(bValue == true && m_bUpdateLocked == false) {
                TextFilesManager::m_Ptr->RefreshTextFiles();
            }
            break;
#ifdef _BUILD_GUI
		case SETBOOL_ENABLE_TRAY_ICON:
	        if(m_bUpdateLocked == false) {
                MainWindow::m_Ptr->UpdateSysTray();
			}

            break;
#endif
        case SETBOOL_AUTO_REG:
            if(m_bUpdateLocked == false) {
				ServerManager::UpdateAutoRegState();
            }
            break;
        case SETBOOL_ENABLE_SCRIPTING:
            UpdateScripting();
            break;
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
        case SETBOOL_ENABLE_DATABASE:
        	if(m_bUpdateLocked == false) {
        		UpdateDatabase();
        	}

        	break;
#endif
        case SETBOOL_RESOLVE_TO_IP:
        	if(m_bUpdateLocked == false) {
        		ServerManager::ResolveHubAddress(true);
        	}
        	break;
        default:
            break;
    }
}
//---------------------------------------------------------------------------

void SettingManager::SetMOTD(char * sTxt, const size_t szLen) {
    if(m_ui16MOTDLen == (uint16_t)szLen &&
        (m_sMOTD != NULL && strcmp(m_sMOTD, sTxt) == 0)) {
        return;
    }

    if(szLen == 0) {
        if(m_sMOTD != NULL) {
#ifdef _WIN32
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sMOTD) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate m_sMOTD in SettingManager::SetMOTD\n");
            }
#else
            free(m_sMOTD);
#endif
			m_sMOTD = NULL;
			m_ui16MOTDLen = 0;
        }
    } else {
        uint16_t ui16OldMOTDLen = m_ui16MOTDLen;
        char * sOldMOTD = m_sMOTD;

		m_ui16MOTDLen = (uint16_t)(szLen < 65024 ? szLen : 65024);

        // (re)allocate memory for sMOTD
#ifdef _WIN32
        if(m_sMOTD == NULL) {
			m_sMOTD = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, m_ui16MOTDLen+1);
        } else {
			m_sMOTD = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldMOTD, m_ui16MOTDLen+1);
        }
#else
		m_sMOTD = (char *)realloc(sOldMOTD, m_ui16MOTDLen+1);
#endif
        if(m_sMOTD == NULL) {
			m_sMOTD = sOldMOTD;
			m_ui16MOTDLen = ui16OldMOTDLen;

            AppendDebugLogFormat("[MEM] Cannot (re)allocate %hu bytes in SettingManager::SetMOTD for sMOTD\n", m_ui16MOTDLen+1);

            return;
        }

        memcpy(m_sMOTD, sTxt, (size_t)m_ui16MOTDLen);
		m_sMOTD[m_ui16MOTDLen] = '\0';

        CheckMOTD();
    }
}
//---------------------------------------------------------------------------

void SettingManager::SetShort(const size_t szShortId, const int16_t i16Value) {
    if(i16Value < 0 || m_i16Shorts[szShortId] == i16Value) {
        return;
    }

    switch(szShortId) {
        case SETSHORT_MIN_SHARE_LIMIT:
        case SETSHORT_MAX_SHARE_LIMIT:
            if(i16Value > 9999) {
                return;
            }
            break;
        case SETSHORT_MIN_SHARE_UNITS:
        case SETSHORT_MAX_SHARE_UNITS:
            if(i16Value > 4) {
                return;
            }
            break;
        case SETSHORT_NO_TAG_OPTION:
        case SETSHORT_FULL_MYINFO_OPTION:
        case SETSHORT_GLOBAL_MAIN_CHAT_ACTION:
        case SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE:
            if(i16Value > 2) {
                return;
            }
            break;
        case SETSHORT_MAX_USERS:
        case SETSHORT_DEFAULT_TEMP_BAN_TIME:
        case SETSHORT_DEFLOOD_TEMP_BAN_TIME:
        case SETSHORT_SR_MESSAGES:
        case SETSHORT_SR_MESSAGES2:
            if(i16Value == 0) {
                return;
            }
            break;
        case SETSHORT_MIN_SLOTS_LIMIT:
        case SETSHORT_MAX_SLOTS_LIMIT:
        case SETSHORT_HUB_SLOT_RATIO_HUBS:
        case SETSHORT_HUB_SLOT_RATIO_SLOTS:
        case SETSHORT_MAX_HUBS_LIMIT:
        case SETSHORT_MAX_CHAT_LINES:
        case SETSHORT_MAX_PM_LINES:
        case SETSHORT_MYINFO_DELAY:
        case SETSHORT_MIN_SEARCH_LEN:
        case SETSHORT_MAX_SEARCH_LEN:
        case SETSHORT_MAX_PM_COUNT_TO_USER:
            if(i16Value > 999) {
                return;
            }
            break;
        case SETSHORT_MAIN_CHAT_MESSAGES:
        case SETSHORT_MAIN_CHAT_TIME:
        case SETSHORT_SAME_MAIN_CHAT_TIME:
        case SETSHORT_PM_MESSAGES:
        case SETSHORT_PM_TIME:
        case SETSHORT_SAME_PM_TIME:
        case SETSHORT_SEARCH_MESSAGES:
        case SETSHORT_SEARCH_TIME:
        case SETSHORT_SAME_SEARCH_TIME:
        case SETSHORT_MYINFO_MESSAGES:
        case SETSHORT_MYINFO_TIME:
        case SETSHORT_GETNICKLIST_MESSAGES:
        case SETSHORT_GETNICKLIST_TIME:
        case SETSHORT_DEFLOOD_WARNING_COUNT:
        case SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES:
        case SETSHORT_GLOBAL_MAIN_CHAT_TIME:
        case SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT:
        case SETSHORT_BRUTE_FORCE_PASS_PROTECT_TEMP_BAN_TIME:
        case SETSHORT_MAIN_CHAT_MESSAGES2:
        case SETSHORT_MAIN_CHAT_TIME2:
        case SETSHORT_PM_MESSAGES2:
        case SETSHORT_PM_TIME2:
        case SETSHORT_SEARCH_MESSAGES2:
        case SETSHORT_SEARCH_TIME2:
        case SETSHORT_MYINFO_MESSAGES2:
        case SETSHORT_MYINFO_TIME2:
        case SETSHORT_CHAT_INTERVAL_MESSAGES:
        case SETSHORT_CHAT_INTERVAL_TIME:
        case SETSHORT_PM_INTERVAL_MESSAGES:
        case SETSHORT_PM_INTERVAL_TIME:
        case SETSHORT_SEARCH_INTERVAL_MESSAGES:
        case SETSHORT_SEARCH_INTERVAL_TIME:
            if(i16Value == 0 || i16Value > 999) {
                return;
            }
            break;
        case SETSHORT_CTM_MESSAGES:
        case SETSHORT_CTM_TIME:
        case SETSHORT_CTM_MESSAGES2:
        case SETSHORT_CTM_TIME2:
        case SETSHORT_RCTM_MESSAGES:
        case SETSHORT_RCTM_TIME:
        case SETSHORT_RCTM_MESSAGES2:
        case SETSHORT_RCTM_TIME2:
        case SETSHORT_SR_TIME:
        case SETSHORT_SR_TIME2:
        case SETSHORT_MAX_DOWN_KB:
        case SETSHORT_MAX_DOWN_TIME:
        case SETSHORT_MAX_DOWN_KB2:
        case SETSHORT_MAX_DOWN_TIME2:
            if(i16Value == 0 || i16Value > 9999) {
                return;
            }
            break;
        case SETSHORT_NEW_CONNECTIONS_COUNT:
        case SETSHORT_NEW_CONNECTIONS_TIME:
            if(i16Value == 0 || i16Value > 999) {
                return;
            }

#ifdef _WIN32
            EnterCriticalSection(&m_csSetting);
#else
			pthread_mutex_lock(&m_mtxSetting);
#endif

            m_i16Shorts[szShortId] = i16Value;

#ifdef _WIN32
            LeaveCriticalSection(&m_csSetting);
#else
			pthread_mutex_unlock(&m_mtxSetting);
#endif
            return;
        case SETSHORT_SAME_MAIN_CHAT_MESSAGES:
        case SETSHORT_SAME_MULTI_MAIN_CHAT_MESSAGES:
        case SETSHORT_SAME_MULTI_MAIN_CHAT_LINES:
        case SETSHORT_SAME_PM_MESSAGES:
        case SETSHORT_SAME_MULTI_PM_MESSAGES:
        case SETSHORT_SAME_MULTI_PM_LINES:
        case SETSHORT_SAME_SEARCH_MESSAGES:
            if(i16Value < 2 || i16Value > 999) {
                return;
            }
            break;
        case SETSHORT_MAIN_CHAT_ACTION:
        case SETSHORT_MAIN_CHAT_ACTION2:
        case SETSHORT_SAME_MAIN_CHAT_ACTION:
        case SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION:
        case SETSHORT_PM_ACTION:
        case SETSHORT_PM_ACTION2:
        case SETSHORT_SAME_PM_ACTION:
        case SETSHORT_SAME_MULTI_PM_ACTION:
        case SETSHORT_SEARCH_ACTION:
        case SETSHORT_SEARCH_ACTION2:
        case SETSHORT_SAME_SEARCH_ACTION:
        case SETSHORT_MYINFO_ACTION:
        case SETSHORT_MYINFO_ACTION2:
        case SETSHORT_GETNICKLIST_ACTION:
        case SETSHORT_CTM_ACTION:
        case SETSHORT_CTM_ACTION2:
        case SETSHORT_RCTM_ACTION:
        case SETSHORT_RCTM_ACTION2:
        case SETSHORT_SR_ACTION:
        case SETSHORT_SR_ACTION2:
        case SETSHORT_MAX_DOWN_ACTION:
        case SETSHORT_MAX_DOWN_ACTION2:
            if(i16Value > 6) {
                return;
            }
            break;
        case SETSHORT_DEFLOOD_WARNING_ACTION:
            if(i16Value > 3) {
                return;
            }
            break;
        case SETSHORT_MIN_NICK_LEN:
        case SETSHORT_MAX_NICK_LEN:
            if(i16Value > 64) {
                return;
            }
            break;
        case SETSHORT_MAX_SIMULTANEOUS_LOGINS:
            if(i16Value == 0 || i16Value > 1000) {
                return;
            }
            break;
        case SETSHORT_MAX_MYINFO_LEN:
            if(i16Value < 64 || i16Value > 512) {
                return;
            }
            break;
        case SETSHORT_MAX_CTM_LEN:
        case SETSHORT_MAX_RCTM_LEN:
            if(i16Value == 0 || i16Value > 512) {
                return;
            }
            break;
        case SETSHORT_MAX_SR_LEN:
            if(i16Value == 0 || i16Value > 8192) {
                return;
            }
            break;
        case SETSHORT_MAX_CONN_SAME_IP:
        case SETSHORT_MIN_RECONN_TIME:
            if(i16Value == 0 || i16Value > 256) {
                return;
            }
            break;
        default:
            break;
    }

	m_i16Shorts[szShortId] = i16Value;

    switch(szShortId) {
        case SETSHORT_MIN_SHARE_LIMIT:
        case SETSHORT_MIN_SHARE_UNITS:
            UpdateMinShare();
            UpdateShareLimitMessage();
            break;
        case SETSHORT_MAX_SHARE_LIMIT:
        case SETSHORT_MAX_SHARE_UNITS:
            UpdateMaxShare();
            UpdateShareLimitMessage();
            break;
        case SETSHORT_MIN_SLOTS_LIMIT:
        case SETSHORT_MAX_SLOTS_LIMIT:
            UpdateSlotsLimitMessage();
            break;
        case SETSHORT_HUB_SLOT_RATIO_HUBS:
        case SETSHORT_HUB_SLOT_RATIO_SLOTS:
            UpdateHubSlotRatioMessage();
            break;
        case SETSHORT_MAX_HUBS_LIMIT:
            UpdateMaxHubsLimitMessage();
            break;
        case SETSHORT_NO_TAG_OPTION:
            UpdateNoTagMessage();
            break;
        case SETSHORT_MIN_NICK_LEN:
        case SETSHORT_MAX_NICK_LEN:
            UpdateNickLimitMessage();
            break;
        default:
            break;
    }
}
//---------------------------------------------------------------------------

void SettingManager::SetText(const size_t szTxtId, char * sTxt) {
    SetText(szTxtId, sTxt, strlen(sTxt));
}
//---------------------------------------------------------------------------

void SettingManager::SetText(const size_t szTxtId, const char * sTxt) {
    SetText(szTxtId, (char *)sTxt, strlen(sTxt));
}
//---------------------------------------------------------------------------

void SettingManager::SetText(const size_t szTxtId, const char * sTxt, const size_t szLen) {
    if(m_ui16TextsLens[szTxtId] == (uint16_t)szLen && (m_sTexts[szTxtId] != NULL && strcmp(m_sTexts[szTxtId], sTxt) == 0)) {
        return;
    }

    switch(szTxtId) {
        case SETTXT_HUB_NAME:
        case SETTXT_HUB_ADDRESS:
            if(szLen == 0 || szLen > 256 || strchr(sTxt, '$') != NULL || strchr(sTxt, '|') != NULL) {
                return;
            }
            break;
        case SETTXT_REG_ONLY_MSG:
        case SETTXT_SHARE_LIMIT_MSG:
        case SETTXT_SLOTS_LIMIT_MSG:
        case SETTXT_HUB_SLOT_RATIO_MSG:
        case SETTXT_MAX_HUBS_LIMIT_MSG:
        case SETTXT_NO_TAG_MSG:
        case SETTXT_NICK_LIMIT_MSG:
            if(szLen == 0 || szLen > 256 || strchr(sTxt, '|') != NULL) {
                return;
            }
            break;
        case SETTXT_BOT_NICK:
            if(szLen == 0 || szLen > 64 || strpbrk(sTxt, " $|") != NULL) {
                return;
            }
            if(ServerManager::m_pServersS != NULL && m_bBotsSameNick == false) {
                ReservedNicksManager::m_Ptr->DelReservedNick(m_sTexts[SETTXT_BOT_NICK]);
            }
            if(m_bBools[SETBOOL_REG_BOT] == true) {
                DisableBot();
            }
            break;
        case SETTXT_OP_CHAT_NICK:
            if(szLen == 0 || szLen > 64 || strpbrk(sTxt, " $|") != NULL) {
                return;
            }
            if(ServerManager::m_pServersS != NULL && m_bBotsSameNick == false) {
                ReservedNicksManager::m_Ptr->DelReservedNick(m_sTexts[SETTXT_OP_CHAT_NICK]);
            }
            if(m_bBools[SETBOOL_REG_OP_CHAT] == true) {
                DisableOpChat();
            }
            break;
        case SETTXT_ADMIN_NICK:
            if(szLen == 0 || szLen > 64 || strpbrk(sTxt, " $|") != NULL) {
                return;
            }
        case SETTXT_TCP_PORTS:
            if(szLen == 0 || szLen > 64) {
                return;
            }
            break;
        case SETTXT_UDP_PORT:
            if(szLen == 0 || szLen > 5) {
                return;
            }
            UpdateUDPPort();
            break;
        case SETTXT_CHAT_COMMANDS_PREFIXES:
            if(szLen == 0 || szLen > 5 || strchr(sTxt, '|') != NULL || strchr(sTxt, ' ') != NULL) {
                return;
            }
            break;
        case SETTXT_HUB_DESCRIPTION:
        case SETTXT_HUB_TOPIC:
            if(szLen > 256 || (szLen != 0 && (strchr(sTxt, '$') != NULL || strchr(sTxt, '|') != NULL))) {
                return;
            }
            break;
        case SETTXT_REDIRECT_ADDRESS:
        case SETTXT_REG_ONLY_REDIR_ADDRESS:
        case SETTXT_SHARE_LIMIT_REDIR_ADDRESS:
        case SETTXT_SLOTS_LIMIT_REDIR_ADDRESS:
        case SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS:
        case SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS:
        case SETTXT_NO_TAG_REDIR_ADDRESS:
        case SETTXT_TEMP_BAN_REDIR_ADDRESS:
        case SETTXT_PERM_BAN_REDIR_ADDRESS:
        case SETTXT_NICK_LIMIT_REDIR_ADDRESS:
        case SETTXT_MSG_TO_ADD_TO_BAN_MSG:
            if(szLen > 256 || (szLen != 0 && strchr(sTxt, '|') != NULL)) {
                return;
            }
            break;
        case SETTXT_REGISTER_SERVERS:
            if(szLen > 1024) {
                return;
            }
            break;
        case SETTXT_BOT_DESCRIPTION:
        case SETTXT_BOT_EMAIL:
            if(szLen > 64 || (szLen != 0 && (strchr(sTxt, '$') != NULL || strchr(sTxt, '|') != NULL))) {
                return;
            }
            if(m_bBools[SETBOOL_REG_BOT] == true) {
                DisableBot(false);
            }
            break;
        case SETTXT_OP_CHAT_DESCRIPTION:
        case SETTXT_OP_CHAT_EMAIL:
            if(szLen > 64 || (szLen != 0 && (strchr(sTxt, '$') != NULL || strchr(sTxt, '|') != NULL))) {
                return;
            }
            if(m_bBools[SETBOOL_REG_OP_CHAT] == true) {
                DisableOpChat(false);
            }
            break;
        case SETTXT_HUB_OWNER_EMAIL:
            if(szLen > 64 || (szLen != 0 && (strchr(sTxt, '$') != NULL || strchr(sTxt, '|') != NULL))) {
                return;
            }
            break;
        case SETTXT_LANGUAGE:
#ifdef _WIN32
            if(szLen != 0 && FileExist((ServerManager::m_sPath+"\\language\\"+string(sTxt, szLen)+".xml").c_str()) == false) {
#else
			if(szLen != 0 && FileExist((ServerManager::m_sPath + "/language/" + string(sTxt, szLen) + ".xml").c_str()) == false) {
#endif
                return;
            }
            break;
        case SETTXT_IPV4_ADDRESS:
            if(szLen > 15) {
                return;
            }
            break;
        case SETTXT_IPV6_ADDRESS:
            if(szLen > 39) {
                return;
            }
            break;
        default:
            if(szLen > 4096) {
                return;
            }
            break;
    }

    if(szTxtId == SETTXT_HUB_NAME || szTxtId == SETTXT_HUB_ADDRESS || szTxtId == SETTXT_HUB_DESCRIPTION
#ifdef _WITH_POSTGRES
		|| szTxtId == SETTXT_POSTGRES_HOST || szTxtId == SETTXT_POSTGRES_PORT || szTxtId == SETTXT_POSTGRES_DBNAME || szTxtId == SETTXT_POSTGRES_USER || szTxtId == SETTXT_POSTGRES_PASS
#elif _WITH_POSTGRES
		|| szTxtId == SETTXT_MYSQL_HOST || szTxtId == SETTXT_MYSQL_USER || szTxtId == SETTXT_MYSQL_PASS || szTxtId == SETTXT_MYSQL_DBNAME || szTxtId == SETTXT_MYSQL_PORT
#endif
		) {
#ifdef _WIN32
        EnterCriticalSection(&m_csSetting);
#else
		pthread_mutex_lock(&m_mtxSetting);
#endif
    }

    if(szLen == 0) {
        if(m_sTexts[szTxtId] != NULL) {
#ifdef _WIN32
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sTexts[szTxtId]) == 0) {
                AppendDebugLogFormat("[MEM] Cannot deallocate m_sTexts[%zu] in SettingManager::SetText\n", szTxtId);
            }
#else
            free(m_sTexts[szTxtId]);
#endif
			m_sTexts[szTxtId] = NULL;
			m_ui16TextsLens[szTxtId] = 0;
        }
    } else {
        char * sOldText = m_sTexts[szTxtId];
#ifdef _WIN32
        if(m_sTexts[szTxtId] == NULL) {
			m_sTexts[szTxtId] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
        } else {
			m_sTexts[szTxtId] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldText, szLen+1);
        }
#else
		m_sTexts[szTxtId] = (char *)realloc(sOldText, szLen+1);
#endif
        if(m_sTexts[szTxtId] == NULL) {
			m_sTexts[szTxtId] = sOldText;

			AppendDebugLogFormat("[MEM] Cannot (re)allocate %zu bytes in SettingManager::SetText\n", szLen+1);

            if(szTxtId == SETTXT_HUB_NAME || szTxtId == SETTXT_HUB_ADDRESS || szTxtId == SETTXT_HUB_DESCRIPTION
#ifdef _WITH_POSTGRES
				|| szTxtId == SETTXT_POSTGRES_HOST || szTxtId == SETTXT_POSTGRES_PORT || szTxtId == SETTXT_POSTGRES_DBNAME || szTxtId == SETTXT_POSTGRES_USER || szTxtId == SETTXT_POSTGRES_PASS
#elif _WITH_POSTGRES
				|| szTxtId == SETTXT_MYSQL_HOST || szTxtId == SETTXT_MYSQL_USER || szTxtId == SETTXT_MYSQL_PASS || szTxtId == SETTXT_MYSQL_DBNAME || szTxtId == SETTXT_MYSQL_PORT
#endif
			) {
#ifdef _WIN32
                LeaveCriticalSection(&m_csSetting);
#else
                pthread_mutex_unlock(&m_mtxSetting);
#endif
            }

            return;
        }
    
        memcpy(m_sTexts[szTxtId], sTxt, szLen);
		m_sTexts[szTxtId][szLen] = '\0';
		m_ui16TextsLens[szTxtId] = (uint16_t)szLen;
    }

    if(szTxtId == SETTXT_HUB_NAME || szTxtId == SETTXT_HUB_ADDRESS || szTxtId == SETTXT_HUB_DESCRIPTION
#ifdef _WITH_POSTGRES
		|| szTxtId == SETTXT_POSTGRES_HOST || szTxtId == SETTXT_POSTGRES_PORT || szTxtId == SETTXT_POSTGRES_DBNAME || szTxtId == SETTXT_POSTGRES_USER || szTxtId == SETTXT_POSTGRES_PASS
#elif _WITH_POSTGRES
		|| szTxtId == SETTXT_MYSQL_HOST || szTxtId == SETTXT_MYSQL_USER || szTxtId == SETTXT_MYSQL_PASS || szTxtId == SETTXT_MYSQL_DBNAME || szTxtId == SETTXT_MYSQL_PORT
#endif
	) {
#ifdef _WIN32
        LeaveCriticalSection(&m_csSetting);
#else
		pthread_mutex_unlock(&m_mtxSetting);
#endif
    }

    switch(szTxtId) {
        case SETTXT_BOT_NICK:
            UpdateHubSec();
            UpdateMOTD();
            UpdateHubNameWelcome();
            UpdateRegOnlyMessage();
            UpdateShareLimitMessage();
            UpdateSlotsLimitMessage();
            UpdateHubSlotRatioMessage();
            UpdateMaxHubsLimitMessage();
            UpdateNoTagMessage();
            UpdateNickLimitMessage();
            UpdateBotsSameNick();

            if(ServerManager::m_pServersS != NULL && m_bBotsSameNick == false) {
                ReservedNicksManager::m_Ptr->AddReservedNick(m_sTexts[SETTXT_BOT_NICK]);
            }

            UpdateBot();

            break;
        case SETTXT_BOT_DESCRIPTION:
        case SETTXT_BOT_EMAIL:
            UpdateBot(false);
            break;
        case SETTXT_OP_CHAT_NICK:
            UpdateBotsSameNick();
            if(ServerManager::m_pServersS != NULL && m_bBotsSameNick == false) {
                ReservedNicksManager::m_Ptr->AddReservedNick(m_sTexts[SETTXT_OP_CHAT_NICK]);
            }
            UpdateOpChat();
            break;
        case SETTXT_HUB_TOPIC:
		case SETTXT_HUB_NAME:
#ifdef _BUILD_GUI
			if(m_bUpdateLocked == false) {
                MainWindow::m_Ptr->UpdateTitleBar();
			}
#endif
            UpdateHubNameWelcome();
            UpdateHubName();

			if(UdpDebug::m_Ptr != NULL) {
				UdpDebug::m_Ptr->UpdateHubName();
			}
            break;
        case SETTXT_LANGUAGE:
            UpdateLanguage();
            UpdateHubNameWelcome();
            break;
        case SETTXT_REDIRECT_ADDRESS:
            UpdateRedirectAddress();
            if(m_bBools[SETBOOL_REG_ONLY_REDIR] == true) {
                UpdateRegOnlyMessage();
            }
            if(m_bBools[SETBOOL_SHARE_LIMIT_REDIR] == true) {
                UpdateShareLimitMessage();
            }
            if(m_bBools[SETBOOL_SLOTS_LIMIT_REDIR] == true) {
                UpdateSlotsLimitMessage();
            }
            if(m_bBools[SETBOOL_HUB_SLOT_RATIO_REDIR] == true) {
                UpdateHubSlotRatioMessage();
            }
            if(m_bBools[SETBOOL_MAX_HUBS_LIMIT_REDIR] == true) {
                UpdateMaxHubsLimitMessage();
            }
            if(m_i16Shorts[SETSHORT_NO_TAG_OPTION] == 2) {
                UpdateNoTagMessage();
            }
            if(m_sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS] != NULL) {
                UpdateTempBanRedirAddress();
            }
            if(m_sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS] != NULL) {
                UpdatePermBanRedirAddress();
            }
            if(m_bBools[SETBOOL_NICK_LIMIT_REDIR] == true) {
                UpdateNickLimitMessage();
            }
            break;
        case SETTXT_REG_ONLY_MSG:
        case SETTXT_REG_ONLY_REDIR_ADDRESS:
            UpdateRegOnlyMessage();
            break;
        case SETTXT_SHARE_LIMIT_MSG:
        case SETTXT_SHARE_LIMIT_REDIR_ADDRESS:
            UpdateShareLimitMessage();
            break;
        case SETTXT_SLOTS_LIMIT_MSG:
        case SETTXT_SLOTS_LIMIT_REDIR_ADDRESS:
            UpdateSlotsLimitMessage();
            break;
        case SETTXT_HUB_SLOT_RATIO_MSG:
        case SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS:
            UpdateHubSlotRatioMessage();
            break;
        case SETTXT_MAX_HUBS_LIMIT_MSG:
        case SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS:
            UpdateMaxHubsLimitMessage();
            break;
        case SETTXT_NO_TAG_MSG:
        case SETTXT_NO_TAG_REDIR_ADDRESS:
            UpdateNoTagMessage();
            break;
        case SETTXT_TEMP_BAN_REDIR_ADDRESS:
            UpdateTempBanRedirAddress();
            break;
        case SETTXT_PERM_BAN_REDIR_ADDRESS:
            UpdatePermBanRedirAddress();
            break;
        case SETTXT_NICK_LIMIT_MSG:
        case SETTXT_NICK_LIMIT_REDIR_ADDRESS:
            UpdateNickLimitMessage();
            break;
        case SETTXT_TCP_PORTS:
            UpdateTCPPorts();
            break;
        case SETTXT_HUB_ADDRESS:
		case SETTXT_IPV4_ADDRESS:
		case SETTXT_IPV6_ADDRESS:
			if(m_bUpdateLocked == false) {
        		ServerManager::ResolveHubAddress(true);
        	}
        	break;
        default:
            break;
    }
}
//---------------------------------------------------------------------------

void SettingManager::SetText(const size_t szTxtId, const string & sTxt) {
    SetText(szTxtId, sTxt.c_str(), sTxt.size());
}
//---------------------------------------------------------------------------

void SettingManager::UpdateAll() {
	m_ui8FullMyINFOOption = (uint8_t)m_i16Shorts[SETSHORT_FULL_MYINFO_OPTION];

    UpdateHubSec();
    UpdateMOTD();
    UpdateHubNameWelcome();
    UpdateHubName();
    UpdateRedirectAddress();
    UpdateRegOnlyMessage();
    UpdateMinShare();
    UpdateMaxShare();
    UpdateShareLimitMessage();
    UpdateSlotsLimitMessage();
    UpdateHubSlotRatioMessage();
    UpdateMaxHubsLimitMessage();
    UpdateNoTagMessage();
    UpdateTempBanRedirAddress();
    UpdatePermBanRedirAddress();
    UpdateNickLimitMessage();
    UpdateTCPPorts();
    UpdateBotsSameNick();
}
//---------------------------------------------------------------------------

void SettingManager::UpdateHubSec() {
    if(m_bUpdateLocked == true) {
        return;
    }

    if(m_bBools[SETBOOL_USE_BOT_NICK_AS_HUB_SEC] == true) {
        char * sOldHubSec = m_sPreTexts[SETPRETXT_HUB_SEC];

#ifdef _WIN32
        if(m_sPreTexts[SETPRETXT_HUB_SEC] == NULL || m_sPreTexts[SETPRETXT_HUB_SEC] == sHubSec) {
			m_sPreTexts[SETPRETXT_HUB_SEC] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, m_ui16TextsLens[SETTXT_BOT_NICK]+1);
        } else {
			m_sPreTexts[SETPRETXT_HUB_SEC] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldHubSec, m_ui16TextsLens[SETTXT_BOT_NICK]+1);
        }
#else
		m_sPreTexts[SETPRETXT_HUB_SEC] = (char *)realloc(sOldHubSec == sHubSec ? NULL : sOldHubSec, m_ui16TextsLens[SETTXT_BOT_NICK]+1);
#endif
        if(m_sPreTexts[SETPRETXT_HUB_SEC] == NULL) {
			m_sPreTexts[SETPRETXT_HUB_SEC] = sOldHubSec;

			AppendDebugLogFormat("[MEM] Cannot (re)allocate %hu bytes in SettingManager::UpdateHubSec\n", m_ui16TextsLens[SETTXT_BOT_NICK]+1);

            return;
        }

        memcpy(m_sPreTexts[SETPRETXT_HUB_SEC], m_sTexts[SETTXT_BOT_NICK], m_ui16TextsLens[SETTXT_BOT_NICK]);
		m_ui16PreTextsLens[SETPRETXT_HUB_SEC] = m_ui16TextsLens[SETTXT_BOT_NICK];
		m_sPreTexts[SETPRETXT_HUB_SEC][m_ui16PreTextsLens[SETPRETXT_HUB_SEC]] = '\0';
    } else {
        if(m_sPreTexts[SETPRETXT_HUB_SEC] != sHubSec) {
#ifdef _WIN32
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sPreTexts[SETPRETXT_HUB_SEC]) == 0) {
				AppendDebugLog("%s - [MEM] Cannot deallocate memory in SettingManager::UpdateHubSec\n");
            }
#else
			free(m_sPreTexts[SETPRETXT_HUB_SEC]);
#endif
        }

		m_sPreTexts[SETPRETXT_HUB_SEC] = (char *)sHubSec;
		m_ui16PreTextsLens[SETPRETXT_HUB_SEC] = 12;
    }
}
//---------------------------------------------------------------------------

void SettingManager::UpdateMOTD() {
    if(m_bUpdateLocked == true) {
        return;
    }

    if(m_bBools[SETBOOL_DISABLE_MOTD] == true || m_sMOTD == NULL) {
        if(m_sPreTexts[SETPRETXT_MOTD] != NULL) {
#ifdef _WIN32
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sPreTexts[SETPRETXT_MOTD]) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate memory in SettingManager::UpdateMOTD\n");
            }
#else
            free(m_sPreTexts[SETPRETXT_MOTD]);
#endif
			m_sPreTexts[SETPRETXT_MOTD] = NULL;
			m_ui16PreTextsLens[SETPRETXT_MOTD] = 0;
        }

        return;
    }

    size_t szNeededMem = (m_bBools[SETBOOL_MOTD_AS_PM] == true ? ((2*(m_ui16PreTextsLens[SETPRETXT_HUB_SEC]))+m_ui16MOTDLen+21) : (m_ui16PreTextsLens[SETPRETXT_HUB_SEC]+m_ui16MOTDLen+5));

    char * sOldMotd = m_sPreTexts[SETPRETXT_MOTD];

#ifdef _WIN32
    if(m_sPreTexts[SETPRETXT_MOTD] == NULL) {
		m_sPreTexts[SETPRETXT_MOTD] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
		m_sPreTexts[SETPRETXT_MOTD] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldMotd, szNeededMem);
    }
#else
	m_sPreTexts[SETPRETXT_MOTD] = (char *)realloc(sOldMotd, szNeededMem);
#endif
    if(m_sPreTexts[SETPRETXT_MOTD] == NULL) {
		m_sPreTexts[SETPRETXT_MOTD] = sOldMotd;

		AppendDebugLogFormat("[MEM] Cannot (re)allocate %zu bytes in SettingManager::UpdateMOTD\n", szNeededMem);

        return;
    }

    int iMsgLen = 0;

    if(m_bBools[SETBOOL_MOTD_AS_PM] == true) {
        iMsgLen = snprintf(m_sPreTexts[SETPRETXT_MOTD], szNeededMem, "$To: %%s From: %s $<%s> %s|", m_sPreTexts[SETPRETXT_HUB_SEC], m_sPreTexts[SETPRETXT_HUB_SEC], m_sMOTD);
    } else {
        iMsgLen = snprintf(m_sPreTexts[SETPRETXT_MOTD], szNeededMem, "<%s> %s|", m_sPreTexts[SETPRETXT_HUB_SEC], m_sMOTD);
    }

    if(iMsgLen <= 0) {
        exit(EXIT_FAILURE);
    }

	m_ui16PreTextsLens[SETPRETXT_MOTD] = (uint16_t)iMsgLen;
}
//---------------------------------------------------------------------------

void SettingManager::UpdateHubNameWelcome() {
    if(m_bUpdateLocked == true) {
        return;
    }

    size_t szNeededMem = 19 + m_ui16TextsLens[SETTXT_HUB_NAME] + m_ui16PreTextsLens[SETPRETXT_HUB_SEC] + LanguageManager::m_Ptr->m_ui16TextsLens[LAN_THIS_HUB_IS_RUNNING] + (sizeof(g_sPtokaXTitle)-1) + LanguageManager::m_Ptr->m_ui16TextsLens[LAN_UPTIME];

    if(m_sTexts[SETTXT_HUB_TOPIC] != NULL) {
        szNeededMem += m_ui16TextsLens[SETTXT_HUB_TOPIC] + 3;
    }

    char * sOldWelcome = m_sPreTexts[SETPRETXT_HUB_NAME_WLCM];

#ifdef _WIN32
    if(m_sPreTexts[SETPRETXT_HUB_NAME_WLCM] == NULL) {
		m_sPreTexts[SETPRETXT_HUB_NAME_WLCM] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
		m_sPreTexts[SETPRETXT_HUB_NAME_WLCM] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldWelcome, szNeededMem);
    }
#else
	m_sPreTexts[SETPRETXT_HUB_NAME_WLCM] = (char *)realloc(sOldWelcome, szNeededMem);
#endif
    if(m_sPreTexts[SETPRETXT_HUB_NAME_WLCM] == NULL) {
		m_sPreTexts[SETPRETXT_HUB_NAME_WLCM] = sOldWelcome;

		AppendDebugLogFormat("[MEM] Cannot (re)allocate %zu bytes in SettingManager::UpdateHubNameWelcome\n", szNeededMem);

        return;
    }

    int iMsgLen = 0;

    if(m_sTexts[SETTXT_HUB_TOPIC] == NULL) {
        iMsgLen = snprintf(m_sPreTexts[SETPRETXT_HUB_NAME_WLCM], szNeededMem, "$HubName %s|<%s> %s %s (%s: ", m_sTexts[SETTXT_HUB_NAME], m_sPreTexts[SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_THIS_HUB_IS_RUNNING], g_sPtokaXTitle, LanguageManager::m_Ptr->m_sTexts[LAN_UPTIME]);
    } else {
        iMsgLen =  snprintf(m_sPreTexts[SETPRETXT_HUB_NAME_WLCM], szNeededMem, "$HubName %s - %s|<%s> %s %s (%s: ", m_sTexts[SETTXT_HUB_NAME], m_sTexts[SETTXT_HUB_TOPIC], m_sPreTexts[SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_THIS_HUB_IS_RUNNING], g_sPtokaXTitle, LanguageManager::m_Ptr->m_sTexts[LAN_UPTIME]);
    }
    
    if(iMsgLen <= 0) {
        exit(EXIT_FAILURE);
    }

	m_ui16PreTextsLens[SETPRETXT_HUB_NAME_WLCM] = (uint16_t)iMsgLen;
}
//---------------------------------------------------------------------------

void SettingManager::UpdateHubName() {
    if(m_bUpdateLocked == true) {
        return;
    }

    size_t szNeededMem = 11 + m_ui16TextsLens[SETTXT_HUB_NAME];

    if(m_sTexts[SETTXT_HUB_TOPIC] != NULL) {
        szNeededMem += m_ui16TextsLens[SETTXT_HUB_TOPIC] + 3;
    }

    char * sOldHubName = m_sPreTexts[SETPRETXT_HUB_NAME];

#ifdef _WIN32
    if(m_sPreTexts[SETPRETXT_HUB_NAME] == NULL) {
		m_sPreTexts[SETPRETXT_HUB_NAME] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
		m_sPreTexts[SETPRETXT_HUB_NAME] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldHubName, szNeededMem);
    }
#else
	m_sPreTexts[SETPRETXT_HUB_NAME] = (char *)realloc(sOldHubName, szNeededMem);
#endif
    if(m_sPreTexts[SETPRETXT_HUB_NAME] == NULL) {
		m_sPreTexts[SETPRETXT_HUB_NAME] = sOldHubName;

		AppendDebugLogFormat("[MEM] Cannot (re)allocate %zu bytes in SettingManager::UpdateHubName\n", szNeededMem);

        return;
    }

	int iMsgLen = 0;

    if(m_sTexts[SETTXT_HUB_TOPIC] == NULL) {
        iMsgLen = snprintf(m_sPreTexts[SETPRETXT_HUB_NAME], szNeededMem, "$HubName %s|", m_sTexts[SETTXT_HUB_NAME]);
    } else {
        iMsgLen = snprintf(m_sPreTexts[SETPRETXT_HUB_NAME], szNeededMem, "$HubName %s - %s|", m_sTexts[SETTXT_HUB_NAME], m_sTexts[SETTXT_HUB_TOPIC]);
    }

    if(iMsgLen <= 0) {
        exit(EXIT_FAILURE);
    }

	m_ui16PreTextsLens[SETPRETXT_HUB_NAME] = (uint16_t)iMsgLen;

	if(ServerManager::m_bServerRunning == true) {
        GlobalDataQueue::m_Ptr->AddQueueItem(m_sPreTexts[SETPRETXT_HUB_NAME], m_ui16PreTextsLens[SETPRETXT_HUB_NAME], NULL, 0, GlobalDataQueue::CMD_HUBNAME);
    }
}
//---------------------------------------------------------------------------

void SettingManager::UpdateRedirectAddress() {
    if(m_bUpdateLocked == true) {
        return;
    }

    if(m_sTexts[SETTXT_REDIRECT_ADDRESS] == NULL) {
        if(m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
#ifdef _WIN32
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS]) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate memory in SettingManager::UpdateRedirectAddress\n");
            }
#else
            free(m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS]);
#endif
			m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] = NULL;
			m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS] = 0;
        }

        return;
    }

    size_t szNeededLen = 13 + m_ui16TextsLens[SETTXT_REDIRECT_ADDRESS];

    char * sOldRedirAddr = m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS];

#ifdef _WIN32
    if(m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] == NULL) {
		m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededLen);
    } else {
		m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldRedirAddr, szNeededLen);
    }
#else
	m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] = (char *)realloc(sOldRedirAddr, szNeededLen);
#endif
    if(m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] == NULL) {
		m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] = sOldRedirAddr;

		AppendDebugLogFormat("[MEM] Cannot (re)allocate %zu bytes in SettingManager::UpdateRedirectAddress\n", szNeededLen);

        return;
    }

    int iMsgLen = snprintf(m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS], szNeededLen, "$ForceMove %s|", m_sTexts[SETTXT_REDIRECT_ADDRESS]);
    if(iMsgLen <= 0) {
        exit(EXIT_FAILURE);
    }

	m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS] = (uint16_t)iMsgLen;
}
//---------------------------------------------------------------------------

void SettingManager::UpdateRegOnlyMessage() {
    if(m_bUpdateLocked == true) {
        return;
    }

    size_t szNeededMem = 5 + m_ui16PreTextsLens[SETPRETXT_HUB_SEC] + m_ui16TextsLens[SETTXT_REG_ONLY_MSG];

    if(m_bBools[SETBOOL_REG_ONLY_REDIR] == true) {
        if(m_sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS] != NULL) {
            szNeededMem += 12 + m_ui16TextsLens[SETTXT_REG_ONLY_REDIR_ADDRESS];
        } else {
            szNeededMem += m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    char * sOldRegOnlyMsg = m_sPreTexts[SETPRETXT_REG_ONLY_MSG];

#ifdef _WIN32
    if(m_sPreTexts[SETPRETXT_REG_ONLY_MSG] == NULL) {
		m_sPreTexts[SETPRETXT_REG_ONLY_MSG] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
		m_sPreTexts[SETPRETXT_REG_ONLY_MSG] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldRegOnlyMsg, szNeededMem);
    }
#else
	m_sPreTexts[SETPRETXT_REG_ONLY_MSG] = (char *)realloc(sOldRegOnlyMsg, szNeededMem);
#endif
    if(m_sPreTexts[SETPRETXT_REG_ONLY_MSG] == NULL) {
		m_sPreTexts[SETPRETXT_REG_ONLY_MSG] = sOldRegOnlyMsg;

		AppendDebugLogFormat("[MEM] Cannot (re)allocate %zu bytes in SettingManager::UpdateRegOnlyMessage\n", szNeededMem);

        return;
    }

    int iMsgLen = snprintf(m_sPreTexts[SETPRETXT_REG_ONLY_MSG], szNeededMem, "<%s> %s|", m_sPreTexts[SETPRETXT_HUB_SEC], m_sTexts[SETTXT_REG_ONLY_MSG]);
    if(iMsgLen <= 0) {
        exit(EXIT_FAILURE);
    }

    if(m_bBools[SETBOOL_REG_ONLY_REDIR] == true) {
        if(m_sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS] != NULL) {
            int iRet = snprintf(m_sPreTexts[SETPRETXT_REG_ONLY_MSG]+iMsgLen, szNeededMem-iMsgLen, "$ForceMove %s|", m_sTexts[SETTXT_REG_ONLY_REDIR_ADDRESS]);
            if(iRet <= 0) {
                exit(EXIT_FAILURE);
            }
            iMsgLen += iRet;
        } else if(m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(m_sPreTexts[SETPRETXT_REG_ONLY_MSG]+iMsgLen, m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            iMsgLen += (int)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

	m_ui16PreTextsLens[SETPRETXT_REG_ONLY_MSG] = (uint16_t)iMsgLen;
	m_sPreTexts[SETPRETXT_REG_ONLY_MSG][iMsgLen] = '\0';
}
//---------------------------------------------------------------------------

void SettingManager::UpdateShareLimitMessage() {
    if(m_bUpdateLocked == true) {
        return;
    }

    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> ", m_sPreTexts[SETPRETXT_HUB_SEC]);
    if(iMsgLen <= 0) {
        exit(EXIT_FAILURE);
    }
    
    static const char* units[] = { "B", "kB", "MB", "GB", "TB", "PB", "EB", " ", " ", " ", " ", " ", " ", " ", " ", " " };

    for(uint16_t ui16i = 0; ui16i < m_ui16TextsLens[SETTXT_SHARE_LIMIT_MSG]; ui16i++) {
        if(m_sTexts[SETTXT_SHARE_LIMIT_MSG][ui16i] == '%') {
            if(strncmp(m_sTexts[SETTXT_SHARE_LIMIT_MSG]+ui16i+1, sMin, 5) == 0) {
                if(m_ui64MinShare != 0) {
                    int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "%hd %s", m_i16Shorts[SETSHORT_MIN_SHARE_LIMIT], units[m_i16Shorts[SETSHORT_MIN_SHARE_UNITS]]);
                    if(iRet <= 0) {
                        exit(EXIT_FAILURE);
                    }
                    iMsgLen += iRet;
                } else {
                    memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, "0 B", 3);
                    iMsgLen += 3;
                }
                ui16i += (uint16_t)5;
                continue;
            } else if(strncmp(m_sTexts[SETTXT_SHARE_LIMIT_MSG]+ui16i+1, sMax, 5) == 0) {
                if(m_ui64MaxShare != 0) {
                    int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "%hd %s", m_i16Shorts[SETSHORT_MAX_SHARE_LIMIT], units[m_i16Shorts[SETSHORT_MAX_SHARE_UNITS]]);
                    if(iRet <= 0) {
                        exit(EXIT_FAILURE);
                    }
                    iMsgLen += iRet;
                } else {
                    memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, LanguageManager::m_Ptr->m_sTexts[LAN_UNLIMITED], LanguageManager::m_Ptr->m_ui16TextsLens[LAN_UNLIMITED]);
                    iMsgLen += LanguageManager::m_Ptr->m_ui16TextsLens[LAN_UNLIMITED];
                }
                ui16i += (uint16_t)5;
                continue;
            }
        }

        ServerManager::m_pGlobalBuffer[iMsgLen] = m_sTexts[SETTXT_SHARE_LIMIT_MSG][ui16i];
        iMsgLen++;
    }

    ServerManager::m_pGlobalBuffer[iMsgLen] = '|';
    iMsgLen++;

    if(m_bBools[SETBOOL_SHARE_LIMIT_REDIR] == true) {
        if(m_sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS] != NULL) {
        	int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "$ForceMove %s|", m_sTexts[SETTXT_SHARE_LIMIT_REDIR_ADDRESS]);
        	if(iRet <= 0) {
                exit(EXIT_FAILURE);
            }
            iMsgLen += iRet;
        }  else if(m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            iMsgLen += (int)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    char * sOldShareLimitMsg = m_sPreTexts[SETPRETXT_SHARE_LIMIT_MSG];

#ifdef _WIN32
    if(m_sPreTexts[SETPRETXT_SHARE_LIMIT_MSG] == NULL) {
		m_sPreTexts[SETPRETXT_SHARE_LIMIT_MSG] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, iMsgLen+1);
    } else {
		m_sPreTexts[SETPRETXT_SHARE_LIMIT_MSG] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldShareLimitMsg, iMsgLen+1);
    }
#else
	m_sPreTexts[SETPRETXT_SHARE_LIMIT_MSG] = (char *)realloc(sOldShareLimitMsg, iMsgLen+1);
#endif
    if(m_sPreTexts[SETPRETXT_SHARE_LIMIT_MSG] == NULL) {
		m_sPreTexts[SETPRETXT_SHARE_LIMIT_MSG] = sOldShareLimitMsg;

		AppendDebugLogFormat("[MEM] Cannot (re)allocate %d bytes in SettingManager::UpdateShareLimitMessage\n", iMsgLen+1);

        return;
    }

	memcpy(m_sPreTexts[SETPRETXT_SHARE_LIMIT_MSG], ServerManager::m_pGlobalBuffer, iMsgLen);
	m_sPreTexts[SETPRETXT_SHARE_LIMIT_MSG][iMsgLen] = '\0';
	m_ui16PreTextsLens[SETPRETXT_SHARE_LIMIT_MSG] = (uint16_t)iMsgLen;
}
//---------------------------------------------------------------------------

void SettingManager::UpdateSlotsLimitMessage() {
    if(m_bUpdateLocked == true) {
        return;
    }

    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> ", m_sPreTexts[SETPRETXT_HUB_SEC]);
    if(iMsgLen <= 0) {
        exit(EXIT_FAILURE);
    }

    for(uint16_t ui16i = 0; ui16i < m_ui16TextsLens[SETTXT_SLOTS_LIMIT_MSG]; ui16i++) {
        if(m_sTexts[SETTXT_SLOTS_LIMIT_MSG][ui16i] == '%') {
            if(strncmp(m_sTexts[SETTXT_SLOTS_LIMIT_MSG]+ui16i+1, sMin, 5) == 0) {
                int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "%hd", m_i16Shorts[SETSHORT_MIN_SLOTS_LIMIT]);
                if(iRet <= 0) {
                    exit(EXIT_FAILURE);
                }
                iMsgLen += iRet;

                ui16i += (uint16_t)5;
                continue;
            } else if(strncmp(m_sTexts[SETTXT_SLOTS_LIMIT_MSG]+ui16i+1, sMax, 5) == 0) {
                if(m_i16Shorts[SETSHORT_MAX_SLOTS_LIMIT] != 0) {
                    int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "%hd", m_i16Shorts[SETSHORT_MAX_SLOTS_LIMIT]);
                    if(iRet <= 0) {
                        exit(EXIT_FAILURE);
                    }
                    iMsgLen += iRet;
                } else {
                    memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, LanguageManager::m_Ptr->m_sTexts[LAN_UNLIMITED], LanguageManager::m_Ptr->m_ui16TextsLens[LAN_UNLIMITED]);
                    iMsgLen += LanguageManager::m_Ptr->m_ui16TextsLens[LAN_UNLIMITED];
                }
                ui16i += (uint16_t)5;
                continue;
            }
        }

        ServerManager::m_pGlobalBuffer[iMsgLen] = m_sTexts[SETTXT_SLOTS_LIMIT_MSG][ui16i];
        iMsgLen++;
    }

    ServerManager::m_pGlobalBuffer[iMsgLen] = '|';
    iMsgLen++;

    if(m_bBools[SETBOOL_SLOTS_LIMIT_REDIR] == true) {
        if(m_sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS] != NULL) {
        	int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "$ForceMove %s|", m_sTexts[SETTXT_SLOTS_LIMIT_REDIR_ADDRESS]);
            if(iRet <= 0) {
                exit(EXIT_FAILURE);
            }
            iMsgLen += iRet;
        }  else if(m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            iMsgLen += (int)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    char * sOldSlotsLimitMsg = m_sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG];

#ifdef _WIN32
    if(m_sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG] == NULL) {
		m_sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, iMsgLen+1);
    } else {
		m_sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldSlotsLimitMsg, iMsgLen+1);
    }
#else
	m_sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG] = (char *)realloc(sOldSlotsLimitMsg, iMsgLen+1);
#endif
    if(m_sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG] == NULL) {
		m_sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG] = sOldSlotsLimitMsg;

		AppendDebugLogFormat("[MEM] Cannot (re)allocate %d bytes in SettingManager::UpdateSlotsLimitMessage\n", iMsgLen+1);

        return;
    }

	memcpy(m_sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG], ServerManager::m_pGlobalBuffer, iMsgLen);
	m_sPreTexts[SETPRETXT_SLOTS_LIMIT_MSG][iMsgLen] = '\0';
	m_ui16PreTextsLens[SETPRETXT_SLOTS_LIMIT_MSG] = (uint16_t)iMsgLen;
}
//---------------------------------------------------------------------------

void SettingManager::UpdateHubSlotRatioMessage() {
    if(m_bUpdateLocked == true) {
        return;
    }

    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> ", m_sPreTexts[SETPRETXT_HUB_SEC]);
    if(iMsgLen <= 0) {
        exit(EXIT_FAILURE);
    }

    static const char * sHubs = "[hubs]";
    static const char * sSlots = "[slots]";

    for(uint16_t ui16i = 0; ui16i < m_ui16TextsLens[SETTXT_HUB_SLOT_RATIO_MSG]; ui16i++) {
        if(m_sTexts[SETTXT_HUB_SLOT_RATIO_MSG][ui16i] == '%') {
            if(strncmp(m_sTexts[SETTXT_HUB_SLOT_RATIO_MSG]+ui16i+1, sHubs, 6) == 0) {
                int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "%hd", m_i16Shorts[SETSHORT_HUB_SLOT_RATIO_HUBS]);
                if(iRet <= 0) {
                    exit(EXIT_FAILURE);
                }
                iMsgLen += iRet;

                ui16i += (uint16_t)6;
                continue;
            } else if(strncmp(m_sTexts[SETTXT_HUB_SLOT_RATIO_MSG]+ui16i+1, sSlots, 7) == 0) {
                int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "%hd", m_i16Shorts[SETSHORT_HUB_SLOT_RATIO_SLOTS]);
                if(iRet <= 0) {
                    exit(EXIT_FAILURE);
                }
                iMsgLen += iRet;

                ui16i += (uint16_t)7;
                continue;
            }
        }

        ServerManager::m_pGlobalBuffer[iMsgLen] = m_sTexts[SETTXT_HUB_SLOT_RATIO_MSG][ui16i];
        iMsgLen++;
    }

    ServerManager::m_pGlobalBuffer[iMsgLen] = '|';
    iMsgLen++;

    if(m_bBools[SETBOOL_HUB_SLOT_RATIO_REDIR] == true) {
        if(m_sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS] != NULL) {
        	int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "$ForceMove %s|", m_sTexts[SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS]);
            if(iRet <= 0) {
                exit(EXIT_FAILURE);
            }
            iMsgLen += iRet;
        }  else if(m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            iMsgLen += (int)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    char * sOldHubSlotLimitMsg = m_sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG];

#ifdef _WIN32
    if(m_sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG] == NULL) {
		m_sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, iMsgLen+1);
    } else {
		m_sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldHubSlotLimitMsg, iMsgLen+1);
    }
#else
	m_sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG] = (char *)realloc(sOldHubSlotLimitMsg, iMsgLen+1);
#endif
    if(m_sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG] == NULL) {
		m_sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG] = sOldHubSlotLimitMsg;

		AppendDebugLogFormat("[MEM] Cannot (re)allocate %d bytes in SettingManager::UpdateHubSlotRatioMessage\n", iMsgLen+1);

        return;
    }

	memcpy(m_sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG], ServerManager::m_pGlobalBuffer, iMsgLen);
	m_sPreTexts[SETPRETXT_HUB_SLOT_RATIO_MSG][iMsgLen] = '\0';
	m_ui16PreTextsLens[SETPRETXT_HUB_SLOT_RATIO_MSG] = (uint16_t)iMsgLen;
}
//---------------------------------------------------------------------------

void SettingManager::UpdateMaxHubsLimitMessage() {
    if(m_bUpdateLocked == true) {
        return;
    }

    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> ", m_sPreTexts[SETPRETXT_HUB_SEC]);
    if(iMsgLen <= 0) {
        exit(EXIT_FAILURE);
    }

    static const char* sHubs = "%[hubs]";

    char * sMatch = strstr(m_sTexts[SETTXT_MAX_HUBS_LIMIT_MSG], sHubs);

    if(sMatch != NULL) {
        if(sMatch > m_sTexts[SETTXT_MAX_HUBS_LIMIT_MSG]) {
            size_t szLen = sMatch-m_sTexts[SETTXT_MAX_HUBS_LIMIT_MSG];
            memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, m_sTexts[SETTXT_MAX_HUBS_LIMIT_MSG], szLen);
            iMsgLen += (int)szLen;
        }

        int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "%hd", m_i16Shorts[SETSHORT_MAX_HUBS_LIMIT]);
        if(iRet <= 0) {
            exit(EXIT_FAILURE);
        }
        iMsgLen += iRet;

        if(sMatch+7 < m_sTexts[SETTXT_MAX_HUBS_LIMIT_MSG]+m_ui16TextsLens[SETTXT_MAX_HUBS_LIMIT_MSG]) {
            size_t szLen = (m_sTexts[SETTXT_MAX_HUBS_LIMIT_MSG]+m_ui16TextsLens[SETTXT_MAX_HUBS_LIMIT_MSG])-(sMatch+7);
            memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, sMatch+7, szLen);
            iMsgLen += (int)szLen;
        }
    } else {
        memcpy(ServerManager::m_pGlobalBuffer, m_sTexts[SETTXT_MAX_HUBS_LIMIT_MSG], m_ui16TextsLens[SETTXT_MAX_HUBS_LIMIT_MSG]);
        iMsgLen = (int)m_ui16TextsLens[SETTXT_MAX_HUBS_LIMIT_MSG];
    }

    ServerManager::m_pGlobalBuffer[iMsgLen] = '|';
    iMsgLen++;

    if(m_bBools[SETBOOL_MAX_HUBS_LIMIT_REDIR] == true) {
        if(m_sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS] != NULL) {
        	int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "$ForceMove %s|", m_sTexts[SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS]);
            if(iRet <= 0) {
                exit(EXIT_FAILURE);
            }
            iMsgLen += iRet;
        }  else if(m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            iMsgLen += (int)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    char * sOldHubLimitMsg = m_sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG];

#ifdef _WIN32
    if(m_sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG] == NULL) {
		m_sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, iMsgLen+1);
    } else {
		m_sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldHubLimitMsg, iMsgLen+1);
    }
#else
	m_sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG] = (char *)realloc(sOldHubLimitMsg, iMsgLen+1);
#endif
    if(m_sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG] == NULL) {
		m_sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG] = sOldHubLimitMsg;

		AppendDebugLogFormat("[MEM] Cannot (re)allocate %d bytes in SettingManager::UpdateMaxHubsLimitMessage\n", iMsgLen+1);

        return;
    }

	memcpy(m_sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG], ServerManager::m_pGlobalBuffer, iMsgLen);
	m_sPreTexts[SETPRETXT_MAX_HUBS_LIMIT_MSG][iMsgLen] = '\0';
	m_ui16PreTextsLens[SETPRETXT_MAX_HUBS_LIMIT_MSG] = (uint16_t)iMsgLen;
}
//---------------------------------------------------------------------------

void SettingManager::UpdateNoTagMessage() {
    if(m_bUpdateLocked == true) {
        return;
    }

    if(m_i16Shorts[SETSHORT_NO_TAG_OPTION] == 0) {
        if(m_sPreTexts[SETPRETXT_NO_TAG_MSG] != NULL) {
#ifdef _WIN32
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sPreTexts[SETPRETXT_NO_TAG_MSG]) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate memory in SettingManager::UpdateNoTagMessage\n");
            }
#else
            free(m_sPreTexts[SETPRETXT_NO_TAG_MSG]);
#endif
			m_sPreTexts[SETPRETXT_NO_TAG_MSG] = NULL;
			m_ui16PreTextsLens[SETPRETXT_NO_TAG_MSG] = 0;
        }

        return;
    }

    size_t szNeededMem = 5 + m_ui16PreTextsLens[SETPRETXT_HUB_SEC] + m_ui16TextsLens[SETTXT_NO_TAG_MSG];

    if(m_i16Shorts[SETSHORT_NO_TAG_OPTION] == 2) {
        if(m_sTexts[SETTXT_NO_TAG_REDIR_ADDRESS] != NULL) {
            szNeededMem += 12 + m_ui16TextsLens[SETTXT_NO_TAG_REDIR_ADDRESS];
        }  else if(m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	szNeededMem += m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    char * sOldNoTagMsg = m_sPreTexts[SETPRETXT_NO_TAG_MSG];

#ifdef _WIN32
    if(m_sPreTexts[SETPRETXT_NO_TAG_MSG] == NULL) {
		m_sPreTexts[SETPRETXT_NO_TAG_MSG] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
		m_sPreTexts[SETPRETXT_NO_TAG_MSG] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldNoTagMsg, szNeededMem);
    }
#else
	m_sPreTexts[SETPRETXT_NO_TAG_MSG] = (char *)realloc(sOldNoTagMsg, szNeededMem);
#endif
    if(m_sPreTexts[SETPRETXT_NO_TAG_MSG] == NULL) {
		m_sPreTexts[SETPRETXT_NO_TAG_MSG] = sOldNoTagMsg;

		AppendDebugLogFormat("[MEM] Cannot (re)allocate %zu bytes in SettingManager::UpdateNoTagMessage\n", szNeededMem);

        return;
    }

    int iMsgLen = snprintf(m_sPreTexts[SETPRETXT_NO_TAG_MSG], szNeededMem, "<%s> %s|", m_sPreTexts[SETPRETXT_HUB_SEC], m_sTexts[SETTXT_NO_TAG_MSG]);
    if(iMsgLen <= 0) {
        exit(EXIT_FAILURE);
    }

    if(m_i16Shorts[SETSHORT_NO_TAG_OPTION] == 2) {
        if(m_sTexts[SETTXT_NO_TAG_REDIR_ADDRESS] != NULL) {
           	int iRet = snprintf(m_sPreTexts[SETPRETXT_NO_TAG_MSG]+iMsgLen, szNeededMem-iMsgLen, "$ForceMove %s|", m_sTexts[SETTXT_NO_TAG_REDIR_ADDRESS]);
            if(iRet <= 0) {
                exit(EXIT_FAILURE);
            }
            iMsgLen += iRet;
        }  else if(m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(m_sPreTexts[SETPRETXT_NO_TAG_MSG]+iMsgLen, m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            iMsgLen += (int)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

	m_ui16PreTextsLens[SETPRETXT_NO_TAG_MSG] = (uint16_t)iMsgLen;
	m_sPreTexts[SETPRETXT_NO_TAG_MSG][iMsgLen] = '\0';
}
//---------------------------------------------------------------------------

void SettingManager::UpdateTempBanRedirAddress() {
    if(m_bUpdateLocked == true) {
        return;
    }

    size_t szNeededMem = 1;

    if(m_sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS] != NULL) {
        szNeededMem += 12 + m_ui16TextsLens[SETTXT_TEMP_BAN_REDIR_ADDRESS];
    }  else if(m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		szNeededMem += m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
    } else {
        if(m_sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] != NULL) {
#ifdef _WIN32
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS]) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate memory in SettingManager::UpdateTempBanRedirAddress\n");
            }
#else
            free(m_sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS]);
#endif
			m_sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = NULL;
			m_ui16PreTextsLens[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = 0;
        }

        return;
    }

    char * sOldTempBanRedirMsg = m_sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS];

#ifdef _WIN32
    if(m_sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] == NULL) {
		m_sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
		m_sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldTempBanRedirMsg, szNeededMem);
    }
#else
	m_sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = (char *)realloc(sOldTempBanRedirMsg, szNeededMem);
#endif
    if(m_sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] == NULL) {
		m_sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = sOldTempBanRedirMsg;

		AppendDebugLogFormat("[MEM] Cannot (re)allocate %zu bytes in SettingManager::UpdateTempBanRedirAddress\n", szNeededMem);

        return;
    }

    int iMsgLen = 0;

    if(m_sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS] != NULL) {
        iMsgLen = snprintf(m_sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS], szNeededMem, "$ForceMove %s|", m_sTexts[SETTXT_TEMP_BAN_REDIR_ADDRESS]);
        if(iMsgLen <= 0) {
            exit(EXIT_FAILURE);
        }
    } else if(m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		memcpy(m_sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS], m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
        iMsgLen = (int)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
    }

	m_ui16PreTextsLens[SETPRETXT_TEMP_BAN_REDIR_ADDRESS] = (uint16_t)iMsgLen;
	m_sPreTexts[SETPRETXT_TEMP_BAN_REDIR_ADDRESS][iMsgLen] = '\0';
}
//---------------------------------------------------------------------------

void SettingManager::UpdatePermBanRedirAddress() {
    if(m_bUpdateLocked == true) {
        return;
    }

    size_t szNeededMem = 1;

    if(m_sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS] != NULL) {
        szNeededMem += 12 + m_ui16TextsLens[SETTXT_PERM_BAN_REDIR_ADDRESS];
    }  else if(m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		szNeededMem += m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
    } else {
        if(m_sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] != NULL) {
#ifdef _WIN32
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS]) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate memory in SettingManager::UpdatePermBanRedirAddress\n");
            }
#else
            free(m_sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS]);
#endif
			m_sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = NULL;
			m_ui16PreTextsLens[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = 0;
        }

        return;
    }

    char * sOldPermBanRedirMsg = m_sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS];

#ifdef _WIN32
    if(m_sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] == NULL) {
		m_sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
		m_sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldPermBanRedirMsg, szNeededMem);
    }
#else
	m_sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = (char *)realloc(sOldPermBanRedirMsg, szNeededMem);
#endif
    if(m_sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] == NULL) {
		m_sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = sOldPermBanRedirMsg;

		AppendDebugLogFormat("[MEM] Cannot (re)allocate %zu bytes in SettingManager::UpdatePermBanRedirAddress\n", szNeededMem);

        return;
    }

    int iMsgLen = 0;

    if(m_sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS] != NULL) {
        iMsgLen = snprintf(m_sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS], szNeededMem, "$ForceMove %s|", m_sTexts[SETTXT_PERM_BAN_REDIR_ADDRESS]);
        if(iMsgLen <= 0) {
            exit(EXIT_FAILURE);
        }
    } else if(m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		memcpy(m_sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS], m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
        iMsgLen = (int)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
    }

	m_ui16PreTextsLens[SETPRETXT_PERM_BAN_REDIR_ADDRESS] = (uint16_t)iMsgLen;
	m_sPreTexts[SETPRETXT_PERM_BAN_REDIR_ADDRESS][iMsgLen] = '\0';
}
//---------------------------------------------------------------------------

void SettingManager::UpdateNickLimitMessage() {
    if(m_bUpdateLocked == true) {
        return;
    }

    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> ", m_sPreTexts[SETPRETXT_HUB_SEC]);
    if(iMsgLen <= 0) {
        exit(EXIT_FAILURE);
    }

    for(uint16_t ui16i = 0; ui16i < m_ui16TextsLens[SETTXT_NICK_LIMIT_MSG]; ui16i++) {
        if(m_sTexts[SETTXT_NICK_LIMIT_MSG][ui16i] == '%') {
            if(strncmp(m_sTexts[SETTXT_NICK_LIMIT_MSG]+ui16i+1, sMin, 5) == 0) {
                int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "%hd", m_i16Shorts[SETSHORT_MIN_NICK_LEN]);
                if(iRet <= 0) {
                    exit(EXIT_FAILURE);
                }
                iMsgLen += iRet;

                ui16i += (uint16_t)5;
                continue;
            } else if(strncmp(m_sTexts[SETTXT_NICK_LIMIT_MSG]+ui16i+1, sMax, 5) == 0) {
                if(m_i16Shorts[SETSHORT_MAX_NICK_LEN] != 0) {
                    int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "%hd", m_i16Shorts[SETSHORT_MAX_NICK_LEN]);
                    if(iRet <= 0) {
                        exit(EXIT_FAILURE);
                    }
                    iMsgLen += iRet;
                } else {
                    memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, LanguageManager::m_Ptr->m_sTexts[LAN_UNLIMITED], LanguageManager::m_Ptr->m_ui16TextsLens[LAN_UNLIMITED]);
                    iMsgLen += LanguageManager::m_Ptr->m_ui16TextsLens[LAN_UNLIMITED];
                }
                ui16i += (uint16_t)5;
                continue;
            }
        }

        ServerManager::m_pGlobalBuffer[iMsgLen] = m_sTexts[SETTXT_NICK_LIMIT_MSG][ui16i];
        iMsgLen++;
    }

    ServerManager::m_pGlobalBuffer[iMsgLen] = '|';
    iMsgLen++;

    if(m_bBools[SETBOOL_NICK_LIMIT_REDIR] == true) {
        if(m_sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS] != NULL) {
            int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "$ForceMove %s|", m_sTexts[SETTXT_NICK_LIMIT_REDIR_ADDRESS]);
            if(iRet <= 0) {
                exit(EXIT_FAILURE);
            }
            iMsgLen += iRet;
        }  else if(m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS] != NULL) {
  		   	memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, m_sPreTexts[SETPRETXT_REDIRECT_ADDRESS], (size_t)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS]);
            iMsgLen += (int)m_ui16PreTextsLens[SETPRETXT_REDIRECT_ADDRESS];
        }
    }

    char * sOldNickLimitMsg = m_sPreTexts[SETPRETXT_NICK_LIMIT_MSG];

#ifdef _WIN32
    if(m_sPreTexts[SETPRETXT_NICK_LIMIT_MSG] == NULL) {
		m_sPreTexts[SETPRETXT_NICK_LIMIT_MSG] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, iMsgLen+1);
    } else {
		m_sPreTexts[SETPRETXT_NICK_LIMIT_MSG] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldNickLimitMsg, iMsgLen+1);
    }
#else
	m_sPreTexts[SETPRETXT_NICK_LIMIT_MSG] = (char *)realloc(sOldNickLimitMsg, iMsgLen+1);
#endif
    if(m_sPreTexts[SETPRETXT_NICK_LIMIT_MSG] == NULL) {
		m_sPreTexts[SETPRETXT_NICK_LIMIT_MSG] = sOldNickLimitMsg;

		AppendDebugLogFormat("[MEM] Cannot (re)allocate %d bytes in SettingManager::UpdateNickLimitMessage\n", iMsgLen+1);

        return;
    }

	memcpy(m_sPreTexts[SETPRETXT_NICK_LIMIT_MSG], ServerManager::m_pGlobalBuffer, iMsgLen);
	m_sPreTexts[SETPRETXT_NICK_LIMIT_MSG][iMsgLen] = '\0';
	m_ui16PreTextsLens[SETPRETXT_NICK_LIMIT_MSG] = (uint16_t)iMsgLen;
}
//---------------------------------------------------------------------------

void SettingManager::UpdateMinShare() {
    if(m_bUpdateLocked == true) {
        return;
    }

	m_ui64MinShare = (uint64_t)(m_i16Shorts[SETSHORT_MIN_SHARE_LIMIT] == 0 ? 0 : m_i16Shorts[SETSHORT_MIN_SHARE_LIMIT] * pow(1024.0, (int)m_i16Shorts[SETSHORT_MIN_SHARE_UNITS]));
}
//---------------------------------------------------------------------------

void SettingManager::UpdateMaxShare() {
    if(m_bUpdateLocked == true) {
        return;
    }

	m_ui64MaxShare = (uint64_t)(m_i16Shorts[SETSHORT_MAX_SHARE_LIMIT] == 0 ? 0 : m_i16Shorts[SETSHORT_MAX_SHARE_LIMIT] * pow(1024.0, (int)m_i16Shorts[SETSHORT_MAX_SHARE_UNITS]));
}
//---------------------------------------------------------------------------

void SettingManager::UpdateTCPPorts() {
    if(m_bUpdateLocked == true) {
        return;
    }

    char * sPort = m_sTexts[SETTXT_TCP_PORTS];
    uint8_t ui8ActualPort = 0;
    for(uint16_t ui16i = 0; ui16i < m_ui16TextsLens[SETTXT_TCP_PORTS] && ui8ActualPort < 25; ui16i++) {
        if(m_sTexts[SETTXT_TCP_PORTS][ui16i] == ';') {
			m_sTexts[SETTXT_TCP_PORTS][ui16i] = '\0';

            if(ui8ActualPort != 0) {
				m_ui16PortNumbers[ui8ActualPort] = (uint16_t)atoi(sPort);
            } else {
#ifdef _WIN32
                EnterCriticalSection(&m_csSetting);
#else
				pthread_mutex_lock(&m_mtxSetting);
#endif

				m_ui16PortNumbers[ui8ActualPort] = (uint16_t)atoi(sPort);

#ifdef _WIN32
                LeaveCriticalSection(&m_csSetting);
#else
				pthread_mutex_unlock(&m_mtxSetting);
#endif
            }

			m_sTexts[SETTXT_TCP_PORTS][ui16i] = ';';

            sPort = m_sTexts[SETTXT_TCP_PORTS]+ui16i+1;
            ui8ActualPort++;
            continue;
        }
    }

    if(sPort[0] != '\0') {
		m_ui16PortNumbers[ui8ActualPort] = (uint16_t)atoi(sPort);
        ui8ActualPort++;
    }

    while(ui8ActualPort < 25) {
		m_ui16PortNumbers[ui8ActualPort] = 0;
        ui8ActualPort++;
    }

	if(ServerManager::m_bServerRunning == false) {
        return;
    }

	ServerManager::UpdateServers();
}
//---------------------------------------------------------------------------

void SettingManager::UpdateBotsSameNick() {
    if(m_bUpdateLocked == true) {
        return;
    }

    if(m_sTexts[SETTXT_BOT_NICK] != NULL && m_sTexts[SETTXT_OP_CHAT_NICK] != NULL && 
		m_bBools[SETBOOL_REG_BOT] == true && m_bBools[SETBOOL_REG_OP_CHAT] == true) {
		m_bBotsSameNick = (strcasecmp(m_sTexts[SETTXT_BOT_NICK], m_sTexts[SETTXT_OP_CHAT_NICK]) == 0);
    } else {
		m_bBotsSameNick = false;
    }
}
//---------------------------------------------------------------------------

void SettingManager::UpdateLanguage() {
    if(m_bUpdateLocked == true) {
        return;
    }

    LanguageManager::m_Ptr->Load();

    UpdateHubNameWelcome();

#ifdef _BUILD_GUI
    MainWindow::m_Ptr->UpdateLanguage();
#endif
}
//---------------------------------------------------------------------------

void SettingManager::UpdateBot(const bool bNickChanged/* = true*/) {
    if(m_bUpdateLocked == true) {
        return;
	}

    if(m_bBools[SETBOOL_REG_BOT] == false) {
        if(m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO] != NULL) {
#ifdef _WIN32
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO]) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate memory in SettingManager::UpdateBot\n");
            }
#else
            free(m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO]);
#endif
			m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO] = NULL;
			m_ui16PreTextsLens[SETPRETXT_HUB_BOT_MYINFO] = 0;
        }

        return;
    }

    size_t szNeededMem = 23 + m_ui16TextsLens[SETTXT_BOT_NICK] + m_ui16TextsLens[SETTXT_BOT_DESCRIPTION] + m_ui16TextsLens[SETTXT_BOT_EMAIL];

    char * sOldHubBotMyinfoMsg = m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO];

#ifdef _WIN32
    if(m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO] == NULL) {
		m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
		m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldHubBotMyinfoMsg, szNeededMem);
    }
#else
	m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO] = (char *)realloc(sOldHubBotMyinfoMsg, szNeededMem);
#endif
    if(m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO] == NULL) {
		m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO] = sOldHubBotMyinfoMsg;

		AppendDebugLogFormat("[MEM] Cannot (re)allocate %zu bytes in SettingManager::UpdateBot\n", szNeededMem);

        return;
    }

    int iMsgLen = snprintf(m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO], szNeededMem, "$MyINFO $ALL %s %s$ $$%s$$|", m_sTexts[SETTXT_BOT_NICK], m_sTexts[SETTXT_BOT_DESCRIPTION] != NULL ? m_sTexts[SETTXT_BOT_DESCRIPTION] : "", m_sTexts[SETTXT_BOT_EMAIL] != NULL ? m_sTexts[SETTXT_BOT_EMAIL] : "");
    if(iMsgLen <= 0) {
        exit(EXIT_FAILURE);
    }

	m_ui16PreTextsLens[SETPRETXT_HUB_BOT_MYINFO] = (uint16_t)iMsgLen;
	m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO][iMsgLen] = '\0';

    if(ServerManager::m_pServersS == NULL) {
        return;
    }

    if(bNickChanged == true && (m_bBotsSameNick == false || ServerManager::m_pServersS->m_bActive == false)) {
        Users::m_Ptr->AddBot2NickList(m_sTexts[SETTXT_BOT_NICK], (size_t)m_ui16TextsLens[SETTXT_BOT_NICK], true);
    }

    Users::m_Ptr->AddBot2MyInfos(m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO]);

    if(ServerManager::m_pServersS->m_bActive == false) {
        return;
    }

    if(bNickChanged == true && m_bBotsSameNick == false) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Hello %s|", m_sTexts[SETTXT_BOT_NICK]);
        if(iMsgLen > 0) {
            GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, iMsgLen, NULL, 0, GlobalDataQueue::CMD_HELLO);
        }
    }

    GlobalDataQueue::m_Ptr->AddQueueItem(m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO], m_ui16PreTextsLens[SETPRETXT_HUB_BOT_MYINFO],
		m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO], m_ui16PreTextsLens[SETPRETXT_HUB_BOT_MYINFO], GlobalDataQueue::CMD_MYINFO);

    if(bNickChanged == true) {
        GlobalDataQueue::m_Ptr->OpListStore(m_sTexts[SETTXT_BOT_NICK]);
    }
}
//---------------------------------------------------------------------------

void SettingManager::DisableBot(const bool bNickChanged/* = true*/, const bool bRemoveMyINFO/* = true*/) {
	if(m_bUpdateLocked == true || ServerManager::m_bServerRunning == false) {
        return;
    }

    if(bNickChanged == true) {
        if(m_bBotsSameNick == false) {
            Users::m_Ptr->DelFromNickList(m_sTexts[SETTXT_BOT_NICK], true);
        }

        int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Quit %s|", m_sTexts[SETTXT_BOT_NICK]);
        if(iMsgLen > 0) {
            if(m_bBotsSameNick == true) {
                // PPK ... send Quit only to users without opchat permission...
                User * curUser = NULL,
                    * next = Users::m_Ptr->m_pUserListS;

                while(next != NULL) {
                    curUser = next;
                    next = curUser->m_pNext;
                    if(curUser->m_ui8State == User::STATE_ADDED && ProfileManager::m_Ptr->IsAllowed(curUser, ProfileManager::ALLOWEDOPCHAT) == false) {
                        curUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iMsgLen);
                    }
                }
            } else {
                GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, iMsgLen, NULL, 0, GlobalDataQueue::CMD_QUIT);
            }
        }
    }

    if(m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO] != NULL && m_bBotsSameNick == false && bRemoveMyINFO == true) {
        Users::m_Ptr->DelBotFromMyInfos(m_sPreTexts[SETPRETXT_HUB_BOT_MYINFO]);
    }
}
//---------------------------------------------------------------------------

void SettingManager::UpdateOpChat(const bool bNickChanged/* = true*/) {
    if(m_bUpdateLocked == true) {
        return;
    }

    if(m_bBools[SETBOOL_REG_OP_CHAT] == false) {
        if(m_sPreTexts[SETPRETXT_OP_CHAT_HELLO] != NULL) {
#ifdef _WIN32
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sPreTexts[SETPRETXT_OP_CHAT_HELLO]) == 0) {
				AppendDebugLog("%s - [MEM] Cannot deallocate memory in SettingManager::UpdateOpChat\n");
            }
#else
			free(m_sPreTexts[SETPRETXT_OP_CHAT_HELLO]);
#endif
			m_sPreTexts[SETPRETXT_OP_CHAT_HELLO] = NULL;
			m_ui16PreTextsLens[SETPRETXT_OP_CHAT_HELLO] = 0;
        }

        if(m_sPreTexts[SETPRETXT_OP_CHAT_MYINFO] != NULL) {
#ifdef _WIN32
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sPreTexts[SETPRETXT_OP_CHAT_MYINFO]) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate memory in SettingManager::UpdateOpChat1\n");
            }
#else
            free(m_sPreTexts[SETPRETXT_OP_CHAT_MYINFO]);
#endif
			m_sPreTexts[SETPRETXT_OP_CHAT_MYINFO] = NULL;
			m_ui16PreTextsLens[SETPRETXT_OP_CHAT_MYINFO] = 0;
        }

        return;
    }

    size_t szNeededMem = 9 + m_ui16TextsLens[SETTXT_OP_CHAT_NICK];

    char * sOldOpChatHelloMsg = m_sPreTexts[SETPRETXT_OP_CHAT_HELLO];

#ifdef _WIN32
    if(m_sPreTexts[SETPRETXT_OP_CHAT_HELLO] == NULL) {
		m_sPreTexts[SETPRETXT_OP_CHAT_HELLO] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
		m_sPreTexts[SETPRETXT_OP_CHAT_HELLO] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldOpChatHelloMsg, szNeededMem);
    }
#else
	m_sPreTexts[SETPRETXT_OP_CHAT_HELLO] = (char *)realloc(sOldOpChatHelloMsg, szNeededMem);
#endif
    if(m_sPreTexts[SETPRETXT_OP_CHAT_HELLO] == NULL) {
		m_sPreTexts[SETPRETXT_OP_CHAT_HELLO] = sOldOpChatHelloMsg;

		AppendDebugLogFormat("[MEM] Cannot (re)allocate %zu bytes in SettingManager::UpdateOpChat\n", szNeededMem);

        return;
    }

    int iMsgLen = snprintf(m_sPreTexts[SETPRETXT_OP_CHAT_HELLO], szNeededMem, "$Hello %s|", m_sTexts[SETTXT_OP_CHAT_NICK]);
    if(iMsgLen <= 0) {
        exit(EXIT_FAILURE);
    }

	m_ui16PreTextsLens[SETPRETXT_OP_CHAT_HELLO] = (uint16_t)iMsgLen;
	m_sPreTexts[SETPRETXT_OP_CHAT_HELLO][iMsgLen] = '\0';

    szNeededMem = 23 + m_ui16TextsLens[SETTXT_OP_CHAT_NICK] + m_ui16TextsLens[SETTXT_OP_CHAT_DESCRIPTION] + m_ui16TextsLens[SETTXT_OP_CHAT_EMAIL];

    char * sOldOpChatMyInfoMsg = m_sPreTexts[SETPRETXT_OP_CHAT_MYINFO];

#ifdef _WIN32
    if(m_sPreTexts[SETPRETXT_OP_CHAT_MYINFO] == NULL) {
		m_sPreTexts[SETPRETXT_OP_CHAT_MYINFO] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNeededMem);
    } else {
		m_sPreTexts[SETPRETXT_OP_CHAT_MYINFO] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldOpChatMyInfoMsg, szNeededMem);
    }
#else
	m_sPreTexts[SETPRETXT_OP_CHAT_MYINFO] = (char *)realloc(sOldOpChatMyInfoMsg, szNeededMem);
#endif
    if(m_sPreTexts[SETPRETXT_OP_CHAT_MYINFO] == NULL) {
		m_sPreTexts[SETPRETXT_OP_CHAT_MYINFO] = sOldOpChatMyInfoMsg;

		AppendDebugLogFormat("[MEM] Cannot (re)allocate %zu bytes in SettingManager::UpdateOpChat1\n", szNeededMem);

		if(m_sPreTexts[SETPRETXT_OP_CHAT_MYINFO] == NULL) {
            exit(EXIT_FAILURE);
        }
        return;
    }

    iMsgLen = snprintf(m_sPreTexts[SETPRETXT_OP_CHAT_MYINFO], szNeededMem, "$MyINFO $ALL %s %s$ $$%s$$|", m_sTexts[SETTXT_OP_CHAT_NICK], m_sTexts[SETTXT_OP_CHAT_DESCRIPTION] != NULL ? m_sTexts[SETTXT_OP_CHAT_DESCRIPTION] : "", m_sTexts[SETTXT_OP_CHAT_EMAIL] != NULL ? m_sTexts[SETTXT_OP_CHAT_EMAIL] : "");
    if(iMsgLen <= 0) {
        exit(EXIT_FAILURE);
    }

	m_ui16PreTextsLens[SETPRETXT_OP_CHAT_MYINFO] = (uint16_t)iMsgLen;
	m_sPreTexts[SETPRETXT_OP_CHAT_MYINFO][iMsgLen] = '\0';

	if(ServerManager::m_bServerRunning == false) {
        return;
    }

    if(m_bBotsSameNick == false) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$OpList %s$$|", m_sTexts[SETTXT_OP_CHAT_NICK]);
        if(iMsgLen <= 0) {
            exit(EXIT_FAILURE);
        }

        User * curUser = NULL,
            * next = Users::m_Ptr->m_pUserListS;

        while(next != NULL) {
            curUser = next;
            next = curUser->m_pNext;
            if(curUser->m_ui8State == User::STATE_ADDED && ProfileManager::m_Ptr->IsAllowed(curUser, ProfileManager::ALLOWEDOPCHAT) == true) {
                if(bNickChanged == true && (((curUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false)) {
                    curUser->SendCharDelayed(m_sPreTexts[SETPRETXT_OP_CHAT_HELLO], m_ui16PreTextsLens[SETPRETXT_OP_CHAT_HELLO]);
                }
                curUser->SendCharDelayed(m_sPreTexts[SETPRETXT_OP_CHAT_MYINFO], m_ui16PreTextsLens[SETPRETXT_OP_CHAT_MYINFO]);
                if(bNickChanged == true) {
                    curUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iMsgLen);
                }    
            }
        }
    }
}
//---------------------------------------------------------------------------

void SettingManager::DisableOpChat(const bool bNickChanged/* = true*/) {
    if(m_bUpdateLocked == true || ServerManager::m_pServersS == NULL || m_bBotsSameNick == true) {
        return;
    }

    if(bNickChanged == true) {
        Users::m_Ptr->DelFromNickList(m_sTexts[SETTXT_OP_CHAT_NICK], true);

        int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Quit %s|", m_sTexts[SETTXT_OP_CHAT_NICK]);
        if(iMsgLen > 0) {
            User * curUser = NULL,
                * next = Users::m_Ptr->m_pUserListS;

            while(next != NULL) {
                curUser = next;
                next = curUser->m_pNext;
                if(curUser->m_ui8State == User::STATE_ADDED && ProfileManager::m_Ptr->IsAllowed(curUser, ProfileManager::ALLOWEDOPCHAT) == true) {
                    curUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iMsgLen);
                }
            }
        }
    }
}
//---------------------------------------------------------------------------

void SettingManager::UpdateUDPPort() {
	if(m_bUpdateLocked == true || ServerManager::m_bServerRunning == false) {
        return;
    }

    UDPThread::Destroy(UDPThread::m_PtrIPv6);
    UDPThread::m_PtrIPv6 = NULL;

    UDPThread::Destroy(UDPThread::m_PtrIPv4);
    UDPThread::m_PtrIPv4 = NULL;

    if((uint16_t)atoi(m_sTexts[SETTXT_UDP_PORT]) != 0) {
    	if(ServerManager::m_bUseIPv6 == false) {
    		UDPThread::m_PtrIPv6 = UDPThread::Create(AF_INET);
    		return;
    	}
    	
    	UDPThread::m_PtrIPv6 = UDPThread::Create(AF_INET6);

		if(m_bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true || ServerManager::m_bIPv6DualStack == false) {
			UDPThread::m_PtrIPv6 = UDPThread::Create(AF_INET);
		}
	}
}
//---------------------------------------------------------------------------

void SettingManager::UpdateScripting() const {
    if(m_bUpdateLocked == true || ServerManager::m_bServerRunning == false) {
        return;
    }

    if(m_bBools[SETBOOL_ENABLE_SCRIPTING] == true) {
		ScriptManager::m_Ptr->Start();
		ScriptManager::m_Ptr->OnStartup();
    } else {
		ScriptManager::m_Ptr->OnExit(true);
		ScriptManager::m_Ptr->Stop();
    }
}
//---------------------------------------------------------------------------

void SettingManager::UpdateDatabase() {
#ifdef _WITH_SQLITE
	if(DBSQLite::m_Ptr == NULL) {
		return;
	}

	delete DBSQLite::m_Ptr;

	DBSQLite::m_Ptr = new (std::nothrow) DBSQLite();
	if(DBSQLite::m_Ptr == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate DBSQLite::m_Ptr in SettingManager::SetBool\n");
		exit(EXIT_FAILURE);
	}
#elif _WITH_POSTGRES
	if(DBPostgreSQL::m_Ptr == NULL) {
		return;
	}

	delete DBPostgreSQL::m_Ptr;
	
	DBPostgreSQL::m_Ptr = new (std::nothrow) DBPostgreSQL();
	if(DBPostgreSQL::m_Ptr == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate DBPostgreSQL::m_Ptr in SettingManager::SetBool\n");
		exit(EXIT_FAILURE);
	}
#elif _WITH_MYSQL
	if(DBMySQL::m_Ptr == NULL) {
		return;
	}

	delete DBMySQL::m_Ptr;
	
	DBMySQL::m_Ptr = new (std::nothrow) DBMySQL();
	if(DBMySQL::m_Ptr == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate DBMySQL::m_Ptr in SettingManager::SetBool\n");
		exit(EXIT_FAILURE);
	}
#endif
}
//---------------------------------------------------------------------------

void SettingManager::CmdLineBasicSetup() {
	m_bUpdateLocked = true;

	printf("\nWelcome to basic setup.\nYou will now be asked for few settings required to run PtokaX.\nWhen you don't want to change default settings, then simply press enter.\n");

	int16_t i16MaxUsers = 0;
	char sMaxUsers[7];

maxusers:
	printf("%sActual value is: %hd\nEnter new value: ", SetShortCom[SETSHORT_MAX_USERS]+2, m_i16Shorts[SETSHORT_MAX_USERS]);
	if(fgets(sMaxUsers, 7, stdin) != NULL) {
		if(sMaxUsers[0] != '\n') {
			char * sMatch = strchr(sMaxUsers, '\n');
			if(sMatch != NULL) {
				sMatch[0] = '\0';
			}
	
			uint8_t ui8Len = (uint8_t)strlen(sMaxUsers);
	
			for(uint8_t ui8i = 0; ui8i < ui8Len; ui8i++) {
				if(isdigit(sMaxUsers[ui8i]) == 0) {
					printf("Character '%c' is not valid number!\n", sMaxUsers[ui8i]);
	
					if(WantAgain() == false) {
						return;
					}
	
					goto maxusers;
				}
			}
	
			i16MaxUsers = (int16_t)atoi(sMaxUsers);
			SetShort(SETSHORT_MAX_USERS, i16MaxUsers);
	
			if(i16MaxUsers != m_i16Shorts[SETSHORT_MAX_USERS]) {
				printf("Failed to set value %hd!\n", i16MaxUsers);
	
				if(WantAgain() == false) {
					return;
				}
	
				goto maxusers;
			}
		}
	} else {
		printf("Error reading value... ending.\n");
		exit(EXIT_FAILURE);
	}

	const uint8_t ui8Strings[] = { SETTXT_HUB_NAME, SETTXT_HUB_ADDRESS, SETTXT_ENCODING,
#ifdef _WITH_POSTGRES
		SETTXT_POSTGRES_HOST, SETTXT_POSTGRES_PORT, SETTXT_POSTGRES_DBNAME, SETTXT_POSTGRES_USER, SETTXT_POSTGRES_PASS,
#elif _WITH_MYSQL
		SETTXT_MYSQL_HOST, SETTXT_MYSQL_PORT, SETTXT_MYSQL_DBNAME, SETTXT_MYSQL_USER, SETTXT_MYSQL_PASS,
#endif
	};

	char sValue[4098];

	for(uint8_t ui8i = 0; ui8i < sizeof(ui8Strings); ui8i++) {
value:
		printf("%sActual value is: %s\nEnter new value: ", SetTxtCom[ui8Strings[ui8i]]+2, m_sTexts[ui8Strings[ui8i]] != NULL ? m_sTexts[ui8Strings[ui8i]] : "");
		if(fgets(sValue, 4098, stdin) != NULL) {
			if(sValue[0] == '\n') {
				continue;
			}

			char * sMatch = strchr(sValue, '\n');
			if(sMatch != NULL) {
				sMatch[0] = '\0';
			}
	
			size_t szLen = strlen(sValue);
			SetText(ui8Strings[ui8i], sValue, szLen);
	
			if((szLen == 0 && m_sTexts[ui8Strings[ui8i]] != NULL) || strcmp(sValue, m_sTexts[ui8Strings[ui8i]]) != 0) {
				printf("Failed to set new string value. Incorrect length or invalid characters?\n");
	
				if(WantAgain() == false) {
					return;
				}
	
				goto value;
			}
		} else {
			printf("Error reading string value... ending.\n");
			exit(EXIT_FAILURE);
		}
	}
}
//---------------------------------------------------------------------------

void SettingManager::CmdLineCompleteSetup() {
	m_bUpdateLocked = true;

	printf("\nWelcome to complete setup.\nYou will now be asked for all PtokaX settings.\nWhen you don't want to change default settings, then simply press enter.\n\nFirst we set boolean settings. Use 1 for enabled and 0 for disabled.\n\n");

	char sValue[4098];

    for(size_t szi = 0; szi < SETBOOL_IDS_END; szi++) {
        // skip obsolete settings
        if(SetBoolStr[szi][0] == '\0') {
            continue;
        }
booleanstart:
		printf("%sActual value is: %c\nEnter new value: ", SetBoolCom[szi]+2, m_bBools[szi] == true ? '1' : '0');
		if(fgets(sValue, 3, stdin) != NULL) {
			if(sValue[0] == '\n') {
				continue;
			}

			if(sValue[0] != '0' && sValue[0] != '1') {
				printf("You need to use 1 or 0 for new value!\n");

				if(WantAgain() == false) {
					return;
				}
	
				goto booleanstart;
			}

			SetBool(szi, sValue[0] == '0' ? false : true);
		} else {
			printf("Error reading boolean value... ending.\n");
			exit(EXIT_FAILURE);
		}
    }

	printf("\nWe finished boolean settings. Now we will set number settings.\n\n");

	int16_t i16Value = 0;

    for(size_t szi = 0; szi < (SETSHORT_IDS_END-1); szi++) {
numberstart:
		printf("%sActual value is: %hd\nEnter new value: ", SetShortCom[szi]+2, m_i16Shorts[szi]);
		if(fgets(sValue, 7, stdin) != NULL) {
			if(sValue[0] == '\n') {
				continue;
			}

			char * sMatch = strchr(sValue, '\n');
			if(sMatch != NULL) {
				sMatch[0] = '\0';
			}

			uint8_t ui8Len = (uint8_t)strlen(sValue);

			for(uint8_t ui8i = 0; ui8i < ui8Len; ui8i++) {
				if(isdigit(sValue[ui8i]) == 0) {
					printf("Character '%c' is not valid number!\n", sValue[ui8i]);
	
					if(WantAgain() == false) {
						return;
					}
	
					goto numberstart;
				}
			}

			i16Value = (int16_t)atoi(sValue);
			SetShort(szi, i16Value);
	
			if(i16Value != m_i16Shorts[szi]) {
				printf("Failed to set value %hd!\n", i16Value);
	
				if(WantAgain() == false) {
					return;
				}
	
				goto numberstart;
			}
		} else {
			printf("Error reading number value... ending.\n");
			exit(EXIT_FAILURE);
		}
    }

	printf("\nWe finished number settings. Now we will set string settings.\n\n");

    for(size_t szi = 0; szi < SETTXT_IDS_END; szi++) {
stringstart:
		printf("%sActual value is: %s\nEnter new value: ", SetTxtCom[szi]+2, m_sTexts[szi] != NULL ? m_sTexts[szi] : "");
		if(fgets(sValue, 4098, stdin) != NULL) {
			if(sValue[0] == '\n') {
				continue;
			}

			char * sMatch = strchr(sValue, '\n');
			if(sMatch != NULL) {
				sMatch[0] = '\0';
			}

			size_t szLen = strlen(sValue);
			SetText(szi, sValue, szLen);
	
			if((szLen == 0 && m_sTexts[szi] != NULL) || strcmp(sValue, m_sTexts[szi]) != 0) {
				printf("Failed to set new string value. Incorrect length or invalid characters?\n");
	
				if(WantAgain() == false) {
					return;
				}
	
				goto stringstart;
			}
		} else {
			printf("Error reading string value... ending.\n");
			exit(EXIT_FAILURE);
		}
    }
}
//---------------------------------------------------------------------------
