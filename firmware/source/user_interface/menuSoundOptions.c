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
#include <hardware/UC1701.h>
#include <user_interface/menuSystem.h>
#include <user_interface/uiLocalisation.h>
#include <functions/fw_settings.h>
#include <user_interface/uiUtilities.h>

static void updateScreen(void);
static void handleEvent(uiEvent_t *ev);

enum SOUND_MENU_LIST { OPTIONS_MENU_TIMEOUT_BEEP = 0, OPTIONS_MENU_BEEP_VOLUME, OPTIONS_MENU_DMR_BEEP, OPTIONS_MIC_GAIN_DMR,
						NUM_SOUND_MENU_ITEMS};


int menuSoundOptions(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		// Store original settings, used on cancel event.
		memcpy(&originalNonVolatileSettings, &nonVolatileSettings, sizeof(settingsStruct_t));
		updateScreen();
	}
	else
	{
		if (ev->hasEvent)
			handleEvent(ev);
	}
	return 0;
}

static void updateScreen(void)
{
	int mNum = 0;
	static const int bufferLen = 17;
	char buf[bufferLen];

	ucClearBuf();
	menuDisplayTitle(currentLanguage->sound_options);

	// Can only display 3 of the options at a time menu at -1, 0 and +1
	for(int i = -1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_SOUND_MENU_ITEMS, i);
		buf[0] = 0;

		switch(mNum)
		{

			case OPTIONS_MENU_TIMEOUT_BEEP:
				if (nonVolatileSettings.txTimeoutBeepX5Secs != 0)
				{
					snprintf(buf, bufferLen, "%s:%d", currentLanguage->timeout_beep, nonVolatileSettings.txTimeoutBeepX5Secs * 5);
				}
				else
				{
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->timeout_beep, currentLanguage->off);
				}
				break;
			case OPTIONS_MENU_BEEP_VOLUME:// Beep volume reduction
				snprintf(buf, bufferLen, "%s:%ddB", currentLanguage->beep_volume, (2 - nonVolatileSettings.beepVolumeDivider) * 3);
				soundBeepVolumeDivider = nonVolatileSettings.beepVolumeDivider;
				break;
			case OPTIONS_MENU_DMR_BEEP:
				{
					const char *beepTX[] = {currentLanguage->none, currentLanguage->start, currentLanguage->stop, currentLanguage->both};
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->dmr_beep, beepTX[nonVolatileSettings.beepOptions]);
				}
				break;
			case OPTIONS_MIC_GAIN_DMR:// DMR Mic gain
				snprintf(buf, bufferLen, "%s:%ddB", currentLanguage->dmr_mic_gain, (nonVolatileSettings.micGainDMR - 11) * 3);
				break;
		}

		buf[bufferLen - 1] = 0;
		menuDisplayEntry(i, mNum, buf);
	}

	ucRender();
	displayLightTrigger();
}

static void handleEvent(uiEvent_t *ev)
{
	if (ev->events & KEY_EVENT)
	{
		if (KEYCHECK_PRESS(ev->keys,KEY_DOWN) && gMenusEndIndex!=0)
		{
			MENU_INC(gMenusCurrentItemIndex, NUM_SOUND_MENU_ITEMS);
		}
		else if (KEYCHECK_PRESS(ev->keys,KEY_UP))
		{
			MENU_DEC(gMenusCurrentItemIndex, NUM_SOUND_MENU_ITEMS);
		}
		else if (KEYCHECK_PRESS(ev->keys,KEY_RIGHT))
		{
			switch(gMenusCurrentItemIndex)
			{
				case OPTIONS_MENU_TIMEOUT_BEEP:
					if (nonVolatileSettings.txTimeoutBeepX5Secs < 4)
					{
						nonVolatileSettings.txTimeoutBeepX5Secs++;
					}
					break;
				case OPTIONS_MENU_BEEP_VOLUME:
					if (nonVolatileSettings.beepVolumeDivider > 0)
					{
						nonVolatileSettings.beepVolumeDivider--;
					}
					break;
				case OPTIONS_MENU_DMR_BEEP:
					if (nonVolatileSettings.beepOptions < (BEEP_TX_START | BEEP_TX_STOP))
					{
						nonVolatileSettings.beepOptions++;
					}
					break;
				case OPTIONS_MIC_GAIN_DMR:// DMR Mic gain
					if (nonVolatileSettings.micGainDMR < 15)
					{
						nonVolatileSettings.micGainDMR++;
						setMicGainDMR(nonVolatileSettings.micGainDMR);
					}
					break;
			}
		}
		else if (KEYCHECK_PRESS(ev->keys,KEY_LEFT))
		{
			switch(gMenusCurrentItemIndex)
			{
				case OPTIONS_MENU_TIMEOUT_BEEP:
					if (nonVolatileSettings.txTimeoutBeepX5Secs > 0)
					{
						nonVolatileSettings.txTimeoutBeepX5Secs--;
					}
					break;
				case OPTIONS_MENU_BEEP_VOLUME:
					if (nonVolatileSettings.beepVolumeDivider < 10)
					{
						nonVolatileSettings.beepVolumeDivider++;
					}
					break;
				case OPTIONS_MENU_DMR_BEEP:
					if (nonVolatileSettings.beepOptions > BEEP_TX_NONE)
					{
						nonVolatileSettings.beepOptions--;
					}
					break;
				case OPTIONS_MIC_GAIN_DMR:// DMR Mic gain
					if (nonVolatileSettings.micGainDMR > 0)
					{
						nonVolatileSettings.micGainDMR--;
						setMicGainDMR(nonVolatileSettings.micGainDMR);
					}
					break;
			}
		}
		else if (KEYCHECK_SHORTUP(ev->keys,KEY_GREEN))
		{
			// All parameters has already been applied
			menuSystemPopAllAndDisplayRootMenu();
			return;
		}
		else if (KEYCHECK_SHORTUP(ev->keys,KEY_RED))
		{
			// Restore original settings.
			memcpy(&nonVolatileSettings, &originalNonVolatileSettings, sizeof(settingsStruct_t));
			soundBeepVolumeDivider = nonVolatileSettings.beepVolumeDivider;
			setMicGainDMR(nonVolatileSettings.micGainDMR);
			menuSystemPopPreviousMenu();
			return;
		}
	}
	updateScreen();
}
