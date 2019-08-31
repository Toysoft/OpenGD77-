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
#include <dmr/DMREmbeddedData.h>
#include "menu/menuSystem.h"
#include "menu/menuHotspot.h"
#include "fw_settings.h"
#include "fw_usb_com.h"
#include "fw_trx.h"
#include "fw_HR-C6000.h"
#include "dmr/DMRFullLC.h"
#include "dmr/DMRShortLC.h"
#include "dmr/DMRSlotType.h"
#include "dmr/QR1676.h"
#include <SeggerRTT/RTT/SEGGER_RTT.h>

#define MMDVM_FRAME_START   0xE0

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

#define MMDVM_HEADER_LENGTH  4U

const uint8_t MMDVM_VOICE_SYNC_PATTERN = 0x20U;

const int EMBEDDED_DATA_OFFSET = 13U;
const int TX_BUFFER_MIN_BEFORE_TRANSMISSION = 2;

const uint8_t START_FRAME_PATTERN[] = {0xFF,0x57,0xD7,0x5D,0xF5,0xD9};
const uint8_t END_FRAME_PATTERN[] 	= {0x5D,0x7F,0x77,0xFD,0x75,0x79};

uint32_t freq_rx;
uint32_t freq_tx;
uint8_t rf_power;
uint32_t savedTGorPC;
int savedPower;
uint32_t MMDVMHostGetStatusCount = 0;
uint8_t hotspotTxLC[9];

volatile int usbComSendBufWritePosition = 0;
volatile int usbComSendBufReadPosition = 0;
volatile int usbComSendBufCount = 0;
volatile usb_status_t lastUSBSerialTxStatus = kStatus_USB_Success;

volatile enum
{
	MMDVMHOST_RX_READY,
	MMDVMHOST_RX_BUSY,
	MMDVMHOST_RX_ERROR
} MMDVMHostRxState;

volatile enum MMDVM_STATE {
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

volatile enum { HOTSPOT_STATE_NOT_CONNECTED,
				HOTSPOT_STATE_INITIALISE,
				HOTSPOT_STATE_RX_START,
				HOTSPOT_STATE_RX_PROCESS,
				HOTSPOT_STATE_RX_END,
				HOTSPOT_STATE_TX_START_BUFFERING,
				HOTSPOT_STATE_TRANSMITTING,
				HOTSPOT_STATE_TX_SHUTDOWN } hotspotState;

const int USB_SERIAL_TX_RETRIES = 2;
const unsigned char VOICE_LC_SYNC_FULL[] 		= { 0x04U, 0x6DU, 0x5DU, 0x7FU, 0x77U, 0xFDU, 0x75U, 0x7EU, 0x30U};
const unsigned char TERMINATOR_LC_SYNC_FULL[]	= { 0x04U, 0xADU, 0x5DU, 0x7FU, 0x77U, 0xFDU, 0x75U, 0x79U, 0x60U};

const uint8_t LC_SYNC_MASK_FULL[]  = { 0x0FU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xF0U};


const uint8_t DMR_AUDIO_SEQ_SYNC[6][7] = {  {0x07U, 0xF0U, 0x00U, 0x00U, 0x00U, 0x0FU, 0xD0U},// seq 0 NOT USED AS THIS IS THE SYNC
											{0x01U, 0x30U, 0x00U, 0x00U, 0x00U, 0x09U, 0x10U},// seq 1
											{0x01U, 0x70U, 0x00U, 0x00U, 0x00U, 0x07U, 0x40U},// seq 2
											{0x01U, 0x70U, 0x00U, 0x00U, 0x00U, 0x07U, 0x40U},// seq 3
											{0x01U, 0x50U, 0x00U, 0x00U, 0x00U, 0x00U, 0x70U},// seq 4
											{0x01U, 0x10U, 0x00U, 0x00U, 0x00U, 0x0EU, 0x20U}};// seq 5

const uint8_t DMR_AUDIO_SEQ_MASK[]  = 		{0x0FU, 0xF0U, 0x00U, 0x00U, 0x00U, 0x0FU, 0xF0U};
const uint8_t DMR_EMBED_SEQ_MASK[]  = 		{0x00U, 0x0FU, 0xFFU, 0xFFU, 0xFFU, 0xF0U, 0x00U};

static void updateScreen();
static void handleEvent(int buttons, int keys, int events);
void handleHotspotRequest();

static void displayDataBytes(uint8_t *buf, int len)
{
	for (int i=0;i<len;i++)
	{
    	SEGGER_RTT_printf(0, " %02x", buf[i]);
	}
	SEGGER_RTT_printf(0, "\n");
}

// Queue system is a single byte header containing the length of the item, followed by the data
// if the block won't fit in the space between the current write location and the end of the buffer,
// a zero is written to the length for that block and the data and its length byte is put at the beginning of the buffer
static void enqueueUSBData(uint8_t *buff,int length)
{
	//SEGGER_RTT_printf(0, "Enqueue: ");
	//displayDataBytes(buff,length);
	if ((usbComSendBufWritePosition + length + 1) > (COM_BUFFER_SIZE - 1) )
	{
		//SEGGER_RTT_printf(0, "Looping write buffer back to start pos:%d len:%d\n");
		usbComSendBuf[usbComSendBufWritePosition]=0;// flag that the data block won't fit and will be put at the start of the buffer
		usbComSendBufWritePosition=0;
	}

	usbComSendBuf[usbComSendBufWritePosition]=length;
	memcpy(usbComSendBuf + usbComSendBufWritePosition +1,buff,length);
	usbComSendBufWritePosition += 1 + length;
	usbComSendBufCount++;
}

static void processUSBDataQueue()
{
	if (usbComSendBufCount!=0)
	{
		//SEGGER_RTT_printf(0, "Sending USB:");
		if (usbComSendBuf[usbComSendBufReadPosition]==0)
		{
			usbComSendBufReadPosition=0;
		}

		lastUSBSerialTxStatus = USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, &usbComSendBuf[usbComSendBufReadPosition+1],usbComSendBuf[usbComSendBufReadPosition]);
		if ( lastUSBSerialTxStatus == kStatus_USB_Success)
		{
			//SEGGER_RTT_printf(0, "Success\n");
			usbComSendBufReadPosition += usbComSendBuf[usbComSendBufReadPosition] + 1;
			if (usbComSendBufReadPosition > (COM_BUFFER_SIZE-1))
			{
				//SEGGER_RTT_printf(0, "Looping read buffer back to start\n");
				usbComSendBufReadPosition=0;
			}
			usbComSendBufCount--;
		}
		else
		{
			SEGGER_RTT_printf(0, "USB Send Fail\n");
		}
	}
}

