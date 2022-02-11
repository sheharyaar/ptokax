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
#include "LuaInc.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "LuaScript.h"
//---------------------------------------------------------------------------
#include "IP2Country.h"
#include "LuaCoreLib.h"
#include "LuaBanManLib.h"
#include "LuaIP2CountryLib.h"
#include "LuaProfManLib.h"
#include "LuaRegManLib.h"
#include "LuaScriptManLib.h"
#include "LuaSetManLib.h"
#include "LuaTmrManLib.h"
#include "LuaUDPDbgLib.h"
#include "ResNickManager.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
	#include "../gui.win/GuiUtil.h"
    #include "../gui.win/MainWindowPageScripts.h"
#endif
//---------------------------------------------------------------------------
char ScriptTimer::m_sDefaultTimerFunc[] = "OnTimer";
//---------------------------------------------------------------------------

static int ScriptPanic(lua_State * pLua) {
    size_t szLen = 0;
    char * stmp = (char*)lua_tolstring(pLua, -1, &szLen);

    string sMsg = "[LUA] At panic -> " + string(stmp, szLen);

    AppendLog(sMsg.c_str());

    return 0;
}
//------------------------------------------------------------------------------

ScriptBot::ScriptBot() : m_pPrev(NULL), m_pNext(NULL), m_sNick(NULL), m_sMyINFO(NULL), m_bIsOP(false) {
    ScriptManager::m_Ptr->m_ui8BotsCount++;
}
//------------------------------------------------------------------------------

ScriptBot::~ScriptBot() {
#ifdef _WIN32
    if(m_sNick != NULL && HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sNick) == 0) {
		AppendDebugLog("%s - [MEM] Cannot deallocate m_sNick in ScriptBot::~ScriptBot\n");
    }
#else
	free(m_sNick);
#endif

#ifdef _WIN32
    if(m_sMyINFO != NULL && HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sMyINFO) == 0) {
		AppendDebugLog("%s - [MEM] Cannot deallocate m_sMyINFO in ScriptBot::~ScriptBot\n");
    }
#else
	free(m_sMyINFO);
#endif

	ScriptManager::m_Ptr->m_ui8BotsCount--;
}
//------------------------------------------------------------------------------

ScriptBot * ScriptBot::CreateScriptBot(char * sBotNick, const size_t szNickLen, char * sDescription, const size_t szDscrLen, char * sEmail, const size_t szEmailLen, const bool bOP) {
    ScriptBot * pScriptBot = new (std::nothrow) ScriptBot();

    if(pScriptBot == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate new pScriptBot in ScriptBot::CreateScriptBot\n");

        return NULL;
    }

#ifdef _WIN32
    pScriptBot->m_sNick = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNickLen+1);
#else
	pScriptBot->m_sNick = (char *)malloc(szNickLen+1);
#endif
    if(pScriptBot->m_sNick == NULL) {
		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for pScriptBot->m_sNick in ScriptBot::CreateScriptBot\n", szNickLen+1);

        delete pScriptBot;
		return NULL;
    }
    memcpy(pScriptBot->m_sNick, sBotNick, szNickLen);
    pScriptBot->m_sNick[szNickLen] = '\0';

    pScriptBot->m_bIsOP = bOP;

    size_t szWantLen = 24+szNickLen+szDscrLen+szEmailLen;

#ifdef _WIN32
    pScriptBot->m_sMyINFO = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szWantLen);
#else
	pScriptBot->m_sMyINFO = (char *)malloc(szWantLen);
#endif
    if(pScriptBot->m_sMyINFO == NULL) {
		AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for pScriptBot->m_sMyINFO in ScriptBot::CreateScriptBot\n", szWantLen);

        delete pScriptBot;
		return NULL;
    }

	if(snprintf(pScriptBot->m_sMyINFO, szWantLen, "$MyINFO $ALL %s %s$ $$%s$$|", sBotNick, sDescription != NULL ? sDescription : "", sEmail != NULL ? sEmail : "") <= 0) {
		pScriptBot->m_sMyINFO[0] = '\0';
	}

    return pScriptBot;
}
//------------------------------------------------------------------------------

ScriptTimer::ScriptTimer() : 
#if defined(_WIN32) && !defined(_WIN_IOT)
	m_uiTimerId(NULL),
#else
	m_ui64Interval(0), m_ui64LastTick(0),
#endif
	m_pPrev(NULL), m_pNext(NULL), m_pLua(NULL), m_sFunctionName(NULL), m_iFunctionRef(0) {
	// ...
}
//------------------------------------------------------------------------------

ScriptTimer::~ScriptTimer() {
#ifdef _WIN32
	if(m_sFunctionName != NULL && m_sFunctionName != m_sDefaultTimerFunc) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sFunctionName) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_sFunctionName in ScriptTimer::~ScriptTimer\n");
        }
#else
    if(m_sFunctionName != m_sDefaultTimerFunc) {
        free(m_sFunctionName);
#endif
    }
}
//------------------------------------------------------------------------------

#if defined(_WIN32) && !defined(_WIN_IOT)
ScriptTimer * ScriptTimer::CreateScriptTimer(UINT_PTR uiTmrId, char * sFunctName, const size_t szLen, const int iRef, lua_State * pLuaState) {
#else
ScriptTimer * ScriptTimer::CreateScriptTimer(char * sFunctName, const size_t szLen, const int iRef, lua_State * pLuaState) {
#endif
    ScriptTimer * pScriptTimer = new (std::nothrow) ScriptTimer();

    if(pScriptTimer == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate new pScriptTimer in ScriptTimer::CreateScriptTimer\n");

        return NULL;
    }

	pScriptTimer->m_pLua = pLuaState;

	if(sFunctName != NULL) {
        if(sFunctName != m_sDefaultTimerFunc) {
#ifdef _WIN32
            pScriptTimer->m_sFunctionName = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
            pScriptTimer->m_sFunctionName = (char *)malloc(szLen+1);
#endif
            if(pScriptTimer->m_sFunctionName == NULL) {
                AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for pScriptTimer->m_sFunctionName in ScriptTimer::CreateScriptTimer\n", szLen+1);

                delete pScriptTimer;
                return NULL;
            }

            memcpy(pScriptTimer->m_sFunctionName, sFunctName, szLen);
            pScriptTimer->m_sFunctionName[szLen] = '\0';
        } else {
            pScriptTimer->m_sFunctionName = m_sDefaultTimerFunc;
        }
    } else {
        pScriptTimer->m_iFunctionRef = iRef;
    }

#if defined(_WIN32) && !defined(_WIN_IOT)
	pScriptTimer->m_uiTimerId = uiTmrId;
#endif

    return pScriptTimer;
}
//------------------------------------------------------------------------------

Script::Script() : m_pPrev(NULL), m_pNext(NULL), m_pBotList(NULL), m_pLua(NULL), m_sName(NULL), m_ui32DataArrivals(4294967295U), m_ui16Functions(65535),
	m_bEnabled(false), m_bRegUDP(false), m_bProcessed(false) {
	// ...
}
//------------------------------------------------------------------------------

Script::~Script() {
    if(m_bRegUDP == true) {
		UdpDebug::m_Ptr->Remove(m_sName);
        m_bRegUDP = false;
    }

    if(m_pLua != NULL) {
        lua_close(m_pLua);
    }

#ifdef _WIN32
	if(m_sName != NULL && HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sName) == 0) {
        AppendDebugLog("%s - [MEM] Cannot deallocate m_sName in Script::~Script\n");
    }
#else
	free(m_sName);
#endif
}
//------------------------------------------------------------------------------

