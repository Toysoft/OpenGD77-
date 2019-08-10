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
#include "fw_settings.h"


static void updateScreen();
static void handleEvent(int buttons, int keys, int events);

static const int PIT_COUNTS_PER_SECOND = 10000;
static int timeInSeconds;
static uint32_t nextSecondPIT;

int menuTxScreen(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		if (trxCheckFrequencyInAmateurBand(currentChannelData->txFreq) || nonVolatileSettings.txFreqLimited == 0x00)
		{
			nextSecondPIT = PITCounter + PIT_COUNTS_PER_SECOND;
			timeInSeconds = currentChannelData->tot*15;

			GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
			GPIO_PinWrite(GPIO_LEDred, Pin_LEDred, 1);
			trxSetFrequency(currentChannelData->txFreq);
			txstopdelay=0;
			trxIsTransmitting=true;

			trx_setTX();
			if (trxGetMode() == RADIO_MODE_ANALOG)
			{
				trx_activateTX();
			}
			updateScreen();
		}
		else
		{
			UC1701_clearBuf();
			UC1701_printCentered(4, "ERROR",UC1701_FONT_16x32);
			UC1701_printCentered(40, "OUT OF BAND",UC1701_FONT_GD77_8x16);
			UC1701_render();
			displayLightOverrideTimeout(-1);
			set_melody(melody_ERROR_beep);
		}
	}
	else
	{
		if (trxIsTransmitting)
		{
			if (PITCounter >= nextSecondPIT )
			{
				if (currentChannelData->tot==0)
				{
					timeInSeconds++;
				}
				else
				{
					timeInSeconds--;
				}
				if (currentChannelData->tot!=0 && timeInSeconds == 0)
				{
					set_melody(melody_tx_timeout_beep);
					UC1701_clearBuf();
					UC1701_printCentered(20, "TIMEOUT",UC1701_FONT_16x32);
					UC1701_render();
				}
				else
				{
					updateScreen();
				}
				nextSecondPIT = PITCounter + PIT_COUNTS_PER_SECOND;
			}
		}
		handleEvent(buttons, keys, events);
	}
	return 0;
}

static void updateScreen()
{

	menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
	if (menuControlData.stack[0]==MENU_VFO_MODE)
	{
		menuVFOModeUpdateScreen(timeInSeconds);
		displayLightOverrideTimeout(-1);
	}
	else
	{
		/*
		char buf[8];
		UC1701_clearBuf();
		sprintf(buf,"%d",timeInSeconds);
		UC1701_printCentered(20, buf,UC1701_FONT_16x32);
		UC1701_render();
		*/

		menuChannelModeUpdateScreen(timeInSeconds);
		displayLightOverrideTimeout(-1);
	}
}


static void handleEvent(int buttons, int keys, int events)
{
	if ((buttons & BUTTON_PTT)==0 || (currentChannelData->tot!=0 && timeInSeconds == 0))
	{
		trxIsTransmitting=false;
		if (txstopdelay>0)
		{
			txstopdelay--;
		}
		if ((slot_state < DMR_STATE_TX_START_1) && (txstopdelay==0))
		{
			GPIO_PinWrite(GPIO_LEDred, Pin_LEDred, 0);
			trx_deactivateTX();
			trx_setRX();
			menuSystemPopPreviousMenu();
		}
	}
}

