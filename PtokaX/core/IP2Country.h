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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef IP2CountryH
#define IP2CountryH
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

class IpP2Country {
private:
    uint32_t * m_ui32RangeFrom, * m_ui32RangeTo;
    uint8_t * m_ui8RangeCI, * m_ui8IPv6RangeCI;
    uint8_t * m_ui128IPv6RangeFrom, * m_ui128IPv6RangeTo;

    uint32_t m_ui32Size, m_ui32IPv6Size;

    IpP2Country(const IpP2Country&);
    const IpP2Country& operator=(const IpP2Country&);

    void LoadIPv4();
    void LoadIPv6();
public:
    static IpP2Country * m_Ptr;

    uint32_t m_ui32Count, m_ui32IPv6Count;

	IpP2Country();
	~IpP2Country();

	const char * Find(const uint8_t * ui128IpHash, const bool bCountryName);
	uint8_t Find(const uint8_t * ui128IpHash);

    static const char * GetCountry(const uint8_t ui8dx, const bool bCountryName);

    void Reload();
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif
