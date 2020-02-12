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
#include "fw_trx.h"

#define VFO_COUNT 4
enum USB_MODE { USB_MODE_CPS, USB_MODE_HOTSPOT, USB_MODE_DEBUG};
enum SETTINGS_UI_MODE { SETTINGS_CHANNEL_MODE=0, SETTINGS_VFO_A_MODE, SETTINGS_VFO_B_MODE};
enum BACKLIGHT_MODE { BACKLIGHT_MODE_AUTO = 0, BACKLIGHT_MODE_MANUAL = 1, BACKLIGHT_MODE_NONE = 2};
enum HOTSPOT_TYPE { HOTSPOT_TYPE_OFF = 0, HOTSPOT_TYPE_MMDVM = 1, HOTSPOT_TYPE_BLUEDV = 2};
enum CONTACT_DISPLAY_PRIO { CONTACT_DISPLAY_PRIO_CC_DB_TA = 0, CONTACT_DISPLAY_PRIO_DB_CC_TA, CONTACT_DISPLAY_PRIO_TA_CC_DB, CONTACT_DISPLAY_PRIO_TA_DB_CC };
enum SCAN_MODE { SCAN_MODE_HOLD = 0, SCAN_MODE_PAUSE, SCAN_MODE_STOP };
enum SPLIT_CONTACT { SPLIT_CONTACT_SINGLE_LINE_ONLY = 0, SPLIT_CONTACT_ON_TWO_LINES, SPLIT_CONTACT_AUTO };


extern int settingsCurrentChannelNumber;
extern bool settingsPrivateCallMuteMode;
extern struct_codeplugChannel_t settingsVFOChannel[2];

typedef struct settingsStruct
{
	int 			magicNumber;
	int16_t			currentChannelIndexInZone;
	int16_t			currentChannelIndexInAllZone;
	int16_t			currentIndexInTRxGroupList[3];// Current Channel, VFO A and VFO B
	int16_t			currentZone;
	uint8_t			backlightMode; // see BACKLIGHT_MODE enum
	uint8_t			backLightTimeout;//0 = never timeout. 1 - 255 time in seconds
	int8_t			displayContrast;
	uint8_t			initialMenuNumber;
	int8_t			displayBacklightPercentage;
	int8_t			displayBacklightPercentageOff; // backlight level when "off"
	bool			displayInverseVideo;
	bool			useCalibration;
	bool			txFreqLimited;
	bool			pttToggle;
	uint8_t			scanModePause;
	bool			transmitTalkerAlias;
	uint16_t		txPowerLevel;
	uint32_t		overrideTG;
	uint8_t			txTimeoutBeepX5Secs;
	uint8_t			beepVolumeDivider;
	uint8_t			micGainDMR;
	uint8_t			tsManualOverride;
	uint16_t		keypadTimerLong;
	uint16_t		keypadTimerRepeat;
	uint8_t			currentVFONumber;
	uint8_t			dmrFilterLevel;
	uint8_t			dmrCaptureTimeout;
	uint8_t			languageIndex;
	uint8_t			scanDelay;
	uint8_t			squelchDefaults[RADIO_BANDS_TOTAL_NUM];// VHF,200Mhz and UHF
	uint8_t			hotspotType;
    bool     		privateCalls;
	uint32_t		vfoAScanLow;                  //low frequency for VFO A Scanning
	uint32_t		vfoAScanHigh;                 //High frequency for VFO A Scanning
	uint32_t		vfoBScanLow;                  //low frequency for VFO B Scanning
	uint32_t		vfoBScanHigh;                 //High frequency for VFO B Scanning
	uint8_t			contactDisplayPriority;
	uint8_t			splitContact;

} settingsStruct_t;

typedef enum DMR_FILTER_TYPE {DMR_FILTER_NONE = 0, DMR_FILTER_TS, DMR_FILTER_TS_TG, DMR_FILTER_TS_DC ,
								NUM_DMR_FILTER_LEVELS} dmrFilter_t;

extern settingsStruct_t nonVolatileSettings;
extern struct_codeplugChannel_t *currentChannelData;
extern struct_codeplugChannel_t channelScreenChannelData;
extern struct_codeplugContact_t contactListContactData;
extern int contactListContactIndex;
extern int settingsUsbMode;

bool settingsSaveSettings(bool includeVFOs);
bool settingsLoadSettings(void);
void settingsRestoreDefaultSettings(void);
void settingsInitVFOChannel(int vfoNumber);

#endif
