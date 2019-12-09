/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
 * 				and	 Colin Durbridge, G4EML
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
#include <user_interface/menuUtilityQSOData.h>
#include <user_interface/uiLocalisation.h>
#include "fw_trx.h"
#include "fw_codeplug.h"
#include "fw_settings.h"

static void updateScreen(void);
static void handleEvent(ui_event_t *ev);

static int CTCSSRxIndex=0;
static int CTCSSTxIndex=0;
static struct_codeplugChannel_t tmpChannel;// update a temporary copy of the channel and only write back if green menu is pressed

enum CHANNEL_DETAILS_DISPLAY_LIST { CH_DETAILS_MODE = 0, CH_DETAILS_DMR_CC, CH_DETAILS_DMR_TS,CH_DETAILS_RXCTCSS, CH_DETAILS_TXCTCSS , CH_DETAILS_RXFREQ, CH_DETAILS_TXFREQ, CH_DETAILS_BANDWIDTH,
									CH_DETAILS_FREQ_STEP, CH_DETAILS_TOT, CH_DETAILS_ZONE_SKIP,CH_DETAILS_ALL_SKIP,CH_DETAILS_RXGROUP,
									NUM_CH_DETAILS_ITEMS};// The last item in the list is used so that we automatically get a total number of items in the list

int menuChannelDetails(ui_event_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		memcpy(&tmpChannel,currentChannelData,sizeof(struct_codeplugChannel_t));

		for(int i=0;i<TRX_NUM_CTCSS;i++)
		{
			if (tmpChannel.txTone==TRX_CTCSSTones[i])
			{
				CTCSSTxIndex=i;
			}
			if (tmpChannel.rxTone==TRX_CTCSSTones[i])
			{
				CTCSSRxIndex=i;
			}
		}
		updateScreen();
	}
	else
	{
		if (ev->hasEvent)
			handleEvent(ev);
	}
	return 0;
}

static void updateScreen(void)
{
	int mNum = 0;
	static const int bufferLen = 17;
	char buf[bufferLen];
	int tmpVal;
	int val_before_dp;
	int val_after_dp;
	struct_codeplugRxGroup_t rxGroupBuf;
	char rxNameBuf[bufferLen];

	UC1701_clearBuf();
	menuDisplayTitle(currentLanguage->channel_details);

	// Can only display 3 of the options at a time menu at -1, 0 and +1
	for(int i = -1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_CH_DETAILS_ITEMS, i);
		buf[0] = 0;

		switch(mNum)
		{
			case CH_DETAILS_MODE:
				if (tmpChannel.chMode == RADIO_MODE_ANALOG)
				{
					snprintf(buf, bufferLen, "%s:FM", currentLanguage->mode);
				}
				else
				{
					snprintf(buf, bufferLen, "%s:DMR", currentLanguage->mode);
				}
				break;
			break;
			case CH_DETAILS_DMR_CC:
				if (tmpChannel.chMode == RADIO_MODE_ANALOG)
				{
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->colour_code, currentLanguage->n_a);
				}
				else
				{
					snprintf(buf, bufferLen, "%s:%d", currentLanguage->colour_code, tmpChannel.rxColor);
				}
				break;
			case CH_DETAILS_DMR_TS:
				if (tmpChannel.chMode == RADIO_MODE_ANALOG)
				{
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->timeSlot, currentLanguage->n_a);
				}
				else
				{
					snprintf(buf, bufferLen, "%s:%d", currentLanguage->timeSlot, ((tmpChannel.flag2 & 0x40) >> 6) + 1);
				}
				break;
			case CH_DETAILS_RXCTCSS:
				if (tmpChannel.chMode == RADIO_MODE_ANALOG)
				{
					if (tmpChannel.txTone == TRX_CTCSS_TONE_NONE)
					{
						snprintf(buf, bufferLen, "Tx CTCSS:%s", currentLanguage->none);
					}
					else
					{
						snprintf(buf, bufferLen, "Tx CTCSS:%d.%dHz", tmpChannel.txTone / 10 , tmpChannel.txTone % 10);
					}
				}
				else
				{
					snprintf(buf, bufferLen, "Tx CTCSS:%s", currentLanguage->n_a);
				}
				break;
			case CH_DETAILS_TXCTCSS:
				if (tmpChannel.chMode == RADIO_MODE_ANALOG)
				{
					if (tmpChannel.rxTone == TRX_CTCSS_TONE_NONE)
					{
						snprintf(buf, bufferLen, "Rx CTCSS:%s", currentLanguage->none);
					}
					else
					{
						snprintf(buf, bufferLen, "Rx CTCSS:%d.%dHz", tmpChannel.rxTone / 10 , tmpChannel.rxTone % 10);
					}
				}
				else
				{
					snprintf(buf, bufferLen, "Rx CTCSS:%s", currentLanguage->n_a);
				}
				break;
			case CH_DETAILS_RXFREQ:
				val_before_dp = tmpChannel.rxFreq / 100000;
				val_after_dp = tmpChannel.rxFreq - val_before_dp * 100000;
				snprintf(buf, bufferLen, "Rx:%d.%05dMHz", val_before_dp, val_after_dp);
				break;
			case CH_DETAILS_TXFREQ:
				val_before_dp = tmpChannel.txFreq / 100000;
				val_after_dp = tmpChannel.txFreq - val_before_dp * 100000;
				snprintf(buf, bufferLen, "Tx:%d.%05dMHz", val_before_dp, val_after_dp);
				break;
			case CH_DETAILS_BANDWIDTH:
				// Bandwidth
				if (tmpChannel.chMode == RADIO_MODE_DIGITAL)
				{
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->bandwidth, currentLanguage->n_a);
				}
				else
				{
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->bandwidth, ((tmpChannel.flag4 & 0x02) == 0x02) ? "25kHz" : "12.5kHz");
				}
				break;
			case CH_DETAILS_FREQ_STEP:
					tmpVal = VFO_FREQ_STEP_TABLE[(tmpChannel.VFOflag5 >> 4)] / 100;
					snprintf(buf, bufferLen, "%s:%d.%02dkHz", currentLanguage->stepFreq, tmpVal, VFO_FREQ_STEP_TABLE[(tmpChannel.VFOflag5 >> 4)] - (tmpVal * 100));
				break;
			case CH_DETAILS_TOT:// TOT
				if (tmpChannel.tot != 0)
				{
					snprintf(buf, bufferLen, "%s:%d", currentLanguage->tot, tmpChannel.tot * 15);
				}
				else
				{
					snprintf(buf, bufferLen, "%s:%s",currentLanguage->tot, currentLanguage->off);
				}
				break;
			case CH_DETAILS_ZONE_SKIP:						// Zone Scan Skip Channel (Using CPS Auto Scan flag)
				snprintf(buf, bufferLen, "%s:%s", currentLanguage->zone_skip, ((tmpChannel.flag4 & 0x20) == 0x20) ? currentLanguage->yes : currentLanguage->no);
				break;
 			case CH_DETAILS_ALL_SKIP:					// All Scan Skip Channel (Using CPS Lone Worker flag)
				snprintf(buf, bufferLen, "%s:%s", currentLanguage->all_skip, ((tmpChannel.flag4 & 0x10) == 0x10) ? currentLanguage->yes : currentLanguage->no);
				break;
				
			case CH_DETAILS_RXGROUP:
				codeplugRxGroupGetDataForIndex(tmpChannel.rxGroupList, &rxGroupBuf);
				codeplugUtilConvertBufToString(rxGroupBuf.name, rxNameBuf, 16);
				snprintf(buf, bufferLen, "%s:%s", currentLanguage->rx_group, rxNameBuf);
				break;
		}

		buf[bufferLen - 1] = 0;
		menuDisplayEntry(i, mNum, buf);
	}

	UC1701_render();
	displayLightTrigger();
}

