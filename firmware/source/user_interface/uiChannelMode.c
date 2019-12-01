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
#include <user_interface/uiLocalisation.h>
#include "fw_trx.h"
#include "fw_codeplug.h"
#include "fw_settings.h"


static void handleEvent(int buttons, int keys, int events);
static void loadChannelData(bool useChannelDataInMemory);
static void scanning(void);
static void handleUpKey(int buttons);
static struct_codeplugZone_t currentZone;
static struct_codeplugRxGroup_t rxGroupData;
static struct_codeplugContact_t contactData;
static char currentZoneName[17];
static int directChannelNumber=0;
static bool displaySquelch=false;
int currentChannelNumber=0;
static bool isDisplayingQSOData=false;
static bool isTxRxFreqSwap=false;
typedef enum
{
	SCAN_SCANNING = 0,
	SCAN_SHORT_PAUSED,
	SCAN_PAUSED,
} ScanZoneState_t;


static int scanTimer=0;
static ScanZoneState_t scanState = SCAN_SCANNING;		//state flag for scan routine
bool uiChannelModeScanActive = false;					//scan active flag
static const int SCAN_SHORT_PAUSE_TIME = 500;			//time to wait after carrier detected to allow time for full signal detection. (CTCSS or DMR)
static const int SCAN_INTERVAL = 50;			    //time between each scan step

static int tmpQuickMenuDmrFilterLevel;
#define MAX_ZONE_SCAN_NUISANCE_CHANNELS 16
static int nuisanceDelete[MAX_ZONE_SCAN_NUISANCE_CHANNELS];
static int nuisanceDeleteIndex = 0;


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
		if (uiChannelModeScanActive==false)
		{
			scanState = SCAN_SCANNING;
		}
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
			if(uiChannelModeScanActive==true)
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

#if 0 // rename: we have an union declared (fw_sound.c) with the same name.
uint16_t byteSwap16(uint16_t in)
{
	return ((in &0xff << 8) | (in >>8));
}
#endif

