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
static void handleEvent(uiEvent_t *ev);

int menuSplashScreen(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		updateScreen();
	}
	else
	{
		handleEvent(ev);
	}
	return 0;
}

static void updateScreen(void)
{
	char line1[16];
	char line2[16];

	codeplugGetBootItemTexts(line1,line2);
	UC1701_clearBuf();
	UC1701_printCentered(10, "OpenGD77",UC1701_FONT_8x16);
	UC1701_printCentered(28, line1,UC1701_FONT_8x16);
	UC1701_printCentered(42, line2,UC1701_FONT_8x16);
	UC1701_render();
	displayLightTrigger();
}

static void handleEvent(uiEvent_t *ev)
{
	static uint32_t m = 0;

	if (m == 0)
	{
		m = ev->ticks;
		return;
	}

	if ((ev->ticks - m) > 2000)
	{
		menuSystemSetCurrentMenu(nonVolatileSettings.initialMenuNumber);
	}
}
