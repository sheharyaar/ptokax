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
#include "PXBReader.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "ServerManager.h"
#include "utility.h"
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _WIN32
	#pragma hdrstop
#endif
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PXBReader::PXBReader() : m_pFile(NULL), m_pActualPosition(NULL), m_szRemainingSize(0), m_ui8AllocatedSize(0), m_bFullRead(false), m_pItemDatas(NULL), m_ui16ItemLengths(NULL), m_sItemIdentifiers(NULL), m_ui8ItemValues(NULL) {
	// ...
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
PXBReader::~PXBReader() {
#ifdef _WIN32
        if(m_pItemDatas != NULL) {
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, m_pItemDatas) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate m_pItemDatas in PXBReader::~PXBReader\n");
            }
        }
#else
		free(m_pItemDatas);
#endif

#ifdef _WIN32
        if(m_ui16ItemLengths != NULL) {
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_ui16ItemLengths) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate m_ui16ItemLengths in PXBReader::~PXBReader\n");
            }
        }
#else
		free(m_ui16ItemLengths);
#endif

#ifdef _WIN32
        if(m_sItemIdentifiers != NULL) {
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_sItemIdentifiers) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate m_sItemIdentifiers in PXBReader::~PXBReader\n");
            }
        }
#else
		free(m_sItemIdentifiers);
#endif

#ifdef _WIN32
        if(m_ui8ItemValues != NULL) {
            if(HeapFree(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE, (void *)m_ui8ItemValues) == 0) {
                AppendDebugLog("%s - [MEM] Cannot deallocate m_ui8ItemValues in PXBReader::~PXBReader\n");
            }
        }
#else
		free(m_ui8ItemValues);
