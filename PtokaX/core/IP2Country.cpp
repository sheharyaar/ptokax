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

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "stdinc.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "IP2Country.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "ServerManager.h"
#include "utility.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
IpP2Country * IpP2Country::m_Ptr = NULL;
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static const char * CountryNames[] = { "Andorra", "United Arab Emirates", "Afghanistan", "Antigua and Barbuda", "Anguilla", "Albania", "Armenia", "Angola", "Antarctica", "Argentina",
    "American Samoa", "Austria", "Australia", "Aruba", "Aland Islands", "Azerbaijan", "Bosnia and Herzegovina", "Barbados", "Bangladesh", "Belgium", "Burkina Faso", "Bulgaria", "Bahrain",
    "Burundi", "Benin", "Saint Barthelemy", "Bermuda", "Brunei Darussalam", "Bolivia", "Bonaire", "Brazil", "Bahamas", "Bhutan", "Bouvet Island", "Botswana", "Belarus", "Belize", "Canada",
    "Cocos (Keeling) Islands", "The Democratic Republic of the Congo", "Central African Republic", "Congo", "Cote D'Ivoire", "Cook Islands", "Chile", "Cameroon", "China", "Colombia",
    "Costa Rica", "Cuba", "Cape Verde", "Curacao", "Christmas Island", "Cyprus", "Czech Republic", "Germany", "Djibouti", "Denmark", "Dominica", "Dominican Republic", "Algeria", "Ecuador",
    "Estonia", "Egypt", "Western Sahara", "Eritrea", "Spain", "Ethiopia", "Finland", "Fiji", "Falkland Islands (Malvinas)", "Micronesia", "Faroe Islands", "France", "Gabon", "United Kingdom",
    "Grenada", "Georgia", "French Guiana", "Guernsey", "Ghana", "Gibraltar", "Greenland", "Gambia", "Guinea", "Guadeloupe", "Equatorial Guinea", "Greece",
    "South Georgia and the South Sandwich Islands", "Guatemala", "Guam", "Guinea-Bissau", "Guyana", "Hong Kong", "Heard Island and McDonald Islands", "Honduras", "Croatia", "Haiti", "Hungary",
    "Switzerland", "Indonesia", "Ireland", "Israel", "Isle of Man", "India", "British Indian Ocean Territory", "Iraq", "Iran", "Iceland", "Italy", "Jersey", "Jamaica", "Jordan", "Japan",
    "Kenya", "Kyrgyzstan", "Cambodia", "Kiribati", "Comoros", "Saint Kitts and Nevis", "Democratic People's Republic of Korea", "Republic of Korea", "Kuwait", "Cayman Islands", "Kazakhstan",
    "Lao People's Democratic Republic", "Lebanon", "Saint Lucia", "Liechtenstein", "Sri Lanka", "Liberia", "Lesotho", "Lithuania", "Luxembourg", "Latvia", "Libyan Arab Jamahiriya", "Morocco",
    "Monaco", "Moldova", "Montenegro", "Saint Martin", "Madagascar", "Marshall Islands", "Macedonia", "Mali", "Myanmar", "Mongolia", "Macao", "Northern Mariana Islands", "Martinique",
    "Mauritania", "Montserrat", "Malta", "Mauritius", "Maldives", "Malawi", "Mexico", "Malaysia", "Mozambique", "Namibia", "New Caledonia", "Niger", "Norfolk Island", "Nigeria", "Nicaragua",
    "Netherlands", "Norway", "Nepal", "Nauru", "Niue", "New Zealand", "Oman", "Panama", "Peru", "French Polynesia", "Papua New Guinea", "Philippines", "Pakistan", "Poland",
    "Saint Pierre and Miquelon", "Pitcairn", "Puerto Rico", "Palestinian Territory", "Portugal", "Palau", "Paraguay", "Qatar", "Reunion", "Romania", "Serbia", "Russian Federation",
    "Rwanda", "Saudi Arabia", "Solomon Islands", "Seychelles", "Sudan", "Sweden", "Singapore", "Saint Helena", "Slovenia", "Svalbard and Jan Mayen", "Slovakia", "Sierra Leone", "San Marino",
    "Senegal", "Somalia", "Suriname", "South Sudan", "Sao Tome and Principe", "El Salvador", "Sint Maarten (Dutch Part)", "Syrian Arab Republic", "Swaziland", "Turks and Caicos Islands", "Chad",
    "French Southern Territories", "Togo", "Thailand", "Tajikistan", "Tokelau", "Timor-Leste", "Turkmenistan", "Tunisia", "Tonga", "Turkey", "Trinidad and Tobago", "Tuvalu", "Taiwan",
    "Tanzania", "Ukraine", "Uganda", "United States Minor Outlying Islands", "United States", "Uruguay", "Uzbekistan", "Holy See (Vatican City State)",
    "Saint Vincent and the Grenadines", "Venezuela", "Virgin Islands, British", "Virgin Islands, U.S.", "Viet Nam", "Vanuatu", "Wallis and Futuna", "Samoa", "Yemen", "Mayotte", "South Africa",
    "Zambia", "Zimbabwe", "Netherlands Antilles", "Unknown (Asia-Pacific)", "Unknown (European Union)", "Unknown",
};
// last updated 25 sep 2011
static const char * CountryCodes[] = { "AD", "AE", "AF", "AG", "AI", "AL", "AM", "AO", "AQ", "AR",
    "AS", "AT", "AU", "AW", "AX", "AZ", "BA", "BB", "BD", "BE", "BF", "BG", "BH",
    "BI", "BJ", "BL", "BM", "BN", "BO", "BQ", "BR", "BS", "BT", "BV", "BW", "BY", "BZ", "CA",
    "CC", "CD", "CF", "CG", "CI", "CK", "CL", "CM", "CN", "CO",
    "CR", "CU", "CV", "CW", "CX", "CY", "CZ", "DE", "DJ", "DK", "DM", "DO", "DZ", "EC",
    "EE", "EG", "EH", "ER", "ES", "ET", "FI", "FJ", "FK", "FM", "FO", "FR", "GA", "GB",
	"GD", "GE", "GF", "GG", "GH", "GI", "GL", "GM", "GN", "GP", "GQ", "GR",
    "GS", "GT", "GU", "GW", "GY", "HK", "HM", "HN", "HR", "HT", "HU",
    "CH", "ID", "IE", "IL", "IM", "IN", "IO", "IQ", "IR", "IS", "IT", "JE", "JM", "JO", "JP",
    "KE", "KG", "KH", "KI", "KM", "KN", "KP", "KR", "KW", "KY", "KZ",
    "LA", "LB", "LC", "LI", "LK", "LR", "LS", "LT", "LU", "LV", "LY", "MA",
    "MC", "MD", "ME", "MF", "MG", "MH", "MK", "ML", "MM", "MN", "MO", "MP", "MQ",
    "MR", "MS", "MT", "MU", "MV", "MW", "MX", "MY", "MZ", "NA", "NC", "NE", "NF", "NG", "NI",
    "NL", "NO", "NP", "NR", "NU", "NZ", "OM", "PA", "PE", "PF", "PG", "PH", "PK", "PL",
    "PM", "PN", "PR", "PS", "PT", "PW", "PY", "QA", "RE", "RO", "RS", "RU",
    "RW", "SA", "SB", "SC", "SD", "SE", "SG", "SH", "SI", "SJ", "SK", "SL", "SM",
    "SN", "SO", "SR", "SS", "ST", "SV", "SX", "SY", "SZ", "TC", "TD",
    "TF", "TG", "TH", "TJ", "TK", "TL", "TM", "TN", "TO", "TR", "TT", "TV", "TW",
	"TZ", "UA", "UG", "UM", "US", "UY", "UZ", "VA",
    "VC", "VE", "VG", "VI", "VN", "VU", "WF", "WS", "YE", "YT", "ZA",
    "ZM", "ZW", "AN", "AP", "EU", "??",
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void IpP2Country::LoadIPv4() {
    if(ServerManager::m_bUseIPv4 == false) {
        return;
    }

#ifdef _WIN32
	FILE * ip2country = fopen((ServerManager::m_sPath + "\\cfg\\IpToCountry.csv").c_str(), "r");
#else
	FILE * ip2country = fopen((ServerManager::m_sPath + "/cfg/IpToCountry.csv").c_str(), "r");
#endif

    if(ip2country == NULL) {
        return;
    }

    if(m_ui32Size == 0) {
		m_ui32Size = 131072;

        if(m_ui32RangeFrom == NULL) {
#ifdef _WIN32
			m_ui32RangeFrom = (uint32_t *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, m_ui32Size * sizeof(uint32_t));
#else
			m_ui32RangeFrom = (uint32_t *)calloc(m_ui32Size, sizeof(uint32_t));
#endif

            if(m_ui32RangeFrom == NULL) {
                AppendDebugLog("%s - [MEM] Cannot create IpP2Country::m_ui32RangeFrom\n");
                fclose(ip2country);
				m_ui32Size = 0;
                return;
            }
        }

        if(m_ui32RangeTo == NULL) {
#ifdef _WIN32
			m_ui32RangeTo = (uint32_t *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, m_ui32Size * sizeof(uint32_t));
#else
			m_ui32RangeTo = (uint32_t *)calloc(m_ui32Size, sizeof(uint32_t));
#endif

            if(m_ui32RangeTo == NULL) {
                AppendDebugLog("%s - [MEM] Cannot create IpP2Country::m_ui32RangeTo\n");
                fclose(ip2country);
				m_ui32Size = 0;
                return;
            }
        }

        if(m_ui8RangeCI == NULL) {
#ifdef _WIN32
			m_ui8RangeCI = (uint8_t *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, m_ui32Size * sizeof(uint8_t));
#else
			m_ui8RangeCI = (uint8_t *)calloc(m_ui32Size, sizeof(uint8_t));
#endif

            if(m_ui8RangeCI == NULL) {
                AppendDebugLog("%s - [MEM] Cannot create IpP2Country::m_ui8RangeCI\n");
                fclose(ip2country);
				m_ui32Size = 0;
                return;
            }
        }
    }

    char sLine[1024];

    while(fgets(sLine, 1024, ip2country) != NULL) {
        if(sLine[0] != '\"') {
            continue;
        }

        if(m_ui32Count == m_ui32Size) {
			m_ui32Size += 512;
            void * oldbuf = m_ui32RangeFrom;
#ifdef _WIN32
			m_ui32RangeFrom = (uint32_t *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, m_ui32Size * sizeof(uint32_t));
#else
			m_ui32RangeFrom = (uint32_t *)realloc(oldbuf, m_ui32Size * sizeof(uint32_t));
#endif
            if(m_ui32RangeFrom == NULL) {
    			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for m_ui32RangeFrom\n", m_ui32Size);
				m_ui32RangeFrom = (uint32_t *)oldbuf;
    			fclose(ip2country);
                return;
    		}

            oldbuf = m_ui32RangeTo;
#ifdef _WIN32
			m_ui32RangeTo = (uint32_t *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, m_ui32Size * sizeof(uint32_t));
#else
			m_ui32RangeTo = (uint32_t *)realloc(oldbuf, m_ui32Size * sizeof(uint32_t));
#endif
            if(m_ui32RangeTo == NULL) {
    			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for m_ui32RangeTo\n", m_ui32Size);
				m_ui32RangeTo = (uint32_t *)oldbuf;
    			fclose(ip2country);
                return;
    		}

            oldbuf = m_ui8RangeCI;
#ifdef _WIN32
			m_ui8RangeCI = (uint8_t *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, m_ui32Size * sizeof(uint8_t));
#else
			m_ui8RangeCI = (uint8_t *)realloc(oldbuf, m_ui32Size * sizeof(uint8_t));
#endif
            if(m_ui8RangeCI == NULL) {
    			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for m_ui8RangeCI\n", m_ui32Size);
				m_ui8RangeCI = (uint8_t *)oldbuf;
    			fclose(ip2country);
                return;
    		}
        }

        char * sStart = sLine+1;
        uint8_t ui8d = 0;

        size_t szLineLen = strlen(sLine);

		for(size_t szi = 1; szi < szLineLen; szi++) {
            if(sLine[szi] == '\"') {
                sLine[szi] = '\0';
                if(ui8d == 0) {
					m_ui32RangeFrom[m_ui32Count] = strtoul(sStart, NULL, 10);
                } else if(ui8d == 1) {
					m_ui32RangeTo[m_ui32Count] = strtoul(sStart, NULL, 10);
                } else if(ui8d == 4) {
                    for(uint8_t ui8i = 0; ui8i < 252; ui8i++) {
                        if(*((uint16_t *)CountryCodes[ui8i]) == *((uint16_t *)sStart)) {
							m_ui8RangeCI[m_ui32Count] = ui8i;
							m_ui32Count++;
                            break;
                        }
                    }

                    break;
                }

                ui8d++;
                szi += (uint16_t)2;
                sStart = sLine+szi+1;

            }
        }
    }

	fclose(ip2country);

    if(m_ui32Count < m_ui32Size) {
		m_ui32Size = m_ui32Count;

        void * oldbuf = m_ui32RangeFrom;
#ifdef _WIN32
		m_ui32RangeFrom = (uint32_t *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, m_ui32Size * sizeof(uint32_t));
#else
		m_ui32RangeFrom = (uint32_t *)realloc(oldbuf, m_ui32Size * sizeof(uint32_t));
#endif
        if(m_ui32RangeFrom == NULL) {
    		AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for ui32RangeFrom\n", m_ui32Size);
			m_ui32RangeFrom = (uint32_t *)oldbuf;
    	}

        oldbuf = m_ui32RangeTo;
#ifdef _WIN32
		m_ui32RangeTo = (uint32_t *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, m_ui32Size * sizeof(uint32_t));
#else
		m_ui32RangeTo = (uint32_t *)realloc(oldbuf, m_ui32Size * sizeof(uint32_t));
#endif
        if(m_ui32RangeTo == NULL) {
    		AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for m_ui32RangeTo\n", m_ui32Size);
			m_ui32RangeTo = (uint32_t *)oldbuf;
    	}

        oldbuf = m_ui8RangeCI;
#ifdef _WIN32
		m_ui8RangeCI = (uint8_t *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, m_ui32Size * sizeof(uint8_t));
#else
		m_ui8RangeCI = (uint8_t *)realloc(oldbuf, m_ui32Size * sizeof(uint8_t));
#endif
        if(m_ui8RangeCI == NULL) {
    		AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for m_ui8RangeCI\n", m_ui32Size);
			m_ui8RangeCI = (uint8_t *)oldbuf;
    	}
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void IpP2Country::LoadIPv6() {
    if(ServerManager::m_bUseIPv6 == false) {
        return;
    }

#ifdef _WIN32
	FILE * ip2country = fopen((ServerManager::m_sPath + "\\cfg\\IpToCountry.6R.csv").c_str(), "r");
#else
	FILE * ip2country = fopen((ServerManager::m_sPath + "/cfg/IpToCountry.6R.csv").c_str(), "r");
#endif

    if(ip2country == NULL) {
        return;
    }

    if(m_ui32IPv6Size == 0) {
		m_ui32IPv6Size = 16384;

        if(m_ui128IPv6RangeFrom == NULL) {
#ifdef _WIN32
			m_ui128IPv6RangeFrom = (uint8_t *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, m_ui32IPv6Size * (sizeof(uint8_t)*16));
#else
			m_ui128IPv6RangeFrom = (uint8_t *)calloc(m_ui32IPv6Size, sizeof(uint8_t) * 16);
#endif

            if(m_ui128IPv6RangeFrom == NULL) {
                AppendDebugLog("%s - [MEM] Cannot create IpP2Country::m_ui128IPv6RangeFrom\n");
                fclose(ip2country);
				m_ui32IPv6Size = 0;
                return;
            }
        }

        if(m_ui128IPv6RangeTo == NULL) {
#ifdef _WIN32
			m_ui128IPv6RangeTo = (uint8_t *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, m_ui32IPv6Size * (sizeof(uint8_t)*16));
#else
			m_ui128IPv6RangeTo = (uint8_t *)calloc(m_ui32IPv6Size, sizeof(uint8_t) * 16);
#endif

            if(m_ui128IPv6RangeTo == NULL) {
                AppendDebugLog("%s - [MEM] Cannot create IpP2Country::m_ui128IPv6RangeTo\n");
                fclose(ip2country);
				m_ui32IPv6Size = 0;
                return;
            }
        }

        if(m_ui8IPv6RangeCI == NULL) {
#ifdef _WIN32
			m_ui8IPv6RangeCI = (uint8_t *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, m_ui32IPv6Size * sizeof(uint8_t));
#else
			m_ui8IPv6RangeCI = (uint8_t *)calloc(m_ui32IPv6Size, sizeof(uint8_t));
#endif

            if(m_ui8IPv6RangeCI == NULL) {
                AppendDebugLog("%s - [MEM] Cannot create IpP2Country::m_ui8IPv6RangeCI\n");
                fclose(ip2country);
				m_ui32IPv6Size = 0;
                return;
            }
        }
    }

    char sLine[1024];

    while(fgets(sLine, 1024, ip2country) != NULL) {
        if(sLine[0] == '#' || sLine[0] < 32) {
            continue;
        }

        if(m_ui32IPv6Count == m_ui32IPv6Size) {
			m_ui32IPv6Size += 512;
            void * oldbuf = m_ui128IPv6RangeFrom;
#ifdef _WIN32
			m_ui128IPv6RangeFrom = (uint8_t *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, m_ui32IPv6Size * (sizeof(uint8_t)*16));
#else
			m_ui128IPv6RangeFrom = (uint8_t *)realloc(oldbuf, m_ui32IPv6Size * (sizeof(uint8_t)*16));
#endif
            if(m_ui128IPv6RangeFrom == NULL) {
    			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for m_ui128IPv6RangeFrom\n", m_ui32IPv6Size);
				m_ui128IPv6RangeFrom = (uint8_t *)oldbuf;
    			fclose(ip2country);
                return;
    		}

            oldbuf = m_ui128IPv6RangeTo;
#ifdef _WIN32
			m_ui128IPv6RangeTo = (uint8_t *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, m_ui32IPv6Size * (sizeof(uint8_t)*16));
#else
			m_ui128IPv6RangeTo = (uint8_t *)realloc(oldbuf, m_ui32IPv6Size * (sizeof(uint8_t)*16));
#endif
            if(m_ui128IPv6RangeTo == NULL) {
    			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for ui128IPv6RangeTo\n", m_ui32IPv6Size);
				m_ui128IPv6RangeTo = (uint8_t *)oldbuf;
    			fclose(ip2country);
                return;
    		}

            oldbuf = m_ui8IPv6RangeCI;
#ifdef _WIN32
			m_ui8IPv6RangeCI = (uint8_t *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, m_ui32IPv6Size * sizeof(uint8_t));
#else
			m_ui8IPv6RangeCI = (uint8_t *)realloc(oldbuf, m_ui32IPv6Size * sizeof(uint8_t));
#endif
            if(m_ui8IPv6RangeCI == NULL) {
    			AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for ui8IPv6RangeCI\n", m_ui32IPv6Size);
				m_ui8IPv6RangeCI = (uint8_t *)oldbuf;
    			fclose(ip2country);
                return;
    		}
        }

        char * sStart = sLine;
        uint8_t ui8d = 0;

        size_t szLineLen = strlen(sLine);

		for(size_t szi = 0; szi < szLineLen; szi++) {
            if(ui8d == 0 && sLine[szi] == '-') {
                sLine[szi] = '\0';
#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
                win_inet_pton(sStart, m_ui128IPv6RangeFrom + (m_ui32IPv6Count*16));
#else
                inet_pton(AF_INET6, sStart, m_ui128IPv6RangeFrom + (m_ui32IPv6Count*16));
#endif
            } else if(sLine[szi] == ',') {
                sLine[szi] = '\0';
                if(ui8d == 1) {
#if defined(_WIN32) && !defined(_WIN64) && !defined(_WIN_IOT)
                    win_inet_pton(sStart, m_ui128IPv6RangeTo + (m_ui32IPv6Count*16));
#else
                    inet_pton(AF_INET6, sStart, m_ui128IPv6RangeTo + (m_ui32IPv6Count*16));
#endif
                } else {
                    for(uint8_t ui8i = 0; ui8i < 252; ui8i++) {
                        if(*((uint16_t *)CountryCodes[ui8i]) == *((uint16_t *)sStart)) {
							m_ui8IPv6RangeCI[m_ui32IPv6Count] = ui8i;
							m_ui32IPv6Count++;

                            break;
                        }
                    }

                    break;
                }
            } else {
                continue;
            }

            ui8d++;
            sStart = sLine+szi+1;
        }
    }

	fclose(ip2country);

    if(m_ui32IPv6Count < m_ui32IPv6Size) {
		m_ui32IPv6Size = m_ui32IPv6Count;

        void * oldbuf = m_ui128IPv6RangeFrom;
#ifdef _WIN32
		m_ui128IPv6RangeFrom = (uint8_t *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, m_ui32IPv6Size * (sizeof(uint8_t)*16));
#else
		m_ui128IPv6RangeFrom = (uint8_t *)realloc(oldbuf, m_ui32IPv6Size * (sizeof(uint8_t)*16));
#endif
        if(m_ui128IPv6RangeFrom == NULL) {
    		AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for ui128IPv6RangeFrom\n", m_ui32IPv6Size);
			m_ui128IPv6RangeFrom = (uint8_t *)oldbuf;
    	}

        oldbuf = m_ui128IPv6RangeTo;
#ifdef _WIN32
		m_ui128IPv6RangeTo = (uint8_t *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, m_ui32IPv6Size * (sizeof(uint8_t)*16));
#else
		m_ui128IPv6RangeTo = (uint8_t *)realloc(oldbuf, m_ui32IPv6Size * (sizeof(uint8_t)*16));
#endif
        if(m_ui128IPv6RangeTo == NULL) {
    		AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for m_ui128IPv6RangeTo\n", m_ui32IPv6Size);
			m_ui128IPv6RangeTo = (uint8_t *)oldbuf;
    	}

        oldbuf = m_ui8IPv6RangeCI;
#ifdef _WIN32
		m_ui8IPv6RangeCI = (uint8_t *)HeapReAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)oldbuf, m_ui32IPv6Size * sizeof(uint8_t));
#else
		m_ui8IPv6RangeCI = (uint8_t *)realloc(oldbuf, m_ui32IPv6Size * sizeof(uint8_t));
#endif
        if(m_ui8IPv6RangeCI == NULL) {
    		AppendDebugLogFormat("[MEM] Cannot reallocate %u bytes in IpP2Country::IpP2Country for m_ui8IPv6RangeCI\n", m_ui32IPv6Size);
			m_ui8IPv6RangeCI = (uint8_t *)oldbuf;
    	}
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

IpP2Country::IpP2Country() : m_ui32RangeFrom(NULL), m_ui32RangeTo(NULL), m_ui8RangeCI(NULL), m_ui8IPv6RangeCI(NULL), m_ui128IPv6RangeFrom(NULL), m_ui128IPv6RangeTo(NULL), m_ui32Size(0), m_ui32IPv6Size(0), m_ui32Count(0), m_ui32IPv6Count(0) {
    LoadIPv4();
    LoadIPv6();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
IpP2Country::~IpP2Country() {
#ifdef _WIN32
    if(m_ui32RangeFrom != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_ui32RangeFrom) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate IpP2Country::m_ui32RangeFrom in IpP2Country::~IpP2Country\n");
        }
    }
#else
	free(m_ui32RangeFrom);
#endif

#ifdef _WIN32
    if(m_ui32RangeTo != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_ui32RangeTo) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate IpP2Country::m_ui32RangeTo in IpP2Country::~IpP2Country\n");
        }
    }
#else
	free(m_ui32RangeTo);
#endif

#ifdef _WIN32
    if(m_ui8RangeCI != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_ui8RangeCI) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate IpP2Country::m_ui8RangeCI in IpP2Country::~IpP2Country\n");
        }
    }
