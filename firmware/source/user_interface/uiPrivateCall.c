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
#include <user_interface/uiUtilityQSOData.h>
#include <functions/fw_ticks.h>
#include "fw_settings.h"

static void updateScreen(void);
static void handleEvent(uiEvent_t *ev);
void menuAcceptCall(void);

int uiPrivateCallState = NOT_IN_CALL;
int uiPrivateCallLastID;

int menuPrivateCall(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		uiPrivateCallState = ACCEPT_PRIVATE_CALL;
		menuUtilityReceivedPcId = LinkHead->id | (PC_CALL_FLAG<<24);

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
	if (!contactIDLookup(LinkHead->id, CONTACT_CALLTYPE_PC, buffer))
	{
		dmrIDLookup( (LinkHead->id & 0xFFFFFF),&currentRec);
		strncpy(buffer, currentRec.text, 16);
		buffer[16] = 0;
	}
	ucPrintCentered(16, buffer, FONT_8x16);

	ucPrintCentered(32, currentLanguage->accept_call, FONT_8x16);
	ucDrawChoice(CHOICE_YESNO, false);
	set_melody(melody_private_call);
	ucRender();

}

static void handleEvent(uiEvent_t *ev)
{
	if (ev->events & KEY_EVENT)
	{
		if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
		{
			uiPrivateCallState = PRIVATE_CALL_DECLINED;
			uiPrivateCallLastID = LinkHead->id;
			menuSystemPopPreviousMenu();
			return;
		}
		else if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
		{
			menuAcceptCall();
			menuSystemPopPreviousMenu();
			return;
		}
	}

	displayLightTrigger();
}

void menuClearPrivateCall(void )
{
	uiPrivateCallState = NOT_IN_CALL;
	uiPrivateCallLastID = 0;
	menuUtilityTgBeforePcMode = 0;
	menuUtilityReceivedPcId = 0;
}

void menuAcceptCall(void )
{
	uiPrivateCallState = MY_CALL;
	uiPrivateCallLastID = LinkHead->id;

	menuUtilityTgBeforePcMode = nonVolatileSettings.overrideTG;// save the current TG
	nonVolatileSettings.overrideTG =  menuUtilityReceivedPcId;
	trxTalkGroupOrPcId = menuUtilityReceivedPcId;
	settingsPrivateCallMuteMode=false;
}

