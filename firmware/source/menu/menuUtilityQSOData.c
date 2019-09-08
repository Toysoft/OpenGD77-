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

#include <menu/menuSystem.h>
#include "menu/menuUtilityQSOData.h"
#include "fw_trx.h"
#include "fw_EEPROM.h"
#include "fw_SPI_Flash.h"
#include "fw_settings.h"
#include "fw_HR-C6000.h"

void updateLastHeardList(int id,int talkGroup);

const int QSO_TIMER_TIMEOUT = 2400;

static const int DMRID_MEMORY_STORAGE_START = 0x30000;
static const int DMRID_HEADER_LENGTH = 0x0C;
LinkItem_t callsList[NUM_LASTHEARD_STORED];
LinkItem_t *LinkHead = callsList;
int numLastHeard=0;
int menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
int qsodata_timer;

uint32_t menuUtilityReceivedPcId 	= 0;// No current Private call awaiting acceptance
uint32_t menuUtilityTgBeforePcMode 	= 0;// No TG saved, prior to a Private call being accepted.

void lastheardInitList()
{
    LinkHead = callsList;

    for(int i=0;i<NUM_LASTHEARD_STORED;i++)
    {
    	callsList[i].id=0;
        callsList[i].talkGroupOrPcId=0;
        if (i==0)
        {
            callsList[i].prev=NULL;
        }
        else
        {
            callsList[i].prev=&callsList[i-1];
        }
        if (i<(NUM_LASTHEARD_STORED-1))
        {
            callsList[i].next=&callsList[i+1];
        }
        else
        {
            callsList[i].next=NULL;
        }
    }
}

LinkItem_t * findInList(int id)
{
    LinkItem_t *item = LinkHead;

    while(item->next!=NULL)
    {
        if (item->id==id)
        {
            // found it
            return item;
        }
        item=item->next;
    }
    return NULL;
}

uint32_t lastID=0;
uint32_t lastTG=0;

bool lastHeardListUpdate(uint8_t *dmrDataBuffer)
{
	bool retVal = false;
	uint32_t talkGroupOrPcId = (dmrDataBuffer[0]<<24) + (dmrDataBuffer[3]<<16)+(dmrDataBuffer[4]<<8)+(dmrDataBuffer[5]<<0);


	if (dmrDataBuffer[0]==TG_CALL_FLAG || dmrDataBuffer[0]==PC_CALL_FLAG)
	{
		uint32_t id=(dmrDataBuffer[6]<<16)+(dmrDataBuffer[7]<<8)+(dmrDataBuffer[8]<<0);

		if (id!=lastID)
		{
			retVal = true;// something has changed
			lastID=id;

			LinkItem_t *item = findInList(id);

			if (item!=NULL)
			{
				// Already in the list
				item->talkGroupOrPcId = talkGroupOrPcId;// update the TG in case they changed TG
				lastTG = talkGroupOrPcId;

				if (item == LinkHead)
				{
					menuDisplayQSODataState = QSO_DISPLAY_CALLER_DATA;// flag that the display needs to update
					return true;// already at top of the list
				}
				else
				{
					// not at top of the list
					// Move this item to the top of the list
					LinkItem_t *next=item->next;
					LinkItem_t *prev=item->prev;

					// set the previous item to skip this item and link to 'items' next item.
					prev->next = next;

					if (item->next!=NULL)
					{
						// not the last in the list
						next->prev = prev;// backwards link the next item to the item before us in the list.
					}

					item->next = LinkHead;// link our next item to the item at the head of the list

					LinkHead->prev = item;// backwards link the hold head item to the item moving to the top of the list.

					item->prev=NULL;// change the items prev to NULL now we are at teh top of the list
					LinkHead = item;// Change the global for the head of the link to the item that is to be at the top of the list.
					menuDisplayQSODataState = QSO_DISPLAY_CALLER_DATA;// flag that the display needs to update
				}
			}
			else
			{
				// Not in the list
				item = LinkHead;// setup to traverse the list from the top.

				// need to use the last item in the list as the new item at the top of the list.
				// find last item in the list
				while(item->next != NULL )
				{
					item=item->next;
				}
				//item is now the last

				(item->prev)->next = NULL;// make the previous item the last

				LinkHead->prev = item;// set the current head item to back reference this item.
				item->next = LinkHead;// set this items next to the current head
				LinkHead = item;// Make this item the new head

				item->id=id;
				item->talkGroupOrPcId =  talkGroupOrPcId;
				lastTG = talkGroupOrPcId;
				memset(item->talkerAlias,0,32);// Clear any TA data
				menuDisplayQSODataState = QSO_DISPLAY_CALLER_DATA;// flag that the display needs to update
			}
		}
		else // update TG even if the DMRID did not change
		{
			if (lastTG != talkGroupOrPcId)
			{
				LinkItem_t *item = findInList(id);

				if (item!=NULL)
				{
					// Already in the list
					item->talkGroupOrPcId = talkGroupOrPcId;// update the TG in case they changed TG
				}
				lastTG = talkGroupOrPcId;
				retVal = true;// something has changed
			}
		}
	}
	else
	{
		int TAStartPos;
		int TABlockLen;
		int TAOffset;

		// Data contains the Talker Alias Data
		switch(DMR_frame_buffer[0])
		{
			case 0x04:
				TAOffset=0;
				TAStartPos=3;
				TABlockLen=6;
				break;
			case 0x05:
				TAOffset=6;
				TAStartPos=2;
				TABlockLen=7;
				break;
			case 0x06:
				TAOffset=13;
				TAStartPos=2;
				TABlockLen=7;
				break;
			case 0x07:
				TAOffset=20;
				TAStartPos=2;
				TABlockLen=7;
				break;
			default:
				TAOffset=0;
				TAStartPos=0;
				TABlockLen=0;
				break;
		}
		if (LinkHead->talkerAlias[TAOffset] == 0x00 && TABlockLen!=0)
		{
			memcpy(&LinkHead->talkerAlias[TAOffset],&DMR_frame_buffer[TAStartPos],TABlockLen);// Brandmeister seems to send callsign as 6 chars only
			menuDisplayQSODataState=QSO_DISPLAY_CALLER_DATA;
		}
	}
	return retVal;
}

