/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2017 Petr Kozelka, PPK at PtokaX dot org

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

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "stdinc.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "DB-SQLite.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "HubCommands.h"
#include "IP2Country.h"
#include "LanguageManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "TextConverter.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include <sqlite3.h>
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
DBSQLite * DBSQLite::m_Ptr = NULL;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

DBSQLite::DBSQLite() : m_bConnected(false) {
	if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_DATABASE] == false) {
		return;
	}

#ifdef _WIN32
	int iRet = sqlite3_open((ServerManager::m_sPath + "\\cfg\\users.sqlite").c_str(), &m_pSqliteDB);
#else
	int iRet = sqlite3_open((ServerManager::m_sPath + "/cfg/users.sqlite").c_str(), &m_pSqliteDB);
#endif
	if(iRet != SQLITE_OK) {
		AppendLog((string("DBSQLite connection failed: ")+sqlite3_errmsg(m_pSqliteDB)).c_str());
		sqlite3_close(m_pSqliteDB);

		return;
	}

	char * sErrMsg = NULL;

	iRet = sqlite3_exec(m_pSqliteDB, "PRAGMA synchronous = NORMAL;"
		"PRAGMA journal_mode = WAL;",
		NULL, NULL, &sErrMsg);

	if(iRet != SQLITE_OK) {
		AppendLog((string("DBSQLite PRAGMA set failed: ")+sErrMsg).c_str());
		sqlite3_free(sErrMsg);
		sqlite3_close(m_pSqliteDB);

		return;
	}

	iRet = sqlite3_exec(m_pSqliteDB, 
		"CREATE TABLE IF NOT EXISTS userinfo ("
			"nick VARCHAR(64) NOT NULL PRIMARY KEY,"
			"last_updated DATETIME NOT NULL,"
			"ip_address VARCHAR(39) NOT NULL,"
			"share VARCHAR(24) NOT NULL,"
			"description VARCHAR(192),"
			"tag VARCHAR(192),"
			"connection VARCHAR(32),"
			"email VARCHAR(96),"
			"UNIQUE (nick COLLATE NOCASE)"
		");", NULL, NULL, &sErrMsg);

	if(iRet != SQLITE_OK) {
		AppendLog((string("DBSQLite check/create table failed: ")+sErrMsg).c_str());
		sqlite3_free(sErrMsg);
		sqlite3_close(m_pSqliteDB);

		return;
	}

	m_bConnected = true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
