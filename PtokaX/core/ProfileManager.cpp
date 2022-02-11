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
#include "ProfileManager.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "hashRegManager.h"
#include "LanguageManager.h"
#include "PXBReader.h"
#include "ServerManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
    #include "../gui.win/ProfilesDialog.h"
    #include "../gui.win/RegisteredUserDialog.h"
    #include "../gui.win/RegisteredUsersDialog.h"
#endif
//---------------------------------------------------------------------------
ProfileManager * ProfileManager::m_Ptr = NULL;
//---------------------------------------------------------------------------
static const char sPtokaXProfiles[] = "PtokaX Profiles";
static const size_t szPtokaXProfilesLen = sizeof(sPtokaXProfiles)-1;
static const char sProfilePermissionIds[] = // PN reserved for profile name!
		"OP" // HASKEYICON
		"DG" // NODEFLOODGETNICKLIST
		"DM" // NODEFLOODMYINFO
		"DS" // NODEFLOODSEARCH
		"DP" // NODEFLOODPM
		"DN" // NODEFLOODMAINCHAT
		"MM" // MASSMSG
		"TO" // TOPIC
		"TB" // TEMP_BAN
		"RT" // REFRESHTXT
		"NT" // NOTAGCHECK
		"TU" // TEMP_UNBAN
		"DR" // DELREGUSER
		"AR" // ADDREGUSER
		"NC" // NOCHATLIMITS
		"NH" // NOMAXHUBCHECK
		"NR" // NOSLOTHUBRATIO
		"NS" // NOSLOTCHECK
		"NA" // NOSHARELIMIT
		"CP" // CLRPERMBAN
		"CT" // CLRTEMPBAN
		"GI" // GETINFO
		"GB" // GETBANLIST
		"RS" // RSTSCRIPTS
		"RH" // RSTHUB
		"TP" // TEMPOP
		"GG" // GAG
		"RE" // REDIRECT
		"BN" // BAN
		"KI" // KICK
		"DR" // DROP
		"EF" // ENTERFULLHUB
		"EB" // ENTERIFIPBAN
		"AO" // ALLOWEDOPCHAT
		"UI" // SENDALLUSERIP
		"RB" // RANGE_BAN
		"RU" // RANGE_UNBAN
		"RT" // RANGE_TBAN
		"RV" // RANGE_TUNBAN
		"GR" // GET_RANGE_BANS
		"CR" // CLR_RANGE_BANS
		"CU" // CLR_RANGE_TBANS
		"UN" // UNBAN
		"NT" // NOSEARCHLIMITS
		"SM" // SENDFULLMYINFOS
		"NI" // NOIPCHECK
		"CL" // CLOSE
		"DC" // NODEFLOODCTM
		"DR" // NODEFLOODRCTM
		"DT" // NODEFLOODSR
		"DU" // NODEFLOODRECV
		"CI" // NOCHATINTERVAL
		"PI" // NOPMINTERVAL
		"SI" // NOSEARCHINTERVAL
		"UI" // NOUSRSAMEIP
		"RT" // NORECONNTIME
;
static const size_t szProfilePermissionIdsLen = sizeof(sProfilePermissionIds)-1;
//---------------------------------------------------------------------------

ProfileItem::ProfileItem() : m_sName(NULL) {
	for(uint16_t ui16i = 0; ui16i < 256; ui16i++) {
        m_bPermissions[ui16i] = false;
    }
}
//---------------------------------------------------------------------------

ProfileItem::~ProfileItem() {
#ifdef _WIN32
    if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sName) == 0) {
        AppendDebugLog("%s - [MEM] Cannot deallocate m_sName in ProfileItem::~ProfileItem\n");
    }
#else
	free(m_sName);
