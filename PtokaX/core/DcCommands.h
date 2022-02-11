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
#ifndef DcCommandsH
#define DcCommandsH
//---------------------------------------------------------------------------
struct User;
struct PrcsdUsrCmd;
struct PassBf;
struct DcCommand;
//---------------------------------------------------------------------------

class DcCommands {
private:
    struct PassBf {
    	PassBf * m_pPrev, * m_pNext;

        int m_iCount;

        uint8_t m_ui128IpHash[16];

        explicit PassBf(const uint8_t * ui128Hash);
        ~PassBf(void) { };

        PassBf(const PassBf&);
        const PassBf& operator=(const PassBf&);
    };

    PassBf * m_pPasswdBfCheck;

    DcCommands(const DcCommands&);
    const DcCommands& operator=(const DcCommands&);

	static void BotINFO(DcCommand * pDcCommand);
    static void ConnectToMe(DcCommand * pDcCommand, const bool bMulti);
	void GetINFO(DcCommand * pDcCommand);
    static bool GetNickList(DcCommand * pDcCommand);
	static void Key(DcCommand * pDcCommand);
	static void Kick(DcCommand * pDcCommand);
    static bool SearchDeflood(DcCommand * pDcCommand, const bool bMulti);
    static void Search(DcCommand * pDcCommand, const bool bMulti);
    static bool MyINFODeflood(DcCommand * pDcCommand);
	static bool MyINFO(DcCommand * pDcCommand);
	void MyPass(DcCommand * pDcCommand);
	static void OpForceMove(DcCommand * pDcCommand);
	static void RevConnectToMe(DcCommand * pDcCommand);
	static void SR(DcCommand * pDcCommand);
	void Supports(DcCommand * pDcCommand);
    static void To(DcCommand * pDcCommand);
	static void ValidateNick(DcCommand * pDcCommand);
	static void Version(DcCommand * pDcCommand);
    static bool ChatDeflood(DcCommand * pDcCommand);
	static void Chat(DcCommand * pDcCommand);
	static void Close(DcCommand * pDcCommand);
    
    void Unknown(DcCommand * pDcCommand, const bool bMyNick = false);
    void MyNick(DcCommand * pDcCommand);
    
    static bool ValidateUserNick(User * pUser, char * sNick, const size_t szNickLen, const bool ValidateNick);

	PassBf * Find(const uint8_t * ui128IpHash);
	void Remove(PassBf * pPassBfItem);

	static uint16_t CheckAndGetPort(char * pPort, const uint8_t ui8PortLen, uint32_t &ui32PortLen);
	static void SendIPFixedMsg(User * pUser, char * pBadIP, char * pRealIP);

    static PrcsdUsrCmd * AddSearch(User * pUser, PrcsdUsrCmd * pCmdSearch, char * sSearch, const size_t szLen, const bool bActive);
public:
	static DcCommands * m_Ptr;

	uint32_t m_ui32StatChat, m_ui32StatCmdUnknown, m_ui32StatCmdTo, m_ui32StatCmdMyInfo;
	uint32_t m_ui32StatCmdSearch, m_ui32StatCmdSR, m_ui32StatCmdRevCTM, m_ui32StatCmdOpForceMove;
	uint32_t m_ui32StatCmdMyPass, m_ui32StatCmdValidate, m_ui32StatCmdKey, m_ui32StatCmdGetInfo;
	uint32_t m_ui32StatCmdGetNickList, m_ui32StatCmdConnectToMe, m_ui32StatCmdVersion, m_ui32StatCmdKick;
	uint32_t m_ui32StatCmdSupports, m_ui32StatBotINFO, m_ui32StatZPipe, m_ui32StatCmdMultiSearch;
	uint32_t m_ui32StatCmdMultiConnectToMe, m_ui32StatCmdClose;

	DcCommands();
    ~DcCommands();

    void PreProcessData(DcCommand * pDcCommand);
    void ProcessCmds(User * pUser);

    static void SRFromUDP(DcCommand * pDcCommand);
};
//---------------------------------------------------------------------------

#endif
