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
#ifndef ProfileManagerH
#define ProfileManagerH
//---------------------------------------------------------------------------
struct User;
//---------------------------------------------------------------------------

struct ProfileItem {
    char * m_sName;

    bool m_bPermissions[256];

    ProfileItem();
    ~ProfileItem();

    ProfileItem(const ProfileItem&);
    const ProfileItem& operator=(const ProfileItem&);
};
//---------------------------------------------------------------------------

class ProfileManager {
private:
    ProfileManager(const ProfileManager&);
    const ProfileManager& operator=(const ProfileManager&);

    ProfileItem * CreateProfile(const char * sName);

	void Load();
    void LoadXML();
public:
    static ProfileManager * m_Ptr;

    ProfileItem ** m_ppProfilesTable;

	uint16_t m_ui16ProfileCount;

    enum ProfilePermissions {
        HASKEYICON,
        NODEFLOODGETNICKLIST,
        NODEFLOODMYINFO,
        NODEFLOODSEARCH,
        NODEFLOODPM,
        NODEFLOODMAINCHAT,
        MASSMSG,
        TOPIC,
        TEMP_BAN,
        REFRESHTXT,
        NOTAGCHECK,
        TEMP_UNBAN,
        DELREGUSER,
        ADDREGUSER,
        NOCHATLIMITS,
        NOMAXHUBCHECK,
        NOSLOTHUBRATIO,
        NOSLOTCHECK,
        NOSHARELIMIT,
        CLRPERMBAN,
        CLRTEMPBAN,
        GETINFO,
        GETBANLIST,
        RSTSCRIPTS,
        RSTHUB,
        TEMPOP,
        GAG,
        REDIRECT,
        BAN,
        KICK,
        DROP,
        ENTERFULLHUB,
        ENTERIFIPBAN,
        ALLOWEDOPCHAT,
        SENDALLUSERIP,
        RANGE_BAN,
        RANGE_UNBAN,
        RANGE_TBAN,
        RANGE_TUNBAN,
        GET_RANGE_BANS,
        CLR_RANGE_BANS,
        CLR_RANGE_TBANS,
        UNBAN,
        NOSEARCHLIMITS, 
        SENDFULLMYINFOS, 
        NOIPCHECK, 
        CLOSE, 
        NODEFLOODCTM, 
        NODEFLOODRCTM, 
        NODEFLOODSR, 
        NODEFLOODRECV, 
        NOCHATINTERVAL, 
        NOPMINTERVAL, 
        NOSEARCHINTERVAL, 
        NOUSRSAMEIP, 
        NORECONNTIME
    };

    ProfileManager();
    ~ProfileManager();

    bool IsAllowed(User * pUser, const uint32_t ui32Option) const;
    bool IsProfileAllowed(const int32_t i32Profile, const uint32_t ui32Option) const;
    int32_t AddProfile(char * sName);
    int32_t GetProfileIndex(const char * sName);
    int32_t RemoveProfileByName(char * sName);
    void MoveProfileDown(const uint16_t ui16Profile);
    void MoveProfileUp(const uint16_t ui16Profile);
    void ChangeProfileName(const uint16_t ui16Profile, char * sName, const size_t szLen);
    void ChangeProfilePermission(const uint16_t ui16Profile, const size_t szId, const bool bValue);
    void SaveProfiles();
    bool RemoveProfile(const uint16_t ui16Profile);
};
//---------------------------------------------------------------------------

#endif
