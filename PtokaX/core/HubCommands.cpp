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
//---------------------------------------------------------------------------
#include "HubCommands.h"
//---------------------------------------------------------------------------
ChatCommand HubCommands::m_ChatCommand = { NULL, NULL, 0, false };
//---------------------------------------------------------------------------

bool HubCommands::DoCommand(User * pUser, char * sCommand, const uint32_t ui32CmdLen, bool bFromPM/* = false*/) {
	m_ChatCommand.m_pUser = pUser;
	m_ChatCommand.m_sCommand = sCommand;
	m_ChatCommand.m_ui32CommandLen = ui32CmdLen;
	m_ChatCommand.m_bFromPM = bFromPM;

    if(bFromPM == false) {
    	// !me text
		if(strncasecmp(sCommand+pUser->m_ui8NickLen, "me ", 3) == 0) {
    	    if(ui32CmdLen - (pUser->m_ui8NickLen + 4) > 4) {
                sCommand[0] = '*';
    			sCommand[1] = ' ';
    			memcpy(sCommand+2, pUser->m_sNick, pUser->m_ui8NickLen);
                Users::m_Ptr->SendChat2All(pUser, sCommand, ui32CmdLen-4, NULL);

                return true;
    	    }

    	    return false;
        }

        // PPK ... optional reply commands in chat to PM
        m_ChatCommand.m_bFromPM = SettingManager::m_Ptr->m_bBools[SETBOOL_REPLY_TO_HUB_COMMANDS_AS_PM];

        m_ChatCommand.m_sCommand[ui32CmdLen-5] = '\0'; // get rid of the pipe
        m_ChatCommand.m_sCommand += pUser->m_ui8NickLen;

        m_ChatCommand.m_ui32CommandLen = ui32CmdLen - (pUser->m_ui8NickLen + 5);
    } else {
        m_ChatCommand.m_sCommand++;
        m_ChatCommand.m_ui32CommandLen = ui32CmdLen - (pUser->m_ui8NickLen + 6);
    }

    switch(tolower(m_ChatCommand.m_sCommand[0])) {
        case 'g':
            // !getbans
			if(m_ChatCommand.m_ui32CommandLen == 7 && strncasecmp(m_ChatCommand.m_sCommand+1, "etbans", 6) == 0) {
				return GetBans(&m_ChatCommand);
            }

            // !gag nick
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "ag ", 3) == 0) {
				return Gag(&m_ChatCommand);
            }

            // !getinfo nick
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "etinfo ", 7) == 0) {
				return GetInfo(&m_ChatCommand);
            }

            // !getipinfo ip
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "etipinfo ", 9) == 0) {
				return GetIpInfo(&m_ChatCommand);
            }

            // !gettempbans
			if(m_ChatCommand.m_ui32CommandLen == 11 && strncasecmp(m_ChatCommand.m_sCommand+1, "ettempbans", 10) == 0) {
				return GetTempBans(&m_ChatCommand);
            }

            // !getscripts
			if(m_ChatCommand.m_ui32CommandLen == 10 && strncasecmp(m_ChatCommand.m_sCommand+1, "etscripts", 9) == 0) {
				return GetScripts(&m_ChatCommand);
            }

            // !getpermbans
			if(m_ChatCommand.m_ui32CommandLen == 11 && strncasecmp(m_ChatCommand.m_sCommand+1, "etpermbans", 10) == 0) {
				return GetPermBans(&m_ChatCommand);
            }

            // !getrangebans
			if(m_ChatCommand.m_ui32CommandLen == 12 && strncasecmp(m_ChatCommand.m_sCommand+1, "etrangebans", 11) == 0) {
				return GetRangeBans(&m_ChatCommand);
            }

            // !getrangepermbans
			if(m_ChatCommand.m_ui32CommandLen == 16 && strncasecmp(m_ChatCommand.m_sCommand+1, "etrangepermbans", 15) == 0) {
				return GetRangePermBans(&m_ChatCommand);
            }

            // !getrangetempbans
			if(m_ChatCommand.m_ui32CommandLen == 16 && strncasecmp(m_ChatCommand.m_sCommand+1, "etrangetempbans", 15) == 0) {
				return GetRangeTempBans(&m_ChatCommand);
            }

            return false;

        case 'n':
            // !nickban nick reason
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "ickban ", 7) == 0) {
				return NickBan(&m_ChatCommand);
            }

            // !nicktempban nick time reason ...  m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "icktempban ", 11) == 0) {
				return NickTempBan(&m_ChatCommand);
            }

            return false;

        case 'b':
        	// !ban nick reason
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "an ", 3) == 0) {
				return Ban(&m_ChatCommand);
            }

            // !banip ip reason
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "anip ", 5) == 0) {
				return BanIp(&m_ChatCommand);
            }

            return false;

        case 't':
            // !tempban nick time reason ... m = minutes, h = hours, d = day, w = weeks, M = months, Y = years
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "empban ", 7) == 0) {
				return TempBan(&m_ChatCommand);
            }

            // !tempbanip nick time reason ... m = minutes, h = hours, d = day, w = weeks, M = months, Y = years
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "empbanip ", 9) == 0) {
				return TempBanIp(&m_ChatCommand);
            }

            // !tempunban nick/ip
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "empunban ", 9) == 0) {
				return TempUnban(&m_ChatCommand);
            }

            // !topic text/off
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "opic ", 5) == 0 ) {
				return Topic(&m_ChatCommand);
            }

            return false;

        case 'm':          
            // !myip
			if(m_ChatCommand.m_ui32CommandLen == 4 && strncasecmp(m_ChatCommand.m_sCommand+1, "yip", 3) == 0) {
				return MyIp(&m_ChatCommand);
            }

            // !massmsg text
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "assmsg ", 7) == 0) {
				return MassMsg(&m_ChatCommand);
            }

            return false;

        case 'r':
        	// !restartscripts
			if(m_ChatCommand.m_ui32CommandLen == 14 && strncasecmp(m_ChatCommand.m_sCommand+1, "estartscripts", 13) == 0) {
				return RestartScripts(&m_ChatCommand);
            }

			// !restart
			if(m_ChatCommand.m_ui32CommandLen == 7 && strncasecmp(m_ChatCommand.m_sCommand+1, "estart", 6) == 0) {
				return Restart(&m_ChatCommand);
            }

            // !reloadtxt
			if(m_ChatCommand.m_ui32CommandLen == 9 && strncasecmp(m_ChatCommand.m_sCommand+1, "eloadtxt", 8) == 0) {
				return ReloadTxt(&m_ChatCommand);
            }

			// !restartscript scriptname
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "estartscript ", 13) == 0) {
				return RestartScript(&m_ChatCommand);
            }

            // !rangeban fromip toip reason
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "angeban ", 8) == 0) {
				return RangeBan(&m_ChatCommand);
            }

            // !rangetempban fromip toip time reason ... m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "angetempban ", 12) == 0) {
				return RangeTempBan(&m_ChatCommand);
            }

            // !rangeunban fromip toip
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "angeunban ", 10) == 0) {
				return RangeUnBan(&m_ChatCommand);
            }

            // !rangetempunban fromip toip
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "angetempunban ", 14) == 0) {
				return RangeTempUnBan(&m_ChatCommand);
            }

            // !rangepermunban fromip toip
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "angepermunban ", 14) == 0) {
				return RangePermUnBan(&m_ChatCommand);
            }

            // !reguser nick profile_name
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "eguser ", 7) == 0) {
				return RegNewUser(&m_ChatCommand);
            }

            return false;

        case 'u':
            // !unban nick/ip
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "nban ", 5) == 0) {
				return Unban(&m_ChatCommand);
            }

            // !ungag nick
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "ngag ", 5) == 0) {
				return Ungag(&m_ChatCommand);
            }

            return false;

        case 'o':
            // !op nick
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "p ", 2) == 0) {
				return Op(&m_ChatCommand);
            }

            // !opmassmsg text
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "pmassmsg ", 9) == 0) {
				return OpMassMsg(&m_ChatCommand);;
            }

            return false;

        case 'd':
            // !drop nick
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "rop ", 4) == 0) {
				return Drop(&m_ChatCommand);
            }

            // !delreguser nick
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "elreguser ", 10) == 0) {
				return DelRegUser(&m_ChatCommand);
            }

            // !debug port/off
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "ebug ", 5) == 0) {
				return Debug(&m_ChatCommand);
            }

            return false;

        case 'c':
