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
#include <user_interface/menuSystem.h>
#include <user_interface/menuUtilityQSOData.h>
#include "fw_trx.h"
#include "fw_codeplug.h"
#include "fw_settings.h"


static void handleEvent(int buttons, int keys, int events);
static void loadChannelData(bool useChannelDataInMemory);
static void scanning(void);
static struct_codeplugZone_t currentZone;
static struct_codeplugRxGroup_t rxGroupData;
static struct_codeplugContact_t contactData;
static char currentZoneName[17];
static int directChannelNumber=0;
static bool displaySquelch=false;
int currentChannelNumber=0;
static bool isDisplayingQSOData=false;
static bool isTxRxFreqSwap=false;
static bool scanActive=false;
static int scanTimer=0;
static int scanState=0;					//state flag for scan routine
static int scanShortPause=500;			//time to wait after carrier detected to allow time for full signal detection. (CTCSS or DMR)
static int scanPause=5000;				//time to wait after valid signal is detected.
static int scanInterval=50;			    //time between each scan step


int menuChannelMode(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		nonVolatileSettings.initialMenuNumber = MENU_CHANNEL_MODE;// This menu.
		currentChannelData = &channelScreenChannelData;// Need to set this as currentChannelData is used by functions called by loadChannelData()
		lastHeardClearLastID();

		if (channelScreenChannelData.rxFreq != 0)
		{
			loadChannelData(true);
		}
		else
		{
			isTxRxFreqSwap = false;
			codeplugZoneGetDataForNumber(nonVolatileSettings.currentZone,&currentZone);
			codeplugUtilConvertBufToString(currentZone.name,currentZoneName,16);// need to convert to zero terminated string
			loadChannelData(false);
		}

		menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		RssiUpdateCounter = RSSI_UPDATE_COUNTER_RELOAD;
		menuChannelModeUpdateScreen(0);

	}
	else
	{
		if (events==0)
		{
			// is there an incoming DMR signal
			if (menuDisplayQSODataState != QSO_DISPLAY_IDLE)
			{
				menuChannelModeUpdateScreen(0);
			}
			else
			{
				if (RssiUpdateCounter-- == 0)
				{
					drawRSSIBarGraph();
					UC1701RenderRows(1,2);// Only render the second row which contains the bar graph, as there is no need to redraw the rest of the screen
					//UC1701_render();
					RssiUpdateCounter = RSSI_UPDATE_COUNTER_RELOAD;
				}
			}
			if(scanActive)
			{
				scanning();
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

	if (strcmp(currentZoneName,"All Channels")==0)
	{
		settingsCurrentChannelNumber = nonVolatileSettings.currentChannelIndexInAllZone;
	}
	else
	{
		settingsCurrentChannelNumber = currentZone.channels[nonVolatileSettings.currentChannelIndexInZone];
	}

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

	trxSetFrequency(channelScreenChannelData.rxFreq,channelScreenChannelData.txFreq,DMR_MODE_AUTO);

	if (channelScreenChannelData.chMode == RADIO_MODE_ANALOG)
	{
		trxSetModeAndBandwidth(channelScreenChannelData.chMode, ((channelScreenChannelData.flag4 & 0x02) == 0x02));
		trxSetTxCTCSS(channelScreenChannelData.txTone);
		trxSetRxCTCSS(channelScreenChannelData.rxTone);
	}
	else
	{
		trxSetModeAndBandwidth(channelScreenChannelData.chMode, false);// bandwidth false = 12.5Khz as DMR uses 12.5kHz
		trxSetDMRColourCode(channelScreenChannelData.rxColor);

		codeplugRxGroupGetDataForIndex(channelScreenChannelData.rxGroupList,&rxGroupData);
		// Check if this channel has an Rx Group
		if (rxGroupData.name[0]!=0)
		{
			codeplugContactGetDataForIndex(rxGroupData.contacts[nonVolatileSettings.currentIndexInTRxGroupList],&contactData);
		}
		else
		{
			codeplugContactGetDataForIndex(channelScreenChannelData.contact,&contactData);
		}

		trxUpdateTsForCurrentChannelWithSpecifiedContact(&contactData);

		if (nonVolatileSettings.overrideTG == 0)
		{
			trxTalkGroupOrPcId = contactData.tgNumber;
		}
		else
		{
			trxTalkGroupOrPcId = nonVolatileSettings.overrideTG;
		}

		if ((nonVolatileSettings.tsManualOverride & 0x0F) != 0)
		{
			trxSetDMRTimeSlot ((nonVolatileSettings.tsManualOverride & 0x0F) -1);
		}
	}
}

void menuChannelModeUpdateScreen(int txTimeSecs)
{
	char nameBuf[17];
	int channelNumber;
	char buffer[32];
	int verticalPositionOffset = 0;
	UC1701_clearBuf();

	menuUtilityRenderHeader();

	switch(menuDisplayQSODataState)
	{
		case QSO_DISPLAY_DEFAULT_SCREEN:
			isDisplayingQSOData=false;
			menuUtilityReceivedPcId = 0x00;
			if (trxIsTransmitting)
			{
				sprintf(buffer," %d ",txTimeSecs);
				UC1701_printCentered(TX_TIMER_Y_OFFSET, buffer,UC1701_FONT_16x32);
				verticalPositionOffset=16;
			}
			else
			{
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
					UC1701_printCentered(50 , (char *)nameBuf,UC1701_FONT_6x8);
				}
				else
				{
					snprintf(nameBuf, 16, "%s",currentZoneName);
					nameBuf[16] = 0;
					UC1701_printCentered(50, (char *)nameBuf,UC1701_FONT_6x8);
				}
			}

			codeplugUtilConvertBufToString(channelScreenChannelData.name,nameBuf,16);
			UC1701_printCentered(32 + verticalPositionOffset, (char *)nameBuf,UC1701_FONT_8x16);

			if (trxGetMode() == RADIO_MODE_DIGITAL)
			{
				if (nonVolatileSettings.overrideTG != 0)
				{
					if((trxTalkGroupOrPcId>>24) == TG_CALL_FLAG)
					{
						sprintf(nameBuf,"TG %d",(trxTalkGroupOrPcId & 0x00FFFFFF));
					}
					else
					{
						dmrIdDataStruct_t currentRec;
						dmrIDLookup((trxTalkGroupOrPcId & 0x00FFFFFF),&currentRec);
						snprintf(nameBuf, 16, "%s",currentRec.text);
						nameBuf[16] = 0;
					}
				}
				else
				{
					codeplugUtilConvertBufToString(contactData.name,nameBuf,16);
				}
				UC1701_printCentered(CONTACT_Y_POS + verticalPositionOffset, (char *)nameBuf,UC1701_FONT_8x16);
			}
			else if(displaySquelch && !trxIsTransmitting)
			{
				sprintf(buffer,"Squelch");
				UC1701_printAt(0,16,buffer,UC1701_FONT_8x16);
				int bargraph= 1 + ((currentChannelData->sql-1)*5)/2 ;
				UC1701_fillRect(62,21,bargraph,8,false);
				displaySquelch=false;
			}

			if (!((scanActive) & (scanState==0)))
			{
			displayLightTrigger();
			}

			UC1701_render();
			break;

		case QSO_DISPLAY_CALLER_DATA:
			isDisplayingQSOData=true;
			menuUtilityRenderQSOData();
			displayLightTrigger();
			UC1701_render();
			break;
	}

	menuDisplayQSODataState = QSO_DISPLAY_IDLE;
}

static void handleEvent(int buttons, int keys, int events)
{
	uint32_t tg = (LinkHead->talkGroupOrPcId & 0xFFFFFF);
	// If Blue button is pressed during reception it sets the Tx TG to the incoming TG
	if (isDisplayingQSOData && (buttons & BUTTON_SK2)!=0 && trxGetMode() == RADIO_MODE_DIGITAL && trxTalkGroupOrPcId != tg)
	{
		lastHeardClearLastID();
		trxTalkGroupOrPcId = tg;
		nonVolatileSettings.overrideTG = trxTalkGroupOrPcId;
		menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		menuChannelModeUpdateScreen(0);
		return;
	}


	if (events & 0x02)
	{
		if (buttons & BUTTON_ORANGE)
		{
			scanActive=false;
			if (buttons & BUTTON_SK2)
			{
				settingsPrivateCallMuteMode = !settingsPrivateCallMuteMode;// Toggle PC mute only mode
				menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
				menuChannelModeUpdateScreen(0);
			}
			else
			{
				// ToDo Quick Menu
				menuSystemPushNewMenu(MENU_CHANNEL_QUICK_MENU);
			}
			return;
		}

	}

	if ((keys & KEY_GREEN)!=0)
	{
		if (menuUtilityHandlePrivateCallActions(buttons,keys,events))
		{
			return;
		}

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
			menuChannelModeUpdateScreen(0);
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
		scanActive=false;
		if (trxGetMode() == RADIO_MODE_DIGITAL)
		{
			menuSystemPushNewMenu(MENU_NUMERICAL_ENTRY);
			return;
		}
	}
	else if ((keys & KEY_RED)!=0)
	{
		scanActive=false;
		if (menuUtilityHandlePrivateCallActions(buttons,keys,events))
		{
			return;
		}
		if(directChannelNumber>0)
		{
			directChannelNumber=0;
			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
			menuChannelModeUpdateScreen(0);
		}
		else
		{
			menuSystemSetCurrentMenu(MENU_VFO_MODE);
			return;
		}
	}
	else if ((keys & KEY_RIGHT)!=0)
	{
		scanActive=false;
		if (buttons & BUTTON_SK2)
		{
			if (nonVolatileSettings.txPowerLevel < 7)
			{
				nonVolatileSettings.txPowerLevel++;
				trxSetPowerFromLevel(nonVolatileSettings.txPowerLevel);
				menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
				menuChannelModeUpdateScreen(0);
			}
		}
		else
		{
			if (trxGetMode() == RADIO_MODE_DIGITAL)
			{
				if (nonVolatileSettings.overrideTG == 0)
				{
					nonVolatileSettings.currentIndexInTRxGroupList++;
					if (nonVolatileSettings.currentIndexInTRxGroupList
							> (rxGroupData.NOT_IN_MEMORY_numTGsInGroup - 1))
					{
						nonVolatileSettings.currentIndexInTRxGroupList = 0;
					}
				}
				nonVolatileSettings.tsManualOverride &= 0xF0; // remove TS override for channel

				if (rxGroupData.name[0]!=0)
				{
					codeplugContactGetDataForIndex(rxGroupData.contacts[nonVolatileSettings.currentIndexInTRxGroupList],&contactData);
				}

				trxUpdateTsForCurrentChannelWithSpecifiedContact(&contactData);

				nonVolatileSettings.overrideTG = 0;// setting the override TG to 0 indicates the TG is not overridden
				trxTalkGroupOrPcId = contactData.tgNumber;
				lastHeardClearLastID();
				menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
				menuChannelModeUpdateScreen(0);
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
				menuChannelModeUpdateScreen(0);
			}
		}

	}
	else if ((keys & KEY_LEFT)!=0)
	{
		scanActive=false;
		if (buttons & BUTTON_SK2)
		{
			if (nonVolatileSettings.txPowerLevel > 0)
			{
				nonVolatileSettings.txPowerLevel--;
				trxSetPowerFromLevel(nonVolatileSettings.txPowerLevel);
				menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
				menuChannelModeUpdateScreen(0);
			}
		}
		else
			{
			if (trxGetMode() == RADIO_MODE_DIGITAL)
			{
				// To Do change TG in on same channel freq
				if (nonVolatileSettings.overrideTG == 0)
				{
					nonVolatileSettings.currentIndexInTRxGroupList--;
					if (nonVolatileSettings.currentIndexInTRxGroupList < 0)
					{
						nonVolatileSettings.currentIndexInTRxGroupList =
								rxGroupData.NOT_IN_MEMORY_numTGsInGroup - 1;
					}
				}
				nonVolatileSettings.tsManualOverride &= 0xF0; // remove TS override from channel
				if (rxGroupData.name[0]!=0)
				{
					codeplugContactGetDataForIndex(rxGroupData.contacts[nonVolatileSettings.currentIndexInTRxGroupList],&contactData);
				}
				trxUpdateTsForCurrentChannelWithSpecifiedContact(&contactData);

				nonVolatileSettings.overrideTG = 0;// setting the override TG to 0 indicates the TG is not overridden
				trxTalkGroupOrPcId = contactData.tgNumber;
				lastHeardClearLastID();
				menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
				menuChannelModeUpdateScreen(0);
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
				menuChannelModeUpdateScreen(0);
			}

		}
	}
	else if ((keys & KEY_STAR)!=0)
	{
		scanActive=false;
		// Toggle TimeSlot
		if (buttons & BUTTON_SK2 )
		{
			if (trxGetMode() == RADIO_MODE_ANALOG)
			{
				channelScreenChannelData.chMode = RADIO_MODE_DIGITAL;
				trxSetModeAndBandwidth(channelScreenChannelData.chMode, false);
			}
			else
			{
				channelScreenChannelData.chMode = RADIO_MODE_ANALOG;
				trxSetModeAndBandwidth(channelScreenChannelData.chMode, ((channelScreenChannelData.flag4 & 0x02) == 0x02));
				trxSetTxCTCSS(currentChannelData->rxTone);
			}
			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
			menuChannelModeUpdateScreen(0);
		}
		else
		{
			if (trxGetMode() == RADIO_MODE_DIGITAL)
			{
				// Toggle timeslot
				trxSetDMRTimeSlot(1-trxGetDMRTimeSlot());
				nonVolatileSettings.tsManualOverride &= 0xF0;// Clear lower nibble value
				nonVolatileSettings.tsManualOverride |= (trxGetDMRTimeSlot()+1);// Store manual TS override

				//	init_digital();
				clearActiveDMRID();
				lastHeardClearLastID();
				menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
				menuChannelModeUpdateScreen(0);
			}
			else
			{
				set_melody(melody_ERROR_beep);
			}
		}
	}
	else if ((keys & KEY_DOWN)!=0)
	{
		scanActive=false;
		if (buttons & BUTTON_SK2)
		{
			int numZones = codeplugZonesGetCount();

			if (nonVolatileSettings.currentZone == 0) {
				nonVolatileSettings.currentZone = numZones-1;
			}
			else
			{
				nonVolatileSettings.currentZone--;
			}
			nonVolatileSettings.overrideTG = 0; // remove any TG override
			nonVolatileSettings.tsManualOverride &= 0xF0; // remove TS override from channel
			nonVolatileSettings.currentChannelIndexInZone = 0;// Since we are switching zones the channel index should be reset
			channelScreenChannelData.rxFreq=0x00; // Flag to the Channel screeen that the channel data is now invalid and needs to be reloaded
			menuSystemPopAllAndDisplaySpecificRootMenu(MENU_CHANNEL_MODE);
		}
		else
		{
			lastHeardClearLastID();
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
		}
		loadChannelData(false);
		menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		menuChannelModeUpdateScreen(0);
	}
	else if ((keys & KEY_UP)!=0)
	{
		if (buttons & BUTTON_SK2)
		{
			scanActive=false;
			int numZones = codeplugZonesGetCount();

			nonVolatileSettings.currentZone++;
			if (nonVolatileSettings.currentZone >= numZones) {
				nonVolatileSettings.currentZone = 0;
			}
			nonVolatileSettings.overrideTG = 0; // remove any TG override
			nonVolatileSettings.tsManualOverride &= 0xF0; // remove TS override from channel
			nonVolatileSettings.currentChannelIndexInZone = 0;// Since we are switching zones the channel index should be reset
			channelScreenChannelData.rxFreq=0x00; // Flag to the Channel screen that the channel data is now invalid and needs to be reloaded
			menuSystemPopAllAndDisplaySpecificRootMenu(MENU_CHANNEL_MODE);
		}
		else
		{
			lastHeardClearLastID();
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
			scanTimer=500;
			scanState=0;
		}
		loadChannelData(false);
		menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		menuChannelModeUpdateScreen(0);
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
			scanActive=false;
			directChannelNumber=(directChannelNumber*10) + keyval;
			if(directChannelNumber>1024) directChannelNumber=0;
			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
			menuChannelModeUpdateScreen(0);
		}
	}
}

