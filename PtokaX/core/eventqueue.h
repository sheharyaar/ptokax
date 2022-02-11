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
#ifndef eventqueueH
#define eventqueueH
//---------------------------------------------------------------------------

class EventQueue {
private:
#ifdef _WIN32
	CRITICAL_SECTION m_csEventQueue;
#else
	pthread_mutex_t m_mtxEventQueue;
#endif

    struct Event {
    	Event * m_pPrev, * m_pNext;

        char * m_sMsg;

        uint8_t m_ui128IpHash[16];
        uint8_t m_ui8Id;

        Event();

        Event(const Event&);
        const Event& operator=(const Event&);
    };

	Event * m_pNormalE, * m_pThreadE;

	EventQueue(const EventQueue&);
	const EventQueue& operator=(const EventQueue&);
public:
    static EventQueue * m_Ptr;

	Event * m_pNormalS, * m_pThreadS;

	enum {
        EVENT_RESTART, 
        EVENT_RSTSCRIPTS, 
        EVENT_RSTSCRIPT, 
        EVENT_STOPSCRIPT, 
        EVENT_STOP_SCRIPTING, 
        EVENT_SHUTDOWN, 
        EVENT_REGSOCK_MSG, 
        EVENT_SRVTHREAD_MSG, 
        EVENT_UDP_SR
	};

    EventQueue();
    ~EventQueue();

    void AddNormal(const uint8_t ui8Id, char * sMsg);
    void AddThread(const uint8_t ui8Id, char * sMsg, const sockaddr_storage * sas = NULL);
    void ProcessEvents();
};
//---------------------------------------------------------------------------

#endif
