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
#include "ServerManager.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "DcCommands.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashUsrManager.h"
#include "hashRegManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "serviceLoop.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "utility.h"
#include "ZlibUtility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "HubCommands.h"
#include "IP2Country.h"
#include "LuaScript.h"
#include "RegThread.h"
#include "ResNickManager.h"
#include "ServerThread.h"
#include "TextConverter.h"
#include "TextFileManager.h"
//#include "TLSManager.h"
#include "UDPThread.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
    #include "../gui.win/MainWindow.h"
    #include "../gui.win/MainWindowPageScripts.h"
#endif
//---------------------------------------------------------------------------
#ifdef _WITH_SQLITE
	#include "DB-SQLite.h"
#elif _WITH_POSTGRES
	#include "DB-PostgreSQL.h"
#elif _WITH_MYSQL
	#include "DB-MySQL.h"
#endif
//---------------------------------------------------------------------------
static ServerThread * m_pServersE = NULL;
//---------------------------------------------------------------------------

#ifdef _WIN32
	#ifndef _WIN_IOT
	    UINT_PTR ServerManager::m_upSecTimer = 0;
	    UINT_PTR ServerManager::m_upRegTimer = 0;
    #endif

	HANDLE ServerManager::m_hConsole = NULL, ServerManager::m_hLuaHeap = NULL, ServerManager::m_hPtokaXHeap = NULL, ServerManager::m_hRecvHeap = NULL, ServerManager::m_hSendHeap = NULL;
	#ifdef _BUILD_GUI
		HWND ServerManager::m_hMainWindow = NULL;
	#endif
	string ServerManager::m_sLuaPath, ServerManager::m_sOS;
#endif

#ifdef __MACH__
	clock_serv_t ServerManager::m_csMachClock;
#endif

string ServerManager::m_sPath, ServerManager::m_sScriptPath;
size_t ServerManager::m_szGlobalBufferSize = 0;
char * ServerManager::m_pGlobalBuffer = NULL;
bool ServerManager::m_bCmdAutoStart = false, ServerManager::m_bCmdNoAutoStart = false, ServerManager::m_bCmdNoTray = false, ServerManager::m_bUseIPv4 = true,
    ServerManager::m_bUseIPv6 = true, ServerManager::m_bIPv6DualStack = false;

double ServerManager::m_dCpuUsages[60];
double ServerManager::m_dCpuUsage = 0;

uint64_t ServerManager::m_ui64ActualTick = 0, ServerManager::m_ui64TotalShare = 0;
uint64_t ServerManager::m_ui64BytesRead = 0, ServerManager::m_ui64BytesSent = 0, ServerManager::m_ui64BytesSentSaved = 0;
uint64_t ServerManager::m_ui64LastBytesRead = 0, ServerManager::m_ui64LastBytesSent = 0;
uint64_t ServerManager::m_ui64Mins = 0, ServerManager::m_ui64Hours = 0, ServerManager::m_ui64Days = 0;

#ifndef _WIN32
	uint32_t ServerManager::m_ui32CpuCount = 0;
#endif

uint32_t ServerManager::m_ui32UploadSpeed[60], ServerManager::m_ui32DownloadSpeed[60];
uint32_t ServerManager::m_ui32Joins = 0, ServerManager::m_ui32Parts = 0, ServerManager::m_ui32Logged = 0, ServerManager::m_ui32Peak = 0;
uint32_t ServerManager::m_ui32ActualBytesRead = 0, ServerManager::m_ui32ActualBytesSent = 0;
uint32_t ServerManager::m_ui32AverageBytesRead = 0, ServerManager::m_ui32AverageBytesSent = 0;

ServerThread * ServerManager::m_pServersS = NULL;

time_t ServerManager::m_tStartTime = 0;

bool ServerManager::m_bServerRunning = false, ServerManager::m_bServerTerminated = false, ServerManager::m_bIsRestart = false, ServerManager::m_bIsClose = false;

#ifdef _WIN32
	#ifndef _BUILD_GUI
	    bool ServerManager::m_bService = false;
	#else
        HINSTANCE ServerManager::m_hInstance;
        HWND ServerManager::m_hWndActiveDialog;
	#endif
#else
	bool ServerManager::m_bDaemon = false;
#endif

char ServerManager::m_sHubIP[16], ServerManager::m_sHubIP6[40];

uint8_t ServerManager::m_ui8SrCntr = 0, ServerManager::m_ui8MinTick = 0;
//---------------------------------------------------------------------------

void ServerManager::OnSecTimer() {
#ifdef _WIN32
	FILETIME tmpa, tmpb, kernelTimeFT, userTimeFT;
	GetProcessTimes(GetCurrentProcess(), &tmpa, &tmpb, &kernelTimeFT, &userTimeFT);
	int64_t kernelTime = kernelTimeFT.dwLowDateTime | (((int64_t)kernelTimeFT.dwHighDateTime) << 32);
	int64_t userTime = userTimeFT.dwLowDateTime | (((int64_t)userTimeFT.dwHighDateTime) << 32);
	double dcpuSec = double(kernelTime + userTime) / double(10000000I64);
	m_dCpuUsage = dcpuSec - m_dCpuUsages[m_ui8MinTick];
	m_dCpuUsages[m_ui8MinTick] = dcpuSec;
#else
    struct rusage rs;

    getrusage(RUSAGE_SELF, &rs);

	double dcpuSec = double(rs.ru_utime.tv_sec) + (double(rs.ru_utime.tv_usec)/1000000) + 
	double(rs.ru_stime.tv_sec) + (double(rs.ru_stime.tv_usec)/1000000);
	m_dCpuUsage = dcpuSec - m_dCpuUsages[m_ui8MinTick];
	m_dCpuUsages[m_ui8MinTick] = dcpuSec;
#endif

	if(++m_ui8MinTick == 60) {
		m_ui8MinTick = 0;
    }

#ifdef _WIN32
	if(m_bServerRunning == false) {
		return;
	}
#endif

	m_ui64ActualTick++;

	m_ui32ActualBytesRead = (uint32_t)(m_ui64BytesRead - m_ui64LastBytesRead);
	m_ui32ActualBytesSent = (uint32_t)(m_ui64BytesSent - m_ui64LastBytesSent);
	m_ui64LastBytesRead = m_ui64BytesRead;
	m_ui64LastBytesSent = m_ui64BytesSent;

	m_ui32AverageBytesSent -= m_ui32UploadSpeed[m_ui8MinTick];
	m_ui32AverageBytesRead -= m_ui32DownloadSpeed[m_ui8MinTick];

	m_ui32UploadSpeed[m_ui8MinTick] = m_ui32ActualBytesSent;
	m_ui32DownloadSpeed[m_ui8MinTick] = m_ui32ActualBytesRead;

	m_ui32AverageBytesSent += m_ui32UploadSpeed[m_ui8MinTick];
	m_ui32AverageBytesRead += m_ui32DownloadSpeed[m_ui8MinTick];

#ifdef _BUILD_GUI
    MainWindow::m_Ptr->UpdateStats();
    MainWindowPageScripts::m_Ptr->UpdateMemUsage();
#endif
}
//---------------------------------------------------------------------------

void ServerManager::OnRegTimer() {
	if(SettingManager::m_Ptr->m_bBools[SETBOOL_AUTO_REG] == true && SettingManager::m_Ptr->m_sTexts[SETTXT_REGISTER_SERVERS] != NULL) {
		// First destroy old hublist reg thread if any
	    if(RegisterThread::m_Ptr != NULL) {
	        RegisterThread::m_Ptr->Close();
	        RegisterThread::m_Ptr->WaitFor();
	        delete RegisterThread::m_Ptr;
	        RegisterThread::m_Ptr = NULL;
	    }
	        
	    // Create hublist reg thread
	    RegisterThread::m_Ptr = new (std::nothrow) RegisterThread();
	    if(RegisterThread::m_Ptr == NULL) {
	        AppendDebugLog("%s - [MEM] Cannot allocate RegisterThread::m_Ptr in ServerOnRegTimer\n");
	        return;
	    }
	        
	    // Setup hublist reg thread
	    RegisterThread::m_Ptr->Setup(SettingManager::m_Ptr->m_sTexts[SETTXT_REGISTER_SERVERS], SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_REGISTER_SERVERS]);
	        
	    // Start the hublist reg thread
	    RegisterThread::m_Ptr->Resume();
	}
}
//---------------------------------------------------------------------------