// Quick Menu functions

enum CHANNEL_SCREEN_QUICK_MENU_ITEMS { CH_SCREEN_QUICK_MENU_SCAN=0, CH_SCREEN_QUICK_MENU_COPY2VFO, CH_SCREEN_QUICK_MENU_COPY_FROM_VFO,
	NUM_CH_SCREEN_QUICK_MENU_ITEMS };// The last item in the list is used so that we automatically get a total number of items in the list

static void updateQuickMenuScreen()
{
	int mNum = 0;
	char buf[17];

	UC1701_clearBuf();
	menuDisplayTitle("Quick Menu");

	for(int i =- 1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_CH_SCREEN_QUICK_MENU_ITEMS, i);

		switch(mNum)
		{
			case CH_SCREEN_QUICK_MENU_SCAN:
				strcpy(buf, "Scan");
				break;
			case CH_SCREEN_QUICK_MENU_COPY2VFO:
				strcpy(buf, "Channel --> VFO");
				break;
			case CH_SCREEN_QUICK_MENU_COPY_FROM_VFO:
				strcpy(buf, "VFO --> Channel");
				break;
			default:
				strcpy(buf, "");
		}

		menuDisplayEntry(i, mNum, buf);
	}

	UC1701_render();
	displayLightTrigger();
}
/*
static void handleTxRxFreqToggle()
{
	isTxRxFreqSwap = !isTxRxFreqSwap;
	if (isTxRxFreqSwap)
	{
		trxSetFrequency(channelScreenChannelData.txFreq,channelScreenChannelData.rxFreq);
	}
	else
	{
		trxSetFrequency(channelScreenChannelData.txFreq,channelScreenChannelData.rxFreq);
	}
}
*/
static void handleQuickMenuEvent(int buttons, int keys, int events)
{
	if ((keys & KEY_RED)!=0)
	{
		menuSystemPopPreviousMenu();
		return;
	}
	else if ((keys & KEY_GREEN)!=0)
	{
		switch(gMenusCurrentItemIndex)
		{
			case CH_SCREEN_QUICK_MENU_SCAN:
				scanActive=true;
				scanTimer=500;
				scanState=0;
				menuSystemPopAllAndDisplaySpecificRootMenu(MENU_CHANNEL_MODE);
				break;
			case CH_SCREEN_QUICK_MENU_COPY2VFO:
				memcpy(&nonVolatileSettings.vfoChannel.rxFreq,&channelScreenChannelData.rxFreq,sizeof(struct_codeplugChannel_t) - 16);// Don't copy the name of channel, which are in the first 16 bytes
				menuSystemPopAllAndDisplaySpecificRootMenu(MENU_VFO_MODE);
				break;
			case CH_SCREEN_QUICK_MENU_COPY_FROM_VFO:
				memcpy(&channelScreenChannelData.rxFreq,&nonVolatileSettings.vfoChannel.rxFreq,sizeof(struct_codeplugChannel_t)- 16);// Don't copy the name of the vfo, which are in the first 16 bytes

				codeplugChannelSaveDataForIndex(settingsCurrentChannelNumber,&channelScreenChannelData);

				menuSystemPopAllAndDisplaySpecificRootMenu(MENU_CHANNEL_MODE);
				break;
		}
		return;
	}
	else if ((keys & KEY_DOWN)!=0)
	{
		MENU_INC(gMenusCurrentItemIndex, NUM_CH_SCREEN_QUICK_MENU_ITEMS);
	}
	else if ((keys & KEY_UP)!=0)
	{
		MENU_DEC(gMenusCurrentItemIndex, NUM_CH_SCREEN_QUICK_MENU_ITEMS);
	}

	updateQuickMenuScreen();
}

