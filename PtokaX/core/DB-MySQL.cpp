/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2004-2017 Petr Kozelka, PPK at PtokaX dot org

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
#include "DB-MySQL.h"
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
#include <mysql.h>
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
DBMySQL * DBMySQL::m_Ptr = NULL;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

DBMySQL::DBMySQL() :
#ifdef _WIN32
    m_hThreadHandle(NULL),
#else
    m_threadId(0),
#endif
	m_bConnected(false), m_bTerminated(false) {

	if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_DATABASE] == false || SettingManager::m_Ptr->m_sTexts[SETTXT_MYSQL_PASS] == NULL) {
		return;
	}

	m_pDBHandle = mysql_init(NULL);
	
	if(m_pDBHandle == NULL) {
		AppendLog("DBMySQL init failed");

		return;
	}

	mysql_options(m_pDBHandle, MYSQL_SET_CHARSET_NAME, "utf8mb4");
	
	if(mysql_real_connect(m_pDBHandle, SettingManager::m_Ptr->m_sTexts[SETTXT_MYSQL_HOST], SettingManager::m_Ptr->m_sTexts[SETTXT_MYSQL_USER], SettingManager::m_Ptr->m_sTexts[SETTXT_MYSQL_PASS], SettingManager::m_Ptr->m_sTexts[SETTXT_MYSQL_DBNAME], atoi(SettingManager::m_Ptr->m_sTexts[SETTXT_MYSQL_PORT]), NULL, 0) == NULL) {
		AppendLog((string("DBMySQL connection failed: ")+mysql_error(m_pDBHandle)).c_str());
		mysql_close(m_pDBHandle);

		return;
	}

	if(mysql_query(m_pDBHandle, 
		"CREATE TABLE IF NOT EXISTS userinfo ("
			"nick VARCHAR(64) NOT NULL PRIMARY KEY,"
			"last_updated DATETIME NOT NULL,"
			"ip_address VARCHAR(39) NOT NULL,"
			"share VARCHAR(24) NOT NULL,"
			"description VARCHAR(192),"
			"tag VARCHAR(192),"
			"connection VARCHAR(32),"
			"email VARCHAR(96),"
			"UNIQUE (nick)"
		")"
		"DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci"
	) != 0) {
		AppendLog((string("DBMySQL check/create table failed: ")+mysql_error(m_pDBHandle)).c_str());
		mysql_close(m_pDBHandle);

		return;
	}

	m_bConnected = true;

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
DBMySQL::~DBMySQL() {
	// When user don't want to save data in database forever then he can set to remove records older than X days.
	if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DB_REMOVE_OLD_RECORDS] != 0) {
		RemoveOldRecords(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DB_REMOVE_OLD_RECORDS]);
	}

	m_bTerminated = true;

