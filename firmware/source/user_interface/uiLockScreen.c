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

enum LOCK_STATE { LOCK_NONE = 0x00, LOCK_KEYPAD = 0x01, LOCK_PTT = 0x02, LOCK_BOTH = 0x03 };

static void updateScreen(bool update);
static void handleEvent(uiEvent_t *ev);

static bool lockDisplayed = false;
static const uint32_t TIMEOUT_MS = 500;
int lockState = LOCK_NONE;

int menuLockScreen(uiEvent_t *ev, bool isFirstRun)
{
	static uint32_t m = 0;

	if (isFirstRun)
	{
		m = fw_millis();

		updateScreen(lockDisplayed);
	}
	else
	{
		if (lockDisplayed && ((ev->time - m) > TIMEOUT_MS))
		{
			lockDisplayed = false;
			menuSystemPopPreviousMenu();
			return 0;
		}

		if (ev->hasEvent)
		{
			m = fw_millis(); // reset timer on each key button/event.

			handleEvent(ev);
		}
	}
	return 0;
}

static void redrawScreen(bool update, bool state)
{
	if (update)
	{
		// Clear inner rect only
		ucFillRoundRect(5, 3, 118, 56, 5, false);
	}
	else
	{
		// Clear whole screen
		ucClearBuf();
		ucDrawRoundRectWithDropShadow(4, 4, 120, 58, 5, true);
	}

	if (state)
	{
		int bufferLen = strlen(currentLanguage->keypad) + 3 + strlen(currentLanguage->ptt) + 1;
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
		buf[bufferLen - 1] = 0;

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

	lockDisplayed = true;
}

static void updateScreen(bool updateOnly)
{
	bool keypadChanged = false;
	bool PTTChanged = false;

	if (keypadLocked)
	{
		if ((lockState & LOCK_KEYPAD) == 0)
		{
			keypadChanged = true;
			lockState |= LOCK_KEYPAD;
		}
	}
	else
	{
		if ((lockState & LOCK_KEYPAD))
		{
			keypadChanged = true;
			lockState &= ~LOCK_KEYPAD;
		}
	}

	if (PTTLocked)
	{
		if ((lockState & LOCK_PTT) == 0)
		{
			PTTChanged = true;
			lockState |= LOCK_PTT;
		}
	}
	else
	{
		if ((lockState & LOCK_PTT))
		{
			PTTChanged = true;
			lockState &= ~LOCK_PTT;
		}
	}

	if (updateOnly)
	{
		if (keypadChanged || PTTChanged)
		{
			redrawScreen(updateOnly, ((lockState & LOCK_KEYPAD) || (lockState & LOCK_PTT)));
		}
		else
		{
			if (lockDisplayed == false)
			{
				redrawScreen(updateOnly, false);
			}
		}
	}
	else
	{
		// Draw everything
		redrawScreen(false, keypadLocked || PTTLocked);
	}
}

static void handleEvent(uiEvent_t *ev)
{
	if (KEYCHECK_DOWN(ev->keys, KEY_STAR) && (ev->buttons & BUTTON_SK2))
	{
		keypadLocked = false;
		PTTLocked = false;
		lockDisplayed = false;
		menuSystemPopAllAndDisplayRootMenu();
		menuSystemPushNewMenu(MENU_LOCK_SCREEN);
	}

	displayLightTrigger();
}

void menuLockScreenPop(void)
{
	lockDisplayed = false;

	if (menuSystemGetCurrentMenuNumber() == MENU_LOCK_SCREEN)
		menuSystemPopPreviousMenu();
}
