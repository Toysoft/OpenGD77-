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

#ifndef _FW_LEDS_H_
#define _FW_LEDS_H_

#include "common.h"

#if defined(PLATFORM_GD77) || defined(PLATFORM_GD77S)

#define Port_LEDgreen	PORTB
#define GPIO_LEDgreen	GPIOB
#define Pin_LEDgreen	18

#define Port_LEDred		PORTC
#define GPIO_LEDred		GPIOC
#define Pin_LEDred		14

#elif defined(PLATFORM_DM1801)

#define Port_LEDgreen	PORTA
#define GPIO_LEDgreen	GPIOA
#define Pin_LEDgreen	17

#define Port_LEDred		PORTC
#define GPIO_LEDred		GPIOC
#define Pin_LEDred		14

#elif defined(PLATFORM_RD5R)

#define Port_LEDgreen	PORTB
#define GPIO_LEDgreen	GPIOB
#define Pin_LEDgreen	18

#define Port_LEDred		PORTB
#define GPIO_LEDred		GPIOB
#define Pin_LEDred		0

#endif

void toggle_torch();
void fw_init_LEDs(void);

#endif /* _FW_LEDS_H_ */