void ServerManager::Initialize() {
    setlocale(LC_ALL, "");

	time_t acctime;
	time(&acctime);
#ifdef _WIN32
	srand((uint32_t)acctime);

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	m_hPtokaXHeap = HeapCreate(HEAP_NO_SERIALIZE, 0x100000, 0);

	if(DirExist((m_sPath+"\\cfg").c_str()) == false) {
		CreateDirectory((m_sPath+"\\cfg").c_str(), NULL);
	}
	if(DirExist((m_sPath+"\\logs").c_str()) == false) {
		CreateDirectory((m_sPath+"\\logs").c_str(), NULL);
	}
	if(DirExist((m_sPath+"\\scripts").c_str()) == false) {
		CreateDirectory((m_sPath+"\\scripts").c_str(), NULL);
    }
	if(DirExist((m_sPath+"\\texts").c_str()) == false) {
		CreateDirectory((m_sPath+"\\texts").c_str(), NULL);
    }

	m_sScriptPath = m_sPath + "\\scripts\\";

	m_sLuaPath = m_sPath + "/";

	char * sTempLuaPath = m_sLuaPath.c_str();
	for(size_t szi = 0; szi < m_sPath.size(); szi++) {
		if(sTempLuaPath[szi] == '\\') {
			sTempLuaPath[szi] = '/';
		}
	}

    SetupOsVersion();
#else
    srandom(acctime);

	if(DirExist((m_sPath+"/logs").c_str()) == false) {
		if(mkdir((m_sPath+"/logs").c_str(), 0755) == -1) {
            if(m_bDaemon == true) {
                syslog(LOG_USER | LOG_ERR, "Creating  of logs directory failed!\n");
            } else {
                printf("Creating  of logs directory failed!");
            }
        }
	}
	if(DirExist((m_sPath+"/cfg").c_str()) == false) {
		if(mkdir((m_sPath+"/cfg").c_str(), 0755) == -1) {
            AppendLog("Creating of cfg directory failed!");
        }
	}
	if(DirExist((m_sPath+"/scripts").c_str()) == false) {
		if(mkdir((m_sPath+"/scripts").c_str(), 0755) == -1) {
            AppendLog("Creating of scripts directory failed!");
        }
    }
	if(DirExist((m_sPath+"/texts").c_str()) == false) {
		if(mkdir((m_sPath+"/texts").c_str(), 0755) == -1) {
            AppendLog("Creating of texts directory failed!");
        }
    }

	m_sScriptPath = m_sPath + "/scripts/";

    // get cpu count
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if(fp != NULL) {
        char buf[1024];
        while(fgets(buf, 1024, fp) != NULL) {
            if(strncasecmp (buf, "model name", 10) == 0 || strncmp (buf, "Processor", 9) == 0 || strncmp (buf, "cpu model", 9) == 0) {
                m_ui32CpuCount++;
            }
        }
    
        fclose(fp);
    }

    if(m_ui32CpuCount == 0) {
        m_ui32CpuCount = 1;
    }
#endif
    CreateGlobalBuffer();

    CheckForIPv6();

#ifdef __MACH__
	mach_port_t mpMachHost = mach_host_self();
	host_get_clock_service(mpMachHost, SYSTEM_CLOCK, &csMachClock);
	mach_port_deallocate(mach_task_self(), mpMachHost);
#endif

	ReservedNicksManager::m_Ptr = new (std::nothrow) ReservedNicksManager();
	if(ReservedNicksManager::m_Ptr == NULL) {
	    AppendDebugLog("%s - [MEM] Cannot allocate ReservedNicksManager::m_Ptr in ServerInitialize\n");
	    exit(EXIT_FAILURE);
	}

	m_ui64ActualTick = m_ui64TotalShare = 0;
	m_ui64BytesRead = m_ui64BytesSent = m_ui64BytesSentSaved = 0;

	m_ui32ActualBytesRead = m_ui32ActualBytesSent = m_ui32AverageBytesRead = m_ui32AverageBytesSent = 0;

	m_ui32Joins = m_ui32Parts = m_ui32Logged = m_ui32Peak = 0;

	m_pServersS = NULL;
	m_pServersE = NULL;

	m_tStartTime = 0;

	m_ui64Mins = m_ui64Hours = m_ui64Days = 0;

	m_bServerRunning = m_bIsRestart = m_bIsClose = false;

	m_sHubIP[0] = '\0';
	m_sHubIP6[0] = '\0';

	m_ui8SrCntr = 0;

    ZlibUtility::m_Ptr = new (std::nothrow) ZlibUtility();
    if(ZlibUtility::m_Ptr == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate ZlibUtility::m_Ptr in ServerInitialize\n");
    	exit(EXIT_FAILURE);
    }

	m_ui8MinTick = 0;

	m_ui64LastBytesRead = m_ui64LastBytesSent = 0;

	for(uint8_t ui8i = 0 ; ui8i < 60; ui8i++) {
		m_dCpuUsages[ui8i] = 0;
		m_ui32UploadSpeed[ui8i] = 0;
		m_ui32DownloadSpeed[ui8i] = 0;
	}

	m_dCpuUsage = 0.0;

	SettingManager::m_Ptr = new (std::nothrow) SettingManager();
    if(SettingManager::m_Ptr == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate SettingManager::m_Ptr in ServerInitialize\n");
    	exit(EXIT_FAILURE);
    }

	TextConverter::m_Ptr = new (std::nothrow) TextConverter();
    if(TextConverter::m_Ptr == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate TextConverter::m_Ptr in ServerInitialize\n");
    	exit(EXIT_FAILURE);
    }

    LanguageManager::m_Ptr = new (std::nothrow) LanguageManager();
    if(LanguageManager::m_Ptr == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate LanguageManager::m_Ptr in ServerInitialize\n");
        exit(EXIT_FAILURE);
    }

    LanguageManager::m_Ptr->Load();

    ProfileManager::m_Ptr = new (std::nothrow) ProfileManager();
    if(ProfileManager::m_Ptr == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate ProfileManager::m_Ptr in ServerInitialize\n");
    	exit(EXIT_FAILURE);
    }

    RegManager::m_Ptr = new (std::nothrow) RegManager();
    if(RegManager::m_Ptr == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate RegManager::m_Ptr in ServerInitialize\n");
    	exit(EXIT_FAILURE);
    }

    // Load registered users
	RegManager::m_Ptr->Load();

    BanManager::m_Ptr = new (std::nothrow) BanManager();
    if(BanManager::m_Ptr == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate BanManager::m_Ptr in ServerInitialize\n");
        exit(EXIT_FAILURE);
    }

    // load banlist
	BanManager::m_Ptr->Load();

    TextFilesManager::m_Ptr = new (std::nothrow) TextFilesManager();
    if(TextFilesManager::m_Ptr == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate TextFilesManager::m_Ptr in ServerInitialize\n");
        exit(EXIT_FAILURE);
    }

    UdpDebug::m_Ptr = new (std::nothrow) UdpDebug();
    if(UdpDebug::m_Ptr == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate UdpDebug::m_Ptr in ServerInitialize\n");
        exit(EXIT_FAILURE);
    }

    ScriptManager::m_Ptr = new (std::nothrow) ScriptManager();
    if(ScriptManager::m_Ptr == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate ScriptManager::m_Ptr in ServerInitialize\n");
        exit(EXIT_FAILURE);
    }

#ifdef _BUILD_GUI
    MainWindow::m_Ptr = new (std::nothrow) MainWindow();

    if(MainWindow::m_Ptr == NULL || MainWindow::m_Ptr->CreateEx() == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate MainWindow::m_Ptr in ServerInitialize\n");
        exit(EXIT_FAILURE);
    }

    m_hMainWindow = MainWindow::m_Ptr->m_hWnd;
#endif

	SettingManager::m_Ptr->UpdateAll();

#if defined(_WIN32) && !defined(_WIN_IOT)
	m_upSecTimer = SetTimer(NULL, 0, 1000, NULL);

	if(m_upSecTimer == 0) {
		AppendDebugLog("%s - [ERR] Cannot startsectimer in ServerInitialize\n");
        exit(EXIT_FAILURE);
    }

	m_upRegTimer = 0;
#endif
}
//---------------------------------------------------------------------------

