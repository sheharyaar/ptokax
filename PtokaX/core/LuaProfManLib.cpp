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
#include "stdinc.h"
//---------------------------------------------------------------------------
#include "LuaInc.h"
//---------------------------------------------------------------------------
#include "LuaProfManLib.h"
//---------------------------------------------------------------------------
#include "ProfileManager.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------

static void PushProfilePermissions(lua_State * pLua, const uint16_t iProfile) {
    ProfileItem *Prof = ProfileManager::m_Ptr->m_ppProfilesTable[iProfile];

    lua_checkstack(pLua, 3); // we need 3 (1 table, 2 id, 3 value) empty slots in stack, check it to be sure

    lua_newtable(pLua);
    int t = lua_gettop(pLua);

    lua_pushliteral(pLua, "bIsOP");
    Prof->m_bPermissions[ProfileManager::HASKEYICON] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoDefloodGetNickList");
    Prof->m_bPermissions[ProfileManager::NODEFLOODGETNICKLIST] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoDefloodNMyINFO");
    Prof->m_bPermissions[ProfileManager::NODEFLOODMYINFO] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoDefloodSearch");
    Prof->m_bPermissions[ProfileManager::NODEFLOODSEARCH] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoDefloodPM");
    Prof->m_bPermissions[ProfileManager::NODEFLOODPM] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoDefloodMainChat");
    Prof->m_bPermissions[ProfileManager::NODEFLOODMAINCHAT] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bMassMsg");
    Prof->m_bPermissions[ProfileManager::MASSMSG] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bTopic");
    Prof->m_bPermissions[ProfileManager::TOPIC] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bTempBan");
    Prof->m_bPermissions[ProfileManager::TEMP_BAN] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bTempUnban");
    Prof->m_bPermissions[ProfileManager::TEMP_UNBAN] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);
    
    lua_pushliteral(pLua, "bRefreshTxt");
    Prof->m_bPermissions[ProfileManager::REFRESHTXT] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoTagCheck");
    Prof->m_bPermissions[ProfileManager::NOTAGCHECK] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bDelRegUser");
    Prof->m_bPermissions[ProfileManager::DELREGUSER] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bAddRegUser");
    Prof->m_bPermissions[ProfileManager::ADDREGUSER] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoChatLimits");
    Prof->m_bPermissions[ProfileManager::NOCHATLIMITS] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoMaxHubCheck");
    Prof->m_bPermissions[ProfileManager::NOMAXHUBCHECK] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoSlotHubRatio");
    Prof->m_bPermissions[ProfileManager::NOSLOTHUBRATIO] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoSlotCheck");
    Prof->m_bPermissions[ProfileManager::NOSLOTCHECK] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoShareLimit");
    Prof->m_bPermissions[ProfileManager::NOSHARELIMIT] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bClrPermBan");
    Prof->m_bPermissions[ProfileManager::CLRPERMBAN] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bClrTempBan");
    Prof->m_bPermissions[ProfileManager::CLRTEMPBAN] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bGetInfo");
    Prof->m_bPermissions[ProfileManager::GETINFO] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bGetBans");
    Prof->m_bPermissions[ProfileManager::GETBANLIST] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bRestartScripts");
    Prof->m_bPermissions[ProfileManager::RSTSCRIPTS] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bRestartHub");
    Prof->m_bPermissions[ProfileManager::RSTHUB] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bTempOP");
    Prof->m_bPermissions[ProfileManager::TEMPOP] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bGag");
    Prof->m_bPermissions[ProfileManager::GAG] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bRedirect");
    Prof->m_bPermissions[ProfileManager::REDIRECT] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bBan");
    Prof->m_bPermissions[ProfileManager::BAN] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bUnban");
    Prof->m_bPermissions[ProfileManager::UNBAN] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);
    
    lua_pushliteral(pLua, "bKick");
    Prof->m_bPermissions[ProfileManager::KICK] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bDrop");
    Prof->m_bPermissions[ProfileManager::DROP] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bEnterFullHub");
    Prof->m_bPermissions[ProfileManager::ENTERFULLHUB] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bEnterIfIPBan");
    Prof->m_bPermissions[ProfileManager::ENTERIFIPBAN] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bAllowedOPChat");
    Prof->m_bPermissions[ProfileManager::ALLOWEDOPCHAT] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bSendFullMyinfos");
    Prof->m_bPermissions[ProfileManager::SENDFULLMYINFOS] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);   
    
    lua_pushliteral(pLua, "bSendAllUserIP");
    Prof->m_bPermissions[ProfileManager::SENDALLUSERIP] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t); 
    
    lua_pushliteral(pLua, "bRangeBan");
    Prof->m_bPermissions[ProfileManager::RANGE_BAN] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);
    
    lua_pushliteral(pLua, "bRangeUnban");
    Prof->m_bPermissions[ProfileManager::RANGE_UNBAN] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bRangeTempBan");
    Prof->m_bPermissions[ProfileManager::RANGE_TBAN] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bRangeTempUnban");
    Prof->m_bPermissions[ProfileManager::RANGE_TUNBAN] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);
    
    lua_pushliteral(pLua, "bGetRangeBans");
    Prof->m_bPermissions[ProfileManager::GET_RANGE_BANS] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bClearRangePermBans");
    Prof->m_bPermissions[ProfileManager::CLR_RANGE_BANS] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bClearRangeTempBans");
    Prof->m_bPermissions[ProfileManager::CLR_RANGE_TBANS] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);
    
    lua_pushliteral(pLua, "bNoIpCheck");
    Prof->m_bPermissions[ProfileManager::NOIPCHECK] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);
    
    lua_pushliteral(pLua, "bClose");
    Prof->m_bPermissions[ProfileManager::CLOSE] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoSearchLimits");
    Prof->m_bPermissions[ProfileManager::NOSEARCHLIMITS] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoDefloodCTM");
    Prof->m_bPermissions[ProfileManager::NODEFLOODCTM] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoDefloodRCTM");
    Prof->m_bPermissions[ProfileManager::NODEFLOODRCTM] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoDefloodSR");
    Prof->m_bPermissions[ProfileManager::NODEFLOODSR] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoDefloodRecv");
    Prof->m_bPermissions[ProfileManager::NODEFLOODRECV] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoChatInterval");
    Prof->m_bPermissions[ProfileManager::NOCHATINTERVAL] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoPMInterval");
    Prof->m_bPermissions[ProfileManager::NOPMINTERVAL] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoSearchInterval");
    Prof->m_bPermissions[ProfileManager::NOSEARCHINTERVAL] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoMaxUsersSameIP");
    Prof->m_bPermissions[ProfileManager::NOUSRSAMEIP] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);

    lua_pushliteral(pLua, "bNoReConnTime");
    Prof->m_bPermissions[ProfileManager::NORECONNTIME] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, t);
}
//------------------------------------------------------------------------------

