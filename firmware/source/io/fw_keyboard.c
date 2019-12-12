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

static char oldKeyboardCode;
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
	 port_pin_config_t config = {
	      kPORT_PullUp,
	      kPORT_FastSlewRate,
	      kPORT_PassiveFilterDisable,
	      kPORT_OpenDrainDisable,
	      kPORT_LowDriveStrength,
	      kPORT_MuxAsGpio,
	      kPORT_UnlockRegister,
	 };

    // column lines
	PORT_SetPinMux(Port_Key_Col0, Pin_Key_Col0, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_Key_Col1, Pin_Key_Col1, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_Key_Col2, Pin_Key_Col2, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_Key_Col3, Pin_Key_Col3, kPORT_MuxAsGpio);

    // row lines
	PORT_SetPinConfig(Port_Key_Row0, Pin_Key_Row0, &config);
	PORT_SetPinConfig(Port_Key_Row1, Pin_Key_Row1, &config);
	PORT_SetPinConfig(Port_Key_Row2, Pin_Key_Row2, &config);
	PORT_SetPinConfig(Port_Key_Row3, Pin_Key_Row3, &config);
	PORT_SetPinConfig(Port_Key_Row4, Pin_Key_Row4, &config);

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

    oldKeyboardCode = 0;
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
		for(volatile int i = 0; i < 100;i++);// small delay to allow voltages to settle. The delay value of 100 is arbitrary.

		result=(result<<5) | fw_read_keyboard_col();

		GPIO_PinWrite(GPIOC, col, 1);
		GPIO_PinInit(GPIOC, col, &pin_config_input);
	}

    return result;
}

bool fw_scan_key(uint32_t scancode, char *keycode)
{
	int col;
	int row = 0;
	int numKeys = 0;
	uint8_t scan;

	*keycode = 0;
	if (scancode == 0) {
		return true;
	}

//	if (scancode == (SCAN_GREEN | SCAN_STAR)) {     // Just an example
//		return KEY_GREENSTAR;
//	}

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
					*keycode = keyMap[col * 5 + row];
				}
				scan >>= 1;
			}
		}
		scancode >>= 5;
	}
	return (numKeys == 1);
}

void fw_check_key_event(keyboardCode_t *keys, int *event)
{
	uint32_t scancode = fw_read_keyboard();
	char keycode;
	bool validKey;
	uint32_t tmp_timer_keypad;
	uint32_t keypadTimerLong = nonVolatileSettings.keypadTimerLong * 1000;
	uint32_t keypadTimerRepeat = nonVolatileSettings.keypadTimerRepeat * 1000;

	*event = EVENT_KEY_NONE;
	keys->event = 0;
	keys->key = 0;
	
	validKey = fw_scan_key(scancode, &keycode);

	switch (keyState) {
	case KEY_IDLE:
		if (scancode != 0) {
			keyState = KEY_DEBOUNCE;
			keyDebounceCounter = 0;
			keyDebounceScancode = scancode;
			oldKeyboardCode = 0;
		}
		break;
	case KEY_DEBOUNCE:
		keyDebounceCounter++;
		if (keyDebounceCounter > KEY_DEBOUNCE_COUNTER) {
			if (keyDebounceScancode == scancode) {
				keyState = KEY_PRESS;
			}
			else
			{
				keyState = KEY_WAIT_RELEASED;
			}
		}
		break;
	case  KEY_PRESS:
		if (!validKey) {
			keyState = KEY_WAIT_RELEASED;
		} else {
			taskENTER_CRITICAL();
			timer_keypad=keypadTimerLong;
			taskEXIT_CRITICAL();

			oldKeyboardCode = keycode;
			keys->key = keycode;
			keys->event = KEY_MOD_DOWN | KEY_MOD_PRESS;
			*event = EVENT_KEY_CHANGE;
			
			keyState = KEY_HOLD;
		}
		break;
	case KEY_HOLD:
		if (!validKey) {
			keyState = KEY_WAIT_RELEASED;
		} else if (keycode == 0) {
			keys->key = oldKeyboardCode;
			keys->event = KEY_MOD_UP;
			*event = EVENT_KEY_CHANGE;
			
			keyState = KEY_IDLE;
		} else {
			taskENTER_CRITICAL();
			tmp_timer_keypad=timer_keypad;
			taskEXIT_CRITICAL();

			if (tmp_timer_keypad == 0)
			{
				taskENTER_CRITICAL();
				timer_keypad=keypadTimerRepeat;
				taskEXIT_CRITICAL();

				keys->key = keycode;
				keys->event = KEY_MOD_LONG | KEY_MOD_DOWN;
				*event = EVENT_KEY_CHANGE;
				keyState = KEY_REPEAT;
			}
		}
		break;
	case KEY_REPEAT:
		if (!validKey) {
			keyState = KEY_WAIT_RELEASED;
		} else if (keycode == 0) {
			keys->key = oldKeyboardCode;
			keys->event = KEY_MOD_LONG | KEY_MOD_UP;
			*event = EVENT_KEY_CHANGE;
			
			keyState = KEY_IDLE;
		} else {
			taskENTER_CRITICAL();
			tmp_timer_keypad=timer_keypad;
			taskEXIT_CRITICAL();

			keys->key = keycode;
			keys->event = KEY_MOD_LONG;
			*event = EVENT_KEY_CHANGE;

			if (tmp_timer_keypad == 0)
			{
				taskENTER_CRITICAL();
				timer_keypad=keypadTimerRepeat;
				taskEXIT_CRITICAL();

				if (keys->key == KEY_LEFT || keys->key == KEY_RIGHT ||
					keys->key == KEY_UP || keys->key == KEY_DOWN) {
					keys->event = KEY_MOD_LONG | KEY_MOD_PRESS;
				}
			}
		}
		break;
	case KEY_WAIT_RELEASED:
		if (scancode == 0) {
			keyState = KEY_IDLE;
		}
		break;
	}

}
