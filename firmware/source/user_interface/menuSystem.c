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
#include <functions/fw_ticks.h>
#include "fw_settings.h"

int menuDisplayLightTimer=-1;
menuItemNew_t *gMenuCurrentMenuList;

menuControlDataStruct_t menuControlData = { .stackPosition = 0, .stack = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, .itemIndex = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};


int menuVFOMode(ui_event_t *event, bool isFirstRun);
int menuVFOModeQuickMenu(ui_event_t *event, bool isFirstRun);
int menuChannelMode(ui_event_t *event, bool isFirstRun);
int menuChannelModeQuickMenu(ui_event_t *event, bool isFirstRun);
int menuZoneList(ui_event_t *event, bool isFirstRun);
int menuDisplayMenuList(ui_event_t *event, bool isFirstRun);
int menuBattery(ui_event_t *event, bool isFirstRun);
int menuSplashScreen(ui_event_t *event, bool isFirstRun);
int menuPowerOff(ui_event_t *event, bool isFirstRun);
int menuFirmwareInfoScreen(ui_event_t *event, bool isFirstRun);
int menuNumericalEntry(ui_event_t *event, bool isFirstRun);
int menuTxScreen(ui_event_t *event, bool isFirstRun);
int menuRSSIScreen(ui_event_t *event, bool isFirstRun);
int menuLastHeard(ui_event_t *event, bool isFirstRun);
int menuOptions(ui_event_t *event, bool isFirstRun);
int menuDisplayOptions(ui_event_t *event, bool isFirstRun);
int menuCredits(ui_event_t *event, bool isFirstRun);
int menuChannelDetails(ui_event_t *event, bool isFirstRun);
int menuHotspotMode(ui_event_t *event, bool isFirstRun);
int menuCPS(ui_event_t *event, bool isFirstRun);
int menuLockScreen(ui_event_t *event, bool isFirstRun);
int menuContactList(ui_event_t *event, bool isFirstRun);
int menuContactListSubMenu(ui_event_t *event, bool isFirstRun);
int menuContactDetails(ui_event_t *event, bool isFirstRun);
int menuLanguage(ui_event_t *event, bool isFirstRun);

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
								};

const MenuFunctionPointer_t menuFunctions[] = { menuSplashScreen,
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
												menuLanguage,
};

void menuSystemPushNewMenu(int menuNumber)
{
	if (menuControlData.stackPosition < 15)
	{
		ui_event_t ev = { .buttons = 0, .keys = 0, .events = 0, .hasEvent = false, .ticks = fw_millis() };

		menuControlData.itemIndex[menuControlData.stackPosition] = gMenusCurrentItemIndex;
		menuControlData.stackPosition++;
		menuControlData.stack[menuControlData.stackPosition] = menuNumber;
		gMenusCurrentItemIndex = (menuNumber == MENU_MAIN_MENU) ? 1 : 0;
		menuFunctions[menuControlData.stack[menuControlData.stackPosition]](&ev, true);
		fw_reset_keyboard();
	}
}
void menuSystemPopPreviousMenu(void)
{
	ui_event_t ev = { .buttons = 0, .keys = 0, .events = 0, .hasEvent = false, .ticks = fw_millis() };

	menuControlData.itemIndex[menuControlData.stackPosition] = 0;
	menuControlData.stackPosition--;
	gMenusCurrentItemIndex = menuControlData.itemIndex[menuControlData.stackPosition];
	menuFunctions[menuControlData.stack[menuControlData.stackPosition]](&ev,true);
	fw_reset_keyboard();
}

void menuSystemPopAllAndDisplayRootMenu(void)
{
	ui_event_t ev = { .buttons = 0, .keys = 0, .events = 0, .hasEvent = false, .ticks = fw_millis() };

	memset(menuControlData.itemIndex, 0, sizeof(menuControlData.itemIndex));
	menuControlData.stackPosition = 0;
	gMenusCurrentItemIndex = 0;
	menuFunctions[menuControlData.stack[menuControlData.stackPosition]](&ev,true);
	fw_reset_keyboard();
}

