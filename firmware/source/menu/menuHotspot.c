/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
 *
 * Using some code ported from MMDVM_HS by Andy CA6JAU
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
#include "fw_settings.h"
#include "fw_usb_com.h"
#include "fw_trx.h"
#include "fw_HR-C6000.h"
#include "dmr/DMRFullLC.h"

#include <SeggerRTT/RTT/SEGGER_RTT.h>


#define MMDVM_FRAME_START    0xE0

#define MMDVM_GET_VERSION   0x00
#define MMDVM_GET_STATUS    0x01
#define MMDVM_SET_CONFIG    0x02
#define MMDVM_SET_MODE      0x03
#define MMDVM_SET_FREQ      0x04
#define MMDVM_CAL_DATA      0x08
#define MMDVM_RSSI_DATA     0x09
#define MMDVM_SEND_CWID     0x0A

#define MMDVM_DMR_DATA1     0x18
#define MMDVM_DMR_LOST1     0x19
#define MMDVM_DMR_DATA2     0x1AU
#define MMDVM_DMR_LOST2     0x1BU
#define MMDVM_DMR_SHORTLC   0x1CU
#define MMDVM_DMR_START     0x1DU
#define MMDVM_DMR_ABORT     0x1EU

#define MMDVM_ACK           0x70U
#define MMDVM_NAK           0x7FU
#define MMDVM_SERIAL        0x80U
#define MMDVM_TRANSPARENT   0x90U
#define MMDVM_QSO_INFO      0x91U
#define MMDVM_DEBUG1        0xF1U
#define MMDVM_DEBUG2        0xF2U
#define MMDVM_DEBUG3        0xF3U
#define MMDVM_DEBUG4        0xF4U
#define MMDVM_DEBUG5        0xF5U
#define PROTOCOL_VERSION    1U

uint32_t freq_rx;
uint32_t freq_tx;
uint8_t rf_power;
uint32_t savedTGorPC;
int savedPower;

enum MMDVM_STATE {
  STATE_IDLE      = 0,
  STATE_DSTAR     = 1,
  STATE_DMR       = 2,
  STATE_YSF       = 3,
  STATE_P25       = 4,
  STATE_NXDN      = 5,
  STATE_POCSAG    = 6,

  // Dummy states start at 90
  STATE_DMRDMO1K  = 92,
  STATE_RSSICAL   = 96,
  STATE_CWID      = 97,
  STATE_DMRCAL    = 98,
  STATE_DSTARCAL  = 99,
  STATE_INTCAL    = 100,
  STATE_POCSAGCAL = 101
} modemState = STATE_IDLE;


static void updateScreen();
static void handleEvent(int buttons, int keys, int events);

volatile bool hotspotModeIsTransmitting = false;

static void sendACK(uint8_t* s_ComBuf)
{
//  SEGGER_RTT_printf(0, "sendACK\r\n");
  s_ComBuf[0U] = MMDVM_FRAME_START;
  s_ComBuf[1U] = 4U;
  s_ComBuf[2U] = MMDVM_ACK;
  s_ComBuf[3U] = com_requestbuffer[2U];
  USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, s_ComBuf, s_ComBuf[1]);
}

static void sendNAK(uint8_t* s_ComBuf,uint8_t err)
{
	SEGGER_RTT_printf(0, "sendNAK\r\n");
  s_ComBuf[0U] = MMDVM_FRAME_START;
  s_ComBuf[1U] = 5U;
  s_ComBuf[2U] = MMDVM_NAK;
  s_ComBuf[3U] = com_requestbuffer[2U];
  s_ComBuf[4U] = err;
  USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, s_ComBuf, s_ComBuf[1]);
}


static void enableTransmission()
{
	if (hotspotModeIsTransmitting || trxIsTransmitting)
	{
		return;
	}


	SEGGER_RTT_printf(0, "enableTransmission\n");
	// turn on the transmitter
	GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
	GPIO_PinWrite(GPIO_LEDred, Pin_LEDred, 1);

	trxSetFrequency(currentChannelData->txFreq);
	txstopdelay=0;
	trxIsTransmitting=true;
	hotspotModeIsTransmitting=true;
	trx_setTX();
}

