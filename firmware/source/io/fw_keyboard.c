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
#include "fw_usb_com.h"

static uint32_t old_keyboard_state;
static bool keypress_long;
static uint32_t keyDebounceState;
static int keyDebounceCounter;
static uint32_t keyHandled;

volatile bool keypadLocked = false;

static const uint32_t keyMap[] = {
		KEY_1, KEY_2, KEY_3, KEY_GREEN, KEY_RIGHT,
		KEY_4, KEY_5, KEY_6, KEY_UP, KEY_LEFT,
		KEY_7, KEY_8, KEY_9, KEY_DOWN, 0,
		KEY_STAR, KEY_0, KEY_HASH, KEY_RED, 0 };

void fw_init_keyboard()
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
	keyDebounceState = 0;
	keyDebounceCounter = 0;
	keyHandled = 0;
}

inline uint8_t fw_read_keyboard_col()
{
	return ~((GPIOB->PDIR)>>19) & 0x1f;
}

uint32_t fw_read_keyboard()
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

uint32_t fw_scan_key(uint32_t keys)
{
	int key = 0;
	int col;
	int row = 0;
	int numKeys = 0;
	uint8_t scan;

	if (keys == (KEY_GREEN | KEY_STAR)) {     // Just an example
		return KEY_GREENSTAR;
	}

	for (col = 0; col < 4; col++)
	{
		scan = keys & 0x1f;
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
		keys >>= 5;
	}
	return (numKeys > 1) ? -1 : key;
}

void fw_check_key_event(uint32_t *keys, int *event)
{
	uint32_t mod = 0x0U;
	uint32_t tmpKeys;
	uint32_t scancode = fw_read_keyboard();

	*event = EVENT_KEY_NONE;
	if (keyDebounceState != scancode)
	{
		keyDebounceCounter = 0;
		keyDebounceState = scancode;
	}
	else
	{
		if (keyDebounceCounter > KEY_DEBOUNCE_COUNTER)
		{
            if (keyHandled != 0 && keyHandled != scancode) {
    			keyDebounceCounter = 0;
				if (scancode == 0) {
					keyHandled = 0;
					old_keyboard_state = 0;
				}
				*keys = 0;
			}
			else
			{
				taskENTER_CRITICAL();
				uint32_t tmp_timer_keyrepeat=timer_keyrepeat;
				uint32_t tmp_timer_keylong=timer_keylong;
				taskEXIT_CRITICAL();

				*keys = fw_scan_key(scancode);

				if (*keys == -1)
				{
					taskENTER_CRITICAL();
					timer_keylong=0;
					timer_keyrepeat=0;
					taskEXIT_CRITICAL();

					keyHandled = 0xffffffffU;
					*keys = 0;
				}
				else
				{
					keyHandled = scancode;

					if (*keys != 0)
					{
						if (tmp_timer_keylong == 0)
						{
							taskENTER_CRITICAL();
							timer_keylong=KEY_LONG_PRESS_COUNTER;
							timer_keyrepeat=KEY_LONG_PRESS_COUNTER + KEY_REPEAT_COUNTER;
							taskEXIT_CRITICAL();
							mod = KEY_MOD_DOWN | KEY_MOD_PRESS;
							keypress_long = false;
						}
						else
						{
							if (tmp_timer_keylong == 1)
							{
								mod = KEY_MOD_LONG;
								if (keypress_long == false)
								{
									mod |= KEY_MOD_DOWN;
								}
								keypress_long = true;
								*event = EVENT_KEY_CHANGE;
							}
							if (tmp_timer_keyrepeat == 0)
							{
								mod |= KEY_MOD_PRESS;
								taskENTER_CRITICAL();
								timer_keyrepeat=KEY_REPEAT_COUNTER;
								taskEXIT_CRITICAL();
								*event = EVENT_KEY_CHANGE;
							}
						}
					}
					else
					{
						if (tmp_timer_keylong > 0 || keypress_long == true)
						{
							mod = KEY_MOD_UP;
							if (keypress_long == true)
							{
								mod |= KEY_MOD_LONG;
							}
						}
						taskENTER_CRITICAL();
						timer_keylong=0;
						timer_keyrepeat=0;
						taskEXIT_CRITICAL();
					}

					if (old_keyboard_state!=*keys)
					{
						*event = EVENT_KEY_CHANGE;
						tmpKeys = old_keyboard_state;
						old_keyboard_state=*keys;
						if (mod == KEY_MOD_UP)
						{
							*keys = tmpKeys;
						}
					}
					*keys = *keys | mod;
				}
			}
		} else {
			keyDebounceCounter++;
		}
	}
}
