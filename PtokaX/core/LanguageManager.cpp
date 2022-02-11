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
#include "LanguageXml.h"
#include "LanguageStrings.h"
#include "LanguageManager.h"
//---------------------------------------------------------------------------
#include "SettingManager.h"
#include "ServerManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
LanguageManager * LanguageManager::m_Ptr = NULL;
//---------------------------------------------------------------------------

LanguageManager::LanguageManager(void) {
    for(size_t szi = 0; szi < LANG_IDS_END; szi++) {
        size_t szTextLen = strlen(LangStr[szi]);
#ifdef _WIN32
        m_sTexts[szi] = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szTextLen+1);
#else
		m_sTexts[szi] = (char *)malloc(szTextLen+1);
#endif
        if(m_sTexts[szi] == NULL) {
            AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes in LanguageManager::LanguageManager\n", szTextLen+1);

            exit(EXIT_FAILURE);
        }
        memcpy(m_sTexts[szi], LangStr[szi], szTextLen);
		m_ui16TextsLens[szi] = (uint16_t)szTextLen;
		m_sTexts[szi][m_ui16TextsLens[szi]] = '\0';
    }
}
//---------------------------------------------------------------------------

LanguageManager::~LanguageManager(void) {
    for(size_t szi = 0; szi < LANG_IDS_END; szi++) {
#ifdef _WIN32
        if(m_sTexts[szi] != NULL && HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sTexts[szi]) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_sTexts[szi] in LanguageManager::~LanguageManager\n");
        }
#else
		free(m_sTexts[szi]);
#endif
    }
}
//---------------------------------------------------------------------------

void LanguageManager::Load() {
    if(SettingManager::m_Ptr->m_sTexts[SETTXT_LANGUAGE] == NULL) {
        for(size_t szi = 0; szi < LANG_IDS_END; szi++) {
            char * sOldText = m_sTexts[szi];

            size_t szTextLen = strlen(LangStr[szi]);
#ifdef _WIN32
			m_sTexts[szi] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldText, szTextLen+1);
#else
			m_sTexts[szi] = (char *)realloc(sOldText, szTextLen+1);
#endif
            if(m_sTexts[szi] == NULL) {
				m_sTexts[szi] = sOldText;

				AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes in LanguageManager::Load\n", szTextLen+1);

                continue;
            }

            memcpy(m_sTexts[szi], LangStr[szi], szTextLen);
			m_ui16TextsLens[szi] = (uint16_t)szTextLen;
			m_sTexts[szi][m_ui16TextsLens[szi]] = '\0';
        }
    } else {
#ifdef _WIN32
		string sLanguageFile = ServerManager::m_sPath+"\\language\\"+string(SettingManager::m_Ptr->m_sTexts[SETTXT_LANGUAGE],
#else
		string sLanguageFile = ServerManager::m_sPath+"/language/"+string(SettingManager::m_Ptr->m_sTexts[SETTXT_LANGUAGE],
#endif
            (size_t)SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_LANGUAGE])+".xml";

        TiXmlDocument doc(sLanguageFile.c_str());
        if(doc.LoadFile() == false) {
            if(doc.ErrorId() != TiXmlBase::TIXML_ERROR_OPENING_FILE && doc.ErrorId() != TiXmlBase::TIXML_ERROR_DOCUMENT_EMPTY) {
                int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "Error loading file %s.xml. %s (Col: %d, Row: %d)", SettingManager::m_Ptr->m_sTexts[SETTXT_LANGUAGE], doc.ErrorDesc(), doc.Column(), doc.Row());
                if(iMsgLen > 0) {
#ifdef _BUILD_GUI
                	::MessageBox(NULL, ServerManager::m_pGlobalBuffer, g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
                	AppendLog(ServerManager::m_pGlobalBuffer);
#endif
				}
            }
        } else {
            TiXmlHandle cfg(&doc);
            TiXmlNode *language = cfg.FirstChild("Language").Node();
            if(language != NULL) {
                TiXmlNode *text = NULL;
                while((text = language->IterateChildren(text)) != NULL) {
                    if(text->ToElement() == NULL) {
                        continue;
                    }

                    const char * sName = text->ToElement()->Attribute("Name");
                    const char * sText = text->ToElement()->GetText();
                    size_t szLen = (sText != NULL ? strlen(sText) : 0);
                    if(szLen != 0 && szLen < 129) {
                        for(size_t szi = 0; szi < LANG_IDS_END; szi++) {
                            if(strcmp(LangXmlStr[szi], sName) == 0) {
                                char * sOldText = m_sTexts[szi];
#ifdef _WIN32
								m_sTexts[szi] = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldText, szLen+1);
#else
								m_sTexts[szi] = (char *)realloc(sOldText, szLen+1);
#endif
                                if(m_sTexts[szi] == NULL) {
									m_sTexts[szi] = sOldText;

									AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes in LanguageManager::Load1\n", szLen+1);

                                    break;
                                }

                                memcpy(m_sTexts[szi], sText, szLen);
								m_ui16TextsLens[szi] = (uint16_t)szLen;
								m_sTexts[szi][m_ui16TextsLens[szi]] = '\0';
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}
//---------------------------------------------------------------------------

void LanguageManager::GenerateXmlExample() {
    TiXmlDocument xmldoc;
    xmldoc.InsertEndChild(TiXmlDeclaration("1.0", "windows-1252", "yes"));
    TiXmlElement xmllanguage("Language");
    xmllanguage.SetAttribute("Name", "Example English Language");
    xmllanguage.SetAttribute("Author", "PtokaX");
    xmllanguage.SetAttribute("Version", PtokaXVersionString " build " BUILD_NUMBER);

    for(int i = 0; i < LANG_IDS_END; i++) {
        TiXmlElement xmlstring("String");
        xmlstring.SetAttribute("Name", LangXmlStr[i]);
        xmlstring.InsertEndChild(TiXmlText(LangStr[i]));
        xmllanguage.InsertEndChild(xmlstring);
    }

    xmldoc.InsertEndChild(xmllanguage);
    xmldoc.SaveFile("English.xml.example");
}
//---------------------------------------------------------------------------
