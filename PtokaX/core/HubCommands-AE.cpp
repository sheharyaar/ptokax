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

bool HubCommands::AddRegUser(ChatCommand * pChatCommand) {
	// !addreguser nick pass profile_name

    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::ADDREGUSER) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen > 255) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_CMD_TOO_LONG]);
        return true;
    }

    char *sCmdParts[] = { NULL, NULL, NULL };
    uint16_t iCmdPartsLen[] = { 0, 0, 0 };

    uint8_t cPart = 0;

    sCmdParts[cPart] = pChatCommand->m_sCommand+11; // nick start

	for(uint32_t ui32i = 11; ui32i < pChatCommand->m_ui32CommandLen; ++ui32i) {
		if(pChatCommand->m_sCommand[ui32i] == ' ') {
			pChatCommand->m_sCommand[ui32i] = '\0';
			iCmdPartsLen[cPart] = (uint16_t)((pChatCommand->m_sCommand+ui32i)-sCmdParts[cPart]);

			// are we on last space ???
			if(cPart == 1) {
				sCmdParts[2] = pChatCommand->m_sCommand+ui32i+1;
				iCmdPartsLen[2] = (uint16_t)(pChatCommand->m_ui32CommandLen-ui32i-1);
				for(uint32_t ui32j = pChatCommand->m_ui32CommandLen; ui32j > ui32i; --ui32j) {
					if(pChatCommand->m_sCommand[ui32j] == ' ') {
						pChatCommand->m_sCommand[ui32j] = '\0';

						sCmdParts[2] = pChatCommand->m_sCommand+ui32j+1;
						iCmdPartsLen[2] = (uint16_t)(pChatCommand->m_ui32CommandLen-ui32j-1);

						pChatCommand->m_sCommand[ui32i] = ' ';
						iCmdPartsLen[1] = (uint16_t)((pChatCommand->m_sCommand+ui32j)-sCmdParts[1]);

						break;
					}
				}
				break;
            }

            cPart++;
			sCmdParts[cPart] = pChatCommand->m_sCommand+ui32i+1;
        }
    }

    if(iCmdPartsLen[0] == 0 || iCmdPartsLen[1] == 0 || iCmdPartsLen[2] == 0) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %caddreguser <%s> <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_PASSWORD_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_PROFILENAME_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

    if(iCmdPartsLen[0] > 65) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        return true;
    }

	if(strpbrk(sCmdParts[0], " $|") != NULL) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_NO_BAD_CHARS_IN_NICK]);
        return true;
    }

    if(iCmdPartsLen[1] > 65) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_PASS_LEN_64_CHARS]);
        return true;
    }

	if(strchr(sCmdParts[1], '|') != NULL) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser6", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_NO_PIPE_IN_PASS]);
        return true;
    }

    int iProfileIdx = ProfileManager::m_Ptr->GetProfileIndex(sCmdParts[2]);
    if(iProfileIdx == -1) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser7", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERR_NO_PROFILE_GIVEN_NAME_EXIST]);
        return true;
    }

    // check hierarchy
    // deny if pUser is not Master and tries add equal or higher profile
    if(pChatCommand->m_pUser->m_i32Profile > 0 && iProfileIdx <= pChatCommand->m_pUser->m_i32Profile) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser8", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_YOU_NOT_ALLOWED_TO_ADD_USER_THIS_PROFILE]);
        return true;
    }

    // try to add the user
    if(RegManager::m_Ptr->AddNew(sCmdParts[0], sCmdParts[1], (uint16_t)iProfileIdx) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser9", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_USER], sCmdParts[0], LanguageManager::m_Ptr->m_sTexts[LAN_IS_ALREDY_REGISTERED]);
        return true;
    }

	UncountDeflood(pChatCommand);

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::AddRegUser10", "<%s> *** %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_SUCCESSFULLY_ADDED], sCmdParts[0], 
			LanguageManager::m_Ptr->m_sTexts[LAN_TO_REGISTERED_USERS]);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::AddRegUser11", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], sCmdParts[0], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SUCCESSFULLY_ADDED_TO_REGISTERED_USERS]);
    }

    User * pOtherUser = HashManager::m_Ptr->FindUser(sCmdParts[0], iCmdPartsLen[0]);
    if(pOtherUser != NULL) {
        bool bAllowedOpChat = ProfileManager::m_Ptr->IsAllowed(pOtherUser, ProfileManager::ALLOWEDOPCHAT);
        pOtherUser->m_i32Profile = iProfileIdx;
        if(((pOtherUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            if(ProfileManager::m_Ptr->IsAllowed(pOtherUser, ProfileManager::HASKEYICON) == true) {
                pOtherUser->m_ui32BoolBits |= User::BIT_OPERATOR;
            } else {
                pOtherUser->m_ui32BoolBits &= ~User::BIT_OPERATOR;
            }

            if(((pOtherUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
                Users::m_Ptr->Add2OpList(pOtherUser);
                GlobalDataQueue::m_Ptr->OpListStore(pOtherUser->m_sNick);
                if(bAllowedOpChat != ProfileManager::m_Ptr->IsAllowed(pOtherUser, ProfileManager::ALLOWEDOPCHAT)) {
                    if(SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true && (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == false || SettingManager::m_Ptr->m_bBotsSameNick == false)) {
                        if(((pOtherUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false) {
                            pOtherUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_OP_CHAT_HELLO], SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_OP_CHAT_HELLO]);
                        }
                        pOtherUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_OP_CHAT_MYINFO], SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_OP_CHAT_MYINFO]);
                        pOtherUser->SendFormat("HubCommands::AddRegUser12", true, "$OpList %s$$|", SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK]);
                    }
                }
            }
        } 
    }
     
    return true;	
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::BanIp(ChatCommand * pChatCommand) { // !banip ip reason
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::BAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 12) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::BanIp", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %cbanip <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

	pChatCommand->m_sCommand += 6;
	pChatCommand->m_ui32CommandLen -= 6;

    return BanIp(pChatCommand, false);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Ban(ChatCommand * pChatCommand) { // !ban nick reason
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::BAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 5) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Ban", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cban <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

	pChatCommand->m_sCommand += 4;
	pChatCommand->m_ui32CommandLen -= 4;

    return Ban(pChatCommand, false);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::ClrTempBans(ChatCommand * pChatCommand) { // !clrtempbans
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::CLRTEMPBAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

	UncountDeflood(pChatCommand);
	BanManager::m_Ptr->ClearTemp();

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::ClrTempBans1", "<%s> *** %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_CLEARED_TEMPBANS]);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::ClrTempBans2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BANS_CLEARED]);
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::ClrPermBans(ChatCommand * pChatCommand) { // !clrpermbans
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::CLRPERMBAN) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

	UncountDeflood(pChatCommand);
	BanManager::m_Ptr->ClearPerm();

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::ClrPermBans1", "<%s> *** %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_CLEARED_PERMBANS]);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::ClrPermBans2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
		LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BANS_CLEARED]);
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::ClrRangeTempBans(ChatCommand * pChatCommand) { // !clrrangetempbans
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::CLR_RANGE_TBANS) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

	UncountDeflood(pChatCommand);
	BanManager::m_Ptr->ClearTempRange();

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::ClrRangeTempBans1", "<%s> *** %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_CLEARED_TEMP_RANGEBANS]);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::ClrRangeTempBans2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_RANGE_BANS_CLEARED]);
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::ClrRangePermBans(ChatCommand * pChatCommand) { // !clrrangepermbans
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::CLR_RANGE_BANS) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

	UncountDeflood(pChatCommand);
	BanManager::m_Ptr->ClearPermRange();

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::DoCommand->clrrangepermbans", "<%s> *** %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_CLEARED_PERM_RANGEBANS]);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DoCommand->clrrangepermbans", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_PERM_RANGE_BANS_CLEARED]);
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::CheckNickBan(ChatCommand * pChatCommand) { // !checknickban nick
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GETBANLIST) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

	UncountDeflood(pChatCommand);

    if(pChatCommand->m_ui32CommandLen < 14 || pChatCommand->m_sCommand[13] == '\0') {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckNickBan1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> ***%s %cchecknickban <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
		return true;
    }

    pChatCommand->m_sCommand += 13;

    BanItem * pBan = BanManager::m_Ptr->FindNick(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen-13);
    if(pBan == NULL) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckNickBan2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_NO_BAN_FOUND]);
        return true;
    }

    int iMsgLen = CheckFromPm(pChatCommand);                        

    int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "<%s> %s: %s", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_NICK], pBan->m_sNick);
    if(iRet <= 0) {
        return true;
    }
    iMsgLen += iRet;

    if(pBan->m_sIp[0] != '\0') {
        if(((pBan->m_ui8Bits & BanManager::IP) == BanManager::IP) == true) {
			iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, " %s", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED]);
            if(iRet <= 0) {
                return true;
            }
            iMsgLen += iRet;
        }
		iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, " %s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_IP], pBan->m_sIp);
        if(iRet <= 0) {
            return true;
        }
        iMsgLen += iRet;

        if(((pBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true) {
			iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, " (%s)", LanguageManager::m_Ptr->m_sTexts[LAN_FULL]);
            if(iRet <= 0) {
                return true;
            }
            iMsgLen += iRet;
        }
    }

    if(pBan->m_sReason != NULL) {
		iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, " %s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_REASON], pBan->m_sReason);
        if(iRet <= 0) {
            return true;
        }
        iMsgLen += iRet;
    }

    if(pBan->m_sBy != NULL) {
        iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, " %s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_BY], pBan->m_sBy);
        if(iRet <= 0) {
            return true;
        }
        iMsgLen += iRet;
    }

    if(((pBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
		iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, " %s: ", LanguageManager::m_Ptr->m_sTexts[LAN_EXPIRE]);
        if(iRet <= 0) {
            return true;
        }
        iMsgLen += iRet;

        struct tm *tm = localtime(&pBan->m_tTempBanExpire);
        size_t szRet = strftime(ServerManager::m_pGlobalBuffer+iMsgLen, 256, "%c.|", tm);
        if(szRet > 0) {
        	iMsgLen += (int)szRet;
        } else {
            ServerManager::m_pGlobalBuffer[iMsgLen] = '.';
            ServerManager::m_pGlobalBuffer[iMsgLen+1] = '|';
            ServerManager::m_pGlobalBuffer[iMsgLen+2] = '\0';
            iMsgLen += 2;
		}
    } else {
        ServerManager::m_pGlobalBuffer[iMsgLen] = '.';
        ServerManager::m_pGlobalBuffer[iMsgLen+1] = '|';
        ServerManager::m_pGlobalBuffer[iMsgLen+2] = '\0';
        iMsgLen += 2;
    }

    pChatCommand->m_pUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iMsgLen);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::CheckIpBan(ChatCommand * pChatCommand) { // !checkipban ip
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GETBANLIST) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

	UncountDeflood(pChatCommand);

    if(pChatCommand->m_ui32CommandLen < 15 || pChatCommand->m_sCommand[11] == '\0') {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckIpBan1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ccheckipban <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
		return true;
    }

    pChatCommand->m_sCommand += 11;

	uint8_t ui128Hash[16];
	memset(ui128Hash, 0, 16);

    // check ip
    if(HashIP(pChatCommand->m_sCommand, ui128Hash) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckIpBan2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ccheckipban <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_NO_VALID_IP_SPECIFIED]);
        return true;
    }
    
    time_t acc_time;
    time(&acc_time);

    BanItem * pNextBan = BanManager::m_Ptr->FindIP(ui128Hash, acc_time);

    if(pNextBan != NULL) {
        int iMsgLen = CheckFromPm(pChatCommand);

        int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "<%s> ", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
        if(iRet <= 0) {
            return true;
        }
        iMsgLen += iRet;

		string Bans(ServerManager::m_pGlobalBuffer, iMsgLen);
        uint32_t iBanNum = 0;
        BanItem * pCurBan = NULL;

        while(pNextBan != NULL) {
            pCurBan = pNextBan;
            pNextBan = pCurBan->m_pHashIpTableNext;

            if(((pCurBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
                // PPK ... check if temban expired
                if(acc_time >= pCurBan->m_tTempBanExpire) {
					BanManager::m_Ptr->Rem(pCurBan);
                    delete pCurBan;

					continue;
                }
            }

            iBanNum++;
            iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\n[%u] %s: %s", iBanNum, LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_IP], pCurBan->m_sIp);
            if(iMsgLen <= 0) {
                return true;
            }

            if(((pCurBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true) {
				iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, " (%s)", LanguageManager::m_Ptr->m_sTexts[LAN_FULL]);
                if(iRet <= 0) {
                    return true;
                }
                iMsgLen += iRet;
            }

			Bans += string(ServerManager::m_pGlobalBuffer, iMsgLen);

            if(pCurBan->m_sNick != NULL) {
                iMsgLen = 0;
                if(((pCurBan->m_ui8Bits & BanManager::NICK) == BanManager::NICK) == true) {
                    iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, " %s", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED]);
                    if(iMsgLen <= 0) {
                        return true;
                    }
                }

				iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, " %s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_NICK], pCurBan->m_sNick);
                if(iRet <= 0) {
                    return true;
                }
                iMsgLen += iRet;
				Bans += ServerManager::m_pGlobalBuffer;
            }

            if(pCurBan->m_sReason != NULL) {
                iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, " %s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_REASON], pCurBan->m_sReason);
                if(iMsgLen <= 0) {
                    return true;
                }
				Bans += ServerManager::m_pGlobalBuffer;
            }

            if(pCurBan->m_sBy != NULL) {
                iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, " %s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_BY], pCurBan->m_sBy);
                if(iMsgLen <= 0) {
                    return true;
                }
				Bans += ServerManager::m_pGlobalBuffer;
            }

            if(((pCurBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
                struct tm *tm = localtime(&pCurBan->m_tTempBanExpire);
                if(strftime(ServerManager::m_pGlobalBuffer, 256, "%c", tm) > 0) {
					Bans += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_EXPIRE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_EXPIRE]) + ": " + string(ServerManager::m_pGlobalBuffer);
				}
            }
        }

        if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GET_RANGE_BANS) == false) {
            Bans += "|";

			pChatCommand->m_pUser->SendCharDelayed(Bans.c_str(), Bans.size());
            return true;
        }

		RangeBanItem * pCurRangeBan = NULL,
            * pNextRangeBan = BanManager::m_Ptr->FindRange(ui128Hash, acc_time);

        while(pNextRangeBan != NULL) {
            pCurRangeBan = pNextRangeBan;
            pNextRangeBan = pCurRangeBan->m_pNext;

			if(((pCurRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
				// PPK ... check if temban expired
				if(acc_time >= pCurRangeBan->m_tTempBanExpire) {
					BanManager::m_Ptr->RemRange(pCurRangeBan);
					delete pCurRangeBan;

					continue;
				}
			}

            if(memcmp(pCurRangeBan->m_ui128FromIpHash, ui128Hash, 16) <= 0 && memcmp(pCurRangeBan->m_ui128ToIpHash, ui128Hash, 16) >= 0) {
                iBanNum++;
                iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\n[%u] %s: %s-%s", iBanNum, LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], pCurRangeBan->m_sIpFrom, pCurRangeBan->m_sIpTo);
                if(iMsgLen <= 0) {
                    return true;
                }

                if(((pCurRangeBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true) {
					iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, " (%s)", LanguageManager::m_Ptr->m_sTexts[LAN_FULL]);
                    if(iRet <= 0) {
                        return true;
                    }
                    iMsgLen += iRet;
                }

				Bans += string(ServerManager::m_pGlobalBuffer, iMsgLen);

                if(pCurRangeBan->m_sReason != NULL) {
                    iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, " %s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_REASON], pCurRangeBan->m_sReason);
                    if(iMsgLen <= 0) {
                        return true;
                    }
					Bans += ServerManager::m_pGlobalBuffer;
                }

                if(pCurRangeBan->m_sBy != NULL) {
                    iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, " %s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_BY], pCurRangeBan->m_sBy);
                    if(iMsgLen <= 0) {
                        return true;
                    }
					Bans += ServerManager::m_pGlobalBuffer;
                }

                if(((pCurRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
                    struct tm *tm = localtime(&pCurRangeBan->m_tTempBanExpire);
                    if(strftime(ServerManager::m_pGlobalBuffer, 256, "%c", tm) > 0) {
						Bans += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_EXPIRE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_EXPIRE]) + ": " + string(ServerManager::m_pGlobalBuffer);
					}
                }
            }
        }

        Bans += "|";

		pChatCommand->m_pUser->SendCharDelayed(Bans.c_str(), Bans.size());

        return true;
    } else if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GET_RANGE_BANS) == true) {
		RangeBanItem * pNextRangeBan = BanManager::m_Ptr->FindRange(ui128Hash, acc_time);

        if(pNextRangeBan != NULL) {
            int iMsgLen = CheckFromPm(pChatCommand);

            int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "<%s> ", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
            if(iRet <= 0) {
                return true;
            }
            iMsgLen += iRet;

			string Bans(ServerManager::m_pGlobalBuffer, iMsgLen);
            uint32_t iBanNum = 0;
            RangeBanItem * pCurRangeBan = NULL;

            while(pNextRangeBan != NULL) {
                pCurRangeBan = pNextRangeBan;
                pNextRangeBan = pCurRangeBan->m_pNext;

				if(((pCurRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
					// PPK ... check if temban expired
					if(acc_time >= pCurRangeBan->m_tTempBanExpire) {
						BanManager::m_Ptr->RemRange(pCurRangeBan);
						delete pCurRangeBan;

						continue;
					}
				}

                if(memcmp(pCurRangeBan->m_ui128FromIpHash, ui128Hash, 16) <= 0 && memcmp(pCurRangeBan->m_ui128ToIpHash, ui128Hash, 16) >= 0) {
                    iBanNum++;
                    iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "\n[%u] %s: %s-%s", iBanNum, LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], pCurRangeBan->m_sIpFrom, pCurRangeBan->m_sIpTo);
                    if(iMsgLen <= 0) {
                        return true;
                    }

                    if(((pCurRangeBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true) {
						iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, " (%s)", LanguageManager::m_Ptr->m_sTexts[LAN_FULL]);
                        if(iRet <= 0) {
                            return true;
                        }
                        iMsgLen += iRet;
                    }

					Bans += string(ServerManager::m_pGlobalBuffer, iMsgLen);

                    if(pCurRangeBan->m_sReason != NULL) {
                        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, " %s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_REASON], pCurRangeBan->m_sReason);
                        if(iMsgLen <= 0) {
                            return true;
                        }
						Bans += ServerManager::m_pGlobalBuffer;
                    }

                    if(pCurRangeBan->m_sBy != NULL) {
                        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, " %s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_BY], pCurRangeBan->m_sBy);
                        if(iMsgLen <= 0) {
                            return true;
                        }
						Bans += ServerManager::m_pGlobalBuffer;
                    }

                    if(((pCurRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
                        struct tm *tm = localtime(&pCurRangeBan->m_tTempBanExpire);
                        if(strftime(ServerManager::m_pGlobalBuffer, 256, "%c", tm) > 0) {
							Bans += " " + string(LanguageManager::m_Ptr->m_sTexts[LAN_EXPIRE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_EXPIRE]) + ": " + string(ServerManager::m_pGlobalBuffer);
						}
                    }
                }
            }

            Bans += "|";

			pChatCommand->m_pUser->SendCharDelayed(Bans.c_str(), Bans.size());

            return true;
        }
    }

    pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckIpBan3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
		LanguageManager::m_Ptr->m_sTexts[LAN_NO_BAN_FOUND]);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::CheckRangeBan(ChatCommand * pChatCommand) { // !checkrangeban ipfrom ipto
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::GET_RANGE_BANS) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

	UncountDeflood(pChatCommand);

    if(pChatCommand->m_ui32CommandLen < 26) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckRangeBan1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ccheckrangeban <%s> <%s>. %s!|", 
			SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], 
			LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
		return true;
    }

    pChatCommand->m_sCommand += 14;

    char * sIpTo = strchr(pChatCommand->m_sCommand, ' ');
	if(sIpTo == NULL) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckRangeBan2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ccheckrangeban <%s> <%s>. %s!|", 
			SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], 
			LanguageManager::m_Ptr->m_sTexts[LAN_BAD_PARAMS_GIVEN]);
		return true;
    }

    sIpTo[0] = '\0';
    sIpTo++;

    // check ipfrom
    if(pChatCommand->m_sCommand[0] == '\0' || sIpTo[0] == '\0') {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckRangeBan3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ccheckrangeban <%s> <%s>. %s!|", 
			SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], 
			LanguageManager::m_Ptr->m_sTexts[LAN_BAD_PARAMS_GIVEN]);
		return true;
    }

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

    if(HashIP(pChatCommand->m_sCommand, ui128FromHash) == false || HashIP(sIpTo, ui128ToHash) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckRangeBan4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %ccheckrangeban <%s> <%s>. %s!|", 
			SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], 
			LanguageManager::m_Ptr->m_sTexts[LAN_NO_VALID_IP_RANGE_SPECIFIED]);
        return true;
    }

    time_t acc_time;
    time(&acc_time);

	RangeBanItem * pRangeBan = BanManager::m_Ptr->FindRange(ui128FromHash, ui128ToHash, acc_time);
    if(pRangeBan == NULL) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::CheckRangeBan5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_NO_RANGE_BAN_FOUND]);
        return true;
    }
    
    int iMsgLen = CheckFromPm(pChatCommand);

    int iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "<%s> %s: %s-%s", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], pRangeBan->m_sIpFrom, pRangeBan->m_sIpTo);
    if(iRet <= 0) {
        return true;
    }
    iMsgLen += iRet;
        
    if(((pRangeBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true) {
        iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, " (%s)", LanguageManager::m_Ptr->m_sTexts[LAN_FULL]);
        if(iRet <= 0) {
            return true;
        }
        iMsgLen += iRet;
    }

    if(pRangeBan->m_sReason != NULL) {
		iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, " %s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_REASON], pRangeBan->m_sReason);
        if(iRet <= 0) {
            return true;
        }
        iMsgLen += iRet;
    }

    if(pRangeBan->m_sBy != NULL) {
		iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, " %s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_BY], pRangeBan->m_sBy);
        if(iRet <= 0) {
            return true;
        }
        iMsgLen += iRet;
    }

    if(((pRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
		iRet = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, " %s: ", LanguageManager::m_Ptr->m_sTexts[LAN_EXPIRE]);
        if(iRet <= 0) {
            return true;
        }
        iMsgLen += iRet;

        struct tm *tm = localtime(&pRangeBan->m_tTempBanExpire);
        size_t szRet = strftime(ServerManager::m_pGlobalBuffer+iMsgLen, 256, "%c.|", tm);
        if(szRet > 0) {
        	iMsgLen += (int)szRet;
        } else {
            ServerManager::m_pGlobalBuffer[iMsgLen] = '.';
            ServerManager::m_pGlobalBuffer[iMsgLen+1] = '|';
            ServerManager::m_pGlobalBuffer[iMsgLen+2] = '\0';
            iMsgLen += 2;
		}
    } else {
        ServerManager::m_pGlobalBuffer[iMsgLen] = '.';
        ServerManager::m_pGlobalBuffer[iMsgLen+1] = '|';
        ServerManager::m_pGlobalBuffer[iMsgLen+2] = '\0';
        iMsgLen += 2;
    }

    pChatCommand->m_pUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iMsgLen);

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Drop(ChatCommand * pChatCommand) { //!drop nick
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::DROP) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 6) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Drop1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cdrop <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

    pChatCommand->m_sCommand += 5;
    pChatCommand->m_ui32CommandLen -= 5;

    uint32_t ui32NickLen = 0;

    char * sReason = strchr(pChatCommand->m_sCommand, ' ');
    if(sReason != NULL) {
        ui32NickLen = (uint32_t)(sReason-pChatCommand->m_sCommand);

        sReason[0] = '\0';
        if(sReason[1] == '\0') {
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
        }
    } else {
        ui32NickLen = pChatCommand->m_ui32CommandLen;
    }

    if(ui32NickLen > 100) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Drop2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cdrop <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        return true;
    }

    if(pChatCommand->m_sCommand[0] == '\0') {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Drop3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cdrop <%s>. %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }
    
    // Self-drop ?
	if(strcasecmp(pChatCommand->m_sCommand, pChatCommand->m_pUser->m_sNick) == 0) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Drop4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_YOU_CANT_DROP_YOURSELF]);
        return true;
    }
    
    User * pOtherUser = HashManager::m_Ptr->FindUser(pChatCommand->m_sCommand, ui32NickLen);
    if(pOtherUser == NULL) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Drop5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_sCommand, 
			LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_IN_USERLIST]);
        return true;
    }
    
	// PPK don't drop user with higher profile
	if(pOtherUser->m_i32Profile != -1 && pChatCommand->m_pUser->m_i32Profile > pOtherUser->m_i32Profile) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Drop6", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_YOU_NOT_ALLOWED_TO], LanguageManager::m_Ptr->m_sTexts[LAN_DROP_LWR], pOtherUser->m_sNick);
		return true;
	}

	UncountDeflood(pChatCommand);

    BanManager::m_Ptr->TempBan(pOtherUser, sReason, pChatCommand->m_pUser->m_sNick, 0, 0, false);

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
		GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::Drop7", "<%s> *** %s %s %s %s %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], pOtherUser->m_sIP, 
			LanguageManager::m_Ptr->m_sTexts[LAN_DROP_ADDED_TEMPBAN_BY], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], sReason == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Drop8", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], pOtherUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_DROP_ADDED_TEMPBAN_BCS], sReason == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
    }

    pOtherUser->Close();

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::DelRegUser(ChatCommand * pChatCommand) { // !delreguser nick
    if(ProfileManager::m_Ptr->IsAllowed(pChatCommand->m_pUser, ProfileManager::DELREGUSER) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen > 255) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DelRegUser1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_CMD_TOO_LONG]);
        return true;
    } else if(pChatCommand->m_ui32CommandLen < 13) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DelRegUser2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cdelreguser <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
        return true;
    }

    pChatCommand->m_sCommand += 11;
    pChatCommand->m_ui32CommandLen -= 11;

    // find him
	RegUser * pReg = RegManager::m_Ptr->Find(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen);
    if(pReg == NULL) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DelRegUser3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_IN_REGS]);
        return true;
    }

    // check hierarchy
    // deny if pUser is not Master and tries delete equal or higher profile
    if(pChatCommand->m_pUser->m_i32Profile > 0 && pReg->m_ui16Profile <= pChatCommand->m_pUser->m_i32Profile) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DelRegUser4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_YOURE_NOT_ALWD_TO_DLT_USER_THIS_PRFL]);
        return true;
    }

	UncountDeflood(pChatCommand);

    RegManager::m_Ptr->Delete(pReg);

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::DelRegUser5", "<%s> *** %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_REMOVED_LWR], pChatCommand->m_sCommand, 
			LanguageManager::m_Ptr->m_sTexts[LAN_FROM_REGS]);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::DelRegUser6", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_sCommand, 
			LanguageManager::m_Ptr->m_sTexts[LAN_REMOVED_FROM_REGS]);
    }

    return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::Debug(ChatCommand * pChatCommand) { // !debug port/off
    if(((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
        SendNoPermission(pChatCommand);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen < 7) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Debug1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cdebug <%s>, %cdebug off. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_PORT_LWR], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_NO_PARAM_GIVEN]);
    }

    pChatCommand->m_sCommand += 6;

	if(strcasecmp(pChatCommand->m_sCommand, "off") == 0) {
        if(UdpDebug::m_Ptr->CheckUdpSub(pChatCommand->m_pUser) == true) {
            if(UdpDebug::m_Ptr->Remove(pChatCommand->m_pUser) == true) {
                UncountDeflood(pChatCommand);

                pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Debug2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
					LanguageManager::m_Ptr->m_sTexts[LAN_UNSUB_FROM_UDP_DBG]);

				UdpDebug::m_Ptr->BroadcastFormat("[SYS] Debug subscription cancelled: %s (%s)", pChatCommand->m_pUser->m_sNick, pChatCommand->m_pUser->m_sIP);

                return true;
            } else {
                pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Debug3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
					LanguageManager::m_Ptr->m_sTexts[LAN_UNABLE_FIND_UDP_DBG_INTER]);
                return true;
            }
        } else {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Debug4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_NOT_UDP_DEBUG_SUBSCRIB]);
            return true;
        }
    } else {
        if(UdpDebug::m_Ptr->CheckUdpSub(pChatCommand->m_pUser) == true) {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Debug5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cdebug off %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_ALRD_UDP_SUBSCRIP_TO_UNSUB], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IN_MAINCHAT]);
            return true;
        }

        int iDbgPort = atoi(pChatCommand->m_sCommand);
        if(iDbgPort <= 0 || iDbgPort > 65535) {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Debug6", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %cdebug <%s>, %cdebug off.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_PORT_LWR], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0]);
            return true;
        }

        if(UdpDebug::m_Ptr->New(pChatCommand->m_pUser, (uint16_t)iDbgPort) == true) {
			UncountDeflood(pChatCommand);

            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Debug7", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %d. %s %cdebug off %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_SUBSCIB_UDP_DEBUG_ON_PORT], iDbgPort, LanguageManager::m_Ptr->m_sTexts[LAN_TO_UNSUB_TYPE], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], LanguageManager::m_Ptr->m_sTexts[LAN_IN_MAINCHAT]);

			UdpDebug::m_Ptr->BroadcastFormat("[SYS] New Debug subscriber: %s (%s)", pChatCommand->m_pUser->m_sNick, pChatCommand->m_pUser->m_sIP);

            return true;
        } else {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Debug8", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_UDP_DEBUG_SUBSCRIB_FAILED]);
            return true;
        }
    }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