/*
            // !crash
            // Code to test if 32bit DC++ based client is vulnerable to zlib bomb
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "rash", 4) == 0) {

                char * sTmp = (char*)calloc(392*1024*1024, 1);

                uint32_t iLen = 0;
                char *sData = ZlibUtility->CreateZPipe(sTmp, 392*1024*1024, iLen);

                if(iLen == 0) {
                    printf("0!");
                } else {
                    printf("LEN %u", iLen);
                    if(UserPutInSendBuf(pUser, sData, iLen)) {
                        UserTry2Send(pUser);
                    }
                }

                free(sTmp);

                return true;
            }
*/
            // !clrtempbans
			if(m_ChatCommand.m_ui32CommandLen == 11 && strncasecmp(m_ChatCommand.m_sCommand+1, "lrtempbans", 10) == 0) {
				return ClrTempBans(&m_ChatCommand);
            }

            // !clrpermbans
			if(m_ChatCommand.m_ui32CommandLen == 11 && strncasecmp(m_ChatCommand.m_sCommand+1, "lrpermbans", 10) == 0) {
				return ClrPermBans(&m_ChatCommand);
            }

            // !clrrangetempbans
			if(m_ChatCommand.m_ui32CommandLen == 16 && strncasecmp(m_ChatCommand.m_sCommand+1, "lrrangetempbans", 15) == 0) {
				return ClrRangeTempBans(&m_ChatCommand);
            }

            // !clrrangepermbans
			if(m_ChatCommand.m_ui32CommandLen == 16 && strncasecmp(m_ChatCommand.m_sCommand+1, "lrrangepermbans", 15) == 0) {
				return ClrRangePermBans(&m_ChatCommand);
            }

            // !checknickban nick
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "hecknickban ", 12) == 0) {
				return CheckNickBan(&m_ChatCommand);
            }

            // !checkipban ip
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "heckipban ", 10) == 0) {
				return CheckIpBan(&m_ChatCommand);
            }

            // !checkrangeban ipfrom ipto
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "heckrangeban ", 13) == 0) {
				return CheckRangeBan(&m_ChatCommand);
            }

            return false;

        case 'a':
            // !addreguser nick pas> profile_name
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "ddreguser ", 10) == 0) {
				return AddRegUser(&m_ChatCommand);
            }

            return false;

        case 'h':
            // !help
			if(m_ChatCommand.m_ui32CommandLen == 4 && strncasecmp(m_ChatCommand.m_sCommand+1, "elp", 3) == 0) {
				return Help(&m_ChatCommand);
            }

            return false;

		case 's':
			// !stat !stats !statistic
			if((m_ChatCommand.m_ui32CommandLen == 4 && strncasecmp(m_ChatCommand.m_sCommand+1, "tat", 3) == 0) || (m_ChatCommand.m_ui32CommandLen == 5 && strncasecmp(m_ChatCommand.m_sCommand+1, "tats", 4) == 0) || (m_ChatCommand.m_ui32CommandLen == 9 && strncasecmp(m_ChatCommand.m_sCommand+1, "tatistic", 8) == 0)) {
				return Stats(&m_ChatCommand);
			}

			// !stopscript scriptname
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "topscript ", 10) == 0) {
				return StopScript(&m_ChatCommand);
			}

			// !startscript scriptname
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "tartscript ", 11) == 0) {
				return StartScript(&m_ChatCommand);
			}

			return false;

		case 'p':
            // !passwd password
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "asswd ", 6) == 0) {
				return Passwd(&m_ChatCommand);
            }

            // !permunban what
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "ermunban ", 9) == 0) {
				return PermUnban(&m_ChatCommand);
            }

            return false;

        case 'f':
            // !fullban nick reason
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "ullban ", 7) == 0) {
				return FullBan(&m_ChatCommand);
            }

            // !fullbanip ip reason
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "ullbanip ", 9) == 0) {
				return FullBanIp(&m_ChatCommand);
            }

            // Hub commands: !fulltempban nick time reason ... PPK m = minutes, h = hours, d = days, w = weeks, M = months , Y = years
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "ulltempban ", 11) == 0) {
				return FullTempBan(&m_ChatCommand);
            }

            // !fulltempbanip ip time reason ... m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "ulltempbanip ", 13) == 0) {
				return FullTempBanIp(&m_ChatCommand);
            }

            // !fullrangeban fromip toip reason
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "ullrangeban ", 12) == 0) {
				return FullRangeBan(&m_ChatCommand);
            }

            // !fullrangetempban fromip toip time reason ... m = minutes, h = hours, d = days, w = weeks, M = months, Y = years
			if(strncasecmp(m_ChatCommand.m_sCommand+1, "ullrangetempban ", 16) == 0) {
				return FullRangeTempBan(&m_ChatCommand);
            }
            
            return false;
    }

	return false; // PPK ... and send to all as chat ;)
}
//---------------------------------------------------------------------------

