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
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "LanguageManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#include "DeFlood.h"
//---------------------------------------------------------------------------

bool DeFloodCheckForFlood(User * pUser, const uint8_t ui8DefloodType, const int16_t ui16Action,
  uint16_t &ui16Count, uint64_t &ui64LastOkTick, 
  const int16_t ui16DefloodCount, const uint32_t ui32DefloodTime, char * sOtherNick/* = NULL*/) {
    if(ui16Count == 0) {
		ui64LastOkTick = ServerManager::m_ui64ActualTick;
    } else if(ui16Count == ui16DefloodCount) {
		if((ui64LastOkTick+ui32DefloodTime) > ServerManager::m_ui64ActualTick) {
            DeFloodDoAction(pUser, ui8DefloodType, ui16Action, ui16Count, sOtherNick);
            return true;
        } else {
            ui64LastOkTick = ServerManager::m_ui64ActualTick;
			ui16Count = 0;
        }
    } else if(ui16Count > ui16DefloodCount) {
        if((ui64LastOkTick+ui32DefloodTime) > ServerManager::m_ui64ActualTick) {
            if(ui16Action == 2 && ui16Count == (ui16DefloodCount*2)) {
				pUser->m_ui32DefloodWarnings++;

                if(DeFloodCheckForWarn(pUser, ui8DefloodType, sOtherNick) == true) {
                    return true;
                }
                ui16Count -= ui16DefloodCount;
            }
            ui16Count++;
            return true;
        } else {
            ui64LastOkTick = ServerManager::m_ui64ActualTick;
			ui16Count = 0;
        }
    } else if((ui64LastOkTick+ui32DefloodTime) <= ServerManager::m_ui64ActualTick) {
        ui64LastOkTick = ServerManager::m_ui64ActualTick;
		ui16Count = 0;
    }

    ui16Count++;
    return false;
}
//---------------------------------------------------------------------------

bool DeFloodCheckForSameFlood(User * pUser, const uint8_t ui8DefloodType, const int16_t ui16Action,
  uint16_t &ui16Count, const uint64_t ui64LastOkTick,
  const int16_t ui16DefloodCount, const uint32_t ui32DefloodTime, 
  char * sNewData, const size_t ui32NewDataLen, 
  char * sOldData, const uint16_t ui16OldDataLen, bool &bNewData, char * sOtherNick/* = NULL*/) {
	if((uint32_t)ui16OldDataLen == ui32NewDataLen && (ServerManager::m_ui64ActualTick >= ui64LastOkTick &&
      (ui64LastOkTick+ui32DefloodTime) > ServerManager::m_ui64ActualTick) &&
	  memcmp(sNewData, sOldData, ui16OldDataLen) == 0) {
		if(ui16Count < ui16DefloodCount) {
			ui16Count++;

            return false;
		} else if(ui16Count == ui16DefloodCount) {
			DeFloodDoAction(pUser, ui8DefloodType, ui16Action, ui16Count, sOtherNick);
            if(pUser->m_ui8State < User::STATE_CLOSING) {
                ui16Count++;
            }

            return true;
		} else {
			if(ui16Action == 2 && ui16Count == (ui16DefloodCount*2)) {
				pUser->m_ui32DefloodWarnings++;

				if(DeFloodCheckForWarn(pUser, ui8DefloodType, sOtherNick) == true) {
					return true;
                }
                ui16Count -= ui16DefloodCount;
            }
			ui16Count++;

            return true;
        }
	} else {
    	bNewData = true;
        return false;
    }
}
//---------------------------------------------------------------------------

bool DeFloodCheckForDataFlood(User * pUser, const uint8_t ui8DefloodType, const int16_t ui16Action,
	uint32_t &ui32Count, uint64_t &ui64LastOkTick,
	const int16_t ui16DefloodCount, const uint32_t ui32DefloodTime) {
	if((uint16_t)(ui32Count/1024) >= ui16DefloodCount) {
		if((ui64LastOkTick+ui32DefloodTime) > ServerManager::m_ui64ActualTick) {
        	if((pUser->m_ui32BoolBits & User::BIT_RECV_FLOODER) == User::BIT_RECV_FLOODER) {
                return true;
            }
			pUser->m_ui32BoolBits |= User::BIT_RECV_FLOODER;
			uint16_t ui16Count = (uint16_t)ui32Count;
			DeFloodDoAction(pUser, ui8DefloodType, ui16Action, ui16Count, NULL);
            return true;
        } else {
			pUser->m_ui32BoolBits &= ~User::BIT_RECV_FLOODER;
            ui64LastOkTick = ServerManager::m_ui64ActualTick;
			ui32Count = 0;
            return false;
        }
	} else if((ui64LastOkTick+ui32DefloodTime) <= ServerManager::m_ui64ActualTick) {
		pUser->m_ui32BoolBits &= ~User::BIT_RECV_FLOODER;
        ui64LastOkTick = ServerManager::m_ui64ActualTick;
        ui32Count = 0;
        return false;
	}

	return false;
}
//---------------------------------------------------------------------------

