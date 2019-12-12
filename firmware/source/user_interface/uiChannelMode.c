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


static void handleEvent(ui_event_t *ev);
static void loadChannelData(bool useChannelDataInMemory);
static void scanning(void);
static void startScan(void);
static void handleUpKey(ui_event_t *ev);

static void updateQuickMenuScreen(void);
static void handleQuickMenuEvent(ui_event_t *ev);

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
static const int SCAN_TOTAL_INTERVAL = 30;			    //time between each scan step
static const int SCAN_FREQ_CHANGE_SETTLING_INTERVAL = 1;//Time after frequency is changed before RSSI sampling starts
static const int SCAN_SKIP_CHANNEL_INTERVAL = 1;		//This is actually just an implicit flag value to indicate the channel should be skipped


#define MAX_ZONE_SCAN_NUISANCE_CHANNELS 16
static int nuisanceDelete[MAX_ZONE_SCAN_NUISANCE_CHANNELS];
static int nuisanceDeleteIndex = 0;

static int tmpQuickMenuDmrFilterLevel;

int menuChannelMode(ui_event_t *ev, bool isFirstRun)
{
	static uint32_t m = 0, sqm = 0;

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
			codeplugZoneGetDataForNumber(nonVolatileSettings.currentZone, &currentZone);
			codeplugUtilConvertBufToString(currentZone.name, currentZoneName, 16);// need to convert to zero terminated string
			loadChannelData(false);
		}

		menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		menuChannelModeUpdateScreen(0);
		if (uiChannelModeScanActive==false)
		{
			scanState = SCAN_SCANNING;
		}
	}
	else
	{
		if (ev->events == NO_EVENT)
		{
			// is there an incoming DMR signal
			if (menuDisplayQSODataState != QSO_DISPLAY_IDLE)
			{
				menuChannelModeUpdateScreen(0);
			}
			else
			{
				// Clear squelch region
				if (displaySquelch && ((ev->ticks - sqm) > 2000))
				{
					displaySquelch = false;

					UC1701_fillRect(0, 16, 128, 16, true);
					UC1701RenderRows(2,4);
				}

				if ((ev->ticks - m) > RSSI_UPDATE_COUNTER_RELOAD)
				{
					m = ev->ticks;
					drawRSSIBarGraph();
					UC1701RenderRows(1,2);// Only render the second row which contains the bar graph, as there is no need to redraw the rest of the screen
					//UC1701_render();
				}
			}

			if(uiChannelModeScanActive==true)
			{
				scanning();
			}
		}
		else
		{
			if (ev->hasEvent)
			{
				if ((trxGetMode() == RADIO_MODE_ANALOG) &&
						(ev->events & KEY_EVENT) && ((ev->keys & KEY_LEFT) || (ev->keys & KEY_RIGHT)))
				{
					sqm = ev->ticks;
				}

				handleEvent(ev);
			}
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
	int channelNumber;
	static const int nameBufferLen = 23;
	char nameBuf[nameBufferLen];
	static const int bufferLen = 17;
	char buffer[bufferLen];
	int verticalPositionOffset = 0;
	struct_codeplugContact_t contact;
	int contactIndex;

	UC1701_clearBuf();
	menuUtilityRenderHeader();

	switch(menuDisplayQSODataState)
	{
		case QSO_DISPLAY_DEFAULT_SCREEN:
			isDisplayingQSOData=false;
			menuUtilityReceivedPcId = 0x00;
			if (trxIsTransmitting)
			{
				snprintf(buffer, bufferLen, " %d ", txTimeSecs);
				buffer[bufferLen - 1] = 0;
				UC1701_printCentered(TX_TIMER_Y_OFFSET, buffer, UC1701_FONT_16x32);
				verticalPositionOffset=16;
			}
			else
			{
				if (strcmp(currentZoneName,currentLanguage->all_channels) == 0)
				{
					channelNumber=nonVolatileSettings.currentChannelIndexInAllZone;
					if (directChannelNumber>0)
					{
						snprintf(nameBuf, nameBufferLen, "%s %d", currentLanguage->gotoChannel, directChannelNumber);
					}
					else
					{
						snprintf(nameBuf, nameBufferLen, "%s Ch:%d",currentLanguage->all_channels, channelNumber);
					}
					nameBuf[nameBufferLen - 1] = 0;
					UC1701_printCentered(50 , nameBuf, UC1701_FONT_6x8);
				}
				else
				{
					channelNumber=nonVolatileSettings.currentChannelIndexInZone+1;
					if (directChannelNumber>0)
					{
						snprintf(nameBuf, nameBufferLen, "%s %d", currentLanguage->gotoChannel, directChannelNumber);
						nameBuf[nameBufferLen - 1] = 0;
					}
					else
					{
						snprintf(nameBuf, nameBufferLen, "%s Ch:%d", currentZoneName,channelNumber);
						nameBuf[nameBufferLen - 1] = 0;
					}
					UC1701_printCentered(50, (char *)nameBuf,UC1701_FONT_6x8);
				}
			}

			codeplugUtilConvertBufToString(channelScreenChannelData.name, nameBuf, 16);
			UC1701_printCentered(32 + verticalPositionOffset, nameBuf, UC1701_FONT_8x16);

			if (trxGetMode() == RADIO_MODE_DIGITAL)
			{
				if (nonVolatileSettings.overrideTG != 0)
				{
					if((trxTalkGroupOrPcId>>24) == TG_CALL_FLAG)
					{
						contactIndex = codeplugContactIndexByTGorPC((trxTalkGroupOrPcId & 0x00FFFFFF), CONTACT_CALLTYPE_TG, &contact);
						if (contactIndex == 0) {
							snprintf(nameBuf, bufferLen, "TG %d", (trxTalkGroupOrPcId & 0x00FFFFFF));
						} else {
							codeplugUtilConvertBufToString(contact.name, nameBuf, 16);
						}
					}
					else
					{
						contactIndex = codeplugContactIndexByTGorPC((trxTalkGroupOrPcId & 0x00FFFFFF), CONTACT_CALLTYPE_PC, &contact);
						if (contactIndex == 0) {
							dmrIdDataStruct_t currentRec;
							dmrIDLookup((trxTalkGroupOrPcId & 0x00FFFFFF), &currentRec);
							strncpy(nameBuf, currentRec.text, bufferLen);
						} else {
							codeplugUtilConvertBufToString(contact.name, nameBuf, 16);
						}
					}
					nameBuf[bufferLen - 1] = 0;
					UC1701_drawRect(0, CONTACT_Y_POS + verticalPositionOffset, 128, 16, true);
				}
				else
				{
					codeplugUtilConvertBufToString(contactData.name, nameBuf, 16);
				}
				UC1701_printCentered(CONTACT_Y_POS + verticalPositionOffset, nameBuf, UC1701_FONT_8x16);
			}
			// Squelch will be cleared later, 2000 ticks after last change
			else if(displaySquelch && !trxIsTransmitting)
			{
				strncpy(buffer, currentLanguage->squelch, 8);
				buffer[7] = 0; // Avoid overlap with bargraph
				UC1701_printAt(0, 16, buffer, UC1701_FONT_8x16);
				int bargraph= 1 + ((currentChannelData->sql-1)*5)/2 ;
				UC1701_fillRect(62, 21, bargraph, 8, false);
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

static void handleEvent(ui_event_t *ev)
{
	// if we are scanning and down key is pressed then enter current channel into nuisance delete array.
	if((scanState==SCAN_PAUSED) && ((ev->events & KEY_EVENT) && (KEYCHAR(ev->keys) == KEY_DOWN)) && (!(ev->buttons & BUTTON_SK2)))
	{
		nuisanceDelete[nuisanceDeleteIndex++]=settingsCurrentChannelNumber;
		if(nuisanceDeleteIndex > (MAX_ZONE_SCAN_NUISANCE_CHANNELS - 1))
		{
			nuisanceDeleteIndex = 0; //rolling list of last MAX_NUISANCE_CHANNELS deletes.
		}
		scanTimer = SCAN_SKIP_CHANNEL_INTERVAL;	//force scan to continue;
		scanState=SCAN_SCANNING;
		fw_reset_keyboard();
		return;
	}

	// stop the scan on any button except UP without Shift (allows scan to be manually continued)
	// or SK2 on its own (allows Backlight to be triggered)
	if (uiChannelModeScanActive &&
			( (ev->events & KEY_EVENT) && ( ((KEYCHAR(ev->keys) == KEY_UP) && ((ev->buttons & BUTTON_SK2) == 0)) == false ) ) )
	{
		uiChannelModeScanActive = false;
		displayLightTrigger();
		fw_reset_keyboard();
		return;
	}

	if (ev->events & BUTTON_EVENT)
	{
		uint32_t tg = (LinkHead->talkGroupOrPcId & 0xFFFFFF);

		// If Blue button is pressed during reception it sets the Tx TG to the incoming TG
		if (isDisplayingQSOData && (ev->buttons & BUTTON_SK2) && trxGetMode() == RADIO_MODE_DIGITAL &&
				(trxTalkGroupOrPcId != tg ||
				(dmrMonitorCapturedTS!=-1 && dmrMonitorCapturedTS != trxGetDMRTimeSlot())))
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

			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
			menuChannelModeUpdateScreen(0);
			return;
		}

		if (ev->buttons & BUTTON_ORANGE)
		{
			if (ev->buttons & BUTTON_SK2)
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

	if (KEYCHECK_SHORTUP(ev->keys,KEY_GREEN))
	{
		if (menuUtilityHandlePrivateCallActions(ev))
		{
			return;
		}

		if (directChannelNumber>0)
		{
			if(strcmp(currentZoneName,currentLanguage->all_channels)==0)
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
			}
			else
			{
				if (directChannelNumber-1<currentZone.NOT_IN_MEMORY_numChannelsInZone)
				{
					nonVolatileSettings.currentChannelIndexInZone=directChannelNumber-1;
					loadChannelData(false);
				}
				else
				{
					set_melody(melody_ERROR_beep);
				}

			}
			directChannelNumber=0;
			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
			menuChannelModeUpdateScreen(0);
		}
		else if (ev->buttons & BUTTON_SK2 )
		{
			menuSystemPushNewMenu(MENU_CHANNEL_DETAILS);
		}
		else
		{
			menuSystemPushNewMenu(MENU_MAIN_MENU);
		}
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_HASH))
	{
		if (trxGetMode() == RADIO_MODE_DIGITAL)
		{
			if ((ev->buttons & BUTTON_SK2) != 0)
			{
				menuSystemPushNewMenu(MENU_CONTACT_QUICKLIST);
			} else {
				menuSystemPushNewMenu(MENU_NUMERICAL_ENTRY);
			}
			return;
		}
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_RED))
	{
		if (menuUtilityHandlePrivateCallActions(ev))
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
	else if (KEYCHECK_PRESS(ev->keys,KEY_RIGHT))
	{
		if (ev->buttons & BUTTON_SK2)
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
				if(currentChannelData->sql == 0)			//If we were using default squelch level
				{
					currentChannelData->sql=nonVolatileSettings.squelchDefaults[trxCurrentBand[TRX_RX_FREQ_BAND]];			//start the adjustment from that point.
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
	else if (KEYCHECK_PRESS(ev->keys,KEY_LEFT))
	{
		if (ev->buttons & BUTTON_SK2)
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
				if(currentChannelData->sql == 0)			//If we were using default squelch level
				{
					currentChannelData->sql=nonVolatileSettings.squelchDefaults[trxCurrentBand[TRX_RX_FREQ_BAND]];			//start the adjustment from that point.
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
	else if (KEYCHECK_PRESS(ev->keys,KEY_STAR))
	{
		// Toggle TimeSlot
		if (ev->buttons & BUTTON_SK2 )
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
	else if (KEYCHECK_PRESS(ev->keys,KEY_DOWN))
	{
		if (ev->buttons & BUTTON_SK2)
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
			if (strcmp(currentZoneName,currentLanguage->all_channels)==0)
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
	else if (KEYCHECK_PRESS(ev->keys,KEY_UP))
	{
		handleUpKey(ev);
	}
	else
	{
		int keyval = menuGetKeypadKeyValue(ev, true);

		if (keyval < 10)
		{
			directChannelNumber=(directChannelNumber*10) + keyval;
			if (strcmp(currentZoneName,currentLanguage->all_channels)==0)
			{
				if(directChannelNumber>1024)
				{
					directChannelNumber=0;
					set_melody(melody_ERROR_beep);
				}
			}
			else
			{
				if(directChannelNumber>currentZone.NOT_IN_MEMORY_numChannelsInZone)
					{
						directChannelNumber=0;
						set_melody(melody_ERROR_beep);
					}

			}

			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
			menuChannelModeUpdateScreen(0);
		}
	}
}


static void handleUpKey(ui_event_t *ev)
{
	if (ev->buttons & BUTTON_SK2)
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
	static const int bufferLen = 17;
	char buf[bufferLen];

	UC1701_clearBuf();
	menuDisplayTitle(currentLanguage->quick_menu);

	for(int i =- 1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_CH_SCREEN_QUICK_MENU_ITEMS, i);
		buf[0] = 0;

		switch(mNum)
		{
			case CH_SCREEN_QUICK_MENU_SCAN:
				strncpy(buf, currentLanguage->scan, bufferLen);
				break;
			case CH_SCREEN_QUICK_MENU_COPY2VFO:
				strncpy(buf, currentLanguage->channelToVfo, bufferLen);
				break;
			case CH_SCREEN_QUICK_MENU_COPY_FROM_VFO:
				strncpy(buf, currentLanguage->vfoToChannel, bufferLen);
				break;
			case CH_SCREEN_QUICK_MENU_DMR_FILTER:
				snprintf(buf, bufferLen, "%s:%s", currentLanguage->filter, DMR_FILTER_LEVELS[tmpQuickMenuDmrFilterLevel]);
				break;
			default:
				strcpy(buf, "");
		}

		buf[bufferLen - 1] = 0;
		menuDisplayEntry(i, mNum, buf);
	}

	UC1701_render();
	displayLightTrigger();
}


static void handleQuickMenuEvent(ui_event_t *ev)
{
	if (KEYCHECK_SHORTUP(ev->keys,KEY_RED))
	{
		uiChannelModeScanActive=false;
		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_GREEN))
	{
		switch(gMenusCurrentItemIndex)
		{
			case CH_SCREEN_QUICK_MENU_SCAN:
				startScan();
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
	else if (KEYCHECK_PRESS(ev->keys,KEY_RIGHT))
	{
		switch(gMenusCurrentItemIndex)
		{
			case CH_SCREEN_QUICK_MENU_DMR_FILTER:
				if (tmpQuickMenuDmrFilterLevel < DMR_FILTER_TS)
				{
					tmpQuickMenuDmrFilterLevel++;
				}
				break;
		}
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_LEFT))
	{
		switch(gMenusCurrentItemIndex)
		{
			case CH_SCREEN_QUICK_MENU_DMR_FILTER:
				if (tmpQuickMenuDmrFilterLevel > DMR_FILTER_NONE)
				{
					tmpQuickMenuDmrFilterLevel--;
				}
				break;
		}
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_DOWN))
	{
		MENU_INC(gMenusCurrentItemIndex, NUM_CH_SCREEN_QUICK_MENU_ITEMS);
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_UP))
	{
		MENU_DEC(gMenusCurrentItemIndex, NUM_CH_SCREEN_QUICK_MENU_ITEMS);
	}
	else if (((ev->events & BUTTON_EVENT) && (ev->buttons & BUTTON_ORANGE)) && (gMenusCurrentItemIndex==CH_SCREEN_QUICK_MENU_SCAN))
	{
		startScan();
	}

	updateQuickMenuScreen();
}


int menuChannelModeQuickMenu(ui_event_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		uiChannelModeScanActive=false;
		tmpQuickMenuDmrFilterLevel = nonVolatileSettings.dmrFilterLevel;
		updateQuickMenuScreen();
	}
	else
	{
		if (ev->hasEvent)
			handleQuickMenuEvent(ev);
	}
	return 0;
}

//Scan Mode

static void startScan(void)
{
	for(int i= 0;i<MAX_ZONE_SCAN_NUISANCE_CHANNELS;i++)						//clear all nuisance delete channels at start of scanning
	{
		nuisanceDelete[i]=-1;
		nuisanceDeleteIndex=0;
	}
	uiChannelModeScanActive=true;
	scanTimer=500;
	scanState = SCAN_SCANNING;
	menuSystemPopAllAndDisplaySpecificRootMenu(MENU_CHANNEL_MODE);
}

static void scanning(void)
{
	if((scanState==SCAN_SCANNING) && (scanTimer > SCAN_SKIP_CHANNEL_INTERVAL) && (scanTimer < ( SCAN_TOTAL_INTERVAL - SCAN_FREQ_CHANGE_SETTLING_INTERVAL)))							    			//after initial settling time
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
		ui_event_t tmpEvent={ .buttons = 0, .keys = 0, .events = NO_EVENT, .hasEvent = 0, .ticks = 0 };

		handleUpKey(&tmpEvent);
		scanTimer = SCAN_TOTAL_INTERVAL;
		scanState = SCAN_SCANNING;													//state 0 = settling and test for carrier present.

		if (strcmp(currentZoneName,currentLanguage->all_channels)==0)
		{
			if(channelScreenChannelData.flag4 & 0x10)									//if this channel has the All Skip bit set
			{
				scanTimer=SCAN_SKIP_CHANNEL_INTERVAL;															//skip over it quickly. (immediate selection of another channel seems to cause crashes)
			}
		}
		else
		{
			if(channelScreenChannelData.flag4 & 0x20)									//if this channel has the Zone Skip skip bit set
			{
				scanTimer=SCAN_SKIP_CHANNEL_INTERVAL;															//skip over it quickly. (immediate selection of another channel seems to cause crashes)
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
					scanTimer=SCAN_SKIP_CHANNEL_INTERVAL;
					break;
				}
			}
		}
	}
}

void menuChannelColdStart(void)
{
	// Force to re-read codeplug data (needed due to "All Channels" translation)
	channelScreenChannelData.rxFreq = 0;
}
