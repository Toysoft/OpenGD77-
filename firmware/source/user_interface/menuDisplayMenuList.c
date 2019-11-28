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
#include "fw_main.h"

static void updateScreen(void);
static void handleEvent(int buttons, int keys, int events);

int menuDisplayMenuList(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{

		gMenuCurrentMenuList = (menuItemNew_t *)menusData[menuSystemGetCurrentMenuNumber()];
		gMenusEndIndex = gMenuCurrentMenuList[0].menuNum;// first entry actually contains the number of entries
		gMenuCurrentMenuList = & gMenuCurrentMenuList[1];// move to first real index
		gMenusCurrentItemIndex=1;
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
	int mNum;

	UC1701_clearBuf();
	menuDisplayTitle(currentLanguage->menu);

	for(int i = -1; i <= 1 ; i++)
	{
		mNum = menuGetMenuOffset(gMenusEndIndex, i);
		if (gMenuCurrentMenuList[mNum].stringNumber>=0)
		{
			menuDisplayEntry(i, mNum, (const char *)currentLanguage + (gMenuCurrentMenuList[mNum].stringNumber * LANGUAGE_TEXTS_LENGTH));
		}
	}

	UC1701_render();
	displayLightTrigger();
}

static void handleEvent(int buttons, int keys, int events)
{
	if ((keys & KEY_DOWN)!=0)
	{
		MENU_INC(gMenusCurrentItemIndex, gMenusEndIndex);
	}
	else if ((keys & KEY_UP)!=0)
	{
		MENU_DEC(gMenusCurrentItemIndex, gMenusEndIndex);
	}
	else if ((keys & KEY_GREEN)!=0)
	{
		if (gMenuCurrentMenuList[gMenusCurrentItemIndex].menuNum!=-1)
		{
			menuSystemPushNewMenu(gMenuCurrentMenuList[gMenusCurrentItemIndex].menuNum);
		}
		return;
	}
	else if ((keys & KEY_RED)!=0)
	{
		menuSystemPopPreviousMenu();
		return;
	}
	else if ((keys & KEY_STAR)!=0 && (menuSystemGetCurrentMenuNumber() == MENU_MAIN_MENU))
	{
		keypadLocked = true;
		menuSystemPopAllAndDisplayRootMenu();
		menuSystemPushNewMenu(MENU_LOCK_SCREEN);
		return;
	}

	updateScreen();
}
