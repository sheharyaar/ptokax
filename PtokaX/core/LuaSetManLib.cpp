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
#include "LuaSetManLib.h"
//---------------------------------------------------------------------------
#include "eventqueue.h"
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "LuaScriptManager.h"
#include "SettingManager.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------

static int Save(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'Save' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    ScriptManager::m_Ptr->SaveScripts();

	SettingManager::m_Ptr->Save();

    return 0;
}
//------------------------------------------------------------------------------

static int GetMOTD(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetMOTD' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 0;
    }

	if(SettingManager::m_Ptr->m_sMOTD != NULL) {
        lua_pushlstring(pLua, SettingManager::m_Ptr->m_sMOTD, (size_t)SettingManager::m_Ptr->m_ui16MOTDLen);
    } else {
        lua_pushnil(pLua);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int SetMOTD(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'SetMOTD' (1 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TSTRING);
		lua_settop(pLua, 0);
        return 0;
    }

    size_t szLen = 0;
    char * sTxt = (char *)lua_tolstring(pLua, 1, &szLen);

	SettingManager::m_Ptr->SetMOTD(sTxt, szLen);

	SettingManager::m_Ptr->UpdateMOTD();

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int GetBool(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'GetBool' (1 expected, got %d)", lua_gettop(pLua));
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
	size_t szId = (size_t)lua_tonumber(pLua, 1);
#else
    size_t szId = (size_t)lua_tointeger(pLua, 1);
#endif

    lua_settop(pLua, 0);

    if(szId >= SETBOOL_IDS_END) {
        luaL_error(pLua, "bad argument #1 to 'GetBool' (it's not valid id)");
        lua_pushnil(pLua);
        return 1;
    }  

    SettingManager::m_Ptr->m_bBools[szId] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);

    return 1;
}
//------------------------------------------------------------------------------

static int SetBool(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'SetBool' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TBOOLEAN) {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        luaL_checktype(pLua, 2, LUA_TBOOLEAN);
        lua_settop(pLua, 0);
        return 0;
    }

#if LUA_VERSION_NUM < 503
	size_t szId = (size_t)lua_tonumber(pLua, 1);
#else
    size_t szId = (size_t)lua_tointeger(pLua, 1);
#endif

    bool bValue = (lua_toboolean(pLua, 2) == 0 ? false : true);

    lua_settop(pLua, 0);

    if(szId >= SETBOOL_IDS_END) {
        luaL_error(pLua, "bad argument #1 to 'SetBool' (it's not valid id)");
        return 0;
    }  

    if(szId == SETBOOL_ENABLE_SCRIPTING && bValue == false) {
        EventQueue::m_Ptr->AddNormal(EventQueue::EVENT_STOP_SCRIPTING, NULL);
        return 0;
    }

    if(szId == SETBOOL_HASH_PASSWORDS && bValue == true) {
        RegManager::m_Ptr->HashPasswords();
    }

    SettingManager::m_Ptr->SetBool(szId, bValue);

    return 0;
}
//------------------------------------------------------------------------------

static int GetNumber(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'GetNumber' (1 expected, got %d)", lua_gettop(pLua));
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
	size_t szId = (size_t)lua_tonumber(pLua, 1);
#else
    size_t szId = (size_t)lua_tointeger(pLua, 1);
#endif

    lua_settop(pLua, 0);

	if(szId >= (SETSHORT_IDS_END-1)) {
        luaL_error(pLua, "bad argument #1 to 'GetNumber' (it's not valid id)");
        lua_pushnil(pLua);
        return 1;
    }  

    lua_pushinteger(pLua, SettingManager::m_Ptr->m_i16Shorts[szId]);

    return 1;
}
//------------------------------------------------------------------------------

static int SetNumber(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'SetNumber' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TNUMBER) {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        lua_settop(pLua, 0);
        return 0;
    }

#if LUA_VERSION_NUM < 503
	size_t szId = (size_t)lua_tonumber(pLua, 1);
#else
    size_t szId = (size_t)lua_tointeger(pLua, 1);
