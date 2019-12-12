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
static void scrollDownOneLine(void);

//#define CREDIT_TEXT_LENGTH 33
static const int NUM_LINES_PER_SCREEN = 6;
const int NUM_CREDITS = 6;
static const char *creditTexts[] = {"Roger VK3KYY","Kai DG4KLU","Jason VK7ZJA","Alex DL4LEX","Daniel F1RMB","Colin G4EML"};
static int currentDisplayIndex=0;

int menuCredits(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		gMenusCurrentItemIndex=5000;
		updateScreen();
	}
	else
	{
		if (ev->hasEvent)
			handleEvent(ev);
/*
 * Uncomment to enable auto scrolling
		if ((gMenusCurrentItemIndex--)==0)
		{
			scrollDownOneLine();
			gMenusCurrentItemIndex=1000;
		}
*/
	}
	return 0;
}

static void updateScreen(void)
{
	ucClearBuf();
	menuDisplayTitle(currentLanguage->credits);

	for(int i=0;i<6;i++)
	{
		if ((i+currentDisplayIndex) < NUM_CREDITS)
		{
			ucPrintCentered(i*8 + 16,(char *)creditTexts[i+currentDisplayIndex], FONT_6x8);
		}
	}
	ucRender();
	displayLightTrigger();
}

static void scrollDownOneLine(void)
{
	if (currentDisplayIndex < (NUM_CREDITS - NUM_LINES_PER_SCREEN) )
	{
		currentDisplayIndex++;
	}
	updateScreen();
}

static void handleEvent(uiEvent_t *ev)
{

	if (KEYCHECK_SHORTUP(ev->keys,KEY_RED))
	{
		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_DOWN))
	{
		scrollDownOneLine();
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_UP))
	{
		if (currentDisplayIndex>0)
		{
			currentDisplayIndex--;
		}
		updateScreen();
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_GREEN))
	{
		menuSystemPopAllAndDisplayRootMenu();
		return;
	}
}
