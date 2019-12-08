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
#include <hardware/fw_HR-C6000.h>
#include <user_interface/menuSystem.h>
#include <user_interface/menuUtilityQSOData.h>
#include <user_interface/uiLocalisation.h>
#include "fw_trx.h"
#include "fw_settings.h"
#include "fw_codeplug.h"

enum VFO_SELECTED_FREQUENCY_INPUT  {VFO_SELECTED_FREQUENCY_INPUT_RX , VFO_SELECTED_FREQUENCY_INPUT_TX};

static char freq_enter_digits[8] = { '-', '-', '-', '-', '-', '-', '-', '-' };
static int freq_enter_idx = 0;
static int selectedFreq = VFO_SELECTED_FREQUENCY_INPUT_RX;

static struct_codeplugRxGroup_t rxGroupData;
static struct_codeplugContact_t contactData;


static bool displaySquelch=false;

// internal prototypes
static void handleEvent(int buttons, int keys, int events);
static void reset_freq_enter_digits(void);
static int read_freq_enter_digits(void);
static void update_frequency(int tmp_frequency);
static void stepFrequency(int increment);
static void loadContact(void);
static void toneScan(void);
static void CCscan(void);
static bool isDisplayingQSOData=false;
static int tmpQuickMenuDmrFilterLevel;

bool toneScanActive = false;					//tone scan active flag  (CTCSS)
bool CCScanActive=false;						//colour code scan active
static int scanTimer=0;
static const int TONESCANINTERVAL=200;			//time between each tone for lowest tone. (higher tones take less time.)
static const int CCSCANINTERVAL=500;
static int scanIndex=0;

// public interface
int menuVFOMode(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		RssiUpdateCounter = RSSI_UPDATE_COUNTER_RELOAD;
		isDisplayingQSOData=false;
		nonVolatileSettings.initialMenuNumber=MENU_VFO_MODE;
		currentChannelData = &settingsVFOChannel[nonVolatileSettings.currentVFONumber];
		settingsCurrentChannelNumber = -1;// This is not a regular channel. Its the special VFO channel!
		trxSetFrequency(currentChannelData->rxFreq,currentChannelData->txFreq,DMR_MODE_AUTO);

		//Need to load the Rx group if specified even if TG is currently overridden as we may need it later when the left or right button is pressed
		if (currentChannelData->rxGroupList != 0)
		{
			codeplugRxGroupGetDataForIndex(currentChannelData->rxGroupList,&rxGroupData);
		}

		if (currentChannelData->chMode == RADIO_MODE_ANALOG)
		{
			trxSetModeAndBandwidth(currentChannelData->chMode, ((currentChannelData->flag4 & 0x02) == 0x02));
			trxSetTxCTCSS(currentChannelData->txTone);
			if (!toneScanActive)
			{
				trxSetRxCTCSS(currentChannelData->rxTone);
			}
		}
		else
		{
			trxSetDMRColourCode(currentChannelData->rxColor);
			trxSetModeAndBandwidth(currentChannelData->chMode, false);

			if (nonVolatileSettings.overrideTG == 0)
			{
				if (currentChannelData->rxGroupList != 0)
				{
					loadContact();

					// Check whether the contact data seems valid
					if (contactData.name[0] == 0 || contactData.tgNumber ==0 || contactData.tgNumber > 9999999)
					{
						nonVolatileSettings.overrideTG = 9;// If the VFO does not have an Rx Group list assigned to it. We can't get a TG from the codeplug. So use TG 9.
						trxTalkGroupOrPcId = nonVolatileSettings.overrideTG;
						trxSetDMRTimeSlot(((currentChannelData->flag2 & 0x40)!=0));
					}
					else
					{
						trxTalkGroupOrPcId = contactData.tgNumber;
						trxUpdateTsForCurrentChannelWithSpecifiedContact(&contactData);
					}
				}
				else
				{
					nonVolatileSettings.overrideTG = 9;// If the VFO does not have an Rx Group list assigned to it. We can't get a TG from the codeplug. So use TG 9.
				}


			}
			else
			{
				trxTalkGroupOrPcId = nonVolatileSettings.overrideTG;
			}

			if ((nonVolatileSettings.tsManualOverride & 0xF0) != 0)
			{
				trxSetDMRTimeSlot(((nonVolatileSettings.tsManualOverride & 0xF0)>>4) - 1);
			}
		}
		lastHeardClearLastID();
		reset_freq_enter_digits();
		menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		menuVFOModeUpdateScreen(0);

	}
	else
	{
		if (events==0)
		{
			// is there an incoming DMR signal
			if (menuDisplayQSODataState != QSO_DISPLAY_IDLE)
			{
				menuVFOModeUpdateScreen(0);
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
			if(toneScanActive==true)
			{
				toneScan();
			}
			if(CCScanActive==true)
			{
				CCscan();
			}
		}
		else
		{
			handleEvent(buttons, keys, events);
			toneScanActive=false;
			if(CCScanActive==true)
			{
				CCScanActive=false;
				trxSetDMRColourCode(currentChannelData->rxColor);
			}

		}
	}
	return 0;
}

