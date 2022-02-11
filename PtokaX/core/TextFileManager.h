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
#ifndef TextFileManagerH
#define TextFileManagerH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

class TextFilesManager {
private:
    struct TextFile {
    	TextFile * m_pPrev, * m_pNext;

        char * m_sCommand, * m_sText;

        TextFile();
        ~TextFile();

        TextFile(const TextFile&);
        const TextFile& operator=(const TextFile&);
    };

	TextFile * m_pTextFiles;

    TextFilesManager(const TextFilesManager&);
    const TextFilesManager& operator=(const TextFilesManager&);
public:
    static TextFilesManager * m_Ptr;

	TextFilesManager();
	~TextFilesManager();

	bool ProcessTextFilesCmd(User * pUser, char * sCommand, const bool bFromPM = false) const;
	void RefreshTextFiles();
};
//---------------------------------------------------------------------------

#endif
