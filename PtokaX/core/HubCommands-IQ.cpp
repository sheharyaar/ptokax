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
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "HubCommands.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _BUILD_GUI
    #include "../gui.win/RegisteredUsersDialog.h"
#endif
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::MyIp(ChatCommand * pChatCommand) { // !myip
    if(pChatCommand->m_pUser->m_sIPv4[0] != '\0') {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::MyIp1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s: %s / %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_YOUR_IP_IS], pChatCommand->m_pUser->m_sIP, pChatCommand->m_pUser->m_sIPv4);
    } else {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::MyIp2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s: %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_YOUR_IP_IS], pChatCommand->m_pUser->m_sIP);
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::MassMsg(ChatCommand * pChatCommand) { // !massmsg text
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::MASSMSG) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 9) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::MassMsg1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cmassmsg <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_MESSAGE_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

	UncountDeflood(pChatCommand);

    if(pChatCommand->m_ui32CommandLen > 64000) {
        pChatCommand->m_sCommand[64000] = '\0';
    }

    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s $<%s> %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, pChatCommand->m_sCommand+8);
    if(iMsgLen > 0) {
		GlobalDataQueue::m_Ptr->SingleItemStore(ServerManager::m_pGlobalBuffer, iMsgLen, pChatCommand->m_pUser, 0, GlobalDataQueue::SI_PM2ALL);
    }

    pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::MassMsg2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
		LanguageManager::m_Ptr->m_sTexts[LAN_MASSMSG_TO_ALL_SENT]);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::NickBan(ChatCommand * pChatCommand) { // !nickban nick reason
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::BAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 9) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cnickban <%s> <%s>. %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0],  LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

    pChatCommand->m_sCommand += 8;

    char * sReason = strchr(pChatCommand->m_sCommand, ' ');
    if(sReason != NULL) {
        sReason[0] = '\0';

        if(sReason[1] == '\0') {
        	pChatCommand->m_ui32CommandLen = (uint32_t)(sReason - pChatCommand->m_sCommand);

            sReason = NULL;
        } else {
            sReason++;

			uint32_t ui32ReasonLen = (uint32_t)(pChatCommand->m_ui32CommandLen - (sReason - pChatCommand->m_sCommand));
            if(ui32ReasonLen > 511) {
            	sReason[508] = '.';
            	sReason[509] = '.';
                sReason[510] = '.';
                sReason[511] = '\0';
            }

            pChatCommand->m_ui32CommandLen = (uint32_t)(sReason - pChatCommand->m_sCommand) - 1;
        }
    } else {
        pChatCommand->m_ui32CommandLen -= 8;
    }

    if(pChatCommand->m_sCommand[0] == '\0') {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %cnickban <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_NICK_SPECIFIED]);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen > 100) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %cnickban <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        return true;
    }

    // Self-ban ?
	if(strcasecmp(pChatCommand->m_sCommand, pChatCommand->m_pUser->m_sNick) == 0) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_YOU_CANT_BAN_YOURSELF]);
        return true;
    }

    User * pOtherUser = HashManager::m_Ptr->FindUser(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen);
    if(pOtherUser != NULL) {
        // PPK don't nickban user with higher profile
        if(pOtherUser->m_i32Profile != -1 && pChatCommand->m_pUser->m_i32Profile > pOtherUser->m_i32Profile) {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_YOU_NOT_ALLOWED_TO], LanguageManager::m_Ptr->m_sTexts[LAN_BAN_LWR], pOtherUser->m_sNick);
            return true;
        }

        UncountDeflood(pChatCommand);

       	pOtherUser->SendFormat("HubCommands::NickBan6", false, "<%s> %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_HAD_BEEN_BANNED_BCS], sReason == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sReason);

        if(BanManager::m_Ptr->NickBan(pOtherUser, NULL, sReason, pChatCommand->m_pUser->m_sNick) == true) {
			UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) nickbanned by %s", pOtherUser->m_sNick, pOtherUser->m_sIP, pChatCommand->m_pUser->m_sNick);
			pOtherUser->Close();
        } else {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan7", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_NICK], pOtherUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_IS_ALREDY_BANNED_DISCONNECT]);

            pOtherUser->Close();
            return true;
        }
    } else {
		return NickBan(pChatCommand, sReason);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
      	GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::NickBan8", "<%s> *** %s %s %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_BEEN_BANNED_BY], pChatCommand->m_pUser->m_sNick, 
		  LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], sReason == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
	}

	if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan9", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_sCommand, 
			LanguageManager::m_Ptr->m_sTexts[LAN_ADDED_TO_BANS]);
	}
    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::NickTempBan(ChatCommand * pChatCommand) { // !nicktempban nick time reason ... m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TEMP_BAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 15) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickTempBan1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|", 
			SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

    // Now in sCommand we have nick, time and maybe reason
    char * sCmdParts[] = { NULL, NULL, NULL };
    uint16_t ui16CmdPartsLen[] = { 0, 0, 0 };

    uint8_t ui8Part = 0;

    sCmdParts[ui8Part] = pChatCommand->m_sCommand+12; // nick start

    for(uint32_t ui32i = 12; ui32i < pChatCommand->m_ui32CommandLen; ui32i++) {
        if(pChatCommand->m_sCommand[ui32i] == ' ') {
            pChatCommand->m_sCommand[ui32i] = '\0';
            ui16CmdPartsLen[ui8Part] = (uint16_t)((pChatCommand->m_sCommand+ui32i)-sCmdParts[ui8Part]);

            // are we on last space ???
            if(ui8Part == 1) {
                sCmdParts[2] = pChatCommand->m_sCommand+ui32i+1;
                ui16CmdPartsLen[2] = (uint16_t)(pChatCommand->m_ui32CommandLen-ui32i-1);
                break;
            }

            ui8Part++;
            sCmdParts[ui8Part] = pChatCommand->m_sCommand+ui32i+1;
        }
    }

    if(sCmdParts[2] == NULL && ui16CmdPartsLen[1] == 0 && sCmdParts[1] != NULL) {
        ui16CmdPartsLen[1] = (uint16_t)(pChatCommand->m_ui32CommandLen-(sCmdParts[1]-pChatCommand->m_sCommand));
    }

    if(sCmdParts[2] != NULL && ui16CmdPartsLen[2] == 0) {
        sCmdParts[2] = NULL;
    }

    if(ui16CmdPartsLen[2] > 511) {
    	sCmdParts[2][508] = '.';
    	sCmdParts[2][509] = '.';
        sCmdParts[2][510] = '.';
        sCmdParts[2][511] = '\0';
    }

    if(ui16CmdPartsLen[0] == 0 || ui16CmdPartsLen[1] == 0) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickTempBan2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|", 
			SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

    if(ui16CmdPartsLen[0] > 100) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickTempBan3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|", 
			SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        return true;
    }

    // Self-ban ?
	if(strcasecmp(sCmdParts[0], pChatCommand->m_pUser->m_sNick) == 0) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickTempBan4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_YOU_CANT_BAN_YOURSELF]);
        return true;
    }
    
    User * pOtherUser = HashManager::m_Ptr->FindUser(sCmdParts[0], ui16CmdPartsLen[0]);
    if(pOtherUser != NULL) {
        // PPK don't tempban user with higher profile
        if(pOtherUser->m_i32Profile != -1 && pChatCommand->m_pUser->m_i32Profile > pOtherUser->m_i32Profile) {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickTempBan5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_YOU_NOT_ALLOWED_TO], LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BAN_NICK], pOtherUser->m_sNick);
            return true;
        }
    } else {
        return TempNickBan(pChatCommand, sCmdParts[0], sCmdParts[1], ui16CmdPartsLen[1], sCmdParts[2]);
    }

    uint8_t ui8Time = sCmdParts[1][ui16CmdPartsLen[1]-1];
    sCmdParts[1][ui16CmdPartsLen[1]-1] = '\0';
    int iTime = atoi(sCmdParts[1]);
    time_t acc_time, ban_time;

    if(iTime <= 0 || GenerateTempBanTime(ui8Time, (uint32_t)iTime, acc_time, ban_time) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickTempBan6", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cnicktempban <%s> <%s> <%s>. %s!|", 
			SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_BAD_TIME_SPECIFIED]);
        return true;
    }

    if(BanManager::m_Ptr->NickTempBan(pOtherUser, NULL, sCmdParts[2], pChatCommand->m_pUser->m_sNick, 0, ban_time) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickTempBan7", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_NICK], pOtherUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_ALRD_BND_LNGR_TIME_DISCONNECTED]);
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Already temp banned user %s (%s) disconnected by %s", pOtherUser->m_sNick, pOtherUser->m_sIP, pChatCommand->m_pUser->m_sNick);

        // Disconnect user
        pOtherUser->Close();

        return true;
    }

	UncountDeflood(pChatCommand);

    char sTime[256];
    strcpy(sTime, formatTime((ban_time-acc_time)/60));

    // Send user a message that he has been tempbanned
    pOtherUser->SendFormat("HubCommands::NickTempBan8", false, "<%s> %s: %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_HAD_BEEN_TEMP_BANNED_TO], sTime, LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], 
		sCmdParts[2] == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::NickTempBan9", "<%s> *** %s %s %s %s: %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], sCmdParts[0], LanguageManager::m_Ptr->m_sTexts[LAN_HAS_BEEN_TMPBND_BY], pChatCommand->m_pUser->m_sNick, 
			LanguageManager::m_Ptr->m_sTexts[LAN_TO_LWR], sTime, LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], sCmdParts[2] == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickTempBan10", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s: %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], sCmdParts[0], 
			LanguageManager::m_Ptr->m_sTexts[LAN_BEEN_TEMP_BANNED_TO], sTime, LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], sCmdParts[2] == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
	}

	UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) tempbanned by %s", pOtherUser->m_sNick, pOtherUser->m_sIP, pChatCommand->m_pUser->m_sNick);

	pOtherUser->Close();

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Op(ChatCommand * pChatCommand) { // !op nick
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::TEMPOP) == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_TEMP_OPERATOR) == User::BIT_TEMP_OPERATOR) == true) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 4 || pChatCommand->m_sCommand[3] == '\0') {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Op1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cop <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

    pChatCommand->m_sCommand += 3;

    if(pChatCommand->m_ui32CommandLen > 100) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Op2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cop <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        return true;
    }

    User * pOtherUser = HashManager::m_Ptr->FindUser(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen-3);
    if(pOtherUser == NULL) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Op3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_IN_USERLIST]);
        return true;
    }

    if(((pOtherUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Op4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pOtherUser->m_sNick, 
			LanguageManager::m_Ptr->m_sTexts[LAN_ALREDY_IS_OP]);
        return true;
    }

	UncountDeflood(pChatCommand);

    int iProfileIndex = ProfileManager::m_Ptr->GetProfileIndex("Operator");
    if(iProfileIndex == -1) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Op5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s. %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], LanguageManager::m_Ptr->m_sTexts[LAN_OPERATOR_PROFILE_MISSING]);
        return true;
    }

    pOtherUser->m_ui32BoolBits |= User::BIT_OPERATOR;
    bool bAllowedOpChat = ProfileManager::m_Ptr->IsAllowed(pOtherUser, ProfileManager::ALLOWEDOPCHAT);
    pOtherUser->m_i32Profile = iProfileIndex;
    pOtherUser->m_ui32BoolBits |= User::BIT_TEMP_OPERATOR; // to disallow adding more tempop by tempop user ;)
    Users::m_Ptr->Add2OpList(pOtherUser);

    if(((pOtherUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false) {
        pOtherUser->SendFormat("HubCommands::Op6", true, "$LogedIn %s|<%s> *** %s.|", pOtherUser->m_sNick, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_GOT_TEMP_OP]);
    } else {
    	pOtherUser->SendFormat("HubCommands::Op7", true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_GOT_TEMP_OP]);
	}

    GlobalDataQueue::m_Ptr->OpListStore(pOtherUser->m_sNick);

    if(bAllowedOpChat != ProfileManager::m_Ptr->IsAllowed(pOtherUser, ProfileManager::ALLOWEDOPCHAT)) {
        if(SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true &&
            (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == false || SettingManager::m_Ptr->m_bBotsSameNick == false)) {
            if(((pOtherUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false) {
                pOtherUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_OP_CHAT_HELLO], SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_OP_CHAT_HELLO]);
            }
            pOtherUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_OP_CHAT_MYINFO], SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_OP_CHAT_MYINFO]);
            pOtherUser->SendFormat("HubCommands::Op8", true, "$OpList %s$$|", SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK]);
        }
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
		GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::Op9", "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_SETS_OP_MODE_TO], pOtherUser->m_sNick);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Op10", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pOtherUser->m_sNick, 
			LanguageManager::m_Ptr->m_sTexts[LAN_GOT_OP_STATUS]);
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::OpMassMsg(ChatCommand * pChatCommand) { // !opmassmsg text
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::MASSMSG) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 11) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::OpMassMsg1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %copmassmsg <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_MESSAGE_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

	UncountDeflood(pChatCommand);

    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s $<%s> %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, pChatCommand->m_sCommand+10);
    if(iMsgLen > 0) {
		GlobalDataQueue::m_Ptr->SingleItemStore(ServerManager::m_pGlobalBuffer, iMsgLen, pChatCommand->m_pUser, 0, GlobalDataQueue::SI_PM2OPS);
    }

    pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::OpMassMsg2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
		LanguageManager::m_Ptr->m_sTexts[LAN_MASSMSG_TO_OPS_SND]);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Passwd(ChatCommand * pChatCommand) { // !passwd password
    RegUser * pReg = RegManager::m_Ptr->Find(pChatCommand->m_pUser);
    if(pChatCommand->m_pUser->m_i32Profile == -1 || pReg == NULL) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Passwd1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_NOT_ALLOWED_TO_CHANGE_PASS]);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 8) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Passwd2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cpasswd <%s>. %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NEW_PASSWORD], LanguageManager::m_Ptr->m_sTexts[LAN_PASS_MUST_SPECIFIED]);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen > 71) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Passwd3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_PASS_LEN_64_CHARS]);
        return true;
    }

	if(strchr(pChatCommand->m_sCommand+7, '|') != NULL) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Passwd4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_NO_PIPE_IN_PASS]);
        return true;
    }

    if(pReg->UpdatePassword(pChatCommand->m_sCommand+7, pChatCommand->m_ui32CommandLen-7) == false) {
        return true;
    }

    RegManager::m_Ptr->Save(true);

