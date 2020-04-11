/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
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
#include <user_interface/menuSystem.h>
#include <user_interface/uiLocalisation.h>
#include <settings.h>
#include <ticks.h>

int menuDisplayLightTimer=-1;
menuItemNew_t *gMenuCurrentMenuList;

int gMenusCurrentItemIndex; // each menu can re-use this var to hold the position in their display list. To save wasted memory if they each had their own variable
int gMenusStartIndex;// as above
int gMenusEndIndex;// as above


menuControlDataStruct_t menuControlData = { .stackPosition = 0, .stack = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, .itemIndex = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};


/*
 * ---------------------- IMPORTANT ----------------------------
 *
 * The menuFunctions array and the menusData array.....
 *
 * MUST match the enum MENU_SCREENS in menuSystem.h
 *
 * ---------------------- IMPORTANT ----------------------------
 */
const menuItemNew_t * menusData[] = { 	NULL,// splash
										NULL,// power off
										NULL,// vfo mode
										NULL,// channel mode
										menuDataMainMenu,
										menuDataContact,
										NULL,// zone
										NULL,// Battery
										NULL,// Firmwareinfo
										NULL,// Numerical entry
										NULL,// Tx
										NULL,// RSSI
										NULL,// LastHeard
										NULL,// Options
										NULL,// Display options
										NULL,// Sound options
										NULL,// Credits
										NULL,// Channel Details
										NULL,// hotspot mode
										NULL,// CPS
										NULL,// Quick menu - Channel
										NULL,// Quick menu - VFO
										NULL,// Lock screen
										NULL,// Contact List
										NULL,// Contact Quick List (SK2+#)
										NULL,// Contact List Quick Menu
										NULL,// Contact Details
										NULL,// New Contact
										NULL,// Language
										NULL,// Private Call
								};

const menuFunctionPointer_t menuFunctions[] = { menuSplashScreen,
												menuPowerOff,
												menuVFOMode,
												menuChannelMode,
												menuDisplayMenuList,
												menuDisplayMenuList,
												menuZoneList,
												menuBattery,
												menuFirmwareInfoScreen,
												menuNumericalEntry,
												menuTxScreen,
												menuRSSIScreen,
												menuLastHeard,
												menuOptions,
												menuDisplayOptions,
												menuSoundOptions,
												menuCredits,
												menuChannelDetails,
												menuHotspotMode,
												menuCPS,
												menuChannelModeQuickMenu,
												menuVFOModeQuickMenu,
                                                menuLockScreen,
												menuContactList,
												menuContactList,
												menuContactListSubMenu,
												menuContactDetails,
												menuContactDetails,
												menuLanguage,
												menuPrivateCall
};

void menuSystemPushNewMenu(int menuNumber)
{
	if (menuControlData.stackPosition < 15)
	{
		uiEvent_t ev = { .buttons = 0, .keys = NO_KEYCODE, .rotary = 0, .function = 0, .events = NO_EVENT, .hasEvent = false, .time = fw_millis() };

		fw_reset_keyboard();
		menuControlData.itemIndex[menuControlData.stackPosition] = gMenusCurrentItemIndex;
		menuControlData.stackPosition++;
		menuControlData.stack[menuControlData.stackPosition] = menuNumber;
		gMenusCurrentItemIndex = (menuNumber == MENU_MAIN_MENU) ? 1 : 0;
		menuFunctions[menuControlData.stack[menuControlData.stackPosition]](&ev, true);
	}
}

void menuSystemPushNewMenuWithQuickFunction(int menuNumber, int quickFunction)
{
	if (menuControlData.stackPosition < 15)
	{
		uiEvent_t ev = { .buttons = 0, .keys = NO_KEYCODE, .rotary = 0, .function = quickFunction, .events = FUNCTION_EVENT, .hasEvent = false, .time = fw_millis() };

		fw_reset_keyboard();
		menuControlData.itemIndex[menuControlData.stackPosition] = gMenusCurrentItemIndex;
		menuControlData.stackPosition++;
		menuControlData.stack[menuControlData.stackPosition] = menuNumber;
		gMenusCurrentItemIndex = (menuNumber == MENU_MAIN_MENU) ? 1 : 0;
		menuFunctions[menuControlData.stack[menuControlData.stackPosition]](&ev, true);
	}
}

void menuSystemPopPreviousMenu(void)
{
	uiEvent_t ev = { .buttons = 0, .keys = NO_KEYCODE, .rotary = 0, .function = 0, .events = NO_EVENT, .hasEvent = false, .time = fw_millis() };

	fw_reset_keyboard();
	menuControlData.itemIndex[menuControlData.stackPosition] = 0;
	menuControlData.stackPosition -= (menuControlData.stackPosition > 0) ? 1 : 0; // Avoid crashing if something goes wrong.
	gMenusCurrentItemIndex = menuControlData.itemIndex[menuControlData.stackPosition];
	menuFunctions[menuControlData.stack[menuControlData.stackPosition]](&ev,true);
}

