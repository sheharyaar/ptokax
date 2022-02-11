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
#include "utility.h"
//---------------------------------------------------------------------------
#include "hashBanManager.h"
#include "LanguageManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
	#include "../gui.win/GuiUtil.h"
    #include "../gui.win/MainWindowPageUsersChat.h"
#endif
//---------------------------------------------------------------------------
#ifndef _WITHOUT_SKEIN
	#include <skein.h>
#endif
//---------------------------------------------------------------------------
/*
static const int MAX_PAT_SIZE = 64;
static const int MAX_ALPHABET_SIZE = 255;
*/
//---------------------------------------------------------------------------

#ifdef _WIN32
void Cout(const string & sMsg) {
	if(ServerManager::m_hConsole != NULL) {
		WriteConsole(ServerManager::m_hConsole, (sMsg+"\n").c_str(), (DWORD)sMsg.size()+1, 0, 0);
    }
}
#else
void Cout(const string &/*sMsg*/) {
	return;
}
#endif
//---------------------------------------------------------------------------
/*
#ifdef _WIN32
	// Boyer-Moore string matching algo.
	// returns :
	// offset in bytes or -1 if no match
	int BMFind(char *text, int N, char *pat, int M) {
	   int i, j, DD[MAX_PAT_SIZE], D[MAX_ALPHABET_SIZE];
	
	   // Predzpracovani
	   preDD(pat, M, DD);
	   preD(pat, M, D);
	
	   // Vyhledavani 
	   // Searching
	   i = 0;
	   while (i <= N-M) {
	      for(j = M-1; j >= 0  &&  pat[j] == text[j+i]; --j);
	      if (j < 0) {
	         return i;
	         //i += DD[0];
	      }
	      else
	         i += DD[j] > (D[text[j + i]] - M + 1 + j) ? DD[j] : (D[text[j + i]] - M + 1 + j);
	   }
	   return -1;
	}
//---------------------------------------------------------------------------
	
	void preD(char *pat, int M, int D[]) {
	   int i;
	   for(i = 0; i < MAX_ALPHABET_SIZE; ++i)
	      D[i] = M;
	   for(i = 0; i < M - 1; ++i)
	      D[pat[i]] = M - i - 1;
	}
//---------------------------------------------------------------------------
	
	// Funkce ulozi do suff[i] delku nejdelsiho podretezce,
	// ktery konci na pozici pat[i]
	// a soucasne je priponou pat
	//
	void suffixes(char *pat, int M, int *suff) {
	   int f = 0, g, i;
	
	   suff[M - 1] = M;
	   g = M - 1;
	   for(i = M - 2; i >= 0; --i) {
	      if (i > g && suff[i + M - 1 - f] < i - g)
	         suff[i] = suff[i + M - 1 - f];
	      else {
	         if (i < g)
	            g = i;
	         f = i;
	         while (g >= 0 && pat[g] == pat[g + M - 1 - f])
	            --g;
	         suff[i] = f - g;
	      }
	   }
	}
//---------------------------------------------------------------------------
	
	void preDD(char *pat, int M, int DD[]) {
	   int i, j, suff[MAX_PAT_SIZE];
	
	   suffixes(pat, M, suff);
	
	   for(i = 0; i < M; ++i)
	      DD[i] = M;
	   j = 0;
	   for(i = M - 1; i >= -1; --i)
	      if (suff[i] == i+1  ||  i == -1)
	         for( ; j < M-1-i; ++j)
	            if (DD[j] == M)
	               DD[j] = M-1-i;
	   for(i = 0; i <= M-2; ++i)
	      DD[M - 1 - suff[i]] = M-1-i;
	}
#endif
*/
//---------------------------------------------------------------------------

char * Lock2Key(char * sLock) {
	static const uint8_t ui8LockSize = 46;
    static char cKey[128];
    cKey[0] = '\0';

    // $Lock EXTENDEDPROTOCOL33MTL6@h5AIad^P2UoPv?fZU]ivM6 Pk=PtokaX|

    sLock = sLock+6; // set begin after $Lock_
    sLock[ui8LockSize] = '\0'; // set end before Pk
    
    uint8_t v;
    
	// first make the crypting stuff
	for(uint8_t ui8i = 0; ui8i < ui8LockSize; ui8i++) {
        if(ui8i == 0) {
            v = (uint8_t)(sLock[0] ^ sLock[45] ^ sLock[44] ^ 5);
        } else {
            v = sLock[ui8i] ^ sLock[ui8i-1];
        }
        
        // swap nibbles (0xF0 = 11110000, 0x0F = 00001111)

        v = (uint8_t)(((v << 4) & 0xF0) | ((v >> 4) & 0x0F));
        
    	switch(v) {
            case   0:
                strcat(cKey, "/%DCN000%/");
                break;
    		case   5:
                strcat(cKey, "/%DCN005%/");
                break;
    		case  36:
                strcat(cKey, "/%DCN036%/");
                break;
			case  96:
                strcat(cKey, "/%DCN096%/");
                break;
    		case 124:
                strcat(cKey, "/%DCN124%/");
                break;
    		case 126:
                strcat(cKey, "/%DCN126%/");
                break;
    		default:
                strncat(cKey, (char *)&v, 1);
                break;
    	}
    }
    return cKey;
}
//---------------------------------------------------------------------------