bool HubCommands::Ban(ChatCommand * pChatCommand, const bool bFull) {
    char * sReason = strchr(pChatCommand->m_sCommand, ' ');
    if(sReason != NULL) {
		pChatCommand->m_ui32CommandLen = (uint32_t)(sReason - pChatCommand->m_sCommand);

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
    }

    if(pChatCommand->m_sCommand[0] == '\0') {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Ban1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%sban <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

    if(pChatCommand->m_ui32CommandLen > 100) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Ban2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%sban <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        return true;
    }

    // Self-ban ?
	if(strcasecmp(pChatCommand->m_sCommand, pChatCommand->m_pUser->m_sNick) == 0) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Ban3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_YOU_CANT_BAN_YOURSELF]);
        return true;
    }

    User * pOtherUser = HashManager::m_Ptr->FindUser(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen);
    if(pOtherUser != NULL) {
        // PPK don't ban user with higher profile
        if(pOtherUser->m_i32Profile != -1 && pChatCommand->m_pUser->m_i32Profile > pOtherUser->m_i32Profile) {
			pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Ban4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_NOT_ALWD_TO_BAN], pChatCommand->m_sCommand);
            return true;
        }

        UncountDeflood(pChatCommand);

        // Ban user
        BanManager::m_Ptr->Ban(pOtherUser, sReason, pChatCommand->m_pUser->m_sNick, bFull);

        // Send user a message that he has been banned
        pOtherUser->SendFormat("HubCommands::Ban5", false, "<%s> %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_HAD_BEEN_BANNED_BCS], sReason == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sReason);

        // Send message to all OPs that the user have been banned
        if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
            GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::Ban6", "<%s> *** %s %s %s %s %s%s %s %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], pOtherUser->m_sIP, 
				LanguageManager::m_Ptr->m_sTexts[LAN_HAS_BEEN], bFull == true ? LanguageManager::m_Ptr->m_sTexts[LAN_FULL_LWR] : "", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_BY_LWR], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], 
				sReason == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
        }

        if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
			pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Ban7", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s %s %s%s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_sCommand, 
				LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], pOtherUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_BEEN], bFull == true ? LanguageManager::m_Ptr->m_sTexts[LAN_FULL_LWR] : "", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_LWR]);
        }

        // Finish him !
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) %sbanned by %s", pOtherUser->m_sNick, pOtherUser->m_sIP, bFull == true ? LanguageManager::m_Ptr->m_sTexts[LAN_FULL_LWR] : "", pChatCommand->m_pUser->m_sNick);
        pOtherUser->Close();

        return true;
    } else if(bFull == true) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::Ban8", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_ONLINE]);
        return true;
    } else {
        return NickBan(pChatCommand, sReason);
    }
}
//---------------------------------------------------------------------------