#ifdef _BUILD_GUI
    if(RegisteredUsersDialog::m_Ptr != NULL) {
        RegisteredUsersDialog::m_Ptr->RemoveReg(pReg);
        RegisteredUsersDialog::m_Ptr->AddReg(pReg);
    }
#endif

    pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Passwd5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
		LanguageManager::m_Ptr->m_sTexts[LAN_YOUR_PASSWORD_UPDATE_SUCCESS]);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::PermUnban(ChatCommand * pChatCommand) { // !permunban what
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::UNBAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 11 || pChatCommand->m_sCommand[10] == '\0') {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::PermUnban1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cpermunban <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP_OR_NICK], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

    pChatCommand->m_sCommand += 10;

    if(pChatCommand->m_ui32CommandLen > 100) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::PermUnban2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cpermunban <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP_OR_NICK], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        return true;
    }

    if(BanManager::m_Ptr->PermUnban(pChatCommand->m_sCommand) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::PermUnban3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SORRY], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_IN_BANS]);
        return true;
    }

	UncountDeflood(pChatCommand);

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
    	GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::PermUnban4", "<%s> *** %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_REMOVED_LWR], pChatCommand->m_sCommand, 
			LanguageManager::m_Ptr->m_sTexts[LAN_FROM_BANS]);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::PermUnban5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_sCommand, 
			LanguageManager::m_Ptr->m_sTexts[LAN_REMOVED_FROM_BANS]);
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
