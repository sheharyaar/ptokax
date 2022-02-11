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
#include "TextFileManager.h"
//---------------------------------------------------------------------------
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
TextFilesManager * TextFilesManager::m_Ptr = NULL;
//---------------------------------------------------------------------------

TextFilesManager::TextFile::TextFile() : m_pPrev(NULL), m_pNext(NULL), m_sCommand(NULL), m_sText(NULL) {
    // ...
}
//---------------------------------------------------------------------------

TextFilesManager::TextFile::~TextFile() {
#ifdef _WIN32
    if(m_sCommand != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sCommand) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_sCommand in TextFilesManager::TextFile::~TextFile\n");
        }
    }
#else
	free(m_sCommand);
#endif

#ifdef _WIN32
    if(m_sText != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sText) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_sText in TextFilesManager::TextFile::~TextFile\n");
        }
    }
#else
	free(m_sText);
#endif
}
//---------------------------------------------------------------------------

TextFilesManager::TextFilesManager() : m_pTextFiles(NULL) {
    // ...
}
//---------------------------------------------------------------------------
	
TextFilesManager::~TextFilesManager() {
    TextFile * cur = NULL,
        * next = m_pTextFiles;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

    	delete cur;
    }
}
//---------------------------------------------------------------------------

bool TextFilesManager::ProcessTextFilesCmd(User * pUser, char * sCommand, const bool bFromPM/* = false*/) const {
    TextFile * cur = NULL,
        * next = m_pTextFiles;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

		if(strcasecmp(cur->m_sCommand, sCommand) == 0) {
            bool bInPM = (SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_TEXT_FILES_AS_PM] == true || bFromPM);
            size_t szHubSecLen = (size_t)SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_HUB_SEC];
            size_t szChatLen = 0;

            // PPK ... to chat or to PM ???
            if(bInPM == true) {
                szChatLen = 18+pUser->m_ui8NickLen+(2*szHubSecLen)+strlen(cur->m_sText);
            } else {
                szChatLen = 4+szHubSecLen+strlen(cur->m_sText);
            }

#ifdef _WIN32
            char * sMSG = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szChatLen);
#else
			char * sMSG = (char *)malloc(szChatLen);
#endif
            if(sMSG == NULL) {
        		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for sMsg in TextFilesManager::ProcessTextFilesCmd\n", szChatLen);

                return true;
            }

            if(bInPM == true) {
                int iRet = snprintf(sMSG, szChatLen, "$To: %s From: %s $<%s> %s", pUser->m_sNick, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], cur->m_sText);
                if(iRet <= 0) {
                    free(sMSG);
                    return true;
                }
            } else {
                int iRet = snprintf(sMSG, szChatLen, "<%s> %s", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], cur->m_sText);
                if(iRet <= 0) {
                    free(sMSG);
                    return true;
                }
            }

            pUser->SendCharDelayed(sMSG, szChatLen-1);

#ifdef _WIN32
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sMSG) == 0) {
        		AppendDebugLog("%s - [MEM] Cannot deallocate sMSG in TextFilesManager::ProcessTextFilesCmd\n");
            }
#else
			free(sMSG);
#endif

        	return true;
        }
    }

    return false;
}
//---------------------------------------------------------------------------

void TextFilesManager::RefreshTextFiles() {
	if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_TEXT_FILES] == false)
        return;

    TextFile * cur = NULL,
        * next = m_pTextFiles;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

    	delete cur;
    }

	m_pTextFiles = NULL;

