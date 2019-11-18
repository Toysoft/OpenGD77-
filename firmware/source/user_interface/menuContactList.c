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
#include <user_interface/menuUtilityQSOData.h>
#include "fw_main.h"
#include "fw_settings.h"

static void updateScreen();
static void handleEvent(int buttons, int keys, int events);
static struct_codeplugContact_t contact;

int menuContactList(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		gMenusEndIndex = codeplugContactsGetCount();
		gMenusCurrentItemIndex = 1;

		updateScreen();
	}
	else
	{
		if (events!=0 && keys!=0)
		{
			handleEvent(buttons, keys, events);
		}
	}
	return 0;
}

static void updateScreen()
{
	char nameBuf[17];
	int mNum;

	UC1701_clearBuf();
	menuDisplayTitle("Contact List");

	for(int i = -1; i <= 1; i++)
	{
		if (gMenusEndIndex <= (i + 1))
		{
			break;
		}

		mNum = menuGetMenuOffset(gMenusEndIndex, i);
		if (mNum == 0) {
			codeplugContactGetDataForNumber(gMenusEndIndex, &contact);
		} else {
			codeplugContactGetDataForNumber(mNum, &contact);
		}

		codeplugUtilConvertBufToString(contact.name, nameBuf, 16);// need to convert to zero terminated string
		menuDisplayEntry(i, mNum, (char* )nameBuf);
	}
	UC1701_render();
	displayLightTrigger();
}

static void handleEvent(int buttons, int keys, int events)
{
	if ((keys & KEY_DOWN)!=0)
	{
		MENU_INC(gMenusCurrentItemIndex, gMenusEndIndex);
	}
	else if ((keys & KEY_UP)!=0)
	{
		MENU_DEC(gMenusCurrentItemIndex, gMenusEndIndex);
	}
	else if ((keys & KEY_GREEN)!=0)
	{
		uint32_t saveTrxTalkGroupOrPcId = trxTalkGroupOrPcId;
		codeplugContactGetDataForNumber(gMenusCurrentItemIndex, &contact);

		nonVolatileSettings.overrideTG = contact.tgNumber;
		if (contact.callType == 0x01)
		{
			// Private Call

			if ((saveTrxTalkGroupOrPcId >> 24) != PC_CALL_FLAG)
			{
				// if the current Tx TG is a TalkGroup then save it so it can be stored after the end of the private call
				menuUtilityTgBeforePcMode = saveTrxTalkGroupOrPcId;
			}
			nonVolatileSettings.overrideTG |= (PC_CALL_FLAG << 24);
		}

		menuSystemPopAllAndDisplayRootMenu();
		return;
	}
	else if ((keys & KEY_RED)!=0)
	{
		menuSystemPopPreviousMenu();
		return;
	}
	updateScreen();
}