void menuVFOModeUpdateScreen(int txTimeSecs)
{
	int val_before_dp;
	int val_after_dp;
	static const int bufferLen = 17;
	char buffer[bufferLen];
	struct_codeplugContact_t contact;
	int contactIndex;

	UC1701_clearBuf();

	menuUtilityRenderHeader();

	switch(menuDisplayQSODataState)
	{
		case QSO_DISPLAY_DEFAULT_SCREEN:

			isDisplayingQSOData=false;
			menuUtilityReceivedPcId = 0x00;
			if (trxGetMode() == RADIO_MODE_DIGITAL)
			{
				if (nonVolatileSettings.overrideTG != 0)
				{
					if((trxTalkGroupOrPcId >> 24) == TG_CALL_FLAG)
					{
						contactIndex = codeplugContactIndexByTGorPC((trxTalkGroupOrPcId & 0x00FFFFFF), CONTACT_CALLTYPE_TG, &contact);
						if (contactIndex == 0) {
							snprintf(buffer, bufferLen, "TG %d", (trxTalkGroupOrPcId & 0x00FFFFFF));
						} else {
							codeplugUtilConvertBufToString(contact.name, buffer, 16);
						}
					}
					else
					{
						contactIndex = codeplugContactIndexByTGorPC((trxTalkGroupOrPcId & 0x00FFFFFF), CONTACT_CALLTYPE_PC, &contact);
						if (contactIndex == 0) {
							dmrIdDataStruct_t currentRec;
							dmrIDLookup((trxTalkGroupOrPcId & 0x00FFFFFF),&currentRec);
							strncpy(buffer, currentRec.text, bufferLen);
						} else {
							codeplugUtilConvertBufToString(contact.name, buffer, 16);
						}
					}
					if (trxIsTransmitting) {
						UC1701_drawRect(0, 34, 128, 16, true);
					} else {
						UC1701_drawRect(0, CONTACT_Y_POS, 128, 16, true);
					}
				}
				else
				{
					codeplugUtilConvertBufToString(contactData.name, buffer, 16);
				}

				buffer[bufferLen - 1] = 0;

				if (trxIsTransmitting)
				{
					UC1701_printCentered(34, buffer, UC1701_FONT_8x16);
				}
				else
				{
					UC1701_printCentered(CONTACT_Y_POS, buffer, UC1701_FONT_8x16);
				}
			}
			else
			{
				if(displaySquelch)
				{
					strncpy(buffer, currentLanguage->squelch, 8);
					buffer[7] = 0; // Avoid overlap with bargraph
					UC1701_printAt(0,16,buffer, UC1701_FONT_8x16);
					int bargraph= 1 + ((currentChannelData->sql - 1) * 5) /2;
					UC1701_fillRect(62, 21, bargraph, 8, false);
					displaySquelch=false;
				}
				if(toneScanActive==true)
				{
					sprintf(buffer,"CTCSS %d.%dHz",TRX_CTCSSTones[scanIndex]/10,TRX_CTCSSTones[scanIndex] % 10);
					UC1701_printCentered(16,buffer, UC1701_FONT_8x16);
				}

			}
			if (freq_enter_idx==0)
			{
				if (!trxIsTransmitting)
				{
					val_before_dp = currentChannelData->rxFreq/100000;
					val_after_dp = currentChannelData->rxFreq - val_before_dp*100000;
					snprintf(buffer, bufferLen, "%cR %d.%05d MHz", (selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_RX) ? '>' : ' ', val_before_dp, val_after_dp);
					buffer[bufferLen - 1 ] = 0;
					UC1701_printCentered(32, buffer, UC1701_FONT_8x16);
				}
				else
				{
					snprintf(buffer, bufferLen, " %d ", txTimeSecs);
					UC1701_printCentered(TX_TIMER_Y_OFFSET, buffer, UC1701_FONT_16x32);
				}

				val_before_dp = currentChannelData->txFreq/100000;
				val_after_dp = currentChannelData->txFreq - val_before_dp*100000;
				snprintf(buffer, bufferLen, "%cT %d.%05d MHz", (selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_TX || trxIsTransmitting) ? '>' : ' ', val_before_dp, val_after_dp);
				buffer[bufferLen - 1 ] = 0;
				UC1701_printCentered(48, buffer, UC1701_FONT_8x16);
			}
			else
			{
				snprintf(buffer, bufferLen, "%c%c%c.%c%c%c%c%c MHz", freq_enter_digits[0], freq_enter_digits[1], freq_enter_digits[2],
						freq_enter_digits[3], freq_enter_digits[4], freq_enter_digits[5], freq_enter_digits[6], freq_enter_digits[7]);
				if (selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_TX)
				{
					UC1701_printCentered(48, buffer, UC1701_FONT_8x16);
				}
				else
				{
					UC1701_printCentered(32, buffer, UC1701_FONT_8x16);
				}
			}

			displayLightTrigger();
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

static void reset_freq_enter_digits(void)
{
	for (int i=0;i<8;i++)
	{
		freq_enter_digits[i]='-';
	}
	freq_enter_idx = 0;
}

static int read_freq_enter_digits(void)
{
	int result=0;
	for (int i=0;i<8;i++)
	{
		result=result*10;
		if ((freq_enter_digits[i]>='0') && (freq_enter_digits[i]<='9'))
		{
			result=result+freq_enter_digits[i]-'0';
		}
	}
	return result;
}

static void update_frequency(int frequency)
{
	if (selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_TX)
	{
		if (trxGetBandFromFrequency(frequency)!=-1)
		{
			currentChannelData->txFreq = frequency;
			trxSetFrequency(currentChannelData->rxFreq,currentChannelData->txFreq,DMR_MODE_AUTO);
			set_melody(melody_ACK_beep);
		}
	}
	else
	{
		int deltaFrequency = frequency - currentChannelData->rxFreq;
		if (trxGetBandFromFrequency(frequency)!=-1)
		{
			currentChannelData->rxFreq = frequency;
			currentChannelData->txFreq = currentChannelData->txFreq + deltaFrequency;
			trxSetFrequency(currentChannelData->rxFreq,currentChannelData->txFreq,DMR_MODE_AUTO);

			if (trxGetBandFromFrequency(currentChannelData->txFreq)!=-1)
			{
				set_melody(melody_ACK_beep);
			}
			else
			{
				currentChannelData->txFreq = frequency;
				set_melody(melody_ERROR_beep);

			}
		}
		else
		{
			set_melody(melody_ERROR_beep);
		}
	}
}

static void checkAndFixIndexInRxGroup(void)
{
	if (nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber]
			> (rxGroupData.NOT_IN_MEMORY_numTGsInGroup - 1))
	{
		nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber] = 0;
	}
}

