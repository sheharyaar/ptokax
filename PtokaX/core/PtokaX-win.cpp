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
#include "LuaInc.h"
//---------------------------------------------------------------------------
#include "LanguageManager.h"
#include "ServerManager.h"
#include "serviceLoop.h"
#include "SettingManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#ifndef _WIN_IOT
	#include "ExceptionHandling.h"
#endif
#include "LuaScript.h"
//---------------------------------------------------------------------------

static int InstallService(const char * sServiceName, const char * sPath) {
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	     
	if(schSCManager == NULL) {
	    printf("OpenSCManager failed (%lu)!", GetLastError());
	    return EXIT_FAILURE;
	}
	
	char sBuf[MAX_PATH+1];
	::GetModuleFileName(NULL, sBuf, MAX_PATH);
	
	string sCmdLine = "\"" + string(sBuf) + "\" -s " + string(sServiceName);
	
	if(sPath != NULL) {
	    sCmdLine += " -c " + string(sPath);
	}
	
	SC_HANDLE schService = CreateService(schSCManager, sServiceName, sServiceName, 0, SERVICE_WIN32_OWN_PROCESS,
	    SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, sCmdLine.c_str(),
	    NULL, NULL, NULL, NULL, NULL);
	
	if(schService == NULL) {
	    printf("CreateService failed (%lu)!", GetLastError());
	    CloseServiceHandle(schSCManager);
	    return EXIT_FAILURE;
	} else {
	    printf("PtokaX service '%s' installed successfully.", sServiceName);
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	
	return EXIT_SUCCESS;
}
//---------------------------------------------------------------------------
	
static int UninstallService(const char * sServiceName) {
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	     
	if(schSCManager == NULL) {
	    printf("OpenSCManager failed (%lu)!", GetLastError());
	    return EXIT_FAILURE;
	}
	
	SC_HANDLE schService = OpenService(schSCManager, sServiceName, SERVICE_QUERY_STATUS | SERVICE_STOP | DELETE);
	     
	if(schService == NULL) {
	    printf("OpenService failed (%lu)!", GetLastError());
	    CloseServiceHandle(schSCManager);
	    return EXIT_FAILURE;
	}
	
	SERVICE_STATUS_PROCESS ssp;
	DWORD dwBytesNeeded;
	
	if(QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded) != 0) {
	    if(ssp.dwCurrentState != SERVICE_STOPPED && ssp.dwCurrentState != SERVICE_STOP_PENDING) {
	        ControlService(schService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp);
	    }
	}
	
	if(DeleteService(schService) == false) {
	    printf("DeleteService failed (%lu)!", GetLastError());
	    CloseServiceHandle(schService);
	    CloseServiceHandle(schSCManager);
	    return EXIT_FAILURE;
	} else {
	    printf("PtokaX service '%s' deleted successfully.", sServiceName);
	    CloseServiceHandle(schService);
	    CloseServiceHandle(schSCManager);
	    return EXIT_SUCCESS;
	}
}
//---------------------------------------------------------------------------
static SERVICE_STATUS_HANDLE ssh;
static SERVICE_STATUS ss;
//---------------------------------------------------------------------------
	
static void WINAPI CtrlHandler(DWORD dwCtrl) {
	switch(dwCtrl) {
	    case SERVICE_CONTROL_SHUTDOWN:
	    case SERVICE_CONTROL_STOP:
	        ss.dwCurrentState = SERVICE_STOP_PENDING;
	        ServerManager::m_bIsClose = true;
			ServerManager::Stop();
	    case SERVICE_CONTROL_INTERROGATE:
	        // Fall through to send current status.
	        break;
	    default:
	        break;
	}
	
	if(SetServiceStatus(ssh, &ss) == false) {
		AppendLog(("CtrlHandler::SetServiceStatus failed ("+string((uint32_t)GetLastError())+")!").c_str());
	}
}
//---------------------------------------------------------------------------

