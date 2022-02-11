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
#ifndef ResNickManagerH
#define ResNickManagerH
//---------------------------------------------------------------------------

class ReservedNicksManager {
private:
    struct ReservedNick {
    	ReservedNick * m_pPrev, * m_pNext;

        char * m_sNick;

        uint32_t m_ui32Hash;

        bool m_bFromScript;

        ReservedNick();
        ~ReservedNick();

        ReservedNick(const ReservedNick&);
        const ReservedNick& operator=(const ReservedNick&);

        static ReservedNick * CreateReservedNick(const char * sNewNick, const uint32_t ui32NickHash);
    };

	ReservedNick * m_pReservedNicks;

    ReservedNicksManager(const ReservedNicksManager&);
    const ReservedNicksManager& operator=(const ReservedNicksManager&);

	void Load();
	void Save() const;
	void LoadXML();
public:
    static ReservedNicksManager * m_Ptr;

	ReservedNicksManager();
	~ReservedNicksManager();

    bool CheckReserved(const char * sNick, const uint32_t ui32Hash) const;
    void AddReservedNick(const char * sNick, const bool bFromScript = false);
    void DelReservedNick(char * sNick, const bool bFromScript = false);
};
//---------------------------------------------------------------------------

#endif