bool ServerManager::Start() {
    time(&m_tStartTime);

    SettingManager::m_Ptr->UpdateAll();

    TextFilesManager::m_Ptr->RefreshTextFiles();

#ifdef _BUILD_GUI
    MainWindow::m_Ptr->EnableStartButton(FALSE);
#endif

	m_ui64ActualTick = m_ui64TotalShare = 0;

	m_ui64BytesRead = m_ui64BytesSent = m_ui64BytesSentSaved = 0;

	m_ui32ActualBytesRead = m_ui32ActualBytesSent = m_ui32AverageBytesRead = m_ui32AverageBytesSent = 0;

	m_ui32Joins = m_ui32Parts = m_ui32Logged = m_ui32Peak = 0;

	m_ui64Mins = m_ui64Hours = m_ui64Days = 0;

	m_ui8SrCntr = 0;

	m_sHubIP[0] = '\0';
	m_sHubIP6[0] = '\0';

#ifdef _BUILD_GUI
	if(ResolveHubAddress() == false) {
        MainWindow::m_Ptr->EnableStartButton(TRUE);
        return false;
    }
#else
	ResolveHubAddress();
#endif

    for(uint8_t ui8i = 0; ui8i < 25; ui8i++) {
        if(SettingManager::m_Ptr->m_ui16PortNumbers[ui8i] == 0) {
            break;
        }

        if(m_bUseIPv6 == false) {
            CreateServerThread(AF_INET, SettingManager::m_Ptr->m_ui16PortNumbers[ui8i]);
            continue;
        }
            
        CreateServerThread(AF_INET6, SettingManager::m_Ptr->m_ui16PortNumbers[ui8i]);

		if(SettingManager::m_Ptr->m_bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true || m_bIPv6DualStack == false) {
        	CreateServerThread(AF_INET, SettingManager::m_Ptr->m_ui16PortNumbers[ui8i]);
        }
    }

	if(m_pServersS == NULL) {
#ifdef _BUILD_GUI
		::MessageBox(MainWindow::m_Ptr->m_hWnd, LanguageManager::m_Ptr->m_sTexts[LAN_NO_VALID_TCP_PORT_SPECIFIED], LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], MB_OK|MB_ICONERROR);
        MainWindow::m_Ptr->EnableStartButton(TRUE);
#else
		AppendLog(LanguageManager::m_Ptr->m_sTexts[LAN_NO_VALID_TCP_PORT_SPECIFIED]);
#endif
        return false;
    }

    AppendLog("Serving started");

//  if(tlsenabled == true) {
/*        TLSManager = new (std::nothrow) TLSMan();
        if(TLSManager == NULL) {
    		AppendDebugLog("%s - [MEM] Cannot allocate TLSManager in ServerStart\n", 0);
        	exit(EXIT_FAILURE);
        }*/
//  }

#ifdef _WITH_SQLITE
    DBSQLite::m_Ptr = new (std::nothrow) DBSQLite();
    if(DBSQLite::m_Ptr == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate DBSQLite::m_Ptr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }
#elif _WITH_POSTGRES
    DBPostgreSQL::m_Ptr = new (std::nothrow) DBPostgreSQL();
    if(DBPostgreSQL::m_Ptr == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate DBPostgreSQL::m_Ptr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }
#elif _WITH_MYSQL
    DBMySQL::m_Ptr = new (std::nothrow) DBMySQL();
    if(DBMySQL::m_Ptr == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate DBMySQL::m_Ptr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }
#endif

    IpP2Country::m_Ptr = new (std::nothrow) IpP2Country();
    if(IpP2Country::m_Ptr == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate IpP2Country::m_Ptr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }

    EventQueue::m_Ptr = new (std::nothrow) EventQueue();
    if(EventQueue::m_Ptr == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate EventQueue::m_Ptr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }

    HashManager::m_Ptr = new (std::nothrow) HashManager();
    if(HashManager::m_Ptr == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate HashManager::m_Ptr in ServerStart\n");
        exit(EXIT_FAILURE);
    }

    Users::m_Ptr = new (std::nothrow) Users();
	if(Users::m_Ptr == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate Users::m_Ptr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }

    GlobalDataQueue::m_Ptr = new (std::nothrow) GlobalDataQueue();
    if(GlobalDataQueue::m_Ptr == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate GlobalDataQueue::m_Ptr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }

    DcCommands::m_Ptr = new (std::nothrow) DcCommands();
    if(DcCommands::m_Ptr == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate DcCommands::m_Ptr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }

    // add botname to reserved nicks
    ReservedNicksManager::m_Ptr->AddReservedNick(SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_NICK]);
    SettingManager::m_Ptr->UpdateBot();

    // add opchat botname to reserved nicks
    ReservedNicksManager::m_Ptr->AddReservedNick(SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK]);
    SettingManager::m_Ptr->UpdateOpChat();

    ReservedNicksManager::m_Ptr->AddReservedNick(SettingManager::m_Ptr->m_sTexts[SETTXT_ADMIN_NICK]);

    if((uint16_t)atoi(SettingManager::m_Ptr->m_sTexts[SETTXT_UDP_PORT]) != 0) {
    	if(m_bUseIPv6 == false) {
    		UDPThread::m_PtrIPv6 = UDPThread::Create(AF_INET);
    	} else {
	    	UDPThread::m_PtrIPv6 = UDPThread::Create(AF_INET6);
	
			if(SettingManager::m_Ptr->m_bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true || m_bIPv6DualStack == false) {
				UDPThread::m_PtrIPv6 = UDPThread::Create(AF_INET);
			}
		}
    }
    
    if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == true) {
		ScriptManager::m_Ptr->Start();
    }

    ServiceLoop::m_Ptr = new (std::nothrow) ServiceLoop();
    if(ServiceLoop::m_Ptr == NULL) {
    	AppendDebugLog("%s - [MEM] Cannot allocate ServiceLoop::m_Ptr in ServerStart\n");
    	exit(EXIT_FAILURE);
    }

    // Start the server socket threads
    ServerThread * cur = NULL,
        * next = m_pServersS;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

		cur->Resume();
    }

	m_bServerRunning = true;

    // Call lua_Main
	ScriptManager::m_Ptr->OnStartup();

