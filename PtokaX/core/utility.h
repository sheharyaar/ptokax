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
#ifndef utilityH
#define utilityH
//---------------------------------------------------------------------------
struct BanItem;
struct RangeBanItem;
//---------------------------------------------------------------------------

void Cout(const string & sMsg);
//---------------------------------------------------------------------------

#ifdef _WIN32
	static void preD(char *pat, int M, int D[]);
	static void suffixes(char *pat, int M, int *suff);
	static void preDD(char *pat, int M, int DD[]);
		
	int BMFind(char *text, int N, char *pat, int M);
#endif

char * Lock2Key(char * sLock);

#ifdef _WIN32
	char * WSErrorStr(const uint32_t ui32Error);
#else
	const char * ErrnoStr(const uint32_t ui32Error);
#endif

char * formatBytes(const uint64_t ui64Bytes);
char * formatBytesPerSecond(const uint64_t ui64Bytes);
char * formatTime(uint64_t ui64Rest);
char * formatSecTime(uint64_t ui64Rest);

char * stristr(const char *str1, const char *str2);
char * stristr2(const char *str1, const char *str2);

bool isIP(char * sIP);

uint32_t HashNick(const char * sNick, const size_t szNickLen);

bool HashIP(const char * sIP, uint8_t * ui128IpHash);
uint16_t GetIpTableIdx(const uint8_t * ui128IpHash);

int GenerateBanMessage(BanItem * pBan, const time_t &tAccTime);
int GenerateRangeBanMessage(RangeBanItem * pRangeBan, const time_t &tAccTime);

bool GenerateTempBanTime(const uint8_t ui8Multiplyer, const uint32_t ui32Time, time_t &tAccTime, time_t &tBanTime);

bool HaveOnlyNumbers(char * sData, const uint16_t ui16Len);

inline size_t Allign256(size_t n) { return ((n+1) & 0xFFFFFF00) + 0x100; }
inline size_t Allign512(size_t n) { return ((n+1) & 0xFFFFFE00) + 0x200; }
inline size_t Allign1024(size_t n) { return ((n+1) & 0xFFFFFC00) + 0x400; }
inline size_t Allign16K(size_t n) { return ((n+1) & 0xFFFFC000) + 0x4000; }
inline size_t Allign128K(size_t n) { return ((n+1) & 0xFFFE0000) + 0x20000; }

void AppendLog(const char * sData, const bool bScript = false);
void AppendDebugLog(const char * sData);
void AppendDebugLogFormat(const char * sFormatMsg, ...);

#ifdef _WIN32
	void GetHeapStats(void * pHeap, DWORD &dwCommitted, DWORD &dwUnCommitted);
#endif

void Memo(const string & sMessage);

#ifdef _WIN32
	char * ExtractFileName(char * sPath);
#endif

bool FileExist(char * sPath);
bool DirExist(char * sPath);

#ifdef _WIN32
	void SetupOsVersion();
	void * LuaAlocator(void * pOld, void * pData, const size_t szOldSize, const size_t szNewSize);
	#if !defined(_WIN64) && !defined(_WIN_IOT)
    	INT win_inet_pton(PCTSTR pAddrString, PVOID pAddrBuf);
    	void win_inet_ntop(PVOID pAddr, PTSTR pStringBuf, size_t szStringBufSize);
    #endif
#endif

void CheckForIPv4();
void CheckForIPv6();

bool GetMacAddress(const char * sIP, char * sMac);

void CreateGlobalBuffer();
void DeleteGlobalBuffer();
bool CheckAndResizeGlobalBuffer(const size_t szWantedSize);
void ReduceGlobalBuffer();

bool HashPassword(char * sPassword, const size_t szPassLen, uint8_t * ui8PassHash);

#ifdef _WIN32
	uint64_t htobe64(const uint64_t & ui64Value);
	uint64_t be64toh(const uint64_t & ui64Value);
#endif

bool WantAgain();
bool IsPrivateIP(const char * sIP);
//---------------------------------------------------------------------------

#endif
