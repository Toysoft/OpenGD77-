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
#ifndef _FW_MENU_LEGACY_CODEPLUG_UTILS_H_
#define _FW_MENU_LEGACY_CODEPLUG_UTILS_H_

#include "fw_common.h"

extern const int CODEPLUG_MAX_VARIABLE_SQUELCH;
extern const int CODEPLUG_MIN_VARIABLE_SQUELCH;
extern const int CODEPLUG_ZONE_DATA_SIZE;
extern const int VFO_FREQ_STEP_TABLE[8];

extern int codeplugChannelsPerZone;

enum CONTACT_CALLTYPE_SELECT { CONTACT_CALLTYPE_TG=0, CONTACT_CALLTYPE_PC, CONTACT_CALLTYPE_ALL };

typedef struct struct_codeplugZone
{
	char name[16];
	uint16_t channels[80];// 16 for the original codeplug, but set this to  80 to allow for the new codeplug zones format
	int	NOT_IN_MEMORY_numChannelsInZone;// This property is not part of the codeplug data, its initialised by the code
	int	NOT_IN_MEMORY_isAllChannelsZone;// This property is not part of the codeplug data, its initialised by the code
}
struct_codeplugZone_t;

typedef struct struct_codeplugChannel
{
	char name[16];
	uint32_t rxFreq;
	uint32_t txFreq;
	uint8_t chMode;
	uint8_t rxRefFreq;
	uint8_t txRefFreq;
	uint8_t tot;
	uint8_t totRekey;
	uint8_t admitCriteria;
	uint8_t rssiThreshold;
	uint8_t scanList;
	uint16_t rxTone;
	uint16_t txTone;
	uint8_t voiceEmphasis;
	uint8_t txSignaling;
	uint8_t unmuteRule;
	uint8_t rxSignaling;
	uint8_t artsInterval;
	uint8_t encrypt;
	uint8_t rxColor;
	uint8_t rxGroupList;
	uint8_t txColor;
	uint8_t emgSystem;
	uint16_t contact;
	uint8_t flag1;
	uint8_t flag2;
	uint8_t flag3;
	uint8_t flag4;// bits... 0x80 = Power, 0x40 = Vox, 0x20 = AutoScan, 0x10 = LoneWoker, 0x08 = AllowTalkaround, 0x04 = OnlyRx, 0x02 = Channel width, 0x01 = Squelch
	uint16_t VFOoffsetFreq;
	uint8_t VFOflag5;// upper 4 bits are the step frequency 2.5,5,6.25,10,12.5,25,30,50kHz
	uint8_t sql;// Does not seem to be used in the official firmware and seems to be always set to 0
} struct_codeplugChannel_t;

typedef struct struct_codeplugRxGroup
{
	char name[16];
	uint16_t contacts[32];
	int	NOT_IN_MEMORY_numTGsInGroup;// NOT IN THE
} struct_codeplugRxGroup_t;

typedef struct struct_codeplugContact
{
	char 		name[16];
	uint32_t 	tgNumber;
	uint8_t		callType;
	uint8_t		callRxTone;
	uint8_t		ringStyle;
	uint8_t		reserve1;
	int         NOT_IN_CODEPLUGDATA_indexNumber;
} struct_codeplugContact_t;

typedef struct struct_codeplugDTMFContact
{
	char name[16];
	uint8_t code[16];
} struct_codeplugDTMFContact_t;

typedef struct struct_codeplugDTMFContactList
{
	struct_codeplugDTMFContact_t contacts[32];
	int numContacts;
} struct_codeplugDTMFContactList_t;

typedef enum { CODEPLUG_CUSTOM_DATA_TYPE_NONE = 0, CODEPLUG_CUSTOM_DATA_TYPE_IMAGE = 1, CODEPLUG_CUSTOM_DATA_TYPE_BEEP = 2 } codeplugCustomDataType_t;

/*
 * deprecated. Use our own non volatile storage instead
 *
void codeplugZoneGetSelected(int *selectedZone,int *selectedChannel);
void codeplugZoneSetSelected(int selectedZone,int selectedChannel);
 */
int codeplugZonesGetCount(void);
void codeplugZoneGetDataForNumber(int indexNum,struct_codeplugZone_t *returnBuf);
void codeplugChannelGetDataForIndex(int index, struct_codeplugChannel_t *channelBuf);
void codeplugUtilConvertBufToString(char *inBuf,char *outBuf,int len);
void codeplugUtilConvertStringToBuf(char *inBuf,char *outBuf,int len);
uint32_t byteSwap32(uint32_t n);
uint32_t bcd2int(uint32_t in);
int int2bcd(int i);

void codeplugRxGroupGetDataForIndex(int index, struct_codeplugRxGroup_t *rxGroupBuf);
bool codeplugContactGetDataForIndex(int index, struct_codeplugContact_t *contact);
void codeplugDTMFContactGetDataForIndex(struct_codeplugDTMFContactList_t *contactList);
int codeplugGetUserDMRID(void);
void codeplugSetUserDMRID(uint32_t dmrId);
void codeplugGetRadioName(char *buf);
void codeplugGetBootScreenData(char *line1, char *line2,uint8_t *displayType, uint8_t *passwordEnable, uint32_t *passwordNumber);
void codeplugGetVFO_ChannelData(struct_codeplugChannel_t *vfoBuf,int VFONumber);
void codeplugSetVFO_ChannelData(struct_codeplugChannel_t *vfoBuf,int VFONumber);
bool codeplugChannelIndexIsValid(int index);
void codeplugChannelIndexSetValid(int index);
bool codeplugChannelSaveDataForIndex(int index, struct_codeplugChannel_t *channelBuf);

int codeplugContactsGetCount(int callType);
int codeplugContactGetDataForNumber(int number, int callType, struct_codeplugContact_t *contact);
int codeplugContactIndexByTGorPC(int tgorpc, int callType, struct_codeplugContact_t *contact);
int codeplugContactSaveDataForIndex(int index, struct_codeplugContact_t *contact);
int codeplugContactGetFreeIndex(void);
bool codeplugContactGetRXGroup(int index);
void codeplugInitChannelsPerZone(void);
bool codeplugGetOpenGD77CustomData(codeplugCustomDataType_t dataType,uint8_t *dataBuf);
int codeplugGetQuickkeyFunctionID(int key);
void codeplugInitContactsCache(void);
bool codeplugContactsContainsPC(uint32_t pc);

#endif
