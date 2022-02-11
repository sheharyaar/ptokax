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
#include "colUsers.h"
#include "DcCommands.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaInc.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "serviceLoop.h"
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

#include "LuaScript.h"
#include "TextFileManager.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RestartScripts(ChatCommand * pChatCommand) { // !restartscripts
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RSTSCRIPTS) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RestartScripts1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERR_SCRIPTS_DISABLED]);
        return true;
    }

	UncountDeflood(pChatCommand);

    // post restart message
    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
   	    GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::RestartScripts2", "<%s> *** %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_RESTARTED_SCRIPTS]);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RestartScripts3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPTS_RESTARTED]);
    }

	ScriptManager::m_Ptr->Restart();

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Restart(ChatCommand * pChatCommand) { // !restart
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RSTHUB) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

	UncountDeflood(pChatCommand);

    // Send message to all that we are going to restart the hub
    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> %s. %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_HUB_WILL_BE_RESTARTED], LanguageManager::m_Ptr->m_sTexts[LAN_BACK_IN_FEW_MINUTES]);
    if(iMsgLen > 0) {
        Users::m_Ptr->SendChat2All(pChatCommand->m_pUser, ServerManager::m_pGlobalBuffer, iMsgLen, NULL);
    }

    // post a restart hub message
    EventQueue::m_Ptr->AddNormal(EventQueue::EVENT_RESTART, NULL);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::ReloadTxt(ChatCommand * pChatCommand) { // !reloadtxt
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::REFRESHTXT) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

   	if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_TEXT_FILES] == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::ReloadTxt1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_TXT_SUP_NOT_ENABLED]);
        return true;
    }

	UncountDeflood(pChatCommand);

   	TextFilesManager::m_Ptr->RefreshTextFiles();

   	if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::ReloadTxt2", "<%s> *** %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_RELOAD_TXT_FILES_LWR]);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::ReloadTxt3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_TXT_FILES_RELOADED]);
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RestartScript(ChatCommand * pChatCommand) { // !restartscript scriptname
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RSTSCRIPTS) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RestartScript1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERR_SCRIPTS_DISABLED]);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 15 || pChatCommand->m_sCommand[14] == '\0') {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RestartScript2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %crestartscript <%s>. %s!|", 
			SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPTNAME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

    pChatCommand->m_sCommand += 14;
    pChatCommand->m_ui32CommandLen -= 14;

    if(pChatCommand->m_ui32CommandLen > 256) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RestartScript3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %crestartscript <%s>. %s!|", 
			SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPTNAME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_SCRIPT_NAME_LEN_256_CHARS]);
        return true;
    }

    Script * curScript = ScriptManager::m_Ptr->FindScript(pChatCommand->m_sCommand);
    if(curScript == NULL || curScript->m_bEnabled == false || curScript->m_pLua == NULL) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RestartScript4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR_SCRIPT], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_NOT_RUNNING]);
        return true;
	}

	UncountDeflood(pChatCommand);

	// stop script
	ScriptManager::m_Ptr->StopScript(curScript, false);

	// try to start script
	if(ScriptManager::m_Ptr->StartScript(curScript, false) == true) {
        if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
            GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::RestartScript5", "<%s> *** %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_RESTARTED_SCRIPT], pChatCommand->m_sCommand);
        }

        if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RestartScript6", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPT], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_RESTARTED_LWR]);
        } 
        return true;
	} else {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RestartScript7", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR_SCRIPT], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_RESTART_FAILED]);
        return true;
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RangeBan(ChatCommand * pChatCommand) { // !rangeban fromip toip reason
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_BAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 24) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeBan", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %crangeban <%s> <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

	pChatCommand->m_sCommand += 9;
	pChatCommand->m_ui32CommandLen -= 9;

    return RangeBan(pChatCommand, false);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RangeTempBan(ChatCommand * pChatCommand) { // !rangetempban fromip toip time reason ... m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_TBAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 31) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeTempBan", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %crangetempban <%s> <%s> <%s> <%s>. %s!|", 
			SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP],  
			LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

	pChatCommand->m_sCommand += 13;
	pChatCommand->m_ui32CommandLen -= 13;

    return RangeTempBan(pChatCommand, false);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RangeUnBan(ChatCommand * pChatCommand) { // !rangeunban fromip toip
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_UNBAN) == false && ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_TUNBAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 26) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnBan", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %crangeunban <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

	pChatCommand->m_sCommand += 11;
	pChatCommand->m_ui32CommandLen -= 11;
	
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_UNBAN) == true) {
        return RangeUnban(pChatCommand);
    } else if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_TUNBAN) == true) {
        return RangeUnban(pChatCommand, BanManager::TEMP);
    } else {
        SendNoPermission(pChatCommand);
        return true;
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RangeTempUnBan(ChatCommand * pChatCommand) { // !rangetempunban fromip toip
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_TUNBAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 30) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeTempUnBan", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %crangetempunban <%s> <%s>. %s!|", 
			SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], 
			LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

	pChatCommand->m_sCommand += 15;
	pChatCommand->m_ui32CommandLen -= 15;

    return RangeUnban(pChatCommand, BanManager::TEMP);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RangePermUnBan(ChatCommand * pChatCommand) { // !rangepermunban fromip toip
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RANGE_UNBAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 30) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangePermUnBan", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %crangepermunban <%s> <%s>. %s!|", 
			SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

	pChatCommand->m_sCommand += 15;
	pChatCommand->m_ui32CommandLen -= 15;

    return RangeUnban(pChatCommand, BanManager::PERM);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RegNewUser(ChatCommand * pChatCommand) { // !reguser nick profile_name
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::ADDREGUSER) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen > 255) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RegNewUser1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_CMD_TOO_LONG]);
        return true;
    }

    char * sNick = pChatCommand->m_sCommand+8; // nick start

    char * sProfile = strchr(pChatCommand->m_sCommand+8, ' ');
    if(sProfile == NULL) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RegNewUser2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %creguser <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_PROFILENAME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

	uint32_t ui32NickLen = (uint32_t)(sProfile-sNick);

    sProfile[0] = '\0';
    sProfile++;

    int iProfile = ProfileManager::m_Ptr->GetProfileIndex(sProfile);
    if(iProfile == -1) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RegNewUser3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERR_NO_PROFILE_GIVEN_NAME_EXIST]);
        return true;
    }

    // check hierarchy
    // deny if pUser is not Master and tries add equal or higher profile
    if(pChatCommand->m_pUser->m_i32Profile > 0 && iProfile <= pChatCommand->m_pUser->m_i32Profile) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RegNewUser4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_YOU_NOT_ALLOWED_TO_ADD_USER_THIS_PROFILE]);
        return true;
    }

    // check if user is registered
    if(RegManager::m_Ptr->Find(sNick, ui32NickLen) != NULL) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RegNewUser5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_USER], sNick, LanguageManager::m_Ptr->m_sTexts[LAN_IS_ALREDY_REGISTERED]);
        return true;
    }

    User * pOtherUser = HashManager::m_Ptr->FindUser(sNick, ui32NickLen);
    if(pOtherUser == NULL) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RegNewUser6", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], sNick, LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_ONLINE]);
        return true;
    }

	UncountDeflood(pChatCommand);

    if(pOtherUser->m_pLogInOut == NULL) {
        pOtherUser->m_pLogInOut = new (std::nothrow) LoginLogout();
        if(pOtherUser->m_pLogInOut == NULL) {
            pOtherUser->m_ui32BoolBits |= User::BIT_ERROR;
            pOtherUser->Close();

            AppendDebugLog("%s - [MEM] Cannot allocate new pOtherUser->pLogInOut in HubCommands::RegNewUser\n");
            return true;
        }
    }

    pOtherUser->SetBuffer(sProfile);
    pOtherUser->m_ui32BoolBits |= User::BIT_WAITING_FOR_PASS;

    pOtherUser->SendFormat("HubCommands::RegNewUser7", true, "<%s> %s.|$GetPass|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_WERE_REGISTERED_PLEASE_ENTER_YOUR_PASSWORD]);

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::RegNewUser8", "<%s> *** %s %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_REGISTERED], sNick, 
			LanguageManager::m_Ptr->m_sTexts[LAN_AS], sProfile);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RegNewUser9", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], sNick, 
			LanguageManager::m_Ptr->m_sTexts[LAN_REGISTERED], LanguageManager::m_Ptr->m_sTexts[LAN_AS], sProfile);
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Stats(ChatCommand * pChatCommand) { // !stat !stats !statistic
	int iMsgLen = CheckFromPm(pChatCommand);

	int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "<%s>", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
	if(iRet <= 0) {
		return true;
	}
	iMsgLen += iRet;

	string Statinfo(ServerManager::m_pGlobalBuffer, iMsgLen);

	Statinfo+="\n------------------------------------------------------------\n";
	Statinfo+="Current stats:\n";
	Statinfo+="------------------------------------------------------------\n";
	Statinfo+="Uptime: "+string(ServerManager::m_ui64Days)+" days, "+string(ServerManager::m_ui64Hours) + " hours, " + string(ServerManager::m_ui64Mins) + " minutes\n";

    Statinfo+="Version: PtokaX DC Hub " PtokaXVersionString

#ifdef _WIN32
	#ifdef _M_X64
	    " x64"
	#elif _M_ARM
		" ARM"
	#endif
#endif

#ifdef _PtokaX_TESTING_
    " [build " BUILD_NUMBER "]"
#endif
    " built on " __DATE__ " " __TIME__ "\n"

#if LUA_VERSION_NUM > 501
	"Lua: " LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "." LUA_VERSION_RELEASE "\n";
#else
    LUA_RELEASE "\n";
#endif
    
#ifdef _WITH_SQLITE
	Statinfo+="SQLite: " SQLITE_VERSION "\n";
#elif _WITH_POSTGRES
	Statinfo+="PostgreSQL: "+string(PQlibVersion())+"\n";
#elif _WITH_MYSQL
	Statinfo+="MySQL: " MYSQL_SERVER_VERSION "\n";
#endif

#ifdef _WIN32
	Statinfo+="OS: "+ServerManager::m_sOS+"\r\n";
#else

	struct utsname osname;
	if(uname(&osname) >= 0) {
		Statinfo+="OS: "+string(osname.sysname)+" "+string(osname.release)+" ("+string(osname.machine)+")"
#ifdef __clang__
			" / Clang " __clang_version__
#elif __GNUC__
			" / GCC " __VERSION__
#endif
			"\n";
	}
#endif

	Statinfo+="Users (Max/Actual Peak (Max Peak)/Logged): "+string(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_USERS])+" / "+string(ServerManager::m_ui32Peak)+" ("+string(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_USERS_PEAK])+") / "+string(ServerManager::m_ui32Logged)+ "\n";
    Statinfo+="Joins / Parts: "+string(ServerManager::m_ui32Joins)+" / "+string(ServerManager::m_ui32Parts)+"\n";
	Statinfo+="Users shared size: "+string(ServerManager::m_ui64TotalShare)+" Bytes / "+string(formatBytes(ServerManager::m_ui64TotalShare))+"\n";
	Statinfo+="Chat messages: "+string(DcCommands::m_Ptr->m_ui32StatChat)+" x\n";
	Statinfo+="Unknown commands: "+string(DcCommands::m_Ptr->m_ui32StatCmdUnknown)+" x\n";
	Statinfo+="PM commands: "+string(DcCommands::m_Ptr->m_ui32StatCmdTo)+" x\n";
	Statinfo+="Key commands: "+string(DcCommands::m_Ptr->m_ui32StatCmdKey)+" x\n";
	Statinfo+="Supports commands: "+string(DcCommands::m_Ptr->m_ui32StatCmdSupports)+" x\n";
	Statinfo+="MyINFO commands: "+string(DcCommands::m_Ptr->m_ui32StatCmdMyInfo)+" x\n";
	Statinfo+="ValidateNick commands: "+string(DcCommands::m_Ptr->m_ui32StatCmdValidate)+" x\n";
	Statinfo+="GetINFO commands: "+string(DcCommands::m_Ptr->m_ui32StatCmdGetInfo)+" x\n";
	Statinfo+="Password commands: "+string(DcCommands::m_Ptr->m_ui32StatCmdMyPass)+" x\n";
	Statinfo+="Version commands: "+string(DcCommands::m_Ptr->m_ui32StatCmdVersion)+" x\n";
	Statinfo+="GetNickList commands: "+string(DcCommands::m_Ptr->m_ui32StatCmdGetNickList)+" x\n";
	Statinfo+="Search commands: "+string(DcCommands::m_Ptr->m_ui32StatCmdSearch)+" x ("+string(DcCommands::m_Ptr->m_ui32StatCmdMultiSearch)+" x)\n";
	Statinfo+="SR commands: "+string(DcCommands::m_Ptr->m_ui32StatCmdSR)+" x\n";
	Statinfo+="CTM commands: "+string(DcCommands::m_Ptr->m_ui32StatCmdConnectToMe)+" x ("+string(DcCommands::m_Ptr->m_ui32StatCmdMultiConnectToMe)+" x)\n";
	Statinfo+="RevCTM commands: "+string(DcCommands::m_Ptr->m_ui32StatCmdRevCTM)+" x\n";
	Statinfo+="BotINFO commands: "+string(DcCommands::m_Ptr->m_ui32StatBotINFO)+" x\n";
	Statinfo+="Close commands: "+string(DcCommands::m_Ptr->m_ui32StatCmdClose)+" x\n";
	//Statinfo+="------------------------------------------------------------\n";
	//Statinfo+="ClientSocket Errors: "+string(iStatUserSocketErrors)+" x\n";
	Statinfo+="------------------------------------------------------------\n";

