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
#ifndef DeFloodH
#define DeFloodH
//---------------------------------------------------------------------------

enum DefloodTypes {
    DEFLOOD_GETNICKLIST, 
    DEFLOOD_MYINFO, 
    DEFLOOD_SEARCH, 
    DEFLOOD_CHAT, 
    DEFLOOD_PM, 
    DEFLOOD_SAME_SEARCH, 
    DEFLOOD_SAME_PM,
    DEFLOOD_SAME_CHAT, 
    DEFLOOD_SAME_MULTI_PM, 
    DEFLOOD_SAME_MULTI_CHAT, 
    DEFLOOD_CTM, 
    DEFLOOD_RCTM, 
    DEFLOOD_SR, 
    DEFLOOD_MAX_DOWN, 
    INTERVAL_CHAT, 
    INTERVAL_PM, 
    INTERVAL_SEARCH
};
//---------------------------------------------------------------------------

bool DeFloodCheckForFlood(User * pUser, const uint8_t ui8DefloodType, const int16_t ui16Action, uint16_t &ui16Count, uint64_t &ui64LastOkTick, const int16_t ui16DefloodCount, const uint32_t ui32DefloodTime, char * sOtherNick = NULL);
bool DeFloodCheckForSameFlood(User * pUser, const uint8_t ui8DefloodType, const int16_t ui16Action, uint16_t &ui16Count, const uint64_t ui64LastOkTick, const int16_t ui16DefloodCount, const uint32_t ui32DefloodTime, char * sNewData, const size_t ui32NewDataLen, char * sOldData, const uint16_t ui16OldDataLen, 
	bool &bNewData, char * sOtherNick = NULL);
bool DeFloodCheckForDataFlood(User * pUser, const uint8_t ui8DefloodType, const int16_t ui16Action, uint32_t &ui32Count, uint64_t &ui64LastOkTick, const int16_t ui16DefloodCount, const uint32_t ui32DefloodTime);
void DeFloodDoAction(User * pUser, const uint8_t ui8DefloodType, const int16_t ui16Action, uint16_t &ui16Count, char * sOtherNick);
bool DeFloodCheckForWarn(User * pUser, const uint8_t ui8DefloodType, char * sOtherNick);
const char * DeFloodGetMessage(const uint8_t ui8DefloodType, const uint8_t ui8MsgId);
void DeFloodReport(User * pUser, const uint8_t ui8DefloodType, char * sAction);
bool DeFloodCheckInterval(User * pUser, const uint8_t ui8DefloodType, uint16_t &ui16Count, uint64_t &ui64LastOkTick, const int16_t ui16DefloodCount, const uint32_t ui32DefloodTime, char * sOtherNick = NULL);
//---------------------------------------------------------------------------

#endif
