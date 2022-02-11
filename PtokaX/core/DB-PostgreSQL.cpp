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
#include "DB-PostgreSQL.h"
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
#include <libpq-fe.h>
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
DBPostgreSQL * DBPostgreSQL::m_Ptr = NULL;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

DBPostgreSQL::DBPostgreSQL() :
#ifdef _WIN32
    m_hThreadHandle(NULL),
#else
    m_threadId(0),
#endif
	m_bConnected(false), m_bTerminated(false) {

	if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_DATABASE] == false || SettingManager::m_Ptr->m_sTexts[SETTXT_POSTGRES_PASS] == NULL) {
		return;
	}

	// Connect to PostgreSQL with our settings.
    const char * sKeyWords[] = { "host", "port", "dbname", "user", "password", NULL };
    const char * sValues[] = { SettingManager::m_Ptr->m_sTexts[SETTXT_POSTGRES_HOST], SettingManager::m_Ptr->m_sTexts[SETTXT_POSTGRES_PORT], SettingManager::m_Ptr->m_sTexts[SETTXT_POSTGRES_DBNAME], SettingManager::m_Ptr->m_sTexts[SETTXT_POSTGRES_USER], SettingManager::m_Ptr->m_sTexts[SETTXT_POSTGRES_PASS], NULL };
    
	m_pDBConn = PQconnectdbParams(sKeyWords, sValues, 0);
	
	if(PQstatus(m_pDBConn) == CONNECTION_BAD) { // Connection to PostgreSQL failed. Save error to log and give up.
		AppendLog((string("DBPostgreSQL connection failed: ")+PQerrorMessage(m_pDBConn)).c_str());
		PQfinish(m_pDBConn);

		return;
	}

	/*	Connection to PostgreSQL was created.
		Now we check if our table exist in database.
		If not then we create table.
		Then we created unique index for column nick based on lower(nick). That will make it case insensitive. */
	PGresult * pResult = PQexec(m_pDBConn,
		"DO $$\n"
			"BEGIN\n"
				"IF NOT EXISTS ("
					"SELECT * FROM pg_catalog.pg_tables WHERE tableowner = 'ptokax' AND tablename = 'userinfo'"
				") THEN\n"
					"CREATE TABLE userinfo ("
						"nick VARCHAR(64) NOT NULL PRIMARY KEY,"
						"last_updated TIMESTAMPTZ NOT NULL,"
						"ip_address VARCHAR(39) NOT NULL,"
						"share VARCHAR(24) NOT NULL,"
						"description VARCHAR(192),"
						"tag VARCHAR(192),"
						"connection VARCHAR(32),"
						"email VARCHAR(96)"
					");"
					"CREATE UNIQUE INDEX nick_unique ON userinfo(LOWER(nick));"
				"END IF;"
			"END;"
		"$$;"
	);

	if(PQresultStatus(pResult) != PGRES_COMMAND_OK) { // Table exist or was succesfully created.
		AppendLog((string("DBPostgreSQL check/create table failed: ")+PQresultErrorMessage(pResult)).c_str());
		PQclear(pResult);
		PQfinish(m_pDBConn);

		return;
	}

	PQclear(pResult);

	m_bConnected = true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
