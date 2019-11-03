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
static const int MAX_POWER = 4100;// Max DAC value is actually 4096 but no one should need to drive the PA that hard, so I rounded this down to 4000
enum UTILITIES_MENU_LIST { UTILITIES_MENU_POWER = 0, UTILITIES_MENU_TIMEOUT_BEEP,UTILITIES_MENU_FACTORY_RESET,UTILITIES_MENU_USE_CALIBRATION,
							UTILITIES_MENU_TX_FREQ_LIMITS,UTILITIES_MENU_BEEP_VOLUME,
							NUM_UTILITIES_MENU_ITEMS};


int menuUtilities(int buttons, int keys, int events, bool isFirstRun)
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
	int mNum=0;
	char buf[17];
	UC1701_clearBuf();
	UC1701_printCentered(0, "Utilities",UC1701_FONT_GD77_8x16);

	// Can only display 3 of the options at a time menu at -1, 0 and +1
	for(int i=-1;i<=1;i++)
	{
		mNum = gMenusCurrentItemIndex+i;
		if (mNum<0)
		{
			mNum = NUM_UTILITIES_MENU_ITEMS + mNum;
		}
		if (mNum >= NUM_UTILITIES_MENU_ITEMS)
		{
			mNum = mNum - NUM_UTILITIES_MENU_ITEMS;
		}

		switch(mNum)
		{
			case UTILITIES_MENU_POWER:
				sprintf(buf,"Power %d",trxGetPower());
				break;
			case UTILITIES_MENU_TIMEOUT_BEEP:
				if (nonVolatileSettings.txTimeoutBeepX5Secs!=0)
				{
					sprintf(buf,"Timeout beep:%d",nonVolatileSettings.txTimeoutBeepX5Secs * 5);
				}
				else
				{
					strcpy(buf,"Timeout beep:OFF");
				}
				break;
			case UTILITIES_MENU_FACTORY_RESET:
				if (doFactoryReset==true)
				{
					strcpy(buf,"Fact Reset:YES");
				}
				else
				{
					strcpy(buf,"Fact Reset:NO");
				}
				break;
			case UTILITIES_MENU_USE_CALIBRATION:
				if (nonVolatileSettings.useCalibration!=0)
				{
					strcpy(buf,"Calibration:ON");
				}
				else
				{
					strcpy(buf,"Calibration:OFF");
				}
				break;
			case UTILITIES_MENU_TX_FREQ_LIMITS:// Tx Freq limits
				if (nonVolatileSettings.txFreqLimited!=0)
				{
					strcpy(buf,"Band Limits:ON");
				}
				else
				{
					strcpy(buf,"Band Limits:OFF");
				}
				break;
			case UTILITIES_MENU_BEEP_VOLUME:// Beep volume reduction
				sprintf(buf,"Beep vol:%ddB", (2 - nonVolatileSettings.beepVolumeDivider)*3);
				soundBeepVolumeDivider = nonVolatileSettings.beepVolumeDivider;
				break;
		}
		if (gMenusCurrentItemIndex==mNum)
		{
			UC1701_fillRect(0,(i+2)*16,128,16,false);
		}

		UC1701_printCore(0,(i+2)*16,buf,UC1701_FONT_GD77_8x16,0,(gMenusCurrentItemIndex==mNum));
	}

	UC1701_render();
	displayLightTrigger();
}

static void handleEvent(int buttons, int keys, int events)
{
	int tmpPower;
	int powerChangeValue=100;
	if ((keys & KEY_DOWN)!=0 && gMenusEndIndex!=0)
	{
		gMenusCurrentItemIndex++;
		if (gMenusCurrentItemIndex>=NUM_UTILITIES_MENU_ITEMS)
		{
			gMenusCurrentItemIndex=0;
		}
	}
	else if ((keys & KEY_UP)!=0)
	{
		gMenusCurrentItemIndex--;
		if (gMenusCurrentItemIndex<0)
		{
			gMenusCurrentItemIndex=NUM_UTILITIES_MENU_ITEMS-1;
		}
	}
	else if ((keys & KEY_RIGHT)!=0)
	{
		switch(gMenusCurrentItemIndex)
		{
			case UTILITIES_MENU_POWER:
				if (buttons && BUTTON_SK2)
				{
					powerChangeValue=10;
				}
				tmpPower = trxGetPower() + powerChangeValue;

				if (tmpPower<=MAX_POWER)
				{
					trxSetPower(tmpPower);
				}
				break;
			case UTILITIES_MENU_TIMEOUT_BEEP:
				if (nonVolatileSettings.txTimeoutBeepX5Secs < 4)
				{
					nonVolatileSettings.txTimeoutBeepX5Secs++;
				}
				break;
			case UTILITIES_MENU_FACTORY_RESET:
				doFactoryReset = true;
				break;
			case UTILITIES_MENU_USE_CALIBRATION:
				nonVolatileSettings.useCalibration=0x01;
				break;
			case UTILITIES_MENU_TX_FREQ_LIMITS:
				nonVolatileSettings.txFreqLimited=0x01;
				break;
			case UTILITIES_MENU_BEEP_VOLUME:
				if (nonVolatileSettings.beepVolumeDivider>0)
				{
					nonVolatileSettings.beepVolumeDivider--;
				}
				break;
		}
	}
	else if ((keys & KEY_LEFT)!=0)
	{

		switch(gMenusCurrentItemIndex)
		{
			case UTILITIES_MENU_POWER:
				{
					uint32_t pwr = trxGetPower();
					if (pwr==4095)
					{
						pwr = 4100;// round up for display purposes
					}
					if (buttons && BUTTON_SK2)
					{
						powerChangeValue=10;
					}
					if (pwr >= powerChangeValue)
					{
						trxSetPower(pwr- powerChangeValue);
					}
				}
				break;
			case UTILITIES_MENU_TIMEOUT_BEEP:
				if (nonVolatileSettings.txTimeoutBeepX5Secs>0)
				{
					nonVolatileSettings.txTimeoutBeepX5Secs--;
				}
				break;
			case UTILITIES_MENU_FACTORY_RESET:
				doFactoryReset = false;
				break;
			case UTILITIES_MENU_USE_CALIBRATION:
				nonVolatileSettings.useCalibration=0x00;
				break;
			case UTILITIES_MENU_TX_FREQ_LIMITS:
				nonVolatileSettings.txFreqLimited=0x00;
				break;
			case UTILITIES_MENU_BEEP_VOLUME:
				if (nonVolatileSettings.beepVolumeDivider<10)
				{
					nonVolatileSettings.beepVolumeDivider++;
				}
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
