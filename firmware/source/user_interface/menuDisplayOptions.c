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
static void updateBacklightMode(uint8_t mode);

static const int BACKLIGHT_MAX_TIMEOUT = 30;
static const int CONTRAST_MAX_VALUE = 30;// Maximum value which still seems to be readable
static const int CONTRAST_MIN_VALUE = 12;// Minimum value which still seems to be readable
static const int BACKLIGHT_TIMEOUT_STEP = 5;
static const int BACKLIGHT_MAX_PERCENTAGE = 100;
static const int BACKLIGHT_PERCENTAGE_STEP = 10;
static const int BACKLIGHT_PERCENTAGE_STEP_SMALL = 1;

static const char *contactOrders[] = { "Ct/DB/TA", "DB/Ct/TA", "TA/Ct/DB", "TA/DB/Ct" };

enum DISPLAY_MENU_LIST { 	DISPLAY_MENU_BRIGHTNESS = 0, DISPLAY_MENU_BRIGHTNESS_OFF, DISPLAY_MENU_CONTRAST, DISPLAY_MENU_BACKLIGHT_MODE, DISPLAY_MENU_TIMEOUT, DISPLAY_MENU_COLOUR_INVERT,
							DISPLAY_MENU_CONTACT_DISPLAY_ORDER, DISPLAY_MENU_CONTACT_DISPLAY_SPLIT_CONTACT,
							NUM_DISPLAY_MENU_ITEMS};


