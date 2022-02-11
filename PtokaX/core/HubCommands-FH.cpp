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

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "stdinc.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "HubCommands.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _WITH_SQLITE
	#include "DB-SQLite.h"
	#include <sqlite3.h>
#elif _WITH_POSTGRES
	#include "DB-PostgreSQL.h"
	#include <libpq-fe.h>
#elif _WITH_MYSQL
	#include "DB-MySQL.h"
	#include <mysql.h>
#endif
#include "IP2Country.h"
#include "LuaScript.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::FullBan(ChatCommand * pChatCommand) { // !fullban nick reason
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::BAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 9) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::FullBan", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cfullban <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

	pChatCommand->m_sCommand += 8;
	pChatCommand->m_ui32CommandLen -= 8;

    return Ban(pChatCommand, true);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::FullBanIp(ChatCommand * pChatCommand) { // !fullbanip ip reason
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::BAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 16) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::FullBanIp", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cfullbanip <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

	pChatCommand->m_sCommand += 10;
	pChatCommand->m_ui32CommandLen -= 10;

    return BanIp(pChatCommand, true);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::FullTempBan(ChatCommand * pChatCommand) { // !fulltempban nick time reason ... m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
	if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TEMP_BAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 16) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::FullTempBan", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cfulltempban <%s> <%s> <%s>. %s!|", 
			SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

	pChatCommand->m_sCommand += 12;
	pChatCommand->m_ui32CommandLen -= 12;

    return TempBan(pChatCommand, true);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::FullTempBanIp(ChatCommand * pChatCommand) { // !fulltempbanip ip time reason ... m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TEMP_BAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 24) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::FullTempBanIp", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cfulltempbanip <%s> <%s> <%s>. %s!|", 
			SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

	pChatCommand->m_sCommand += 14;
	pChatCommand->m_ui32CommandLen -= 14;

    return TempBanIp(pChatCommand, true);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::FullRangeBan(ChatCommand * pChatCommand) { // !fullrangeban fromip toip reason
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_BAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 28) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::FullRangeBan", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cfullrangeban <%s> <%s> <%s>. %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

	pChatCommand->m_sCommand += 13;
	pChatCommand->m_ui32CommandLen -= 13;

    return RangeBan(pChatCommand, true);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::FullRangeTempBan(ChatCommand * pChatCommand) { // !fullrangetempban fromip toip time reason ... m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_TBAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 35) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::FullRangeTempBan", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cfullrangetempban <%s> <%s> <%s> <%s>. %s!|", 
			SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], 
			LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

	pChatCommand->m_sCommand += 17;
	pChatCommand->m_ui32CommandLen -= 17;

    return RangeTempBan(pChatCommand, true);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetBans(ChatCommand * pChatCommand) { // !getbans
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GETBANLIST) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    UncountDeflood(pChatCommand);

    int iMsgLen = CheckFromPm(pChatCommand);

    int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "<%s> ", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
    if(iRet <= 0) {
        return true;
    }
    iMsgLen += iRet;

	string BanList(ServerManager::m_pGlobalBuffer, iMsgLen);
    bool bIsEmpty = true;

    if(BanManager::m_Ptr->m_pTempBanListS != NULL) {
        uint32_t ui32BanNum = 0;

        time_t acc_time;
        time(&acc_time);

        BanItem * curBan = NULL,
            * nextBan = BanManager::m_Ptr->m_pTempBanListS;

        while(nextBan != NULL) {
            curBan = nextBan;
            nextBan = curBan->m_pNext;

            if(acc_time > curBan->m_tTempBanExpire) {
				BanManager::m_Ptr->Rem(curBan);
                delete curBan;

				continue;
            }

            if(ui32BanNum == 0) {
				BanList += string(LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BANS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_TEMP_BANS]) + ":\n\n";
            }

            ui32BanNum++;
			BanList += "[ " + string(ui32BanNum) + " ]";

            if(curBan->m_sIp[0] != '\0') {
                if(((curBan->m_ui8Bits & BanManager::IP) == BanManager::IP) == true) {
					BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_BANNED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_BANNED]);
                }
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_IP], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_IP]) + ": " + string(curBan->m_sIp);
                if(((curBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true) {
					BanList += " (" + string(LanguageManager::m_Ptr->m_sTexts[LAN_FULL], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_FULL]) + ")";
                }
            }

            if(curBan->m_sNick != NULL) {
                if(((curBan->m_ui8Bits & BanManager::NICK) == BanManager::NICK) == true) {
					BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_BANNED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_BANNED]);
                }
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_NICK], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_NICK]) + ": " + string(curBan->m_sNick);
            }

            if(curBan->m_sBy != NULL) {
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_BY], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_BY]) + ": " + string(curBan->m_sBy);
            }

            if(curBan->m_sReason != NULL) {
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_REASON], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_REASON]) + ": " + string(curBan->m_sReason);
            }

            struct tm *tm = localtime(&curBan->m_tTempBanExpire);
            strftime(ServerManager::m_pGlobalBuffer, 256, "%c\n", tm);

			BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_EXPIRE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_EXPIRE]) + ": " + string(ServerManager::m_pGlobalBuffer);
        }

        if(ui32BanNum != 0) {
            bIsEmpty = false;
            BanList += "\n\n";
        }
    }

    if(BanManager::m_Ptr->m_pPermBanListS != NULL) {
        bIsEmpty = false;

		BanList += string(LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BANS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_PERM_BANS]) + ":\n\n";

        uint32_t iBanNum = 0;
        BanItem * curBan = NULL,
            * nextBan = BanManager::m_Ptr->m_pPermBanListS;

        while(nextBan != NULL) {
            curBan = nextBan;
            nextBan = curBan->m_pNext;

            iBanNum++;
			BanList += "[ " + string(iBanNum) + " ]";
            
            if(curBan->m_sIp[0] != '\0') {
                if(((curBan->m_ui8Bits & BanManager::IP) == BanManager::IP) == true) {
					BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_BANNED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_BANNED]);
				}
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_IP], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_IP]) + ": " + string(curBan->m_sIp);
                if(((curBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true) {
					BanList += " (" + string(LanguageManager::m_Ptr->m_sTexts[LAN_FULL], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_FULL]) + ")";
                }
            }

            if(curBan->m_sNick != NULL) {
				if(((curBan->m_ui8Bits & BanManager::NICK) == BanManager::NICK) == true) {
					   BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_BANNED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_BANNED]);
                }
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_NICK], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_NICK]) + ": " + string(curBan->m_sNick);
            }

            if(curBan->m_sBy != NULL) {
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_BY], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_BY]) + ": " + string(curBan->m_sBy);
            }

            if(curBan->m_sReason != NULL) {
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_REASON], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_REASON]) + ": " + string(curBan->m_sReason);
            }

            BanList += "\n";
        }
    }

    if(bIsEmpty == true) {
		BanList += string(LanguageManager::m_Ptr->m_sTexts[LAN_NO_BANS_FOUND], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_NO_BANS_FOUND]) + "...|";
    } else {
        BanList += "|";
    }

    pChatCommand->m_pUser->SendCharDelayed(BanList.c_str(), BanList.size());

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Gag(ChatCommand * pChatCommand) { // !gag nick
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GAG) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

   	if(pChatCommand->m_ui32CommandLen < 5 || pChatCommand->m_sCommand[4] == '\0') {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Gag1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cgag <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen > 100) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Gag2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cgag <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        return true;
    }

    pChatCommand->m_sCommand += 4;

    // Self-gag ?
	if(strcasecmp(pChatCommand->m_sCommand, pChatCommand->m_pUser->m_sNick) == 0) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Gag3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_YOU_CANT_GAG_YOURSELF]);
        return true;
    }

    User * pOtherUser = HashManager::m_Ptr->FindUser(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen-4);
	if(pOtherUser == NULL) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Gag4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s: %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], pChatCommand->m_sCommand,  LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_IN_USERLIST]);
		return true;
    }

    if(((pOtherUser->m_ui32BoolBits & User::BIT_GAGGED) == User::BIT_GAGGED) == true) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Gag5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s: %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], pOtherUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_IS_ALREDY_GAGGED]);
		return true;
    }

	// PPK don't gag user with higher profile
	if(pOtherUser->m_i32Profile != -1 && pChatCommand->m_pUser->m_i32Profile > pOtherUser->m_i32Profile) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Gag6", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_NOT_ALW_TO_GAG], pOtherUser->m_sNick);
		return true;
    }

	UncountDeflood(pChatCommand);

    pOtherUser->m_ui32BoolBits |= User::BIT_GAGGED;
    pOtherUser->SendFormat("HubCommands::Gag7", true, "<%s> %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_GAGGED_BY], pChatCommand->m_pUser->m_sNick);

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::Gag8", "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_GAGGED], pOtherUser->m_sNick);
    } 
            
    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Gag9", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pOtherUser->m_sNick, 
			LanguageManager::m_Ptr->m_sTexts[LAN_HAS_GAGGED]);
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetInfo(ChatCommand * pChatCommand) { // !getinfo nick
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GETINFO) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 9 || pChatCommand->m_sCommand[8] == 0) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::GetInfo1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cgetinfo <%s>. %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],  LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen > 100) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::GetInfo2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cgetinfo <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],  LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        return true;
    }

    pChatCommand->m_sCommand += 8;
    pChatCommand->m_ui32CommandLen -= 8;

    User * pOtherUser = HashManager::m_Ptr->FindUser(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen);
    if(pOtherUser == NULL) {
#ifdef _WITH_SQLITE
		if(DBSQLite::m_Ptr->SearchNick(pChatCommand) == true) {
			UncountDeflood(pChatCommand);
			return true;
		}
#elif _WITH_POSTGRES
		if(DBPostgreSQL::m_Ptr->SearchNick(pChatCommand) == true) {
			UncountDeflood(pChatCommand);
			return true;
		}
#elif _WITH_MYSQL
		if(DBMySQL::m_Ptr->SearchNick(pChatCommand) == true) {
			UncountDeflood(pChatCommand);
			return true;
		}
#endif
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::GetInfo3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s: %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_NOT_FOUND]);
        return true;
    }
    
	UncountDeflood(pChatCommand);

    int iMsgLen = CheckFromPm(pChatCommand);

    int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "<%s> \n%s: %s", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_NICK], pOtherUser->m_sNick);
    if(iRet <= 0) {
        return true;
    }
    iMsgLen += iRet;

    if(pOtherUser->m_i32Profile != -1) {
        iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_PROFILE], ProfileManager::m_Ptr->m_ppProfilesTable[pOtherUser->m_i32Profile]->m_sName);
        if(iRet <= 0) {
            return true;
        }
        iMsgLen += iRet;
    }

    iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s ", LanguageManager::m_Ptr->m_sTexts[LAN_STATUS], LanguageManager::m_Ptr->m_sTexts[LAN_ONLINE_FROM]);
    if(iRet <= 0) {
        return true;
    }
    iMsgLen += iRet;

    struct tm *tm = localtime(&pOtherUser->m_tLoginTime);
    iRet = (int)strftime(ServerManager::m_pGlobalBuffer+iMsgLen, 256, "%c", tm);
    if(iRet <= 0) {
        return true;
    }
    iMsgLen += iRet;

	if(pOtherUser->m_sIPv4[0] != '\0') {
		iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s / %s\n%s: %0.02f %s", LanguageManager::m_Ptr->m_sTexts[LAN_IP], pOtherUser->m_sIP, pOtherUser->m_sIPv4, LanguageManager::m_Ptr->m_sTexts[LAN_SHARE_SIZE], (double)pOtherUser->m_ui64SharedSize/1073741824, 
			LanguageManager::m_Ptr->m_sTexts[LAN_GIGA_BYTES]);
		if(iRet <= 0) {
			return true;
		}
		iMsgLen += iRet;
	} else {
		iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s\n%s: %0.02f %s", LanguageManager::m_Ptr->m_sTexts[LAN_IP], pOtherUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_SHARE_SIZE], (double)pOtherUser->m_ui64SharedSize/1073741824, 
			LanguageManager::m_Ptr->m_sTexts[LAN_GIGA_BYTES]);
		if(iRet <= 0) {
			return true;
		}
		iMsgLen += iRet;
	}

    if(pOtherUser->m_sDescription != NULL) {
        iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[LAN_DESCRIPTION]);
        if(iRet <= 0) {
            return true;
        }
        iMsgLen += iRet;
        memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, pOtherUser->m_sDescription, pOtherUser->m_ui8DescriptionLen);
        iMsgLen += pOtherUser->m_ui8DescriptionLen;
    }

    if(pOtherUser->m_sTag != NULL) {
        iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[LAN_TAG]);
        if(iRet <= 0) {
            return true;
        }
        iMsgLen += iRet;
        memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, pOtherUser->m_sTag, pOtherUser->m_ui8TagLen);
        iMsgLen += (int)pOtherUser->m_ui8TagLen;
    }
        
    if(pOtherUser->m_sConnection != NULL) {
		iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[LAN_CONNECTION]);
        if(iRet <= 0) {
            return true;
        }
        iMsgLen += iRet;
        memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, pOtherUser->m_sConnection, pOtherUser->m_ui8ConnectionLen);
        iMsgLen += pOtherUser->m_ui8ConnectionLen;
    }

    if(pOtherUser->m_sEmail != NULL) {
		iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[LAN_EMAIL]);
        if(iRet <= 0) {
            return true;
        }
        iMsgLen += iRet;
        memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, pOtherUser->m_sEmail, pOtherUser->m_ui8EmailLen);
        iMsgLen += pOtherUser->m_ui8EmailLen;
    }

    if(IpP2Country::m_Ptr->m_ui32Count != 0) {
		iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: ", LanguageManager::m_Ptr->m_sTexts[LAN_COUNTRY]);
        if(iRet <= 0) {
            return true;
        }
        iMsgLen += iRet;
        memcpy(ServerManager::m_pGlobalBuffer+iMsgLen, IpP2Country::m_Ptr->GetCountry(pOtherUser->m_ui8Country, false), 2);
        iMsgLen += 2;
    }

    ServerManager::m_pGlobalBuffer[iMsgLen] = '|';
    ServerManager::m_pGlobalBuffer[iMsgLen+1] = '\0';  

    pChatCommand->m_pUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iMsgLen+1);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetIpInfo(ChatCommand * pChatCommand) { // !getipinfo ip
#if !defined(_WITH_SQLITE) && !defined(_WITH_POSTGRES) && !defined(_WITH_MYSQL)
	return false;
#endif
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GETINFO) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 11 || pChatCommand->m_sCommand[10] == 0) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::GetIpInfo1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cgetipinfo <%s>. %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen > 102) {
        pChatCommand->m_pUser->SendFormatCheckPM("HHubCommands::GetIpInfo2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cgetipinfo <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],  LanguageManager::m_Ptr->m_sTexts[LAN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_IP_LEN_39_CHARS]);
        return true;
    }

    pChatCommand->m_sCommand += 10;
    pChatCommand->m_ui32CommandLen -= 10;

	if(isIP(pChatCommand->m_sCommand) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HHubCommands::GetIpInfo3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cgetipinfo <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],  LanguageManager::m_Ptr->m_sTexts[LAN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_NO_VALID_IP_SPECIFIED]);
        return true;
	}