void DeFloodDoAction(User * pUser, const uint8_t ui8DefloodType, const int16_t ui16Action, uint16_t &ui16Count, char * sOtherNick) {
    switch(ui16Action) {
        case 1: {
            pUser->SendFormatCheckPM("DeFloodDoAction1", sOtherNick, true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], DeFloodGetMessage(ui8DefloodType, 0));

            if(ui8DefloodType != DEFLOOD_MAX_DOWN) {
                ui16Count++;
            }
            return;
        }
        case 2:
			pUser->m_ui32DefloodWarnings++;

			if(DeFloodCheckForWarn(pUser, ui8DefloodType, sOtherNick) == false && ui8DefloodType != DEFLOOD_MAX_DOWN) {
                ui16Count++;
            }

            return;
        case 3: {
            pUser->SendFormatCheckPM("DeFloodDoAction2", sOtherNick, false, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], DeFloodGetMessage(ui8DefloodType, 0));

			DeFloodReport(pUser, ui8DefloodType, LanguageManager::m_Ptr->m_sTexts[LAN_WAS_DISCONNECTED]);

			pUser->Close();
            return;
        }
        case 4: {
			BanManager::m_Ptr->TempBan(pUser, DeFloodGetMessage(ui8DefloodType, 1), NULL, 0, 0, false);

            pUser->SendFormatCheckPM("DeFloodDoAction3", sOtherNick, false, "<%s> %s: %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC],  LanguageManager::m_Ptr->m_sTexts[LAN_YOU_BEING_KICKED_BCS], DeFloodGetMessage(ui8DefloodType, 1));

            DeFloodReport(pUser, ui8DefloodType, LanguageManager::m_Ptr->m_sTexts[LAN_WAS_KICKED]);

			pUser->Close();
            return;
        }
        case 5: {
			BanManager::m_Ptr->TempBan(pUser, DeFloodGetMessage(ui8DefloodType, 1), NULL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFLOOD_TEMP_BAN_TIME], 0, false);

            pUser->SendFormatCheckPM("DeFloodDoAction4", sOtherNick, false, "<%s> %s: %s %s: %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_HAD_BEEN_TEMP_BANNED_TO], formatTime(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFLOOD_TEMP_BAN_TIME]), 
				LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], DeFloodGetMessage(ui8DefloodType, 1));

            DeFloodReport(pUser, ui8DefloodType, LanguageManager::m_Ptr->m_sTexts[LAN_WAS_TEMPORARY_BANNED]);

			pUser->Close();
            return;
        }
        case 6: {
			BanManager::m_Ptr->Ban(pUser, DeFloodGetMessage(ui8DefloodType, 1), NULL, false);

            pUser->SendFormatCheckPM("DeFloodDoAction5", sOtherNick, false, "<%s> %s: %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_BEING_BANNED_BECAUSE], DeFloodGetMessage(ui8DefloodType, 1));

            DeFloodReport(pUser, ui8DefloodType, LanguageManager::m_Ptr->m_sTexts[LAN_WAS_BANNED]);

			pUser->Close();
            return;
        }
    }
}
//---------------------------------------------------------------------------

