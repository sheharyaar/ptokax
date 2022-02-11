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

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "stdinc.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "TextConverter.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "utility.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
TextConverter * TextConverter::m_Ptr = NULL;
#ifdef _WIN32
	static wchar_t wcTempBuf[2048];
#else
	#ifndef ICONV_CONST
		#if defined(__NetBSD__) || (defined(__sun) && defined(__SVR4))
			#define ICONV_CONST const
		#else
			#define ICONV_CONST
		#endif
	#endif
#endif
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

TextConverter::TextConverter() {
#ifndef _WIN32
	if(SettingManager::m_Ptr->m_sTexts[SETTXT_ENCODING] == NULL) {
		AppendLog("TextConverter failed to initialize - TextEncoding not set!");
		exit(EXIT_FAILURE);
	}

	m_iconvUtfCheck = iconv_open("utf-8", "utf-8");
	if(m_iconvUtfCheck == (iconv_t)-1) {
		AppendLog("TextConverter iconv_open for m_iconvUtfCheck failed!");
		exit(EXIT_FAILURE);
	}

	m_iconvAsciiToUtf = iconv_open("utf-8//TRANSLIT//IGNORE", SettingManager::m_Ptr->m_sTexts[SETTXT_ENCODING]);
	if(m_iconvAsciiToUtf == (iconv_t)-1) {
		AppendLog("TextConverter iconv_open for m_iconvAsciiToUtf failed!");
		exit(EXIT_FAILURE);
	}
#endif
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
TextConverter::~TextConverter() {
#ifndef _WIN32
	iconv_close(m_iconvUtfCheck);
	iconv_close(m_iconvAsciiToUtf);
#endif
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#ifdef _WIN32
bool TextConverter::CheckUtf8Validity(char * sInput, const uint8_t ui8InputLen, char * /*sOutput*/, const uint8_t /*ui8OutputSize*/) {
	if(::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, sInput, ui8InputLen, NULL, 0) == 0) {
		return false;
#else
bool TextConverter::CheckUtf8Validity(char * sInput, const uint8_t ui8InputLen, char * sOutput, const uint8_t ui8OutputSize) {
	char * sInBuf = sInput;
	size_t szInbufLeft = ui8InputLen;

	char * sOutBuf = sOutput;
	size_t szOutbufLeft = ui8OutputSize-1;

	size_t szRet = iconv(m_iconvUtfCheck, (ICONV_CONST char**)&sInBuf, &szInbufLeft, &sOutBuf, &szOutbufLeft);
	if(szRet == (size_t)-1) {
		return false;
#endif
	} else {
		return true;
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

size_t TextConverter::CheckUtf8AndConvert(char * sInput, const uint8_t ui8InputLen, char * sOutput, const uint8_t ui8OutputSize) {
#ifdef _WIN32
	if(CheckUtf8Validity(sInput, ui8InputLen, sOutput, ui8OutputSize) == true) {
		memcpy(sOutput, sInput, ui8InputLen);
		sOutput[ui8InputLen] = '\0';

		return ui8InputLen;
	}

	int iMtoWRegLen = MultiByteToWideChar(CP_ACP, 0, sInput, ui8InputLen, NULL, 0);
	if(iMtoWRegLen == 0) {
		sOutput[0] = '\0';
		return 0;
	}

	wchar_t * wcTemp = 	wcTempBuf;
	if(iMtoWRegLen > 2048) {
		wcTemp = (wchar_t *)::HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, iMtoWRegLen * sizeof(wchar_t));
		if(wcTemp == NULL) {	
			sOutput[0] = '\0';
			return 0;
		}
	}

	if(::MultiByteToWideChar(CP_ACP, 0, sInput, ui8InputLen, wcTemp, iMtoWRegLen) == 0) {
		if(wcTemp != wcTempBuf) {
			::HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)wcTemp);
		}
				
		sOutput[0] = '\0';
		return 0;
	}

	int iWtoMRegLen = ::WideCharToMultiByte(CP_UTF8, 0, wcTemp, iMtoWRegLen, NULL, 0, NULL, NULL);
	if(iWtoMRegLen == 0) {
		if(wcTemp != wcTempBuf) {
			::HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)wcTemp);
		}
		sOutput[0] = '\0';
		return 0;
	}

	if(iWtoMRegLen > (ui8OutputSize-1)) {
		iWtoMRegLen = ::WideCharToMultiByte(CP_UTF8, 0, wcTemp, --iMtoWRegLen, NULL, 0, NULL, NULL);

		while(iWtoMRegLen > (ui8OutputSize-1) && iMtoWRegLen > 0) {
			iWtoMRegLen = ::WideCharToMultiByte(CP_UTF8, 0, wcTemp, --iMtoWRegLen, NULL, 0, NULL, NULL);
		}

		if(iMtoWRegLen == 0) {
			if(wcTemp != wcTempBuf) {
				::HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)wcTemp);
			}

			sOutput[0] = '\0';
			return 0;
		}
	}

	if(::WideCharToMultiByte(CP_UTF8, 0, wcTemp, iMtoWRegLen, sOutput, iWtoMRegLen, NULL, NULL) == 0) {
		if(wcTemp != wcTempBuf) {
			::HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)wcTemp);
		}

		sOutput[0] = '\0';
		return 0;
	}

	if(wcTemp != wcTempBuf) {
		::HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)wcTemp);
	}

	sOutput[iWtoMRegLen] = '\0';
	return iWtoMRegLen;
#else
	if(CheckUtf8Validity(sInput, ui8InputLen, sOutput, ui8OutputSize) == true) {
		sOutput[ui8InputLen] = '\0';
		return ui8InputLen;
	}

	char * sInBuf = sInput;
	size_t szInbufLeft = ui8InputLen;

	char * sOutBuf = sOutput;
	size_t szOutbufLeft = ui8OutputSize-1;

	size_t szRet = iconv(m_iconvAsciiToUtf, (ICONV_CONST char**)&sInBuf, &szInbufLeft, &sOutBuf, &szOutbufLeft);
	if(szRet == (size_t)-1) {
		if(errno == E2BIG) {
			string sMsg = "[LOG] TextConverter::DoIconv iconv E2BIG for param: " + string(sInput, ui8InputLen);
			UdpDebug::m_Ptr->Broadcast(sMsg.c_str(), sMsg.size());
		} else if(errno == EILSEQ) {
			string sMsg = "[LOG] TextConverter::DoIconv iconv EILSEQ for param: "+string(sInput, ui8InputLen);
			UdpDebug::m_Ptr->Broadcast(sMsg.c_str(), sMsg.size());
		} else if(errno == EINVAL) {
			string sMsg = "[LOG] TextConverter::DoIconv iconv EINVAL for param: "+string(sInput, ui8InputLen);
			UdpDebug::m_Ptr->Broadcast(sMsg.c_str(), sMsg.size());
		}
		sOutput[0] = '\0';
		return 0;
	}

	sOutput[(ui8OutputSize-szOutbufLeft)-1] = '\0';
	return (ui8OutputSize-szOutbufLeft)-1;
#endif
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