static void PushProfile(lua_State * pLua, const uint16_t iProfile) {
	lua_checkstack(pLua, 3); // we need 3 (1 table, 2 id, 3 value) empty slots in stack, check it to be sure

    lua_newtable(pLua);
    int i = lua_gettop(pLua);

    lua_pushliteral(pLua, "sProfileName");
	lua_pushstring(pLua, ProfileManager::m_Ptr->m_ppProfilesTable[iProfile]->m_sName);
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "iProfileNumber");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, iProfile);
#else
	lua_pushinteger(pLua, iProfile);
#endif
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "tProfilePermissions");
	PushProfilePermissions(pLua, iProfile);
    lua_rawset(pLua, i);
}
//------------------------------------------------------------------------------

static int AddProfile(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'AddProfile' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    size_t szLen;
    char * sProfileName = (char *)lua_tolstring(pLua, 1, &szLen);

    if(szLen == 0 || szLen > 64) {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

	int32_t idx = ProfileManager::m_Ptr->AddProfile(sProfileName);
    if(idx == -1 || idx == -2) {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    lua_settop(pLua, 0);

#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, idx);
#else
    lua_pushinteger(pLua, idx);
#endif

    return 1;
}
//------------------------------------------------------------------------------

static int RemoveProfile(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'RemoveProfile' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) == LUA_TSTRING) {
        size_t szLen;
        char * sProfileName = (char *)lua_tolstring(pLua, 1, &szLen);

        if(szLen == 0) {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        int32_t result = ProfileManager::m_Ptr->RemoveProfileByName(sProfileName);
        if(result != 1) {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        lua_settop(pLua, 0);
        lua_pushboolean(pLua, 1);
        return 1;
    } else if(lua_type(pLua, 1) == LUA_TNUMBER) {
#if LUA_VERSION_NUM < 503
        uint16_t idx = (uint16_t)lua_tonumber(pLua, 1);
#else
    	uint16_t idx = (uint16_t)lua_tointeger(pLua, 1);
#endif

    	lua_settop(pLua, 0);

        // if the requested index is out of bounds return nil
        if(idx >= ProfileManager::m_Ptr->m_ui16ProfileCount) {
            lua_pushnil(pLua);
            return 1;
        }

        if(ProfileManager::m_Ptr->RemoveProfile(idx) == false) {
            lua_pushnil(pLua);
            return 1;
        }

        lua_pushboolean(pLua, 1);
        return 1;
    }

    luaL_error(pLua, "bad argument #1 to 'RemoveProfile' (string or number expected, got %d)", lua_typename(pLua, lua_type(pLua, 1)));
	lua_settop(pLua, 0);
	lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int MoveDown(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'MoveDown' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TNUMBER) {
        luaL_checktype(pLua, 1, LUA_TNUMBER);

		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

#if LUA_VERSION_NUM < 503
	uint16_t iProfile = (uint16_t)lua_tonumber(pLua, 1);
#else
	uint16_t iProfile = (uint16_t)lua_tointeger(pLua, 1);
#endif

    lua_settop(pLua, 0);

    // if the requested index is out of bounds return nil
    if(iProfile >= ProfileManager::m_Ptr->m_ui16ProfileCount-1) {
		lua_pushnil(pLua);
        return 1;
	}

    ProfileManager::m_Ptr->MoveProfileDown(iProfile);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int MoveUp(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'MoveUp' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TNUMBER) {
        luaL_checktype(pLua, 1, LUA_TNUMBER);

		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

#if LUA_VERSION_NUM < 503
	uint16_t iProfile = (uint16_t)lua_tonumber(pLua, 1);
#else
	uint16_t iProfile = (uint16_t)lua_tointeger(pLua, 1);
#endif

	lua_settop(pLua, 0);

    // if the requested index is out of bounds return nil
    if(iProfile == 0 || iProfile >= ProfileManager::m_Ptr->m_ui16ProfileCount) {
		lua_pushnil(pLua);
        return 1;
	}

    ProfileManager::m_Ptr->MoveProfileUp(iProfile);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int GetProfile(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'GetProfile' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) == LUA_TSTRING) {
        size_t szLen;
        char *profName = (char *)lua_tolstring(pLua, 1, &szLen);

        if(szLen == 0) {
        	lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

		int32_t idx = ProfileManager::m_Ptr->GetProfileIndex(profName);

        lua_settop(pLua, 0);

        if(idx == -1) {
            lua_pushnil(pLua);
            return 1;
        }

        PushProfile(pLua, (uint16_t)idx);
        return 1;
    } else if(lua_type(pLua, 1) == LUA_TNUMBER) {
#if LUA_VERSION_NUM < 503
		uint16_t idx = (uint16_t)lua_tonumber(pLua, 1);
#else
    	uint16_t idx = (uint16_t)lua_tointeger(pLua, 1);
#endif

    	lua_settop(pLua, 0);
    
        // if the requested index is out of bounds return nil
        if(idx >= ProfileManager::m_Ptr->m_ui16ProfileCount) {
            lua_pushnil(pLua);
            return 1;
        }
    
        PushProfile(pLua, idx);
        return 1;
    }

    luaL_error(pLua, "bad argument #1 to 'GetProfile' (string or number expected, got %d)", lua_typename(pLua, lua_type(pLua, 1)));
	lua_settop(pLua, 0);
	lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int GetProfiles(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetProfiles' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    lua_newtable(pLua);
    int t = lua_gettop(pLua);

    for(uint16_t ui16i = 0; ui16i < ProfileManager::m_Ptr->m_ui16ProfileCount; ui16i++) {
#if LUA_VERSION_NUM < 503
		lua_pushnumber(pLua, (ui16i+1));
#else
        lua_pushinteger(pLua, (ui16i+1));
#endif

        PushProfile(pLua, ui16i);
        lua_rawset(pLua, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetProfilePermission(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'GetProfilePermission' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TNUMBER) {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

#if LUA_VERSION_NUM < 503
	uint16_t iProfile = (uint16_t)lua_tonumber(pLua, 1);
	size_t szId = (size_t)lua_tonumber(pLua, 2);
#else
    uint16_t iProfile = (uint16_t)lua_tointeger(pLua, 1);
    size_t szId = (size_t)lua_tointeger(pLua, 2);
#endif
    
    lua_settop(pLua, 0);
    
	if(iProfile >= ProfileManager::m_Ptr->m_ui16ProfileCount) {
        lua_pushnil(pLua);
        return 1;
    }

	if(szId > ProfileManager::NORECONNTIME) {
		luaL_error(pLua, "bad argument #2 to 'GetProfilePermission' (it's not valid id)");
		lua_pushnil(pLua);
		return 1;
	}

    ProfileManager::m_Ptr->m_ppProfilesTable[iProfile]->m_bPermissions[szId] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);

    return 1;
}
//------------------------------------------------------------------------------

static int GetProfilePermissions(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'GetProfilePermissions' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TNUMBER) {
        luaL_checktype(pLua, 1, LUA_TNUMBER);

		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

#if LUA_VERSION_NUM < 503
	uint16_t iProfile = (uint16_t)lua_tonumber(pLua, 1);
#else
	uint16_t iProfile = (uint16_t)lua_tointeger(pLua, 1);
#endif

	lua_settop(pLua, 0);

    // if the requested index is out of bounds return nil
    if(iProfile >= ProfileManager::m_Ptr->m_ui16ProfileCount) {
		lua_pushnil(pLua);
        return 1;
	}

    PushProfilePermissions(pLua, iProfile);

    return 1;
}
//------------------------------------------------------------------------------

static int SetProfileName(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'SetProfileName' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        luaL_checktype(pLua, 2, LUA_TSTRING);
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

#if LUA_VERSION_NUM < 503
	uint16_t iProfile = (uint16_t)lua_tonumber(pLua, 1);
#else
    uint16_t iProfile = (uint16_t)lua_tointeger(pLua, 1);
#endif

	lua_settop(pLua, 0);

    if(iProfile >= ProfileManager::m_Ptr->m_ui16ProfileCount) {
        lua_pushnil(pLua);
        return 1;
    }

    size_t szLen;
	char * sName = (char *)lua_tolstring(pLua, 2, &szLen);

    if(szLen == 0 || szLen > 64) {
        lua_pushnil(pLua);
        return 1;
    }

	ProfileManager::m_Ptr->ChangeProfileName(iProfile, sName, szLen);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int SetProfilePermission(lua_State * pLua) {
	if(lua_gettop(pLua) != 3) {
        luaL_error(pLua, "bad argument count to 'SetProfilePermission' (3 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TNUMBER || lua_type(pLua, 3) != LUA_TBOOLEAN) {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        luaL_checktype(pLua, 3, LUA_TBOOLEAN);
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

#if LUA_VERSION_NUM < 503
	uint16_t iProfile = (uint16_t)lua_tonumber(pLua, 1);
	size_t szId = (size_t)lua_tonumber(pLua, 2);
#else
    uint16_t iProfile = (uint16_t)lua_tointeger(pLua, 1);
    size_t szId = (size_t)lua_tointeger(pLua, 2);
#endif

    bool bValue = lua_toboolean(pLua, 3) == 0 ? false : true;
    
    lua_settop(pLua, 0);
    
	if(iProfile >= ProfileManager::m_Ptr->m_ui16ProfileCount) {
		lua_pushnil(pLua);
		return 1;
	}

	if(szId > ProfileManager::NORECONNTIME) {
		luaL_error(pLua, "bad argument #2 to 'SetProfilePermission' (it's not valid id)");
		lua_pushnil(pLua);
		return 1;
	}

    ProfileManager::m_Ptr->ChangeProfilePermission(iProfile, szId, bValue);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int Save(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'Save' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

	ProfileManager::m_Ptr->SaveProfiles();

    return 0;
}
//------------------------------------------------------------------------------

static const luaL_Reg ProfManRegs[] = {
	{ "AddProfile", AddProfile },
	{ "RemoveProfile", RemoveProfile }, 
	{ "MoveDown", MoveDown }, 
	{ "MoveUp", MoveUp }, 
	{ "GetProfile", GetProfile }, 
	{ "GetProfiles", GetProfiles }, 
	{ "GetProfilePermission", GetProfilePermission }, 
	{ "GetProfilePermissions", GetProfilePermissions }, 
	{ "SetProfileName", SetProfileName }, 
	{ "SetProfilePermission", SetProfilePermission }, 
	{ "Save", Save },
	{ NULL, NULL }
};
//---------------------------------------------------------------------------

#if LUA_VERSION_NUM > 501
int RegProfMan(lua_State * pLua) {
    luaL_newlib(pLua, ProfManRegs);
    return 1;
#else
void RegProfMan(lua_State * pLua) {
    luaL_register(pLua, "ProfMan", ProfManRegs);
#endif
}
//---------------------------------------------------------------------------
