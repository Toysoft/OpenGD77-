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
#include <user_interface/uiUtilities.h>
#include <user_interface/uiLocalisation.h>
#include "fw_main.h"
#include "fw_settings.h"

static void updateScreen(void);
static void handleEvent(uiEvent_t *ev);
static struct_codeplugContact_t contact;
static int contactCallType;
static int menuContactListDisplayState;
static int menuContactListTimeout;
static int menuContactListOverrideState = 0;

enum MENU_CONTACT_LIST_STATE
{
	MENU_CONTACT_LIST_DISPLAY = 0,
	MENU_CONTACT_LIST_CONFIRM,
	MENU_CONTACT_LIST_DELETED,
	MENU_CONTACT_LIST_TG_IN_RXGROUP
};

static void reloadContactList(void)
{
	gMenusEndIndex = codeplugContactsGetCount(contactCallType);
	if (gMenusEndIndex > 0) {
		if (gMenusCurrentItemIndex >= gMenusEndIndex) {
			gMenusCurrentItemIndex = 0;
		}
		contactListContactIndex = codeplugContactGetDataForNumber(gMenusCurrentItemIndex+1, contactCallType, &contactListContactData);
	} else {
		contactListContactIndex = 0;
	}
}

int menuContactList(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		if (menuContactListOverrideState == 0) {
			if (contactListContactIndex == 0) {
				contactCallType = CONTACT_CALLTYPE_TG;
			} else {
				codeplugContactGetDataForIndex(contactListContactIndex, &contactListContactData);
				contactCallType = contactListContactData.callType;
			}
			reloadContactList();
			menuContactListDisplayState = MENU_CONTACT_LIST_DISPLAY;
		}
		else
		{
			codeplugContactGetDataForIndex(contactListContactIndex, &contactListContactData);
			menuContactListDisplayState = menuContactListOverrideState;
			menuContactListOverrideState = 0;
		}
		updateScreen();
	}
	else
	{
		if (ev->hasEvent || (menuContactListTimeout > 0))
		{
			handleEvent(ev);
		}
	}
	return 0;
}

