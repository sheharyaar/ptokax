/*
 * PtokaX - hub server for Direct Connect peer to peer network.

 * Copyright (C) 2002-2005  Ptaczek, Ptaczek at PtokaX dot org
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
#include "DcCommands.h"
//---------------------------------------------------------------------------
#include "colUsers.h"
#include "GlobalDataQueue.h"
#include "hashBanManager.h"
#include "hashRegManager.h"
#include "hashUsrManager.h"
#include "LanguageManager.h"
#include "LuaScriptManager.h"
#include "ProfileManager.h"
#include "ServerManager.h"
#include "SettingManager.h"
#include "UdpDebug.h"
#include "User.h"
#include "utility.h"
#include "ZlibUtility.h"
//---------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//---------------------------------------------------------------------------
#include "DeFlood.h"
#include "HubCommands.h"
#include "IP2Country.h"
#include "ResNickManager.h"
#include "TextFileManager.h"
//---------------------------------------------------------------------------
#ifdef _BUILD_GUI
	#include "../gui.win/GuiUtil.h"
    #include "../gui.win/MainWindowPageUsersChat.h"
#endif
//---------------------------------------------------------------------------
DcCommands * DcCommands::m_Ptr = NULL;
//---------------------------------------------------------------------------

DcCommands::PassBf::PassBf(const uint8_t * ui128Hash) : m_pPrev (NULL), m_pNext(NULL), m_iCount(1) {
	memcpy(m_ui128IpHash, ui128Hash, 16);
}
//---------------------------------------------------------------------------

DcCommands::DcCommands() : m_pPasswdBfCheck(NULL), m_ui32StatChat(0), m_ui32StatCmdUnknown(0), m_ui32StatCmdTo(0), m_ui32StatCmdMyInfo(0), m_ui32StatCmdSearch(0), m_ui32StatCmdSR(0), m_ui32StatCmdRevCTM(0), m_ui32StatCmdOpForceMove(0), m_ui32StatCmdMyPass(0), 
	m_ui32StatCmdValidate(0), m_ui32StatCmdKey(0), m_ui32StatCmdGetInfo(0), m_ui32StatCmdGetNickList(0), m_ui32StatCmdConnectToMe(0), m_ui32StatCmdVersion(0), m_ui32StatCmdKick(0), m_ui32StatCmdSupports(0), m_ui32StatBotINFO(0), m_ui32StatZPipe(0), m_ui32StatCmdMultiSearch(0),
	m_ui32StatCmdMultiConnectToMe(0), m_ui32StatCmdClose(0) {
	// ...
}
//---------------------------------------------------------------------------

DcCommands::~DcCommands() {
	PassBf * cur = NULL,
        * next = m_pPasswdBfCheck;

    while(next != NULL) {
        cur = next;
		next = cur->m_pNext;

		delete cur;
    }

	m_pPasswdBfCheck = NULL;
}
//---------------------------------------------------------------------------

// Process DC data form User
void DcCommands::PreProcessData(DcCommand * pDcCommand) {
#ifdef _BUILD_GUI
    // Full raw data trace for better logging
    if(::SendMessage(MainWindowPageUsersChat::m_Ptr->m_hWndPageItems[MainWindowPageUsersChat::BTN_SHOW_COMMANDS], BM_GETCHECK, 0, 0) == BST_CHECKED) {
		int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s (%s) > ", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);
		if(iMsgLen > 0) {
			RichEditAppendText(MainWindowPageUsersChat::m_Ptr->m_hWndPageItems[MainWindowPageUsersChat::REDT_CHAT], (ServerManager::m_pGlobalBuffer+string(pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen)).c_str());
		}
    }
#endif

    // micro spam
    if(pDcCommand->m_ui32CommandLen < 5) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Garbage DATA from %s (%s) -> %s", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

		pDcCommand->m_pUser->Close();
        return;
    }

    static const uint32_t ui32ulti = *((uint32_t *)"ulti");
    static const uint32_t ui32ick = *((uint32_t *)"ick ");

    switch(pDcCommand->m_pUser->m_ui8State) {
    	case User::STATE_SOCKET_ACCEPTED:
            if(pDcCommand->m_sCommand[0] == '$') {
                if(memcmp(pDcCommand->m_sCommand+1, "MyNick ", 7) == 0) {
                    MyNick(pDcCommand);
                    return;
                }
            }
            break;
        case User::STATE_KEY_OR_SUP:
            if(pDcCommand->m_sCommand[0] == '$') {
                if(memcmp(pDcCommand->m_sCommand+1, "Supports ", 9) == 0) {
					m_ui32StatCmdSupports++;
                    Supports(pDcCommand);
                    return;
                } else if(*((uint32_t *)(pDcCommand->m_sCommand+1)) == *((uint32_t *)"Key ")) {
					m_ui32StatCmdKey++;
                    Key(pDcCommand);
                    return;
                } else if(memcmp(pDcCommand->m_sCommand+1, "MyNick ", 7) == 0) {
                    MyNick(pDcCommand);
                    return;
                }
            }
            break;
        case User::STATE_VALIDATE: {
            if(pDcCommand->m_sCommand[0] == '$') {
                switch(pDcCommand->m_sCommand[1]) {
                    case 'K':
                        if(memcmp(pDcCommand->m_sCommand+2, "ey ", 3) == 0) {
							m_ui32StatCmdKey++;
                            if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HAVE_SUPPORTS) == User::BIT_HAVE_SUPPORTS) == false) {
                                Key(pDcCommand);
                            } else {
								pDcCommand->m_pUser->FreeBuffer();
                            }

                            return;
                        }
                        break;
                    case 'V':
                        if(memcmp(pDcCommand->m_sCommand+2, "alidateNick ", 12) == 0) {
							m_ui32StatCmdValidate++;
                            ValidateNick(pDcCommand);
                            return;
                        }
                        break;
                    case 'M':
                        if(memcmp(pDcCommand->m_sCommand+2, "yINFO $ALL ", 11) == 0) {
							m_ui32StatCmdMyInfo++;
                            if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false) {
                                // bad state
								UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $MyINFO %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

								pDcCommand->m_pUser->Close();
                                return;
                            }
                            
                            if(MyINFODeflood(pDcCommand) == false) {
                                return;
                            }
                            
                            // PPK [ Strikes back ;) ] ... get nick from MyINFO
                            char *cTemp;
                            if((cTemp = strchr(pDcCommand->m_sCommand+13, ' ')) == NULL) {
								UdpDebug::m_Ptr->BroadcastFormat("[SYS] Attempt to validate empty nick  from %s (%s) - user closed. (QuickList -> %s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

								pDcCommand->m_pUser->Close();
                                return;
                            }
                            // PPK ... one null please :)
                            cTemp[0] = '\0';
                            
                            if(ValidateUserNick(pDcCommand->m_pUser, pDcCommand->m_sCommand+13, (cTemp-pDcCommand->m_sCommand)-13, false) == false) return;
                            
                            cTemp[0] = ' ';

                            // 1st time MyINFO, user is being added to nicklist
                            if(MyINFO(pDcCommand) == false || (pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_WAITING_FOR_PASS) == User::BIT_WAITING_FOR_PASS ||
                                ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == true)
                                return;

							pDcCommand->m_pUser->AddMeOrIPv4Check();

                            return;
                        }
                        break;
                    case 'G':
                        if(pDcCommand->m_ui32CommandLen == 13 && memcmp(pDcCommand->m_sCommand+2, "etNickList", 10) == 0) {
							m_ui32StatCmdGetNickList++;
                            if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false &&
                                ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false) {
                                // bad state
								UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $GetNickList %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

								pDcCommand->m_pUser->Close();
                                return;
                            }
                            GetNickList(pDcCommand);
                            return;
                        }
                        break;
                    default:
                        break;
                }                                            
            }
            break;
        }
        case User::STATE_VERSION_OR_MYPASS: {
            if(pDcCommand->m_sCommand[0] == '$') {
                switch(pDcCommand->m_sCommand[1]) {
                    case 'V':
                        if(memcmp(pDcCommand->m_sCommand+2, "ersion ", 7) == 0) {
							m_ui32StatCmdVersion++;
                            Version(pDcCommand);
                            return;
                        }
                        break;
                    case 'G':
                        if(pDcCommand->m_ui32CommandLen == 13 && memcmp(pDcCommand->m_sCommand+2, "etNickList", 10) == 0) {
							m_ui32StatCmdGetNickList++;
                            if(GetNickList(pDcCommand) == true && ((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false) {
								pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_GETNICKLIST;
                            }
                            return;
                        }
                        break;
                    case 'M':
                        if(pDcCommand->m_sCommand[2] == 'y') {
                            if(memcmp(pDcCommand->m_sCommand+3, "INFO $ALL ", 10) == 0) {
								m_ui32StatCmdMyInfo++;
                                if(MyINFODeflood(pDcCommand) == false) {
                                    return;
                                }
                                
                                // Am I sending MyINFO of someone other ?
                                // OR i try to fuck up hub with some chars after my nick ??? ... PPK
                                if((pDcCommand->m_sCommand[13+pDcCommand->m_pUser->m_ui8NickLen] != ' ') || (memcmp(pDcCommand->m_pUser->m_sNick, pDcCommand->m_sCommand+13, pDcCommand->m_pUser->m_ui8NickLen) != 0)) {
									UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in myinfo from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

									pDcCommand->m_pUser->Close();
                                    return;
                                }
        
                                if(MyINFO(pDcCommand) == false || (pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_WAITING_FOR_PASS) == User::BIT_WAITING_FOR_PASS || ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == true) {
                                    return;
								}
                                
								pDcCommand->m_pUser->AddMeOrIPv4Check();

                                return;
                            } else if(memcmp(pDcCommand->m_sCommand+3, "Pass ", 5) == 0) {
								m_ui32StatCmdMyPass++;
                                MyPass(pDcCommand);
                                return;
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
            break;
        }
        case User::STATE_GETNICKLIST_OR_MYINFO: {
            if(pDcCommand->m_sCommand[0] == '$') {
                if(pDcCommand->m_ui32CommandLen == 13 && memcmp(pDcCommand->m_sCommand+1, "GetNickList", 11) == 0) {
					m_ui32StatCmdGetNickList++;
                    if(GetNickList(pDcCommand) == true) {
						pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_GETNICKLIST;
                    }
                    return;
                } else if(memcmp(pDcCommand->m_sCommand+1, "MyINFO $ALL ", 12) == 0) {
					m_ui32StatCmdMyInfo++;
                    if(MyINFODeflood(pDcCommand) == false) {
                        return;
                    }
                                
                    // Am I sending MyINFO of someone other ?
                    // OR i try to fuck up hub with some chars after my nick ??? ... PPK
                    if((pDcCommand->m_sCommand[13+pDcCommand->m_pUser->m_ui8NickLen] != ' ') || (memcmp(pDcCommand->m_pUser->m_sNick, pDcCommand->m_sCommand+13, pDcCommand->m_pUser->m_ui8NickLen) != 0)) {
						UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in myinfo from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

						pDcCommand->m_pUser->Close();
                        return;
                    }
        
                    if(MyINFO(pDcCommand) == false || (pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_WAITING_FOR_PASS) == User::BIT_WAITING_FOR_PASS ||
                        ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == true)
                        return;
                    
					pDcCommand->m_pUser->AddMeOrIPv4Check();

                    return;
                }
            }
            break;
        }
        case User::STATE_IPV4_CHECK:
        case User::STATE_ADDME:
        case User::STATE_ADDME_1LOOP: {
            if(pDcCommand->m_sCommand[0] == '$') {
                switch(pDcCommand->m_sCommand[1]) {
                    case 'G':
                        if(pDcCommand->m_ui32CommandLen == 13 && memcmp(pDcCommand->m_sCommand+2, "etNickList", 10) == 0) {
							m_ui32StatCmdGetNickList++;
                            if(GetNickList(pDcCommand) == true) {
								pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_GETNICKLIST;
                            }
                            return;
                        }
                        break;
                    case 'M': {
                        if(memcmp(pDcCommand->m_sCommand+2, "yINFO $ALL ", 11) == 0) {
							m_ui32StatCmdMyInfo++;
                            if(MyINFODeflood(pDcCommand) == false) {
                                return;
                            }

                            // Am I sending MyINFO of someone other ?
                            // OR i try to fuck up hub with some chars after my nick ??? ... PPK
                            if((pDcCommand->m_sCommand[13+pDcCommand->m_pUser->m_ui8NickLen] != ' ') || (memcmp(pDcCommand->m_pUser->m_sNick, pDcCommand->m_sCommand+13, pDcCommand->m_pUser->m_ui8NickLen) != 0)) {
								UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in myinfo from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

								pDcCommand->m_pUser->Close();
                                return;
                            }

                            MyINFO(pDcCommand);
                            
                            return;
                        } else if(memcmp(pDcCommand->m_sCommand+2, "ultiSearch ", 11) == 0) {
							m_ui32StatCmdMultiSearch++;
                            SearchDeflood(pDcCommand, true);
                            return;
                        }
                        break;
                    }
                    case 'S':
                        if(memcmp(pDcCommand->m_sCommand+2, "earch ", 6) == 0) {
							m_ui32StatCmdSearch++;
                            SearchDeflood(pDcCommand, false);
                            return;
                        }
                        break;
                    default:
                        break;
                }
            } else if(pDcCommand->m_sCommand[0] == '<') {
				m_ui32StatChat++;
                ChatDeflood(pDcCommand);
                return;
            }
            break;
        }
        case User::STATE_ADDED: {
            if(pDcCommand->m_sCommand[0] == '$') {
                switch(pDcCommand->m_sCommand[1]) {
                    case 'S': {
                        if(memcmp(pDcCommand->m_sCommand+2, "earch ", 6) == 0) {
							m_ui32StatCmdSearch++;
                            if(SearchDeflood(pDcCommand, false) == true) {
                                Search(pDcCommand, false);
                            }
                            return;
                        } else if(*((uint16_t *)(pDcCommand->m_sCommand+2)) == *((uint16_t *)"R ")) {
							m_ui32StatCmdSR++;
                            SR(pDcCommand);
                            return;
                        }
                        break;
                    }
                    case 'C':
                        if(memcmp(pDcCommand->m_sCommand+2, "onnectToMe ", 11) == 0) {
							m_ui32StatCmdConnectToMe++;
                            ConnectToMe(pDcCommand, false);
                            return;
                        } else if(memcmp(pDcCommand->m_sCommand+2, "lose ", 5) == 0) {
							m_ui32StatCmdClose++;
                            Close(pDcCommand);
                            return;
                        }
                        break;
                    case 'R':
                        if(memcmp(pDcCommand->m_sCommand+2, "evConnectToMe ", 14) == 0) {
							m_ui32StatCmdRevCTM++;
                            RevConnectToMe(pDcCommand);
                            return;
                        }
                        break;
                    case 'M':
                        if(memcmp(pDcCommand->m_sCommand+2, "yINFO $ALL ", 11) == 0) {
							m_ui32StatCmdMyInfo++;
                            if(MyINFODeflood(pDcCommand) == false) {
                                return;
                            }
                                    
                            // Am I sending MyINFO of someone other ?
                            // OR i try to fuck up hub with some chars after my nick ??? ... PPK
                            if((pDcCommand->m_sCommand[13+pDcCommand->m_pUser->m_ui8NickLen] != ' ') || (memcmp(pDcCommand->m_pUser->m_sNick, pDcCommand->m_sCommand+13, pDcCommand->m_pUser->m_ui8NickLen) != 0)) {
								UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in myinfo from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

								pDcCommand->m_pUser->Close();
                                return;
                            }
                                                  
                            if(MyINFO(pDcCommand) == true) {
								pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_PRCSD_MYINFO;
                            }
                            return;
                        } else if(*((uint32_t *)(pDcCommand->m_sCommand+2)) == ui32ulti) {
                            if(memcmp(pDcCommand->m_sCommand+6, "Search ", 7) == 0) {
								m_ui32StatCmdMultiSearch++;
                                if(SearchDeflood(pDcCommand, true) == true) {
                                    Search(pDcCommand, true);
                                }
                                return;
                            } else if(memcmp(pDcCommand->m_sCommand+6, "ConnectToMe ", 12) == 0) {
								m_ui32StatCmdMultiConnectToMe++;
                                ConnectToMe(pDcCommand, true);
                                return;
                            }
                        } else if(memcmp(pDcCommand->m_sCommand+2, "yPass ", 6) == 0) {
							m_ui32StatCmdMyPass++;
                            //MyPass(pUser, sData, ui32Len);
                            if((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_WAITING_FOR_PASS) == User::BIT_WAITING_FOR_PASS) {
								pDcCommand->m_pUser->m_ui32BoolBits &= ~User::BIT_WAITING_FOR_PASS;

                                if(pDcCommand->m_pUser->m_pLogInOut != NULL && pDcCommand->m_pUser->m_pLogInOut->m_pBuffer != NULL) {
                                    int iProfile = ProfileManager::m_Ptr->GetProfileIndex(pDcCommand->m_pUser->m_pLogInOut->m_pBuffer);
                                    if(iProfile == -1) {
										pDcCommand->m_pUser->SendFormat("DcCommands::PreProcessData::MyPass->RegUser1", true, "<%s> %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_ERR_NO_PROFILE_GIVEN_NAME_EXIST]);

                                        delete pDcCommand->m_pUser->m_pLogInOut;
										pDcCommand->m_pUser->m_pLogInOut = NULL;

                                        return;
                                    }

                                    if(pDcCommand->m_ui32CommandLen < 10) {
										pDcCommand->m_pUser->SendFormat("DcCommands::PreProcessData::MyPass->RegUser2", true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_PASS_MUST_SPECIFIED]);

										pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_WAITING_FOR_PASS;
                                        return;
                                    }

                                    if(pDcCommand->m_ui32CommandLen > 73) {
										pDcCommand->m_pUser->SendFormat("DcCommands::PreProcessData::MyPass->RegUser2-1", true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_PASS_LEN_64_CHARS]);

										pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_WAITING_FOR_PASS;
                                        return;
                                    }

									pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '\0'; // cutoff pipe

                                    if(RegManager::m_Ptr->AddNew(pDcCommand->m_pUser->m_sNick, pDcCommand->m_sCommand+8, (uint16_t)iProfile) == false) {
										pDcCommand->m_pUser->SendFormat("DcCommands::PreProcessData::MyPass->RegUser3", true, "<%s> %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SORRY_YOU_ARE_ALREADY_REGISTERED]);
                                    } else {
										pDcCommand->m_pUser->SendFormat("DcCommands::PreProcessData::MyPass->RegUser4", true, "<%s> %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_THANK_YOU_FOR_PASSWORD_YOU_ARE_NOW_REGISTERED_AS], 
											pDcCommand->m_pUser->m_pLogInOut->m_pBuffer);
                                    }

                                    delete pDcCommand->m_pUser->m_pLogInOut;
									pDcCommand->m_pUser->m_pLogInOut = NULL;

									pDcCommand->m_pUser->m_i32Profile = iProfile;

                                    if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
                                        if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::HASKEYICON) == false) {
                                            return;
                                        }
                                        
										pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_OPERATOR;

                                        Users::m_Ptr->Add2OpList(pDcCommand->m_pUser);
                                        GlobalDataQueue::m_Ptr->OpListStore(pDcCommand->m_pUser->m_sNick);

                                        if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::ALLOWEDOPCHAT) == true) {
                                            if(SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true &&
                                                (SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == false || SettingManager::m_Ptr->m_bBotsSameNick == false)) {
                                                if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false) {
													pDcCommand->m_pUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_OP_CHAT_HELLO],
                                                        SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_OP_CHAT_HELLO]);
                                                }

												pDcCommand->m_pUser->SendCharDelayed(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_OP_CHAT_MYINFO], SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_OP_CHAT_MYINFO]);
												pDcCommand->m_pUser->SendFormat("DcCommands::PreProcessData::MyPass->RegUser5", true, "$OpList %s$$|", SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK]);
                                            }
                                        }
                                    }
                                }

                                return;
                            }
                        }
                        break;
                    case 'G': {
                        if(pDcCommand->m_ui32CommandLen == 13 && memcmp(pDcCommand->m_sCommand+2, "etNickList", 10) == 0) {
							m_ui32StatCmdGetNickList++;
                            if(GetNickList(pDcCommand) == true) {
								pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_GETNICKLIST;
                            }
                            return;
                        } else if(memcmp(pDcCommand->m_sCommand+2, "etINFO ", 7) == 0) {
							m_ui32StatCmdGetInfo++;
                            GetINFO(pDcCommand);
                            return;
                        }
                        break;
                    }
                    case 'T':
                        if(memcmp(pDcCommand->m_sCommand+2, "o: ", 3) == 0) {
							m_ui32StatCmdTo++;
                            To(pDcCommand);
                            return;
                        }
                    case 'K':
                        if(*((uint32_t *)(pDcCommand->m_sCommand+2)) == ui32ick) {
							m_ui32StatCmdKick++;
                            Kick(pDcCommand);
                            return;
                        }
                        break;
                    case 'O':
                        if(memcmp(pDcCommand->m_sCommand+2, "pForceMove $Who:", 16) == 0) {
							m_ui32StatCmdOpForceMove++;
                            OpForceMove(pDcCommand);
                            return;
                        }
                    default:
                        break;
                }
            } else if(pDcCommand->m_sCommand[0] == '<') {
				m_ui32StatChat++;
                if(ChatDeflood(pDcCommand) == true) {
                    Chat(pDcCommand);
                }
                
                return;
            }
            break;
        }
        case User::STATE_CLOSING:
        case User::STATE_REMME:
            return;
    }
    
    // PPK ... fallback to full command identification and disconnect on bad state or unknown command not handled by script
    switch(pDcCommand->m_sCommand[0]) {
        case '$':
            switch(pDcCommand->m_sCommand[1]) {
                case 'B': {
					if(memcmp(pDcCommand->m_sCommand+2, "otINFO", 6) == 0) {
						m_ui32StatBotINFO++;
                        BotINFO(pDcCommand);
                        return;
                    }
                    break;
                }
                case 'C':
                    if(memcmp(pDcCommand->m_sCommand+2, "onnectToMe ", 11) == 0) {
						m_ui32StatCmdConnectToMe++;

						UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $ConnectToMe %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

						pDcCommand->m_pUser->Close();
                        return;
                    } else if(memcmp(pDcCommand->m_sCommand+2, "lose ", 5) == 0) {
						m_ui32StatCmdClose++;

						UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $Close %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

						pDcCommand->m_pUser->Close();
                        return;
                    }
                    break;
                case 'G': {
                    if(*((uint16_t *)(pDcCommand->m_sCommand+2)) == *((uint16_t *)"et")) {
                        if(memcmp(pDcCommand->m_sCommand+4, "INFO ", 5) == 0) {
							m_ui32StatCmdGetInfo++;

							UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $GetINFO %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

							pDcCommand->m_pUser->Close();
                            return;
                        } else if(pDcCommand->m_ui32CommandLen == 13 && *((uint64_t *)(pDcCommand->m_sCommand+4)) == *((uint64_t *)"NickList")) {
							m_ui32StatCmdGetNickList++;

							UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $GetNickList %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

							pDcCommand->m_pUser->Close();
                            return;
                        }
                    }
                    break;
                }
                case 'K':
                    if(memcmp(pDcCommand->m_sCommand+2, "ey ", 3) == 0) {
						m_ui32StatCmdKey++;

						UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $Key %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

						pDcCommand->m_pUser->Close();
                        return;
                    } else if(*((uint32_t *)(pDcCommand->m_sCommand+2)) == ui32ick) {
						m_ui32StatCmdKick++;

						UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $Kick %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

						pDcCommand->m_pUser->Close();
                        return;
                    }
                    break;
                case 'M':
                    if(*((uint32_t *)(pDcCommand->m_sCommand+2)) == ui32ulti) {
                        if(memcmp(pDcCommand->m_sCommand+6, "ConnectToMe ", 12) == 0) {
							m_ui32StatCmdMultiConnectToMe++;

							UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $MultiConnectToMe %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

							pDcCommand->m_pUser->Close();
                            return;
                        } else if(memcmp(pDcCommand->m_sCommand+6, "Search ", 7) == 0) {
							m_ui32StatCmdMultiSearch++;

							UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $MultiSearch %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

							pDcCommand->m_pUser->Close();
                            return;
                        }
                    } else if(pDcCommand->m_sCommand[2] == 'y') {
                        if(memcmp(pDcCommand->m_sCommand+3, "INFO $ALL ", 10) == 0) {
							m_ui32StatCmdMyInfo++;

							UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $MyINFO %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

							pDcCommand->m_pUser->Close();
                            return;
                        } else if(memcmp(pDcCommand->m_sCommand+3, "Pass ", 5) == 0) {
							m_ui32StatCmdMyPass++;

							UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $MyPass %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

							pDcCommand->m_pUser->Close();
                            return;
                        }
                    }
                    break;
                case 'O':
                    if(memcmp(pDcCommand->m_sCommand+2, "pForceMove $Who:", 16) == 0) {
						m_ui32StatCmdOpForceMove++;

						UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $OpForceMove %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

						pDcCommand->m_pUser->Close();
                        return;
                    }
                    break;
                case 'R':
                    if(memcmp(pDcCommand->m_sCommand+2, "evConnectToMe ", 14) == 0) {
						m_ui32StatCmdRevCTM++;

						UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $RevConnectToMe %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

						pDcCommand->m_pUser->Close();
                        return;
                    }
                    break;
                case 'S':
                    switch(pDcCommand->m_sCommand[2]) {
                        case 'e': {
                            if(memcmp(pDcCommand->m_sCommand+3, "arch ", 5) == 0) {
								m_ui32StatCmdSearch++;

								UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $Search %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

								pDcCommand->m_pUser->Close();
                                return;
                            }
                            break;
                        }
                        case 'R':
                            if(pDcCommand->m_sCommand[3] == ' ') {
								m_ui32StatCmdSR++;

								UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $SR %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

								pDcCommand->m_pUser->Close();
                                return;
                            }
                            break;
                        case 'u': {
                            if(memcmp(pDcCommand->m_sCommand+3, "pports ", 7) == 0) {
								m_ui32StatCmdSupports++;

								UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $Supports %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

								pDcCommand->m_pUser->Close();
                                return;
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                case 'T':
                    if(memcmp(pDcCommand->m_sCommand+2, "o: ", 3) == 0) {
						m_ui32StatCmdTo++;

						UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $To %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

						pDcCommand->m_pUser->Close();
                        return;
                    }
                    break;
                case 'V':
                    if(memcmp(pDcCommand->m_sCommand+2, "alidateNick ", 12) == 0) {
						m_ui32StatCmdValidate++;

						UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $ValidateNick %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

						pDcCommand->m_pUser->Close();
                        return;
                    } else if(memcmp(pDcCommand->m_sCommand+2, "ersion ", 7) == 0) {
						m_ui32StatCmdVersion++;

						UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in $Version %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

						pDcCommand->m_pUser->Close();
                        return;
                    }
                    break;
                default:
                    break;
            }
            break;
        case '<': {
			m_ui32StatChat++;

			UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad state (%hhu) in Chat %s (%s) - user closed.", pDcCommand->m_pUser->m_ui8State, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

			pDcCommand->m_pUser->Close();
            return;
        }
        default:
            break;
    }


    Unknown(pDcCommand);
}
//---------------------------------------------------------------------------

// $BotINFO pinger identification|
void DcCommands::BotINFO(DcCommand * pDcCommand) {
	if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false || ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HAVE_BOTINFO) == User::BIT_HAVE_BOTINFO) == true) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Not pinger $BotINFO or $BotINFO flood from %s (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return;
    }

    if(pDcCommand->m_ui32CommandLen < 9) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $BotINFO (%s) from %s (%s) - user closed.", pDcCommand->m_sCommand, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return;
    }

	pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_HAVE_BOTINFO;

	if(ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::BOTINFO_ARRIVAL) == true ||
		pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
		return;
	}

	pDcCommand->m_pUser->SendFormat("DcCommands::BotINFO", true, "$HubINFO %s$%s:%hu$%s.px.$%hd$%" PRIu64 "$%hd$%hd$PtokaX$%s|", SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_NAME], SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_ADDRESS], SettingManager::m_Ptr->m_ui16PortNumbers[0], 
		SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_DESCRIPTION] == NULL ? "" : SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_DESCRIPTION], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_USERS], SettingManager::m_Ptr->m_ui64MinShare, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SLOTS_LIMIT], 
		SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_HUBS_LIMIT], SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_OWNER_EMAIL] == NULL ? "" : SettingManager::m_Ptr->m_sTexts[SETTXT_HUB_OWNER_EMAIL]);

	if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HAVE_GETNICKLIST) == User::BIT_HAVE_GETNICKLIST) == true) {
		pDcCommand->m_pUser->Close();
    }
}
//---------------------------------------------------------------------------

// $ConnectToMe <nickname> <ownip>:<ownlistenport>
// $MultiConnectToMe <nick> <ownip:port> <hub[:port]>
void DcCommands::ConnectToMe(DcCommand * pDcCommand, const bool bMulti) {
    if((bMulti == false && pDcCommand->m_ui32CommandLen < 23) || (bMulti == true && pDcCommand->m_ui32CommandLen < 28)) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $%sConnectToMe (%s) from %s (%s) - user closed.", bMulti == false ? "" : "Multi", pDcCommand->m_sCommand, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return;
    }

    // PPK ... check flood ...
    if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODCTM) == false) { 
        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION] != 0) {
            if(DeFloodCheckForFlood(pDcCommand->m_pUser, DEFLOOD_CTM, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION],
				pDcCommand->m_pUser->m_ui16CTMs, pDcCommand->m_pUser->m_ui64CTMsTick, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_MESSAGES],
              (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_TIME]) == true) {
				return;
            }
        }

        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION2] != 0) {
            if(DeFloodCheckForFlood(pDcCommand->m_pUser, DEFLOOD_CTM, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_ACTION2],
				pDcCommand->m_pUser->m_ui16CTMs2, pDcCommand->m_pUser->m_ui64CTMsTick2, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_MESSAGES2],
			  (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CTM_TIME2]) == true) {
                return;
            }
        }
    }

    if(pDcCommand->m_ui32CommandLen > (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_CTM_LEN]) {
		pDcCommand->m_pUser->SendFormat("DcCommands::ConnectToMe", true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_CTM_TOO_LONG]);

		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Long $ConnectToMe from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

		pDcCommand->m_pUser->Close();
		return;
    }

    // PPK ... $CTM means user is active ?!? Probably yes, let set it active and use on another places ;)
    if(pDcCommand->m_pUser->m_sTag == NULL) {
		pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_IPV4_ACTIVE;
    }

    // full data only and allow blocking
	if(ScriptManager::m_Ptr->Arrival(pDcCommand, (uint8_t)(bMulti == false ? ScriptManager::CONNECTTOME_ARRIVAL : ScriptManager::MULTICONNECTTOME_ARRIVAL)) == true ||
		pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
		return;
	}

    char * pSpace = strchr(pDcCommand->m_sCommand+(bMulti == false ? 13 : 18), ' ');
    if(pSpace == NULL) {
        return;
    }

    pSpace[0] = '\0';

    User * pOtherUser = HashManager::m_Ptr->FindUser(pDcCommand->m_sCommand+(bMulti == false ? 13 : 18), pSpace-(pDcCommand->m_sCommand+(bMulti == false ? 13 : 18)));
    // PPK ... no connection to yourself !!!
    if(pOtherUser == NULL || pOtherUser == pDcCommand->m_pUser || pOtherUser->m_ui8State != User::STATE_ADDED) {
        return;
    }

    pSpace[0] = ' ';

    // IP check
    if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOIPCHECK) == false) {
    	// At end of sData is | and we need to remove it.
		pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '\0';

    	// Now between pSpace+1 and end of string is something that should be IP:Port .. let's check that and start with length.
    	uint32_t ui32IpPortLen = (uint32_t)((pDcCommand->m_ui32CommandLen-1) - ((pSpace+1) - pDcCommand->m_sCommand));

		// Check minimal (shortest ip:port can be [::1]:1) and maximal (longest ip:port can be [1234:1234:1234:1234:1234:1234:1234:1234]:12345S) length.
		if(ui32IpPortLen < 7 || ui32IpPortLen > 48) {
			pDcCommand->m_pUser->SendFormat("DcCommands::ConnectToMe bad IP:Port len", true, "<%s> %s '%s'!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOUR_CLIENT_SEND_INCORRECT_IPPORT_IN_COMMAND], pSpace+1);
	
			UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad IP:Port length in %sCTM from %s (%s). (%s)", bMulti == false ? "" : "M", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);
	
			pDcCommand->m_pUser->Close();
	        return;
		}
		
		// Check if Port is valid. Here can be S (as secure [what a joke lol]) to indicate TLS connection request after port ...
		uint32_t ui32PortLen = 0;
		uint16_t ui16Port = 0; // Zero == invalid port ...
		bool bSecure = false; // To restore secure status after checks

		if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_TLS2) == User::SUPPORTBIT_TLS2) == true && pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-2] == 'S') {
			// Remove S character and set secure to true
			pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-2] = '\0';
			bSecure = true;

			// Check for port validity and get Port when valid
			ui16Port = CheckAndGetPort(pSpace+1+(ui32IpPortLen-7), (uint8_t)(ui32IpPortLen-((pSpace+(ui32IpPortLen-6))-pSpace)), ui32PortLen);
		} else {
    		// Check for port validity and get Port when valid
			ui16Port = CheckAndGetPort(pSpace+1+(ui32IpPortLen-6), (uint8_t)(ui32IpPortLen-((pSpace+(ui32IpPortLen-6))-pSpace)), ui32PortLen);
		}

		// Check if we get valid port number
		if(ui16Port == 0) {
			pDcCommand->m_pUser->SendFormat("DcCommands::ConnectToMe invalid Port", true, "<%s> %s '%s'!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOUR_CLIENT_SEND_INCORRECT_PORT_IN_CTM], pSpace+1);
	
			UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad Port in %sCTM from %s (%s). (%s)", bMulti == false ? "" : "M", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);
	
			pDcCommand->m_pUser->Close();
	        return;
		}

		uint32_t ui32IpLen = (ui32IpPortLen - ui32PortLen) - 1;
		if(bSecure == true) {
			ui32IpLen--;
		}
	    
	    bool bInvalidIP = true;
	    char * pIP = pSpace+1;
	    
		// Check if we get valid IP address
		if((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
	        if((pDcCommand->m_pUser->m_ui8IpLen + 2U) == ui32IpLen && pIP[0] == '[' && pIP[1+pDcCommand->m_pUser->m_ui8IpLen] == ']' && strncmp(pIP+1, pDcCommand->m_pUser->m_sIP, pDcCommand->m_pUser->m_ui8IpLen) == 0) {
	        	bInvalidIP = false;
	        } else if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) && pDcCommand->m_pUser->m_ui8IPv4Len == ui32IpLen &&  strncmp(pIP, pDcCommand->m_pUser->m_sIPv4, pDcCommand->m_pUser->m_ui8IPv4Len) == 0) {
	        	bInvalidIP = false;
	        }
	    } else if(pDcCommand->m_pUser->m_ui8IpLen == ui32IpLen && strncmp(pIP, pDcCommand->m_pUser->m_sIP, pDcCommand->m_pUser->m_ui8IpLen) == 0) {
	    	bInvalidIP = false;
	    }

		if(bInvalidIP == true) {
			if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_WARNED_WRONG_IP) == User::BIT_WARNED_WRONG_IP) == false) {
				UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad Ip in %sCTM from %s (%s/%s). (%s)", bMulti == false ? "" : "M", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_pUser->m_sIPv4, pDcCommand->m_sCommand);
    		}

	        if((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6 && (pOtherUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
	            int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$ConnectToMe %s [%s]:%hu%s|", pOtherUser->m_sNick, pDcCommand->m_pUser->m_sIP, ui16Port, bSecure == true ? "S" : "");
	            if(iMsgLen > 0) {
					pDcCommand->m_pUser->AddPrcsdCmd(PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, ServerManager::m_pGlobalBuffer, iMsgLen, pOtherUser);
	            }
	        } else if((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4 && (pOtherUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) {
	            char * sIP = pDcCommand->m_pUser->m_ui8IPv4Len == 0 ? pDcCommand->m_pUser->m_sIP : pDcCommand->m_pUser->m_sIPv4;
	
	            int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$ConnectToMe %s %s:%hu%s|", pOtherUser->m_sNick, sIP, ui16Port, bSecure == true ? "S" : "");
	            if(iMsgLen > 0) {
					pDcCommand->m_pUser->AddPrcsdCmd(PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, ServerManager::m_pGlobalBuffer, iMsgLen, pOtherUser);
	            }
	        }

            if(ui32IpLen != 0 && pIP[ui32IpLen-1] == ']') {
            	pIP[ui32IpLen-1] = '\0';
			} else {
            	pIP[ui32IpLen] = '\0';
            }

			if(pIP[0] == '[') {
				pIP++;
			}

            SendIPFixedMsg(pDcCommand->m_pUser, pIP, pDcCommand->m_pUser->m_sIP);
            return;
		} else {
			if(bSecure == true) {
				pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-2] = 'S';
			}
			pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '|';
		}
    }

    if(bMulti == true) {
		pDcCommand->m_sCommand[5] = '$';
    }

	pDcCommand->m_pUser->AddPrcsdCmd(PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, bMulti == false ? pDcCommand->m_sCommand : pDcCommand->m_sCommand+5, bMulti == false ? pDcCommand->m_ui32CommandLen : pDcCommand->m_ui32CommandLen-5, pOtherUser);
}
//---------------------------------------------------------------------------

// $GetINFO <nickname> <ownnickname>
void DcCommands::GetINFO(DcCommand * pDcCommand) {
	if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_NOGETINFO) == User::SUPPORTBIT_NOGETINFO) == true || ((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == true || ((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == true) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Not allowed user %s (%s) send $GetINFO - user closed.", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return;
    }
    
    // PPK ... code change, added own nick and space on right place check
    if(pDcCommand->m_ui32CommandLen < (12u+pDcCommand->m_pUser->m_ui8NickLen) || pDcCommand->m_ui32CommandLen > (75u+pDcCommand->m_pUser->m_ui8NickLen) || pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-pDcCommand->m_pUser->m_ui8NickLen-2] != ' ' || 
		memcmp(pDcCommand->m_sCommand+(pDcCommand->m_ui32CommandLen-pDcCommand->m_pUser->m_ui8NickLen-1), pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_ui8NickLen) != 0)
	{
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $GetINFO from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

		pDcCommand->m_pUser->Close();
        return;
    }

	if(ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::GETINFO_ARRIVAL) == true ||
		pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
		return;
	}
}
//---------------------------------------------------------------------------

// $GetNickList
bool DcCommands::GetNickList(DcCommand * pDcCommand) {
    if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == true &&
        ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HAVE_GETNICKLIST) == User::BIT_HAVE_GETNICKLIST) == true) {
        // PPK ... refresh not allowed !
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $GetNickList request %s (%s) - user closed.", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return false;
    } else if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == true) {
		if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HAVE_GETNICKLIST) == User::BIT_HAVE_GETNICKLIST) == false) {
			pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_BIG_SEND_BUFFER;
            if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false && Users::m_Ptr->m_ui32NickListLen > 11) {
                if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
					pDcCommand->m_pUser->SendCharDelayed(Users::m_Ptr->m_pNickList, Users::m_Ptr->m_ui32NickListLen);
                } else {
                    if(Users::m_Ptr->m_ui32ZNickListLen == 0) {
                        Users::m_Ptr->m_pZNickList = ZlibUtility::m_Ptr->CreateZPipe(Users::m_Ptr->m_pNickList, Users::m_Ptr->m_ui32NickListLen, Users::m_Ptr->m_pZNickList,
                            Users::m_Ptr->m_ui32ZNickListLen, Users::m_Ptr->m_ui32ZNickListSize, Allign16K);
                        if(Users::m_Ptr->m_ui32ZNickListLen == 0) {
							pDcCommand->m_pUser->SendCharDelayed(Users::m_Ptr->m_pNickList, Users::m_Ptr->m_ui32NickListLen);
                        } else {
							pDcCommand->m_pUser->PutInSendBuf(Users::m_Ptr->m_pZNickList, Users::m_Ptr->m_ui32ZNickListLen);
                            ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32NickListLen-Users::m_Ptr->m_ui32ZNickListLen;
                        }
                    } else {
						pDcCommand->m_pUser->PutInSendBuf(Users::m_Ptr->m_pZNickList, Users::m_Ptr->m_ui32ZNickListLen);
                        ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32NickListLen-Users::m_Ptr->m_ui32ZNickListLen;
                    }
                }
            }
            
            if(SettingManager::m_Ptr->m_ui8FullMyINFOOption == 2) {
                if(Users::m_Ptr->m_ui32MyInfosLen != 0) {
                    if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
						pDcCommand->m_pUser->SendCharDelayed(Users::m_Ptr->m_pMyInfos, Users::m_Ptr->m_ui32MyInfosLen);
                    } else {
                        if(Users::m_Ptr->m_ui32ZMyInfosLen == 0) {
                            Users::m_Ptr->m_pZMyInfos = ZlibUtility::m_Ptr->CreateZPipe(Users::m_Ptr->m_pMyInfos, Users::m_Ptr->m_ui32MyInfosLen, Users::m_Ptr->m_pZMyInfos,
                                Users::m_Ptr->m_ui32ZMyInfosLen, Users::m_Ptr->m_ui32ZMyInfosSize, Allign128K);
                            if(Users::m_Ptr->m_ui32ZMyInfosLen == 0) {
								pDcCommand->m_pUser->SendCharDelayed(Users::m_Ptr->m_pMyInfos, Users::m_Ptr->m_ui32MyInfosLen);
                            } else {
								pDcCommand->m_pUser->PutInSendBuf(Users::m_Ptr->m_pZMyInfos, Users::m_Ptr->m_ui32ZMyInfosLen);
                                ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32MyInfosLen-Users::m_Ptr->m_ui32ZMyInfosLen;
                            }
                        } else {
							pDcCommand->m_pUser->PutInSendBuf(Users::m_Ptr->m_pZMyInfos, Users::m_Ptr->m_ui32ZMyInfosLen);
                            ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32MyInfosLen-Users::m_Ptr->m_ui32ZMyInfosLen;
                        }
                    }
                }
            } else if(Users::m_Ptr->m_ui32MyInfosTagLen != 0) {
                if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
					pDcCommand->m_pUser->SendCharDelayed(Users::m_Ptr->m_pMyInfosTag, Users::m_Ptr->m_ui32MyInfosTagLen);
                } else {
                    if(Users::m_Ptr->m_ui32ZMyInfosTagLen == 0) {
                        Users::m_Ptr->m_pZMyInfosTag = ZlibUtility::m_Ptr->CreateZPipe(Users::m_Ptr->m_pMyInfosTag, Users::m_Ptr->m_ui32MyInfosTagLen, Users::m_Ptr->m_pZMyInfosTag,
                            Users::m_Ptr->m_ui32ZMyInfosTagLen, Users::m_Ptr->m_ui32ZMyInfosTagSize, Allign128K);
                        if(Users::m_Ptr->m_ui32ZMyInfosTagLen == 0) {
							pDcCommand->m_pUser->SendCharDelayed(Users::m_Ptr->m_pMyInfosTag, Users::m_Ptr->m_ui32MyInfosTagLen);
                        } else {
							pDcCommand->m_pUser->PutInSendBuf(Users::m_Ptr->m_pZMyInfosTag, Users::m_Ptr->m_ui32ZMyInfosTagLen);
                            ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32MyInfosTagLen-Users::m_Ptr->m_ui32ZMyInfosTagLen;
                        }
                    } else {
						pDcCommand->m_pUser->PutInSendBuf(Users::m_Ptr->m_pZMyInfosTag, Users::m_Ptr->m_ui32ZMyInfosTagLen);
                        ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32MyInfosTagLen-Users::m_Ptr->m_ui32ZMyInfosTagLen;
                    }  
                }
            }
            
 			if(Users::m_Ptr->m_ui32OpListLen > 9) {
                if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
					pDcCommand->m_pUser->SendCharDelayed(Users::m_Ptr->m_pOpList, Users::m_Ptr->m_ui32OpListLen);
                } else {
                    if(Users::m_Ptr->m_ui32ZOpListLen == 0) {
                        Users::m_Ptr->m_pZOpList = ZlibUtility::m_Ptr->CreateZPipe(Users::m_Ptr->m_pOpList, Users::m_Ptr->m_ui32OpListLen, Users::m_Ptr->m_pZOpList,
                            Users::m_Ptr->m_ui32ZOpListLen, Users::m_Ptr->m_ui32ZOpListSize, Allign16K);
                        if(Users::m_Ptr->m_ui32ZOpListLen == 0) {
							pDcCommand->m_pUser->SendCharDelayed(Users::m_Ptr->m_pOpList, Users::m_Ptr->m_ui32OpListLen);
                        } else {
							pDcCommand->m_pUser->PutInSendBuf(Users::m_Ptr->m_pZOpList, Users::m_Ptr->m_ui32ZOpListLen);
                            ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32OpListLen-Users::m_Ptr->m_ui32ZOpListLen;
                        }
                    } else {
						pDcCommand->m_pUser->PutInSendBuf(Users::m_Ptr->m_pZOpList, Users::m_Ptr->m_ui32ZOpListLen);
                        ServerManager::m_ui64BytesSentSaved += Users::m_Ptr->m_ui32OpListLen-Users::m_Ptr->m_ui32ZOpListLen;
                    }
                }
            }
            
 			if(pDcCommand->m_pUser->m_ui32SendBufDataLen != 0) {
				pDcCommand->m_pUser->Try2Send();
            }
 			
			pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_HAVE_GETNICKLIST;
  			
   			if(SettingManager::m_Ptr->m_bBools[SETBOOL_REPORT_PINGERS] == true) {
                GlobalDataQueue::m_Ptr->StatusMessageFormat("DcCommands::GetNickList", "<%s> *** %s: %s %s: %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_PINGER_FROM_IP], pDcCommand->m_pUser->m_sIP, 
					LanguageManager::m_Ptr->m_sTexts[LAN_WITH_NICK], pDcCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_DETECTED_LWR]);
			}

			if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HAVE_BOTINFO) == User::BIT_HAVE_BOTINFO) == true) {
				pDcCommand->m_pUser->Close();
            }
			return false;
		} else {
			UdpDebug::m_Ptr->BroadcastFormat("[SYS] $GetNickList flood from pinger %s (%s) - user closed.", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

			pDcCommand->m_pUser->Close();
			return false;
		}
	}

	pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_HAVE_GETNICKLIST;
    
     // PPK ... check flood...
    if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODGETNICKLIST) == false && 
      SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GETNICKLIST_ACTION] != 0) {
        if(DeFloodCheckForFlood(pDcCommand->m_pUser, DEFLOOD_GETNICKLIST, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GETNICKLIST_ACTION],
			pDcCommand->m_pUser->m_ui16GetNickLists, pDcCommand->m_pUser->m_ui64GetNickListsTick, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GETNICKLIST_MESSAGES],
          ((uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_GETNICKLIST_TIME])*60) == true) {
            return false;
        }
    }

	if(ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::GETNICKLIST_ARRIVAL) == true ||
		pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING ||
		((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == true) {
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------

// $Key
void DcCommands::Key(DcCommand * pDcCommand) {
    if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HAVE_KEY) == User::BIT_HAVE_KEY) == true) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] $Key flood from %s (%s) - user closed.", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return;
    }
    
	pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_HAVE_KEY;

	pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '\0'; // cutoff pipe

    if(pDcCommand->m_ui32CommandLen < 6 || strcmp(Lock2Key(pDcCommand->m_pUser->m_pLogInOut->m_pBuffer), pDcCommand->m_sCommand+5) != 0) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $Key from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

		pDcCommand->m_pUser->Close();
        return;
    }

	pDcCommand->m_pUser->FreeBuffer();

	pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '|'; // add back pipe

	ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::KEY_ARRIVAL);

	if(pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
		return;
	}

	pDcCommand->m_pUser->m_ui8State = User::STATE_VALIDATE;
}
//---------------------------------------------------------------------------

// $Kick <name>
void DcCommands::Kick(DcCommand * pDcCommand) {
    if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::KICK) == false) {
		pDcCommand->m_pUser->SendFormat("DcCommands::Kick1", true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD]);
        return;
    } 

    if(pDcCommand->m_ui32CommandLen < 8) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $Kick (%s) from %s (%s) - user closed.", pDcCommand->m_sCommand, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return;
    }

	if(ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::KICK_ARRIVAL) == true || pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
		return;
	}

	pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '\0'; // cutoff pipe

    User *OtherUser = HashManager::m_Ptr->FindUser(pDcCommand->m_sCommand+6, pDcCommand->m_ui32CommandLen-7);
    if(OtherUser != NULL) {
        // Self-kick
        if(OtherUser == pDcCommand->m_pUser) {
			pDcCommand->m_pUser->SendFormat("DcCommands::Kick2", true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_CANT_KICK_YOURSELF]);
            return;
    	}
    	
        if(OtherUser->m_i32Profile != -1 && pDcCommand->m_pUser->m_i32Profile > OtherUser->m_i32Profile) {
			pDcCommand->m_pUser->SendFormat("DcCommands::Kick3", true, "<%s> %s %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_NOT_ALLOWED_TO_KICK], OtherUser->m_sNick);
        	return;
        }

        if(pDcCommand->m_pUser->m_pCmdToUserStrt != NULL) {
            PrcsdToUsrCmd * cur = NULL, * prev = NULL,
                * next = pDcCommand->m_pUser->m_pCmdToUserStrt;

            while(next != NULL) {
                cur = next;
                next = cur->m_pNext;
                                       
                if(OtherUser == cur->m_pToUser) {
                    cur->m_pToUser->SendChar(cur->m_sCommand, cur->m_ui32Len);

                    if(prev == NULL) {
                        if(cur->m_pNext == NULL) {
							pDcCommand->m_pUser->m_pCmdToUserStrt = NULL;
							pDcCommand->m_pUser->m_pCmdToUserEnd = NULL;
                        } else {
							pDcCommand->m_pUser->m_pCmdToUserStrt = cur->m_pNext;
                        }
                    } else if(cur->m_pNext == NULL) {
                        prev->m_pNext = NULL;
						pDcCommand->m_pUser->m_pCmdToUserEnd = prev;
                    } else {
                        prev->m_pNext = cur->m_pNext;
                    }

#ifdef _WIN32
					if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->m_sCommand) == 0) {
						AppendDebugLog("%s - [MEM] Cannot deallocate cur->m_sCommand in DcCommands::Kick\n");
                    }
#else
					free(cur->m_sCommand);
#endif
                    cur->m_sCommand = NULL;

#ifdef _WIN32
                    if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->m_sToNick) == 0) {
						AppendDebugLog("%s - [MEM] Cannot deallocate cur->m_sToNick in DcCommands::Kick\n");
                    }
#else
					free(cur->m_sToNick);
#endif
                    cur->m_sToNick = NULL;

					delete cur;
                    break;
                }
                prev = cur;
            }
        }

        char * sBanTime;
		if(OtherUser->m_pLogInOut != NULL && OtherUser->m_pLogInOut->m_pBuffer != NULL &&
			(sBanTime = stristr(OtherUser->m_pLogInOut->m_pBuffer, "_BAN_")) != NULL) {
			sBanTime[0] = '\0';

			if(sBanTime[5] == '\0' || sBanTime[5] == ' ') { // permban
                if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::BAN) == false) {
					pDcCommand->m_pUser->SendFormat("DcCommands::Kick4", true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD]);
                    return;
                }

				BanManager::m_Ptr->Ban(OtherUser, OtherUser->m_pLogInOut->m_pBuffer, pDcCommand->m_pUser->m_sNick, false);
    
                GlobalDataQueue::m_Ptr->StatusMessageFormat("DcCommands::Kick1", "<%s> *** %s %s %s %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], OtherUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], OtherUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_BEEN], 
					LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_BY_LWR], pDcCommand->m_pUser->m_sNick);

                if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
					pDcCommand->m_pUser->SendFormat("DcCommands::Kick5", true, "<%s> *** %s %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], OtherUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], OtherUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_BEEN], 
						LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_LWR]);
                }

        		// disconnect the user
				UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) kicked by %s", OtherUser->m_sNick, OtherUser->m_sIP, pDcCommand->m_pUser->m_sNick);

				OtherUser->Close();

                return;
			} else if(isdigit(sBanTime[5]) != 0) { // tempban
                if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::TEMP_BAN) == false) {
					pDcCommand->m_pUser->SendFormat("DcCommands::Kick6", true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD]);
                    return;
                }

				uint32_t i = 6;
				while(sBanTime[i] != '\0' && isdigit(sBanTime[i]) != 0) {
                	i++;
                }

                char cTime = sBanTime[i];
                sBanTime[i] = '\0';
                int iTime = atoi(sBanTime+5);
				time_t acc_time, ban_time;

				if(cTime != '\0' && iTime > 0 && GenerateTempBanTime(cTime, iTime, acc_time, ban_time) == true) {
					BanManager::m_Ptr->TempBan(OtherUser, OtherUser->m_pLogInOut->m_pBuffer, pDcCommand->m_pUser->m_sNick, 0, ban_time, false);

                    static char sTime[256];
                    strcpy(sTime, formatTime((ban_time-acc_time)/60));

                    GlobalDataQueue::m_Ptr->StatusMessageFormat("DcCommands::Kick2", "<%s> *** %s %s %s %s %s %s %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], OtherUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], OtherUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_BEEN], 
						LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BANNED], LanguageManager::m_Ptr->m_sTexts[LAN_BY_LWR], pDcCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_TO_LWR], sTime);
                
                    if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
						pDcCommand->m_pUser->SendFormat("DcCommands::Kick7", true, "<%s> *** %s %s %s %s %s %s: %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], OtherUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], OtherUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_HAS_BEEN], 
							LanguageManager::m_Ptr->m_sTexts[LAN_TEMP_BANNED], LanguageManager::m_Ptr->m_sTexts[LAN_TO_LWR], sTime);
                	}
    
                    // disconnect the user
					UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) kicked by %s", OtherUser->m_sNick, OtherUser->m_sIP, pDcCommand->m_pUser->m_sNick);

					OtherUser->Close();

					return;
                }
            }
		}

        BanManager::m_Ptr->TempBan(OtherUser, OtherUser->m_pLogInOut != NULL ? OtherUser->m_pLogInOut->m_pBuffer : NULL, pDcCommand->m_pUser->m_sNick, 0, 0, false);

        GlobalDataQueue::m_Ptr->StatusMessageFormat("DcCommands::Kick3", "<%s> *** %s %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], OtherUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], OtherUser->m_sIP, 
			LanguageManager::m_Ptr->m_sTexts[LAN_WAS_KICKED_BY], pDcCommand->m_pUser->m_sNick);

        if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
			pDcCommand->m_pUser->SendFormat("DcCommands::Kick8", true, "<%s> *** %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], OtherUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], OtherUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_WAS_KICKED]);
        }

        // disconnect the user
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) kicked by %s", OtherUser->m_sNick, OtherUser->m_sIP, pDcCommand->m_pUser->m_sNick);

        OtherUser->Close();
    }
}
//---------------------------------------------------------------------------

// $Search $MultiSearch
bool DcCommands::SearchDeflood(DcCommand * pDcCommand, const bool bMulti) {
    // search flood protection ... modified by PPK ;-)
    if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODSEARCH) == false) {
        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION] != 0) {
            if(DeFloodCheckForFlood(pDcCommand->m_pUser, DEFLOOD_SEARCH, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION],
				pDcCommand->m_pUser->m_ui16Searchs, pDcCommand->m_pUser->m_ui64SearchsTick, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_MESSAGES],
              (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_TIME]) == true) {
                return false;
            }
        }

        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION2] != 0) {
            if(DeFloodCheckForFlood(pDcCommand->m_pUser, DEFLOOD_SEARCH, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_ACTION2],
				pDcCommand->m_pUser->m_ui16Searchs2, pDcCommand->m_pUser->m_ui64SearchsTick2, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_MESSAGES2],
              (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_TIME2]) == true) {
                return false;
            }
        }

        // 2nd check for same search flood
		if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_SEARCH_ACTION] != 0) {
			bool bNewData = false;
            if(DeFloodCheckForSameFlood(pDcCommand->m_pUser, DEFLOOD_SAME_SEARCH, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_SEARCH_ACTION],
				pDcCommand->m_pUser->m_ui16SameSearchs, pDcCommand->m_pUser->m_ui64SameSearchsTick, 
				SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_SEARCH_MESSAGES], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_SEARCH_TIME],
				pDcCommand->m_sCommand+(bMulti == true ? 13 : 8), pDcCommand->m_ui32CommandLen-(bMulti == true ? 13 : 8),
				pDcCommand->m_pUser->m_sLastSearch, pDcCommand->m_pUser->m_ui16LastSearchLen, bNewData) == true)
			{
                return false;
            }

			if(bNewData == true) {
				pDcCommand->m_pUser->SetLastSearch(pDcCommand->m_sCommand+(bMulti == true ? 13 : 8), pDcCommand->m_ui32CommandLen-(bMulti == true ? 13 : 8));
			}
        }
    }
    
    return true;
}
//---------------------------------------------------------------------------

// $Search $MultiSearch
void DcCommands::Search(DcCommand * pDcCommand, const bool bMulti) {
    uint32_t iAfterCmd;
    if(bMulti == false) {
        if(pDcCommand->m_ui32CommandLen < 10) {
			UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $Search (%s) from %s (%s) - user closed.", pDcCommand->m_sCommand, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

			pDcCommand->m_pUser->Close();
			return;
        }
        iAfterCmd = 8;
    } else {
        if(pDcCommand->m_ui32CommandLen < 15) {
			UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $MultiSearch (%s) from %s (%s) - user closed.", pDcCommand->m_sCommand, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

			pDcCommand->m_pUser->Close();
            return;
        }
        iAfterCmd = 13;
    }

    if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOSEARCHINTERVAL) == false) {
        if(DeFloodCheckInterval(pDcCommand->m_pUser, INTERVAL_SEARCH, pDcCommand->m_pUser->m_ui16SearchsInt, 
			pDcCommand->m_pUser->m_ui64SearchsIntTick, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_INTERVAL_MESSAGES],
            (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SEARCH_INTERVAL_TIME]) == true) {
            return;
        }
    }

	if(ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::SEARCH_ARRIVAL) == true ||
		pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
		return;
	}

    // send search from actives to all, from passives to actives only
    // PPK ... optimization ;o)
    if(bMulti == false && *((uint32_t *)(pDcCommand->m_sCommand+iAfterCmd)) == *((uint32_t *)"Hub:")) {
        if(pDcCommand->m_pUser->m_sTag == NULL) {
			pDcCommand->m_pUser->m_ui32BoolBits &= ~User::BIT_IPV4_ACTIVE;
        }

        // PPK ... check nick !!!
        if((pDcCommand->m_sCommand[iAfterCmd+4+pDcCommand->m_pUser->m_ui8NickLen] != ' ') || (memcmp(pDcCommand->m_sCommand+iAfterCmd+4, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_ui8NickLen) != 0)) {
			UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in search from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

			pDcCommand->m_pUser->Close();
            return;
        }

        if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOSEARCHLIMITS) == false &&
            (SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SEARCH_LEN] != 0 || SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SEARCH_LEN] != 0)) {
            // PPK ... search string len check
            // $Search Hub:PPK F?T?0?2?test|
            uint32_t iChar = iAfterCmd+8+pDcCommand->m_pUser->m_ui8NickLen+1;
            uint32_t iCount = 0;
            for(; iChar < pDcCommand->m_ui32CommandLen; iChar++) {
                if(pDcCommand->m_sCommand[iChar] == '?') {
                    iCount++;
                    if(iCount == 2)
                        break;
                }
            }

            iCount = pDcCommand->m_ui32CommandLen-2-iChar;

            if(iCount < (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SEARCH_LEN]) {
				pDcCommand->m_pUser->SendFormat("DcCommands::Search1", true, "<%s> %s %hd.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SORRY_MIN_SEARCH_LEN_IS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SEARCH_LEN]);
                return;
            }
            if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SEARCH_LEN] != 0 && iCount > (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SEARCH_LEN]) {
				pDcCommand->m_pUser->SendFormat("DcCommands::Search2", true, "<%s> %s %hd.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SORRY_MAX_SEARCH_LEN_IS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SEARCH_LEN]);
                return;
            }
        }

		pDcCommand->m_pUser->m_ui32SR = 0;

		pDcCommand->m_pUser->m_pCmdPassiveSearch = AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdPassiveSearch, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, false);
    } else {
        if(pDcCommand->m_pUser->m_sTag == NULL) {
			pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_IPV4_ACTIVE;
        }

        if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOSEARCHLIMITS) == false && (SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SEARCH_LEN] != 0 || SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SEARCH_LEN] != 0)) {
            // PPK ... search string len check
            // $Search 1.2.3.4:1 F?F?0?2?test| / $Search [::1]:1 F?F?0?2?test|
            uint32_t ui32Char = iAfterCmd+9;
            uint32_t ui32QCount = 0;

            for(; ui32Char < pDcCommand->m_ui32CommandLen; ui32Char++) {
                if(pDcCommand->m_sCommand[ui32Char] == '?') {
                    ui32QCount++;
                    if(ui32QCount == 4)
                        break;
                }
            }

            uint32_t ui32Count = pDcCommand->m_ui32CommandLen-2-ui32Char;

            if(ui32QCount != 4 || ui32Count < (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SEARCH_LEN]) {
				pDcCommand->m_pUser->SendFormat("DcCommands::Search3", true, "<%s> %s %hd.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SORRY_MIN_SEARCH_LEN_IS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_SEARCH_LEN]);
                return;
            }
            if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SEARCH_LEN] != 0 && ui32Count > (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SEARCH_LEN]) {
				pDcCommand->m_pUser->SendFormat("DcCommands::Search4", true, "<%s> %s %hd.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SORRY_MAX_SEARCH_LEN_IS], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SEARCH_LEN]);
                return;
            }
        }

		// Get space after IP:Port
        char * pSpace = strchr(pDcCommand->m_sCommand+iAfterCmd, ' ');
	    if(pSpace == NULL) {
	        return;
	    }
	
	    pSpace[0] = '\0';

		// Now we have between sData+iAfterCmd and pSpace IP:Port ... or we should have. Let's check that and start with length.
    	uint32_t ui32IpPortLen = (uint32_t)(pSpace - (pDcCommand->m_sCommand+iAfterCmd));

		// Check minimal (shortest ip:port can be [::1]:1) and maximal (longest ip:port can be [1234:1234:1234:1234:1234:1234:1234:1234]:12345S) length.
		if(ui32IpPortLen < 7 || ui32IpPortLen > 48) {
			pDcCommand->m_pUser->SendFormat("DcCommands::Search bad IP:Port len", true, "<%s> %s '%s'!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOUR_CLIENT_SEND_INCORRECT_IPPORT_IN_COMMAND], pDcCommand->m_sCommand+iAfterCmd);

			pSpace[0] = ' ';
			UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad IP:Port length in %sSearch from %s (%s). (%s)", bMulti == false ? "" : "M", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);
	
			pDcCommand->m_pUser->Close();
	        return;
		}

		// Check if Port is valid.
		uint32_t ui32PortLen = 0;
		uint16_t ui16Port = 0; // Zero == invalid port ...

    	// Check for port validity and get Port when valid
		ui16Port = CheckAndGetPort(pDcCommand->m_sCommand+iAfterCmd+(ui32IpPortLen-6), (uint8_t)(ui32IpPortLen-(((pDcCommand->m_sCommand+iAfterCmd)+(ui32IpPortLen-6))-(pDcCommand->m_sCommand+iAfterCmd))), ui32PortLen);

		// Check if we get valid port number
		if(ui16Port == 0) {
			pDcCommand->m_pUser->SendFormat("DcCommands::Search invalid Port", true, "<%s> %s '%s'!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOUR_CLIENT_SEND_INCORRECT_PORT_IN_SEARCH], pDcCommand->m_sCommand+iAfterCmd);

			pSpace[0] = ' ';
			UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad Port in %sSearch from %s (%s). (%s)", bMulti == false ? "" : "M", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);
	
			pDcCommand->m_pUser->Close();
	        return;
		}

		uint32_t ui32IpLen = (ui32IpPortLen - ui32PortLen) - 1;
		bool bInvalidIP = true;
		char * pIP = pDcCommand->m_sCommand+iAfterCmd;

		// Check if we get valid IP address
		if((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
	        if((pDcCommand->m_pUser->m_ui8IpLen + 2U) == ui32IpLen && pIP[0] == '[' && pIP[1+pDcCommand->m_pUser->m_ui8IpLen] == ']' && strncmp(pIP+1, pDcCommand->m_pUser->m_sIP, pDcCommand->m_pUser->m_ui8IpLen) == 0) {
	        	bInvalidIP = false;
	        } else if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) && pDcCommand->m_pUser->m_ui8IPv4Len == ui32IpLen &&  strncmp(pIP, pDcCommand->m_pUser->m_sIPv4, pDcCommand->m_pUser->m_ui8IPv4Len) == 0) {
	        	bInvalidIP = false;
	        }
	    } else if(pDcCommand->m_pUser->m_ui8IpLen == ui32IpLen && strncmp(pIP, pDcCommand->m_pUser->m_sIP, pDcCommand->m_pUser->m_ui8IpLen) == 0) {
	    	bInvalidIP = false;
	    }

        // IP check
        if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOIPCHECK) == false && bInvalidIP == true) {
        	if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_WARNED_WRONG_IP) == User::BIT_WARNED_WRONG_IP) == false) {
        		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad Ip in %sSearch from %s (%s/%s). (%s)", bMulti == false ? "" : "M", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_pUser->m_sIPv4, pDcCommand->m_sCommand);
        	}

            if((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
                if((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV6_ACTIVE) == User::BIT_IPV6_ACTIVE) {
                    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Search [%s]:%hu %s", pDcCommand->m_pUser->m_sIP, ui16Port, pSpace+1);
                    if(iMsgLen > 0) {
						pDcCommand->m_pUser->m_pCmdActive6Search = AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdActive6Search, ServerManager::m_pGlobalBuffer, iMsgLen, true);
                    }
                } else {
                    int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Search Hub:%s %s", pDcCommand->m_pUser->m_sNick, pSpace+1);
                    if(iMsgLen > 0) {
						pDcCommand->m_pUser->m_pCmdPassiveSearch = AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdPassiveSearch, ServerManager::m_pGlobalBuffer, iMsgLen, false);
                    }
                }

				if((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) {
                    if((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE) {
                        int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Search %s:%hu %s", pDcCommand->m_pUser->m_sIPv4, ui16Port, pSpace+1);
                        if(iMsgLen > 0) {
							pDcCommand->m_pUser->m_pCmdActive4Search = AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdActive4Search, ServerManager::m_pGlobalBuffer, iMsgLen, true);
                        }
                    } else {
                        int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Search Hub:%s %s", pDcCommand->m_pUser->m_sNick, pSpace+1);
                        if(iMsgLen > 0) {
							pDcCommand->m_pUser->m_pCmdPassiveSearch = AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdPassiveSearch, ServerManager::m_pGlobalBuffer, iMsgLen, false);
                        }
                    }
				}
            } else if((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) {
                char * sIP = pDcCommand->m_pUser->m_ui8IPv4Len == 0 ? pDcCommand->m_pUser->m_sIP : pDcCommand->m_pUser->m_sIPv4;

                int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Search %s:%hu %s", sIP, ui16Port, pSpace+1);
                if(iMsgLen > 0) {
					pDcCommand->m_pUser->m_pCmdActive4Search = AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdActive4Search, ServerManager::m_pGlobalBuffer, iMsgLen, true);
                }
            }

            if(ui32IpLen != 0 && pIP[ui32IpLen-1] == ']') {
            	pIP[ui32IpLen-1] = '\0';
			} else {
            	pIP[ui32IpLen] = '\0';
            }

			if(pIP[0] == '[') {
				pIP++;
			}

            SendIPFixedMsg(pDcCommand->m_pUser, pIP, pDcCommand->m_pUser->m_sIP);
            return;
        }

		// Restore space after IP:Port
        pSpace[0] = ' ';

        if(bMulti == true) {
			pDcCommand->m_sCommand[5] = '$';
			pDcCommand->m_sCommand += 5;
			pDcCommand->m_ui32CommandLen -= 5;
        }

		if((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
			if(pDcCommand->m_sCommand[8] == '[') {
				pDcCommand->m_pUser->m_pCmdActive6Search = AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdActive6Search, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, true);

				// IPv6 user sent active request.. when he have available IPv4 then create proper IPv4 request too.
				if((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV4) == User::BIT_IPV4) {
                	if((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV4_ACTIVE) == User::BIT_IPV4_ACTIVE) {
                    	int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Search %s:%hu %s", pDcCommand->m_pUser->m_sIPv4, ui16Port, pSpace+1);
                    	if(iMsgLen > 0) {
							pDcCommand->m_pUser->m_pCmdActive4Search = AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdActive4Search, ServerManager::m_pGlobalBuffer, iMsgLen, true);
                    	}
					} else {
                    	int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "$Search Hub:%s %s", pDcCommand->m_pUser->m_sNick, pSpace+1);
                    	if(iMsgLen > 0) {
							pDcCommand->m_pUser->m_pCmdPassiveSearch = AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdPassiveSearch, ServerManager::m_pGlobalBuffer, iMsgLen, false);
                    	}
                    }
				}
			} else {
				// When IPv6 user sent active search request with IPv4 address (he should't do that) then just send this request...
				pDcCommand->m_pUser->m_pCmdActive4Search = AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdActive4Search, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, true);
			}
		} else {
			pDcCommand->m_pUser->m_pCmdActive4Search = AddSearch(pDcCommand->m_pUser, pDcCommand->m_pUser->m_pCmdActive4Search, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, true);
		}
    }
}
//---------------------------------------------------------------------------

// $MyINFO $ALL  $ $$$$|
bool DcCommands::MyINFODeflood(DcCommand * pDcCommand) {
    if(pDcCommand->m_ui32CommandLen < (22u+pDcCommand->m_pUser->m_ui8NickLen)) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $MyINFO (%s) from %s (%s) - user closed.", pDcCommand->m_sCommand, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return false;
    }

    // PPK ... check flood ...
    if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODMYINFO) == false) { 
        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION] != 0) {
            if(DeFloodCheckForFlood(pDcCommand->m_pUser, DEFLOOD_MYINFO, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION],
				pDcCommand->m_pUser->m_ui16MyINFOs, pDcCommand->m_pUser->m_ui64MyINFOsTick, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_MESSAGES],
              (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_TIME]) == true) {
                return false;
            }
        }

        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION2] != 0) {
            if(DeFloodCheckForFlood(pDcCommand->m_pUser, DEFLOOD_MYINFO, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_ACTION2],
				pDcCommand->m_pUser->m_ui16MyINFOs2, pDcCommand->m_pUser->m_ui64MyINFOsTick2, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_MESSAGES2],
              (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_TIME2]) == true) {
                return false;
            }
        }
    }

    if(pDcCommand->m_ui32CommandLen > (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_MYINFO_LEN]) {
		pDcCommand->m_pUser->SendFormat("DcCommands::MyINFODeflood", true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_MYINFO_TOO_LONG]);

		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $MyINFO from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

		pDcCommand->m_pUser->Close();
		return false;
    }
    
    return true;
}
//---------------------------------------------------------------------------

// $MyINFO $ALL  $ $$$$|
bool DcCommands::MyINFO(DcCommand * pDcCommand) {
    // if no change, just return
    // else store MyINFO and perform all checks again
    if(pDcCommand->m_pUser->m_sMyInfoOriginal != NULL) { // PPK ... optimizations
       	if(pDcCommand->m_ui32CommandLen == pDcCommand->m_pUser->m_ui16MyInfoOriginalLen && memcmp(pDcCommand->m_pUser->m_sMyInfoOriginal+14+pDcCommand->m_pUser->m_ui8NickLen, pDcCommand->m_sCommand+14+pDcCommand->m_pUser->m_ui8NickLen, pDcCommand->m_pUser->m_ui16MyInfoOriginalLen-14-pDcCommand->m_pUser->m_ui8NickLen) == 0) {
           return false;
        }
    }

	pDcCommand->m_pUser->SetMyInfoOriginal(pDcCommand->m_sCommand, (uint16_t)pDcCommand->m_ui32CommandLen);

    if(pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
        return false;
    }

    if(pDcCommand->m_pUser->ProcessRules() == false) {
		pDcCommand->m_pUser->Close();
        return false;
    }
    
    // TODO 3 -oPTA -ccheckers: Slots fetching for no tag users
    //search command for slots fetch for users without tag
    //if(pDcCommand->m_pUser->Tag == NULL)
    //{
    //    pDcCommand->m_pUser->SendText("$Search "+HubAddress->Text+":411 F?F?0?1?.|");
    //}

    // SEND myinfo to others (including me) only if this is
    // a refresh MyINFO event. Else dispatch it in addMe condition
    // of service loop

    // PPK ... moved lua here -> another "optimization" ;o)
	ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::MYINFO_ARRIVAL);

	if(pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
		return false;
	}
    
    return true;
}
//---------------------------------------------------------------------------

// $MyPass
void DcCommands::MyPass(DcCommand * pDcCommand) {
    RegUser * pReg = RegManager::m_Ptr->Find(pDcCommand->m_pUser);
    if(pReg != NULL && (pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_WAITING_FOR_PASS) == User::BIT_WAITING_FOR_PASS) {
		pDcCommand->m_pUser->m_ui32BoolBits &= ~User::BIT_WAITING_FOR_PASS;
    } else {
        // We don't send $GetPass!
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] $MyPass without request from %s (%s) - user closed.", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return;
    }

    if(pDcCommand->m_ui32CommandLen < 10|| pDcCommand->m_ui32CommandLen > 73) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $MyPass from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

		pDcCommand->m_pUser->Close();
        return;
    }

	pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '\0'; // cutoff pipe

    bool bBadPass = false;

	if(pReg->m_bPassHash == true) {
		uint8_t ui8Hash[64];

		size_t szLen = pDcCommand->m_ui32CommandLen-9;

		if(HashPassword(pDcCommand->m_sCommand+8, szLen, ui8Hash) == false || memcmp(pReg->m_ui8PassHash, ui8Hash, 64) != 0) {
			bBadPass = true;
		}
	} else {
		if(strcmp(pReg->m_sPass, pDcCommand->m_sCommand+8) != 0) {
			bBadPass = true;
		}
	}

    // if password is wrong, close the connection
    if(bBadPass == true) {
        if(SettingManager::m_Ptr->m_bBools[SETBOOL_ADVANCED_PASS_PROTECTION] == true) {
            time(&pReg->m_tLastBadPass);
            if(pReg->m_ui8BadPassCount < 255)
                pReg->m_ui8BadPassCount++;
        }
    
        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE] != 0) {
            // brute force password protection
			PassBf * PassBfItem = Find(pDcCommand->m_pUser->m_ui128IpHash);
            if(PassBfItem == NULL) {
                PassBfItem = new (std::nothrow) PassBf(pDcCommand->m_pUser->m_ui128IpHash);
                if(PassBfItem == NULL) {
					AppendDebugLog("%s - [MEM] Cannot allocate new PassBfItem in DcCommands::MyPass\n");
                	return;
                }

                if(m_pPasswdBfCheck != NULL) {
					m_pPasswdBfCheck->m_pPrev = PassBfItem;
                    PassBfItem->m_pNext = m_pPasswdBfCheck;
					m_pPasswdBfCheck = PassBfItem;
                }
				m_pPasswdBfCheck = PassBfItem;
            } else {
                if(PassBfItem->m_iCount == 2) {
                    BanItem * pBan = BanManager::m_Ptr->FindFull(pDcCommand->m_pUser->m_ui128IpHash);
                    if(pBan == NULL || ((pBan->m_ui8Bits & BanManager::FULL) == BanManager::FULL) == false) {
                        int iRet = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "3x bad password for nick %s", pDcCommand->m_pUser->m_sNick);
                        if(iRet <= 0) {
                            ServerManager::m_pGlobalBuffer[0] = '\0';
                        }

                        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_BAN_TYPE] == 1) {
                            BanManager::m_Ptr->BanIp(pDcCommand->m_pUser, NULL, ServerManager::m_pGlobalBuffer, NULL, true);
                        } else {
                            BanManager::m_Ptr->TempBanIp(pDcCommand->m_pUser, NULL, ServerManager::m_pGlobalBuffer, NULL, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_BRUTE_FORCE_PASS_PROTECT_TEMP_BAN_TIME]*60, 0, true);
                        }
                        Remove(PassBfItem);

						pDcCommand->m_pUser->SendFormat("DcCommands::MyPass1", false, "<%s> %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOUR_IP_BANNED_BRUTE_FORCE_ATTACK]);
                    } else {
						pDcCommand->m_pUser->SendFormat("DcCommands::MyPass2", false, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOUR_IS_BANNED]);
                    }
                    if(SettingManager::m_Ptr->m_bBools[SETBOOL_REPORT_3X_BAD_PASS] == true) {
                        GlobalDataQueue::m_Ptr->StatusMessageFormat("DcCommands::MyPass", "<%s> *** %s %s %s %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_IP], pDcCommand->m_pUser->m_sIP, 
							LanguageManager::m_Ptr->m_sTexts[LAN_BANNED_BECAUSE_3X_BAD_PASS_FOR_NICK], pDcCommand->m_pUser->m_sNick);
                    }

					UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad 3x password from %s (%s) - user banned. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

					pDcCommand->m_pUser->Close();
                    return;
                } else {
                    PassBfItem->m_iCount++;
                }
            }
        }

		pDcCommand->m_pUser->SendFormat("DcCommands::MyPass3", false, "$BadPass|<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_INCORRECT_PASSWORD]);

		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad password from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

		pDcCommand->m_pUser->Close();
        return;
    } else {
		pDcCommand->m_pUser->m_i32Profile = (int32_t)pReg->m_ui16Profile;

        pReg->m_ui8BadPassCount = 0;

        PassBf * PassBfItem = Find(pDcCommand->m_pUser->m_ui128IpHash);
        if(PassBfItem != NULL) {
            Remove(PassBfItem);
        }

		pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '|'; // add back pipe

        // PPK ... Lua DataArrival only if pass is ok
		ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::PASSWORD_ARRIVAL);

		if(pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
    		return;
    	}

        if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::HASKEYICON) == true) {
			pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_OPERATOR;
        } else {
			pDcCommand->m_pUser->m_ui32BoolBits &= ~User::BIT_OPERATOR;
        }
        
        // PPK ... addition for registered users, kill your own ghost >:-]
        if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HASHED) == User::BIT_HASHED) == false) {
            User *OtherUser = HashManager::m_Ptr->FindUser(pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_ui8NickLen);
            if(OtherUser != NULL) {
				UdpDebug::m_Ptr->BroadcastFormat("[SYS] Ghost %s (%s) closed.", OtherUser->m_sNick, OtherUser->m_sIP);

                if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false) {
           	        OtherUser->Close();
                } else {
                    OtherUser->Close(true);
                }
            }
            if(HashManager::m_Ptr->Add(pDcCommand->m_pUser) == false) {
                return;
            }
			pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_HASHED;
        }
        if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false) {
            // welcome the new user
            // PPK ... fixed bad DC protocol implementation, $LogedIn is only for OPs !!!
            // registered DC1 users have enabled OP menu :)))))))))
            if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == true) {
				pDcCommand->m_pUser->SendFormat("DcCommands::MyPass4", true, "$Hello %s|$LogedIn %s|", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sNick);
            } else {
				pDcCommand->m_pUser->SendFormat("DcCommands::MyPass5", true, "$Hello %s|", pDcCommand->m_pUser->m_sNick);
            }

            return;
        } else {
            if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false) {
				pDcCommand->m_pUser->AddMeOrIPv4Check();
            }
        }
    }     
}
//---------------------------------------------------------------------------

// $OpForceMove $Who:<nickname>$Where:<iptoredirect>$Msg:<a message>
void DcCommands::OpForceMove(DcCommand * pDcCommand) {
    if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::REDIRECT) == false) {
		pDcCommand->m_pUser->SendFormat("DcCommands::OpForceMove1", true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD]);
        return;
    }

    if(pDcCommand->m_ui32CommandLen < 31) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $OpForceMove (%s) from %s (%s) - user closed.", pDcCommand->m_sCommand, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return;
    }

	if(ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::OPFORCEMOVE_ARRIVAL) == true ||
		pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
		return;
	}

	pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '\0'; // cutoff pipe

    char *sCmdParts[] = { NULL, NULL, NULL };
    uint16_t iCmdPartsLen[] = { 0, 0, 0 };
                
    uint8_t cPart = 0;
                
    sCmdParts[cPart] = pDcCommand->m_sCommand+18; // nick start
                
    for(uint32_t ui32i = 18; ui32i < pDcCommand->m_ui32CommandLen; ui32i++) {
        if(pDcCommand->m_sCommand[ui32i] == '$') {
			pDcCommand->m_sCommand[ui32i] = '\0';
            iCmdPartsLen[cPart] = (uint16_t)((pDcCommand->m_sCommand+ui32i)-sCmdParts[cPart]);
                    
            // are we on last $ ???
            if(cPart == 1) {
                sCmdParts[2] = pDcCommand->m_sCommand+ui32i+1;
                iCmdPartsLen[2] = (uint16_t)(pDcCommand->m_ui32CommandLen-ui32i-1);
                break;
            }
                    
            cPart++;
            sCmdParts[cPart] = pDcCommand->m_sCommand+ui32i+1;
        }
    }

    if(iCmdPartsLen[0] == 0 || iCmdPartsLen[1] < 7 || iCmdPartsLen[2] < 5 || iCmdPartsLen[1] > 4096 || iCmdPartsLen[2] > 16384) {
        return;
    }

    User *OtherUser = HashManager::m_Ptr->FindUser(sCmdParts[0], iCmdPartsLen[0]);
    if(OtherUser) {
   	    if(OtherUser->m_i32Profile != -1 && pDcCommand->m_pUser->m_i32Profile > OtherUser->m_i32Profile) {
			pDcCommand->m_pUser->SendFormat("DcCommands::OpForceMove2", true, "<%s> %s %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_NOT_ALLOWED_TO_REDIRECT], OtherUser->m_sNick);
            return;
        } else {
            OtherUser->SendFormat("DcCommands::OpForceMove3", false, "<%s> %s %s %s %s. %s: %s|$ForceMove %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_REDIRECTED_TO], sCmdParts[1]+6, 
				LanguageManager::m_Ptr->m_sTexts[LAN_BY_LWR], pDcCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_MESSAGE], sCmdParts[2]+4, sCmdParts[1]+6);

            // PPK ... close user !!!
			UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) redirected by %s", OtherUser->m_sNick, OtherUser->m_sIP, pDcCommand->m_pUser->m_sNick);

            OtherUser->Close();

            if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
                GlobalDataQueue::m_Ptr->StatusMessageFormat("DcCommands::OpForceMove", "<%s> *** %s %s %s %s %s. %s: %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], OtherUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_IS_REDIRECTED_TO], sCmdParts[1]+6, 
					LanguageManager::m_Ptr->m_sTexts[LAN_BY_LWR], pDcCommand->m_pUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_MESSAGE], sCmdParts[2]+4);
            }

            if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
				pDcCommand->m_pUser->SendFormat("DcCommands::OpForceMove4", true, "<%s> *** %s %s %s. %s: %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], OtherUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_IS_REDIRECTED_TO], sCmdParts[1]+6, LanguageManager::m_Ptr->m_sTexts[LAN_MESSAGE], sCmdParts[2]+4);
            }
        }
    }
}
//---------------------------------------------------------------------------

// $RevConnectToMe <ownnick> <nickname>
void DcCommands::RevConnectToMe(DcCommand * pDcCommand) {
    if(pDcCommand->m_ui32CommandLen < 19) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $RevConnectToMe (%s) from %s (%s) - user closed.", pDcCommand->m_sCommand, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return;
    }

    // PPK ... optimizations
    if((pDcCommand->m_sCommand[16+pDcCommand->m_pUser->m_ui8NickLen] != ' ') || (memcmp(pDcCommand->m_sCommand+16, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_ui8NickLen) != 0)) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in RCTM from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

		pDcCommand->m_pUser->Close();
        return;
    }

    // PPK ... check flood ...
    if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODRCTM) == false) { 
        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION] != 0) {
            if(DeFloodCheckForFlood(pDcCommand->m_pUser, DEFLOOD_RCTM, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION],
				pDcCommand->m_pUser->m_ui16RCTMs, pDcCommand->m_pUser->m_ui64RCTMsTick, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_MESSAGES],
              (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_TIME]) == true) {
				return;
            }
        }

        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION2] != 0) {
            if(DeFloodCheckForFlood(pDcCommand->m_pUser, DEFLOOD_RCTM, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_ACTION2],
				pDcCommand->m_pUser->m_ui16RCTMs2, pDcCommand->m_pUser->m_ui64RCTMsTick2, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_MESSAGES2],
			  (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_RCTM_TIME2]) == true) {
                return;
            }
        }
    }

    if(pDcCommand->m_ui32CommandLen > (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_RCTM_LEN]) {
		pDcCommand->m_pUser->SendFormat("DcCommands::RevConnectToMe", true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_RCTM_TOO_LONG]);

		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Long $RevConnectToMe from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

		pDcCommand->m_pUser->Close();
        return;
    }

    // PPK ... $RCTM means user is pasive ?!? Probably yes, let set it not active and use on another places ;)
    if(pDcCommand->m_pUser->m_sTag == NULL) {
		pDcCommand->m_pUser->m_ui32BoolBits &= ~User::BIT_IPV4_ACTIVE;
    }

	if(ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::REVCONNECTTOME_ARRIVAL) == true ||
		pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
		return;
	}

	pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '\0'; // cutoff pipe
       
    User *OtherUser = HashManager::m_Ptr->FindUser(pDcCommand->m_sCommand+17+pDcCommand->m_pUser->m_ui8NickLen, pDcCommand->m_ui32CommandLen-(18+pDcCommand->m_pUser->m_ui8NickLen));
    // PPK ... no connection to yourself !!!
    if(OtherUser != NULL && OtherUser != pDcCommand->m_pUser && OtherUser->m_ui8State == User::STATE_ADDED) {
		pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '|'; // add back pipe
		pDcCommand->m_pUser->AddPrcsdCmd(PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, OtherUser);
    }   
}
//---------------------------------------------------------------------------

// $SR <nickname> - Search Respond for passive users
void DcCommands::SR(DcCommand * pDcCommand) {
    if(pDcCommand->m_ui32CommandLen < 6u+pDcCommand->m_pUser->m_ui8NickLen) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $SR (%s) from %s (%s) - user closed.", pDcCommand->m_sCommand, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return;
    }

    // PPK ... check flood ...
    if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODSR) == false) { 
        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION] != 0) {
            if(DeFloodCheckForFlood(pDcCommand->m_pUser, DEFLOOD_SR, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION],
				pDcCommand->m_pUser->m_ui16SRs, pDcCommand->m_pUser->m_ui64SRsTick, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_MESSAGES],
              (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_TIME]) == true) {
				return;
            }
        }

        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION2] != 0) {
            if(DeFloodCheckForFlood(pDcCommand->m_pUser, DEFLOOD_SR, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_ACTION2],
				pDcCommand->m_pUser->m_ui16SRs2, pDcCommand->m_pUser->m_ui64SRsTick2, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_MESSAGES2],
			  (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SR_TIME2]) == true) {
                return;
            }
        }
    }

    if(pDcCommand->m_ui32CommandLen > (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_SR_LEN]) {
		pDcCommand->m_pUser->SendFormat("DcCommands::SR", true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SR_TOO_LONG]);

		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Long $SR from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);


		pDcCommand->m_pUser->Close();
		return;
    }

    // check $SR spoofing (thanx Fusbar)
    // PPK... added checking for empty space after nick
    if(pDcCommand->m_sCommand[4+pDcCommand->m_pUser->m_ui8NickLen] != ' ' || memcmp(pDcCommand->m_sCommand+4, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_ui8NickLen) != 0) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in SR from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

		pDcCommand->m_pUser->Close();
        return;
    }

    // past SR to script only if it's not a data for SlotFetcher
	if(ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::SR_ARRIVAL) == true ||
		pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
		return;
	}

	pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '\0'; // cutoff pipe

    char * pToNick = strrchr(pDcCommand->m_sCommand, '\5');
    if(pToNick == NULL) return;

    User *OtherUser = HashManager::m_Ptr->FindUser(pToNick+1, pDcCommand->m_ui32CommandLen-2-(pToNick-pDcCommand->m_sCommand));
    // PPK ... no $SR to yourself !!!
    if(OtherUser != NULL && OtherUser != pDcCommand->m_pUser && OtherUser->m_ui8State == User::STATE_ADDED) {
        // PPK ... search replies limiting
        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_PASIVE_SR] != 0) {
			if(OtherUser->m_ui32SR >= (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_PASIVE_SR])
                return;
                        
            OtherUser->m_ui32SR++;
        }

        // cutoff the last part // PPK ... and do it fast ;)
		pToNick[0] = '|';
		pToNick[1] = '\0';
		pDcCommand->m_pUser->AddPrcsdCmd(PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen-OtherUser->m_ui8NickLen-1, OtherUser);
    }   
}

//---------------------------------------------------------------------------

// $SR <nickname> - Search Respond for active users from UDP
void DcCommands::SRFromUDP(DcCommand * pDcCommand) {
	if(ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::UDP_SR_ARRIVAL) == true ||
		pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
		return;
	}
}
//---------------------------------------------------------------------------

// $Supports item item item... PPK $Supports UserCommand NoGetINFO NoHello UserIP2 QuickList|
void DcCommands::Supports(DcCommand * pDcCommand) {
    if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_HAVE_SUPPORTS) == User::BIT_HAVE_SUPPORTS) == true) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] $Supports flood from %s (%s) - user closed.", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return;
    }

	pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_HAVE_SUPPORTS;
    
    if(pDcCommand->m_ui32CommandLen < 13) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $Supports (%s) from %s (%s) - user closed.", pDcCommand->m_sCommand, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return;
    }

    if(pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-2] == ' ') {
        if(SettingManager::m_Ptr->m_bBools[SETBOOL_NO_QUACK_SUPPORTS] == false) {
			pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_QUACK_SUPPORTS;
        } else {
			UdpDebug::m_Ptr->BroadcastFormat("[SYS] Quack $Supports from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

			pDcCommand->m_pUser->SendFormat("DcCommands::Supports1", false, "<%s> %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_QUACK_SUPPORTS]);

			pDcCommand->m_pUser->Close();
			return;
		}
    }

	ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::SUPPORTS_ARRIVAL);

	if(pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
    	return;
    }

    char * sSupport = pDcCommand->m_sCommand+10;
	size_t szDataLen;
                    
    for(uint32_t ui32i = 10; ui32i < pDcCommand->m_ui32CommandLen-1; ui32i++) {
        if(pDcCommand->m_sCommand[ui32i] == ' ') {
			pDcCommand->m_sCommand[ui32i] = '\0';
        } else if(ui32i != pDcCommand->m_ui32CommandLen-2) {
            continue;
        } else {
            ui32i++;
        }

        szDataLen = (pDcCommand->m_sCommand+ui32i)-sSupport;

        switch(sSupport[0]) {
            case 'N':
                if(sSupport[1] == 'o') {
                    if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) == false && szDataLen == 7 && memcmp(sSupport+2, "Hello", 5) == 0) {
						pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_NOHELLO;
                    } else if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_NOGETINFO) == User::SUPPORTBIT_NOGETINFO) == false && szDataLen == 9 && memcmp(sSupport+2, "GetINFO", 7) == 0) {
						pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_NOGETINFO;
                    }
                }
                break;
            case 'Q': {
                if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == false && szDataLen == 9 && *((uint64_t *)(sSupport+1)) == *((uint64_t *)"uickList")) {
					pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_QUICKLIST;
                    // PPK ... in fact NoHello is only not fully implemented Quicklist (without diferent login sequency)
                    // That's why i overide NoHello here and use bQuicklist only for login, on other places is same as NoHello ;)
					pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_NOHELLO;
                }
                break;
            }
            case 'U': {
                if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_USERCOMMAND) == User::SUPPORTBIT_USERCOMMAND) == false && szDataLen == 11 && memcmp(sSupport+1, "serCommand", 10) == 0) {
					pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_USERCOMMAND;
                } else if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_USERIP2) == User::SUPPORTBIT_USERIP2) == false && szDataLen == 7 && memcmp(sSupport+1, "serIP2", 6) == 0) {
					pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_USERIP2;
                }
                break;
            }
            case 'B': {
                if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false && szDataLen == 7 && memcmp(sSupport+1, "otINFO", 6) == 0) {
                    if(SettingManager::m_Ptr->m_bBools[SETBOOL_DONT_ALLOW_PINGERS] == true) {
						pDcCommand->m_pUser->SendFormat("DcCommands::Supports2", false, "<%s> %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SORRY_THIS_HUB_NOT_ALLOW_PINGERS]);
						pDcCommand->m_pUser->Close();
                        return;
                    } else {
						pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_PINGER;
						pDcCommand->m_pUser->SendFormat("DcCommands::Supports4", true, "%s%" PRIu64 " %s, %" PRIu64 " %s, %" PRIu64 " %s / %s: %u)|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_NAME_WLCM], ServerManager::m_ui64Days, LanguageManager::m_Ptr->m_sTexts[LAN_DAYS_LWR], 
							ServerManager::m_ui64Hours, LanguageManager::m_Ptr->m_sTexts[LAN_HOURS_LWR], ServerManager::m_ui64Mins, LanguageManager::m_Ptr->m_sTexts[LAN_MINUTES_LWR], LanguageManager::m_Ptr->m_sTexts[LAN_USERS], ServerManager::m_ui32Logged);
                    }
                }
                break;
            }
            case 'Z': {
                if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == false) {
                	if(szDataLen == 6 && memcmp(sSupport+1, "Pipe0", 5) == 0) {
						pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_ZPIPE0;
						pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_ZPIPE;
						m_ui32StatZPipe++;
					} else if(szDataLen == 5 && *((uint32_t *)(sSupport+1)) == *((uint32_t *)"Pipe")) {
						pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_ZPIPE;
						m_ui32StatZPipe++;
                    }
                }
                break;
            }
            case 'I': {
                if(szDataLen == 4) {
                    if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_IP64) == User::SUPPORTBIT_IP64) == false && *((uint32_t *)sSupport) == *((uint32_t *)"IP64")) {
						pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_IP64;
                    } else if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_IPV4) == User::SUPPORTBIT_IPV4) == false && *((uint32_t *)sSupport) == *((uint32_t *)"IPv4")) {
						pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_IPV4;
                    }
                }
                break;
            }
            case 'T': {
            	if(szDataLen == 4) {
                    if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_TLS2) == User::SUPPORTBIT_TLS2) == false && *((uint32_t *)sSupport) == *((uint32_t *)"TLS2")) {
						pDcCommand->m_pUser->m_ui32SupportBits |= User::SUPPORTBIT_TLS2;
                    }
            	}
				break;
			}
            case '\0': {
                // PPK ... corrupted $Supports ???
				UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $Supports from %s (%s) - user closed.", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

				pDcCommand->m_pUser->Close();
                return;
            }
            default:
                // PPK ... unknown supports
                break;
        }
                
        sSupport = pDcCommand->m_sCommand+ui32i+1;
    }
    
	pDcCommand->m_pUser->m_ui8State = User::STATE_VALIDATE;
    
	pDcCommand->m_pUser->AddPrcsdCmd(PrcsdUsrCmd::SUPPORTS, NULL, 0, NULL);
}
//---------------------------------------------------------------------------

// $To: nickname From: ownnickname $<ownnickname> <message>
void DcCommands::To(DcCommand * pDcCommand) {
    char * pTemp = strchr(pDcCommand->m_sCommand+5, ' ');

    if(pDcCommand->m_ui32CommandLen < 19 || pTemp == NULL) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad To from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

		pDcCommand->m_pUser->Close();
        return;
    }
    
    size_t szNickLen = pTemp-(pDcCommand->m_sCommand+5);

    if(szNickLen > 64) {
		pDcCommand->m_pUser->SendFormat("DcCommands::To1", true, "<%s> *** %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_MAX_ALWD_NICK_LEN_64_CHARS]);
        return;
    }

    // is the mesg really from us ?
    // PPK ... replaced by better and faster code ;)
    int iRet = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "From: %s $<%s> ", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sNick);
    if(iRet <= 0 || strncmp(pTemp+1, ServerManager::m_pGlobalBuffer, iRet) != 0) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in To from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

		pDcCommand->m_pUser->Close();
        return;
    }

    //FloodCheck
    if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODPM) == false) {
        // PPK ... pm antiflood
        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_PM_ACTION] != 0) {
            pTemp[0] = '\0';
            if(DeFloodCheckForFlood(pDcCommand->m_pUser, DEFLOOD_PM, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_PM_ACTION],
				pDcCommand->m_pUser->m_ui16PMs, pDcCommand->m_pUser->m_ui64PMsTick, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_PM_MESSAGES],
                (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_PM_TIME], pDcCommand->m_sCommand+5) == true) {
                return;
            }
            pTemp[0] = ' ';
        }

        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_PM_ACTION2] != 0) {
            pTemp[0] = '\0';
            if(DeFloodCheckForFlood(pDcCommand->m_pUser, DEFLOOD_PM, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_PM_ACTION2],
				pDcCommand->m_pUser->m_ui16PMs2, pDcCommand->m_pUser->m_ui64PMsTick2, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_PM_MESSAGES2],
                (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_PM_TIME2], pDcCommand->m_sCommand+5) == true) {
                return;
            }
            pTemp[0] = ' ';
        }

        // 2nd check for PM flooding
		if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_PM_ACTION] != 0) {
			bool bNewData = false;
			pTemp[0] = '\0';
			if(DeFloodCheckForSameFlood(pDcCommand->m_pUser, DEFLOOD_SAME_PM, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_PM_ACTION],
				pDcCommand->m_pUser->m_ui16SamePMs, pDcCommand->m_pUser->m_ui64SamePMsTick, 
                SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_PM_MESSAGES], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_PM_TIME],
                pTemp+(12+(2*pDcCommand->m_pUser->m_ui8NickLen)), (pDcCommand->m_ui32CommandLen-(pTemp-pDcCommand->m_sCommand))-(12+(2*pDcCommand->m_pUser->m_ui8NickLen)), 
				pDcCommand->m_pUser->m_sLastPM, pDcCommand->m_pUser->m_ui16LastPMLen, bNewData, pDcCommand->m_sCommand+5) == true)
			{
                return;
            }
            pTemp[0] = ' ';

			if(bNewData == true) {
				pDcCommand->m_pUser->SetLastPM(pTemp+(12+(2*pDcCommand->m_pUser->m_ui8NickLen)), (pDcCommand->m_ui32CommandLen-(pTemp-pDcCommand->m_sCommand))-(12+(2*pDcCommand->m_pUser->m_ui8NickLen)));
			}
        }
    }

    if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOCHATLIMITS) == false) {
        // 1st check for length limit for PM message
        size_t szMessLen = pDcCommand->m_ui32CommandLen-(2*pDcCommand->m_pUser->m_ui8NickLen)-(pTemp-pDcCommand->m_sCommand)-13;
        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_PM_LEN] != 0 && szMessLen > (size_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_PM_LEN]) {
       	    // PPK ... hubsec alias
       	    pTemp[0] = '\0';
			pDcCommand->m_pUser->SendFormat("DcCommands::To2", true, "$To: %s From: %s $<%s> %s!|", pDcCommand->m_pUser->m_sNick, pDcCommand->m_sCommand+5, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_THE_MESSAGE_WAS_TOO_LONG]);
            return;
        }

        // PPK ... check for message lines limit
        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_PM_LINES] != 0 || SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_PM_ACTION] != 0) {
			if(pDcCommand->m_pUser->m_ui16SamePMs < 2) {
				uint16_t iLines = 1;
                for(uint32_t ui32i = 9+pDcCommand->m_pUser->m_ui8NickLen; ui32i < pDcCommand->m_ui32CommandLen-(pTemp-pDcCommand->m_sCommand)-1; ui32i++) {
                    if(pTemp[ui32i] == '\n') {
                        iLines++;
                    }
                }
				pDcCommand->m_pUser->m_ui16LastPmLines = iLines;
                if(pDcCommand->m_pUser->m_ui16LastPmLines > 1) {
					pDcCommand->m_pUser->m_ui16SameMultiPms++;
                }
            } else if(pDcCommand->m_pUser->m_ui16LastPmLines > 1) {
				pDcCommand->m_pUser->m_ui16SameMultiPms++;
            }

			if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODPM) == false && SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_PM_ACTION] != 0) {
				if(pDcCommand->m_pUser->m_ui16SameMultiPms > SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_PM_MESSAGES] &&
					pDcCommand->m_pUser->m_ui16LastPmLines >= SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_PM_LINES]) {
					pTemp[0] = '\0';
					uint16_t lines = 0;
					DeFloodDoAction(pDcCommand->m_pUser, DEFLOOD_SAME_MULTI_PM, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_PM_ACTION], lines, pDcCommand->m_sCommand+5);
                    return;
                }
            }

            if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_PM_LINES] != 0 && pDcCommand->m_pUser->m_ui16LastPmLines > SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_PM_LINES]) {
                pTemp[0] = '\0';
				pDcCommand->m_pUser->SendFormat("DcCommands::To3", true, "$To: %s From: %s $<%s> %s!|", pDcCommand->m_pUser->m_sNick, pDcCommand->m_sCommand+5, SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_THE_MESSAGE_WAS_TOO_LONG]);
                return;
            }
        }
    }

    if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOPMINTERVAL) == false) {
        pTemp[0] = '\0';
        if(DeFloodCheckInterval(pDcCommand->m_pUser, INTERVAL_PM, pDcCommand->m_pUser->m_ui16PMsInt, 
			pDcCommand->m_pUser->m_ui64PMsIntTick, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_PM_INTERVAL_MESSAGES],
            (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_PM_INTERVAL_TIME], pDcCommand->m_sCommand+5) == true) {
            return;
        }
        pTemp[0] = ' ';
    }

	if(ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::TO_ARRIVAL) == true ||
		pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
		return;
	}

    pTemp[0] = '\0';

    // ignore the silly debug messages !!!
    if(memcmp(pDcCommand->m_sCommand+5, "Vandel\\Debug", 12) == 0) {
        return;
    }

    // Everything's ok lets chat
    // if this is a PM to OpChat or Hub bot, process the message
    if(SettingManager::m_Ptr->m_bBools[SETBOOL_REG_BOT] == true && strcmp(pDcCommand->m_sCommand+5, SettingManager::m_Ptr->m_sTexts[SETTXT_BOT_NICK]) == 0) {
        pTemp += 9+pDcCommand->m_pUser->m_ui8NickLen;
        // PPK ... check message length, return if no mess found
        uint32_t ui32Len1 = (uint32_t)((pDcCommand->m_ui32CommandLen-(pTemp-pDcCommand->m_sCommand))+1);
        if(ui32Len1 <= pDcCommand->m_pUser->m_ui8NickLen+4u)
            return;

        // find chat message data
        char *sBuff = pTemp+pDcCommand->m_pUser->m_ui8NickLen+3;

        // non-command chat msg
        for(uint8_t ui8i = 0; ui8i < (uint8_t)SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_CHAT_COMMANDS_PREFIXES]; ui8i++) {
            if(sBuff[0] == SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][ui8i]) {
				pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '\0'; // cutoff pipe
                // built-in commands
                if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_TEXT_FILES] == true && 
                    TextFilesManager::m_Ptr->ProcessTextFilesCmd(pDcCommand->m_pUser, sBuff+1, true)) {
                    return;
                }
           
                // HubCommands
                if(ui32Len1-pDcCommand->m_pUser->m_ui8NickLen >= 10) {
                    if(HubCommands::DoCommand(pDcCommand->m_pUser, sBuff, ui32Len1, true)) return;
                }
                        
				pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '|'; // add back pipe
                break;
            }
        }
        // PPK ... if i am here is not textfile request or hub command, try opchat
        if(SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true && ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::ALLOWEDOPCHAT) == true && 
            SettingManager::m_Ptr->m_bBotsSameNick == true) {
            uint32_t iOpChatLen = SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_OP_CHAT_NICK];
            memcpy(pTemp-iOpChatLen-2, SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK], iOpChatLen);
			pDcCommand->m_pUser->AddPrcsdCmd(PrcsdUsrCmd::TO_OP_CHAT, pTemp-iOpChatLen-2, pDcCommand->m_ui32CommandLen-((pTemp-iOpChatLen-2)-pDcCommand->m_sCommand), NULL);
        }
    } else if(SettingManager::m_Ptr->m_bBools[SETBOOL_REG_OP_CHAT] == true && 
        ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::ALLOWEDOPCHAT) == true && 
        strcmp(pDcCommand->m_sCommand+5, SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK]) == 0) {
        pTemp += 9+pDcCommand->m_pUser->m_ui8NickLen;
        uint32_t iOpChatLen = SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_OP_CHAT_NICK];
        memcpy(pTemp-iOpChatLen-2, SettingManager::m_Ptr->m_sTexts[SETTXT_OP_CHAT_NICK], iOpChatLen);
		pDcCommand->m_pUser->AddPrcsdCmd(PrcsdUsrCmd::TO_OP_CHAT, pTemp-iOpChatLen-2, (pDcCommand->m_ui32CommandLen-(pTemp-pDcCommand->m_sCommand))+iOpChatLen+2, NULL);
    } else {       
        User *OtherUser = HashManager::m_Ptr->FindUser(pDcCommand->m_sCommand+5, szNickLen);
        // PPK ... pm to yourself ?!? NO!
        if(OtherUser != NULL && OtherUser != pDcCommand->m_pUser && OtherUser->m_ui8State == User::STATE_ADDED) {
            pTemp[0] = ' ';
			pDcCommand->m_pUser->AddPrcsdCmd(PrcsdUsrCmd::CTM_MCTM_RCTM_SR_TO, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, OtherUser, true);
        }
    }
}
//---------------------------------------------------------------------------

// $ValidateNick
void DcCommands::ValidateNick(DcCommand * pDcCommand) {
    if(((pDcCommand->m_pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) == true) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] $ValidateNick with QuickList support from %s (%s) - user closed.", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return;
    }

    if(pDcCommand->m_ui32CommandLen < 16) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Attempt to Validate empty nick (%s) from %s (%s) - user closed.", pDcCommand->m_sCommand, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return;
    }

	pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '\0'; // cutoff pipe
    if(ValidateUserNick(pDcCommand->m_pUser, pDcCommand->m_sCommand+14, pDcCommand->m_ui32CommandLen-15, true) == false) return;

	pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '|'; // add back pipe

	ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::VALIDATENICK_ARRIVAL);
}
//---------------------------------------------------------------------------

// $Version
void DcCommands::Version(DcCommand * pDcCommand) {
    if(pDcCommand->m_ui32CommandLen < 11) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $Version (%s) from %s (%s) - user closed.", pDcCommand->m_sCommand, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return;
    }

	pDcCommand->m_pUser->m_ui8State = User::STATE_GETNICKLIST_OR_MYINFO;

	ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::VERSION_ARRIVAL);

	if(pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
    	return;
    }
    
	pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '\0'; // cutoff pipe
	pDcCommand->m_pUser->SetVersion(pDcCommand->m_sCommand+9);
}
//---------------------------------------------------------------------------

// Chat message
bool DcCommands::ChatDeflood(DcCommand * pDcCommand) {
#ifdef _BUILD_GUI
    if(::SendMessage(MainWindowPageUsersChat::m_Ptr->m_hWndPageItems[MainWindowPageUsersChat::BTN_SHOW_CHAT], BM_GETCHECK, 0, 0) == BST_CHECKED) {
		pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '\0';
        RichEditAppendText(MainWindowPageUsersChat::m_Ptr->m_hWndPageItems[MainWindowPageUsersChat::REDT_CHAT], pDcCommand->m_sCommand);
		pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '|';
    }
#endif
    
	// if the user is sending chat as other user, kick him
	if(pDcCommand->m_sCommand[1+pDcCommand->m_pUser->m_ui8NickLen] != '>' || pDcCommand->m_sCommand[2+pDcCommand->m_pUser->m_ui8NickLen] != ' ' || memcmp(pDcCommand->m_pUser->m_sNick, pDcCommand->m_sCommand+1, pDcCommand->m_pUser->m_ui8NickLen) != 0) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick spoofing in chat from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

		pDcCommand->m_pUser->Close();
		return false;
	}
        
	// PPK ... check flood...
	if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODMAINCHAT) == false) {
		if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION] != 0) {
            if(DeFloodCheckForFlood(pDcCommand->m_pUser, DEFLOOD_CHAT, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION],
				pDcCommand->m_pUser->m_ui16ChatMsgs, pDcCommand->m_pUser->m_ui64ChatMsgsTick, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_MESSAGES],
                (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_TIME]) == true) {
                return false;
            }
		}

		if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION2] != 0) {
            if(DeFloodCheckForFlood(pDcCommand->m_pUser, DEFLOOD_CHAT, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_ACTION2],
				pDcCommand->m_pUser->m_ui16ChatMsgs2, pDcCommand->m_pUser->m_ui64ChatMsgsTick2, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_MESSAGES2],
                (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAIN_CHAT_TIME2]) == true) {
                return false;
            }
		}

		// 2nd check for chatmessage flood
		if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION] != 0) {
			bool bNewData = false;
			if(DeFloodCheckForSameFlood(pDcCommand->m_pUser, DEFLOOD_SAME_CHAT, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MAIN_CHAT_ACTION],
				pDcCommand->m_pUser->m_ui16SameChatMsgs, pDcCommand->m_pUser->m_ui64SameChatsTick,
				SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MAIN_CHAT_MESSAGES], SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MAIN_CHAT_TIME],
				pDcCommand->m_sCommand+pDcCommand->m_pUser->m_ui8NickLen+3, pDcCommand->m_ui32CommandLen-(pDcCommand->m_pUser->m_ui8NickLen+3), pDcCommand->m_pUser->m_sLastChat, pDcCommand->m_pUser->m_ui16LastChatLen, bNewData) == true)
			{
				return false;
			}

			if(bNewData == true) {
				pDcCommand->m_pUser->SetLastChat(pDcCommand->m_sCommand+pDcCommand->m_pUser->m_ui8NickLen+3, pDcCommand->m_ui32CommandLen-(pDcCommand->m_pUser->m_ui8NickLen+3));
			}
        }
    }

    // PPK ... ignore empty chat ;)
    if(pDcCommand->m_ui32CommandLen < pDcCommand->m_pUser->m_ui8NickLen+5u) {
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------

// Chat message
void DcCommands::Chat(DcCommand * pDcCommand) {
    if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOCHATLIMITS) == false) {
    	// PPK ... check for message limit length
 	    if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_CHAT_LEN] != 0 && (pDcCommand->m_ui32CommandLen-pDcCommand->m_pUser->m_ui8NickLen-4) > (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_CHAT_LEN]) {
			pDcCommand->m_pUser->SendFormat("DcCommands::Chat1", true, "<%s> %s !|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_THE_MESSAGE_WAS_TOO_LONG]);
	        return;
 	    }

        // PPK ... check for message lines limit
        if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_CHAT_LINES] != 0 || SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] != 0) {
            if(pDcCommand->m_pUser->m_ui16SameChatMsgs < 2) {
                uint16_t iLines = 1;

                for(uint32_t ui32i = pDcCommand->m_pUser->m_ui8NickLen+3; ui32i < pDcCommand->m_ui32CommandLen-1; ui32i++) {
                    if(pDcCommand->m_sCommand[ui32i] == '\n') {
                        iLines++;
                    }
                }

				pDcCommand->m_pUser->m_ui16LastChatLines = iLines;

                if(pDcCommand->m_pUser->m_ui16LastChatLines > 1) {
					pDcCommand->m_pUser->m_ui16SameMultiChats++;
                }
			} else if(pDcCommand->m_pUser->m_ui16LastChatLines > 1) {
				pDcCommand->m_pUser->m_ui16SameMultiChats++;
            }

			if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NODEFLOODMAINCHAT) == false && SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION] != 0) {
                if(pDcCommand->m_pUser->m_ui16SameMultiChats > SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_MESSAGES] &&
					pDcCommand->m_pUser->m_ui16LastChatLines >= SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_LINES]) {
                    uint16_t lines = 0;
					DeFloodDoAction(pDcCommand->m_pUser, DEFLOOD_SAME_MULTI_CHAT, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_SAME_MULTI_MAIN_CHAT_ACTION], lines, NULL);
					return;
				}
			}

			if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_CHAT_LINES] != 0 && pDcCommand->m_pUser->m_ui16LastChatLines > SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_CHAT_LINES]) {
				pDcCommand->m_pUser->SendFormat("DcCommands::Chat2", true, "<%s> %s !|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_THE_MESSAGE_WAS_TOO_LONG]);
                return;
			}
		}
	}

    if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::NOCHATINTERVAL) == false) {
        if(DeFloodCheckInterval(pDcCommand->m_pUser, INTERVAL_CHAT, pDcCommand->m_pUser->m_ui16ChatIntMsgs, 
			pDcCommand->m_pUser->m_ui64ChatIntMsgsTick, SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CHAT_INTERVAL_MESSAGES],
            (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_CHAT_INTERVAL_TIME]) == true) {
            return;
        }
    }

    if(((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_GAGGED) == User::BIT_GAGGED) == true)
        return;

    void * pQueueItem1 = GlobalDataQueue::m_Ptr->GetLastQueueItem();

	if(ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::CHAT_ARRIVAL) == true ||
		pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
		return;
	}

    void * pQueueItem = NULL;
    void * pQueueItem2 = GlobalDataQueue::m_Ptr->GetLastQueueItem();

    if(pQueueItem1 != pQueueItem2) {
        if(pQueueItem1 == NULL) {
            pQueueItem = GlobalDataQueue::m_Ptr->InsertBlankQueueItem(GlobalDataQueue::m_Ptr->GetFirstQueueItem(), GlobalDataQueue::CMD_CHAT);
        } else {
            pQueueItem = GlobalDataQueue::m_Ptr->InsertBlankQueueItem(pQueueItem1, GlobalDataQueue::CMD_CHAT);
        }

        if(pQueueItem != NULL) {
			pDcCommand->m_pUser->m_ui32BoolBits |= User::BIT_CHAT_INSERT;
        }
    } else if((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_CHAT_INSERT) == User::BIT_CHAT_INSERT) {
        pQueueItem = GlobalDataQueue::m_Ptr->InsertBlankQueueItem(pQueueItem1, GlobalDataQueue::CMD_CHAT);
    }

	// PPK ... filtering kick messages
	if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::KICK) == true) {
    	if(pDcCommand->m_ui32CommandLen > pDcCommand->m_pUser->m_ui8NickLen+21u) {
            char * pTemp = strchr(pDcCommand->m_sCommand+pDcCommand->m_pUser->m_ui8NickLen+3, '\n');
            if(pTemp != NULL) {
            	pTemp[0] = '\0';
            }

            char * pIsKicking = stristr(pDcCommand->m_sCommand+pDcCommand->m_pUser->m_ui8NickLen+3, "is kicking ");
            if(pIsKicking != NULL) {
				char * pBecause = stristr(pIsKicking+12, " because: ");
				if(pBecause != NULL) {
					// PPK ... catch kick message and store for later use in $Kick for tempban reason
					pBecause[0] = '\0';
					User * KickedUser = HashManager::m_Ptr->FindUser(pIsKicking + 11, pBecause - (pIsKicking + 11));
					pBecause[0] = ' ';
					if(KickedUser != NULL) {
						// PPK ... valid kick messages for existing user, remove this message from deflood ;)
						if(pDcCommand->m_pUser->m_ui16ChatMsgs != 0) {
							pDcCommand->m_pUser->m_ui16ChatMsgs--;
							pDcCommand->m_pUser->m_ui16ChatMsgs2--;
						}
						if(pBecause[10] != '|') {
							pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '\0'; // get rid of the pipe
							KickedUser->SetBuffer(pBecause + 10, pDcCommand->m_ui32CommandLen - (pBecause - pDcCommand->m_sCommand) - 11);
							pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen - 1] = '|'; // add back pipe
						}
					}

					if(pTemp != NULL) {
						pTemp[0] = '\n';
					}

					// PPK ... kick messages filtering
					if(SettingManager::m_Ptr->m_bBools[SETBOOL_FILTER_KICK_MESSAGES] == true) {
						if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_KICK_MESSAGES_TO_OPS] == true) {
							if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES_AS_PM] == true) {
								int iRet = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "%s $%s", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], pDcCommand->m_sCommand);
								if(iRet > 0) {
									GlobalDataQueue::m_Ptr->SingleItemStore(ServerManager::m_pGlobalBuffer, iRet, NULL, 0, GlobalDataQueue::SI_PM2OPS);
								}
							}
							else {
								GlobalDataQueue::m_Ptr->AddQueueItem(pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, NULL, 0, GlobalDataQueue::CMD_OPS);
							}
						}
						else {
							pDcCommand->m_pUser->SendCharDelayed(pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen);
						}
						return;
					}
				}
            }

            if(pTemp != NULL) {
                pTemp[0] = '\n';
            }
        }        
	}

	pDcCommand->m_pUser->AddPrcsdCmd(PrcsdUsrCmd::CHAT, pDcCommand->m_sCommand, pDcCommand->m_ui32CommandLen, reinterpret_cast<User *>(pQueueItem));
}
//---------------------------------------------------------------------------

// $Close nick|
void DcCommands::Close(DcCommand * pDcCommand) {
    if(ProfileManager::m_Ptr->IsAllowed(pDcCommand->m_pUser, ProfileManager::CLOSE) == false) {
		pDcCommand->m_pUser->SendFormat("DcCommands::Close1", true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_NOT_ALWD_TO_USE_THIS_CMD]);
        return;
    } 

    if(pDcCommand->m_ui32CommandLen < 9) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $Close (%s) from %s (%s) - user closed.", pDcCommand->m_sCommand, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

		pDcCommand->m_pUser->Close();
        return;
    }

	if(ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::CLOSE_ARRIVAL) == true || pDcCommand->m_pUser->m_ui8State >= User::STATE_CLOSING) {
		return;
	}

	pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '\0'; // cutoff pipe

    User *OtherUser = HashManager::m_Ptr->FindUser(pDcCommand->m_sCommand+7, pDcCommand->m_ui32CommandLen-8);
    if(OtherUser != NULL) {
        // Self-kick
        if(OtherUser == pDcCommand->m_pUser) {
			pDcCommand->m_pUser->SendFormat("DcCommands::Close2", true, "<%s> %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_CANT_CLOSE_YOURSELF]);
            return;
    	}
    	
        if(OtherUser->m_i32Profile != -1 && pDcCommand->m_pUser->m_i32Profile > OtherUser->m_i32Profile) {
			pDcCommand->m_pUser->SendFormat("DcCommands::Close3", true, "<%s> %s %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOU_ARE_NOT_ALLOWED_TO_CLOSE], OtherUser->m_sNick);
        	return;
        }

        // disconnect the user
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) closed by %s", OtherUser->m_sNick, OtherUser->m_sIP, pDcCommand->m_pUser->m_sNick);

        OtherUser->Close();
        
        if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == true) {
            GlobalDataQueue::m_Ptr->StatusMessageFormat("DcCommands::Close", "<%s> *** %s %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], OtherUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], OtherUser->m_sIP, 
				LanguageManager::m_Ptr->m_sTexts[LAN_WAS_CLOSED_BY], pDcCommand->m_pUser->m_sNick);
        }
        
        if(SettingManager::m_Ptr->m_bBools[SETBOOL_SEND_STATUS_MESSAGES] == false || ((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_OPERATOR) == User::BIT_OPERATOR) == false) {
			pDcCommand->m_pUser->SendFormat("DcCommands::Close4", true, "<%s> *** %s %s %s %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], OtherUser->m_sNick, LanguageManager::m_Ptr->m_sTexts[LAN_WITH_IP], OtherUser->m_sIP, LanguageManager::m_Ptr->m_sTexts[LAN_WAS_CLOSED]);
        }
    }
}
//---------------------------------------------------------------------------

void DcCommands::Unknown(DcCommand * pDcCommand, const bool bMyNick/* = false*/) {
	m_ui32StatCmdUnknown++;

    #ifdef _DBG
        Memo(">>> Unimplemented Cmd "+pUser->Nick+" [" + pUser->IP + "]: " + sData);
    #endif

    // if we got unknown command sooner than full login finished
    // PPK ... fixed posibility to send (or flood !!!) hub with unknown command before full login
    // Give him chance with script...
    // if this is unkncwn command and script dont clarify that it's ok, disconnect the user
    if(ScriptManager::m_Ptr->Arrival(pDcCommand, ScriptManager::UNKNOWN_ARRIVAL) == false) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Unknown command from %s (%s) - user closed. (%s)", pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP, pDcCommand->m_sCommand);

		if(bMyNick == true) {
			pDcCommand->m_pUser->SendCharDelayed("$Error CTM2HUB|", 15);
		}

		pDcCommand->m_pUser->Close();
    }
}
//---------------------------------------------------------------------------

