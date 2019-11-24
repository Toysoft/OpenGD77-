/*
 * Copyright (C)2019 	Kai Ludwig, DG4KLU
 * 				and		Roger Clark, VK3KYY / G4KYF
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

#include <hardware/fw_EEPROM.h>
#include <user_interface/menuSystem.h>
#include "fw_settings.h"
#include "fw_trx.h"
#include "fw_codeplug.h"
#include "fw_sound.h"

const int BAND_VHF_MIN 	= 14400000;
const int BAND_VHF_MAX 	= 14800000;
const int BAND_UHF_MIN 	= 43000000;
const int BAND_UHF_MAX 	= 45000000;

static const int STORAGE_BASE_ADDRESS 		= 0x6000;
static const int STORAGE_MAGIC_NUMBER 		= 0x4726;

settingsStruct_t nonVolatileSettings;
struct_codeplugChannel_t *currentChannelData;
struct_codeplugChannel_t channelScreenChannelData={.rxFreq=0};
struct_codeplugContact_t contactListContactData;
struct_codeplugChannel_t settingsVFOChannel[2];// VFO A and VFO B from the codeplug.
int contactListContactIndex;
int settingsUsbMode = USB_MODE_CPS;
int settingsCurrentChannelNumber=0;
bool settingsPrivateCallMuteMode = false;
bool enableHotspot = false;
dmrFilter_t settingsDmrFilterLevel = DMR_FILTER_CC_TS;

bool settingsSaveSettings(bool includeVFOs)
{
	if (includeVFOs)
	{
		codeplugSetVFO_ChannelData(&settingsVFOChannel[0],0);
		codeplugSetVFO_ChannelData(&settingsVFOChannel[1],1);
	}
	return EEPROM_Write(STORAGE_BASE_ADDRESS, (uint8_t*)&nonVolatileSettings, sizeof(settingsStruct_t));
}

bool settingsLoadSettings(void)
{
	bool readOK = EEPROM_Read(STORAGE_BASE_ADDRESS, (uint8_t*)&nonVolatileSettings, sizeof(settingsStruct_t));
	if (nonVolatileSettings.magicNumber != STORAGE_MAGIC_NUMBER || readOK != true)
	{
		settingsRestoreDefaultSettings();
	}

	codeplugGetVFO_ChannelData(&settingsVFOChannel[0],0);
	codeplugGetVFO_ChannelData(&settingsVFOChannel[1],1);
	settingsInitVFOChannel(0);// clean up any problems with VFO data
	settingsInitVFOChannel(1);

	trxDMRID = codeplugGetUserDMRID();

	// Added this parameter without changing the magic number, so need to check for default / invalid numbers
	if (nonVolatileSettings.txTimeoutBeepX5Secs > 4)
	{
		nonVolatileSettings.txTimeoutBeepX5Secs=0;
	}

	// Added this parameter without changing the magic number, so need to check for default / invalid numbers
	if (nonVolatileSettings.beepVolumeDivider>10)
	{
		nonVolatileSettings.beepVolumeDivider = 2;// no reduction in volume
	}
	soundBeepVolumeDivider = nonVolatileSettings.beepVolumeDivider;

	return readOK;
}

void settingsInitVFOChannel(int vfoNumber)
{

	// temporary hack in case the code plug has no RxGroup selected
	// The TG needs to come from the RxGroupList
	if (settingsVFOChannel[vfoNumber].rxGroupList == 0)
	{
		settingsVFOChannel[vfoNumber].rxGroupList=1;
	}

	if (settingsVFOChannel[vfoNumber].contact == 0)
	{
		settingsVFOChannel[vfoNumber].contact=1;
	}

/*
	if (settingsVFOChannel[vfoNumber].chMode == RADIO_MODE_ANALOG)
	{
		// In Analog mode, some crucial DMR settings will be invalid.
		// So we need to set them to usable defaults
		settingsVFOChannel[vfoNumber].rxGroupList=1;
		settingsVFOChannel[vfoNumber].rxColor = 1;
		nonVolatileSettings.overrideTG = 9;// Set the override TG to local TG 9
		trxTalkGroupOrPcId = nonVolatileSettings.overrideTG;
	}

	if (!trxCheckFrequencyInAmateurBand(settingsVFOChannel[vfoNumber].rxFreq))
	{
		settingsVFOChannel[vfoNumber].rxFreq = BAND_UHF_MIN;
	}

	if (!trxCheckFrequencyInAmateurBand(settingsVFOChannel[vfoNumber].txFreq))
	{
		settingsVFOChannel[vfoNumber].txFreq = BAND_UHF_MIN;
	}
*/
}

void settingsRestoreDefaultSettings(void)
{
	nonVolatileSettings.magicNumber=STORAGE_MAGIC_NUMBER;
	nonVolatileSettings.currentChannelIndexInZone = 0;
	nonVolatileSettings.currentChannelIndexInAllZone = 1;
	nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_CHANNEL_MODE]=0;
	nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_A_MODE]=0;
	nonVolatileSettings.currentIndexInTRxGroupList[SETTINGS_VFO_B_MODE]=0;
	nonVolatileSettings.currentZone = 0;
	nonVolatileSettings.backLightTimeout = 0;//0 = never timeout. 1 - 255 time in seconds
	nonVolatileSettings.displayContrast = 0x12;
	nonVolatileSettings.initialMenuNumber=MENU_VFO_MODE;
	nonVolatileSettings.displayBacklightPercentage=100U;// 100% brightness
	nonVolatileSettings.displayInverseVideo=false;// Not inverse video
	nonVolatileSettings.useCalibration = true;// enable the new calibration system
	nonVolatileSettings.txFreqLimited = true;// Limit Tx frequency to US Amateur bands
	nonVolatileSettings.txPowerLevel=3;// 1 WW
	nonVolatileSettings.overrideTG=0;// 0 = No override
	nonVolatileSettings.txTimeoutBeepX5Secs = 0;
	nonVolatileSettings.beepVolumeDivider = 1;// no reduction in volume
	nonVolatileSettings.micGainDMR = 11;// Normal value used by the official firmware
	nonVolatileSettings.tsManualOverride = 0; // No manual TS override using the Star key
	nonVolatileSettings.keypadTimerLong = 3;
	nonVolatileSettings.keypadTimerRepeat = 5;
	nonVolatileSettings.currentVFONumber = 0;

	currentChannelData = &settingsVFOChannel[nonVolatileSettings.currentVFONumber];// Set the current channel data to point to the VFO data since the default screen will be the VFO

	settingsSaveSettings(false);
}