static void loadContact(void)
{
	// Check if this channel has an Rx Group
	if (rxGroupData.name[0]!=0 && nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber] < rxGroupData.NOT_IN_MEMORY_numTGsInGroup)
	{
		codeplugContactGetDataForIndex(rxGroupData.contacts[nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber]],&contactData);
	}
	else
	{
		codeplugContactGetDataForIndex(currentChannelData->contact,&contactData);
	}
}

static void handleEvent(int buttons, int keys, int events)
{
	uint32_t tg = (LinkHead->talkGroupOrPcId & 0xFFFFFF);
	// If Blue button is pressed during reception it sets the Tx TG to the incoming TG

	if (isDisplayingQSOData && (buttons & BUTTON_SK2)!=0 && trxGetMode() == RADIO_MODE_DIGITAL &&
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
		menuVFOModeUpdateScreen(0);
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
				menuVFOModeUpdateScreen(0);
			}
			else
			{
				menuSystemPushNewMenu(MENU_VFO_QUICK_MENU);
			}
			return;
		}
	}

	if (KEYCHECK_SHORTUP(keys,KEY_GREEN))
	{
		if (menuUtilityHandlePrivateCallActions(buttons,keys,events))
		{
			reset_freq_enter_digits();
			return;
		}
		if (buttons & BUTTON_SK2 )
		{
			menuSystemPushNewMenu(MENU_CHANNEL_DETAILS);
			reset_freq_enter_digits();
			return;
		}
		else
		{
			if (freq_enter_idx == 0)
			{
				menuSystemPushNewMenu(MENU_MAIN_MENU);
				return;
			}
		}
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
		}
		return;
	}

	if (freq_enter_idx==0)
	{
		if (KEYCHECK_SHORTUP(keys,KEY_STAR))
		{
			if (buttons & BUTTON_SK2 )
			{
				if (trxGetMode() == RADIO_MODE_ANALOG)
				{
					currentChannelData->chMode = RADIO_MODE_DIGITAL;
					trxSetModeAndBandwidth(currentChannelData->chMode, false);
					checkAndFixIndexInRxGroup();
					// Check if the contact data for the VFO has previous been loaded
					if (contactData.name[0] == 0x00)
					{
						loadContact();
					}
				}
				else
				{
					currentChannelData->chMode = RADIO_MODE_ANALOG;
					trxSetModeAndBandwidth(currentChannelData->chMode, ((channelScreenChannelData.flag4 & 0x02) == 0x02));
					trxSetTxCTCSS(currentChannelData->rxTone);
				}
				menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
			}
			else
			{
				if (trxGetMode() == RADIO_MODE_DIGITAL)
				{
					// Toggle TimeSlot
					trxSetDMRTimeSlot(1-trxGetDMRTimeSlot());
					nonVolatileSettings.tsManualOverride &= 0x0F;// Clear upper nibble value
					nonVolatileSettings.tsManualOverride |= (trxGetDMRTimeSlot()+1)<<4;// Store manual TS override for VFO in upper nibble

					//init_digital();
					clearActiveDMRID();
					lastHeardClearLastID();
					menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
					menuVFOModeUpdateScreen(0);
				}
				else
				{
	        	    set_melody(melody_ERROR_beep);
				}
			}
		}
		else if (KEYCHECK_PRESS(keys,KEY_DOWN))
		{
			if (buttons & BUTTON_SK2 )
			{
				selectedFreq = VFO_SELECTED_FREQUENCY_INPUT_TX;
			}
			else
			{
				stepFrequency(VFO_FREQ_STEP_TABLE[(currentChannelData->VFOflag5 >> 4)] * -1);
			}
			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		}
		else if (KEYCHECK_PRESS(keys,KEY_UP))
		{
			if (buttons & BUTTON_SK2 )
			{
				selectedFreq = VFO_SELECTED_FREQUENCY_INPUT_RX;
			}
			else
			{
				stepFrequency(VFO_FREQ_STEP_TABLE[(currentChannelData->VFOflag5 >> 4)]);
			}
			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;

		}
		else if (KEYCHECK_SHORTUP(keys,KEY_RED))
		{
			if (menuUtilityHandlePrivateCallActions(buttons,keys,events))
			{
				return;
			}
			menuSystemSetCurrentMenu(MENU_CHANNEL_MODE);
			return;
		}
		else
		if (KEYCHECK_PRESS(keys,KEY_RIGHT))
		{
			if (buttons & BUTTON_SK2)
			{
				if (nonVolatileSettings.txPowerLevel < 7)
				{
					nonVolatileSettings.txPowerLevel++;
					trxSetPowerFromLevel(nonVolatileSettings.txPowerLevel);
					menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
					menuVFOModeUpdateScreen(0);
				}
			}
			else
			{
				if (trxGetMode() == RADIO_MODE_DIGITAL)
				{
					if (nonVolatileSettings.overrideTG == 0)
					{
						nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber]++;
						checkAndFixIndexInRxGroup();
					}
					nonVolatileSettings.tsManualOverride &= 0x0F; // remove TS override for VFO

					// Check if this channel has an Rx Group
					if (rxGroupData.name[0]!=0 && nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber] < rxGroupData.NOT_IN_MEMORY_numTGsInGroup)
					{
						codeplugContactGetDataForIndex(rxGroupData.contacts[nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber]],&contactData);
					}
					else
					{
						codeplugContactGetDataForIndex(currentChannelData->contact,&contactData);
					}

					trxUpdateTsForCurrentChannelWithSpecifiedContact(&contactData);

					nonVolatileSettings.overrideTG = 0;// setting the override TG to 0 indicates the TG is not overridden
					trxTalkGroupOrPcId = contactData.tgNumber;
					lastHeardClearLastID();
					menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
					menuVFOModeUpdateScreen(0);
				}
				else
				{
					if(currentChannelData->sql == 0)			//If we were using default squelch level
					{
						currentChannelData->sql = nonVolatileSettings.squelchDefaults[trxCurrentBand[TRX_RX_FREQ_BAND]];			//start the adjustment from that point.
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
					menuVFOModeUpdateScreen(0);
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
					menuVFOModeUpdateScreen(0);
				}
			}
			else
			{
				if (trxGetMode() == RADIO_MODE_DIGITAL)
				{
					// To Do change TG in on same channel freq
					if (nonVolatileSettings.overrideTG == 0)
					{
						nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber]--;
						if (nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber] < 0)
						{
							nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber] =
									rxGroupData.NOT_IN_MEMORY_numTGsInGroup - 1;
						}
					}
					nonVolatileSettings.tsManualOverride &= 0x0F; // remove TS override for VFO

					// Check if this channel has an Rx Group
					if (rxGroupData.name[0]!=0 && nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber] < rxGroupData.NOT_IN_MEMORY_numTGsInGroup)
					{
						codeplugContactGetDataForIndex(rxGroupData.contacts[nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE + nonVolatileSettings.currentVFONumber]],&contactData);
					}
					else
					{
						codeplugContactGetDataForIndex(currentChannelData->contact,&contactData);
					}

					nonVolatileSettings.overrideTG = 0;// setting the override TG to 0 indicates the TG is not overridden
					trxTalkGroupOrPcId = contactData.tgNumber;

					trxUpdateTsForCurrentChannelWithSpecifiedContact(&contactData);

					lastHeardClearLastID();
					menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
					menuVFOModeUpdateScreen(0);
				}
				else
				{
					if(currentChannelData->sql == 0) //If we were using default squelch level
					{
						currentChannelData->sql = nonVolatileSettings.squelchDefaults[trxCurrentBand[TRX_RX_FREQ_BAND]];//start the adjustment from that point.
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
					menuVFOModeUpdateScreen(0);
				}
			}
		}
	}
	else
	{
		if (KEYCHECK_PRESS(keys,KEY_LEFT))
		{
			freq_enter_idx--;
			freq_enter_digits[freq_enter_idx]='-';
			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		}
		else if (KEYCHECK_SHORTUP(keys,KEY_RED))
		{
			reset_freq_enter_digits();
    	    set_melody(melody_NACK_beep);
    		menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		}
		else if (KEYCHECK_SHORTUP(keys, KEY_GREEN))
		{
			int tmp_frequency=read_freq_enter_digits();
			if (trxGetBandFromFrequency(tmp_frequency)!=-1)
			{
				update_frequency(tmp_frequency);
				reset_freq_enter_digits();
//	        	    set_melody(melody_ACK_beep);
			}
			else
			{
        	    set_melody(melody_ERROR_beep);
			}
			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		}
	}
	if (freq_enter_idx<8)
	{
		char c = keypressToNumberChar(keys);
		if (c!='\0')
		{
			freq_enter_digits[freq_enter_idx]=c;
			freq_enter_idx++;
			if (freq_enter_idx==8)
			{
				int tmp_frequency=read_freq_enter_digits();
				if (trxGetBandFromFrequency(tmp_frequency)!=-1)
				{
					update_frequency(tmp_frequency);
					reset_freq_enter_digits();
//	        	    set_melody(melody_ACK_beep);
				}
				else
				{
	        	    set_melody(melody_ERROR_beep);
				}
			}
			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		}
	}
