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
#include "fw_settings.h"

static void updateScreen();
static void handleEvent(int buttons, int keys, int events);

static uint8_t originalBrightness;
static uint8_t contrast;
static uint8_t inverseVideo;
static int8_t 	backLightTimeout;// used int_8 to save space

const int BACKLIGHT_MAX_TIMEOUT = 30;
const int CONTRAST_MAX_VALUE = 30;// Maximum value which still seems to be readable
const int CONTRAST_MIN_VALUE = 12;// Minimum value which still seems to be readable
enum DISPLAY_MENU_LIST { DISPLAY_MENU_BRIGHTNESS = 0, DISPLAY_MENU_CONTRAST, DISPLAY_MENU_TIMEOUT, DISPLAY_MENU_COLOUR_INVERT, NUM_DISPLAY_MENU_ITEMS};

int menuDisplayOptions(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		gMenusCurrentItemIndex=0;
		originalBrightness = nonVolatileSettings.displayBacklightPercentage;
		contrast = nonVolatileSettings.displayContrast;
		inverseVideo = nonVolatileSettings.displayInverseVideo;
		backLightTimeout = 	nonVolatileSettings.backLightTimeout;
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
	UC1701_printCentered(0, "Display options",UC1701_FONT_GD77_8x16);

	// Can only display 3 of the options at a time menu at -1, 0 and +1
	for(int i=-1;i<=1;i++)
	{
		mNum = gMenusCurrentItemIndex+i;
		if (mNum<0)
		{
			mNum = NUM_DISPLAY_MENU_ITEMS + mNum;
		}
		if (mNum >= NUM_DISPLAY_MENU_ITEMS)
		{
			mNum = mNum - NUM_DISPLAY_MENU_ITEMS;
		}

		switch(mNum)
		{
			case DISPLAY_MENU_BRIGHTNESS:
				sprintf(buf,"Brightness %d%%",nonVolatileSettings.displayBacklightPercentage);
				break;
			case DISPLAY_MENU_CONTRAST:
				sprintf(buf,"Contrast %d",contrast);
				break;
			case DISPLAY_MENU_TIMEOUT:
				sprintf(buf,"Timeout %d",backLightTimeout);
				break;
			case DISPLAY_MENU_COLOUR_INVERT:
				if (inverseVideo)
				{
					strcpy(buf,"Color:Invert");
				}
				else
				{
					strcpy(buf,"Color:Normal");
				}
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
	if ((keys & KEY_DOWN)!=0 && gMenusEndIndex!=0)
	{
		gMenusCurrentItemIndex++;
		if (gMenusCurrentItemIndex > NUM_DISPLAY_MENU_ITEMS)
		{
			gMenusCurrentItemIndex = 0;
		}
	}
	else if ((keys & KEY_UP)!=0)
	{
		gMenusCurrentItemIndex--;
		if (gMenusCurrentItemIndex < 0)
		{
			gMenusCurrentItemIndex=NUM_DISPLAY_MENU_ITEMS - 1;
		}
	}
	else if ((keys & KEY_RIGHT)!=0)
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
				UC1701_setContrast(contrast);
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
	else if ((keys & KEY_LEFT)!=0)
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
				UC1701_setContrast(contrast);
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
	else if ((keys & KEY_GREEN)!=0)
	{
		nonVolatileSettings.displayInverseVideo = inverseVideo;
		nonVolatileSettings.displayContrast = contrast;
		nonVolatileSettings.backLightTimeout = backLightTimeout;
		fw_init_display(nonVolatileSettings.displayInverseVideo);// Need to perform a full reset on the display to change back to non-inverted
		menuSystemPopAllAndDisplayRootMenu();
		return;
	}
	else if ((keys & KEY_RED)!=0)
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
