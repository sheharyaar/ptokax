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
#include "LuaBanManLib.h"
//---------------------------------------------------------------------------
#include "hashBanManager.h"
#include "hashUsrManager.h"
#include "LuaScriptManager.h"
#include "ServerManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "LuaScript.h"
//---------------------------------------------------------------------------

static void PushBan(lua_State * pLua, BanItem * pBan) {
	lua_checkstack(pLua, 3); // we need 3 (1 table, 2 id, 3 value) empty slots in stack, check it to be sure

	lua_newtable(pLua);
	int i = lua_gettop(pLua);

    lua_pushliteral(pLua, "sIP");
    if(pBan->m_sIp[0] == '\0') {
		lua_pushnil(pLua);
	} else {
		lua_pushstring(pLua, pBan->m_sIp);
	}
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sNick");
    if(pBan->m_sNick == NULL) {
		lua_pushnil(pLua);
	} else {
		lua_pushstring(pLua, pBan->m_sNick);
	}
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sReason");
    if(pBan->m_sReason == NULL) {
		lua_pushnil(pLua);
	} else {
		lua_pushstring(pLua, pBan->m_sReason);
	}
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sBy");
    if(pBan->m_sBy == NULL) {
		lua_pushnil(pLua);
	} else {
		lua_pushstring(pLua, pBan->m_sBy);
	}
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "iExpireTime");
#if LUA_VERSION_NUM < 503
	((pBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == false ? lua_pushnil(pLua) : lua_pushnumber(pLua, (double)pBan->m_tTempBanExpire);
#else
    ((pBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == false ? lua_pushnil(pLua) : lua_pushinteger(pLua, pBan->m_tTempBanExpire);
#endif
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "bIpBan");
    ((pBan->m_ui8Bits & BanManager::IP) == BanManager::IP) == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "bNickBan");
    ((pBan->m_ui8Bits & BanManager::NICK) == BanManager::NICK) == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "bFullIpBan");
    ((pBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, i);
}
//------------------------------------------------------------------------------

static void PushRangeBan(lua_State * pLua, RangeBanItem * pRangeBan) {
	lua_checkstack(pLua, 3); // we need 3 (1 table, 2 id, 3 value) empty slots in stack, check it to be sure

	lua_newtable(pLua);
	int i = lua_gettop(pLua);

    lua_pushliteral(pLua, "sIPFrom");
    lua_pushstring(pLua, pRangeBan->m_sIpFrom);
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sIPTo");
    lua_pushstring(pLua, pRangeBan->m_sIpTo);
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sReason");
    if(pRangeBan->m_sReason == NULL) {
		lua_pushnil(pLua);
	} else {
		lua_pushstring(pLua, pRangeBan->m_sReason);
	}
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "sBy");
    if(pRangeBan->m_sBy == NULL) {
		lua_pushnil(pLua);
	} else {
		lua_pushstring(pLua, pRangeBan->m_sBy);
	}
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "iExpireTime");
#if LUA_VERSION_NUM < 503
	((pRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == false ? lua_pushnil(pLua) : lua_pushnumber(pLua, (double)pRangeBan->m_tTempBanExpire);
#else
    ((pRangeBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == false ? lua_pushnil(pLua) : lua_pushinteger(pLua, pRangeBan->m_tTempBanExpire);
#endif
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "bFullIpBan");
    ((pRangeBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, i);
}
//------------------------------------------------------------------------------

static int Save(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'Save' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

	BanManager::m_Ptr->Save(true);

    return 0;
}
//------------------------------------------------------------------------------

static int GetBans(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetBans' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    lua_newtable(pLua);
    int t = lua_gettop(pLua), i = 0;

    time_t acc_time;
    time(&acc_time);

    BanItem * curBan = NULL,
        * nextBan = BanManager::m_Ptr->m_pTempBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
		nextBan = curBan->m_pNext;

        if(acc_time > curBan->m_tTempBanExpire) {
			BanManager::m_Ptr->Rem(curBan);
            delete curBan;

			continue;
        }

#if LUA_VERSION_NUM < 503
		lua_pushnumber(pLua, ++i);
#else
        lua_pushinteger(pLua, ++i);
#endif
        PushBan(pLua, curBan);        
        lua_rawset(pLua, t);
    }

	nextBan = BanManager::m_Ptr->m_pPermBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
		nextBan = curBan->m_pNext;

#if LUA_VERSION_NUM < 503
		lua_pushnumber(pLua, ++i);
#else
        lua_pushinteger(pLua, ++i);
#endif
        PushBan(pLua, curBan);        
        lua_rawset(pLua, t);
    }

    return 1;
}
//---------------------------------------------------------------------------

static int GetTempBans(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetTempBans' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    lua_newtable(pLua);
    int t = lua_gettop(pLua), i = 0;

    time_t acc_time;
    time(&acc_time);

    BanItem * curBan = NULL,
        * nextBan = BanManager::m_Ptr->m_pTempBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
		nextBan = curBan->m_pNext;

        if(acc_time > curBan->m_tTempBanExpire) {
			BanManager::m_Ptr->Rem(curBan);
            delete curBan;

			continue;
        }

#if LUA_VERSION_NUM < 503
		lua_pushnumber(pLua, ++i);
#else
        lua_pushinteger(pLua, ++i);
#endif
        PushBan(pLua, curBan);        
        lua_rawset(pLua, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetPermBans(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetPermBans' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    lua_newtable(pLua);
    int t = lua_gettop(pLua), i = 0;

	BanItem * curBan = NULL,
        * nextBan = BanManager::m_Ptr->m_pPermBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
		nextBan = curBan->m_pNext;

#if LUA_VERSION_NUM < 503
		lua_pushnumber(pLua, ++i);
#else
        lua_pushinteger(pLua, ++i);
#endif
        PushBan(pLua, curBan);        
        lua_rawset(pLua, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetBan(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'GetBan' (1 expected, got %d)", lua_gettop(pLua));
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

    time_t acc_time;
    time(&acc_time);

    size_t szLen;
    char * sValue = (char *)lua_tolstring(pLua, 1, &szLen);

	BanItem *Ban = BanManager::m_Ptr->FindNick(sValue, szLen);

	uint8_t ui128Hash[16];
	memset(ui128Hash, 0, 16);

	if(HashIP(sValue, ui128Hash) == true) {
        lua_settop(pLua, 0);
        
        lua_newtable(pLua);
        int t = lua_gettop(pLua), i = 0;

        if(Ban != NULL) {
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, ++i);
#else
            lua_pushinteger(pLua, ++i);
#endif
            PushBan(pLua, Ban); 
            lua_rawset(pLua, t);
        }

		Ban = BanManager::m_Ptr->FindIP(ui128Hash, acc_time);
        if(Ban != NULL) {
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, ++i);
#else
            lua_pushinteger(pLua, ++i);
#endif
            PushBan(pLua, Ban);        
            lua_rawset(pLua, t);

            BanItem * curBan = NULL,
                * nextBan = Ban->m_pHashIpTableNext;
        
            while(nextBan != NULL) {
                curBan = nextBan;
				nextBan = curBan->m_pHashIpTableNext;

				if((((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) && acc_time > curBan->m_tTempBanExpire) {
					BanManager::m_Ptr->Rem(curBan);
                    delete curBan;

        			continue;
                }

#if LUA_VERSION_NUM < 503
				lua_pushnumber(pLua, ++i);
#else
                lua_pushinteger(pLua, ++i);
#endif
                PushBan(pLua, curBan);        
                lua_rawset(pLua, t);
            }
        }
        return 1;
    } else {
        lua_settop(pLua, 0);

        if(Ban == NULL) {
        	lua_pushnil(pLua);
            return 1;
        }

        PushBan(pLua, Ban); 
        return 1;
    }
}
//------------------------------------------------------------------------------

static int GetPermBan(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'GetPermBan' (1 expected, got %d)", lua_gettop(pLua));
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
    char * sValue = (char *)lua_tolstring(pLua, 1, &szLen);

	BanItem *Ban = BanManager::m_Ptr->FindPermNick(sValue, szLen);

	uint8_t ui128Hash[16];
	memset(ui128Hash, 0, 16);

    if(HashIP(sValue, ui128Hash) == true) {
        lua_settop(pLua, 0);
        
        lua_newtable(pLua);
        int t = lua_gettop(pLua), i = 0;

        if(Ban != NULL) {
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, ++i);
#else
            lua_pushinteger(pLua, ++i);
#endif
            PushBan(pLua, Ban); 
            lua_rawset(pLua, t);
        }

		Ban = BanManager::m_Ptr->FindPermIP(ui128Hash);
        if(Ban != NULL) {
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, ++i);
#else
            lua_pushinteger(pLua, ++i);
#endif
            PushBan(pLua, Ban);        
            lua_rawset(pLua, t);

            BanItem * curBan = NULL,
                * nextBan = Ban->m_pHashIpTableNext;
        
            while(nextBan != NULL) {
                curBan = nextBan;
        		nextBan = curBan->m_pHashIpTableNext;

				if(((curBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM) == false) {
        			continue;
                }

#if LUA_VERSION_NUM < 503
				lua_pushnumber(pLua, ++i);
#else
                lua_pushinteger(pLua, ++i);
#endif
                PushBan(pLua, curBan);        
                lua_rawset(pLua, t);
            }
        }
        return 1;
    } else {
        lua_settop(pLua, 0);

        if(Ban == NULL) {
        	lua_pushnil(pLua);
            return 1;
        }

        PushBan(pLua, Ban); 
        return 1;
    }
}
//------------------------------------------------------------------------------

static int GetTempBan(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'GetTempBan' (1 expected, got %d)", lua_gettop(pLua));
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

    time_t acc_time;
    time(&acc_time);

    size_t szLen;
    char * sValue = (char *)lua_tolstring(pLua, 1, &szLen);

	BanItem * Ban = BanManager::m_Ptr->FindTempNick(sValue, szLen);

	uint8_t ui128Hash[16];
	memset(ui128Hash, 0, 16);

    if(HashIP(sValue, ui128Hash) == true) {
        lua_settop(pLua, 0);
        
        lua_newtable(pLua);
        int t = lua_gettop(pLua), i = 0;

        if(Ban != NULL) {
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, ++i);
#else
            lua_pushinteger(pLua, ++i);
#endif
            PushBan(pLua, Ban); 
            lua_rawset(pLua, t);
        }

		Ban = BanManager::m_Ptr->FindTempIP(ui128Hash, acc_time);
        if(Ban != NULL) {
#if LUA_VERSION_NUM < 503
			lua_pushnumber(pLua, ++i);
#else
            lua_pushinteger(pLua, ++i);
#endif
            PushBan(pLua, Ban);        
            lua_rawset(pLua, t);

            BanItem * curBan = NULL,
                * nextBan = Ban->m_pHashIpTableNext;
        
            while(nextBan != NULL) {
                curBan = nextBan;
        		nextBan = curBan->m_pHashIpTableNext;

				if(((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
                    if(acc_time > curBan->m_tTempBanExpire) {
						BanManager::m_Ptr->Rem(curBan);
                        delete curBan;
    
            			continue;
                    }

#if LUA_VERSION_NUM < 503
					lua_pushnumber(pLua, ++i);
#else
                    lua_pushinteger(pLua, ++i);
#endif
                    PushBan(pLua, curBan);        
                    lua_rawset(pLua, t);
                }
            }
        }
        return 1;
    } else {
        lua_settop(pLua, 0);

        if(Ban == NULL) {
        	lua_pushnil(pLua);
            return 1;
        }

        PushBan(pLua, Ban); 
        return 1;
    }
}
//------------------------------------------------------------------------------

static int GetRangeBans(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetRangeBans' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }
    
    lua_newtable(pLua);
	int t = lua_gettop(pLua), i = 0;

    time_t acc_time;
    time(&acc_time);

    RangeBanItem * curBan = NULL,
        * nextBan = BanManager::m_Ptr->m_pRangeBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
		nextBan = curBan->m_pNext;
        
        if((((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) && acc_time > curBan->m_tTempBanExpire) {
			BanManager::m_Ptr->RemRange(curBan);
            delete curBan;

			continue;
        }

#if LUA_VERSION_NUM < 503
		lua_pushnumber(pLua, ++i);
#else
        lua_pushinteger(pLua, ++i);
#endif
        PushRangeBan(pLua, curBan);
        lua_rawset(pLua, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetTempRangeBans(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetTempRangeBans' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }
    
    lua_newtable(pLua);
	int t = lua_gettop(pLua), i = 0;

    time_t acc_time;
    time(&acc_time);

    RangeBanItem * curBan = NULL,
        * nextBan = BanManager::m_Ptr->m_pRangeBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
		nextBan = curBan->m_pNext;

        if(((curBan->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == false) {
            continue;
        }
        
        if(acc_time > curBan->m_tTempBanExpire) {
			BanManager::m_Ptr->RemRange(curBan);
            delete curBan;

			continue;
        }

#if LUA_VERSION_NUM < 503
		lua_pushnumber(pLua, ++i);
#else
        lua_pushinteger(pLua, ++i);
#endif
        PushRangeBan(pLua, curBan);
        lua_rawset(pLua, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetPermRangeBans(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetPermRangeBans' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }
    
    lua_newtable(pLua);
	int t = lua_gettop(pLua), i = 0;

    RangeBanItem * curBan = NULL,
        * nextBan = BanManager::m_Ptr->m_pRangeBanListS;

    while(nextBan != NULL) {
        curBan = nextBan;
		nextBan = curBan->m_pNext;

        if(((curBan->m_ui8Bits & BanManager::PERM) == BanManager::PERM) == false) {
            continue;
        }

#if LUA_VERSION_NUM < 503
		lua_pushnumber(pLua, ++i);
#else
        lua_pushinteger(pLua, ++i);
#endif
        PushRangeBan(pLua, curBan);
        lua_rawset(pLua, t);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int GetRangeBan(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'GetRangeBan' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    size_t szFromLen, szToLen;
    char * sFrom = (char *)lua_tolstring(pLua, 1, &szFromLen);
    char * sTo = (char *)lua_tolstring(pLua, 2, &szToLen);

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

    if(szFromLen == 0 || szToLen == 0 || HashIP(sFrom, ui128FromHash) == false || HashIP(sTo, ui128ToHash) == false || memcmp(ui128ToHash, ui128FromHash, 16) <= 0) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    lua_settop(pLua, 0);

    time_t acc_time;
    time(&acc_time);

	RangeBanItem * cur = NULL,
        * next = BanManager::m_Ptr->m_pRangeBanListS;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(memcmp(cur->m_ui128FromIpHash, ui128FromHash, 16) == 0 && memcmp(cur->m_ui128ToIpHash, ui128ToHash, 16) == 0) {
            // PPK ... check if it's temban and then if it's expired
            if(((cur->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
                if(acc_time >= cur->m_tTempBanExpire) {
					BanManager::m_Ptr->RemRange(cur);
                    delete cur;

					continue;
                }
            }
            PushRangeBan(pLua, cur);
            return 1;
        }
    }

	lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int GetRangePermBan(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'GetRangePermBan' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    size_t szFromLen, szToLen;
    char * sFrom = (char *)lua_tolstring(pLua, 1, &szFromLen);
    char * sTo = (char *)lua_tolstring(pLua, 2, &szToLen);

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

	if(szFromLen == 0 || szToLen == 0 || HashIP(sFrom, ui128FromHash) == false || HashIP(sTo, ui128ToHash) == false || memcmp(ui128ToHash, ui128FromHash, 16) <= 0) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    lua_settop(pLua, 0);

    RangeBanItem * cur = NULL,
        * next = BanManager::m_Ptr->m_pRangeBanListS;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(memcmp(cur->m_ui128FromIpHash, ui128FromHash, 16) == 0 && memcmp(cur->m_ui128ToIpHash, ui128ToHash, 16) == 0) {
            if(((cur->m_ui8Bits & BanManager::PERM) == BanManager::PERM) == true) {
                PushRangeBan(pLua, cur);
                return 1;
            }
        }
    }

	lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int GetRangeTempBan(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'GetRangeTempBan' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    size_t szFromLen, szToLen;
    char * sFrom = (char *)lua_tolstring(pLua, 1, &szFromLen);
    char * sTo = (char *)lua_tolstring(pLua, 2, &szToLen);

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

	if(szFromLen == 0 || szToLen == 0 || HashIP(sFrom, ui128FromHash) == false || HashIP(sTo, ui128ToHash) == false || memcmp(ui128ToHash, ui128FromHash, 16) <= 0) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    lua_settop(pLua, 0);

    time_t acc_time;
    time(&acc_time);

    RangeBanItem * cur = NULL,
        * next = BanManager::m_Ptr->m_pRangeBanListS;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        if(memcmp(cur->m_ui128FromIpHash, ui128FromHash, 16) == 0 && memcmp(cur->m_ui128ToIpHash, ui128ToHash, 16) == 0) {
            // PPK ... check if it's temban and then if it's expired
            if(((cur->m_ui8Bits & BanManager::TEMP) == BanManager::TEMP) == true) {
                if(acc_time >= cur->m_tTempBanExpire) {
					BanManager::m_Ptr->RemRange(cur);
                    delete cur;

					continue;
                }

                PushRangeBan(pLua, cur);
                return 1;
            }
        }
    }

	lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int Unban(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'Unban' (1 expected, got %d)", lua_gettop(pLua));
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
    char * sWhat = (char *)lua_tolstring(pLua, 1, &szLen);

    if(szLen == 0) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }
    
    if(BanManager::m_Ptr->Unban(sWhat) == false) {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
    } else {
        lua_settop(pLua, 0);
        lua_pushboolean(pLua, 1);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int UnbanPerm(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'UnbanPerm' (1 expected, got %d)", lua_gettop(pLua));
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
    char * sWhat = (char *)lua_tolstring(pLua, 1, &szLen);

    if(szLen == 0) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }
    
    if(BanManager::m_Ptr->PermUnban(sWhat) == false) {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
    } else {
        lua_settop(pLua, 0);
        lua_pushboolean(pLua, 1);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int UnbanTemp(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'UnbanTemp' (1 expected, got %d)", lua_gettop(pLua));
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
    char * sWhat = (char *)lua_tolstring(pLua, 1, &szLen);

    if(szLen == 0) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }
    
    if(BanManager::m_Ptr->TempUnban(sWhat) == false) {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
    } else {
        lua_settop(pLua, 0);
        lua_pushboolean(pLua, 1);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int UnbanAll(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'UnbanAll' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
		lua_settop(pLua, 0);
        return 0;
    }

    size_t szLen;
    char * sIP = (char *)lua_tolstring(pLua, 1, &szLen);

	uint8_t ui128Hash[16];
	memset(ui128Hash, 0, 16);

    if(szLen == 0 || HashIP(sIP, ui128Hash) == false) {
		lua_settop(pLua, 0);
        return 0;
    }

    lua_settop(pLua, 0);

	BanManager::m_Ptr->RemoveAllIP(ui128Hash);

    return 0;
}
//------------------------------------------------------------------------------

static int UnbanPermAll(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'UnbanPermAll' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
		lua_settop(pLua, 0);
        return 0;
    }

    size_t szLen;
    char * sIP = (char *)lua_tolstring(pLua, 1, &szLen);

	uint8_t ui128Hash[16];
	memset(ui128Hash, 0, 16);

    if(szLen == 0 || HashIP(sIP, ui128Hash) == false) {
		lua_settop(pLua, 0);
        return 0;
    }

    lua_settop(pLua, 0);

	BanManager::m_Ptr->RemovePermAllIP(ui128Hash);

    return 0;
}
//------------------------------------------------------------------------------

static int UnbanTempAll(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'UnbanTempAll' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
		lua_settop(pLua, 0);
        return 0;
    }

    size_t szLen;
    char * sIP = (char *)lua_tolstring(pLua, 1, &szLen);

	uint8_t ui128Hash[16];
	memset(ui128Hash, 0, 16);

    if(szLen == 0 || HashIP(sIP, ui128Hash) == false) {
		lua_settop(pLua, 0);
        return 0;
    }

    lua_settop(pLua, 0);

	BanManager::m_Ptr->RemoveTempAllIP(ui128Hash);

    return 0;
}
//------------------------------------------------------------------------------

static int RangeUnban(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'RangeUnban' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    size_t szFromIpLen, szToIpLen;
    char * sFromIp = (char *)lua_tolstring(pLua, 1, &szFromIpLen);
    char * sToIp = (char *)lua_tolstring(pLua, 2, &szToIpLen);

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

	if(szFromIpLen != 0 && szToIpLen != 0 && HashIP(sFromIp, ui128FromHash) == true && HashIP(sToIp, ui128ToHash) == true &&
		memcmp(ui128ToHash, ui128FromHash, 16) > 0 && BanManager::m_Ptr->RangeUnban(ui128FromHash, ui128ToHash) == true) {
        lua_settop(pLua, 0);
        lua_pushboolean(pLua, 1);
        return 1;
    }

    lua_settop(pLua, 0);
    lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int RangeUnbanPerm(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'RangeUnbanPerm' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    size_t szFromIpLen, szToIpLen;
    char * sFromIp = (char *)lua_tolstring(pLua, 1, &szFromIpLen);
    char * sToIp = (char *)lua_tolstring(pLua, 2, &szToIpLen);

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

	if(szFromIpLen != 0 && szToIpLen != 0 && HashIP(sFromIp, ui128FromHash) == true && HashIP(sToIp, ui128ToHash) == true &&
		memcmp(ui128ToHash, ui128FromHash, 16) > 0 && BanManager::m_Ptr->RangeUnban(ui128FromHash, ui128ToHash, BanManager::PERM) == true) {
        lua_settop(pLua, 0);
        lua_pushboolean(pLua, 1);
        return 1;
    }

    lua_settop(pLua, 0);
    lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int RangeUnbanTemp(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'RangeUnbanTemp' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    size_t szFromIpLen, szToIpLen;
    char * sFromIp = (char *)lua_tolstring(pLua, 1, &szFromIpLen);
    char * sToIp = (char *)lua_tolstring(pLua, 2, &szToIpLen);

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

	if(szFromIpLen != 0 && szToIpLen != 0 && HashIP(sFromIp, ui128FromHash) == true && HashIP(sToIp, ui128ToHash) == true &&
		memcmp(ui128ToHash, ui128FromHash, 16) > 0 && BanManager::m_Ptr->RangeUnban(ui128FromHash, ui128ToHash, BanManager::TEMP) == true) {
        lua_settop(pLua, 0);
        lua_pushboolean(pLua, 1);
        return 1;
    }

    lua_settop(pLua, 0);
    lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int ClearBans(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'ClearBans' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

	BanManager::m_Ptr->ClearTemp();
	BanManager::m_Ptr->ClearPerm();

    return 0;
}
//------------------------------------------------------------------------------

static int ClearPermBans(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'ClearPermBans' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

	BanManager::m_Ptr->ClearPerm();

    return 0;
}
//------------------------------------------------------------------------------

static int ClearTempBans(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'ClearTempBans' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

	BanManager::m_Ptr->ClearTemp();

    return 0;
}
//------------------------------------------------------------------------------

static int ClearRangeBans(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'ClearRangeBans' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

	BanManager::m_Ptr->ClearRange();

    return 0;
}
//------------------------------------------------------------------------------

static int ClearRangePermBans(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'ClearRangePermBans' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

	BanManager::m_Ptr->ClearPermRange();

    return 0;
}
//------------------------------------------------------------------------------

static int ClearRangeTempBans(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'ClearRangeTempBans' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    BanManager::m_Ptr->ClearTempRange();

    return 0;
}
//------------------------------------------------------------------------------

static int Ban(lua_State * pLua) {
	if(lua_gettop(pLua) != 4) {
        luaL_error(pLua, "bad argument count to 'Ban' (4 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING || lua_type(pLua, 4) != LUA_TBOOLEAN) {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        luaL_checktype(pLua, 4, LUA_TBOOLEAN);
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    User *u = ScriptGetUser(pLua, 4, "Ban");
                
    if(u == NULL) {
		lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szReasonLen;
    char * sReason = (char *)lua_tolstring(pLua, 2, &szReasonLen);
    if(szReasonLen == 0) {
        sReason = NULL;
    }

	size_t szByLen;
    char *sBy = (char *)lua_tolstring(pLua, 3, &szByLen);
    if(szByLen == 0) {
        sBy = NULL;
    }

    bool bFull = lua_toboolean(pLua, 4) == 0 ? false : true;

    BanManager::m_Ptr->Ban(u, sReason, sBy, bFull);

	UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) banned by script.", u->m_sNick, u->m_sIP);

    u->Close();

    lua_settop(pLua, 0);
    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int BanIP(lua_State * pLua) {
	if(lua_gettop(pLua) != 4) {
        luaL_error(pLua, "bad argument count to 'Ban' (4 expected, got %d)", lua_gettop(pLua));
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

    size_t szIpLen;
    char * sIP = (char *)lua_tolstring(pLua, 1, &szIpLen);
    if(szIpLen == 0) {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    size_t szReasonLen;
    char * sReason = (char *)lua_tolstring(pLua, 2, &szReasonLen);
    if(szReasonLen == 0) {
        sReason = NULL;
    }

    size_t szByLen;
    char * sBy = (char *)lua_tolstring(pLua, 3, &szByLen);
    if(szByLen == 0) {
        sBy = NULL;
    }

    bool bFull = lua_toboolean(pLua, 4) == 0 ? false : true;

    if(BanManager::m_Ptr->BanIp(NULL, sIP, sReason, sBy, bFull) == 0) {
        lua_settop(pLua, 0);
        lua_pushboolean(pLua, 1);
    } else {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int BanNick(lua_State * pLua) {
	if(lua_gettop(pLua) != 3) {
        luaL_error(pLua, "bad argument count to 'BanNick' (3 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
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

    size_t szReasonLen;
    char * sReason = (char *)lua_tolstring(pLua, 2, &szReasonLen);
    if(szReasonLen == 0) {
        sReason = NULL;
    }

    size_t szByLen;
    char * sBy = (char *)lua_tolstring(pLua, 3, &szByLen);
    if(szByLen == 0) {
        sBy = NULL;
    }

    User *curUser = HashManager::m_Ptr->FindUser(sNick, szNickLen);
    if(curUser != NULL) {
        if(BanManager::m_Ptr->NickBan(curUser, NULL, sReason, sBy) == true) {
			UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) nickbanned by script.", curUser->m_sNick, curUser->m_sIP);

            curUser->Close();
            lua_settop(pLua, 0);
            lua_pushboolean(pLua, 1);
        } else {
            curUser->Close();
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
        }
    } else {
        if(BanManager::m_Ptr->NickBan(NULL, sNick, sReason, sBy) == true) {
			UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick %s nickbanned by script.", sNick);

            lua_settop(pLua, 0);
            lua_pushboolean(pLua, 1);
        } else {
        	lua_settop(pLua, 0);
            lua_pushnil(pLua);
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

static int TempBan(lua_State * pLua) {
	if(lua_gettop(pLua) != 5) {
        luaL_error(pLua, "bad argument count to 'TempBan' (5 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TTABLE || lua_type(pLua, 2) != LUA_TNUMBER || lua_type(pLua, 3) != LUA_TSTRING || 
        lua_type(pLua, 4) != LUA_TSTRING || lua_type(pLua, 5) != LUA_TBOOLEAN) {
        luaL_checktype(pLua, 1, LUA_TTABLE);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        luaL_checktype(pLua, 4, LUA_TSTRING);
        luaL_checktype(pLua, 5, LUA_TBOOLEAN);
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    User *u = ScriptGetUser(pLua, 5, "TempBan");

    if(u == NULL) {
		lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

#if LUA_VERSION_NUM < 503
	uint32_t iMinutes = (uint32_t)lua_tonumber(pLua, 2);
#else
	uint32_t iMinutes = (uint32_t)lua_tointeger(pLua, 2);
#endif

	size_t szReasonLen;
    char * sReason = (char *)lua_tolstring(pLua, 3, &szReasonLen);
    if(szReasonLen == 0) {
        sReason = NULL;
    }

	size_t szByLen;
    char * sBy = (char *)lua_tolstring(pLua, 4, &szByLen);
    if(szByLen == 0) {
        sBy = NULL;
    }

    bool bFull = lua_toboolean(pLua, 5) == 0 ? false : true;

    BanManager::m_Ptr->TempBan(u, sReason, sBy, iMinutes, 0, bFull);

	UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) tempbanned by script.", u->m_sNick, u->m_sIP);

    u->Close();

    lua_settop(pLua, 0);
    lua_pushboolean(pLua, 1);
    return 1;
}
//------------------------------------------------------------------------------

static int TempBanIP(lua_State * pLua) {
	if(lua_gettop(pLua) != 5) {
        luaL_error(pLua, "bad argument count to 'TempBanIP' (5 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TNUMBER || lua_type(pLua, 3) != LUA_TSTRING || 
        lua_type(pLua, 4) != LUA_TSTRING || lua_type(pLua, 5) != LUA_TBOOLEAN) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        luaL_checktype(pLua, 4, LUA_TSTRING);
        luaL_checktype(pLua, 5, LUA_TBOOLEAN);
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    size_t szIpLen;
    char * sIP = (char *)lua_tolstring(pLua, 1, &szIpLen);
    if(szIpLen == 0) {
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

#if LUA_VERSION_NUM < 503
	uint32_t i32Minutes = (uint32_t)lua_tonumber(pLua, 2);
#else
	uint32_t i32Minutes = (uint32_t)lua_tointeger(pLua, 2);
#endif

    size_t szReasonLen;
    char * sReason = (char *)lua_tolstring(pLua, 3, &szReasonLen);
    if(szReasonLen == 0) {
        sReason = NULL;
    }

    size_t szByLen;
    char * sBy = (char *)lua_tolstring(pLua, 4, &szByLen);
    if(szByLen == 0) {
        sBy = NULL;
    }

    bool bFull = lua_toboolean(pLua, 5) == 0 ? false : true;

    if(BanManager::m_Ptr->TempBanIp(NULL, sIP, sReason, sBy, i32Minutes, 0, bFull) == 0) {
        lua_settop(pLua, 0);
        lua_pushboolean(pLua, 1);
    } else {
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int TempBanNick(lua_State * pLua) {
	if(lua_gettop(pLua) != 4) {
        luaL_error(pLua, "bad argument count to 'TempBanNick' (4 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TNUMBER || lua_type(pLua, 3) != LUA_TSTRING || lua_type(pLua, 4) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        luaL_checktype(pLua, 4, LUA_TSTRING);
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

#if LUA_VERSION_NUM < 503
	uint32_t i32Minutes = (uint32_t)lua_tonumber(pLua, 2);
#else
	uint32_t i32Minutes = (uint32_t)lua_tointeger(pLua, 2);
#endif

    size_t szReasonLen;
    char * sReason = (char *)lua_tolstring(pLua, 3, &szReasonLen);
    if(szReasonLen == 0) {
        sReason = NULL;
    }

    size_t szByLen;
    char * sBy = (char *)lua_tolstring(pLua, 4, &szByLen);
    if(szByLen == 0) {
        sBy = NULL;
    }

    User *curUser = HashManager::m_Ptr->FindUser(sNick, szNickLen);
    if(curUser != NULL) {
        if(BanManager::m_Ptr->NickTempBan(curUser, NULL, sReason, sBy, i32Minutes, 0) == true) {
			UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) nickbanned by script.", curUser->m_sNick, curUser->m_sIP);

			curUser->Close();
            lua_settop(pLua, 0);
            lua_pushboolean(pLua, 1);
        } else {
            curUser->Close();
            lua_settop(pLua, 0);
            lua_pushnil(pLua);
        }
    } else {
        if(BanManager::m_Ptr->NickTempBan(NULL, sNick, sReason, sBy, i32Minutes, 0) == true) {
			UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick %s nickbanned by script.", sNick);

            lua_settop(pLua, 0);
            lua_pushboolean(pLua, 1);
        } else {
        	lua_settop(pLua, 0);
            lua_pushnil(pLua);
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

static int RangeBan(lua_State * pLua) {
	if(lua_gettop(pLua) != 5) {
        luaL_error(pLua, "bad argument count to 'RangeBan' (5 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING || 
        lua_type(pLua, 4) != LUA_TSTRING || lua_type(pLua, 5) != LUA_TBOOLEAN) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        luaL_checktype(pLua, 4, LUA_TSTRING);
        luaL_checktype(pLua, 5, LUA_TBOOLEAN);
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    size_t szFromIpLen;
    char * sFromIP = (char *)lua_tolstring(pLua, 1, &szFromIpLen);

    size_t szToIpLen;
    char * sToIP = (char *)lua_tolstring(pLua, 2, &szToIpLen);

    size_t szReasonLen;
    char * sReason = (char *)lua_tolstring(pLua, 3, &szReasonLen);
    if(szReasonLen == 0) {
        sReason = NULL;
    }

    size_t szByLen;
    char * sBy = (char *)lua_tolstring(pLua, 4, &szByLen);
    if(szByLen == 0) {
        sBy = NULL;
    }

    bool bFull = lua_toboolean(pLua, 5) == 0 ? false : true;

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

	if(szFromIpLen != 0 && szToIpLen != 0 && HashIP(sFromIP, ui128FromHash) == true && HashIP(sToIP, ui128ToHash) == true &&
        memcmp(ui128ToHash, ui128FromHash, 16) > 0 && BanManager::m_Ptr->RangeBan(sFromIP, ui128FromHash, sToIP, ui128ToHash, sReason, sBy, bFull) == true) {
		lua_settop(pLua, 0);
		lua_pushboolean(pLua, 1);
        return 1;
    }

	lua_settop(pLua, 0);
	lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static int RangeTempBan(lua_State * pLua) {
	if(lua_gettop(pLua) != 6) {
        luaL_error(pLua, "bad argument count to 'RangeTempBan' (6 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 1;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TNUMBER || 
        lua_type(pLua, 4) != LUA_TSTRING || lua_type(pLua, 5) != LUA_TSTRING || lua_type(pLua, 6) != LUA_TBOOLEAN) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TNUMBER);
        luaL_checktype(pLua, 4, LUA_TSTRING);
        luaL_checktype(pLua, 5, LUA_TSTRING);
        luaL_checktype(pLua, 6, LUA_TBOOLEAN);
		lua_settop(pLua, 0);
		lua_pushnil(pLua);
        return 1;
    }

    size_t szFromIpLen;
    char * sFromIP = (char *)lua_tolstring(pLua, 1, &szFromIpLen);

    size_t szToIpLen;
    char * sToIP = (char *)lua_tolstring(pLua, 2, &szToIpLen);

#if LUA_VERSION_NUM < 503
	uint32_t i32Minutes = (uint32_t)lua_tonumber(pLua, 3);
#else
    uint32_t i32Minutes = (uint32_t)lua_tointeger(pLua, 3);
#endif

    size_t szReasonLen;
    char * sReason = (char *)lua_tolstring(pLua, 4, &szReasonLen);
    if(szReasonLen == 0) {
        sReason = NULL;
    }

    size_t szByLen;
    char * sBy = (char *)lua_tolstring(pLua, 5, &szByLen);
    if(szByLen == 0) {
        sBy = NULL;
    }

    bool bFull = lua_toboolean(pLua, 6) == 0 ? false : true;

	uint8_t ui128FromHash[16], ui128ToHash[16];
	memset(ui128FromHash, 0, 16);
	memset(ui128ToHash, 0, 16);

	if(szFromIpLen != 0 && szToIpLen != 0 && HashIP(sFromIP, ui128FromHash) == true && HashIP(sToIP, ui128ToHash) == true &&
		memcmp(ui128ToHash, ui128FromHash, 16) > 0 && BanManager::m_Ptr->RangeTempBan(sFromIP, ui128FromHash, sToIP, ui128ToHash, sReason, sBy, i32Minutes, 0, bFull) == true) {
		lua_settop(pLua, 0);
		lua_pushboolean(pLua, 1);
        return 1;
    }

	lua_settop(pLua, 0);
	lua_pushnil(pLua);
    return 1;
}
//------------------------------------------------------------------------------

static const luaL_Reg BanManRegs[] = {
    { "Save", Save },
	{ "GetBans", GetBans }, 
	{ "GetTempBans", GetTempBans }, 
	{ "GetPermBans", GetPermBans }, 
	{ "GetBan", GetBan }, 
	{ "GetPermBan", GetPermBan }, 
	{ "GetTempBan", GetTempBan }, 
	{ "GetRangeBans", GetRangeBans }, 
	{ "GetTempRangeBans", GetTempRangeBans }, 
	{ "GetPermRangeBans", GetPermRangeBans }, 
	{ "GetRangeBan", GetRangeBan }, 
	{ "GetRangePermBan", GetRangePermBan }, 
	{ "GetRangeTempBan", GetRangeTempBan }, 
	{ "Unban", Unban }, 
	{ "UnbanPerm", UnbanPerm }, 
	{ "UnbanTemp", UnbanTemp }, 
	{ "UnbanAll", UnbanAll }, 
	{ "UnbanPermAll", UnbanPermAll }, 
	{ "UnbanTempAll", UnbanTempAll }, 
	{ "RangeUnban", RangeUnban }, 
	{ "RangeUnbanPerm", RangeUnbanPerm }, 
	{ "RangeUnbanTemp", RangeUnbanTemp }, 
	{ "ClearBans", ClearBans }, 
	{ "ClearPermBans", ClearPermBans }, 
	{ "ClearTempBans", ClearTempBans }, 
	{ "ClearRangeBans", ClearRangeBans }, 
	{ "ClearRangePermBans", ClearRangePermBans }, 
	{ "ClearRangeTempBans", ClearRangeTempBans }, 
	{ "Ban", Ban }, 
	{ "BanIP", BanIP }, 
	{ "BanNick", BanNick }, 
	{ "TempBan", TempBan }, 
	{ "TempBanIP", TempBanIP }, 
	{ "TempBanNick", TempBanNick }, 
	{ "RangeBan", RangeBan }, 
	{ "RangeTempBan", RangeTempBan }, 
	{ NULL, NULL }
};
//---------------------------------------------------------------------------

#if LUA_VERSION_NUM > 501
int RegBanMan(lua_State * pLua) {
    luaL_newlib(pLua, BanManRegs);
    return 1;
#else
void RegBanMan(lua_State * pLua) {
    luaL_register(pLua, "BanMan", BanManRegs);
#endif
}
//---------------------------------------------------------------------------
