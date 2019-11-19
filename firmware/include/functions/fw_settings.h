/*
 * Copyright (C)2019 	Roger Clark, VK3KYY / G4KYF
 * 				and		Kai Ludwig, DG4KLU
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

#ifndef _FW_SETTINGS_H_
#define _FW_SETTINGS_H_

#include "fw_common.h"
#include "fw_codeplug.h"

extern const int BAND_VHF_MIN;
extern const int BAND_VHF_MAX;
extern const int BAND_UHF_MIN;
extern const int BAND_UHF_MAX;
#define VFO_COUNT 4
enum USB_MODE { USB_MODE_CPS, USB_MODE_HOTSPOT, USB_MODE_DEBUG};
extern int settingsCurrentChannelNumber;
extern bool settingsPrivateCallMuteMode;

typedef struct settingsStruct
{
	int 			magicNumber;
	int16_t			currentChannelIndexInZone;
	int16_t			currentChannelIndexInAllZone;
	int16_t			currentIndexInTRxGroupList;
	int16_t			currentZone;
	uint8_t			backLightTimeout;//0 = never timeout. 1 - 255 time in seconds
	int8_t			displayContrast;
	uint8_t			initialMenuNumber;
	int8_t			displayBacklightPercentage;
	bool			displayInverseVideo;
	bool			useCalibration;
	bool			txFreqLimited;
	uint16_t		txPowerLevel;
	struct_codeplugChannel_t vfoChannel;
	uint32_t		overrideTG;
	uint8_t			txTimeoutBeepX5Secs;
	uint8_t			beepVolumeDivider;
	uint8_t			micGainDMR;
	uint8_t			tsManualOverride;
} settingsStruct_t;

extern settingsStruct_t nonVolatileSettings;
extern struct_codeplugChannel_t *currentChannelData;
extern struct_codeplugChannel_t channelScreenChannelData;
extern int settingsUsbMode;
extern bool enableHotspot;

bool settingsSaveSettings();
bool settingsLoadSettings();
void settingsRestoreDefaultSettings();
void settingsInitVFOChannel();

#endif
