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
#include "eventqueue.h"
#include "GlobalDataQueue.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ServerManager.h"
#include "serviceLoop.h"
#include "SettingManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
static bool bTerminatedBySignal = false;
static int iSignal = 0;
//---------------------------------------------------------------------------

static void SigHandler(int iSig) {
    bTerminatedBySignal = true;

    iSignal = iSig;

	// restore to default...
	struct sigaction sigact;
	sigact.sa_handler = SIG_DFL;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	    
	sigaction(iSig, &sigact, NULL);
}
//---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
	bool bSetup = false;

	char * sPidFile = NULL;

	for(int i = 1; i < argc; i++) {
	    if(strcasecmp(argv[i], "-d") == 0) {
	    	ServerManager::m_bDaemon = true;
	    } else if(strcasecmp(argv[i], "-c") == 0) {
	    	if(++i == argc) {
	            printf("Missing config directory!\n");
	            return EXIT_FAILURE;
	    	}
	
			if(argv[i][0] != '/') {
	            printf("Config directory must be absolute path!\n");
	            return EXIT_FAILURE;
			}
	
	        size_t szLen = strlen(argv[i]);
			if(argv[i][szLen - 1] == '/') {
	            ServerManager::m_sPath = string(argv[i], szLen - 1);
			} else {
	            ServerManager::m_sPath = string(argv[i], szLen);
	        }
	
	        if(DirExist(ServerManager::m_sPath.c_str()) == false) {
	        	if(mkdir(ServerManager::m_sPath.c_str(), 0755) == -1) {
	                if(ServerManager::m_bDaemon == true) {
	                    syslog(LOG_USER | LOG_ERR, "Config directory not exist and can't be created!\n");
	                } else {
	                    printf("Config directory not exist and can't be created!");
	                }
	            }
            }
	    } else if(strcasecmp(argv[i], "-v") == 0) {
	        printf("%s built on %s %s\n", g_sPtokaXTitle, __DATE__, __TIME__);
	        return EXIT_SUCCESS;
	    } else if(strcasecmp(argv[i], "-h") == 0) {
	        printf("Usage: PtokaX [-d] [-v] [-m] [-c configdir] [-p pidfile]\n\n"
				"Options:\n"
				"\t-d\t\t- run as daemon.\n"
				"\t-c configdir\t- absolute path to PtokaX configuration directory.\n"
				"\t-p pidfile\t-p <pidfile>	- path with filename where PtokaX PID will be stored.\n"
				"\t-v\t\t- show PtokaX version with build date and time.\n"
				"\t-m\t\t- show PtokaX configuration menu.\n"
			);
	        return EXIT_SUCCESS;
	    } else if(strcasecmp(argv[i], "-p") == 0) {
	    	if(++i == argc) {
	            printf("Missing pid file!\n");
	            return EXIT_FAILURE;
	    	}
	
			sPidFile = argv[i];
	    } else if(strcasecmp(argv[i], "/generatexmllanguage") == 0) {
	        LanguageManager::GenerateXmlExample();
	        return EXIT_SUCCESS;
	    } else if(strcasecmp(argv[i], "-m") == 0) {
	    	bSetup = true;
	    } else {
	    	printf("Unknown parameter %s.\nUsage: PtokaX [-d] [-v] [-m] [-c configdir] [-p pidfile]\n\n"
				"Options:\n"
				"\t-d\t\t- run as daemon.\n"
				"\t-c configdir\t- absolute path to PtokaX configuration directory.\n"
				"\t-p pidfile\t-p <pidfile>	- path with filename where PtokaX PID will be stored.\n"
				"\t-v\t\t- show PtokaX version with build date and time.\n"
				"\t-m\t\t- show PtokaX configuration menu.\n",
				argv[i]);
	    	return EXIT_SUCCESS;
		}
	}
	
	if(ServerManager::m_sPath.size() == 0) {
	    char* home;
	    char curdir[PATH_MAX];
	    if(ServerManager::m_bDaemon == true && (home = getenv("HOME")) != NULL) {
	        ServerManager::m_sPath = string(home) + "/.PtokaX";
	            
	        if(DirExist(ServerManager::m_sPath.c_str()) == false) {
	            if(mkdir(ServerManager::m_sPath.c_str(), 0755) == -1) {
	                syslog(LOG_USER | LOG_ERR, "Config directory not exist and can't be created!\n");
	            }
	        }
	    } else if(getcwd(curdir, PATH_MAX) != NULL) {
	        ServerManager::m_sPath = curdir;
	    } else {
	        ServerManager::m_sPath = ".";
	    }
	}

	if(bSetup == true) {
		ServerManager::Initialize();

		ServerManager::CommandLineSetup();
		
		ServerManager::FinalClose();

		return EXIT_SUCCESS;
	}

	if(ServerManager::m_bDaemon == true) {
	    printf("Starting %s as daemon using %s as config directory.\n", g_sPtokaXTitle, ServerManager::m_sPath.c_str());
	
	    pid_t pid1 = fork();
	    if(pid1 == -1) {
	        syslog(LOG_USER | LOG_ERR, "First fork failed!\n");
	        return EXIT_FAILURE;
	    } else if(pid1 > 0) {
	        return EXIT_SUCCESS;
	    }

	    if(setsid() == -1) {
	        syslog(LOG_USER | LOG_ERR, "Setsid failed!\n");
	        return EXIT_FAILURE;
	    }
	
	    pid_t pid2 = fork();
	    if(pid2 == -1) {
	        syslog(LOG_USER | LOG_ERR, "Second fork failed!\n");
	        return EXIT_FAILURE;
	    } else if(pid2 > 0) {
            return EXIT_SUCCESS;
	    }

		if(sPidFile != NULL) {
			FILE * fw = fopen(sPidFile, "w");
			if(fw != NULL) {
				fprintf(fw, "%ld\n", (long)getpid());
				fclose(fw);
			}
		}

	    if(chdir("/") == -1) {
	        syslog(LOG_USER | LOG_ERR, "chdir failed!\n");
	        return EXIT_FAILURE;
	    } else if(pid2 > 0) {
        }
	
	    close(STDIN_FILENO);
	    close(STDOUT_FILENO);
	    close(STDERR_FILENO);
	
	    if(open("/dev/null", O_RDWR) == -1) {
	        syslog(LOG_USER | LOG_ERR, "Failed to open /dev/null!\n");
	        return EXIT_FAILURE;
	    }
	
	    if(dup(0) == -1) {
	        syslog(LOG_USER | LOG_ERR, "First dup(0) failed!\n");
	        return EXIT_FAILURE;
        }

	    if(dup(0) == -1) {
	        syslog(LOG_USER | LOG_ERR, "Second dup(0) failed!\n");
	        return EXIT_FAILURE;
        }
	}
	
	sigset_t sst;
	sigemptyset(&sst);
	sigaddset(&sst, SIGPIPE);
	sigaddset(&sst, SIGURG);
	sigaddset(&sst, SIGALRM);
	
	if(ServerManager::m_bDaemon == true) {
	    sigaddset(&sst, SIGHUP);
	}
	
	pthread_sigmask(SIG_BLOCK, &sst, NULL);
	
	struct sigaction sigact;
	sigact.sa_handler = SigHandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	
	if(sigaction(SIGINT, &sigact, NULL) == -1) {
	    AppendDebugLog("%s - [ERR] Cannot create sigaction SIGINT in main\n");
	    exit(EXIT_FAILURE);
	}
	
	if(sigaction(SIGTERM, &sigact, NULL) == -1) {
	    AppendDebugLog("%s - [ERR] Cannot create sigaction SIGTERM in main\n");
	    exit(EXIT_FAILURE);
	}
	
	if(sigaction(SIGQUIT, &sigact, NULL) == -1) {
	    AppendDebugLog("%s - [ERR] Cannot create sigaction SIGQUIT in main\n");
	    exit(EXIT_FAILURE);
	}
	
	if(ServerManager::m_bDaemon == false && sigaction(SIGHUP, &sigact, NULL) == -1) {
	    AppendDebugLog("%s - [ERR] Cannot create sigaction SIGHUP in main\n");
	    exit(EXIT_FAILURE);
	}

	ServerManager::Initialize();

	if(ServerManager::Start() == false) {
		if(ServerManager::m_bDaemon == false) {
		    printf("Server start failed!\n");
		} else {
		    syslog(LOG_USER | LOG_ERR, "Server start failed!\n");
		}
		return EXIT_FAILURE;
	} else if(ServerManager::m_bDaemon == false) {
		printf("%s running...\n", g_sPtokaXTitle);
	}
	
	struct timespec sleeptime;
	sleeptime.tv_sec = 0;
	sleeptime.tv_nsec = 100000000;
	
	while(true) {
		ServiceLoop::m_Ptr->Looper();
		
		if(ServerManager::m_bServerTerminated == true) {
		    break;
		}
	
	    if(bTerminatedBySignal == true) {
	        if(ServerManager::m_bIsClose == true) {
	            break;
	        }
	
	        string str("Received signal ");
	
	        if(iSignal == SIGINT) {
	            str += "SIGINT";
	        } else if(iSignal == SIGTERM) {
	            str += "SIGTERM";
	        } else if(iSignal == SIGQUIT) {
	            str += "SIGQUIT";
	        } else if(iSignal == SIGHUP) {
	            str += "SIGHUP";
	        } else {
	            str += string(iSignal);
	        }
	
	        str += " ending...";
	
	        AppendLog(str.c_str());
	
	        ServerManager::m_bIsClose = true;
	        ServerManager::Stop();
	
	        // tell the scripts about the end
	        ScriptManager::m_Ptr->OnExit();
	
	        // send last possible global data
	        GlobalDataQueue::m_Ptr->SendFinalQueue();
	
	        ServerManager::FinalStop(true);
	
	        break;
	    }
	
	    nanosleep(&sleeptime, NULL);
	}

	if(ServerManager::m_bDaemon == false) {
	    printf("%s ending...\n", g_sPtokaXTitle);
	} else if(sPidFile != NULL) {
		unlink(sPidFile);
	}

    return EXIT_SUCCESS;
}
//---------------------------------------------------------------------------
