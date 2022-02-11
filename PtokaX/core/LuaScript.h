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
#ifndef LuaScriptH
#define LuaScriptH
//---------------------------------------------------------------------------
struct User;
struct Script;
//---------------------------------------------------------------------------

struct ScriptBot {
	ScriptBot * m_pPrev, * m_pNext;

    char * m_sNick;
    char * m_sMyINFO;

    bool m_bIsOP;

    ScriptBot();
    ~ScriptBot();

    ScriptBot(const ScriptBot&);
    const ScriptBot& operator=(const ScriptBot&);

    static ScriptBot * CreateScriptBot(char * sBotNick, const size_t szNickLen, char * sDescription, const size_t szDscrLen, char * sEmail, const size_t szEmailLen, const bool bOP);
};
//------------------------------------------------------------------------------

struct ScriptTimer {
#if defined(_WIN32) && !defined(_WIN_IOT)
    UINT_PTR m_uiTimerId;
#else
	uint64_t m_ui64Interval;
	uint64_t m_ui64LastTick;
#endif

	ScriptTimer * m_pPrev, * m_pNext;

	lua_State * m_pLua;

    char * m_sFunctionName;

    int m_iFunctionRef;

	static char m_sDefaultTimerFunc[];

    ScriptTimer();
    ~ScriptTimer();

    ScriptTimer(const ScriptTimer&);
    const ScriptTimer& operator=(const ScriptTimer&);

#if defined(_WIN32) && !defined(_WIN_IOT)
    static ScriptTimer * CreateScriptTimer(UINT_PTR uiTmrId, char * sFunctName, const size_t szLen, const int iRef, lua_State * pLuaState);
#else
	static ScriptTimer * CreateScriptTimer(char * sFunctName, const size_t szLen, const int iRef, lua_State * pLuaState);
#endif
};
//------------------------------------------------------------------------------

struct Script {
    Script * m_pPrev, * m_pNext;

    ScriptBot * m_pBotList;

    lua_State * m_pLua;

    char * m_sName;

    uint32_t m_ui32DataArrivals;

	uint16_t m_ui16Functions;

    bool m_bEnabled, m_bRegUDP, m_bProcessed;

    enum LuaFunctions {
		ONSTARTUP         = 0x1,
		ONEXIT            = 0x2,
		ONERROR           = 0x4,
		USERCONNECTED     = 0x8,
		REGCONNECTED      = 0x10,
		OPCONNECTED       = 0x20,
		USERDISCONNECTED  = 0x40,
		REGDISCONNECTED   = 0x80,
		OPDISCONNECTED    = 0x100
	};

    Script();
    ~Script();

    Script(const Script&);
    const Script& operator=(const Script&);

    static Script * CreateScript(char * sName, const bool enabled);
};
//------------------------------------------------------------------------------

bool ScriptStart(Script * pScript);
void ScriptStop(Script * pScript);

int ScriptGetGC(Script * pScript);

void ScriptOnStartup(Script * pScript);
void ScriptOnExit(Script * pScript);

void ScriptPushUser(lua_State * pLua, User * pUser, const bool bFullTable = false);
void ScriptPushUserExtended(lua_State * pLua, User * pUser, const int iTable);

User * ScriptGetUser(lua_State * pLua, const int iTop, const char * sFunction);

void ScriptError(Script * pScript);

#if defined(_WIN32) && !defined(_WIN_IOT)
    void ScriptOnTimer(const UINT_PTR uiTimerId);
#else
	void ScriptOnTimer(const uint64_t &ui64ActualMillis);
#endif

int ScriptTraceback(lua_State * pLua);
//------------------------------------------------------------------------------

#endif