Script * Script::CreateScript(char * sName, const bool enabled) {
    Script * pScript = new (std::nothrow) Script();

    if(pScript == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate new pScript in Script::CreateScript\n");

        return NULL;
    }

#ifdef _WIN32
	string ExtractedFilename(ExtractFileName(sName));
	size_t szNameLen = ExtractedFilename.size();

    pScript->m_sName = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szNameLen+1);
#else
    size_t szNameLen = strlen(sName);

    pScript->m_sName = (char *)malloc(szNameLen+1);
#endif
    if(pScript->m_sName == NULL) {
        AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes in Script::CreateScript\n", szNameLen+1);

        delete pScript;
        return NULL;
    }
#ifdef _WIN32
    memcpy(pScript->m_sName, ExtractedFilename.c_str(), ExtractedFilename.size());
#else
    memcpy(pScript->m_sName, sName, szNameLen);
#endif
    pScript->m_sName[szNameLen] = '\0';

    pScript->m_bEnabled = enabled;

    return pScript;
}
//------------------------------------------------------------------------------

static int OsExit(lua_State * /* pLua*/) {
	EventQueue::m_Ptr->AddNormal(EventQueue::EVENT_SHUTDOWN, NULL);

    return 0;
}
//------------------------------------------------------------------------------

