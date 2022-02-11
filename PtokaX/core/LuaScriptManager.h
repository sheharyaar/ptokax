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

//------------------------------------------------------------------------------
#ifndef LuaScriptManagerH
#define LuaScriptManagerH
//------------------------------------------------------------------------------
struct lua_State;
struct Script;
struct ScriptTimer;
struct User;
struct DcCommand;
//------------------------------------------------------------------------------

class ScriptManager {
private:
	Script * m_pRunningScriptE;

    ScriptManager(const ScriptManager&);
    const ScriptManager& operator=(const ScriptManager&);

	void AddRunningScript(Script * pScript);
	void RemoveRunningScript(Script * pScript);

	void LoadXML();
public:
    static ScriptManager * m_Ptr;

    Script * m_pRunningScriptS;

    Script ** m_ppScriptTable;
	User * m_pActualUser;

	ScriptTimer * m_pTimerListS, * m_pTimerListE;

    uint8_t m_ui8ScriptCount, m_ui8BotsCount;

    bool m_bMoved;

    enum LuaArrivals {
        CHAT_ARRIVAL,
        KEY_ARRIVAL,
        VALIDATENICK_ARRIVAL,
        PASSWORD_ARRIVAL,
        VERSION_ARRIVAL,
        GETNICKLIST_ARRIVAL,
        MYINFO_ARRIVAL,
        GETINFO_ARRIVAL,
        SEARCH_ARRIVAL,
        TO_ARRIVAL,
        CONNECTTOME_ARRIVAL,
        MULTICONNECTTOME_ARRIVAL,
        REVCONNECTTOME_ARRIVAL,
        SR_ARRIVAL,
        UDP_SR_ARRIVAL,
        KICK_ARRIVAL,
        OPFORCEMOVE_ARRIVAL,
        SUPPORTS_ARRIVAL,
        BOTINFO_ARRIVAL,
        CLOSE_ARRIVAL, 
        UNKNOWN_ARRIVAL
    };
    
    ScriptManager();
    ~ScriptManager();
    
    void Start();
    void Stop();

    void SaveScripts();

    void CheckForDeletedScripts();
    void CheckForNewScripts();

    void Restart();

    Script * FindScript(char * sName);
    Script * FindScript(lua_State * pLua);
    uint8_t FindScriptIdx(char * sName);

	bool AddScript(char * sName, const bool bEnabled, const bool bNew);

	bool StartScript(Script * pScript, const bool bEnable);
	void StopScript(Script * pScript, const bool bDisable);

	void MoveScript(const uint8_t ui8ScriptPosInTbl, const bool bUp);

    void DeleteScript(const uint8_t ui8ScriptPosInTbl);

    void OnStartup();
    void OnExit(const bool bForce = false);
    bool Arrival(DcCommand * pDcCommand, const uint8_t ui8Type);
    bool UserConnected(User * pUser);
	void UserDisconnected(User * pUser, Script * pScript = NULL);

    void PrepareMove(lua_State * pLua);
};
//------------------------------------------------------------------------------

#endif