bool DeFloodCheckForWarn(User * pUser, const uint8_t ui8DefloodType, char * sOtherNick) {
	if(pUser->m_ui32DefloodWarnings < (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFLOOD_WARNING_COUNT]) {
        pUser->SendFormat("DeFloodCheckForWarn", true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], DeFloodGetMessage(ui8DefloodType, 0));
        return false;
    } else {
        switch(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFLOOD_WARNING_ACTION]) {
			case 0: {
                pUser->SendFormatCheckPM("DeFloodCheckForWarn1", sOtherNick, false, "<%s> %s: %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_BEING_DISCONNECTED_BECAUSE], DeFloodGetMessage(ui8DefloodType, 1));

                DeFloodReport(pUser, ui8DefloodType, LanguageManager::m_Ptr->m_sTexts[LAN_WAS_DISCONNECTED]);

				break;
			}
			case 1: {
				BanManager::m_Ptr->TempBan(pUser, DeFloodGetMessage(ui8DefloodType, 1), NULL, 0, 0, false);

                pUser->SendFormatCheckPM("DeFloodCheckForWarn2", sOtherNick, false, "<%s> %s: %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_BEING_KICKED_BCS], DeFloodGetMessage(ui8DefloodType, 1));

                DeFloodReport(pUser, ui8DefloodType, LanguageManager::m_Ptr->m_sTexts[LAN_WAS_KICKED]);

				break;
			}
			case 2: {
				BanManager::m_Ptr->TempBan(pUser, DeFloodGetMessage(ui8DefloodType, 1), NULL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFLOOD_TEMP_BAN_TIME], 0, false);

                pUser->SendFormatCheckPM("DeFloodCheckForWarn3", sOtherNick, false, "<%s> %s: %s %s: %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_HAD_BEEN_TEMP_BANNED_TO], formatTime(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFLOOD_TEMP_BAN_TIME]), 
					LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], DeFloodGetMessage(ui8DefloodType, 1));

                DeFloodReport(pUser, ui8DefloodType, LanguageManager::m_Ptr->m_sTexts[LAN_WAS_TEMPORARY_BANNED]);

				break;
			}
            case 3: {
				BanManager::m_Ptr->Ban(pUser, DeFloodGetMessage(ui8DefloodType, 1), NULL, false);

                pUser->SendFormatCheckPM("DeFloodCheckForWarn4", sOtherNick, false, "<%s> %s: %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_BEING_BANNED_BECAUSE], DeFloodGetMessage(ui8DefloodType, 1));
                
                DeFloodReport(pUser, ui8DefloodType, LanguageManager::m_Ptr->m_sTexts[LAN_WAS_BANNED]);

                break;
            }
        }

        pUser->Close();
        return true;
    }
}
//---------------------------------------------------------------------------

