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

static uint32_t old_keyboard_state;
static bool keypress_long;
static uint32_t keyDebounceState;
static int keyDebounceCounter;
static uint32_t keyLongCounter;
static uint32_t keyRepeatCounter;
static uint32_t keyRepeatSpeed;
static uint32_t keyHandled;

volatile bool keypadLocked = false;

static const uint32_t keyMap[] =
{
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
	keyLongCounter = 0;
	keyHandled = 0;
}

uint8_t fw_read_keyboard_col()
{
	uint8_t result=0;

	if (GPIO_PinRead(GPIO_Key_Row0, Pin_Key_Row0)==0)
	{
		result|=0x01;
	}
	if (GPIO_PinRead(GPIO_Key_Row1, Pin_Key_Row1)==0)
	{
		result|=0x02;
	}
	if (GPIO_PinRead(GPIO_Key_Row2, Pin_Key_Row2)==0)
	{
		result|=0x04;
	}
	if (GPIO_PinRead(GPIO_Key_Row3, Pin_Key_Row3)==0)
	{
		result|=0x08;
	}
	if (GPIO_PinRead(GPIO_Key_Row4, Pin_Key_Row4)==0)
	{
		result|=0x10;
	}
	return result;
}

uint32_t fw_read_keyboard()
{
    GPIO_PinInit(GPIO_Key_Col3, Pin_Key_Col3, &pin_config_output);
	GPIO_PinWrite(GPIO_Key_Col3, Pin_Key_Col3, 0);
	uint32_t result=fw_read_keyboard_col();
	GPIO_PinWrite(GPIO_Key_Col3, Pin_Key_Col3, 1);
    GPIO_PinInit(GPIO_Key_Col3, Pin_Key_Col3, &pin_config_input);

    GPIO_PinInit(GPIO_Key_Col2, Pin_Key_Col2, &pin_config_output);
	GPIO_PinWrite(GPIO_Key_Col2, Pin_Key_Col2, 0);
	result=(result<<5)|fw_read_keyboard_col();
	GPIO_PinWrite(GPIO_Key_Col2, Pin_Key_Col2, 1);
    GPIO_PinInit(GPIO_Key_Col2, Pin_Key_Col2, &pin_config_input);

    GPIO_PinInit(GPIO_Key_Col1, Pin_Key_Col1, &pin_config_output);
	GPIO_PinWrite(GPIO_Key_Col1, Pin_Key_Col1, 0);
	result=(result<<5)|fw_read_keyboard_col();
	GPIO_PinWrite(GPIO_Key_Col1, Pin_Key_Col1, 1);
    GPIO_PinInit(GPIO_Key_Col1, Pin_Key_Col1, &pin_config_input);

    GPIO_PinInit(GPIO_Key_Col0, Pin_Key_Col0, &pin_config_output);
	GPIO_PinWrite(GPIO_Key_Col0, Pin_Key_Col0, 0);
	result=(result<<5)|fw_read_keyboard_col();
	GPIO_PinWrite(GPIO_Key_Col0, Pin_Key_Col0, 1);
    GPIO_PinInit(GPIO_Key_Col0, Pin_Key_Col0, &pin_config_input);

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
	if (numKeys > 1)
	{
		return -1;
	}
	return (numKeys == 1) ? key : 0;
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
		keyDebounceCounter++;
		if (keyDebounceCounter > KEY_DEBOUNCE_COUNTER)
		{
			keyDebounceCounter = 0;
            if (keyHandled != 0 && keyHandled != scancode) {
				if (scancode == 0) {
					keyHandled = 0;
					old_keyboard_state = 0;
				}
				*keys = 0;
			}
			else
			{
				*keys = fw_scan_key(scancode);

				if (*keys == -1)
				{
					keyLongCounter = 0;
					keyHandled = 0xffffffffU;
					*keys = 0;
				}
				else
				{
					keyHandled = scancode;

					if (*keys != 0)
					{
						if (keyLongCounter == 0)
						{
							keyLongCounter = PITCounter;
							keyRepeatCounter = PITCounter;
							keyRepeatSpeed = KEY_LONG_PRESS_COUNTER + KEY_REPEAT_COUNTER;
							mod = KEY_MOD_DOWN | KEY_MOD_PRESS;
							keypress_long = false;
						}
						else
						{
							if (PITCounter - keyLongCounter > KEY_LONG_PRESS_COUNTER)
							{
								mod = KEY_MOD_LONG;
								if (keypress_long == false)
								{
									mod |= KEY_MOD_DOWN;
								}
								keypress_long = true;
								*event = EVENT_KEY_CHANGE;
							}
							if (PITCounter - keyRepeatCounter > keyRepeatSpeed)
							{
								mod |= KEY_MOD_PRESS;
								keyRepeatCounter = PITCounter;
								keyRepeatSpeed = KEY_REPEAT_COUNTER;
								*event = EVENT_KEY_CHANGE;
							}
						}
					}
					else
					{
						if (keyLongCounter > 0)
						{
							mod = KEY_MOD_UP;
							if (keypress_long == true)
							{
								mod |= KEY_MOD_LONG;
							}
						}
						keyLongCounter = 0;
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
		}
	}
}

bool fw_key_check(uint32_t keys, uint32_t key, uint32_t mod) {
	return ((keys & (key | mod)) == (key | mod));
}