#endif

    int16_t iValue = (int16_t)lua_tointeger(pLua, 2);

    lua_settop(pLua, 0);

	if(szId >= (SETSHORT_IDS_END-1)) {
        luaL_error(pLua, "bad argument #1 to 'SetNumber' (it's not valid id)");
        return 0;
    }  

    SettingManager::m_Ptr->SetShort(szId, iValue);

    return 0;
}
//------------------------------------------------------------------------------

static int GetString(lua_State * pLua) {
	if(lua_gettop(pLua) != 1) {
        luaL_error(pLua, "bad argument count to 'GetString' (1 expected, got %d)", lua_gettop(pLua));
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
	size_t szId = (size_t)lua_tonumber(pLua, 1);
#else
    size_t szId = (size_t)lua_tointeger(pLua, 1);
#endif

    lua_settop(pLua, 0);

    if(szId >= SETTXT_IDS_END) {
        luaL_error(pLua, "bad argument #1 to 'GetString' (it's not valid id)");
        lua_pushnil(pLua);
        return 1;
    }  

    if(SettingManager::m_Ptr->m_sTexts[szId] != NULL) {
        lua_pushlstring(pLua, SettingManager::m_Ptr->m_sTexts[szId], (size_t)SettingManager::m_Ptr->m_ui16TextsLens[szId]);
    } else {
        lua_pushnil(pLua);
    }

    return 1;
}
//------------------------------------------------------------------------------

static int SetString(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'SetString' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        lua_settop(pLua, 0);
        return 0;
    }

#if LUA_VERSION_NUM < 503
	size_t szId = (size_t)lua_tonumber(pLua, 1);
#else
    size_t szId = (size_t)lua_tointeger(pLua, 1);
#endif

    if(szId >= SETTXT_IDS_END) {
        luaL_error(pLua, "bad argument #1 to 'SetString' (it's not valid id)");
        return 0;
    }  

    size_t szLen;
    char * sValue = (char *)lua_tolstring(pLua, 2, &szLen);

    SettingManager::m_Ptr->SetText(szId, sValue, szLen);

    return 0;
}
//------------------------------------------------------------------------------

static int GetMinShare(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetMinShare' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 0;
    }

#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, (double)SettingManager::m_Ptr->m_ui64MinShare);
#else
    lua_pushinteger(pLua, SettingManager::m_Ptr->m_ui64MinShare);
#endif

    return 1;
}
//------------------------------------------------------------------------------