bool DcCommands::ValidateUserNick(User * pUser, char * sNick, const size_t szNickLen, const bool ValidateNick) {
    // illegal characters in nick?
    for(uint32_t ui32i = 0; ui32i < szNickLen; ui32i++) {
        switch(sNick[ui32i]) {
            case ' ':
            case '$':
            case '|': {
                pUser->SendFormat("DcCommands::ValidateUserNick1", false, "<%s> %s '%c' ! %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOUR_NICK_CONTAINS_ILLEGAL_CHARACTER], sNick[ui32i], LanguageManager::m_Ptr->m_sTexts[LAN_PLS_CORRECT_IT_AND_GET_BACK_AGAIN]);

//				UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick with bad chars (%s) from %s (%s) - user closed.", Nick, pUser->sNick, pUser->sIP);

                pUser->Close();
                return false;
            }
            default:
                if((unsigned char)sNick[ui32i] < 32) {
                    pUser->SendFormat("DcCommands::ValidateUserNick2", false, "<%s> %s! %s.|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOUR_NICK_CONTAINS_ILLEGAL_WHITE_CHARACTER], LanguageManager::m_Ptr->m_sTexts[LAN_PLS_CORRECT_IT_AND_GET_BACK_AGAIN]);

//					UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick with white chars (%s) from %s (%s) - user closed.", Nick, pUser->sNick, pUser->sIP);

                    pUser->Close();
                    return false;
                }

                continue;
        }
    }

    pUser->SetNick(sNick, (uint8_t)szNickLen);
    
    // check for reserved nicks
    if(ReservedNicksManager::m_Ptr->CheckReserved(pUser->m_sNick, pUser->m_ui32NickHash) == true) {
        pUser->SendFormat("DcCommands::ValidateUserNick3", false, "<%s> %s. %s.|$ValidateDenide %s|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_THE_NICK_IS_RESERVED_FOR_SOMEONE_OTHER], LanguageManager::m_Ptr->m_sTexts[LAN_CHANGE_YOUR_NICK_AND_GET_BACK_AGAIN], sNick);

//		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Reserved nick (%s) from %s (%s) - user closed.", Nick, pUser->sNick, pUser->sIP);

        pUser->Close();
        return false;
    }

    // PPK ... check if we already have ban for this user
    if(pUser->m_pLogInOut->m_pBan != NULL && pUser->m_ui32NickHash == pUser->m_pLogInOut->m_pBan->m_ui32NickHash) {
        pUser->SendChar(pUser->m_pLogInOut->m_pBan->m_sMessage, pUser->m_pLogInOut->m_pBan->m_ui32Len);
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Banned user %s (%s) - user closed.", pUser->m_sNick, pUser->m_sIP);

        pUser->Close();
        return false;
    }

	time_t tmAccTime;
	time(&tmAccTime);

    // check for banned nicks
    BanItem * pBan = BanManager::m_Ptr->FindNick(pUser);
    if(pBan != NULL) {
        int iMsgLen = GenerateBanMessage(pBan, tmAccTime);
        if(iMsgLen != 0) {
        	pUser->SendChar(ServerManager::m_pGlobalBuffer, iMsgLen);
        }

		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Banned user %s (%s) - user closed.", pUser->m_sNick, pUser->m_sIP);

        pUser->Close();
        return false;
    }

    int32_t i32Profile = -1;

    // Nick is ok, check for registered nick
    RegUser *Reg = RegManager::m_Ptr->Find(pUser);
    if(Reg != NULL) {
        if(SettingManager::m_Ptr->m_bBools[SETBOOL_ADVANCED_PASS_PROTECTION] == true && Reg->m_ui8BadPassCount != 0) {
			uint32_t iMinutes2Wait = (uint32_t)pow(2.0, (double)Reg->m_ui8BadPassCount-1);

            if(tmAccTime < (time_t)(Reg->m_tLastBadPass+(60*iMinutes2Wait))) {
                pUser->SendFormat("DcCommands::ValidateUserNick4", false, "<%s> %s %s %s!|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_LAST_PASS_WAS_WRONG_YOU_NEED_WAIT], formatSecTime((Reg->m_tLastBadPass+(60*iMinutes2Wait))- tmAccTime),
					LanguageManager::m_Ptr->m_sTexts[LAN_BEFORE_YOU_TRY_AGAIN]);

				UdpDebug::m_Ptr->BroadcastFormat("[SYS] User %s (%s) not allowed to send password (%" PRIu64 ") - user closed.", pUser->m_sNick, pUser->m_sIP, (uint64_t)((Reg->m_tLastBadPass+(60*iMinutes2Wait))- tmAccTime));

                pUser->Close();
                return false;
            }
        }

        i32Profile = (int32_t)Reg->m_ui16Profile;
    }

    // PPK ... moved IP ban check here, we need to allow registered users on shared IP to log in if not have banned nick, but only IP.
    if(ProfileManager::m_Ptr->IsProfileAllowed(i32Profile, ProfileManager::ENTERIFIPBAN) == false) {
        // PPK ... check if we already have ban for this user
        if(pUser->m_pLogInOut->m_pBan != NULL) {
            pUser->SendChar(pUser->m_pLogInOut->m_pBan->m_sMessage, pUser->m_pLogInOut->m_pBan->m_ui32Len);

			// UdpDebug::m_Ptr->BroadcastFormat("[SYS] uBanned user %s (%s) - user closed.", pUser->sNick, pUser->sIP);

            pUser->Close();
            return false;
        }
    }

    // PPK ... delete user ban if we have it
    if(pUser->m_pLogInOut->m_pBan != NULL) {
        delete pUser->m_pLogInOut->m_pBan;
        pUser->m_pLogInOut->m_pBan = NULL;
    }

    // first check for user limit ! PPK ... allow hublist pinger to check hub any time ;)
    if(ProfileManager::m_Ptr->IsProfileAllowed(i32Profile, ProfileManager::ENTERFULLHUB) == false && ((pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false) {
        // user is NOT allowed enter full hub, check for maxClients
        if(ServerManager::m_ui32Joins-ServerManager::m_ui32Parts > (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_USERS]) {
            pUser->SendFormat("DcCommands::ValidateUserNick5", false, "$HubIsFull|<%s> %s. %u %s.|%s", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_THIS_HUB_IS_FULL], ServerManager::m_ui32Logged, LanguageManager::m_Ptr->m_sTexts[LAN_USERS_ONLINE_LWR],
				(SettingManager::m_Ptr->m_bBools[SETBOOL_REDIRECT_WHEN_HUB_FULL] == true && SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_REDIRECT_ADDRESS] != NULL) ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_REDIRECT_ADDRESS] : "");
//			UdpDebug::m_Ptr->BroadcastFormat("[SYS] Hub full for %s (%s) - user closed.", pUser->sNick, pUser->sIP);

            pUser->Close();
            return false;
        }
    }

    // Check for maximum connections from same IP
    if(ProfileManager::m_Ptr->IsProfileAllowed(i32Profile, ProfileManager::NOUSRSAMEIP) == false) {
        uint32_t ui32Count = HashManager::m_Ptr->GetUserIpCount(pUser);
        if(ui32Count >= (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_CONN_SAME_IP]) {
            pUser->SendFormat("DcCommands::ValidateUserNick6", false, "<%s> %s.|%s", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_SORRY_ALREADY_MAX_IP_CONNS],
				(SettingManager::m_Ptr->m_bBools[SETBOOL_REDIRECT_WHEN_HUB_FULL] == true && SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_REDIRECT_ADDRESS] != NULL) ? SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_REDIRECT_ADDRESS] : "");

			int iMsgLen = snprintf(ServerManager::m_pGlobalBuffer, ServerManager::m_szGlobalBufferSize, "[SYS] Max connections from same IP (%u) for %s (%s) - user closed. ", ui32Count, pUser->m_sNick, pUser->m_sIP);
            if(iMsgLen <= 0) {
	            pUser->Close();
	            return false;
            }

            //UdpDebug::m_Ptr->Broadcast(ServerManager::m_pGlobalBuffer, iMsgLen);
			string tmp(ServerManager::m_pGlobalBuffer, iMsgLen);

            User * cur = NULL,
                * nxt = HashManager::m_Ptr->FindUser(pUser->m_ui128IpHash);

            while(nxt != NULL) {
        		cur = nxt;
                nxt = cur->m_pHashIpTableNext;

                tmp += " "+string(cur->m_sNick, cur->m_ui8NickLen);
            }

            UdpDebug::m_Ptr->Broadcast(tmp.c_str(), tmp.size());

            pUser->Close();
            return false;
        }
    }

    // Check for reconnect time
    if(ProfileManager::m_Ptr->IsProfileAllowed(i32Profile, ProfileManager::NORECONNTIME) == false && Users::m_Ptr->CheckRecTime(pUser) == true) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Fast reconnect from %s (%s) - user closed.", pUser->m_sNick, pUser->m_sIP);

        pUser->Close();
        return false;
    }

    pUser->m_ui8Country = IpP2Country::m_Ptr->Find(pUser->m_ui128IpHash);

    // check for nick in userlist. If taken, check for dupe's socket state
    // if still active, send $ValidateDenide and close()
    User *OtherUser = HashManager::m_Ptr->FindUser(pUser);

    if(OtherUser != NULL) {
   	    if(OtherUser->m_ui8State < User::STATE_CLOSING) {
            // check for socket error, or if user closed connection
            int iRet = recv(OtherUser->m_Socket, ServerManager::m_pGlobalBuffer, 16, MSG_PEEK);

            // if socket error or user closed connection then allow new user to log in
#ifdef _WIN32
            if((iRet == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) || iRet == 0) {
#else
			if((iRet == -1 && errno != EAGAIN) || iRet == 0) {
#endif
                OtherUser->m_ui32BoolBits |= User::BIT_ERROR;

				UdpDebug::m_Ptr->BroadcastFormat("[SYS] Ghost in validate nick %s (%s) - user closed.", OtherUser->m_sNick, OtherUser->m_sIP);

                OtherUser->Close();
                return false;
            } else {
                if(Reg == NULL) {
                    pUser->SendFormat("DcCommands::ValidateUserNick7", false, "$ValidateDenide %s|", sNick);

                    if(strcmp(OtherUser->m_sIP, pUser->m_sIP) != 0 || strcmp(OtherUser->m_sNick, pUser->m_sNick) != 0) {
						UdpDebug::m_Ptr->BroadcastFormat("[SYS] Nick taken [%s (%s)] %s (%s) - user closed.", OtherUser->m_sNick, OtherUser->m_sIP, pUser->m_sNick, pUser->m_sIP);
                    }

                    pUser->Close();
                    return false;
                } else {
                    // PPK ... addition for registered users, kill your own ghost >:-]
                    pUser->m_ui8State = User::STATE_VERSION_OR_MYPASS;
                    pUser->m_ui32BoolBits |= User::BIT_WAITING_FOR_PASS;
                    pUser->AddPrcsdCmd(PrcsdUsrCmd::GETPASS, NULL, 0, NULL);
                    return true;
                }
            }
        }
    }
        
    if(Reg == NULL) {
        // user is NOT registered
        
        // nick length check
        if((SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_NICK_LEN] != 0 && szNickLen < (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MIN_NICK_LEN]) || 
			(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_NICK_LEN] != 0 && szNickLen > (uint32_t)SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MAX_NICK_LEN]))
		{
            pUser->SendChar(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_NICK_LIMIT_MSG], SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_NICK_LIMIT_MSG]);
			// UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad nick length (%d) from %s (%s) - user closed.", (int)szNickLen, pUser->sNick, pUser->sIP);

            pUser->Close();
            return false;
        }

        if(SettingManager::m_Ptr->m_bBools[SETBOOL_REG_ONLY] == true && ((pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) == false) {
            pUser->SendChar(SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_REG_ONLY_MSG], SettingManager::m_Ptr->m_ui16PreTextsLens[SettingManager::SETPRETXT_REG_ONLY_MSG]);

//			UdpDebug::m_Ptr->BroadcastFormat("[SYS] Hub for reg only %s (%s) - user closed.", pUser->sNick, pUser->sIP);

            pUser->Close();
            return false;
        } else {
       	    // hub is public, proceed to Hello
            if(HashManager::m_Ptr->Add(pUser) == false) {
                return false;
            }

            pUser->m_ui32BoolBits |= User::BIT_HASHED;

            if(ValidateNick == true) {
                pUser->m_ui8State = User::STATE_VERSION_OR_MYPASS; // waiting for $Version
                pUser->AddPrcsdCmd(PrcsdUsrCmd::LOGINHELLO, NULL, 0, NULL);
            }
            return true;
        }
    } else {
        // user is registered, wait for password
        if(HashManager::m_Ptr->Add(pUser) == false) {
            return false;
        }

        pUser->m_ui32BoolBits |= User::BIT_HASHED;
        pUser->m_ui8State = User::STATE_VERSION_OR_MYPASS;
        pUser->m_ui32BoolBits |= User::BIT_WAITING_FOR_PASS;
        pUser->AddPrcsdCmd(PrcsdUsrCmd::GETPASS, NULL, 0, NULL);
        return true;
    }
}
//---------------------------------------------------------------------------

