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
#include "menu/menuSystem.h"
#include "menu/menuUtilityQSOData.h"
#include "fw_trx.h"
#include "fw_codeplug.h"
#include "fw_settings.h"

static void updateScreen();
static void handleEvent(int buttons, int keys, int events);
static void loadChannelData(bool useChannelDataInMemory);
static struct_codeplugZone_t currentZone;
static struct_codeplugRxGroup_t rxGroupData;
static struct_codeplugContact_t contactData;
static int currentIndexInTRxGroup=0;
static char currentZoneName[17];
static int directChannelNumber=0;
static bool displaySquelch=false;

int menuChannelMode(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		nonVolatileSettings.initialMenuNumber = MENU_CHANNEL_MODE;// This menu.
		codeplugZoneGetDataForIndex(nonVolatileSettings.currentZone,&currentZone);
		codeplugUtilConvertBufToString(currentZone.name,currentZoneName,16);// need to convert to zero terminated string
		if (channelScreenChannelData.rxFreq != 0)
		{
			loadChannelData(true);
		}
		else
		{
			loadChannelData(false);
		}
		currentChannelData = &channelScreenChannelData;
		menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		updateScreen();
	}
	else
	{
		if (events==0)
		{
			// is there an incoming DMR signal
			if (menuDisplayQSODataState != QSO_DISPLAY_IDLE)
			{
				updateScreen();
			}
		}
		else
		{
			handleEvent(buttons, keys, events);
		}
	}
	return 0;
}
uint16_t byteSwap16(uint16_t in)
{
	return ((in &0xff << 8) | (in >>8));
}

static void loadChannelData(bool useChannelDataInMemory)
{
	if (!useChannelDataInMemory)
	{
		if (strcmp(currentZoneName,"All Channels")==0)
		{
			codeplugChannelGetDataForIndex(nonVolatileSettings.currentChannelIndexInAllZone,&channelScreenChannelData);
		}
		else
		{
			codeplugChannelGetDataForIndex(currentZone.channels[nonVolatileSettings.currentChannelIndexInZone],&channelScreenChannelData);
		}
	}

	trxSetFrequency(channelScreenChannelData.rxFreq);
	if (channelScreenChannelData.chMode == RADIO_MODE_ANALOG)
	{
		trxSetModeAndBandwidth(channelScreenChannelData.chMode, ((channelScreenChannelData.flag4 & 0x02) == 0x02));
	}
	else
	{
		trxSetModeAndBandwidth(channelScreenChannelData.chMode, false);
	}
	trxSetDMRColourCode(channelScreenChannelData.rxColor);
	trxSetPower(nonVolatileSettings.txPower);
	trxSetTxCTCSS(channelScreenChannelData.txTone);
	trxSetRxCTCSS(channelScreenChannelData.rxTone);

	codeplugRxGroupGetDataForIndex(channelScreenChannelData.rxGroupList,&rxGroupData);
	codeplugContactGetDataForIndex(rxGroupData.contacts[currentIndexInTRxGroup],&contactData);
	if (nonVolatileSettings.overrideTG == 0)
	{
		trxTalkGroup = contactData.tgNumber;
	}
	else
	{
		trxTalkGroup = nonVolatileSettings.overrideTG;
	}
}