#ifdef _WIN32
    if(m_hThreadHandle != NULL) {
    	WaitForSingleObject(m_hThreadHandle, INFINITE);
        CloseHandle(m_hThreadHandle);
#else
    if(m_threadId != 0) {
        pthread_join(m_threadId, NULL);
#endif
	}

	if(m_bConnected == true) {
		mysql_close(m_pDBHandle);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Now that important part. Function to update or insert user to database.
void DBMySQL::UpdateRecord(User * pUser) {
	if(m_bConnected == false) {
		return;
	}

	char sUtfNick[65];
	size_t szLen = TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sNick, pUser->m_ui8NickLen, sUtfNick, 65);
	if(szLen == 0) {
		return;
	}

	char sNick[129];
	if(mysql_real_escape_string(m_pDBHandle, sNick, sUtfNick, szLen) == 0) {
		return;
	}

	char sShare[24];
	if(snprintf(sShare, 24, "%0.02f GB", (double)pUser->m_ui64SharedSize/1073741824) <= 0) {
		return;
	}

	char sDescription[385];
	if(pUser->m_sDescription != NULL) {
		char sUtfDescription[193];

		szLen = TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sDescription, pUser->m_ui8DescriptionLen, sUtfDescription, 193);
		if(szLen != 0) {
			mysql_real_escape_string(m_pDBHandle, sDescription, sUtfDescription, szLen);
		} else {
			sDescription[0] = '\0';
		}
	} else {
		sDescription[0] = '\0';
	}

	char sTag[385];
	if(pUser->m_sTag != NULL) {
		char sUtfTag[193];

		szLen = TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sTag, pUser->m_ui8TagLen, sUtfTag, 193);
		if(szLen != 0) {
			mysql_real_escape_string(m_pDBHandle, sTag, sUtfTag, szLen);
		} else {
			sTag[0] = '\0';
		}
	} else {
		sTag[0] = '\0';
	}

	char sConnection[65];
	if(pUser->m_sConnection != NULL) {
		char sUtfConnection[33];

		szLen = TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sConnection, pUser->m_ui8ConnectionLen, sUtfConnection, 33);
		if(szLen != 0) {
			mysql_real_escape_string(m_pDBHandle, sConnection, sUtfConnection, szLen);
		} else {
			sConnection[0] = '\0';
		}
	} else {
		sConnection[0] = '\0';
	}

	char sEmail[193];
	if(pUser->m_sEmail != NULL) {
		char sUtfEmail[97];

		szLen = TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sEmail, pUser->m_ui8EmailLen, sUtfEmail, 97);
		if(szLen != 0) {
			mysql_real_escape_string(m_pDBHandle, sEmail, sUtfEmail, szLen);
		} else {
			sEmail[0] = '\0';
		}
	} else {
		sEmail[0] = '\0';
	}

	char sSQLCommand[4096];
	if(snprintf(sSQLCommand, 4096,
		"INSERT INTO userinfo (nick, last_updated, ip_address, share, description, tag, connection, email) VALUES ("
			"'%s', NOW(), '%s', '%s', '%s', '%s', '%s', '%s'"
		")  ON DUPLICATE KEY UPDATE "
			"last_updated = NOW(), ip_address = '%s', share = '%s',"
			"description = '%s', tag = '%s', connection = '%s', email = '%s'",
		sNick, pUser->m_sIP, sShare, sDescription, sTag, sConnection, sEmail,
		pUser->m_sIP, sShare, sDescription, sTag, sConnection, sEmail) <= 0)
	{
		return;
	}

	if(mysql_query(m_pDBHandle, sSQLCommand) != 0) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL insert/update record failed: %s", mysql_error(m_pDBHandle));
		ReconnectDb();
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function to send data returned by SELECT to Operator who requested them.
bool DBMySQL::SendQueryResults(ChatCommand * pChatCommand, MYSQL_RES * pResult, uint64_t &ui64Rows) {
	unsigned int uiCount = mysql_num_fields(pResult);
	if(uiCount != 8) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL mysql_num_fields wrong fields count: %u", uiCount);
		return false;
	}

	if(ui64Rows == 1) { // When we have only one result then we send whole info about user.
		MYSQL_ROW mRow = mysql_fetch_row(pResult);
		if(mRow == NULL) {
			UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL mysql_fetch_row failed: %s", mysql_error(m_pDBHandle));
			return false;
		}

		unsigned long * pLengths = mysql_fetch_lengths(pResult);
		if(pLengths == NULL) {
			UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL mysql_fetch_lengths failed: %s", mysql_error(m_pDBHandle));
			return false;
		}

		int iMsgLen = 0;

		if(pChatCommand->m_bFromPM == true) {
    		iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$To: %s From: %s $", pChatCommand->m_pUser->m_sNick, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
    		if(iMsgLen <= 0) {
        		return false;
    		}
    	}

		if(pLengths[0] == 0 || pLengths[0] > 64) {
			UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL search returned invalid nick length: %" PRIu64, (uint64_t)pLengths[0]);
			return false;
		}

        int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "<%s> \n%s: %s", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_NICK], mRow[0]);
        if(iRet <= 0) {
			return false;
        }
        iMsgLen += iRet;

		RegUser * pReg = RegManager::m_Ptr->Find(mRow[0], pLengths[0]);
        if(pReg != NULL) {
            iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_PROFILE], ProfileManager::m_Ptr->m_ppProfilesTable[pReg->m_ui16Profile]->m_sName);
        	if(iRet <= 0) {
        		return false;
            }
            iMsgLen += iRet;
        }

		// In case when SQL wildcards were used is possible that user is online. Then we don't use data from database, but data that are in server memory.
		User * pOnlineUser = HashManager::m_Ptr->FindUser(mRow[0], pLengths[0]);
		if(pOnlineUser != NULL) {
            iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s ", LanguageManager::m_Ptr->m_sTexts[LAN_STATUS], LanguageManager::m_Ptr->m_sTexts[LAN_ONLINE_FROM]);
        	if(iRet <= 0) {
        		return false;
            }
            iMsgLen += iRet;

            struct tm *tm = localtime(&pOnlineUser->m_tLoginTime);
            iRet = (int)strftime(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "%c", tm);
            if(iRet <= 0) {
        		return false;
            }
            iMsgLen += iRet;

			if(pOnlineUser->m_sIPv4[0] != '\0') {
				iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s / %s\n%s: %0.02f %s", LanguageManager::m_Ptr->m_sTexts[LAN_IP], pOnlineUser->m_sIP, pOnlineUser->m_sIPv4, LanguageManager::m_Ptr->m_sTexts[LAN_SHARE_SIZE], (double)pOnlineUser->m_ui64SharedSize/1073741824, LanguageManager::m_Ptr->m_sTexts[LAN_GIGA_BYTES]);
            	if(iRet <= 0) {
        			return false;
				}
				iMsgLen += iRet;
			} else {
				iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s\n%s: %0.02f %s", LanguageManager::m_Ptr->m_sTexts[LAN_IP], pOnlineUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_SHARE_SIZE], (double)pOnlineUser->m_ui64SharedSize/1073741824, LanguageManager::m_Ptr->m_sTexts[LAN_GIGA_BYTES]);
            	if(iRet <= 0) {
        			return false;
				}
				iMsgLen += iRet;
			}

            if(pOnlineUser->m_sDescription != NULL) {
                iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[LAN_DESCRIPTION]);
            	if(iRet <= 0) {
                    return false;
                }
                iMsgLen += iRet;
                memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, pOnlineUser->m_sDescription, pOnlineUser->m_ui8DescriptionLen);
                iMsgLen += (int)pOnlineUser->m_ui8DescriptionLen;
            }

            if(pOnlineUser->m_sTag != NULL) {
                iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[LAN_TAG]);
            	if(iRet <= 0) {
                    return false;
                }
                iMsgLen += iRet;
                memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, pOnlineUser->m_sTag, pOnlineUser->m_ui8TagLen);
                iMsgLen += (int)pOnlineUser->m_ui8TagLen;
            }
                    
            if(pOnlineUser->m_sConnection != NULL) {
                iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[LAN_CONNECTION]);
            	if(iRet <= 0) {
                    return false;
                }
                iMsgLen += iRet;
                memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, pOnlineUser->m_sConnection, pOnlineUser->m_ui8ConnectionLen);
                iMsgLen += (int)pOnlineUser->m_ui8ConnectionLen;
            }

            if(pOnlineUser->m_sEmail != NULL) {
                iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[LAN_EMAIL]);
            	if(iRet <= 0) {
                    return false;
                }
                iMsgLen += iRet;
                memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, pOnlineUser->m_sEmail, pOnlineUser->m_ui8EmailLen);
                iMsgLen += (int)pOnlineUser->m_ui8EmailLen;
            }

            if(IpP2Country::m_Ptr->m_ui32Count != 0) {
                iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[LAN_COUNTRY]);
            	if(iRet <= 0) {
                    return false;
                }
                iMsgLen += iRet;
                memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, IpP2Country::m_Ptr->GetCountry(pOnlineUser->m_ui8Country, false), 2);
                iMsgLen += 2;
            }

            ServerManager::m_pGlobalBuffer[iMsgLen] = '|';
            ServerManager::m_pGlobalBuffer[iMsgLen+1] = '\0';

            pChatCommand->m_pUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iMsgLen+1);

            return true;
		} else { // User is offline, then we use data from database.
            iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s ", LanguageManager::m_Ptr->m_sTexts[LAN_STATUS], LanguageManager::m_Ptr->m_sTexts[LAN_OFFLINE_FROM]);
        	if(iRet <= 0) {
        		return false;
            }
            iMsgLen += iRet;
#ifdef _WIN32
        	time_t tTime = (time_t)_strtoui64(mRow[1], NULL, 10);
#else
			time_t tTime = (time_t)strtoull(mRow[1], NULL, 10);
#endif
            struct tm *tm = localtime(&tTime);
            iRet = (int)strftime(ServerManager::m_pGlobalBuffer+iMsgLen, 256, "%c", tm);
            if(iRet <= 0) {
        		return false;
            }
            iMsgLen += iRet;

			if(pLengths[2] == 0 || pLengths[2] > 39) {
				UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL search returned invalid ip length: %" PRIu64, (uint64_t)pLengths[2]);
				return false;
			}

			if(pLengths[3] == 0 || pLengths[3] > 24) {
				UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL search returned invalid share length: %" PRIu64, (uint64_t)pLengths[3]);
				return false;
			}

			char * sIP = mRow[2];

			iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_IP], sIP, LanguageManager::m_Ptr->m_sTexts[LAN_SHARE_SIZE], mRow[3]);
            if(iRet <= 0) {
        		return false;
			}
			iMsgLen += iRet;

            if(mRow[4] != NULL) {
				if(pLengths[4] > 192) {
					UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL search returned invalid description length: %" PRIu64, (uint64_t)pLengths[4]);
					return  false;
				}

				if(pLengths[4] > 0) {
                	iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_DESCRIPTION], mRow[4]);
            		if(iRet <= 0) {
        				return false;
        			}
        			iMsgLen += iRet;
                }
            }

            if(mRow[5] != NULL) {
				if(pLengths[5] > 192) {
					UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL search returned invalid tag length: %" PRIu64, (uint64_t)pLengths[5]);
					return false;
				}

				if(pLengths[5] > 0) {
                	iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_TAG], mRow[5]);
            		if(iRet <= 0) {
                    	return false;
                    }
                    iMsgLen += iRet;
                }
            }
                    
            if(mRow[6] != NULL) {
				if(pLengths[6] > 32) {
					UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL search returned invalid connection length: %" PRIu64, (uint64_t)pLengths[6]);
					return false;
				}

				if(pLengths[6] > 0) {
                	iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_CONNECTION], mRow[6]);
            		if(iRet <= 0) {
                    	return false;
                    }
                    iMsgLen += iRet;
                }
            }

            if(mRow[7] != NULL) {
				if(pLengths[7] > 96) {
					UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL search returned invalid email length: %" PRIu64, (uint64_t)pLengths[7]);
					return false;
				}

				if(pLengths[7] > 0) {
                	iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_EMAIL], mRow[7]);
            		if(iRet <= 0) {
                    	return false;
                    }
                    iMsgLen += iRet;
                }
            }

			uint8_t ui128IPHash[16];
			memset(ui128IPHash, 0, 16);

            if(IpP2Country::m_Ptr->m_ui32Count != 0 && HashIP(sIP, ui128IPHash) == true) {
                iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[LAN_COUNTRY]);
            	if(iRet <= 0) {
                    return false;
                }
                iMsgLen += iRet;

                memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, IpP2Country::m_Ptr->Find(ui128IPHash, false), 2);
                iMsgLen += 2;
            }

            ServerManager::m_pGlobalBuffer[iMsgLen] = '|';
            ServerManager::m_pGlobalBuffer[iMsgLen+1] = '\0';

            pChatCommand->m_pUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iMsgLen+1);

            return true;
        }
	} else { // When we have more than 1 result from database then we send only short info with user nick and ip address.
		int iMsgLen = 0;

		if(pChatCommand->m_bFromPM == true) {
    		iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$To: %s From: %s $<%s> ", pChatCommand->m_pUser->m_sNick, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
    	} else {
    		iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> ", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
		}

	    if(iMsgLen <= 0) {
    		return false;
		}

		for(uint64_t ui64ActualRow = 0; ui64ActualRow < ui64Rows; ui64ActualRow++) {
			MYSQL_ROW mRow = mysql_fetch_row(pResult);
			if(mRow == NULL) {
				UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL mysql_fetch_row failed: %s", mysql_error(m_pDBHandle));
				return false;
			}
	
			unsigned long * pLengths = mysql_fetch_lengths(pResult);
			if(pLengths == NULL) {
				UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL mysql_fetch_lengths failed: %s", mysql_error(m_pDBHandle));
				return false;
			}

			if(pLengths[0] == 0 || pLengths[0] > 64) {
				UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL search returned invalid nick length: %" PRIu64, (uint64_t)pLengths[0]);
				return false;
			}

			if(pLengths[2] == 0 || pLengths[2] > 39) {
				UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL search returned invalid ip length: %" PRIu64, (uint64_t)pLengths[2]);
				return false;
			}

        	int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s\t\t%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_NICK], mRow[0], LanguageManager::m_Ptr->m_sTexts[LAN_IP], mRow[2]);
        	if(iRet <= 0) {
				return false;
        	}
        	iMsgLen += iRet;
		}

        ServerManager::m_pGlobalBuffer[iMsgLen] = '|';
        ServerManager::m_pGlobalBuffer[iMsgLen+1] = '\0';

        pChatCommand->m_pUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iMsgLen+1);

        return true;
	}
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// First of two functions to search data in database. Nick will be probably most used.
bool DBMySQL::SearchNick(ChatCommand * pChatCommand) {
	if(m_bConnected == false) {
		return false;
	}

	char sUtfNick[65];
	size_t szLen = TextConverter::m_Ptr->CheckUtf8AndConvert(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen, sUtfNick, 65);
	if(szLen == 0) {
		return false;
	}

	char sEscapedNick[129];
	if(mysql_real_escape_string(m_pDBHandle, sEscapedNick, sUtfNick, szLen) == 0) {
		return false;
	}

	char sSQLCommand[256];
	if(snprintf(sSQLCommand, 256, "SELECT nick, UNIX_TIMESTAMP(last_updated), ip_address, share, description, tag, connection, email FROM userinfo WHERE LOWER(nick) LIKE LOWER('%s') ORDER BY last_updated DESC LIMIT 50", sEscapedNick) <= 0) {
		return false;
	}

	if(mysql_query(m_pDBHandle, sSQLCommand) != 0) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL search for nick failed: %s", mysql_error(m_pDBHandle));
		ReconnectDb();

		return false;
	}

	MYSQL_RES * pResult = mysql_store_result(m_pDBHandle);

	if(pResult == NULL) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL search for nick store result failed: %s", mysql_error(m_pDBHandle));

		return false;
	}

	uint64_t ui64Rows = (uint64_t)mysql_num_rows(pResult);

	bool bResult = false;

	if(ui64Rows != 0) {
		bResult = SendQueryResults(pChatCommand, pResult, ui64Rows);
	}

	mysql_free_result(pResult);

	return bResult;

}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Second of two fnctions to search data in database. Now using IP.
bool DBMySQL::SearchIP(ChatCommand * pChatCommand) {
	if(m_bConnected == false) {
		return false;
	}

	char sEscapedIP[79];
	if(mysql_real_escape_string(m_pDBHandle, sEscapedIP, pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen < 40 ? pChatCommand->m_ui32CommandLen : 39) == 0) {
		return false;
	}

	char sSQLCommand[256];
	if(snprintf(sSQLCommand, 256, "SELECT nick, UNIX_TIMESTAMP(last_updated), ip_address, share, description, tag, connection, email FROM userinfo WHERE ip_address LIKE '%s' ORDER BY last_updated DESC LIMIT 50", sEscapedIP) <= 0) {
		return false;
	}

	if(mysql_query(m_pDBHandle, sSQLCommand) != 0) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL search for IP failed: %s", mysql_error(m_pDBHandle));
		ReconnectDb();

		return false;
	}
	
	my_ulonglong mullRows = mysql_affected_rows(m_pDBHandle);

	if(mullRows == 0) {
        return false;
	}

	MYSQL_RES * pResult = mysql_store_result(m_pDBHandle);

	if(pResult == NULL) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL search for IP store result failed: %s", mysql_error(m_pDBHandle));

		return false;
	}

	uint64_t ui64Rows = (uint64_t)mysql_num_rows(pResult);

	bool bResult = false;

	if(ui64Rows != 0) {
		bResult = SendQueryResults(pChatCommand, pResult, ui64Rows);
	}

	mysql_free_result(pResult);

	return bResult;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function to remove X days old records from database.