static void sendACK()
{
	uint8_t buf[4];
//	SEGGER_RTT_printf(0, "sendACK\n");

	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 4U;
	buf[2U] = MMDVM_ACK;
	buf[3U] = com_requestbuffer[2U];
	enqueueUSBData(buf,4);
}

static void sendNAK(uint8_t err)
{
	uint8_t buf[5];
	SEGGER_RTT_printf(0, "sendNAK\n");
	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 5U;
	buf[2U] = MMDVM_NAK;
	buf[3U] = com_requestbuffer[2U];
	buf[4U] = err;
	enqueueUSBData(buf,5);
}

static void enableTransmission()
{
	SEGGER_RTT_printf(0, "EnableTransmission\n");

	GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
	GPIO_PinWrite(GPIO_LEDred, Pin_LEDred, 1);

	trxSetFrequency(freq_tx);
	txstopdelay=0;
	trxIsTransmitting=true;
	trx_setTX();
}

static void hotspotSendVoiceFrame(uint8_t *receivedDMRDataAndAudio)
{
	uint8_t frameData[DMR_FRAME_LENGTH_BYTES+MMDVM_HEADER_LENGTH] = {0xE0,0x25,MMDVM_DMR_DATA2};
	uint8_t embData[DMR_FRAME_LENGTH_BYTES];
	int i;
	int sequenceNumber = receivedDMRDataAndAudio[27+0x0c + 1] - 1;

	// copy the audio sections
	memcpy(frameData+MMDVM_HEADER_LENGTH,receivedDMRDataAndAudio+0x0C,14);
	memcpy(frameData+MMDVM_HEADER_LENGTH+EMBEDDED_DATA_OFFSET+6,receivedDMRDataAndAudio+0x0C+EMBEDDED_DATA_OFFSET,14);

	if (sequenceNumber == 0)
	{
		frameData[3] = MMDVM_VOICE_SYNC_PATTERN;// sequence 0
		for (i = 0U; i < 7U; i++)
		{
			frameData[i + EMBEDDED_DATA_OFFSET+MMDVM_HEADER_LENGTH] = (frameData[i + EMBEDDED_DATA_OFFSET+MMDVM_HEADER_LENGTH] & ~SYNC_MASK[i]) | MS_SOURCED_AUDIO_SYNC[i];
		}
	}
	else
	{
		frameData[3] = sequenceNumber;
		DMREmbeddedData_getData(embData, sequenceNumber);
		for (i = 0U; i < 7U; i++)
		{
			frameData[i + EMBEDDED_DATA_OFFSET+MMDVM_HEADER_LENGTH] = (frameData[i + EMBEDDED_DATA_OFFSET+MMDVM_HEADER_LENGTH] & ~DMR_AUDIO_SEQ_MASK[i]) | DMR_AUDIO_SEQ_SYNC[sequenceNumber][i];
			frameData[i + EMBEDDED_DATA_OFFSET+MMDVM_HEADER_LENGTH] = (frameData[i + EMBEDDED_DATA_OFFSET+MMDVM_HEADER_LENGTH] & ~DMR_EMBED_SEQ_MASK[i]) | embData[i + EMBEDDED_DATA_OFFSET];
		}
	}

	//displayDataBytes(frameData,DMR_FRAME_LENGTH_BYTES+MMDVM_HEADER_LENGTH);
	//SEGGER_RTT_printf(0, "hotspotSendVoiceFrame\n");
	enqueueUSBData(frameData,DMR_FRAME_LENGTH_BYTES+MMDVM_HEADER_LENGTH);
}

static void sendVoiceHeaderLC_Frame(volatile uint8_t *receivedDMRDataAndAudio)
{
	uint8_t frameData[DMR_FRAME_LENGTH_BYTES+MMDVM_HEADER_LENGTH] = {0xE0,0x25,0x1A,DMR_SYNC_DATA | DT_VOICE_LC_HEADER};
	DMRLC_T lc;
	memset(&lc,0,sizeof(DMRLC_T));// clear automatic variable
	lc.srcId = (receivedDMRDataAndAudio[6]<<16)+(receivedDMRDataAndAudio[7]<<8)+(receivedDMRDataAndAudio[8]<<0);
	lc.dstId = (receivedDMRDataAndAudio[3]<<16)+(receivedDMRDataAndAudio[4]<<8)+(receivedDMRDataAndAudio[5]<<0);
	lc.FLCO = receivedDMRDataAndAudio[0];// Private or group call

	DMRFullLC_encode(&lc,frameData + MMDVM_HEADER_LENGTH, DT_VOICE_LC_HEADER);// Encode the src and dst Ids etc
	DMREmbeddedData_setLC(&lc);
	for (unsigned int i = 0U; i < 8U; i++)
	{
		frameData[i + 12U+MMDVM_HEADER_LENGTH] = (frameData[i + 12U+MMDVM_HEADER_LENGTH] & ~LC_SYNC_MASK_FULL[i]) | VOICE_LC_SYNC_FULL[i];
	}

	//SEGGER_RTT_printf(0, "sendVoiceHeaderLC_Frame\n");
	enqueueUSBData(frameData,DMR_FRAME_LENGTH_BYTES+MMDVM_HEADER_LENGTH);
}