bool HubCommands::BanIp(ChatCommand * pChatCommand, const bool bFull) {
    char * sReason = strchr(pChatCommand->m_sCommand, ' ');
    if(sReason != NULL) {
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
    }

    if(pChatCommand->m_sCommand[0] == '\0') {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::BanIp1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%sbanip <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager::m_Ptr->m_sTexts[LAN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

	// Check IP!
	if(isIP(pChatCommand->m_sCommand) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::BanIp2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%sbanip <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager::m_Ptr->m_sTexts[LAN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_VALID_IP_SPECIFIED]);
        return true;
	}

    switch(BanManager::m_Ptr->BanIp(NULL, pChatCommand->m_sCommand, sReason, pChatCommand->m_pUser->m_sNick, bFull)) {
        case 0: {
			UncountDeflood(pChatCommand);

			uint8_t ui128Hash[16];
			memset(ui128Hash, 0, 16);

            HashIP(pChatCommand->m_sCommand, ui128Hash);
          
            User * pCurUser = NULL,
                * pNextUser = HashManager::m_Ptr->FindUser(ui128Hash);

            while(pNextUser != NULL) {
            	pCurUser = pNextUser;
                pNextUser = pCurUser->m_pHashIpTableNext;

                if(pCurUser == pChatCommand->m_pUser || (pCurUser->m_i32Profile != -1 && ProfileManager::m_Ptr->IsAllowed(pCurUser, ProfileManager::ENTERIFIPBAN) == true)) {
                    continue;
                }

                // PPK don't nickban user with higher profile
                if(pCurUser->m_i32Profile != -1 && pChatCommand->m_pUser->m_i32Profile > pCurUser->m_i32Profile) {
					pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::BanIp3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
						LanguageManager::m_Ptr->m_sTexts[LAN_YOU_NOT_ALLOWED_TO], LanguageManager::m_Ptr->m_sTexts[LAN_BAN_LWR], pCurUser->m_sNick);

                    continue;
                }

                pCurUser->SendFormat("HubCommands::BanIp4", false, "<%s> %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_HAD_BEEN_BANNED_BCS], sReason == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sReason);

				UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) ipbanned by %s", pCurUser->m_sNick, pCurUser->m_sIP, pChatCommand->m_pUser->m_sNick);

                pCurUser->Close();
            }

            if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::BanIp5", "<%s> *** %s %s %s%s %s %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_IS_LWR], bFull == true ? LanguageManager::m_Ptr->m_sTexts[LAN_FULL_LWR] : "", 
					LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_BY_LWR], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], sReason == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
            }

            if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
				pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::BanIp6", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s%s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_sCommand, 
				LanguageManager::m_Ptr->m_sTexts[LAN_IS_LWR], bFull == true ? LanguageManager::m_Ptr->m_sTexts[LAN_FULL_LWR] : "", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_LWR]);
            }

            return true;
        }
        case 1: {
			pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::BanIp7", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%sbanip <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager::m_Ptr->m_sTexts[LAN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_VALID_IP_SPECIFIED]);

            return true;
        }
        case 2: {
			pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::BanIp8", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s %s%s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_IP], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_IS_ALREADY], bFull == true ? LanguageManager::m_Ptr->m_sTexts[LAN_FULL_LWR] : "", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_LWR]);

            return true;
        }
        default:
            return true;
    }
}
//---------------------------------------------------------------------------

bool HubCommands::NickBan(ChatCommand * pChatCommand, char * sReason) {
    RegUser * pReg = RegManager::m_Ptr->Find(pChatCommand->m_sCommand, pChatCommand->m_ui32CommandLen);

    // don't nickban user with higher profile
    if(pReg != NULL && pChatCommand->m_pUser->m_i32Profile > pReg->m_ui16Profile) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_YOU_NOT_ALLOWED_TO], LanguageManager::m_Ptr->m_sTexts[LAN_BAN_LWR], pChatCommand->m_sCommand);
        return true;
    }

    if(BanManager::m_Ptr->NickBan(NULL, pChatCommand->m_sCommand, sReason, pChatCommand->m_pUser->m_sNick) == true) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s nickbanned by %s", pChatCommand->m_sCommand, pChatCommand->m_pUser->m_sNick);
    } else {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_NICK], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_IS_ALREDY_BANNED]);
        return true;
    }

    UncountDeflood(pChatCommand);

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::NickBan3", "<%s> *** %s %s %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_BEEN_BANNED_BY], pChatCommand->m_pUser->m_sNick, 
			LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], sReason == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::NickBan4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_sCommand, 
			LanguageManager::m_Ptr->m_sTexts[LAN_ADDED_TO_BANS]);
    }

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::TempBan(ChatCommand * pChatCommand, const bool bFull) {
    char * sCmdParts[] = { NULL, NULL, NULL };
    uint16_t ui16CmdPartsLen[] = { 0, 0, 0 };

    uint8_t ui8Part = 0;

    sCmdParts[ui8Part] = pChatCommand->m_sCommand; // nick start

    for(uint32_t ui32i = 0; ui32i < pChatCommand->m_ui32CommandLen; ui32i++) {
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
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBan1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%stempban <%s> <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

    if(ui16CmdPartsLen[0] > 100) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBan2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%stempban <%s> <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        return true;
    }

    // Self-ban ?
	if(strcasecmp(sCmdParts[0], pChatCommand->m_pUser->m_sNick) == 0) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBan3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_YOU_CANT_BAN_YOURSELF]);
        return true;
    }
           	    
    User * pOtherUser = HashManager::m_Ptr->FindUser(sCmdParts[0], ui16CmdPartsLen[0]);
    if(pOtherUser != NULL) {
        // PPK don't tempban user with higher profile
        if(pOtherUser->m_i32Profile != -1 && pChatCommand->m_pUser->m_i32Profile > pOtherUser->m_i32Profile) {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBan4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_NOT_ALWD_TO_TEMPBAN], sCmdParts[0]);
            return true;
        }

        uint8_t ui8Time = sCmdParts[1][ui16CmdPartsLen[1]-1];
        sCmdParts[1][ui16CmdPartsLen[1]-1] = '\0';
        int iTime = atoi(sCmdParts[1]);
        time_t acc_time, ban_time;

        if(iTime <= 0 || GenerateTempBanTime(ui8Time, (uint32_t)iTime, acc_time, ban_time) == false) {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBan5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%stempban <%s> <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], 
				LanguageManager::m_Ptr->m_sTexts[LAN_BAD_TIME_SPECIFIED]);
            return true;
        }

        BanManager::m_Ptr->TempBan(pOtherUser, sCmdParts[2], pChatCommand->m_pUser->m_sNick, 0, ban_time, bFull);
        UncountDeflood(pChatCommand);

        static char sTime[256];
        strcpy(sTime, formatTime((ban_time-acc_time)/60));

        // Send user a message that he has been tempbanned
        pOtherUser->SendFormat("HubCommands::TempBan6", false, "<%s> %s: %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_HAD_BEEN_TEMP_BANNED_TO], sTime, LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], 
			sCmdParts[2] == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);

        if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
            GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::TempBan7", "<%s> *** %s %s %s %s %s%s %s %s %s: %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], sCmdParts[0], LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], pOtherUser->m_sIP, 
				LanguageManager::m_Ptr->m_sTexts[LAN_HAS_BEEN], bFull == true ? LanguageManager::m_Ptr->m_sTexts[LAN_FULL_LWR] : "", LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BANNED], LanguageManager::m_Ptr->m_sTexts[LAN_BY_LWR], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_TO_LWR], sTime, 
				LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], sCmdParts[2] == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
        }

        if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
            pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBan8", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s %s %s%s %s: %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				sCmdParts[0], LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], pOtherUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_BEEN], bFull == true ? LanguageManager::m_Ptr->m_sTexts[LAN_FULL_LWR] : "", LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BANNED], LanguageManager::m_Ptr->m_sTexts[LAN_TO_LWR], sTime, 
				LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], sCmdParts[2] == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
        }

        // Finish him !
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) %stemp banned by %s", pOtherUser->m_sNick, pOtherUser->m_sIP, bFull == true ? "full " : "", pChatCommand->m_pUser->m_sNick);

        pOtherUser->Close();
        return true;
    } else if(bFull == true) {
        pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBan9", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], sCmdParts[0], LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_ONLINE]);
        return true;
    } else {
        return TempNickBan(pChatCommand, sCmdParts[0], sCmdParts[1], ui16CmdPartsLen[1], sCmdParts[2], true);
    }
}
//---------------------------------------------------------------------------