#ifdef _WIN32
    struct _finddata_t textfile;
    intptr_t hFile = _findfirst((ServerManager::m_sPath+"\\texts\\*.txt").c_str(), &textfile);

	if(hFile != -1) {
		do {
			if((textfile.attrib & _A_SUBDIR) != 0 ||
				stricmp(textfile.name+(strlen(textfile.name)-4), ".txt") != 0) {
				continue;
			}

        	FILE *f = fopen((ServerManager::m_sPath+"\\texts\\"+textfile.name).c_str(), "rb");
			if(f != NULL) {
				if(textfile.size != 0) {
					TextFile * pNewTxtFile = new (std::nothrow) TextFile();
					if(pNewTxtFile == NULL) {
						AppendDebugLog("%s - [MEM] Cannot allocate pNewTxtFile in TextFilesManager::RefreshTextFiles\n");

						fclose(f);
						_findclose(hFile);

                        return;
                    }

					pNewTxtFile->m_sText = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, textfile.size+2);

					if(pNewTxtFile->m_sText == NULL) {
						AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu64 " bytes for m_sText in TextFilesManager::RefreshTextFiles\n", (uint64_t)(textfile.size+2));

						fclose(f);
						_findclose(hFile);

                        delete pNewTxtFile;

						return;
 					}

					size_t size = fread(pNewTxtFile->m_sText, 1, textfile.size, f);

					pNewTxtFile->m_sText[size] = '|';
					pNewTxtFile->m_sText[size+1] = '\0';

					size_t szFileName = strlen(textfile.name)-4;
					pNewTxtFile->m_sCommand = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szFileName+1);
					if(pNewTxtFile->m_sCommand == NULL) {
						AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sCommand in TextFilesManager::RefreshTextFiles\n", szFileName+1);

						fclose(f);
						_findclose(hFile);

                        delete pNewTxtFile;

						return;
					}

					memcpy(pNewTxtFile->m_sCommand, textfile.name, szFileName);
					pNewTxtFile->m_sCommand[szFileName] = '\0';

					pNewTxtFile->m_pPrev = NULL;

					if(m_pTextFiles == NULL) {
						pNewTxtFile->m_pNext = NULL;
					} else {
						m_pTextFiles->m_pPrev = pNewTxtFile;
						pNewTxtFile->m_pNext = m_pTextFiles;
					}

					m_pTextFiles = pNewTxtFile;
				}

				fclose(f);
			}
	    } while(_findnext(hFile, &textfile) == 0);

		_findclose(hFile);
    }
#else
    string txtdir = ServerManager::m_sPath + "/texts/";

    DIR * p_txtdir = opendir(txtdir.c_str());

    if(p_txtdir == NULL) {
        return;
    }

    struct dirent * p_dirent;
    struct stat s_buf;

    while((p_dirent = readdir(p_txtdir)) != NULL) {
        string txtfile = txtdir + p_dirent->d_name;

        if(stat(txtfile.c_str(), &s_buf) != 0 || 
            (s_buf.st_mode & S_IFDIR) != 0 || 
            strcasecmp(p_dirent->d_name + (strlen(p_dirent->d_name)-4), ".txt") != 0) {
            continue;
        }

        FILE *f = fopen(txtfile.c_str(), "rb");
		if(f != NULL) {
			if(s_buf.st_size != 0) {
                TextFile * pNewTxtFile = new (std::nothrow) TextFile();
				if(pNewTxtFile == NULL) {
					AppendDebugLog("%s - [MEM] Cannot allocate pNewTxtFile in TextFilesManager::RefreshTextFiles1\n");

					fclose(f);
					closedir(p_txtdir);

                    return;
                }

				pNewTxtFile->m_sText = (char *)malloc(s_buf.st_size+2);
				if(pNewTxtFile->m_sText == NULL) {
					AppendDebugLogFormat("[MEM] Cannot allocate %" PRIu64 " bytes for m_sText in TextFilesManager::RefreshTextFiles\n", (uint64_t)(s_buf.st_size+2));

					fclose(f);
					closedir(p_txtdir);

                    delete pNewTxtFile;

                    return;
                }
    	        size_t size = fread(pNewTxtFile->m_sText, 1, s_buf.st_size, f);
				pNewTxtFile->m_sText[size] = '|';
                pNewTxtFile->m_sText[size+1] = '\0';

				size_t szFileName = strlen(p_dirent->d_name)-4;
				pNewTxtFile->m_sCommand = (char *)malloc(szFileName+1);
				if(pNewTxtFile->m_sCommand == NULL) {
					AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for m_sCommand in TextFilesManager::RefreshTextFiles\n", szFileName+1);

					fclose(f);
					closedir(p_txtdir);

                    delete pNewTxtFile;

                    return;
                }

                memcpy(pNewTxtFile->m_sCommand, p_dirent->d_name, szFileName);
                pNewTxtFile->m_sCommand[szFileName] = '\0';

                pNewTxtFile->m_pPrev = NULL;

                if(m_pTextFiles == NULL) {
                    pNewTxtFile->m_pNext = NULL;
                } else {
                    m_pTextFiles->m_pPrev = pNewTxtFile;
                    pNewTxtFile->m_pNext = m_pTextFiles;
                }

				m_pTextFiles = pNewTxtFile;
    	    }

            fclose(f);
    	}
    }

    closedir(p_txtdir);
#endif
}
//---------------------------------------------------------------------------