static void sendTerminator_LC_Frame(volatile uint8_t *receivedDMRDataAndAudio)
{
	uint8_t frameData[DMR_FRAME_LENGTH_BYTES+MMDVM_HEADER_LENGTH] = {0xE0,0x25,0x1A,DMR_SYNC_DATA | DT_TERMINATOR_WITH_LC};
	DMRLC_T lc;
	memset(&lc,0,sizeof(DMRLC_T));// clear automatic variable
	lc.srcId = (receivedDMRDataAndAudio[6]<<16)+(receivedDMRDataAndAudio[7]<<8)+(receivedDMRDataAndAudio[8]<<0);
	lc.dstId = (receivedDMRDataAndAudio[3]<<16)+(receivedDMRDataAndAudio[4]<<8)+(receivedDMRDataAndAudio[5]<<0);

	DMRFullLC_encode(&lc,frameData + MMDVM_HEADER_LENGTH, DT_TERMINATOR_WITH_LC);// Encode the src and dst Ids etc

	for (unsigned int i = 0U; i < 8U; i++)
	{
		frameData[i + 12U+MMDVM_HEADER_LENGTH] = (frameData[i + 12U + MMDVM_HEADER_LENGTH] & ~LC_SYNC_MASK_FULL[i]) | TERMINATOR_LC_SYNC_FULL[i];
	}

	//SEGGER_RTT_printf(0, "sendTerminator_LC_Frame\n");
	enqueueUSBData(frameData,DMR_FRAME_LENGTH_BYTES+MMDVM_HEADER_LENGTH);
}
#if false
static void startupTests()
{
	uint8_t dataBuff[27 + 0x0C+2];
	DMRLC_T lc;
	uint32_t srcId = 2344235;
	uint32_t dstId = 777;

	memset(&lc,0,sizeof(DMRLC_T));// clear automatic variable
	lc.srcId = srcId;
	lc.dstId = dstId;
	DMREmbeddedData_setLC(&lc);

	dataBuff[27+0x0c+1] = 1;// sequence 0

	for(int i=0;i<27;i++)
	{
		dataBuff[0x0c+i]=0x10+i;
	}

	// setup src and dst ID's
	dataBuff[3] =  ((dstId>>16) & 0xFF);
	dataBuff[4] =  ((dstId>> 8) & 0xFF);
	dataBuff[5] =  ((dstId>> 0) & 0xFF);

	dataBuff[6] =  ((srcId>>16) & 0xFF);
	dataBuff[7] =  ((srcId>> 8) & 0xFF);
	dataBuff[8] =  ((srcId>> 0) & 0xFF);

	hotspotSendVoiceFrame(dataBuff);
}

static void startupTests2()
{
	uint8_t frameData[DMR_FRAME_LENGTH_BYTES+MMDVM_HEADER_LENGTH] = {0xE0,0x25,0x1A};
	uint8_t embData[DMR_FRAME_LENGTH_BYTES];
	int i;

	DMRLC_T lc;
	memset(&lc,0,sizeof(DMRLC_T));// clear automatic variable
	lc.srcId = 2344235;
	lc.dstId = 777;
	DMREmbeddedData_setLC(&lc);

	for(int sequenceNumber=0;sequenceNumber<6;sequenceNumber++)
	{
		memset(frameData+4,0,DMR_FRAME_LENGTH_BYTES);//clear
//		SEGGER_RTT_printf(0, "Sequence %d\n",sequenceNumber);
		vTaskDelay(portTICK_PERIOD_MS * 10);

		if (sequenceNumber == 0)
		{
			frameData[3] = 0x20;
			for (i = 0U; i < 7U; i++)
			{
				frameData[i + EMBEDDED_DATA_OFFSET+MMDVM_HEADER_LENGTH] = (frameData[i + EMBEDDED_DATA_OFFSET+MMDVM_HEADER_LENGTH] & ~SYNC_MASK[i]) | MS_SOURCED_AUDIO_SYNC[i];
			}
		}
		else
		{
			frameData[3] = sequenceNumber;

			DMREmbeddedData_getData(embData, sequenceNumber);
			for (i = 0U; i < 7U; i++)
			{
				frameData[i + EMBEDDED_DATA_OFFSET+MMDVM_HEADER_LENGTH] = (frameData[i + EMBEDDED_DATA_OFFSET+MMDVM_HEADER_LENGTH] & ~DMR_AUDIO_SEQ_MASK[i]) | DMR_AUDIO_SEQ_SYNC[sequenceNumber][i];
				frameData[i + EMBEDDED_DATA_OFFSET+MMDVM_HEADER_LENGTH] = (frameData[i + EMBEDDED_DATA_OFFSET+MMDVM_HEADER_LENGTH] & ~DMR_EMBED_SEQ_MASK[i]) | embData[i + EMBEDDED_DATA_OFFSET];
			}
		}

		//displayFrameData(frameData,DMR_FRAME_LENGTH_BYTES+MMDVM_HEADER_LENGTH);
		vTaskDelay(portTICK_PERIOD_MS * 10);
	}
}
#endif

volatile int  	rfFrameBufReadIdx=0;
volatile int  	rfFrameBufWriteIdx=0;
volatile int	rfFrameBufCount=0;

void hotspotRxFrameHandler(uint8_t* frameBuf)
{
	taskENTER_CRITICAL();
	memcpy((uint8_t *)&wavbuffer[rfFrameBufWriteIdx],frameBuf,27 + 0x0c  + 2);// 27 audio + 0x0c header + 2 hotspot signalling bytes
	rfFrameBufCount++;
	rfFrameBufWriteIdx++;
	if (rfFrameBufWriteIdx > (WAV_BUFFER_COUNT - 1))
	{
		rfFrameBufWriteIdx=0;
	}
	taskEXIT_CRITICAL();
}

