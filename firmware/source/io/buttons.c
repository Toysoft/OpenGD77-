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

#include <buttons.h>
#include <pit.h>
#include <settings.h>
#include <usb_com.h>


static uint32_t old_button_state;
volatile bool PTTLocked = false;

#if defined(PLATFORM_GD77S)
static bool orangeButtonPressed;
static bool orangeButtonReleased;
#endif

void fw_init_buttons(void)
{
    PORT_SetPinMux(Port_PTT, Pin_PTT, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_SK1, Pin_SK1, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_SK2, Pin_SK2, kPORT_MuxAsGpio);
#if ! defined(PLATFORM_RD5R)
    PORT_SetPinMux(Port_Orange, Pin_Orange, kPORT_MuxAsGpio);
#if defined(PLATFORM_GD77S)
#endif

#endif

    GPIO_PinInit(GPIO_PTT, Pin_PTT, &pin_config_input);
    GPIO_PinInit(GPIO_SK1, Pin_SK1, &pin_config_input);
    GPIO_PinInit(GPIO_SK2, Pin_SK2, &pin_config_input);
#if ! defined(PLATFORM_RD5R)
    GPIO_PinInit(GPIO_Orange, Pin_Orange, &pin_config_input);

#if defined(PLATFORM_GD77S)
    orangeButtonPressed = false;
    orangeButtonReleased = true;
#endif

#endif

    old_button_state = 0;
}

uint32_t fw_read_buttons(void)
{
	uint32_t result = BUTTON_NONE;

#if ! defined(PLATFORM_RD5R)
	if (GPIO_PinRead(GPIO_Orange, Pin_Orange)==0)
	{
		result |= BUTTON_ORANGE;

#if defined(PLATFORM_GD77S)
		if (orangeButtonReleased && (orangeButtonPressed == false))
		{
			taskENTER_CRITICAL();
			timer_keypad = (nonVolatileSettings.keypadTimerLong * 1000);
			taskEXIT_CRITICAL();

			orangeButtonPressed = true;
			orangeButtonReleased = false;
		}
#endif

	}
#endif

	if (GPIO_PinRead(GPIO_PTT, Pin_PTT)==0)
	{
		result |= BUTTON_PTT;
	}

	if (GPIO_PinRead(GPIO_SK1, Pin_SK1)==0)
	{
		result |= BUTTON_SK1;
	}

	if (GPIO_PinRead(GPIO_SK2, Pin_SK2)==0)
	{
		result |= BUTTON_SK2;
	}


	return result;
}

void fw_check_button_event(uint32_t *buttons, int *event)
{
	*buttons = fw_read_buttons();

#if defined(PLATFORM_GD77S)
	taskENTER_CRITICAL();
	uint32_t tmp_timer_keypad = timer_keypad;
	taskEXIT_CRITICAL();

	if ((*buttons & BUTTON_ORANGE) && orangeButtonPressed && (orangeButtonReleased == false) && (tmp_timer_keypad == 0))
	{
		// Long press
		orangeButtonReleased = true;
		// Set LONG bit
		*buttons |= (BUTTON_ORANGE | BUTTON_ORANGE_LONG);
	}
	else if (((*buttons & BUTTON_ORANGE) == 0) && orangeButtonPressed && (orangeButtonReleased == false) && (tmp_timer_keypad != 0))
	{
		// Short press/release cycle
		orangeButtonPressed = false;
		orangeButtonReleased = true;

		taskENTER_CRITICAL();
		timer_keypad = 0;
		taskEXIT_CRITICAL();

		// Set SHORT press
		*buttons |= BUTTON_ORANGE;
		*buttons &= ~BUTTON_ORANGE_LONG;
	}
	else if (((*buttons & BUTTON_ORANGE) == 0) && orangeButtonPressed && orangeButtonReleased)
	{
		// Button was still down after a long press, now handle release
		orangeButtonPressed = false;
	}
	else
	{
		// Hide Orange state, as short will happen on press/release cycle
		*buttons &= ~(BUTTON_ORANGE | BUTTON_ORANGE_LONG);
	}
#endif

	if (old_button_state != *buttons)
	{
		old_button_state = *buttons;
		*event = EVENT_BUTTON_CHANGE;
	}
	else
	{
		*event = EVENT_BUTTON_NONE;
	}
}