#endif

    if(m_pFile != NULL) {
        fclose(m_pFile);
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::OpenFileRead(const char * sFilename, const uint8_t ui8SubItems) {
	if(PrepareArrays(ui8SubItems) == false) {
		return false;
	}

	m_pFile = fopen(sFilename, "rb");

    if(m_pFile == NULL) {
        return false;
    }

    fseek(m_pFile, 0, SEEK_END);
    long lFileLen = ftell(m_pFile);

    if(lFileLen <= 0) {
        return false;
    }

    fseek(m_pFile, 0, SEEK_SET);

	m_szRemainingSize = 131072;

    if((size_t)lFileLen < m_szRemainingSize) {
		m_szRemainingSize = lFileLen;

		m_bFullRead = true;
    }

    if(fread(ServerManager::m_pGlobalBuffer, 1, m_szRemainingSize, m_pFile) != m_szRemainingSize) {
        return false;
    }

	m_pActualPosition = ServerManager::m_pGlobalBuffer;

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void PXBReader::ReadNextFilePart() {
    memmove(ServerManager::m_pGlobalBuffer, m_pActualPosition, m_szRemainingSize);

    size_t szReadSize = fread(ServerManager::m_pGlobalBuffer + m_szRemainingSize, 1, 131072 - m_szRemainingSize, m_pFile);

    if(szReadSize != (131072 - m_szRemainingSize)) {
		m_bFullRead = true;
    }

	m_pActualPosition = ServerManager::m_pGlobalBuffer;
	m_szRemainingSize += szReadSize;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::ReadNextItem(const uint16_t * pExpectedIdentificators, const uint8_t ui8ExpectedSubItems, const uint8_t ui8ExtraSubItems/* = 0*/) {
    if(m_szRemainingSize == 0) {
        return false;
    }

    memset(m_pItemDatas, 0, m_ui8AllocatedSize*sizeof(void *));
    memset(m_ui16ItemLengths, 0, m_ui8AllocatedSize*sizeof(uint16_t));

    if(m_szRemainingSize < 4) {
        if(m_bFullRead == true) {
            return false;
        } else { // read next part of file
            ReadNextFilePart();

            if(m_szRemainingSize < 4) {
                return false;
            }
        }
    }

    uint32_t ui32ItemSize = ntohl(*((uint32_t *)m_pActualPosition));

    if(ui32ItemSize > m_szRemainingSize) {
        if(m_bFullRead == true) {
            return false;
        } else { // read next part of file
            ReadNextFilePart();

            if(ui32ItemSize > m_szRemainingSize) {
                return false;
            }
        }
    }

	m_pActualPosition += 4;
	m_szRemainingSize -= 4;
    ui32ItemSize -= 4;

    uint8_t ui8ActualItem = 0;

    uint16_t ui16SubItemSize = 0;

    while(ui32ItemSize > 0) {
        ui16SubItemSize = ntohs(*((uint16_t *)m_pActualPosition));

        if(ui16SubItemSize > ui32ItemSize) {
            return false;
        }

        if(ui8ActualItem < ui8ExpectedSubItems && pExpectedIdentificators[ui8ActualItem] == *((uint16_t *)(m_pActualPosition+2))) {
			m_ui16ItemLengths[ui8ActualItem] = (ui16SubItemSize - 4);
			m_pItemDatas[ui8ActualItem] = (m_pActualPosition + 4);

            ui8ActualItem++;
        } else { // for compatibility with newer versions...
            for(uint8_t ui8i = 0; ui8i < (ui8ExpectedSubItems + ui8ExtraSubItems); ui8i++) {
                if(pExpectedIdentificators[ui8i] == *((uint16_t *)(m_pActualPosition+2))) {
					m_ui16ItemLengths[ui8i] = (ui16SubItemSize - 4);
					m_pItemDatas[ui8i] = (m_pActualPosition + 4);

                    ui8ActualItem++;
                }
            }
        }

		m_pActualPosition += ui16SubItemSize;
		m_szRemainingSize -= ui16SubItemSize;
        ui32ItemSize -= ui16SubItemSize;
    }

    if(ui8ActualItem != ui8ExpectedSubItems) {
        return false;
    } else {
        return true;
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::OpenFileSave(const char * sFilename, const uint8_t ui8Size) {
	if(PrepareArrays(ui8Size) == false) {
		return false;
	}

	m_pFile = fopen(sFilename, "wb");

    if(m_pFile == NULL) {
        return false;
    }

	m_szRemainingSize = 131072;

	m_pActualPosition = ServerManager::m_pGlobalBuffer;

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::WriteNextItem(const uint32_t ui32Length, const uint8_t ui8SubItems) {
    uint32_t ui32ItemLength = ui32Length + 4 + (4 * ui8SubItems);

    if(ui32ItemLength > m_szRemainingSize) {
        fwrite(ServerManager::m_pGlobalBuffer, 1, m_pActualPosition-ServerManager::m_pGlobalBuffer, m_pFile);
		m_pActualPosition = ServerManager::m_pGlobalBuffer;
		m_szRemainingSize = 131072;
    }

    (*((uint32_t *)m_pActualPosition)) = htonl(ui32ItemLength);
	m_pActualPosition += 4;
	m_szRemainingSize -= 4;

    for(uint8_t ui8i = 0; ui8i < ui8SubItems; ui8i++) {
        (*((uint16_t *)(m_pActualPosition))) = htons(m_ui16ItemLengths[ui8i] + 4);

		m_pActualPosition[2] = m_sItemIdentifiers[(ui8i*2)];
		m_pActualPosition[3] = m_sItemIdentifiers[(ui8i*2)+1];

        switch(m_ui8ItemValues[ui8i]) {
            case PXB_BYTE:
				m_pActualPosition[4] = (m_pItemDatas[ui8i] == 0 ? '0' : '1');
                break;
            case PXB_TWO_BYTES:
                (*((uint16_t *)(m_pActualPosition+4))) = htons(*((uint16_t *)m_pItemDatas[ui8i]));
                break;
            case PXB_FOUR_BYTES:
                (*((uint32_t *)(m_pActualPosition+4))) = htonl(*((uint32_t *)m_pItemDatas[ui8i]));
                break;
            case PXB_EIGHT_BYTES:
            	(*((uint64_t *)(m_pActualPosition+4))) = htobe64(*((uint64_t *)m_pItemDatas[ui8i]));
            	break;
            case PXB_STRING:
                memcpy(m_pActualPosition+4, m_pItemDatas[ui8i], m_ui16ItemLengths[ui8i]);
                break;
            default:
                break;
        }

		m_pActualPosition += m_ui16ItemLengths[ui8i] + 4;
		m_szRemainingSize -= m_ui16ItemLengths[ui8i] + 4;
    }

    return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void PXBReader::WriteRemaining() {
    if((m_pActualPosition-ServerManager::m_pGlobalBuffer) > 0) {
        fwrite(ServerManager::m_pGlobalBuffer, 1, m_pActualPosition-ServerManager::m_pGlobalBuffer, m_pFile);
    }

    fclose(m_pFile);
	m_pFile = NULL;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PXBReader::PrepareArrays(const uint8_t ui8Size) {
#ifdef _WIN32
	m_pItemDatas = (void **)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ui8Size*sizeof(void *));
#else
	m_pItemDatas = (void **)calloc(ui8Size, sizeof(void *));
#endif
    if(m_pItemDatas == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create pItemDatas in PXBReader::PrepareArrays\n");
		return false;
    }

#ifdef _WIN32
	m_ui16ItemLengths = (uint16_t *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ui8Size*sizeof(uint16_t));
#else
	m_ui16ItemLengths = (uint16_t *)calloc(ui8Size, sizeof(uint16_t));
#endif
    if(m_ui16ItemLengths == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create ui16ItemLengths in PXBReader::PrepareArrays\n");
		return false;
    }

#ifdef _WIN32
	m_sItemIdentifiers = (char *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ui8Size*(sizeof(char)*2));
#else
	m_sItemIdentifiers = (char *)calloc(ui8Size, sizeof(char)*2);
#endif
    if(m_sItemIdentifiers == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create sItemIdentifiers in PXBReader::PrepareArrays\n");
		return false;
    }

#ifdef _WIN32
	m_ui8ItemValues = (uint8_t *)HeapAlloc(ServerManager::m_hPtokaXHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, ui8Size*sizeof(uint8_t));
#else
	m_ui8ItemValues = (uint8_t *)calloc(ui8Size, sizeof(uint8_t));
#endif
    if(m_ui8ItemValues == NULL) {
		AppendDebugLog("%s - [MEM] Cannot create ui8ItemValues in PXBReader::PrepareArrays\n");
		return false;
    }

	m_ui8AllocatedSize = ui8Size;
	return true;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
