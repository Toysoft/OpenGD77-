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

#include "fw_keyboard.h"
#include "fw_pit.h"
#include "fw_settings.h"
#include "fw_usb_com.h"

static uint32_t old_keyboard_state;
static uint32_t keyDebounceScancode;
static int keyDebounceCounter;
static uint8_t keyState;

enum KEY_STATE { KEY_IDLE=0, KEY_DEBOUNCE, KEY_PRESS, KEY_HOLD, KEY_REPEAT, KEY_WAIT_RELEASED };

volatile bool keypadLocked = false;

static const uint32_t keyMap[] = {
		KEY_1, KEY_2, KEY_3, KEY_GREEN, KEY_RIGHT,
		KEY_4, KEY_5, KEY_6, KEY_UP, KEY_LEFT,
		KEY_7, KEY_8, KEY_9, KEY_DOWN, 0,
		KEY_STAR, KEY_0, KEY_HASH, KEY_RED, 0 };

void fw_init_keyboard(void)
{
    // column lines
    PORT_SetPinMux(Port_Key_Col0, Pin_Key_Col0, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_Key_Col1, Pin_Key_Col1, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_Key_Col2, Pin_Key_Col2, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_Key_Col3, Pin_Key_Col3, kPORT_MuxAsGpio);

    // row lines
    PORT_SetPinMux(Port_Key_Row0, Pin_Key_Row0, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_Key_Row1, Pin_Key_Row1, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_Key_Row2, Pin_Key_Row2, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_Key_Row3, Pin_Key_Row3, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_Key_Row4, Pin_Key_Row4, kPORT_MuxAsGpio);

    // column lines
    GPIO_PinInit(GPIO_Key_Col0, Pin_Key_Col0, &pin_config_input);
    GPIO_PinInit(GPIO_Key_Col1, Pin_Key_Col1, &pin_config_input);
    GPIO_PinInit(GPIO_Key_Col2, Pin_Key_Col2, &pin_config_input);
    GPIO_PinInit(GPIO_Key_Col3, Pin_Key_Col3, &pin_config_input);

    // row lines
    GPIO_PinInit(GPIO_Key_Row0, Pin_Key_Row0, &pin_config_input);
    GPIO_PinInit(GPIO_Key_Row1, Pin_Key_Row1, &pin_config_input);
    GPIO_PinInit(GPIO_Key_Row2, Pin_Key_Row2, &pin_config_input);
    GPIO_PinInit(GPIO_Key_Row3, Pin_Key_Row3, &pin_config_input);
    GPIO_PinInit(GPIO_Key_Row4, Pin_Key_Row4, &pin_config_input);

    old_keyboard_state = 0;
	keyDebounceScancode = 0;
	keyDebounceCounter = 0;
	keyState = KEY_IDLE;
}

void fw_reset_keyboard(void)
{
	keyState = KEY_WAIT_RELEASED;
}

inline uint8_t fw_read_keyboard_col(void)
{
	return ~((GPIOB->PDIR)>>19) & 0x1f;
}

uint32_t fw_read_keyboard(void)
{
	uint32_t result = 0;

	for (int col=3; col>=0; col--)
	{
		GPIO_PinInit(GPIOC, col, &pin_config_output);
		GPIO_PinWrite(GPIOC, col, 0);

		result=(result<<5) | fw_read_keyboard_col();

		GPIO_PinWrite(GPIOC, col, 1);
		GPIO_PinInit(GPIOC, col, &pin_config_input);
	}

    return result;
}

uint32_t fw_scan_key(uint32_t scancode)
{
	int key = 0;
	int col;
	int row = 0;
	int numKeys = 0;
	uint8_t scan;

	if (scancode == (SCAN_GREEN | SCAN_STAR)) {     // Just an example
		return KEY_GREENSTAR;
	}

	for (col = 0; col < 4; col++)
	{
		scan = scancode & 0x1f;
		if (scan)
		{
			for (row = 0; row < 5; row++)
			{
				if (scan & 0x01)
				{
					numKeys++;
					key = keyMap[col * 5 + row];
				}
				scan >>= 1;
			}
		}
		scancode >>= 5;
	}
	return (numKeys > 1) ? -1 : key;
}

