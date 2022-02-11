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
#include "LuaCoreLib.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "eventqueue.h"
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "IP2Country.h"
#include "ResNickManager.h"
#include "LuaScript.h"
//---------------------------------------------------------------------------

static int Restart(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'Restart' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

	EventQueue::m_Ptr->AddNormal(EventQueue::EVENT_RESTART, NULL);

    return 0;
}
//------------------------------------------------------------------------------

static int Shutdown(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'Shutdown' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

	EventQueue::m_Ptr->AddNormal(EventQueue::EVENT_SHUTDOWN, NULL);

    return 0;
}
//------------------------------------------------------------------------------

static int ResumeAccepts(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'ResumeAccepts' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

	ServerManager::ResumeAccepts();

    return 0;
}
//------------------------------------------------------------------------------

static int SuspendAccepts(lua_State * pLua) {
    int n = lua_gettop(pLua);

    if(n == 0) {
        ServerManager::SuspendAccepts(0);
    } else if(n == 1) {
        if(lua_type(pLua, 1) != LUA_TNUMBER) {
            luaL_checktype(pLua, 1, LUA_TNUMBER);
    		lua_settop(pLua, 0);
            return 0;
        }

#if LUA_VERSION_NUM < 503
		uint32_t iSec = (uint32_t)lua_tonumber(pLua, 1);
#else
    	uint32_t iSec = (uint32_t)lua_tointeger(pLua, 1);
#endif
    
        if(iSec != 0) {
            ServerManager::SuspendAccepts(iSec);
        }
        lua_settop(pLua, 0);
    } else {
        luaL_error(pLua, "bad argument count to 'SuspendAccepts' (0 or 1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
    }
    
    return 0;
}
//------------------------------------------------------------------------------

static int RegBot(lua_State * pLua) {
    if(ScriptManager::m_Ptr->m_ui8BotsCount > 63) {
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

	if(lua_gettop(pLua) != 4) {
        luaL_error(pLua, "bad argument count to 'RegBot' (4 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING || lua_type(pLua, 4) != LUA_TBOOLEAN) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        luaL_checktype(pLua, 4, LUA_TBOOLEAN);

		lua_settop(pLua, 0);

		lua_pushnil(pLua);
        return 1;
    }

    size_t szNickLen, szDescrLen, szEmailLen;

    char *nick = (char *)lua_tolstring(pLua, 1, &szNickLen);
    char *description = (char *)lua_tolstring(pLua, 2, &szDescrLen);
    char *email = (char *)lua_tolstring(pLua, 3, &szEmailLen);

    bool bIsOP = lua_toboolean(pLua, 4) == 0 ? false : true;

    if(szNickLen == 0 || szNickLen > 64 || strpbrk(nick, " $|") != NULL || szDescrLen > 64 || strpbrk(description, "$|") != NULL ||
        szEmailLen > 64 || strpbrk(email, "$|") != NULL || HashManager::m_Ptr->FindUser(nick, szNickLen) != NULL || ReservedNicksManager::m_Ptr->CheckReserved(nick, HashNick(nick, szNickLen)) != false) {
		lua_settop(pLua, 0);

		lua_pushnil(pLua);
        return 1;
    }

    ScriptBot * pNewBot = ScriptBot::CreateScriptBot(nick, szNickLen, description, szDescrLen, email, szEmailLen, bIsOP);
    if(pNewBot == NULL) {
        AppendDebugLog("%s - [MEM] Cannot allocate pNewBot in Core.RegBot\n");

		lua_settop(pLua, 0);

		lua_pushnil(pLua);
        return 1;
    }

    // PPK ... finally we can clear stack
    lua_settop(pLua, 0);

	Script * pScript = ScriptManager::m_Ptr->FindScript(pLua);
	if(pScript == NULL) {
		delete pNewBot;

		lua_pushnil(pLua);
        return 1;
    }

    ScriptBot * cur = NULL,
        * next = pScript->m_pBotList;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

		if(strcasecmp(pNewBot->m_sNick, cur->m_sNick) == 0) {
            delete pNewBot;

            lua_pushnil(pLua);
            return 1;
        }
    }

    if(pScript->m_pBotList == NULL) {
		pScript->m_pBotList = pNewBot;
    } else {
		pScript->m_pBotList->m_pPrev = pNewBot;
        pNewBot->m_pNext = pScript->m_pBotList;
		pScript->m_pBotList = pNewBot;
    }

	ReservedNicksManager::m_Ptr->AddReservedNick(pNewBot->m_sNick, true);

	Users::m_Ptr->AddBot2NickList(pNewBot->m_sNick, szNickLen, pNewBot->m_bIsOP);

    Users::m_Ptr->AddBot2MyInfos(pNewBot->m_sMyINFO);

    // PPK ... fixed hello sending only to users without NoHello
    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Hello %s|", pNewBot->m_sNick);
    if(iMsgLen > 0) {
        GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, iMsgLen, NULL, 0, GlobalDataQueue::CMD_HELLO);
    }

    size_t szMyINFOLen = strlen(pNewBot->m_sMyINFO);
    GlobalDataQueue::m_Ptr->AddQueueItem(pNewBot->m_sMyINFO, szMyINFOLen, pNewBot->m_sMyINFO, szMyINFOLen, GlobalDataQueue::CMD_MYINFO);
        
    if(pNewBot->m_bIsOP == true) {
        GlobalDataQueue::m_Ptr->OpListStore(pNewBot->m_sNick);
    }

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int UnregBot(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'UnregBot' (1 expected, got %d)", lua_gettop(pLua));
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
    char * botnick = (char *)lua_tolstring(pLua, 1, &szLen);
    
    if(szLen == 0) {
		lua_settop(pLua, 0);

		lua_pushnil(pLua);
        return 1;
    }

	Script * obj = ScriptManager::m_Ptr->FindScript(pLua);
	if(obj == NULL) {
		lua_settop(pLua, 0);

		lua_pushnil(pLua);
        return 1;
    }

    ScriptBot * cur = NULL,
        * next = obj->m_pBotList;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

		if(strcasecmp(botnick, cur->m_sNick) == 0) {
            ReservedNicksManager::m_Ptr->DelReservedNick(cur->m_sNick, true);

            Users::m_Ptr->DelFromNickList(cur->m_sNick, cur->m_bIsOP);

            Users::m_Ptr->DelBotFromMyInfos(cur->m_sMyINFO);

            int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Quit %s|", cur->m_sNick);
            if(iMsgLen > 0) {
                GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, iMsgLen, NULL, 0, GlobalDataQueue::CMD_QUIT);
            }

            if(cur->m_pPrev == NULL) {
                if(cur->m_pNext == NULL) {
                    obj->m_pBotList = NULL;
                } else {
                    cur->m_pNext->m_pPrev = NULL;
                    obj->m_pBotList = cur->m_pNext;
                }
            } else if(cur->m_pNext == NULL) {
                cur->m_pPrev->m_pNext = NULL;
            } else {
                cur->m_pPrev->m_pNext = cur->m_pNext;
                cur->m_pNext->m_pPrev = cur->m_pPrev;
            }

            delete cur;

            lua_settop(pLua, 0);

            lua_pushboolean(pLua, 1);
            return 1;
        }
    }

    lua_settop(pLua, 0);

    lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int GetBots(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetBots' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    lua_newtable(pLua);

    int t = lua_gettop(pLua), i = 0;

	Script * cur = NULL,
        * next = ScriptManager::m_Ptr->m_pRunningScriptS;

    while(next != NULL) {
    	cur = next;
        next = cur->m_pNext;

        ScriptBot * pBot = NULL,
            * nextbot = cur->m_pBotList;
        
        while(nextbot != NULL) {
			pBot = nextbot;
            nextbot = pBot->m_pNext;

#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, ++i);
#else
            lua_pushinteger(pLua, ++i);
#endif

            lua_newtable(pLua);

            int b = lua_gettop(pLua);

        	lua_pushliteral(pLua, "sNick");
        	lua_pushstring(pLua, pBot->m_sNick);
        	lua_rawset(pLua, b);

        	lua_pushliteral(pLua, "sMyINFO");
        	lua_pushstring(pLua, pBot->m_sMyINFO);
        	lua_rawset(pLua, b);

        	lua_pushliteral(pLua, "bIsOP");
			pBot->m_bIsOP == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
        	lua_rawset(pLua, b);

        	lua_pushliteral(pLua, "sScriptName");
        	lua_pushstring(pLua, cur->m_sName);
        	lua_rawset(pLua, b);

            lua_rawset(pLua, t);
        }

    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetActualUsersPeak(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetActualUsersPeak' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, ServerManager::m_ui32Peak);
#else
    lua_pushinteger(pLua, ServerManager::m_ui32Peak);
#endif

    return 1;
}
//------------------------------------------------------------------------------

static int GetMaxUsersPeak(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetMaxUsersPeak' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_USERS_PEAK]);
#else
    lua_pushinteger(pLua, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_USERS_PEAK]);
#endif

    return 1;
}
//------------------------------------------------------------------------------

static int GetCurrentSharedSize(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetCurrentSharedSize' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, (double)ServerManager::m_ui64TotalShare);
#else
    lua_pushinteger(pLua, ServerManager::m_ui64TotalShare);
#endif

    return 1;
}
//------------------------------------------------------------------------------

static int GetHubIP(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetHubIP' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    if(ServerManager::m_sHubIP[0] != '\0') {
		lua_pushstring(pLua, ServerManager::m_sHubIP);
	} else {
		lua_pushnil(pLua);
	}

    return 1;
}
//------------------------------------------------------------------------------

static int GetHubIPs(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetHubIPs' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    if(ServerManager::m_sHubIP[0] == '\0' && ServerManager::m_sHubIP6[0] == '\0') {
        lua_pushnil(pLua);
        return 1;
    }

    lua_newtable(pLua);
    int t = lua_gettop(pLua), i = 0;

    if(ServerManager::m_sHubIP6[0] != '\0') {
        i++;

#if LUA_VERSION_NUM < 503
		lua_pushnumber(pLua, i);
#else
        lua_pushinteger(pLua, i);
#endif

		lua_pushstring(pLua, ServerManager::m_sHubIP6);
		lua_rawset(pLua, t);
	}

    if(ServerManager::m_sHubIP[0] != '\0') {
        i++;

#if LUA_VERSION_NUM < 503
		lua_pushnumber(pLua, i);
#else
        lua_pushinteger(pLua, i);
#endif

		lua_pushstring(pLua, ServerManager::m_sHubIP);
		lua_rawset(pLua, t);
	}

    return 1;
}
//------------------------------------------------------------------------------