#endif
}
//---------------------------------------------------------------------------
void ProfileManager::Load() {
    PXBReader pxbProfiles;

    // Open profiles file
#ifdef _WIN32
    if(pxbProfiles.OpenFileRead((ServerManager::m_sPath + "\\cfg\\Profiles.pxb").c_str(), NORECONNTIME+2) == false) {
#else
    if(pxbProfiles.OpenFileRead((ServerManager::m_sPath + "/cfg/Profiles.pxb").c_str(), NORECONNTIME+2) == false) {
#endif
    	AppendDebugLog("%s - [ERR] Cannot open Profiles.pxb in ProfileManager::Load\n");
        return;
    }

    // Read file header
    uint16_t ui16Identificators[NORECONNTIME+2];
    ui16Identificators[0] = *((uint16_t *)"FI");
    ui16Identificators[1] = *((uint16_t *)"FV");

    if(pxbProfiles.ReadNextItem(ui16Identificators, 2) == false) {
        return;
    }

    // Check header if we have correct file
    if(pxbProfiles.m_ui16ItemLengths[0] != szPtokaXProfilesLen || strncmp((char *)pxbProfiles.m_pItemDatas[0], sPtokaXProfiles, szPtokaXProfilesLen) != 0) {
        return;
    }

    {
        uint32_t ui32FileVersion = ntohl(*((uint32_t *)(pxbProfiles.m_pItemDatas[1])));

        if(ui32FileVersion < 1) {
            return;
        }
    }

    // Read settings =)
    ui16Identificators[0] = *((uint16_t *)"PN");
    memcpy(ui16Identificators+1, sProfilePermissionIds, szProfilePermissionIdsLen);

    bool bSuccess = pxbProfiles.ReadNextItem(ui16Identificators, NORECONNTIME+2);

    while(bSuccess == true) {
    	ProfileItem * pNewProfile = CreateProfile((char *)pxbProfiles.m_pItemDatas[0]);

        for(uint16_t ui16i = 0; ui16i <= NORECONNTIME; ui16i++) {
			if(((char *)pxbProfiles.m_pItemDatas[ui16i+1])[0] == '0') {
				pNewProfile->m_bPermissions[ui16i] = false;
			} else {
				pNewProfile->m_bPermissions[ui16i] = true;
			}
        }

        bSuccess = pxbProfiles.ReadNextItem(ui16Identificators, NORECONNTIME+2);
    }
}
//---------------------------------------------------------------------------

void ProfileManager::LoadXML() {
#ifdef _WIN32
    TiXmlDocument doc((ServerManager::m_sPath+"\\cfg\\Profiles.xml").c_str());
#else
	TiXmlDocument doc((ServerManager::m_sPath+"/cfg/Profiles.xml").c_str());
#endif
	if(doc.LoadFile() == false) {
         if(doc.ErrorId() != TiXmlBase::TIXML_ERROR_OPENING_FILE && doc.ErrorId() != TiXmlBase::TIXML_ERROR_DOCUMENT_EMPTY) {
            int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "Error loading file Profiles.xml. %s (Col: %d, Row: %d)", doc.ErrorDesc(), doc.Column(), doc.Row());
			if(iMsgLen > 0) {
#ifdef _BUILD_GUI
				::MessageBox(NULL, ServerManager::m_pGlobalBuffer, g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
				AppendLog(ServerManager::m_pGlobalBuffer);
#endif
			}

            exit(EXIT_FAILURE);
        }
	}

	TiXmlHandle cfg(&doc);
	TiXmlNode *profiles = cfg.FirstChild("Profiles").Node();
	if(profiles != NULL) {
		TiXmlNode *child = NULL;
		while((child = profiles->IterateChildren(child)) != NULL) {
			TiXmlNode *profile = child->FirstChild("Name");

			if(profile == NULL || (profile = profile->FirstChild()) == NULL) {
				continue;
			}

			const char *sName = profile->Value();

			if((profile = child->FirstChildElement("Permissions")) == NULL ||
				(profile = profile->FirstChild()) == NULL) {
				continue;
			}

			const char *sRights = profile->Value();

			ProfileItem * pNewProfile = CreateProfile(sName);

			if(strlen(sRights) == 32) {
				for(uint8_t ui8i = 0; ui8i < 32; ui8i++) {
					if(sRights[ui8i] == '1') {
						pNewProfile->m_bPermissions[ui8i] = true;
					} else {
						pNewProfile->m_bPermissions[ui8i] = false;
					}
				}
			} else if(strlen(sRights) == 256) {
				for(uint16_t ui8i = 0; ui8i < 256; ui8i++) {
					if(sRights[ui8i] == '1') {
						pNewProfile->m_bPermissions[ui8i] = true;
					} else {
						pNewProfile->m_bPermissions[ui8i] = false;
					}
				}
			} else {
				delete pNewProfile;
				continue;
			}
		}
	} else {
#ifdef _BUILD_GUI
		::MessageBox(NULL, LanguageManager::m_Ptr->m_sTexts[LAN_PROFILES_LOAD_FAIL], g_sPtokaXTitle, MB_OK | MB_ICONERROR);
#else
		AppendLog(LanguageManager::m_Ptr->m_sTexts[LAN_PROFILES_LOAD_FAIL]);
#endif
		exit(EXIT_FAILURE);
	}
}
//---------------------------------------------------------------------------

ProfileManager::ProfileManager() : m_ppProfilesTable(NULL), m_ui16ProfileCount(0) {
	#ifdef _WIN32
    if(FileExist((ServerManager::m_sPath + "\\cfg\\Profiles.pxb").c_str()) == true) {
#else
    if(FileExist((ServerManager::m_sPath + "/cfg/Profiles.pxb").c_str()) == true) {
#endif
        Load();
        return;
#ifdef _WIN32
    } else if(FileExist((ServerManager::m_sPath + "\\cfg\\Profiles.xml").c_str()) == true) {
#else
    } else if(FileExist((ServerManager::m_sPath + "/cfg/Profiles.xml").c_str()) == true) {
#endif
        LoadXML();
        return;
    } else {
	    const char * sProfileNames[] = { "Master", "Operator", "VIP", "Reg" };
	    const char * sProfilePermisions[] = {
	        "10011111111111111111111111111111111111111111101000111111",
	        "10011111101111111110011000111111111000000011101000111111",
	        "00000000000000011110000000000001100000000000000000000111",
	        "00000000000000000000000000000001100000000000000000000000"
	    };

		for(uint8_t ui8i = 0; ui8i < 4; ui8i++) {
			ProfileItem * pNewProfile = CreateProfile(sProfileNames[ui8i]);

			for(uint8_t ui8j = 0; ui8j < strlen(sProfilePermisions[ui8i]); ui8j++) {
				if(sProfilePermisions[ui8i][ui8j] == '1') {
					pNewProfile->m_bPermissions[ui8j] = true;
				} else {
					pNewProfile->m_bPermissions[ui8j] = false;
				}
			}
		}

	    SaveProfiles();
	}

}
//---------------------------------------------------------------------------

ProfileManager::~ProfileManager() {
    SaveProfiles();

    for(uint16_t ui16i = 0; ui16i < m_ui16ProfileCount; ui16i++) {
        delete m_ppProfilesTable[ui16i];
    }

#ifdef _WIN32
    if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_ppProfilesTable) == 0) {
        AppendDebugLog("%s - [MEM] Cannot deallocate m_ppProfilesTable in ProfileManager::~ProfileManager\n");
    }
#else
	free(m_ppProfilesTable);
#endif
}
//---------------------------------------------------------------------------

