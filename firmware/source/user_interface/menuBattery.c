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
static void handleEvent(uiEvent_t *ev);

int menuBattery(uiEvent_t *ev, bool isFirstRun)
{
	static uint32_t m = 0;

	if (isFirstRun)
	{
		updateScreen();
	}
	else
	{
		if ((ev->ticks - m) > 10000)
		{
			m = ev->ticks;
			updateScreen();// update the screen once per second to show any changes to the battery voltage
		}

		if (ev->hasEvent)
			handleEvent(ev);
	}
	return 0;
}

static void updateScreen(void)
{
	const int MAX_BATTERY_BAR_HEIGHT = 36;
	char buffer[17];

	UC1701_clearBuf();
	menuDisplayTitle(currentLanguage->battery);

	int val1 = averageBatteryVoltage/10;
	int val2 = averageBatteryVoltage - (val1 * 10);

	snprintf(buffer, 17, "%d.%dV", val1, val2);
	buffer[16] = 0;
	UC1701_printAt(20, 22, buffer, UC1701_FONT_16x32);
	uint32_t h = (uint32_t)(((averageBatteryVoltage - CUTOFF_VOLTAGE_UPPER_HYST) * MAX_BATTERY_BAR_HEIGHT) / (BATTERY_MAX_VOLTAGE - CUTOFF_VOLTAGE_UPPER_HYST));

	if (h > MAX_BATTERY_BAR_HEIGHT)
	{
		h = MAX_BATTERY_BAR_HEIGHT;
	}

	// Inner body frame
	UC1701_drawRoundRect(97, 20, 26, 42, 3, true);
	// Outer body frame
	UC1701_drawRoundRect(96, 19, 28, 44, 3, true);
	// Positive pole frame
	UC1701_fillRoundRect(96+9, 15, 10, 6, 2, true);
	// Level
	UC1701_fillRoundRect(100, 23 + MAX_BATTERY_BAR_HEIGHT - h , 20, h, 2, true);

	UC1701_render();
	displayLightTrigger();
}

static void handleEvent(uiEvent_t *ev)
{
	if (KEYCHECK_SHORTUP(ev->keys,KEY_RED))
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
