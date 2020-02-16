/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdint.h>
#include <stdio.h>
#include <hardware/fw_EEPROM.h>
#include <hardware/fw_SPI_Flash.h>
#include "fw_codeplug.h"
#include "fw_trx.h"
#include "fw_usb_com.h"
#include <user_interface/uiLocalisation.h>


const int CODEPLUG_ADDR_EX_ZONE_BASIC = 0x8000;
const int CODEPLUG_ADDR_EX_ZONE_INUSE_PACKED_DATA =  0x8010;
const int CODEPLUG_ADDR_EX_ZONE_INUSE_PACKED_DATA_SIZE =  32;
const int CODEPLUG_ADDR_EX_ZONE_LIST =  0x8030;
//const int CODEPLUG_ZONE_DATA_SIZE = 48;
const int CODEPLUG_ZONE_MAX_COUNT = 250;
const int CODEPLUG_CHANNEL_DATA_SIZE = 56;
const int CODEPLUG_ADDR_CHANNEL_EEPROM = 0x3790;
const int CODEPLUG_ADDR_CHANNEL_FLASH = 0x7B1C0;

const int CODEPLUG_ADDR_RX_GROUP = 0x8D6A0;//
const int CODEPLUG_RX_GROUP_LEN = 0x50;

const int CODEPLUG_ADDR_CONTACTS = 0x87620;
const int CODEPLUG_CONTACT_DATA_LEN = 0x18;

const int CODEPLUG_ADDR_DTMF_CONTACTS = 0x02f88;
const int CODEPLUG_DTMF_CONTACTS_LEN = 0x10;
const int CODEPLUG_DTMF_CONTACTS_MAX_COUNT = 32;

const int CODEPLUG_ADDR_USER_DMRID = 0x00E8;
const int CODEPLUG_ADDR_USER_CALLSIGN = 0x00E0;

const int CODEPLUG_ADDR_BOOT_INTRO_SCREEN = 0x7518;// 0x01 = Chars 0x00 = Picture
const int CODEPLUG_ADDR_BOOT_PASSWORD_ENABLE= 0x7519;// 0x00 = password disabled 0x01 = password enable
const int CODEPLUG_ADDR_BOOT_PASSWORD_AREA = 0x751C;// Seems to be 3 bytes coded as BCD e.f. 0x12 0x34 0x56
const int CODEPLUG_BOOT_PASSWORD_LEN = 3;
const int CODEPLUG_ADDR_BOOT_LINE1 = 0x7540;
const int CODEPLUG_ADDR_BOOT_LINE2 = 0x7550;
const int CODEPLUG_ADDR_VFO_A_CHANNEL = 0x7590;

const int CODEPLUG_ADDR_QUICKKEYS = 0x7524;   // LSB,HSB

int codeplugChannelsPerZone = 16;

const int VFO_FREQ_STEP_TABLE[8] = {250,500,625,1000,1250,2500,3000,5000};

const int CODEPLUG_MAX_VARIABLE_SQUELCH = 21;
const int CODEPLUG_MIN_VARIABLE_SQUELCH = 1;

typedef struct
{
	uint32_t tgOrPCNum;
	uint16_t index;
} codeplugContactCache_t;


typedef struct
{
	int numTGContacts;
	int numPCContacts;
	codeplugContactCache_t contactsLookupCache[1024];
} codeplugContactsCache_t;

__attribute__((section(".data.$RAM2"))) codeplugContactsCache_t codeplugContactsCache;


uint32_t byteSwap32(uint32_t n)
{
    return ((((n)&0x000000FFU) << 24U) | (((n)&0x0000FF00U) << 8U) | (((n)&0x00FF0000U) >> 8U) | (((n)&0xFF000000U) >> 24U));// from usb_misc.h
}

// BCD encoding to integer conversion
uint32_t bcd2int(uint32_t i)
{
    int result = 0;
    int multiplier = 1;
    while (i)
    {
        result +=  (i &0x0f)* multiplier;
        multiplier *=10;
        i = i >> 4;
    }
    return result;
}

// Needed to convert the legacy DMR ID data which uses BCD encoding for the DMR ID numbers
int int2bcd(int i)
{
    int result = 0;
    int shift = 0;

    while (i)
    {
        result +=  (i % 10) << shift;
        i = i / 10;
        shift += 4;
    }
    return result;
}

void codeplugUtilConvertBufToString(char *inBuf,char *outBuf,int len)
{
	for(int i=0;i<len;i++)
	{
		if (inBuf[i]==0xff)
		{
			inBuf[i]=0;
		}
		outBuf[i]=inBuf[i];
	}
	outBuf[len]=0;
	return;
}