void ProfileManager::SaveProfiles() {
    PXBReader pxbProfiles;

    // Open profiles file
#ifdef _WIN32
    if(pxbProfiles.OpenFileSave((ServerManager::m_sPath + "\\cfg\\Profiles.pxb").c_str(), NORECONNTIME+2) == false) {
#else
    if(pxbProfiles.OpenFileSave((ServerManager::m_sPath + "/cfg/Profiles.pxb").c_str(), NORECONNTIME+2) == false) {
#endif
		AppendDebugLog("%s - [ERR] Cannot open Profiles.pxb in ProfileManager::SaveProfiles\n");
        return;
    }

    // Write file header
    pxbProfiles.m_sItemIdentifiers[0] = 'F';
    pxbProfiles.m_sItemIdentifiers[1] = 'I';
    pxbProfiles.m_ui16ItemLengths[0] = (uint16_t)szPtokaXProfilesLen;
    pxbProfiles.m_pItemDatas[0] = (void *)sPtokaXProfiles;
    pxbProfiles.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

    pxbProfiles.m_sItemIdentifiers[2] = 'F';
    pxbProfiles.m_sItemIdentifiers[3] = 'V';
    pxbProfiles.m_ui16ItemLengths[1] = 4;
    uint32_t ui32Version = 1;
    pxbProfiles.m_pItemDatas[1] = (void *)&ui32Version;
    pxbProfiles.m_ui8ItemValues[1] = PXBReader::PXB_FOUR_BYTES;

    if(pxbProfiles.WriteNextItem(szPtokaXProfilesLen+4, 2) == false) {
        return;
    }

    pxbProfiles.m_sItemIdentifiers[0] = 'P';
    pxbProfiles.m_sItemIdentifiers[1] = 'N';
    pxbProfiles.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

	memcpy(pxbProfiles.m_sItemIdentifiers+2, sProfilePermissionIds, szProfilePermissionIdsLen);
	memset(pxbProfiles.m_ui8ItemValues+1, PXBReader::PXB_BYTE, NORECONNTIME+1);

    for(uint16_t ui16i = 0; ui16i < m_ui16ProfileCount; ui16i++) {
        pxbProfiles.m_ui16ItemLengths[0] = (uint16_t)strlen(m_ppProfilesTable[ui16i]->m_sName);
        pxbProfiles.m_pItemDatas[0] = (void *)m_ppProfilesTable[ui16i]->m_sName;
        pxbProfiles.m_ui8ItemValues[0] = PXBReader::PXB_STRING;

        for(uint16_t ui16j = 0; ui16j <= NORECONNTIME; ui16j++) {
	        pxbProfiles.m_ui16ItemLengths[ui16j+1] = 1;
	        pxbProfiles.m_pItemDatas[ui16j+1] = (m_ppProfilesTable[ui16i]->m_bPermissions[ui16j] == true ? (void *)1 : 0);
	        pxbProfiles.m_ui8ItemValues[ui16j+1] = PXBReader::PXB_BYTE;
        }

        if(pxbProfiles.WriteNextItem(pxbProfiles.m_ui16ItemLengths[0] + NORECONNTIME+1, NORECONNTIME+2) == false) {
            break;
        }
    }

    pxbProfiles.WriteRemaining();
}
//---------------------------------------------------------------------------

bool ProfileManager::IsAllowed(User * pUser, const uint32_t ui32Option) const {
    // profile number -1 = normal user/no profile assigned
    if(pUser->m_i32Profile == -1)
        return false;
        
    // return right of the profile
    return m_ppProfilesTable[pUser->m_i32Profile]->m_bPermissions[ui32Option];
}
//---------------------------------------------------------------------------

bool ProfileManager::IsProfileAllowed(const int32_t i32Profile, const uint32_t ui32Option) const {
    // profile number -1 = normal user/no profile assigned
    if(i32Profile == -1)
        return false;
        
    // return right of the profile
    return m_ppProfilesTable[i32Profile]->m_bPermissions[ui32Option];
}
//---------------------------------------------------------------------------

int32_t ProfileManager::AddProfile(char * sName) {
    for(uint16_t ui16i = 0; ui16i < m_ui16ProfileCount; ui16i++) {
		if(strcasecmp(m_ppProfilesTable[ui16i]->m_sName, sName) == 0) {
            return -1;
        }
    }

    uint32_t ui32j = 0;
    while(true) {
        switch(sName[ui32j]) {
            case '\0':
                break;
            case '|':
                return -2;
            default:
                if(sName[ui32j] < 33) {
                    return -2;
                }
                
                ui32j++;
                continue;
        }

        break;
    }

    CreateProfile(sName);

#ifdef _BUILD_GUI
    if(ProfilesDialog::m_Ptr != NULL) {
        ProfilesDialog::m_Ptr->AddProfile();
    }
#endif

#ifdef _BUILD_GUI
    if(RegisteredUserDialog::m_Ptr != NULL) {
        RegisteredUserDialog::m_Ptr->UpdateProfiles();
    }
#endif

    return (int32_t)(m_ui16ProfileCount-1);
}
//---------------------------------------------------------------------------

int32_t ProfileManager::GetProfileIndex(const char * sName) {
    for(uint16_t ui16i = 0; ui16i < m_ui16ProfileCount; ui16i++) {
		if(strcasecmp(m_ppProfilesTable[ui16i]->m_sName, sName) == 0) {
            return ui16i;
        }
    }
    
    return -1;
}
//---------------------------------------------------------------------------

// RemoveProfileByName(name)
// returns: 0 if the name doesnot exists or is a default profile idx 0-3
//          -1 if the profile is in use
//          1 on success
int32_t ProfileManager::RemoveProfileByName(char * sName) {
    for(uint16_t ui16i = 0; ui16i < m_ui16ProfileCount; ui16i++) {
		if(strcasecmp(m_ppProfilesTable[ui16i]->m_sName, sName) == 0) {
            return (RemoveProfile(ui16i) == true ? 1 : -1);
        }
    }
    
    return 0;
}

//---------------------------------------------------------------------------

bool ProfileManager::RemoveProfile(const uint16_t ui16Profile) {
    RegUser * curReg = NULL,
        * next = RegManager::m_Ptr->m_pRegListS;

    while(next != NULL) {
        curReg = next;
		next = curReg->m_pNext;

		if(curReg->m_ui16Profile == ui16Profile) {
            //Profile in use can't be deleted!
            return false;
        }
    }
    
    m_ui16ProfileCount--;

#ifdef _BUILD_GUI
    if(ProfilesDialog::m_Ptr != NULL) {
        ProfilesDialog::m_Ptr->RemoveProfile(ui16Profile);
    }
#endif
    
    delete m_ppProfilesTable[ui16Profile];
    
	for(uint16_t ui16i = ui16Profile; ui16i < m_ui16ProfileCount; ui16i++) {
        m_ppProfilesTable[ui16i] = m_ppProfilesTable[ui16i+1];
    }

    // Update profiles for online users
    if(ServerManager::m_bServerRunning == true) {
        User * curUser = NULL,
            * nextUser = Users::m_Ptr->m_pUserListS;

        while(nextUser != NULL) {
            curUser = nextUser;
            nextUser = curUser->m_pNext;
            
            if(curUser->m_i32Profile > ui16Profile) {
                curUser->m_i32Profile--;
            }
        }
    }

    // Update profiles for registered users
    next = RegManager::m_Ptr->m_pRegListS;
    while(next != NULL) {
        curReg = next;
		next = curReg->m_pNext;
        if(curReg->m_ui16Profile > ui16Profile) {
            curReg->m_ui16Profile--;
        }
    }

    ProfileItem ** pOldTable = m_ppProfilesTable;
#ifdef _WIN32
    m_ppProfilesTable = (ProfileItem **)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldTable, m_ui16ProfileCount*sizeof(ProfileItem *));
#else
	m_ppProfilesTable = (ProfileItem **)realloc(pOldTable, m_ui16ProfileCount*sizeof(ProfileItem *));
#endif
    if(m_ppProfilesTable == NULL) {
        m_ppProfilesTable = pOldTable;

		AppendDebugLog("%s - [MEM] Cannot reallocate m_ppProfilesTable in ProfileManager::RemoveProfile\n");
    }

#ifdef _BUILD_GUI
    if(RegisteredUserDialog::m_Ptr != NULL) {
        RegisteredUserDialog::m_Ptr->UpdateProfiles();
    }

	if(RegisteredUsersDialog::m_Ptr != NULL) {
		RegisteredUsersDialog::m_Ptr->UpdateProfiles();
	}
#endif

    return true;
}
//---------------------------------------------------------------------------