#ifdef _WIN32
	FILETIME tmpa, tmpb, kernelTimeFT, userTimeFT;
	GetProcessTimes(GetCurrentProcess(), &tmpa, &tmpb, &kernelTimeFT, &userTimeFT);
	int64_t kernelTime = kernelTimeFT.dwLowDateTime | (((int64_t)kernelTimeFT.dwHighDateTime) << 32);
	int64_t userTime = userTimeFT.dwLowDateTime | (((int64_t)userTimeFT.dwHighDateTime) << 32);
	int64_t icpuSec = (kernelTime + userTime) / (10000000I64);

	char cpuusage[32];
	int iLen = snprintf(cpuusage, 32, "%.2f%%\r\n", ServerManager::m_dCpuUsage/0.6);
	if(iLen <= 0) {
		return true;
	}
	Statinfo+="CPU usage (60 sec avg): "+string(cpuusage, iLen);

	char cputime[64];
	iLen = snprintf(cputime, 64, "%01I64d:%02d:%02d", icpuSec / (60*60), (int32_t)((icpuSec / 60) % 60), (int32_t)(icpuSec % 60));
	if(iLen <= 0) {
		return true;
	}
	Statinfo+="CPU time: "+string(cputime, iLen)+"\r\n";

	PROCESS_MEMORY_COUNTERS pmc;
	memset(&pmc, 0, sizeof(PROCESS_MEMORY_COUNTERS));
	pmc.cb = sizeof(pmc);

	::GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));

	Statinfo+="Mem usage (Peak): "+string(formatBytes(pmc.WorkingSetSize))+ " ("+string(formatBytes(pmc.PeakWorkingSetSize))+")\r\n";
	Statinfo+="VM size (Peak): "+string(formatBytes(pmc.PagefileUsage))+ " ("+string(formatBytes(pmc.PeakPagefileUsage))+")\r\n";