int menuDisplayOptions(uiEvent_t *ev, bool isFirstRun)
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
	const char *backlightModes[] = { currentLanguage->Auto, currentLanguage->manual, currentLanguage->none };

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
			case DISPLAY_MENU_BRIGHTNESS_OFF:
				snprintf(buf, bufferLen, "%s:%d%%", currentLanguage->brightness_off, nonVolatileSettings.displayBacklightPercentageOff);
				break;
			case DISPLAY_MENU_CONTRAST:
				snprintf(buf, bufferLen, "%s:%d", currentLanguage->contrast, nonVolatileSettings.displayContrast);
				break;
			case DISPLAY_MENU_BACKLIGHT_MODE:
				snprintf(buf, bufferLen, "%s:%s", currentLanguage->mode, backlightModes[nonVolatileSettings.backlightMode]);
				break;
			case DISPLAY_MENU_TIMEOUT:
				if (nonVolatileSettings.backlightMode == BACKLIGHT_MODE_AUTO)
				{
					if (nonVolatileSettings.backLightTimeout == 0)
						snprintf(buf, bufferLen, "%s:%s", currentLanguage->backlight_timeout, currentLanguage->no);
					else
						snprintf(buf, bufferLen, "%s:%ds", currentLanguage->backlight_timeout, nonVolatileSettings.backLightTimeout);
				}
				else
				{
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->backlight_timeout, currentLanguage->n_a);
				}
				break;
			case DISPLAY_MENU_COLOUR_INVERT:
				if (nonVolatileSettings.displayInverseVideo)
				{
					strncpy(buf, currentLanguage->colour_invert, bufferLen);
				}
				else
				{
					strncpy(buf, currentLanguage->colour_normal, bufferLen);
				}
				break;
			case DISPLAY_MENU_CONTACT_DISPLAY_ORDER:
				{
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->priority_order, contactOrders[nonVolatileSettings.contactDisplayPriority]);
				}
				break;
			case DISPLAY_MENU_CONTACT_DISPLAY_SPLIT_CONTACT:
				{
					const char *splitContact[] = { currentLanguage->one_line, currentLanguage->two_lines, currentLanguage->Auto };
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->contact, splitContact[nonVolatileSettings.splitContact]);
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
	if (ev->events & FUNCTION_EVENT)
	{
		if (ev->function == DEC_BRIGHTNESS)
		{
			if (nonVolatileSettings.displayBacklightPercentage <= BACKLIGHT_PERCENTAGE_STEP)
			{
				nonVolatileSettings.displayBacklightPercentage -= BACKLIGHT_PERCENTAGE_STEP_SMALL;
			}
			else
			{
				nonVolatileSettings.displayBacklightPercentage -= BACKLIGHT_PERCENTAGE_STEP;
			}

			if (nonVolatileSettings.displayBacklightPercentage<0)
			{
				nonVolatileSettings.displayBacklightPercentage=0;
			}
			displayLightTrigger();
			menuSystemPopPreviousMenu();
			return;
			//			gMenusCurrentItemIndex = DISPLAY_MENU_BRIGHTNESS;
		}
		else if (ev->function == INC_BRIGHTNESS)
		{
			if (nonVolatileSettings.displayBacklightPercentage < BACKLIGHT_PERCENTAGE_STEP)
			{
				nonVolatileSettings.displayBacklightPercentage += BACKLIGHT_PERCENTAGE_STEP_SMALL;
			}
			else
			{
				nonVolatileSettings.displayBacklightPercentage += BACKLIGHT_PERCENTAGE_STEP;
			}

			if (nonVolatileSettings.displayBacklightPercentage>BACKLIGHT_MAX_PERCENTAGE)
			{
				nonVolatileSettings.displayBacklightPercentage=BACKLIGHT_MAX_PERCENTAGE;
			}
			displayLightTrigger();
			menuSystemPopPreviousMenu();
			return;
		}
	}
	if (ev->events & KEY_EVENT)
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
					if (nonVolatileSettings.displayBacklightPercentage < BACKLIGHT_PERCENTAGE_STEP)
					{
						nonVolatileSettings.displayBacklightPercentage += BACKLIGHT_PERCENTAGE_STEP_SMALL;
					}
					else
					{
						nonVolatileSettings.displayBacklightPercentage += BACKLIGHT_PERCENTAGE_STEP;
					}

					if (nonVolatileSettings.displayBacklightPercentage > BACKLIGHT_MAX_PERCENTAGE)
					{
						nonVolatileSettings.displayBacklightPercentage = BACKLIGHT_MAX_PERCENTAGE;
					}
					break;
				case DISPLAY_MENU_BRIGHTNESS_OFF:
					if (nonVolatileSettings.displayBacklightPercentageOff < nonVolatileSettings.displayBacklightPercentage)
					{
						if (nonVolatileSettings.displayBacklightPercentageOff < BACKLIGHT_PERCENTAGE_STEP)
						{
							nonVolatileSettings.displayBacklightPercentageOff += BACKLIGHT_PERCENTAGE_STEP_SMALL;
						}
						else
						{
							nonVolatileSettings.displayBacklightPercentageOff += BACKLIGHT_PERCENTAGE_STEP;
						}

						if (nonVolatileSettings.displayBacklightPercentageOff > BACKLIGHT_MAX_PERCENTAGE)
						{
							nonVolatileSettings.displayBacklightPercentageOff = BACKLIGHT_MAX_PERCENTAGE;
						}

						if (nonVolatileSettings.displayBacklightPercentageOff > nonVolatileSettings.displayBacklightPercentage)
						{
							nonVolatileSettings.displayBacklightPercentageOff = nonVolatileSettings.displayBacklightPercentage;
						}
					}
					break;
				case DISPLAY_MENU_CONTRAST:
					if (nonVolatileSettings.displayContrast < CONTRAST_MAX_VALUE)
					{
						nonVolatileSettings.displayContrast++;
					}
					ucSetContrast(nonVolatileSettings.displayContrast);
					break;
				case DISPLAY_MENU_BACKLIGHT_MODE:
					if (nonVolatileSettings.backlightMode < BACKLIGHT_MODE_NONE)
					{
						updateBacklightMode((++nonVolatileSettings.backlightMode));
					}
					break;
				case DISPLAY_MENU_TIMEOUT:
					nonVolatileSettings.backLightTimeout += BACKLIGHT_TIMEOUT_STEP;
					if (nonVolatileSettings.backLightTimeout > BACKLIGHT_MAX_TIMEOUT)
					{
						nonVolatileSettings.backLightTimeout = BACKLIGHT_MAX_TIMEOUT;
					}
					break;
				case DISPLAY_MENU_COLOUR_INVERT:
					nonVolatileSettings.displayInverseVideo = !nonVolatileSettings.displayInverseVideo;
					fw_init_display(nonVolatileSettings.displayInverseVideo);// Need to perform a full reset on the display to change back to non-inverted
					break;
				case DISPLAY_MENU_CONTACT_DISPLAY_ORDER:
					if (nonVolatileSettings.contactDisplayPriority < CONTACT_DISPLAY_PRIO_TA_DB_CC)
					{
						nonVolatileSettings.contactDisplayPriority++;
					}
					break;
				case DISPLAY_MENU_CONTACT_DISPLAY_SPLIT_CONTACT:
					if (nonVolatileSettings.splitContact < SPLIT_CONTACT_AUTO)
					{
						nonVolatileSettings.splitContact++;
					}
					break;
			}
		}
		else if (KEYCHECK_PRESS(ev->keys,KEY_LEFT))
		{
			switch(gMenusCurrentItemIndex)
			{
				case DISPLAY_MENU_BRIGHTNESS:
					if (nonVolatileSettings.displayBacklightPercentage <= BACKLIGHT_PERCENTAGE_STEP)
					{
						nonVolatileSettings.displayBacklightPercentage -= 1;
					}
					else
					{
						nonVolatileSettings.displayBacklightPercentage -= BACKLIGHT_PERCENTAGE_STEP;
					}

					if (nonVolatileSettings.displayBacklightPercentage < 0)
					{
						nonVolatileSettings.displayBacklightPercentage = 0;
					}

					if (nonVolatileSettings.displayBacklightPercentageOff > nonVolatileSettings.displayBacklightPercentage)
					{
						nonVolatileSettings.displayBacklightPercentageOff = nonVolatileSettings.displayBacklightPercentage;
					}
					break;
				case DISPLAY_MENU_BRIGHTNESS_OFF:
					if (nonVolatileSettings.displayBacklightPercentageOff <= BACKLIGHT_PERCENTAGE_STEP)
					{
						nonVolatileSettings.displayBacklightPercentageOff -= BACKLIGHT_PERCENTAGE_STEP_SMALL;
					}
					else
					{
						nonVolatileSettings.displayBacklightPercentageOff -= BACKLIGHT_PERCENTAGE_STEP;
					}

					if (nonVolatileSettings.displayBacklightPercentageOff < 0)
					{
						nonVolatileSettings.displayBacklightPercentageOff = 0;
					}
					break;
				case DISPLAY_MENU_CONTRAST:
					if (nonVolatileSettings.displayContrast > CONTRAST_MIN_VALUE)
					{
						nonVolatileSettings.displayContrast--;
					}
					ucSetContrast(nonVolatileSettings.displayContrast);
					break;
				case DISPLAY_MENU_BACKLIGHT_MODE:
					if (nonVolatileSettings.backlightMode > BACKLIGHT_MODE_AUTO)
					{
						updateBacklightMode((--nonVolatileSettings.backlightMode));
					}
					break;
				case DISPLAY_MENU_TIMEOUT:
					if (nonVolatileSettings.backLightTimeout >= BACKLIGHT_TIMEOUT_STEP)
					{
						nonVolatileSettings.backLightTimeout -= BACKLIGHT_TIMEOUT_STEP;
					}
					break;
				case DISPLAY_MENU_COLOUR_INVERT:
					nonVolatileSettings.displayInverseVideo = !nonVolatileSettings.displayInverseVideo;
					fw_init_display(nonVolatileSettings.displayInverseVideo);// Need to perform a full reset on the display to change back to non-inverted
					break;
				case DISPLAY_MENU_CONTACT_DISPLAY_ORDER:
					if (nonVolatileSettings.contactDisplayPriority > CONTACT_DISPLAY_PRIO_CC_DB_TA)
					{
						nonVolatileSettings.contactDisplayPriority--;
					}
					break;
				case DISPLAY_MENU_CONTACT_DISPLAY_SPLIT_CONTACT:
					if (nonVolatileSettings.splitContact > SPLIT_CONTACT_SINGLE_LINE_ONLY)
					{
						nonVolatileSettings.splitContact--;
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
			if (nonVolatileSettings.displayContrast != originalNonVolatileSettings.displayContrast)
			{
				nonVolatileSettings.displayContrast = originalNonVolatileSettings.displayContrast;
				ucSetContrast(nonVolatileSettings.displayContrast);
			}

			if (nonVolatileSettings.displayInverseVideo != originalNonVolatileSettings.displayInverseVideo)
			{
				nonVolatileSettings.displayInverseVideo = originalNonVolatileSettings.displayInverseVideo;
				fw_init_display(nonVolatileSettings.displayInverseVideo);// Need to perform a full reset on the display to change back to non-inverted
			}

			nonVolatileSettings.displayBacklightPercentage = originalNonVolatileSettings.displayBacklightPercentage;
			nonVolatileSettings.displayBacklightPercentageOff = originalNonVolatileSettings.displayBacklightPercentageOff;
			nonVolatileSettings.backLightTimeout = originalNonVolatileSettings.backLightTimeout;

			if (nonVolatileSettings.backlightMode != originalNonVolatileSettings.backlightMode)
			{
				updateBacklightMode(originalNonVolatileSettings.backlightMode);
			}

			menuSystemPopPreviousMenu();
			return;
		}
	}
	updateScreen();
}

static void updateBacklightMode(uint8_t mode)
{
	nonVolatileSettings.backlightMode = mode;

	switch (mode)
	{
		case BACKLIGHT_MODE_MANUAL:
		case BACKLIGHT_MODE_NONE:
			if (fw_displayIsBacklightLit())
				fw_displayEnableBacklight(false);
			break;
		case BACKLIGHT_MODE_AUTO:
			displayLightTrigger();
			break;
	}
}