ProfileItem * ProfileManager::CreateProfile(const char * sName) {
    ProfileItem ** pOldTable = m_ppProfilesTable;
#ifdef _WIN32
    if(m_ppProfilesTable == NULL) {
        m_ppProfilesTable = (ProfileItem **)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (m_ui16ProfileCount+1)*sizeof(ProfileItem *));
    } else {
        m_ppProfilesTable = (ProfileItem **)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldTable, (m_ui16ProfileCount+1)*sizeof(ProfileItem *));
    }
#else
	m_ppProfilesTable = (ProfileItem **)realloc(pOldTable, (m_ui16ProfileCount+1)*sizeof(ProfileItem *));
#endif
    if(m_ppProfilesTable == NULL) {
#ifdef _WIN32
        HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldTable);
#else
        free(pOldTable);
#endif
		AppendDebugLog("%s - [MEM] Cannot (re)allocate m_ppProfilesTable in ProfileManager::CreateProfile\n");
        exit(EXIT_FAILURE);
    }

    ProfileItem * pNewProfile = new (std::nothrow) ProfileItem();
    if(pNewProfile == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pNewProfile in ProfileManager::CreateProfile\n");
        exit(EXIT_FAILURE);
    }
 
    size_t szLen = strlen(sName);
#ifdef _WIN32
    pNewProfile->m_sName = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
	pNewProfile->m_sName = (char *)malloc(szLen+1);
