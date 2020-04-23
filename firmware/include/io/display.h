/*
 * Copyright (C)2019 Kai Ludwig, DG4KLU
 *
 * PWM modifications by Roger Clark VK3KYY / G4KYF
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

#ifndef _FW_DISPLAY_H_
#define _FW_DISPLAY_H_

#include "common.h"
#define DISPLAY_LED_PWM


#if defined(PLATFORM_GD77) || defined(PLATFORM_GD77S)

#define Port_Display_Light	PORTC
#define GPIO_Display_Light	GPIOC
#define Pin_Display_Light	4
#define Port_Display_CS		PORTC
#define GPIO_Display_CS		GPIOC
#define Pin_Display_CS		8
#define Port_Display_RST	PORTC
#define GPIO_Display_RST	GPIOC
#define Pin_Display_RST		9
#define Port_Display_RS		PORTC
#define GPIO_Display_RS		GPIOC
#define Pin_Display_RS		10
#define Port_Display_SCK	PORTC
#define GPIO_Display_SCK 	GPIOC
#define Pin_Display_SCK		11
#define Port_Display_SDA    PORTC
#define GPIO_Display_SDA 	GPIOC
#define Pin_Display_SDA		12

#define BOARD_FTM_BASEADDR FTM0
#define BOARD_FTM_CHANNEL kFTM_Chnl_3

#elif defined(PLATFORM_DM1801)

#define Port_Display_Light	PORTC
#define GPIO_Display_Light	GPIOC
#define Pin_Display_Light	4
#define Port_Display_CS		PORTC
#define GPIO_Display_CS		GPIOC
#define Pin_Display_CS		8
#define Port_Display_RST	PORTC
#define GPIO_Display_RST	GPIOC
#define Pin_Display_RST		9
#define Port_Display_RS		PORTC
#define GPIO_Display_RS		GPIOC
#define Pin_Display_RS		10
#define Port_Display_SCK	PORTC
#define GPIO_Display_SCK 	GPIOC
#define Pin_Display_SCK		11
#define Port_Display_SDA    PORTC
#define GPIO_Display_SDA 	GPIOC
#define Pin_Display_SDA		12

#define BOARD_FTM_BASEADDR FTM0
#define BOARD_FTM_CHANNEL kFTM_Chnl_3

#elif defined(PLATFORM_RD5R)

#define Port_Display_Light	PORTC
#define GPIO_Display_Light	GPIOC
#define Pin_Display_Light	5
#define Port_Display_CS		PORTC
#define GPIO_Display_CS		GPIOC
#define Pin_Display_CS		11
#define Port_Display_RST	PORTC
#define GPIO_Display_RST	GPIOC
#define Pin_Display_RST		9
#define Port_Display_RS		PORTC
#define GPIO_Display_RS		GPIOC
#define Pin_Display_RS		12
#define Port_Display_SCK	PORTC
#define GPIO_Display_SCK 	GPIOC
#define Pin_Display_SCK		8
#define Port_Display_SDA    PORTC
#define GPIO_Display_SDA 	GPIOC
#define Pin_Display_SDA		10

#define BOARD_FTM_BASEADDR FTM0
#define BOARD_FTM_CHANNEL kFTM_Chnl_2

#endif


void displayInit(bool isInverseColour);
void displayEnableBacklight(bool enable);
bool displayIsBacklightLit(void);
void displaySetBacklightIntensityPercentage(uint8_t intensityPercentage);

#endif /* _FW_DISPLAY_H_ */
