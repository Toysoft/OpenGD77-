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
#include "menu/menuSystem.h"
#include "fw_i2c.h"


static void updateScreen();
static void handleEvent(int buttons, int keys, int events);
static void sampleRSSIAndNoise();

static const int NUM_SAMPLES = 256;
static int sampleCount;
static int RSSI_totalVal;

int menuRSSIScreen(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		sampleCount=0;
		RSSI_totalVal=0;
	}
	else
	{
		if (events!=0 && keys!=0)
		{
			handleEvent(buttons, keys, events);
		}

		sampleCount++;

		if (sampleCount < NUM_SAMPLES)
		{
			sampleRSSIAndNoise();
		}
		else
		{
			updateScreen();
			sampleCount=0;
			RSSI_totalVal=0;
		}
	}
	return 0;
}


static void updateScreen()
{
	int dBm;
	int barGraphLength;
	char buffer[17];

		RSSI_totalVal /= NUM_SAMPLES;

		if (trxCheckFrequencyIsVHF(trxGetFrequency()))
		{
			// VHF
			// Use fixed point maths to scale the RSSI value to dBm, based on data from VK4JWT and VK7ZJA
			dBm = -164 + ((RSSI_totalVal * 32) / 27);
		}
		else
		{
			// Use fixed point maths to scale the RSSI value to dBm, based on data from VK4JWT and VK7ZJA
			dBm = -151 + RSSI_totalVal;// Note no the RSSI value on UHF does not need to be scaled like it does on VHF
		}

		UC1701_clearBuf();
		UC1701_printCentered(0, "RSSI",UC1701_FONT_GD77_8x16);

		sprintf(buffer,"%d", RSSI_totalVal);
		UC1701_printCore(0,0,buffer,UC1701_FONT_GD77_8x16,2,false);

		sprintf(buffer,"%ddBm", dBm);
		UC1701_printCentered(20, buffer,UC1701_FONT_GD77_8x16);

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

		UC1701_printCore(5,50,"S1  S3  S5  S7  S9",UC1701_FONT_6X8,0,false);
		UC1701_render();
		displayLightTrigger();

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

static void sampleRSSIAndNoise()
{
    uint8_t RX_signal;
    uint8_t RX_noise;

    read_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x1b, &RX_signal, &RX_noise);
    RSSI_totalVal += RX_signal;
}