static void handleEvent(ui_event_t *ev)
{
	int tmpVal;
	struct_codeplugRxGroup_t rxGroupBuf;

	if (KEYCHECK_PRESS(ev->keys,KEY_DOWN))
	{
		MENU_INC(gMenusCurrentItemIndex, NUM_CH_DETAILS_ITEMS);
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_UP))
	{
		MENU_DEC(gMenusCurrentItemIndex, NUM_CH_DETAILS_ITEMS);
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_RIGHT))
	{
		switch(gMenusCurrentItemIndex)
		{
			case CH_DETAILS_MODE:
				if (tmpChannel.chMode ==RADIO_MODE_ANALOG)
				{
					tmpChannel.chMode = RADIO_MODE_DIGITAL;
				}
				else
				{
					tmpChannel.chMode = RADIO_MODE_ANALOG;
				}
				break;
			case CH_DETAILS_DMR_CC:
				if (tmpChannel.rxColor<15)
				{
					tmpChannel.rxColor++;
					trxSetDMRColourCode(tmpChannel.rxColor);
				}
				break;
			case CH_DETAILS_DMR_TS:
				tmpChannel.flag2 |= 0x40;// set TS 2 bit
				break;
			case CH_DETAILS_RXCTCSS:
				if (tmpChannel.chMode==RADIO_MODE_ANALOG)
				{
					CTCSSTxIndex++;
					if (CTCSSTxIndex>=TRX_NUM_CTCSS)
					{
						CTCSSTxIndex=TRX_NUM_CTCSS-1;
					}
					tmpChannel.txTone=TRX_CTCSSTones[CTCSSTxIndex];
					trxSetTxCTCSS(tmpChannel.txTone);
				}
				break;
			case CH_DETAILS_TXCTCSS:
				if (tmpChannel.chMode==RADIO_MODE_ANALOG)
				{
					CTCSSRxIndex++;
					if (CTCSSRxIndex>=TRX_NUM_CTCSS)
					{
						CTCSSRxIndex=TRX_NUM_CTCSS-1;
					}
					tmpChannel.rxTone=TRX_CTCSSTones[CTCSSRxIndex];
					trxSetRxCTCSS(tmpChannel.rxTone);
				}
				break;
			case CH_DETAILS_BANDWIDTH:
				tmpChannel.flag4 |= 0x02;// set 25kHz bit
				break;
			case CH_DETAILS_FREQ_STEP:
				tmpVal = (tmpChannel.VFOflag5>>4)+1;
				if (tmpVal>7)
				{
					tmpVal=7;
				}
				tmpChannel.VFOflag5 &= 0x0F;
				tmpChannel.VFOflag5 |= tmpVal<<4;
				break;
			case CH_DETAILS_TOT:
				if (tmpChannel.tot<255)
				{
					tmpChannel.tot++;
				}
				break;
			case CH_DETAILS_ZONE_SKIP:
				tmpChannel.flag4 |= 0x20;// set Channel Zone Skip bit (was Auto Scan)
				break;
			case CH_DETAILS_ALL_SKIP:
				tmpChannel.flag4 |= 0x10;// set Channel All Skip bit (was Lone Worker)
				break;				
			case CH_DETAILS_RXGROUP:
				tmpVal = tmpChannel.rxGroupList;
				tmpVal++;
				while (tmpVal < 76) {
					codeplugRxGroupGetDataForIndex(tmpVal, &rxGroupBuf);
					if (rxGroupBuf.name[0] != 0) {
						tmpChannel.rxGroupList = tmpVal;
						break;
					}
					tmpVal++;
				}
				break;
		}
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_LEFT))
	{
		switch(gMenusCurrentItemIndex)
		{
			case CH_DETAILS_MODE:
				if (tmpChannel.chMode==RADIO_MODE_ANALOG)
				{
					tmpChannel.chMode = RADIO_MODE_DIGITAL;
				}
				else
				{
					tmpChannel.chMode = RADIO_MODE_ANALOG;
				}
				break;
			case CH_DETAILS_DMR_CC:
				if (tmpChannel.rxColor>0)
				{
					tmpChannel.rxColor--;
					trxSetDMRColourCode(tmpChannel.rxColor);
				}
				break;
			case CH_DETAILS_DMR_TS:
				tmpChannel.flag2 &= 0xBF;// Clear TS 2 bit
				break;
			case CH_DETAILS_RXCTCSS:
				if (tmpChannel.chMode == RADIO_MODE_ANALOG)
				{
					CTCSSTxIndex--;
					if (CTCSSTxIndex < 0)
					{
						CTCSSTxIndex=0;
					}
					tmpChannel.txTone=TRX_CTCSSTones[CTCSSTxIndex];
					trxSetTxCTCSS(tmpChannel.txTone);
				}
				break;
			case CH_DETAILS_TXCTCSS:
				if (tmpChannel.chMode == RADIO_MODE_ANALOG)
				{
					CTCSSRxIndex--;
					if (CTCSSRxIndex < 0)
					{
						CTCSSRxIndex=0;
					}
					tmpChannel.rxTone=TRX_CTCSSTones[CTCSSRxIndex];
					trxSetRxCTCSS(tmpChannel.rxTone);
				}
				break;
			case CH_DETAILS_BANDWIDTH:
				tmpChannel.flag4 &= ~0x02;// clear 25kHz bit
				break;
			case CH_DETAILS_FREQ_STEP:
				tmpVal = (tmpChannel.VFOflag5>>4)-1;
				if (tmpVal<0)
				{
					tmpVal=0;
				}
				tmpChannel.VFOflag5 &= 0x0F;
				tmpChannel.VFOflag5 |= tmpVal<<4;
				break;
			case CH_DETAILS_TOT:
				if (tmpChannel.tot>0)
				{
					tmpChannel.tot--;
				}
				break;
			case CH_DETAILS_ZONE_SKIP:
				tmpChannel.flag4 &= ~0x20;// clear Channel Zone Skip Bit (was Auto Scan bit)
				break;
			case CH_DETAILS_ALL_SKIP:
				tmpChannel.flag4 &= ~0x10;// clear Channel All Skip Bit (was Lone Worker bit)
				break;				
			case CH_DETAILS_RXGROUP:
					tmpVal = tmpChannel.rxGroupList;
					tmpVal--;
					while (tmpVal > 0) {
						codeplugRxGroupGetDataForIndex(tmpVal, &rxGroupBuf);
						if (rxGroupBuf.name[0] != 0) {
							tmpChannel.rxGroupList = tmpVal;
							break;
						}
						tmpVal--;
					}
				break;

		}
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_GREEN))
	{
		memcpy(currentChannelData,&tmpChannel,sizeof(struct_codeplugChannel_t));

		// settingsCurrentChannelNumber is -1 when in VFO mode
		// But the VFO is stored in the nonVolatile settings, and not saved back to the codeplug
		if (settingsCurrentChannelNumber != -1 )
		{
			codeplugChannelSaveDataForIndex(settingsCurrentChannelNumber,currentChannelData);
		}

		menuSystemPopAllAndDisplayRootMenu();
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_RED))
	{
		menuSystemPopPreviousMenu();
		return;
	}

	updateScreen();
}