static void AddSettingIds(lua_State * pLua) {
	int iTable = lua_gettop(pLua);

	lua_newtable(pLua);
	int iNewTable = lua_gettop(pLua);

	const uint8_t ui8Bools[] = { SETBOOL_ANTI_MOGLO, SETBOOL_AUTO_START, SETBOOL_REDIRECT_ALL, SETBOOL_REDIRECT_WHEN_HUB_FULL, SETBOOL_AUTO_REG, SETBOOL_REG_ONLY, 
		SETBOOL_REG_ONLY_REDIR, SETBOOL_SHARE_LIMIT_REDIR, SETBOOL_SLOTS_LIMIT_REDIR, SETBOOL_HUB_SLOT_RATIO_REDIR, SETBOOL_MAX_HUBS_LIMIT_REDIR, 
		SETBOOL_MODE_TO_MYINFO, SETBOOL_MODE_TO_DESCRIPTION, SETBOOL_STRIP_DESCRIPTION, SETBOOL_STRIP_TAG, SETBOOL_STRIP_CONNECTION, 
		SETBOOL_STRIP_EMAIL, SETBOOL_REG_BOT, SETBOOL_USE_BOT_NICK_AS_HUB_SEC, SETBOOL_REG_OP_CHAT, SETBOOL_TEMP_BAN_REDIR, SETBOOL_PERM_BAN_REDIR, 
		SETBOOL_ENABLE_SCRIPTING, SETBOOL_KEEP_SLOW_USERS, SETBOOL_CHECK_NEW_RELEASES, SETBOOL_ENABLE_TRAY_ICON, SETBOOL_START_MINIMIZED, 
		SETBOOL_FILTER_KICK_MESSAGES, SETBOOL_SEND_KICK_MESSAGES_TO_OPS, SETBOOL_SEND_STATUS_MESSAGES, SETBOOL_SEND_STATUS_MESSAGES_AS_PM, 
		SETBOOL_ENABLE_TEXT_FILES, SETBOOL_SEND_TEXT_FILES_AS_PM, SETBOOL_STOP_SCRIPT_ON_ERROR, SETBOOL_MOTD_AS_PM, SETBOOL_DEFLOOD_REPORT, 
		SETBOOL_REPLY_TO_HUB_COMMANDS_AS_PM, SETBOOL_DISABLE_MOTD, SETBOOL_DONT_ALLOW_PINGERS, SETBOOL_REPORT_PINGERS, SETBOOL_REPORT_3X_BAD_PASS, 
		SETBOOL_ADVANCED_PASS_PROTECTION, SETBOOL_BIND_ONLY_SINGLE_IP, SETBOOL_RESOLVE_TO_IP, SETBOOL_NICK_LIMIT_REDIR, SETBOOL_BAN_MSG_SHOW_IP, 
		SETBOOL_BAN_MSG_SHOW_RANGE, SETBOOL_BAN_MSG_SHOW_NICK, SETBOOL_BAN_MSG_SHOW_REASON, SETBOOL_BAN_MSG_SHOW_BY, SETBOOL_REPORT_SUSPICIOUS_TAG, 
		SETBOOL_LOG_SCRIPT_ERRORS, SETBOOL_NO_QUACK_SUPPORTS, SETBOOL_HASH_PASSWORDS,
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
		SETBOOL_ENABLE_DATABASE,
#endif
	};

	const char * pBoolsNames[] = { "AntiMoGlo", "AutoStart", "RedirectAll", "RedirectWhenHubFull", "AutoReg", "RegOnly",
		"RegOnlyRedir", "ShareLimitRedir", "SlotLimitRedir", "HubSlotRatioRedir", "MaxHubsLimitRedir", 
		"ModeToMyInfo", "ModeToDescription", "StripDescription", "StripTag", "StripConnection", 
		"StripEmail", "RegBot", "UseBotAsHubSec", "RegOpChat", "TempBanRedir", "PermBanRedir", 
		"EnableScripting", "KeepSlowUsers", "CheckNewReleases", "EnableTrayIcon", "StartMinimized",
		"FilterKickMessages", "SendKickMessagesToOps", "SendStatusMessages", "SendStatusMessagesAsPm",
		"EnableTextFiles", "SendTextFilesAsPm", "StopScriptOnError", "SendMotdAsPm", "DefloodReport",
		"ReplyToHubCommandsAsPm", "DisableMotd", "DontAllowPingers", "ReportPingers", "Report3xBadPass",
		"AdvancedPassProtection", "ListenOnlySingleIp", "ResolveToIp", "NickLimitRedir", "BanMsgShowIp",
		"BanMsgShowRange", "BanMsgShowNick", "BanMsgShowReason", "BanMsgShowBy", "ReportSuspiciousTag",
		"LogScriptErrors", "DisallowBadSupports", "HashPasswords", 
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
		"EnableDatabase",
#endif
	};

	for(uint8_t ui8i = 0; ui8i < sizeof(ui8Bools); ui8i++) {
		lua_pushinteger(pLua, ui8Bools[ui8i]);
		lua_setfield(pLua, iNewTable, pBoolsNames[ui8i]);
	}

	lua_setfield(pLua, iTable, "tBooleans");

	lua_newtable(pLua);
	iNewTable = lua_gettop(pLua);

	const uint8_t ui8Numbers[] = { SETSHORT_MAX_USERS, SETSHORT_MIN_SHARE_LIMIT, SETSHORT_MIN_SHARE_UNITS, SETSHORT_MAX_SHARE_LIMIT, SETSHORT_MAX_SHARE_UNITS, 
		SETSHORT_MIN_SLOTS_LIMIT, SETSHORT_MAX_SLOTS_LIMIT, SETSHORT_HUB_SLOT_RATIO_HUBS, SETSHORT_HUB_SLOT_RATIO_SLOTS, SETSHORT_MAX_HUBS_LIMIT, 
		SETSHORT_NO_TAG_OPTION, SETSHORT_FULL_MYINFO_OPTION, SETSHORT_MAX_CHAT_LEN, SETSHORT_MAX_CHAT_LINES, SETSHORT_MAX_PM_LEN, 
		SETSHORT_MAX_PM_LINES, SETSHORT_DEFAULT_TEMP_BAN_TIME, SETSHORT_MAX_PASIVE_SR, SETSHORT_MYINFO_DELAY, SETSHORT_MAIN_CHAT_MESSAGES, 
		SETSHORT_MAIN_CHAT_TIME, SETSHORT_MAIN_CHAT_ACTION, SETSHORT_SAME_MAIN_CHAT_MESSAGES, SETSHORT_SAME_MAIN_CHAT_TIME, SETSHORT_SAME_MAIN_CHAT_ACTION, 
		SETSHORT_SAME_MULTI_MAIN_CHAT_MESSAGES, SETSHORT_SAME_MULTI_MAIN_CHAT_LINES, SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION, SETSHORT_PM_MESSAGES, SETSHORT_PM_TIME, 
		SETSHORT_PM_ACTION, SETSHORT_SAME_PM_MESSAGES, SETSHORT_SAME_PM_TIME, SETSHORT_SAME_PM_ACTION, SETSHORT_SAME_MULTI_PM_MESSAGES, 
		SETSHORT_SAME_MULTI_PM_LINES, SETSHORT_SAME_MULTI_PM_ACTION, SETSHORT_SEARCH_MESSAGES, SETSHORT_SEARCH_TIME, SETSHORT_SEARCH_ACTION, 
		SETSHORT_SAME_SEARCH_MESSAGES, SETSHORT_SAME_SEARCH_TIME, SETSHORT_SAME_SEARCH_ACTION, SETSHORT_MYINFO_MESSAGES, SETSHORT_MYINFO_TIME, 
		SETSHORT_MYINFO_ACTION, SETSHORT_GETNICKLIST_MESSAGES, SETSHORT_GETNICKLIST_TIME, SETSHORT_GETNICKLIST_ACTION, SETSHORT_NEW_CONNECTIONS_COUNT, 
		SETSHORT_NEW_CONNECTIONS_TIME, SETSHORT_DEFLOOD_WARNING_COUNT, SETSHORT_DEFLOOD_WARNING_ACTION, SETSHORT_DEFLOOD_TEMP_BAN_TIME, SETSHORT_GLOBAL_MAIN_CHAT_MESSAGES, 
		SETSHORT_GLOBAL_MAIN_CHAT_TIME, SETSHORT_GLOBAL_MAIN_CHAT_TIMEOUT, SETSHORT_GLOBAL_MAIN_CHAT_ACTION, SETSHORT_MIN_SEARCH_LEN, SETSHORT_MAX_SEARCH_LEN, 
		SETSHORT_MIN_NICK_LEN, SETSHORT_MAX_NICK_LEN, SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE, SETSHORT_BRUTE_FORCE_PASS_PROTECT_TEMP_BAN_TIME, SETSHORT_MAX_PM_COUNT_TO_USER,
		SETSHORT_MAX_SIMULTANEOUS_LOGINS, SETSHORT_MAIN_CHAT_MESSAGES2, SETSHORT_MAIN_CHAT_TIME2, SETSHORT_MAIN_CHAT_ACTION2, SETSHORT_PM_MESSAGES2, 
		SETSHORT_PM_TIME2, SETSHORT_PM_ACTION2, SETSHORT_SEARCH_MESSAGES2, SETSHORT_SEARCH_TIME2, SETSHORT_SEARCH_ACTION2, 
		SETSHORT_MYINFO_MESSAGES2, SETSHORT_MYINFO_TIME2, SETSHORT_MYINFO_ACTION2, SETSHORT_MAX_MYINFO_LEN, SETSHORT_CTM_MESSAGES, 
		SETSHORT_CTM_TIME, SETSHORT_CTM_ACTION, SETSHORT_CTM_MESSAGES2, SETSHORT_CTM_TIME2, SETSHORT_CTM_ACTION2, 
		SETSHORT_RCTM_MESSAGES, SETSHORT_RCTM_TIME, SETSHORT_RCTM_ACTION, SETSHORT_RCTM_MESSAGES2, SETSHORT_RCTM_TIME2, 
		SETSHORT_RCTM_ACTION2, SETSHORT_MAX_CTM_LEN, SETSHORT_MAX_RCTM_LEN, SETSHORT_SR_MESSAGES, SETSHORT_SR_TIME, 
		SETSHORT_SR_ACTION, SETSHORT_SR_MESSAGES2, SETSHORT_SR_TIME2, SETSHORT_SR_ACTION2, SETSHORT_MAX_SR_LEN, 
		SETSHORT_MAX_DOWN_ACTION, SETSHORT_MAX_DOWN_KB, SETSHORT_MAX_DOWN_TIME, SETSHORT_MAX_DOWN_ACTION2, SETSHORT_MAX_DOWN_KB2, 
		SETSHORT_MAX_DOWN_TIME2, SETSHORT_CHAT_INTERVAL_MESSAGES, SETSHORT_CHAT_INTERVAL_TIME, SETSHORT_PM_INTERVAL_MESSAGES, SETSHORT_PM_INTERVAL_TIME, 
		SETSHORT_SEARCH_INTERVAL_MESSAGES, SETSHORT_SEARCH_INTERVAL_TIME, SETSHORT_MAX_CONN_SAME_IP, SETSHORT_MIN_RECONN_TIME,
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
		SETSHORT_DB_REMOVE_OLD_RECORDS,
#endif
	};

	const char * pNumbersNames[] = { "MaxUsers", "MinShareLimit", "MinShareUnits", "MaxShareLimit", "MaxShareUnits",
		"MinSlotsLimit", "MaxSlotsLimit", "HubSlotRatioHubs", "HubSlotRatioSlots", "MaxHubsLimit",
		"NoTagOption", "LongMyinfoOption", "MaxChatLen", "MaxChatLines", "MaxPmLen",
		"MaxPmLines", "DefaultTempBanTime", "MaxPasiveSr", "MyInfoDelay", "MainChatMessages",
		"MainChatTime", "MainChatAction", "SameMainChatMessages", "SameMainChatTime", "SameMainChatAction",
		"SameMultiMainChatMessages", "SameMultiMainChatLines", "SameMultiMainChatAction", "PmMessages", "PmTime",
		"PmAction", "SamePmMessages", "SamePmTime", "SamePmAction", "SameMultiPmMessages",
		"SameMultiPmLines", "SameMultiPmAction", "SearchMessages", "SearchTime", "SearchAction",
		"SameSearchMessages", "SameSearchTime", "SameSearchAction", "MyinfoMessages", "MyinfoTime",
		"MyinfoAction", "GetnicklistMessages", "GetnicklistTime", "GetnicklistAction", "NewConnectionsCount",
		"NewConnectionsTime", "DefloodWarningCount", "DefloodWarningAction", "DefloodTempBanTime", "GlobalMainChatMessages",
		"GlobalMainChatTime", "GlobalMainChatTimeout", "GlobalMainChatAction", "MinSearchLen", "MaxSearchLen",
		"MinNickLen", "MaxNickLen", "BruteForcePassProtectBanType", "BruteForcePassProtectTempBanTime", "MaxPmCountToUser",
		"MaxSimultaneousLogins", "MainChatMessages2", "MainChatTime2", "MainChatAction2", "PmMessages2", 
		"PmTime2", "PmAction2", "SearchMessages2", "SearchTime2", "SearchAction2",
		"MyinfoMessages2", "MyinfoTime2", "MyinfoAction2", "MaxMyinfoLen", "CtmMessages",
		"CtmTime", "CtmAction", "CtmMessages2", "CtmTime2", "CtmAction2", 
		"RctmMessages", "RctmTime", "RctmAction", "RctmMessages2", "RctmTime2",
		"RctmAction2", "MaxCtmLen", "MaxRctmLen", "SrMessages", "SrTime",
		"SrAction", "SrMessages2", "SrTime2", "SrAction2", "MaxSrLen",
		"MaxDownAction", "MaxDownKB", "MaxDownTime", "MaxDownAction2", "MaxDownKB2",
		"MaxDownTime2", "ChatIntervalMessages", "ChatIntervalTime", "PmIntervalMessages", "PmIntervalTime",
		"SearchIntervalMessages", "SearchIntervalTime", "MaxConnsSameIp", "MinReconnTime",
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
		"DbRemoveOldRecords",
#endif
	};

	for(uint8_t ui8i = 0; ui8i < sizeof(ui8Numbers); ui8i++) {
		lua_pushinteger(pLua, ui8Numbers[ui8i]);
		lua_setfield(pLua, iNewTable, pNumbersNames[ui8i]);
	}

	lua_setfield(pLua, iTable, "tNumbers");

	lua_newtable(pLua);
	iNewTable = lua_gettop(pLua);

	const uint8_t ui8Strings[] = { SETTXT_HUB_NAME, SETTXT_ADMIN_NICK, SETTXT_HUB_ADDRESS, SETTXT_TCP_PORTS, SETTXT_UDP_PORT, SETTXT_HUB_DESCRIPTION, SETTXT_REDIRECT_ADDRESS, 
		SETTXT_REGISTER_SERVERS, SETTXT_REG_ONLY_MSG, SETTXT_REG_ONLY_REDIR_ADDRESS, SETTXT_HUB_TOPIC, SETTXT_SHARE_LIMIT_MSG, SETTXT_SHARE_LIMIT_REDIR_ADDRESS, 
		SETTXT_SLOTS_LIMIT_MSG, SETTXT_SLOTS_LIMIT_REDIR_ADDRESS, SETTXT_HUB_SLOT_RATIO_MSG, SETTXT_HUB_SLOT_RATIO_REDIR_ADDRESS, SETTXT_MAX_HUBS_LIMIT_MSG, 
		SETTXT_MAX_HUBS_LIMIT_REDIR_ADDRESS, SETTXT_NO_TAG_MSG, SETTXT_NO_TAG_REDIR_ADDRESS, SETTXT_BOT_NICK, SETTXT_BOT_DESCRIPTION, SETTXT_BOT_EMAIL, 
		SETTXT_OP_CHAT_NICK, SETTXT_OP_CHAT_DESCRIPTION, SETTXT_OP_CHAT_EMAIL, SETTXT_TEMP_BAN_REDIR_ADDRESS, SETTXT_PERM_BAN_REDIR_ADDRESS, 
		SETTXT_CHAT_COMMANDS_PREFIXES, SETTXT_HUB_OWNER_EMAIL, SETTXT_NICK_LIMIT_MSG, SETTXT_NICK_LIMIT_REDIR_ADDRESS, SETTXT_MSG_TO_ADD_TO_BAN_MSG, 
		SETTXT_LANGUAGE, SETTXT_IPV4_ADDRESS, SETTXT_IPV6_ADDRESS, SETTXT_ENCODING, 
#ifdef _WITH_POSTGRES
		SETTXT_POSTGRES_HOST, SETTXT_POSTGRES_PORT, SETTXT_POSTGRES_DBNAME, SETTXT_POSTGRES_USER, SETTXT_POSTGRES_PASS,
#elif _WITH_MYSQL
		SETTXT_MYSQL_HOST, SETTXT_MYSQL_PORT, SETTXT_MYSQL_DBNAME, SETTXT_MYSQL_USER, SETTXT_MYSQL_PASS,
#endif
	};

	const char * pStringsNames[] = { "HubName", "AdminNick", "HubAddress", "TCPPorts", "UDPPort", "HubDescription", "MainRedirectAddress",
		"HublistRegisterAddresses", "RegOnlyMessage", "RegOnlyRedirAddress", "HubTopic", "ShareLimitMessage", "ShareLimitRedirAddress", 
		"SlotLimitMessage", "SlotLimitRedirAddress", "HubSlotRatioMessage", "HubSlotRatioRedirAddress", "MaxHubsLimitMessage", 
		"MaxHubsLimitRedirAddress", "NoTagMessage", "NoTagRedirAddress", "HubBotNick", "HubBotDescription", "HubBotEmail", 
		"OpChatNick", "OpChatDescription", "OpChatEmail", "TempBanRedirAddress", "PermBanRedirAddress", 
		"ChatCommandsPrefixes", "HubOwnerEmail", "NickLimitMessage", "NickLimitRedirAddress", "MessageToAddToBanMessage",
		"Language", "IPv4Address", "IPv6Address", "Encoding", 
#ifdef _WITH_POSTGRES
		"PostgresHost", "PostgresPort", "PostgresDBName", "PostgresUser", "PostgresPass",
#elif _WITH_MYSQL
		"MySQLHost", "MySQLPort", "MySQLDBName", "MySQLUser", "MySQLPass",
#endif
	};

	for(uint8_t ui8i = 0; ui8i < sizeof(ui8Strings); ui8i++) {
		lua_pushinteger(pLua, ui8Strings[ui8i]);
		lua_setfield(pLua, iNewTable, pStringsNames[ui8i]);
	}

	lua_setfield(pLua, iTable, "tStrings");

    lua_pop(pLua, 1);
}
//------------------------------------------------------------------------------

