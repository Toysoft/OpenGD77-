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
static void loadContact();
static bool isDisplayingQSOData=false;

// public interface
int menuVFOMode(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		RssiUpdateCounter = RSSI_UPDATE_COUNTER_RELOAD;
		isDisplayingQSOData=false;
		gMenusCurrentItemIndex=0;
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
			trxSetRxCTCSS(currentChannelData->rxTone);
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
		}
		else
		{
			handleEvent(buttons, keys, events);
		}
	}
	return 0;
}

void menuVFOModeUpdateScreen(int txTimeSecs)
{
	int val_before_dp;
	int val_after_dp;

	char buffer[32];
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
					if((trxTalkGroupOrPcId>>24) == TG_CALL_FLAG)
					{
						sprintf(buffer,"TG %d",(trxTalkGroupOrPcId & 0x00FFFFFF));
					}
					else
					{
						dmrIdDataStruct_t currentRec;
						dmrIDLookup((trxTalkGroupOrPcId & 0x00FFFFFF),&currentRec);
						snprintf(buffer, 20, "%s",currentRec.text);
						buffer[20] = 0;
					}
				}
				else
				{
					codeplugUtilConvertBufToString(contactData.name,buffer,16);
				}

				if (trxIsTransmitting)
				{
					UC1701_printCentered(34,buffer,UC1701_FONT_8x16);
				}
				else
				{
					UC1701_printCentered(CONTACT_Y_POS,buffer,UC1701_FONT_8x16);
				}
			}
			else if(displaySquelch)
			{
				sprintf(buffer,"Squelch");
				UC1701_printAt(0,16,buffer,UC1701_FONT_8x16);
				int bargraph= 1 + ((currentChannelData->sql-1)*5)/2 ;
				UC1701_fillRect(62,21,bargraph,8,false);
				displaySquelch=false;
			}

			if (freq_enter_idx==0)
			{
				if (!trxIsTransmitting)
				{
					val_before_dp = currentChannelData->rxFreq/100000;
					val_after_dp = currentChannelData->rxFreq - val_before_dp*100000;
					sprintf(buffer,"%cR %d.%05d MHz", (selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_RX)?'>':' ',val_before_dp, val_after_dp);
					UC1701_printCentered(32, buffer,UC1701_FONT_8x16);
				}
				else
				{
					sprintf(buffer," %d ",txTimeSecs);
					UC1701_printCentered(TX_TIMER_Y_OFFSET, buffer,UC1701_FONT_16x32);
				}

				val_before_dp = currentChannelData->txFreq/100000;
				val_after_dp = currentChannelData->txFreq - val_before_dp*100000;
				sprintf(buffer,"%cT %d.%05d MHz", (selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_TX || trxIsTransmitting)?'>':' ',val_before_dp, val_after_dp);
				UC1701_printCentered(48, buffer,UC1701_FONT_8x16);
			}
			else
			{
				sprintf(buffer,"%c%c%c.%c%c%c%c%c MHz", freq_enter_digits[0], freq_enter_digits[1], freq_enter_digits[2], freq_enter_digits[3], freq_enter_digits[4], freq_enter_digits[5], freq_enter_digits[6], freq_enter_digits[7] );
				if (selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_TX)
				{
					UC1701_printCentered(48, buffer,UC1701_FONT_8x16);
				}
				else
				{
					UC1701_printCentered(32, buffer,UC1701_FONT_8x16);
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
		if (trxCheckFrequencyIsSupportedByTheRadioHardware(frequency))
		{
			currentChannelData->txFreq = frequency;
			trxSetFrequency(currentChannelData->rxFreq,currentChannelData->txFreq,DMR_MODE_AUTO);
			set_melody(melody_ACK_beep);
		}
	}
	else
	{
		int deltaFrequency = frequency - currentChannelData->rxFreq;
		if (trxCheckFrequencyIsSupportedByTheRadioHardware(frequency))
		{
			currentChannelData->rxFreq = frequency;
			currentChannelData->txFreq = currentChannelData->txFreq + deltaFrequency;
			trxSetFrequency(currentChannelData->rxFreq,currentChannelData->txFreq,DMR_MODE_AUTO);

			if (!trxCheckFrequencyIsSupportedByTheRadioHardware(currentChannelData->txFreq))
			{
				currentChannelData->txFreq = frequency;
				set_melody(melody_ERROR_beep);
			}
			else
			{
				set_melody(melody_ACK_beep);
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

static void loadContact()
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
	if (isDisplayingQSOData && (buttons & BUTTON_SK2)!=0 && trxGetMode() == RADIO_MODE_DIGITAL && trxTalkGroupOrPcId != tg)
	{
		lastHeardClearLastID();
		trxTalkGroupOrPcId = tg;
		nonVolatileSettings.overrideTG = trxTalkGroupOrPcId;
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

	if ((keys & KEY_GREEN)!=0)
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
	else if ((keys & KEY_HASH)!=0)
	{
		if (trxGetMode() == RADIO_MODE_DIGITAL)
		{
			if ((buttons & BUTTON_SK2) != 0)
			{
				menuSystemPushNewMenu(MENU_CONTACT_LIST);
			} else {
				menuSystemPushNewMenu(MENU_NUMERICAL_ENTRY);
			}
		}
		return;
	}

	if (freq_enter_idx==0)
	{
		if ((keys & KEY_STAR)!=0)
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
		else if ((keys & KEY_DOWN)!=0)
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
		else if ((keys & KEY_UP)!=0)
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
		else if ((keys & KEY_RED)!=0)
		{
			if (menuUtilityHandlePrivateCallActions(buttons,keys,events))
			{
				return;
			}
			menuSystemSetCurrentMenu(MENU_CHANNEL_MODE);
			return;
		}
		else
		if ((keys & KEY_RIGHT)!=0)
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
					menuVFOModeUpdateScreen(0);
				}
			}
		}
		else if ((keys & KEY_LEFT)!=0)
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
					menuVFOModeUpdateScreen(0);
				}
			}
		}
	}
	else
	{
		if ((keys & KEY_LEFT)!=0)
		{
			freq_enter_idx--;
			freq_enter_digits[freq_enter_idx]='-';
			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		}
		else if ((keys & KEY_RED)!=0)
		{
			reset_freq_enter_digits();
    	    set_melody(melody_NACK_beep);
    		menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
		}
		else if ((keys & KEY_GREEN) != 0)
		{
			int tmp_frequency=read_freq_enter_digits();
			if (trxCheckFrequencyIsSupportedByTheRadioHardware(tmp_frequency))
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
		char c='\0';
		if ((keys & KEY_0)!=0)
		{
			c='0';
		}
		else if ((keys & KEY_1)!=0)
		{
			c='1';
		}
		else if ((keys & KEY_2)!=0)
		{
			c='2';
		}
		else if ((keys & KEY_3)!=0)
		{
			c='3';
		}
		else if ((keys & KEY_4)!=0)
		{
			c='4';
		}
		else if ((keys & KEY_5)!=0)
		{
			c='5';
		}
		else if ((keys & KEY_6)!=0)
		{
			c='6';
		}
		else if ((keys & KEY_7)!=0)
		{
			c='7';
		}
		else if ((keys & KEY_8)!=0)
		{
			c='8';
		}
		else if ((keys & KEY_9)!=0)
		{
			c='9';
		}
		if (c!='\0')
		{
			freq_enter_digits[freq_enter_idx]=c;
			freq_enter_idx++;
			if (freq_enter_idx==8)
			{
				int tmp_frequency=read_freq_enter_digits();
				if (trxCheckFrequencyIsSupportedByTheRadioHardware(tmp_frequency))
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
	if (trxCheckFrequencyIsSupportedByTheRadioHardware(tmp_frequencyRx))
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
enum VFO_SCREEN_QUICK_MENU_ITEMS { VFO_SCREEN_QUICK_MENU_TX_SWAP_RX = 0, VFO_SCREEN_QUICK_MENU_BOTH_TO_RX, VFO_SCREEN_QUICK_MENU_BOTH_TO_TX,
	NUM_VFO_SCREEN_QUICK_MENU_ITEMS };// The last item in the list is used so that we automatically get a total number of items in the list

static void updateQuickMenuScreen(void)
{
	int mNum = 0;
	char buf[17];

	UC1701_clearBuf();
	menuDisplayTitle("Quick Menu");

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
			default:
				strcpy(buf, "");
		}

		menuDisplayEntry(i, mNum, buf);
	}

	UC1701_render();
	displayLightTrigger();
}

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
			case VFO_SCREEN_QUICK_MENU_BOTH_TO_RX:
				currentChannelData->txFreq = currentChannelData->rxFreq;
				trxSetFrequency(currentChannelData->rxFreq,currentChannelData->txFreq,DMR_MODE_AUTO);
				menuSystemPopPreviousMenu();
				break;
			case VFO_SCREEN_QUICK_MENU_TX_SWAP_RX:
				{
					int tmpFreq = currentChannelData->txFreq;
					currentChannelData->txFreq = currentChannelData->rxFreq;
					currentChannelData->rxFreq =  tmpFreq;
					trxSetFrequency(currentChannelData->rxFreq,currentChannelData->txFreq,DMR_MODE_AUTO);
					menuSystemPopPreviousMenu();
				}
				break;
			case VFO_SCREEN_QUICK_MENU_BOTH_TO_TX:
				currentChannelData->rxFreq = currentChannelData->txFreq;
				trxSetFrequency(currentChannelData->rxFreq,currentChannelData->txFreq,DMR_MODE_AUTO);
				menuSystemPopPreviousMenu();
				break;
		}
		return;
	}
	else if ((keys & KEY_DOWN)!=0)
	{
		MENU_INC(gMenusCurrentItemIndex, NUM_VFO_SCREEN_QUICK_MENU_ITEMS);
		updateQuickMenuScreen();
	}
	else if ((keys & KEY_UP)!=0)
	{
		MENU_DEC(gMenusCurrentItemIndex, NUM_VFO_SCREEN_QUICK_MENU_ITEMS);
		updateQuickMenuScreen();
	}
}

int menuVFOModeQuickMenu(int buttons, int keys, int events, bool isFirstRun)
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