//	menuVFOModeUpdateScreen(0);
}



static void stepFrequency(int increment)
{
	int tmp_frequencyTx;
	int tmp_frequencyRx;

	if (selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_TX)
	{
		tmp_frequencyTx  = currentChannelData->txFreq + increment;
		tmp_frequencyRx  = currentChannelData->rxFreq;// Needed later for the band limited checking
	}
	else
	{
		tmp_frequencyRx  = currentChannelData->rxFreq + increment;
		tmp_frequencyTx  = currentChannelData->txFreq + increment;
	}
	if (trxGetBandFromFrequency(tmp_frequencyRx)!=-1)
	{
		currentChannelData->txFreq = tmp_frequencyTx;
		currentChannelData->rxFreq =  tmp_frequencyRx;
		if (selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_RX)
		{
			trxSetFrequency(currentChannelData->rxFreq,currentChannelData->txFreq,DMR_MODE_AUTO);
		}
	}
	else
	{
		set_melody(melody_ERROR_beep);
	}
}

// Quick Menu functions
enum VFO_SCREEN_QUICK_MENU_ITEMS { 	VFO_SCREEN_QUICK_MENU_TX_SWAP_RX = 0, VFO_SCREEN_QUICK_MENU_BOTH_TO_RX, VFO_SCREEN_QUICK_MENU_BOTH_TO_TX,
									VFO_SCREEN_QUICK_MENU_DMR_FILTER,VFO_SCREEN_QUICK_MENU_VFO_A_B,VFO_SCREEN_CODE_SCAN,
									NUM_VFO_SCREEN_QUICK_MENU_ITEMS };// The last item in the list is used so that we automatically get a total number of items in the list

