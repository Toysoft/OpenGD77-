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
#include <main.h>
#include <user_interface/menuSystem.h>
#include <user_interface/uiLocalisation.h>

static void updateScreen(void);
static void handleEvent(uiEvent_t *ev);

int menuDisplayMenuList(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		gMenuCurrentMenuList = (menuItemNew_t *)menusData[menuSystemGetCurrentMenuNumber()];
		gMenusEndIndex = gMenuCurrentMenuList[0].menuNum;// first entry actually contains the number of entries
		gMenuCurrentMenuList = &gMenuCurrentMenuList[1];// move to first real index
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
	int mNum;

	ucClearBuf();
	menuDisplayTitle(currentLanguage->menu);

	for(int i = -1; i <= 1 ; i++)
	{
		mNum = menuGetMenuOffset(gMenusEndIndex, i);

		if (mNum < gMenusEndIndex)
		{
			if (gMenuCurrentMenuList[mNum].stringOffset >= 0)
			{
				menuDisplayEntry(i, mNum, (const char *)currentLanguage + (gMenuCurrentMenuList[mNum].stringOffset * LANGUAGE_TEXTS_LENGTH));
			}
		}
	}

	ucRender();
	displayLightTrigger();
}

static void handleEvent(uiEvent_t *ev)
{
	displayLightTrigger();

	if (KEYCHECK_PRESS(ev->keys,KEY_DOWN))
	{
		MENU_INC(gMenusCurrentItemIndex, gMenusEndIndex);
		updateScreen();
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_UP))
	{
		MENU_DEC(gMenusCurrentItemIndex, gMenusEndIndex);
		updateScreen();
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_GREEN))
	{
		if (gMenuCurrentMenuList[gMenusCurrentItemIndex].menuNum!=-1)
		{
			menuSystemPushNewMenu(gMenuCurrentMenuList[gMenusCurrentItemIndex].menuNum);
		}
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_RED))
	{
		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_STAR) && (menuSystemGetCurrentMenuNumber() == MENU_MAIN_MENU))
	{
		keypadLocked = true;
		menuSystemPopAllAndDisplayRootMenu();
		menuSystemPushNewMenu(UI_LOCK_SCREEN);
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_HASH) && (menuSystemGetCurrentMenuNumber() == MENU_MAIN_MENU))
	{
		PTTLocked = !PTTLocked;
		menuSystemPopAllAndDisplayRootMenu();
		menuSystemPushNewMenu(UI_LOCK_SCREEN);
		return;
	}
}