DcCommands::PassBf * DcCommands::Find(const uint8_t * ui128IpHash) {
	PassBf * cur = NULL,
        * next = m_pPasswdBfCheck;

    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

		if(memcmp(cur->m_ui128IpHash, ui128IpHash, 16) == 0) {
            return cur;
        }
    }
    return NULL;
}
//---------------------------------------------------------------------------

void DcCommands::Remove(PassBf * pPassBfItem) {
    if(pPassBfItem->m_pPrev == NULL) {
        if(pPassBfItem->m_pNext == NULL) {
			m_pPasswdBfCheck = NULL;
        } else {
            pPassBfItem->m_pNext->m_pPrev = NULL;
			m_pPasswdBfCheck = pPassBfItem->m_pNext;
        }
    } else if(pPassBfItem->m_pNext == NULL) {
        pPassBfItem->m_pPrev->m_pNext = NULL;
    } else {
        pPassBfItem->m_pPrev->m_pNext = pPassBfItem->m_pNext;
        pPassBfItem->m_pNext->m_pPrev = pPassBfItem->m_pPrev;
    }
	delete pPassBfItem;
}
//---------------------------------------------------------------------------

void DcCommands::ProcessCmds(User * pUser) {
    pUser->m_ui32BoolBits &= ~User::BIT_CHAT_INSERT;

    PrcsdUsrCmd * cur = NULL,
        * next = pUser->m_pCmdStrt;
    
    while(next != NULL) {
        cur = next;
        next = cur->m_pNext;

        switch(cur->m_ui8Type) {
            case PrcsdUsrCmd::SUPPORTS: {
                memcpy(ServerManager::m_pGlobalBuffer, "$Supports", 9);
                uint32_t iSupportsLen = 9;
                
                // PPK ... why dc++ have that 0 on end ? that was not in original documentation.. stupid duck...
                if(((pUser->m_ui32SupportBits & User::SUPPORTBIT_ZPIPE0) == User::SUPPORTBIT_ZPIPE0) == true) {
                    memcpy(ServerManager::m_pGlobalBuffer+iSupportsLen, " ZPipe0", 7);
                    iSupportsLen += 7;
                } else if(((pUser->m_ui32SupportBits & User::SUPPORTBIT_ZPIPE) == User::SUPPORTBIT_ZPIPE) == true) {
                    memcpy(ServerManager::m_pGlobalBuffer+iSupportsLen, " ZPipe", 6);
                    iSupportsLen += 6;
                }
                
                // PPK ... yes yes yes finally QuickList support in PtokaX !!! ;))
                if((pUser->m_ui32SupportBits & User::SUPPORTBIT_QUICKLIST) == User::SUPPORTBIT_QUICKLIST) {
                    memcpy(ServerManager::m_pGlobalBuffer+iSupportsLen, " QuickList", 10);
                    iSupportsLen += 10;
                } else if((pUser->m_ui32SupportBits & User::SUPPORTBIT_NOHELLO) == User::SUPPORTBIT_NOHELLO) {
                    // PPK ... Hmmm Client not really need it, but for now send it ;-)
                    memcpy(ServerManager::m_pGlobalBuffer+iSupportsLen, " NoHello", 8);
                    iSupportsLen += 8;
                } else if((pUser->m_ui32SupportBits & User::SUPPORTBIT_NOGETINFO) == User::SUPPORTBIT_NOGETINFO) {
                    // PPK ... if client support NoHello automatically supports NoGetINFO another badwith wasting !
                    memcpy(ServerManager::m_pGlobalBuffer+iSupportsLen, " NoGetINFO", 10);
                    iSupportsLen += 10;
                }

                if((pUser->m_ui32SupportBits & User::SUPPORTBIT_IP64) == User::SUPPORTBIT_IP64) {
                    memcpy(ServerManager::m_pGlobalBuffer+iSupportsLen, " IP64", 5);
                    iSupportsLen += 5;
                }

                if(((pUser->m_ui32SupportBits & User::SUPPORTBIT_IPV4) == User::SUPPORTBIT_IPV4) && ((pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6)) {
                    // Only client connected with IPv6 sending this, so only that client is getting reply
                    memcpy(ServerManager::m_pGlobalBuffer+iSupportsLen, " IPv4", 5);
                    iSupportsLen += 5;
                }

                if((pUser->m_ui32SupportBits & User::SUPPORTBIT_USERCOMMAND) == User::SUPPORTBIT_USERCOMMAND) {
                    memcpy(ServerManager::m_pGlobalBuffer+iSupportsLen, " UserCommand", 12);
                    iSupportsLen += 12;
                }

                if((pUser->m_ui32SupportBits & User::SUPPORTBIT_USERIP2) == User::SUPPORTBIT_USERIP2 && ((pUser->m_ui32BoolBits & User::BIT_QUACK_SUPPORTS) == User::BIT_QUACK_SUPPORTS) == false) {
                    memcpy(ServerManager::m_pGlobalBuffer+iSupportsLen, " UserIP2", 8);
                    iSupportsLen += 8;
                }

                if((pUser->m_ui32SupportBits & User::SUPPORTBIT_TLS2) == User::SUPPORTBIT_TLS2) {
                    memcpy(ServerManager::m_pGlobalBuffer+iSupportsLen, " TLS2", 5);
                    iSupportsLen += 5;
                }

                if((pUser->m_ui32BoolBits & User::BIT_PINGER) == User::BIT_PINGER) {
                    memcpy(ServerManager::m_pGlobalBuffer+iSupportsLen, " BotINFO HubINFO", 16);
                    iSupportsLen += 16;
                }

                memcpy(ServerManager::m_pGlobalBuffer+iSupportsLen, "|\0", 2);
                pUser->SendCharDelayed(ServerManager::m_pGlobalBuffer, iSupportsLen+1);
                break;
            }
            case PrcsdUsrCmd::LOGINHELLO: {
                pUser->SendFormat("DcCommands::ProcessCmds", true, "$Hello %s|", pUser->m_sNick);
                break;
            }
            case PrcsdUsrCmd::GETPASS: {
                uint32_t ui32Len = 9;
                pUser->SendCharDelayed("$GetPass|", ui32Len); // query user for password
                break;
            }
            case PrcsdUsrCmd::CHAT: {
            	// find chat message data
                char *sBuff = cur->m_sCommand+pUser->m_ui8NickLen+3;

            	// non-command chat msg
                bool bNonChat = false;
            	for(uint8_t ui8i = 0; ui8i < (uint8_t)SettingManager::m_Ptr->m_ui16TextsLens[SETTXT_CHAT_COMMANDS_PREFIXES]; ui8i++) {
            	    if(sBuff[0] == SettingManager::m_Ptr->m_sTexts[SETTXT_CHAT_COMMANDS_PREFIXES][ui8i] ) {
                        bNonChat = true;
                	    break;
                    }
            	}

                if(bNonChat == true) {
                    // text files...
                    if(SettingManager::m_Ptr->m_bBools[SETBOOL_ENABLE_TEXT_FILES] == true) {
                        cur->m_sCommand[cur->m_ui32Len-1] = '\0'; // get rid of the pipe
                        
                        if(TextFilesManager::m_Ptr->ProcessTextFilesCmd(pUser, sBuff+1)) {
                            break;
                        }
    
                        cur->m_sCommand[cur->m_ui32Len-1] = '|'; // add back pipe
                    }
    
                    // built-in commands
                    if(cur->m_ui32Len-pUser->m_ui8NickLen >= 9) {
                        if(HubCommands::DoCommand(pUser, sBuff-(pUser->m_ui8NickLen-1), cur->m_ui32Len))
							break;
                        
                        cur->m_sCommand[cur->m_ui32Len-1] = '|'; // add back pipe
                    }
                }
           
            	// everything's ok, let's chat
            	Users::m_Ptr->SendChat2All(pUser, cur->m_sCommand, cur->m_ui32Len, cur->m_pPtr);
            
                break;
            }
            case PrcsdUsrCmd::TO_OP_CHAT: {
                GlobalDataQueue::m_Ptr->SingleItemStore(cur->m_sCommand, cur->m_ui32Len, pUser, 0, GlobalDataQueue::SI_OPCHAT);
                break;
            }
        }

#ifdef _WIN32
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)cur->m_sCommand) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate cur->m_sCommand in DcCommands::ProcessCmds\n");
        }