const int TX_BUFFER_MIN_BEFORE_TRANSMISSION = 2;

static void storeNetFrame(uint8_t *com_requestbuffer)
{
	const uint8_t END_FRAME_PATTERN[] = {0x5D,0x7F,0x77,0xFD,0x75,0x79};

	if (memcmp((uint8_t *)&com_requestbuffer[18],END_FRAME_PATTERN,6)!=0)
	{
		SEGGER_RTT_printf(0, "storeNetFrame\n");
		memcpy((uint8_t *)wavbuffer[wavbuffer_read_idx],com_requestbuffer+4,13);//copy the first 13 bytes
		wavbuffer[wavbuffer_read_idx][13] = (com_requestbuffer[17] & 0xF0) | (com_requestbuffer[23] & 0x0F);
		memcpy((uint8_t *)&wavbuffer[wavbuffer_read_idx][14],(uint8_t *)&com_requestbuffer[24],13);//copy the last 13 bytes

		wavbuffer_count++;
		wavbuffer_write_idx++;
		if (wavbuffer_write_idx > (WAV_BUFFER_COUNT - 1))
		{
			wavbuffer_write_idx=0;
		}
	}
	else
	{
		SEGGER_RTT_printf(0, "End frame detected.  %d Frames in the buffer\n",wavbuffer_count);
	}
}

bool hotspotModeReceiveNetFrame(uint8_t *com_requestbuffer,uint8_t *s_ComBuf, int timeSlot)
{
	DMRLC_T lc;

	DMRFullLC_decode(com_requestbuffer + 4U, DT_VOICE_LC_HEADER,&lc);// Need to decode the frame to get the source and destination

	// can't start transmitting until we have a valid source and destination Id
	if (!hotspotModeIsTransmitting)
	{
		if 	(lc.srcId!=0 && lc.dstId!=0)
		{
			trxTalkGroupOrPcId  = lc.dstId;
			trxDMRID = lc.srcId;
			wavbuffer_read_idx=0;
			wavbuffer_write_idx=0;
			wavbuffer_count=0;
			SEGGER_RTT_printf(0, "Net frame FID:%d FLCO:%d PF:%d R:%d dstId:%d src:Id:%d options:0x%02x\n",lc.FID,lc.FLCO,lc.PF,lc.R,lc.dstId,lc.srcId,lc.options);
			storeNetFrame(com_requestbuffer);
			enableTransmission();
			sendACK(s_ComBuf);
			return true;
		}
	}
	else
	{
		storeNetFrame(com_requestbuffer);
	}

	sendACK(s_ComBuf);
	return true;

}


int menuHotspotMode(int buttons, int keys, int events, bool isFirstRun)
{

	if (isFirstRun)
	{
		savedPower = nonVolatileSettings.txPower;
		nonVolatileSettings.txPower=800;// set very low power for testing
		savedTGorPC = trxTalkGroupOrPcId;// Save the current TG or PC
		trxTalkGroupOrPcId=0;

		trxSetModeAndBandwidth(RADIO_MODE_DIGITAL,trxGetBandwidthIs25kHz());// hotspot mode is for DMR i.e Digital mode

#if false
		// Just testing the frame encode and decode functions
		const uint8_t frameData[] = {	0x0B,0xE6,0x08,0x92,0x13,0xE0,0x20,0xF8,
										0x12,0xD0,0x9A,0x21,0x44,0x6D,0x5D,0x3F,
										0x77,0xFD,0x35,0x7E,0x32,0xA8,0x03,0xC8,
										0x2C,0x70,0x80,0x21,0x6B,0x42,0xB1,0x83,
										0x15};
		uint8_t frameDataEncoded[33];
		memset(frameDataEncoded,0,33);// clear
		/*
		false
		false
		FLCO_GROUP (0x00000000)
		0x00 '\0'
		0x00 '\0'
		0x0023c52b
		0x000013ba
		*/
		DMRLC_T lc,lcEncodedDecoded;
		DMRFullLC_decode(frameData, DT_VOICE_LC_HEADER,&lc);
		DMRFullLC_encode(&lc,frameDataEncoded, DT_VOICE_LC_HEADER);
		DMRFullLC_decode(frameDataEncoded, DT_VOICE_LC_HEADER,&lcEncodedDecoded);
		// end of test code
#endif

		freq_rx = currentChannelData->rxFreq;
		freq_tx = currentChannelData->txFreq;
		settingsUsbMode = USB_MODE_HOTSPOT;
		updateScreen();
	}
	else
	{
		handleEvent(buttons, keys, events);
	}
	return 0;
}

