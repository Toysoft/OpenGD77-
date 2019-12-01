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
static void handleEvent(int buttons, int keys, int events);
static int updateCounter;

int menuBattery(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		updateCounter=0;
		updateScreen();
	}
	else
	{
		if (++updateCounter > 10000)
		{
			updateCounter=0;
			updateScreen();// update the screen once per second to show any changes to the battery voltage
		}
		if (events!=0 && keys!=0)
		{
			handleEvent(buttons, keys, events);
		}
	}
	return 0;
}

static void updateScreen(void)
{
	const int MAX_BATTERY_BAR_HEIGHT = 36;
	char buffer[8];

	UC1701_clearBuf();
	menuDisplayTitle(currentLanguage->battery);

	int val1 = averageBatteryVoltage/10;
	int val2 = averageBatteryVoltage - (val1 * 10);

	sprintf(buffer,currentLanguage->batteryVoltage, val1, val2);
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

static void handleEvent(int buttons, int keys, int events)
{
	if (KEYCHECK_PRESS(keys,KEY_RED))
	{
		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_PRESS(keys,KEY_GREEN))
	{
		menuSystemPopAllAndDisplayRootMenu();
		return;
	}
}
