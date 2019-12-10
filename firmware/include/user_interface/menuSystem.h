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
#ifndef _FW_MENUSYSTEM_H_
#define _FW_MENUSYSTEM_H_
#include "fw_main.h"


typedef struct
{
	uint32_t      buttons;
	uint32_t      keys;
	int      events; // See *_EVENT below
	bool     hasEvent;
	uint32_t ticks;
} ui_event_t;

#define NO_EVENT       0x00
#define KEY_EVENT      0x01
#define BUTTON_EVENT   0x02


#define MENU_MAX_DISPLAYED_ENTRIES 3
#define MENU_INC(O, M) do { O = (O + 1) % M; } while(0)
#define MENU_DEC(O, M) do { O = (O + M - 1) % M; } while(0)

extern bool uiChannelModeScanActive;
extern int menuDisplayLightTimer;


typedef int (*MenuFunctionPointer_t)(ui_event_t *,bool); // Typedef for menu function pointers.  Functions are passed the key, the button and the event data. Event can be a Key or a button or both. Last arg is for when the function is only called to initialise and display its screen.
typedef struct menuControlDataStruct
{
	int currentMenuNumber;
	int stackPosition;
	int stack[16];
	int itemIndex[16];
} menuControlDataStruct_t;

typedef struct menuItemNew
{
       int stringNumber;
       int menuNum;
} menuItemNew_t;

extern menuControlDataStruct_t menuControlData;

void menuDisplayTitle(const char *title);
void menuDisplayEntry(int loopOffset, int focusedItem,const char *entryText);
int menuGetMenuOffset(int maxMenuEntries, int loopOffset);

void menuChannelModeUpdateScreen(int txTimeSecs);
void menuChannelColdStart();
void menuVFOModeUpdateScreen(int txTimeSecs);
void menuVFOModeStopScan(void);
void menuCPSUpdate(int command,int x, int y, UC1701_Font_t fontSize, UC1701_Text_Align_t alignment, bool isInverted,char *szMsg);

void menuInitMenuSystem(void);
void menuSystemLanguageHasChanged(void);
void displayLightTrigger(void);
void displayLightOverrideTimeout(int timeout);
void menuSystemPushNewMenu(int menuNumber);

void menuSystemSetCurrentMenu(int menuNumber);
int menuSystemGetCurrentMenuNumber(void);

void menuSystemPopPreviousMenu(void);
void menuSystemPopAllAndDisplayRootMenu(void);
void menuSystemPopAllAndDisplaySpecificRootMenu(int newRootMenu);

void menuSystemCallCurrentMenuTick(ui_event_t *ev);

/*
 * ---------------------- IMPORTANT ----------------------------
 *
 * These enums must match the menuFunctions array in menuSystem.c
 *
 * ---------------------- IMPORTANT ----------------------------
 */
enum MENU_SCREENS { MENU_SPLASH_SCREEN=0,
					MENU_POWER_OFF,
					MENU_VFO_MODE,
					MENU_CHANNEL_MODE,
					MENU_MAIN_MENU,
					MENU_CONTACTS_MENU,
					MENU_ZONE_LIST,
					MENU_BATTERY,
					MENU_FIRMWARE_INFO,
					MENU_NUMERICAL_ENTRY,
					MENU_TX_SCREEN,
					MENU_RSSI_SCREEN,
					MENU_LAST_HEARD,
					MENU_OPTIONS,
					MENU_DISPLAY,
					MENU_CREDITS,
					MENU_CHANNEL_DETAILS,
					MENU_HOTSPOT_MODE,
					MENU_CPS,
					MENU_CHANNEL_QUICK_MENU,
					MENU_VFO_QUICK_MENU,
					MENU_LOCK_SCREEN,
					MENU_CONTACT_LIST,
					MENU_CONTACT_QUICKLIST,
					MENU_CONTACT_LIST_SUBMENU,
					MENU_CONTACT_DETAILS,
					MENU_LANGUAGE,
};

// This is used to store current position in menus. The system keeps track of its value, e.g entering in
// a submenu, it will be restored exiting that submenu. It's up to the menuFunction() to override its
// value when isFirstRun == true.
extern int gMenusCurrentItemIndex;

extern int gMenusStartIndex;
extern int gMenusEndIndex;
extern menuItemNew_t *gMenuCurrentMenuList;

extern const char menuStringTable[32][17];

extern const menuItemNew_t menuDataMainMenu[];
extern const menuItemNew_t menuDataContact[];
extern const menuItemNew_t menuDataContactContact [];
extern const menuItemNew_t * menusData[];


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

#endif