void menuSystemPopAllAndDisplayRootMenu(void)
{
	uiEvent_t ev = { .buttons = 0, .keys = NO_KEYCODE, .rotary = 0, .function = 0, .events = NO_EVENT, .hasEvent = false, .time = fw_millis() };

	fw_reset_keyboard();
	memset(menuControlData.itemIndex, 0, sizeof(menuControlData.itemIndex));
	menuControlData.stackPosition = 0;
	gMenusCurrentItemIndex = 0;
	menuFunctions[menuControlData.stack[menuControlData.stackPosition]](&ev,true);
}

void menuSystemPopAllAndDisplaySpecificRootMenu(int newRootMenu)
{
	uiEvent_t ev = { .buttons = 0, .keys = NO_KEYCODE, .rotary = 0, .function = 0, .events = NO_EVENT, .hasEvent = false, .time = fw_millis() };

	fw_reset_keyboard();
	memset(menuControlData.itemIndex, 0, sizeof(menuControlData.itemIndex));
	menuControlData.stack[0]  = newRootMenu;
	menuControlData.stackPosition = 0;
	gMenusCurrentItemIndex = (newRootMenu == MENU_MAIN_MENU) ? 1 : 0;
	menuFunctions[menuControlData.stack[menuControlData.stackPosition]](&ev,true);
}

void menuSystemSetCurrentMenu(int menuNumber)
{
	uiEvent_t ev = { .buttons = 0, .keys = NO_KEYCODE, .rotary = 0, .function = 0, .events = NO_EVENT, .hasEvent = false, .time = fw_millis() };

	fw_reset_keyboard();
	menuControlData.stack[menuControlData.stackPosition] = menuNumber;
	gMenusCurrentItemIndex = menuControlData.itemIndex[menuControlData.stackPosition];
	menuFunctions[menuControlData.stack[menuControlData.stackPosition]](&ev,true);
}

int menuSystemGetCurrentMenuNumber(void)
{
	return menuControlData.stack[menuControlData.stackPosition];
}

void menuSystemCallCurrentMenuTick(uiEvent_t *ev)
{
	menuFunctions[menuControlData.stack[menuControlData.stackPosition]](ev,false);
}

void displayLightTrigger(void)
{
	if ((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_AUTO) ||
			((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_MANUAL) && fw_displayIsBacklightLit()))
	{
		if (nonVolatileSettings.backlightMode == BACKLIGHT_MODE_AUTO)
		{
			menuDisplayLightTimer = nonVolatileSettings.backLightTimeout * 1000;
		}
		fw_displayEnableBacklight(true);
	}
}

// use -1 to force LED on all the time
void displayLightOverrideTimeout(int timeout)
{
	if (nonVolatileSettings.backlightMode == BACKLIGHT_MODE_AUTO)
	{
		menuDisplayLightTimer = timeout;
		fw_displayEnableBacklight(true);
	}
}

void menuInitMenuSystem(void)
{
	uiEvent_t ev = { .buttons = 0, .keys = NO_KEYCODE, .rotary = 0, .function = 0, .events = NO_EVENT, .hasEvent = false, .time = fw_millis() };

	menuDisplayLightTimer = -1;
	menuControlData.stack[menuControlData.stackPosition]  = MENU_SPLASH_SCREEN;// set the very first screen as the splash screen
	gMenusCurrentItemIndex = 0;
	menuFunctions[menuControlData.stack[menuControlData.stackPosition]](&ev,true);// Init and display this screen
}

void menuSystemLanguageHasChanged(void)
{
	// Force full update of menuChannelMode() on next call (if isFirstRun arg. is true)
	menuChannelColdStart();
}


/*
const char menuStringTable[32][17] = { "",//0
                                         "Menu",//1
                                         "Contacts",//2
                                         "Message",//3
                                         "Call Logs",//4
                                         "Set",//5
                                         "Zone",//6
                                         "New Contact",//7
                                         "Manual Dial",//8
                                         "InBox",//9
                                         "New Message",//10
                                         "Manual Dial",//11
                                         "OutBox",//12
                                         "Draft",//13
                                         "Quick test",//14
										 "Battery",//15
										 "Firmware info",//16
										 "RSSI",//17
										 "Last heard",//18
										 "Options",//19
										 "Display options",//20
										 "Credits",//21
										 "Channel details",//22
										 "Hotspot mode",//23
										 "Contact List",//24
										 "Contact Details",//25
};
*/