#ifdef _WIN32
	char * WSErrorStr(const uint32_t ui32Error) {
		static char errStrings[][64] = {
			{"0"},{"1"},{"2"},{"3"},
			{"WSAEINTR"},
			{"5"},{"6"},{"7"},{"8"},
			{"WSAEBADF"},
			{"10"},{"11"},{"12"},
			{"WSAEACCES"},
			{"WSAEFAULT"},
			{"15"},{"16"},{"17"},{"18"},{"19"},{"20"},{"21"},
			{"WSAEINVAL"},
			{"23"},
			{"WSAEMFILE"},
			{"25"},{"26"},{"27"},{"28"},{"29"},{"30"},{"31"},{"32"},{"33"},{"34"},
	        {"WSAEWOULDBLOCK"},
	        {"WSAEINPROGRESS"},   // This error is returned if any Windows Sockets API function is called while a blocking function is in progress.
	        {"WSAEALREADY"},
	        {"WSAENOTSOCK"},
	        {"WSAEDESTADDRREQ"},
	        {"WSAEMSGSIZE"},
	        {"WSAEPROTOTYPE"},
	        {"WSAENOPROTOOPT"},
	        {"WSAEPROTONOSUPPORT"},
	        {"WSAESOCKTNOSUPPORT"},
	        {"WSAEOPNOTSUPP"},
	        {"WSAEPFNOSUPPORT"},
	        {"WSAEAFNOSUPPORT"},
	        {"WSAEADDRINUSE"},
	        {"WSAEADDRNOTAVAIL"},
	        {"WSAENETDOWN"}, // This error may be reported at any time if the Windows Sockets implementation detects an underlying failure.
	        {"WSAENETUNREACH"},
	        {"WSAENETRESET"},
	        {"WSAECONNABORTED"},
	        {"WSAECONNRESET"},
	        {"WSAENOBUFS"},
	        {"WSAEISCONN"},
	        {"WSAENOTCONN"},
	        {"WSAESHUTDOWN"},
	        {"WSAETOOMANYREFS"},
	        {"WSAETIMEDOUT"},
	        {"WSAECONNREFUSED"},
	        {"WSAELOOP"},
	        {"WSAENAMETOOLONG"},
	        {"WSAEHOSTDOWN"},
	        {"WSAEHOSTUNREACH"}, // 10065
	        {"WSASYSNOTREADY"}, // 10091
	        {"WSAVERNOTSUPPORTED"}, // 10092
	        {"WSANOTINITIALISED"}, // 10093
	        {"WSAHOST_NOT_FOUND"}, // 11001
	        {"WSATRY_AGAIN"}, // 11002
	        {"WSANO_RECOVERY"}, // 11003
	        {"WSANO_DATA"} // 11004
		};
	
		switch(ui32Error) {
			case 10091 :
		    	return errStrings[66]; // WSASYSNOTREADY
			case 10092 :
		    	return errStrings[67]; // WSAVERNOTSUPPORTED
			case 10093 :
		    	return errStrings[68]; // WSANOTINITIALISED
			case 11001 :
		    	return errStrings[69]; // WSAHOST
			case 11002 :
		    	return errStrings[70]; // WSATRY
			case 11003 :
		    	return errStrings[71]; // WSANO
			case 11004 :
		    	return errStrings[72]; // WSANO
		    default :
		    	return errStrings[ui32Error-10000];
	    }
	}
#else
	const char * ErrnoStr(const uint32_t ui32Error) {
		static const char *errStrings[] = {
	        "UNDEFINED", 
	        "EADDRINUSE", 
	        "ECONNRESET", 
	        "ETIMEDOUT", 
	        "ECONNREFUSED", 
	        "EHOSTUNREACH", 
		};
	
		switch(ui32Error) {
			case 98:
		    	return errStrings[1];
			case 104:
		    	return errStrings[2];
			case 110:
		    	return errStrings[3];
			case 111:
		    	return errStrings[4];
			case 113:
		    	return errStrings[5];
		    default :
		    	return errStrings[0];
	    }
	}
#endif
//---------------------------------------------------------------------------