static int SetMinShare(lua_State * pLua) {
    int n = lua_gettop(pLua);

    if(n == 2) {
        if(lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TNUMBER) {
            luaL_checktype(pLua, 1, LUA_TNUMBER);
            luaL_checktype(pLua, 2, LUA_TNUMBER);
            lua_settop(pLua, 0);
            return 0;
        }

		SettingManager::m_Ptr->m_bUpdateLocked = true;

#if LUA_VERSION_NUM < 503
		SettingManager::m_Ptr->SetShort(SETSHORT_MIN_SHARE_LIMIT, (int16_t)lua_tonumber(pLua, 1));
		SettingManager::m_Ptr->SetShort(SETSHORT_MIN_SHARE_UNITS, (int16_t)lua_tonumber(pLua, 2));
#else
        SettingManager::m_Ptr->SetShort(SETSHORT_MIN_SHARE_LIMIT, (int16_t)lua_tointeger(pLua, 1));
		SettingManager::m_Ptr->SetShort(SETSHORT_MIN_SHARE_UNITS, (int16_t)lua_tointeger(pLua, 2));
#endif

		SettingManager::m_Ptr->m_bUpdateLocked = false;
    } else if(n == 1) {
        if(lua_type(pLua, 1) != LUA_TNUMBER) {
            luaL_checktype(pLua, 1, LUA_TNUMBER);
            lua_settop(pLua, 0);
            return 0;
        }

		double dBytes = lua_tonumber(pLua, 1);
        uint16_t iter = 0;
		for(; dBytes > 1024; iter++) {
			dBytes /= 1024;
		}

		SettingManager::m_Ptr->m_bUpdateLocked = true;

        SettingManager::m_Ptr->SetShort(SETSHORT_MIN_SHARE_LIMIT, (int16_t)dBytes);
		SettingManager::m_Ptr->SetShort(SETSHORT_MIN_SHARE_UNITS, iter);

		SettingManager::m_Ptr->m_bUpdateLocked = false;
    } else {
        luaL_error(pLua, "bad argument count to 'SetMinShare' (1 or 2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

	SettingManager::m_Ptr->UpdateMinShare();
	SettingManager::m_Ptr->UpdateShareLimitMessage();

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int GetMaxShare(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetMaxShare' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 0;
    }

#if LUA_VERSION_NUM < 503
	lua_pushnumber(pLua, (double)SettingManager::m_Ptr->m_ui64MaxShare);
#else
    lua_pushinteger(pLua, SettingManager::m_Ptr->m_ui64MaxShare);
#endif

    return 1;
}
//------------------------------------------------------------------------------

static int SetMaxShare(lua_State * pLua) {
    int n = lua_gettop(pLua);

    if(n == 2) {
        if(lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TNUMBER) {
            luaL_checktype(pLua, 1, LUA_TNUMBER);
            luaL_checktype(pLua, 2, LUA_TNUMBER);
            lua_settop(pLua, 0);
            return 0;
        }

		SettingManager::m_Ptr->m_bUpdateLocked = true;

#if LUA_VERSION_NUM < 503
		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_SHARE_LIMIT, (int16_t)lua_tonumber(pLua, 1));
		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_SHARE_UNITS, (int16_t)lua_tonumber(pLua, 2));
#else
        SettingManager::m_Ptr->SetShort(SETSHORT_MAX_SHARE_LIMIT, (int16_t)lua_tointeger(pLua, 1));
		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_SHARE_UNITS, (int16_t)lua_tointeger(pLua, 2));
#endif

		SettingManager::m_Ptr->m_bUpdateLocked = false;
    } else if(n == 1) {
        if(lua_type(pLua, 1) != LUA_TNUMBER) {
            luaL_checktype(pLua, 1, LUA_TNUMBER);
            lua_settop(pLua, 0);
            return 0;
        }

       	double dBytes = (double)lua_tonumber(pLua, 1);
        uint16_t iter = 0;
		for(; dBytes > 1024; iter++) {
			dBytes /= 1024;
		}

		SettingManager::m_Ptr->m_bUpdateLocked = true;

		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_SHARE_LIMIT, (int16_t)dBytes);
		SettingManager::m_Ptr->SetShort(SETSHORT_MAX_SHARE_UNITS, iter);

		SettingManager::m_Ptr->m_bUpdateLocked = false;
    } else {
        luaL_error(pLua, "bad argument count to 'SetMaxShare' (1 or 2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

	SettingManager::m_Ptr->UpdateMaxShare();
	SettingManager::m_Ptr->UpdateShareLimitMessage();

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int SetHubSlotRatio(lua_State * pLua) {
	if(lua_gettop(pLua) != 2) {
        luaL_error(pLua, "bad argument count to 'SetHubSlotRatio' (2 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TNUMBER || lua_type(pLua, 2) != LUA_TNUMBER) {
        luaL_checktype(pLua, 1, LUA_TNUMBER);
        luaL_checktype(pLua, 2, LUA_TNUMBER);
        lua_settop(pLua, 0);
        return 0;
    }

	SettingManager::m_Ptr->m_bUpdateLocked = true;

#if LUA_VERSION_NUM < 503
	SettingManager::m_Ptr->SetShort(SETSHORT_HUB_SLOT_RATIO_HUBS, (int16_t)lua_tonumber(pLua, 1));
	SettingManager::m_Ptr->SetShort(SETSHORT_HUB_SLOT_RATIO_SLOTS, (int16_t)lua_tonumber(pLua, 2));
#else
    SettingManager::m_Ptr->SetShort(SETSHORT_HUB_SLOT_RATIO_HUBS, (int16_t)lua_tointeger(pLua, 1));
	SettingManager::m_Ptr->SetShort(SETSHORT_HUB_SLOT_RATIO_SLOTS, (int16_t)lua_tointeger(pLua, 2));
#endif

	SettingManager::m_Ptr->m_bUpdateLocked = false;

	SettingManager::m_Ptr->UpdateHubSlotRatioMessage();

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int GetOpChat(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetOpChat' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 0;
    }

	lua_newtable(pLua);
	int i = lua_gettop(pLua);

	lua_pushliteral(pLua, "sNick");
    if(SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK] == NULL) {
		lua_pushnil(pLua);
	} else {
        lua_pushlstring(pLua, SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK],
        (size_t)SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_OP_CHAT_NICK]);
	}
	lua_rawset(pLua, i);

	lua_pushliteral(pLua, "sDescription");
    if(SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_DESCRIPTION] == NULL) {
		lua_pushnil(pLua);
	} else {
        lua_pushlstring(pLua, SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_DESCRIPTION],
        (size_t)SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_OP_CHAT_DESCRIPTION]);
	}
	lua_rawset(pLua, i);

	lua_pushliteral(pLua, "sEmail");
    if(SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_EMAIL] == NULL) {
		lua_pushnil(pLua);
	} else {
        lua_pushlstring(pLua, SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_EMAIL],
        (size_t)SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_OP_CHAT_EMAIL]);
	}
	lua_rawset(pLua, i);

    lua_pushliteral(pLua, "bEnabled");
    SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, i);

    return 1;
}
//------------------------------------------------------------------------------