const menuItemNew_t menuDataMainMenu[] = {
	{ 12, 12 }, // Special entry: number of menus entries, both member must be set to the same number
	{ 2,  MENU_CREDITS },
	{ 3,  MENU_ZONE_LIST },
	{ 4,  MENU_RSSI_SCREEN },
	{ 5,  MENU_BATTERY },
	{ 6,  MENU_CONTACTS_MENU },
	{ 7,  MENU_LAST_HEARD },
	{ 8,  MENU_FIRMWARE_INFO },
	{ 9,  MENU_OPTIONS },
	{ 10, MENU_DISPLAY },
	{ 11, MENU_SOUND },
	{ 12, MENU_CHANNEL_DETAILS},
	{ 13, MENU_LANGUAGE},
};
const menuItemNew_t menuDataContact[] = {
	{ 2, 2} , // Special entry: number of menus entries, both member must be set to the same number
	{ 14, MENU_CONTACT_NEW },
	{ 15, MENU_CONTACT_LIST },
};

/*
const menuItemNew_t menuDataContactContact [] = {
	{ 6,6 },// length
	{ 9, -1 },// InBox
	{ 10, -1 },// New Message
	{ 11, -1 },// Manual Dial
	{ 12, -1 },// OutBox
	{ 13, -1 },// Draft
	{ 14, -1 }// Quick Test
};*/

void menuDisplayTitle(const char *title)
{
	ucDrawFastHLine(0, 13, 128, true);
	ucPrintCore(0, 3, title, FONT_SIZE_2, TEXT_ALIGN_CENTER, false);
}

void menuDisplayEntry(int loopOffset, int focusedItem,const char *entryText)
{
#if defined(PLATFORM_RD5R)
const int MENU_START_Y = 28;
const int HIGHLIGHT_START_Y = 27;
#else
const int MENU_START_Y = 32;
const int HIGHLIGHT_START_Y = 32;
#endif

	bool focused = (focusedItem == gMenusCurrentItemIndex);

	if (focused)
	{
		ucFillRoundRect(0, HIGHLIGHT_START_Y +  (loopOffset * FONT_SIZE_3_HEIGHT), 128, FONT_SIZE_3_HEIGHT, 2, true);
	}

	ucPrintCore(0,  MENU_START_Y +  (loopOffset * FONT_SIZE_3_HEIGHT), entryText, FONT_SIZE_3, TEXT_ALIGN_LEFT, focused);

}

int menuGetMenuOffset(int maxMenuEntries, int loopOffset)
{
	int offset = gMenusCurrentItemIndex + loopOffset;

	if (offset < 0)
	{
		if ((maxMenuEntries == 1) && (maxMenuEntries < MENU_MAX_DISPLAYED_ENTRIES))
			offset = MENU_MAX_DISPLAYED_ENTRIES - 1;
		else
			offset = maxMenuEntries + offset;
	}

	if (maxMenuEntries < MENU_MAX_DISPLAYED_ENTRIES)
	{
		if (loopOffset == 1)
			offset = MENU_MAX_DISPLAYED_ENTRIES - 1;
	}
	else
	{
		if (offset >= maxMenuEntries)
			offset = offset - maxMenuEntries;
	}

	return offset;
}

/*
 * Returns 99 if key is unknown, or not numerical when digitsOnly is true
 */
int menuGetKeypadKeyValue(uiEvent_t *ev, bool digitsOnly)
{
	uint32_t keypadKeys[] = {
			KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
			KEY_LEFT, KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_STAR, KEY_HASH
	};

	for (int i = 0; i < ((sizeof(keypadKeys) / sizeof(keypadKeys[0])) - (digitsOnly ? 6 : 0 )); i++)
	{
		if (KEYCHECK_PRESS(ev->keys, keypadKeys[i]))
				return i;
	}

	return 99;
}

void menuUpdateCursor(int pos, bool moved, bool render)
{
	static uint32_t lastBlink = 0;
	static bool     blink = false;
	uint32_t        m = fw_millis();

	if (moved) {
		blink = true;
	}
	if (moved || (m - lastBlink) > 500)
	{
		ucDrawFastHLine(pos*8, 46, 8, blink);

		blink = !blink;
		lastBlink = m;

		if (render) {
			ucRenderRows(5,6);
		}
	}
}

void moveCursorLeftInString(char *str, int *pos, bool delete)
{
	int nLen = strlen(str);

	if (*pos > 0) {
		*pos -=1;
		if (delete)
		{
			for (int i = *pos; i <= nLen; i++)
			{
				str[i] = str[i + 1];
			}
		}
	}
}

void moveCursorRightInString(char *str, int *pos, int max, bool insert)
{
	int nLen = strlen(str);

	if (*pos < strlen(str))
	{
		if (insert)
		{
			if (nLen < max)
			{
				for (int i = nLen; i > *pos; i--)
				{
					str[i] = str[i - 1];
				}
				str[*pos] = ' ';
			}
		}
		if (*pos < max-1) {
			*pos += 1;
		}
	}
}
