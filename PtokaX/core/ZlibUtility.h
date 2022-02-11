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
#ifndef zlibutilityH
#define zlibutilityH
//---------------------------------------------------------------------------

class ZlibUtility {
private:
	char * m_pZbuffer;

    size_t m_szZbufferSize;

    ZlibUtility(const ZlibUtility&);
    const ZlibUtility& operator=(const ZlibUtility&);
public:
    static ZlibUtility * m_Ptr;

	ZlibUtility();
	~ZlibUtility();
	
    char * CreateZPipe(const char * sInData, const size_t szInDataSize, uint32_t &ui32OutDataLen);
    char * CreateZPipe(char * sInData, const size_t szInDataSize, char * sOutData, size_t &szOutDataLen, size_t &szOutDataSize);
    char * CreateZPipe(char *sInData, const size_t szInDataSize, char * sOutData, uint32_t &ui32OutDataLen, uint32_t &ui32OutDataSize, size_t (* pAllignFunc)(size_t n));
};
//---------------------------------------------------------------------------

#endif