DBPostgreSQL::~DBPostgreSQL() {
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
		PQfinish(m_pDBConn);
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Now that important part. Function to update or insert user to database.
void DBPostgreSQL::UpdateRecord(User * pUser) {
	if(m_bConnected == false) {
		return;
	}

	/*	Prepare params for PQexecParams.
		With this function params are separated from command string and we don't need to do annoying quoting and escaping for them.
		Of course we do conversion to utf-8 when needed. */
	char * paramValues[7];
	memset(&paramValues, 0, sizeof(paramValues));


	char sNick[65];
	paramValues[0] = sNick;
	if(TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sNick, pUser->m_ui8NickLen, sNick, 65) == 0) {
		return;
	}

	paramValues[1] = pUser->m_sIP;

	char sShare[24];
	if(snprintf(sShare, 24, "%0.02f GB", (double)pUser->m_ui64SharedSize/1073741824) <= 0) {
		return;
	}

	paramValues[2] = sShare;

	char sDescription[193];
	if(pUser->m_sDescription != NULL && TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sDescription, pUser->m_ui8DescriptionLen, sDescription, 193) != 0) {
		paramValues[3] = sDescription;
	}

	char sTag[193];
	if(pUser->m_sTag != NULL && TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sTag, pUser->m_ui8TagLen, sTag, 193) != 0) {
		paramValues[4] = sTag;
	}

	char sConnection[33];
	if(pUser->m_sConnection != NULL && TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sConnection, pUser->m_ui8ConnectionLen, sConnection, 33) != 0) {
		paramValues[5] = sConnection;
	}

	char sEmail[97];
	if(pUser->m_sEmail != NULL && TextConverter::m_Ptr->CheckUtf8AndConvert(pUser->m_sEmail, pUser->m_ui8EmailLen, sEmail, 97) != 0) {
		paramValues[6] = sEmail;
	}

	/*	PostgreSQL don't support UPSERT.
		So we need to do it hard way.
		First try UPDATE.
		Timestamp is truncated to seconds. */
	PGresult * pResult = PQexecParams(m_pDBConn,
		"UPDATE userinfo SET "
		"last_updated = DATE_TRUNC('second', NOW())," // last_updated
		"ip_address = $2," // ip
		"share = $3," // share
		"description = $4," // description
		"tag = $5," // tag
		"connection = $6," // connection
		"email = $7" // email
		"WHERE LOWER(nick) = LOWER($1);", // nick
		7,
		NULL,
		paramValues,
		NULL,
		NULL,
		0
	);

	ExecStatusType estRet = PQresultStatus(pResult);
	if(estRet != PGRES_COMMAND_OK) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBPostgreSQL update record failed: %s", PQresultErrorMessage(pResult));
		if(estRet == PGRES_FATAL_ERROR) {
			ReconnectDb();
		}
	}

	char * sRows = PQcmdTuples(pResult);

	/*	UPDATE should always return OK, but that not mean anything was changed.
		We need to check how much rows was changed.
		When 0 then it means that user not exist in table. */
	if(sRows[0] != '0' || sRows[1] != '\0') {
		PQclear(pResult);
		return;
	}
	
	PQclear(pResult);

	// When update changed 0 rows then we need to insert user to table.
	pResult = PQexecParams(m_pDBConn,
		"INSERT INTO userinfo (nick, last_updated, ip_address, share, description, tag, connection, email) VALUES ("
		"$1," // nick
		"DATE_TRUNC('second', NOW())," // last_updated
		"$2," // ip
		"$3," // share
		"$4," // description
		"$5," // tag
		"$6," // connection
		"$7" // email
		");",
		7,
		NULL,
		paramValues,
		NULL,
		NULL,
		0
	);

	estRet = PQresultStatus(pResult);
	if(estRet != PGRES_COMMAND_OK) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBPostgreSQL insert record failed: %s", PQresultErrorMessage(pResult));
		if(estRet == PGRES_FATAL_ERROR) {
			ReconnectDb();
		}
	}

	PQclear(pResult);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function to send data returned by SELECT to Operator who requested them.