static bool startedEmbeddedSearch = false;
int getEmbeddedData(uint8_t *com_requestbuffer)
{
	int lcss;
	unsigned char DMREMB[2U];
	DMREMB[0U]  = (com_requestbuffer[MMDVM_HEADER_LENGTH + 13U] << 4) & 0xF0U;
	DMREMB[0U] |= (com_requestbuffer[MMDVM_HEADER_LENGTH + 14U] >> 4) & 0x0FU;
	DMREMB[1U]  = (com_requestbuffer[MMDVM_HEADER_LENGTH + 18U] << 4) & 0xF0U;
	DMREMB[1U] |= (com_requestbuffer[MMDVM_HEADER_LENGTH + 19U] >> 4) & 0x0FU;
	CQR1676_decode(DMREMB);

//	m_colorCode = (DMREMB[0U] >> 4) & 0x0FU;
//	m_PI        = (DMREMB[0U] & 0x08U) == 0x08U;

	lcss = (DMREMB[0U] >> 1) & 0x03U;

	if (startedEmbeddedSearch==false)
	{
		DMREmbeddedData_initEmbeddedDataBuffers();
		startedEmbeddedSearch=true;
	}

	if (DMREmbeddedData_addData(com_requestbuffer+4,lcss))
	{
		DMRLC_T lc;
		//uint8_t embData[9U];

		int flco = DMREmbeddedData_getFLCO();
		//SEGGER_RTT_printf(0, "Embedded FCLO:%d\n",DMREmbeddedData_getFLCO());
		DMREmbeddedData_getRawData(hotspotTxLC);

		switch (flco)
		{
			case FLCO_GROUP:
				DMREmbeddedData_getLC(&lc);
				SEGGER_RTT_printf(0, "Emb Group  FID:%d FLCO:%d PF:%d R:%d dstId:%d src:Id:%d options:0x%02x\n",lc.FID,lc.FLCO,lc.PF,lc.R,lc.dstId,lc.srcId,lc.options);
				displayDataBytes(hotspotTxLC,9);
				break;
			case FLCO_USER_USER:
				DMREmbeddedData_getLC(&lc);
				SEGGER_RTT_printf(0, "Emb User  FID:%d FLCO:%d PF:%d R:%d dstId:%d src:Id:%d options:0x%02x\n",lc.FID,lc.FLCO,lc.PF,lc.R,lc.dstId,lc.srcId,lc.options);
				displayDataBytes(hotspotTxLC,9);
				break;

			case FLCO_GPS_INFO:
				SEGGER_RTT_printf(0, "Emb GPS\n");
				displayDataBytes(hotspotTxLC,9);
				break;

			case FLCO_TALKER_ALIAS_HEADER:
				SEGGER_RTT_printf(0, "Emb FLCO_TALKER_ALIAS_HEADER\n");
				displayDataBytes(hotspotTxLC,9);
				break;

			case FLCO_TALKER_ALIAS_BLOCK1:
				SEGGER_RTT_printf(0, "Emb FLCO_TALKER_ALIAS_BLOCK1\n");
				displayDataBytes(hotspotTxLC,9);
				break;

			case FLCO_TALKER_ALIAS_BLOCK2:
				SEGGER_RTT_printf(0, "Emb FLCO_TALKER_ALIAS_BLOCK2\n");
				displayDataBytes(hotspotTxLC,9);
				break;

			case FLCO_TALKER_ALIAS_BLOCK3:
				SEGGER_RTT_printf(0, "Emb FLCO_TALKER_ALIAS_BLOCK3\n");
				displayDataBytes(hotspotTxLC,9);
				break;

			default:
				SEGGER_RTT_printf(0, "Emb UNKNOWN TYPE\n");
				break;
		}
		startedEmbeddedSearch=false;
	}


	return 0;

#if false
		//displayDataBytes(com_requestbuffer, MMDVM_HEADER_LENGTH+33);
		for(int i=0;i<6;i++)
		{
		if (	(com_requestbuffer[MMDVM_HEADER_LENGTH + EMBEDDED_DATA_OFFSET + 0] & 0x0F) 	==  DMR_AUDIO_SEQ_SYNC[i][0] &&
				(com_requestbuffer[MMDVM_HEADER_LENGTH + EMBEDDED_DATA_OFFSET + 1] & 0xF0)  ==  DMR_AUDIO_SEQ_SYNC[i][1] &&
				(com_requestbuffer[MMDVM_HEADER_LENGTH + EMBEDDED_DATA_OFFSET + 5] & 0x0F)  ==  DMR_AUDIO_SEQ_SYNC[i][5] &&
				(com_requestbuffer[MMDVM_HEADER_LENGTH + EMBEDDED_DATA_OFFSET + 6] & 0xF0)  ==  DMR_AUDIO_SEQ_SYNC[i][6] )
			{
				return i;
			}
		}

	return -1;
#endif
}