#else // _WIN32
	char cpuusage[32];
	int iLen = snprintf(cpuusage, 32, "%.2f%%\n", (ServerManager::m_dCpuUsage/0.6)/(double)ServerManager::m_ui32CpuCount);
	if(iLen <= 0) {
		return true;
	}
	Statinfo+="CPU usage (60 sec avg): "+string(cpuusage, iLen);

	struct rusage rs;

	getrusage(RUSAGE_SELF, &rs);

	uint64_t ui64cpuSec = uint64_t(rs.ru_utime.tv_sec) + (uint64_t(rs.ru_utime.tv_usec)/1000000) +
		uint64_t(rs.ru_stime.tv_sec) + (uint64_t(rs.ru_stime.tv_usec)/1000000);

	char cputime[64];
	iLen = snprintf(cputime, 64, "%01" PRIu64 ":%02" PRIu64 ":%02" PRIu64, ui64cpuSec / (60*60), (ui64cpuSec / 60) % 60, ui64cpuSec % 60);
	if(iLen <= 0) {
		return true;
	}
	Statinfo+="CPU time: "+string(cputime, iLen)+"\n";

	FILE *fp = fopen("/proc/self/status", "r");
	if(fp != NULL) {
		string memrss, memhwm, memvms, memvmp, memstk, memlib;
		char buf[1024];
		char * tmp = NULL;

		while(fgets(buf, 1024, fp) != NULL) {
			if(strncmp(buf, "VmRSS:", 6) == 0) {
				tmp = buf+6;
				while(isspace(*tmp) && *tmp) {
					tmp++;
				}
				memrss = string(tmp, strlen(tmp)-1);
			} else if(strncmp(buf, "VmHWM:", 6) == 0) {
				tmp = buf+6;
				while(isspace(*tmp) && *tmp) {
					tmp++;
				}
				memhwm = string(tmp, strlen(tmp)-1);
			} else if(strncmp(buf, "VmSize:", 7) == 0) {
				tmp = buf+7;
				while(isspace(*tmp) && *tmp) {
					tmp++;
				}
				memvms = string(tmp, strlen(tmp)-1);
			} else if(strncmp(buf, "VmPeak:", 7) == 0) {
				tmp = buf+7;
				while(isspace(*tmp) && *tmp) {
					tmp++;
				}
				memvmp = string(tmp, strlen(tmp)-1);
			} else if(strncmp(buf, "VmStk:", 6) == 0) {
				tmp = buf+6;
				while(isspace(*tmp) && *tmp) {
					tmp++;
				}
				memstk = string(tmp, strlen(tmp)-1);
			} else if(strncmp(buf, "VmLib:", 6) == 0) {
				tmp = buf+6;
				while(isspace(*tmp) && *tmp) {
					tmp++;
				}
				memlib = string(tmp, strlen(tmp)-1);
			}
		}

		fclose(fp);

		if(memhwm.size() != 0 && memrss.size() != 0) {
			Statinfo+="Mem usage (Peak): "+memrss+ " ("+memhwm+")\n";
		} else if(memrss.size() != 0) {
			Statinfo+="Mem usage: "+memrss+"\n";
		}

		if(memvmp.size() != 0 && memvms.size() != 0) {
			Statinfo+="VM size (Peak): "+memvms+ " ("+memvmp+")\n";
		} else if(memrss.size() != 0) {
			Statinfo+="VM size: "+memvms+"\n";
		}

		if(memstk.size() != 0 && memlib.size() != 0) {
			Statinfo+="Stack size / Libs size: "+memstk+ " / "+memlib+"\n";
		}
	}
