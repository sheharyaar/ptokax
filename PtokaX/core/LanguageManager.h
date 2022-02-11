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
#ifndef LanguageManagerH
#define LanguageManagerH
//---------------------------------------------------------------------------
#include "LanguageIds.h"
//---------------------------------------------------------------------------

class LanguageManager {
private:
    LanguageManager(const LanguageManager&);
    const LanguageManager& operator=(const LanguageManager&);
public:
    static LanguageManager * m_Ptr;

    char * m_sTexts[LANG_IDS_END]; //LanguageManager::m_Ptr->m_sTexts[]
    uint16_t m_ui16TextsLens[LANG_IDS_END]; //LanguageManager::m_Ptr->m_ui16TextsLens[]

    LanguageManager(void);
    ~LanguageManager(void);

	void Load();
	
	static void GenerateXmlExample();
};
//---------------------------------------------------------------------------

#endif