static void MainLoop() {
#ifdef _WIN_IOT
	while(true) {
		ServiceLoop::m_Ptr->Looper();
		
		if(ServerManager::m_bServerTerminated == true) {
		    break;
		}

		::Sleep(100);
	}
#else
	MSG msg = { 0 };
	BOOL bRet = -1;
	        
	while((bRet = ::GetMessage(&msg, NULL, 0, 0)) != 0) {
	    if(bRet == -1) {
	        // handle the error and possibly exit
	    } else {
	    	if(msg.message == WM_PX_DO_LOOP) {
	    		ServiceLoop::m_Ptr->Looper();
	        } else if(msg.message == WM_TIMER) {
				if (msg.wParam == ServerManager::m_upSecTimer) {
					ServerManager::OnSecTimer();
                } else if(msg.wParam == ServerManager::m_upRegTimer) {
					ServerManager::OnRegTimer();
                } else {
                    //Must be script timer
                    ScriptOnTimer(msg.wParam);
                }
	        }

	    	::TranslateMessage(&msg);
	        ::DispatchMessage(&msg);
	    }
	}

    ExceptionHandlingUnitialize();
#endif
}
//---------------------------------------------------------------------------

static void WINAPI StartService(DWORD /*argc*/, char* argv[]) {
	ssh = RegisterServiceCtrlHandler(argv[0], CtrlHandler);
	
	if(ssh == NULL) {
		AppendLog(("RegisterServiceCtrlHandler failed ("+string((uint32_t)GetLastError())+")!").c_str());
	    return;
	}
	
	ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ss.dwCurrentState = SERVICE_START_PENDING;
	ss.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP;
	ss.dwWin32ExitCode = NO_ERROR;
	ss.dwCheckPoint = 0;
	ss.dwWaitHint = 10 * 1000;
	
	if(SetServiceStatus(ssh, &ss) == false) {
		AppendLog(("StartService::SetServiceStatus failed ("+string((uint32_t)GetLastError())+")!").c_str());
		return;
	}
	
	ServerManager::Initialize();
	
	if (ServerManager::Start() == false) {
	    AppendLog("Server start failed!");
		ss.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(ssh, &ss);
		return;
	}
	
	ss.dwCurrentState = SERVICE_RUNNING;
	
	if(SetServiceStatus(ssh, &ss) == false) {
		AppendLog(("StartService::SetServiceStatus1 failed ("+string((uint32_t)GetLastError())+")!").c_str());
		return;
	}

	MainLoop();

	ss.dwCurrentState = SERVICE_STOPPED;
	SetServiceStatus(ssh, &ss);
}
//---------------------------------------------------------------------------

