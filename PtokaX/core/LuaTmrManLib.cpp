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
#include "LuaTmrManLib.h"
//---------------------------------------------------------------------------
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

static int AddTimer(lua_State * pLua) {
	Script * cur = ScriptManager::m_Ptr->FindScript(pLua);
    if(cur == NULL) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
	}

    int n = lua_gettop(pLua);

	size_t szLen = 0;
    char * sFunctionName = NULL;
    int iRef = 0;

	if(n == 2) {
        if(lua_type(pLua, 1) != LUA_TNUMBER) {
            luaL_checktype(pLua, 1, LUA_TNUMBER);
    		lua_settop(pLua, 0);
    		lua_pushnil(pLua);
            return 1;
        }

        if(lua_type(pLua, 2) == LUA_TSTRING) {
            sFunctionName = (char *)lua_tolstring(pLua, 2, &szLen);

            if(szLen == 0) {
				lua_settop(pLua, 0);
				lua_pushnil(pLua);
                return 1;
            }
        } else if(lua_type(pLua, 2) == LUA_TFUNCTION) {
            iRef = luaL_ref(pLua, LUA_REGISTRYINDEX);
        } else {
            luaL_error(pLua, "bad argument #2 to 'AddTimer' (string or function expected, got %s)", lua_typename(pLua, lua_type(pLua, 2)));
    		lua_settop(pLua, 0);
    		lua_pushnil(pLua);
            return 1;
        }
	} else if(n == 1) {
        if(lua_type(pLua, 1) != LUA_TNUMBER) {
            luaL_checktype(pLua, 1, LUA_TNUMBER);
    		lua_settop(pLua, 0);
    		lua_pushnil(pLua);
            return 1;
		}

        sFunctionName = ScriptTimer::m_sDefaultTimerFunc;
    } else {
        luaL_error(pLua, "bad argument count to 'AddTimer' (1 or 2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(sFunctionName != NULL) {
        lua_getglobal(pLua, sFunctionName);
        int i = lua_gettop(pLua);
        if(lua_isfunction(pLua, i) == 0) {
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
			return 1;
        }
    }

#if defined(_WIN32) && !defined(_WIN_IOT)
	#if LUA_VERSION_NUM < 503
		UINT_PTR timer = SetTimer(NULL, 0, (UINT)lua_tonumber(pLua, 1), NULL);
	#else
		UINT_PTR timer = SetTimer(NULL, 0, (UINT)lua_tointeger(pLua, 1), NULL);
#endif

    if(timer == 0) {
        lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

	ScriptTimer * pNewtimer = ScriptTimer::CreateScriptTimer(timer, sFunctionName, szLen, iRef, cur->m_pLua);
#else
	ScriptTimer * pNewtimer = ScriptTimer::CreateScriptTimer(sFunctionName, szLen, iRef, cur->m_pLua);
#endif

    if(pNewtimer == NULL) {
#if defined(_WIN32) && !defined(_WIN_IOT)
        KillTimer(NULL, timer);
#endif
        AppendDebugLog("%s - [MEM] Cannot allocate pNewtimer in TmrMan.AddTimer\n");
        lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

#if !defined(_WIN32) || (defined(_WIN32) && defined(_WIN_IOT))
#if LUA_VERSION_NUM < 503
	pNewtimer->m_ui64Interval = (uint64_t)lua_tonumber(pLua, 1);// ms
#else
    pNewtimer->m_ui64Interval = (uint64_t)lua_tointeger(pLua, 1);// ms
#endif
	#ifdef __MACH__
		mach_timespec_t mts;
		clock_get_time(ServerManager::m_csMachClock, &mts);
		pNewtimer->m_ui64LastTick = (uint64_t(mts.tv_sec) * 1000) + (uint64_t(mts.tv_nsec) / 1000000);
	#elif _WIN32
		pNewtimer->m_ui64LastTick = ::GetTickCount64();
	#else
		timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		pNewtimer->m_ui64LastTick = (uint64_t(ts.tv_sec) * 1000) + (uint64_t(ts.tv_nsec) / 1000000);
	#endif
#endif

    lua_settop(pLua, 0);

#if defined(_WIN32) && !defined(_WIN_IOT)
    lua_pushlightuserdata(pLua, (void *)pNewtimer->m_uiTimerId);
#else
    lua_pushlightuserdata(pLua, (void *)pNewtimer);
#endif

    if(ScriptManager::m_Ptr->m_pTimerListS == NULL) {
        ScriptManager::m_Ptr->m_pTimerListS = pNewtimer;
        ScriptManager::m_Ptr->m_pTimerListE = pNewtimer;
    } else {
    	pNewtimer->m_pPrev = ScriptManager::m_Ptr->m_pTimerListE;
    	ScriptManager::m_Ptr->m_pTimerListE->m_pNext = pNewtimer;
    	ScriptManager::m_Ptr->m_pTimerListE = pNewtimer;
    }

    return 1;
}
//------------------------------------------------------------------------------

static int RemoveTimer(lua_State * pLua) {
    if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'RemoveTimer' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TLIGHTUSERDATA) {
        luaL_checktype(pLua, 1, LUA_TLIGHTUSERDATA);
		lua_settop(pLua, 0);
        return 0;
    }
    
	Script * cur = ScriptManager::m_Ptr->FindScript(pLua);
    if(cur == NULL) {
		lua_settop(pLua, 0);
        return 0;
	}

#if defined(_WIN32) && !defined(_WIN_IOT)
    UINT_PTR timer = (UINT_PTR)lua_touserdata(pLua, 1);
#else
	ScriptTimer * timer = reinterpret_cast<ScriptTimer *>(lua_touserdata(pLua, 1));
#endif

    ScriptTimer * pCurTmr = NULL,
        * pNextTmr = ScriptManager::m_Ptr->m_pTimerListS;

    while(pNextTmr != NULL) {
        pCurTmr = pNextTmr;
        pNextTmr = pCurTmr->m_pNext;

#if defined(_WIN32) && !defined(_WIN_IOT)
        if(pCurTmr->m_uiTimerId == timer) {
            KillTimer(NULL, pCurTmr->m_uiTimerId);
#else
        if(pCurTmr == timer) {
#endif
			if(pCurTmr->m_pPrev == NULL) {
				if(pCurTmr->m_pNext == NULL) {
					ScriptManager::m_Ptr->m_pTimerListS = NULL;
					ScriptManager::m_Ptr->m_pTimerListE = NULL;
				} else {
					ScriptManager::m_Ptr->m_pTimerListS = pCurTmr->m_pNext;
					ScriptManager::m_Ptr->m_pTimerListS->m_pPrev = NULL;
				}
			} else if(pCurTmr->m_pNext == NULL) {
				ScriptManager::m_Ptr->m_pTimerListE = pCurTmr->m_pPrev;
				ScriptManager::m_Ptr->m_pTimerListE->m_pNext = NULL;
			} else {
				pCurTmr->m_pPrev->m_pNext = pCurTmr->m_pNext;
				pCurTmr->m_pNext->m_pPrev = pCurTmr->m_pPrev;
			}

            if(pCurTmr->m_sFunctionName == NULL) {
                luaL_unref(pLua, LUA_REGISTRYINDEX, pCurTmr->m_iFunctionRef);
            }

            delete pCurTmr;

            break;
        }
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static const luaL_Reg TmrManRegs[] = {
	{ "AddTimer", AddTimer },
	{ "RemoveTimer", RemoveTimer }, 
	{ NULL, NULL }
};
//---------------------------------------------------------------------------

#if LUA_VERSION_NUM > 501
int RegTmrMan(lua_State * pLua) {
    luaL_newlib(pLua, TmrManRegs);
    return 1;
#else
void RegTmrMan(lua_State * pLua) {
    luaL_register(pLua, "TmrMan", TmrManRegs);
#endif
}
//---------------------------------------------------------------------------
