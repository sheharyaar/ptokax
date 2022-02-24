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
#ifndef SettingDefaultsH
#define SettingDefaultsH
//---------------------------------------------------------------------------

bool SetBoolDef[] = {
    true, //ANTI_MOGLO
    false, //AUTO_START
    false, //REDIRECT_ALL
    true, //REDIRECT_WHEN_HUB_FULL
    true, //AUTO_REG
    false, //REG_ONLY
    false, //REG_ONLY_REDIR
    true, //SHARE_LIMIT_REDIR
    true, //SLOTS_LIMIT_REDIR
    true, //HUB_SLOT_RATIO_REDIR
    true, //MAX_HUBS_LIMIT_REDIR
    true, //MODE_TO_MYINFO
    false, //MODE_TO_DESCRIPTION
    false, //STRIP_DESCRIPTION
    true, //STRIP_TAG
    false, //STRIP_CONNECTION
    false, //STRIP_EMAIL
    true, //REG_BOT
    true, //USE_BOT_NICK_AS_HUB_SEC
    true, //REG_OP_CHAT
    true, //TEMP_BAN_REDIR
    true, //PERM_BAN_REDIR
    true, //ENABLE_SCRIPTING
    true, //KEEP_SLOW_USERS
    true, //CHECK_NEW_RELEASES
    true, //ENABLE_TRAY_ICON
    false, //START_MINIMIZED
    true, //FILTER_KICK_MESSAGES
    true, //SEND_KICK_MESSAGES_TO_OPS
    true, //SEND_STATUS_MESSAGES
    false, //SEND_STATUS_MESSAGES_AS_PM
    true, //ENABLE_TEXT_FILES
    false, //SEND_TEXT_FILES_AS_PM
    false, //STOP_SCRIPT_ON_ERROR
    false, //MOTD_AS_PM
    false, //DEFLOOD_REPORT
    false, //REPLY_TO_HUB_COMMANDS_AS_PM
    false, //DISABLE_MOTD
    false, //DONT_ALLOW_PINGERS
    false, //REPORT_PINGERS
    true, //REPORT_3X_BAD_PASS
    true, //ADVANCED_PASS_PROTECTION
    false, //BIND_ONLY_SINGLE_IP
    true, //RESOLVE_TO_IP
    true, //NICK_LIMIT_REDIR
/*OBSOLOTE*/true, //ABSOLOTE_SEND_USERIP2_TO_USER_ON_LOGIN
    true, //BAN_MSG_SHOW_IP
    true, //BAN_MSG_SHOW_RANGE
    true, //BAN_MSG_SHOW_NICK
    true, //BAN_MSG_SHOW_REASON
    true, //BAN_MSG_SHOW_BY
    true, //REPORT_SUSPICIOUS_TAG
/*OBSOLETE*/true, //ACCEPT_UNKNOWN_TAG
/*OBSOLETE*/true, //CHECK_IP_IN_COMMANDS
/*OBSOLETE*/false, //ABSOLOTE_POPUP_SCRIPT_WINDOW
#ifdef _BUILD_GUI
    false, //LOG_SCRIPT_ERRORS
#else
	true, //LOG_SCRIPT_ERRORS
#endif
    false, //NO_QUACK_SUPPORTS
    false, //HASH_PASSWORDS
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
    true, //ENABLE_DATABASE,
#endif
};