#endif

	Statinfo+="------------------------------------------------------------\n";
	Statinfo+="SendRests (Peak): "+string(ServiceLoop::m_Ptr->m_ui32LastSendRest)+" ("+string(ServiceLoop::m_Ptr->m_ui32SendRestsPeak)+")\n";
	Statinfo+="RecvRests (Peak): "+string(ServiceLoop::m_Ptr->m_ui32LastRecvRest)+" ("+string(ServiceLoop::m_Ptr->m_ui32RecvRestsPeak)+")\n";
	Statinfo+="Compression saved: "+string(formatBytes(ServerManager::m_ui64BytesSentSaved))+" ("+string(DcCommands::m_Ptr->m_ui32StatZPipe)+")\n";
	Statinfo+="Data sent: "+string(formatBytes(ServerManager::m_ui64BytesSent))+ "\n";
	Statinfo+="Data received: "+string(formatBytes(ServerManager::m_ui64BytesRead))+ "\n";
	Statinfo+="Tx (60 sec avg): "+string(formatBytesPerSecond(ServerManager::m_ui32ActualBytesSent))+" ("+string(formatBytesPerSecond(ServerManager::m_ui32AverageBytesSent/60))+")\n";
	Statinfo+="Rx (60 sec avg): "+string(formatBytesPerSecond(ServerManager::m_ui32ActualBytesRead))+" ("+string(formatBytesPerSecond(ServerManager::m_ui32AverageBytesRead/60))+")|";

	pChatCommand->m_pUser->SendCharDelayed(Statinfo.c_str(), Statinfo.size());

	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::StopScript(ChatCommand * pChatCommand) { // !stopscript scriptname
	if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RSTSCRIPTS) == false) {
		SendNoPermission(pChatCommand);
		return true;
	}

	if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StopScript1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERR_SCRIPTS_DISABLED]);
		return true;
	}

	if(pChatCommand->m_ui32CommandLen < 12) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StopScript2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cstopscript <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPTNAME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
		return true;
	}

	pChatCommand->m_sCommand += 11;
	pChatCommand->m_ui32CommandLen -= 11;

    if(pChatCommand->m_ui32CommandLen > 256) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StopScript3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cstopscript <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPTNAME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_SCRIPT_NAME_LEN_256_CHARS]);
        return true;
    }

	Script * curScript = ScriptManager::m_Ptr->FindScript(pChatCommand->m_sCommand);
	if(curScript == NULL || curScript->m_bEnabled == false || curScript->m_pLua == NULL) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StopScript4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR_SCRIPT], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_NOT_RUNNING]);
		return true;
	}

	UncountDeflood(pChatCommand);

	ScriptManager::m_Ptr->StopScript(curScript, true);

	if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
		GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::StopScript5", "<%s> *** %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_STOPPED_SCRIPT], pChatCommand->m_sCommand);
	}

	if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StopScript6", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPT], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_STOPPED_LWR]);
	}

	return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::StartScript(ChatCommand * pChatCommand) { // !startscript scriptname
	if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::RSTSCRIPTS) == false) {
		SendNoPermission(pChatCommand);
		return true;
	}

	if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StartScript1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERR_SCRIPTS_DISABLED]);
		return true;
	}

	if(pChatCommand->m_ui32CommandLen < 13) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StartScript2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cstartscript <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPTNAME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
		return true;
	}

	pChatCommand->m_sCommand += 12;
	pChatCommand->m_ui32CommandLen -= 12;

    if(pChatCommand->m_ui32CommandLen > 256) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StartScript3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cstartscript <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPTNAME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_SCRIPT_NAME_LEN_256_CHARS]);
        return true;
    }

    char * sBadChar = strpbrk(pChatCommand->m_sCommand, "/\\");
	if(sBadChar != NULL || FileExist((ServerManager::m_sScriptPath + pChatCommand->m_sCommand).c_str()) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StartScript4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR_SCRIPT_NOT_EXIST]);
		return true;
	}

	Script * pScript = ScriptManager::m_Ptr->FindScript(pChatCommand->m_sCommand);
	if(pScript != NULL) {
		if(pScript->m_bEnabled == true && pScript->m_pLua != NULL) {
			pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StartScript5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_ERROR_SCRIPT_ALREDY_RUNNING]);
			return true;
		}

		UncountDeflood(pChatCommand);

		if(ScriptManager::m_Ptr->StartScript(pScript, true) == true) {
			if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
				GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::StartScript6", "<%s> *** %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_STARTED_SCRIPT], pChatCommand->m_sCommand);
			}

			if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
				pChatCommand->m_pUser->SendFormatCheckPM("HHubCommands::StartScript7", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
					LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPT], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_STARTED_LWR]);
			}
			return true;
		} else {
			pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StartScript8", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_ERROR_SCRIPT], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_START_FAILED]);
			return true;
		}
	}

	UncountDeflood(pChatCommand);

	if(ScriptManager::m_Ptr->AddScript(pChatCommand->m_sCommand, true, true) == true && ScriptManager::m_Ptr->StartScript(ScriptManager::m_Ptr->m_ppScriptTable[ScriptManager::m_Ptr->m_ui8ScriptCount-1], false) == true) {
		if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
			GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::StartScript9", "<%s> *** %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_STARTED_SCRIPT], pChatCommand->m_sCommand);
		}

		if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
			pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StartScript10", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPT], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_STARTED_LWR]);
		}
		return true;
	} else {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::StartScript11", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR_SCRIPT], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_START_FAILED]);
		return true;
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::TempBan(ChatCommand * pChatCommand) { // !tempban nick time reason ... m = minutes, h = hours, d = day, w = weeks, M = months, Y = years
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TEMP_BAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 11) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBan", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ctempban <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

	pChatCommand->m_sCommand += 8;
	pChatCommand->m_ui32CommandLen -= 8;

    return TempBan(pChatCommand, false);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::TempBanIp(ChatCommand * pChatCommand) { // !tempbanip nick time reason ... m = minutes, h = hours, d = day, w = weeks, M = months, Y = years
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TEMP_BAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 14) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBanIp", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ctempbanip <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

	pChatCommand->m_sCommand += 10;
	pChatCommand->m_ui32CommandLen -= 10;

    return TempBanIp(pChatCommand, false);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::TempUnban(ChatCommand * pChatCommand) { // !tempunban nick/ip
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TEMP_UNBAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 11 || pChatCommand->m_sCommand[10] == '\0') {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempUnban1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ctempunban <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP_OR_NICK], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen > 100) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempUnban2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ctempunban <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP_OR_NICK], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        return true;
    }

    pChatCommand->m_sCommand += 10;
    pChatCommand->m_ui32CommandLen -= 10;

    if(BanManager::m_Ptr->TempUnban(pChatCommand->m_sCommand) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempUnban3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SORRY], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_IN_MY_TEMP_BANS]);
        return true;
    }

	UncountDeflood(pChatCommand);

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
    	GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::TempUnban4", "<%s> *** %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_REMOVED_LWR], pChatCommand->m_sCommand, 
			LanguageManager::m_Ptr->m_sTexts[LAN_FROM_TEMP_BANS]);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempUnban5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_sCommand, 
			LanguageManager::m_Ptr->m_sTexts[LAN_REMOVED_FROM_TEMP_BANS]);
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Topic(ChatCommand * pChatCommand) { // !topic text/off
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TOPIC) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 7 || pChatCommand->m_sCommand[6] == '\0') {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Topic1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ctopic <%s>, %ctopic <off>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NEW_TOPIC], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

    pChatCommand->m_sCommand += 6;
    pChatCommand->m_ui32CommandLen -= 6;

	UncountDeflood(pChatCommand);

    if(pChatCommand->m_ui32CommandLen > 256) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Topic2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ctopic <%s>, %ctopic <off>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NEW_TOPIC], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_TOPIC_LEN_256_CHARS]);
        return true;
    }

	if(strcasecmp(pChatCommand->m_sCommand, "off") == 0) {
        SettingManager::m_Ptr->SetText(SETTXT_HUB_TOPIC, "", 0);

        GlobalDataQueue::m_Ptr->AddQueueItem(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_NAME], SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_HUB_NAME], NULL, 0, GlobalDataQueue::CMD_HUBNAME);

        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Topic3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_TOPIC_HAS_BEEN_CLEARED]);
    } else {
        SettingManager::m_Ptr->SetText(SETTXT_HUB_TOPIC, pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen);

        GlobalDataQueue::m_Ptr->AddQueueItem(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_NAME], SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_HUB_NAME], NULL, 0, GlobalDataQueue::CMD_HUBNAME);

        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Topic4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_TOPIC_HAS_BEEN_SET_TO], pChatCommand->m_sCommand);
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Unban(ChatCommand * pChatCommand) { // !unban nick/ip
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::UNBAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 7 || pChatCommand->m_sCommand[6] == '\0') {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Unban1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cunban <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP_OR_NICK], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen > 106) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Unban2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cunban <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP_OR_NICK], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        return true;
    }

    pChatCommand->m_sCommand += 6;
    pChatCommand->m_ui32CommandLen -= 6;

    if(BanManager::m_Ptr->Unban(pChatCommand->m_sCommand) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Unban3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SORRY], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_IN_BANS]);
        return true;
    }

	UncountDeflood(pChatCommand);

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
    	GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::Unban4", "<%s> *** %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_REMOVED_LWR], pChatCommand->m_sCommand, 
			LanguageManager::m_Ptr->m_sTexts[LAN_FROM_BANS]);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Unban5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_sCommand, 
			LanguageManager::m_Ptr->m_sTexts[LAN_REMOVED_FROM_BANS]);
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Ungag(ChatCommand * pChatCommand) { // !ungag nick
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GAG) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

   	if(pChatCommand->m_ui32CommandLen < 7 || pChatCommand->m_sCommand[6] == '\0') {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DoCommand->ungag1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cungag <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen > 106) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DoCommand->ungag2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cungag <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        return true;
    }

    pChatCommand->m_sCommand += 6;
    pChatCommand->m_ui32CommandLen -= 6;

    User * pOtherUser = HashManager::m_Ptr->FindUser(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen);
    if(pOtherUser == NULL) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DoCommand->ungag3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_IN_USERLIST]);
        return true;
    }

    if(((pOtherUser->m_ui32BoolBits & User::BIT_GAGGED) == User::BIT_GAGGED) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DoCommand->ungag4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_GAGGED]);
		return true;
    }

	UncountDeflood(pChatCommand);

    pOtherUser->m_ui32BoolBits &= ~User::BIT_GAGGED;

    pOtherUser->SendFormat("HubCommands::DoCommand->ungag", pChatCommand->m_bFromPM, "<%s> %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_UNGAGGED_BY], pChatCommand->m_pUser->m_sNick);

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
	    GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::DoCommand->ungag", "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_UNGAGGED], pOtherUser->m_sNick);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DoCommand->ungag5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pOtherUser->m_sNick, 
			LanguageManager::m_Ptr->m_sTexts[LAN_HAS_UNGAGGED]);
    }

    return true;
}