int menuChannelModeQuickMenu(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		gMenusCurrentItemIndex=0;
		updateQuickMenuScreen();
	}
	else
	{
		if (events!=0 && keys!=0)
		{
			handleQuickMenuEvent(buttons, keys, events);
		}
	}
	return 0;
}

//Scan Mode

static void scanning(void)
{
	if(trxIsTransmitting)															//stop scanning if we press the PTT
	{
		scanActive=false;
		return;
	}

	if((scanState==0) & (scanTimer==10))							    			//after initial settling time
	{
		if(trx_carrier_detected())												 	//test for presence of RF Carrier
		{
			scanTimer=scanShortPause;												//start short delay to allow full detection of signal
			scanState=1;															//state 1 = pause and test for valid signal that produces audio
		}
	}

	if(scanState==1)
	{
	    if (GPIO_PinRead(GPIO_audio_amp_enable, Pin_audio_amp_enable)==1)	    	// if speaker on we must be receiving a signal
	    {
	    	scanTimer=scanPause;													//extend the time before resuming scan.
	    }
	}

	if(scanTimer>0)
	{
		scanTimer--;
	}
	else
	{
		trx_measure_count=0;														//needed to allow time for Rx to settle after channel change.
			handleEvent(0,KEY_UP,0);												//Increment the channel
		if(channelScreenChannelData.flag4 & 0x20)									//if this channel has the skip bit set
		{
			scanTimer=10;															//skip over it quickly. (immediate selection of another channel seems to cause crashes)
		}
		else
		{
			scanTimer=scanInterval;
			scanState=0;															//state 0 = settling and test for carrier present.
		}

	}


}

