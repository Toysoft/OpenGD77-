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
#include <hardware/fw_HR-C6000.h>
#include <user_interface/menuSystem.h>
#include <user_interface/menuUtilityQSOData.h>
#include "fw_settings.h"
#include "fw_codeplug.h"

static char digits[9];
static int pcIdx;
static struct_codeplugContact_t contact;
static void updateScreen();
static void handleEvent(int buttons, int keys, int events);

static const char *menuName[] = { "TG entry", "PC entry", "Contact", "User DMR ID" };

// public interface
int menuNumericalEntry(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		gMenusCurrentItemIndex=0;
		digits[0]=0x00;
		pcIdx = 0;
		updateScreen();
	}
	else
	{
		if (events==0)
		{
			//updateScreen();
		}
		else
		{
			handleEvent(buttons, keys, events);
		}
	}
	return 0;
}


static void updateScreen()
{
	char buf[17];

	UC1701_clearBuf();

	UC1701_printCentered(8, (char *)menuName[gMenusCurrentItemIndex],UC1701_FONT_GD77_8x16);

	if (pcIdx == 0)
	{
		UC1701_printCentered(32, (char *)digits,UC1701_FONT_GD77_8x16);
	}
	else
	{
		codeplugUtilConvertBufToString(contact.name, buf, 16);
		UC1701_printCentered(32, buf, UC1701_FONT_GD77_8x16);
		UC1701_printCentered(52, (char *)digits,UC1701_FONT_6X8);
	}
	displayLightTrigger();

	UC1701_render();
}

static int getNextContact(int curidx, int dir, struct_codeplugContact_t *contact)
{
	int idx = curidx;

	do {
		idx += dir;
		if (idx >= 1024) {
			if (curidx == 0) {
				idx = 0;
				break;
			}
			idx = 1;
		} else if (idx ==0) {
			if (curidx == 0) {
				idx = 0;
				break;
			}
			idx = 1024;
		}
		codeplugContactGetDataForIndex(idx, contact);
	} while ((curidx != idx) && ((*contact).tgNumber == 0));

	return idx;
}

static void handleEvent(int buttons, int keys, int events)
{
	if ((keys & KEY_RED)!=0)
	{
		menuSystemPopPreviousMenu();
		return;
	}
	else if ((keys & KEY_GREEN)!=0)
	{
		if (gMenusCurrentItemIndex!=3)
		{
			uint32_t saveTrxTalkGroupOrPcId = trxTalkGroupOrPcId;
			trxTalkGroupOrPcId = atoi(digits);
			nonVolatileSettings.overrideTG = trxTalkGroupOrPcId;
			if (gMenusCurrentItemIndex == 1 || (pcIdx != 0 && contact.callType == 0x01))
			{
				// Private Call

				if ((saveTrxTalkGroupOrPcId >> 24) != PC_CALL_FLAG)
				{
					// if the current Tx TG is a TalkGroup then save it so it can be stored after the end of the private call
					menuUtilityTgBeforePcMode = saveTrxTalkGroupOrPcId;
				}
				nonVolatileSettings.overrideTG |= (PC_CALL_FLAG << 24);
			}
		}
		else
		{
			trxDMRID = atoi(digits);
			if (buttons & BUTTON_SK2)
			{
				// make the change to DMR ID permanent if Function + Green is pressed
				codeplugSetUserDMRID(trxDMRID);
			}
		}
		menuSystemPopAllAndDisplayRootMenu();
	}
	else if ((keys & KEY_HASH)!=0)
	{
		pcIdx = 0;
		if ((buttons & BUTTON_SK2)!= 0  && gMenusCurrentItemIndex == 2)
		{
			gMenusCurrentItemIndex = 3;
		}
		else
		{
			gMenusCurrentItemIndex++;
			if (gMenusCurrentItemIndex > 2)
			{
				gMenusCurrentItemIndex = 0;
			} else if (gMenusCurrentItemIndex == 2)
			{
				pcIdx = getNextContact(0, 1, &contact);
				if (pcIdx != 0 ) {
					itoa(contact.tgNumber, digits, 10);
				}
			}
		}

		updateScreen();
	}
	if (gMenusCurrentItemIndex == 2) {
		int idx = pcIdx;

		if ((keys & KEY_RIGHT) != 0) {
			idx = getNextContact(pcIdx, 1, &contact);
		} else if ((keys & KEY_LEFT) != 0) {
			idx = getNextContact(pcIdx, -1, &contact);
		}
		if (pcIdx != idx ) {
			pcIdx = idx;
			itoa(contact.tgNumber, digits, 10);
			updateScreen();
		}
	}
	else if (strlen(digits) < 7) {
		char c[2] = {0, 0};
		if ((keys & KEY_0) != 0)
		{
			c[0]='0';
		}
		else if ((keys & KEY_1)!=0)
		{
			c[0]='1';
		}
		else if ((keys & KEY_2)!=0)
		{
			c[0]='2';
		}
		else if ((keys & KEY_3)!=0)
		{
			c[0]='3';
		}
		else if ((keys & KEY_4)!=0)
		{
			c[0]='4';
		}
		else if ((keys & KEY_5)!=0)
		{
			c[0]='5';
		}
		else if ((keys & KEY_6)!=0)
		{
			c[0]='6';
		}
		else if ((keys & KEY_7)!=0)
		{
			c[0]='7';
		}
		else if ((keys & KEY_8)!=0)
		{
			c[0]='8';
		}
		else if ((keys & KEY_9)!=0)
		{
			c[0]='9';
		}
		else if ((keys & KEY_LEFT)!=0 && strlen(digits)>0)
		{
			digits[strlen(digits)-1]=0;
			updateScreen();
		}
		if (c[0]!=0)
		{
			strcat(digits,c);
			updateScreen();
		}
	}
}