char * formatBytes(const uint64_t ui64Bytes) {
    static const char *unit[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB", " ", " ", " ", " ", " ", " ", " "};
    static char sBytes[128];

   	if(ui64Bytes < 1024) {
        int iLen = snprintf(sBytes, 128, "%" PRIu64 " %s", ui64Bytes & 0xffffffff, unit[0]);
        if(iLen <= 0) {
            sBytes[0] = '\0';
        }
   	} else {
       	long double ldBytes = (long double)ui64Bytes;
        uint8_t iter = 0;
       	for(; ldBytes > 1024; iter++)
           	ldBytes /= 1024;
           	
   	    int iLen = snprintf(sBytes, 128, "%0.2Lf %s", ldBytes, unit[iter]);
   	    if(iLen <= 0) {
            sBytes[0] = '\0';
        }
    }

    return sBytes;
}
//---------------------------------------------------------------------------

char * formatBytesPerSecond(const uint64_t ui64Bytes) {
    static const char *secondunit[] = {"B/s", "kB/s", "MB/s", "GB/s", "TB/s", "PB/s", "EB/s", "ZB/s", "YB/s", " ", " ", " ", " ", " ", " ", " "};
    static char sBytes[128];

   	if(ui64Bytes < 1024) {
        int iLen = snprintf(sBytes, 128, "%" PRIu64 " %s", ui64Bytes & 0xffffffff, secondunit[0]);
        if(iLen <= 0) {
            sBytes[0] = '\0';
        }
   	} else {
       	long double ldBytes = (long double)ui64Bytes;
        uint8_t iter = 0;
       	for(; ldBytes > 1024; iter++)
           	ldBytes /= 1024;
           	
   	    int iLen = snprintf(sBytes, 128, "%0.2Lf %s", ldBytes, secondunit[iter]);
   	    if(iLen <= 0) {
            sBytes[0] = '\0';
        }
    }
	
    return sBytes;
}
//---------------------------------------------------------------------------

char * formatTime(uint64_t ui64Rest) {
    static char time[256];
    time[0] = '\0';

	char buf[128];
	uint8_t i = 0;
    uint64_t n = ui64Rest / 525600;
	ui64Rest -= n*525600;

	if(n != 0) {
		int iLen = snprintf(buf, 128, "%" PRIu64 " %s", n, n > 1 ? LanguageManager::m_Ptr->m_sTexts[LAN_YEARS_LWR] : LanguageManager::m_Ptr->m_sTexts[LAN_YEAR_LWR]);
		if(iLen <= 0) {
            time[0] = '\0';
            return time;
        }
		strcat(time, buf);
		i++;
	}

    n = ui64Rest / 43200;
	ui64Rest -= n*43200;

	if(n != 0) {       
		int iLen = snprintf(buf, 128, "%s%" PRIu64 " %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager::m_Ptr->m_sTexts[LAN_MONTHS_LWR] : LanguageManager::m_Ptr->m_sTexts[LAN_MONTH_LWR]);
		if(iLen <= 0) {
            time[0] = '\0';
            return time;
        }

		strcat(time, buf);
		i++;
	}

	n = ui64Rest / 1440;
	ui64Rest -= n*1440;

	if(n != 0) {       
		int iLen = snprintf(buf, 128, "%s%" PRIu64 " %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager::m_Ptr->m_sTexts[LAN_DAYS_LWR] : LanguageManager::m_Ptr->m_sTexts[LAN_DAY_LWR]);
		if(iLen <= 0) {
            time[0] = '\0';
            return time;
        }
		
		strcat(time, buf);
		i++;
	}

    n = ui64Rest / 60;
	ui64Rest -= n*60;

	if(n != 0) {	
		int iLen = snprintf(buf, 128, "%s%" PRIu64 " %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager::m_Ptr->m_sTexts[LAN_HOURS_LWR] : LanguageManager::m_Ptr->m_sTexts[LAN_HOUR_LWR]);
		if(iLen <= 0) {
            time[0] = '\0';
            return time;
        }

		strcat(time, buf);
		i++;
	}

	if(ui64Rest != 0) {
		int iLen = snprintf(buf, 128, "%s%" PRIu64 " %s", i > 0 ? " " : "", ui64Rest, LanguageManager::m_Ptr->m_sTexts[LAN_MIN_LWR]);
		if(iLen <= 0) {
            time[0] = '\0';
            return time;
        }
		strcat(time, buf);
	}

	return time;
}
//---------------------------------------------------------------------------

char * formatSecTime(uint64_t ui64Rest) {
    static char time[256];
    time[0] = '\0';

	char buf[128];
	uint8_t i = 0;
    uint64_t n = ui64Rest / 31536000;
	ui64Rest -= n*31536000;

	if(n != 0) {
		int iLen = snprintf(buf, 128, "%" PRIu64 " %s", n, n > 1 ? LanguageManager::m_Ptr->m_sTexts[LAN_YEARS_LWR] : LanguageManager::m_Ptr->m_sTexts[LAN_YEAR_LWR]);
		if(iLen <= 0) {
            time[0] = '\0';
            return time;
        }

		strcat(time, buf);
		i++;
	}

    n = ui64Rest / 2592000;
	ui64Rest -= n*2592000;

	if(n != 0) {
		int iLen = snprintf(buf, 128, "%s%" PRIu64 " %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager::m_Ptr->m_sTexts[LAN_MONTHS_LWR] : LanguageManager::m_Ptr->m_sTexts[LAN_MONTH_LWR]);
		if(iLen <= 0) {
            time[0] = '\0';
            return time;
        }
		
		strcat(time, buf);
		i++;
	}

	n = ui64Rest / 86400;
	ui64Rest -= n*86400;

	if(n != 0) {
		int iLen = snprintf(buf, 128, "%s%" PRIu64 " %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager::m_Ptr->m_sTexts[LAN_DAYS_LWR] : LanguageManager::m_Ptr->m_sTexts[LAN_DAY_LWR]);
		if(iLen <= 0) {
            time[0] = '\0';
            return time;
        }

		strcat(time, buf);
		i++;
	}

    n = ui64Rest / 3600;
	ui64Rest -= n*3600;

	if(n != 0) {
		int iLen = snprintf(buf, 128, "%s%" PRIu64 " %s", i > 0 ? " " : "", n, n > 1 ? LanguageManager::m_Ptr->m_sTexts[LAN_HOURS_LWR] : LanguageManager::m_Ptr->m_sTexts[LAN_HOUR_LWR]);
		if(iLen <= 0) {
            time[0] = '\0';
            return time;
        }
		
		strcat(time, buf);
		i++;
	}

	n = ui64Rest / 60;
	ui64Rest -= n*60;

	if(n != 0) {
		int iLen = snprintf(buf, 128, "%s%" PRIu64 " %s", i > 0 ? " " : "", n, LanguageManager::m_Ptr->m_sTexts[LAN_MIN_LWR]);
		if(iLen <= 0) {
            time[0] = '\0';
            return time;
        }

		strcat(time, buf);
		i++;
	}

	if(ui64Rest != 0) {
		int iLen = snprintf(buf, 128, "%s%" PRIu64 " %s", i > 0 ? " " : "", ui64Rest, LanguageManager::m_Ptr->m_sTexts[LAN_SEC_LWR]);
		if(iLen <= 0) {
            time[0] = '\0';
            return time;
        }

		strcat(time, buf);
	}
	return time;
}
//---------------------------------------------------------------------------

char* stristr(const char *str1, const char *str2) {
	char *s1, *s2;
	char *cp = (char *)str1;
	if(*str2 == 0)
		return (char *)str1;
	while(*cp != 0) {
		s1 = cp;
		s2 = (char *) str2;
		while(*s1 != 0 && *s2 != 0 && ((*s1-*s2) == 0 ||
			(*s1-tolower(*s2)) == 0 || (*s1-toupper(*s2)) == 0))
				s1++, s2++;
		if(*s2 == 0) {
			return cp;
		}
		cp++;
	}
	return NULL;
}
//---------------------------------------------------------------------------

char* stristr2(const char *str1, const char *str2) {
	char *s1, *s2;
	char *cp = (char *)str1;
	if(*str2 == 0)
		return (char *)str1;
	while(*cp != 0) {
		s1 = cp;
		s2 = (char *) str2;
		while(*s1 != 0 && *s2 != 0 && ((*s1-*s2) == 0 ||
			(tolower(*s1)-*s2) == 0))
				s1++, s2++;
		if(*s2 == 0) {
			return cp;
		}
		cp++;
	}
	return NULL;
}
//---------------------------------------------------------------------------

// check IP string.
// false - no ip
// true - valid ip
bool isIP(char * sIP) {
    if(ServerManager::m_bUseIPv6 == true && strchr(sIP, '.') == NULL) {
    	if(strlen(sIP) > 39) {
    		return false;
		}

        uint8_t ui128IpHash[16];
#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
        if(win_inet_pton(sIP, ui128IpHash) != 1) {
#else
        if(inet_pton(AF_INET6, sIP, ui128IpHash) != 1) {
#endif
            return false;
        }
    } else {
    	if(strlen(sIP) > 15) {
    		return false;
		}

        uint32_t ui32IpHash = inet_addr(sIP);

        if(ui32IpHash == INADDR_NONE) {
            return false;
        }
    }

	return true;
}
//---------------------------------------------------------------------------

uint32_t HashNick(const char * sNick, const size_t szNickLen) {
	unsigned char c = 0;
    uint32_t h = 5381;

	for(size_t szi = 0; szi < szNickLen; szi++) {
        c = (unsigned char)tolower(sNick[szi]);
        h += (h << 5);
        h ^= c;
    }

	return (h+1);
}
//---------------------------------------------------------------------------

bool HashIP(const char * sIP, uint8_t * ui128IpHash) {
    if(ServerManager::m_bUseIPv6 == true && strchr(sIP, '.') == NULL) {
    	if(strlen(sIP) > 39) {
    		return false;
		}

#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
        if(win_inet_pton(sIP, ui128IpHash) != 1) {
#else
        if(inet_pton(AF_INET6, sIP, ui128IpHash) != 1) {
#endif
            return false;
        }
    } else {
    	if(strlen(sIP) > 15) {
    		return false;
		}

        uint32_t ui32IpHash = inet_addr(sIP);

        if(ui32IpHash == INADDR_NONE) {
            return false;
        }

        memset(ui128IpHash, 0, 16);

        ui128IpHash[10] = 255;
        ui128IpHash[11] = 255;

        memcpy(ui128IpHash+12, &ui32IpHash, 4);
    }

	return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

uint16_t GetIpTableIdx(const uint8_t * ui128IpHash) {
	unsigned char c = 0;
    uint32_t h = 5381;

	for(uint8_t ui8i = 0; ui8i < 16; ui8i++) {
        c = (unsigned char)ui128IpHash[ui8i];
        h += (h << 5);
        h ^= c;
    }

    h += 1;

    uint16_t ui16Idx = 0;
    memcpy(&ui16Idx, &h, 2);

	return ui16Idx;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int GenerateBanMessage(BanItem * pBan, const time_t &tAccTime) {
	int iMsgLen = 0;

    if(((pBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM) == true) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> %s.", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SORRY_PERM_BANNED]);

    } else {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> %s: %s.", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SORRY_TEMP_BANNED], formatSecTime(pBan->m_tTempBanExpire-tAccTime));

    }

	if(iMsgLen <= 0) {
		AppendDebugLogFormat("[ERR] snprintf wrong value %d in GenerateBanMessage1\n", iMsgLen);

		return 0;
	}


    if(SettingManager::m_Ptr->m_bBools[SETBOOL_BAN_MSG_SHOW_IP] == true && pBan->m_sIp[0] != '\0') {
        int iLen = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_IP], pBan->m_sIp);
		if(iLen <= 0) {
			AppendDebugLogFormat("[ERR] snprintf wrong value %d in GenerateBanMessage2\n", iLen);
	
			return 0;
		}

        iMsgLen += iLen;
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_BAN_MSG_SHOW_NICK] == true && pBan->m_sNick != NULL) {
        int iLen = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_NICK], pBan->m_sNick);
		if(iLen <= 0) {
			AppendDebugLogFormat("[ERR] snprintf wrong value %d in GenerateBanMessage3\n", iLen);
	
			return 0;
		}

        iMsgLen += iLen;
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_BAN_MSG_SHOW_REASON] == true && pBan->m_sReason != NULL) {
        int iLen = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_REASON], pBan->m_sReason);
		if(iLen <= 0) {
			AppendDebugLogFormat("[ERR] snprintf wrong value %d in GenerateBanMessage4\n", iLen);
	
			return 0;
		}

        iMsgLen += iLen;
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_BAN_MSG_SHOW_BY] == true && pBan->m_sBy != NULL) {
        int iLen = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_BY], pBan->m_sBy);
		if(iLen <= 0) {
			AppendDebugLogFormat("[ERR] snprintf wrong value %d in GenerateBanMessage5\n", iLen);
	
			return 0;
		}

        iMsgLen += iLen;
    }

    if(SettingManager::m_Ptr->m_sTexts[SETTXT_MSG_TO_ADD_TO_BAN_MSG] != NULL) {
        int iLen = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s|", SettingManager::m_Ptr->m_sTexts[SETTXT_MSG_TO_ADD_TO_BAN_MSG]);
		if(iLen <= 0) {
			AppendDebugLogFormat("[ERR] snprintf wrong value %d in GenerateBanMessage6\n", iLen);
	
			return 0;
		}

        iMsgLen += iLen;
    } else {
        ServerManager::m_pGlobalBuffer[iMsgLen] = '|';
        iMsgLen++;
        ServerManager::m_pGlobalBuffer[iMsgLen] = '\0';
    }

    if(((pBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM) == true) {
        if(SettingManager::m_Ptr->m_bBools[SETBOOL_PERM_BAN_REDIR] == true && SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_PERM_BAN_REDIR_ADDRESS] != NULL) {
        	strcpy(ServerManager::m_pGlobalBuffer+iMsgLen, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_PERM_BAN_REDIR_ADDRESS]);
            iMsgLen += (int)SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_PERM_BAN_REDIR_ADDRESS];
        }
    } else {
        if(SettingManager::m_Ptr->m_bBools[SETBOOL_TEMP_BAN_REDIR] == true && SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_TEMP_BAN_REDIR_ADDRESS] != NULL) {
        	strcpy(ServerManager::m_pGlobalBuffer+iMsgLen, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_TEMP_BAN_REDIR_ADDRESS]);
            iMsgLen += (int)SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_TEMP_BAN_REDIR_ADDRESS];
        }
    }

    return iMsgLen;
}
//---------------------------------------------------------------------------

