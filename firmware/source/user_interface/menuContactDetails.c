/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
 * 				and	 Colin Durbridge, G4EML
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
#include <user_interface/menuUtilityQSOData.h>
#include "fw_trx.h"
#include "fw_codeplug.h"
#include "fw_settings.h"

static void updateScreen(void);
static void handleEvent(int buttons, int keys, int events);

static struct_codeplugContact_t tmpContact;
const static char callTypeString[3][8] = { "Group", "Private", "All" };

static char digits[9];

enum CONTACT_DETAILS_DISPLAY_LIST { /*CONTACT_DETAILS_NAME=0,*/ CONTACT_DETAILS_TG=0, CONTACT_DETAILS_CALLTYPE, CONTACT_DETAILS_TS,
	NUM_CONTACT_DETAILS_ITEMS};// The last item in the list is used so that we automatically get a total number of items in the list

int menuContactDetails(int buttons, int keys, int events, bool isFirstRun)
{
	char buf[17];
	if (isFirstRun)
	{
		if (contactListContactIndex == 0) {
			contactListContactIndex = codeplugContactGetFreeIndex();

			sprintf(buf,"Contact%d",contactListContactIndex);
			codeplugUtilConvertStringToBuf(buf, tmpContact.name, 16);
			tmpContact.callType = CONTACT_CALLTYPE_TG;
			tmpContact.reserve1 = 0xff;
			tmpContact.tgNumber = 0;
			digits[0] = 0x00;
		} else {
			memcpy(&tmpContact, &contactListContactData,sizeof(struct_codeplugContact_t));
			itoa(tmpContact.tgNumber, digits, 10);
		}

		gMenusCurrentItemIndex=0;

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

static void updateScreen(void)
{
	int mNum = 0;
	char buf[17];

	UC1701_clearBuf();
//	menuDisplayTitle("Contact details");

	codeplugUtilConvertBufToString(tmpContact.name, buf, 16);
	menuDisplayTitle(buf);

	// Can only display 3 of the options at a time menu at -1, 0 and +1
	for(int i = -1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_CONTACT_DETAILS_ITEMS, i);

		switch (mNum)
		{
//			case CONTACT_DETAILS_Name:
//				strcpy(buf,"Name");
//				break;
		case CONTACT_DETAILS_TG:
			switch (tmpContact.callType)
			{
			case CONTACT_CALLTYPE_TG:
				sprintf(buf, "TG:%s", digits);
				break;
			case CONTACT_CALLTYPE_PC: // Private
				sprintf(buf, "PC:%s", digits);
				break;
			case CONTACT_CALLTYPE_ALL: // All Call
				strcpy(buf, "All:16777215");
				break;
			}
			break;
		case CONTACT_DETAILS_CALLTYPE:
			strcpy(buf, "Type:");
			strcat(buf, callTypeString[tmpContact.callType]);
			break;
		case CONTACT_DETAILS_TS:
			switch (tmpContact.reserve1 & 0x3)
			{
			case 1:
			case 3:
				strcpy(buf, "Timeslot:none");
				break;
			case 0:
				strcpy(buf, "Timeslot:1");
				break;
			case 2:
				strcpy(buf, "Timeslot: 2");
				break;
			}
			break;
		}

		menuDisplayEntry(i, mNum, buf);
	}

	UC1701_render();
	displayLightTrigger();
}

static void handleEvent(int buttons, int keys, int events)
{
	int sLen = strlen(digits);

	if (events & 0x01) {
		if (KEYCHECK_PRESS(keys,KEY_DOWN))
		{
			MENU_INC(gMenusCurrentItemIndex, NUM_CONTACT_DETAILS_ITEMS);
		}
		else if (KEYCHECK_PRESS(keys,KEY_UP))
		{
			MENU_DEC(gMenusCurrentItemIndex, NUM_CONTACT_DETAILS_ITEMS);
		}
		else if (KEYCHECK_PRESS(keys,KEY_RIGHT))
		{
			switch(gMenusCurrentItemIndex)
			{
//			case CONTACT_DETAILS_NAME:
//				break;
			case CONTACT_DETAILS_TG:
				break;
			case CONTACT_DETAILS_CALLTYPE:
				MENU_INC(tmpContact.callType,3);
				break;
			case CONTACT_DETAILS_TS:
				switch (tmpContact.reserve1 & 0x3) {
				case 1:
				case 3:
					tmpContact.reserve1 &= 0xfc;
					break;
				case 0:
					tmpContact.reserve1 |= 0x02;
					break;
				case 2:
					tmpContact.reserve1 |= 0x03;
					break;
				}
				break;
			}
		}
		else if (KEYCHECK_PRESS(keys,KEY_LEFT))
		{
			switch(gMenusCurrentItemIndex)
			{
//			case CONTACT_DETAILS_NAME:
//				break;
			case CONTACT_DETAILS_TG:
				if (sLen>0) {
					digits[sLen-1] = 0x00;
				}
				break;
			case CONTACT_DETAILS_CALLTYPE:
				MENU_DEC(tmpContact.callType,3);
				break;
			case CONTACT_DETAILS_TS:
				switch (tmpContact.reserve1 & 0x3) {
				case 1:
				case 3:
					tmpContact.reserve1 &= 0xfc;
					tmpContact.reserve1 |= 0x02;
					break;
				case 0:
					tmpContact.reserve1 |= 0x03;
					break;
				case 2:
					tmpContact.reserve1 &= 0xfc;
					break;
				}
				break;
			}
		}
		else if (KEYCHECK_SHORTUP(keys,KEY_GREEN))
		{
			if (tmpContact.callType == CONTACT_CALLTYPE_ALL) {
				tmpContact.tgNumber = 16777215;
			}
			if (contactListContactIndex > 0 && contactListContactIndex <= 1024) {
				tmpContact.tgNumber = atoi(digits);

				codeplugContactSaveDataForIndex(contactListContactIndex, &tmpContact);
				contactListContactIndex = 0;
				menuSystemPushNewMenu(MENU_SAVED_SCREEN);
			}
			return;
		}
		else if (KEYCHECK_SHORTUP(keys,KEY_RED))
		{
			contactListContactIndex = 0;
			menuSystemPopPreviousMenu();
			return;
		}
		else if (KEYCHECK_LONGDOWN(keys, KEY_RED))
		{
			contactListContactIndex = 0;
			menuSystemPopAllAndDisplayRootMenu();
			return;
		}
		else if (gMenusCurrentItemIndex == CONTACT_DETAILS_TG) {
			// Add a digit
			if (sLen < 7)
			{
				char c[2] = {0, 0};

				if (KEYCHECK_PRESS(keys,KEY_0))
				{
					c[0]='0';
				}
				else if (KEYCHECK_PRESS(keys,KEY_1))
				{
					c[0]='1';
				}
				else if (KEYCHECK_PRESS(keys,KEY_2))
				{
					c[0]='2';
				}
				else if (KEYCHECK_PRESS(keys,KEY_3))
				{
					c[0]='3';
				}
				else if (KEYCHECK_PRESS(keys,KEY_4))
				{
					c[0]='4';
				}
				else if (KEYCHECK_PRESS(keys,KEY_5))
				{
					c[0]='5';
				}
				else if (KEYCHECK_PRESS(keys,KEY_6))
				{
					c[0]='6';
				}
				else if (KEYCHECK_PRESS(keys,KEY_7))
				{
					c[0]='7';
				}
				else if (KEYCHECK_PRESS(keys,KEY_8))
				{
					c[0]='8';
				}
				else if (KEYCHECK_PRESS(keys,KEY_9))
				{
					c[0]='9';
				}

				if (c[0]!=0)
				{
					strcat(digits,c);
				}
			}
		}
	}
	updateScreen();
}