#ifdef _BUILD_GUI
    MainWindow::m_Ptr->SetStatusValue((string(LanguageManager::m_Ptr->m_sTexts[LAN_RUNNING], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_RUNNING])+"...").c_str());
    MainWindow::m_Ptr->SetStartButtonText(LanguageManager::m_Ptr->m_sTexts[LAN_STOP_HUB]);
    MainWindow::m_Ptr->EnableStartButton(TRUE);
    MainWindow::m_Ptr->EnableGuiItems(TRUE);
#endif

#if defined(_WIN32) && !defined(_WIN_IOT)
    //Start the HubRegistration timer
    if(SettingManager::m_Ptr->m_bBools[SETBOOL_AUTO_REG] == true) {
		m_upRegTimer = SetTimer(NULL, 0, 901000, NULL);
		

        if(m_upRegTimer == 0) {
			AppendDebugLog("%s - [ERR] Cannot start regtimer in ServerStart\n");
        	exit(EXIT_FAILURE);
        }
    }

	if(::SetEvent(ServiceLoop::m_Ptr->m_hLoopEvents[0]) == 0) {
	    AppendDebugLog("%s - [ERR] Cannot set m_hLoopEvent in ServerManager::Start\n");
	    exit(EXIT_FAILURE);
	}
#endif

    return true;
}
//---------------------------------------------------------------------------

void ServerManager::Stop() {
#ifdef _BUILD_GUI
    MainWindow::m_Ptr->EnableStartButton(FALSE);
#endif

    int iRet = snprintf(m_pGlobalBuffer, m_szGlobalBufferSize, "Serving stopped (UL: %" PRIu64 " [%" PRIu64 "], DL: %" PRIu64 ")", m_ui64BytesSent, m_ui64BytesSentSaved, m_ui64BytesRead);
    if(iRet > 0) {
        AppendLog(m_pGlobalBuffer);
    }

#if defined(_WIN32) && !defined(_WIN_IOT)
	//Stop the HubRegistration timer
	if(SettingManager::m_Ptr->m_bBools[SETBOOL_AUTO_REG] == true) {
        if(KillTimer(NULL, m_upRegTimer) == 0) {
    		AppendDebugLog("%s - [ERR] Cannot stop regtimer in ServerStop\n");
        	exit(EXIT_FAILURE);
        }
    }
#endif

    ServerThread * cur = NULL,
        * next = m_pServersS;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

		cur->Close();
		cur->WaitFor();

		delete cur;
    }

	m_pServersS = NULL;
	m_pServersE = NULL;

	// stop the main hub loop
	if(ServiceLoop::m_Ptr != NULL) {
		m_bServerTerminated = true;
	} else {
		FinalStop(false);
    }
}
//---------------------------------------------------------------------------

void ServerManager::FinalStop(const bool bDeleteServiceLoop) {
    if(bDeleteServiceLoop == true) {
		delete ServiceLoop::m_Ptr;
		ServiceLoop::m_Ptr = NULL;
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_SCRIPTING] == true) {
		ScriptManager::m_Ptr->Stop();
    }

    UDPThread::Destroy(UDPThread::m_PtrIPv6);
    UDPThread::m_PtrIPv6 = NULL;

    UDPThread::Destroy(UDPThread::m_PtrIPv4);
    UDPThread::m_PtrIPv4 = NULL;

	// delete userlist field
	if(Users::m_Ptr != NULL) {
		Users::m_Ptr->DisconnectAll();
		delete Users::m_Ptr;
		Users::m_Ptr = NULL;
    }

	delete DcCommands::m_Ptr;
    DcCommands::m_Ptr = NULL;

	// delete hashed userlist manager
    delete HashManager::m_Ptr;
    HashManager::m_Ptr = NULL;

	delete GlobalDataQueue::m_Ptr;
    GlobalDataQueue::m_Ptr = NULL;

    if(RegisterThread::m_Ptr != NULL) {
        RegisterThread::m_Ptr->Close();
        RegisterThread::m_Ptr->WaitFor();
        delete RegisterThread::m_Ptr;
        RegisterThread::m_Ptr = NULL;
    }

	delete EventQueue::m_Ptr;
    EventQueue::m_Ptr = NULL;

	delete IpP2Country::m_Ptr;
    IpP2Country::m_Ptr = NULL;

#ifdef _WITH_SQLITE
	delete DBSQLite::m_Ptr;
    DBSQLite::m_Ptr = NULL;
#elif _WITH_POSTGRES
	delete DBPostgreSQL::m_Ptr;
    DBPostgreSQL::m_Ptr = NULL;
#elif _WITH_MYSQL
	delete DBMySQL::m_Ptr;
    DBMySQL::m_Ptr = NULL;
#endif

/*	if(TLSManager != NULL) {
		delete TLSManager;
        TLSManager = NULL;
    }*/

	//userstat  // better here ;)
//    sqldb->FinalizeAllVisits();

#ifdef _BUILD_GUI
    MainWindow::m_Ptr->SetStatusValue((string(LanguageManager::m_Ptr->m_sTexts[LAN_STOPPED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_STOPPED])+".").c_str());
    MainWindow::m_Ptr->SetStartButtonText(LanguageManager::m_Ptr->m_sTexts[LAN_START_HUB]);
    MainWindow::m_Ptr->EnableStartButton(TRUE);
    MainWindow::m_Ptr->EnableGuiItems(FALSE);