static void storeNetFrame(uint8_t *com_requestbuffer)
{
	//SEGGER_RTT_printf(0, "storeNetFrame\n");
	if (memcmp((uint8_t *)&com_requestbuffer[18],END_FRAME_PATTERN,6) !=0
			&& memcmp((uint8_t *)&com_requestbuffer[18],START_FRAME_PATTERN,6)!=0
			&& (	hotspotState == HOTSPOT_STATE_TX_START_BUFFERING ||
					hotspotState == HOTSPOT_STATE_TRANSMITTING ||
					hotspotState == HOTSPOT_STATE_TX_SHUTDOWN))
	{
    	if (wavbuffer_count>=16)
    	{
    		SEGGER_RTT_printf(0, "------------------------------ Buffer overflow ---------------------------\n");
    	}

    	getEmbeddedData(com_requestbuffer);
    	//displayDataBytes(com_requestbuffer, 16);
    	taskENTER_CRITICAL();
		memcpy((uint8_t *)&wavbuffer[wavbuffer_write_idx][0x0C],com_requestbuffer+4,13);//copy the first 13, whole bytes of audio
		wavbuffer[wavbuffer_write_idx][0x0C + 13] = (com_requestbuffer[17] & 0xF0) | (com_requestbuffer[23] & 0x0F);
		memcpy((uint8_t *)&wavbuffer[wavbuffer_write_idx][0x0C + 14],(uint8_t *)&com_requestbuffer[24],13);//copy the last 13, whole bytes of audio

		memcpy((uint8_t *)&wavbuffer[wavbuffer_write_idx],hotspotTxLC,9);// copy the current LC into the data (mainly for use with the embedded data);
		wavbuffer_count++;
		wavbuffer_write_idx++;
		if (wavbuffer_write_idx > (WAV_BUFFER_COUNT - 1))
		{
			wavbuffer_write_idx=0;
		}
		taskEXIT_CRITICAL();
	}
	else
	{
		//SEGGER_RTT_printf(0, "Non audio frame  %d Frames in the buffer\n",wavbuffer_count);
	}
}

bool hotspotModeReceiveNetFrame(uint8_t *com_requestbuffer, int timeSlot)
{
	DMRLC_T lc;
	bool lcDecodeOK;
//	uint32_t colorCode,dataType;

	lc.srcId=0;// zero these values as they are checked later in the function, but only updated if the data type is DT_VOICE_LC_HEADER
	lc.dstId=0;


	lcDecodeOK = DMRFullLC_decode(com_requestbuffer + MMDVM_HEADER_LENGTH, DT_VOICE_LC_HEADER,&lc);// Need to decode the frame to get the source and destination
	/*	DMRSlotType_decode(com_requestbuffer + MMDVM_HEADER_LENGTH,&colorCode,&dataType);
	//SEGGER_RTT_printf(0, "SlotType:$d %d\n",dataType,colorCode);
	switch(dataType)
	{

		case DT_VOICE_LC_HEADER:
			SEGGER_RTT_printf(0, "DT_VOICE_LC_HEADER\n");
			break;
		case DT_TERMINATOR_WITH_LC:
			SEGGER_RTT_printf(0, "DT_TERMINATOR_WITH_LC\n");
			//trxTalkGroupOrPcId  = 0;
			//trxDMRID = 0;
			break;

		case DT_VOICE_PI_HEADER:
			SEGGER_RTT_printf(0, "DT_VOICE_PI_HEADER\n");
			break;
		case DT_CSBK:
			SEGGER_RTT_printf(0, "DT_CSBK\n");
			break;
		case DT_DATA_HEADER:
			SEGGER_RTT_printf(0, "DT_DATA_HEADER\n",);
			break;
		case DT_RATE_12_DATA:
			SEGGER_RTT_printf(0, "DT_RATE_12_DATA\n");
			break;
		case DT_RATE_34_DATA:
			SEGGER_RTT_printf(0, "DT_RATE_34_DATA\n",);
			break;
		case DT_IDLE:
			SEGGER_RTT_printf(0, "DT_IDLE\n");
			break;
		case DT_RATE_1_DATA:
			SEGGER_RTT_printf(0, "DT_RATE_1_DATA\n");
			break;
		default:
			//SEGGER_RTT_printf(0, "Frame dataType %d\n",dataType);
			break;
	}*/

	// update the src and destination ID's if valid
	if 	(lc.srcId!=0 && lc.dstId!=0)
	{
		trxTalkGroupOrPcId  = lc.dstId | (lc.FLCO << 24);
		trxDMRID = lc.srcId;

		if (hotspotState != HOTSPOT_STATE_TX_START_BUFFERING)
		{
			SEGGER_RTT_printf(0, "Net frame LC_decodOK:%d FID:%d FLCO:%d PF:%d R:%d dstId:%d src:Id:%d options:0x%02x\n",lcDecodeOK,lc.FID,lc.FLCO,lc.PF,lc.R,lc.dstId,lc.srcId,lc.options);
			memcpy(hotspotTxLC,lc.rawData,9);//
			// the Src and Dst Id's have been sent, and we are in RX mode then an incoming Net normally arrives next
			hotspotState = HOTSPOT_STATE_TX_START_BUFFERING;

		}
	}
	else
	{
		storeNetFrame(com_requestbuffer);
	}
	//SEGGER_RTT_printf(0, "hotspotModeReceiveNetFrame\n");
	sendACK();
	return true;
}

uint8_t lastRxState=HOTSPOT_RX_IDLE;

