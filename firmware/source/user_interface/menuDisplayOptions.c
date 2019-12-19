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
#include "fw_settings.h"

static void updateScreen(void);
static void handleEvent(uiEvent_t *ev);

static uint8_t originalBrightness;
static uint8_t contrast;
static uint8_t inverseVideo;
static int8_t 	backLightTimeout;// used int_8 to save space

const int BACKLIGHT_MAX_TIMEOUT = 30;
const int CONTRAST_MAX_VALUE = 30;// Maximum value which still seems to be readable
const int CONTRAST_MIN_VALUE = 12;// Minimum value which still seems to be readable
enum DISPLAY_MENU_LIST { DISPLAY_MENU_BRIGHTNESS = 0, DISPLAY_MENU_CONTRAST, DISPLAY_MENU_TIMEOUT, DISPLAY_MENU_COLOUR_INVERT, NUM_DISPLAY_MENU_ITEMS};

int menuDisplayOptions(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		originalBrightness = nonVolatileSettings.displayBacklightPercentage;
		contrast = nonVolatileSettings.displayContrast;
		inverseVideo = nonVolatileSettings.displayInverseVideo;
		backLightTimeout = 	nonVolatileSettings.backLightTimeout;
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
	menuDisplayTitle(currentLanguage->display_options);

	// Can only display 3 of the options at a time menu at -1, 0 and +1
	for(int i = -1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_DISPLAY_MENU_ITEMS, i);
		buf[0] = 0;

		switch(mNum)
		{
			case DISPLAY_MENU_BRIGHTNESS:
				snprintf(buf, bufferLen, "%s:%d%%", currentLanguage->brightness, nonVolatileSettings.displayBacklightPercentage);
				break;
			case DISPLAY_MENU_CONTRAST:
				snprintf(buf, bufferLen, "%s:%d", currentLanguage->contrast, contrast);
				break;
			case DISPLAY_MENU_TIMEOUT:
				if (backLightTimeout == 0)
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->backlight_timeout, currentLanguage->no);
				else
					snprintf(buf, bufferLen, "%s:%d", currentLanguage->backlight_timeout, backLightTimeout);
				break;
			case DISPLAY_MENU_COLOUR_INVERT:
				if (inverseVideo)
				{
					strncpy(buf, currentLanguage->colour_invert, bufferLen);
				}
				else
				{
					strncpy(buf, currentLanguage->colour_normal, bufferLen);
				}
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
	if (KEYCHECK_PRESS(ev->keys,KEY_DOWN) && gMenusEndIndex!=0)
	{
		MENU_INC(gMenusCurrentItemIndex, NUM_DISPLAY_MENU_ITEMS);
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_UP))
	{
		MENU_DEC(gMenusCurrentItemIndex, NUM_DISPLAY_MENU_ITEMS);
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_RIGHT))
	{
		switch(gMenusCurrentItemIndex)
		{
			case DISPLAY_MENU_BRIGHTNESS:
				if (nonVolatileSettings.displayBacklightPercentage<10)
				{
					nonVolatileSettings.displayBacklightPercentage += 1;
				}
				else
				{
					nonVolatileSettings.displayBacklightPercentage += 10;
				}

				if (nonVolatileSettings.displayBacklightPercentage>100)
				{
					nonVolatileSettings.displayBacklightPercentage=100;
				}
				break;
			case DISPLAY_MENU_CONTRAST:
				contrast++;
				if (contrast > CONTRAST_MAX_VALUE)
				{
					contrast = CONTRAST_MAX_VALUE;
				}
				ucSetContrast(contrast);
				break;
			case DISPLAY_MENU_TIMEOUT:
				backLightTimeout += 5;
				if (backLightTimeout > BACKLIGHT_MAX_TIMEOUT)
				{
					backLightTimeout=0;
				}
				break;
			case DISPLAY_MENU_COLOUR_INVERT:
				inverseVideo=!inverseVideo;
				fw_init_display(inverseVideo);// Need to perform a full reset on the display to change back to non-inverted
				break;
		}
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_LEFT))
	{
		switch(gMenusCurrentItemIndex)
		{
			case DISPLAY_MENU_BRIGHTNESS:
				if (nonVolatileSettings.displayBacklightPercentage <= 10)
				{
					nonVolatileSettings.displayBacklightPercentage -= 1;
				}
				else
				{
					nonVolatileSettings.displayBacklightPercentage -= 10;
				}

				if (nonVolatileSettings.displayBacklightPercentage<0)
				{
					nonVolatileSettings.displayBacklightPercentage=0;
				}
				break;
			case DISPLAY_MENU_CONTRAST:
				contrast--;
				if (contrast < CONTRAST_MIN_VALUE)
				{
					contrast = CONTRAST_MIN_VALUE;
				}
				ucSetContrast(contrast);
				break;
			case DISPLAY_MENU_TIMEOUT:
				backLightTimeout -= 5;
				if (backLightTimeout < 0)
				{
					backLightTimeout = BACKLIGHT_MAX_TIMEOUT;
				}
				break;
			case DISPLAY_MENU_COLOUR_INVERT:
				inverseVideo=!inverseVideo;
				fw_init_display(inverseVideo);// Need to perform a full reset on the display to change back to non-inverted
				break;
		}
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_GREEN))
	{
		nonVolatileSettings.displayInverseVideo = inverseVideo;
		nonVolatileSettings.displayContrast = contrast;
		nonVolatileSettings.backLightTimeout = backLightTimeout;
		fw_init_display(nonVolatileSettings.displayInverseVideo);// Need to perform a full reset on the display to change back to non-inverted
		menuSystemPopAllAndDisplayRootMenu();
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_RED))
	{
		if (nonVolatileSettings.displayContrast != contrast || nonVolatileSettings.displayInverseVideo != inverseVideo)
		{
			fw_init_display(nonVolatileSettings.displayInverseVideo);// Need to perform a full reset on the display to change back to non-inverted
		}
		nonVolatileSettings.displayBacklightPercentage = originalBrightness;

		menuSystemPopPreviousMenu();
		return;
	}
	updateScreen();
}
