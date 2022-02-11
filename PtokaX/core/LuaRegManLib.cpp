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
#include "LuaRegManLib.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "GlobalDataQueue.h"
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------

static void PushReg(lua_State * pLua, RegUser * pReg) {
	lua_checkstack(pLua, 3); // we need 3 (1 table, 2 id, 3 value) empty slots in stack, check it to be sure

    lua_newtable(pLua);
    int i = lua_gettop(pLua);

    lua_pushliteral(pLua, "sNick");
    lua_pushstring(pLua, pReg->m_sNick);
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sPassword");
    if(pReg->m_bPassHash == true) {
        lua_pushnil(pLua);
    } else {
        lua_pushstring(pLua, pReg->m_sPass);
    }
    lua_rawset(pLua, i);
            
    lua_pushliteral(pLua, "iProfile");
#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, pReg->m_ui16Profile);
#else
    lua_pushinteger(pLua, pReg->m_ui16Profile);
#endif
    lua_rawset(pLua, i);
}
//------------------------------------------------------------------------------

static int Save(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'Save' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

	RegManager::m_Ptr->Save();

    return 0;
}
//------------------------------------------------------------------------------

static int GetRegsByProfile(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'GetRegsByProfile' (1 expected, got %d)", lua_gettop(pLua));
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

    lua_newtable(pLua);
    int t = lua_gettop(pLua), i = 0;

	RegUser * cur = NULL,
        * next = RegManager::m_Ptr->m_pRegListS;
        
    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;
        
		if(cur->m_ui16Profile == iProfile) {
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, ++i);
#else
            lua_pushinteger(pLua, ++i);
#endif
            PushReg(pLua, cur);
            lua_rawset(pLua, t);
        }
    }
    
    return 1;
}
//------------------------------------------------------------------------------

