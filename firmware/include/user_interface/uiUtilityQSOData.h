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
#ifndef _MENU_UTILITY_QSO_DATA_H_
#define _MENU_UTILITY_QSO_DATA_H_                    /**< Symbol preventing repeated inclusion */
#include <user_interface/menuSystem.h>
#include "fw_common.h"

#define NUM_LASTHEARD_STORED 16
extern const int QSO_TIMER_TIMEOUT;
extern const int TX_TIMER_Y_OFFSET;
extern const int CONTACT_Y_POS;
extern const int FREQUENCY_X_POS;

typedef struct dmrIdDataStruct
{
	int id;
	char text[20];
} dmrIdDataStruct_t;


typedef struct LinkItem
{
    struct LinkItem *prev;
    uint32_t 	id;
    uint32_t 	talkGroupOrPcId;
    char 		talkerAlias[32];// 4 blocks of data. 6 bytes + 7 bytes + 7 bytes + 7 bytes . plus 1 for termination some more for safety.
    char 		locator[7];
    uint32_t	time;// current system time when this station was heard
    struct LinkItem *next;
} LinkItem_t;

enum QSO_DISPLAY_STATE
{
	QSO_DISPLAY_IDLE,
	QSO_DISPLAY_DEFAULT_SCREEN,
	QSO_DISPLAY_CALLER_DATA,
	QSO_DISPLAY_CALLER_DATA_UPDATE
};

extern const char *POWER_LEVELS[];
extern const char *DMR_FILTER_LEVELS[];
extern LinkItem_t *LinkHead;
extern int menuDisplayQSODataState;
extern int qsodata_timer;
extern uint32_t menuUtilityReceivedPcId;
extern uint32_t menuUtilityTgBeforePcMode;
extern const uint32_t RSSI_UPDATE_COUNTER_RELOAD;

char *chomp(char *str);
int32_t getCallsignEndingPos(char *str);
void printSplitOrSpanText(uint8_t y, char *text, bool smallFont);
bool dmrIDLookup(int targetId, dmrIdDataStruct_t *foundRecord);
bool contactIDLookup(uint32_t id, int calltype, char *buffer);
void menuUtilityRenderQSOData(void);
void menuUtilityRenderHeader(void);
void lastheardInitList(void);
bool lastHeardListUpdate(uint8_t *dmrDataBuffer);
bool menuUtilityHandlePrivateCallActions(uiEvent_t *ev);
void lastHeardClearLastID(void);
void drawRSSIBarGraph(void);
void drawDMRMicLevelBarGraph(void);
void setOverrideTGorPC(int tgOrPc, bool privateCall);
char keypressToNumberChar(keyboardCode_t keys);
void printFrequency(bool isTX, bool hasFocus, uint8_t y, uint32_t frequency, bool displayVFOChannel);
void printToneAndSquelch(void);

#endif