#endif

	m_ui8SrCntr = 0;
	m_ui32Joins = m_ui32Parts = m_ui32Logged = 0;

    UdpDebug::m_Ptr->Cleanup();

#ifdef _WIN32
    HeapCompact(GetProcessHeap(), 0);
    HeapCompact(m_hPtokaXHeap, 0);
#endif

	m_bServerRunning = false;

    if(m_bIsRestart == true) {
		m_bIsRestart = false;

		// start hub
#ifdef _BUILD_GUI
        if(Start() == false) {
            MainWindow::m_Ptr->SetStatusValue((string(LanguageManager::m_Ptr->m_sTexts[LAN_READY], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_READY])+".").c_str());
        }
#else
		if(Start() == false) {
            AppendLog("[ERR] Server start failed in ServerFinalStop");
            exit(EXIT_FAILURE);
        }
#endif
    } else if(m_bIsClose == true) {
		FinalClose();
    }
}
//---------------------------------------------------------------------------

void ServerManager::FinalClose() {
#if defined(_WIN32) && !defined(_WIN_IOT)
    KillTimer(NULL, m_upSecTimer);
#endif

	BanManager::m_Ptr->Save(true);

    ProfileManager::m_Ptr->SaveProfiles();

    RegManager::m_Ptr->Save();

	ScriptManager::m_Ptr->SaveScripts();

	SettingManager::m_Ptr->Save();

    delete ScriptManager::m_Ptr;
	ScriptManager::m_Ptr = NULL;

    delete TextFilesManager::m_Ptr;
    TextFilesManager::m_Ptr = NULL;

    delete ProfileManager::m_Ptr;
    ProfileManager::m_Ptr = NULL;

    delete UdpDebug::m_Ptr;
    UdpDebug::m_Ptr = NULL;

    delete RegManager::m_Ptr;
    RegManager::m_Ptr = NULL;

    delete BanManager::m_Ptr;
    BanManager::m_Ptr = NULL;

    delete ZlibUtility::m_Ptr;
    ZlibUtility::m_Ptr = NULL;

    delete LanguageManager::m_Ptr;
    LanguageManager::m_Ptr = NULL;

    delete TextConverter::m_Ptr;
    TextConverter::m_Ptr = NULL;

    delete SettingManager::m_Ptr;
    SettingManager::m_Ptr = NULL;

    delete ReservedNicksManager::m_Ptr;
    ReservedNicksManager::m_Ptr = NULL;

#ifdef _BUILD_GUI
    MainWindow::m_Ptr->SaveGuiSettings();
#endif

#ifdef __MACH__
	mach_port_deallocate(mach_task_self(), csMachClock);
#endif

    DeleteGlobalBuffer();

#ifdef _WIN32
	HeapDestroy(m_hPtokaXHeap);
	
	WSACleanup();

	#ifndef _WIN_IOT
		::PostQuitMessage(0);
	#endif
#endif
}
//---------------------------------------------------------------------------

void ServerManager::UpdateServers() {
    bool bFound = false;

    // Remove servers for ports we don't want use anymore
    ServerThread * pCur = NULL,
        * pNext = m_pServersS;

    while(pNext != NULL) {
		pCur = pNext;
		pNext = pCur->m_pNext;

        bFound = false;

        for(uint8_t ui8i = 0; ui8i < 25; ui8i++) {
            if(SettingManager::m_Ptr->m_ui16PortNumbers[ui8i] == 0) {
                break;
            }

            if(pCur->m_ui16Port == SettingManager::m_Ptr->m_ui16PortNumbers[ui8i]) {
                bFound = true;
                break;
            }
        }

        if(bFound == false) {
            if(pCur->m_pPrev == NULL) {
                if(pCur->m_pNext == NULL) {
					m_pServersS = NULL;
					m_pServersE = NULL;
                } else {
					pCur->m_pNext->m_pPrev = NULL;
					m_pServersS = pCur->m_pNext;
                }
            } else if(pCur->m_pNext == NULL) {
				pCur->m_pPrev->m_pNext = NULL;
				m_pServersE = pCur->m_pPrev;
            } else {
				pCur->m_pPrev->m_pNext = pCur->m_pNext;
				pCur->m_pNext->m_pPrev = pCur->m_pPrev;
            }

			pCur->Close();
			pCur->WaitFor();

        	delete pCur;
        }
    }

    // Add servers for ports that not running
    for(uint8_t ui8i = 0; ui8i < 25; ui8i++) {
        if(SettingManager::m_Ptr->m_ui16PortNumbers[ui8i] == 0) {
            break;
        }

        bFound = false;

		pCur = NULL,
			pNext = m_pServersS;

        while(pNext != NULL) {
			pCur = pNext;
			pNext = pCur->m_pNext;

            if(pCur->m_ui16Port == SettingManager::m_Ptr->m_ui16PortNumbers[ui8i]) {
                bFound = true;
                break;
            }
        }

        if(bFound == false) {
	        if(m_bUseIPv6 == false) {
	            CreateServerThread(AF_INET, SettingManager::m_Ptr->m_ui16PortNumbers[ui8i]);
	            continue;
	        }

	        CreateServerThread(AF_INET6, SettingManager::m_Ptr->m_ui16PortNumbers[ui8i]);
	
			if(SettingManager::m_Ptr->m_bBools[SETBOOL_BIND_ONLY_SINGLE_IP] == true || m_bIPv6DualStack == false) {
	        	CreateServerThread(AF_INET, SettingManager::m_Ptr->m_ui16PortNumbers[ui8i]);
	        }
        }
    }
}
//---------------------------------------------------------------------------

void ServerManager::ResumeAccepts() {
	if(m_bServerRunning == false) {
        return;
    }

    ServerThread * cur = NULL,
        * next = m_pServersS;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        cur->ResumeSck();
    }
}
//---------------------------------------------------------------------------