#else
	free(m_ui8RangeCI);
#endif

#ifdef _WIN32
    if(m_ui128IPv6RangeFrom != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_ui128IPv6RangeFrom) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate IpP2Country::m_ui128IPv6RangeFrom in IpP2Country::~IpP2Country\n");
        }
    }
#else
	free(m_ui128IPv6RangeFrom);
#endif

#ifdef _WIN32
    if(m_ui128IPv6RangeTo != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_ui128IPv6RangeTo) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate IpP2Country::m_ui128IPv6RangeTo in IpP2Country::~IpP2Country\n");
        }
    }
#else
	free(m_ui128IPv6RangeTo);
#endif

#ifdef _WIN32
    if(m_ui8IPv6RangeCI != NULL) {
        if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_ui8IPv6RangeCI) == 0) {
            AppendDebugLog("%s - [MEM] Cannot deallocate IpP2Country::m_ui8IPv6RangeCI in IpP2Country::~IpP2Country\n");
        }
    }
#else
	free(m_ui8IPv6RangeCI);
#endif
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

const char * IpP2Country::Find(const uint8_t * ui128IpHash, const bool bCountryName) {
    bool bIPv4 = false;
    uint32_t ui32IpHash = 0;

    if(ServerManager::m_bUseIPv6 == false || IN6_IS_ADDR_V4MAPPED((in6_addr *)ui128IpHash)) {
        bIPv4 = true;

        ui32IpHash = 16777216 * ui128IpHash[12] + 65536 * ui128IpHash[13] + 256 * ui128IpHash[14] + ui128IpHash[15];
    }

    if(bIPv4 == false) {
        if(ui128IpHash[0] == 32 && ui128IpHash[1] == 2) { // 6to4 tunnel
            bIPv4 = true;

            ui32IpHash = 16777216 * ui128IpHash[2] + 65536 * ui128IpHash[3] + 256 * ui128IpHash[4] + ui128IpHash[5];
        } else if(ui128IpHash[0] == 32 && ui128IpHash[1] == 1 && ui128IpHash[2] == 0 && ui128IpHash[3] == 0) { // teredo tunnel
            bIPv4 = true;

            ui32IpHash = (16777216 * ui128IpHash[12] + 65536 * ui128IpHash[13] + 256 * ui128IpHash[14] + ui128IpHash[15]) ^ 0xffffffff;
        }
    }

    if(bIPv4 == true) {
        for(uint32_t ui32i = 0; ui32i < m_ui32Count; ui32i++) {
            if(m_ui32RangeFrom[ui32i] <= ui32IpHash && m_ui32RangeTo[ui32i] >= ui32IpHash) {
                if(bCountryName == false) {
                    return CountryCodes[m_ui8RangeCI[ui32i]];
                } else {
                    return CountryNames[m_ui8RangeCI[ui32i]];
                }
            }
        }
    } else {
        for(uint32_t ui32i = 0; ui32i < m_ui32IPv6Count; ui32i++) {
            if(memcmp(m_ui128IPv6RangeFrom+(ui32i*16), ui128IpHash, 16) <= 0 && memcmp(m_ui128IPv6RangeTo+(ui32i*16), ui128IpHash, 16) >= 0) {
                if(bCountryName == false) {
                    return CountryCodes[m_ui8IPv6RangeCI[ui32i]];
                } else {
                    return CountryNames[m_ui8IPv6RangeCI[ui32i]];
                }
            }
        }
    }

    if(bCountryName == false) {
        return CountryCodes[252];
    } else {
        return CountryNames[252];
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

uint8_t IpP2Country::Find(const uint8_t * ui128IpHash) {
    bool bIPv4 = false;
    uint32_t ui32IpHash = 0;

    if(ServerManager::m_bUseIPv6 == false || IN6_IS_ADDR_V4MAPPED((in6_addr *)ui128IpHash)) {
        bIPv4 = true;

        ui32IpHash = 16777216 * ui128IpHash[12] + 65536 * ui128IpHash[13] + 256 * ui128IpHash[14] + ui128IpHash[15];
    }

    if(bIPv4 == false) {
        if(ui128IpHash[0] == 32 && ui128IpHash[1] == 2) { // 6to4 tunnel
            bIPv4 = true;

            ui32IpHash = 16777216 * ui128IpHash[2] + 65536 * ui128IpHash[3] + 256 * ui128IpHash[4] + ui128IpHash[5];
        } else if(ui128IpHash[0] == 32 && ui128IpHash[1] == 1 && ui128IpHash[2] == 0 && ui128IpHash[3] == 0) { // teredo tunnel
            bIPv4 = true;

            ui32IpHash = (16777216 * ui128IpHash[12] + 65536 * ui128IpHash[13] + 256 * ui128IpHash[14] + ui128IpHash[15]) ^ 0xffffffff;
        }
    }

    if(bIPv4 == true) {
        for(uint32_t ui32i = 0; ui32i < m_ui32Count; ui32i++) {
            if(m_ui32RangeFrom[ui32i] <= ui32IpHash && m_ui32RangeTo[ui32i] >= ui32IpHash) {
                return m_ui8RangeCI[ui32i];
            }
        }
    } else {
        for(uint32_t ui32i = 0; ui32i < m_ui32IPv6Count; ui32i++) {
            if(memcmp(m_ui128IPv6RangeFrom+(ui32i*16), ui128IpHash, 16) <= 0 && memcmp(m_ui128IPv6RangeTo+(ui32i*16), ui128IpHash, 16) >= 0) {
                return m_ui8IPv6RangeCI[ui32i];
            }
        }
    }

    return 252;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

const char * IpP2Country::GetCountry(const uint8_t ui8dx, const bool bCountryName) {
    if(bCountryName == false) {
        return CountryCodes[ui8dx];
    } else {
        return CountryNames[ui8dx];
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void IpP2Country::Reload() {
	m_ui32Count = 0;
    LoadIPv4();

	m_ui32IPv6Count = 0;
    LoadIPv6();
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
