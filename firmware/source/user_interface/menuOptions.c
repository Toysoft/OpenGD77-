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
#include "fw_settings.h"
#include "fw_wdog.h"

static void updateScreen();
static void handleEvent(int buttons, int keys, int events);
static bool	doFactoryReset;
enum OPTIONS_MENU_LIST { OPTIONS_MENU_TIMEOUT_BEEP=0,OPTIONS_MENU_FACTORY_RESET,OPTIONS_MENU_USE_CALIBRATION,
							OPTIONS_MENU_TX_FREQ_LIMITS,OPTIONS_MENU_BEEP_VOLUME,OPTIONS_MIC_GAIN_DMR,
							NUM_OPTIONS_MENU_ITEMS};


int menuOptions(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		gMenusCurrentItemIndex=0;
		doFactoryReset=false;
		updateScreen();
	}
	else
	{
		if (events!=0 && keys!=0)
		{
			handleEvent(buttons, keys, events);
		}
	}
	return 0;
}

static void updateScreen()
{
	int mNum = 0;
	char buf[17];
	UC1701_clearBuf();
	UC1701_printCentered(0, "Options", UC1701_FONT_GD77_8x16);

	// Can only display 3 of the options at a time menu at -1, 0 and +1
	for(int i = -1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_OPTIONS_MENU_ITEMS, i);

		switch(mNum)
		{
			case OPTIONS_MENU_TIMEOUT_BEEP:
				if (nonVolatileSettings.txTimeoutBeepX5Secs != 0)
				{
					sprintf(buf, "Timeout beep:%d", nonVolatileSettings.txTimeoutBeepX5Secs * 5);
				}
				else
				{
					strcpy(buf, "Timeout beep:OFF");
				}
				break;
			case OPTIONS_MENU_FACTORY_RESET:
				if (doFactoryReset == true)
				{
					strcpy(buf, "Fact Reset:YES");
				}
				else
				{
					strcpy(buf, "Fact Reset:NO");
				}
				break;
			case OPTIONS_MENU_USE_CALIBRATION:
				if (nonVolatileSettings.useCalibration)
				{
					strcpy(buf, "Calibration:ON");
				}
				else
				{
					strcpy(buf, "Calibration:OFF");
				}
				break;
			case OPTIONS_MENU_TX_FREQ_LIMITS:// Tx Freq limits
				if (nonVolatileSettings.txFreqLimited)
				{
					strcpy(buf, "Band Limits:ON");
				}
				else
				{
					strcpy(buf, "Band Limits:OFF");
				}
				break;
			case OPTIONS_MENU_BEEP_VOLUME:// Beep volume reduction
				sprintf(buf, "Beep vol:%ddB", (2 - nonVolatileSettings.beepVolumeDivider) * 3);
				soundBeepVolumeDivider = nonVolatileSettings.beepVolumeDivider;
				break;
			case OPTIONS_MIC_GAIN_DMR:// DMR Mic gain
				sprintf(buf, "DMR mic:%ddB", (nonVolatileSettings.micGainDMR - 11) * 3);
				break;

		}
		if (gMenusCurrentItemIndex == mNum)
		{
			UC1701_fillRect(0, (i + 2) * 16, 128, 16, false);
		}

		UC1701_printCore(0, (i + 2) * 16, buf, UC1701_FONT_GD77_8x16, 0, (gMenusCurrentItemIndex == mNum));
	}

	UC1701_render();
	displayLightTrigger();
}

static void handleEvent(int buttons, int keys, int events)
{
	if ((keys & KEY_DOWN)!=0 && gMenusEndIndex!=0)
	{
		MENU_INC(gMenusCurrentItemIndex, NUM_OPTIONS_MENU_ITEMS);
	}
	else if ((keys & KEY_UP)!=0)
	{
		MENU_DEC(gMenusCurrentItemIndex, NUM_OPTIONS_MENU_ITEMS);
	}
	else if ((keys & KEY_RIGHT)!=0)
	{
		switch(gMenusCurrentItemIndex)
		{
			case OPTIONS_MENU_TIMEOUT_BEEP:
				if (nonVolatileSettings.txTimeoutBeepX5Secs < 4)
				{
					nonVolatileSettings.txTimeoutBeepX5Secs++;
				}
				break;
			case OPTIONS_MENU_FACTORY_RESET:
				doFactoryReset = true;
				break;
			case OPTIONS_MENU_USE_CALIBRATION:
				nonVolatileSettings.useCalibration=true;
				break;
			case OPTIONS_MENU_TX_FREQ_LIMITS:
				nonVolatileSettings.txFreqLimited=true;
				break;
			case OPTIONS_MENU_BEEP_VOLUME:
				if (nonVolatileSettings.beepVolumeDivider>0)
				{
					nonVolatileSettings.beepVolumeDivider--;
				}
				break;
			case OPTIONS_MIC_GAIN_DMR:// DMR Mic gain
				if (nonVolatileSettings.micGainDMR<15 )
				{
					nonVolatileSettings.micGainDMR++;
					setMicGainDMR(nonVolatileSettings.micGainDMR);
				}
				break;
		}
	}
	else if ((keys & KEY_LEFT)!=0)
	{

		switch(gMenusCurrentItemIndex)
		{
			case OPTIONS_MENU_TIMEOUT_BEEP:
				if (nonVolatileSettings.txTimeoutBeepX5Secs>0)
				{
					nonVolatileSettings.txTimeoutBeepX5Secs--;
				}
				break;
			case OPTIONS_MENU_FACTORY_RESET:
				doFactoryReset = false;
				break;
			case OPTIONS_MENU_USE_CALIBRATION:
				nonVolatileSettings.useCalibration=false;
				break;
			case OPTIONS_MENU_TX_FREQ_LIMITS:
				nonVolatileSettings.txFreqLimited=false;
				break;
			case OPTIONS_MENU_BEEP_VOLUME:
				if (nonVolatileSettings.beepVolumeDivider<10)
				{
					nonVolatileSettings.beepVolumeDivider++;
				}
				break;
			case OPTIONS_MIC_GAIN_DMR:// DMR Mic gain
				if (nonVolatileSettings.micGainDMR>0)
				{
					nonVolatileSettings.micGainDMR--;
					setMicGainDMR(nonVolatileSettings.micGainDMR);
				}
				break;
		}
	}
	else if ((keys & KEY_GREEN)!=0)
	{
		if (doFactoryReset==true)
		{
			settingsRestoreDefaultSettings();
			watchdogReboot();
		}
		menuSystemPopAllAndDisplayRootMenu();
		return;
	}
	else if ((keys & KEY_RED)!=0)
	{
		menuSystemPopPreviousMenu();
		return;
	}
	updateScreen();
}