static void hotspotStateMachine()
{
	switch(hotspotState)
	{
		case HOTSPOT_STATE_NOT_CONNECTED:
			// do nothing
			break;
		case HOTSPOT_STATE_INITIALISE:
			wavbuffer_read_idx=0;
			wavbuffer_write_idx=0;
			wavbuffer_count=0;
			hotspotState = HOTSPOT_STATE_RX_START;
			SEGGER_RTT_printf(0, "STATE_INITIALISE -> STATE_RX\n");
			break;
		case HOTSPOT_STATE_RX_START:
			SEGGER_RTT_printf(0, "STATE_RX_START\n");
			trxSetFrequency(freq_rx);
			wavbuffer_read_idx=0;
			wavbuffer_write_idx=0;
			wavbuffer_count=0;
			updateScreen();
			hotspotState = HOTSPOT_STATE_RX_PROCESS;
			break;
		case HOTSPOT_STATE_RX_PROCESS:

        	if (rfFrameBufCount > 0)
        	{
        		if (MMDVMHostRxState == MMDVMHOST_RX_READY)
        		{
        			int rx_command = wavbuffer[rfFrameBufReadIdx][27+0x0c];
        			//SEGGER_RTT_printf(0, "RX_PROCESS cmd:%d buf:%d\n",rx_command,wavbuffer_count);

					switch(rx_command)
					{
						case HOTSPOT_RX_START:
							SEGGER_RTT_printf(0, "RX_START\n",rx_command,wavbuffer_count);
							sendVoiceHeaderLC_Frame(wavbuffer[rfFrameBufReadIdx]);
							lastRxState = HOTSPOT_RX_START;
							break;
						case HOTSPOT_RX_START_LATE:
							SEGGER_RTT_printf(0, "RX_START_LATE\n");
							sendVoiceHeaderLC_Frame(wavbuffer[rfFrameBufReadIdx]);
							lastRxState = HOTSPOT_RX_START_LATE;
							break;
						case HOTSPOT_RX_AUDIO_FRAME:
							//SEGGER_RTT_printf(0, "HOTSPOT_RX_AUDIO_FRAME\n");
							hotspotSendVoiceFrame((uint8_t *)wavbuffer[rfFrameBufReadIdx]);
							lastRxState = HOTSPOT_RX_AUDIO_FRAME;
							break;
						case HOTSPOT_RX_STOP:
							SEGGER_RTT_printf(0, "RX_STOP\n");
							sendTerminator_LC_Frame(wavbuffer[rfFrameBufReadIdx]);
							lastRxState = HOTSPOT_RX_STOP;
							hotspotState = HOTSPOT_STATE_RX_END;
							break;
						case HOTSPOT_RX_IDLE_OR_REPEAT:
							SEGGER_RTT_printf(0, "RX_IDLE_OR_REPEAT\n");/*
							switch(lastRxState)
							{
								case HOTSPOT_RX_START:
									sendVoiceHeaderLC_Frame(wavbuffer[wavbuffer_read_idx]);
									break;
								case HOTSPOT_RX_STOP:
									sendTerminator_LC_Frame(wavbuffer[wavbuffer_read_idx]);
									break;
								default:
									SEGGER_RTT_printf(0, "ERROR: Unkown HOTSPOT_RX_IDLE_OR_REPEAT\n");
									break;
							}
							*/
							break;

						default:
							SEGGER_RTT_printf(0, "ERROR: Unkown Hotspot RX state\n");
							break;
					}

					rfFrameBufReadIdx++;
					if (rfFrameBufReadIdx > (WAV_BUFFER_COUNT-1))
					{
						rfFrameBufReadIdx=0;
					}

					if (rfFrameBufCount>0)
					{
						rfFrameBufCount--;
					}
        		}
        		else
        		{
        			// Rx Error NAK
        			hotspotState = HOTSPOT_STATE_RX_END;
        		}
        	}
			break;
		case HOTSPOT_STATE_RX_END:
			hotspotState = HOTSPOT_STATE_RX_START;
			break;
		case HOTSPOT_STATE_TX_START_BUFFERING:
			// If MMDVMHost tells us to go back to idle. (receiving)
			if (modemState == STATE_IDLE)
			{
				modemState = STATE_DMR;
				wavbuffer_read_idx=0;
				wavbuffer_write_idx=0;
				wavbuffer_count=0;
				//hotspotState = HOTSPOT_STATE_INITIALISE;
				SEGGER_RTT_printf(0, "modemState == STATE_IDLE: TX_START_BUFFERING -> INITIALISE\n");
			}
			else
			{
				if (wavbuffer_count > 4)
				{
					hotspotState = HOTSPOT_STATE_TRANSMITTING;
					SEGGER_RTT_printf(0, "TX_START_BUFFERING -> TRANSMITTING %d\n",wavbuffer_count);
					enableTransmission();
					updateScreen();
				}
			}
			break;
		case HOTSPOT_STATE_TRANSMITTING:
			// Stop transmitting when there is no data in the buffer or if MMDVMHost sends the idle command
			if (wavbuffer_count == 0 || modemState == STATE_IDLE)
			{
				hotspotState = HOTSPOT_STATE_TX_SHUTDOWN;
				SEGGER_RTT_printf(0, "TRANSMITTING -> TX_SHUTDOWN %d %d\n",wavbuffer_count,modemState);
				trxIsTransmitting=false;
			}
			break;
		case HOTSPOT_STATE_TX_SHUTDOWN:
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
					hotspotState = HOTSPOT_STATE_RX_START;
					SEGGER_RTT_printf(0, "TX_SHUTDOWN -> STATE_RX\n");
					trxSetFrequency(freq_rx);
					wavbuffer_read_idx=0;
					wavbuffer_write_idx=0;
					wavbuffer_count=0;
				}
			}
			break;
	}
}

int menuHotspotMode(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		hotspotState = HOTSPOT_STATE_NOT_CONNECTED;
		savedPower = nonVolatileSettings.txPower;
		//nonVolatileSettings.txPower=1800;// Set around 1W
		savedTGorPC = trxTalkGroupOrPcId;// Save the current TG or PC
		trxTalkGroupOrPcId=0;

		trxSetModeAndBandwidth(RADIO_MODE_DIGITAL,trxGetBandwidthIs25kHz());// hotspot mode is for DMR i.e Digital mode

		freq_rx = currentChannelData->rxFreq;
		freq_tx = currentChannelData->txFreq;
		settingsUsbMode = USB_MODE_HOTSPOT;
		MMDVMHostRxState = MMDVMHOST_RX_READY; // We have not sent anything to MMDVMHost, so it can't be busy yet.
		UC1701_clearBuf();
		UC1701_printCentered(0, "Hotspot",UC1701_FONT_GD77_8x16);
		UC1701_printCentered(32, "Waiting for",UC1701_FONT_GD77_8x16);
		UC1701_printCentered(48, "PiStar",UC1701_FONT_GD77_8x16);
		UC1701_render();
		displayLightTrigger();
//		updateScreen();
	}
	else
	{
		handleEvent(buttons, keys, events);
	}

	processUSBDataQueue();
	if (com_request==1)
	{
		handleHotspotRequest();
		com_request=0;
	}
	hotspotStateMachine();

	return 0;
}