void ServerManager::SuspendAccepts(const uint32_t ui32Time) {
	if(m_bServerRunning == false) {
        return;
    }

    if(ui32Time != 0) {
        UdpDebug::m_Ptr->BroadcastFormat("[SYS] Suspending listening threads to %u seconds.", ui32Time);
    } else {
		const char sSuspendMsg[] = "[SYS] Suspending listening threads.";
        UdpDebug::m_Ptr->Broadcast(sSuspendMsg, sizeof(sSuspendMsg)-1);
    }

    ServerThread * cur = NULL,
        * next = m_pServersS;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        cur->SuspendSck(ui32Time);
    }
}
//---------------------------------------------------------------------------

void ServerManager::UpdateAutoRegState() {
    if(m_bServerRunning == false) {
        return;
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_AUTO_REG] == true) {
#if defined(_WIN32) && !defined(_WIN_IOT)
		m_upRegTimer = SetTimer(NULL, 0, 901000, NULL);

        if(m_upRegTimer == 0) {
			AppendDebugLog("%s - [ERR] Cannot start regtimer in ServerUpdateAutoRegState\n");
            exit(EXIT_FAILURE);
        }
#else
	#ifdef __MACH__
		mach_timespec_t mts;
		clock_get_time(csMachClock, &mts);
		ServiceLoop::m_Ptr->m_ui64LastRegToHublist = mts.tv_sec;
	#elif _WIN32
		ServiceLoop::m_Ptr->m_ui64LastRegToHublist = ::GetTickCount64() / 1000;
	#else
		timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		ServiceLoop::m_Ptr->m_ui64LastRegToHublist = ts.tv_sec;
	#endif
#endif
#if defined(_WIN32) && !defined(_WIN_IOT)
    } else {
        if(KillTimer(NULL, m_upRegTimer) == 0) {
    		AppendDebugLog("%s - [ERR] Cannot stop regtimer in ServerUpdateAutoRegState\n");
        	exit(EXIT_FAILURE);
        }
#endif
    }
}
//---------------------------------------------------------------------------

void ServerManager::CreateServerThread(const int iAddrFamily, const uint16_t ui16PortNumber, const bool bResume/* = false*/) {
	ServerThread * pServer = new (std::nothrow) ServerThread(iAddrFamily, ui16PortNumber);
    if(pServer == NULL) {
		AppendDebugLog("%s - [MEM] Cannot allocate pServer in ServerCreateServerThread\n");
        exit(EXIT_FAILURE);
    }

	if(pServer->Listen() == true) {
		if(m_pServersE == NULL) {
			m_pServersS = pServer;
			m_pServersE = pServer;
        } else {
            pServer->m_pPrev = m_pServersE;
			m_pServersE->m_pNext = pServer;
			m_pServersE = pServer;
        }
    } else {
        delete pServer;
    }

    if(bResume == true) {
        pServer->Resume();
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ServerManager::CommandLineSetup() {
	printf("%s built on %s %s\n\n", g_sPtokaXTitle, __DATE__, __TIME__);
	printf("Welcome to PtokaX configuration setup.\nDirectory for PtokaX configuration is: %s\nWhen this directory is wrong, then exit this setup.\nTo specify correct configuration directory start PtokaX with -c configdir parameter.", m_sPath.c_str());

	const char sMenu[] = "\n\nAvailable options:\n"
		"1. Basic setup. Only few things required for PtokaX run.\n"
		"2. Complete setup. Long setup, where you can change all PtokaX setings.\n"
		"3. Add registered user.\n"
		"4. Exit this setup.\n\n"
		"Your choice: ";

	printf(sMenu);

	while(true) {
		int iChar = getchar();
	
		while(getchar() != '\n') {
			// boredom...
		};

		switch(iChar) {
			case '1':
				SettingManager::m_Ptr->CmdLineBasicSetup();
				printf(sMenu);
				continue;
			case '2':
				SettingManager::m_Ptr->CmdLineCompleteSetup();
				printf(sMenu);
				continue;
			case '3':
				RegManager::m_Ptr->AddRegCmdLine();
				printf(sMenu);
				continue;
			case '4':
				printf("%s ending...\n", g_sPtokaXTitle);
				break;
			default:
				printf("Unknown option: %c\nYour choice: ", iChar);
				continue;
		}

		break;
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool ServerManager::ResolveHubAddress(const bool bSilent/* = false*/) {
    if(SettingManager::m_Ptr->m_bBools[SETBOOL_RESOLVE_TO_IP] == true) {
        if(isIP(SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_ADDRESS]) == false) {
#ifdef _BUILD_GUI
            MainWindow::m_Ptr->SetStatusValue((string(LanguageManager::m_Ptr->m_sTexts[LAN_RESOLVING_HUB_ADDRESS], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_RESOLVING_HUB_ADDRESS])+"...").c_str());
#endif

            struct addrinfo hints;
            memset(&hints, 0, sizeof(addrinfo));

            if(m_bUseIPv6 == true) {
                hints.ai_family = AF_UNSPEC;
            } else {
                hints.ai_family = AF_INET;
            }

            struct addrinfo *res;

            if(::getaddrinfo(SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_ADDRESS], NULL, &hints, &res) != 0 || (res->ai_family != AF_INET && res->ai_family != AF_INET6)) {
            	if(bSilent == false) {
#ifdef _WIN32
            		int err = WSAGetLastError();
	#ifdef _BUILD_GUI
					::MessageBox(MainWindow::m_Ptr->m_hWnd,(string(LanguageManager::m_Ptr->m_sTexts[LAN_RESOLVING_OF_HOSTNAME], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_RESOLVING_OF_HOSTNAME])+" '"+string(SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_ADDRESS])+"' "+
						string(LanguageManager::m_Ptr->m_sTexts[LAN_HAS_FAILED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_HAS_FAILED])+".\n"+string(LanguageManager::m_Ptr->m_sTexts[LAN_ERROR_CODE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_ERROR_CODE])+": "+string(WSErrorStr(err))+" ("+string(err)+")\n\n"+
						string(LanguageManager::m_Ptr->m_sTexts[LAN_CHECK_THE_ADDRESS_PLEASE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_CHECK_THE_ADDRESS_PLEASE])+".").c_str(), LanguageManager::m_Ptr->m_sTexts[LAN_ERROR], MB_OK|MB_ICONERROR);
	#else
                	AppendLog((string(LanguageManager::m_Ptr->m_sTexts[LAN_RESOLVING_OF_HOSTNAME], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_RESOLVING_OF_HOSTNAME])+
						" '"+string(SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_ADDRESS])+"' "+string(LanguageManager::m_Ptr->m_sTexts[LAN_HAS_FAILED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_HAS_FAILED])+".\n"+string(LanguageManager::m_Ptr->m_sTexts[LAN_ERROR_CODE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_ERROR_CODE])+
						": "+string(WSErrorStr(err))+" ("+string(err)+")\n\n"+string(LanguageManager::m_Ptr->m_sTexts[LAN_CHECK_THE_ADDRESS_PLEASE], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_CHECK_THE_ADDRESS_PLEASE])+".").c_str());
	#endif
#else
                	AppendLog((string(LanguageManager::m_Ptr->m_sTexts[LAN_RESOLVING_OF_HOSTNAME], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_RESOLVING_OF_HOSTNAME])+
						" '"+string(SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_ADDRESS])+"' "+string(LanguageManager::m_Ptr->m_sTexts[LAN_HAS_FAILED], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_HAS_FAILED])+".\n"+string(LanguageManager::m_Ptr->m_sTexts[LAN_CHECK_THE_ADDRESS_PLEASE], 
						(size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_CHECK_THE_ADDRESS_PLEASE])+".").c_str());
