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

enum CONTACT_DETAILS_DISPLAY_LIST { CONTACT_DETAILS_HEADER=0, CONTACT_DETAILS_NAME, CONTACT_DETAILS_TG, CONTACT_DETAILS_CALLTYPE, CONTACT_DETAILS_TS,
	NUM_CONTACT_DETAILS_ITEMS};// The last item in the list is used so that we automatically get a total number of items in the list

int menuContactDetails(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		memcpy(&tmpContact, &contactListContactData,sizeof(struct_codeplugContact_t));

		gMenusCurrentItemIndex=1;
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
	menuDisplayTitle("Contact details");

	// Can only display 3 of the options at a time menu at -1, 0 and +1
	for(int i = -1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_CONTACT_DETAILS_ITEMS, i);

		switch(mNum)
		{
			case CONTACT_DETAILS_HEADER:
				strcpy(buf,"Name:");
				break;
			case CONTACT_DETAILS_NAME:
				codeplugUtilConvertBufToString(tmpContact.name, buf, 16);
				break;
			case CONTACT_DETAILS_TG:
				switch (tmpContact.callType) {
					case 2: // All Call
						strcpy(buf,"All:16777215");
						break;
					case 1: // Private
						sprintf(buf,"PC:%d",tmpContact.tgNumber);
						break;
					case 0:
						sprintf(buf,"TG:%d",tmpContact.tgNumber);
						break;
				}
				break;
			case CONTACT_DETAILS_CALLTYPE:
				strcpy(buf,"Type:");
				strcat(buf,callTypeString[tmpContact.callType]);
				break;
			case CONTACT_DETAILS_TS:
				switch (tmpContact.reserve1 & 0x3) {
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
	if (events & 0x01) {
		if ((keys & KEY_DOWN)!=0)
		{
			MENU_INC(gMenusCurrentItemIndex, NUM_CONTACT_DETAILS_ITEMS);
		}
		else if ((keys & KEY_UP)!=0)
		{
			MENU_DEC(gMenusCurrentItemIndex, NUM_CONTACT_DETAILS_ITEMS);
		}
		else if ((keys & KEY_RIGHT)!=0)
		{
			switch(gMenusCurrentItemIndex)
			{
			case CONTACT_DETAILS_NAME:
				break;
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
		else if ((keys & KEY_LEFT)!=0)
		{
			switch(gMenusCurrentItemIndex)
			{
			case CONTACT_DETAILS_NAME:
				break;
			case CONTACT_DETAILS_TG:
				tmpContact.tgNumber = tmpContact.tgNumber / 10;
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
		else if ((keys & KEY_GREEN)!=0)
		{
			menuSystemPopAllAndDisplayRootMenu();
			return;
		}
		else if ((keys & KEY_RED)!=0)
		{
			menuSystemPopPreviousMenu();
			return;
		}
	}
	updateScreen();
}
