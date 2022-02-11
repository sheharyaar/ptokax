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
#include "LuaIP2CountryLib.h"
//---------------------------------------------------------------------------
#include "LuaScriptManager.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "IP2Country.h"
#include "LuaScript.h"
//---------------------------------------------------------------------------

static int GetCountryCode(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
		luaL_error(pLua, "bad argument count to 'GetCountryCode' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING) {
        luaL_error(pLua, "bad argument to 'GetCountryCode' (string expected, got %s)", lua_typename(pLua, lua_type(pLua, 1)));
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    size_t szLen;
    char * sIP = (char *)lua_tolstring(pLua, 1, &szLen);

    uint8_t ui128Hash[16];
    memset(ui128Hash, 0, 16);

    if(szLen == 0 || HashIP(sIP, ui128Hash) == false) {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    const char * sCountry = IpP2Country::m_Ptr->Find(ui128Hash, false);

    lua_settop(pLua, 0);

    lua_pushlstring(pLua, sCountry, 2);

    return 1;
}
//------------------------------------------------------------------------------

static int GetCountryName(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
		luaL_error(pLua, "bad argument count to 'GetCountryName' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    const char * sCountry;

    if(lua_type(pLua, 1) == LUA_TSTRING) {
        size_t szLen;
        char * sIP = (char *)lua_tolstring(pLua, 1, &szLen);

        uint8_t ui128Hash[16];
        memset(ui128Hash, 0, 16);

        if(szLen == 0 || HashIP(sIP, ui128Hash) == false) {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        sCountry = IpP2Country::m_Ptr->Find(ui128Hash, true);
    } else if(lua_type(pLua, 1) == LUA_TTABLE) {
		User * u = ScriptGetUser(pLua, 1, "GetCountryName");
        if(u == NULL) {
    		lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }

        sCountry = IpP2Country::m_Ptr->GetCountry(u->m_ui8Country, true);
    } else {
        luaL_error(pLua, "bad argument to 'GetCountryName' (string or table expected, got %s)", lua_typename(pLua, lua_type(pLua, 1)));
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    lua_settop(pLua, 0);

    lua_pushstring(pLua, sCountry);

    return 1;
}
//------------------------------------------------------------------------------

static int Reload(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'IP2Country.Reload' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

	IpP2Country::m_Ptr->Reload();

    return 0;
}
//------------------------------------------------------------------------------

static const luaL_Reg Ip2CountryRegs[] = {
	{ "GetCountryCode", GetCountryCode },
	{ "GetCountryName", GetCountryName }, 
	{ "Reload", Reload },
	{ NULL, NULL }
};
//---------------------------------------------------------------------------

#if LUA_VERSION_NUM > 501
int RegIP2Country(lua_State * pLua) {
    luaL_newlib(pLua, Ip2CountryRegs);
    return 1;
#else
void RegIP2Country(lua_State * pLua) {
    luaL_register(pLua, "IP2Country", Ip2CountryRegs);
#endif
}
//---------------------------------------------------------------------------
