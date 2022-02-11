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
#ifndef ServerThreadH
#define ServerThreadH
//---------------------------------------------------------------------------

class ServerThread {
private:
    struct AntiConFlood {
        uint64_t m_ui64Time;

        AntiConFlood * m_pPrev, * m_pNext;

        int16_t m_ui16Hits;

        uint8_t m_ui128IpHash[16];

        explicit AntiConFlood(const uint8_t * pIpHash);

        AntiConFlood(const AntiConFlood&);
        const AntiConFlood& operator=(const AntiConFlood&);
    };

	AntiConFlood * m_pAntiFloodList;

#ifdef _WIN32
	HANDLE m_hThreadHandle;

	CRITICAL_SECTION m_csServerThread;

    SOCKET m_Server;
#else
	pthread_t m_ThreadId;

	pthread_mutex_t m_mtxServerThread;

    int m_Server;
#endif
	uint32_t m_ui32SuspendTime;

    int m_iAdressFamily;

	bool m_bTerminated;

	ServerThread(const ServerThread&);
	const ServerThread& operator=(const ServerThread&);
public:
    ServerThread * m_pPrev, * m_pNext;

    uint16_t m_ui16Port;

    bool m_bActive, m_bSuspended;

	ServerThread(const int iAddrFamily, const uint16_t ui16PortNumber);
	~ServerThread();

	void Resume();
	void Run();
	void Close();
	void WaitFor();
	bool Listen(const bool bSilent = false);
#ifdef _WIN32
	bool isFlooder(const SOCKET s, const sockaddr_storage &addr);
#else
	bool isFlooder(const int s, const sockaddr_storage &addr);
#endif
	void RemoveConFlood(AntiConFlood * pACF);
	void ResumeSck();
	void SuspendSck(const uint32_t ui32Time);
};
//---------------------------------------------------------------------------

#endif