void codeplugUtilConvertStringToBuf(char *inBuf,char *outBuf,int len)
{
	memset(outBuf,0xff,len);
	for (int i = 0; i < len; i++)
	{
		if (inBuf[i] == 0x00)
		{
			break;
		}
		outBuf[i] = inBuf[i];
	}
	return;
}

int codeplugZonesGetCount(void)
{
	uint8_t buf[CODEPLUG_ADDR_EX_ZONE_INUSE_PACKED_DATA_SIZE];
	int numZones = 0;

	EEPROM_Read(CODEPLUG_ADDR_EX_ZONE_INUSE_PACKED_DATA, (uint8_t*)&buf, CODEPLUG_ADDR_EX_ZONE_INUSE_PACKED_DATA_SIZE);
	for(int i=0;i<CODEPLUG_ADDR_EX_ZONE_INUSE_PACKED_DATA_SIZE;i++)
	{
		numZones += __builtin_popcount(buf[i]);
	}
	return numZones+1; // Add one extra zone to allow for the special 'All Channels' Zone
}

void codeplugZoneGetDataForNumber(int zoneNum, struct_codeplugZone_t *returnBuf)
{

	if (zoneNum==codeplugZonesGetCount()-1) //special case: return a special Zone called 'All Channels'
	{
		memset(returnBuf->name, 0, sizeof(returnBuf->name));
		strncpy(returnBuf->name, currentLanguage->all_channels, sizeof(returnBuf->name) - 1);

		for(int i=0;i<codeplugChannelsPerZone;i++)
		{
			returnBuf->channels[i]=0;
		}
		returnBuf->NOT_IN_MEMORY_numChannelsInZone=0;
		returnBuf->NOT_IN_MEMORY_isAllChannelsZone = true;
	}
	else
	{
		// Need to find the index into the Zones data for the specific Zone number.
		// Because the Zones data is not guaranteed to be packed by the CPS (though we should attempt to make the CPS always pack the Zones)

		// Read the In Use data which is the first 32 byes of the Zones data
		uint8_t inUseBuf[CODEPLUG_ADDR_EX_ZONE_INUSE_PACKED_DATA_SIZE];
		EEPROM_Read(CODEPLUG_ADDR_EX_ZONE_INUSE_PACKED_DATA, (uint8_t*)&inUseBuf, CODEPLUG_ADDR_EX_ZONE_INUSE_PACKED_DATA_SIZE);

		int count=-1;// Need to start counting at -1 because the Zone number is zero indexed
		int foundIndex=-1;

		// Go though each byte in the In Use table
		for(int i=0;(i<32 && foundIndex==-1);i++)
		{
			// Go though each binary bit, counting them one by one
			for(int j=0;j<8;j++)
			{
				if ((inUseBuf[i] & 0x01 )==0x01)
				{
					count++;
					if (count == zoneNum)
					{
						// found it. So save the index before we exit the "for" loops
						foundIndex = (i * 8) + j;
						break;// Will break out of this loop, but the outer loop breaks becuase it also checks for foundIndex
					}
				}
				inUseBuf[i] = inUseBuf[i]>>1;
			}
		}

		// IMPORTANT. read size is different from the size of the data, because I added a extra property to the struct to hold the number of channels in the zone.
		EEPROM_Read(CODEPLUG_ADDR_EX_ZONE_LIST + (foundIndex  * (16 + (2 * codeplugChannelsPerZone))), (uint8_t*)returnBuf, sizeof(struct_codeplugZone_t));
		returnBuf->NOT_IN_MEMORY_isAllChannelsZone = false;
		for(int i=0;i<codeplugChannelsPerZone;i++)
		{
			// Empty channels seem to be filled with zeros
			if (returnBuf->channels[i] == 0)
			{
				returnBuf->NOT_IN_MEMORY_numChannelsInZone = i;
				return;
			}
		}
		returnBuf->NOT_IN_MEMORY_numChannelsInZone=codeplugChannelsPerZone;

	}
}

