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
#include <settings.h>
#include <user_interface/menuSystem.h>
#include <user_interface/uiLocalisation.h>


static void updateScreen(void);
static void handleEvent(uiEvent_t *ev);


int menuLanguage(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
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
	//stringsTable_t *lang;
	ucClearBuf();
	menuDisplayTitle("Language");

	// Can only display 3 of the options at a time menu at -1, 0 and +1
	for(int i = -1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_LANGUAGES, i);
		menuDisplayEntry(i, mNum, (char *)languages[mNum].LANGUAGE_NAME);
	}

	ucRender();
	displayLightTrigger();
}

static void handleEvent(uiEvent_t *ev)
{
	displayLightTrigger();

	if (KEYCHECK_PRESS(ev->keys,KEY_DOWN) && gMenusEndIndex!=0)
	{
		MENU_INC(gMenusCurrentItemIndex, NUM_LANGUAGES);
		updateScreen();
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_UP))
	{
		MENU_DEC(gMenusCurrentItemIndex, NUM_LANGUAGES);
		updateScreen();
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_GREEN))
	{
		nonVolatileSettings.languageIndex = gMenusCurrentItemIndex;
		currentLanguage = &languages[gMenusCurrentItemIndex];
		menuSystemLanguageHasChanged();
		SETTINGS_PLATFORM_SPECIFIC_SAVE_SETTINGS(false);// Some platform require the settings to be saved immediately
		menuSystemPopAllAndDisplayRootMenu();
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_RED))
	{
		menuSystemPopPreviousMenu();
		return;
	}
}
