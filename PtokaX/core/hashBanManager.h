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
#ifndef hashBanManagerH
#define hashBanManagerH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

struct BanItem {
    time_t m_tTempBanExpire;

    char * m_sNick, * m_sReason, * m_sBy;

    BanItem * m_pPrev, * m_pNext;
    BanItem * m_pHashNickTablePrev, * m_pHashNickTableNext;
    BanItem * m_pHashIpTablePrev, * m_pHashIpTableNext;

    uint32_t m_ui32NickHash;

    uint8_t m_ui128IpHash[16];

    uint8_t m_ui8Bits;
    char m_sIp[40];

    BanItem();
    ~BanItem();

    BanItem(const BanItem&);
    const BanItem& operator=(const BanItem&);
}; 
//---------------------------------------------------------------------------

struct RangeBanItem {
    time_t m_tTempBanExpire;

    char * m_sReason, * m_sBy;

    RangeBanItem * m_pPrev, * m_pNext;

    uint8_t m_ui128FromIpHash[16], m_ui128ToIpHash[16];

    uint8_t m_ui8Bits;

    char m_sIpFrom[40], m_sIpTo[40] ;

    RangeBanItem();
    ~RangeBanItem();

    RangeBanItem(const RangeBanItem&);
    const RangeBanItem& operator=(const RangeBanItem&);
};
//---------------------------------------------------------------------------

class BanManager {
private:
    BanItem * m_pNickTable[65536];

    struct IpTableItem {
        IpTableItem * m_pPrev, * m_pNext;

        BanItem * m_pFirstBan;

        IpTableItem() : m_pPrev(NULL), m_pNext(NULL), m_pFirstBan(NULL) { };

        IpTableItem(const IpTableItem&);
        const IpTableItem& operator=(const IpTableItem&);
    };

	IpTableItem * m_pIpTable[65536];

    uint32_t m_ui32SaveCalled;

    BanManager(const BanManager&);
    const BanManager& operator=(const BanManager&);
public:
    static BanManager * m_Ptr;

    BanItem * m_pTempBanListS, * m_pTempBanListE;
    BanItem * m_pPermBanListS, * m_pPermBanListE;
    RangeBanItem * m_pRangeBanListS, * m_pRangeBanListE;

    enum BanBits {
        PERM        = 0x1,
        TEMP        = 0x2,
        FULL        = 0x4,
        IP          = 0x8,
        NICK        = 0x10
    };

    BanManager(void);
    ~BanManager(void);


    bool Add(BanItem * pBan);
    bool Add2Table(BanItem * pBan);
	void Add2NickTable(BanItem * pBan);
	bool Add2IpTable(BanItem * pBan);
    void Rem(BanItem * pBan, const bool bFromGui = false);
    void RemFromTable(BanItem * pBan);
    void RemFromNickTable(BanItem * pBan);
	void RemFromIpTable(BanItem * pBan);

	BanItem* Find(BanItem * pBan); // from gui
	void Remove(BanItem * pBan); // from gui

    void AddRange(RangeBanItem * pRangeBan);
	void RemRange(RangeBanItem * pRangeBan, const bool bFromGui = false);

	RangeBanItem* FindRange(RangeBanItem * pRangeBan); // from gui
	void RemoveRange(RangeBanItem * pRangeBan); // from gui

    BanItem* FindNick(User * pUser);
    BanItem* FindIP(User * pUser);
    RangeBanItem* FindRange(User * pUser);

	BanItem* FindFull(const uint8_t * ui128IpHash);
    BanItem* FindFull(const uint8_t * ui128IpHash, const time_t &tAccTime);
    RangeBanItem* FindFullRange(const uint8_t * ui128IpHash, const time_t &tAccTime);

    BanItem* FindNick(char * sNick, const size_t szNickLen);
    BanItem* FindNick(const uint32_t ui32Hash, const time_t &tAccTime, char * sNick);
    BanItem* FindIP(const uint8_t * ui128IpHash, const time_t &tAccTime);
    RangeBanItem* FindRange(const uint8_t * ui128IpHash, const time_t &tAccTime);
    RangeBanItem* FindRange(const uint8_t * ui128FromHash, const uint8_t * ui128ToHash, const time_t &tAccTime);

    BanItem* FindTempNick(char * sNick, const size_t szNickLen);
    BanItem* FindTempNick(const uint32_t ui32Hash, const time_t &tAccTime, char * sNick);
    BanItem* FindTempIP(const uint8_t * ui128IpHash, const time_t &tAccTime);
    
    BanItem* FindPermNick(char * sNick, const size_t szNickLen);
    BanItem* FindPermNick(const uint32_t ui32Hash, char * sNick);
    BanItem* FindPermIP(const uint8_t * ui128IpHash);

    void Load();
    void LoadXML();
    void Save(const bool bForce = false);

    void ClearTemp(void);
    void ClearPerm(void);
    void ClearRange(void);
    void ClearTempRange(void);
    void ClearPermRange(void);

    void Ban(User * pUser, const char * sReason, char * sBy, const bool bFull);
    char BanIp(User * pUser, char * sIp, char * sReason, char * sBy, const bool bFull);
    bool NickBan(User * pUser, char * sNick, char * sReason, char * sBy);
    
    void TempBan(User * pUser, const char * sReason, char * sBy, const uint32_t ui32Minutes, const time_t &tExpireTime, const bool bFull);
    char TempBanIp(User * pUser, char * sIp, char * sReason, char * sBy, const uint32_t ui32Minutes, const time_t &tExpireTime, const bool bFull);
    bool NickTempBan(User * pUser, char * sNick, char * sReason, char * sBy, const uint32_t ui32Minutes, const time_t &tExpireTime);
    
    bool Unban(char * sWhat);
    bool PermUnban(char * sWhat);
    bool TempUnban(char * sWhat);

    void RemoveAllIP(const uint8_t * ui128IpHash);
    void RemovePermAllIP(const uint8_t * ui128IpHash);
    void RemoveTempAllIP(const uint8_t * ui128IpHash);

    bool RangeBan(char * sIpFrom, const uint8_t * ui128FromIpHash, char * sIpTo, const uint8_t * ui128ToIpHash, char * sReason, char * sBy, const bool bFull);
    bool RangeTempBan(char * sIpFrom, const uint8_t * ui128FromIpHash, char * sIpTo, const uint8_t * ui128ToIpHash, char * sReason, char * sBy, const uint32_t ui32Minutes,
        const time_t &tExpireTime, const bool bFull);
        
    bool RangeUnban(const uint8_t * ui128FromIpHash, const uint8_t * ui128ToIpHash);
    bool RangeUnban(const uint8_t * ui128FromIpHash, const uint8_t * ui128ToIpHash, const uint8_t ui8Type);
};
//---------------------------------------------------------------------------

#endif