bool HubCommands::TempBanIp(ChatCommand * pChatCommand, const bool bFull) {
    char * sCmdParts[] = { NULL, NULL, NULL };
    uint16_t ui16CmdPartsLen[] = { 0, 0, 0 };

    uint8_t ui8Part = 0;

    sCmdParts[ui8Part] = pChatCommand->m_sCommand; // nick start

    for(uint32_t ui32i = 0; ui32i < pChatCommand->m_ui32CommandLen; ui32i++) {
        if(pChatCommand->m_sCommand[ui32i] == ' ') {
			pChatCommand->m_sCommand[ui32i] = '\0';
			ui16CmdPartsLen[ui8Part] = (uint16_t)((pChatCommand->m_sCommand+ui32i)-sCmdParts[ui8Part]);

            // are we on last space ???
            if(ui8Part == 1) {
                sCmdParts[2] = pChatCommand->m_sCommand+pChatCommand->m_ui32CommandLen+1;
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
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBanIp1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%stempbanip <%s> <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager::m_Ptr->m_sTexts[LAN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

    uint8_t ui8Time = sCmdParts[1][ui16CmdPartsLen[1]-1];
    sCmdParts[1][ui16CmdPartsLen[1]-1] = '\0';
    int iTime = atoi(sCmdParts[1]);
    time_t acc_time, ban_time;

    if(iTime <= 0 || GenerateTempBanTime(ui8Time, (uint32_t)iTime, acc_time, ban_time) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBanIp2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%stempbanip <%s> <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager::m_Ptr->m_sTexts[LAN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_BAD_TIME_SPECIFIED]);
        return true;
    }

	// Check IP!
	if(isIP(sCmdParts[0]) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBanIp2-1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%stempbanip <%s> <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager::m_Ptr->m_sTexts[LAN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_NO_VALID_IP_SPECIFIED]);
        return true;
	}

    switch(BanManager::m_Ptr->TempBanIp(NULL, sCmdParts[0], sCmdParts[2], pChatCommand->m_pUser->m_sNick, 0, ban_time, bFull)) {
        case 0: {
			uint8_t ui128Hash[16];
			memset(ui128Hash, 0, 16);

            HashIP(pChatCommand->m_sCommand, ui128Hash);
          
            User * pCur = NULL,
                * pNext = HashManager::m_Ptr->FindUser(ui128Hash);
            while(pNext != NULL) {
            	pCur = pNext;
                pNext = pCur->m_pHashIpTableNext;

                if(pCur == pChatCommand->m_pUser || (pCur->m_i32Profile != -1 && ProfileManager::m_Ptr->IsAllowed(pCur, ProfileManager::ENTERIFIPBAN) == true)) {
                    continue;
                }

                // PPK don't nickban user with higher profile
                if(pCur->m_i32Profile != -1 && pChatCommand->m_pUser->m_i32Profile > pCur->m_i32Profile) {
					pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBanIp3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
						LanguageManager::m_Ptr->m_sTexts[LAN_YOU_NOT_ALLOWED_TO], LanguageManager::m_Ptr->m_sTexts[LAN_BAN_LWR], pCur->m_sNick);
                    continue;
                }

                pCur->SendFormat("HubCommands::TempBanIp", false, "<%s> %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_HAD_BEEN_BANNED_BCS], sCmdParts[2] == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);

				UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) tempipbanned by %s", pCur->m_sNick, pCur->m_sIP, pChatCommand->m_pUser->m_sNick);

                pCur->Close();
            }
            break;
        }
        case 1: {
			pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBanIp4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%sbanip <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager::m_Ptr->m_sTexts[LAN_IP], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_NO_VALID_IP_SPECIFIED]);
            return true;
        }
        case 2: {
			pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBanIp5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s %s%s %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
				LanguageManager::m_Ptr->m_sTexts[LAN_IP], sCmdParts[0], LanguageManager::m_Ptr->m_sTexts[LAN_IS_ALREADY], bFull == true ? LanguageManager::m_Ptr->m_sTexts[LAN_FULL_LWR] : "", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TO_LONGER_TIME]);
            return true;
        }
        default:
            return true;
    }

	UncountDeflood(pChatCommand);

    static char sTime[256];
    strcpy(sTime, formatTime((ban_time-acc_time)/60));

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::TempBanIp", "<%s> *** %s %s %s%s %s %s %s: %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_BEEN], 
			bFull == true ? LanguageManager::m_Ptr->m_sTexts[LAN_FULL_LWR] : "", LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BANNED], LanguageManager::m_Ptr->m_sTexts[LAN_BY_LWR], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_TO_LWR], sTime, LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], 
			sCmdParts[2] == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempBanIp6", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s%s %s: %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], sCmdParts[0], 
			LanguageManager::m_Ptr->m_sTexts[LAN_HAS_BEEN], bFull == true ? LanguageManager::m_Ptr->m_sTexts[LAN_FULL_LWR] : "", LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BANNED], LanguageManager::m_Ptr->m_sTexts[LAN_TO_LWR], sTime, LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], 
			sCmdParts[2] == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
    }

	UdpDebug::m_Ptr->BroadcastFormat("[SYS] IP %s %stemp banned by %s", sCmdParts[0], bFull == true ? "full " : "", pChatCommand->m_pUser->m_sNick);

    return true;
}
//---------------------------------------------------------------------------

bool HubCommands::TempNickBan(ChatCommand * pChatCommand, char * sNick, char * sTime, const uint16_t ui16TimeLen, char * sReason, const bool bNotNickBan/* = false*/) {
    RegUser * pReg = RegManager::m_Ptr->Find(sNick, strlen(sNick));

    // don't nickban user with higher profile
    if(pReg != NULL && pChatCommand->m_pUser->m_i32Profile > pReg->m_ui16Profile) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempNickBan1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_YOU_NOT_ALLOWED_TO], LanguageManager::m_Ptr->m_sTexts[LAN_BAN_LWR], sNick);
        return true;
    }

    uint8_t ui8Time = sTime[ui16TimeLen-1];
    sTime[ui16TimeLen-1] = '\0';
    int iTime = atoi(sTime);
    time_t acc_time, ban_time;

    if(iTime <= 0 || GenerateTempBanTime(ui8Time, (uint32_t)iTime, acc_time, ban_time) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempNickBan2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%stempban <%s> <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bNotNickBan == false ? "nick" : "", LanguageManager::m_Ptr->m_sTexts[LAN_NICK_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_BAD_TIME_SPECIFIED]);
        return true;
    }

    if(BanManager::m_Ptr->NickTempBan(NULL, sNick, sReason, pChatCommand->m_pUser->m_sNick, 0, ban_time) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempNickBan3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_NICK], sNick, LanguageManager::m_Ptr->m_sTexts[LAN_IS_ALRD_BANNED_LONGER_TIME]);
        return true;
    }

	UncountDeflood(pChatCommand);

    char sBanTime[256];
    strcpy(sBanTime, formatTime((ban_time-acc_time)/60));

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::TempNickBan", "<%s> *** %s %s %s %s: %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], sNick, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_BEEN_TMPBND_BY], pChatCommand->m_pUser->m_sNick, 
			LanguageManager::m_Ptr->m_sTexts[LAN_TO_LWR], sBanTime, LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], sReason == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::TempNickBan4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s: %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], sNick, 
			LanguageManager::m_Ptr->m_sTexts[LAN_BEEN_TEMP_BANNED_TO], sBanTime, LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], sReason == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sReason);
    }

    UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick %s tempbanned by %s", sNick, pChatCommand->m_pUser->m_sNick);

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HubCommands::RangeBan(ChatCommand * pChatCommand, const bool bFull) {
    char *sCmdParts[] = { NULL, NULL, NULL };
    uint16_t ui16CmdPartsLen[] = { 0, 0, 0 };

    uint8_t ui8Part = 0;

    sCmdParts[ui8Part] = pChatCommand->m_sCommand; // nick start

    for(uint32_t ui32i = 0; ui32i < pChatCommand->m_ui32CommandLen; ui32i++) {
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

    if(ui16CmdPartsLen[0] > 39 || ui16CmdPartsLen[1] > 39) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeBan1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %c%srangeban <%s> <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_IP_LEN_39_CHARS]);
        return true;
    }

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

    if(ui16CmdPartsLen[0] == 0 || ui16CmdPartsLen[1] == 0 || HashIP(sCmdParts[0], ui128FromHash) == false || HashIP(sCmdParts[1], ui128ToHash) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeBan2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %c%srangeban <%s> <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

    if(memcmp(ui128ToHash, ui128FromHash, 16) <= 0) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeBan3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s %s %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR_FROM], sCmdParts[0], LanguageManager::m_Ptr->m_sTexts[LAN_TO_LWR], sCmdParts[1], LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_VALID_RANGE]);
        return true;
    }

    if(BanManager::m_Ptr->RangeBan(sCmdParts[0], ui128FromHash, sCmdParts[1], ui128ToHash, sCmdParts[2], pChatCommand->m_pUser->m_sNick, bFull) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeBan4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s-%s %s %s%s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], sCmdParts[0], sCmdParts[1], LanguageManager::m_Ptr->m_sTexts[LAN_IS_ALREADY], bFull == true ? LanguageManager::m_Ptr->m_sTexts[LAN_FULL_LWR] : "", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_LWR]);
        return true;
    }

	UncountDeflood(pChatCommand);

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::RangeBan5", "<%s> *** %s %s-%s %s %s%s %s %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], sCmdParts[0], sCmdParts[1], LanguageManager::m_Ptr->m_sTexts[LAN_IS_LWR], 
			bFull == true ? LanguageManager::m_Ptr->m_sTexts[LAN_FULL_LWR] : "", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_BY_LWR], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], 
			sCmdParts[2] == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[2]);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeBan6", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s-%s %s %s%s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], sCmdParts[0], sCmdParts[1], LanguageManager::m_Ptr->m_sTexts[LAN_IS_LWR], bFull == true ? LanguageManager::m_Ptr->m_sTexts[LAN_FULL_LWR] : "", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_LWR]);
    }

    return true;
}
//---------------------------------------------------------------------------