static void loadChannelData(bool useChannelDataInMemory)
{

	if (strcmp(currentZoneName,currentLanguage->all_channels)==0)
	{
		settingsCurrentChannelNumber = nonVolatileSettings.currentChannelIndexInAllZone;
	}
	else
	{
		settingsCurrentChannelNumber = currentZone.channels[nonVolatileSettings.currentChannelIndexInZone];
	}

	if (!useChannelDataInMemory)
	{
		if (strcmp(currentZoneName,currentLanguage->all_channels)==0)
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
		if (rxGroupData.name[0]!=0 && nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE] < rxGroupData.NOT_IN_MEMORY_numTGsInGroup)
		{
			codeplugContactGetDataForIndex(rxGroupData.contacts[nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE]],&contactData);
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
	char buffer[33];
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
						sprintf(nameBuf,currentLanguage->gotoChannel,directChannelNumber);
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
				sprintf(buffer,currentLanguage->squelch);
				UC1701_printAt(0,16,buffer,UC1701_FONT_8x16);
				int bargraph= 1 + ((currentChannelData->sql-1)*5)/2 ;
				UC1701_fillRect(62,21,bargraph,8,false);
				displaySquelch=false;
			}

			if (!((uiChannelModeScanActive) & (scanState==SCAN_SCANNING)))
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

	if((scanState==SCAN_PAUSED) && (keys & KEY_DOWN) && (!(buttons & BUTTON_SK2)))                    // if we are scanning and down key is pressed then enter current channel into nuisance delete array.
	{
		nuisanceDelete[nuisanceDeleteIndex++]=settingsCurrentChannelNumber;
		if(nuisanceDeleteIndex > (MAX_ZONE_SCAN_NUISANCE_CHANNELS - 1))
		{
			nuisanceDeleteIndex = 0;																			//rolling list of last MAX_NUISANCE_CHANNELS deletes.
		}
		scanTimer=5;																				//force scan to continue;
		scanState=SCAN_SCANNING;
		return;
	}

	if ((uiChannelModeScanActive) && (!(keys==0)) && (!((keys & KEY_UP) && (!(buttons & BUTTON_SK2)))))    // stop the scan on any button except UP without Shift ( allows scan to be manually continued) or SK2 on its own (allows Backlight to be triggered)
	{
		uiChannelModeScanActive = false;
		displayLightTrigger();
		return;
	}


	// If Blue button is pressed during reception it sets the Tx TG to the incoming TG
	if (isDisplayingQSOData && (buttons & BUTTON_SK2)!=0 && trxGetMode() == RADIO_MODE_DIGITAL &&
			(trxTalkGroupOrPcId != tg ||
			(dmrMonitorCapturedTS!=-1 && dmrMonitorCapturedTS != trxGetDMRTimeSlot()) ||
			(dmrMonitorCapturedCC!=-1 && dmrMonitorCapturedCC != trxGetDMRColourCode())))
	{
		lastHeardClearLastID();


		// Set TS to overriden TS
		if (dmrMonitorCapturedTS!=-1 && dmrMonitorCapturedTS != trxGetDMRTimeSlot())
		{
			trxSetDMRTimeSlot(dmrMonitorCapturedTS);
			nonVolatileSettings.tsManualOverride &= 0xF0;// Clear lower nibble value
			nonVolatileSettings.tsManualOverride |= (dmrMonitorCapturedTS+1);// Store manual TS override
		}
		if (trxTalkGroupOrPcId != tg)
		{
			trxTalkGroupOrPcId = tg;
			nonVolatileSettings.overrideTG = trxTalkGroupOrPcId;
		}

		if(dmrMonitorCapturedCC!=-1 && dmrMonitorCapturedCC != trxGetDMRColourCode())
		{
			trxSetDMRColourCode(dmrMonitorCapturedCC);
		}

		menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		menuChannelModeUpdateScreen(0);
		return;
	}


	if (events & 0x02)
	{
		if (buttons & BUTTON_ORANGE)
		{
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

	if (KEYCHECK_SHORTUP(keys,KEY_GREEN))
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
	else if (KEYCHECK_SHORTUP(keys,KEY_HASH))
	{
		if (trxGetMode() == RADIO_MODE_DIGITAL)
		{
			if ((buttons & BUTTON_SK2) != 0)
			{
				menuSystemPushNewMenu(MENU_CONTACT_QUICKLIST);
			} else {
				menuSystemPushNewMenu(MENU_NUMERICAL_ENTRY);
			}
			return;
		}
	}
	else if (KEYCHECK_SHORTUP(keys,KEY_RED))
	{
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
	else if (KEYCHECK_PRESS(keys,KEY_RIGHT))
	{
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
					nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE]++;
					if (nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE]
							> (rxGroupData.NOT_IN_MEMORY_numTGsInGroup - 1))
					{
						nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE] = 0;
					}
				}
				nonVolatileSettings.tsManualOverride &= 0xF0; // remove TS override for channel

				if (rxGroupData.name[0]!=0 && nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE] < rxGroupData.NOT_IN_MEMORY_numTGsInGroup)
				{
					codeplugContactGetDataForIndex(rxGroupData.contacts[nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE]],&contactData);
				}
				else
				{
					codeplugContactGetDataForIndex(channelScreenChannelData.contact,&contactData);
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
	else if (KEYCHECK_PRESS(keys,KEY_LEFT))
	{
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
					nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE]--;
					if (nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE] < 0)
					{
						nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE] =
								rxGroupData.NOT_IN_MEMORY_numTGsInGroup - 1;
					}
				}
				nonVolatileSettings.tsManualOverride &= 0xF0; // remove TS override from channel
				if (rxGroupData.name[0]!=0 && nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE] < rxGroupData.NOT_IN_MEMORY_numTGsInGroup)
				{
					codeplugContactGetDataForIndex(rxGroupData.contacts[nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE]],&contactData);
				}
				else
				{
					codeplugContactGetDataForIndex(channelScreenChannelData.contact,&contactData);
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
	else if (KEYCHECK_PRESS(keys,KEY_STAR))
	{
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
	else if (KEYCHECK_PRESS(keys,KEY_DOWN))
	{
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
	else if (KEYCHECK_PRESS(keys,KEY_UP))
	{
		handleUpKey(buttons);
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
			menuChannelModeUpdateScreen(0);
		}
	}
}


static void handleUpKey(int buttons)
{
	if (buttons & BUTTON_SK2)
	{
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
		if (strcmp(currentZoneName,currentLanguage->all_channels)==0)
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
		scanState = SCAN_SCANNING;
	}
	loadChannelData(false);
	menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
	menuChannelModeUpdateScreen(0);
}

// Quick Menu functions

enum CHANNEL_SCREEN_QUICK_MENU_ITEMS { CH_SCREEN_QUICK_MENU_SCAN=0, CH_SCREEN_QUICK_MENU_COPY2VFO, CH_SCREEN_QUICK_MENU_COPY_FROM_VFO,
	CH_SCREEN_QUICK_MENU_DMR_FILTER,
	NUM_CH_SCREEN_QUICK_MENU_ITEMS };// The last item in the list is used so that we automatically get a total number of items in the list

static void updateQuickMenuScreen(void)
{
	int mNum = 0;
	char buf[33];

	UC1701_clearBuf();
	menuDisplayTitle(currentLanguage->quick_menu);

	for(int i =- 1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_CH_SCREEN_QUICK_MENU_ITEMS, i);

		switch(mNum)
		{
			case CH_SCREEN_QUICK_MENU_SCAN:
				strcpy(buf,currentLanguage->scan);
				break;
			case CH_SCREEN_QUICK_MENU_COPY2VFO:
				strcpy(buf, currentLanguage->channelToVfo);
				break;
			case CH_SCREEN_QUICK_MENU_COPY_FROM_VFO:
				strcpy(buf, currentLanguage->vfoToChannel);
				break;
			case CH_SCREEN_QUICK_MENU_DMR_FILTER:
				sprintf(buf, currentLanguage->filter,DMR_FILTER_LEVELS[tmpQuickMenuDmrFilterLevel]);
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
static void handleTxRxFreqToggle(void)
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
	if (KEYCHECK_SHORTUP(keys,KEY_RED))
	{
		uiChannelModeScanActive=false;
		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_SHORTUP(keys,KEY_GREEN))
	{
		switch(gMenusCurrentItemIndex)
		{
			case CH_SCREEN_QUICK_MENU_SCAN:
				for(int i= 0;i<MAX_ZONE_SCAN_NUISANCE_CHANNELS;i++)						//clear all nuisance delete channels at start of scanning
				{
					nuisanceDelete[i]=-1;
					nuisanceDeleteIndex=0;
				}
				uiChannelModeScanActive=true;
				scanTimer=500;
				scanState = SCAN_SCANNING;
				menuSystemPopAllAndDisplaySpecificRootMenu(MENU_CHANNEL_MODE);
				break;
			case CH_SCREEN_QUICK_MENU_COPY2VFO:
				memcpy(&settingsVFOChannel[nonVolatileSettings.currentVFONumber].rxFreq,&channelScreenChannelData.rxFreq,sizeof(struct_codeplugChannel_t) - 16);// Don't copy the name of channel, which are in the first 16 bytes
				menuSystemPopAllAndDisplaySpecificRootMenu(MENU_VFO_MODE);
				break;
			case CH_SCREEN_QUICK_MENU_COPY_FROM_VFO:
				memcpy(&channelScreenChannelData.rxFreq,&settingsVFOChannel[nonVolatileSettings.currentVFONumber].rxFreq,sizeof(struct_codeplugChannel_t)- 16);// Don't copy the name of the vfo, which are in the first 16 bytes

				codeplugChannelSaveDataForIndex(settingsCurrentChannelNumber,&channelScreenChannelData);

				menuSystemPopAllAndDisplaySpecificRootMenu(MENU_CHANNEL_MODE);
				break;
			case CH_SCREEN_QUICK_MENU_DMR_FILTER:
				nonVolatileSettings.dmrFilterLevel = tmpQuickMenuDmrFilterLevel;
				menuSystemPopAllAndDisplaySpecificRootMenu(MENU_CHANNEL_MODE);
				break;
		}
		return;
	}
	else if (KEYCHECK_PRESS(keys,KEY_RIGHT))
	{
		switch(gMenusCurrentItemIndex)
		{
			case CH_SCREEN_QUICK_MENU_DMR_FILTER:
				if (tmpQuickMenuDmrFilterLevel < DMR_FILTER_CC_TS)
				{
					tmpQuickMenuDmrFilterLevel++;
				}
				break;
		}
	}
	else if (KEYCHECK_PRESS(keys,KEY_LEFT))
	{
		switch(gMenusCurrentItemIndex)
		{
			case CH_SCREEN_QUICK_MENU_DMR_FILTER:
				if (tmpQuickMenuDmrFilterLevel > DMR_FILTER_CC)
				{
					tmpQuickMenuDmrFilterLevel--;
				}
				break;
		}
	}
	else if (KEYCHECK_PRESS(keys,KEY_DOWN))
	{
		MENU_INC(gMenusCurrentItemIndex, NUM_CH_SCREEN_QUICK_MENU_ITEMS);
	}
	else if (KEYCHECK_PRESS(keys,KEY_UP))
	{
		MENU_DEC(gMenusCurrentItemIndex, NUM_CH_SCREEN_QUICK_MENU_ITEMS);
	}

	updateQuickMenuScreen();
}



int menuChannelModeQuickMenu(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		uiChannelModeScanActive=false;
		tmpQuickMenuDmrFilterLevel = nonVolatileSettings.dmrFilterLevel;
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
	if((scanState==SCAN_SCANNING) && (scanTimer>5) && (scanTimer< (SCAN_INTERVAL -20)))							    			//after initial settling time
	{
		//test for presence of RF Carrier.
		// In FM mode the dmr slot_state will always be DMR_STATE_IDLE
		if (slot_state != DMR_STATE_IDLE)
		{
			scanState=SCAN_PAUSED;
			scanTimer=nonVolatileSettings.scanDelay*1000;
		}
		else
		{
			if(trx_carrier_detected() )
			{
				scanTimer=SCAN_SHORT_PAUSE_TIME;												//start short delay to allow full detection of signal
				scanState=SCAN_SHORT_PAUSED;															//state 1 = pause and test for valid signal that produces audio
			}
		}
	}

	if(((scanState==SCAN_PAUSED)&&(nonVolatileSettings.scanModePause==false)) || (scanState==SCAN_SHORT_PAUSED))   // only do this once if scan mode is PAUSE do it every time if scan mode is HOLD
	{
	    if (GPIO_PinRead(GPIO_audio_amp_enable, Pin_audio_amp_enable)==1)	    	// if speaker on we must be receiving a signal so extend the time before resuming scan.
	    {
	    	scanTimer=nonVolatileSettings.scanDelay*1000;
	    	scanState=SCAN_PAUSED;
	    }
	}

	if(scanTimer>0)
	{
		scanTimer--;
	}
	else
	{
		trx_measure_count=0;														//needed to allow time for Rx to settle after channel change.
		handleUpKey(0);
		scanTimer=SCAN_INTERVAL;
		scanState = SCAN_SCANNING;													//state 0 = settling and test for carrier present.

		if (strcmp(currentZoneName,currentLanguage->all_channels)==0)
		{
			if(channelScreenChannelData.flag4 & 0x10)									//if this channel has the All Skip bit set
			{
				scanTimer=5;															//skip over it quickly. (immediate selection of another channel seems to cause crashes)
			}
		}
		else
		{
			if(channelScreenChannelData.flag4 & 0x20)									//if this channel has the Zone Skip skip bit set
			{
				scanTimer=5;															//skip over it quickly. (immediate selection of another channel seems to cause crashes)
			}
		}

		for(int i=0;i<MAX_ZONE_SCAN_NUISANCE_CHANNELS;i++)														//check all nuisance delete entries and skip channel if there is a match
		{
			if (nuisanceDelete[i]==-1)
			{
				break;
			}
			else
			{
				if(nuisanceDelete[i]==settingsCurrentChannelNumber)
				{
					scanTimer=5;
					break;
				}
			}
		}
	}
}