#ifdef _WITH_SQLITE
	if(DBSQLite::m_Ptr->SearchIP(pChatCommand) == true) {
		UncountDeflood(pChatCommand);
		return true;
	}
#elif _WITH_POSTGRES
	if(DBPostgreSQL::m_Ptr->SearchIP(pChatCommand) == true) {
		UncountDeflood(pChatCommand);
		return true;
	}
#elif _WITH_MYSQL
	if(DBMySQL::m_Ptr->SearchIP(pChatCommand) == true) {
		UncountDeflood(pChatCommand);
		return true;
	}
#endif
	pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::GetIpInfo4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s: %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
		LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_NOT_FOUND]);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetTempBans(ChatCommand * pChatCommand) { // !gettempbans
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GETBANLIST) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

	UncountDeflood(pChatCommand);

    int iMsgLen = CheckFromPm(pChatCommand);

    int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "<%s> ", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
    if(iRet <= 0) {
        return true;
    }
    iMsgLen += iRet;

	string BanList = string(ServerManager::m_pGlobalBuffer, iMsgLen);

    if(BanManager::m_Ptr->m_pTempBanListS != NULL) {
        uint32_t ui32BanNum = 0;

        time_t acc_time;
        time(&acc_time);

        BanItem * curBan = NULL,
            * nextBan = BanManager::m_Ptr->m_pTempBanListS;

        while(nextBan != NULL) {
            curBan = nextBan;
            nextBan = curBan->m_pNext;

            if(acc_time > curBan->m_tTempBanExpire) {
				BanManager::m_Ptr->Rem(curBan);
                delete curBan;

				continue;
            }

			if(ui32BanNum == 0) {
				BanList += string(LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BANS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_TEMP_BANS]) + ":\n\n";
            }

            ui32BanNum++;
			BanList += "[ " + string(ui32BanNum) + " ]";

            if(curBan->m_sIp[0] != '\0') {
                if(((curBan->m_ui8Bits & BanManager::IP) == BanManager::IP) == true) {
					BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_BANNED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_BANNED]);
                }
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_IP], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_IP]) + ": " + string(curBan->m_sIp);
                if(((curBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true) {
					BanList += " (" + string(LanguageManager::m_Ptr->m_sTexts[LAN_FULL], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_FULL]) + ")";
                }
            }

            if(curBan->m_sNick != NULL) {
                if(((curBan->m_ui8Bits & BanManager::NICK) == BanManager::NICK) == true) {
					BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_BANNED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_BANNED]);
                }
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_NICK], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_NICK]) + ": " + string(curBan->m_sNick);
            }

			if(curBan->m_sBy != NULL) {
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_BY], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_BY]) + ": " + string(curBan->m_sBy);
            }

            if(curBan->m_sReason != NULL) {
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_REASON], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_REASON]) + ": " + string(curBan->m_sReason);
            }

            struct tm *tm = localtime(&curBan->m_tTempBanExpire);
            strftime(ServerManager::m_pGlobalBuffer, 256, "%c\n", tm);

			BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_EXPIRE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_EXPIRE]) + ": " + string(ServerManager::m_pGlobalBuffer);
        }

        if(ui32BanNum > 0) {
            BanList += "|";
        } else {
			BanList += string(LanguageManager::m_Ptr->m_sTexts[LAN_NO_TEMP_BANS_FOUND], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_NO_TEMP_BANS_FOUND]) + "...|";
        }
    } else {
		BanList += string(LanguageManager::m_Ptr->m_sTexts[LAN_NO_TEMP_BANS_FOUND], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_NO_TEMP_BANS_FOUND]) + "...|";
    }

	pChatCommand->m_pUser->SendCharDelayed(BanList.c_str(), BanList.size());

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetScripts(ChatCommand * pChatCommand) { // !getscripts
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RSTSCRIPTS) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

	UncountDeflood(pChatCommand);

    int iMsgLen = CheckFromPm(pChatCommand);

    int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "<%s> ", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
    if(iRet <= 0) {
        return true;
    }
    iMsgLen += iRet;

	string ScriptList(ServerManager::m_pGlobalBuffer, iMsgLen);

	ScriptList += string(LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPTS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SCRIPTS]) + ":\n\n";

	for(uint8_t ui8i = 0; ui8i < ScriptManager::m_Ptr->m_ui8ScriptCount; ui8i++) {
		ScriptList += "[ " + string(ScriptManager::m_Ptr->m_ppScriptTable[ui8i]->m_bEnabled == true ? "1" : "0") +
			" ] " + string(ScriptManager::m_Ptr->m_ppScriptTable[ui8i]->m_sName);

		if(ScriptManager::m_Ptr->m_ppScriptTable[ui8i]->m_bEnabled == true) {
            ScriptList += " ("+string(ScriptGetGC(ScriptManager::m_Ptr->m_ppScriptTable[ui8i]))+" kB)\n";
        } else {
            ScriptList += "\n";
        }
	}

    ScriptList += "|";
	pChatCommand->m_pUser->SendCharDelayed(ScriptList.c_str(), ScriptList.size());

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetPermBans(ChatCommand * pChatCommand) { // !getpermbans
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GETBANLIST) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

	UncountDeflood(pChatCommand);

    int iMsgLen = CheckFromPm(pChatCommand);

    int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "<%s> ", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
    if(iRet <= 0) {
        return true;
    }
    iMsgLen += iRet;

	string BanList(ServerManager::m_pGlobalBuffer, iMsgLen);

    if(BanManager::m_Ptr->m_pPermBanListS != NULL) {
		BanList += string(LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BANS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_PERM_BANS]) + ":\n\n";

        uint32_t ui32BanNum = 0;

        BanItem * curBan = NULL,
            * nextBan = BanManager::m_Ptr->m_pPermBanListS;

        while(nextBan != NULL) {
            curBan = nextBan;
            nextBan = curBan->m_pNext;

			ui32BanNum++;
			BanList += "[ " + string(ui32BanNum) + " ]";

            if(curBan->m_sIp[0] != '\0') {
                if(((curBan->m_ui8Bits & BanManager::IP) == BanManager::IP) == true) {
					BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_BANNED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_BANNED]);
                }
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_IP], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_IP]) + ": " + string(curBan->m_sIp);
                if(((curBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true) {
					BanList += " (" + string(LanguageManager::m_Ptr->m_sTexts[LAN_FULL], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_FULL]) + ")";
                }
            }

            if(curBan->m_sNick != NULL) {
                if(((curBan->m_ui8Bits & BanManager::NICK) == BanManager::NICK) == true) {
					BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_BANNED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_BANNED]);
                }
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_NICK], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_NICK]) + ": " + string(curBan->m_sNick);
            }

            if(curBan->m_sBy != NULL) {
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_BY], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_BY]) + ": " + string(curBan->m_sBy);
            }

            if(curBan->m_sReason != NULL) {
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_REASON], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_REASON]) + ": " + string(curBan->m_sReason);
            }

            BanList += "\n";
        }

		BanList += "|";
    } else {
		BanList += string(LanguageManager::m_Ptr->m_sTexts[LAN_NO_PERM_BANS_FOUND], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_NO_PERM_BANS_FOUND]) + "...|";
    }

	pChatCommand->m_pUser->SendCharDelayed(BanList.c_str(), BanList.size());

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetRangeBans(ChatCommand * pChatCommand) { // !getrangebans
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GET_RANGE_BANS) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

	UncountDeflood(pChatCommand);

    int iMsgLen = CheckFromPm(pChatCommand);

    int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "<%s> ", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
    if(iRet <= 0) {
        return true;
    }
    iMsgLen += iRet;

	string BanList(ServerManager::m_pGlobalBuffer, iMsgLen);
    bool bIsEmpty = true;

    if(BanManager::m_Ptr->m_pRangeBanListS != NULL) {
        uint32_t ui32BanNum = 0;

        time_t acc_time;
        time(&acc_time);

        RangeBanItem * curBan = NULL,
            * nextBan = BanManager::m_Ptr->m_pRangeBanListS;

        while(nextBan != NULL) {
            curBan = nextBan;
            nextBan = curBan->m_pNext;

            if(((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == false)
                continue;

            if(acc_time > curBan->m_tTempBanExpire) {
				BanManager::m_Ptr->RemRange(curBan);
                delete curBan;

				continue;
            }

            if(ui32BanNum == 0) {
				BanList += string(LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_RANGE_BANS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_TEMP_RANGE_BANS]) + ":\n\n";
            }

            ui32BanNum++;

			BanList += "[ " + string(ui32BanNum) + " ]";
			BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_RANGE]) + ": " + string(curBan->m_sIpFrom) + "-" + string(curBan->m_sIpTo);

            if(((curBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true) {
				BanList += " (" + string(LanguageManager::m_Ptr->m_sTexts[LAN_FULL], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_FULL]) + ")";
            }

            if(curBan->m_sBy != NULL) {
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_BY], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_BY]) + ": " + string(curBan->m_sBy);
            }

            if(curBan->m_sReason != NULL) {
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_REASON], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_REASON]) +
					": " + string(curBan->m_sReason);
            }

            struct tm *tm = localtime(&curBan->m_tTempBanExpire);
            strftime(ServerManager::m_pGlobalBuffer, 256, "%c\n", tm);

			BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_EXPIRE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_EXPIRE]) + ": " + string(ServerManager::m_pGlobalBuffer);
        }

        if(ui32BanNum > 0) {
            bIsEmpty = false;
            BanList += "\n\n";
        }

        ui32BanNum = 0;
        nextBan = BanManager::m_Ptr->m_pRangeBanListS;

        while(nextBan != NULL) {
            curBan = nextBan;
            nextBan = curBan->m_pNext;

            if(((curBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM) == false)
                continue;

            if(ui32BanNum == 0) {
                bIsEmpty = false;
				BanList += string(LanguageManager::m_Ptr->m_sTexts[LAN_PERM_RANGE_BANS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_PERM_RANGE_BANS]) + ":\n\n";
            }

            ui32BanNum++;

			BanList += "[ " + string(ui32BanNum) + " ]";
			BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_RANGE]) + ": " + string(curBan->m_sIpFrom) + "-" + string(curBan->m_sIpTo);

            if(((curBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true) {
				BanList += " (" + string(LanguageManager::m_Ptr->m_sTexts[LAN_FULL], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_FULL]) + ")";
            }

            if(curBan->m_sBy != NULL) {
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_BY], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_BY]) + ": " + string(curBan->m_sBy);
            }

            if(curBan->m_sReason != NULL) {
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_REASON], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_REASON]) +
					": " + string(curBan->m_sReason);
            }

            BanList += "\n";
        }
    }

    if(bIsEmpty == true) {
		BanList += string(LanguageManager::m_Ptr->m_sTexts[LAN_NO_RANGE_BANS_FOUND], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_NO_RANGE_BANS_FOUND]) + "...|";
    } else {
        BanList += "|";
    }

	pChatCommand->m_pUser->SendCharDelayed(BanList.c_str(), BanList.size());

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetRangePermBans(ChatCommand * pChatCommand) { // !getrangepermbans
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GET_RANGE_BANS) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

	UncountDeflood(pChatCommand);

    int iMsgLen = CheckFromPm(pChatCommand);

    int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "<%s> ", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
    if(iRet <= 0) {
        return true;
    }
    iMsgLen += iRet;

	string BanList(ServerManager::m_pGlobalBuffer, iMsgLen);
    bool bIsEmpty = true;

    if(BanManager::m_Ptr->m_pRangeBanListS != NULL) {
        uint32_t ui32BanNum = 0;

        RangeBanItem * curBan = NULL,
            * nextBan = BanManager::m_Ptr->m_pRangeBanListS;

        while(nextBan != NULL) {
            curBan = nextBan;
            nextBan = curBan->m_pNext;

            if(((curBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM) == false)
                continue;

            if(ui32BanNum == 0) {
                bIsEmpty = false;

				BanList += string(LanguageManager::m_Ptr->m_sTexts[LAN_PERM_RANGE_BANS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_PERM_RANGE_BANS]) + ":\n\n";
            }

            ui32BanNum++;

			BanList += "[ " + string(ui32BanNum) + " ]";
			BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_RANGE]) +
				": " + string(curBan->m_sIpFrom) + "-" + string(curBan->m_sIpTo);

            if(((curBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true) {
				BanList += " (" + string(LanguageManager::m_Ptr->m_sTexts[LAN_FULL], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_FULL]) + ")";
            }

            if(curBan->m_sBy != NULL) {
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_BY], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_BY]) + ": " + string(curBan->m_sBy);
            }

            if(curBan->m_sReason != NULL) {
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_REASON], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_REASON]) +
					": " + string(curBan->m_sReason);
            }

            BanList += "\n";
        }
    }

    if(bIsEmpty == true) {
		BanList += string(LanguageManager::m_Ptr->m_sTexts[LAN_NO_RANGE_PERM_BANS_FOUND], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_NO_RANGE_PERM_BANS_FOUND]) + "...|";
    } else {
        BanList += "|";
    }

	pChatCommand->m_pUser->SendCharDelayed(BanList.c_str(), BanList.size());

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::GetRangeTempBans(ChatCommand * pChatCommand) { // !getrangetempbans
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GET_RANGE_BANS) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

	UncountDeflood(pChatCommand);

    int iMsgLen = CheckFromPm(pChatCommand);

    int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "<%s> ", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
    if(iRet <= 0) {
        return true;
    }
    iMsgLen += iRet;

	string BanList(ServerManager::m_pGlobalBuffer, iMsgLen);
    bool bIsEmpty = true;

    if(BanManager::m_Ptr->m_pRangeBanListS != NULL) {
        uint32_t ui32BanNum = 0;

        time_t acc_time;
        time(&acc_time);

        RangeBanItem * curBan = NULL,
            * nextBan = BanManager::m_Ptr->m_pRangeBanListS;

        while(nextBan != NULL) {
            curBan = nextBan;
            nextBan = curBan->m_pNext;

            if(((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == false)
                continue;

            if(acc_time > curBan->m_tTempBanExpire) {
				BanManager::m_Ptr->RemRange(curBan);
                delete curBan;

				continue;
            }

            if(ui32BanNum == 0) {
				BanList += string(LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_RANGE_BANS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_TEMP_RANGE_BANS]) + ":\n\n";
            }

            ui32BanNum++;

			BanList += "[ " + string(ui32BanNum) + " ]";
			BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_RANGE]) +
				": " + string(curBan->m_sIpFrom) + "-" + string(curBan->m_sIpTo);

            if(((curBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true) {
				BanList += " (" + string(LanguageManager::m_Ptr->m_sTexts[LAN_FULL], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_FULL]) + ")";
            }

			if(curBan->m_sBy != NULL) {
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_BY], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_BY]) + ": " + string(curBan->m_sBy);
            }

            if(curBan->m_sReason != NULL) {
				BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_REASON], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_REASON]) +
					": " + string(curBan->m_sReason);
            }

            struct tm *tm = localtime(&curBan->m_tTempBanExpire);
            strftime(ServerManager::m_pGlobalBuffer, 256, "%c\n", tm);

			BanList += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_EXPIRE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_EXPIRE]) + ": " + string(ServerManager::m_pGlobalBuffer);
        }

        if(ui32BanNum != 0) {
            bIsEmpty = false;
        }
    }

    if(bIsEmpty == true) {
		BanList += string(LanguageManager::m_Ptr->m_sTexts[LAN_NO_RANGE_TEMP_BANS_FOUND], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_NO_RANGE_TEMP_BANS_FOUND]) + "...|";
    } else {
        BanList += "|";
    }

	pChatCommand->m_pUser->SendCharDelayed(BanList.c_str(), BanList.size());

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Help(ChatCommand * pChatCommand) { // !help
    int iMsgLen = CheckFromPm(pChatCommand);

    int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize+iMsgLen, "<%s> ", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
    if(iRet <= 0) {
        return true;
    }
    iMsgLen += iRet;

	string help(ServerManager::m_pGlobalBuffer, iMsgLen);
    bool bFull = false;
    bool bTemp = false;

    iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s:\n", LanguageManager::m_Ptr->m_sTexts[LAN_FOLOW_COMMANDS_AVAILABLE_TO_YOU]);
    if(iMsgLen <= 0) {
        return true;
    }
	help += ServerManager::m_pGlobalBuffer;

    if(pChatCommand->m_pUser->m_i32Profile != -1) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\n%s:\n", LanguageManager::m_Ptr->m_sTexts[LAN_PROFILE_SPECIFIC_CMDS]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cpasswd <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NEW_PASSWORD], LanguageManager::m_Ptr->m_sTexts[LAN_CHANGE_YOUR_PASSWORD]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::BAN)) {
        bFull = true;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cban <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR],
            LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN_USER_GIVEN_NICK_DISCONNECT]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cbanip <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP],
            LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN_IP_ADDRESS]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cfullban <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR],
            LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN_USER_GIVEN_NICK_DISCONNECT]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cfullbanip <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP],
            LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN_IP_ADDRESS]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cnickban <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR],
            LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_BAN_USERS_NICK_IFCONN_THENDISCONN]);
        if(iMsgLen <= 0) {
            return true;
        }
        help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TEMP_BAN)) {
        bFull = true; bTemp = true;

		iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%ctempban <%s> <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR],
            LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN_USER_GIVEN_NICK_DISCONNECT]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%ctempbanip <%s> <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP],
            LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN_IP_ADDRESS]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cfulltempban <%s> <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR],
            LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN_USER_GIVEN_NICK_DISCONNECT]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cfulltempbanip <%s> <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP],
            LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN_IP_ADDRESS]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cnicktempban <%s> <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR],
            LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN_USERS_NICK_IFCONN_THENDISCONN]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::UNBAN) || ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TEMP_UNBAN)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cunban <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP_OR_NICK],
            LanguageManager::m_Ptr->m_sTexts[LAN_UNBAN_IP_OR_NICK]);
        if(iMsgLen <= 0) {
            return true;
		}
		help += ServerManager::m_pGlobalBuffer;
	}

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::UNBAN)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cpermunban <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP_OR_NICK],
            LanguageManager::m_Ptr->m_sTexts[LAN_UNBAN_PERM_BANNED_IP_OR_NICK]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TEMP_UNBAN)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%ctempunban <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP_OR_NICK],
            LanguageManager::m_Ptr->m_sTexts[LAN_UNBAN_TEMP_BANNED_IP_OR_NICK]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GETBANLIST)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cgetbans - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_DISPLAY_LIST_OF_BANS]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cgetpermbans - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_DISPLAY_LIST_OF_PERM_BANS]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cgettempbans - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_DISPLAY_LIST_OF_TEMP_BANS]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::CLRPERMBAN)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cclrpermbans - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_CLEAR_PERM_BANS_LWR]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::CLRTEMPBAN)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cclrtempbans - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_CLEAR_TEMP_BANS_LWR]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_BAN)) {
        bFull = true;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%crangeban <%s> <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP],
            LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN_GIVEN_IP_RANGE]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cfullrangeban <%s> <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP],
            LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BAN_GIVEN_IP_RANGE]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_TBAN)) {
        bFull = true; bTemp = true;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%crangetempban <%s> <%s> <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], 
			LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN_GIVEN_IP_RANGE]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cfullrangetempban <%s> <%s> <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], 
			LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN_GIVEN_IP_RANGE]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_UNBAN) || ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_TUNBAN)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%crangeunban <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP],
            LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], LanguageManager::m_Ptr->m_sTexts[LAN_UNBAN_BANNED_IP_RANGE]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_UNBAN)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%crangepermunban <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP],
            LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], LanguageManager::m_Ptr->m_sTexts[LAN_UNBAN_PERM_BANNED_IP_RANGE]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_TUNBAN)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%crangetempunban <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP],
            LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], LanguageManager::m_Ptr->m_sTexts[LAN_UNBAN_TEMP_BANNED_IP_RANGE]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GET_RANGE_BANS)) {
        bFull = true;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cgetrangebans - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_DISPLAY_LIST_OF_RANGE_BANS]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cgetrangepermbans - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_DISPLAY_LIST_OF_RANGE_PERM_BANS]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cgetrangetempbans - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_DISPLAY_LIST_OF_RANGE_TEMP_BANS]);
        if(iMsgLen <= 0) {
            return true;
        }
        help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::CLR_RANGE_BANS)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cclrrangepermbans - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_CLEAR_PERM_RANGE_BANS_LWR]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::CLR_RANGE_TBANS)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cclrrangetempbans - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_CLEAR_TEMP_RANGE_BANS_LWR]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GETBANLIST)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cchecknickban <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_DISPLAY_BAN_FOUND_FOR_GIVEN_NICK]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%ccheckipban <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_DISPLAY_BANS_FOUND_FOR_GIVEN_IP]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GET_RANGE_BANS)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%ccheckrangeban <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], 
			LanguageManager::m_Ptr->m_sTexts[LAN_DISPLAY_RANGE_BAN_FOR_GIVEN_RANGE]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::DROP)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cdrop <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_DISCONNECT_WITH_TEMPBAN]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GETINFO)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cgetinfo <%s> - %s."
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
			" %s."