static void AddPermissionsIds(lua_State * pLua) {
	int iTable = lua_gettop(pLua);

	lua_newtable(pLua);
	int iNewTable = lua_gettop(pLua);

	const uint8_t ui8Permissions[] = { ProfileManager::HASKEYICON, ProfileManager::NODEFLOODGETNICKLIST, ProfileManager::NODEFLOODMYINFO, ProfileManager::NODEFLOODSEARCH, ProfileManager::NODEFLOODPM,
		ProfileManager::NODEFLOODMAINCHAT, ProfileManager::MASSMSG, ProfileManager::TOPIC, ProfileManager::TEMP_BAN, ProfileManager::REFRESHTXT,
		ProfileManager::NOTAGCHECK, ProfileManager::TEMP_UNBAN, ProfileManager::DELREGUSER, ProfileManager::ADDREGUSER, ProfileManager::NOCHATLIMITS,
		ProfileManager::NOMAXHUBCHECK, ProfileManager::NOSLOTHUBRATIO, ProfileManager::NOSLOTCHECK, ProfileManager::NOSHARELIMIT, ProfileManager::CLRPERMBAN,
		ProfileManager::CLRTEMPBAN, ProfileManager::GETINFO, ProfileManager::GETBANLIST, ProfileManager::RSTSCRIPTS, ProfileManager::RSTHUB,
		ProfileManager::TEMPOP, ProfileManager::GAG, ProfileManager::REDIRECT, ProfileManager::BAN, ProfileManager::KICK, ProfileManager::DROP,
		ProfileManager::ENTERFULLHUB, ProfileManager::ENTERIFIPBAN, ProfileManager::ALLOWEDOPCHAT, ProfileManager::SENDALLUSERIP, ProfileManager::RANGE_BAN,
		ProfileManager::RANGE_UNBAN, ProfileManager::RANGE_TBAN, ProfileManager::RANGE_TUNBAN, ProfileManager::GET_RANGE_BANS, ProfileManager::CLR_RANGE_BANS,
		ProfileManager::CLR_RANGE_TBANS, ProfileManager::UNBAN, ProfileManager::NOSEARCHLIMITS, ProfileManager:: SENDFULLMYINFOS, ProfileManager::NOIPCHECK,
		ProfileManager::CLOSE, ProfileManager::NODEFLOODCTM, ProfileManager::NODEFLOODRCTM, ProfileManager::NODEFLOODSR, ProfileManager::NODEFLOODRECV,
		ProfileManager::NOCHATINTERVAL, ProfileManager::NOPMINTERVAL, ProfileManager::NOSEARCHINTERVAL, ProfileManager::NOUSRSAMEIP, ProfileManager::NORECONNTIME
	};

	const char * pPermissionsNames[] = { "IsOperator", "NoDefloodGetnicklist", "NoDefloodMyinfo", "NoDefloodSearch", "NoDefloodPm",
		"NoDefloodMainChat", "MassMsg", "Topic", "TempBan", "ReloadTxtFiles",
		"NoTagCheck", "TempUnban", "DelRegUser", "AddRegUser", "NoChatLimits",
		"NoMaxHubsCheck", "NoSlotHubRatioCheck", "NoSlotCheck", "NoShareLimit", "ClrPermBan",
		"ClrTempBan", "GetInfo", "GetBans", "ScriptControl", "RstHub",
		"TempOp", "GagUngag", "Redirect", "Ban", "Kick", "Drop",
		"EnterIfHubFull", "EnterIfIpBan", "AllowedOpChat", "SendAllUsersIp", "RangeBan",
		"RangeUnban", "RangeTempBan", "RangeTempUnban", "GetRangeBans", "ClrRangePermBans",
		"ClrRangeTempBans", "Unban", "NoSearchLimits", "SendLongMyinfos", "NoIpCheck",
		"Close", "NoDefloodCtm", "NoDefloodRctm", "NoDefloodSr", "NoDefloodRecv",
		"NoChatInterval", "NoPmInterval", "NoSearchInterval", "NoMaxUsrSameIp", "NoReconnTime"
	};

	for(uint8_t ui8i = 0; ui8i < sizeof(ui8Permissions); ui8i++) {
		lua_pushinteger(pLua, ui8Permissions[ui8i]);
		lua_setfield(pLua, iNewTable, pPermissionsNames[ui8i]);
	}

	lua_setfield(pLua, iTable, "tPermissions");

    lua_pop(pLua, 1);
}
//------------------------------------------------------------------------------