static void updateScreen()
{
	int val_before_dp;
	int val_after_dp;

	char buffer[32];

	UC1701_clearBuf();
	UC1701_printCentered(0, "Hotspot mode",UC1701_FONT_GD77_8x16);

	val_before_dp = freq_rx/10000;

	val_after_dp = freq_rx - val_before_dp*10000;
	sprintf(buffer,"R %d.%04d MHz",val_before_dp, val_after_dp);
	UC1701_printCentered(32, buffer,UC1701_FONT_GD77_8x16);

	val_before_dp = freq_tx/10000;
	val_after_dp = freq_tx - val_before_dp*10000;
	sprintf(buffer,"T %d.%04d MHz",val_before_dp, val_after_dp);
	UC1701_printCentered(48, buffer,UC1701_FONT_GD77_8x16);

	UC1701_render();
	displayLightTrigger();
}

static void handleEvent(int buttons, int keys, int events)
{
	if ((keys & KEY_RED)!=0)
	{
		trxIsTransmitting = false;
		trx_deactivateTX();
		trx_setRX();
		trxTalkGroupOrPcId = savedTGorPC;// restore the current TG or PC
		nonVolatileSettings.txPower = savedPower;// restore power setting
		trxDMRID = codeplugGetUserDMRID();
		settingsUsbMode = USB_MODE_CPS;
		GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
		GPIO_PinWrite(GPIO_LEDred, Pin_LEDred, 1);
		menuSystemPopAllAndDisplayRootMenu();
		return;
	}

	if (!hotspotModeIsTransmitting && !trxIsTransmitting)
	{
		if ((buttons & BUTTON_PTT)!=0)
		{
			// To Do. Send dummy transmission so that the TG can be set on the network server.
		}
	}

	// Stop transmitting when there is no data in the buffer
	if (hotspotModeIsTransmitting == true && wavbuffer_count == 0)
	{
		if (trxIsTransmitting)
		{
			trxIsTransmitting=false;
		}

		if (txstopdelay>0)
		{
			txstopdelay--;
		}
		else
		{
			if ((slot_state < DMR_STATE_TX_START_1))
			{
				GPIO_PinWrite(GPIO_LEDred, Pin_LEDred, 0);
				trx_deactivateTX();
				trx_setRX();
				hotspotModeIsTransmitting = false;
			}
		}
	}
}