bool SendQueryResults(ChatCommand * pChatCommand, PGresult * pResult, int &iTuples) {
	if(iTuples == 1) { // When we have only one result then we send whole info about user.
		int iMsgLen = 0;

		if(pChatCommand->m_bFromPM == true) {
    		iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$To: %s From: %s $", pChatCommand->m_pUser->m_sNick, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
    		if(iMsgLen <= 0) {
        		return false;
    		}
    	}

		int iLength = PQgetlength(pResult, 0, 0);
		if(iLength <= 0 || iLength > 64) {
			UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid nick length: %d", iLength);
			return false;
		}

		char * sValue = PQgetvalue(pResult, 0, 0);
        int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "<%s> \n%s: %s", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_NICK], sValue);
        if(iRet <= 0) {
			return false;
        }
        iMsgLen += iRet;

		RegUser * pReg = RegManager::m_Ptr->Find(sValue, iLength);
        if(pReg != NULL) {
            iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_PROFILE], ProfileManager::m_Ptr->m_ppProfilesTable[pReg->m_ui16Profile]->m_sName);
        	if(iRet <= 0) {
        		return false;
            }
            iMsgLen += iRet;
        }

		// In case when SQL wildcards were used is possible that user is online. Then we don't use data from database, but data that are in server memory.
		User * pOnlineUser = HashManager::m_Ptr->FindUser(sValue, iLength);
		if(pOnlineUser != NULL) {
            iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s ", LanguageManager::m_Ptr->m_sTexts[LAN_STATUS], LanguageManager::m_Ptr->m_sTexts[LAN_ONLINE_FROM]);
        	if(iRet <= 0) {
        		return false;
            }
            iMsgLen += iRet;

            struct tm *tm = localtime(&pOnlineUser->m_tLoginTime);
            iRet = (int)strftime(ServerManager::m_pGlobalBuffer+iMsgLen, 256, "%c", tm);
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
        	time_t tTime = (time_t)_strtoui64(PQgetvalue(pResult, 0, 1), NULL, 10);
#else
			time_t tTime = (time_t)strtoull(PQgetvalue(pResult, 0, 1), NULL, 10);
#endif
            struct tm *tm = localtime(&tTime);
            iRet = (int)strftime(ServerManager::m_pGlobalBuffer+iMsgLen, 256, "%c", tm);
            if(iRet <= 0) {
        		return false;
            }
            iMsgLen += iRet;

			iLength = PQgetlength(pResult, 0, 2);
			if(iLength <= 0 || iLength > 39) {
				UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid ip length: %d", iLength);
				return false;
			}

			iLength = PQgetlength(pResult, 0, 3);
			if(iLength <= 0 || iLength > 24) {
				UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid share length: %d", iLength);
				return false;
			}

			char * sIP = PQgetvalue(pResult, 0, 2);

			iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_IP], sIP, LanguageManager::m_Ptr->m_sTexts[LAN_SHARE_SIZE], PQgetvalue(pResult, 0, 3));
            if(iRet <= 0) {
        		return false;
			}
			iMsgLen += iRet;

            if(PQgetisnull(pResult, 0, 4) == 0) {
				iLength = PQgetlength(pResult, 0, 4);
				if(iLength > 192) {
					UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid description length: %d", iLength);
					return  false;
				}

				if(iLength > 0) {
                	iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_DESCRIPTION], PQgetvalue(pResult, 0, 4));
            		if(iRet <= 0) {
        				return false;
        			}
        			iMsgLen += iRet;
                }
            }

            if(PQgetisnull(pResult, 0, 5) == 0) {
				iLength = PQgetlength(pResult, 0, 5);
				if(iLength > 192) {
					UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid tag length: %d", iLength);
					return false;
				}

				if(iLength > 0) {
                	iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_TAG], PQgetvalue(pResult, 0, 5));
            		if(iRet <= 0) {
                    	return false;
                    }
                    iMsgLen += iRet;
                }
            }
                    
            if(PQgetisnull(pResult, 0, 6) == 0) {
				iLength = PQgetlength(pResult, 0, 6);
				if(iLength > 32) {
					UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid connection length: %d", iLength);
					return false;
				}

				if(iLength > 0) {
                	iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_CONNECTION], PQgetvalue(pResult, 0, 6));
            		if(iRet <= 0) {
                    	return false;
                    }
                    iMsgLen += iRet;
                }
            }

            if(PQgetisnull(pResult, 0, 7) == 0) {
				iLength = PQgetlength(pResult, 0, 7);
				if(iLength > 96) {
					UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid email length: %d", iLength);
					return false;
				}

				if(iLength > 0) {
                	iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_EMAIL], PQgetvalue(pResult, 0, 7));
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

		for(int iActualTuple = 0; iActualTuple < iTuples; iActualTuple++) {
			int iLength = PQgetlength(pResult, iActualTuple, 0);
			if(iLength <= 0 || iLength > 64) {
				UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid nick length: %d", iLength);
				return false;
			}

			iLength = PQgetlength(pResult, iActualTuple, 2);
			if(iLength <= 0 || iLength > 39) {
				UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBPostgreSQL search returned invalid ip length: %d", iLength);
				return false;
			}

        	int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s\t\t%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_NICK], PQgetvalue(pResult, iActualTuple, 0), LanguageManager::m_Ptr->m_sTexts[LAN_IP], PQgetvalue(pResult, iActualTuple, 2));
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
bool DBPostgreSQL::SearchNick(ChatCommand * pChatCommand) {
	if(m_bConnected == false) {
		return false;
	}

	// Prepare param for PQexecParams.
	char * paramValues[1] = { NULL };

	char sUtfNick[65];
	paramValues[0] = sUtfNick;
	if(TextConverter::m_Ptr->CheckUtf8AndConvert(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen, sUtfNick, 65) == 0) {
		return false;
	}

	/*	Run SELECT command.
		We need timestamp as epoch.
		That allow us to use that result for standard C date/time formating functions and send result to user correctly formated using system locales.
		LIKE allow us to use SQL wildcards. Very usefull :)
		And we will return max 50 results sorted by timestamp from newest. */		
	PGresult * pResult = PQexecParams(m_pDBConn, "SELECT nick, EXTRACT(EPOCH FROM last_updated), ip_address, share, description, tag, connection, email FROM userinfo WHERE LOWER(nick) LIKE LOWER($1) ORDER BY last_updated DESC LIMIT 50;", 1, NULL, paramValues, NULL, NULL, 0);

	ExecStatusType estRet = PQresultStatus(pResult);
	if(estRet != PGRES_TUPLES_OK) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBPostgreSQL search for nick failed: %s", PQresultErrorMessage(pResult));
		PQclear(pResult);

		if(estRet == PGRES_FATAL_ERROR) {
			ReconnectDb();
		}

		return false;
	}

	int iTuples = PQntuples(pResult);

	if(iTuples == 0) {
		PQclear(pResult);
        return false;
	}

	bool bResult = SendQueryResults(pChatCommand, pResult, iTuples);

	PQclear(pResult);

	return bResult;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Second of two fnctions to search data in database. Now using IP.