bool dmrIDLookup( int targetId,dmrIdDataStruct_t *foundRecord)
{
	uint32_t l = 0;
	uint32_t numRecords;
	uint32_t r;
	uint32_t m;
	uint32_t recordLenth;//15+4;
	uint8_t headerBuf[32];
	memset(foundRecord,0,sizeof(dmrIdDataStruct_t));

	int targetIdBCD=int2bcd(targetId);

	SPI_Flash_read(DMRID_MEMORY_STORAGE_START,headerBuf,DMRID_HEADER_LENGTH);

	if (headerBuf[0] != 'I' || headerBuf[1] != 'D' || headerBuf[2] != '-')
	{
		return false;
	}

	numRecords = (uint32_t) headerBuf[8] | (uint32_t) headerBuf[9] << 8 | (uint32_t)headerBuf[10] <<16 | (uint32_t)headerBuf[11] << 24 ;

	recordLenth = (uint32_t) headerBuf[3] - 0x4a;

	r = numRecords - 1;

	while (l <= r)
	{
		m = (l + r) >> 1;

		SPI_Flash_read((DMRID_MEMORY_STORAGE_START+DMRID_HEADER_LENGTH) + recordLenth*m,(uint8_t *)foundRecord,recordLenth);

		if (foundRecord->id < targetIdBCD)
		{
			l = m + 1;
		}
		else
		{
			if (foundRecord->id >targetIdBCD)
			{
				r = m - 1;
			}
			else
			{
				return true;
			}
		}
	}
	sprintf(foundRecord->text,"ID:%d",targetId);
	return false;
}

bool menuUtilityHandlePrivateCallActions(int buttons, int keys, int events)
{
	if ((buttons & BUTTON_SK2 )!=0 &&   menuUtilityTgBeforePcMode != 0 && (keys & KEY_RED))
	{
		trxTalkGroupOrPcId = menuUtilityTgBeforePcMode;
		nonVolatileSettings.overrideTG = menuUtilityTgBeforePcMode;
		menuUtilityReceivedPcId = 0;
		menuUtilityTgBeforePcMode = 0;
		menuDisplayQSODataState= QSO_DISPLAY_DEFAULT_SCREEN;// Force redraw
		return true;// The event has been handled
	}

	// Note.  menuUtilityReceivedPcId is used to store the PcId but also used as a flag to indicate that a Pc request has occurred.
	if (menuUtilityReceivedPcId != 0x00 && (LinkHead->talkGroupOrPcId>>24) == PC_CALL_FLAG && nonVolatileSettings.overrideTG != LinkHead->talkGroupOrPcId)
	{
		if ((keys & KEY_GREEN)!=0)
		{
			// User has accepted the private call
			menuUtilityTgBeforePcMode = trxTalkGroupOrPcId;// save the current TG
			nonVolatileSettings.overrideTG =  menuUtilityReceivedPcId;
			trxTalkGroupOrPcId = menuUtilityReceivedPcId;
			settingsPrivateCallMuteMode=false;
			menuUtilityRenderQSOData();
		}
		menuUtilityReceivedPcId = 0;
		qsodata_timer=1;// time out the qso timer will force the VFO or Channel mode screen to redraw its normal display
		return true;// The event has been handled
	}
	return false;// The event has not been handled
}

