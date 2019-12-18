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
#include <functions/fw_ticks.h>
#include "fw_settings.h"

static void updateScreen(void);
static void handleEvent(uiEvent_t *ev);

static bool lockDisplay = false;
static const uint32_t TIMEOUT_MS = 2000;
static int lockScreenState;

enum LOCK_SCREEN_STATE { LOCK_SCREEN_STATE_IDLE=0, LOCK_SCREEN_STATE_CHANGED };

int menuLockScreen(uiEvent_t *ev, bool isFirstRun)
{
	static uint32_t m = 0;

	if (isFirstRun)
	{
		m = fw_millis();
		lockScreenState = LOCK_SCREEN_STATE_CHANGED;
		updateScreen();
		lockDisplay = true;
	}
	else
	{
		if (lockDisplay && ((ev->ticks - m) > TIMEOUT_MS))
		{
			menuSystemPopPreviousMenu();
			lockDisplay = false;
			return 0;
		}

		if (ev->hasEvent)
			handleEvent(ev);
	}
	return 0;
}

static void updateScreen(void)
{
	if (lockScreenState == LOCK_SCREEN_STATE_CHANGED)
	{
		ucClearBuf();
		ucDrawRoundRectWithDropShadow(4, 4, 120, 58, 5, true);
		if (keypadLocked || PTTLocked)
		{
			size_t bufferLen = strlen(currentLanguage->keypad) + 3 + strlen(currentLanguage->ptt) + 1;
			char buf[bufferLen];

			memset(buf, 0, bufferLen);

			if (keypadLocked)
				strcat(buf, currentLanguage->keypad);

			if (PTTLocked)
			{
				if (keypadLocked)
					strcat(buf, " & ");

				strcat(buf, currentLanguage->ptt);
			}
			buf[bufferLen -1] = 0;

			ucPrintCentered(6, buf, FONT_8x16);
			ucPrintCentered(22, currentLanguage->locked, FONT_8x16);
			ucPrintCentered(40, currentLanguage->press_blue_plus_star, FONT_6x8);
			ucPrintCentered(48, currentLanguage->to_unlock, FONT_6x8);
		}
		else
		{
			ucPrintCentered(24, currentLanguage->unlocked, FONT_8x16);
		}
		ucRender();
		displayLightTrigger();
	}
	lockScreenState = LOCK_SCREEN_STATE_IDLE;
}

static void handleEvent(uiEvent_t *ev)
{
	if (KEYCHECK_DOWN(ev->keys, KEY_STAR) && (ev->buttons & BUTTON_SK2))
	{
		keypadLocked = false;
		PTTLocked = false;
		menuSystemPopAllAndDisplayRootMenu();
		menuSystemPushNewMenu(MENU_LOCK_SCREEN);
	}
	else
	{
		// Hide immediately the lock/unlock window on key event, without waiting for timeout.
		if (lockDisplay && (((ev->keys.key != 0) && (ev->keys.event & KEY_MOD_UP)) ||
				((ev->events & BUTTON_EVENT) && (ev->buttons == BUTTON_NONE))))
		{
			menuSystemPopPreviousMenu();
			lockDisplay = false;
		}
	}
}