bool ScriptStart(Script * pScript) {
	pScript->m_ui16Functions = 65535;
	pScript->m_ui32DataArrivals = 4294967295U;

	pScript->m_pPrev = NULL;
	pScript->m_pNext = NULL;

#ifdef _WIN32
	pScript->m_pLua = lua_newstate(LuaAlocator, NULL);
#else
	pScript->m_pLua = luaL_newstate();
#endif

    if(pScript->m_pLua == NULL) {
        return false;
    }

	luaL_openlibs(pScript->m_pLua);

	lua_atpanic(pScript->m_pLua, ScriptPanic);

    // replace internal lua os.exit with correct shutdown
    lua_getglobal(pScript->m_pLua, "os");

    if(lua_istable(pScript->m_pLua, -1)) {
        lua_pushcfunction(pScript->m_pLua, OsExit);
        lua_setfield(pScript->m_pLua, -2, "exit");

        lua_pop(pScript->m_pLua, 1);
    }

#if LUA_VERSION_NUM > 501
    luaL_requiref(pScript->m_pLua, "Core", RegCore, 1);
	lua_pop(pScript->m_pLua, 1);

    luaL_requiref(pScript->m_pLua, "SetMan", RegSetMan, 1);
	AddSettingIds(pScript->m_pLua);

    luaL_requiref(pScript->m_pLua, "RegMan", RegRegMan, 1);
    lua_pop(pScript->m_pLua, 1);

    luaL_requiref(pScript->m_pLua, "BanMan", RegBanMan, 1);
    lua_pop(pScript->m_pLua, 1);

    luaL_requiref(pScript->m_pLua, "ProfMan", RegProfMan, 1);
	AddPermissionsIds(pScript->m_pLua);

    luaL_requiref(pScript->m_pLua, "TmrMan", RegTmrMan, 1);
    lua_pop(pScript->m_pLua, 1);

    luaL_requiref(pScript->m_pLua, "UDPDbg", RegUDPDbg, 1);
    lua_pop(pScript->m_pLua, 1);

    luaL_requiref(pScript->m_pLua, "ScriptMan", RegScriptMan, 1);
    lua_pop(pScript->m_pLua, 1);

    luaL_requiref(pScript->m_pLua, "IP2Country", RegIP2Country, 1);
    lua_pop(pScript->m_pLua, 1);
#else
	RegCore(pScript->m_pLua);

	RegSetMan(pScript->m_pLua);

    lua_getglobal(pScript->m_pLua, "SetMan");

    if(lua_istable(pScript->m_pLua, -1)) {
        AddSettingIds(pScript->m_pLua);
    }

	RegRegMan(pScript->m_pLua);
	RegBanMan(pScript->m_pLua);

	RegProfMan(pScript->m_pLua);

    lua_getglobal(pScript->m_pLua, "ProfMan");

    if(lua_istable(pScript->m_pLua, -1)) {
        AddPermissionsIds(pScript->m_pLua);
    }

	RegTmrMan(pScript->m_pLua);
	RegUDPDbg(pScript->m_pLua);
	RegScriptMan(pScript->m_pLua);
	RegIP2Country(pScript->m_pLua);
#endif

	if(luaL_dofile(pScript->m_pLua, (ServerManager::m_sScriptPath+pScript->m_sName).c_str()) == 0) {
#ifdef _BUILD_GUI
        RichEditAppendText(MainWindowPageScripts::m_Ptr->m_hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS], 
			(string(LanguageManager::m_Ptr->m_sTexts[LAN_NO_SYNERR_IN_SCRIPT_FILE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_NO_SYNERR_IN_SCRIPT_FILE]) + " " + string(pScript->m_sName)).c_str());
#endif

        return true;
	} else {
        size_t szLen = 0;
        char * stmp = (char*)lua_tolstring(pScript->m_pLua, -1, &szLen);

        string sMsg(stmp, szLen);

#ifdef _BUILD_GUI
        RichEditAppendText(MainWindowPageScripts::m_Ptr->m_hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS],
            (string(LanguageManager::m_Ptr->m_sTexts[LAN_SYNTAX], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SYNTAX]) + " " + sMsg).c_str());
#endif

		UdpDebug::m_Ptr->BroadcastFormat("[LUA] %s", sMsg.c_str());

        if(SettingManager::m_Ptr->m_bBools[SETBOOL_LOG_SCRIPT_ERRORS] == true) {
            AppendLog(sMsg.c_str(), true);
        }

		lua_close(pScript->m_pLua);
		pScript->m_pLua = NULL;
    
        return false;
    }
}
//------------------------------------------------------------------------------