#endif
    if(pNewProfile->m_sName == NULL) {
		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes in ProfileManager::CreateProfile for pNewProfile->sName\n", szLen);

        exit(EXIT_FAILURE);
    } 
    memcpy(pNewProfile->m_sName, sName, szLen);
    pNewProfile->m_sName[szLen] = '\0';

    for(uint16_t ui16i = 0; ui16i < 256; ui16i++) {
        pNewProfile->m_bPermissions[ui16i] = false;
    }
    
    m_ui16ProfileCount++;

    m_ppProfilesTable[m_ui16ProfileCount-1] = pNewProfile;

    return pNewProfile;
}
//---------------------------------------------------------------------------

void ProfileManager::MoveProfileDown(const uint16_t ui16Profile) {
    ProfileItem *first = m_ppProfilesTable[ui16Profile];
    ProfileItem *second = m_ppProfilesTable[ui16Profile+1];
    
	m_ppProfilesTable[ui16Profile+1] = first;
	m_ppProfilesTable[ui16Profile] = second;

    RegUser * curReg = NULL,
        * nextReg = RegManager::m_Ptr->m_pRegListS;

	while(nextReg != NULL) {
        curReg = nextReg;
		nextReg = curReg->m_pNext;

		if(curReg->m_ui16Profile == ui16Profile) {
			curReg->m_ui16Profile++;
		} else if(curReg->m_ui16Profile == ui16Profile+1) {
			curReg->m_ui16Profile--;
		}
	}

#ifdef _BUILD_GUI
    if(ProfilesDialog::m_Ptr != NULL) {
        ProfilesDialog::m_Ptr->MoveDown(ui16Profile);
    }

    if(RegisteredUserDialog::m_Ptr != NULL) {
        RegisteredUserDialog::m_Ptr->UpdateProfiles();
    }

	if(RegisteredUsersDialog::m_Ptr != NULL) {
		RegisteredUsersDialog::m_Ptr->UpdateProfiles();
	}
#endif

    if(Users::m_Ptr == NULL) {
        return;
    }

    User * curUser = NULL,
        * nextUser = Users::m_Ptr->m_pUserListS;

	while(nextUser != NULL) {
        curUser = nextUser;
		nextUser = curUser->m_pNext;

		if(curUser->m_i32Profile == (int32_t)ui16Profile) {
			curUser->m_i32Profile++;
		} else if(curUser->m_i32Profile == (int32_t)(ui16Profile+1)) {
			curUser->m_i32Profile--;
		}
    }
}
//---------------------------------------------------------------------------