bool DBPostgreSQL::SearchIP(ChatCommand * pChatCommand) {
	if(m_bConnected == false) {
		return false;
	}

	// Prepare param for PQexecParams.
	char * paramValues[1] = { pChatCommand->m_sCommand };

	//	Run SELECT command. Similar as in SearchNick.
	PGresult * pResult = PQexecParams(m_pDBConn, "SELECT nick, EXTRACT(EPOCH FROM last_updated), ip_address, share, description, tag, connection, email FROM userinfo WHERE ip_address LIKE $1 ORDER BY last_updated DESC LIMIT 50;", 1, NULL, paramValues, NULL, NULL, 0);

	ExecStatusType estRet = PQresultStatus(pResult);
	if(estRet != PGRES_TUPLES_OK) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBPostgreSQL search for IP failed: %s", PQresultErrorMessage(pResult));
		PQclear(pResult);

		if(estRet == PGRES_FATAL_ERROR) {
			ReconnectDb();
		}

		return false;
	}

	int iTuples = PQntuples(pResult);

	if(iTuples == 0) {
		PQclear(pResult);
        return false;
	}

	bool bResult = SendQueryResults(pChatCommand, pResult, iTuples);

	PQclear(pResult);

	return bResult;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function to remove X days old records from database.
void DBPostgreSQL::RemoveOldRecords(const uint16_t ui16Days) {
	if(m_bConnected == false) {
		return;
	}

	// Prepare X days old time in C
	time_t acc_time = 0;
    time(&acc_time);

    struct tm *tm = localtime(&acc_time);

    tm->tm_mday -= ui16Days;
    tm->tm_isdst = -1;

    time_t tTime = mktime(tm);

    if(tTime == (time_t)-1) {
        return;
    }

	// Prepare param for PQexecParams.
	string sTimeStamp((uint64_t)tTime);
	char * paramValues[1] = { sTimeStamp.c_str() };

	// Run DELETE command. It is simple. We given timestamp in epoch and to_timestamp will convert it to PostgreSQL timestamp format.
	PGresult * pResult = PQexecParams(m_pDBConn, "DELETE FROM userinfo WHERE last_updated < to_timestamp($1);", 1, NULL, paramValues, NULL, NULL, 0);

	if(PQresultStatus(pResult) != PGRES_COMMAND_OK) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBPostgreSQL remove old records failed: %s", PQresultErrorMessage(pResult));
	}

	char * sRows = PQcmdTuples(pResult);

	/*	When non-zero rows was affected then send info to UDP debug */
	if(sRows[0] != '0' || sRows[1] != '\0') {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] DBPostgreSQL removed old records: %s", sRows);
	}

	PQclear(pResult);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#ifdef _WIN32