#endif
				}

                return false;
            } else {
				Memo("*** "+string(SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_ADDRESS], (size_t)SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_HUB_ADDRESS])+" "+string(LanguageManager::m_Ptr->m_sTexts[LAN_RESOLVED_SUCCESSFULLY], (size_t)LanguageManager::m_Ptr->m_ui16TextsLens[LAN_RESOLVED_SUCCESSFULLY])+".");

                if(m_bUseIPv6 == true) {
                    struct addrinfo *next = res;
                    while(next != NULL) {
                        if(next->ai_family == AF_INET) {
                            if(((sockaddr_in *)(next->ai_addr))->sin_addr.s_addr != INADDR_ANY) {
                                strcpy(m_sHubIP, inet_ntoa(((sockaddr_in *)(next->ai_addr))->sin_addr));
                            }
                        } else if(next->ai_family == AF_INET6) {
#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
                            win_inet_ntop(&((struct sockaddr_in6 *)next->ai_addr)->sin6_addr, m_sHubIP6, 40);
#else
                            inet_ntop(AF_INET6, &((struct sockaddr_in6 *)next->ai_addr)->sin6_addr, m_sHubIP6, 40);
#endif
                        }

                        next = next->ai_next;
                    }
                } else if(((sockaddr_in *)(res->ai_addr))->sin_addr.s_addr != INADDR_ANY) {
                    strcpy(m_sHubIP, inet_ntoa(((sockaddr_in *)(res->ai_addr))->sin_addr));
                }

                if(m_sHubIP[0] != '\0') {
                    string msg = "*** "+string(m_sHubIP);
                    if(IsPrivateIP(m_sHubIP) == true) {
                    	SettingManager::m_Ptr->SetBool(SETBOOL_AUTO_REG, false);
					}
                    if(m_sHubIP6[0] != '\0') {
                        msg += " / "+string(m_sHubIP6);
                        if(IsPrivateIP(m_sHubIP6) == true) {
	                    	SettingManager::m_Ptr->SetBool(SETBOOL_AUTO_REG, false);
						}
                    }

				    Memo(msg);
                } else if(m_sHubIP6[0] != '\0') {
				    Memo("*** "+string(m_sHubIP6));
				    if(IsPrivateIP(m_sHubIP6) == true) {
                    	SettingManager::m_Ptr->SetBool(SETBOOL_AUTO_REG, false);
					}
                }

				freeaddrinfo(res);
            }
        } else {
            strcpy(m_sHubIP, SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_ADDRESS]);
            if(IsPrivateIP(SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_ADDRESS]) == true) {
                SettingManager::m_Ptr->SetBool(SETBOOL_AUTO_REG, false);
			}
        }
    } else {
        if(SettingManager::m_Ptr->m_sTexts[SETTXT_IPV4_ADDRESS] != NULL) {
            strcpy(m_sHubIP, SettingManager::m_Ptr->m_sTexts[SETTXT_IPV4_ADDRESS]);
        } else {
			m_sHubIP[0] = '\0';
        }

        if(SettingManager::m_Ptr->m_sTexts[SETTXT_IPV6_ADDRESS] != NULL) {
            strcpy(m_sHubIP6, SettingManager::m_Ptr->m_sTexts[SETTXT_IPV6_ADDRESS]);
        } else {
			m_sHubIP6[0] = '\0';
        }

        if(isIP(SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_ADDRESS]) == true) {
        	if(IsPrivateIP(SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_ADDRESS]) == true) {
                SettingManager::m_Ptr->SetBool(SETBOOL_AUTO_REG, false);
			}
        }
    }

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
