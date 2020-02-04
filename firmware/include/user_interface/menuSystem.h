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

typedef enum { NO_EVENT = 0, KEY_EVENT = 0x01, BUTTON_EVENT = 0x02, FUNCTION_EVENT = 0x04 } uiEventInput_t;

typedef struct
{
	uint32_t	    buttons;
	keyboardCode_t  keys;
	uint16_t		function;
	uiEventInput_t	events;
	bool		    hasEvent;
	uint32_t 	    time;
} uiEvent_t;

#define MENU_MAX_DISPLAYED_ENTRIES 3
#define MENU_INC(O, M) do { O = (O + 1) % M; } while(0)
#define MENU_DEC(O, M) do { O = (O + M - 1) % M; } while(0)

extern bool uiChannelModeScanActive;
extern int menuDisplayLightTimer;
extern int uiPrivateCallState;
extern int uiPrivateCallLastID;


typedef int (*menuFunctionPointer_t)(uiEvent_t *, bool); // Typedef for menu function pointers.  Functions are passed the key, the button and the event data. Event can be a Key or a button or both. Last arg is for when the function is only called to initialise and display its screen.
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
void menuVFOModeStopScanning(void);
bool menuVFOModeIsScanning(void);
bool menuChannelModeIsScanning(void);
void menuChannelModeStopScanning(void);
void menuCPSUpdate(int command,int x, int y, ucFont_t fontSize, ucTextAlign_t alignment, bool isInverted,char *szMsg);

void menuInitMenuSystem(void);
void menuSystemLanguageHasChanged(void);
void displayLightTrigger(void);
void displayLightOverrideTimeout(int timeout);
void menuSystemPushNewMenu(int menuNumber);
void menuSystemPushNewMenuWithQuickFunction(int menuNumber, int quickFunction);

void menuSystemSetCurrentMenu(int menuNumber);
int menuSystemGetCurrentMenuNumber(void);

void menuSystemPopPreviousMenu(void);
void menuSystemPopAllAndDisplayRootMenu(void);
void menuSystemPopAllAndDisplaySpecificRootMenu(int newRootMenu);

void menuSystemCallCurrentMenuTick(uiEvent_t *ev);
int menuGetKeypadKeyValue(uiEvent_t *ev, bool digitsOnly);
void menuUpdateCursor(int pos, bool moved, bool render);
void moveCursorLeftInString(char *str, int *pos, bool delete);
void moveCursorRightInString(char *str, int *pos, int max, bool insert);

void menuBatteryInit(void);
void menuBatteryPushBackVoltage(int32_t voltage);

void menuLockScreenPop(void);

void menuLastHeardUpdateScreen(bool showTitleOrHeader, bool displayDetails);

void menuClearPrivateCall(void);
void menuAcceptPrivateCall(int id);


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
					MENU_CONTACT_NEW,
					MENU_LANGUAGE,
					MENU_PRIVATE_CALL,
					NUM_MENU_ENTRIES
};

enum QUICK_FUNCTIONS {  QUICK_FUNCTIONS_MENU_PLACEHOLDER = 20,   // All values lower than this are used as menu entries
						START_SCANNING,
						INC_BRIGHTNESS,
						DEC_BRIGHTNESS
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


int menuVFOMode(uiEvent_t *event, bool isFirstRun);
int menuVFOModeQuickMenu(uiEvent_t *event, bool isFirstRun);
int menuChannelMode(uiEvent_t *event, bool isFirstRun);
int menuChannelModeQuickMenu(uiEvent_t *event, bool isFirstRun);
int menuZoneList(uiEvent_t *event, bool isFirstRun);
int menuDisplayMenuList(uiEvent_t *event, bool isFirstRun);
int menuBattery(uiEvent_t *event, bool isFirstRun);
int menuSplashScreen(uiEvent_t *event, bool isFirstRun);
int menuPowerOff(uiEvent_t *event, bool isFirstRun);
int menuFirmwareInfoScreen(uiEvent_t *event, bool isFirstRun);
int menuNumericalEntry(uiEvent_t *event, bool isFirstRun);
int menuTxScreen(uiEvent_t *event, bool isFirstRun);
int menuRSSIScreen(uiEvent_t *event, bool isFirstRun);
int menuLastHeard(uiEvent_t *event, bool isFirstRun);
int menuOptions(uiEvent_t *event, bool isFirstRun);
int menuDisplayOptions(uiEvent_t *event, bool isFirstRun);
int menuCredits(uiEvent_t *event, bool isFirstRun);
int menuChannelDetails(uiEvent_t *event, bool isFirstRun);
int menuHotspotMode(uiEvent_t *event, bool isFirstRun);
int menuCPS(uiEvent_t *event, bool isFirstRun);
int menuLockScreen(uiEvent_t *event, bool isFirstRun);
int menuContactList(uiEvent_t *event, bool isFirstRun);
int menuContactListSubMenu(uiEvent_t *event, bool isFirstRun);
int menuContactDetails(uiEvent_t *event, bool isFirstRun);
int menuLanguage(uiEvent_t *event, bool isFirstRun);
int menuPrivateCall(uiEvent_t *event, bool isFirstRun);

#endif