static uint8_t setFreq(const uint8_t* data, uint8_t length)
{
// satellite frequencies banned frequency ranges
const int BAN1_MIN  = 14580000;
const int BAN1_MAX  = 14600000;
const int BAN2_MIN  = 43500000;
const int BAN2_MAX  = 43800000;

	SEGGER_RTT_printf(0, "setConfig\r\n");

	// Very old MMDVMHost, set full power
	if (length == 9U)
	{
		rf_power = 255U;
	}
	// Current MMDVMHost, set power from MMDVM.ini
	if (length >= 10U)
	{
		rf_power = data[9U];// 255 = max power
	}

	freq_rx  = data[1U] << 0;
	freq_rx |= data[2U] << 8;
	freq_rx |= data[3U] << 16;
	freq_rx |= data[4U] << 24;
	freq_rx= freq_rx / 100;

	freq_tx  = data[5U] << 0;
	freq_tx |= data[6U] << 8;
	freq_tx |= data[7U] << 16;
	freq_tx |= data[8U] << 24;
	freq_tx=freq_tx / 100;



	if ((freq_tx>= BAN1_MIN && freq_tx <= BAN1_MAX) || (freq_tx>= BAN2_MIN && freq_tx <= BAN2_MAX))
	{
		return 4U;// invalid frequency
	}

	if (trxCheckFrequencyInAmateurBand(freq_rx) && trxCheckFrequencyInAmateurBand(freq_tx))
	{
		SEGGER_RTT_printf(0, "Tx freq = %d, Rx freq = %d, Power = %d\n",freq_tx,freq_rx,rf_power);
		trxSetFrequency(freq_rx);
	}
	else
	{
		return 4U;// invalid frequency
	}

  return 0x00;
}


static bool hasRXOverflow()
{
	return false;// TO DO.
}
static bool hasTXOverflow()
{
	return false;// TO DO.
}


static void getStatus(uint8_t* s_ComBuf)
{
//	SEGGER_RTT_printf(0, "getStatus\r\n");

  // Send all sorts of interesting internal values
	s_ComBuf[0U]  = MMDVM_FRAME_START;
	s_ComBuf[1U]  = 13U;
	s_ComBuf[2U]  = MMDVM_GET_STATUS;
	s_ComBuf[3U]  = 0x00U;
	s_ComBuf[3U] |= 0x02U;// DMR ENABLED
	s_ComBuf[4U]  = modemState;;
	s_ComBuf[5U]  = trxIsTransmitting  ? 0x01U : 0x00U;

	if (hasRXOverflow())
	{
		s_ComBuf[5U] |= 0x04U;
	}

	if (hasTXOverflow())
	{
		s_ComBuf[5U] |= 0x08U;
	}
	s_ComBuf[6U] = 	0U;// No DSTAR
	s_ComBuf[7U] = 	10U;
	s_ComBuf[8U] = 	WAV_BUFFER_COUNT - wavbuffer_count;
	s_ComBuf[9U] = 	0U;// No YSF
	s_ComBuf[10U] = 0U;// No P25
	s_ComBuf[11U] = 0U;// no NXDN
	s_ComBuf[12U] = 0U;// no POCSAG
	USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, s_ComBuf, s_ComBuf[1]);
}

static uint8_t setConfig(const uint8_t* data, uint8_t length)
{
	SEGGER_RTT_printf(0, "setConfig \r\n");

  uint8_t txDelay = data[2U];
  if (txDelay > 50U)
  {
	  return 4U;
  }

  if (data[3U] != STATE_IDLE && data[3U] != STATE_DMR)
  {
	  return 4U;// only DMR mode supported
  }
  modemState = data[3U];

  uint8_t colorCode = data[6U];
  if (colorCode > 15U)
  {
    return 4U;
  }

  /* To Do
  m_cwIdTXLevel = data[5U]>>2;
  uint8_t dmrTXLevel    = data[10U];
  io.setDeviations(dstarTXLevel, dmrTXLevel, ysfTXLevel, p25TXLevel, nxdnTXLevel, pocsagTXLevel, ysfLoDev);
  dmrDMOTX.setTXDelay(txDelay);
  dmrDMORX.setColorCode(colorCode);
   */
  return 0U;
}