static int SetOpChat(lua_State * pLua) {
	if(lua_gettop(pLua) != 4) {
        luaL_error(pLua, "bad argument count to 'SetOpChat' (4 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TBOOLEAN || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING || lua_type(pLua, 4) != LUA_TSTRING) {
        luaL_checktype(pLua, 1, LUA_TBOOLEAN);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        luaL_checktype(pLua, 4, LUA_TSTRING);
        lua_settop(pLua, 0);
        return 0;
    }

    char *NewBotName, *NewBotDescription, *NewBotEmail;
    size_t szNameLen, szDescrLen, szEmailLen;

    NewBotName = (char *)lua_tolstring(pLua, 2, &szNameLen);
    NewBotDescription = (char *)lua_tolstring(pLua, 3, &szDescrLen);
    NewBotEmail = (char *)lua_tolstring(pLua, 4, &szEmailLen);

    if(szNameLen == 0 || szNameLen > 64 || szDescrLen > 64 || szEmailLen > 64 ||
        strpbrk(NewBotName, " $|") != NULL || strpbrk(NewBotDescription, "$|") != NULL ||
		strpbrk(NewBotEmail, "$|") != NULL || HashManager::m_Ptr->FindUser(NewBotName, szNameLen) != NULL) {
        lua_settop(pLua, 0);
        return 0;
    }

    bool bBotHaveNewNick = (strcmp(SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK], NewBotName) != 0);
    bool bEnableBot = (lua_toboolean(pLua, 1) == 0 ? false : true);

    bool bRegStateChange = false, bDescriptionChange = false, bEmailChange = false;

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] != bEnableBot) {
        bRegStateChange = true;
    }

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true) {
        SettingManager::m_Ptr->DisableOpChat((bBotHaveNewNick == true || bEnableBot == false));
    }

    SettingManager::m_Ptr->m_bUpdateLocked = true;

    SettingManager::m_Ptr->SetBool(SETBOOL_REG_OP_CHAT, bEnableBot);

    if(bBotHaveNewNick == true) {
        SettingManager::m_Ptr->SetText(SETTXT_OP_CHAT_NICK, NewBotName);
    }

    if(SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_DESCRIPTION] == NULL ||
        strcmp(SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_DESCRIPTION], NewBotDescription) != 0) {
        if(szDescrLen != (size_t)(SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_DESCRIPTION] == NULL ? 0 : -1)) {
            bDescriptionChange = true;
        }

        SettingManager::m_Ptr->SetText(SETTXT_OP_CHAT_DESCRIPTION, NewBotDescription);
    }

    if(SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_EMAIL] == NULL ||
        strcmp(SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_EMAIL], NewBotEmail) != 0) {
        if(szEmailLen != (size_t)(SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_EMAIL] == NULL ? 0 : -1)) {
            bEmailChange = true;
        }

        SettingManager::m_Ptr->SetText(SETTXT_OP_CHAT_EMAIL, NewBotEmail);
    }

	SettingManager::m_Ptr->m_bUpdateLocked = false;

    SettingManager::m_Ptr->UpdateBotsSameNick();

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true &&
        (bRegStateChange == true || bBotHaveNewNick == true || bDescriptionChange == true || bEmailChange == true)) {
        SettingManager::m_Ptr->UpdateOpChat((bBotHaveNewNick == true || bRegStateChange == true));
    }

    lua_settop(pLua, 0);
    return 0;
}
//------------------------------------------------------------------------------

