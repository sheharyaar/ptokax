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
#ifndef colUsersH
#define colUsersH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------
static const uint32_t NICKLISTSIZE = 1024*64;
static const uint32_t OPLISTSIZE = 1024*32;
//---------------------------------------------------------------------------

class Users {
private:
	uint64_t m_ui64ChatMsgsTick, m_ui64ChatLockFromTick;

    struct RecTime {
        uint64_t m_ui64DisConnTick;

        RecTime * m_pPrev, * m_pNext;

        char * m_sNick;

        uint32_t m_ui32NickHash;

        uint8_t m_ui128IpHash[16];

        explicit RecTime(const uint8_t * pIpHash);

        RecTime(const RecTime&);
        const RecTime& operator=(const RecTime&);
    };

    RecTime * m_pRecTimeList;

	User * m_pUserListE;

	uint16_t m_ui16ChatMsgs;

    bool m_bChatLocked;

    Users(const Users&);
    const Users& operator=(const Users&);
public:
    static Users * m_Ptr;

	User * m_pUserListS;

    char * m_pNickList, * m_pZNickList, * m_pOpList, * m_pZOpList;
	char * m_pUserIPList, * m_pZUserIPList, * m_pMyInfos, * m_pZMyInfos;
	char * m_pMyInfosTag, * m_pZMyInfosTag;

    uint32_t m_ui32MyInfosLen, m_ui32MyInfosSize, m_ui32ZMyInfosLen, m_ui32ZMyInfosSize;
    uint32_t m_ui32MyInfosTagLen, m_ui32MyInfosTagSize, m_ui32ZMyInfosTagLen, m_ui32ZMyInfosTagSize;
    uint32_t m_ui32NickListLen, m_ui32NickListSize, m_ui32ZNickListLen, m_ui32ZNickListSize;
    uint32_t m_ui32OpListLen, m_ui32OpListSize, m_ui32ZOpListLen, m_ui32ZOpListSize;
    uint32_t m_ui32UserIPListSize, m_ui32UserIPListLen, m_ui32ZUserIPListSize, m_ui32ZUserIPListLen;
    
    uint16_t m_ui16ActSearchs, m_ui16PasSearchs;

    Users();
    ~Users();

    void DisconnectAll();
    void AddUser(User * pUser);
    void RemUser(User * pUser);
    void Add2NickList(User * pUser);
    void AddBot2NickList(char * sNick, const size_t szNickLen, const bool bIsOp);
    void Add2OpList(User * pUser);
    void DelFromNickList(char * sNick, const bool bIsOp);
    void DelFromOpList(char * sNick);
    void SendChat2All(User * pUser, char * sData, const size_t szChatLen, void * pQueueItem);
	void Add2MyInfos(User * pUser);
	void DelFromMyInfos(User * pUser);
    void Add2MyInfosTag(User * pUser);
	void DelFromMyInfosTag(User * pUser);
    void AddBot2MyInfos(char * sMyInfo);
	void DelBotFromMyInfos(char * sMyInfo);
	void Add2UserIP(User * pUser);
	void DelFromUserIP(User * pUser);
	void Add2RecTimes(User * pUser);
	bool CheckRecTime(User * pUser);
};
//---------------------------------------------------------------------------

#endif
