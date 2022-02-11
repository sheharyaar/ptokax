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
#ifndef HubCommandsH
#define HubCommandsH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

struct ChatCommand {
	User * m_pUser;

	char * m_sCommand;

	uint32_t m_ui32CommandLen;

	bool m_bFromPM;
};

class HubCommands {
private:
	static ChatCommand m_ChatCommand;

	static bool AddRegUser(ChatCommand * pChatCommand);
	static bool Ban(ChatCommand * pChatCommand);
	static bool BanIp(ChatCommand * pChatCommand);
	static bool ClrTempBans(ChatCommand * pChatCommand);
	static bool ClrPermBans(ChatCommand * pChatCommand);
	static bool ClrRangeTempBans(ChatCommand * pChatCommand);
	static bool ClrRangePermBans(ChatCommand * pChatCommand);
	static bool CheckNickBan(ChatCommand * pChatCommand);
	static bool CheckIpBan(ChatCommand * pChatCommand);
	static bool CheckRangeBan(ChatCommand * pChatCommand);
	static bool Drop(ChatCommand * pChatCommand);
	static bool DelRegUser(ChatCommand * pChatCommand);
	static bool Debug(ChatCommand * pChatCommand);

	static bool FullBan(ChatCommand * pChatCommand);
	static bool FullBanIp(ChatCommand * pChatCommand);
	static bool FullTempBan(ChatCommand * pChatCommand);
	static bool FullTempBanIp(ChatCommand * pChatCommand);
	static bool FullRangeBan(ChatCommand * pChatCommand);
	static bool FullRangeTempBan(ChatCommand * pChatCommand);
	static bool GetBans(ChatCommand * pChatCommand);
	static bool Gag(ChatCommand * pChatCommand);
	static bool GetInfo(ChatCommand * pChatCommand);
	static bool GetIpInfo(ChatCommand * pChatCommand);
	static bool GetTempBans(ChatCommand * pChatCommand);
	static bool GetScripts(ChatCommand * pChatCommand);
	static bool GetPermBans(ChatCommand * pChatCommand);
	static bool GetRangeBans(ChatCommand * pChatCommand);
	static bool GetRangePermBans(ChatCommand * pChatCommand);
	static bool GetRangeTempBans(ChatCommand * pChatCommand);
	static bool Help(ChatCommand * pChatCommand);

	static bool MyIp(ChatCommand * pChatCommand);
	static bool MassMsg(ChatCommand * pChatCommand);
	static bool NickBan(ChatCommand * pChatCommand);
	static bool NickTempBan(ChatCommand * pChatCommand);
	static bool Op(ChatCommand * pChatCommand);
	static bool OpMassMsg(ChatCommand * pChatCommand);
	static bool Passwd(ChatCommand * pChatCommand);
	static bool PermUnban(ChatCommand * pChatCommand);

	static bool RestartScripts(ChatCommand * pChatCommand);
	static bool Restart(ChatCommand * pChatCommand);
	static bool ReloadTxt(ChatCommand * pChatCommand);
	static bool RestartScript(ChatCommand * pChatCommand);
	static bool RangeBan(ChatCommand * pChatCommand);
	static bool RangeTempBan(ChatCommand * pChatCommand);
	static bool RangeUnBan(ChatCommand * pChatCommand);
	static bool RangeTempUnBan(ChatCommand * pChatCommand);
	static bool RangePermUnBan(ChatCommand * pChatCommand);
	static bool RegNewUser(ChatCommand * pChatCommand);
	static bool Stats(ChatCommand * pChatCommand);
	static bool StopScript(ChatCommand * pChatCommand);
	static bool StartScript(ChatCommand * pChatCommand);
	static bool TempBan(ChatCommand * pChatCommand);
	static bool TempBanIp(ChatCommand * pChatCommand);
	static bool TempUnban(ChatCommand * pChatCommand);
	static bool Topic(ChatCommand * pChatCommand);
	static bool Unban(ChatCommand * pChatCommand);
	static bool Ungag(ChatCommand * pChatCommand);

    static bool Ban(ChatCommand * pChatCommand, const bool bFull);
    static bool BanIp(ChatCommand * pChatCommand, const bool bFull);
    static bool NickBan(ChatCommand * pChatCommand, char * sReason);
    static bool TempBan(ChatCommand * pChatCommand, const bool bFull);
    static bool TempBanIp(ChatCommand * pChatCommand, const bool bFull);
    static bool TempNickBan(ChatCommand * pChatCommand, char * sNick, char * sTime, const uint16_t ui16TimeLen, char * sReason, const bool bNotNickBan = false);
    static bool RangeBan(ChatCommand * pChatCommand, const bool bFull);
    static bool RangeTempBan(ChatCommand * pChatCommand, const bool bFull);
    static bool RangeUnban(ChatCommand * pChatCommand);
    static bool RangeUnban(ChatCommand * pChatCommand, const uint8_t ui8Type);

    static void SendNoPermission(ChatCommand * pChatCommand);
    static int CheckFromPm(ChatCommand * pChatCommand);
    static void UncountDeflood(ChatCommand * pChatCommand);
public:
    static bool DoCommand(User * pUser, char * sCommand, const uint32_t ui32CmdLen, bool bFromPM = false);
};
//---------------------------------------------------------------------------

#endif