uint8_t setMode(const uint8_t* data, uint8_t length)
{
	SEGGER_RTT_printf(0, "MMDVM SetMode len:%d %02X %02X %02X %02X %02X %02X %02X %02X\r\n",length,data[0U],data[1U],data[2U],data[3U],data[4U],data[5U],data[6U],data[7U]);

	if (modemState == data[0U])
	{
		return 0U;
	}

	// only supported mode is DMR (or idle)
	if  (data[0U] != STATE_DMR && data[0U] != STATE_IDLE)
	{
		return 4U;
	}

	// MMDVMHost seems to send setMode commands longer than 1 byte. This seems wrong according to the spec, so we ignore those.
	if (data[0U] == STATE_IDLE || (length==1 && data[0U] == STATE_DMR))
	{
		modemState = data[0U];
	}

	// MMDVHost on the PC seems to send mode DMR when the transmitter should be turned on and IDLE when it should be turned off.
	switch(modemState)
	{
		case STATE_IDLE:
			//enableTransmission(false);
			break;
		case STATE_DMR:
			//enableTransmission(true);
			break;
		default:
			break;
	}

  return 0U;
}



static void getVersion(uint8_t s_ComBuf[])
{
	SEGGER_RTT_printf(0, "getVersion\r\n");

	const char HOTSPOT_NAME[] = "OpenGD77";
	s_ComBuf[1]= 4 + strlen(HOTSPOT_NAME);// minus 1 because there is no terminator
	s_ComBuf[3]= PROTOCOL_VERSION;
	strcpy((char *)&s_ComBuf[4],HOTSPOT_NAME);
	USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, s_ComBuf, s_ComBuf[1]);
}



bool hotspotModeSend_RF_StartFrame()
{
	/*
 	for (int i=0;i<(0x0c+27);i++)
	{
    	SEGGER_RTT_printf(0, " %02x", DMR_frame_buffer[i]);
	}
	SEGGER_RTT_printf(0, "\r\n");
*/

	taskENTER_CRITICAL();

	memcpy((uint8_t *)com_buffer+3,DMR_frame_buffer,39);// 12 bytes Link Control + 27 bytes AMBE data
	taskEXIT_CRITICAL();

	com_buffer[0] = MMDVM_FRAME_START;
	com_buffer[1] = 39 + 3;
	com_buffer[2] = MMDVM_DMR_DATA2;
	USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, (uint8_t *)com_buffer, 39+3);
	return true;
}

bool hotspotModeSend_RF_AudioFrame()
{
	/*
 	for (int i=0;i<(0x0c+27);i++)
	{
    	SEGGER_RTT_printf(0, " %02x", DMR_frame_buffer[i]);
	}
	SEGGER_RTT_printf(0, "\r\n");
*/

	taskENTER_CRITICAL();

	memcpy((uint8_t *)com_buffer+3,DMR_frame_buffer,39);// 12 bytes Link Control + 27 bytes AMBE data
	taskEXIT_CRITICAL();

	com_buffer[0] = MMDVM_FRAME_START;
	com_buffer[1] = 39 + 3;
	com_buffer[2] = MMDVM_DMR_DATA2;
	USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, (uint8_t *)com_buffer, 39+3);
	return true;
}



static void handleDMRShortLC(uint8_t *com_requestbuffer,uint8_t *s_ComBuf)
{
	SEGGER_RTT_printf(0, "MMDVM ShortLC\n %02X %02X %02X %02X %02X %02X %02X %02X %02X \n",
							com_requestbuffer[0U],com_requestbuffer[1U],com_requestbuffer[2U],com_requestbuffer[3U],com_requestbuffer[4U],
							com_requestbuffer[5U],com_requestbuffer[6U],com_requestbuffer[7U],com_requestbuffer[8U]);

}


