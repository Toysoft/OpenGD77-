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

static void updateScreen(void);
static void handleEvent(ui_event_t *ev);

int menuFirmwareInfoScreen(ui_event_t *ev, bool isFirstRun)
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
	char buf[17];

	snprintf(buf, 16, "[ %s ]", GITVERSION);
	buf[11] = 0; // git hash id 7 char long;

	UC1701_clearBuf();
	UC1701_printCentered(5, "OpenGD77",UC1701_FONT_8x16);
	UC1701_printCentered(24, currentLanguage->built, UC1701_FONT_8x8);
	UC1701_printCentered(34,__TIME__,UC1701_FONT_8x8);
	UC1701_printCentered(44,__DATE__,UC1701_FONT_8x8);
	UC1701_printCentered(54, buf, UC1701_FONT_8x8);
	UC1701_render();
	displayLightTrigger();
}


static void handleEvent(ui_event_t *ev)
{
	if (KEYCHECK_PRESS(ev->keys,KEY_RED))
	{
		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_GREEN))
	{
		menuSystemPopAllAndDisplayRootMenu();
		return;
	}
}