int __cdecl main(int argc, char* argv[]) {
#ifndef _WIN_IOT
    ::SetDllDirectory("");
#endif

#if !defined(_WIN64) && !defined(_WIN_IOT)
    HINSTANCE hKernel32 = ::LoadLibrary("Kernel32.dll");

    typedef BOOL (WINAPI * SPDEPP)(DWORD);
    SPDEPP pSPDEPP = (SPDEPP)::GetProcAddress(hKernel32, "SetProcessDEPPolicy");

    if(pSPDEPP != NULL) {
        pSPDEPP(PROCESS_DEP_ENABLE);
    }

    ::FreeLibrary(hKernel32);
#endif

#ifdef _DEBUG
//    AllocConsole();
//    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
//    Cout("PtokaX Debug console\n");
#endif
	
	char sBuf[MAX_PATH+1];
	::GetModuleFileName(NULL, sBuf, MAX_PATH);
	char * sPath = strrchr(sBuf, '\\');
	if(sPath != NULL) {
		ServerManager::m_sPath = string(sBuf, sPath - sBuf);
	} else {
		ServerManager::m_sPath = sBuf;
	}

	char * sServiceName = NULL;
	
	bool bInstallService = false, bSetup = false;

	for(int i = 1; i < argc; i++) {
	    if(stricmp(argv[i], "-s") == NULL || stricmp(argv[i], "/service") == NULL) {
	    	if(++i == argc) {
	            AppendLog("Missing service name!");
	            return EXIT_FAILURE;
	    	}
	    	sServiceName = argv[i];
			ServerManager::m_bService = true;
	    } else if(stricmp(argv[i], "-c") == 0) {
	        if(++i == argc) {
	            printf("Missing config directory!");
	            return EXIT_FAILURE;
	        }
	
	        size_t szLen = strlen(argv[i]);
	        if(szLen >= 1 && argv[i][0] != '\\' && argv[i][0] != '/') {
	            if(szLen < 4 || (argv[i][1] != ':' || (argv[i][2] != '\\' && argv[i][2] != '/'))) {
	                printf("Config directory must be absolute path!");
	                return EXIT_FAILURE;
	            }
	    	}
	
	    	if(argv[i][szLen - 1] == '/' || argv[i][szLen - 1] == '\\') {
				ServerManager::m_sPath = string(argv[i], szLen - 1);
	    	} else {
				ServerManager::m_sPath = string(argv[i], szLen);
	        }
	    
			if(DirExist(ServerManager::m_sPath.c_str()) == false) {
				if(CreateDirectory(ServerManager::m_sPath.c_str(), NULL) == 0) {
	                printf("Config directory not exist and can't be created!");
	                return EXIT_FAILURE;
	            }
	        }
	    } else if(stricmp(argv[i], "-i") == NULL || stricmp(argv[i], "/install") == NULL) {
	    	if(++i == argc) {
	            printf("Please specify service name!");
	    		return EXIT_FAILURE;
	    	}
	    	sServiceName = argv[i];
	    	bInstallService = true;
	    } else if(stricmp(argv[i], "-u") == NULL || stricmp(argv[i], "/uninstall") == NULL) {
	    	if(++i == argc) {
	            printf("Please specify service name!");
	    		return EXIT_FAILURE;
	    	}
	    	sServiceName = argv[i];
	    	return UninstallService(sServiceName);
	    } else if(stricmp(argv[i], "-v") == NULL || stricmp(argv[i], "/version") == NULL) {
	    	printf("%s built on %s %s\n", g_sPtokaXTitle, __DATE__, __TIME__);
	    	return EXIT_SUCCESS;
	    } else if(stricmp(argv[i], "-h") == NULL || stricmp(argv[i], "/help") == NULL) {
	        printf("Usage: PtokaX [-v] [-m] [-i servicename] [-u servicename] [-c configdir]\n\n"
				"Options:\n"
				"\t-i servicename\t\t- install ptokax service with given name.\n"
				"\t-u servicename\t\t- uninstall ptokax service with given name.\n"
				"\t-c configdir\t- absolute path to PtokaX configuration directory.\n"
				"\t-v\t\t- show PtokaX version with build date and time.\n"
				"\t-m\t\t- show PtokaX configuration menu.\n"
			);
	    	return EXIT_SUCCESS;
	    } else if(stricmp(argv[i], "/generatexmllanguage") == NULL) {
	        LanguageManager::GenerateXmlExample();
	        return EXIT_SUCCESS;
	    } else if(strcasecmp(argv[i], "-m") == 0) {
	    	bSetup = true;
	    } else {
	    	printf("Unknown parameter %s.\nUsage: PtokaX [-v] [-m] [-i servicename] [-u servicename] [-c configdir]\n\n"
				"Options:\n"
				"\t-i servicename\t\t- install ptokax service with given name.\n"
				"\t-u servicename\t\t- uninstall ptokax service with given name.\n"
				"\t-c configdir\t- absolute path to PtokaX configuration directory.\n"
				"\t-v\t\t- show PtokaX version with build date and time.\n"
				"\t-m\t\t- show PtokaX configuration menu.\n",
				argv[i]);
	    	return EXIT_SUCCESS;
		}
	}

	if(bSetup == true) {
		ServerManager::Initialize();

		ServerManager::CommandLineSetup();
		
		ServerManager::FinalClose();

		return EXIT_SUCCESS;
	}

	if(bInstallService == true) {
	    if(sPath == NULL && strcmp(ServerManager::m_sPath.c_str(), sBuf) == 0) {
	        return InstallService(sServiceName, NULL);
		} else {
			return InstallService(sServiceName, ServerManager::m_sPath.c_str());
		}
	}

#ifndef _WIN_IOT
    ExceptionHandlingInitialize(ServerManager::m_sPath, sBuf);
#endif

	if(ServerManager::m_bService == false) {
	    ServerManager::Initialize();
	
	    if(ServerManager::Start() == false) {
	        printf("Server start failed!");
#ifndef _WIN_IOT
            ExceptionHandlingUnitialize();
#endif
	        return EXIT_FAILURE;
	    } else {
	        printf("%s running...\n", g_sPtokaXTitle);
	    }

		MainLoop();
	} else {
	    SERVICE_TABLE_ENTRY DispatchTable[] = {
	        { sServiceName, StartService },
	        { NULL, NULL }
	    };
	       
	    if(StartServiceCtrlDispatcher(DispatchTable) == false) {
			AppendLog(("StartServiceCtrlDispatcher failed ("+string((uint32_t)GetLastError())+")!").c_str());
#ifndef _WIN_IOT
            ExceptionHandlingUnitialize();
#endif
	        return EXIT_FAILURE;
	    }
	}

    return EXIT_SUCCESS;
}
//---------------------------------------------------------------------------