void ScriptStop(Script * pScript) {
	if(pScript->m_bRegUDP == true) {
        UdpDebug::m_Ptr->Remove(pScript->m_sName);
		pScript->m_bRegUDP = false;
    }

    ScriptTimer * pCurTmr = NULL,
        * pNextTmr = ScriptManager::m_Ptr->m_pTimerListS;
    
    while(pNextTmr != NULL) {
        pCurTmr = pNextTmr;
        pNextTmr = pCurTmr->m_pNext;

		if(pScript->m_pLua == pCurTmr->m_pLua) {
#if defined(_WIN32) && !defined(_WIN_IOT)
	        if(pCurTmr->m_uiTimerId != 0) {
	            KillTimer(NULL, pCurTmr->m_uiTimerId);
	        }
#endif
			if(pCurTmr->m_pPrev == NULL) {
				if(pCurTmr->m_pNext == NULL) {
					ScriptManager::m_Ptr->m_pTimerListS = NULL;
					ScriptManager::m_Ptr->m_pTimerListE = NULL;
				} else {
					ScriptManager::m_Ptr->m_pTimerListS = pCurTmr->m_pNext;
					ScriptManager::m_Ptr->m_pTimerListS->m_pPrev = NULL;
				}
			} else if(pCurTmr->m_pNext == NULL) {
				ScriptManager::m_Ptr->m_pTimerListE = pCurTmr->m_pPrev;
				ScriptManager::m_Ptr->m_pTimerListE->m_pNext = NULL;
			} else {
				pCurTmr->m_pPrev->m_pNext = pCurTmr->m_pNext;
				pCurTmr->m_pNext->m_pPrev = pCurTmr->m_pPrev;
			}

			delete pCurTmr;
		}
    }

    if(pScript->m_pLua != NULL) {
        lua_close(pScript->m_pLua);
		pScript->m_pLua = NULL;
    }

    ScriptBot * pBot = NULL,
        * next = pScript->m_pBotList;
    
    while(next != NULL) {
        pBot = next;
        next = pBot->m_pNext;

        ReservedNicksManager::m_Ptr->DelReservedNick(pBot->m_sNick, true);

        if(ServerManager::m_bServerRunning == true) {
   			Users::m_Ptr->DelFromNickList(pBot->m_sNick, pBot->m_bIsOP);

            Users::m_Ptr->DelBotFromMyInfos(pBot->m_sMyINFO);

			int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Quit %s|", pBot->m_sNick);
           	if(iMsgLen > 0) {
                GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, iMsgLen, NULL, 0, GlobalDataQueue::CMD_QUIT);
            }
		}

		delete pBot;
    }

	pScript->m_pBotList = NULL;
}
//------------------------------------------------------------------------------

int ScriptGetGC(Script * pScript) {
	return lua_gc(pScript->m_pLua, LUA_GCCOUNT, 0);
}
//------------------------------------------------------------------------------

void ScriptOnStartup(Script * pScript) {
    lua_pushcfunction(pScript->m_pLua, ScriptTraceback);
    int iTraceback = lua_gettop(pScript->m_pLua);

    lua_getglobal(pScript->m_pLua, "OnStartup");
    int i = lua_gettop(pScript->m_pLua);

    if(lua_isfunction(pScript->m_pLua, i) == 0) {
		pScript->m_ui16Functions &= ~Script::ONSTARTUP;
		lua_settop(pScript->m_pLua, 0);
        return;
    }

    if(lua_pcall(pScript->m_pLua, 0, 0, iTraceback) != 0) {
        ScriptError(pScript);

        lua_settop(pScript->m_pLua, 0);
        return;
    }

    // clear the stack for sure
    lua_settop(pScript->m_pLua, 0);
}
//------------------------------------------------------------------------------

void ScriptOnExit(Script * pScript) {
    lua_pushcfunction(pScript->m_pLua, ScriptTraceback);
    int iTraceback = lua_gettop(pScript->m_pLua);

    lua_getglobal(pScript->m_pLua, "OnExit");
    int i = lua_gettop(pScript->m_pLua);
    if(lua_isfunction(pScript->m_pLua, i) == 0) {
		pScript->m_ui16Functions &= ~Script::ONEXIT;
		lua_settop(pScript->m_pLua, 0);
        return;
    }

    if(lua_pcall(pScript->m_pLua, 0, 0, iTraceback) != 0) {
        ScriptError(pScript);

        lua_settop(pScript->m_pLua, 0);
        return;
	}

	// clear the stack for sure
    lua_settop(pScript->m_pLua, 0);
}
//------------------------------------------------------------------------------

static bool ScriptOnError(Script * pScript, char * sErrorMsg, const size_t szMsgLen) {
    lua_pushcfunction(pScript->m_pLua, ScriptTraceback);
    int iTraceback = lua_gettop(pScript->m_pLua);

	lua_getglobal(pScript->m_pLua, "OnError");
    int i = lua_gettop(pScript->m_pLua);
    if(lua_isfunction(pScript->m_pLua, i) == 0) {
		pScript->m_ui16Functions &= ~Script::ONERROR;
		lua_settop(pScript->m_pLua, 0);
        return true;
    }

	ScriptManager::m_Ptr->m_pActualUser = NULL;
    
    lua_pushlstring(pScript->m_pLua, sErrorMsg, szMsgLen);

	if(lua_pcall(pScript->m_pLua, 1, 0, iTraceback) != 0) { // 1 passed parameters, zero returned
		size_t szLen = 0;
		char * stmp = (char*)lua_tolstring(pScript->m_pLua, -1, &szLen);

		string sMsg(stmp, szLen);

#ifdef _BUILD_GUI
        RichEditAppendText(MainWindowPageScripts::m_Ptr->m_hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS],
            (string(LanguageManager::m_Ptr->m_sTexts[LAN_SYNTAX], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SYNTAX]) + " " + sMsg).c_str());
        RichEditAppendText(MainWindowPageScripts::m_Ptr->m_hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS],
            (string(LanguageManager::m_Ptr->m_sTexts[LAN_FATAL_ERR_SCRIPT], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_FATAL_ERR_SCRIPT]) + " " + string(pScript->m_sName) + " ! " +
			string(LanguageManager::m_Ptr->m_sTexts[LAN_SCRIPT_STOPPED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SCRIPT_STOPPED]) + "!").c_str());
#endif

		if(SettingManager::m_Ptr->m_bBools[SETBOOL_LOG_SCRIPT_ERRORS] == true) {
			AppendLog(sMsg.c_str(), true);
		}

        lua_settop(pScript->m_pLua, 0);
        return false;
    }

    // clear the stack for sure
    lua_settop(pScript->m_pLua, 0);
    return true;
}
//------------------------------------------------------------------------------

void ScriptPushUser(lua_State * pLua, User * pUser, const bool bFullTable/* = false*/) {
	lua_checkstack(pLua, 3); // we need 3 (1 table, 2 id, 3 value) empty slots in stack, check it to be sure

	lua_newtable(pLua);
	int i = lua_gettop(pLua);

	lua_pushliteral(pLua, "sNick");
	lua_pushlstring(pLua, pUser->m_sNick, pUser->m_ui8NickLen);
	lua_rawset(pLua, i);

	lua_pushliteral(pLua, "uptr");
	lua_pushlightuserdata(pLua, (void *)pUser);
	lua_rawset(pLua, i);

	lua_pushliteral(pLua, "sIP");
	lua_pushlstring(pLua, pUser->m_sIP, pUser->m_ui8IpLen);
	lua_rawset(pLua, i);

	lua_pushliteral(pLua, "iProfile");
	lua_pushinteger(pLua, pUser->m_i32Profile);
	lua_rawset(pLua, i);

    if(bFullTable == true) {
        ScriptPushUserExtended(pLua, pUser, i);
    }
}
//------------------------------------------------------------------------------