static int GetHubBot(lua_State * pLua) {
	if(lua_gettop(pLua) != 0) {
        luaL_error(pLua, "bad argument count to 'GetHubBot' (0 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        lua_pushnil(pLua);
        return 0;
    }

	lua_newtable(pLua);
	int i = lua_gettop(pLua);

	lua_pushliteral(pLua, "sNick");
    if(SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_NICK] == NULL) {
		lua_pushnil(pLua);
	} else {
        lua_pushlstring(pLua, SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_NICK],
        (size_t)SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_BOT_NICK]);
	}
	lua_rawset(pLua, i);

	lua_pushliteral(pLua, "sDescription");
    if(SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_DESCRIPTION] == NULL) {
		lua_pushnil(pLua); 
	} else {
        lua_pushlstring(pLua, SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_DESCRIPTION],
        (size_t)SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_BOT_DESCRIPTION]);
	}
	lua_rawset(pLua, i);

	lua_pushliteral(pLua, "sEmail");
    if(SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_EMAIL] == NULL) {
		lua_pushnil(pLua); 
	} else {
        lua_pushlstring(pLua, SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_EMAIL],
        (size_t)SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_BOT_EMAIL]);
	}
	lua_rawset(pLua, i);

    lua_pushliteral(pLua, "bEnabled");
    SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, i);

    lua_pushliteral(pLua, "bUsedAsHubSecAlias");
    SettingManager::m_Ptr->m_bBools[SETBOOL_USE_BOT_NICK_AS_HUB_SEC] == true ? lua_pushboolean(pLua, 1) : lua_pushnil(pLua);
    lua_rawset(pLua, i);

    return 1;
}
//------------------------------------------------------------------------------

