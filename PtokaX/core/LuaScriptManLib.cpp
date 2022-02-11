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
#include "LuaScriptManLib.h"
//---------------------------------------------------------------------------
#include "eventqueue.h"
#include "LuaScriptManager.h"
#include "ServerManager.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "LuaScript.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
    #include "../gui.win/MainWindowPageScripts.h"
#endif
//---------------------------------------------------------------------------

static int GetScript(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetScript' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

	Script * cur = ScriptManager::m_Ptr->FindScript(pLua);
	if(cur == NULL) {
        lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

	lua_newtable(pLua);
	int s = lua_gettop(pLua);

	lua_pushliteral(pLua, "sName");
	lua_pushstring(pLua, cur->m_sName);
	lua_rawset(pLua, s);

	lua_pushliteral(pLua, "bEnabled");
	lua_pushboolean(pLua, 1);
	lua_rawset(pLua, s);

	lua_pushliteral(pLua, "iMemUsage");

#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, lua_gc(cur->m_pLua, LUA_GCCOUNT, 0));
#else
	lua_pushinteger(pLua, lua_gc(cur->m_pLua, LUA_GCCOUNT, 0));
#endif

	lua_rawset(pLua, s);

    return 1;
}
//------------------------------------------------------------------------------

static int GetScripts(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetScripts' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

	lua_newtable(pLua);

	int t = lua_gettop(pLua), n = 0;

	for(uint8_t ui8i = 0; ui8i < ScriptManager::m_Ptr->m_ui8ScriptCount; ui8i++) {
#if LUA_VERSION_NUM < 503
		lua_pushnumber(pLua, ++n);
#else
		lua_pushinteger(pLua, ++n);
#endif

		lua_newtable(pLua);
		int s = lua_gettop(pLua);

		lua_pushliteral(pLua, "sName");
		lua_pushstring(pLua, ScriptManager::m_Ptr->m_ppScriptTable[ui8i]->m_sName);
		lua_rawset(pLua, s);

		lua_pushliteral(pLua, "bEnabled");
		ScriptManager::m_Ptr->m_ppScriptTable[ui8i]->m_bEnabled == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
		lua_rawset(pLua, s);

		lua_pushliteral(pLua, "iMemUsage");
		ScriptManager::m_Ptr->m_ppScriptTable[ui8i]->m_pLua == NULL ? lua_pushnil(pLua) :
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, lua_gc(ScriptManager::m_Ptr->m_ppScriptTable[ui8i]->m_pLua, LUA_GCCOUNT, 0));
#else
            lua_pushinteger(pLua, lua_gc(ScriptManager::m_Ptr->m_ppScriptTable[ui8i]->m_pLua, LUA_GCCOUNT, 0));
#endif

		lua_rawset(pLua, s);

		lua_rawset(pLua, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int Move(lua_State * pLua, const bool bUp) {
	if(lua_gettop(pLua) != 1) {
		luaL_error(pLua, "bad argument count to '%s' (1 expected, got %d)", bUp == true ? "MoveUp" : "MoveDown", lua_gettop(pLua));
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
    char *sName = (char *)lua_tolstring(pLua, 1, &szLen);
    
    if(szLen == 0) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    uint8_t ui8idx = ScriptManager::m_Ptr->FindScriptIdx(sName);
	if(ui8idx == ScriptManager::m_Ptr->m_ui8ScriptCount || (bUp == true && ui8idx == 0) ||
		(bUp == false && ui8idx == ScriptManager::m_Ptr->m_ui8ScriptCount-1)) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    ScriptManager::m_Ptr->PrepareMove(pLua);

	ScriptManager::m_Ptr->MoveScript(ui8idx, bUp);

#ifdef _BUILD_GUI
    MainWindowPageScripts::m_Ptr->MoveScript(ui8idx, bUp);
#endif

    lua_settop(pLua, 0);
	lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int MoveUp(lua_State * pLua) {
	return Move(pLua, true);
}
//------------------------------------------------------------------------------

static int MoveDown(lua_State * pLua) {
    return Move(pLua, false);
}
//------------------------------------------------------------------------------

static int StartScript(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'StartScript' (1 expected, got %d)", lua_gettop(pLua));
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
    char * sName = (char *)lua_tolstring(pLua, 1, &szLen);
    
    if(szLen == 0) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

	if(FileExist((ServerManager::m_sScriptPath + sName).c_str()) == false) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
	}

    Script * curScript = ScriptManager::m_Ptr->FindScript(sName);
    if(curScript != NULL) {
        lua_settop(pLua, 0);

        if(curScript->m_pLua != NULL) {
    		lua_pushnil(pLua);
            return 1;
        }

		if(ScriptManager::m_Ptr->StartScript(curScript, true) == false) {
    		lua_pushnil(pLua);
            return 1;
        }

    	lua_pushboolean(pLua, 1);
        return 1;
	}

	if(ScriptManager::m_Ptr->AddScript(sName, true, true) == true && ScriptManager::m_Ptr->StartScript(ScriptManager::m_Ptr->m_ppScriptTable[ScriptManager::m_Ptr->m_ui8ScriptCount-1], false) == true) {
        lua_settop(pLua, 0);
    	lua_pushboolean(pLua, 1);
        return 1;
    }

    lua_settop(pLua, 0);
    lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int RestartScript(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'RestartScript' (1 expected, got %d)", lua_gettop(pLua));
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
    char *sName = (char *)lua_tolstring(pLua, 1, &szLen);
    
    if(szLen == 0) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    Script * curScript = ScriptManager::m_Ptr->FindScript(sName);
    if(curScript == NULL || curScript->m_pLua == NULL) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    lua_settop(pLua, 0);

    if(curScript->m_pLua == pLua) {
        EventQueue::m_Ptr->AddNormal(EventQueue::EVENT_RSTSCRIPT, curScript->m_sName);

    	lua_pushboolean(pLua, 1);
        return 1;
    }

    ScriptManager::m_Ptr->StopScript(curScript, false);

    if(ScriptManager::m_Ptr->StartScript(curScript, false) == true) {
    	lua_pushboolean(pLua, 1);
        return 1;
    }

	lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int StopScript(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'StopScript' (1 expected, got %d)", lua_gettop(pLua));
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
    char * sName = (char *)lua_tolstring(pLua, 1, &szLen);

    if(szLen == 0) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    Script * curScript = ScriptManager::m_Ptr->FindScript(sName);
    if(curScript == NULL || curScript->m_pLua == NULL) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
	}

	lua_settop(pLua, 0);

    if(curScript->m_pLua == pLua) {
		EventQueue::m_Ptr->AddNormal(EventQueue::EVENT_STOPSCRIPT, curScript->m_sName);

    	lua_pushboolean(pLua, 1);
        return 1;
    }

    ScriptManager::m_Ptr->StopScript(curScript, true);

	lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int Restart(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'Restart' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    EventQueue::m_Ptr->AddNormal(EventQueue::EVENT_RSTSCRIPTS, NULL);

    return 0;
}
//------------------------------------------------------------------------------

static int Refresh(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'Refresh' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    ScriptManager::m_Ptr->CheckForDeletedScripts();
	ScriptManager::m_Ptr->CheckForNewScripts();

#ifdef _BUILD_GUI
	MainWindowPageScripts::m_Ptr->AddScriptsToList(true);
#endif

    return 0;
}
//------------------------------------------------------------------------------

static const luaL_Reg ScriptManRegs[] = {
	{ "GetScript", GetScript },
	{ "GetScripts", GetScripts }, 
	{ "MoveUp", MoveUp }, 
	{ "MoveDown", MoveDown }, 
	{ "StartScript", StartScript }, 
	{ "RestartScript", RestartScript }, 
    { "StopScript", StopScript }, 
	{ "Restart", Restart }, 
	{ "Refresh", Refresh }, 
	{ NULL, NULL }
};
//---------------------------------------------------------------------------

#if LUA_VERSION_NUM > 501
int RegScriptMan(lua_State * pLua) {
    luaL_newlib(pLua, ScriptManRegs);
    return 1;
#else
void RegScriptMan(lua_State * pLua) {
    luaL_register(pLua, "ScriptMan", ScriptManRegs);
#endif
}
//---------------------------------------------------------------------------