static void updateScreen()
{
	int val_before_dp;
	int val_after_dp;
	char buffer[32];

	UC1701_clearBuf();
	UC1701_printCentered(0, "Hotspot ACTIVE",UC1701_FONT_GD77_8x16);


	if (trxIsTransmitting)
	{
		sprintf(buffer,"ID %d",trxDMRID & 0xFFFFFF);
		UC1701_printCentered(16, buffer,UC1701_FONT_GD77_8x16);
		if ((trxTalkGroupOrPcId & 0xFF000000) == 0)
		{
			sprintf(buffer,"TG %d",trxTalkGroupOrPcId & 0xFFFFFF);
		}
		else
		{
			sprintf(buffer,"PC %d",trxTalkGroupOrPcId &0xFFFFFF);
		}
		UC1701_printCentered(32, buffer,UC1701_FONT_GD77_8x16);

		val_before_dp = freq_tx/10000;
		val_after_dp = freq_tx - val_before_dp*10000;
		sprintf(buffer,"T %d.%04d MHz",val_before_dp, val_after_dp);
	}
	else
	{
		val_before_dp = freq_rx/10000;
		val_after_dp = freq_rx - val_before_dp*10000;
		sprintf(buffer,"R %d.%04d MHz",val_before_dp, val_after_dp);
	}
	UC1701_printCentered(48, buffer,UC1701_FONT_GD77_8x16);

	UC1701_render();
	//displayLightTrigger();
}

static void handleEvent(int buttons, int keys, int events)
{
	if ((keys & KEY_RED)!=0)
	{
		if (trxIsTransmitting)
		{
			trxIsTransmitting = false;
			trx_deactivateTX();
			trx_setRX();

			GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
			GPIO_PinWrite(GPIO_LEDred, Pin_LEDred, 0);
		}
		trxTalkGroupOrPcId = savedTGorPC;// restore the current TG or PC
		nonVolatileSettings.txPower = savedPower;// restore power setting
		trxDMRID = codeplugGetUserDMRID();
		settingsUsbMode = USB_MODE_CPS;

		menuSystemPopAllAndDisplayRootMenu();
		return;
	}

	if (hotspotState == HOTSPOT_STATE_RX_START)
	{
		if ((buttons & BUTTON_PTT)!=0)
		{

		}
	}
}