int GenerateRangeBanMessage(RangeBanItem * pRangeBan, const time_t &tAccTime) {
    int iMsgLen = 0;

    if(((pRangeBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM) == true) {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> %s.", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SORRY_PERM_BANNED]);
    } else {
        iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "<%s> %s: %s", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SORRY_TEMP_BANNED], formatSecTime(pRangeBan->m_tTempBanExpire-tAccTime));
    }

	if(iMsgLen <= 0) {
		AppendDebugLogFormat("[ERR] snprintf wrong value %d in GenerateRangeBanMessage1\n", iMsgLen);

		return 0;
	}

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_BAN_MSG_SHOW_RANGE] == true) {
        int iLen = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s-%s", LanguageManager::m_Ptr->m_sTexts[LAN_RANGE], pRangeBan->m_sIpFrom, pRangeBan->m_sIpTo);
		if(iLen <= 0) {
			AppendDebugLogFormat("[ERR] snprintf wrong value %d in GenerateRangeBanMessage2\n", iLen);
	
			return 0;
		}

        iMsgLen += iLen;
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_BAN_MSG_SHOW_REASON] == true && pRangeBan->m_sReason != NULL) {
        int iLen = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_REASON], pRangeBan->m_sReason);
		if(iLen <= 0) {
			AppendDebugLogFormat("[ERR] snprintf wrong value %d in GenerateRangeBanMessage3\n", iLen);
	
			return 0;
		}

        iMsgLen += iLen;
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_BAN_MSG_SHOW_BY] == true && pRangeBan->m_sBy != NULL) {
        int iLen = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s: %s", LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_BY], pRangeBan->m_sBy);
		if(iLen <= 0) {
			AppendDebugLogFormat("[ERR] snprintf wrong value %d in GenerateRangeBanMessage4\n", iLen);
	
			return 0;
		}

        iMsgLen += iLen;
    }

    if(SettingManager::m_Ptr->m_sTexts[SETTXT_MSG_TO_ADD_TO_BAN_MSG] != NULL) {
        int iLen = snprintf(ServerManager::m_pGlobalBuffer+iMsgLen, ServerManager::m_szGlobalBufferSize-iMsgLen, "\n%s|", SettingManager::m_Ptr->m_sTexts[SETTXT_MSG_TO_ADD_TO_BAN_MSG]);
		if(iLen <= 0) {
			AppendDebugLogFormat("[ERR] snprintf wrong value %d in GenerateRangeBanMessage5\n", iLen);
	
			return 0;
		}

        iMsgLen += iLen;
    } else {
        ServerManager::m_pGlobalBuffer[iMsgLen] = '|';
        iMsgLen++;
        ServerManager::m_pGlobalBuffer[iMsgLen] = '\0';
    }

    if(((pRangeBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM) == true) {
        if(SettingManager::m_Ptr->m_bBools[SETBOOL_PERM_BAN_REDIR] == true && SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_PERM_BAN_REDIR_ADDRESS] != NULL) {
            strcpy(ServerManager::m_pGlobalBuffer+iMsgLen, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_PERM_BAN_REDIR_ADDRESS]);
            iMsgLen += (int)SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_PERM_BAN_REDIR_ADDRESS];
        }
    } else {
        if(SettingManager::m_Ptr->m_bBools[SETBOOL_TEMP_BAN_REDIR] == true && SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_TEMP_BAN_REDIR_ADDRESS] != NULL) {
            strcpy(ServerManager::m_pGlobalBuffer+iMsgLen, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_TEMP_BAN_REDIR_ADDRESS]);
            iMsgLen += (int)SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_TEMP_BAN_REDIR_ADDRESS];
        }
    }

    return iMsgLen;
}
//---------------------------------------------------------------------------

