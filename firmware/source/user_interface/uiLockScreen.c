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

static void updateScreen();
static void handleEvent(int buttons, int keys, int events);

static bool lockDisplay = false;
static int uiLockLastKey;

int menuLockScreen(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		menuTimer = 1000;
		updateScreen();
		uiLockLastKey = keys;
	}
	else
	{
		handleEvent(buttons, keys, events);
	}
	return 0;
}

static void updateScreen()
{
	char line1[16];
	char line2[16];

	codeplugGetBootItemTexts(line1,line2);
	UC1701_clearBuf();
	UC1701_drawRoundRect(4, 4, 120, 56, 5, true);
	if (keypadLocked) {
		UC1701_printCentered(10, "Keypad locked",UC1701_FONT_GD77_8x16);
		UC1701_printCentered(30, "Press Green, *",UC1701_FONT_6X8);
		UC1701_printCentered(38, "to unlock",UC1701_FONT_6X8);
	} else {
		UC1701_printCentered(10, "Unlocked",UC1701_FONT_GD77_8x16);
	}
	UC1701_render();
	displayLightTrigger();
}

static void handleEvent(int buttons, int keys, int events)
{
	menuTimer--;
	if (menuTimer == 0)
	{
		menuSystemPopPreviousMenu();
		lockDisplay = false;
	}
	if ((keys & KEY_GREEN) != 0)
	{
		uiLockLastKey = keys;
		return;
	}
	if ((uiLockLastKey & KEY_GREEN) != 0 && (keys & KEY_STAR) != 0)
	{
		keypadLocked = false;
		menuSystemPopAllAndDisplayRootMenu();
		menuSystemPushNewMenu(MENU_LOCK_SCREEN);
	}
}
