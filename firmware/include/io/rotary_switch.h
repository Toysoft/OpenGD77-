/*
 * Copyright (C)2020	Kai Ludwig, DG4KLU
 *               and	Roger Clark, VK3KYY / G4KYF
 *               and    Daniel Caujolle-Bert, F1RMB
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
#ifndef _ROTARY_SWITCH_H_
#define _ROTARY_SWITCH_H_

#include <common.h>


#if defined(PLATFORM_GD77S)

#define Port_RotarySW_Line0   PORTD
#define GPIO_RotarySW_Line0 	GPIOD
#define Pin_RotarySW_Line0	4
#define Port_RotarySW_Line1   PORTD
#define GPIO_RotarySW_Line1 	GPIOD
#define Pin_RotarySW_Line1 	5
#define Port_RotarySW_Line2   PORTD
#define GPIO_RotarySW_Line2 	GPIOD
#define Pin_RotarySW_Line2 	6
#define Port_RotarySW_Line3   PORTD
#define GPIO_RotarySW_Line3 	GPIOD
#define Pin_RotarySW_Line3 	7

#endif // PLATFORM_GD77S


void init_rotary_switch(void);
uint8_t get_rotary_switch_position(void);
void check_rotary_switch_event(uint32_t *position, int *event);

#define EVENT_ROTARY_NONE   0
#define EVENT_ROTARY_CHANGE 1


#endif // _ROTARY_SWITCH_H_
