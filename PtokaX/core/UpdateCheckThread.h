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
#ifndef UpdateCheckThreadH
#define UpdateCheckThreadH
//---------------------------------------------------------------------------

class UpdateCheckThread {
private:
	HANDLE m_hThread;

	char * m_sRecvBuf;

    SOCKET m_Socket;

	uint32_t m_ui32FileLen;

    uint32_t m_ui32RecvBufLen, m_ui32RecvBufSize;
    uint32_t m_ui32BytesRead, m_ui32BytesSent;
    
    bool m_bOk, m_bData, m_bTerminated;

	char m_sMsg[2048];

    static void Message(char * sMessage, const size_t szLen);
    bool Receive();
    bool SendHeader();

    UpdateCheckThread(const UpdateCheckThread&);
    const UpdateCheckThread& operator=(const UpdateCheckThread&);
public:
    static UpdateCheckThread * m_Ptr;

	UpdateCheckThread();
	~UpdateCheckThread();

    void Resume();
    void Run();
	void Close();
	void WaitFor();
};
//---------------------------------------------------------------------------

#endif
