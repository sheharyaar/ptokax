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
#include "ZlibUtility.h"
//---------------------------------------------------------------------------
#include "ServerManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include <zlib.h>
//---------------------------------------------------------------------------
static const uint32_t ZBUFFERLEN = 131072;
static const uint32_t ZMINLEN = 100;
//---------------------------------------------------------------------------
ZlibUtility * ZlibUtility::m_Ptr = NULL;
//---------------------------------------------------------------------------

ZlibUtility::ZlibUtility() : m_pZbuffer(NULL), m_szZbufferSize(0) {
	// allocate buffer for zlib
#ifdef _WIN32
	m_pZbuffer = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ZBUFFERLEN);
#else
	m_pZbuffer = (char *)calloc(ZBUFFERLEN, 1);
#endif
	if(m_pZbuffer == NULL) {
        AppendDebugLogFormat("[MEM] Cannot allocate %u bytes for m_pZbuffer in ZlibUtility::ZlibUtility\n", ZBUFFERLEN);
		exit(EXIT_FAILURE);
	}
	memcpy(m_pZbuffer, "$ZOn|", 5);
    m_szZbufferSize = ZBUFFERLEN;
}
//---------------------------------------------------------------------------
	
ZlibUtility::~ZlibUtility() {
#ifdef _WIN32
    if(m_pZbuffer != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_pZbuffer) == 0) {
			AppendDebugLog("%s - [MEM] Cannot deallocate m_pZbuffer in ZlibUtility::~ZlibUtility\n");
        }
    }
#else
	free(m_pZbuffer);
#endif
}
//---------------------------------------------------------------------------

char * ZlibUtility::CreateZPipe(const char * sInData, const size_t szInDataSize, uint32_t &ui32OutDataLen) {
    // prepare Zbuffer
    if(m_szZbufferSize < szInDataSize + 128) {
        size_t szOldZbufferSize = m_szZbufferSize;

		m_szZbufferSize = Allign128K(szInDataSize + 128);

        char * pOldBuf = m_pZbuffer;
#ifdef _WIN32
		m_pZbuffer = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, m_szZbufferSize);
#else
		m_pZbuffer = (char *)realloc(pOldBuf, m_szZbufferSize);
#endif
        if(m_pZbuffer == NULL) {
			m_pZbuffer = pOldBuf;
			m_szZbufferSize = szOldZbufferSize;
            ui32OutDataLen = 0;

			AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes for m_pZbuffer in ZlibUtility::CreateZPipe\n", m_szZbufferSize);

            return m_pZbuffer;
        }
    }
    
    z_stream stream;

    // init zlib struct
    memset(&stream, 0 , sizeof(stream));

    stream.zalloc = Z_NULL;
    stream.zfree  = Z_NULL;
    stream.data_type = Z_TEXT;

    deflateInit(&stream, Z_BEST_COMPRESSION);

    stream.next_in  = (Bytef*)sInData;
    stream.avail_in = (uInt)szInDataSize;

    stream.next_out = (Bytef*)m_pZbuffer+5;
    stream.avail_out = (uInt)m_szZbufferSize-5;

    // compress
    if(deflate(&stream, Z_FINISH) != Z_STREAM_END) {
        deflateEnd(&stream);
        AppendDebugLog("%s - [ERR] deflate error\n");
        ui32OutDataLen = 0;
        return m_pZbuffer;
    }
    
    ui32OutDataLen = stream.total_out+5;

    // cleanup zlib
    deflateEnd(&stream);

    if(ui32OutDataLen >= szInDataSize) {
        ui32OutDataLen = 0;
        return m_pZbuffer;
    }
   
    return m_pZbuffer;
}
//---------------------------------------------------------------------------