bool HubCommands::RangeTempBan(ChatCommand * pChatCommand, const bool bFull) {
    char *sCmdParts[] = { NULL, NULL, NULL, NULL };
    uint16_t ui16CmdPartsLen[] = { 0, 0, 0, 0 };

    uint8_t ui8Part = 0;

    sCmdParts[ui8Part] = pChatCommand->m_sCommand; // nick start

    for(uint32_t ui32i = 0; ui32i < pChatCommand->m_ui32CommandLen; ui32i++) {
        if(pChatCommand->m_sCommand[ui32i] == ' ') {
            pChatCommand->m_sCommand[ui32i] = '\0';
            ui16CmdPartsLen[ui8Part] = (uint16_t)((pChatCommand->m_sCommand+ui32i)-sCmdParts[ui8Part]);

            // are we on last space ???
            if(ui8Part == 2) {
                sCmdParts[3] = pChatCommand->m_sCommand+ui32i+1;
                ui16CmdPartsLen[3] = (uint16_t)(pChatCommand->m_ui32CommandLen-ui32i-1);
                break;
            }

            ui8Part++;
            sCmdParts[ui8Part] = pChatCommand->m_sCommand+ui32i+1;
        }
    }

    if(sCmdParts[3] == NULL && ui16CmdPartsLen[2] == 0 && sCmdParts[2] != NULL) {
        ui16CmdPartsLen[2] = (uint16_t)(pChatCommand->m_ui32CommandLen-(sCmdParts[2]-pChatCommand->m_sCommand));
    }

    if(sCmdParts[3] != NULL && ui16CmdPartsLen[3] == 0) {
        sCmdParts[3] = NULL;
    }

	if(ui16CmdPartsLen[3] > 511) {
		sCmdParts[3][508] = '.';
		sCmdParts[3][509] = '.';
		sCmdParts[3][510] = '.';
		sCmdParts[3][511] = '\0';
	}

    if(ui16CmdPartsLen[0] > 39 || ui16CmdPartsLen[1] > 39) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeTempBan1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %c%srangetempban <%s> <%s> <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_IP_LEN_39_CHARS]);
        return true;
    }

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

    if(ui16CmdPartsLen[0] == 0 || ui16CmdPartsLen[1] == 0 || ui16CmdPartsLen[2] == 0 || HashIP(sCmdParts[0], ui128FromHash) == false || HashIP(sCmdParts[1], ui128ToHash) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeTempBan2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %c%srangetempban <%s> <%s> <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

    if(memcmp(ui128ToHash, ui128FromHash, 16) <= 0) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeTempBan3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s %s %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_ERROR_FROM], 
			sCmdParts[0], LanguageManager::m_Ptr->m_sTexts[LAN_TO_LWR], sCmdParts[1], LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_VALID_RANGE]);
        return true;
    }

    uint8_t ui8Time = sCmdParts[2][ui16CmdPartsLen[2]-1];
    sCmdParts[2][ui16CmdPartsLen[2]-1] = '\0';
    int iTime = atoi(sCmdParts[2]);
    time_t acc_time, ban_time;

    if(iTime <= 0 || GenerateTempBanTime(ui8Time, (uint32_t)iTime, acc_time, ban_time) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeTempBan4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %c%srangetempban <%s> <%s> <%s> <%s>. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][0], bFull == true ? "full" : "", LanguageManager::m_Ptr->m_sTexts[LAN_FROMIP], LanguageManager::m_Ptr->m_sTexts[LAN_TOIP], LanguageManager::m_Ptr->m_sTexts[LAN_TIME_LWR], 
			LanguageManager::m_Ptr->m_sTexts[LAN_REASON_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_BAD_TIME_SPECIFIED]);
        return true;
    }

    if(BanManager::m_Ptr->RangeTempBan(sCmdParts[0], ui128FromHash, sCmdParts[1], ui128ToHash, sCmdParts[3], pChatCommand->m_pUser->m_sNick, 0, ban_time, bFull) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeTempBan5", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s-%s %s %s%s %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], 
			sCmdParts[0], sCmdParts[1], LanguageManager::m_Ptr->m_sTexts[LAN_IS_ALREADY], bFull == true ? LanguageManager::m_Ptr->m_sTexts[LAN_FULL_LWR] : "", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_TO_LONGER_TIME]);
        return true;
    }

	UncountDeflood(pChatCommand);

    static char sTime[256];
    strcpy(sTime, formatTime((ban_time-acc_time)/60));

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::RangeTempBan", "<%s> *** %s %s-%s %s %s%s %s %s %s: %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], sCmdParts[0], sCmdParts[1], LanguageManager::m_Ptr->m_sTexts[LAN_IS_LWR], 
			bFull == true ? LanguageManager::m_Ptr->m_sTexts[LAN_FULL_LWR] : "", LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BANNED], LanguageManager::m_Ptr->m_sTexts[LAN_BY_LWR], pChatCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_TO_LWR], sTime, LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], 
			sCmdParts[3] == NULL ? LanguageManager::m_Ptr->m_sTexts[LAN_NO_REASON_SPECIFIED] : sCmdParts[3]);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeTempBan6", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s-%s %s %s%s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], 
			sCmdParts[0], sCmdParts[1], LanguageManager::m_Ptr->m_sTexts[LAN_IS_LWR], bFull == true ? LanguageManager::m_Ptr->m_sTexts[LAN_FULL_LWR] : "", LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BANNED], LanguageManager::m_Ptr->m_sTexts[LAN_TO_LWR], sTime);
    }

    return true;
}
//---------------------------------------------------------------------------