static void updateScreen(void)
{
	char nameBuf[33];
	int mNum;
	int idx;
	const char *calltypeName[] = { currentLanguage->group_call, currentLanguage->private_call, currentLanguage->all_call };

	ucClearBuf();

	switch (menuContactListDisplayState)
	{
	case MENU_CONTACT_LIST_DISPLAY:
		menuDisplayTitle((char *) calltypeName[contactCallType]);

		if (gMenusEndIndex == 0)
		{
			ucPrintCentered(32, currentLanguage->empty_list, FONT_8x16);
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
		codeplugUtilConvertBufToString(contactListContactData.name, nameBuf, 16);
		menuDisplayTitle(nameBuf);
		ucPrintCentered(16, currentLanguage->delete_contact_qm, FONT_8x16);
		ucDrawChoice(CHOICE_YESNO, false);
		break;
	case MENU_CONTACT_LIST_DELETED:
		codeplugUtilConvertBufToString(contactListContactData.name, nameBuf, 16);
//		menuDisplayTitle(nameBuf);
		ucPrintCentered(16, currentLanguage->contact_deleted, FONT_8x16);
		ucDrawChoice(CHOICE_DISMISS, false);
		break;
	case MENU_CONTACT_LIST_TG_IN_RXGROUP:
		codeplugUtilConvertBufToString(contactListContactData.name, nameBuf, 16);
		menuDisplayTitle(nameBuf);
		ucPrintCentered(16, currentLanguage->contact_used, FONT_8x16);
		ucPrintCentered(32, currentLanguage->in_rx_group, FONT_8x16);
		ucDrawChoice(CHOICE_DISMISS, false);
		break;
	}
	ucRender();
	displayLightTrigger();
}

static void handleEvent(uiEvent_t *ev)
{
	switch (menuContactListDisplayState)
	{
	case MENU_CONTACT_LIST_DISPLAY:
		if (KEYCHECK_PRESS(ev->keys, KEY_DOWN))
		{
			MENU_INC(gMenusCurrentItemIndex, gMenusEndIndex);
			contactListContactIndex = codeplugContactGetDataForNumber(
					gMenusCurrentItemIndex + 1, contactCallType,
					&contactListContactData);
			updateScreen();
		}
		else if (KEYCHECK_PRESS(ev->keys, KEY_UP))
		{
			MENU_DEC(gMenusCurrentItemIndex, gMenusEndIndex);
			contactListContactIndex = codeplugContactGetDataForNumber(
					gMenusCurrentItemIndex + 1, contactCallType,
					&contactListContactData);
			updateScreen();
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_HASH))
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
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
		{
			if (menuSystemGetCurrentMenuNumber() == MENU_CONTACT_QUICKLIST)
			{
				setOverrideTGorPC(contactListContactData.tgNumber, contactListContactData.callType == CONTACT_CALLTYPE_PC);
				contactListContactIndex = 0;
				menuSystemPopAllAndDisplayRootMenu();
				return;
			} else {
				menuSystemPushNewMenu(MENU_CONTACT_LIST_SUBMENU);
				return;
			}
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
		{
			contactListContactIndex = 0;
			menuSystemPopPreviousMenu();
			return;
		}
		break;

	case MENU_CONTACT_LIST_CONFIRM:
		if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
		{
			memset(contact.name, 0xff, 16);
			contact.tgNumber = 0;
			contact.callType = 0xFF;
			codeplugContactSaveDataForIndex(contactListContactIndex, &contact);
			contactListContactIndex = 0;
			menuContactListTimeout = 2000;
			menuContactListDisplayState = MENU_CONTACT_LIST_DELETED;
			reloadContactList();
			updateScreen();
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
		{
			menuContactListDisplayState = MENU_CONTACT_LIST_DISPLAY;
			reloadContactList();
			updateScreen();
		}
		break;

	case MENU_CONTACT_LIST_DELETED:
	case MENU_CONTACT_LIST_TG_IN_RXGROUP:
		menuContactListTimeout--;
		if ((menuContactListTimeout == 0) || KEYCHECK_SHORTUP(ev->keys, KEY_GREEN) || KEYCHECK_SHORTUP(ev->keys, KEY_RED))
		{
			menuContactListDisplayState = MENU_CONTACT_LIST_DISPLAY;
			reloadContactList();
		}
		updateScreen();
		break;
	}
}

enum CONTACT_LIST_QUICK_MENU_ITEMS
{
	CONTACT_LIST_QUICK_MENU_SELECT = 0,
	CONTACT_LIST_QUICK_MENU_EDIT,
	CONTACT_LIST_QUICK_MENU_DELETE,
	NUM_CONTACT_LIST_QUICK_MENU_ITEMS    // The last item in the list is used so that we automatically get a total number of items in the list
};

static void updateSubMenuScreen(void)
{
	int mNum = 0;
	static const int bufferLen = 17;
	char buf[bufferLen];

	ucClearBuf();

	codeplugUtilConvertBufToString(contactListContactData.name, buf, 16);
	menuDisplayTitle(buf);

	for(int i = -1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_CONTACT_LIST_QUICK_MENU_ITEMS, i);

		switch(mNum)
		{
			case CONTACT_LIST_QUICK_MENU_SELECT:
				strncpy(buf, currentLanguage->select_tx, 17);
				break;
			case CONTACT_LIST_QUICK_MENU_EDIT:
				strncpy(buf, currentLanguage->edit_contact, 17);
				break;
			case CONTACT_LIST_QUICK_MENU_DELETE:
				strncpy(buf, currentLanguage->delete_contact, 17);
				break;
			default:
				strcpy(buf, "");
		}
		buf[bufferLen - 1] = 0;
		menuDisplayEntry(i, mNum, buf);
	}

	ucRender();
	displayLightTrigger();
}

static void handleSubMenuEvent(uiEvent_t *ev)
{
	if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
	{
		menuSystemPopPreviousMenu();
	}
	else if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
	{
		menuContactListOverrideState = 0;
		switch (gMenusCurrentItemIndex)
		{
		case CONTACT_LIST_QUICK_MENU_SELECT:
			setOverrideTGorPC(contactListContactData.tgNumber, contactListContactData.callType == CONTACT_CALLTYPE_PC);
			contactListContactIndex = 0;
			menuSystemPopAllAndDisplayRootMenu();
			break;
		case CONTACT_LIST_QUICK_MENU_EDIT:
			menuSystemSetCurrentMenu(MENU_CONTACT_DETAILS);
			break;
		case CONTACT_LIST_QUICK_MENU_DELETE:
 			if (contactListContactIndex > 0)
			{
				if (contactListContactData.callType == CONTACT_CALLTYPE_TG && codeplugContactGetRXGroup(contactListContactData.NOT_IN_CODEPLUGDATA_indexNumber)) {
					menuContactListTimeout = 2000;
					menuContactListOverrideState = MENU_CONTACT_LIST_TG_IN_RXGROUP;
				} else {
					menuContactListOverrideState = MENU_CONTACT_LIST_CONFIRM;
				}
				menuSystemPopPreviousMenu();
			}
			break;
		}
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_DOWN))
	{
		MENU_INC(gMenusCurrentItemIndex, NUM_CONTACT_LIST_QUICK_MENU_ITEMS);
		updateSubMenuScreen();
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_UP))
	{
		MENU_DEC(gMenusCurrentItemIndex, NUM_CONTACT_LIST_QUICK_MENU_ITEMS);
		updateSubMenuScreen();
	}
}

int menuContactListSubMenu(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		updateSubMenuScreen();
		fw_init_keyboard();
	}
	else
	{
		if (ev->hasEvent)
			handleSubMenuEvent(ev);
	}
	return 0;
}
