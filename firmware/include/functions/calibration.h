/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
 * 				and  Kai Ludwig, DG4KLU
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
#ifndef _FW_CALIBRATION_H_
#define _FW_CALIBRATION_H_

#include "common.h"
#include "SPI_Flash.h"


typedef struct calibrationPowerValues
{
	uint32_t lowPower;
	uint32_t highPower;
} calibrationPowerValues_t;

typedef struct calibrationRSSIMeter
{
	uint8_t minVal;
	uint8_t rangeVal;
} calibrationRSSIMeter_t;

typedef struct deviationToneStruct
{
	uint8_t dtmf;
	uint8_t tone;
	uint8_t ctcss_wide;
	uint8_t ctcss_narrow;
	uint8_t dcs_wide;
	uint8_t dcs_narrow;
} deviationToneStruct_t;


void read_val_DACDATA_shift(int offset, uint8_t* val_shift);
void read_val_twopoint_mod(int offset, uint8_t* val_0x47, uint8_t* val_0x48);
void read_val_Q_MOD2_offset(int offset, uint8_t* val_0x04);
void read_val_phase_reduce(int offset, uint8_t* val_0x46);

void read_val_pga_gain(int offset, uint8_t* value);
void read_val_voice_gain_tx(int offset, uint8_t* value);
void read_val_gain_tx(int offset, uint8_t* value);
void read_val_padrv_ibit(int offset, uint8_t* value);

void read_val_xmitter_dev_wideband(int offset, uint16_t* value);
void read_val_xmitter_dev_narrowband(int offset, uint16_t* value);
void read_val_dev_tone(int index, uint8_t *value);

void read_val_dac_vgain_analog(int offset, uint8_t* value);
void read_val_volume_analog(int offset, uint8_t* value);

void read_val_noise1_th_wideband(int offset, uint16_t* value);
void read_val_noise2_th_wideband(int offset, uint16_t* value);
void read_val_rssi3_th_wideband(int offset, uint16_t* value);
void read_val_noise1_th_narrowband(int offset, uint16_t* value);
void read_val_noise2_th_narrowband(int offset, uint16_t* value);
void read_val_rssi3_th_narrowband(int offset, uint16_t* value);

void read_val_squelch_th(int offset, int mod, uint16_t* value);
bool calibrationGetPowerForFrequency(int freq, calibrationPowerValues_t *powerSettings);
bool calibrationGetRSSIMeterParams(calibrationRSSIMeter_t *rssiMeterValues);
bool checkAndCopyCalibrationToCommonLocation(void);
#endif