void fw_check_key_event(uint32_t *keys, int *event)
{
	uint32_t scancode = fw_read_keyboard();
	uint32_t tmp_timer_keypad;
	uint32_t keypadTimerLong = nonVolatileSettings.keypadTimerLong * 1000;
	uint32_t keypadTimerRepeat = nonVolatileSettings.keypadTimerRepeat * 1000;

	*event = EVENT_KEY_NONE;
	*keys = 0;

	if (scancode != 0) {
		*keys = fw_scan_key(scancode);
	}

	switch (keyState) {
	case KEY_IDLE:
		if (scancode != 0) {
			keyState = KEY_DEBOUNCE;
			keyDebounceCounter = 0;
			keyDebounceScancode = scancode;
			old_keyboard_state = 0;
		}
		break;
	case KEY_DEBOUNCE:
		keyDebounceCounter++;
		if (keyDebounceCounter > KEY_DEBOUNCE_COUNTER && keyDebounceScancode == scancode) {
			if (*keys == -1) {
				keyState = KEY_WAIT_RELEASED;
			} else {
				keyState = KEY_PRESS;
			}
		}
//		else if (keyDebounceScancode != scancode) {
//			keyState = KEY_IDLE;
//		}
		*keys = 0;
		break;
	case  KEY_PRESS:
		if (*keys == -1) {
			keyState = KEY_WAIT_RELEASED;
			*keys=0;
		} else {
			taskENTER_CRITICAL();
			timer_keypad=keypadTimerLong;
			taskEXIT_CRITICAL();

			old_keyboard_state = *keys;
			*keys |= KEY_MOD_DOWN | KEY_MOD_PRESS;
			*event = EVENT_KEY_CHANGE;
			keyState = KEY_HOLD;
		}
		break;
	case KEY_HOLD:
		if (*keys == -1) {
			keyState = KEY_WAIT_RELEASED;
			*keys=0;
		} else if (*keys == 0) {
			*keys = old_keyboard_state | KEY_MOD_UP;
			*event = EVENT_KEY_CHANGE;
			keyState = KEY_IDLE;
		} else {
			taskENTER_CRITICAL();
			tmp_timer_keypad=timer_keypad;
			taskEXIT_CRITICAL();

			old_keyboard_state = *keys;
			if (tmp_timer_keypad == 0)
			{
				taskENTER_CRITICAL();
				timer_keypad=keypadTimerRepeat;
				taskEXIT_CRITICAL();

				*keys |= KEY_MOD_LONG | KEY_MOD_DOWN;
				*event = EVENT_KEY_CHANGE;
				keyState = KEY_REPEAT;
			}
		}
		break;
	case KEY_REPEAT:
		if (*keys == -1) {
			keyState = KEY_WAIT_RELEASED;
			*keys=0;
		} else if (*keys == 0) {
			*keys = old_keyboard_state | KEY_MOD_UP;
			*event = EVENT_KEY_CHANGE;
			keyState = KEY_IDLE;
		} else {
			taskENTER_CRITICAL();
			tmp_timer_keypad=timer_keypad;
			taskEXIT_CRITICAL();

			*keys |= KEY_MOD_LONG;
			if (tmp_timer_keypad == 0)
			{
				taskENTER_CRITICAL();
				timer_keypad=keypadTimerRepeat;
				taskEXIT_CRITICAL();

				if (KEYCHECK(*keys, KEY_LEFT) || KEYCHECK(*keys,KEY_RIGHT) ||
						KEYCHECK(*keys, KEY_UP) || KEYCHECK(*keys, KEY_DOWN)) {
					*keys |= KEY_MOD_PRESS;
				}
			}
			*event = EVENT_KEY_CHANGE;
		}
		break;
	case KEY_WAIT_RELEASED:
		if (scancode == 0) {
			keyState = KEY_IDLE;
		}
		*keys = 0;
		break;
	}

}