int16_t SetShortDef[] = {
    500, //MAX_USERS
    0, //MIN_SHARE_LIMIT
    0, //MIN_SHARE_UNITS
    0, //MAX_SHARE_LIMIT
    0, //MAX_SHARE_UNITS
    0, //MIN_SLOTS_LIMIT
    0, //MAX_SLOTS_LIMIT
    0, //HUB_SLOT_RATIO_HUBS
    0, //HUB_SLOT_RATIO_SLOTS
    0, //MAX_HUBS_LIMIT
    0, //NO_TAG_OPTION
    1, //FULL_MYINFO_OPTION
    300, //MAX_CHAT_LEN
    5, //MAX_CHAT_LINES
    512, //MAX_PM_LEN
    25, //MAX_PM_LINES
    20, //DEFAULT_TEMP_BAN_TIME
    100, //MAX_PASIVE_SR
    30, //MYINFO_DELAY
    20, //MAIN_CHAT_MESSAGES
    20, //MAIN_CHAT_TIME
    2, //MAIN_CHAT_ACTION
    5, //SAME_MAIN_CHAT_MESSAGES
    60, //SAME_MAIN_CHAT_TIME
    2, //SAME_MAIN_CHAT_ACTION
    2, //SAME_MULTI_MAIN_CHAT_MESSAGES
    2, //SAME_MULTI_MAIN_CHAT_LINES
    3, //SAME_MULTI_MAIN_CHAT_ACTION
    10, //PM_MESSAGES
    10, //PM_TIME
    2, //PM_ACTION
    5, //SAME_PM_MESSAGES
    60, //SAME_PM_TIME
    2, //SAME_PM_ACTION
    2, //SAME_MULTI_PM_MESSAGES
    2, //SAME_MULTI_PM_LINES
    3, //SAME_MULTI_PM_ACTION
    2, //SEARCH_MESSAGES
    10, //SEARCH_TIME
    1, //SEARCH_ACTION
    1, //SAME_SEARCH_MESSAGES
    60, //SAME_SEARCH_TIME
    1, //SAME_SEARCH_ACTION
    6, //MYINFO_MESSAGES
    60, //MYINFO_TIME
    2, //MYINFO_ACTION
    1, //GETNICKLIST_MESSAGES
    120, //GETNICKLIST_TIME
    3, //GETNICKLIST_ACTION
    10, //NEW_CONNECTIONS_COUNT
    60, //NEW_CONNECTIONS_TIME
    6, //DEFLOOD_WARNING_COUNT
    2, //DEFLOOD_WARNING_ACTION
    240, //DEFLOOD_TEMP_BAN_TIME
    20, //GLOBAL_MAIN_CHAT_MESSAGES
    10, //GLOBAL_MAIN_CHAT_TIME
    10, //GLOBAL_MAIN_CHAT_TIMEOUT
    2, //GLOBAL_MAIN_CHAT_ACTION
    1, //MIN_SEARCH_LEN
    96, //MAX_SEARCH_LEN
    2, //MIN_NICK_LEN
    64, //MAX_NICK_LEN
    1, //BRUTE_FORCE_PASS_PROTECT_BAN_TYPE
    24, //BRUTE_FORCE_PASS_PROTECT_TEMP_BAN_TIME
    100, //MAX_PM_COUNT_TO_USER
    25, //MAX_SIMULTANEOUS_LOGINS
    120, //MAIN_CHAT_MESSAGES2
    600, //MAIN_CHAT_TIME2
    3, //MAIN_CHAT_ACTION2
    60, //PM_MESSAGES2
    300, //PM_TIME2
    3, //PM_ACTION2
    31, //SEARCH_MESSAGES2
    300, //SEARCH_TIME2
    3, //SEARCH_ACTION2
    30, //MYINFO_MESSAGES2
    900, //MYINFO_TIME2
    3, //MYINFO_ACTION2
    256, //MAX_MYINFO_LEN
    500, //CTM_MESSAGES
    60, //CTM_TIME
    1, //CTM_ACTION
    5000, //CTM_MESSAGES2
    600, //CTM_TIME2
    0, //CTM_ACTION2
    250, //RCTM_MESSAGES
    60, //RCTM_TIME
    1, //RCTM_ACTION
    2500, //RCTM_MESSAGES2
    600, //RCTM_TIME2
    3, //RCTM_ACTION2
    128, //MAX_CTM_LEN
    160, //MAX_RCTM_LEN
    1000, //SR_MESSAGES
    60, //SR_TIME
    1, //SR_ACTION
    10000, //SR_MESSAGES2
    600, //SR_TIME2
    0, //SR_ACTION2
    1024, //MAX_SR_LEN
    4, //MAX_DOWN_ACTION, 
    128, //MAX_DOWN_KB, 
    60, //MAX_DOWN_TIME, 
    5, //MAX_DOWN_ACTION2, 
    256, //MAX_DOWN_KB2, 
    300, //MAX_DOWN_TIME2, 
    5, //CHAT_INTERVAL_MESSAGES, 
    10, //CHAT_INTERVAL_TIME, 
    5, //PM_INTERVAL_MESSAGES, 
    10, //PM_INTERVAL_TIME, 
    5, //SEARCH_INTERVAL_MESSAGES, 
    60, //SEARCH_INTERVAL_TIME, 
    5, //MAX_CONN_SAME_IP
    10, //MIN_RECONN_TIME
#if defined(_WITH_SQLITE) || defined(_WITH_POSTGRES) || defined(_WITH_MYSQL)
    0, //DB_REMOVE_OLD_RECORDS
#endif
    0, //SETSHORT_MAX_USERS_PEAK
};