bool GenerateTempBanTime(const uint8_t ui8Multiplyer, const uint32_t ui32Time, time_t &tAccTime, time_t &tBanTime) {
    time(&tAccTime);
    struct tm *tm = localtime(&tAccTime);

    switch(ui8Multiplyer) {
        case 'm':
            tm->tm_min += ui32Time;
            break;
        case 'h':
            tm->tm_hour += ui32Time;
            break;
        case 'd':
            tm->tm_mday += ui32Time;
            break;
        case 'w':
            tm->tm_mday += ui32Time*7;
            break;
        case 'M':
            tm->tm_mon += ui32Time;
            break;
        case 'Y':
            tm->tm_year += ui32Time;
            break;
        default:
            return false;
    }

    tm->tm_isdst = -1;

    tBanTime = mktime(tm);

    if(tBanTime == (time_t)-1) {
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------

bool HaveOnlyNumbers(char *sData, const uint16_t ui16Len) {
    for(uint16_t ui16i = 0; ui16i < ui16Len; ui16i++) {
        if(isdigit(sData[ui16i]) == 0)
            return false;
    }
    return true;
}
//---------------------------------------------------------------------------

void AppendLog(const char * sData, const bool bScript/* == false*/) {
	FILE * fw;

	if(bScript == false) {
#ifdef _WIN32
        fw = fopen((ServerManager::m_sPath + "\\logs\\system.log").c_str(), "a");
#else
		fw = fopen((ServerManager::m_sPath + "/logs/system.log").c_str(), "a");
#endif
    } else {
#ifdef _WIN32
        fw = fopen((ServerManager::m_sPath + "\\logs\\script.log").c_str(), "a");
#else
		fw = fopen((ServerManager::m_sPath + "/logs/script.log").c_str(), "a");
#endif
    }

	if(fw != NULL) {
	   time_t acc_time;
	   time(&acc_time);

	   struct tm * acc_tm;
	   acc_tm = localtime(&acc_time);

	   char sBuf[64];
	   strftime(sBuf, 64, "%c", acc_tm);

	   fprintf(fw, "%s - %s\n", sBuf, sData);

	   fclose(fw);
	}

    if(UdpDebug::m_Ptr != NULL && bScript == false) {
		UdpDebug::m_Ptr->BroadcastFormat("[LOG] %s", sData);
    }
}
//---------------------------------------------------------------------------

void AppendDebugLog(const char * sData) {
#ifdef _WIN32
	FILE * fw = fopen((ServerManager::m_sPath + "\\logs\\debug.log").c_str(), "a");
#else
	FILE * fw = fopen((ServerManager::m_sPath + "/logs/debug.log").c_str(), "a");
#endif

	if(fw == NULL) {
		return;
	}

	time_t acc_time;
	time(&acc_time);

	struct tm * acc_tm;
	acc_tm = localtime(&acc_time);

	char sBuf[64];
	strftime(sBuf, 64, "%c", acc_tm);

    fprintf(fw, sData, sBuf); // "%s - xxx\n"

	fclose(fw);
}
//---------------------------------------------------------------------------

void AppendDebugLogFormat(const char * sFormatMsg, ...) {
#ifdef _WIN32
	FILE * fw = fopen((ServerManager::m_sPath + "\\logs\\debug.log").c_str(), "a");
#else
	FILE * fw = fopen((ServerManager::m_sPath + "/logs/debug.log").c_str(), "a");
#endif

	if(fw == NULL) {
		return;
	}

	time_t tmAccTime;
	time(&tmAccTime);

	size_t szLen = strftime(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%c - ", localtime(&tmAccTime));

	if(szLen != 0) {
		fwrite(ServerManager::m_pGlobalBuffer, 1, szLen, fw);
	}

	va_list vlArgs;
	va_start(vlArgs, sFormatMsg);

	vfprintf(fw, sFormatMsg, vlArgs);

	va_end(vlArgs);

	fclose(fw);
}
//---------------------------------------------------------------------------
#ifdef _WIN32
	void GetHeapStats(void * pHeap, DWORD &dwCommitted, DWORD &dwUnCommitted) {
	    PROCESS_HEAP_ENTRY *lpEntry;
	
	    lpEntry = (PROCESS_HEAP_ENTRY *)calloc(1, sizeof(PROCESS_HEAP_ENTRY));
	    if(lpEntry == NULL) {
            return;
        }

	    lpEntry->lpData = NULL;
	
	    while(HeapWalk((HANDLE)pHeap, (PROCESS_HEAP_ENTRY *)lpEntry) != 0) {
	        if((lpEntry->wFlags & PROCESS_HEAP_REGION)) {
	            dwCommitted += lpEntry->Region.dwCommittedSize;
	            dwUnCommitted += lpEntry->Region.dwUnCommittedSize;
	        }
	    }
	
	    free(lpEntry);
	}
	//---------------------------------------------------------------------------

	char * ExtractFileName(char * sPath) {
		char * sName = strrchr(sPath, '\\');
	
		if(sName != NULL) {
			return sName;
		} else {
	        return sPath;
	    }
	}
#endif
//---------------------------------------------------------------------------

#ifdef _BUILD_GUI
	void Memo(const string &sMessage) {
        RichEditAppendText(MainWindowPageUsersChat::m_Ptr->m_hWndPageItems[MainWindowPageUsersChat::REDT_CHAT], sMessage.c_str());
    }
#else
	void Memo(const string &/*sMessage*/) {
	    // ...
	}
#endif
//---------------------------------------------------------------------------

bool FileExist(char * sPath) {
#ifdef _WIN32
	DWORD code = GetFileAttributes(sPath);
	if(code != INVALID_FILE_ATTRIBUTES && code != FILE_ATTRIBUTE_DIRECTORY) {
#else
    struct stat st;
	if(stat(sPath, &st) == 0 && S_ISDIR(st.st_mode) == 0) {
#endif
		return true;
	}

	return false;
}
//---------------------------------------------------------------------------

bool DirExist(char * sPath) {
#ifdef _WIN32
	DWORD code = GetFileAttributes(sPath);
	if(code == FILE_ATTRIBUTE_DIRECTORY) {
#else
    struct stat st;
	if(stat(sPath, &st) == 0 && S_ISDIR(st.st_mode)) {
#endif
		return true;
	}

	return false;
}
//---------------------------------------------------------------------------

#ifdef _WIN32
	void SetupOsVersion() {
		OSVERSIONINFOEX ver;
		memset(&ver, 0, sizeof(OSVERSIONINFOEX));
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

		if(::GetVersionEx((OSVERSIONINFO*)&ver) == 0) {
			ServerManager::m_sOS = "Windows (unknown version)";

			return;
		}

		if(ver.dwMajorVersion == 10) {
			if(ver.dwMinorVersion == 0) {
                ServerManager::m_sOS = "Windows 10";
                return;
            }
	    } else if(ver.dwMajorVersion == 6) {
            if(ver.dwMinorVersion == 3) {
                ServerManager::m_sOS = "Windows 8.1";
                return;
            } else if(ver.dwMinorVersion == 2) {
                ServerManager::m_sOS = "Windows 8";
                return;
            } else if(ver.dwMinorVersion == 1) {
	           if(ver.wProductType == VER_NT_WORKSTATION) {
	               ServerManager::m_sOS = "Windows 7";
	               return;
	           } else {
	               ServerManager::m_sOS = "Windows 2008 R2";
	               return;
	           }
            } else {
	           if(ver.wProductType == VER_NT_WORKSTATION) {
	               ServerManager::m_sOS = "Windows Vista";
	               return;
	           } else {
	               ServerManager::m_sOS = "Windows 2008";
	               return;
	           }
            }
		} else if(ver.dwMajorVersion == 5) {
	        if(ver.dwMinorVersion == 2) {
                if(ver.wProductType != VER_NT_WORKSTATION) {
				    ServerManager::m_sOS = "Windows 2003";
				    return;
                } else {
                    SYSTEM_INFO si;
                    memset(&si, 0, sizeof(SYSTEM_INFO));

                    ::GetNativeSystemInfo(&si);

                    if(si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64) {
                        ServerManager::m_sOS = "Windows XP x64";
                        return;
                    }
                }

                // should not happen, but for sure...
                ServerManager::m_sOS = "Windows 2003/XP64";
                return;
			} else if(ver.dwMinorVersion == 1) {
				ServerManager::m_sOS = "Windows XP";
				return;
			}
		}

		ServerManager::m_sOS = "Windows (unknown version)";
	}
//---------------------------------------------------------------------------

    void * LuaAlocator(void * /*pOld*/, void * pData, const size_t /*szOldSize*/, const size_t szNewSize) {
        if(szNewSize == 0) {
            if(pData != NULL) {
                ::HeapFree(ServerManager::m_hLuaHeap, 0, pData);
            }

            return NULL;
        } else {
            if(pData != NULL) {
                return ::HeapReAlloc(ServerManager::m_hLuaHeap, 0, pData, szNewSize);
            } else  {
                return ::HeapAlloc(ServerManager::m_hLuaHeap, 0, szNewSize);
            }
        }
    }
#endif
//---------------------------------------------------------------------------

#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
	typedef INT (WSAAPI * pInetPton)(INT, PCTSTR, PVOID);
	pInetPton MyInetPton = NULL;
	typedef PCTSTR (WSAAPI *pInetNtop)(INT, PVOID, PTSTR, size_t);
	pInetNtop MyInetNtop = NULL;
#endif
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void CheckForIPv4() {
#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(sock == INVALID_SOCKET) {
        int iError = WSAGetLastError();
        if(iError == WSAEAFNOSUPPORT) {
#else
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock == -1) {
        if(errno == EAFNOSUPPORT) {
#endif
            ServerManager::m_bUseIPv4 = false;
            return;
        }
    }

#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void CheckForIPv6() {
#ifdef _WIN32
    SOCKET sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if(sock == INVALID_SOCKET) {
        int iError = WSAGetLastError();
        if(iError == WSAEAFNOSUPPORT) {
#else
    int sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if(sock == -1) {
        if(errno == EAFNOSUPPORT) {
#endif
            ServerManager::m_bUseIPv6 = false;
            return;
        }
    }

#ifdef _WIN32
    DWORD dwIPv6 = 0;
    if(setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&dwIPv6, sizeof(dwIPv6)) != SOCKET_ERROR) {
#else
    int iIPv6 = 0;
    if(setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &iIPv6, sizeof(iIPv6)) != -1) {
#endif
        ServerManager::m_bIPv6DualStack = true;
    }

#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif

#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
    HINSTANCE hWs2_32 = ::LoadLibrary("Ws2_32.dll");

    MyInetPton = (pInetPton)::GetProcAddress(hWs2_32, "inet_pton");
    MyInetNtop = (pInetNtop)::GetProcAddress(hWs2_32, "inet_ntop");

    ::FreeLibrary(hWs2_32);
#endif
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
INT win_inet_pton(PCTSTR pAddrString, PVOID pAddrBuf) {
    if(MyInetPton != NULL) {
        return MyInetPton(AF_INET6, pAddrString, pAddrBuf);
    } else {
        sockaddr_storage sas_addr;
        socklen_t sas_len = sizeof(sockaddr_storage);
        memset(&sas_addr, 0, sas_len);

        if(::WSAStringToAddressA((LPSTR)pAddrString, AF_INET6, NULL, (struct sockaddr *)&sas_addr, &sas_len) == 0) {
            memcpy(pAddrBuf, &((struct sockaddr_in6 *)&sas_addr)->sin6_addr.s6_addr, 16);
            return 1;
        } else {
            return 0;
        }
    }
}
//---------------------------------------------------------------------------

void win_inet_ntop(PVOID pAddr, PTSTR pStringBuf, size_t szStringBufSize) {
    if(MyInetNtop != NULL) {
        MyInetNtop(AF_INET6, pAddr, pStringBuf, szStringBufSize);
    } else {
        struct sockaddr_in6 sin6;
        socklen_t sin6_len = sizeof(sockaddr_in6);

        memset(&sin6, 0, sin6_len);
        sin6.sin6_family = AF_INET6;
        memcpy(&sin6.sin6_addr, pAddr, 16);

        ::WSAAddressToStringA((struct sockaddr *)&sin6, sin6_len, NULL, pStringBuf, (LPDWORD)&szStringBufSize);
    }
}
//---------------------------------------------------------------------------
#endif

bool GetMacAddress(const char * sIP, char * sMac) {
#ifdef _WIN32
    uint32_t uiIP = ::inet_addr(sIP);

    MIB_IPNETTABLE * pINT = (MIB_IPNETTABLE *)new (std::nothrow) char[131072];
    if(pINT == NULL) {
        return false;
    }

    ULONG ulSize = 131072;
    DWORD dwRes = ::GetIpNetTable(pINT, &ulSize, TRUE);
    if(dwRes == NO_ERROR) {
        for(DWORD dwi = 0; dwi < pINT->dwNumEntries; dwi++) {
            if(pINT->table[dwi].dwAddr == uiIP && pINT->table[dwi].dwType != MIB_IPNET_TYPE_INVALID) {
                snprintf(sMac, 18, "%02x:%02x:%02x:%02x:%02x:%02x", pINT->table[dwi].bPhysAddr[0], pINT->table[dwi].bPhysAddr[1], pINT->table[dwi].bPhysAddr[2],
                    pINT->table[dwi].bPhysAddr[3], pINT->table[dwi].bPhysAddr[4], pINT->table[dwi].bPhysAddr[5]);
                delete []pINT;
                return true;
            }
        }
    }
    delete []pINT;
#else
    FILE *fp = fopen("/proc/net/arp", "r");
    if(fp != NULL) {
        bool bLastCharSpace = true;
        uint8_t ui8NonSpaces = 0;
        uint16_t ui16IpLen = (uint16_t)strlen(sIP);
        char buf[1024];

        while(fgets(buf, 1024, fp) != NULL) {
            if(strncmp(buf, sIP, ui16IpLen) == 0 && buf[ui16IpLen] == ' ') {
                bLastCharSpace = true;
                ui8NonSpaces = 0;
                while(buf[ui16IpLen] != '\0') {
                    if(buf[ui16IpLen] == ' ') {
                        bLastCharSpace = true;
                    } else {
                        if(bLastCharSpace == true) {
                            bLastCharSpace = false;
                            ui8NonSpaces++;
                        }
                    }

                    if(ui8NonSpaces == 3) {
                        strncpy(sMac, buf + ui16IpLen, 17);
                        sMac[17] = '\0';
                        fclose(fp);
                        return true;
                    }

                    ui16IpLen++;
                }
            }
        }

        fclose(fp);
    }
#endif

    return false;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void CreateGlobalBuffer() {
    ServerManager::m_szGlobalBufferSize = 131072;
#ifdef _WIN32
    ServerManager::m_pGlobalBuffer = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ServerManager::m_szGlobalBufferSize);
#else
    ServerManager::m_pGlobalBuffer = (char *)calloc(ServerManager::m_szGlobalBufferSize, 1);
#endif
    if(ServerManager::m_pGlobalBuffer == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create ServerManager::m_pGlobalBuffer\n");
		exit(EXIT_FAILURE);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void DeleteGlobalBuffer() {
#ifdef _WIN32
    if(ServerManager::m_pGlobalBuffer != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)ServerManager::m_pGlobalBuffer) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate ServerManager::m_pGlobalBuffer\n");
        }
    }
#else
	free(ServerManager::m_pGlobalBuffer);
#endif
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool CheckAndResizeGlobalBuffer(const size_t szWantedSize) {
    if(ServerManager::m_szGlobalBufferSize >= szWantedSize) {
        return true;
    }

    size_t szOldSize = ServerManager::m_szGlobalBufferSize;
    char * sOldBuf = ServerManager::m_pGlobalBuffer;

    ServerManager::m_szGlobalBufferSize = Allign128K(szWantedSize);

#ifdef _WIN32
    ServerManager::m_pGlobalBuffer = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldBuf, ServerManager::m_szGlobalBufferSize);
#else
    ServerManager::m_pGlobalBuffer = (char *)realloc(sOldBuf, ServerManager::m_szGlobalBufferSize);
#endif
    if(ServerManager::m_pGlobalBuffer == NULL) {
        ServerManager::m_pGlobalBuffer = sOldBuf;

		AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes in CheckAndResizeGlobalBuffer for ServerManager::m_pGlobalBuffer\n", ServerManager::m_szGlobalBufferSize);

        ServerManager::m_szGlobalBufferSize = szOldSize;
        return false;
	}

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReduceGlobalBuffer() {
    if(ServerManager::m_szGlobalBufferSize == 131072) {
        return;
    }

    size_t szOldSize = ServerManager::m_szGlobalBufferSize;
    char * sOldBuf = ServerManager::m_pGlobalBuffer;

    ServerManager::m_szGlobalBufferSize = 131072;

#ifdef _WIN32
    ServerManager::m_pGlobalBuffer = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)sOldBuf, ServerManager::m_szGlobalBufferSize);
#else
    ServerManager::m_pGlobalBuffer = (char *)realloc(sOldBuf, ServerManager::m_szGlobalBufferSize);
#endif
    if(ServerManager::m_pGlobalBuffer == NULL) {
        ServerManager::m_pGlobalBuffer = sOldBuf;

		AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes in ReduceGlobalBuffer for ServerManager::m_pGlobalBuffer\n", ServerManager::m_szGlobalBufferSize);

        ServerManager::m_szGlobalBufferSize = szOldSize;
        return;
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool HashPassword(char * sPassword, const size_t szPassLen, uint8_t * ui8PassHash) {
#ifndef _WITHOUT_SKEIN
    Skein1024_Ctxt_t ctx1024;

    if(Skein1024_Init(&ctx1024, 512) == SKEIN_SUCCESS) {
        if(Skein1024_Update(&ctx1024, (u08b_t *)sPassword, szPassLen) == SKEIN_SUCCESS) {
            if(Skein1024_Final(&ctx1024, ui8PassHash) == SKEIN_SUCCESS) {
                return true;
            }
        }
    }
#endif
    return false;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#ifdef _WIN32
static const uint16_t ui16Num = 42; // Answer to the Ultimate Question
static const uint8_t ui8Num = 42; //  of Life, The Universe, and Everything

uint64_t htobe64(const uint64_t & ui64Value) {
	if(*(uint8_t *)&ui16Num == ui8Num) { // LE
		return _byteswap_uint64(ui64Value);
	} else { // BE
		return ui64Value;
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

uint64_t be64toh(const uint64_t & ui64Value) {
	if(*(uint8_t *)&ui16Num == ui8Num) { // LE
		return _byteswap_uint64(ui64Value);
	} else { // BE
		return ui64Value;
	}
}
#endif
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool WantAgain() {
	printf("Do you want to try it again (Y/n) ? ");
	int iChar = getchar();
	
	while(getchar() != '\n') {
		// boredom...
	};

	if(toupper(iChar) == 'Y') {
		return true;
	}

	return false;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool IsPrivateIP(const char * sIP) {
    if(ServerManager::m_bUseIPv6 == true && strchr(sIP, '.') == NULL) {
        uint8_t ui128IpHash[16];
#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
        if(win_inet_pton(sIP, ui128IpHash) != 1) {
#else
        if(inet_pton(AF_INET6, sIP, ui128IpHash) != 1) {
#endif
            return false;
        }

		if(IN6_IS_ADDR_LOOPBACK((in6_addr *)ui128IpHash) || IN6_IS_ADDR_LINKLOCAL((in6_addr *)ui128IpHash) || IN6_IS_ADDR_SITELOCAL((in6_addr *)ui128IpHash)) {
			return true;
		}
    } else {
        uint32_t ui32IpHash = inet_addr(sIP);

        if(ui32IpHash == INADDR_NONE) {
            return false;
        }

		uint8_t ui32IP[4];
		memcpy(ui32IP, &ui32IpHash, 4);

		if(ui32IP[0] == 10 || ui32IP[0] == 127 || (ui32IP[0] == 169 && ui32IP[1] == 254) || (ui32IP[0] == 172 && ui32IP[1] == 16) || (ui32IP[0] == 192 && ui32IP[1] == 168)) {
			return true;
		}
    }

	return false;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