DBSQLite::~DBSQLite() {
	// When user don't want to save data in database forever then he can set to remove records older than X days.
	if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DB_REMOVE_OLD_RECORDS] != 0) {
		RemoveOldRecords(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DB_REMOVE_OLD_RECORDS]);
	}

	if(m_bConnected == true) {
		sqlite3_close(m_pSqliteDB);
	}

	sqlite3_shutdown();
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Now that important part. Function to update or insert user to database.
void DBSQLite::UpdateRecord(User * pUser) {
	if(m_bConnected == false) {
		return;
	}

	char sNick[65];
	if(TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sNick, pUser->m_ui8NickLen, sNick, 65) == 0) {
		return;
	}

	char sShare[24];
	if(snprintf(sShare, 24, "%0.02f GB", (double)pUser->m_ui64SharedSize/1073741824) <= 0) {
		return;
	}

	char sDescription[193];
	sDescription[0] = '\0';

	if(pUser->m_sDescription != NULL) {
		TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sDescription, pUser->m_ui8DescriptionLen, sDescription, 193);
	}

	char sTag[193];
	sTag[0] = '\0';

	if(pUser->m_sTag != NULL) {
		TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sTag, pUser->m_ui8TagLen, sTag, 193);
	}

	char sConnection[33];
	sConnection[0] = '\0';

	if(pUser->m_sConnection != NULL) {
		TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sConnection, pUser->m_ui8ConnectionLen, sConnection, 33);
	}

	char sEmail[97];
	sEmail[0] = '\0';

	if(pUser->m_sEmail != NULL) {
		TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sEmail, pUser->m_ui8EmailLen, sEmail, 97);
	}

	char sSQLCommand[1024];
	sqlite3_snprintf(1024, sSQLCommand,
		"UPDATE userinfo SET "
		"nick = %Q,"
		"last_updated = DATETIME('now')," // last_updated
		"ip_address = %Q," // ip
		"share = %Q," // share
		"description = %Q," // description
		"tag = %Q," // tag
		"connection = %Q," // connection
		"email = %Q" // email
		"WHERE LOWER(nick) = LOWER(%Q);", // nick
		sNick, pUser->m_sIP, sShare, sDescription, sTag, sConnection, sEmail, sNick
	);

	char * sErrMsg = NULL;

	int iRet = sqlite3_exec(m_pSqliteDB, sSQLCommand, NULL, NULL, &sErrMsg);

	if(iRet != SQLITE_OK) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite update record failed: %s", sErrMsg);
		sqlite3_free(sErrMsg);
	}

	iRet = sqlite3_changes(m_pSqliteDB);
	if(iRet != 0) {
		return;
	}

	sqlite3_snprintf(1024, sSQLCommand,
		"INSERT INTO userinfo (nick, last_updated, ip_address, share, description, tag, connection, email) VALUES ("
		"%Q," // nick
		"DATETIME('now')," // last_updated
		"%Q," // ip
		"%Q," // share
		"%Q," // description
		"%Q," // tag
		"%Q," // connection
		"%Q" // email
		");",
		sNick, pUser->m_sIP, sShare, sDescription, sTag, sConnection, sEmail
	);

	iRet = sqlite3_exec(m_pSqliteDB, sSQLCommand, NULL, NULL, &sErrMsg);

	if(iRet != SQLITE_OK) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite insert record failed: %s", sErrMsg);
		sqlite3_free(sErrMsg);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static bool bFirst = true;
static bool bSecond = false;
static char sFirstNick[65];
static char sFirstIP[40];
static int iMsgLen = 0;
static int iAfterHubSecMsgLen = 0;

