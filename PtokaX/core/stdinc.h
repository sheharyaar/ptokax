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
#ifndef stdincH
#define stdincH
//---------------------------------------------------------------------------
#ifndef _WIN32
	#define _REENTRANT 1
	#define __STDC_FORMAT_MACROS 1
	#define __STDC_LIMIT_MACROS 1
#endif
//---------------------------------------------------------------------------
#include <stdlib.h>
#ifdef _WIN32
	#include <malloc.h>
#endif
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <new>
#include <stdint.h>
#include <stdarg.h>
#ifdef _WIN32
	#include <dos.h>

	#pragma warning(disable: 4996) // Deprecated stricmp

	#include <winsock2.h>
	#include <ws2tcpip.h>
	#ifdef _BUILD_GUI
		#include <commctrl.h>
		#include <RichEdit.h>
		#include <Windowsx.h>
	#endif
#endif
#include <locale.h>
#include <time.h>
#include <inttypes.h>
#ifdef _WIN32
	#include <process.h>
#else
	#include <unistd.h>
	#include <errno.h>
	#include <dirent.h>
	#include <limits.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <pthread.h>
	#include <signal.h>
	#include <sys/ioctl.h>
	#if defined(__SVR4) && defined(__sun)
	   #include <sys/filio.h>
	#endif
	#include <sys/resource.h> 
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/stat.h>
	#include <sys/utsname.h>
	#include <syslog.h>
	#include <iconv.h>
#endif
#include <fcntl.h>
#ifdef _WIN32
	#define TIXML_USE_STL
#endif
#include <tinyxml.h>
#ifdef _WIN32
	#include <psapi.h>
	#include <io.h>
	#include <Iphlpapi.h>
#elif __MACH__
	#include <mach/clock.h>
	#include <mach/mach.h>
	#include <libkern/OSByteOrder.h>

	#define htobe64(x) OSSwapHostToBigInt64(x)
	#define be64toh(x) OSSwapBigToHostInt64(x)
#elif __HAIKU__
	#include <support/ByteOrder.h>
	#define htobe64(x) B_HOST_TO_BENDIAN_INT64(x)
	#define be64toh(x) B_BENDIAN_TO_HOST_INT64(x)
#elif defined(__SVR4) && defined(__sun)
	#define htobe64(x) htonll(x)
	#define be64toh(x) ntohll(x)
#elif defined(__NetBSD__) || defined(__FreeBSD__)
	#include <sys/endian.h>
#else // Linux, OpenBSD
	#include <endian.h>
#endif
#include "pxstring.h"
//---------------------------------------------------------------------------
#define PtokaXVersionString "0.5.2.2"
#define BUILD_NUMBER "556"
const char g_sPtokaXTitle[] = "PtokaX DC Hub " PtokaXVersionString
#ifdef _PtokaX_TESTING_
	" [build " BUILD_NUMBER "]";
#else
	;
#endif

#ifdef _WIN32
    #define strcasecmp stricmp
    #define strncasecmp strnicmp
#endif
//---------------------------------------------------------------------------

#endif