static int SetHubBot(lua_State * pLua) {
	if(lua_gettop(pLua) != 5) {
        luaL_error(pLua, "bad argument count to 'SetHubBot' (5 expected, got %d)", lua_gettop(pLua));
        lua_settop(pLua, 0);
        return 0;
    }

    if(lua_type(pLua, 1) != LUA_TBOOLEAN || lua_type(pLua, 2) != LUA_TSTRING || lua_type(pLua, 3) != LUA_TSTRING || 
        lua_type(pLua, 4) != LUA_TSTRING || lua_type(pLua, 5) != LUA_TBOOLEAN) {
        luaL_checktype(pLua, 1, LUA_TBOOLEAN);
        luaL_checktype(pLua, 2, LUA_TSTRING);
        luaL_checktype(pLua, 3, LUA_TSTRING);
        luaL_checktype(pLua, 4, LUA_TSTRING);
        luaL_checktype(pLua, 5, LUA_TBOOLEAN);
        lua_settop(pLua, 0);
        return 0;
    }

    char *NewBotName, *NewBotDescription, *NewBotEmail;
    size_t szNameLen, szDescrLen, szEmailLen;

    NewBotName = (char *)lua_tolstring(pLua, 2, &szNameLen);
    NewBotDescription = (char *)lua_tolstring(pLua, 3, &szDescrLen);
    NewBotEmail = (char *)lua_tolstring(pLua, 4, &szEmailLen);

    if(szNameLen == 0 || szNameLen > 64 || szDescrLen > 64 || szEmailLen > 64 ||
        strpbrk(NewBotName, " $|") != NULL || strpbrk(NewBotDescription, "$|") != NULL ||
        strpbrk(NewBotEmail, "$|") != NULL || HashManager::m_Ptr->FindUser(NewBotName, szNameLen) != NULL) {
        lua_settop(pLua, 0);
        return 0;
    }

    bool bBotHaveNewNick = (strcmp(SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_NICK], NewBotName) != 0);
    bool bEnableBot = (lua_toboolean(pLua, 1) == 0 ? false : true);

    bool bRegStateChange = false, bDescriptionChange = false, bEmailChange = false;

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] != bEnableBot) {
        bRegStateChange = true;
    }

    SettingManager::m_Ptr->m_bUpdateLocked = true;

    if(SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_DESCRIPTION] == NULL ||
        strcmp(SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_DESCRIPTION], NewBotDescription) != 0) {
        if(szDescrLen != (size_t)(SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_DESCRIPTION] == NULL ? 0 : -1)) {
            bDescriptionChange = true;
        }

        SettingManager::m_Ptr->SetText(SETTXT_BOT_DESCRIPTION, NewBotDescription);
    }

    if(SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_EMAIL] == NULL ||
        strcmp(SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_EMAIL], NewBotEmail) != 0) {
        if(szEmailLen != (size_t)(SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_EMAIL] == NULL ? 0 : -1)) {
            bEmailChange = true;
        }

        SettingManager::m_Ptr->SetText(SETTXT_BOT_EMAIL, NewBotEmail);
    }

    SettingManager::m_Ptr->SetBool(SETBOOL_USE_BOT_NICK_AS_HUB_SEC, (lua_toboolean(pLua, 5) == 0 ? false : true));

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == true) {
        SettingManager::m_Ptr->m_bUpdateLocked = false;
        SettingManager::m_Ptr->DisableBot((bBotHaveNewNick == true || bEnableBot == false),
            (bRegStateChange == true || bBotHaveNewNick == true || bDescriptionChange == true || bEmailChange == true) ? true : false);
        SettingManager::m_Ptr->m_bUpdateLocked = true;
    }

    SettingManager::m_Ptr->SetBool(SETBOOL_REG_BOT, bEnableBot);

    if(bBotHaveNewNick == true) {
        SettingManager::m_Ptr->SetText(SETTXT_BOT_NICK, NewBotName);
    }

    SettingManager::m_Ptr->m_bUpdateLocked = false;

    SettingManager::m_Ptr->UpdateHubSec();
    SettingManager::m_Ptr->UpdateMOTD();
    SettingManager::m_Ptr->UpdateHubNameWelcome();
    SettingManager::m_Ptr->UpdateRegOnlyMessage();
    SettingManager::m_Ptr->UpdateShareLimitMessage();
    SettingManager::m_Ptr->UpdateSlotsLimitMessage();
    SettingManager::m_Ptr->UpdateHubSlotRatioMessage();
    SettingManager::m_Ptr->UpdateMaxHubsLimitMessage();
    SettingManager::m_Ptr->UpdateNoTagMessage();
    SettingManager::m_Ptr->UpdateNickLimitMessage();
    SettingManager::m_Ptr->UpdateBotsSameNick();

    if(SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == true &&
        (bRegStateChange == true || bBotHaveNewNick == true || bDescriptionChange == true || bEmailChange == true)) {
        SettingManager::m_Ptr->UpdateBot((bBotHaveNewNick == true || bRegStateChange == true));
    }

    return 0;
}
//------------------------------------------------------------------------------

static const luaL_Reg SetManRegs[] = {
    { "Save", Save },
	{ "GetMOTD", GetMOTD }, 
	{ "SetMOTD", SetMOTD }, 
	{ "GetBool", GetBool }, 
	{ "SetBool", SetBool }, 
	{ "GetNumber", GetNumber }, 
	{ "SetNumber", SetNumber }, 
	{ "GetString", GetString }, 
	{ "SetString", SetString }, 
	{ "GetMinShare", GetMinShare }, 
	{ "SetMinShare", SetMinShare }, 
	{ "GetMaxShare", GetMaxShare }, 
	{ "SetMaxShare", SetMaxShare }, 
	{ "SetHubSlotRatio", SetHubSlotRatio }, 
	{ "GetOpChat", GetOpChat }, 
	{ "SetOpChat", SetOpChat }, 
	{ "GetHubBot", GetHubBot }, 
	{ "SetHubBot", SetHubBot }, 
	{ NULL, NULL }
};
//---------------------------------------------------------------------------

#if LUA_VERSION_NUM > 501
int RegSetMan(lua_State * pLua) {
    luaL_newlib(pLua, SetManRegs);
    return 1;
#else
void RegSetMan(lua_State * pLua) {
    luaL_register(pLua, "SetMan", SetManRegs);
#endif
}
//---------------------------------------------------------------------------
