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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef GlobalDataQueueH
#define GlobalDataQueueH
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct User;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

class GlobalDataQueue {
private:
    struct QueueItem {
    	QueueItem * m_pNext;

        char * m_pCommand1, * m_pCommand2;

        size_t m_szLen1, m_szLen2;

        uint8_t m_ui8CommandType;

        QueueItem() : m_pNext(NULL), m_pCommand1(NULL), m_pCommand2(NULL), m_szLen1(0), m_szLen2(0), m_ui8CommandType(0) { };

        QueueItem(const QueueItem&);
        const QueueItem& operator=(const QueueItem&);
    };

    struct GlobalQueue {
    	GlobalQueue * m_pNext;

        char * m_pBuffer, * m_pZbuffer;

        size_t m_szLen, m_szSize, m_szZlen, m_szZsize;

        bool m_bCreated, m_bZlined;

        GlobalQueue() : m_pNext(NULL), m_pBuffer(NULL), m_pZbuffer(NULL), m_szLen(0), m_szSize(0), m_szZlen(0), m_szZsize(0), m_bCreated(false), m_bZlined(false) { };

        GlobalQueue(const GlobalQueue&);
        const GlobalQueue& operator=(const GlobalQueue&);
    };

    struct OpsQueue {
    	char * m_pBuffer;

    	size_t m_szLen, m_szSize;

        OpsQueue() : m_pBuffer(NULL), m_szLen(0), m_szSize(0) { };

        OpsQueue(const OpsQueue&);
        const OpsQueue& operator=(const OpsQueue&);
    };

    struct IPsQueue {
    	char * m_pBuffer;

    	size_t m_szLen, m_szSize;

    	bool m_bHaveDollars;

        IPsQueue() : m_pBuffer(NULL), m_szLen(0), m_szSize(0), m_bHaveDollars(false) { };

        IPsQueue(const IPsQueue&);
        const IPsQueue& operator=(const IPsQueue&);
    };

    struct SingleDataItem {
        SingleDataItem * m_pPrev, * m_pNext;

        User * m_pFromUser;

        char * m_pData;

        size_t m_szDataLen;

        int32_t m_i32Profile;

        uint8_t m_ui8Type;

        SingleDataItem() : m_pPrev(NULL), m_pNext(NULL), m_pFromUser(NULL), m_pData(NULL), m_szDataLen(0), m_i32Profile(0), m_ui8Type(0) { };

        SingleDataItem(const SingleDataItem&);
        const SingleDataItem& operator=(const SingleDataItem&);
    };

	GlobalQueue m_GlobalQueues[144];

	struct OpsQueue m_OpListQueue;
	struct IPsQueue m_UserIPQueue;

    GlobalQueue * m_pCreatedGlobalQueues;

    QueueItem * m_pNewQueueItems[2], * m_pQueueItems;
    SingleDataItem * m_pNewSingleItems[2];

    GlobalDataQueue(const GlobalDataQueue&);
    const GlobalDataQueue& operator=(const GlobalDataQueue&);

    static void AddDataToQueue(GlobalQueue & rQueue, char * sData, const size_t szLen);
public:
    static GlobalDataQueue * m_Ptr;

    SingleDataItem * m_pSingleItems;

    bool m_bHaveItems;

    enum {
        CMD_HUBNAME,
        CMD_CHAT,
        CMD_HELLO,
        CMD_MYINFO,
        CMD_QUIT,
        CMD_OPS,
        CMD_LUA,
        CMD_ACTIVE_SEARCH_V6,
        CMD_ACTIVE_SEARCH_V64,
        CMD_ACTIVE_SEARCH_V4,
        CMD_PASSIVE_SEARCH_V6,
        CMD_PASSIVE_SEARCH_V64,
        CMD_PASSIVE_SEARCH_V4,
        CMD_PASSIVE_SEARCH_V4_ONLY,
        CMD_PASSIVE_SEARCH_V6_ONLY,
    };

    enum {
        BIT_LONG_MYINFO                     = 0x1,
        BIT_ALL_SEARCHES_IPV64              = 0x2,
        BIT_ALL_SEARCHES_IPV6               = 0x4,
        BIT_ALL_SEARCHES_IPV4               = 0x8,
        BIT_ACTIVE_SEARCHES_IPV64           = 0x10,
        BIT_ACTIVE_SEARCHES_IPV6            = 0x20,
        BIT_ACTIVE_SEARCHES_IPV4            = 0x40,
        BIT_HELLO                           = 0x80,
        BIT_OPERATOR                        = 0x100,
        BIT_USERIP                          = 0x200,
        BIT_ALL_SEARCHES_IPV6_ACTIVE_IPV4   = 0x400,
        BIT_ACTIVE_SEARCHES_IPV6_ALL_IPV4   = 0x800,
    };

    enum {
        SI_PM2ALL,
        SI_PM2OPS,
        SI_OPCHAT,
        SI_TOPROFILE,
        SI_PM2PROFILE,
    };

    GlobalDataQueue();
    ~GlobalDataQueue();

    void AddQueueItem(char * sCommand1, const size_t szLen1, char * sCommand2, const size_t szLen2, const uint8_t ui8CmdType);
    void OpListStore(char * sNick);
    void UserIPStore(User * pUser);
    void PrepareQueueItems();
    void ClearQueues();
    void ProcessQueues(User * pUser);
    void ProcessSingleItems(User * pUser) const;
    void SingleItemStore(char * sData, const size_t szDataLen, User * pFromUser, const int32_t i32Profile, const uint8_t ui8Type);
    void SendFinalQueue();
    void * GetLastQueueItem();
    void * GetFirstQueueItem();
    void * InsertBlankQueueItem(void * pAfterItem, const uint8_t ui8CmdType);
    static void FillBlankQueueItem(char * sCommand, const size_t szLen, void * pVoidQueueItem);
    void StatusMessageFormat(const char * sFrom, const char * sFormatMsg, ...);
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif
