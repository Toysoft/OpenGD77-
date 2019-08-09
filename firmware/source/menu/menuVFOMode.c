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
#include "fw_settings.h"
#include "fw_codeplug.h"
#include "fw_HR-C6000.h"

enum VFO_SELECTED_FREQUENCY_INPUT  {VFO_SELECTED_FREQUENCY_INPUT_RX , VFO_SELECTED_FREQUENCY_INPUT_TX};

static char freq_enter_digits[7] = { '-', '-', '-', '-', '-', '-', '-' };
static int freq_enter_idx = 0;
static int selectedFreq = VFO_SELECTED_FREQUENCY_INPUT_RX;

static struct_codeplugRxGroup_t rxGroupData;
static struct_codeplugContact_t contactData;

static int currentIndexInTRxGroup=0;
static bool displaySquelch=false;

// internal prototypes
static void handleEvent(int buttons, int keys, int events);

static void reset_freq_enter_digits();
static int read_freq_enter_digits();
static void update_frequency(int tmp_frequency);
static void stepFrequency(int increment);

// public interface
int menuVFOMode(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		nonVolatileSettings.initialMenuNumber=MENU_VFO_MODE;
		currentChannelData = &nonVolatileSettings.vfoChannel;
		settingsCurrentChannelNumber = -1;// This is not a regular channel. Its the special VFO channel!

		trxSetFrequency(currentChannelData->rxFreq);
		if (currentChannelData->chMode == RADIO_MODE_ANALOG)
		{
			trxSetModeAndBandwidth(currentChannelData->chMode, ((currentChannelData->flag4 & 0x02) == 0x02));
		}
		else
		{
			trxSetModeAndBandwidth(currentChannelData->chMode, false);
		}
		trxSetDMRColourCode(currentChannelData->rxColor);
		trxSetPower(nonVolatileSettings.txPower);
		trxSetTxCTCSS(currentChannelData->txTone);
		trxSetRxCTCSS(currentChannelData->rxTone);

		//Need to load the Rx group if specificed even if TG is currently overridden as we may need it later when the left or right button is pressed
		if (currentChannelData->rxGroupList != 0)
		{
			codeplugRxGroupGetDataForIndex(currentChannelData->rxGroupList,&rxGroupData);
		}


		if (nonVolatileSettings.overrideTG == 0)
		{
			if (currentChannelData->rxGroupList != 0)
			{
				codeplugContactGetDataForIndex(rxGroupData.contacts[currentIndexInTRxGroup],&contactData);

				// Check whether the contact data seems valid
				if (contactData.name[0] == 0 || contactData.tgNumber ==0 || contactData.tgNumber > 9999999)
				{
					nonVolatileSettings.overrideTG = 9;// If the VFO does not have an Rx Group list assigned to it. We can't get a TG from the codeplug. So use TG 9.
					trxTalkGroupOrPcId = nonVolatileSettings.overrideTG;
				}
				else
				{
					trxTalkGroupOrPcId = contactData.tgNumber;
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
						sprintf(buffer,"PC %d",(trxTalkGroupOrPcId & 0x00FFFFFF));
					}
				}
				else
				{
					codeplugUtilConvertBufToString(contactData.name,buffer,16);
				}

				if (trxIsTransmitting)
				{
					UC1701_printAt(0,32,buffer,UC1701_FONT_GD77_8x16);
					sprintf(buffer,"%dmW",((nonVolatileSettings.txPower-790)*50)/23);// Approximate calculation.
					UC1701_printCore(0, 32, buffer, UC1701_FONT_GD77_8x16, 2, false);
				}
				else
				{
					UC1701_printCentered(16,buffer,UC1701_FONT_GD77_8x16);
				}
			}
			else if(displaySquelch)
			{
				sprintf(buffer,"Squelch");
				UC1701_printAt(0,16,buffer,UC1701_FONT_GD77_8x16);
				int bargraph= 1 + ((currentChannelData->sql-1)*5)/2 ;
				UC1701_fillRect(62,21,bargraph,8,false);
				displaySquelch=false;
			}

			if (freq_enter_idx==0)
			{
				if (!trxIsTransmitting)
				{
					val_before_dp = currentChannelData->rxFreq/10000;
					val_after_dp = currentChannelData->rxFreq - val_before_dp*10000;
					sprintf(buffer,"%cR %d.%04d MHz", (selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_RX)?'>':' ',val_before_dp, val_after_dp);
					UC1701_printCentered(32, buffer,UC1701_FONT_GD77_8x16);
				}
				else
				{
					sprintf(buffer," %d ",txTimeSecs);
					UC1701_printCentered(0, buffer,UC1701_FONT_16x32);
				}

				val_before_dp = currentChannelData->txFreq/10000;
				val_after_dp = currentChannelData->txFreq - val_before_dp*10000;
				sprintf(buffer,"%cT %d.%04d MHz", (selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_TX || trxIsTransmitting)?'>':' ',val_before_dp, val_after_dp);
				UC1701_printCentered(48, buffer,UC1701_FONT_GD77_8x16);
			}
			else
			{
				sprintf(buffer,"%c%c%c.%c%c%c%c MHz", freq_enter_digits[0], freq_enter_digits[1], freq_enter_digits[2], freq_enter_digits[3], freq_enter_digits[4], freq_enter_digits[5], freq_enter_digits[6] );
				if (selectedFreq == VFO_SELECTED_FREQUENCY_INPUT_TX)
				{
					UC1701_printCentered(48, buffer,UC1701_FONT_GD77_8x16);
				}
				else
				{
					UC1701_printCentered(32, buffer,UC1701_FONT_GD77_8x16);
				}
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

static void reset_freq_enter_digits()
{
	for (int i=0;i<7;i++)
	{
		freq_enter_digits[i]='-';
	}
	freq_enter_idx = 0;
}

static int read_freq_enter_digits()
{
	int result=0;
	for (int i=0;i<7;i++)
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
			trxSetFrequency(frequency);
			set_melody(melody_ACK_beep);
		}
		else
		{
			set_melody(melody_ERROR_beep);
		}
	}

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
		if (buttons & BUTTON_SK2 )
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

	if (freq_enter_idx==0)
	{
		if ((keys & KEY_STAR)!=0)
		{
			if (trxGetMode() == RADIO_MODE_ANALOG)
			{
				channelScreenChannelData.chMode = RADIO_MODE_DIGITAL;
				trxSetModeAndBandwidth(RADIO_MODE_DIGITAL, false);
			}
			else
			{
				channelScreenChannelData.chMode = RADIO_MODE_ANALOG;
				trxSetModeAndBandwidth(RADIO_MODE_ANALOG, ((currentChannelData->flag4 & 0x02) == 0x02));
				trxSetTxCTCSS(currentChannelData->rxTone);
			}
			currentChannelData->chMode = trxGetMode();
			menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
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
			menuSystemSetCurrentMenu(MENU_CHANNEL_MODE);
			return;
		}
		else if ((keys & KEY_RIGHT)!=0)
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
				trxTalkGroupOrPcId = contactData.tgNumber;

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
				trxTalkGroupOrPcId = contactData.tgNumber;

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
	}
	if (freq_enter_idx<7)
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
			if (freq_enter_idx==7)
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

	menuVFOModeUpdateScreen(0);
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
			trxSetFrequency(currentChannelData->rxFreq);
		}
	}
	else
	{
		set_melody(melody_ERROR_beep);
	}
}