const char * DeFloodGetMessage(const uint8_t ui8DefloodType, const uint8_t ui8MsgId) {
    switch(ui8DefloodType) {
        case DEFLOOD_GETNICKLIST:
            switch(ui8MsgId) {
                case 0:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_PLS_DONT_FLOOD_WITH_GetNickList];
                case 1:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_GetNickList_FLOODING];
                case 2:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_GetNickList_FLOODER];
            }
        case DEFLOOD_MYINFO:
            switch(ui8MsgId) {
                case 0:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_PLS_DONT_FLOOD_WITH_MyINFO];
                case 1:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_MyINFO_FLOODING];
                case 2:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_MyINFO_FLOODER];
            }
        case DEFLOOD_SEARCH:
            switch(ui8MsgId) {
                case 0:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_PLS_DONT_FLOOD_WITH_SEARCHES];
                case 1:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_SEARCH_FLOODING];
                case 2:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_SEARCH_FLOODER];
            }
        case DEFLOOD_CHAT:
            switch(ui8MsgId) {
                case 0:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_PLS_DONT_FLOOD_CHAT];
                case 1:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_CHAT_FLOODING];
                case 2:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_CHAT_FLOODER];
            }
        case DEFLOOD_PM:
            switch(ui8MsgId) {
                case 0:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_PLS_DONT_FLOOD_WITH_PM];
                case 1:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_PM_FLOODING];
                case 2:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_PM_FLOODER];
            }
        case DEFLOOD_SAME_SEARCH:
            switch(ui8MsgId) {
                case 0:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_PLS_DONT_FLOOD_WITH_SAME_SEARCHES];
                case 1:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_SAME_SEARCH_FLOODING];
                case 2:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_SAME_SEARCH_FLOODER];
            }
        case DEFLOOD_SAME_PM:
            switch(ui8MsgId) {
                case 0:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_PLS_DONT_FLOOD_WITH_SAME_PM];
                case 1:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_SAME_PM_FLOODING];
                case 2:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_SAME_PM_FLOODER];
            }
        case DEFLOOD_SAME_CHAT:
            switch(ui8MsgId) {
                case 0:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_PLS_DONT_FLOOD_SAME_CHAT];
                case 1:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_SAME_CHAT_FLOODING];
                case 2:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_SAME_CHAT_FLOODER];
            }
        case DEFLOOD_SAME_MULTI_PM:
            switch(ui8MsgId) {
                case 0:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_PLS_DONT_FLOOD_WITH_SAME_MULTI_PM];
                case 1:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_SAME_MULTI_PM_FLOODING];
                case 2:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_SAME_MULTI_PM_FLOODER];
            }
        case DEFLOOD_SAME_MULTI_CHAT:
            switch(ui8MsgId) {
                case 0:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_PLS_DONT_FLOOD_SAME_MULTI_CHAT];
                case 1:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_SAME_MULTI_CHAT_FLOODING];
                case 2:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_SAME_MULTI_CHAT_FLOODER];
            }
        case DEFLOOD_CTM:
            switch(ui8MsgId) {
                case 0:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_PLS_DONT_FLOOD_WITH_CTM];
                case 1:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_CTM_FLOODING];
                case 2:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_CTM_FLOODER];
            }
        case DEFLOOD_RCTM:
            switch(ui8MsgId) {
                case 0:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_PLS_DONT_FLOOD_WITH_RCTM];
                case 1:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_RCTM_FLOODING];
                case 2:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_RCTM_FLOODER];
            }
        case DEFLOOD_SR:
            switch(ui8MsgId) {
                case 0:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_PLS_DONT_FLOOD_WITH_SR];
                case 1:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_SR_FLOODING];
                case 2:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_SR_FLOODER];
            }
        case DEFLOOD_MAX_DOWN:
            switch(ui8MsgId) {
                case 0:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_PLS_DONT_FLOOD_WITH_DATA];
                case 1:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_DATA_FLOODING];
                case 2:
                    return LanguageManager::m_Ptr->m_sTexts[LAN_DATA_FLOODER];
            }
        case INTERVAL_CHAT:
            return LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_BEFORE_NEXT_CHAT_MSG];
        case INTERVAL_PM:
            return LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_BEFORE_NEXT_PM];
        case INTERVAL_SEARCH:
            return LanguageManager::m_Ptr->m_sTexts[LAN_SECONDS_BEFORE_NEXT_SEARCH];
	}
	return "";
}
//---------------------------------------------------------------------------

void DeFloodReport(User * pUser, const uint8_t ui8DefloodType, char *sAction) {
    if(SettingManager::m_Ptr->m_bBools[SETBOOL_DEFLOOD_REPORT] == true) {
        GlobalDataQueue::m_Ptr->StatusMessageFormat("DeFloodReport", "<%s> *** %s %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], DeFloodGetMessage(ui8DefloodType, 2), pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], pUser->m_sIP, sAction);
    }

	UdpDebug::m_Ptr->BroadcastFormat("[SYS] Flood type %hu from %s (%s) - user closed.", (uint16_t)ui8DefloodType, pUser->m_sNick, pUser->m_sIP);
}
//---------------------------------------------------------------------------

bool DeFloodCheckInterval(User * pUser, const uint8_t ui8DefloodType, uint16_t &ui16Count, uint64_t &ui64LastOkTick, const int16_t ui16DefloodCount, const uint32_t ui32DefloodTime, char * sOtherNick/* = NULL*/) {
    if(ui16Count == 0) {
		ui64LastOkTick = ServerManager::m_ui64ActualTick;
    } else if(ui16Count >= ui16DefloodCount) {
		if((ui64LastOkTick+ui32DefloodTime) > ServerManager::m_ui64ActualTick) {
            ui16Count++;

            pUser->SendFormatCheckPM("DeFloodCheckInterval", sOtherNick, true, "<%s> %s %" PRIu64 " %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_PLEASE_WAIT], (ui64LastOkTick+ui32DefloodTime)-ServerManager::m_ui64ActualTick, DeFloodGetMessage(ui8DefloodType, 0));

            return true;
        } else {
            ui64LastOkTick = ServerManager::m_ui64ActualTick;
			ui16Count = 0;
        }
    } else if((ui64LastOkTick+ui32DefloodTime) <= ServerManager::m_ui64ActualTick) {
        ui64LastOkTick = ServerManager::m_ui64ActualTick;
        ui16Count = 0;
    }

    ui16Count++;
    return false;
}
//---------------------------------------------------------------------------
