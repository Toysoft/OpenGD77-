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
#include <user_interface/uiUtilities.h>
#include <settings.h>
#include <ticks.h>

static void updateScreen(void);
static void handleEvent(uiEvent_t *ev);

int uiPrivateCallState = NOT_IN_CALL;
int uiPrivateCallLastID;

int menuPrivateCall(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		set_melody(melody_private_call);
		uiPrivateCallState = PRIVATE_CALL_ACCEPT;
		menuUtilityReceivedPcId = LinkHead->id;

		displayLightTrigger();
		updateScreen();
	}
	else
	{
		if (ev->hasEvent)
		{
			handleEvent(ev);
		}
	}
	return 0;
}

static void updateScreen(void)
{
	static const int bufferLen = 33; // displayChannelNameOrRxFrequency() use 6x8 font
	char buffer[bufferLen];// buffer passed to the DMR ID lookup function, needs to be large enough to hold worst case text length that is returned. Currently 16+1
	dmrIdDataStruct_t currentRec;

	ucClearBuf();
	if (!contactIDLookup(menuUtilityReceivedPcId, CONTACT_CALLTYPE_PC, buffer))
	{
		dmrIDLookup(menuUtilityReceivedPcId, &currentRec);
		strncpy(buffer, currentRec.text, 16);
		buffer[16] = 0;
	}
	ucPrintCentered(32, buffer, FONT_SIZE_3);

	ucPrintCentered(16, currentLanguage->accept_call, FONT_SIZE_3);
	ucDrawChoice(CHOICE_YESNO, false);
	ucRender();

}

static void handleEvent(uiEvent_t *ev)
{
	displayLightTrigger();

	if (ev->events & KEY_EVENT)
	{
		if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
		{
			uiPrivateCallState = PRIVATE_CALL_DECLINED;
			uiPrivateCallLastID = 0;
			menuSystemPopPreviousMenu();
			return;
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
		{
			menuAcceptPrivateCall(menuUtilityReceivedPcId);
			menuSystemPopPreviousMenu();
			return;
		}
	}
}

void menuClearPrivateCall(void )
{
	uiPrivateCallState = NOT_IN_CALL;
	uiPrivateCallLastID = 0;
	menuUtilityTgBeforePcMode = 0;
	menuUtilityReceivedPcId = 0;
}

void menuAcceptPrivateCall(int id )
{
	uiPrivateCallState = PRIVATE_CALL;
	uiPrivateCallLastID = (id & 0xffffff);
	settingsPrivateCallMuteMode=false;
	menuUtilityReceivedPcId = 0;

	setOverrideTGorPC(uiPrivateCallLastID, true);

}