bool codeplugChannelIndexIsValid(int index)
{
	uint8_t bitarray[16];

	index--;
	int channelbank=index/128;
	int channeloffset=index%128;

	if(channelbank==0)
	{
		EEPROM_Read(CODEPLUG_ADDR_CHANNEL_EEPROM-16,(uint8_t *)bitarray,16);
	}
	else
	{
		SPI_Flash_read(CODEPLUG_ADDR_CHANNEL_FLASH-16+(channelbank-1)*(128*CODEPLUG_CHANNEL_DATA_SIZE+16),(uint8_t *)bitarray,16);
	}

	int byteno=channeloffset/8;
	int bitno=channeloffset%8;
	if (((bitarray[byteno] >> bitno) & 0x01) == 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void codeplugChannelIndexSetValid(int index)
{
	uint8_t bitarray[16];

	index--;
	int channelbank=index/128;
	int channeloffset=index%128;

	if(channelbank==0)
	{
		EEPROM_Read(CODEPLUG_ADDR_CHANNEL_EEPROM-16,(uint8_t *)bitarray,16);
	}
	else
	{
		SPI_Flash_read(CODEPLUG_ADDR_CHANNEL_FLASH-16+(channelbank-1)*(128*CODEPLUG_CHANNEL_DATA_SIZE+16),(uint8_t *)bitarray,16);
	}

	int byteno=channeloffset/8;
	int bitno=channeloffset%8;

	bitarray[byteno] |= 1 << bitno;

	if(channelbank==0)
	{
		EEPROM_Write(CODEPLUG_ADDR_CHANNEL_EEPROM-16,(uint8_t *)bitarray,16);
	}
	else
	{
		SPI_Flash_write(CODEPLUG_ADDR_CHANNEL_FLASH-16+(channelbank-1)*(128*CODEPLUG_CHANNEL_DATA_SIZE+16),(uint8_t *)bitarray,16);
	}
}

void codeplugChannelGetDataForIndex(int index, struct_codeplugChannel_t *channelBuf)
{
	// lower 128 channels are in EEPROM. Remaining channels are in Flash ! (What a mess...)
	index--; // I think the channel index numbers start from 1 not zero.
	if (index<128)
	{
		EEPROM_Read(CODEPLUG_ADDR_CHANNEL_EEPROM + index*sizeof(struct_codeplugChannel_t),(uint8_t *)channelBuf,sizeof(struct_codeplugChannel_t));
	}
	else
	{
		int flashReadPos = CODEPLUG_ADDR_CHANNEL_FLASH;

		index -= 128;// First 128 channels are in the EEPOM, so subtract 128 from the number when looking in the Flash

		// Every 128 bytes there seem to be 16 bytes gaps. I don't know why,bits since 16*8 = 128 bits, its likely they are flag bytes to indicate which channel in the next block are in use
		flashReadPos += 16 * (index/128);// we just need to skip over that these flag bits when calculating the position of the channel data in memory

		SPI_Flash_read(flashReadPos + index*sizeof(struct_codeplugChannel_t),(uint8_t *)channelBuf,sizeof(struct_codeplugChannel_t));
	}

	channelBuf->chMode = (channelBuf->chMode==0)?RADIO_MODE_ANALOG:RADIO_MODE_DIGITAL;
	// Convert the the legacy codeplug tx and rx freq values into normal integers
	channelBuf->txFreq = bcd2int(channelBuf->txFreq);
	channelBuf->rxFreq = bcd2int(channelBuf->rxFreq);
	if (channelBuf->rxTone != 0xffff)
	{
		channelBuf->rxTone = bcd2int(channelBuf->rxTone);
	}
	if (channelBuf->txTone != 0xffff)
	{
		channelBuf->txTone = bcd2int(channelBuf->txTone);
	}

	// Sanity check the sql value, because its not used by the official firmware and may contain random value e.g. 255
	if (channelBuf->sql>21)
	{
		channelBuf->sql = 10;
	}
	// Sanity check the digital contact and set it to 1 is its not been assigned, even for FM channels, as the user could switch to DMR on this channel
	if (channelBuf->contact == 0)
	{
		channelBuf->contact = 1;
	}

}


bool codeplugChannelSaveDataForIndex(int index, struct_codeplugChannel_t *channelBuf)
{
	bool retVal=true;

	channelBuf->chMode = (channelBuf->chMode==RADIO_MODE_ANALOG)?0:1;
	// Convert the the legacy codeplug tx and rx freq values into normal integers
	channelBuf->txFreq = int2bcd(channelBuf->txFreq);
	channelBuf->rxFreq = int2bcd(channelBuf->rxFreq);
	if (channelBuf->rxTone != 0xffff)
	{
		channelBuf->rxTone = int2bcd(channelBuf->rxTone);
	}

	if (channelBuf->txTone != 0xffff)
	{
		channelBuf->txTone = int2bcd(channelBuf->txTone);
	}

	// lower 128 channels are in EEPROM. Remaining channels are in Flash ! (What a mess...)
	index--; // I think the channel index numbers start from 1 not zero.
	if (index<128)
	{
		retVal = EEPROM_Write(CODEPLUG_ADDR_CHANNEL_EEPROM + index*sizeof(struct_codeplugChannel_t),(uint8_t *)channelBuf,sizeof(struct_codeplugChannel_t));
	}
	else
	{
		int flashWritePos = CODEPLUG_ADDR_CHANNEL_FLASH;
		int flashSector;
		int flashEndSector;
		int bytesToWriteInCurrentSector = sizeof(struct_codeplugChannel_t);

		index -= 128;// First 128 channels are in the EEPOM, so subtract 128 from the number when looking in the Flash

		// Every 128 bytes there seem to be 16 bytes gaps. I don't know why,bits since 16*8 = 128 bits, its likely they are flag bytes to indicate which channel in the next block are in use
		flashWritePos += 16 * (index/128);// we just need to skip over that these flag bits when calculating the position of the channel data in memory
		flashWritePos += index*sizeof(struct_codeplugChannel_t);// go to the position of the specific index

		flashSector 	= flashWritePos/4096;
		flashEndSector 	= (flashWritePos+sizeof(struct_codeplugChannel_t))/4096;

		if (flashSector!=flashEndSector)
		{
			bytesToWriteInCurrentSector = (flashEndSector*4096) - flashWritePos;
		}

		SPI_Flash_read(flashSector*4096,SPI_Flash_sectorbuffer,4096);
		uint8_t *writePos = SPI_Flash_sectorbuffer + flashWritePos - (flashSector *4096);
		memcpy( writePos,
				channelBuf,
				bytesToWriteInCurrentSector);

		retVal = SPI_Flash_eraseSector(flashSector*4096);
		if (!retVal)
		{
			return false;
		}

		for (int i=0;i<16;i++)
		{
			retVal = SPI_Flash_writePage(flashSector*4096+i*256,SPI_Flash_sectorbuffer+i*256);
			if (!retVal)
			{
				return false;
			}
		}

		if (flashSector!=flashEndSector)
		{
			uint8_t *channelBufPusOffset = (uint8_t *)channelBuf + bytesToWriteInCurrentSector;
			bytesToWriteInCurrentSector = sizeof(struct_codeplugChannel_t) - bytesToWriteInCurrentSector;

			SPI_Flash_read(flashEndSector*4096,SPI_Flash_sectorbuffer,4096);
			memcpy(SPI_Flash_sectorbuffer,
					(uint8_t *)channelBufPusOffset,
					bytesToWriteInCurrentSector);

			retVal = SPI_Flash_eraseSector(flashEndSector*4096);

			if (!retVal)
			{
				return false;
			}
			for (int i=0;i<16;i++)
			{
				retVal = SPI_Flash_writePage(flashEndSector*4096+i*256,SPI_Flash_sectorbuffer+i*256);

				if (!retVal)
				{
					return false;
				}
			}

		}
	}

	// Need to restore the values back to what we need for the operation of the firmware rather than the BCD values the codeplug uses

	channelBuf->chMode = (channelBuf->chMode==0)?RADIO_MODE_ANALOG:RADIO_MODE_DIGITAL;
	// Convert the the legacy codeplug tx and rx freq values into normal integers
	channelBuf->txFreq = bcd2int(channelBuf->txFreq);
	channelBuf->rxFreq = bcd2int(channelBuf->rxFreq);
	if (channelBuf->rxTone != 0xffff)
	{
		channelBuf->rxTone = bcd2int(channelBuf->rxTone);
	}
	if (channelBuf->txTone != 0xffff)
	{
		channelBuf->txTone = bcd2int(channelBuf->txTone);
	}

	return retVal;
}

void codeplugRxGroupGetDataForIndex(int index, struct_codeplugRxGroup_t *rxGroupBuf)
{
	int i=0;
	index--; //Index numbers start from 1 not zero
// Not our struct contains an extra property to hold the number of TGs in the group
	SPI_Flash_read(CODEPLUG_ADDR_RX_GROUP + index*(sizeof(struct_codeplugRxGroup_t) - sizeof(int)),(uint8_t *)rxGroupBuf,sizeof(struct_codeplugRxGroup_t) - sizeof(int));
	for(i=0;i<32;i++)
	{
		// Empty groups seem to be filled with zeros
		if (rxGroupBuf->contacts[i] == 0)
		{
			break;
		}
	}
	rxGroupBuf->NOT_IN_MEMORY_numTGsInGroup = i;
}



int codeplugContactsGetCount(int callType) // 0:TG 1:PC
{
	if (callType == CONTACT_CALLTYPE_PC)
	{
		return codeplugContactsCache.numPCContacts;
	}
	else
	{
		return codeplugContactsCache.numTGContacts;
	}
}

int codeplugContactGetDataForNumber(int number, int callType, struct_codeplugContact_t *contact)
{
	int pos = 0;

	for (int i = 0; i < 1024; i++)
	{
		if ((codeplugContactsCache.contactsLookupCache[i].tgOrPCNum>>24) == callType)
		{
			number--;
		}

		if (number == 0)
		{
			codeplugContactGetDataForIndex(codeplugContactsCache.contactsLookupCache[i].index, contact);
			pos = i+1;
			break;
		}
	}
	return pos;
}

int codeplugContactIndexByTGorPC(int tgorpc, int callType, struct_codeplugContact_t *contact)
{
	int numContacts =  codeplugContactsCache.numTGContacts + codeplugContactsCache.numPCContacts;
	for (int i = 0; i < numContacts; i++)
	{
		if ((codeplugContactsCache.contactsLookupCache[i].tgOrPCNum&0xFFFFFF) == tgorpc && (codeplugContactsCache.contactsLookupCache[i].tgOrPCNum>>24) == callType)
		{
			codeplugContactGetDataForIndex(codeplugContactsCache.contactsLookupCache[i].index, contact);
			return i;
		}
	}
	return 0;
}

bool codeplugContactsContainsPC(uint32_t pc)
{
	int numContacts =  codeplugContactsCache.numTGContacts + codeplugContactsCache.numPCContacts;
	pc = pc & 0x00FFFFFF;
	pc = pc | (CONTACT_CALLTYPE_PC << 24);

	for (int i = 0; i < numContacts; i++)
	{
		if (codeplugContactsCache.contactsLookupCache[i].tgOrPCNum == pc)
		{
			return true;
		}
	}
	return false;
}

void codeplugInitContactsCache(void)
{
	struct_codeplugContact_t contact;
	int codeplugNumContacts=0;
	codeplugContactsCache.numTGContacts=0;
	codeplugContactsCache.numPCContacts=0;

	for(int i=0;i<1024;i++)
	{
		SPI_Flash_read((CODEPLUG_ADDR_CONTACTS + (i * CODEPLUG_CONTACT_DATA_LEN)), (uint8_t *)&contact, 16+4+1);// Name + TG/ID + Call type
		if (contact.name[0]!=0xFF)
		{
			codeplugContactsCache.contactsLookupCache[codeplugNumContacts].tgOrPCNum = bcd2int(byteSwap32(contact.tgNumber));
			codeplugContactsCache.contactsLookupCache[codeplugNumContacts].index=i+1;// Contacts are numbered from 1 to 1024
			codeplugContactsCache.contactsLookupCache[codeplugNumContacts].tgOrPCNum |= (contact.callType<<24);// Store the call type in the upper byte
			if (contact.callType == CONTACT_CALLTYPE_PC)
			{
				codeplugContactsCache.numPCContacts++;
			}
			else
			{
				codeplugContactsCache.numTGContacts++;
			}
			codeplugNumContacts++;
		}
	}
}

void codeplugContactsCacheUpdateOrInsertContactAt(int index, struct_codeplugContact_t *contact)
{
	int numContacts =  codeplugContactsCache.numTGContacts + codeplugContactsCache.numPCContacts;
	int numContactsMinus1 = numContacts-1;

	for(int i = 0;i < numContacts;i++)
	{
		// Check if the contact is already in the cache, and is being modified
		if (codeplugContactsCache.contactsLookupCache[i].index == index)
		{
			uint8_t callType = codeplugContactsCache.contactsLookupCache[i].tgOrPCNum>>24;// get call type from cache
			if (callType != contact->callType)
			{
				if (contact->callType == CONTACT_CALLTYPE_PC)
				{
					codeplugContactsCache.numPCContacts++;
					codeplugContactsCache.numTGContacts--;
				}
				else
				{
					codeplugContactsCache.numTGContacts++;
					codeplugContactsCache.numPCContacts--;
				}
			}
			//update the
			codeplugContactsCache.contactsLookupCache[i].tgOrPCNum = bcd2int(byteSwap32(contact->tgNumber));
			codeplugContactsCache.contactsLookupCache[i].tgOrPCNum |= (contact->callType<<24);// Store the call type in the upper byte

			return;
		}
		else
		{
			if(i < numContactsMinus1 && codeplugContactsCache.contactsLookupCache[i].index < index && codeplugContactsCache.contactsLookupCache[i+1].index > index)
			{
				if (contact->callType == CONTACT_CALLTYPE_PC)
				{
					codeplugContactsCache.numPCContacts++;
				}
				else
				{
					codeplugContactsCache.numTGContacts++;
				}

				numContacts++;// Total contacts increases by 1

				// Note . Need to use memmove as the source and destination overlap.
				memmove(&codeplugContactsCache.contactsLookupCache[i+2],&codeplugContactsCache.contactsLookupCache[i+1],(numContacts - 2 - i) *sizeof(codeplugContactCache_t));

				codeplugContactsCache.contactsLookupCache[i+1].tgOrPCNum = bcd2int(byteSwap32(contact->tgNumber));
				codeplugContactsCache.contactsLookupCache[i+1].index=index;// Contacts are numbered from 1 to 1024
				codeplugContactsCache.contactsLookupCache[i+1].tgOrPCNum |= (contact->callType<<24);// Store the call type in the upper byte
				return;
			}
		}
	}

	// Did not find the index in the cache or a gap between 2 existing indexes. So the new contact needs to be added to the end of the cache

	if (contact->callType == CONTACT_CALLTYPE_PC)
	{
		codeplugContactsCache.numPCContacts++;
	}
	else
	{
		codeplugContactsCache.numTGContacts++;
	}

	// Note. We can use numContacts as the the index as the array is zero indexed but the number of contacts is starts from 1
	// Hence is already in some ways pre incremented in terms of being an array index
	codeplugContactsCache.contactsLookupCache[numContacts].tgOrPCNum = bcd2int(byteSwap32(contact->tgNumber));
	codeplugContactsCache.contactsLookupCache[numContacts].index=index;// Contacts are numbered from 1 to 1024
	codeplugContactsCache.contactsLookupCache[numContacts].tgOrPCNum |= (contact->callType<<24);// Store the call type in the upper byte
}

void codeplugContactsCacheRemoveContactAt(int index)
{
	int numContacts =  codeplugContactsCache.numTGContacts + codeplugContactsCache.numPCContacts;
	for(int i=0;i<numContacts;i++)
	{
		if(codeplugContactsCache.contactsLookupCache[i].index == index)
		{
			if ((codeplugContactsCache.contactsLookupCache[i].tgOrPCNum>>24) == CONTACT_CALLTYPE_PC)
			{
				codeplugContactsCache.numPCContacts--;
			}
			else
			{
				codeplugContactsCache.numTGContacts--;
			}
			// Note memcpy should work here, because memcpy normally copys from the lowest memory location upwards
			memcpy(&codeplugContactsCache.contactsLookupCache[i],&codeplugContactsCache.contactsLookupCache[i+1],(numContacts - 1 - i) *sizeof(codeplugContactCache_t));
			return;
		}
	}
}

int codeplugContactGetFreeIndex(void)
{
	int i;
	int lastIndex=0;

	int numContacts =  codeplugContactsCache.numTGContacts + codeplugContactsCache.numPCContacts;

	for (i = 0; i < numContacts; i++)
	{
		if (codeplugContactsCache.contactsLookupCache[i].index!=lastIndex+1)
		{
			return lastIndex+1;
		}
		lastIndex = codeplugContactsCache.contactsLookupCache[i].index;
	}

	if (i < 1024)
	{
		return codeplugContactsCache.contactsLookupCache[i-1].index+1;
	}

	return 0;
}

bool codeplugContactGetDataForIndex(int index, struct_codeplugContact_t *contact)
{
	if (index>0 && index<=1024)
	{
		index--;
		SPI_Flash_read(CODEPLUG_ADDR_CONTACTS + index* CODEPLUG_CONTACT_DATA_LEN,(uint8_t *)contact, CODEPLUG_CONTACT_DATA_LEN);
		contact->NOT_IN_CODEPLUGDATA_indexNumber = index+1;
		contact->tgNumber = bcd2int(byteSwap32(contact->tgNumber));
		return true;
	}
	else
	{
		// If an invalid contact number has been requested, return a TG 9 contact
		contact->tgNumber = 9;
		contact->reserve1=0xff;
		codeplugUtilConvertStringToBuf("TG 9",contact->name,16);
		return false;
	}
}

int codeplugContactSaveDataForIndex(int index, struct_codeplugContact_t *contact)
{
	int retVal;
	int flashWritePos = CODEPLUG_ADDR_CONTACTS;
	int flashSector;
	int flashEndSector;
	int bytesToWriteInCurrentSector = CODEPLUG_CONTACT_DATA_LEN;

	index--;
	contact->tgNumber = byteSwap32(int2bcd(contact->tgNumber));


	flashWritePos += index*CODEPLUG_CONTACT_DATA_LEN;// go to the position of the specific index

	flashSector 	= flashWritePos/4096;
	flashEndSector 	= (flashWritePos+CODEPLUG_CONTACT_DATA_LEN)/4096;

	if (flashSector!=flashEndSector)
	{
		bytesToWriteInCurrentSector = (flashEndSector*4096) - flashWritePos;
	}

	SPI_Flash_read(flashSector*4096,SPI_Flash_sectorbuffer,4096);
	uint8_t *writePos = SPI_Flash_sectorbuffer + flashWritePos - (flashSector *4096);
	memcpy( writePos,
			contact,
			bytesToWriteInCurrentSector);

	retVal = SPI_Flash_eraseSector(flashSector*4096);
	if (!retVal)
	{
		return false;
	}

	for (int i=0;i<16;i++)
	{
		retVal = SPI_Flash_writePage(flashSector*4096+i*256,SPI_Flash_sectorbuffer+i*256);
		if (!retVal)
		{
			return false;
		}
	}

	if (flashSector!=flashEndSector)
	{
		uint8_t *channelBufPusOffset = (uint8_t *)contact + bytesToWriteInCurrentSector;
		bytesToWriteInCurrentSector = CODEPLUG_CONTACT_DATA_LEN - bytesToWriteInCurrentSector;

		SPI_Flash_read(flashEndSector*4096,SPI_Flash_sectorbuffer,4096);
		memcpy(SPI_Flash_sectorbuffer,
				(uint8_t *)channelBufPusOffset,
				bytesToWriteInCurrentSector);

		retVal = SPI_Flash_eraseSector(flashEndSector*4096);

		if (!retVal)
		{
			return false;
		}
		for (int i=0;i<16;i++)
		{
			retVal = SPI_Flash_writePage(flashEndSector*4096+i*256,SPI_Flash_sectorbuffer+i*256);

			if (!retVal)
			{
				return false;
			}
		}

	}
	if (contact->name[0]==0xff || contact->callType == 0xFF)
	{
		codeplugContactsCacheRemoveContactAt(index+1);// index was decremented at the start of the function
	}
	else
	{
		codeplugContactsCacheUpdateOrInsertContactAt(index+1,contact);
		//initCodeplugContactsCache();// Update the cache
	}
	return retVal;
}

bool codeplugContactGetRXGroup(int index)
{
	struct_codeplugRxGroup_t rxGroupBuf;
	int i;

	for (i = 1; i <= 76; i++) {
		codeplugRxGroupGetDataForIndex(i, &rxGroupBuf);
		for (int j = 0; j<32; j++) {
			if (rxGroupBuf.contacts[j] == index) {
				return true;
			}
		}
	}
	return false;
}

void codeplugDTMFContactGetDataForIndex(struct_codeplugDTMFContactList_t *contactList)
{
	int i;

	EEPROM_Read(CODEPLUG_ADDR_DTMF_CONTACTS,(uint8_t *)contactList, sizeof(struct_codeplugDTMFContact_t) * CODEPLUG_DTMF_CONTACTS_MAX_COUNT);

	for(i=0;i<CODEPLUG_DTMF_CONTACTS_MAX_COUNT;i++)
	{
		// Empty contact names contain 0xFF
		if (contactList->contacts->name[0] == 0xff)
		{
			break;
		}
	}
	contactList->numContacts = i;
}


int codeplugGetUserDMRID(void)
{
	int dmrId;
	EEPROM_Read(CODEPLUG_ADDR_USER_DMRID,(uint8_t *)&dmrId,4);
	return bcd2int(byteSwap32(dmrId));
}

void codeplugSetUserDMRID(uint32_t dmrId)
{
	dmrId = byteSwap32(int2bcd(dmrId));
	EEPROM_Write(CODEPLUG_ADDR_USER_DMRID,(uint8_t *)&dmrId,4);
}

// Max length the user can enter is 8. Hence buf must be 16 chars to allow for the termination
void codeplugGetRadioName(char *buf)
{
	memset(buf,0,9);
	EEPROM_Read(CODEPLUG_ADDR_USER_CALLSIGN,(uint8_t *)buf,8);
	codeplugUtilConvertBufToString(buf,buf,8);
}

// Max length the user can enter is 15. Hence buf must be 16 chars to allow for the termination
void codeplugGetBootScreenData(char *line1, char *line2,uint8_t *displayType, uint8_t *passwordEnable, uint32_t *passwordNumber)
{
	uint8_t tmp[8];
	memset(line1,0,16);
	memset(line2,0,16);

	EEPROM_Read(CODEPLUG_ADDR_BOOT_LINE1,(uint8_t *)line1,15);
	codeplugUtilConvertBufToString(line1,line1,15);
	EEPROM_Read(CODEPLUG_ADDR_BOOT_LINE2,(uint8_t *)line2,15);
	codeplugUtilConvertBufToString(line2,line2,15);

	EEPROM_Read(CODEPLUG_ADDR_BOOT_INTRO_SCREEN,(uint8_t *)tmp,8);// read the display type, and password enable and password
	*displayType = tmp[0];
	*passwordEnable = tmp[1];
	*passwordNumber = bcd2int((tmp[4]<<16) + (tmp[5]<<8) + tmp[6]);
}


void codeplugGetVFO_ChannelData(struct_codeplugChannel_t *vfoBuf,int VFONumber)
{
	EEPROM_Read(CODEPLUG_ADDR_VFO_A_CHANNEL + (sizeof(struct_codeplugChannel_t)*VFONumber),(uint8_t *)vfoBuf,sizeof(struct_codeplugChannel_t));

	// Convert the the legacy codeplug tx and rx freq values into normal integers
	vfoBuf->chMode = (vfoBuf->chMode==0)?RADIO_MODE_ANALOG:RADIO_MODE_DIGITAL;
	vfoBuf->txFreq = bcd2int(vfoBuf->txFreq);
	vfoBuf->rxFreq = bcd2int(vfoBuf->rxFreq);
}

void codeplugSetVFO_ChannelData(struct_codeplugChannel_t *vfoBuf,int VFONumber)
{
	struct_codeplugChannel_t tmpChannel;
	memcpy(&tmpChannel,vfoBuf,sizeof(struct_codeplugChannel_t));// save current VFO data as we need to modify
	tmpChannel.chMode = (vfoBuf->chMode==RADIO_MODE_ANALOG)?0:1;
	tmpChannel.txFreq = int2bcd(vfoBuf->txFreq);
	tmpChannel.rxFreq = int2bcd(vfoBuf->rxFreq);
	EEPROM_Write(CODEPLUG_ADDR_VFO_A_CHANNEL+(sizeof(struct_codeplugChannel_t)*VFONumber),(uint8_t *)&tmpChannel,sizeof(struct_codeplugChannel_t));
}

void codeplugInitChannelsPerZone(void)
{
	uint8_t buf[16];
	// 0x806F is the last byte of the name of the second Zone if 16 channels per zone
	// And because the CPS terminates the zone name with 0xFF, this will be 0xFF if its a 16 channel per zone schema
	// If the zone has 80 channels per zone this address will be the upper byte of the channel number and because the max channels is 1024
	// this value can never me 0xff if its a channel number

	// Note. I tried to read just 1 byte but it crashed. So I am now reading 16 bytes and checking the last one
	EEPROM_Read(0x8060,(uint8_t *)buf,16);
	if (buf[15] >= 0x00 && buf[15]<=0x04)
	{
		codeplugChannelsPerZone=80;// Must be the new 80 channel per zone format
	}
}

typedef struct
{
	int dataType;
	int dataLength;
} codeplugCustomDataBlockHeader_t;

bool codeplugGetOpenGD77CustomData(codeplugCustomDataType_t dataType,uint8_t *dataBuf)
{
	uint8_t tmpBuf[12];
	int dataHeaderAddress = 12;
	const int MAX_BLOCK_ADDRESS = 0x10000;

	SPI_Flash_read(0,tmpBuf,12);

	if (memcmp("OpenGD77",tmpBuf,8)==0)
	{
		codeplugCustomDataBlockHeader_t blockHeader;
		do
		{
			SPI_Flash_read(dataHeaderAddress,(uint8_t *)&blockHeader,sizeof(codeplugCustomDataBlockHeader_t));
			if (blockHeader.dataType == dataType)
			{
				SPI_Flash_read(dataHeaderAddress + sizeof(codeplugCustomDataBlockHeader_t),dataBuf,blockHeader.dataLength);
				return true;
			}
			dataHeaderAddress += sizeof(codeplugCustomDataBlockHeader_t) + blockHeader.dataLength;

		} while (dataHeaderAddress < MAX_BLOCK_ADDRESS);

	}
	return false;
}

int codeplugGetQuickkeyFunctionID(int key)
{
	uint16_t functionId = 0;

	if (key >='0' && key <='9')
	{
		key = key-'0';
		EEPROM_Read(CODEPLUG_ADDR_QUICKKEYS+2*key,(uint8_t *)&functionId,2);
	}

	return functionId;
}