void ProfileManager::MoveProfileUp(const uint16_t ui16Profile) {
    ProfileItem *first = m_ppProfilesTable[ui16Profile];
    ProfileItem *second = m_ppProfilesTable[ui16Profile-1];
    
	m_ppProfilesTable[ui16Profile-1] = first;
	m_ppProfilesTable[ui16Profile] = second;

	RegUser * curReg = NULL,
        * nextReg = RegManager::m_Ptr->m_pRegListS;

	while(nextReg != NULL) {
		curReg = nextReg;
		nextReg = curReg->m_pNext;

        if(curReg->m_ui16Profile == ui16Profile) {
			curReg->m_ui16Profile--;
		} else if(curReg->m_ui16Profile == ui16Profile-1) {
			curReg->m_ui16Profile++;
		}
	}

#ifdef _BUILD_GUI
    if(ProfilesDialog::m_Ptr != NULL) {
        ProfilesDialog::m_Ptr->MoveUp(ui16Profile);
    }

    if(RegisteredUserDialog::m_Ptr != NULL) {
        RegisteredUserDialog::m_Ptr->UpdateProfiles();
    }

	if(RegisteredUsersDialog::m_Ptr != NULL) {
		RegisteredUsersDialog::m_Ptr->UpdateProfiles();
	}
#endif

    if(Users::m_Ptr == NULL) {
        return;
    }

    User * curUser = NULL,
        * nextUser = Users::m_Ptr->m_pUserListS;

    while(nextUser != NULL) {
        curUser = nextUser;
		nextUser = curUser->m_pNext;

		if(curUser->m_i32Profile == (int32_t)ui16Profile) {
			curUser->m_i32Profile--;
		} else if(curUser->m_i32Profile == (int32_t)(ui16Profile-1)) {
			curUser->m_i32Profile++;
		}
    }
}
//---------------------------------------------------------------------------