static void updateScreen()
{
	char nameBuf[17];
	int channelNumber;
	char buffer[32];
	UC1701_clearBuf();

	menuUtilityRenderHeader();

	switch(menuDisplayQSODataState)
	{
		case QSO_DISPLAY_DEFAULT_SCREEN:
			codeplugUtilConvertBufToString(channelScreenChannelData.name,nameBuf,16);
			UC1701_printCentered(32, (char *)nameBuf,UC1701_FONT_GD77_8x16);

			if (strcmp(currentZoneName,"All Channels") == 0)
			{
				channelNumber=nonVolatileSettings.currentChannelIndexInAllZone;
				if (directChannelNumber>0)
				{
					sprintf(nameBuf,"Goto %d",directChannelNumber);
				}
				else
				{
					sprintf(nameBuf,"CH %d",channelNumber);
				}
				UC1701_printCentered(50, (char *)nameBuf,UC1701_FONT_6X8);
			}
			else
			{
				sprintf(nameBuf,"%s",currentZoneName);
				UC1701_printCentered(50, (char *)nameBuf,UC1701_FONT_6X8);
			}

			if (trxGetMode() == RADIO_MODE_DIGITAL)
			{
				if (nonVolatileSettings.overrideTG != 0)
				{
					sprintf(nameBuf,"TG %d",trxTalkGroup);
				}
				else
				{
					codeplugUtilConvertBufToString(contactData.name,nameBuf,16);
				}
				UC1701_printCentered(16, (char *)nameBuf,UC1701_FONT_GD77_8x16);
			}
			else if(displaySquelch)
			{
				sprintf(buffer,"Squelch");
				UC1701_printAt(0,16,buffer,UC1701_FONT_GD77_8x16);
				int bargraph= 1 + ((currentChannelData->sql-1)*5)/2 ;
				UC1701_fillRect(62,21,bargraph,8,false);
				displaySquelch=false;
			}

			displayLightTrigger();
			UC1701_render();
			break;

		case QSO_DISPLAY_CALLER_DATA:
			menuUtilityRenderQSOData();
			displayLightTrigger();
			UC1701_render();
			break;
	}

	menuDisplayQSODataState = QSO_DISPLAY_IDLE;
}

