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

#include "menu/menuSystem.h"
#include "menu/menuUtilityQSOData.h"
#include "fw_trx.h"
#include "fw_codeplug.h"
#include "fw_settings.h"

static void updateScreen();
static void handleEvent(int buttons, int keys, int events);
static const int CTCSS_TONE_NONE = 65535;
static const unsigned int CTCSSTones[]={65535,625,670,693,719,744,770,797,825,854,
										885,915,948,974,1000,1035,1072,1109,1148,
										1188,1230,1273,1318,1365,1413,1462,1514,
										1567,1598,1622,1655,1679,1713,1738,1773,
										1799,1835,1862,1899,1928,1966,1995,2035,
										2065,2107,2181,2257,2291,2336,2418,2503,2541};
static int NUM_CTCSS=52;
static int CTCSSRxIndex=0;
static int CTCSSTxIndex=0;
static int NUM_MENUS=5;
static struct_codeplugChannel_t tmpChannel;// update a temporary copy of the channel and only write back if green menu is pressed

int menuChannelDetails(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		memcpy(&tmpChannel,currentChannelData,sizeof(struct_codeplugChannel_t));
		gMenusCurrentItemIndex=0;
		for(int i=0;i<NUM_CTCSS;i++)
		{
			if (tmpChannel.txTone==CTCSSTones[i])
			{
				CTCSSTxIndex=i;
			}
			if (tmpChannel.rxTone==CTCSSTones[i])
			{
				CTCSSRxIndex=i;
			}
		}
		updateScreen();
	}
	else
	{
		if (events!=0 && keys!=0)
		{
			handleEvent(buttons, keys, events);
		}
	}
	return 0;
}

static void updateScreen()
{
	int mNum=0;
	char buf[17];
	UC1701_clearBuf();
	UC1701_printCentered(0, "Channel info",UC1701_FONT_GD77_8x16);

	// Can only display 3 of the options at a time menu at -1, 0 and +1
	for(int i=-1;i<=1;i++)
	{
		mNum = gMenusCurrentItemIndex+i;
		if (mNum<0)
		{
			mNum = NUM_MENUS + mNum;
		}
		if (mNum >= NUM_MENUS)
		{
			mNum = mNum - NUM_MENUS;
		}

		switch(mNum)
		{
			case 0:
				if (trxGetMode()==RADIO_MODE_ANALOG)
				{
					strcpy(buf,"Color Code:N/A");
				}
				else
				{
					sprintf(buf,"Color Code:%d",tmpChannel.rxColor);
				}
				break;
			case 1:
				if (trxGetMode()==RADIO_MODE_ANALOG)
				{
					strcpy(buf,"Timeslot:N/A");
				}
				else
				{
					sprintf(buf,"Timeslot:%d",((tmpChannel.flag2 & 0x40) >> 6) + 1);
				}
				break;
			case 2:
				if (trxGetMode()==RADIO_MODE_ANALOG)
				{
					if (tmpChannel.txTone==CTCSS_TONE_NONE)
					{
						strcpy(buf,"Tx CTCSS:None");
					}
					else
					{
						sprintf(buf,"Tx CTCSS:%d.%dHz",tmpChannel.txTone/10 ,tmpChannel.txTone%10 );
					}
				}
				else
				{
					strcpy(buf,"Tx CTCSS:N/A");
				}
				break;
			case 3:
				if (trxGetMode()==RADIO_MODE_ANALOG)
				{
					if (tmpChannel.rxTone==CTCSS_TONE_NONE)
					{
						strcpy(buf,"Rx CTCSS:None");
					}
					else
					{
						sprintf(buf,"Rx CTCSS:%d.%dHz",tmpChannel.rxTone/10 ,tmpChannel.rxTone%10 );
					}
				}
				else
				{
					strcpy(buf,"Rx CTCSS:N/A");
				}
				break;
			case 4:
				// Bandwidth
				if (trxGetMode()==RADIO_MODE_DIGITAL)
				{
					strcpy(buf,"Bandwidth:N/A");
				}
				else
				{
					sprintf(buf,"Bandwidth:%s",((tmpChannel.flag4 & 0x02) == 0x02)?"25kHz":"12.5kHz");
				}
				break;
		}

		if (gMenusCurrentItemIndex==mNum)
		{
			UC1701_fillRect(0,(i+2)*16,128,16,false);
		}

		UC1701_printCore(0,(i+2)*16,buf,UC1701_FONT_GD77_8x16,0,(gMenusCurrentItemIndex==mNum));
	}

	UC1701_render();
	displayLightTrigger();
}