static uint8_t setFreq(volatile const uint8_t* data, uint8_t length)
{
// satellite frequencies banned frequency ranges
const int BAN1_MIN  = 14580000;
const int BAN1_MAX  = 14600000;
const int BAN2_MIN  = 43500000;
const int BAN2_MAX  = 43800000;

	SEGGER_RTT_printf(0, "setFreq\r\n");
	hotspotState = HOTSPOT_STATE_INITIALISE;
	displayLightOverrideTimeout(-1);// turn the backlight on permanently

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

	SEGGER_RTT_printf(0, "Tx freq = %d, Rx freq = %d, Power = %d\n",freq_tx,freq_rx,rf_power);

	if ((freq_tx>= BAN1_MIN && freq_tx <= BAN1_MAX) || (freq_tx>= BAN2_MIN && freq_tx <= BAN2_MAX))
	{
		return 4U;// invalid frequency
	}

	if (trxCheckFrequencyInAmateurBand(freq_rx) && trxCheckFrequencyInAmateurBand(freq_tx))
	{
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

static void getStatus()
{
	uint8_t buf[16];
//	SEGGER_RTT_printf(0, "getStatus\n");
  // Send all sorts of interesting internal values
	buf[0U]  = MMDVM_FRAME_START;
	buf[1U]  = 13U;
	buf[2U]  = MMDVM_GET_STATUS;
	buf[3U]  = 0x00U;
	buf[3U] |= 0x02U;// DMR ENABLED
	buf[4U]  = modemState;
	buf[5U]  = (	hotspotState == HOTSPOT_STATE_TX_START_BUFFERING ||
						hotspotState == HOTSPOT_STATE_TRANSMITTING ||
						hotspotState == HOTSPOT_STATE_TX_SHUTDOWN)  ? 0x01U : 0x00U;

	if (hasRXOverflow())
	{
		buf[5U] |= 0x04U;
	}

	if (hasTXOverflow())
	{
		buf[5U] |= 0x08U;
	}
	buf[6U] = 	0U;// No DSTAR
	buf[7U] = 	10U;// DMR
	buf[8U] = 	WAV_BUFFER_COUNT - wavbuffer_count;
	buf[9U] = 	0U;// No YSF
	buf[10U] = 0U;// No P25
	buf[11U] = 0U;// no NXDN
	buf[12U] = 0U;// no POCSAG

	//SEGGER_RTT_printf(0, "getStatus buffers=%d\r\n",s_ComBuf[8U]);

	enqueueUSBData(buf,buf[1U]);
}

static uint8_t setConfig(volatile const uint8_t* data, uint8_t length)
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

  SEGGER_RTT_printf(0, "Colour code:%d\n",colorCode);
  trxSetDMRColourCode(colorCode);

  /* To Do
  m_cwIdTXLevel = data[5U]>>2;
  uint8_t dmrTXLevel    = data[10U];
  io.setDeviations(dstarTXLevel, dmrTXLevel, ysfTXLevel, p25TXLevel, nxdnTXLevel, pocsagTXLevel, ysfLoDev);
  dmrDMOTX.setTXDelay(txDelay);
  dmrDMORX.setColorCode(colorCode);
   */
  return 0U;
}

uint8_t setMode(volatile const uint8_t* data, uint8_t length)
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

static void getVersion()
{
	uint8_t buf[64];
	SEGGER_RTT_printf(0, "getVersion\r\n");

	const char HOTSPOT_NAME[] = "OpenGD77 Hotspot";
	buf[0U]  = MMDVM_FRAME_START;
	buf[1U]= 4 + strlen(HOTSPOT_NAME);// minus 1 because there is no terminator
	buf[2U]  = MMDVM_GET_VERSION;
	buf[3]= PROTOCOL_VERSION;
	strcpy((char *)&buf[4],HOTSPOT_NAME);

	enqueueUSBData(buf,buf[1]);
}

static void handleDMRShortLC()
{
//	uint8_t LCBuf[5];
//	DMRShortLC_decode((uint8_t *) com_requestbuffer + 3U,LCBuf);
//	SEGGER_RTT_printf(0, "MMDVM ShortLC\n %02X %02X %02X %02X %02X\n",LCBuf[0U],LCBuf[1U],LCBuf[2U],LCBuf[3U],LCBuf[4U],LCBuf[5U]);
}

void handleHotspotRequest()
{
	int err;
//	SEGGER_RTT_printf(0, "handleHotspotRequest 0x%0x 0x%0x 0x%0x\r\n",com_requestbuffer[0],com_requestbuffer[1],com_requestbuffer[2]);
	if (com_requestbuffer[0]==MMDVM_FRAME_START)
	{
		//SEGGER_RTT_printf(0, "MMDVM %02x\n",com_requestbuffer[2]);
		switch(com_requestbuffer[2])
		{
			case MMDVM_GET_VERSION:
				getVersion();
				break;
			case MMDVM_GET_STATUS:
				getStatus();
				break;

			case MMDVM_SET_CONFIG:
				err = setConfig(com_requestbuffer + 3U, com_requestbuffer[1] - 3U);
				if (err == 0U)
				{
				  sendACK();
				}
				else
				{
				  sendNAK(err);
				}
				break;
			case MMDVM_SET_MODE:
				err = setMode(com_requestbuffer + 3U, com_requestbuffer[1] - 3U);
				if (err == 0U)
				{
					sendACK();
				}
				else
				{
					sendNAK(err);
				}
				break;
			case MMDVM_SET_FREQ:
	            err = setFreq(com_requestbuffer + 3U, com_requestbuffer[1] - 3U);
	            if (err == 0x00)
	            {
	              sendACK();
	            }
	            else
	            {
	              sendNAK(err);
	            }
	        	updateScreen();
				break;

			case MMDVM_CAL_DATA:
				SEGGER_RTT_printf(0, "MMDVM_CAL_DATA\r\n");
				sendACK();
				break;
			case MMDVM_RSSI_DATA:
				SEGGER_RTT_printf(0, "MMDVM_RSSI_DATA\r\n");
				sendACK();
				break;
			case MMDVM_SEND_CWID:
				SEGGER_RTT_printf(0, "MMDVM_SEND_CWID\r\n");
				sendACK();
				break;
			case MMDVM_DMR_DATA1:
				SEGGER_RTT_printf(0, "MMDVM_DMR_DATA1\r\n");
				hotspotModeReceiveNetFrame((uint8_t *)com_requestbuffer,1);

				break;
			case MMDVM_DMR_LOST1:
				SEGGER_RTT_printf(0, "MMDVM_DMR_LOST1\r\n");
				sendACK();
				break;
			case MMDVM_DMR_DATA2:
				//SEGGER_RTT_printf(0, "MMDVM_DMR_DATA2\r\n");
				hotspotModeReceiveNetFrame((uint8_t *)com_requestbuffer,2);
				break;
			case MMDVM_DMR_LOST2:
				SEGGER_RTT_printf(0, "MMDVM_DMR_LOST2\r\n");
				sendACK();
				break;
			case MMDVM_DMR_SHORTLC:
				SEGGER_RTT_printf(0, "MMDVM_DMR_SHORTLC\r\n");
				handleDMRShortLC();
				sendACK();
				break;
			case MMDVM_DMR_START:
				SEGGER_RTT_printf(0, "MMDVM_DMR_START\r\n");
				sendACK();
				break;
			case MMDVM_DMR_ABORT:
				SEGGER_RTT_printf(0, "MMDVM_DMR_ABORT\r\n");
				sendACK();
				break;
			case MMDVM_SERIAL:
				SEGGER_RTT_printf(0, "MMDVM_SERIAL\r\n");
				sendACK();
				break;
			case MMDVM_TRANSPARENT:
				SEGGER_RTT_printf(0, "MMDVM_TRANSPARENT\r\n");
				sendACK();
				break;
			case MMDVM_QSO_INFO:
				SEGGER_RTT_printf(0, "MMDVM_QSO_INFO\r\n");
				sendACK();
				break;
			case MMDVM_DEBUG1:
				SEGGER_RTT_printf(0, "MMDVM_DEBUG1\r\n");
				sendACK();
				break;
			case MMDVM_DEBUG2:
				SEGGER_RTT_printf(0, "MMDVM_DEBUG2\r\n");
				sendACK();
				break;
			case MMDVM_DEBUG3:
				SEGGER_RTT_printf(0, "MMDVM_DEBUG3\r\n");
				sendACK();
				break;
			case MMDVM_DEBUG4:
				SEGGER_RTT_printf(0, "MMDVM_DEBUG4\r\n");
				sendACK();
				break;
			case MMDVM_DEBUG5:
				SEGGER_RTT_printf(0, "MMDVM_DEBUG5\r\n");
				sendACK();
				break;
			case MMDVM_ACK:
				//MMDVMHostRxState = MMDVMHOST_RX_READY;
				break;
			case MMDVM_NAK:
				MMDVMHostRxState = MMDVMHOST_RX_ERROR;
				SEGGER_RTT_printf(0, "MMDVMHost returned NAK\n");
				break;
			default:
				SEGGER_RTT_printf(0, "Unhandled command type %d\n",com_requestbuffer[2]);
				sendNAK(com_requestbuffer[2]);
				break;
		}
	}
	else
	{
		SEGGER_RTT_printf(0, "Invalid MMDVM header byte %d\n",com_requestbuffer[0]);
	}
}
