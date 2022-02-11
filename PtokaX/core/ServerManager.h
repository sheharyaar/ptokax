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
#ifndef ServerManagerH
#define ServerManagerH
//---------------------------------------------------------------------------
#ifdef _WIN32
	#define WM_PX_DO_LOOP (WM_USER+1)
#endif
//---------------------------------------------------------------------------
class ServerThread;
//---------------------------------------------------------------------------

class ServerManager {
public:
    static string m_sPath, m_sScriptPath;

#ifdef _WIN32
	static string m_sLuaPath, m_sOS;
#endif

    static double m_dCpuUsages[60], m_dCpuUsage;

    static uint64_t m_ui64ActualTick, m_ui64TotalShare;
    static uint64_t m_ui64BytesRead, m_ui64BytesSent, m_ui64BytesSentSaved;
    static uint64_t m_ui64LastBytesRead, m_ui64LastBytesSent;
    static uint64_t m_ui64Mins, m_ui64Hours, m_ui64Days;

#ifdef _WIN32
	static HANDLE m_hConsole, m_hLuaHeap, m_hPtokaXHeap, m_hRecvHeap, m_hSendHeap;
	#ifdef _BUILD_GUI
		static HWND m_hMainWindow;
	#endif
#endif

#ifdef __MACH__
	static clock_serv_t m_csMachClock;
#endif

	static time_t m_tStartTime;

#if defined(_WIN32) && !defined(_WIN_IOT)
    static UINT_PTR m_upSecTimer;
    static UINT_PTR m_upRegTimer;
	#ifdef _BUILD_GUI
        static HINSTANCE m_hInstance;
        static HWND m_hWndActiveDialog;
	#endif
#endif

	static ServerThread * m_pServersS;

    static char * m_pGlobalBuffer;

	static size_t m_szGlobalBufferSize;

#ifndef _WIN32
    static uint32_t m_ui32CpuCount;
#endif

    static uint32_t m_ui32Joins, m_ui32Parts, m_ui32Logged, m_ui32Peak;
    static uint32_t m_ui32UploadSpeed[60], m_ui32DownloadSpeed[60];
    static uint32_t m_ui32ActualBytesRead, m_ui32ActualBytesSent;
    static uint32_t m_ui32AverageBytesRead, m_ui32AverageBytesSent;

    static bool m_bServerRunning, m_bServerTerminated, m_bIsRestart, m_bIsClose;

#ifdef _WIN32
	#ifndef _BUILD_GUI
        static bool m_bService;
	#endif
#else
    static bool m_bDaemon;
#endif

    static bool m_bCmdAutoStart, m_bCmdNoAutoStart, m_bCmdNoTray, m_bUseIPv4, m_bUseIPv6, m_bIPv6DualStack;

    static char m_sHubIP[16], m_sHubIP6[40];

    static uint8_t m_ui8SrCntr, m_ui8MinTick;

	static void OnSecTimer();
    static void OnRegTimer();

    static void Initialize();

    static void FinalStop(const bool bDeleteServiceLoop);
    static void ResumeAccepts();
    static void SuspendAccepts(const uint32_t ui32Time);
    static void UpdateAutoRegState();

    static bool Start();
    static void UpdateServers();
    static void Stop();
    static void FinalClose();
    static void CreateServerThread(const int iAddrFamily, const uint16_t ui16PortNumber, const bool bResume = false);

	static void CommandLineSetup();

	static bool ResolveHubAddress(const bool bSilent = false);
};
//--------------------------------------------------------------------------- 

#endif
