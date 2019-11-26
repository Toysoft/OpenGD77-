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
static int menuContactListDisplayState;
static int menuContactListTimeout;

enum MENU_CONTACT_LIST_STATE { MENU_CONTACT_LIST_DISPLAY=0, MENU_CONTACT_LIST_CONFIRM, MENU_CONTACT_LIST_DELETED, MENU_CONTACT_LIST_TG_IN_RXGROUP };

static const char *calltypeName[] = { "Group Call", "Private Call" };

void reloadContactList(void)
{
	gMenusEndIndex = codeplugContactsGetCount(contactCallType);
	gMenusCurrentItemIndex = 0;
	if (gMenusEndIndex > 0) {
		contactListContactIndex = codeplugContactGetDataForNumber(gMenusCurrentItemIndex+1, contactCallType, &contactListContactData);
	}
}

int menuContactList(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		contactCallType = CONTACT_CALLTYPE_TG;
		reloadContactList();
		fw_reset_keyboard();
		menuContactListDisplayState = MENU_CONTACT_LIST_DISPLAY;
		updateScreen();
	}
	else
	{
		if ((events!=0 && keys!=0) || menuContactListTimeout > 0)
		{
			handleEvent(buttons, keys, events);
		}
	}
	return 0;
}

static void updateScreen(void)
{
	char nameBuf[17];
	int mNum;
	int idx;

	UC1701_clearBuf();

	switch (menuContactListDisplayState)
	{
	case MENU_CONTACT_LIST_DISPLAY:
		menuDisplayTitle((char *) calltypeName[contactCallType]);

		if (gMenusEndIndex == 0)
		{
			UC1701_printCentered(32, "Empty List", UC1701_FONT_8x16);
		}
		else
		{
			for (int i = -1; i <= 1; i++)
			{
				mNum = menuGetMenuOffset(gMenusEndIndex, i);
				idx = codeplugContactGetDataForNumber(mNum + 1, contactCallType, &contact);

				if (idx > 0)
				{
					codeplugUtilConvertBufToString(contact.name, nameBuf, 16); // need to convert to zero terminated string
					menuDisplayEntry(i, mNum, (char*) nameBuf);
				}
			}
		}
		break;
	case MENU_CONTACT_LIST_CONFIRM:
		codeplugUtilConvertBufToString(contact.name, nameBuf, 16);
		menuDisplayTitle(nameBuf);
		UC1701_printCentered(16, "Delete contact?",UC1701_FONT_8x16);
		UC1701_printCentered(48, "YES          NO",UC1701_FONT_8x16);
		break;
	case MENU_CONTACT_LIST_DELETED:
		codeplugUtilConvertBufToString(contact.name, nameBuf, 16);
		menuDisplayTitle(nameBuf);
		UC1701_printCentered(16, "Contact deleted",UC1701_FONT_8x16);
		UC1701_drawChoice(UC1701_CHOICE_OK, false);
		break;
	case MENU_CONTACT_LIST_TG_IN_RXGROUP:
		codeplugUtilConvertBufToString(contact.name, nameBuf, 16);
		menuDisplayTitle(nameBuf);
		UC1701_printCentered(16, "Contact used",UC1701_FONT_8x16);
		UC1701_printCentered(32, "in RX group",UC1701_FONT_8x16);
		UC1701_drawChoice(UC1701_CHOICE_OK, false);
		break;
	}
	UC1701_render();
	displayLightTrigger();
}

static void handleEvent(int buttons, int keys, int events)
{
	if (KEYCHECK_LONGDOWN(keys, KEY_RED))
	{
		contactListContactIndex = 0;
		menuSystemPopAllAndDisplayRootMenu();
		return;
	}
	switch (menuContactListDisplayState)
	{
	case MENU_CONTACT_LIST_DISPLAY:
		if (events & 0x01)
		{
			if (KEYCHECK_PRESS(keys, KEY_DOWN))
			{
				MENU_INC(gMenusCurrentItemIndex, gMenusEndIndex);
				contactListContactIndex = codeplugContactGetDataForNumber(
						gMenusCurrentItemIndex + 1, contactCallType,
						&contactListContactData);
				updateScreen();
			}
			else if (KEYCHECK_PRESS(keys, KEY_UP))
			{
				MENU_DEC(gMenusCurrentItemIndex, gMenusEndIndex);
				contactListContactIndex = codeplugContactGetDataForNumber(
						gMenusCurrentItemIndex + 1, contactCallType,
						&contactListContactData);
				updateScreen();
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
				reloadContactList();
				updateScreen();
			}
			else if (KEYCHECK_SHORTUP(keys, KEY_GREEN))
			{
				contactListContactIndex = codeplugContactGetDataForNumber(
						gMenusCurrentItemIndex + 1, contactCallType, &contact);
				setOverrideTGorPC(contact.tgNumber,
						contact.callType == CONTACT_CALLTYPE_PC);
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
				if ((buttons & BUTTON_SK2) != 0)
				{
					contactListContactIndex = codeplugContactGetDataForNumber(
							gMenusCurrentItemIndex + 1, contactCallType,
							&contact);
					if (contactListContactIndex > 0)
					{
						if (contact.callType == CONTACT_CALLTYPE_TG && codeplugContactGetRXGroup(contact.NOT_IN_CODEPLUGDATA_indexNumber)) {
							menuContactListTimeout = 2000;
							menuContactListDisplayState = MENU_CONTACT_LIST_TG_IN_RXGROUP;
						} else {
							menuContactListDisplayState = MENU_CONTACT_LIST_CONFIRM;
						}
						updateScreen();
						return;
					}
				}
				contactListContactIndex = 0;
				menuSystemPopPreviousMenu();
				return;
			}
		}
		break;

	case MENU_CONTACT_LIST_CONFIRM:
		if (KEYCHECK_SHORTUP(keys, KEY_GREEN))
		{
			memset(contact.name, 0xff, 16);
			contact.tgNumber = 0;
			contact.callType = 0;
			codeplugContactSaveDataForIndex(contactListContactIndex, &contact);
			contactListContactIndex = 0;
			reloadContactList();
			menuContactListTimeout = 2000;
			menuContactListDisplayState = MENU_CONTACT_LIST_DELETED;
			updateScreen();
		}
		else if (KEYCHECK_SHORTUP(keys, KEY_RED))
		{
			menuContactListDisplayState = MENU_CONTACT_LIST_DISPLAY;
			updateScreen();
		}
		break;

	case MENU_CONTACT_LIST_DELETED:
	case MENU_CONTACT_LIST_TG_IN_RXGROUP:
		menuContactListTimeout--;
		if ((menuContactListTimeout == 0) || (KEYCHECK_SHORTUP(keys, KEY_GREEN)))
		{
			menuContactListDisplayState = MENU_CONTACT_LIST_DISPLAY;
		}
		updateScreen();
		break;
	}
}