void ScriptPushUserExtended(lua_State * pLua, User * pUser, const int iTable) {
	lua_pushliteral(pLua, "sMode");
	if(pUser->m_sModes[0] != '\0') {
		lua_pushstring(pLua, pUser->m_sModes);
	} else {
		lua_pushnil(pLua);
	}
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "sMyInfoString");
	if(pUser->m_sMyInfoOriginal != NULL) {
		lua_pushlstring(pLua, pUser->m_sMyInfoOriginal, pUser->m_ui16MyInfoOriginalLen);
	} else {
		lua_pushnil(pLua);
	}
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "sDescription");
	if(pUser->m_sDescription != NULL) {
		lua_pushlstring(pLua, pUser->m_sDescription, (size_t)pUser->m_ui8DescriptionLen);
	} else {
		lua_pushnil(pLua);
	}
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "sTag");
	if(pUser->m_sTag != NULL) {
		lua_pushlstring(pLua, pUser->m_sTag, (size_t)pUser->m_ui8TagLen);
	} else {
		lua_pushnil(pLua);
	}
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "sConnection");
	if(pUser->m_sConnection != NULL) {
		lua_pushlstring(pLua, pUser->m_sConnection, (size_t)pUser->m_ui8ConnectionLen);
	} else {
		lua_pushnil(pLua);
	}
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "sEmail");
	if(pUser->m_sEmail != NULL) {
		lua_pushlstring(pLua, pUser->m_sEmail, (size_t)pUser->m_ui8EmailLen);
	} else {
		lua_pushnil(pLua);
	}
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "sClient");
	if(pUser->m_sClient != NULL) {
		lua_pushlstring(pLua, pUser->m_sClient, (size_t)pUser->m_ui8ClientLen);
	} else {
		lua_pushnil(pLua);
	}
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "sClientVersion");
	if(pUser->m_sTagVersion != NULL) {
		lua_pushlstring(pLua, pUser->m_sTagVersion, (size_t)pUser->m_ui8TagVersionLen);
	} else {
		lua_pushnil(pLua);
	}
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "sVersion");
	if(pUser->m_sVersion != NULL) {
		lua_pushstring(pLua, pUser->m_sVersion);
	} else {
		lua_pushnil(pLua);
	}
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "sCountryCode");
	if(IpP2Country::m_Ptr->m_ui32Count != 0) {
		lua_pushlstring(pLua, IpP2Country::m_Ptr->GetCountry(pUser->m_ui8Country, false), 2);
	} else {
		lua_pushnil(pLua);
	}
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "bConnected");
	pUser->m_ui8State == User::STATE_ADDED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "bActive");
	if((pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
        (pUser->m_ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    } else {
	   (pUser->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    }
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "bOperator");
	(pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "bUserCommand");
	(pUser->m_ui32SupportBits & User::SUPPORTBIT_USERCOMMAND) == User::SUPPORTBIT_USERCOMMAND ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "bQuickList");
	(pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "bSuspiciousTag");
	(pUser->m_ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "iShareSize");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, (double)pUser->m_ui64SharedSize);
#else
	lua_pushinteger(pLua, pUser->m_ui64SharedSize);
#endif
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "iHubs");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, pUser->m_ui32Hubs);
#else
	lua_pushinteger(pLua, pUser->m_ui32Hubs);
#endif
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "iNormalHubs");
#if LUA_VERSION_NUM < 503
	(pUser->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushnumber(pLua, pUser->m_ui32NormalHubs);
#else
	(pUser->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, pUser->m_ui32NormalHubs);
#endif
	lua_rawset(pLua, iTable);
    
	lua_pushliteral(pLua, "iRegHubs");
#if LUA_VERSION_NUM < 503
	(pUser->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushnumber(pLua, pUser->m_ui32RegHubs);
#else
	(pUser->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, pUser->m_ui32RegHubs);
#endif
	lua_rawset(pLua, iTable);
    
	lua_pushliteral(pLua, "iOpHubs");
#if LUA_VERSION_NUM < 503
	(pUser->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushnumber(pLua, pUser->m_ui32OpHubs);
#else
	(pUser->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, pUser->m_ui32OpHubs);
#endif
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "iSlots");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, pUser->m_ui32Slots);
#else
	lua_pushinteger(pLua, pUser->m_ui32Slots);
#endif
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "iLlimit");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, pUser->m_ui32LLimit);
#else
	lua_pushinteger(pLua, pUser->m_ui32LLimit);
#endif
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "iDefloodWarns");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, pUser->m_ui32DefloodWarnings);
#else
	lua_pushinteger(pLua, pUser->m_ui32DefloodWarnings);
#endif
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "iMagicByte");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, pUser->m_ui8MagicByte);
#else
	lua_pushinteger(pLua, pUser->m_ui8MagicByte);
#endif
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "iLoginTime");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, (double)pUser->m_tLoginTime);
#else
	lua_pushinteger(pLua, pUser->m_tLoginTime);
#endif
	lua_rawset(pLua, iTable);

	lua_pushliteral(pLua, "sMac");
    char sMac[18];
    if(GetMacAddress(pUser->m_sIP, sMac) == true) {
        lua_pushlstring(pLua, sMac, 17);
    } else {
        lua_pushnil(pLua);
    }
	lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "bDescriptionChanged");
    (pUser->m_ui32InfoBits & User::INFOBIT_DESCRIPTION_CHANGED) == User::INFOBIT_DESCRIPTION_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "bTagChanged");
    (pUser->m_ui32InfoBits & User::INFOBIT_TAG_CHANGED) == User::INFOBIT_TAG_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "bConnectionChanged");
    (pUser->m_ui32InfoBits & User::INFOBIT_CONNECTION_CHANGED) == User::INFOBIT_CONNECTION_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "bEmailChanged");
    (pUser->m_ui32InfoBits & User::INFOBIT_EMAIL_CHANGED) == User::INFOBIT_EMAIL_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "bShareChanged");
    (pUser->m_ui32InfoBits & User::INFOBIT_SHARE_CHANGED) == User::INFOBIT_SHARE_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sScriptedDescriptionShort");
    if(pUser->m_sChangedDescriptionShort != NULL) {
		lua_pushlstring(pLua, pUser->m_sChangedDescriptionShort, pUser->m_ui8ChangedDescriptionShortLen);
	} else {
		lua_pushnil(pLua);
	}
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sScriptedDescriptionLong");
    if(pUser->m_sChangedDescriptionLong != NULL) {
		lua_pushlstring(pLua, pUser->m_sChangedDescriptionLong, pUser->m_ui8ChangedDescriptionLongLen);
	} else {
		lua_pushnil(pLua);
	}
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sScriptedTagShort");
    if(pUser->m_sChangedTagShort != NULL) {
		lua_pushlstring(pLua, pUser->m_sChangedTagShort, pUser->m_ui8ChangedTagShortLen);
	} else {
		lua_pushnil(pLua);
	}
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sScriptedTagLong");
    if(pUser->m_sChangedTagLong != NULL) {
		lua_pushlstring(pLua, pUser->m_sChangedTagLong, pUser->m_ui8ChangedTagLongLen);
	} else {
		lua_pushnil(pLua);
	}
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sScriptedConnectionShort");
    if(pUser->m_sChangedConnectionShort != NULL) {
		lua_pushlstring(pLua, pUser->m_sChangedConnectionShort, pUser->m_ui8ChangedConnectionShortLen);
	} else {
		lua_pushnil(pLua);
	}
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sScriptedConnectionLong");
    if(pUser->m_sChangedConnectionLong != NULL) {
		lua_pushlstring(pLua, pUser->m_sChangedConnectionLong, pUser->m_ui8ChangedConnectionLongLen);
	} else {
		lua_pushnil(pLua);
	}
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sScriptedEmailShort");
    if(pUser->m_sChangedEmailShort != NULL) {
		lua_pushlstring(pLua, pUser->m_sChangedEmailShort, pUser->m_ui8ChangedEmailShortLen);
	} else {
		lua_pushnil(pLua);
	}
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "sScriptedEmailLong");
    if(pUser->m_sChangedEmailLong != NULL) {
		lua_pushlstring(pLua, pUser->m_sChangedEmailLong, pUser->m_ui8ChangedEmailLongLen);
	} else {
		lua_pushnil(pLua);
	}
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "iScriptediShareSizeShort");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, (double)pUser->m_ui64ChangedSharedSizeShort);
#else
    lua_pushinteger(pLua, pUser->m_ui64ChangedSharedSizeShort);
#endif
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "iScriptediShareSizeLong");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, (double)pUser->m_ui64ChangedSharedSizeLong);
#else
    lua_pushinteger(pLua, pUser->m_ui64ChangedSharedSizeLong);
#endif
    lua_rawset(pLua, iTable);

    lua_pushliteral(pLua, "tIPs");
    lua_newtable(pLua);

    int t = lua_gettop(pLua);

