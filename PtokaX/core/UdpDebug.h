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
#ifndef UdpDebugH
#define UdpDebugH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

class UdpDebug {
private:
	char * m_sDebugBuffer, * m_sDebugHead;

    struct UdpDbgItem {
    	sockaddr_storage m_sasTo;

		UdpDbgItem * m_pPrev, * m_pNext;

        char * m_sNick;
#ifdef _WIN32
        SOCKET m_Socket;
#else
		int m_Socket;
#endif

        int m_sasLen;

        uint32_t m_ui32Hash;

        bool m_bIsScript, m_bAllData;

        UdpDbgItem();
        ~UdpDbgItem();

        UdpDbgItem(const UdpDbgItem&);
        const UdpDbgItem& operator=(const UdpDbgItem&);
    };

    UdpDebug(const UdpDebug&);
    const UdpDebug& operator=(const UdpDebug&);

	void CreateBuffer();
	void DeleteBuffer();
public:
    static UdpDebug * m_Ptr;

    UdpDbgItem * m_pDbgItemList;

	UdpDebug();
	~UdpDebug();

	void Broadcast(const char * sMsg, const size_t szMsgLen) const;
    void BroadcastFormat(const char * sFormatMsg, ...) const;
	bool New(User * pUser, const uint16_t ui16Port);
	bool New(char * sIP, const uint16_t ui16Port, const bool bAllData, char * sScriptName);
	bool Remove(User * pUser);
	void Remove(char * sScriptName);
	bool CheckUdpSub(User * pUser, const bool bSendMsg = false) const;
	void Send(const char * sScriptName, char * sMessage, const size_t szMsgLen) const;
	void Cleanup();
	void UpdateHubName();
};
//---------------------------------------------------------------------------

#endif