void handleHotspotRequest(uint8_t com_requestbuffer[],uint8_t s_ComBuf[])
{
	int err;

//	SEGGER_RTT_printf(0, "handleHotspotRequest 0x%0x 0x%0x 0x%0x\r\n",com_requestbuffer[0],com_requestbuffer[1],com_requestbuffer[2]);
	if (com_requestbuffer[0]==MMDVM_FRAME_START)
	{
		//SEGGER_RTT_printf(0, "command is %d\r\n",com_requestbuffer[0]);
		s_ComBuf[0] = com_requestbuffer[0];
		s_ComBuf[2] = com_requestbuffer[2];
		switch(com_requestbuffer[2])
		{
			case MMDVM_GET_VERSION:
				getVersion(s_ComBuf);
				break;
			case MMDVM_GET_STATUS:
				getStatus(s_ComBuf);
				break;

			case MMDVM_SET_CONFIG:
				err = setConfig(com_requestbuffer + 3U, s_ComBuf[1] - 3U);
				if (err == 0U)
				{
				  sendACK(s_ComBuf);
				}
				else
				{
				  sendNAK(s_ComBuf,err);
				}
				updateScreen();
				break;
			case MMDVM_SET_MODE:
				err = setMode(com_requestbuffer + 3U, s_ComBuf[1] - 3U);
				if (err == 0U)
				{
					sendACK(s_ComBuf);
				}
				else
				{
					sendNAK(s_ComBuf,err);
				}
				updateScreen();
				break;
			case MMDVM_SET_FREQ:
	            err = setFreq(com_requestbuffer + 3U, s_ComBuf[1] - 3U);
	            if (err == 0x00)
	            {
	              sendACK(s_ComBuf);
	            }
	            else
	            {
	              sendNAK(s_ComBuf,err);
	            }
	        	updateScreen();
				break;

			case MMDVM_CAL_DATA:
				SEGGER_RTT_printf(0, "MMDVM_CAL_DATA\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_RSSI_DATA:
				SEGGER_RTT_printf(0, "MMDVM_RSSI_DATA\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_SEND_CWID:
				SEGGER_RTT_printf(0, "MMDVM_SEND_CWID\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_DMR_DATA1:
				//SEGGER_RTT_printf(0, "MMDVM_DMR_DATA1\r\n");
				hotspotModeReceiveNetFrame(com_requestbuffer,s_ComBuf,1);

				break;
			case MMDVM_DMR_LOST1:
				SEGGER_RTT_printf(0, "MMDVM_DMR_LOST1\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_DMR_DATA2:
				//SEGGER_RTT_printf(0, "MMDVM_DMR_DATA2\r\n");
				hotspotModeReceiveNetFrame(com_requestbuffer,s_ComBuf,2);
				break;
			case MMDVM_DMR_LOST2:
				SEGGER_RTT_printf(0, "MMDVM_DMR_LOST2\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_DMR_SHORTLC:
				SEGGER_RTT_printf(0, "MMDVM_DMR_SHORTLC\r\n");
				handleDMRShortLC(com_requestbuffer + 3U,s_ComBuf);
				sendACK(s_ComBuf);
				break;
			case MMDVM_DMR_START:
				SEGGER_RTT_printf(0, "MMDVM_DMR_START\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_DMR_ABORT:
				SEGGER_RTT_printf(0, "MMDVM_DMR_ABORT\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_SERIAL:
				SEGGER_RTT_printf(0, "MMDVM_SERIAL\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_TRANSPARENT:
				SEGGER_RTT_printf(0, "MMDVM_TRANSPARENT\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_QSO_INFO:
				SEGGER_RTT_printf(0, "MMDVM_QSO_INFO\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_DEBUG1:
				SEGGER_RTT_printf(0, "MMDVM_DEBUG1\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_DEBUG2:
				SEGGER_RTT_printf(0, "MMDVM_DEBUG2\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_DEBUG3:
				SEGGER_RTT_printf(0, "MMDVM_DEBUG3\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_DEBUG4:
				SEGGER_RTT_printf(0, "MMDVM_DEBUG4\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_DEBUG5:
				SEGGER_RTT_printf(0, "MMDVM_DEBUG5\r\n");
				sendACK(s_ComBuf);
				break;
			default:
				SEGGER_RTT_printf(0, "Unhandled command type %d\n",com_requestbuffer[2]);
				sendACK(s_ComBuf);
				break;
		}
	}
	else
	{
		SEGGER_RTT_printf(0, "Invalid start code type %d\n",com_requestbuffer[0]);
		s_ComBuf[0] = '?';
		USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, s_ComBuf, 1);
	}
}