void menuSystemPopAllAndDisplaySpecificRootMenu(int newRootMenu)
{
	ui_event_t ev = { .buttons = 0, .keys = 0, .events = 0, .hasEvent = false, .ticks = fw_millis() };

	memset(menuControlData.itemIndex, 0, sizeof(menuControlData.itemIndex));
	menuControlData.stack[0]  = newRootMenu;
	menuControlData.stackPosition = 0;
	gMenusCurrentItemIndex = (newRootMenu == MENU_MAIN_MENU) ? 1 : 0;
	menuFunctions[menuControlData.stack[menuControlData.stackPosition]](&ev,true);
	fw_reset_keyboard();
}

void menuSystemSetCurrentMenu(int menuNumber)
{
	ui_event_t ev = { .buttons = 0, .keys = 0, .events = 0, .hasEvent = false, .ticks = fw_millis() };

	menuControlData.stack[menuControlData.stackPosition] = menuNumber;
	gMenusCurrentItemIndex = menuControlData.itemIndex[menuControlData.stackPosition];
	menuFunctions[menuControlData.stack[menuControlData.stackPosition]](&ev,true);
	fw_reset_keyboard();
}
int menuSystemGetCurrentMenuNumber(void)
{
	return menuControlData.stack[menuControlData.stackPosition];
}

void menuSystemCallCurrentMenuTick(ui_event_t *ev)
{
	menuFunctions[menuControlData.stack[menuControlData.stackPosition]](ev,false);
}

void displayLightTrigger(void)
{
	menuDisplayLightTimer = nonVolatileSettings.backLightTimeout * 1000;
	fw_displayEnableBacklight(true);
}

// use -1 to force LED on all the time
void displayLightOverrideTimeout(int timeout)
{
	menuDisplayLightTimer = timeout;
	fw_displayEnableBacklight(true);
}

const int MENU_EVENT_SAVE_SETTINGS = -1;
int gMenusCurrentItemIndex; // each menu can re-use this var to hold the position in their display list. To save wasted memory if they each had their own variable
int gMenusStartIndex;// as above
int gMenusEndIndex;// as above


void menuInitMenuSystem(void)
{
	ui_event_t ev = { .buttons = 0, .keys = 0, .events = 0, .hasEvent = false, .ticks = fw_millis() };

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
	{11,11},// number of menus
	{ 2, MENU_CREDITS },
	{ 3, MENU_ZONE_LIST },
	{ 4, MENU_RSSI_SCREEN },
	{ 5, MENU_BATTERY },
	{ 6, MENU_CONTACTS_MENU },
	{ 7, MENU_LAST_HEARD },
	{ 8, MENU_FIRMWARE_INFO },
	{ 9, MENU_OPTIONS },
	{ 10, MENU_DISPLAY},
	{ 11, MENU_CHANNEL_DETAILS},
	{ 12, MENU_LANGUAGE},
};
const menuItemNew_t menuDataContact[] = {
	{ 2, 2} ,// length
	{ 13 , MENU_CONTACT_DETAILS },// 7 New Contact
	{ 14, MENU_CONTACT_LIST },// 24 Contacts List
	{ -1, MENU_CONTACT_LIST },// 24 Contacts Lis
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
	UC1701_drawFastHLine(0, 13, 128, true);
	UC1701_printCore(0, 3, title, UC1701_FONT_8x8, UC1701_TEXT_ALIGN_CENTER, false);
}

void menuDisplayEntry(int loopOffset, int focusedItem,const char *entryText)
{
	bool focused = (focusedItem == gMenusCurrentItemIndex);

	if (focused)
		UC1701_fillRoundRect(0, (loopOffset + 2) * 16, 128, 16, 2, true);

	UC1701_printCore(0, (loopOffset + 2) * 16, entryText, UC1701_FONT_8x16, UC1701_TEXT_ALIGN_LEFT, focused);
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