#else
		free(cur->m_sCommand);
#endif
        cur->m_sCommand = NULL;

        delete cur;
    }
    
    pUser->m_pCmdStrt = NULL;
    pUser->m_pCmdEnd = NULL;

    if((pUser->m_ui32BoolBits & User::BIT_PRCSD_MYINFO) == User::BIT_PRCSD_MYINFO) {
        pUser->m_ui32BoolBits &= ~User::BIT_PRCSD_MYINFO;

        if(((pUser->m_ui32BoolBits & User::BIT_HAVE_BADTAG) == User::BIT_HAVE_BADTAG) == true) {
            pUser->HasSuspiciousTag();
        }

        if(SettingManager::m_Ptr->m_ui8FullMyINFOOption == 0) {
            if(pUser->GenerateMyInfoLong() == false) {
                return;
            }

            Users::m_Ptr->Add2MyInfosTag(pUser);

            if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_DELAY] == 0 || ServerManager::m_ui64ActualTick > ((60*SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_DELAY])+pUser->m_ui64LastMyINFOSendTick)) {
				GlobalDataQueue::m_Ptr->AddQueueItem(pUser->m_sMyInfoLong, pUser->m_ui16MyInfoLongLen, NULL, 0, GlobalDataQueue::CMD_MYINFO);
                pUser->m_ui64LastMyINFOSendTick = ServerManager::m_ui64ActualTick;
            } else {
				GlobalDataQueue::m_Ptr->AddQueueItem(pUser->m_sMyInfoLong, pUser->m_ui16MyInfoLongLen, NULL, 0, GlobalDataQueue::CMD_OPS);
            }

            return;
        } else if(SettingManager::m_Ptr->m_ui8FullMyINFOOption == 2) {
            if(pUser->GenerateMyInfoShort() == false) {
                return;
            }

            Users::m_Ptr->Add2MyInfos(pUser);

            if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_DELAY] == 0 || ServerManager::m_ui64ActualTick > ((60*SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_DELAY])+pUser->m_ui64LastMyINFOSendTick)) {
				GlobalDataQueue::m_Ptr->AddQueueItem(pUser->m_sMyInfoShort, pUser->m_ui16MyInfoShortLen, NULL, 0, GlobalDataQueue::CMD_MYINFO);
                pUser->m_ui64LastMyINFOSendTick = ServerManager::m_ui64ActualTick;
            } else {
				GlobalDataQueue::m_Ptr->AddQueueItem(pUser->m_sMyInfoShort, pUser->m_ui16MyInfoShortLen, NULL, 0, GlobalDataQueue::CMD_OPS);
            }

            return;
		}

		if(pUser->GenerateMyInfoLong() == false) {
			return;
		}

		Users::m_Ptr->Add2MyInfosTag(pUser);

		char * sShortMyINFO = NULL;
		size_t szShortMyINFOLen = 0;

		if(pUser->GenerateMyInfoShort() == true) {
			Users::m_Ptr->Add2MyInfos(pUser);

			if(SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_DELAY] == 0 || ServerManager::m_ui64ActualTick > ((60*SettingManager::m_Ptr->m_i16Shorts[SETSHORT_MYINFO_DELAY])+pUser->m_ui64LastMyINFOSendTick)) {
				sShortMyINFO = pUser->m_sMyInfoShort;
				szShortMyINFOLen = pUser->m_ui16MyInfoShortLen;
				pUser->m_ui64LastMyINFOSendTick = ServerManager::m_ui64ActualTick;
			}
		}

		GlobalDataQueue::m_Ptr->AddQueueItem(sShortMyINFO, szShortMyINFOLen, pUser->m_sMyInfoLong, pUser->m_ui16MyInfoLongLen, GlobalDataQueue::CMD_MYINFO);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void DcCommands::MyNick(DcCommand * pDcCommand) {
    if((pDcCommand->m_pUser->m_ui32BoolBits & User::BIT_IPV6) == User::BIT_IPV6) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] IPv6 $MyNick (%s) from %s (%s) - user closed.", pDcCommand->m_sCommand, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

        Unknown(pDcCommand, true);
        return;
    }

    if(pDcCommand->m_ui32CommandLen < 10) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Short $MyNick (%s) from %s (%s) - user closed.", pDcCommand->m_sCommand, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

        Unknown(pDcCommand, true);
        return;
    }

	pDcCommand->m_sCommand[pDcCommand->m_ui32CommandLen-1] = '\0'; // cutoff pipe

    User * pOtherUser = HashManager::m_Ptr->FindUser(pDcCommand->m_sCommand+8, pDcCommand->m_ui32CommandLen-9);

    if(pOtherUser == NULL || pOtherUser->m_ui8State != User::STATE_IPV4_CHECK) {
		UdpDebug::m_Ptr->BroadcastFormat("[SYS] Bad $MyNick (%s) from %s (%s) - user closed.", pDcCommand->m_sCommand, pDcCommand->m_pUser->m_sNick, pDcCommand->m_pUser->m_sIP);

        Unknown(pDcCommand, true);
        return;
    }

    strcpy(pOtherUser->m_sIPv4, pDcCommand->m_pUser->m_sIP);
    pOtherUser->m_ui8IPv4Len = pDcCommand->m_pUser->m_ui8IpLen;
    pOtherUser->m_ui32BoolBits |= User::BIT_IPV4;

    pOtherUser->m_ui8State = User::STATE_ADDME;

	pDcCommand->m_pUser->Close();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

