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
#include "LuaUDPDbgLib.h"
//---------------------------------------------------------------------------
#include "LuaScriptManager.h"
#include "UdpDebug.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "LuaScript.h"
//---------------------------------------------------------------------------

static int Reg(lua_State * pLua) {
	if(lua_gettop(pLua) != 3) {
        luaL_error(pLua, "bad argument count to 'Reg' (3 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TNUMBER || lua_type(pLua, 3) != LUA_TBOOLEAN) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        luaL_checktype(pLua, 3, LUA_TBOOLEAN);

		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

	Script * cur = ScriptManager::m_Ptr->FindScript(pLua);
	if(cur == NULL || cur->m_bRegUDP == true) {
        lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    size_t szLen;
    char * sIP = (char *)lua_tolstring(pLua, 1, &szLen);

    if(szLen < 7 || szLen > 39 || isIP(sIP) == false) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

#if LUA_VERSION_NUM < 503
	uint16_t usPort = (uint16_t)lua_tonumber(pLua, 2);
#else
    uint16_t usPort = (uint16_t)lua_tointeger(pLua, 2);
#endif

    bool bAllData = lua_toboolean(pLua, 3) == 0 ? false : true;

    if(UdpDebug::m_Ptr->New(sIP, usPort, bAllData, cur->m_sName) == false) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    cur->m_bRegUDP = true;

    lua_settop(pLua, 0);
    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int Unreg(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'Unreg' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

	Script * cur = ScriptManager::m_Ptr->FindScript(pLua);
	if(cur == NULL || cur->m_bRegUDP == false) {
        return 0;
    }

    UdpDebug::m_Ptr->Remove(cur->m_sName);

    cur->m_bRegUDP = false;

    return 0;
}
//------------------------------------------------------------------------------

static int Send(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'Send' (1 expected, got %d)", lua_gettop(pLua));
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
    char * sMsg = (char *)lua_tolstring(pLua, 1, &szLen);

    if(szLen == 0) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

	Script * cur = ScriptManager::m_Ptr->FindScript(pLua);
	if(cur == NULL || cur->m_bRegUDP == false) {
        UdpDebug::m_Ptr->Broadcast(sMsg, szLen);
        lua_settop(pLua, 0);
        lua_pushboolean(pLua, 1);
        return 1;
    }

    UdpDebug::m_Ptr->Send(cur->m_sName, sMsg, szLen);

	lua_settop(pLua, 0);
    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static const luaL_Reg UdpDbgRegs[] = {
	{ "Reg", Reg }, 
	{ "Unreg", Unreg }, 
	{ "Send", Send }, 
	{ NULL, NULL }
};
//---------------------------------------------------------------------------

#if LUA_VERSION_NUM > 501
int RegUDPDbg(lua_State * pLua) {
    luaL_newlib(pLua, UdpDbgRegs);
    return 1;
#else
void RegUDPDbg(lua_State * pLua) {
    luaL_register(pLua, "UDPDbg", UdpDbgRegs);
#endif
}
//---------------------------------------------------------------------------