static void handleEvent(int buttons, int keys, int events)
{
	if ((keys & KEY_DOWN)!=0)
	{
		gMenusCurrentItemIndex++;
		if (gMenusCurrentItemIndex>=NUM_MENUS)
		{
			gMenusCurrentItemIndex=0;
		}
	}
	else if ((keys & KEY_UP)!=0)
	{
		gMenusCurrentItemIndex--;
		if (gMenusCurrentItemIndex<0)
		{
			gMenusCurrentItemIndex=NUM_MENUS-1;
		}
	}
	else if ((keys & KEY_RIGHT)!=0)
	{
		switch(gMenusCurrentItemIndex)
		{
			case 0:
				if (tmpChannel.rxColor<15)
				{
					tmpChannel.rxColor++;
					trxSetDMRColourCode(tmpChannel.rxColor);
				}
				break;
			case 1:
				tmpChannel.flag2 |= 0x40;// set TS 2 bit
				break;
			case 2:
				if (trxGetMode()==RADIO_MODE_ANALOG)
				{
					CTCSSTxIndex++;
					if (CTCSSTxIndex>=NUM_CTCSS)
					{
						CTCSSTxIndex=NUM_CTCSS-1;
					}
					tmpChannel.txTone=CTCSSTones[CTCSSTxIndex];
					trxSetTxCTCSS(tmpChannel.txTone);
				}
				break;
			case 3:
				if (trxGetMode()==RADIO_MODE_ANALOG)
				{
					CTCSSRxIndex++;
					if (CTCSSRxIndex>=NUM_CTCSS)
					{
						CTCSSRxIndex=NUM_CTCSS-1;
					}
					tmpChannel.rxTone=CTCSSTones[CTCSSRxIndex];
					trxSetRxCTCSS(tmpChannel.rxTone);
				}
				break;
			case 4:
				tmpChannel.flag4 |= 0x02;// set 25kHz bit
				break;
		}
	}
	else if ((keys & KEY_LEFT)!=0)
	{
		switch(gMenusCurrentItemIndex)
		{
			case 0:
				if (tmpChannel.rxColor>0)
				{
					tmpChannel.rxColor--;
					trxSetDMRColourCode(tmpChannel.rxColor);
				}
				break;
			case 1:
				tmpChannel.flag2 &= 0xBF;// Clear TS 2 bit
				break;
			case 2:
				if (trxGetMode()==RADIO_MODE_ANALOG)
				{
					CTCSSTxIndex--;
					if (CTCSSTxIndex < 0)
					{
						CTCSSTxIndex=0;
					}
					tmpChannel.txTone=CTCSSTones[CTCSSTxIndex];
					trxSetTxCTCSS(tmpChannel.txTone);
				}
				break;
			case 3:
				if (trxGetMode()==RADIO_MODE_ANALOG)
				{
					CTCSSRxIndex--;
					if (CTCSSRxIndex < 0)
					{
						CTCSSRxIndex=0;
					}
					tmpChannel.rxTone=CTCSSTones[CTCSSRxIndex];
					trxSetRxCTCSS(tmpChannel.rxTone);
				}
				break;
			case 4:
				tmpChannel.flag4 &= ~0x02;// clear 25kHz bit
				break;
		}
	}
	else if ((keys & KEY_GREEN)!=0)
	{
		memcpy(currentChannelData,&tmpChannel,sizeof(struct_codeplugChannel_t));
		menuSystemPopAllAndDisplayRootMenu();
		return;
	}
	else if ((keys & KEY_RED)!=0)
	{
		menuSystemPopPreviousMenu();
		return;
	}

	updateScreen();
}
