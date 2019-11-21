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

const int CODEPLUG_ADDR_EX_ZONE_BASIC = 0x8000;
const int CODEPLUG_ADDR_EX_ZONE_INUSE_PACKED_DATA =  0x8010;
const int CODEPLUG_ADDR_EX_ZONE_INUSE_PACKED_DATA_SIZE =  32;
const int CODEPLUG_ADDR_EX_ZONE_LIST =  0x8030;
const int CODEPLUG_ZONE_DATA_SIZE = 48;
const int CODEPLUG_ZONE_MAX_COUNT = 250;
const int CODEPLUG_CHANNEL_DATA_SIZE = 56;
const int CODEPLUG_ADDR_CHANNEL_EEPROM = 0x3790;
const int CODEPLUG_ADDR_CHANNEL_FLASH = 0x7B1C0;

const int CODEPLUG_ADDR_RX_GROUP = 0x8D6A0;//
const int CODEPLUG_RX_GROUP_LEN = 0x50;

const int CODEPLUG_ADDR_CONTACTS = 0x87620;
const int CODEPLUG_CONTACTS_LEN = 0x18;

const int CODEPLUG_ADDR_DTMF_CONTACTS = 0x02f88;
const int CODEPLUG_DTMF_CONTACTS_LEN = 0x10;
const int CODEPLUG_DTMF_CONTACTS_MAX_COUNT = 32;

const int CODEPLUG_ADDR_USER_DMRID = 0x00E8;
const int CODEPLUG_ADDR_USER_CALLSIGN = 0x00E0;

const int CODEPLUG_ADDR_BOOT_LINE1 = 0x7540;
const int CODEPLUG_ADDR_BOOT_LINE2 = 0x7550;
const int CODEPLUG_ADDR_VFO_A_CHANNEL = 0x7590;

const int MAX_CHANNELS_PER_ZONE = 16;

const int VFO_FREQ_STEP_TABLE[8] = {250,500,625,1000,1250,2500,3000,5000};

const int CODEPLUG_MAX_VARIABLE_SQUELCH = 21;
const int CODEPLUG_MIN_VARIABLE_SQUELCH = 1;

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
	for (int i = 0; i < len; i++)
	{
		if (inBuf[i] == 0x0)
		{
			outBuf[i] = 0xff;
		}
		else
		{
			outBuf[i] = inBuf[i];
		}
	}
	outBuf[len] = 0;
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

void codeplugZoneGetDataForNumber(int zoneNum,struct_codeplugZone_t *returnBuf)
{


	if (zoneNum==codeplugZonesGetCount()-1) //special case: return a special Zone called 'All Channels'
	{
		sprintf(returnBuf->name,"All Channels");
		for(int i=0;i<MAX_CHANNELS_PER_ZONE;i++)
		{
			returnBuf->channels[i]=0;
		}
		returnBuf->NOT_IN_MEMORY_numChannelsInZone=0;
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
		EEPROM_Read(CODEPLUG_ADDR_EX_ZONE_LIST + (foundIndex  * CODEPLUG_ZONE_DATA_SIZE), (uint8_t*)returnBuf, sizeof(struct_codeplugZone_t));

		for(int i=0;i<MAX_CHANNELS_PER_ZONE;i++)
		{
			// Empty channels seem to be filled with zeros
			if (returnBuf->channels[i] == 0)
			{
				returnBuf->NOT_IN_MEMORY_numChannelsInZone = i;
				return;
			}
		}
		returnBuf->NOT_IN_MEMORY_numChannelsInZone=MAX_CHANNELS_PER_ZONE;
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
	struct_codeplugContact_t contact;
	int numContacts=0;

	for (int i = 1; i <= 1024; i++)
	{
		codeplugContactGetDataForIndex(i, &contact);
		if (contact.name[0] != 0xff && contact.callType == callType) {
			numContacts++;
		}
	}
	return numContacts;
}

int codeplugContactGetDataForNumber(int index, int callType, struct_codeplugContact_t *contact)
{
	int pos = 0;
	for (int i = 1; i <= 1024; i++)
	{
		codeplugContactGetDataForIndex(i, contact);
		if (contact->name[0] != 0xff && contact->callType == callType) {
			index--;
		}
		if (index == 0) {
			pos = i;
			break;
		}
	}
	return pos;
}

int codeplugContactGetFreeIndex(void)
{
	int i;
	bool found = false;
	struct_codeplugContact_t contact;

	for (i = 1; i <= 1024; i++)
	{
		codeplugContactGetDataForIndex(i, &contact);
		if (contact.name[0] == 0xff)
		{
			found = true;
			break;
		}
	}
	return (found == true) ? i : 0;
}

void codeplugContactGetDataForIndex(int index, struct_codeplugContact_t *contact)
{
	index--;
	SPI_Flash_read(CODEPLUG_ADDR_CONTACTS + index*sizeof(struct_codeplugContact_t),(uint8_t *)contact,sizeof(struct_codeplugContact_t));
	contact->tgNumber = bcd2int(byteSwap32(contact->tgNumber));
}

int codeplugContactSaveDataForIndex(int index, struct_codeplugContact_t *contact)
{
	int retVal;
	int flashWritePos = CODEPLUG_ADDR_CONTACTS;
	int flashSector;
	int flashEndSector;
	int bytesToWriteInCurrentSector = sizeof(struct_codeplugContact_t);

	index--;
	contact->tgNumber = byteSwap32(int2bcd(contact->tgNumber));


	flashWritePos += index*sizeof(struct_codeplugContact_t);// go to the position of the specific index

	flashSector 	= flashWritePos/4096;
	flashEndSector 	= (flashWritePos+sizeof(struct_codeplugChannel_t))/4096;

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
	return retVal;
}

void codeplugDTMFContactGetDataForIndex(struct_codeplugDTMFContactList_t *contactList)
{
	int i;

	EEPROM_Read(CODEPLUG_ADDR_DTMF_CONTACTS,(uint8_t *)contactList, sizeof(struct_codeplugContact_t) * CODEPLUG_DTMF_CONTACTS_MAX_COUNT);

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
void codeplugGetBootItemTexts(char *line1, char *line2)
{
	memset(line1,0,16);
	memset(line2,0,16);

	EEPROM_Read(CODEPLUG_ADDR_BOOT_LINE1,(uint8_t *)line1,15);
	codeplugUtilConvertBufToString(line1,line1,15);
	EEPROM_Read(CODEPLUG_ADDR_BOOT_LINE2,(uint8_t *)line2,15);
	codeplugUtilConvertBufToString(line2,line2,15);
}


void codeplugVFO_A_ChannelData(struct_codeplugChannel_t *vfoBuf)
{
	EEPROM_Read(CODEPLUG_ADDR_VFO_A_CHANNEL,(uint8_t *)vfoBuf,sizeof(struct_codeplugChannel_t));

	// Convert the the legacy codeplug tx and rx freq values into normal integers
	vfoBuf->chMode = (vfoBuf->chMode==0)?RADIO_MODE_ANALOG:RADIO_MODE_DIGITAL;
	vfoBuf->txFreq = bcd2int(vfoBuf->txFreq);
	vfoBuf->rxFreq = bcd2int(vfoBuf->rxFreq);
}