static int SelectCallBack(void *, int iArgCount, char ** ppArgSTrings, char **) {
	if(iArgCount != 8) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite SelectCallBack wrong iArgCount: %d", iArgCount);
		return 0;
	}

	if(bFirst == true) {
		bFirst = false;
		bSecond = true;

		size_t szLength = strlen(ppArgSTrings[0]);
		memcpy(sFirstNick, ppArgSTrings[0], szLength);
		sFirstNick[szLength] = '\0';

		szLength = strlen(ppArgSTrings[2]);
		memcpy(sFirstIP, ppArgSTrings[2], szLength);
		sFirstIP[szLength] = '\0';

		szLength = strlen(ppArgSTrings[0]);
		if(szLength == 0 || szLength > 64) {
			UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid nick length: %zu", szLength);
			return 0;
		}

        int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_NICK], ppArgSTrings[0]);
        if(iRet <= 0) {
			return 0;
        }
        iMsgLen += iRet;

		RegUser * pReg = RegManager::m_Ptr->Find(ppArgSTrings[0], szLength);
        if(pReg != NULL) {
            iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_PROFILE], ProfileManager::m_Ptr->m_ppProfilesTable[pReg->m_ui16Profile]->m_sName);
        	if(iRet <= 0) {
        		return 0;
            }
            iMsgLen += iRet;
        }

		// In case when SQL wildcards were used is possible that user is online. Then we don't use data from database, but data that are in server memory.
		User * pOnlineUser = HashManager::m_Ptr->FindUser(ppArgSTrings[0], szLength);
		if(pOnlineUser != NULL) {
            iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s ", LanguageManager::m_Ptr->m_sTexts[LAN_STATUS], LanguageManager::m_Ptr->m_sTexts[LAN_ONLINE_FROM]);
        	if(iRet <= 0) {
        		return 0;
            }
            iMsgLen += iRet;

            struct tm *tm = localtime(&pOnlineUser->m_tLoginTime);
            iRet = (int)strftime(ServerManager::m_pGlobalBuffer+iMsgLen, 256, "%c", tm);
            if(iRet <= 0) {
        		return 0;
            }
            iMsgLen += iRet;

			if(pOnlineUser->m_sIPv4[0] != '\0') {
				iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s / %s\n%s: %0.02f %s", LanguageManager::m_Ptr->m_sTexts[LAN_IP], pOnlineUser->m_sIP, pOnlineUser->m_sIPv4, LanguageManager::m_Ptr->m_sTexts[LAN_SHARE_SIZE], (double)pOnlineUser->m_ui64SharedSize/1073741824, LanguageManager::m_Ptr->m_sTexts[LAN_GIGA_BYTES]);
            	if(iRet <= 0) {
        			return 0;
				}
				iMsgLen += iRet;
			} else {
				iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s\n%s: %0.02f %s", LanguageManager::m_Ptr->m_sTexts[LAN_IP], pOnlineUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_SHARE_SIZE], (double)pOnlineUser->m_ui64SharedSize/1073741824, LanguageManager::m_Ptr->m_sTexts[LAN_GIGA_BYTES]);
            	if(iRet <= 0) {
        			return 0;
				}
				iMsgLen += iRet;
			}

            if(pOnlineUser->m_sDescription != NULL) {
                iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[LAN_DESCRIPTION]);
            	if(iRet <= 0) {
                    return 0;
                }
                iMsgLen += iRet;
                memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, pOnlineUser->m_sDescription, pOnlineUser->m_ui8DescriptionLen);
                iMsgLen += (int)pOnlineUser->m_ui8DescriptionLen;
            }

            if(pOnlineUser->m_sTag != NULL) {
                iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[LAN_TAG]);
            	if(iRet <= 0) {
                    return 0;
                }
                iMsgLen += iRet;
                memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, pOnlineUser->m_sTag, pOnlineUser->m_ui8TagLen);
                iMsgLen += (int)pOnlineUser->m_ui8TagLen;
            }
                    
            if(pOnlineUser->m_sConnection != NULL) {
                iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[LAN_CONNECTION]);
            	if(iRet <= 0) {
                    return 0;
                }
                iMsgLen += iRet;
                memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, pOnlineUser->m_sConnection, pOnlineUser->m_ui8ConnectionLen);
                iMsgLen += (int)pOnlineUser->m_ui8ConnectionLen;
            }

            if(pOnlineUser->m_sEmail != NULL) {
                iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[LAN_EMAIL]);
            	if(iRet <= 0) {
                    return 0;
                }
                iMsgLen += iRet;
                memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, pOnlineUser->m_sEmail, pOnlineUser->m_ui8EmailLen);
                iMsgLen += (int)pOnlineUser->m_ui8EmailLen;
            }

            if(IpP2Country::m_Ptr->m_ui32Count != 0) {
                iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[LAN_COUNTRY]);
            	if(iRet <= 0) {
                    return 0;
                }
                iMsgLen += iRet;
                memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, IpP2Country::m_Ptr->GetCountry(pOnlineUser->m_ui8Country, false), 2);
                iMsgLen += 2;
            }

            return 0;
		} else { // User is offline, then we use data from database.
            iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s ", LanguageManager::m_Ptr->m_sTexts[LAN_STATUS], LanguageManager::m_Ptr->m_sTexts[LAN_OFFLINE_FROM]);
        	if(iRet <= 0) {
        		return 0;
            }
            iMsgLen += iRet;
#ifdef _WIN32
        	time_t tTime = (time_t)_strtoui64(ppArgSTrings[1], NULL, 10);
#else
			time_t tTime = (time_t)strtoull(ppArgSTrings[1], NULL, 10);
#endif
            struct tm *tm = localtime(&tTime);
            iRet = (int)strftime(ServerManager::m_pGlobalBuffer+iMsgLen, 256, "%c", tm);
            if(iRet <= 0) {
        		return 0;
            }
            iMsgLen += iRet;

			szLength = strlen(ppArgSTrings[2]);
			if(szLength == 0 || szLength > 39) {
				UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid ip length: %zu", szLength);
				return 0;
			}

			szLength = strlen(ppArgSTrings[3]);
			if(szLength == 0 || szLength > 24) {
				UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid share length: %zu", szLength);
				return 0;
			}

			char * sIP = ppArgSTrings[2];

			iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_IP], sIP, LanguageManager::m_Ptr->m_sTexts[LAN_SHARE_SIZE], ppArgSTrings[3]);
            if(iRet <= 0) {
        		return 0;
			}
			iMsgLen += iRet;

			szLength = strlen(ppArgSTrings[4]);
            if(szLength != 0) {
				if(szLength > 192) {
					UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid description length: %zu", szLength);
					return 0;
				}

				if(szLength > 0) {
                	iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_DESCRIPTION], ppArgSTrings[4]);
            		if(iRet <= 0) {
        				return 0;
        			}
        			iMsgLen += iRet;
                }
            }

			szLength = strlen(ppArgSTrings[5]);
            if(szLength != 0) {
				if(szLength > 192) {
					UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid tag length: %zu", szLength);
					return 0;
				}

				if(szLength > 0) {
                	iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_TAG], ppArgSTrings[5]);
            		if(iRet <= 0) {
                    	return 0;
                    }
                    iMsgLen += iRet;
                }
            }

			szLength = strlen(ppArgSTrings[6]);
            if(szLength != 0) {
				if(szLength > 32) {
					UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid connection length: %zu", szLength);
					return 0;
				}

				if(szLength > 0) {
                	iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_CONNECTION], ppArgSTrings[6]);
            		if(iRet <= 0) {
                    	return 0;
                    }
                    iMsgLen += iRet;
                }
            }

			szLength = strlen(ppArgSTrings[7]);
            if(szLength != 0) {
				if(szLength > 96) {
					UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid email length: %zu", szLength);
					return 0;
				}

				if(szLength > 0) {
                	iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_EMAIL], ppArgSTrings[7]);
            		if(iRet <= 0) {
                    	return 0;
                    }
                    iMsgLen += iRet;
                }
            }

			uint8_t ui128IPHash[16];
			memset(ui128IPHash, 0, 16);

            if(IpP2Country::m_Ptr->m_ui32Count != 0 && HashIP(sIP, ui128IPHash) == true) {
                iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[LAN_COUNTRY]);
            	if(iRet <= 0) {
                    return 0;
                }
                iMsgLen += iRet;

                memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, IpP2Country::m_Ptr->Find(ui128IPHash, false), 2);
                iMsgLen += 2;
            }

            return 0;
        }
	} else if(bSecond == true) {
		bSecond = false;

		size_t szLength = strlen(sFirstNick);
		if(szLength == 0 || szLength > 64) {
			UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid nick length: %zu", szLength);
			return 0;
		}
	
		szLength = strlen(sFirstIP);
		if(szLength == 0 || szLength > 39) {
			UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid ip length: %zu", szLength);
			return 0;
		}
	
	    int iRet = snprintf(ServerManager::m_pGlobalBuffer+iAfterHubSecMsgLen, ServerManager::m_szGlobalBufferSize-iAfterHubSecMsgLen, "\n%s: %s\t\t%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_NICK], sFirstNick, LanguageManager::m_Ptr->m_sTexts[LAN_IP], sFirstIP);
	    if(iRet <= 0) {
			return 0;
	    }
	    iMsgLen = iAfterHubSecMsgLen+iRet;
	}

	size_t szLength = strlen(ppArgSTrings[0]);
	if(szLength == 0 || szLength > 64) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid nick length: %zu", szLength);
		return 0;
	}

	szLength = strlen(ppArgSTrings[2]);
	if(szLength == 0 || szLength > 39) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search returned invalid ip length: %zu", szLength);
		return 0;
	}

    int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s\t\t%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_NICK], ppArgSTrings[0], LanguageManager::m_Ptr->m_sTexts[LAN_IP], ppArgSTrings[2]);
    if(iRet <= 0) {
		return 0;
    }
    iMsgLen += iRet;

	return 0;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// First of two functions to search data in database. Nick will be probably most used.
