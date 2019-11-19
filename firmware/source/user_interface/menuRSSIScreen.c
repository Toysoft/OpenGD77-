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
#include <user_interface/menuUtilityQSOData.h>
#include "fw_calibration.h"
#include "fw_settings.h"

static calibrationRSSIMeter_t rssiCalibration;
static void updateScreen();
static void handleEvent(int buttons, int keys, int events);

int menuRSSIScreen(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		RssiUpdateCounter = RSSI_UPDATE_COUNTER_RELOAD;
		calibrationGetRSSIMeterParams(currentChannelData->rxFreq,&rssiCalibration);
	}
	else
	{
		if (events!=0 && keys!=0)
		{
			handleEvent(buttons, keys, events);
		}

		if (RssiUpdateCounter-- == 0)
		{
			updateScreen();
			RssiUpdateCounter = RSSI_UPDATE_COUNTER_RELOAD;
		}
	}
	return 0;
}


static void updateScreen()
{
	int dBm;
	int barGraphLength;
	char buffer[17];

		if (trxCheckFrequencyIsUHF(trxGetFrequency()))
		{
			// Use fixed point maths to scale the RSSI value to dBm, based on data from VK4JWT and VK7ZJA
			dBm = -151 + trxRxSignal;// Note no the RSSI value on UHF does not need to be scaled like it does on VHF
		}
		else
		{
			// VHF
			// Use fixed point maths to scale the RSSI value to dBm, based on data from VK4JWT and VK7ZJA
			dBm = -164 + ((trxRxSignal * 32) / 27);
		}

		UC1701_clearBuf();
		menuDisplayTitle("RSSI");

		sprintf(buffer,"%d", trxRxSignal);
		UC1701_printCore(0, 3, buffer, UC1701_FONT_8x8, UC1701_TEXT_ALIGN_RIGHT, false);

		sprintf(buffer,"%ddBm", dBm);
		UC1701_printCentered(20, buffer,UC1701_FONT_8x16);

		barGraphLength = ((dBm + 130) * 24)/10;
		if (barGraphLength<0)
		{
			barGraphLength=0;
		}

		if (barGraphLength>123)
		{
			barGraphLength=123;
		}
		UC1701_fillRect(4, 40,barGraphLength,8,false);

		UC1701_printCore(5,50,"S1  S3  S5  S7  S9",UC1701_FONT_6x8,UC1701_TEXT_ALIGN_LEFT,false);
		UC1701_render();
		displayLightTrigger();
		trxRxSignal=0;

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
		menuSystemPopAllAndDisplayRootMenu();
		return;
	}
}