static int GetRegsByOpStatus(lua_State * pLua, const bool bOperator) {
	if(lua_gettop(pLua) != 0) {
        if(bOperator == true) {
            luaL_error(pLua, "bad argument count to 'GetOps' (0 expected, got %d)", lua_gettop(pLua));
        } else {
            luaL_error(pLua, "bad argument count to 'GetNonOps' (0 expected, got %d)", lua_gettop(pLua));
        }
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }
    
    lua_newtable(pLua);
    int t = lua_gettop(pLua), i = 0;

    RegUser * curReg = NULL,
        * next = RegManager::m_Ptr->m_pRegListS;

    while(next != NULL) {
        curReg = next;
		next = curReg->m_pNext;

        if(ProfileManager::m_Ptr->IsProfileAllowed(curReg->m_ui16Profile, ProfileManager::HASKEYICON) == bOperator) {
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, ++i);
#else
            lua_pushinteger(pLua, ++i);
#endif
			PushReg(pLua, curReg);
            lua_rawset(pLua, t);
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetNonOps(lua_State * pLua) {
    return GetRegsByOpStatus(pLua, false);
}
//------------------------------------------------------------------------------

static int GetOps(lua_State * pLua) {
    return GetRegsByOpStatus(pLua, true);
}
//------------------------------------------------------------------------------

static int GetReg(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'GetReg' (1 expected, got %d)", lua_gettop(pLua));
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
    char * sNick = (char *)lua_tolstring(pLua, 1, &szLen);

    if(szLen == 0) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    RegUser * r = RegManager::m_Ptr->Find(sNick, szLen);

    lua_settop(pLua, 0);

    if(r == NULL) {
		lua_pushnil(pLua);
        return 1;
    }

    PushReg(pLua, r);

    return 1;
}
//------------------------------------------------------------------------------

static int GetRegs(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetRegs' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }
    
    lua_newtable(pLua);
    int t = lua_gettop(pLua), i = 0;

    RegUser * curReg = NULL,
        * next = RegManager::m_Ptr->m_pRegListS;

    while(next != NULL) {
        curReg = next;
		next = curReg->m_pNext;

#if LUA_VERSION_NUM < 503
		lua_pushnumber(pLua, ++i);
#else
        lua_pushinteger(pLua, ++i);
#endif

		PushReg(pLua, curReg); 
    
        lua_rawset(pLua, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int AddReg(lua_State * pLua) {
	if(lua_gettop(pLua) == 3) {
        if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TNUMBER) {
            luaL_checktype(pLua, 1, LUA_TSTRING);
            luaL_checktype(pLua, 2, LUA_TSTRING);
            luaL_checktype(pLua, 3, LUA_TNUMBER);
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        size_t szNickLen, szPassLen;
        char *sNick = (char *)lua_tolstring(pLua, 1, &szNickLen);
        char *sPass = (char *)lua_tolstring(pLua, 2, &szPassLen);

#if LUA_VERSION_NUM < 503
		uint16_t i16Profile = (uint16_t)lua_tonumber(pLua, 3);
#else
        uint16_t i16Profile = (uint16_t)lua_tointeger(pLua, 3);
#endif

        if(i16Profile > ProfileManager::m_Ptr->m_ui16ProfileCount-1 || szNickLen == 0 || szNickLen > 64 || szPassLen == 0 || szPassLen > 64 || strpbrk(sNick, " $|") != NULL || strchr(sPass, '|') != NULL) {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        bool bAdded = RegManager::m_Ptr->AddNew(sNick, sPass, i16Profile);

        lua_settop(pLua, 0);

        if(bAdded == false) {
            lua_pushnil(pLua);
            return 1;
        }

        lua_pushboolean(pLua, 1);
        return 1;
    } else if(lua_gettop(pLua) == 2) {
        if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TNUMBER) {
            luaL_checktype(pLua, 1, LUA_TSTRING);
            luaL_checktype(pLua, 2, LUA_TNUMBER);
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        size_t szNickLen;
        char *sNick = (char *)lua_tolstring(pLua, 1, &szNickLen);

#if LUA_VERSION_NUM < 503
		uint16_t ui16Profile = (uint16_t)lua_tonumber(pLua, 2);
#else
        uint16_t ui16Profile = (uint16_t)lua_tointeger(pLua, 2);
#endif

        if(ui16Profile > ProfileManager::m_Ptr->m_ui16ProfileCount-1 || szNickLen == 0 || szNickLen > 64 || strpbrk(sNick, " $|") != NULL) {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        // check if user is registered
        if(RegManager::m_Ptr->Find(sNick, szNickLen) != NULL) {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        User * pUser = HashManager::m_Ptr->FindUser(sNick, szNickLen);
        if(pUser == NULL) {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        if(pUser->m_pLogInOut == NULL) {
            pUser->m_pLogInOut = new (std::nothrow) LoginLogout();
            if(pUser->m_pLogInOut == NULL) {
                pUser->m_ui32BoolBits |= User::BIT_ERROR;
                pUser->Close();

                AppendDebugLog("%s - [MEM] Cannot allocate new pUser->pLogInOut in RegMan.AddReg\n");
                lua_settop(pLua, 0);
                lua_pushnil(pLua);
                return 1;
            }
        }

        pUser->SetBuffer(ProfileManager::m_Ptr->m_ppProfilesTable[ui16Profile]->m_sName);
        pUser->m_ui32BoolBits |= User::BIT_WAITING_FOR_PASS;

        pUser->SendFormat("RegMan.AddReg", true, "<%s> %s.|$GetPass|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_WERE_REGISTERED_PLEASE_ENTER_YOUR_PASSWORD]);

        lua_settop(pLua, 0);
        lua_pushboolean(pLua, 1);
        return 1;
    } else {
        luaL_error(pLua, "bad argument count to 'RegMan.AddReg' (2 or 3 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }
}
//------------------------------------------------------------------------------

static int DelReg(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'DelReg' (1 expected, got %d)", lua_gettop(pLua));
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

    size_t szNickLen;
    char * sNick = (char *)lua_tolstring(pLua, 1, &szNickLen);

    if(szNickLen == 0) {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

	RegUser *reg = RegManager::m_Ptr->Find(sNick, szNickLen);

    lua_settop(pLua, 0);

    if(reg == NULL) {
        lua_pushnil(pLua);
        return 1;
    }

    RegManager::m_Ptr->Delete(reg);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int ChangeReg(lua_State * pLua) {
	if(lua_gettop(pLua) != 3) {
        luaL_error(pLua, "bad argument count to 'ChangeReg' (3 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TNUMBER) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TNUMBER);
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    size_t szNickLen, szPassLen = 0;
    char * sNick = (char *)lua_tolstring(pLua, 1, &szNickLen);
    char * sPass = NULL;

    if(lua_type(pLua, 2) == LUA_TSTRING) {
        sPass = (char *)lua_tolstring(pLua, 2, &szPassLen);
        if(szPassLen == 0 || szPassLen > 64 || strpbrk(sPass, "|") != NULL) {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }
    } else if(lua_type(pLua, 2) != LUA_TNIL) {
        luaL_checktype(pLua, 2, LUA_TSTRING);
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

#if LUA_VERSION_NUM < 503
	uint16_t i16Profile = (uint16_t)lua_tonumber(pLua, 3);
#else
	uint16_t i16Profile = (uint16_t)lua_tointeger(pLua, 3);
#endif

	if(i16Profile > ProfileManager::m_Ptr->m_ui16ProfileCount-1 || szNickLen == 0 || szNickLen > 64 || strpbrk(sNick, " $|") != NULL) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

	RegUser *reg = RegManager::m_Ptr->Find(sNick, szNickLen);

    if(reg == NULL) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    RegManager::m_Ptr->ChangeReg(reg, sPass, i16Profile);

    lua_settop(pLua, 0);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int ClrRegBadPass(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'ClrRegBadPass' (1 expected, got %d)", lua_gettop(pLua));
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
    
    size_t szNickLen;
    char * sNick = (char*)lua_tolstring(pLua, 1, &szNickLen);

    if(szNickLen != 0) {
        RegUser *Reg = RegManager::m_Ptr->Find(sNick, szNickLen);
        if(Reg != NULL) {
            Reg->m_ui8BadPassCount = 0;
        } else {
			lua_settop(pLua, 0);
			lua_pushnil(pLua);
			return 1;
        }
    }

	lua_settop(pLua, 0);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static const luaL_Reg RegManRegs[] = {
    { "Save", Save },
	{ "GetRegsByProfile", GetRegsByProfile }, 
	{ "GetNonOps", GetNonOps }, 
	{ "GetOps", GetOps }, 
	{ "GetReg", GetReg }, 
	{ "GetRegs", GetRegs }, 
	{ "AddReg", AddReg }, 
	{ "DelReg", DelReg }, 
	{ "ChangeReg", ChangeReg }, 
	{ "ClrRegBadPass", ClrRegBadPass }, 
	{ NULL, NULL }
};
//---------------------------------------------------------------------------

#if LUA_VERSION_NUM > 501
int RegRegMan(lua_State * pLua) {
    luaL_newlib(pLua, RegManRegs);
    return 1;
#else
void RegRegMan(lua_State * pLua) {
    luaL_register(pLua, "RegMan", RegManRegs);
#endif
}
//---------------------------------------------------------------------------