#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, 1);
#else
	lua_pushinteger(pLua, 1);
#endif
	lua_pushlstring(pLua, pUser->m_sIP, pUser->m_ui8IpLen);
	lua_rawset(pLua, t);

    if(pUser->m_sIPv4[0] != '\0') {
#if LUA_VERSION_NUM < 503
		lua_pushnumber(pLua, 2);
#else
        lua_pushinteger(pLua, 2);
#endif
        lua_pushlstring(pLua, pUser->m_sIPv4, pUser->m_ui8IPv4Len);
        lua_rawset(pLua, t);
    }

    lua_rawset(pLua, iTable);
}
//------------------------------------------------------------------------------

User * ScriptGetUser(lua_State * pLua, const int iTop, const char * sFunction) {
    lua_pushliteral(pLua, "uptr");
    lua_gettable(pLua, 1);

    if(lua_gettop(pLua) != iTop+1 || lua_type(pLua, iTop+1) != LUA_TLIGHTUSERDATA) {
        luaL_error(pLua, "bad argument #1 to '%s' (it's not user table)", sFunction);
        return NULL;
    }

    User *u = reinterpret_cast<User *>(lua_touserdata(pLua, iTop+1));
                
    if(u == NULL) {
        return NULL;
    }

	if(u != ScriptManager::m_Ptr->m_pActualUser) {
        lua_pushliteral(pLua, "sNick");
        lua_gettable(pLua, 1);

        if(lua_gettop(pLua) != iTop+2 || lua_type(pLua, iTop+2) != LUA_TSTRING) {
            luaL_error(pLua, "bad argument #1 to '%s' (it's not user table)", sFunction);
            return NULL;
        }
    
        size_t szNickLen;
        char * sNick = (char *)lua_tolstring(pLua, iTop+2, &szNickLen);

		if(u != HashManager::m_Ptr->FindUser(sNick, szNickLen)) {
            return NULL;
        }
    }

    return u;
}
//------------------------------------------------------------------------------

void ScriptError(Script * pScript) {
	size_t szLen = 0;
	char * stmp = (char*)lua_tolstring(pScript->m_pLua, -1, &szLen);

	string sMsg(stmp, szLen);

#ifdef _BUILD_GUI
    RichEditAppendText(MainWindowPageScripts::m_Ptr->m_hWndPageItems[MainWindowPageScripts::REDT_SCRIPTS_ERRORS],
        (string(LanguageManager::m_Ptr->m_sTexts[LAN_SYNTAX], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_SYNTAX]) + " " + sMsg).c_str());
#endif

	UdpDebug::m_Ptr->BroadcastFormat("[LUA] %s", sMsg.c_str());

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_LOG_SCRIPT_ERRORS] == true) {
        AppendLog(sMsg.c_str(), true);
    }

	if((((pScript->m_ui16Functions & Script::ONERROR) == Script::ONERROR) == true && ScriptOnError(pScript, stmp, szLen) == false) ||
        SettingManager::m_Ptr->m_bBools[SETBOOL_STOP_SCRIPT_ON_ERROR] == true) {
        // PPK ... stop buggy script ;)
		EventQueue::m_Ptr->AddNormal(EventQueue::EVENT_STOPSCRIPT, pScript->m_sName);
    }
}
//------------------------------------------------------------------------------

#if defined(_WIN32) && !defined(_WIN_IOT)
    void ScriptOnTimer(const UINT_PTR uiTimerId) {
#else
	void ScriptOnTimer(const uint64_t &ui64ActualMillis) {
#endif
	lua_State * pLuaState = NULL;

    ScriptTimer * pCurTmr = NULL,
        * pNextTmr = ScriptManager::m_Ptr->m_pTimerListS;
        
    while(pNextTmr != NULL) {
        pCurTmr = pNextTmr;
        pNextTmr = pCurTmr->m_pNext;

#if defined(_WIN32) && !defined(_WIN_IOT)
        if(pCurTmr->m_uiTimerId == uiTimerId) {
#else
		while((pCurTmr->m_ui64LastTick < ui64ActualMillis) && (ui64ActualMillis - pCurTmr->m_ui64LastTick) >= pCurTmr->m_ui64Interval) {
			pCurTmr->m_ui64LastTick += pCurTmr->m_ui64Interval;
#endif
            lua_pushcfunction(pCurTmr->m_pLua, ScriptTraceback);
            int iTraceback = lua_gettop(pCurTmr->m_pLua);

            if(pCurTmr->m_sFunctionName != NULL) {
				lua_getglobal(pCurTmr->m_pLua, pCurTmr->m_sFunctionName);
				int i = lua_gettop(pCurTmr->m_pLua);

				if(lua_isfunction(pCurTmr->m_pLua, i) == 0) {
					lua_settop(pCurTmr->m_pLua, 0);
#if defined(_WIN32) && !defined(_WIN_IOT)
				    return;
#else
					continue;
#endif
				}
			} else {
                lua_rawgeti(pCurTmr->m_pLua, LUA_REGISTRYINDEX, pCurTmr->m_iFunctionRef);
            }

			ScriptManager::m_Ptr->m_pActualUser = NULL;

			lua_checkstack(pCurTmr->m_pLua, 1); // we need 1 empty slots in stack, check it to be sure

#if defined(_WIN32) && !defined(_WIN_IOT)
            lua_pushlightuserdata(pCurTmr->m_pLua, (void *)uiTimerId);
#else
			lua_pushlightuserdata(pCurTmr->m_pLua, (void *)pCurTmr);
#endif

			pLuaState = pCurTmr->m_pLua; // For case when timer will be removed in OnTimer
	
			// 1 passed parameters, 0 returned
			if(lua_pcall(pCurTmr->m_pLua, 1, 0, iTraceback) != 0) {
				ScriptError(ScriptManager::m_Ptr->FindScript(pCurTmr->m_pLua));

				lua_settop(pCurTmr->m_pLua, 0);
#if defined(_WIN32) && !defined(_WIN_IOT)
				return;
#else
				if(pNextTmr == NULL) {
					if(ScriptManager::m_Ptr->m_pTimerListE != pCurTmr) {
						break;
					}
				} else if(pNextTmr->m_pPrev != pCurTmr) {
					break;
				}

				continue;
#endif
			}

			// clear the stack for sure
			lua_settop(pLuaState, 0);
#if defined(_WIN32) && !defined(_WIN_IOT)
			return;
#else
			if(pNextTmr == NULL) {
				if(ScriptManager::m_Ptr->m_pTimerListE != pCurTmr) {
					break;
				}
			} else if(pNextTmr->m_pPrev != pCurTmr) {
				break;
			}

			continue;
#endif
        }

	}
}
//------------------------------------------------------------------------------

int ScriptTraceback(lua_State * pLua) {
#if LUA_VERSION_NUM > 501
    const char * sMsg = lua_tostring(pLua, 1);
    if(sMsg != NULL) {
        luaL_traceback(pLua, pLua, sMsg, 1);
        return 1;
    }

    return 0;
#else
    if(!lua_isstring(pLua, 1)) {
        return 1;
    }

    lua_getfield(pLua, LUA_GLOBALSINDEX, "debug");
    if(!lua_istable(pLua, -1)) {
        lua_pop(pLua, 1);
        return 1;
    }

    lua_getfield(pLua, -1, "traceback");
    if(lua_isfunction(pLua, -1) == 0) {
        lua_pop(pLua, 2);
        return 1;
    }

    lua_pushvalue(pLua, 1);
    lua_pushinteger(pLua, 2);
    lua_call(pLua, 2, 1);
    return 1;
#endif
}
//------------------------------------------------------------------------------