void menuUtilityRenderQSOData()
{
	char buffer[32];// buffer passed to the DMR ID lookup function, needs to be large enough to hold worst case text length that is returned. Currently 16+1
	dmrIdDataStruct_t currentRec;

	menuUtilityReceivedPcId=0;//reset the received PcId

	if ((LinkHead->talkGroupOrPcId>>24) == PC_CALL_FLAG)
	{
		// Its a Private call
		dmrIDLookup( (LinkHead->id & 0xFFFFFF),&currentRec);
		sprintf(buffer,"%s", currentRec.text);
		UC1701_printCentered(16, buffer,UC1701_FONT_GD77_8x16);

		// Are we already in PC mode to this caller ?
		if (trxTalkGroupOrPcId != (LinkHead->id | (PC_CALL_FLAG<<24)))
		{
			// No either we are not in PC mode or not on a Private Call to this station
			UC1701_printCentered(32, "Accept call?",UC1701_FONT_GD77_8x16);
			UC1701_printCentered(48, "YES          NO",UC1701_FONT_GD77_8x16);
			menuUtilityReceivedPcId = LinkHead->id | (PC_CALL_FLAG<<24);
		    set_melody(melody_private_call);
		}
		else
		{
			UC1701_printCentered(32, "Private call",UC1701_FONT_GD77_8x16);
		}
	}
	else
	{
		// Group call
		sprintf(buffer,"TG %d", (LinkHead->talkGroupOrPcId & 0xFFFFFF));
		UC1701_printCentered(16, buffer,UC1701_FONT_GD77_8x16);

		// first check if we have this ID in the DMR ID data
		if (dmrIDLookup( LinkHead->id,&currentRec))
		{
			sprintf(buffer,"%s", currentRec.text);
			UC1701_printCentered(32, buffer,UC1701_FONT_GD77_8x16);
		}
		else
		{
			// We don't have this ID, so try looking in the Talker alias data
			if (LinkHead->talkerAlias[0] != 0x00)
			{
				if (strlen(LinkHead->talkerAlias)> 6)
				{
					// More than 1 line wide of text, so we need to split onto 2 lines.
					memcpy(buffer,LinkHead->talkerAlias,6);
					buffer[6]=0x00;
					UC1701_printCentered(32, buffer,UC1701_FONT_GD77_8x16);

					memcpy(buffer,&LinkHead->talkerAlias[6],16);
					buffer[16]=0x00;
					UC1701_printAt(0,48,buffer,UC1701_FONT_GD77_8x16);
				}
				else
				{
					UC1701_printCentered(32,LinkHead->talkerAlias,UC1701_FONT_GD77_8x16);
				}
			}
			else
			{
				// No talker alias. So we can only show the ID.
				sprintf(buffer,"ID: %d", LinkHead->id);
				UC1701_printCentered(32, buffer,UC1701_FONT_GD77_8x16);
			}
		}
	}
}

void menuUtilityRenderHeader()
{
	char buffer[24];

	switch(trxGetMode())
	{
		case RADIO_MODE_ANALOG:
			strcpy(buffer, "FM");
			if ((currentChannelData->txTone!=65535)||(currentChannelData->rxTone!=65535))
			{
				strcat(buffer," C");
			}
			if (currentChannelData->txTone!=65535)
			{
				strcat(buffer,"T");
			}
			if (currentChannelData->rxTone!=65535)
			{
				strcat(buffer,"R");
			}
			break;
		case RADIO_MODE_DIGITAL:
			strcpy(buffer, "DMR");
			break;
	}

	UC1701_printAt(0,8, buffer,UC1701_FONT_6X8);

	if (trxGetMode() == RADIO_MODE_DIGITAL && settingsPrivateCallMuteMode == true)
	{
		UC1701_printCentered(8, "MUTE",UC1701_FONT_6X8);
	}

	int  batteryPerentage = (int)(((battery_voltage - CUTOFF_VOLTAGE_UPPER_HYST) * 100) / (BATTERY_MAX_VOLTAGE - CUTOFF_VOLTAGE_UPPER_HYST));
	if (batteryPerentage>100)
	{
		batteryPerentage=100;
	}
	if (batteryPerentage<0)
	{
		batteryPerentage=0;
	}
	sprintf(buffer,"%d%%",batteryPerentage);
	UC1701_printCore(0,8,buffer,UC1701_FONT_6X8,2,false);// Display battery percentage at the right
}
