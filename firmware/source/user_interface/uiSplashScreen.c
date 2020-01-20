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
#include "fw_settings.h"

static void updateScreen(void);
static void handleEvent(uiEvent_t *ev);


int menuSplashScreen(uiEvent_t *ev, bool isFirstRun)
{
	uint8_t melodyBuf[512];
	if (isFirstRun)
	{
		if (codeplugGetOpenGD77CustomData(CODEPLUG_CUSTOM_DATA_TYPE_BEEP, melodyBuf))
		{
		   create_song(melodyBuf);
		   set_melody(melody_generic);
		}
		else
		{
			set_melody(melody_poweron);
		}
		updateScreen();
	}
	else
	{
		handleEvent(ev);
	}
	return 0;
}

static void updateScreen(void)
{
	char line1[16];
	char line2[16];
	uint8_t bootScreenType;
	uint8_t bootScreenPasswordEnabled;
	uint32_t bootScreenPassword;
	bool customDataHasImage=false;

	codeplugGetBootScreenData(line1,line2,&bootScreenType,&bootScreenPasswordEnabled,&bootScreenPassword);

	strcpy(talkAliasText,line1);
	strcat(talkAliasText,line2);


	if (bootScreenType==0)
	{
		customDataHasImage = codeplugGetOpenGD77CustomData(CODEPLUG_CUSTOM_DATA_TYPE_IMAGE,ucGetDisplayBuffer() );
	}

	if (!customDataHasImage)
	{
		ucClearBuf();
		ucPrintCentered(10, "OpenGD77", FONT_8x16);
		ucPrintCentered(28, line1, FONT_8x16);
		ucPrintCentered(42, line2, FONT_8x16);
	}

	ucRender();
	displayLightTrigger();
}

static void handleEvent(uiEvent_t *ev)
{
	if (melody_play==NULL)
	{
		ucClearBuf();
		ucRender();
		menuSystemSetCurrentMenu(nonVolatileSettings.initialMenuNumber);
	}
}
