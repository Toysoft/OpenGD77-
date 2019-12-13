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
#include <fw_codeplug.h>
#include <user_interface/menuSystem.h>
#include <user_interface/uiLocalisation.h>
#include "fw_main.h"
#include "fw_settings.h"

static void updateScreen(void);
static void handleEvent(uiEvent_t *ev);

int menuZoneList(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		gMenusEndIndex = codeplugZonesGetCount();
		gMenusCurrentItemIndex = nonVolatileSettings.currentZone;
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
	char nameBuf[17];
	int mNum;
	struct_codeplugZone_t zoneBuf;

	ucClearBuf();
	menuDisplayTitle(currentLanguage->zones);

	for(int i = -1; i <= 1; i++)
	{
		if (gMenusEndIndex <= (i + 1))
		{
			break;
		}

		mNum = menuGetMenuOffset(gMenusEndIndex, i);

		codeplugZoneGetDataForNumber(mNum, &zoneBuf);
		codeplugUtilConvertBufToString(zoneBuf.name, nameBuf, 16);// need to convert to zero terminated string

		menuDisplayEntry(i, mNum, (char* )nameBuf);
	}

	ucRender();
	displayLightTrigger();
}

static void handleEvent(uiEvent_t *ev)
{
	if (KEYCHECK_PRESS(ev->keys,KEY_DOWN))
	{
		MENU_INC(gMenusCurrentItemIndex, gMenusEndIndex);
		updateScreen();
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_UP))
	{
		MENU_DEC(gMenusCurrentItemIndex, gMenusEndIndex);
		updateScreen();
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_GREEN))
	{
		nonVolatileSettings.overrideTG = 0; // remove any TG override
		nonVolatileSettings.currentZone = gMenusCurrentItemIndex;
		nonVolatileSettings.currentChannelIndexInZone = 0;// Since we are switching zones the channel index should be reset
		nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE]=0;// Since we are switching zones the TRx Group index should be reset
		channelScreenChannelData.rxFreq=0x00; // Flag to the Channel screen that the channel data is now invalid and needs to be reloaded
		menuSystemPopAllAndDisplaySpecificRootMenu(MENU_CHANNEL_MODE);

		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_RED))
	{
		menuSystemPopPreviousMenu();
		return;
	}
}