bool HubCommands::RangeUnban(ChatCommand * pChatCommand, const uint8_t ui8Type) {
	char * sToIp = strchr(pChatCommand->m_sCommand, ' ');
	if(sToIp != NULL) {
        sToIp[0] = '\0';
        sToIp++;
    }

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

	if(sToIp == NULL || pChatCommand->m_sCommand[0] == '\0' || sToIp[0] == '\0' || HashIP(pChatCommand->m_sCommand, ui128FromHash) == false || HashIP(sToIp, ui128ToHash) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnban1", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], LanguageManager::m_Ptr->m_sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

    if(memcmp(ui128ToHash, ui128FromHash, 16) <= 0) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnban2", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s %s %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR_FROM], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_TO_LWR], sToIp, LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_VALID_RANGE]);
        return true;
    }

    if(BanManager::m_Ptr->RangeUnban(ui128FromHash, ui128ToHash, ui8Type) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnban3", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s-%s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], pChatCommand->m_sCommand, sToIp, LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_IN_MY_RANGE], ui8Type == 1 ? LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BANS_LWR] : LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BANS_LWR]);
        return true;
    }

	UncountDeflood(pChatCommand);

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::RangeUnban", "<%s> *** %s %s-%s %s %s by %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], pChatCommand->m_sCommand, sToIp, 
			LanguageManager::m_Ptr->m_sTexts[LAN_IS_REMOVED_FROM_RANGE], ui8Type == 1 ? LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BANS_LWR] : LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BANS_LWR], pChatCommand->m_pUser->m_sNick);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnban4", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s-%s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], pChatCommand->m_sCommand, sToIp, LanguageManager::m_Ptr->m_sTexts[LAN_IS_REMOVED_FROM_RANGE], ui8Type == 1 ? LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BANS_LWR] : LanguageManager::m_Ptr->m_sTexts[LAN_PERM_BANS_LWR]);
    }
    
    return true;
}
//---------------------------------------------------------------------------

bool HubCommands::RangeUnban(ChatCommand * pChatCommand) {
	char * sToIp = strchr(pChatCommand->m_sCommand, ' ');
	if(sToIp != NULL) {
        sToIp[0] = '\0';
        sToIp++;
    }

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

	if(sToIp == NULL || pChatCommand->m_sCommand[0] == '\0' || sToIp[0] == '\0' || HashIP(pChatCommand->m_sCommand, ui128FromHash) == false || HashIP(sToIp, ui128ToHash) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnban21", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s. %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_SNTX_ERR_IN_CMD], LanguageManager::m_Ptr->m_sTexts[LAN_BAD_PARAMS_GIVEN]);
        return true;
    }

    if(memcmp(ui128ToHash, ui128FromHash, 16) <= 0) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnban22", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> *** %s %s %s %s %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_ERROR_FROM], pChatCommand->m_sCommand, LanguageManager::m_Ptr->m_sTexts[LAN_TO_LWR], sToIp, LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_VALID_RANGE]);
        return true;
    }

    if(BanManager::m_Ptr->RangeUnban(ui128FromHash, ui128ToHash) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnban23", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s-%s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], pChatCommand->m_sCommand, sToIp, LanguageManager::m_Ptr->m_sTexts[LAN_IS_NOT_IN_MY_RANGE_BANS]);
        return true;
    }

    UncountDeflood(pChatCommand);

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("HubCommands::RangeUnban2", "<%s> *** %s %s-%s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], pChatCommand->m_sCommand, sToIp, 
			LanguageManager::m_Ptr->m_sTexts[LAN_IS_REMOVED_FROM_RANGE_BANS_BY], pChatCommand->m_pUser->m_sNick);
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pChatCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
		pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::RangeUnban24", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s %s-%s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
			LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], pChatCommand->m_sCommand, sToIp, LanguageManager::m_Ptr->m_sTexts[LAN_IS_REMOVED_FROM_RANGE_BANS]);
    }
    
    return true;
}
//---------------------------------------------------------------------------

void HubCommands::SendNoPermission(ChatCommand * pChatCommand) {
	pChatCommand->m_pUser->SendFormatCheckPM("HubCommands::SendNoPermission", pChatCommand->m_bFromPM == true ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC] : NULL, true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], 
		LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD]);
}
//---------------------------------------------------------------------------

int HubCommands::CheckFromPm(ChatCommand * pChatCommand) {
    if(pChatCommand->m_bFromPM == false) {
        return 0;
    }

    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$To: %s From: %s $", pChatCommand->m_pUser->m_sNick, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC]);
    if(iMsgLen <= 0) {
        return 0;
    }

    return iMsgLen;
}
//---------------------------------------------------------------------------

void HubCommands::UncountDeflood(ChatCommand * pChatCommand) {
    if(pChatCommand->m_bFromPM == false) {
        if(pChatCommand->m_pUser->m_ui16ChatMsgs != 0) {
			pChatCommand->m_pUser->m_ui16ChatMsgs--;
			pChatCommand->m_pUser->m_ui16ChatMsgs2--;
        }
    } else {
        if(pChatCommand->m_pUser->m_ui16PMs != 0) {
			pChatCommand->m_pUser->m_ui16PMs--;
			pChatCommand->m_pUser->m_ui16PMs2--;
        }
    }
}
//---------------------------------------------------------------------------