void ProfileManager::ChangeProfileName(const uint16_t ui16Profile, char * sName, const size_t szLen) {
    char * sOldName = m_ppProfilesTable[ui16Profile]->m_sName;

#ifdef _WIN32
    m_ppProfilesTable[ui16Profile]->m_sName = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldName, szLen+1);
#else
	m_ppProfilesTable[ui16Profile]->m_sName = (char *)realloc(sOldName, szLen+1);
#endif
    if(m_ppProfilesTable[ui16Profile]->m_sName == NULL) {
        m_ppProfilesTable[ui16Profile]->m_sName = sOldName;

		AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes in ProfileManager::ChangeProfileName for m_ppProfilesTable[ui16Profile]->sName\n", szLen);

        return;
    } 
	memcpy(m_ppProfilesTable[ui16Profile]->m_sName, sName, szLen);
    m_ppProfilesTable[ui16Profile]->m_sName[szLen] = '\0';

#ifdef _BUILD_GUI
    if(RegisteredUserDialog::m_Ptr != NULL) {
        RegisteredUserDialog::m_Ptr->UpdateProfiles();
    }

	if(RegisteredUsersDialog::m_Ptr != NULL) {
		RegisteredUsersDialog::m_Ptr->UpdateProfiles();
	}
#endif
}
//---------------------------------------------------------------------------

void ProfileManager::ChangeProfilePermission(const uint16_t ui16Profile, const size_t szId, const bool bValue) {
    m_ppProfilesTable[ui16Profile]->m_bPermissions[szId] = bValue;
}
//---------------------------------------------------------------------------