static int GetHubSecAlias(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetHubSecAlias' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }
    
    lua_pushlstring(pLua, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], (size_t)SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_HUB_SEC]);

    return 1;
}
//------------------------------------------------------------------------------

static int GetPtokaXPath(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetPtokaXPath' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

#ifdef _WIN32
    lua_pushlstring(pLua, ServerManager::m_sLuaPath.c_str(), ServerManager::m_sLuaPath.size());
#else
    string path = ServerManager::m_sPath+"/";
    lua_pushlstring(pLua, path.c_str(), path.size());
#endif

    return 1;
}
//------------------------------------------------------------------------------

static int GetUsersCount(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetUsersCount' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, ServerManager::m_ui32Logged);
#else
    lua_pushinteger(pLua, ServerManager::m_ui32Logged);
#endif

    return 1;
}
//------------------------------------------------------------------------------

static int GetUpTime(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetUpTime' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

	time_t t;
    time(&t);

#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, (double)(t-ServerManager::m_tStartTime));
#else
    lua_pushinteger(pLua, t-ServerManager::m_tStartTime);
#endif

    return 1;
}
//------------------------------------------------------------------------------

static int GetOnlineByOpStatus(lua_State * pLua, const bool bOperator) {
    bool bFullTable = false;

    int n = lua_gettop(pLua);

	if(n != 0) {
        if(n != 1) {
            if(bOperator == true) {
                luaL_error(pLua, "bad argument count to 'GetOnlineOps' (0 or 1 expected, got %d)", lua_gettop(pLua));
            } else {
                luaL_error(pLua, "bad argument count to 'GetOnlineNonOps' (0 or 1 expected, got %d)", lua_gettop(pLua));
            }
            lua_settop(pLua, 0);
    
            lua_pushnil(pLua);
            return 1;
        } else if(lua_type(pLua, 1) != LUA_TBOOLEAN) {
            luaL_checktype(pLua, 1, LUA_TBOOLEAN);

            lua_settop(pLua, 0);

            lua_pushnil(pLua);
            return 1;
        } else {
            bFullTable = lua_toboolean(pLua, 1) == 0 ? false : true;

            lua_settop(pLua, 0);
        }
    }

    lua_newtable(pLua);

    int t = lua_gettop(pLua), i = 0;

    User * curUser = NULL,
        * next = Users::m_Ptr->m_pUserListS;

    while(next != NULL) {
        curUser = next;
        next = curUser->m_pNext;

        if(curUser->m_ui8State == User::STATE_ADDED && ((curUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == bOperator) {
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, ++i);
#else
            lua_pushinteger(pLua, ++i);
#endif

			ScriptPushUser(pLua, curUser, bFullTable);
            lua_rawset(pLua, t);
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetOnlineNonOps(lua_State * pLua) {
	return GetOnlineByOpStatus(pLua, false);
}
//------------------------------------------------------------------------------

static int GetOnlineOps(lua_State * pLua) {
    return GetOnlineByOpStatus(pLua, true);
}
//------------------------------------------------------------------------------

static int GetOnlineRegs(lua_State * pLua) {
    bool bFullTable = false;

    int n = lua_gettop(pLua);

	if(n != 0) {
        if(n != 1) {
            luaL_error(pLua, "bad argument count to 'GetOnlineRegs' (0 or 1 expected, got %d)", lua_gettop(pLua));
            lua_settop(pLua, 0);
    
            lua_pushnil(pLua);
            return 1;
        } else if(lua_type(pLua, 1) != LUA_TBOOLEAN) {
            luaL_checktype(pLua, 1, LUA_TBOOLEAN);

            lua_settop(pLua, 0);

            lua_pushnil(pLua);
            return 1;
        } else {
            bFullTable = lua_toboolean(pLua, 1) == 0 ? false : true;

            lua_settop(pLua, 0);
        }
    }

    lua_newtable(pLua);

    int t = lua_gettop(pLua), i = 0;

    User * curUser = NULL,
        * next = Users::m_Ptr->m_pUserListS;

    while(next != NULL) {
        curUser = next;
        next = curUser->m_pNext;

        if(curUser->m_ui8State == User::STATE_ADDED && curUser->m_i32Profile != -1) {
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, ++i);
#else
            lua_pushinteger(pLua, ++i);
#endif

			ScriptPushUser(pLua, curUser, bFullTable);
            lua_rawset(pLua, t);
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetOnlineUsers(lua_State * pLua) {
    bool bFullTable = false;

    int32_t iProfile = -2;

    int n = lua_gettop(pLua);

    if(n == 2) {
        if(lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TBOOLEAN) {
            luaL_checktype(pLua, 1, LUA_TNUMBER);
            luaL_checktype(pLua, 2, LUA_TBOOLEAN);
    
            lua_settop(pLua, 0);
    
            lua_pushnil(pLua);
            return 1;
        }

        iProfile = (int32_t)lua_tointeger(pLua, 1);
        bFullTable = lua_toboolean(pLua, 2) == 0 ? false : true;

        lua_settop(pLua, 0);
    } else if(n == 1) {
        if(lua_type(pLua, 1) == LUA_TNUMBER) {
            iProfile = (int32_t)lua_tointeger(pLua, 1);
        } else if(lua_type(pLua, 1) == LUA_TBOOLEAN) {
            bFullTable = lua_toboolean(pLua, 1) == 0 ? false : true;
        } else {
            luaL_error(pLua, "bad argument #1 to 'GetOnlineUsers' (number or boolean expected, got %s)", lua_typename(pLua, lua_type(pLua, 1)));
            lua_settop(pLua, 0);
        
            lua_pushnil(pLua);
            return 1;
        }

        lua_settop(pLua, 0);
    } else if(n != 0) {
        luaL_error(pLua, "bad argument count to 'GetOnlineUsers' (0, 1 or 2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        
        lua_pushnil(pLua);
        return 1;
    }

    lua_newtable(pLua);

    int t = lua_gettop(pLua), i = 0;

    User * curUser = NULL,
        * next = Users::m_Ptr->m_pUserListS;

    if(iProfile == -2) {
	    while(next != NULL) {
	        curUser = next;
	        next = curUser->m_pNext;

	        if(curUser->m_ui8State == User::STATE_ADDED) {
#if LUA_VERSION_NUM < 503
				lua_pushnumber(pLua, ++i);
#else
	            lua_pushinteger(pLua, ++i);
#endif

				ScriptPushUser(pLua, curUser, bFullTable);
	            lua_rawset(pLua, t);
	        }
	    }
    } else {
		while(next != NULL) {
		    curUser = next;
		    next = curUser->m_pNext;

		    if(curUser->m_ui8State == User::STATE_ADDED && curUser->m_i32Profile == iProfile) {
#if LUA_VERSION_NUM < 503
				lua_pushnumber(pLua, ++i);
#else
		        lua_pushinteger(pLua, ++i);
#endif

				ScriptPushUser(pLua, curUser, bFullTable);
		        lua_rawset(pLua, t);
		    }
		}
    }
    
    return 1;
}
//------------------------------------------------------------------------------

static int GetUser(lua_State * pLua) {
    bool bFullTable = false;

    size_t szLen = 0;

    char * nick;
    
    int n = lua_gettop(pLua);

    if(n == 2) {
        if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TBOOLEAN) {
            luaL_checktype(pLua, 1, LUA_TSTRING);
            luaL_checktype(pLua, 2, LUA_TBOOLEAN);
    
            lua_settop(pLua, 0);
    
            lua_pushnil(pLua);
            return 1;
        }

        nick = (char *)lua_tolstring(pLua, 1, &szLen);

        bFullTable = lua_toboolean(pLua, 2) == 0 ? false : true;
    } else if(n == 1) {
        if(lua_type(pLua, 1) != LUA_TSTRING) {
            luaL_checktype(pLua, 1, LUA_TSTRING);

            lua_settop(pLua, 0);
    
            lua_pushnil(pLua);
            return 1;
        }

        nick = (char *)lua_tolstring(pLua, 1, &szLen);
    } else {
        luaL_error(pLua, "bad argument count to 'GetUser' (1 or 2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        
        lua_pushnil(pLua);
        return 1;
    }

    if(szLen == 0) {
        lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }
    
    User *u = HashManager::m_Ptr->FindUser(nick, szLen);

    lua_settop(pLua, 0);

    if(u != NULL) {
		ScriptPushUser(pLua, u, bFullTable);
	} else {
        lua_pushnil(pLua);
    }
    
    return 1;
}
//------------------------------------------------------------------------------

static int GetUsers(lua_State * pLua) {
    bool bFullTable = false;

    size_t szLen = 0;

    char * sIP;
    
    int n = lua_gettop(pLua);

    if(n == 2) {
        if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TBOOLEAN) {
            luaL_checktype(pLua, 1, LUA_TSTRING);
            luaL_checktype(pLua, 2, LUA_TBOOLEAN);
    
            lua_settop(pLua, 0);
    
            lua_pushnil(pLua);
            return 1;
        }

        sIP = (char *)lua_tolstring(pLua, 1, &szLen);

        bFullTable = lua_toboolean(pLua, 2) == 0 ? false : true;
    } else if(n == 1) {
        if(lua_type(pLua, 1) != LUA_TSTRING) {
            luaL_checktype(pLua, 1, LUA_TSTRING);

            lua_settop(pLua, 0);
    
            lua_pushnil(pLua);
            return 1;
        }

        sIP = (char *)lua_tolstring(pLua, 1, &szLen);
    } else {
        luaL_error(pLua, "bad argument count to 'GetUsers' (1 or 2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        
        lua_pushnil(pLua);
        return 1;
    }

    uint8_t ui128Hash[16];
    memset(ui128Hash, 0, 16);

    if(szLen == 0 || HashIP(sIP, ui128Hash) == false) {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);

        return 1;
    }

    User * next = HashManager::m_Ptr->FindUser(ui128Hash);

    lua_settop(pLua, 0);

    if(next == NULL) {
        lua_pushnil(pLua);
        return 1;
    }

    lua_newtable(pLua);
    
    int t = lua_gettop(pLua), i = 0;
    
    User * curUser = NULL;

    while(next != NULL) {
		curUser = next;
        next = curUser->m_pHashIpTableNext;

#if LUA_VERSION_NUM < 503
		lua_pushnumber(pLua, ++i);
#else
        lua_pushinteger(pLua, ++i);
#endif

    	ScriptPushUser(pLua, curUser, bFullTable);
        lua_rawset(pLua, t);
    }
    
    return 1;
}
//------------------------------------------------------------------------------

static int GetUserAllData(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'GetUserAllData' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TTABLE) {
        luaL_checktype(pLua, 1, LUA_TTABLE);
		lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    User *u = ScriptGetUser(pLua, 1, "GetUserAllData");

    if(u == NULL) {
		lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    ScriptPushUserExtended(pLua, u, 1);

    lua_settop(pLua, 0);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int GetUserData(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'GetUserData' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TNUMBER) {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
		lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    User *u = ScriptGetUser(pLua, 2, "GetUserData");

    if(u == NULL) {
		lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

#if LUA_VERSION_NUM < 503
	uint8_t ui8DataId = (uint8_t)lua_tonumber(pLua, 2);
#else
    uint8_t ui8DataId = (uint8_t)lua_tointeger(pLua, 2);
#endif

    switch(ui8DataId) {
        case 0:
        	lua_pushliteral(pLua, "sMode");
        	if(u->m_sModes[0] != '\0') {
				lua_pushstring(pLua, u->m_sModes);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 1:
        	lua_pushliteral(pLua, "sMyInfoString");
        	if(u->m_sMyInfoOriginal != NULL) {
				lua_pushlstring(pLua, u->m_sMyInfoOriginal, u->m_ui16MyInfoOriginalLen);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 2:
        	lua_pushliteral(pLua, "sDescription");
        	if(u->m_sDescription != NULL) {
				lua_pushlstring(pLua, u->m_sDescription, (size_t)u->m_ui8DescriptionLen);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 3:
        	lua_pushliteral(pLua, "sTag");
        	if(u->m_sTag != NULL) {
				lua_pushlstring(pLua, u->m_sTag, (size_t)u->m_ui8TagLen);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 4:
        	lua_pushliteral(pLua, "sConnection");
        	if(u->m_sConnection != NULL) {
				lua_pushlstring(pLua, u->m_sConnection, (size_t)u->m_ui8ConnectionLen);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 5:
        	lua_pushliteral(pLua, "sEmail");
        	if(u->m_sEmail != NULL) {
				lua_pushlstring(pLua, u->m_sEmail, (size_t)u->m_ui8EmailLen);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 6:
        	lua_pushliteral(pLua, "sClient");
        	if(u->m_sClient != NULL) {
				lua_pushlstring(pLua, u->m_sClient, (size_t)u->m_ui8ClientLen);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 7:
        	lua_pushliteral(pLua, "sClientVersion");
        	if(u->m_sTagVersion != NULL) {
				lua_pushlstring(pLua, u->m_sTagVersion, (size_t)u->m_ui8TagVersionLen);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 8:
        	lua_pushliteral(pLua, "sVersion");
        	if(u->m_sVersion!= NULL) {
				lua_pushstring(pLua, u->m_sVersion);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 9:
        	lua_pushliteral(pLua, "bConnected");
        	u->m_ui8State == User::STATE_ADDED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	lua_rawset(pLua, 1);
            break;
        case 10:
        	lua_pushliteral(pLua, "bActive");
        	if((u->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
                (u->m_ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
            } else {
                (u->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
            }
        	lua_rawset(pLua, 1);
            break;
        case 11:
        	lua_pushliteral(pLua, "bOperator");
        	(u->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	lua_rawset(pLua, 1);
            break;
        case 12:
        	lua_pushliteral(pLua, "bUserCommand");
        	(u->m_ui32SupportBits & User::SUPPORTBIT_USERCOMMAND) == User::SUPPORTBIT_USERCOMMAND ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	lua_rawset(pLua, 1);
            break;
        case 13:
        	lua_pushliteral(pLua, "bQuickList");
        	(u->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	lua_rawset(pLua, 1);
            break;
        case 14:
        	lua_pushliteral(pLua, "bSuspiciousTag");
        	(u->m_ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	lua_rawset(pLua, 1);
            break;
        case 15:
        	lua_pushliteral(pLua, "iProfile");
        	lua_pushinteger(pLua, u->m_i32Profile);
        	lua_rawset(pLua, 1);
            break;
        case 16:
        	lua_pushliteral(pLua, "iShareSize");
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, (double)u->m_ui64SharedSize);
#else
        	lua_pushinteger(pLua, u->m_ui64SharedSize);
#endif
        	lua_rawset(pLua, 1);
            break;
        case 17:
        	lua_pushliteral(pLua, "iHubs");
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, u->m_ui32Hubs);
#else
        	lua_pushinteger(pLua, u->m_ui32Hubs);
#endif
        	lua_rawset(pLua, 1);
            break;
        case 18:
        	lua_pushliteral(pLua, "iNormalHubs");
#if LUA_VERSION_NUM < 503
			(u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushnumber(pLua, u->m_ui32NormalHubs);
#else
        	(u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, u->m_ui32NormalHubs);
#endif
        	lua_rawset(pLua, 1);
            break;
        case 19:
        	lua_pushliteral(pLua, "iRegHubs");
#if LUA_VERSION_NUM < 503
			(u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushnumber(pLua, u->m_ui32RegHubs);
#else
        	(u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, u->m_ui32RegHubs);
#endif
        	lua_rawset(pLua, 1);
            break;
        case 20:
        	lua_pushliteral(pLua, "iOpHubs");
#if LUA_VERSION_NUM < 503
			(u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushnumber(pLua, u->m_ui32OpHubs);
#else
        	(u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, u->m_ui32OpHubs);
#endif
        	lua_rawset(pLua, 1);
            break;
        case 21:
        	lua_pushliteral(pLua, "iSlots");
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, u->m_ui32Slots);
#else
        	lua_pushinteger(pLua, u->m_ui32Slots);
#endif
        	lua_rawset(pLua, 1);
            break;
        case 22:
        	lua_pushliteral(pLua, "iLlimit");
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, u->m_ui32LLimit);
#else
        	lua_pushinteger(pLua, u->m_ui32LLimit);
#endif
        	lua_rawset(pLua, 1);
            break;
        case 23:
        	lua_pushliteral(pLua, "iDefloodWarns");
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, u->m_ui32DefloodWarnings);
#else
        	lua_pushinteger(pLua, u->m_ui32DefloodWarnings);
#endif
        	lua_rawset(pLua, 1);
            break;
        case 24:
        	lua_pushliteral(pLua, "iMagicByte");
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, u->m_ui8MagicByte);
#else
        	lua_pushinteger(pLua, u->m_ui8MagicByte);
#endif
        	lua_rawset(pLua, 1);
            break;   
        case 25:
        	lua_pushliteral(pLua, "iLoginTime");
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, (double)u->m_tLoginTime);
#else
        	lua_pushinteger(pLua, u->m_tLoginTime);
#endif
        	lua_rawset(pLua, 1);
            break;
        case 26:
        	lua_pushliteral(pLua, "sCountryCode");
        	if(IpP2Country::m_Ptr->m_ui32Count != 0) {
				lua_pushlstring(pLua, IpP2Country::m_Ptr->GetCountry(u->m_ui8Country, false), 2);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 27: {
            lua_pushliteral(pLua, "sMac");
            char sMac[18];
            if(GetMacAddress(u->m_sIP, sMac) == true) {
                lua_pushlstring(pLua, sMac, 17);
            } else {
                lua_pushnil(pLua);
            }
        	lua_rawset(pLua, 1);
            break;
        }
        case 28:
            lua_pushliteral(pLua, "bDescriptionChanged");
        	(u->m_ui32InfoBits & User::INFOBIT_DESCRIPTION_CHANGED) == User::INFOBIT_DESCRIPTION_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	lua_rawset(pLua, 1);
            break;
        case 29:
            lua_pushliteral(pLua, "bTagChanged");
        	(u->m_ui32InfoBits & User::INFOBIT_TAG_CHANGED) == User::INFOBIT_TAG_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	lua_rawset(pLua, 1);
            break;
        case 30:
            lua_pushliteral(pLua, "bConnectionChanged");
        	(u->m_ui32InfoBits & User::INFOBIT_CONNECTION_CHANGED) == User::INFOBIT_CONNECTION_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	lua_rawset(pLua, 1);
            break;
        case 31:
            lua_pushliteral(pLua, "bEmailChanged");
        	(u->m_ui32InfoBits & User::INFOBIT_EMAIL_CHANGED) == User::INFOBIT_EMAIL_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	lua_rawset(pLua, 1);
            break;
        case 32:
            lua_pushliteral(pLua, "bShareChanged");
        	(u->m_ui32InfoBits & User::INFOBIT_SHARE_CHANGED) == User::INFOBIT_SHARE_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	lua_rawset(pLua, 1);
            break;
        case 33:
            lua_pushliteral(pLua, "sScriptedDescriptionShort");
        	if(u->m_sChangedDescriptionShort != NULL) {
				lua_pushlstring(pLua, u->m_sChangedDescriptionShort, u->m_ui8ChangedDescriptionShortLen);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 34:
            lua_pushliteral(pLua, "sScriptedDescriptionLong");
        	if(u->m_sChangedDescriptionLong != NULL) {
				lua_pushlstring(pLua, u->m_sChangedDescriptionLong, u->m_ui8ChangedDescriptionLongLen);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 35:
            lua_pushliteral(pLua, "sScriptedTagShort");
        	if(u->m_sChangedTagShort != NULL) {
				lua_pushlstring(pLua, u->m_sChangedTagShort, u->m_ui8ChangedTagShortLen);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 36:
            lua_pushliteral(pLua, "sScriptedTagLong");
        	if(u->m_sChangedTagLong != NULL) {
				lua_pushlstring(pLua, u->m_sChangedTagLong, u->m_ui8ChangedTagLongLen);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 37:
            lua_pushliteral(pLua, "sScriptedConnectionShort");
        	if(u->m_sChangedConnectionShort != NULL) {
				lua_pushlstring(pLua, u->m_sChangedConnectionShort, u->m_ui8ChangedConnectionShortLen);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 38:
            lua_pushliteral(pLua, "sScriptedConnectionLong");
        	if(u->m_sChangedConnectionLong != NULL) {
				lua_pushlstring(pLua, u->m_sChangedConnectionLong, u->m_ui8ChangedConnectionLongLen);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 39:
            lua_pushliteral(pLua, "sScriptedEmailShort");
        	if(u->m_sChangedEmailShort != NULL) {
				lua_pushlstring(pLua, u->m_sChangedEmailShort, u->m_ui8ChangedEmailShortLen);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 40:
            lua_pushliteral(pLua, "sScriptedEmailLong");
        	if(u->m_sChangedEmailLong != NULL) {
				lua_pushlstring(pLua, u->m_sChangedEmailLong, u->m_ui8ChangedEmailLongLen);
			} else {
				lua_pushnil(pLua);
			}
        	lua_rawset(pLua, 1);
            break;
        case 41:
            lua_pushliteral(pLua, "iScriptediShareSizeShort");
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, (double)u->m_ui64ChangedSharedSizeShort);
#else
        	lua_pushinteger(pLua, u->m_ui64ChangedSharedSizeShort);
#endif
        	lua_rawset(pLua, 1);
            break;
        case 42:
            lua_pushliteral(pLua, "iScriptediShareSizeLong");
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, (double)u->m_ui64ChangedSharedSizeLong);
#else
        	lua_pushinteger(pLua, u->m_ui64ChangedSharedSizeLong);
#endif
        	lua_rawset(pLua, 1);
            break;
        case 43: {
            lua_pushliteral(pLua, "tIPs");
            lua_newtable(pLua);

            int t = lua_gettop(pLua);

#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, 1);
#else
            lua_pushinteger(pLua, 1);
#endif
            lua_pushlstring(pLua, u->m_sIP, u->m_ui8IpLen);
            lua_rawset(pLua, t);

            if(u->m_sIPv4[0] != '\0') {
#if LUA_VERSION_NUM < 503
				lua_pushnumber(pLua, 2);
#else
                lua_pushinteger(pLua, 2);
#endif
                lua_pushlstring(pLua, u->m_sIPv4, u->m_ui8IPv4Len);
                lua_rawset(pLua, t);
            }

        	lua_rawset(pLua, 1);
            break;
        }
        default:
            luaL_error(pLua, "bad argument #2 to 'GetUserData' (it's not valid id)");
    		lua_settop(pLua, 0);
    
            lua_pushnil(pLua);
            return 1;       
    }

    lua_settop(pLua, 0);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int GetUserValue(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'GetUserValue' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TNUMBER) {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
		lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    User *u = ScriptGetUser(pLua, 2, "GetUserValue");

    if(u == NULL) {
		lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

#if LUA_VERSION_NUM < 503
	uint8_t ui8DataId = (uint8_t)lua_tonumber(pLua, 2);
#else
    uint8_t ui8DataId = (uint8_t)lua_tointeger(pLua, 2);
#endif

    lua_settop(pLua, 0);

    switch(ui8DataId) {
        case 0:
        	if(u->m_sModes[0] != '\0') {
				lua_pushstring(pLua, u->m_sModes);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 1:
        	if(u->m_sMyInfoOriginal != NULL) {
				lua_pushlstring(pLua, u->m_sMyInfoOriginal, u->m_ui16MyInfoOriginalLen);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 2:
        	if(u->m_sDescription != NULL) {
				lua_pushlstring(pLua, u->m_sDescription, u->m_ui8DescriptionLen);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 3:
        	if(u->m_sTag != NULL) {
				lua_pushlstring(pLua, u->m_sTag, u->m_ui8TagLen);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 4:
        	if(u->m_sConnection != NULL) {
				lua_pushlstring(pLua, u->m_sConnection, u->m_ui8ConnectionLen);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 5:
        	if(u->m_sEmail != NULL) {
				lua_pushlstring(pLua, u->m_sEmail, u->m_ui8EmailLen);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 6:
        	if(u->m_sClient != NULL) {
				lua_pushlstring(pLua, u->m_sClient, u->m_ui8ClientLen);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 7:
        	if(u->m_sTagVersion != NULL) {
				lua_pushlstring(pLua, u->m_sTagVersion, u->m_ui8TagVersionLen);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 8:
        	if(u->m_sVersion != NULL) {
				lua_pushstring(pLua, u->m_sVersion);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 9:
        	u->m_ui8State == User::STATE_ADDED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	return 1;
        case 10:
            if((u->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
                (u->m_ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
            } else {
        	   (u->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
            }
        	return 1;
        case 11:
        	(u->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	return 1;
        case 12:
        	(u->m_ui32SupportBits & User::SUPPORTBIT_USERCOMMAND) == User::SUPPORTBIT_USERCOMMAND ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	return 1;
        case 13:
        	(u->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	return 1;
        case 14:
        	(u->m_ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	return 1;
        case 15:
        	lua_pushinteger(pLua, u->m_i32Profile);
        	return 1;
        case 16:
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, (double)u->m_ui64SharedSize);
#else
        	lua_pushinteger(pLua, u->m_ui64SharedSize);
#endif
        	return 1;
        case 17:
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, u->m_ui32Hubs);
#else
        	lua_pushinteger(pLua, u->m_ui32Hubs);
#endif
        	return 1;
        case 18:
#if LUA_VERSION_NUM < 503
			(u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushnumber(pLua, u->m_ui32NormalHubs);
#else
        	(u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, u->m_ui32NormalHubs);
#endif
        	return 1;
        case 19:
#if LUA_VERSION_NUM < 503
			(u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushnumber(pLua, u->m_ui32RegHubs);
#else
        	(u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, u->m_ui32RegHubs);
#endif
        	return 1;
        case 20:
#if LUA_VERSION_NUM < 503
			(u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushnumber(pLua, u->m_ui32OpHubs);
#else
        	(u->m_ui32BoolBits & User::BIT_OLDHUBSTAG) == User::BIT_OLDHUBSTAG ? lua_pushnil(pLua) : lua_pushinteger(pLua, u->m_ui32OpHubs);
#endif
        	return 1;
        case 21:
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, u->m_ui32Slots);
#else
        	lua_pushinteger(pLua, u->m_ui32Slots);
#endif
        	return 1;
        case 22:
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, u->m_ui32LLimit);
#else
        	lua_pushinteger(pLua, u->m_ui32LLimit);
#endif
        	return 1;
        case 23:
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, u->m_ui32DefloodWarnings);
#else
        	lua_pushinteger(pLua, u->m_ui32DefloodWarnings);
#endif
        	return 1;
        case 24:
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, u->m_ui8MagicByte);
#else
        	lua_pushinteger(pLua, u->m_ui8MagicByte);
#endif
        	return 1;  
        case 25:
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, (double)u->m_tLoginTime);
#else
        	lua_pushinteger(pLua, u->m_tLoginTime);
#endif
        	return 1;
        case 26:
        	if(IpP2Country::m_Ptr->m_ui32Count != 0) {
				lua_pushlstring(pLua, IpP2Country::m_Ptr->GetCountry(u->m_ui8Country, false), 2);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 27: {
            char sMac[18];
            if(GetMacAddress(u->m_sIP, sMac) == true) {
                lua_pushlstring(pLua, sMac, 17);
                return 1;
            } else {
                lua_pushnil(pLua);
                return 1;
            }
        }
        case 28:
        	(u->m_ui32InfoBits & User::INFOBIT_DESCRIPTION_CHANGED) == User::INFOBIT_DESCRIPTION_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	return 1;
        case 29:
        	(u->m_ui32InfoBits & User::INFOBIT_TAG_CHANGED) == User::INFOBIT_TAG_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	return 1;
        case 30:
        	(u->m_ui32InfoBits & User::INFOBIT_CONNECTION_CHANGED) == User::INFOBIT_CONNECTION_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	return 1;
        case 31:
        	(u->m_ui32InfoBits & User::INFOBIT_EMAIL_CHANGED) == User::INFOBIT_EMAIL_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	return 1;
        case 32:
        	(u->m_ui32InfoBits & User::INFOBIT_SHARE_CHANGED) == User::INFOBIT_SHARE_CHANGED ? lua_pushboolean(pLua, 1) : lua_pushboolean(pLua, 0);
        	return 1;
        case 33:
        	if(u->m_sChangedDescriptionShort != NULL) {
				lua_pushlstring(pLua, u->m_sChangedDescriptionShort, u->m_ui8ChangedDescriptionShortLen);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 34:
        	if(u->m_sChangedDescriptionLong != NULL) {
				lua_pushlstring(pLua, u->m_sChangedDescriptionLong, u->m_ui8ChangedDescriptionLongLen);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 35:
        	if(u->m_sChangedTagShort != NULL) {
				lua_pushlstring(pLua, u->m_sChangedTagShort, u->m_ui8ChangedTagShortLen);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 36:
        	if(u->m_sChangedTagLong != NULL) {
				lua_pushlstring(pLua, u->m_sChangedTagLong, u->m_ui8ChangedTagLongLen);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 37:
        	if(u->m_sChangedConnectionShort != NULL) {
				lua_pushlstring(pLua, u->m_sChangedConnectionShort, u->m_ui8ChangedConnectionShortLen);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 38:
        	if(u->m_sChangedConnectionLong != NULL) {
				lua_pushlstring(pLua, u->m_sChangedConnectionLong, u->m_ui8ChangedConnectionLongLen);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 39:
        	if(u->m_sChangedEmailShort != NULL) {
				lua_pushlstring(pLua, u->m_sChangedEmailShort, u->m_ui8ChangedEmailShortLen);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 40:
        	if(u->m_sChangedEmailLong != NULL) {
				lua_pushlstring(pLua, u->m_sChangedEmailLong, u->m_ui8ChangedEmailLongLen);
			} else {
				lua_pushnil(pLua);
			}
        	return 1;
        case 41:
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, (double)u->m_ui64ChangedSharedSizeShort);
#else
        	lua_pushinteger(pLua, u->m_ui64ChangedSharedSizeShort);
#endif
        	return 1;
        case 42:
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, (double)u->m_ui64ChangedSharedSizeLong);
#else
        	lua_pushinteger(pLua, u->m_ui64ChangedSharedSizeLong);
#endif
        	return 1;
        case 43: {
            lua_newtable(pLua);

            int t = lua_gettop(pLua);

#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, 1);
#else
            lua_pushinteger(pLua, 1);
#endif
            lua_pushlstring(pLua, u->m_sIP, u->m_ui8IpLen);
            lua_rawset(pLua, t);

            if(u->m_sIPv4[0] != '\0') {
#if LUA_VERSION_NUM < 503
				lua_pushnumber(pLua, 2);
#else
                lua_pushinteger(pLua, 2);
#endif
                lua_pushlstring(pLua, u->m_sIPv4, u->m_ui8IPv4Len);
                lua_rawset(pLua, t);
            }

            return 1;
        }
        default:
            luaL_error(pLua, "bad argument #2 to 'GetUserValue' (it's not valid id)");
            
            lua_pushnil(pLua);
            return 1;
    }
}
//------------------------------------------------------------------------------

static int Disconnect(lua_State * pLua) {
    if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'Disconnect' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    User * u;

    if(lua_type(pLua, 1) == LUA_TTABLE) {
        u = ScriptGetUser(pLua, 1, "Disconnect");

        if(u == NULL) {
    		lua_settop(pLua, 0);
            lua_pushnil(pLua);
            return 1;
        }
    } else if(lua_type(pLua, 1) == LUA_TSTRING) {
        size_t szLen;
        char * nick = (char *)lua_tolstring(pLua, 1, &szLen);
    
        if(szLen == 0) {
            return 0;
        }
    
        u = HashManager::m_Ptr->FindUser(nick, szLen);

        if(u == NULL) {
    		lua_settop(pLua, 0);
    
            lua_pushnil(pLua);
            return 1;
        }
    } else {
        luaL_error(pLua, "bad argument #1 to 'Disconnect' (user table or string expected, got %s)", lua_typename(pLua, lua_type(pLua, 1)));
        lua_settop(pLua, 0);
        
        lua_pushnil(pLua);
        return 1;
    }


//    UdpDebug->BroadcastFormat("[SYS] User %s (%s) disconnected by script.", u->sNick, u->sIP);
    u->Close();

    lua_settop(pLua, 0);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int Kick(lua_State * pLua) {
    if(lua_gettop(pLua) != 3) {
        luaL_error(pLua, "bad argument count to 'Kick' (3 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
		lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    User * pUser = ScriptGetUser(pLua, 3, "Kick");

    if(pUser == NULL) {
		lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szKickerLen, szReasonLen;
    char *sKicker = (char *)lua_tolstring(pLua, 2, &szKickerLen);
    char *sReason = (char *)lua_tolstring(pLua, 3, &szReasonLen);

    if(szKickerLen == 0 || szKickerLen > 64 || szReasonLen == 0 || szReasonLen > 128000) {
    	lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    BanManager::m_Ptr->TempBan(pUser, sReason, sKicker, 0, 0, false);

    pUser->SendFormat("Core.Kick", false, "<%s> %s: %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_BEING_KICKED_BCS], sReason);

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
    	GlobalDataQueue::m_Ptr->StatusMessageFormat("Core.Kick", "<%s> *** %s %s IP %s %s %s %s: %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_LWR], pUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_WAS_KICKED_BY], sKicker, 
			LanguageManager::m_Ptr->m_sTexts[LAN_BECAUSE_LWR], sReason);
    }

    // disconnect the user
	UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) kicked by script.", pUser->m_sNick, pUser->m_sIP);

    pUser->Close();
    
    lua_settop(pLua, 0);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int Redirect(lua_State * pLua) {
    if(lua_gettop(pLua) != 3) {
        luaL_error(pLua, "bad argument count to 'Redirect' (3 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
		lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    User * pUser = ScriptGetUser(pLua, 3, "Redirect");

    if(pUser == NULL) {
		lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szAddressLen, szReasonLen;
    char *sAddress = (char *)lua_tolstring(pLua, 2, &szAddressLen);
    char *sReason = (char *)lua_tolstring(pLua, 3, &szReasonLen);

    if(szAddressLen == 0 || szAddressLen > 1024 || szReasonLen == 0 || szReasonLen > 128000) {
		lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    pUser->SendFormat("Core.Redirect", false, "<%s> %s %s. %s: %s|$ForceMove %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_REDIR_TO], sAddress, LanguageManager::m_Ptr->m_sTexts[LAN_MESSAGE], sReason, sAddress);


//	UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) redirected by script.", pUser->sNick, pUser->sIP);

    pUser->Close();
    
    lua_settop(pLua, 0);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int DefloodWarn(lua_State * pLua) {
    if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'DefloodWarn' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TTABLE) {
        luaL_checktype(pLua, 1, LUA_TTABLE);
		lua_settop(pLua, 0);

        lua_pushnil(pLua);
        return 1;
    }

    User *u = ScriptGetUser(pLua, 1, "DefloodWarn");

    if(u == NULL) {
		lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    u->m_ui32DefloodWarnings++;

    if(u->m_ui32DefloodWarnings >= (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFLOOD_WARNING_COUNT]) {
        switch(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFLOOD_WARNING_ACTION]) {
            case 0:
                break;
            case 1:
				BanManager::m_Ptr->TempBan(u, LanguageManager::m_Ptr->m_sTexts[LAN_FLOODING], NULL, 0, 0, false);
                break;
            case 2:
                BanManager::m_Ptr->TempBan(u, LanguageManager::m_Ptr->m_sTexts[LAN_FLOODING], NULL, 
                    SettingManager::m_Ptr->m_i16Shorts[SETSHORT_DEFLOOD_TEMP_BAN_TIME], 0, false);
                break;
            case 3: {
                BanManager::m_Ptr->Ban(u, LanguageManager::m_Ptr->m_sTexts[LAN_FLOODING], NULL, false);
                break;
            }
            default:
                break;
        }

        if(SettingManager::m_Ptr->m_bBools[SETBOOL_DEFLOOD_REPORT] == true) {
            GlobalDataQueue::m_Ptr->StatusMessageFormat("Core.DefloodWarn", "<%s> *** %s %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_FLOODER], u->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], u->m_sIP, 
				LanguageManager::m_Ptr->m_sTexts[LAN_DISCONN_BY_SCRIPT]);
        }

		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Flood from %s (%s) - user closed by script.", u->m_sNick, u->m_sIP);

        u->Close();
    }

    lua_settop(pLua, 0);

    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int SendToAll(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'SendToAll' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
		lua_settop(pLua, 0);
        return 0;
    }

    size_t szLen;
    char * sData = (char *)lua_tolstring(pLua, 1, &szLen);

    if(sData[0] != '\0' && szLen < 128001) {
		if(sData[szLen-1] != '|') {
            memcpy(ServerManager::m_pGlobalBuffer, sData, szLen);
            ServerManager::m_pGlobalBuffer[szLen] = '|';
            ServerManager::m_pGlobalBuffer[szLen+1] = '\0';
			GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, szLen+1, NULL, 0, GlobalDataQueue::CMD_LUA);
		} else {
			GlobalDataQueue::m_Ptr->AddQueueItem(sData, szLen, NULL, 0, GlobalDataQueue::CMD_LUA);
        }
    }

	lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendToNick(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'SendToNick' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
		lua_settop(pLua, 0);
        return 0;
    }

    size_t szNickLen, szDataLen;
    char *sNick = (char *)lua_tolstring(pLua, 1, &szNickLen);
    char *sData = (char *)lua_tolstring(pLua, 2, &szDataLen);

    if(szNickLen == 0 || szDataLen == 0 || szDataLen > 128000) {
        lua_settop(pLua, 0);
        return 0;
    }

    User *u = HashManager::m_Ptr->FindUser(sNick, szNickLen);
    if(u != NULL) {
        if(sData[szDataLen-1] != '|') {
            memcpy(ServerManager::m_pGlobalBuffer, sData, szDataLen);
            ServerManager::m_pGlobalBuffer[szDataLen] = '|';
            ServerManager::m_pGlobalBuffer[szDataLen+1] = '\0';
            u->SendCharDelayed(ServerManager::m_pGlobalBuffer, szDataLen+1);
        } else {
            u->SendCharDelayed(sData, szDataLen);
        }
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendToOpChat(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'SendToOpChat' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
		lua_settop(pLua, 0);
        return 0;
    }

    size_t szDataLen;
    char * sData = (char *)lua_tolstring(pLua, 1, &szDataLen);

    if(szDataLen == 0 || szDataLen > 128000) {
        lua_settop(pLua, 0);
        return 0;
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true) {
        int iLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s $<%s> %s|", SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK], SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK], sData);
        if(iLen > 0) {
			GlobalDataQueue::m_Ptr->SingleItemStore(ServerManager::m_pGlobalBuffer, iLen, NULL, 0, GlobalDataQueue::SI_OPCHAT);
        }
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendToOps(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'SendToOps' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
		lua_settop(pLua, 0);
        return 0;
    }

    size_t szLen;
    char * sData = (char *)lua_tolstring(pLua, 1, &szLen);
    
    if(szLen == 0 || szLen > 128000) {
        lua_settop(pLua, 0);
        return 0;
    }

    if(sData[szLen-1] != '|') {
        memcpy(ServerManager::m_pGlobalBuffer, sData, szLen);
        ServerManager::m_pGlobalBuffer[szLen] = '|';
        ServerManager::m_pGlobalBuffer[szLen+1] = '\0';
        GlobalDataQueue::m_Ptr->AddQueueItem(ServerManager::m_pGlobalBuffer, szLen+1, NULL, 0, GlobalDataQueue::CMD_OPS);
    } else {
        GlobalDataQueue::m_Ptr->AddQueueItem(sData, szLen, NULL, 0, GlobalDataQueue::CMD_OPS);
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendToProfile(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'SendToProfile' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        luaL_checktype(pLua, 2, LUA_TSTRING);
		lua_settop(pLua, 0);
        return 0;
    }

    int32_t i32Profile = (int32_t)lua_tointeger(pLua, 1);

    size_t szDataLen;
    char * sData = (char *)lua_tolstring(pLua, 2, &szDataLen);

    if(szDataLen == 0 || szDataLen > 128000) {
        lua_settop(pLua, 0);
        return 0;
    }

    if(sData[szDataLen-1] != '|') {
        memcpy(ServerManager::m_pGlobalBuffer, sData, szDataLen);
		ServerManager::m_pGlobalBuffer[szDataLen] = '|';
        ServerManager::m_pGlobalBuffer[szDataLen+1] = '\0';
		GlobalDataQueue::m_Ptr->SingleItemStore(ServerManager::m_pGlobalBuffer, szDataLen+1, NULL, i32Profile, GlobalDataQueue::SI_TOPROFILE);
    } else {
		GlobalDataQueue::m_Ptr->SingleItemStore(sData, szDataLen, NULL, i32Profile, GlobalDataQueue::SI_TOPROFILE);
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendToUser(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'SendToUser' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TSTRING);
		lua_settop(pLua, 0);
        return 0;
    }

    User *u = ScriptGetUser(pLua, 2, "SendToUser");

    if(u == NULL) {
		lua_settop(pLua, 0);
        return 0;
    }

    size_t szLen;
    char * sData = (char *)lua_tolstring(pLua, 2, &szLen);

    if(szLen == 0 || szLen > 128000) {
        lua_settop(pLua, 0);
        return 0;
    }

    if(sData[szLen-1] != '|') {
        memcpy(ServerManager::m_pGlobalBuffer, sData, szLen);
        ServerManager::m_pGlobalBuffer[szLen] = '|';
        ServerManager::m_pGlobalBuffer[szLen+1] = '\0';
    	u->SendCharDelayed(ServerManager::m_pGlobalBuffer, szLen+1);
    } else {
        u->SendCharDelayed(sData, szLen);
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendPmToAll(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'SendPmToAll' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
		lua_settop(pLua, 0);
        return 0;
    }

    size_t szFromLen, szDataLen;
    char * sFrom = (char *)lua_tolstring(pLua, 1, &szFromLen);
    char * sData = (char *)lua_tolstring(pLua, 2, &szDataLen);

    if(szFromLen == 0 || szFromLen > 64 || szDataLen == 0 || szDataLen > 128000) {
        lua_settop(pLua, 0); 
        return 0;
    }

    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s $<%s> %s|", sFrom, sFrom, sData);
    if(iMsgLen > 0) {
		GlobalDataQueue::m_Ptr->SingleItemStore(ServerManager::m_pGlobalBuffer, iMsgLen, NULL, 0, GlobalDataQueue::SI_PM2ALL);
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendPmToNick(lua_State * pLua) {
	if(lua_gettop(pLua) != 3) {
        luaL_error(pLua, "bad argument count to 'SendPmToNick' (3 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
		lua_settop(pLua, 0);
        return 0;
    }

    size_t szToLen, szFromLen, szDataLen;
    char * sTo = (char *)lua_tolstring(pLua, 1, &szToLen);
    char * sFrom = (char *)lua_tolstring(pLua, 2, &szFromLen);
    char * sData = (char *)lua_tolstring(pLua, 3, &szDataLen);

    if(szToLen == 0 || szFromLen == 0 || szFromLen > 64 || szDataLen == 0 || szDataLen > 128000) {
        lua_settop(pLua, 0);
        return 0;
    }

    User * pUser = HashManager::m_Ptr->FindUser(sTo, szToLen);
    if(pUser != NULL) {
        pUser->SendFormat("Core.SendPmToNick", true, "$To: %s From: %s $<%s> %s|", sTo, sFrom, sFrom, sData);
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendPmToOps(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'SendPmToOps' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
		lua_settop(pLua, 0);
        return 0;
    }

    size_t szFromLen, szDataLen;
    char * sFrom = (char *)lua_tolstring(pLua, 1, &szFromLen);
    char * sData = (char *)lua_tolstring(pLua, 2, &szDataLen);

    if(szFromLen == 0 || szFromLen > 64 || szDataLen == 0 || szDataLen > 128000) {
        lua_settop(pLua, 0);
        return 0;
    }

    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s $<%s> %s|", sFrom, sFrom, sData);
    if(iMsgLen > 0) {
		GlobalDataQueue::m_Ptr->SingleItemStore(ServerManager::m_pGlobalBuffer, iMsgLen, NULL, 0, GlobalDataQueue::SI_PM2OPS);
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendPmToProfile(lua_State * pLua) {
	if(lua_gettop(pLua) != 3) {
        luaL_error(pLua, "bad argument count to 'SendPmToProfile' (3 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
		lua_settop(pLua, 0);
        return 0;
    }

    int32_t iProfile = (int32_t)lua_tointeger(pLua, 1);

    size_t szFromLen, szDataLen;
    char * sFrom = (char *)lua_tolstring(pLua, 2, &szFromLen);
    char * sData = (char *)lua_tolstring(pLua, 3, &szDataLen);

    if(szFromLen == 0 || szFromLen > 64 || szDataLen == 0 || szDataLen > 128000) {
        lua_settop(pLua, 0);
        return 0;
    }

    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s $<%s> %s|", sFrom, sFrom, sData);
    if(iMsgLen > 0) {
		GlobalDataQueue::m_Ptr->SingleItemStore(ServerManager::m_pGlobalBuffer, iMsgLen, NULL, iProfile, GlobalDataQueue::SI_PM2PROFILE);
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SendPmToUser(lua_State * pLua) {
	if(lua_gettop(pLua) != 3) {
        luaL_error(pLua, "bad argument count to 'SendPmToUser' (3 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
		lua_settop(pLua, 0);
        return 0;
    }

    User * pUser = ScriptGetUser(pLua, 3, "SendPmToUser");

    if(pUser == NULL) {
		lua_settop(pLua, 0);
        return 0;
    }

    size_t szFromLen, szDataLen;
    char * sFrom = (char *)lua_tolstring(pLua, 2, &szFromLen);
    char * sData = (char *)lua_tolstring(pLua, 3, &szDataLen);
    
    if(szFromLen == 0 || szFromLen > 64 || szDataLen == 0 || szDataLen > 128000) {
		lua_settop(pLua, 0);
        return 0;
    }

    pUser->SendFormat("Core.SendPmToUser", true, "$To: %s From: %s $<%s> %s|", pUser->m_sNick, sFrom, sFrom, sData);

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SetUserInfo(lua_State * pLua) {
	if(lua_gettop(pLua) != 4) {
        luaL_error(pLua, "bad argument count to 'SetUserInfo' (4 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TNUMBER || lua_type(pLua, 4) != LUA_TBOOLEAN) {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        luaL_checktype(pLua, 4, LUA_TBOOLEAN);
		lua_settop(pLua, 0);
        return 0;
    }

    User * pUser = ScriptGetUser(pLua, 4, "SetUserInfo");

    if(pUser == NULL) {
		lua_settop(pLua, 0);
        return 0;
    }

#if LUA_VERSION_NUM < 503
	uint32_t ui32DataToChange = (uint32_t)lua_tonumber(pLua, 2);
#else
    uint32_t ui32DataToChange = (uint32_t)lua_tointeger(pLua, 2);
#endif

    if(ui32DataToChange > 9) {
		lua_settop(pLua, 0);
        return 0;
    }

    bool bPermanent = lua_toboolean(pLua, 4) == 0 ? false : true;

    static const char * sMyInfoPartsNames[] = { "sChangedDescriptionShort", "sChangedDescriptionLong", "sChangedTagShort", "sChangedTagLong",
        "sChangedConnectionShort", "sChangedConnectionLong", "sChangedEmailShort", "sChangedEmailLong" };

    if(lua_type(pLua, 3) == LUA_TSTRING) {
        if(ui32DataToChange > 7) {
            lua_settop(pLua, 0);
            return 0;
        }

        size_t szDataLen;
        char * sData = (char *)lua_tolstring(pLua, 3, &szDataLen);

        if(szDataLen > 64 || strpbrk(sData, "$|") != NULL) {
            lua_settop(pLua, 0);
            return 0;
        }

        switch(ui32DataToChange) {
            case 0:
                pUser->m_sChangedDescriptionShort = User::SetUserInfo(pUser->m_sChangedDescriptionShort, pUser->m_ui8ChangedDescriptionShortLen, sData, szDataLen, sMyInfoPartsNames[ui32DataToChange]);
                if(bPermanent == true) {
                    pUser->m_ui32InfoBits |= User::INFOBIT_DESCRIPTION_SHORT_PERM;
                } else {
                    pUser->m_ui32InfoBits &= ~User::INFOBIT_DESCRIPTION_SHORT_PERM;
                }
                break;
            case 1:
                pUser->m_sChangedDescriptionLong = User::SetUserInfo(pUser->m_sChangedDescriptionLong, pUser->m_ui8ChangedDescriptionLongLen, sData, szDataLen, sMyInfoPartsNames[ui32DataToChange]);
                if(bPermanent == true) {
                    pUser->m_ui32InfoBits |= User::INFOBIT_DESCRIPTION_LONG_PERM;
                } else {
                    pUser->m_ui32InfoBits &= ~User::INFOBIT_DESCRIPTION_LONG_PERM;
                }
                break;
            case 2:
                pUser->m_sChangedTagShort = User::SetUserInfo(pUser->m_sChangedTagShort, pUser->m_ui8ChangedTagShortLen, sData, szDataLen, sMyInfoPartsNames[ui32DataToChange]);
                if(bPermanent == true) {
                    pUser->m_ui32InfoBits |= User::INFOBIT_TAG_SHORT_PERM;
                } else {
                    pUser->m_ui32InfoBits &= ~User::INFOBIT_TAG_SHORT_PERM;
                }
                break;
            case 3:
                pUser->m_sChangedTagLong = User::SetUserInfo(pUser->m_sChangedTagLong, pUser->m_ui8ChangedTagLongLen, sData, szDataLen, sMyInfoPartsNames[ui32DataToChange]);
                if(bPermanent == true) {
                    pUser->m_ui32InfoBits |= User::INFOBIT_TAG_LONG_PERM;
                } else {
                    pUser->m_ui32InfoBits &= ~User::INFOBIT_TAG_LONG_PERM;
                }
                break;
            case 4:
                pUser->m_sChangedConnectionShort = User::SetUserInfo(pUser->m_sChangedConnectionShort, pUser->m_ui8ChangedConnectionShortLen, sData, szDataLen, sMyInfoPartsNames[ui32DataToChange]);
                if(bPermanent == true) {
                    pUser->m_ui32InfoBits |= User::INFOBIT_CONNECTION_SHORT_PERM;
                } else {
                    pUser->m_ui32InfoBits &= ~User::INFOBIT_CONNECTION_SHORT_PERM;
                }
                break;
            case 5:
                pUser->m_sChangedConnectionLong = User::SetUserInfo(pUser->m_sChangedConnectionLong, pUser->m_ui8ChangedConnectionLongLen, sData, szDataLen, sMyInfoPartsNames[ui32DataToChange]);
                if(bPermanent == true) {
                    pUser->m_ui32InfoBits |= User::INFOBIT_CONNECTION_LONG_PERM;
                } else {
                    pUser->m_ui32InfoBits &= ~User::INFOBIT_CONNECTION_LONG_PERM;
                }
                break;
            case 6:
                pUser->m_sChangedEmailShort = User::SetUserInfo(pUser->m_sChangedEmailShort, pUser->m_ui8ChangedEmailShortLen, sData, szDataLen, sMyInfoPartsNames[ui32DataToChange]);
                if(bPermanent == true) {
                    pUser->m_ui32InfoBits |= User::INFOBIT_EMAIL_SHORT_PERM;
                } else {
                    pUser->m_ui32InfoBits &= ~User::INFOBIT_EMAIL_SHORT_PERM;
                }
                break;
            case 7:
                pUser->m_sChangedEmailLong = User::SetUserInfo(pUser->m_sChangedEmailLong, pUser->m_ui8ChangedEmailLongLen, sData, szDataLen, sMyInfoPartsNames[ui32DataToChange]);
                if(bPermanent == true) {
                    pUser->m_ui32InfoBits |= User::INFOBIT_EMAIL_LONG_PERM;
                } else {
                    pUser->m_ui32InfoBits &= ~User::INFOBIT_EMAIL_LONG_PERM;
                }
                break;
            default:
                break;
        }
    } else if(lua_type(pLua, 3) == LUA_TNIL) {
        switch(ui32DataToChange) {
            case 0:
                if(pUser->m_sChangedDescriptionShort != NULL) {
                    User::FreeInfo(pUser->m_sChangedDescriptionShort, sMyInfoPartsNames[ui32DataToChange]);
                    pUser->m_sChangedDescriptionShort = NULL;
                    pUser->m_ui8ChangedDescriptionShortLen = 0;
                }
                pUser->m_ui32InfoBits &= ~User::INFOBIT_DESCRIPTION_SHORT_PERM;
                break;
            case 1:
                if(pUser->m_sChangedDescriptionLong != NULL) {
                    User::FreeInfo(pUser->m_sChangedDescriptionLong, sMyInfoPartsNames[ui32DataToChange]);
                    pUser->m_sChangedDescriptionLong = NULL;
                    pUser->m_ui8ChangedDescriptionLongLen = 0;
                }
                pUser->m_ui32InfoBits &= ~User::INFOBIT_DESCRIPTION_LONG_PERM;
                break;
            case 2:
                if(pUser->m_sChangedTagShort != NULL) {
                    User::FreeInfo(pUser->m_sChangedTagShort, sMyInfoPartsNames[ui32DataToChange]);
                    pUser->m_sChangedTagShort = NULL;
                    pUser->m_ui8ChangedTagShortLen = 0;
                }
                pUser->m_ui32InfoBits &= ~User::INFOBIT_TAG_SHORT_PERM;
                break;
            case 3:
                if(pUser->m_sChangedTagLong != NULL) {
                    User::FreeInfo(pUser->m_sChangedTagLong, sMyInfoPartsNames[ui32DataToChange]);
                    pUser->m_sChangedTagLong = NULL;
                    pUser->m_ui8ChangedTagLongLen = 0;
                }
                pUser->m_ui32InfoBits &= ~User::INFOBIT_TAG_LONG_PERM;
                break;
            case 4:
                if(pUser->m_sChangedConnectionShort != NULL) {
                    User::FreeInfo(pUser->m_sChangedConnectionShort, sMyInfoPartsNames[ui32DataToChange]);
                    pUser->m_sChangedConnectionShort = NULL;
                    pUser->m_ui8ChangedConnectionShortLen = 0;
                }
                pUser->m_ui32InfoBits &= ~User::INFOBIT_CONNECTION_SHORT_PERM;
                break;
            case 5:
                if(pUser->m_sChangedConnectionLong != NULL) {
                    User::FreeInfo(pUser->m_sChangedConnectionLong, sMyInfoPartsNames[ui32DataToChange]);
                    pUser->m_sChangedConnectionLong = NULL;
                    pUser->m_ui8ChangedConnectionLongLen = 0;
                }
                pUser->m_ui32InfoBits &= ~User::INFOBIT_CONNECTION_LONG_PERM;
                break;
            case 6:
                if(pUser->m_sChangedEmailShort != NULL) {
                    User::FreeInfo(pUser->m_sChangedEmailShort, sMyInfoPartsNames[ui32DataToChange]);
                    pUser->m_sChangedEmailShort = NULL;
                    pUser->m_ui8ChangedEmailShortLen = 0;
                }
                pUser->m_ui32InfoBits &= ~User::INFOBIT_EMAIL_SHORT_PERM;
                break;
            case 7:
                if(pUser->m_sChangedEmailLong != NULL) {
                    User::FreeInfo(pUser->m_sChangedEmailLong, sMyInfoPartsNames[ui32DataToChange]);
                    pUser->m_sChangedEmailLong = NULL;
                    pUser->m_ui8ChangedEmailLongLen = 0;
                }
                pUser->m_ui32InfoBits &= ~User::INFOBIT_EMAIL_LONG_PERM;
                break;
            case 8:
                pUser->m_ui64ChangedSharedSizeShort = pUser->m_ui64SharedSize;
                pUser->m_ui32InfoBits &= ~User::INFOBIT_SHARE_SHORT_PERM;
                break;
            case 9:
                pUser->m_ui64ChangedSharedSizeLong = pUser->m_ui64SharedSize;
                pUser->m_ui32InfoBits &= ~User::INFOBIT_SHARE_LONG_PERM;
                break;
            default:
                break;
        }
    } else if(lua_type(pLua, 3) == LUA_TNUMBER) {
        if(ui32DataToChange < 8) {
            lua_settop(pLua, 0);
            return 0;
        }

        if(ui32DataToChange == 8) {
#if LUA_VERSION_NUM < 503
			pUser->m_ui64ChangedSharedSizeShort = (uint64_t)lua_tonumber(pLua, 3);
#else
            pUser->m_ui64ChangedSharedSizeShort = lua_tointeger(pLua, 3);
#endif
            if(bPermanent == true) {
                pUser->m_ui32InfoBits |= User::INFOBIT_SHARE_SHORT_PERM;
            } else {
                pUser->m_ui32InfoBits &= ~User::INFOBIT_SHARE_SHORT_PERM;
            }
        } else if(ui32DataToChange == 9) {
#if LUA_VERSION_NUM < 503
			pUser->m_ui64ChangedSharedSizeLong = (uint64_t)lua_tonumber(pLua, 3);
#else
            pUser->m_ui64ChangedSharedSizeLong = lua_tointeger(pLua, 3);
#endif
            if(bPermanent == true) {
                pUser->m_ui32InfoBits |= User::INFOBIT_SHARE_LONG_PERM;
            } else {
                pUser->m_ui32InfoBits &= ~User::INFOBIT_SHARE_LONG_PERM;
            }
        }
    } else {
        luaL_error(pLua, "bad argument #3 to 'SetUserInfo' (string or number or nil expected, got %s)", lua_typename(pLua, lua_type(pLua, 3)));
        lua_settop(pLua, 0);
        return 0;
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static const luaL_Reg CoreRegs[] = {
	{ "Restart", Restart },
	{ "Shutdown", Shutdown }, 
	{ "ResumeAccepts", ResumeAccepts }, 
	{ "SuspendAccepts", SuspendAccepts }, 
	{ "RegBot", RegBot },  
	{ "UnregBot", UnregBot }, 
	{ "GetBots", GetBots }, 
	{ "GetActualUsersPeak", GetActualUsersPeak }, 
	{ "GetMaxUsersPeak", GetMaxUsersPeak }, 
	{ "GetCurrentSharedSize", GetCurrentSharedSize }, 
	{ "GetHubIP", GetHubIP }, 
	{ "GetHubIPs", GetHubIPs },
	{ "GetHubSecAlias", GetHubSecAlias }, 
	{ "GetPtokaXPath", GetPtokaXPath }, 
	{ "GetUsersCount", GetUsersCount }, 
	{ "GetUpTime", GetUpTime }, 
	{ "GetOnlineNonOps", GetOnlineNonOps }, 
	{ "GetOnlineOps", GetOnlineOps }, 
	{ "GetOnlineRegs", GetOnlineRegs }, 
	{ "GetOnlineUsers", GetOnlineUsers }, 
	{ "GetUser", GetUser }, 
	{ "GetUsers", GetUsers }, 
	{ "GetUserAllData", GetUserAllData }, 
	{ "GetUserData", GetUserData }, 
	{ "GetUserValue", GetUserValue }, 
	{ "Disconnect", Disconnect }, 
	{ "Kick", Kick }, 
	{ "Redirect", Redirect }, 
	{ "DefloodWarn", DefloodWarn }, 
    { "SendToAll", SendToAll }, 
	{ "SendToNick", SendToNick }, 
	{ "SendToOpChat", SendToOpChat }, 
	{ "SendToOps", SendToOps }, 
	{ "SendToProfile", SendToProfile }, 
	{ "SendToUser", SendToUser }, 
	{ "SendPmToAll", SendPmToAll }, 
	{ "SendPmToNick", SendPmToNick }, 
	{ "SendPmToOps", SendPmToOps }, 
	{ "SendPmToProfile", SendPmToProfile }, 
	{ "SendPmToUser", SendPmToUser }, 
	{ "SetUserInfo", SetUserInfo },
	{ NULL, NULL }
};
//---------------------------------------------------------------------------

#if LUA_VERSION_NUM > 501
int RegCore(lua_State * pLua) {
    luaL_newlib(pLua, CoreRegs);
#else
void RegCore(lua_State * pLua) {
    luaL_register(pLua, "Core", CoreRegs);
#endif

    lua_pushliteral(pLua, "Version");
	lua_pushliteral(pLua, PtokaXVersionString);
	lua_settable(pLua, -3);
	lua_pushliteral(pLua, "BuildNumber");
#ifdef _WIN32
#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, (double)_strtoui64(BUILD_NUMBER, NULL, 10));
#else
	lua_pushinteger(pLua, _strtoui64(BUILD_NUMBER, NULL, 10));
#endif
#else
#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, (double)strtoull(BUILD_NUMBER, NULL, 10));
#else
    lua_pushinteger(pLua, strtoull(BUILD_NUMBER, NULL, 10));
#endif
#endif
	lua_settable(pLua, -3);

#if LUA_VERSION_NUM > 501
    return 1;
#endif
}
//---------------------------------------------------------------------------