void DBMySQL::RemoveOldRecords(const uint16_t ui16Days) {
	if(m_bConnected == false) {
		return;
	}

	char sSQLCommand[256];
	if(snprintf(sSQLCommand, 256, "DELETE FROM userinfo WHERE last_updated < UNIX_TIMESTAMP(DATE_SUB(NOW(), INTERVAL %hu DAY))", ui16Days) <= 0) {
		return;
	}

	if(mysql_query(m_pDBHandle, sSQLCommand) != 0) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL remove old records failed: %s", mysql_error(m_pDBHandle));

		return;
	}

	uint64_t ui64Rows = (uint64_t)mysql_affected_rows(m_pDBHandle);

	if(ui64Rows != 0) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBMySQL removed old records: %" PRIu64, ui64Rows);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#ifdef _WIN32
unsigned __stdcall ExecuteReconnect(void * pThread) {
#else
static void* ExecuteReconnect(void * pThread) {
#endif
	(reinterpret_cast<DBMySQL *>(pThread))->RunReconnect();

	return 0;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void DBMySQL::ReconnectDb()
{
	m_bConnected = false;

	mysql_close(m_pDBHandle);

	m_bTerminated = true;
#ifdef _WIN32
    if(m_hThreadHandle != NULL) {
        CloseHandle(m_hThreadHandle);
#else
    if(m_threadId != 0) {
        pthread_join(m_threadId, NULL);
#endif
	}

	m_bTerminated = false;

#ifdef _WIN32
	m_hThreadHandle = (HANDLE)_beginthreadex(NULL, 0, ExecuteReconnect, this, 0, NULL);
	if(m_hThreadHandle == NULL) {
#else
	int iRet = pthread_create(&m_threadId, NULL, ExecuteReconnect, this);
	if(iRet != 0) {
#endif
		AppendDebugLog("%s - [ERR] Failed to create new MySQL ReconnectDb thread\n");
    }	
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void DBMySQL::RunReconnect() {
	if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_DATABASE] == false) {
		return;
	}

	m_pDBHandle = mysql_init(NULL);
	
	if(m_pDBHandle == NULL) {
		m_bConnected = false;
		AppendLog("DBMySQL init failed");

		return;
	}

	mysql_options(m_pDBHandle, MYSQL_SET_CHARSET_NAME, "utf8mb4");

	char sHost[257], sPort[6], sDbName[257], sUser[257], sPass[257];
	SettingManager::m_Ptr->GetText(SETTXT_MYSQL_HOST, sHost);
	SettingManager::m_Ptr->GetText(SETTXT_MYSQL_PORT, sPort);
	SettingManager::m_Ptr->GetText(SETTXT_MYSQL_DBNAME, sDbName);
	SettingManager::m_Ptr->GetText(SETTXT_MYSQL_USER, sUser);
	SettingManager::m_Ptr->GetText(SETTXT_MYSQL_PASS, sPass);

	if(sHost[0] == '\0' || sPort[0] == '\0' || sDbName[0] == '\0' || sUser[0] == '\0' || sPass[0] == '\0') {
		return;
	}

#ifndef _WIN32
    struct timespec sleeptime;
    sleeptime.tv_sec = 1;
    sleeptime.tv_nsec = 0;
#endif

	while(m_bTerminated == false) {
		if(mysql_real_connect(m_pDBHandle, sHost, sUser, sPass, sDbName, atoi(sPort), NULL, 0) == NULL) {
			mysql_close(m_pDBHandle);

#ifdef _WIN32
			::Sleep(1000);
#else
			nanosleep(&sleeptime, NULL);
#endif
			continue;
		}
	
		if(mysql_query(m_pDBHandle, 
			"CREATE TABLE IF NOT EXISTS userinfo ("
				"nick VARCHAR(64) NOT NULL PRIMARY KEY,"
				"last_updated DATETIME NOT NULL,"
				"ip_address VARCHAR(39) NOT NULL,"
				"share VARCHAR(24) NOT NULL,"
				"description VARCHAR(192),"
				"tag VARCHAR(192),"
				"connection VARCHAR(32),"
				"email VARCHAR(96),"
				"UNIQUE (nick)"
			")"
			"DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci"
		) != 0) {
			mysql_close(m_pDBHandle);

#ifdef _WIN32
			::Sleep(1000);
#else
			nanosleep(&sleeptime, NULL);
#endif
			continue;
		}
	
		m_bConnected = true;
		break;
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
