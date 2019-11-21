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
static int contactCallType;

static const char *calltypeName[] = { "Group Call", "Private Call" };

int menuContactList(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		gMenusEndIndex = codeplugContactsGetCount(contactCallType);
		gMenusCurrentItemIndex = 1;
		contactListContactIndex = codeplugContactGetDataForNumber(gMenusCurrentItemIndex, contactCallType, &contactListContactData);

		fw_reset_keyboard();
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
	menuDisplayTitle( (char *)calltypeName[contactCallType]);

	if (gMenusEndIndex == 0)
	{
		UC1701_printCentered(48, "Empty List", UC1701_FONT_8x16);
	} else {
		for (int i = -1; i <= 1; i++) {
			if (gMenusEndIndex <= (i + 1)) {
				break;
			}

			mNum = menuGetMenuOffset(gMenusEndIndex, i);
			if (mNum == 0) {
				codeplugContactGetDataForNumber(gMenusEndIndex, contactCallType, &contact);
			} else {
				codeplugContactGetDataForNumber(mNum, contactCallType, &contact);
			}

			codeplugUtilConvertBufToString(contact.name, nameBuf, 16); // need to convert to zero terminated string
			menuDisplayEntry(i, mNum, (char*) nameBuf);
		}
	}
	UC1701_render();
	displayLightTrigger();
}

static void handleEvent(int buttons, int keys, int events)
{
	if (events & 0x01)
	{
		USB_DEBUG_printf("key: %d mod: %d", keys & 0xffffff,keys>>24);
		if (KEYCHECK_PRESS(keys, KEY_DOWN))
		{
			MENU_INC(gMenusCurrentItemIndex, gMenusEndIndex);
			contactListContactIndex = codeplugContactGetDataForNumber(gMenusCurrentItemIndex, contactCallType, &contactListContactData);
		}
		else if (KEYCHECK_PRESS(keys, KEY_UP))
		{
			MENU_DEC(gMenusCurrentItemIndex, gMenusEndIndex);
			contactListContactIndex = codeplugContactGetDataForNumber(gMenusCurrentItemIndex, contactCallType, &contactListContactData);
		}
		else if (KEYCHECK_SHORTUP(keys, KEY_HASH))
		{
			if (contactCallType == CONTACT_CALLTYPE_TG)
			{
				contactCallType = CONTACT_CALLTYPE_PC;
			}
			else
			{
				contactCallType = CONTACT_CALLTYPE_TG;
			}
			gMenusEndIndex = codeplugContactsGetCount(contactCallType);
			gMenusCurrentItemIndex = 1;
			contactListContactIndex = codeplugContactGetDataForNumber(gMenusCurrentItemIndex, contactCallType, &contactListContactData);
		}
		else if (KEYCHECK_SHORTUP(keys, KEY_GREEN))
		{
			contactListContactIndex = codeplugContactGetDataForNumber(gMenusCurrentItemIndex, contactCallType, &contact);
			setOverrideTGorPC(contact.tgNumber, contact.callType == CONTACT_CALLTYPE_PC);
			contactListContactIndex = 0;
			menuSystemPopAllAndDisplayRootMenu();
			return;
		}
		else if (KEYCHECK_LONGDOWN(keys, KEY_GREEN))
		{
			menuSystemPushNewMenu(MENU_CONTACT_DETAILS);
			return;
		}
		else if (KEYCHECK_SHORTUP(keys, KEY_RED))
		{
			contactListContactIndex = 0;
			menuSystemPopPreviousMenu();
			return;
		} else if (KEYCHECK_LONGDOWN(keys, KEY_RED))
		{
			contactListContactIndex = 0;
			menuSystemPopAllAndDisplayRootMenu();
			return;
		}
	}
	updateScreen();
}