uint16_t DcCommands::CheckAndGetPort(char * pPort, const uint8_t ui8PortLen, uint32_t &ui32PortLen) {
    uint8_t ui8Index = ui8PortLen-1;
    while(true)
    {
    	if(pPort[ui8Index] == ':')
    	{
    		// We have something which looks like port.. return values
    		ui32PortLen = (ui8PortLen - ui8Index) - 1;
    		return (uint16_t)atoi((pPort + ui8Index) + 1);
		} else if(isdigit(pPort[ui8Index]) == 0) {
			// It is not : and it is not number... this port is not valid
			return 0;
		}

		if(ui8Index == 0) {
			break;
		}

		ui8Index--;
	}

	// Valid port not found
	return 0;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void DcCommands::SendIPFixedMsg(User * pUser, char * pBadIP, char * pRealIP) {
    if((pUser->m_ui32BoolBits & User::BIT_WARNED_WRONG_IP) == User::BIT_WARNED_WRONG_IP) {
        return;
    }

    pUser->SendFormat("DcCommands::SendIPFixedMsg", true, "<%s> %s '%s' %s %s !|", SettingManager::m_Ptr->m_sPreTexts[SettingManager::SETPRETXT_HUB_SEC], LanguageManager::m_Ptr->m_sTexts[LAN_YOUR_CLIENT_SEND_INCORRECT_IP], pBadIP,
        LanguageManager::m_Ptr->m_sTexts[LAN_IN_COMMAND_HUB_REPLACED_IT_WITH_YOUR_REAL_IP], pRealIP);

    pUser->m_ui32BoolBits |= User::BIT_WARNED_WRONG_IP;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PrcsdUsrCmd * DcCommands::AddSearch(User * pUser, PrcsdUsrCmd * pCmdSearch, char * sSearch, const size_t szLen, const bool bActive) {
    if(pCmdSearch != NULL) {
        char * pOldBuf = pCmdSearch->m_sCommand;
#ifdef _WIN32
        pCmdSearch->m_sCommand = (char *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)pOldBuf, pCmdSearch->m_ui32Len+szLen+1);
#else
		pCmdSearch->m_sCommand = (char *)realloc(pOldBuf, pCmdSearch->m_ui32Len+szLen+1);
#endif
        if(pCmdSearch->m_sCommand == NULL) {
            pCmdSearch->m_sCommand = pOldBuf;
            pUser->m_ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

			AppendDebugLogFormat("[MEM] Cannot reallocate %zu bytes for DcCommands::AddSearch1\n", pCmdSearch->m_ui32Len+szLen+1);

            return pCmdSearch;
        }
        memcpy(pCmdSearch->m_sCommand+pCmdSearch->m_ui32Len, sSearch, szLen);
        pCmdSearch->m_ui32Len += (uint32_t)szLen;
        pCmdSearch->m_sCommand[pCmdSearch->m_ui32Len] = '\0';

        if(bActive == true) {
            Users::m_Ptr->m_ui16ActSearchs++;
        } else {
            Users::m_Ptr->m_ui16PasSearchs++;
        }
    } else {
        pCmdSearch = new (std::nothrow) PrcsdUsrCmd();
        if(pCmdSearch == NULL) {
            pUser->m_ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

			AppendDebugLog("%s - [MEM] Cannot allocate new pCmdSearch in DcCommands::AddSearch1\n");
            return pCmdSearch;
        }

#ifdef _WIN32
        pCmdSearch->m_sCommand = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, szLen+1);
#else
		pCmdSearch->m_sCommand = (char *)malloc(szLen+1);
#endif
		if(pCmdSearch->m_sCommand == NULL) {
            delete pCmdSearch;

            pUser->m_ui32BoolBits |= User::BIT_ERROR;
            pUser->Close();

			AppendDebugLogFormat("[MEM] Cannot allocate %zu bytes for DcCommands::Search5\n", szLen+1);

            return NULL;
        }

        memcpy(pCmdSearch->m_sCommand, sSearch, szLen);
        pCmdSearch->m_sCommand[szLen] = '\0';

        pCmdSearch->m_ui32Len = (uint32_t)szLen;

        if(bActive == true) {
            Users::m_Ptr->m_ui16ActSearchs++;
        } else {
            Users::m_Ptr->m_ui16PasSearchs++;
        }
    }

    return pCmdSearch;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