char * ZlibUtility::CreateZPipe(char * sInData, const size_t szInDataSize, char * sOutData, size_t &szOutDataLen, size_t &szOutDataSize) {
    if(szInDataSize < ZMINLEN)
        return sOutData;

    // prepare Zbuffer
    if(m_szZbufferSize < szInDataSize + 128) {
        size_t szOldZbufferSize = m_szZbufferSize;

		m_szZbufferSize = Allign128K(szInDataSize + 128);

        char * pOldBuf = m_pZbuffer;
#ifdef _WIN32
		m_pZbuffer = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, m_szZbufferSize);
#else
		m_pZbuffer = (char *)realloc(pOldBuf, m_szZbufferSize);
#endif
        if(m_pZbuffer == NULL) {
			m_pZbuffer = pOldBuf;
			m_szZbufferSize = szOldZbufferSize;
            szOutDataLen = 0;

			AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes for m_pZbuffer in ZlibUtility::CreateZPipe\n", m_szZbufferSize);

            return sOutData;
        }
    }
    
    z_stream stream;

    // init zlib struct
    memset(&stream, 0 , sizeof(stream));

    stream.zalloc = Z_NULL;
    stream.zfree  = Z_NULL;
    stream.data_type = Z_TEXT;

    deflateInit(&stream, Z_BEST_COMPRESSION);

    stream.next_in  = (Bytef*)sInData;
    stream.avail_in = (uInt)szInDataSize;

    stream.next_out = (Bytef*)m_pZbuffer+5;
    stream.avail_out = (uInt)m_szZbufferSize-5;

    // compress
    if(deflate(&stream, Z_FINISH) != Z_STREAM_END) {
        deflateEnd(&stream);
        AppendDebugLog("%s - [ERR] deflate error\n");
        return sOutData;
    }
    
    szOutDataLen = stream.total_out+5;

    // cleanup zlib
    deflateEnd(&stream);

    if(szOutDataLen >= szInDataSize) {
        szOutDataLen = 0;
        return sOutData;
    }
    
    // prepare out buffer
    if(szOutDataSize < szOutDataLen) {
        size_t szOldOutDataSize = szOutDataSize;

        szOutDataSize = Allign1024(szOutDataLen)-1;
        char * pOldBuf = sOutData;
#ifdef _WIN32
        sOutData = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, szOutDataSize+1);
#else
		sOutData = (char *)realloc(pOldBuf, szOutDataSize+1);
#endif
        if(sOutData == NULL) {
            sOutData = pOldBuf;
            szOutDataSize = szOldOutDataSize;
            szOutDataLen = 0;

			AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes for sOutData in ZlibUtility::CreateZPipe\n", szOutDataSize+1);

            return sOutData;
        }
    }
    
    memcpy(sOutData, m_pZbuffer, szOutDataLen);
    
    return sOutData;
}
//---------------------------------------------------------------------------

char * ZlibUtility::CreateZPipe(char * sInData, const size_t szInDataSize, char * sOutData, uint32_t &ui32OutDataLen, uint32_t &ui32OutDataSize, size_t (* pAllignFunc)(size_t n)) {
    if(szInDataSize < ZMINLEN)
        return sOutData;

    // prepare Zbuffer
    if(m_szZbufferSize < szInDataSize + 128) {
        size_t szOldZbufferSize = m_szZbufferSize;

		m_szZbufferSize = Allign128K(szInDataSize + 128);

        char * pOldBuf = m_pZbuffer;
#ifdef _WIN32
		m_pZbuffer = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, m_szZbufferSize);
#else
		m_pZbuffer = (char *)realloc(pOldBuf, m_szZbufferSize);
#endif
        if(m_pZbuffer == NULL) {
			m_pZbuffer = pOldBuf;
			m_szZbufferSize = szOldZbufferSize;
            ui32OutDataLen = 0;

			AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes for m_pZbuffer in ZlibUtility::CreateZPipe\n", m_szZbufferSize);

            return sOutData;
        }
    }
    
    z_stream stream;

    // init zlib struct
    memset(&stream, 0 , sizeof(stream));

    stream.zalloc = Z_NULL;
    stream.zfree  = Z_NULL;
    stream.data_type = Z_TEXT;

    deflateInit(&stream, Z_BEST_COMPRESSION);

    stream.next_in  = (Bytef*)sInData;
    stream.avail_in = (uInt)szInDataSize;

    stream.next_out = (Bytef*)m_pZbuffer+5;
    stream.avail_out = (uInt)m_szZbufferSize-5;

    // compress
    if(deflate(&stream, Z_FINISH) != Z_STREAM_END) {
        deflateEnd(&stream);
        AppendDebugLog("%s - [ERR] deflate error\n");
        return sOutData;
    }
    
    ui32OutDataLen = stream.total_out+5;

    // cleanup zlib
    deflateEnd(&stream);

    if(ui32OutDataLen >= szInDataSize) {
        ui32OutDataLen = 0;
        return sOutData;
    }

    // prepare out buffer
    if(ui32OutDataSize < ui32OutDataLen) {
        unsigned int uiOldOutDataSize = ui32OutDataSize;

        ui32OutDataSize = (unsigned int)(* pAllignFunc)(ui32OutDataLen+1);

        char * pOldBuf = sOutData;
#ifdef _WIN32
        sOutData = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, ui32OutDataSize);
#else
		sOutData = (char *)realloc(pOldBuf, ui32OutDataSize);
#endif
        if(sOutData == NULL) {
            sOutData = pOldBuf;
            ui32OutDataSize = uiOldOutDataSize;
            ui32OutDataLen = 0;

			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes for sOutData in ZlibUtility::CreateZPipe\n", ui32OutDataSize+1);

            return sOutData;
        }
    }
    
    memcpy(sOutData, m_pZbuffer, ui32OutDataLen);
   
    return sOutData;
}
//---------------------------------------------------------------------------