const char* SetTxtDef[] = {
    "MetaHub", //HUB_NAME
    "Admin", //ADMIN_NICK
    "10.112.5.167", //HUB_ADDRESS
    "1209;411", //TCP_PORTS
    "0", //UDP_PORT
    "", //HUB_DESCRIPTION
    "10.112.5.167:411", //REDIRECT_ADDRESS
    "reg.hublist.org;serv.hubs-list.com;hublist.cz;hublist.dreamland-net.eu;allhublista.myip.hu;publichublist-nl.no-ip.org;reg.hublist.dk;hublist.te-home.net;dc.gwhublist.com", //REGISTER_SERVERS
    "Sorry, this hub is only for registered users.", //REG_ONLY_MSG
    "", //REG_ONLY_REDIR_ADDRESS
    "", //HUB_TOPIC
    "Your share is outside the limits. Min share is %[min], max share is %[max].", //SHARE_LIMIT_MSG
    "", //SHARE_LIMIT_REDIR_ADDRESS
    "Your slots count is outside the limits. Min slots limit is %[min], max slots limit is %[max].", //SLOTS_LIMIT_MSG
    "", //SLOTS_LIMIT_REDIR_ADDRESS
    "Your hubs/slots ratio outside the limit. Maximum allowed ratio is %[hubs]/%[slots].", //HUB_SLOT_RATIO_MSG
    "", //HUB_SLOT_RATIO_REDIR_ADDRESS
    "Your hubs count is higher than allowed %[hubs] hubs.", //MAX_HUBS_LIMIT_MSG
    "", //MAX_HUBS_LIMIT_REDIR_ADDRESS
    "Your client don't send description tag, or your client is not supported here.", //NO_TAG_MSG
    "", //NO_TAG_REDIR_ADDRESS
    "PtokaX", //BOT_NICK
    "", //BOT_DESCRIPTION
    "", //BOT_EMAIL 
    "OpChat", //OP_CHAT_NICK 
    "", //OP_CHAT_DESCRIPTION
    "", //OP_CHAT_EMAIL
    "", //TEMP_BAN_REDIR_ADDRESS
    "", //PERM_BAN_REDIR_ADDRESS
    "!+-/*", //CHAT_COMMANDS_PREFIXES
    "", //HUB_OWNER_EMAIL
    "Your nick length is outside the limit. Allowed min is %[min] and max %[max].", //NICK_LIMIT_MSG
    "", //NICK_LIMIT_REDIR_ADDRESS
    "", //MSG_TO_ADD_TO_BAN_MSG
    "", //LANGUAGE
    "", //IPV4_ADDRESS
    "", //IPV6_ADDRESS
	"cp1252", //ENCODING
#ifdef _WITH_POSTGRES
	"localhost", //POSTGRES_HOST
	"5432", //POSTGRES_PORT
	"ptokax", //POSTGRES_DBNAME
	"ptokax", //POSTGRES_USER
	"", //POSTGRES_PASS
#elif _WITH_MYSQL
	"localhost", // MYSQL_HOST,
	"3306", // MYSQL_PORT,
	"ptokax", //MYSQL_DBNAME,
	"ptokax", //MYSQL_USER,
	"", //MYSQL_PASS,
#endif
};
//---------------------------------------------------------------------------

#endif
