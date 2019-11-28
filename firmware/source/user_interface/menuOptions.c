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
#include <user_interface/uiLocalisation.h>
#include "fw_settings.h"
#include "fw_wdog.h"

static void updateScreen(void);
static void handleEvent(int buttons, int keys, int events);
static bool	doFactoryReset;
enum OPTIONS_MENU_LIST { OPTIONS_MENU_TIMEOUT_BEEP=0,OPTIONS_MENU_FACTORY_RESET,OPTIONS_MENU_USE_CALIBRATION,
							OPTIONS_MENU_TX_FREQ_LIMITS,OPTIONS_MENU_BEEP_VOLUME,OPTIONS_MIC_GAIN_DMR,
							OPTIONS_MENU_KEYPAD_TIMER_LONG, OPTIONS_MENU_KEYPAD_TIMER_REPEAT, OPTIONS_MENU_DMR_MONITOR_CAPTURE_TIMEOUT,
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

static void updateScreen(void)
{
	int mNum = 0;
	char buf[17];

	UC1701_clearBuf();
	menuDisplayTitle(currentLanguage->options);

	// Can only display 3 of the options at a time menu at -1, 0 and +1
	for(int i = -1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_OPTIONS_MENU_ITEMS, i);

		switch(mNum)
		{
			case OPTIONS_MENU_TIMEOUT_BEEP:
				if (nonVolatileSettings.txTimeoutBeepX5Secs != 0)
				{
					sprintf(buf, "%s:%d",currentLanguage->timeout_beep, nonVolatileSettings.txTimeoutBeepX5Secs * 5);
				}
				else
				{
					sprintf(buf, "%s:%s",currentLanguage->timeout_beep,currentLanguage->off);
				}
				break;
			case OPTIONS_MENU_FACTORY_RESET:
				if (doFactoryReset == true)
				{
					sprintf(buf, "%s:%s",currentLanguage->factory_reset,currentLanguage->yes);
				}
				else
				{
					sprintf(buf, "%s:%s",currentLanguage->factory_reset,currentLanguage->no);
				}
				break;
			case OPTIONS_MENU_USE_CALIBRATION:
				if (nonVolatileSettings.useCalibration)
				{
					sprintf(buf, "%s:%s",currentLanguage->calibration,currentLanguage->on);
				}
				else
				{
					sprintf(buf, "%s:%",currentLanguage->calibration,currentLanguage->off);
				}
				break;
			case OPTIONS_MENU_TX_FREQ_LIMITS:// Tx Freq limits
				if (nonVolatileSettings.txFreqLimited)
				{
					sprintf(buf, "%s:%",currentLanguage->band_limits,currentLanguage->on);
				}
				else
				{
					sprintf(buf, "%s:%",currentLanguage->band_limits,currentLanguage->off);
				}
				break;
			case OPTIONS_MENU_BEEP_VOLUME:// Beep volume reduction
				sprintf(buf, "%s:%ddB",currentLanguage->beep_volume, (2 - nonVolatileSettings.beepVolumeDivider) * 3);
				soundBeepVolumeDivider = nonVolatileSettings.beepVolumeDivider;
				break;
			case OPTIONS_MIC_GAIN_DMR:// DMR Mic gain
				sprintf(buf, "%s:%ddB", currentLanguage->dmr_mic_gain,(nonVolatileSettings.micGainDMR - 11) * 3);
				break;
			case OPTIONS_MENU_KEYPAD_TIMER_LONG:// Timer longpress
				sprintf(buf, "%:%1d.%1ds",currentLanguage->key_long, nonVolatileSettings.keypadTimerLong / 10, nonVolatileSettings.keypadTimerLong % 10);
				break;
			case OPTIONS_MENU_KEYPAD_TIMER_REPEAT:// Timer repeat
				sprintf(buf, "%s:%1d.%1ds",currentLanguage->key_repeat, nonVolatileSettings.keypadTimerRepeat/10, nonVolatileSettings.keypadTimerRepeat % 10);
				break;
			case OPTIONS_MENU_DMR_MONITOR_CAPTURE_TIMEOUT:// DMR filtr timeout repeat
				sprintf(buf, "%s:%ds", currentLanguage->dmr_filter_timeout,nonVolatileSettings.dmrCaptureTimeout);
				break;

		}

		menuDisplayEntry(i, mNum, buf);
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
			case OPTIONS_MENU_KEYPAD_TIMER_LONG:
				if (nonVolatileSettings.keypadTimerLong<90)
				{
					nonVolatileSettings.keypadTimerLong++;
				}
				break;
			case OPTIONS_MENU_KEYPAD_TIMER_REPEAT:
				if (nonVolatileSettings.keypadTimerRepeat<90)
				{
					nonVolatileSettings.keypadTimerRepeat++;
				}
				break;
			case OPTIONS_MENU_DMR_MONITOR_CAPTURE_TIMEOUT:
				if (nonVolatileSettings.dmrCaptureTimeout<90)
				{
					nonVolatileSettings.dmrCaptureTimeout++;
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
			case OPTIONS_MENU_KEYPAD_TIMER_LONG:
				if (nonVolatileSettings.keypadTimerLong>1)
				{
					nonVolatileSettings.keypadTimerLong--;
				}
				break;
			case OPTIONS_MENU_KEYPAD_TIMER_REPEAT:
				if (nonVolatileSettings.keypadTimerRepeat>0)
				{
					nonVolatileSettings.keypadTimerRepeat--;
				}
				break;
			case OPTIONS_MENU_DMR_MONITOR_CAPTURE_TIMEOUT:
				if (nonVolatileSettings.dmrCaptureTimeout>1)
				{
					nonVolatileSettings.dmrCaptureTimeout--;
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