static void handleEvent(int buttons, int keys, int events)
{
	if (events & 0x02)
	{
		if (buttons & BUTTON_ORANGE)
		{
			menuSystemPushNewMenu(MENU_ZONE_LIST);
			return;
		}
	}

	if ((keys & KEY_GREEN)!=0)
	{
		if (directChannelNumber>0)
		{
			if (codeplugChannelIndexIsValid(directChannelNumber))
			{
				nonVolatileSettings.currentChannelIndexInAllZone=directChannelNumber;
				loadChannelData(false);
			}
			else
			{
				set_melody(melody_ERROR_beep);
			}
			directChannelNumber=0;
			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
			updateScreen();
		}
		else if (buttons & BUTTON_SK2 )
		{
			menuSystemPushNewMenu(MENU_CHANNEL_DETAILS);
		}
		else
		{
			menuSystemPushNewMenu(MENU_MAIN_MENU);
		}
		return;
	}
	else if ((keys & KEY_HASH)!=0)
	{
		if (trxGetMode() == RADIO_MODE_DIGITAL)
		{
			menuSystemPushNewMenu(MENU_NUMERICAL_ENTRY);
			return;
		}
	}
	else if ((keys & KEY_RED)!=0)
	{
		if(directChannelNumber>0)
		{
			directChannelNumber=0;
			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
			updateScreen();
		}
		else
		{
			menuSystemSetCurrentMenu(MENU_VFO_MODE);
			return;
		}
	}


	if ((keys & KEY_RIGHT)!=0)
	{
		if (trxGetMode() == RADIO_MODE_DIGITAL)
		{
			currentIndexInTRxGroup++;
			if (currentIndexInTRxGroup > (rxGroupData.NOT_IN_MEMORY_numTGsInGroup -1))
			{
				currentIndexInTRxGroup =  0;
			}
			codeplugContactGetDataForIndex(rxGroupData.contacts[currentIndexInTRxGroup],&contactData);

			nonVolatileSettings.overrideTG = 0;// setting the override TG to 0 indicates the TG is not overridden
			trxTalkGroup = contactData.tgNumber;

			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
			updateScreen();
		}
		else
		{
			if(currentChannelData->sql==0)			//If we were using default squelch level
			{
				currentChannelData->sql=10;			//start the adjustment from that point.
			}
			else
			{
				if (currentChannelData->sql < CODEPLUG_MAX_VARIABLE_SQUELCH)

				{
					currentChannelData->sql++;
				}
			}

			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
			displaySquelch=true;
			updateScreen();
		}

	}
	else if ((keys & KEY_LEFT)!=0)
	{
		if (trxGetMode() == RADIO_MODE_DIGITAL)
		{
			// To Do change TG in on same channel freq
			currentIndexInTRxGroup--;
			if (currentIndexInTRxGroup < 0)
			{
				currentIndexInTRxGroup =  rxGroupData.NOT_IN_MEMORY_numTGsInGroup - 1;
			}

			codeplugContactGetDataForIndex(rxGroupData.contacts[currentIndexInTRxGroup],&contactData);
			nonVolatileSettings.overrideTG = 0;// setting the override TG to 0 indicates the TG is not overridden
			trxTalkGroup = contactData.tgNumber;

			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
			updateScreen();
		}
		else
		{
			if(currentChannelData->sql==0)			//If we were using default squelch level
			{
				currentChannelData->sql=10;			//start the adjustment from that point.
			}
			else
			{
				if (currentChannelData->sql > CODEPLUG_MIN_VARIABLE_SQUELCH)
				{
					currentChannelData->sql--;
				}
			}

			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
			displaySquelch=true;
			updateScreen();
		}

	}
	else if ((keys & KEY_STAR)!=0)
	{
		if (trxGetMode() == RADIO_MODE_ANALOG)
		{
			channelScreenChannelData.chMode = RADIO_MODE_DIGITAL;
			trxSetModeAndBandwidth(RADIO_MODE_DIGITAL, false);
		}
		else
		{
			channelScreenChannelData.chMode = RADIO_MODE_ANALOG;
			trxSetModeAndBandwidth(RADIO_MODE_ANALOG, ((channelScreenChannelData.flag4 & 0x02) == 0x02));
			trxSetTxCTCSS(currentChannelData->rxTone);
		}
		menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		updateScreen();
	}
	else if ((keys & KEY_DOWN)!=0)
	{
		if (strcmp(currentZoneName,"All Channels")==0)
		{
			do
			{
				nonVolatileSettings.currentChannelIndexInAllZone--;
				if (nonVolatileSettings.currentChannelIndexInAllZone<1)
				{
					nonVolatileSettings.currentChannelIndexInAllZone=1024;
				}
			} while(!codeplugChannelIndexIsValid(nonVolatileSettings.currentChannelIndexInAllZone));
		}
		else
		{
			nonVolatileSettings.currentChannelIndexInZone--;
			if (nonVolatileSettings.currentChannelIndexInZone < 0)
			{
				nonVolatileSettings.currentChannelIndexInZone =  currentZone.NOT_IN_MEMORY_numChannelsInZone - 1;
			}
		}
		loadChannelData(false);
		menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		updateScreen();
	}
	else if ((keys & KEY_UP)!=0)
	{
		if (strcmp(currentZoneName,"All Channels")==0)
		{
			do
			{
				nonVolatileSettings.currentChannelIndexInAllZone++;
				if (nonVolatileSettings.currentChannelIndexInAllZone>1024)
				{
					nonVolatileSettings.currentChannelIndexInAllZone=1;
				}
			} while(!codeplugChannelIndexIsValid(nonVolatileSettings.currentChannelIndexInAllZone));
		}
		else
		{
			nonVolatileSettings.currentChannelIndexInZone++;
			if (nonVolatileSettings.currentChannelIndexInZone > currentZone.NOT_IN_MEMORY_numChannelsInZone - 1)
			{
				nonVolatileSettings.currentChannelIndexInZone = 0;
			}
		}
		loadChannelData(false);
		menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		updateScreen();
	}
	else if (strcmp(currentZoneName,"All Channels")==0)
	{
		int keyval=99;
		if ((keys& KEY_1)!=0)
		{
			keyval=1;
		}
		if ((keys& KEY_2)!=0)
		{
			keyval=2;
		}
		if ((keys& KEY_3)!=0)
		{
			keyval=3;
		}
		if ((keys& KEY_4)!=0)
		{
			keyval=4;
		}
		if ((keys& KEY_5)!=0)
		{
			keyval=5;
		}
		if ((keys& KEY_6)!=0)
		{
			keyval=6;
		}
		if ((keys& KEY_7)!=0)
		{
			keyval=7;
		}
		if ((keys& KEY_8)!=0)
		{
			keyval=8;
		}
		if ((keys& KEY_9)!=0)
		{
			keyval=9;
		}
		if ((keys& KEY_0)!=0)
		{
			keyval=0;
		}

		if (keyval<10)
		{
			directChannelNumber=(directChannelNumber*10) + keyval;
			if(directChannelNumber>1024) directChannelNumber=0;
			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
			updateScreen();
		}
	}
}
