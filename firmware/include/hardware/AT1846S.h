/*
 * Copyright (C)2019 Kai Ludwig, DG4KLU
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

#ifndef _FW_AT1846S_H_
#define _FW_AT1846S_H_

#include "FreeRTOS.h"
#include "task.h"
#include "i2c.h"

#define AT1846_BYTES_PER_COMMAND 3
#define BANDWIDTH_12P5KHZ false
#define BANDWIDTH_25KHZ true

#define AT1846_VOICE_CHANNEL_NONE   0x00
#define AT1846_VOICE_CHANNEL_TONE1  0x10
#define AT1846_VOICE_CHANNEL_TONE2  0x20
#define AT1846_VOICE_CHANNEL_DTMF   0x30
#define AT1846_VOICE_CHANNEL_MIC    0x40

void I2C_AT1846S_init(void);
void I2C_AT1846_Postinit(void);
void I2C_AT1846_SetBandwidth(void);
void I2C_AT1846_SetMode(void);
void setMicGainFM(uint8_t gain);


#endif /* _FW_AT1846S_H_ */