#endif
			"\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_DISPLAY_INFO_GIVEN_NICK]
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
			, LanguageManager::m_Ptr->m_sTexts[LAN_YOU_CAN_USE_SQL_WILDCARDS]
#endif
		);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cgetipinfo <%s> - %s. %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP],
            LanguageManager::m_Ptr->m_sTexts[LAN_DISPLAY_INFO_GIVEN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_CAN_USE_SQL_WILDCARDS]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
#endif
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TEMPOP)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cop <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_GIVE_TEMP_OP]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GAG)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cgag <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_DISALLOW_USER_TO_POST_IN_MAIN]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cungag <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_USER_CAN_POST_IN_MAIN_AGAIN]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RSTHUB)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%crestart - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_RESTART_HUB_LWR]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RSTSCRIPTS)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cstartscript <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FILENAME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_START_SCRIPT_GIVEN_FILENAME]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cstopscript <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FILENAME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_STOP_SCRIPT_GIVEN_FILENAME]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%crestartscript <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FILENAME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_RESTART_SCRIPT_GIVEN_FILENAME]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%crestartscripts - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_RESTART_SCRIPTING_PART_OF_THE_HUB]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cgetscripts - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_DISPLAY_LIST_OF_SCRIPTS]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::REFRESHTXT)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%creloadtxt - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_RELOAD_ALL_TEXT_FILES]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::ADDREGUSER)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%creguser <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR],
            LanguageManager::m_Ptr->m_sTexts[LAN_PROFILENAME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REG_USER_WITH_PROFILE]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%caddreguser <%s> <%s> <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_PASSWORD_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_PROFILENAME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_ADD_REG_USER_WITH_PROFILE]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::DELREGUSER)) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cdelreguser <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REMOVE_REG_USER]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TOPIC) == true) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%ctopic <%s> - %s %ctopic <off> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NEW_TOPIC], LanguageManager::m_Ptr->m_sTexts[LAN_SET_NEW_TOPIC_OR], 
			SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_CLEAR_TOPIC]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::MASSMSG) == true) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cmassmsg <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_MESSAGE_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_SEND_MSG_TO_ALL_USERS]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%copmassmsg <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_MESSAGE_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_SEND_MSG_TO_ALL_OPS]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(bFull == true) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "*** %s.\n", LanguageManager::m_Ptr->m_sTexts[LAN_REASON_IS_ALWAYS_OPTIONAL]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;

        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "*** %s.\n", LanguageManager::m_Ptr->m_sTexts[LAN_FULLBAN_HELP_TXT]);
        if(iMsgLen <= 0) {
            return true;
        }
		help += ServerManager::m_pGlobalBuffer;
    }

    if(bTemp == true) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "*** %s: m = %s, h = %s, d = %s, w = %s, M = %s, Y = %s.\n", LanguageManager::m_Ptr->m_sTexts[LAN_TEMPBAN_TIME_VALUES], LanguageManager::m_Ptr->m_sTexts[LAN_MINUTES_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_HOURS_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_DAYS_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_WEEKS_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_MONTHS_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_YEARS_LWR]);
        if(iMsgLen <= 0) {
            return true;
        }
        help += ServerManager::m_pGlobalBuffer;
    }

    iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\n%s:\n", LanguageManager::m_Ptr->m_sTexts[LAN_GLOBAL_COMMANDS]);
    if(iMsgLen <= 0) {
        return true;
    }
	help += ServerManager::m_pGlobalBuffer;

    iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cme <%s> - %s.\n", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_MESSAGE_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_SPEAK_IN_3RD_PERSON]);
    if(iMsgLen <= 0) {
        return true;
    }
	help += ServerManager::m_pGlobalBuffer;

    iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\t%cmyip - %s.|", SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_SHOW_YOUR_IP]);
    if(iMsgLen <= 0) {
        return true;
    }
	help += ServerManager::m_pGlobalBuffer;

	pChatCommand->m_pUser->SendCharDelayed(help.c_str(), help.size());

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