bool DBSQLite::SearchNick(ChatCommand * pChatCommand) {
	if(m_bConnected == false) {
		return false;
	}

	char sUtfNick[65];
	if(TextConverter::m_Ptr->CheckUtf8AndConvert(pChatCommand->m_sCommand, (uint8_t)pChatCommand->m_ui32CommandLen, sUtfNick, 65) == 0) {
		return false;
	}

	if(pChatCommand->m_bFromPM == true) {
    	iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$To: %s From: %s $<%s> ", pChatCommand->m_pUser->m_sNick, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
    } else {
    	iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> ", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
	}

	if(iMsgLen <= 0) {
        return false;
    }

	iAfterHubSecMsgLen = iMsgLen;

	bFirst = true;
	bSecond = false;

	char sSQLCommand[256];
	sqlite3_snprintf(256, sSQLCommand, "SELECT nick, %s, ip_address, share, description, tag, connection, email FROM userinfo WHERE LOWER(nick) LIKE LOWER(%Q) ORDER BY last_updated DESC LIMIT 50;", "strftime('%s', last_updated)", sUtfNick);

	char * sErrMsg = NULL;

	int iRet = sqlite3_exec(m_pSqliteDB, sSQLCommand, SelectCallBack, NULL, &sErrMsg);

	if(iRet != SQLITE_OK) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search for nick failed: %s", sErrMsg);
		sqlite3_free(sErrMsg);

		return false;
	}

	if(iMsgLen == iAfterHubSecMsgLen) {
		return false;
	} else {
		ServerManager::m_pGlobalBuffer[iMsgLen] = '|';
        ServerManager::m_pGlobalBuffer[iMsgLen+1] = '\0';  

        pChatCommand->m_pUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iMsgLen+1);

		return true;
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Second of two fnctions to search data in database. Now using IP.
bool DBSQLite::SearchIP(ChatCommand * pChatCommand) {
	if(m_bConnected == false) {
		return false;
	}

	if(pChatCommand->m_bFromPM == true) {
    	iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$To: %s From: %s $<%s> ", pChatCommand->m_pUser->m_sNick, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
    } else {
    	iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> ", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
	}

    if(iMsgLen <= 0) {
    	return false;
	}

	iAfterHubSecMsgLen = iMsgLen;

	bFirst = true;
	bSecond = false;

	char sSQLCommand[256];
	sqlite3_snprintf(256, sSQLCommand, "SELECT nick, %s, ip_address, share, description, tag, connection, email FROM userinfo WHERE ip_address LIKE %Q ORDER BY last_updated DESC LIMIT 50;", "strftime('%s', last_updated)", pChatCommand->m_sCommand);

	char * sErrMsg = NULL;

	int iRet = sqlite3_exec(m_pSqliteDB, sSQLCommand, SelectCallBack, NULL, &sErrMsg);

	if(iRet != SQLITE_OK) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite search for nick failed: %s", sErrMsg);
		sqlite3_free(sErrMsg);

		return false;
	}

	if(iMsgLen == iAfterHubSecMsgLen) {
		return false;
	} else {
        ServerManager::m_pGlobalBuffer[iMsgLen] = '|';
        ServerManager::m_pGlobalBuffer[iMsgLen+1] = '\0'; 
 
        pChatCommand->m_pUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iMsgLen+1);

		return true;
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function to remove X days old records from database.
void DBSQLite::RemoveOldRecords(const uint16_t ui16Days) {
	if(m_bConnected == false) {
		return;
	}

	char sSQLCommand[256];
	if(snprintf(sSQLCommand, 256, "DELETE FROM userinfo WHERE last_updated < DATETIME('now', '-%hu days', 'localtime');", ui16Days) <= 0) {
		return;
	}

	char * sErrMsg = NULL;

	int iRet = sqlite3_exec(m_pSqliteDB, sSQLCommand, SelectCallBack, NULL, &sErrMsg);

	if(iRet != SQLITE_OK) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite remove old records failed: %s", sErrMsg);
		sqlite3_free(sErrMsg);
	}

	iRet = sqlite3_changes(m_pSqliteDB);
	if(iRet != 0) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBSQLite removed old records: %d", iRet);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