unsigned __stdcall ExecuteReconnect(void * pThread) {
#else
static void* ExecuteReconnect(void * pThread) {
#endif
	(reinterpret_cast<DBPostgreSQL *>(pThread))->RunReconnect();

	return 0;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void DBPostgreSQL::ReconnectDb()
{
	m_bConnected = false;

	PQfinish(m_pDBConn);

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
		AppendDebugLog("%s - [ERR] Failed to create new PostgreSQL ReconnectDb thread\n");
    }	
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void DBPostgreSQL::RunReconnect() {
	if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_DATABASE] == false) {
		return;
	}

	char sHost[257], sPort[6], sDbName[257], sUser[257], sPass[257];
	SettingManager::m_Ptr->GetText(SETTXT_POSTGRES_HOST, sHost);
	SettingManager::m_Ptr->GetText(SETTXT_POSTGRES_PORT, sPort);
	SettingManager::m_Ptr->GetText(SETTXT_POSTGRES_DBNAME, sDbName);
	SettingManager::m_Ptr->GetText(SETTXT_POSTGRES_USER, sUser);
	SettingManager::m_Ptr->GetText(SETTXT_POSTGRES_PASS, sPass);

	if(sHost[0] == '\0' || sPort[0] == '\0' || sDbName[0] == '\0' || sUser[0] == '\0' || sPass[0] == '\0') {
		return;
	}

	// Connect to PostgreSQL with our settings.
    const char * sKeyWords[] = { "host", "port", "dbname", "user", "password", NULL };
    const char * sValues[] = { sHost, sPort, sDbName, sUser, sPass, NULL };

#ifndef _WIN32
    struct timespec sleeptime;
    sleeptime.tv_sec = 1;
    sleeptime.tv_nsec = 0;
#endif

	while(m_bTerminated == false) {
		m_pDBConn = PQconnectdbParams(sKeyWords, sValues, 0);
		
		if(PQstatus(m_pDBConn) == CONNECTION_BAD) { // Connection to PostgreSQL failed...
			PQfinish(m_pDBConn);

#ifdef _WIN32
			::Sleep(1000);
#else
			nanosleep(&sleeptime, NULL);
#endif
			continue;
		}
	
		/*	Connection to PostgreSQL was created.
			Now we check if our table exist in database.
			If not then we create table.
			Then we created unique index for column nick based on lower(nick). That will make it case insensitive. */
		PGresult * pResult = PQexec(m_pDBConn,
			"DO $$\n"
				"BEGIN\n"
					"IF NOT EXISTS ("
						"SELECT * FROM pg_catalog.pg_tables WHERE tableowner = 'ptokax' AND tablename = 'userinfo'"
					") THEN\n"
						"CREATE TABLE userinfo ("
							"nick VARCHAR(64) NOT NULL PRIMARY KEY,"
							"last_updated TIMESTAMPTZ NOT NULL,"
							"ip_address VARCHAR(39) NOT NULL,"
							"share VARCHAR(24) NOT NULL,"
							"description VARCHAR(192),"
							"tag VARCHAR(192),"
							"connection VARCHAR(32),"
							"email VARCHAR(96)"
						");"
						"CREATE UNIQUE INDEX nick_unique ON userinfo(LOWER(nick));"
					"END IF;"
				"END;"
			"$$;"
		);
	
		if(PQresultStatus(pResult) != PGRES_COMMAND_OK) { // Table exist or was succesfully created.
			PQclear(pResult);
			PQfinish(m_pDBConn);

#ifdef _WIN32
			::Sleep(1000);
#else
			nanosleep(&sleeptime, NULL);
#endif
			continue;
		}
	
		PQclear(pResult);
	
		m_bConnected = true;
		break;
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
