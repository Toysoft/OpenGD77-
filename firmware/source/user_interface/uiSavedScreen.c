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

static void updateScreen(void);
static void handleEvent(int buttons, int keys, int events);

static const int TIMEOUT_MS = 2000;

int menuSavedScreen(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		menuTimer = TIMEOUT_MS;
		updateScreen();
	}
	else
	{
		handleEvent(buttons, keys, events);
	}
	return 0;
}

static void updateScreen(void)
{
	UC1701_clearBuf();
	UC1701_drawRoundRect(4, 4, 120, 56, 5, true);
	UC1701_printCentered(18, "Saved",UC1701_FONT_16x32);
	UC1701_render();
	displayLightTrigger();
}

static void handleEvent(int buttons, int keys, int events)
{
	menuTimer--;
	if (menuTimer == 0)
	{
		menuSystemPopAllAndDisplayRootMenu();
	}

	if ((events & 0x01) && (keys != 0))
	{
		menuSystemPopAllAndDisplayRootMenu();
	}
}
