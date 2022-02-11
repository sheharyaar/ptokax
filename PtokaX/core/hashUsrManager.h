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
#ifndef hashUsrManagerH
#define hashUsrManagerH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

class HashManager {
private:
    User * m_pNickTable[65536];

    struct IpTableItem {
        IpTableItem * m_pPrev, * m_pNext;

        User * m_pFirstUser;

        uint16_t m_ui16Count;

        IpTableItem() : m_pPrev(NULL), m_pNext(NULL), m_pFirstUser(NULL), m_ui16Count(0) { };

        IpTableItem(const IpTableItem&);
        const IpTableItem& operator=(const IpTableItem&);
    };

	IpTableItem * m_pIpTable[65536];

    HashManager(const HashManager&);
    const HashManager& operator=(const HashManager&);
public:
    static HashManager * m_Ptr;

    HashManager();
    ~HashManager();

    bool Add(User * pUser);
    void Remove(User * pUser);

    User * FindUser(char * sNick, const size_t szNickLen);
    User * FindUser(User * pUser);
    User * FindUser(const uint8_t * ui128IpHash);

    uint32_t GetUserIpCount(User * pUser) const;
};
//---------------------------------------------------------------------------

#endif