static void updateQuickMenuScreen(void)
{
	int mNum = 0;
	static const int bufferLen = 17;
	char buf[bufferLen];

	UC1701_clearBuf();
	menuDisplayTitle(currentLanguage->quick_menu);

	for(int i = -1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_VFO_SCREEN_QUICK_MENU_ITEMS, i);

		switch(mNum)
		{
			case VFO_SCREEN_QUICK_MENU_BOTH_TO_RX:
				strcpy(buf, "Rx --> Tx");
				break;
			case VFO_SCREEN_QUICK_MENU_TX_SWAP_RX:
				strcpy(buf, "Tx <--> Rx");
				break;
			case VFO_SCREEN_QUICK_MENU_BOTH_TO_TX:
				strcpy(buf, "Tx --> Rx");
				break;
			case VFO_SCREEN_QUICK_MENU_DMR_FILTER:
				snprintf(buf, bufferLen, "%s:%s", currentLanguage->filter, DMR_FILTER_LEVELS[tmpQuickMenuDmrFilterLevel]);
				break;
			case VFO_SCREEN_QUICK_MENU_VFO_A_B:
				sprintf(buf, "VFO:%c", ((nonVolatileSettings.currentVFONumber==0) ? 'A' : 'B'));
				break;
			case VFO_SCREEN_CODE_SCAN:
				if(trxGetMode() == RADIO_MODE_ANALOG)
				{
					strcpy(buf, "Tone scan");
				}
				else
				{
					strcpy(buf, "CC scan");
				}

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

static void handleQuickMenuEvent(int buttons, int keys, int events)
{
	if (KEYCHECK_SHORTUP(keys,KEY_RED))
	{
		toneScanActive=false;
		if(CCScanActive==true)
		{
			CCScanActive=false;
			trxSetDMRColourCode(currentChannelData->rxColor);
		}

		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_SHORTUP(keys,KEY_GREEN))
	{
		switch(gMenusCurrentItemIndex)
		{
			case VFO_SCREEN_QUICK_MENU_BOTH_TO_RX:
				currentChannelData->txFreq = currentChannelData->rxFreq;
				trxSetFrequency(currentChannelData->rxFreq,currentChannelData->txFreq,DMR_MODE_AUTO);
				break;
			case VFO_SCREEN_QUICK_MENU_TX_SWAP_RX:
				{
					int tmpFreq = currentChannelData->txFreq;
					currentChannelData->txFreq = currentChannelData->rxFreq;
					currentChannelData->rxFreq =  tmpFreq;
					trxSetFrequency(currentChannelData->rxFreq,currentChannelData->txFreq,DMR_MODE_AUTO);
				}
				break;
			case VFO_SCREEN_QUICK_MENU_BOTH_TO_TX:
				currentChannelData->rxFreq = currentChannelData->txFreq;
				trxSetFrequency(currentChannelData->rxFreq,currentChannelData->txFreq,DMR_MODE_AUTO);

				break;
			case VFO_SCREEN_QUICK_MENU_DMR_FILTER:
				nonVolatileSettings.dmrFilterLevel = tmpQuickMenuDmrFilterLevel;
				break;

			case VFO_SCREEN_CODE_SCAN:
				if(trxGetMode() == RADIO_MODE_ANALOG)
				{
					toneScanActive=true;
					scanTimer=TONESCANINTERVAL;
					scanIndex=1;
					trxSetRxCTCSS(TRX_CTCSSTones[scanIndex]);
					GPIO_PinWrite(GPIO_audio_amp_enable, Pin_audio_amp_enable,0);// turn off the audio amp
				}
				else
				{
					CCScanActive=true;
					scanTimer=CCSCANINTERVAL;
					scanIndex=0;
					trxSetDMRColourCode(scanIndex);
				}
				break;
		}
		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_PRESS(keys,KEY_RIGHT))
		{
			switch(gMenusCurrentItemIndex)
			{
				case VFO_SCREEN_QUICK_MENU_DMR_FILTER:
					if (tmpQuickMenuDmrFilterLevel < DMR_FILTER_TS)
					{
						tmpQuickMenuDmrFilterLevel++;
					}
					break;
				case VFO_SCREEN_QUICK_MENU_VFO_A_B:
					if (nonVolatileSettings.currentVFONumber==0)
					{
						nonVolatileSettings.currentVFONumber++;
						currentChannelData = &settingsVFOChannel[nonVolatileSettings.currentVFONumber];
					}
					break;

			}
		}
		else if (KEYCHECK_PRESS(keys,KEY_LEFT))
		{
			switch(gMenusCurrentItemIndex)
			{
				case VFO_SCREEN_QUICK_MENU_DMR_FILTER:
					if (tmpQuickMenuDmrFilterLevel > DMR_FILTER_NONE)
					{
						tmpQuickMenuDmrFilterLevel--;
					}
					break;
				case VFO_SCREEN_QUICK_MENU_VFO_A_B:
					if (nonVolatileSettings.currentVFONumber==1)
					{
						nonVolatileSettings.currentVFONumber--;
						currentChannelData = &settingsVFOChannel[nonVolatileSettings.currentVFONumber];
					}
					break;
			}
		}
	else if (KEYCHECK_PRESS(keys,KEY_DOWN))
	{
		MENU_INC(gMenusCurrentItemIndex, NUM_VFO_SCREEN_QUICK_MENU_ITEMS);
	}
	else if (KEYCHECK_PRESS(keys,KEY_UP))
	{
		MENU_DEC(gMenusCurrentItemIndex, NUM_VFO_SCREEN_QUICK_MENU_ITEMS);
	}
	updateQuickMenuScreen();
}

int menuVFOModeQuickMenu(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		tmpQuickMenuDmrFilterLevel = nonVolatileSettings.dmrFilterLevel;
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

void toneScan(void)
{
	if (GPIO_PinRead(GPIO_audio_amp_enable, Pin_audio_amp_enable)==1)
	{
		currentChannelData->txTone=TRX_CTCSSTones[scanIndex];
		currentChannelData->rxTone=TRX_CTCSSTones[scanIndex];
		trxSetTxCTCSS(currentChannelData->txTone);
		menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		menuVFOModeUpdateScreen(0);
		toneScanActive=false;
	}

	if(scanTimer>0)
	{
		scanTimer--;
	}
	else
	{
		scanIndex++;
		if(scanIndex > (TRX_NUM_CTCSS-1))
		{
			toneScanActive=false;
			trxSetRxCTCSS(currentChannelData->rxTone);
		}
		else
		{
			trxSetRxCTCSS(TRX_CTCSSTones[scanIndex]);
			scanTimer=TONESCANINTERVAL-(scanIndex*2);
		}
		menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		menuVFOModeUpdateScreen(0);
	}
}

void CCscan(void)
{
	if (GPIO_PinRead(GPIO_audio_amp_enable, Pin_audio_amp_enable)==1)
	{
		currentChannelData->rxColor=scanIndex;
		menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		menuVFOModeUpdateScreen(0);
		CCScanActive=false;
	}

	if(scanTimer>0)
	{
		scanTimer--;
	}
	else
	{
		scanIndex++;
		if(scanIndex>15) scanIndex=0;
		trxSetDMRColourCode(scanIndex);
		scanTimer=CCSCANINTERVAL;
		menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		menuVFOModeUpdateScreen(0);
	}
}
