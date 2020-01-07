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
#include <hardware/fw_HR-C6000.h>
#include "fw_sound.h"
#include "fw_calibration.h"
#include "fw_settings.h"
#include "fw_usb_com.h"
#include "fw_trx.h"
#include "dmr/DMRFullLC.h"
#include "dmr/DMRShortLC.h"
#include "dmr/DMRSlotType.h"
#include "dmr/QR1676.h"
#include <SeggerRTT/RTT/SEGGER_RTT.h>
#include <user_interface/menuHotspot.h>
#include <user_interface/menuSystem.h>
#include <user_interface/uiUtilityQSOData.h>
#include <functions/fw_ticks.h>

/*
                                Problems with MD-390 on the same frequency
                                ------------------------------------------

 Using a Dual Hat:
M: 2020-01-07 08:46:23.337 DMR Slot 2, received RF voice header from F1RMB to 5000
M: 2020-01-07 08:46:24.143 DMR Slot 2, received RF end of voice transmission from F1RMB to 5000, 0.7 seconds, BER: 0.4%
M: 2020-01-07 08:46:24.644 DMR Slot 2, RF user 12935424 rejected

 Using OpenHD77 HS
 PC 5000 sent from a GD-77
M: 2020-01-07 09:51:55.467 DMR Slot 2, received RF voice header from F1RMB to 5000
M: 2020-01-07 09:51:56.727 DMR Slot 2, received RF end of voice transmission from F1RMB to 5000, 1.1 seconds, BER: 0.3%
M: 2020-01-07 09:52:00.068 DMR Slot 2, received network voice header from 5000 to TG 9
M: 2020-01-07 09:52:03.428 DMR Slot 2, received network end of voice transmission from 5000 to TG 9, 3.5 seconds, 0% packet loss, BER: 0.0%

 Its echo (data sent from the MD-390, by itself):
M: 2020-01-07 09:52:07.300 DMR Slot 2, received RF voice header from F1RMB to 5000
M: 2020-01-07 09:52:09.312 DMR Slot 2, RF voice transmission lost from F1RMB to 5000, 0.0 seconds, BER: 0.0%
M: 2020-01-07 09:52:11.856 DMR Slot 2, received network voice header from 5000 to TG 9
M: 2020-01-07 09:52:15.246 DMR Slot 2, received network end of voice transmission from 5000 to TG 9, 3.5 seconds, 0% packet loss, BER: 0.0%


	 There is a problem if you have a MD-390, GPS enabled, listening on the same frequency (even without different DMRId, I checked !).
	 It send invalid data from time to time (that's not critical, it's simply rejected), but once it heard a PC or TG call (from other
	 transceiver), it will keep repeating that, instead of invalid data.

	 After investigations, it's turns out if you enable 'GPS System' (even with correct configuration, like
	 GC xxx999 for BM), it will send such weird or echo data each GPS interval.

 ** Long story short: turn off GPS stuff on your MD-390 if it's listening local transmissions from other transceivers. **

 */


// Uncomment this to enable all sendDebug*() functions. You will see the results in the MMDVMHost log file.
//#define MMDVM_SEND_DEBUG

// Uncomment this to enable debug screen instead of Hotspot one
#define DEBUG_HS_SCREEN


#define MMDVM_FRAME_START   0xE0U

#define MMDVM_GET_VERSION   0x00U
#define MMDVM_GET_STATUS    0x01U
#define MMDVM_SET_CONFIG    0x02U
#define MMDVM_SET_MODE      0x03U
#define MMDVM_SET_FREQ      0x04U
#define MMDVM_CAL_DATA      0x08U
#define MMDVM_RSSI_DATA     0x09U
#define MMDVM_SEND_CWID     0x0AU

#define MMDVM_DSTAR_HEADER  0x10U
#define MMDVM_DSTAR_DATA    0x11U
#define MMDVM_DSTAR_LOST    0x12U
#define MMDVM_DSTAR_EOT     0x13U

#define MMDVM_DMR_DATA1     0x18U
#define MMDVM_DMR_LOST1     0x19U
#define MMDVM_DMR_DATA2     0x1AU
#define MMDVM_DMR_LOST2     0x1BU
#define MMDVM_DMR_SHORTLC   0x1CU
#define MMDVM_DMR_START     0x1DU
#define MMDVM_DMR_ABORT     0x1EU

#define MMDVM_YSF_DATA      0x20U
#define MMDVM_YSF_LOST      0x21U

#define MMDVM_P25_HDR       0x30U
#define MMDVM_P25_LDU       0x31U
#define MMDVM_P25_LOST      0x32U

#define MMDVM_NXDN_DATA     0x40U
#define MMDVM_NXDN_LOST     0x41U

#define MMDVM_POCSAG_DATA   0x50U

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

//#define HOTSPOT_VERSION_STRING "MMDVM_HS_Hat OpenGD77 Hotspot v0.0.72" // This permits to see OpenGD77 hotspot as a MMDVM_HS in dashboards
#define HOTSPOT_VERSION_STRING "OpenGD77_HS v0.0.73"
#define concat(a, b) a " GitID #" b ""
static const char HARDWARE[] = concat(HOTSPOT_VERSION_STRING, GITVERSION);


static const uint8_t MMDVM_VOICE_SYNC_PATTERN = 0x20U;

static const int EMBEDDED_DATA_OFFSET = 13U;
static const int TX_BUFFER_MIN_BEFORE_TRANSMISSION = 4;

static const uint8_t START_FRAME_PATTERN[] = {0xFF,0x57,0xD7,0x5D,0xF5,0xD9};
static const uint8_t END_FRAME_PATTERN[] 	= {0x5D,0x7F,0x77,0xFD,0x75,0x79};
//static const uint32_t HOTSPOT_BUFFER_LENGTH = 0xA0;

static uint32_t freq_rx;
static uint32_t freq_tx;
static uint8_t rf_power;
static uint32_t savedTGorPC;
static uint8_t hotspotTxLC[9];
static bool startedEmbeddedSearch = false;

volatile int usbComSendBufWritePosition = 0;
volatile int usbComSendBufReadPosition = 0;
volatile int usbComSendBufCount = 0;
volatile usb_status_t lastUSBSerialTxStatus = kStatus_USB_Success;
volatile int  	rfFrameBufReadIdx = 0;
volatile int  	rfFrameBufWriteIdx = 0;
volatile int	rfFrameBufCount = 0;
static uint8_t lastRxState = HOTSPOT_RX_IDLE;
static const int TX_BUFFERING_TIMEOUT = 5000;// 500mS
static int timeoutCounter;
static int savedPowerLevel = -1;// no power level saved yet
static int hotspotPowerLevel = 0;// no power level saved yet
static uint32_t mmdvmHostLastActiveTick = 0;
static bool mmdvmHostIsConnected = false;

static volatile enum
{
	MMDVMHOST_RX_READY,
	MMDVMHOST_RX_BUSY,
	MMDVMHOST_RX_ERROR
} MMDVMHostRxState;

typedef enum
{
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
} MMDVM_STATE;

static volatile MMDVM_STATE modemState = STATE_IDLE;


typedef enum
{
	HOTSPOT_STATE_NOT_CONNECTED,
	HOTSPOT_STATE_INITIALISE,
	HOTSPOT_STATE_RX_START,
	HOTSPOT_STATE_RX_PROCESS,
	HOTSPOT_STATE_RX_END,
	HOTSPOT_STATE_TX_START_BUFFERING,
	HOTSPOT_STATE_TRANSMITTING,
	HOTSPOT_STATE_TX_SHUTDOWN
} HOTSPOT_STATE;

static volatile HOTSPOT_STATE hotspotState = HOTSPOT_STATE_NOT_CONNECTED;

//const int USB_SERIAL_TX_RETRIES = 2;
static const uint8_t VOICE_LC_SYNC_FULL[] 		= { 0x04U, 0x6DU, 0x5DU, 0x7FU, 0x77U, 0xFDU, 0x75U, 0x7EU, 0x30U};
static const uint8_t TERMINATOR_LC_SYNC_FULL[]	= { 0x04U, 0xADU, 0x5DU, 0x7FU, 0x77U, 0xFDU, 0x75U, 0x79U, 0x60U};

static const uint8_t LC_SYNC_MASK_FULL[]  		= { 0x0FU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xF0U};


static const uint8_t DMR_AUDIO_SEQ_SYNC[6][7] 	= {  {0x07U, 0xF0U, 0x00U, 0x00U, 0x00U, 0x0FU, 0xD0U},  // seq 0 NOT USED AS THIS IS THE SYNC
													 {0x01U, 0x30U, 0x00U, 0x00U, 0x00U, 0x09U, 0x10U},  // seq 1
													 {0x01U, 0x70U, 0x00U, 0x00U, 0x00U, 0x07U, 0x40U},  // seq 2
													 {0x01U, 0x70U, 0x00U, 0x00U, 0x00U, 0x07U, 0x40U},  // seq 3
													 {0x01U, 0x50U, 0x00U, 0x00U, 0x00U, 0x00U, 0x70U},  // seq 4
													 {0x01U, 0x10U, 0x00U, 0x00U, 0x00U, 0x0EU, 0x20U}}; // seq 5

static const uint8_t DMR_AUDIO_SEQ_MASK[]  		= {0x0FU, 0xF0U, 0x00U, 0x00U, 0x00U, 0x0FU, 0xF0U};
static const uint8_t DMR_EMBED_SEQ_MASK[]  		= {0x00U, 0x0FU, 0xFFU, 0xFFU, 0xFFU, 0xF0U, 0x00U};


static void updateScreen(uint8_t rxState);
static void handleEvent(uiEvent_t *ev);
static void leaveHotspotMenu(void);
static void hotspotStateMachine(void);
static void processUSBDataQueue(void);
static void handleHotspotRequest(void);


#if defined(DEBUG_HS_SCREEN)
static void dbgAct0(uint8_t cmd)
{
	static bool evblink = false;
	//static uint32_t m = 0;
	char buf[16];

//	if ((fw_millis() - m) > 100)
	{
		ucPrintCore(4, 0, "K", FONT_6x8, TEXT_ALIGN_LEFT, evblink);
		evblink = !evblink;
		//m = fw_millis();

		sprintf(buf, "0x%x", cmd);
		ucFillRect(32, 0, 6*6, 8, true);
		ucPrintAt(32, 0, buf, FONT_6x8);

//		ucRenderRows(3,4);
		ucRenderRows(0, 1);
	}
}

static void dbgAct1(uint8_t cmd)
{
	static bool evblink = false;
	//static uint32_t m = 0;
	char buf[16];

//	if ((fw_millis() - m) > 100)
	{
		ucPrintCore(4, 8, "Q", FONT_6x8, TEXT_ALIGN_LEFT, evblink);
		evblink = !evblink;
		//m = fw_millis();

		sprintf(buf, "0x%x", cmd);
		ucFillRect(32, 8, 6*6, 8, true);
		ucPrintAt(32, 8, buf, FONT_6x8);

		ucRenderRows(1,2);
	}
}
#if 0
static void dbgPrint0(char *text)
{
	char buf[22];
	static uint8_t tick = 0;

	snprintf(buf, 21, "%s %03d", text, ++tick);
	buf[21] = 0;
	ucFillRect(0, 0, 128, 8, true);
	ucPrintAt(0, 0, buf, FONT_6x8);

	ucRenderRows(0,1);
}
static void dbgPrint1(char *text)
{
	char buf[22];
	static uint8_t tick = 0;

	snprintf(buf,21, "%s %03d", text, ++tick);
	buf[21] = 0;
	ucFillRect(0, 8, 128, 8, true);
	ucPrintAt(0, 8, buf, FONT_6x8);

	ucRenderRows(1,2);
}
#endif
static void dbgPrint2(char *text)
{
	char buf[22];
	static uint8_t tick = 0;

	snprintf(buf, 21, "%s %03d", text, ++tick);
	buf[21] = 0;
	ucFillRect(0, 16, 128, 8, true);
	ucPrintAt(0, 16, buf, FONT_6x8);

	//ucRender();//Rows(4,5);
	ucRenderRows(2,3);
}

static void dbgPrint3(char *text, int v)
{
	char buf[22];
	static uint8_t tick = 0;

	snprintf(buf, 21, "%s %d %03d", text, v, ++tick);
	buf[21] = 0;
	ucFillRect(0, 24, 128, 8, true);
	ucPrintAt(0, 24, buf, FONT_6x8);

	//ucRender();//Rows(4,5);
	ucRenderRows(3,4);
}

static void dbgPrint4(char *text, MMDVM_STATE mmdvmState, HOTSPOT_STATE HSState, uint8_t lastRXState)
{
	char buf[22];
	static uint8_t tick = 0;
	snprintf(buf, 21, "%s %d %d %u %03d", text, mmdvmState, HSState, lastRXState, ++tick);
	buf[21] = 0;
	ucFillRect(0, 32, 128, 8, true);
	ucPrintAt(0, 32, buf, FONT_6x8);

	//ucRender();//Rows(4,5);
	ucRenderRows(4,5);
}

static void dbgPrint5(char *text, MMDVM_STATE mmdvmState, HOTSPOT_STATE HSState, uint8_t lastRXState)
{
	char buf[22];
	static uint8_t tick = 0;

	snprintf(buf, 21, "%s %d %d %u %03d", text, mmdvmState, HSState, lastRXState, ++tick);
	buf[21] = 0;
	ucFillRect(0, 40, 128, 8, true);
	ucPrintAt(0, 40, buf, FONT_6x8);

	//ucRender();//Rows(4,5);
	ucRenderRows(5,6);
}

static void dbgPrint6(char *text)
{
	char buf[22];
	static uint8_t tick = 0;

	snprintf(buf, 21, "%s %03d", text, ++tick);
	buf[21] = 0;
	ucFillRect(0, 48, 128, 8, true);
	ucPrintAt(0, 48, buf, FONT_6x8);

	//ucRender();//Rows(4,5);
	ucRenderRows(6,7);
}

static void dbgPrint7(char *text, bool state)
{
	char buf[22];
	static uint8_t tick = 0;

	snprintf(buf, 21, "%s %d %03d", text, state, ++tick);
	buf[21] = 0;
	ucFillRect(0, 56, 128, 8, true);
	ucPrintAt(0, 56, buf, FONT_6x8);

	//ucRender();//Rows(4,5);
	ucRenderRows(7,8);
}

#else
#define dbgAct0(cmd) do {} while (0)
#define dbgAct1(cmd) do {} while (0)
#define dbgPrint0(text) do {} while (0)
#define dbgPrint1(text) do {} while (0)
#define dbgPrint2(text) do {} while (0)
#define dbgPrint3(text, v) do {} while (0)
#define dbgPrint4(text, mmdvmState, HSState, lastRXState) do {} while (0)
#define dbgPrint5(text, mmdvmState, HSState, lastRXState) do {} while (0)
#define dbgPrint6(text) do {} while (0)
#define dbgPrint7(text, state) do {} while (0)
#endif


int menuHotspotMode(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		hotspotState = HOTSPOT_STATE_NOT_CONNECTED;

		savedTGorPC = trxTalkGroupOrPcId;// Save the current TG or PC
		trxTalkGroupOrPcId = 0;
		mmdvmHostIsConnected = false;
		rfFrameBufCount = 0;

		trxSetModeAndBandwidth(RADIO_MODE_DIGITAL, false);// hotspot mode is for DMR i.e Digital mode

		freq_tx = freq_rx = 43000000;
		settingsUsbMode = USB_MODE_HOTSPOT;
		MMDVMHostRxState = MMDVMHOST_RX_READY; // We have not sent anything to MMDVMHost, so it can't be busy yet.

		usbComSendBufWritePosition = usbComSendBufReadPosition = 0;
		memset(&usbComSendBuf, 0, COM_BUFFER_SIZE);

		ucClearBuf();
#if !defined(DEBUG_HS_SCREEN)
		ucPrintCentered(0, "Hotspot", FONT_8x16);
		ucPrintCentered(32, "Waiting for", FONT_8x16);
		ucPrintCentered(48, "PiStar", FONT_8x16);
#endif
		ucRender();
		displayLightTrigger();
//		updateScreen(HOTSPOT_RX_IDLE);
	}
	else
	{
		if (ev->hasEvent)
			handleEvent(ev);
	}

	processUSBDataQueue();
	if (com_request == 1)
	{
		handleHotspotRequest();
		com_request = 0;
	}
	hotspotStateMachine();

	return 0;
}

static void updateScreen(uint8_t rxCommandState)
{
	int val_before_dp;
	int val_after_dp;
	static const int bufferLen = 17;
	char buffer[bufferLen];
	dmrIdDataStruct_t currentRec;

#if !defined(DEBUG_HS_SCREEN)
	ucClearBuf();
	ucPrintAt(0,0, "DMR Hotspot", FONT_8x16);
#else
	buffer[0] = 0;
#endif
	int  batteryPerentage = (int)(((averageBatteryVoltage - CUTOFF_VOLTAGE_UPPER_HYST) * 100) / (BATTERY_MAX_VOLTAGE - CUTOFF_VOLTAGE_UPPER_HYST));
	if (batteryPerentage>100)
	{
		batteryPerentage=100;
	}
	if (batteryPerentage<0)
	{
		batteryPerentage=0;
	}

	snprintf(buffer, bufferLen, "%d%%", batteryPerentage);
	buffer[bufferLen - 1] = 0;
#if !defined(DEBUG_HS_SCREEN)
	ucPrintCore(0, 4, buffer, FONT_6x8, TEXT_ALIGN_RIGHT, false);// Display battery percentage at the right
#endif

	if (trxIsTransmitting)
	{
		dmrIDLookup((trxDMRID & 0xFFFFFF), &currentRec);
		strncpy(buffer, currentRec.text, bufferLen);
		buffer[bufferLen - 1] = 0;
#if !defined(DEBUG_HS_SCREEN)
		ucPrintCentered(16, buffer, FONT_8x16);
#else
		buffer[0] = 0;
#endif
	//	sprintf(buffer,"ID %d",trxDMRID & 0xFFFFFF);
	//	UC1701_printCentered(16, buffer,UC1701_FONT_8x16);
		if ((trxTalkGroupOrPcId & 0xFF000000) == 0)
		{
			snprintf(buffer, bufferLen, "TG %d", trxTalkGroupOrPcId & 0xFFFFFF);
		}
		else
		{
			snprintf(buffer, bufferLen, "PC %d", trxTalkGroupOrPcId & 0xFFFFFF);
		}
		buffer[bufferLen - 1] = 0;

#if !defined(DEBUG_HS_SCREEN)
		ucPrintCentered(32, buffer, FONT_8x16);
#endif

		val_before_dp = freq_tx / 100000;
		val_after_dp = freq_tx - val_before_dp * 100000;
		sprintf(buffer, "T %d.%05d MHz", val_before_dp, val_after_dp);
	}
	else
	{
		if (rxCommandState == HOTSPOT_RX_START  || rxCommandState == HOTSPOT_RX_START_LATE)
		{
			uint32_t srcId = (audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx][6] << 16) + (audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx][7] << 8) + (audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx][8] << 0);
			uint32_t dstId = (audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx][3] << 16) + (audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx][4] << 8) + (audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx][5] << 0);
			uint32_t FLCO  = audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx][0];// Private or group call

			dmrIDLookup(srcId, &currentRec);
			strncpy(buffer, currentRec.text, bufferLen);
			buffer[bufferLen - 1] = 0;
#if !defined(DEBUG_HS_SCREEN)
			ucPrintCentered(16, buffer, FONT_8x16);
#else
			buffer[0] = 0;
#endif
			if (FLCO == 0)
			{
				snprintf(buffer, bufferLen, "TG %d", dstId);
			}
			else
			{
				snprintf(buffer, bufferLen, "PC %d", dstId);
			}
			buffer[bufferLen - 1] = 0;
#if !defined(DEBUG_HS_SCREEN)
			ucPrintCentered(32, buffer, FONT_8x16);
#else
			buffer[0] = 0;
#endif
		}
		else
		{
			snprintf(buffer, bufferLen, "CC:%d", trxGetDMRColourCode());//, trxGetDMRTimeSlot()+1) ;
			buffer[bufferLen - 1] = 0;
#if !defined(DEBUG_HS_SCREEN)
			ucPrintCore(0, 32, buffer, FONT_8x16, TEXT_ALIGN_LEFT, false);

			ucPrintCore(0, 32, (char *)POWER_LEVELS[hotspotPowerLevel], FONT_8x16, TEXT_ALIGN_RIGHT, false);
#else
			buffer[0] = 0;
#endif
		}
		val_before_dp = freq_rx / 100000;
		val_after_dp = freq_rx - val_before_dp * 100000;
		snprintf(buffer, bufferLen, "R %d.%05d MHz", val_before_dp, val_after_dp);
		buffer[bufferLen - 1] = 0;
	}
#if !defined(DEBUG_HS_SCREEN)
	ucPrintCentered(48, buffer, FONT_8x16);
	ucRender();
#endif
	displayLightTrigger();
}

static void handleEvent(uiEvent_t *ev)
{
	if (KEYCHECK_SHORTUP(ev->keys,KEY_RED))
	{
		//enableHotspot = false;
		if (trxIsTransmitting)
		{
			trxIsTransmitting = false;
			trxActivateRx();
			trx_setRX();

			GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
			GPIO_PinWrite(GPIO_LEDred, Pin_LEDred, 0);
		}

		leaveHotspotMenu();
		return;
	}
}

static void leaveHotspotMenu(void)
{
	trxTalkGroupOrPcId = savedTGorPC;// restore the current TG or PC
	if (savedPowerLevel != -1)
	{
		trxSetPowerFromLevel(savedPowerLevel);
	}

	trxDMRID = codeplugGetUserDMRID();
	settingsUsbMode = USB_MODE_CPS;
	mmdvmHostIsConnected = false;

	menuSystemPopAllAndDisplayRootMenu();
}


/*
 *
 //Enable when debugging
static void displayDataBytes(uint8_t *buf, int len)
{
	for (int i=0;i<len;i++)
	{
    	//SEGGER_RTT_printf(0, " %02x", buf[i]);
	}
	//SEGGER_RTT_printf(0, "\n");
}
*/

// Queue system is a single byte header containing the length of the item, followed by the data
// if the block won't fit in the space between the current write location and the end of the buffer,
// a zero is written to the length for that block and the data and its length byte is put at the beginning of the buffer
static void enqueueUSBData(uint8_t *buff, uint8_t length)
{
	//SEGGER_RTT_printf(0, "Enqueue: ");
	//displayDataBytes(buff,length);
	if ((usbComSendBufWritePosition + length + 1) > (COM_BUFFER_SIZE - 1) )
	{
		//SEGGER_RTT_printf(0, "Looping write buffer back to start pos:%d len:%d\n");
		usbComSendBuf[usbComSendBufWritePosition] = 0;// flag that the data block won't fit and will be put at the start of the buffer
		usbComSendBufWritePosition = 0;
	}

	usbComSendBuf[usbComSendBufWritePosition] = length;
	memcpy(usbComSendBuf + usbComSendBufWritePosition + 1, buff, length);
	usbComSendBufWritePosition += 1 + length;
	usbComSendBufCount++;
}

static void processUSBDataQueue(void)
{
	if (usbComSendBufCount != 0)
	{
		if (usbComSendBuf[usbComSendBufReadPosition] == 0)
		{
			usbComSendBufReadPosition = 0;
		}

		lastUSBSerialTxStatus = USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, &usbComSendBuf[usbComSendBufReadPosition + 1], usbComSendBuf[usbComSendBufReadPosition]);
		if (lastUSBSerialTxStatus == kStatus_USB_Success)
		{
			usbComSendBufReadPosition += usbComSendBuf[usbComSendBufReadPosition] + 1;
			if (usbComSendBufReadPosition > (COM_BUFFER_SIZE - 1))
			{
				usbComSendBufReadPosition = 0;
			}
			usbComSendBufCount--;
		}
		else
		{
			//SEGGER_RTT_printf(0, "USB Send Fail\n");
		}
	}
}

static void enableTransmission(void)
{
	//SEGGER_RTT_printf(0, "Enable Transmission\n");

	GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
	GPIO_PinWrite(GPIO_LEDred, Pin_LEDred, 1);

	txstopdelay=0;
	trx_setTX();
}

static void disableTransmission(void)
{
	//SEGGER_RTT_printf(0, "Disable Transmission\n");

	GPIO_PinWrite(GPIO_LEDred, Pin_LEDred, 0);
	// Need to wrap this in Task Critical to avoid bus contention on the I2C bus.
	taskENTER_CRITICAL();
	trxActivateRx();
	taskEXIT_CRITICAL();
	//trxSetFrequency(freq_rx,freq_tx);

}

static void hotspotSendVoiceFrame(volatile const uint8_t *receivedDMRDataAndAudio)
{
	uint8_t frameData[DMR_FRAME_LENGTH_BYTES + MMDVM_HEADER_LENGTH] = {0xE0, 0x25, MMDVM_DMR_DATA2};
	uint8_t embData[DMR_FRAME_LENGTH_BYTES];
	int i;
	int sequenceNumber = receivedDMRDataAndAudio[27 + 0x0c + 1] - 1;

	// copy the audio sections
	memcpy(frameData + MMDVM_HEADER_LENGTH, (uint8_t *)receivedDMRDataAndAudio + 0x0C, 14);
	memcpy(frameData + MMDVM_HEADER_LENGTH + EMBEDDED_DATA_OFFSET + 6, (uint8_t *)receivedDMRDataAndAudio + 0x0C + EMBEDDED_DATA_OFFSET, 14);

	if (sequenceNumber == 0)
	{
		frameData[3] = MMDVM_VOICE_SYNC_PATTERN;// sequence 0
		for (i = 0U; i < 7U; i++)
		{
			frameData[i + EMBEDDED_DATA_OFFSET + MMDVM_HEADER_LENGTH] = (frameData[i + EMBEDDED_DATA_OFFSET + MMDVM_HEADER_LENGTH] & ~SYNC_MASK[i]) | MS_SOURCED_AUDIO_SYNC[i];
		}
	}
	else
	{
		frameData[3] = sequenceNumber;
		DMREmbeddedData_getData(embData, sequenceNumber);
		for (i = 0U; i < 7U; i++)
		{
			frameData[i + EMBEDDED_DATA_OFFSET + MMDVM_HEADER_LENGTH] = (frameData[i + EMBEDDED_DATA_OFFSET + MMDVM_HEADER_LENGTH] & ~DMR_AUDIO_SEQ_MASK[i]) | DMR_AUDIO_SEQ_SYNC[sequenceNumber][i];
			frameData[i + EMBEDDED_DATA_OFFSET + MMDVM_HEADER_LENGTH] = (frameData[i + EMBEDDED_DATA_OFFSET + MMDVM_HEADER_LENGTH] & ~DMR_EMBED_SEQ_MASK[i]) | embData[i + EMBEDDED_DATA_OFFSET];
		}
	}

	enqueueUSBData(frameData, DMR_FRAME_LENGTH_BYTES + MMDVM_HEADER_LENGTH);
}

static void sendVoiceHeaderLC_Frame(volatile const uint8_t *receivedDMRDataAndAudio)
{
	uint8_t frameData[DMR_FRAME_LENGTH_BYTES + MMDVM_HEADER_LENGTH] = {0xE0, 0x25, 0x1A, DMR_SYNC_DATA | DT_VOICE_LC_HEADER};
	DMRLC_T lc;

	memset(&lc, 0, sizeof(DMRLC_T));// clear automatic variable
	lc.srcId = (receivedDMRDataAndAudio[6] << 16) + (receivedDMRDataAndAudio[7] << 8) + (receivedDMRDataAndAudio[8] << 0);
	lc.dstId = (receivedDMRDataAndAudio[3] << 16) + (receivedDMRDataAndAudio[4] << 8) + (receivedDMRDataAndAudio[5] << 0);
	lc.FLCO = receivedDMRDataAndAudio[0];// Private or group call

	if ((lc.srcId == 0) || (lc.dstId == 0))
	{
		dbgPrint6("VOICE_HEADER_LC_0");
		return;
	}

	// Encode the src and dst Ids etc
	if (!DMRFullLC_encode(&lc, frameData + MMDVM_HEADER_LENGTH, DT_VOICE_LC_HEADER)) // Encode the src and dst Ids etc
	{
		dbgPrint6("LC_ENCODE");
		return;
	}

	DMREmbeddedData_setLC(&lc);
	for (uint8_t i = 0U; i < 8U; i++)
	{
		frameData[i + 12U + MMDVM_HEADER_LENGTH] = (frameData[i + 12U + MMDVM_HEADER_LENGTH] & ~LC_SYNC_MASK_FULL[i]) | VOICE_LC_SYNC_FULL[i];
	}

	//SEGGER_RTT_printf(0, "sendVoiceHeaderLC_Frame\n");
	enqueueUSBData(frameData, DMR_FRAME_LENGTH_BYTES + MMDVM_HEADER_LENGTH);
}

static void sendTerminator_LC_Frame(volatile const uint8_t *receivedDMRDataAndAudio)
{
	uint8_t frameData[DMR_FRAME_LENGTH_BYTES + MMDVM_HEADER_LENGTH] = {0xE0, 0x25, 0x1A, DMR_SYNC_DATA | DT_TERMINATOR_WITH_LC};
	DMRLC_T lc;

	memset(&lc, 0, sizeof(DMRLC_T));// clear automatic variable
	lc.srcId = (receivedDMRDataAndAudio[6] << 16) + (receivedDMRDataAndAudio[7] << 8) + (receivedDMRDataAndAudio[8] << 0);
	lc.dstId = (receivedDMRDataAndAudio[3] << 16) + (receivedDMRDataAndAudio[4] << 8) + (receivedDMRDataAndAudio[5] << 0);

	if ((lc.srcId == 0) || (lc.dstId == 0))
	{
		dbgPrint6("TERM_HEADER_LC_0");
		return;
	}

	// Encode the src and dst Ids etc
	if (!DMRFullLC_encode(&lc, frameData + MMDVM_HEADER_LENGTH, DT_TERMINATOR_WITH_LC))
	{
		dbgPrint6("LC_ENCODE");
		return;
	}

	for (uint8_t i = 0U; i < 8U; i++)
	{
		frameData[i + 12U + MMDVM_HEADER_LENGTH] = (frameData[i + 12U + MMDVM_HEADER_LENGTH] & ~LC_SYNC_MASK_FULL[i]) | TERMINATOR_LC_SYNC_FULL[i];
	}

	//SEGGER_RTT_printf(0, "sendTerminator_LC_Frame\n");
	enqueueUSBData(frameData, DMR_FRAME_LENGTH_BYTES + MMDVM_HEADER_LENGTH);
}

void hotspotRxFrameHandler(uint8_t* frameBuf)
{
	taskENTER_CRITICAL();
	memcpy((uint8_t *)&audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufWriteIdx], frameBuf, 27 + 0x0c  + 2);// 27 audio + 0x0c header + 2 hotspot signalling bytes
	rfFrameBufCount++;
	rfFrameBufWriteIdx = ((rfFrameBufWriteIdx + 1) % HOTSPOT_BUFFER_COUNT);
	taskEXIT_CRITICAL();
}

static bool getEmbeddedData(volatile const uint8_t *com_requestbuffer)
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

	if (startedEmbeddedSearch == false)
	{
		DMREmbeddedData_initEmbeddedDataBuffers();
		startedEmbeddedSearch = true;
	}

	if (DMREmbeddedData_addData((uint8_t *)com_requestbuffer + 4, lcss))
	{
		//DMRLC_T lc;

		DMREmbeddedData_getRawData(hotspotTxLC);

		/*
		int flco = DMREmbeddedData_getFLCO();
		switch (flco)
		{
			case FLCO_GROUP:
				DMREmbeddedData_getLC(&lc);
				//SEGGER_RTT_printf(0, "Emb Group  FID:%d FLCO:%d PF:%d R:%d dstId:%d src:Id:%d options:0x%02x\n",lc.FID,lc.FLCO,lc.PF,lc.R,lc.dstId,lc.srcId,lc.options);
				//displayDataBytes(hotspotTxLC,9);
				break;
			case FLCO_USER_USER:
				DMREmbeddedData_getLC(&lc);
				//SEGGER_RTT_printf(0, "Emb User  FID:%d FLCO:%d PF:%d R:%d dstId:%d src:Id:%d options:0x%02x\n",lc.FID,lc.FLCO,lc.PF,lc.R,lc.dstId,lc.srcId,lc.options);
				//displayDataBytes(hotspotTxLC,9);
				break;

			case FLCO_GPS_INFO:
				//SEGGER_RTT_printf(0, "Emb GPS\n");
				//displayDataBytes(hotspotTxLC,9);
				break;

			case FLCO_TALKER_ALIAS_HEADER:
				//SEGGER_RTT_printf(0, "Emb FLCO_TALKER_ALIAS_HEADER\n");
				//displayDataBytes(hotspotTxLC,9);
				break;

			case FLCO_TALKER_ALIAS_BLOCK1:
				//SEGGER_RTT_printf(0, "Emb FLCO_TALKER_ALIAS_BLOCK1\n");
				//displayDataBytes(hotspotTxLC,9);
				break;

			case FLCO_TALKER_ALIAS_BLOCK2:
				//SEGGER_RTT_printf(0, "Emb FLCO_TALKER_ALIAS_BLOCK2\n");
				//displayDataBytes(hotspotTxLC,9);
				break;

			case FLCO_TALKER_ALIAS_BLOCK3:
				//SEGGER_RTT_printf(0, "Emb FLCO_TALKER_ALIAS_BLOCK3\n");
				//displayDataBytes(hotspotTxLC,9);
				break;

			default:
				//SEGGER_RTT_printf(0, "Emb UNKNOWN TYPE\n");
				break;
		}
		*/
		startedEmbeddedSearch = false;
	}

	return false;
}

static void storeNetFrame(volatile const uint8_t *com_requestbuffer)
{
	bool foundEmbedded;

	//SEGGER_RTT_printf(0, "storeNetFrame\n");
	if (memcmp((uint8_t *)&com_requestbuffer[18], END_FRAME_PATTERN, 6) == 0)
	{
		//SEGGER_RTT_printf(0, "END_FRAME_PATTERN %d\n",wavbuffer_count);
		return;
	}

	if (memcmp((uint8_t *)&com_requestbuffer[18], START_FRAME_PATTERN, 6) == 0)
	{
		//SEGGER_RTT_printf(0, "START_FRAME_PATTERN %d\n",wavbuffer_count);
		return;
	}


	foundEmbedded = getEmbeddedData(com_requestbuffer);

	if (	foundEmbedded &&
			(hotspotTxLC[0] == 0 || hotspotTxLC[0] == 3) &&
			(hotspotState != HOTSPOT_STATE_TX_START_BUFFERING && hotspotState != HOTSPOT_STATE_TRANSMITTING))
	{
		//SEGGER_RTT_printf(0, "LATE START -> HOTSPOT_STATE_TX_BUFFERING\n");
		timeoutCounter = TX_BUFFERING_TIMEOUT;// set buffering timeout
		hotspotState = HOTSPOT_STATE_TX_START_BUFFERING;
	}

	if (hotspotState == HOTSPOT_STATE_TRANSMITTING ||
		hotspotState == HOTSPOT_STATE_TX_SHUTDOWN  ||
		hotspotState == HOTSPOT_STATE_TX_START_BUFFERING)
	{
		if (wavbuffer_count >= HOTSPOT_BUFFER_COUNT)
		{
			//SEGGER_RTT_printf(0, "------------------------------ Buffer overflow ---------------------------\n");
		}

		//displayDataBytes(com_requestbuffer, 16);
		taskENTER_CRITICAL();
		memcpy((uint8_t *)&audioAndHotspotDataBuffer.hotspotBuffer[wavbuffer_write_idx][0x0C], (uint8_t *)com_requestbuffer + 4, 13);//copy the first 13, whole bytes of audio
		audioAndHotspotDataBuffer.hotspotBuffer[wavbuffer_write_idx][0x0C + 13] = (com_requestbuffer[17] & 0xF0) | (com_requestbuffer[23] & 0x0F);
		memcpy((uint8_t *)&audioAndHotspotDataBuffer.hotspotBuffer[wavbuffer_write_idx][0x0C + 14], (uint8_t *)&com_requestbuffer[24], 13);//copy the last 13, whole bytes of audio

		memcpy((uint8_t *)&audioAndHotspotDataBuffer.hotspotBuffer[wavbuffer_write_idx], hotspotTxLC, 9);// copy the current LC into the data (mainly for use with the embedded data);
		wavbuffer_count++;
		wavbuffer_write_idx = ((wavbuffer_write_idx + 1) % HOTSPOT_BUFFER_COUNT);
		taskEXIT_CRITICAL();
	}

}

static uint8_t hotspotModeReceiveNetFrame(volatile const uint8_t *com_requestbuffer, uint8_t timeSlot)
{
	DMRLC_T lc;

	lc.srcId = 0;// zero these values as they are checked later in the function, but only updated if the data type is DT_VOICE_LC_HEADER
	lc.dstId = 0;

	DMRFullLC_decode((uint8_t *)com_requestbuffer + MMDVM_HEADER_LENGTH, DT_VOICE_LC_HEADER, &lc);// Need to decode the frame to get the source and destination
	/*	DMRSlotType_decode(com_requestbuffer + MMDVM_HEADER_LENGTH,&colorCode,&dataType);
	//SEGGER_RTT_printf(0, "SlotType:$d %d\n",dataType,colorCode);
	switch(dataType)
	{

		case DT_VOICE_LC_HEADER:
			//SEGGER_RTT_printf(0, "DT_VOICE_LC_HEADER\n");
			break;
		case DT_TERMINATOR_WITH_LC:
			//SEGGER_RTT_printf(0, "DT_TERMINATOR_WITH_LC\n");
			//trxTalkGroupOrPcId  = 0;
			//trxDMRID = 0;
			break;

		case DT_VOICE_PI_HEADER:
			//SEGGER_RTT_printf(0, "DT_VOICE_PI_HEADER\n");
			break;
		case DT_CSBK:
			//SEGGER_RTT_printf(0, "DT_CSBK\n");
			break;
		case DT_DATA_HEADER:
			//SEGGER_RTT_printf(0, "DT_DATA_HEADER\n",);
			break;
		case DT_RATE_12_DATA:
			//SEGGER_RTT_printf(0, "DT_RATE_12_DATA\n");
			break;
		case DT_RATE_34_DATA:
			//SEGGER_RTT_printf(0, "DT_RATE_34_DATA\n",);
			break;
		case DT_IDLE:
			//SEGGER_RTT_printf(0, "DT_IDLE\n");
			break;
		case DT_RATE_1_DATA:
			//SEGGER_RTT_printf(0, "DT_RATE_1_DATA\n");
			break;
		default:
			//SEGGER_RTT_printf(0, "Frame dataType %d\n",dataType);
			break;
	}*/

	// update the src and destination ID's if valid
	if 	(lc.srcId != 0 && lc.dstId != 0)
	{
		trxTalkGroupOrPcId  = lc.dstId | (lc.FLCO << 24);
		trxDMRID = lc.srcId;

		if (hotspotState != HOTSPOT_STATE_TX_START_BUFFERING)
		{
			//SEGGER_RTT_printf(0, "Net frame LC_decodOK:%d FID:%d FLCO:%d PF:%d R:%d dstId:%d src:Id:%d options:0x%02x\n",lcDecodeOK,lc.FID,lc.FLCO,lc.PF,lc.R,lc.dstId,lc.srcId,lc.options);
			memcpy(hotspotTxLC, lc.rawData, 9);//Hotspot uses LC Data bytes rather than the src and dst ID's for the embed data
			// the Src and Dst Id's have been sent, and we are in RX mode then an incoming Net normally arrives next
			//SEGGER_RTT_printf(0,"hospot state %d -> HOTSPOT_STATE_TX_BUFFERING\n");
			timeoutCounter = TX_BUFFERING_TIMEOUT;
			hotspotState = HOTSPOT_STATE_TX_START_BUFFERING;

		}
	}
	else
	{
		storeNetFrame(com_requestbuffer);
	}
	//SEGGER_RTT_printf(0, "hotspotModeReceiveNetFrame\n");
	return 0U;
}

#if defined(MMDVM_SEND_DEBUG)
static void sendDebug1(const char *text)
{
	uint8_t buf[130U];

	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 0U;
	buf[2U] = MMDVM_DEBUG1;

	uint8_t count = 3U;
	for (uint8_t i = 0U; text[i] != '\0'; i++, count++)
		buf[count] = text[i];

	buf[1U] = count;

	enqueueUSBData(buf, buf[1U]);
}

static void sendDebug2(const char *text, int16_t n1)
{
	uint8_t buf[130U];

	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 0U;
	buf[2U] = MMDVM_DEBUG2;

	uint8_t count = 3U;
	for (uint8_t i = 0U; text[i] != '\0'; i++, count++)
		buf[count] = text[i];

	buf[count++] = (n1 >> 8) & 0xFF;
	buf[count++] = (n1 >> 0) & 0xFF;

	buf[1U] = count;

	enqueueUSBData(buf, buf[1U]);
}

static void sendDebug3(const char *text, int16_t n1, int16_t n2)
{
	uint8_t buf[130U];

	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 0U;
	buf[2U] = MMDVM_DEBUG3;

	uint8_t count = 3U;
	for (uint8_t i = 0U; text[i] != '\0'; i++, count++)
		buf[count] = text[i];

	buf[count++] = (n1 >> 8) & 0xFF;
	buf[count++] = (n1 >> 0) & 0xFF;

	buf[count++] = (n2 >> 8) & 0xFF;
	buf[count++] = (n2 >> 0) & 0xFF;

	buf[1U] = count;

	enqueueUSBData(buf, buf[1U]);
}

static void sendDebug4(const char *text, int16_t n1, int16_t n2, int16_t n3)
{
	uint8_t buf[130U];

	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 0U;
	buf[2U] = MMDVM_DEBUG4;

	uint8_t count = 3U;
	for (uint8_t i = 0U; text[i] != '\0'; i++, count++)
		buf[count] = text[i];

	buf[count++] = (n1 >> 8) & 0xFF;
	buf[count++] = (n1 >> 0) & 0xFF;

	buf[count++] = (n2 >> 8) & 0xFF;
	buf[count++] = (n2 >> 0) & 0xFF;

	buf[count++] = (n3 >> 8) & 0xFF;
	buf[count++] = (n3 >> 0) & 0xFF;

	buf[1U] = count;

	enqueueUSBData(buf, buf[1U]);
}

static void sendDebug5(const char *text, int16_t n1, int16_t n2, int16_t n3, int16_t n4)
{
	uint8_t buf[130U];

	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 0U;
	buf[2U] = MMDVM_DEBUG5;

	uint8_t count = 3U;
	for (uint8_t i = 0U; text[i] != '\0'; i++, count++)
		buf[count] = text[i];

	buf[count++] = (n1 >> 8) & 0xFF;
	buf[count++] = (n1 >> 0) & 0xFF;

	buf[count++] = (n2 >> 8) & 0xFF;
	buf[count++] = (n2 >> 0) & 0xFF;

	buf[count++] = (n3 >> 8) & 0xFF;
	buf[count++] = (n3 >> 0) & 0xFF;

	buf[count++] = (n4 >> 8) & 0xFF;
	buf[count++] = (n4 >> 0) & 0xFF;

	buf[1U] = count;

	enqueueUSBData(buf, buf[1U]);
}
#else
#define sendDebug1(text) do {} while (0)
#define sendDebug2(text, n1) do {} while (0)
#define sendDebug3(text, n1, n2) do {} while (0)
#define sendDebug4(text, n1, n2, n3) do {} while (0)
#define sendDebug5(text, n1, n2, n3, n4) do {} while (0)
#endif

static void sendDMRLost(void)
{
	uint8_t buf[3U];

	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 3U;
	buf[2U] = MMDVM_DMR_LOST2;

	enqueueUSBData(buf, buf[1U]);
}

static void sendACK(void)
{
	uint8_t buf[4];
//	//SEGGER_RTT_printf(0, "sendACK\n");
	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 4U;
	buf[2U] = MMDVM_ACK;
	buf[3U] = com_requestbuffer[2U];

	enqueueUSBData(buf, buf[1U]);
}

static void sendNAK(uint8_t err)
{
	uint8_t buf[5];
	//SEGGER_RTT_printf(0, "sendNAK\n");
	buf[0U] = MMDVM_FRAME_START;
	buf[1U] = 5U;
	buf[2U] = MMDVM_NAK;
	buf[3U] = com_requestbuffer[2U];
	buf[4U] = err;

	enqueueUSBData(buf, buf[1U]);
}

static void hotspotStateMachine(void)
{
	static uint32_t rxFrameTicks = 0;

	dbgAct1(hotspotState);
	dbgPrint7("MMDVMHost", mmdvmHostIsConnected);

	switch(hotspotState)
	{
		case HOTSPOT_STATE_NOT_CONNECTED:
			dbgPrint3("NOT_CONN", rfFrameBufCount);
			// do nothing
			lastRxState = HOTSPOT_RX_UNKNOWN;
			rfFrameBufCount = 0;
			if (mmdvmHostIsConnected)
			{
				hotspotState = HOTSPOT_STATE_INITIALISE;
			}
			else
			{
				if ((nonVolatileSettings.hotspotType == HOTSPOT_TYPE_MMDVM) &&
						((fw_millis() - mmdvmHostLastActiveTick) > 10000))
				{
					wavbuffer_count = 0;

					leaveHotspotMenu();
					break;
				}
			}
			break;

		case HOTSPOT_STATE_INITIALISE:
			dbgPrint3("INIT", rfFrameBufCount);
#if defined(DEBUG_HS_SCREEN)
			ucClearBuf();
			ucRender();
#endif
			wavbuffer_read_idx = 0;
			wavbuffer_write_idx = 0;
			wavbuffer_count = 0;
			rfFrameBufCount = 0;

			hotspotState = HOTSPOT_STATE_RX_START;
			//SEGGER_RTT_printf(0, "STATE_INITIALISE -> STATE_RX\n");
			break;

		case HOTSPOT_STATE_RX_START:
			dbgPrint3("RX_START", rfFrameBufCount);
			//SEGGER_RTT_printf(0, "STATE_RX_START\n");

			// force immediate shutdown of Tx if we get here and the tx is on for some reason.
			if (trxIsTransmitting)
			{
				trxIsTransmitting = false;
				disableTransmission();
			}

			wavbuffer_read_idx = 0;
			wavbuffer_write_idx = 0;
			wavbuffer_count = 0;
			rxFrameTicks = fw_millis();

			hotspotState = HOTSPOT_STATE_RX_PROCESS;
			break;

		case HOTSPOT_STATE_RX_PROCESS:
			dbgPrint3("RX_PROCESS", rfFrameBufCount);

			if (mmdvmHostIsConnected)
			{
				// No activity from MMDVMHost
				if ((nonVolatileSettings.hotspotType == HOTSPOT_TYPE_MMDVM) &&
						((fw_millis() - mmdvmHostLastActiveTick) > 10000))
				{
					mmdvmHostIsConnected = false;
					hotspotState = HOTSPOT_STATE_NOT_CONNECTED;
					rfFrameBufCount = 0;
					wavbuffer_count = 0;

					leaveHotspotMenu();
					break;
				}
			}
			else
			{
				hotspotState = HOTSPOT_STATE_NOT_CONNECTED;
				rfFrameBufCount = 0;
				wavbuffer_count = 0;

				if (trxIsTransmitting)
				{
					trxIsTransmitting = false;
					disableTransmission();
				}
				break;
			}

        	if (rfFrameBufCount > 0)
        	{
        		if (MMDVMHostRxState == MMDVMHOST_RX_READY)
        		{
        			uint8_t rx_command = audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx][27 + 0x0c];
        			//SEGGER_RTT_printf(0, "RX_PROCESS cmd:%d buf:%d\n",rx_command,wavbuffer_count);

        			switch(rx_command)
					{
						case HOTSPOT_RX_IDLE:
							dbgPrint4("RX_IDLE", modemState, hotspotState, lastRxState);
							break;

						case HOTSPOT_RX_START:
							dbgPrint4("RX_START", modemState, hotspotState, lastRxState);
							//SEGGER_RTT_printf(0, "RX_START\n",rx_command,wavbuffer_count);
							updateScreen(rx_command);
							sendVoiceHeaderLC_Frame(audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx]);
							lastRxState = HOTSPOT_RX_START;
							rxFrameTicks = fw_millis();
							break;

						case HOTSPOT_RX_START_LATE:
							dbgPrint4("RX_START_L", modemState, hotspotState, lastRxState);
							//SEGGER_RTT_printf(0, "RX_START_LATE\n");
							updateScreen(rx_command);
							sendVoiceHeaderLC_Frame(audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx]);
							lastRxState = HOTSPOT_RX_START_LATE;
							rxFrameTicks = fw_millis();
							break;

						case HOTSPOT_RX_AUDIO_FRAME:
							dbgPrint4("RX_AUD_FRA", modemState, hotspotState, lastRxState);
							//SEGGER_RTT_printf(0, "HOTSPOT_RX_AUDIO_FRAME\n");
							hotspotSendVoiceFrame(audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx]);
							lastRxState = HOTSPOT_RX_AUDIO_FRAME;
							rxFrameTicks = fw_millis();
							break;

						case HOTSPOT_RX_STOP:
							dbgPrint4("RX_STOP", modemState, hotspotState, lastRxState);
							//SEGGER_RTT_printf(0, "RX_STOP\n");
							updateScreen(rx_command);
							sendTerminator_LC_Frame(audioAndHotspotDataBuffer.hotspotBuffer[rfFrameBufReadIdx]);
							lastRxState = HOTSPOT_RX_STOP;
							hotspotState = HOTSPOT_STATE_RX_END;
							break;

						case HOTSPOT_RX_IDLE_OR_REPEAT:
							dbgPrint4("RX_IDLE_REP", modemState, hotspotState, lastRxState);
							lastRxState = HOTSPOT_RX_IDLE_OR_REPEAT;
							//SEGGER_RTT_printf(0, "RX_IDLE_OR_REPEAT\n");
							/*
								switch(lastRxState)
								{
									case HOTSPOT_RX_START:
										sendVoiceHeaderLC_Frame(audioAndHotspotDataBuffer.hotspotBuffer[wavbuffer_read_idx]);
										break;
									case HOTSPOT_RX_STOP:
										sendTerminator_LC_Frame(audioAndHotspotDataBuffer.hotspotBuffer[wavbuffer_read_idx]);
										break;
									default:
										//SEGGER_RTT_printf(0, "ERROR: Unkown HOTSPOT_RX_IDLE_OR_REPEAT\n");
										break;
								}
							 */
							break;

						default:
							dbgPrint4("RX_UNKN", modemState, hotspotState, lastRxState);
							lastRxState = HOTSPOT_RX_UNKNOWN;
							//SEGGER_RTT_printf(0, "ERROR: Unkown Hotspot RX state\n");
							break;
					}

					rfFrameBufReadIdx = ((rfFrameBufReadIdx + 1) % HOTSPOT_BUFFER_COUNT);

					if (rfFrameBufCount > 0)
					{
						rfFrameBufCount--;
					}
        		}
        		else
        		{
        			dbgPrint4("NOT RX_READY", modemState, hotspotState, lastRxState);
        			// Rx Error NAK
        			hotspotState = HOTSPOT_STATE_RX_END;
        		}
        	}
        	else
        	{
        		// Timeout: no RF data for too long
        		if (((lastRxState == HOTSPOT_RX_AUDIO_FRAME) || (lastRxState == HOTSPOT_RX_START)) &&
        				((fw_millis() - rxFrameTicks) > 3000))
        		{
        			dbgPrint5("DMR_LOST", modemState, hotspotState, lastRxState);
        			sendDMRLost();
        			updateScreen(HOTSPOT_RX_IDLE);
        			lastRxState = HOTSPOT_RX_STOP;
        			hotspotState = HOTSPOT_STATE_RX_END;
    				rfFrameBufCount = 0;
    				//rfFrameBufReadIdx = 0;
    				//wavbuffer_count = 0;
					return;
        		}
        	}
			break;

		case HOTSPOT_STATE_RX_END:
			dbgPrint3("RX_END", rfFrameBufCount);
			hotspotState = HOTSPOT_STATE_RX_START;
			break;

		case HOTSPOT_STATE_TX_START_BUFFERING:
			dbgPrint3("TX_START_BUF", rfFrameBufCount);
			// If MMDVMHost tells us to go back to idle. (receiving)
			if (modemState == STATE_IDLE)
			{
				//modemState = STATE_DMR;
				//wavbuffer_read_idx = 0;
				//wavbuffer_write_idx = 0;
				//wavbuffer_count = 0;
				rfFrameBufCount = 0;
    			lastRxState = HOTSPOT_RX_IDLE;
				hotspotState = HOTSPOT_STATE_TX_SHUTDOWN;
				mmdvmHostIsConnected = false;
				disableTransmission();

				//SEGGER_RTT_printf(0, "modemState == STATE_IDLE: TX_START_BUFFERING -> HOTSPOT_STATE_TX_SHUTDOWN\n");
			}
			else
			{
				if (wavbuffer_count > TX_BUFFER_MIN_BEFORE_TRANSMISSION)
				{
					hotspotState = HOTSPOT_STATE_TRANSMITTING;
					//SEGGER_RTT_printf(0, "TX_START_BUFFERING -> TRANSMITTING %d\n",wavbuffer_count);
					enableTransmission();
					updateScreen(HOTSPOT_RX_IDLE);
				}
				else
				{
					if (--timeoutCounter == 0)
					{
						sendDMRLost();
						hotspotState = HOTSPOT_STATE_INITIALISE;
						//SEGGER_RTT_printf(0, "Timeout while buffering TX_START_BUFFERING -> HOTSPOT_STATE_INITIALISE\n");
					}
				}
			}
			break;

		case HOTSPOT_STATE_TRANSMITTING:
			dbgPrint3("TRANSMITTING", rfFrameBufCount);
			// Stop transmitting when there is no data in the buffer or if MMDVMHost sends the idle command
			if (wavbuffer_count == 0 || modemState == STATE_IDLE)
			{
				hotspotState = HOTSPOT_STATE_TX_SHUTDOWN;
				//SEGGER_RTT_printf(0, "TRANSMITTING -> TX_SHUTDOWN %d %d\n",wavbuffer_count,modemState);
				trxIsTransmitting = false;
			}
			break;

		case HOTSPOT_STATE_TX_SHUTDOWN:
			dbgPrint3("TX_SHUTDOWN", rfFrameBufCount);
			if (txstopdelay > 0)
			{
				txstopdelay--;
				if (wavbuffer_count > 0)
				{
					// restart
					//SEGGER_RTT_printf(0, "Restarting transmission %d\n",wavbuffer_count);
					enableTransmission();
					timeoutCounter = TX_BUFFERING_TIMEOUT;
					hotspotState = HOTSPOT_STATE_TX_START_BUFFERING;
				}
			}
			else
			{
				if ((slot_state < DMR_STATE_TX_START_1) ||
						((modemState == STATE_IDLE) && trxIsTransmitting)) // MMDVMHost asked to go back to IDLE (mostly on shutdown)
				{
					disableTransmission();
					hotspotState = HOTSPOT_STATE_RX_START;
					//SEGGER_RTT_printf(0, "TX_SHUTDOWN -> STATE_RX\n");
					updateScreen(HOTSPOT_RX_IDLE);

					/*
					wavbuffer_read_idx=0;
					wavbuffer_write_idx=0;
					wavbuffer_count=0;
					*/
				}
			}
			break;
	}
}

static uint8_t setFreq(volatile const uint8_t* data, uint8_t length)
{
	// satellite frequencies banned frequency ranges
	const int BAN1_MIN  = 14580000;
	const int BAN1_MAX  = 14600000;
	const int BAN2_MIN  = 43500000;
	const int BAN2_MAX  = 43800000;
	uint32_t fRx, fTx;

	//SEGGER_RTT_printf(0, "setFreq\n");
	hotspotState = HOTSPOT_STATE_INITIALISE;

	if (!mmdvmHostIsConnected)
		mmdvmHostIsConnected = true;

//	displayLightOverrideTimeout(-1);// turn the backlight on permanently

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

	fRx = (data[1U] << 0 | data[2U] << 8  | data[3U] << 16 | data[4U] << 24) / 10;
	fTx = (data[5U] << 0 | data[6U] << 8  | data[7U] << 16 | data[8U] << 24) / 10;

//	//SEGGER_RTT_printf(0, "Tx freq = %d, Rx freq = %d, Power = %d\n",fRx,fRx,rf_power);


	if ((fTx >= BAN1_MIN && fTx <= BAN1_MAX) || (fTx >= BAN2_MIN && fTx <= BAN2_MAX))
	{
		return 4U;// invalid frequency
	}

	if (trxCheckFrequencyInAmateurBand(fRx) && trxCheckFrequencyInAmateurBand(fTx))
	{
		freq_rx = fRx;
		freq_tx = fTx;
		trxSetFrequency(freq_rx, freq_tx, DMR_MODE_ACTIVE);// Override the default assumptions about DMR mode based on frequency
	}
	else
	{
		return 4U;// invalid frequency
	}

	hotspotPowerLevel = nonVolatileSettings.txPowerLevel;
	// If the power level sent by MMDVMHost is 255 it means the user has left the setting at 100% and potentially does not realise that there is even a setting for this
	// As the GD-77 can't be run at full power, this power level will be ignored and instead the level specified for normal operation will be used.
	if (rf_power != 255)
	{
		savedPowerLevel = nonVolatileSettings.txPowerLevel;

		if (rf_power < 50)
		{
			hotspotPowerLevel = rf_power / 16;
		}
		else
		{
			hotspotPowerLevel = (rf_power / 50) + 2;
		}
		trxSetPowerFromLevel(hotspotPowerLevel);
	}

  return 0U;
}

static bool hasRXOverflow(void)
{
	return false;// TO DO.
}
static bool hasTXOverflow(void)
{
	return false;// TO DO.
}

static void getStatus(void)
{
	uint8_t buf[16];
//	//SEGGER_RTT_printf(0, "getStatus\n");
	// Send all sorts of interesting internal values
	buf[0U]  = MMDVM_FRAME_START;
	buf[1U]  = 13U;
	buf[2U]  = MMDVM_GET_STATUS;
	//buf[3U]  = 0x00U;
	buf[3U]  = 0x02U;// DMR ENABLED
	buf[4U]  = modemState;
	buf[5U]  = ( ((hotspotState == HOTSPOT_STATE_TX_START_BUFFERING) ||
					(hotspotState == HOTSPOT_STATE_TRANSMITTING) ||
					(hotspotState == HOTSPOT_STATE_TX_SHUTDOWN)) ) ? 0x01U : 0x00U;

	if (hasRXOverflow())
	{
		buf[5U] |= 0x04U;
	}

	if (hasTXOverflow())
	{
		buf[5U] |= 0x08U;
	}

	buf[6U]  = 0U;// No DSTAR

	buf[7U]  = 10U;// DMR Simplex
	buf[8U]  = (HOTSPOT_BUFFER_COUNT - wavbuffer_count);

	buf[9U]  = 0U;// No YSF
	buf[10U] = 0U;// No P25
	buf[11U] = 0U;// no NXDN
	buf[12U] = 0U;// no POCSAG

	//SEGGER_RTT_printf(0, "getStatus buffers=%d\n",s_ComBuf[8U]);

	if (!mmdvmHostIsConnected)
	{
		hotspotState = HOTSPOT_STATE_INITIALISE;
		mmdvmHostIsConnected = true;
	}

	enqueueUSBData(buf, buf[1U]);
}

static uint8_t setConfig(volatile const uint8_t* data, uint8_t length)
{
	//SEGGER_RTT_printf(0, "setConfig \n");

	uint8_t txDelay = data[2U];
	if (txDelay > 50U)
	{
		return 4U;
	}

	if (data[3U] != STATE_IDLE && data[3U] != STATE_DMR)
	{
		return 4U;// only DMR mode supported
	}

	uint8_t colorCode = data[6U];
	if (colorCode > 15U)
	{
		return 4U;
	}

	trxSetDMRColourCode(colorCode);

	/* To Do
	 m_cwIdTXLevel = data[5U]>>2;
     uint8_t dmrTXLevel    = data[10U];
     io.setDeviations(dstarTXLevel, dmrTXLevel, ysfTXLevel, p25TXLevel, nxdnTXLevel, pocsagTXLevel, ysfLoDev);
     dmrDMOTX.setTXDelay(txDelay);
     */

	modemState = (MMDVM_STATE)data[3U];

	if ((modemState == STATE_DMR) && (hotspotState == HOTSPOT_STATE_NOT_CONNECTED))
	{
		hotspotState = HOTSPOT_STATE_INITIALISE;

		if (!mmdvmHostIsConnected)
			mmdvmHostIsConnected = true;
	}

	return 0U;
}

static uint8_t setMode(volatile const uint8_t* data, uint8_t length)
{
	//SEGGER_RTT_printf(0, "MMDVM SetMode len:%d %02X %02X %02X %02X %02X %02X %02X %02X\n",length,data[0U],data[1U],data[2U],data[3U],data[4U],data[5U],data[6U],data[7U]);

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

static void getVersion(void)
{
	uint8_t buf[128];
	uint8_t count = 0U;

	buf[0U]  = MMDVM_FRAME_START;
	buf[1U]  = 0U;
	buf[2U]  = MMDVM_GET_VERSION;
	buf[3U]  = PROTOCOL_VERSION;

	count = 4U;

	for (uint8_t i = 0U; HARDWARE[i] != 0x00U; i++, count++)
		buf[count] = HARDWARE[i];

	buf[1U] = count;

	enqueueUSBData(buf, buf[1U]);
}

static uint8_t handleDMRShortLC(volatile const uint8_t *data, uint8_t length)
{
	////	uint8_t LCBuf[5];
	////	DMRShortLC_decode((uint8_t *) com_requestbuffer + 3U,LCBuf);
	////	//SEGGER_RTT_printf(0, "MMDVM ShortLC\n %02X %02X %02X %02X %02X\n",LCBuf[0U],LCBuf[1U],LCBuf[2U],LCBuf[3U],LCBuf[4U],LCBuf[5U]);

	return 0U;
}

static void handleHotspotRequest(void)
{
//	//SEGGER_RTT_printf(0, "handleHotspotRequest 0x%0x 0x%0x 0x%0x\n",com_requestbuffer[0],com_requestbuffer[1],com_requestbuffer[2]);
	if (com_requestbuffer[0] == MMDVM_FRAME_START)
	{
		uint8_t err = 2U;

		dbgAct0(com_requestbuffer[2U]);

		mmdvmHostLastActiveTick = fw_millis();

		//SEGGER_RTT_printf(0, "MMDVM %02x\n",com_requestbuffer[2]);
		switch(com_requestbuffer[2U])
		{
			case MMDVM_GET_STATUS:
				dbgPrint2("GET_STATUS");
				getStatus();
				break;

			case MMDVM_GET_VERSION:
				dbgPrint2("GET_VERSION");
				getVersion();
				break;

			case MMDVM_SET_CONFIG:
				dbgPrint2("SET_CONFIG");
				err = setConfig(com_requestbuffer + 3U, com_requestbuffer[1U] - 3U);
				if (err == 0U)
				{
				  sendACK();
				  updateScreen(HOTSPOT_RX_IDLE);
				}
				else
				{
				  sendNAK(err);
				}
				break;

			case MMDVM_SET_MODE:
				dbgPrint2("SET_MODE");
				err = setMode(com_requestbuffer + 3U, com_requestbuffer[1U] - 3U);
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
				dbgPrint2("SET_FREQ");
	            err = setFreq(com_requestbuffer + 3U, com_requestbuffer[1U] - 3U);
	            if (err == 0U)
	            {
	              sendACK();
	              updateScreen(HOTSPOT_RX_IDLE);
	            }
	            else
	            {
	              sendNAK(err);
	            }

				break;

			case MMDVM_CAL_DATA:
				dbgPrint2("CAL_DATA");
				//SEGGER_RTT_printf(0, "MMDVM_CAL_DATA\n");
				sendNAK(err);
				break;

			case MMDVM_SEND_CWID:
				dbgPrint2("SEND_CWID");
				err = 5U;
				if (modemState == STATE_IDLE)
				{
					err = 0U;
				}
				if (err != 0U)
				{
					sendNAK(err);
				}
				//SEGGER_RTT_printf(0, "MMDVM_SEND_CWID\n");
				//sendACK();
				break;

			case MMDVM_DSTAR_HEADER:
			case MMDVM_DSTAR_DATA:
			case MMDVM_DSTAR_EOT:
				dbgPrint2("DSTAR");
				sendNAK(err);
				break;

			case MMDVM_DMR_DATA1: // We are a simplex hotspot, no TS1 support
				dbgPrint2("DMR_DATA1");
//				//SEGGER_RTT_printf(0, "MMDVM_DMR_DATA1\n");
//				hotspotModeReceiveNetFrame((uint8_t *)com_requestbuffer, 1U);
				sendNAK(err);
				break;

			case MMDVM_DMR_DATA2:
				dbgPrint2("DMR_DATA2");
				//SEGGER_RTT_printf(0, "MMDVM_DMR_DATA2\n");
				err = hotspotModeReceiveNetFrame(com_requestbuffer, 2U);
				if (err == 0U)
				{
					sendACK();
				}
				else
				{
					sendNAK(err);
				}
				break;

			case MMDVM_DMR_START: // Only for duplex
				dbgPrint2("DMR_START");
//				//SEGGER_RTT_printf(0, "MMDVM_DMR_START\n");
				sendACK();
				break;

			case MMDVM_DMR_SHORTLC: // Only for duplex
				dbgPrint2("DMR_SHORTLC");
//				//SEGGER_RTT_printf(0, "MMDVM_DMR_SHORTLC\n");
				err = handleDMRShortLC((uint8_t *)com_requestbuffer, com_requestbuffer[1U]);
				if (err == 0U)
				{
					sendACK();
				}
				else
				{
					sendNAK(err);
				}
				break;

			case MMDVM_DMR_ABORT: // Only for duplex
				dbgPrint2("DMR_ABORT");
//				//SEGGER_RTT_printf(0, "MMDVM_DMR_ABORT\n");
				sendACK();
				break;

#if 0  // Serial passthrough (a.k.a Nextion serial port), unhandled
			case MMDVM_SERIAL:
//				//SEGGER_RTT_printf(0, "MMDVM_SERIAL\n");
//				//displayDataBytes(com_requestbuffer, com_requestbuffer[1]);
//				//sendACK();
				break;
#endif

			case MMDVM_YSF_DATA:
				dbgPrint2("YSF");
				sendNAK(err);
				break;

			case MMDVM_P25_HDR:
			case MMDVM_P25_LDU:
				dbgPrint2("P25");
				sendNAK(err);
				break;

			case MMDVM_NXDN_DATA:
				dbgPrint2("NXDN");
				sendNAK(err);
				break;

			case MMDVM_POCSAG_DATA:
				dbgPrint2("POCSAG");
				sendNAK(err);
				break;

			case MMDVM_TRANSPARENT: // Do nothing, stay silent
			case MMDVM_QSO_INFO:
				dbgPrint2("QSO_INFO");
				break;

			default:
			{
				char buf[16];
				sprintf(buf, "UNKN: %d", com_requestbuffer[2U]);
				dbgPrint2(buf);
				//SEGGER_RTT_printf(0, "Unhandled command type %d\n",com_requestbuffer[2]);
				sendNAK(1U);
				//sendNAK(com_requestbuffer[2]);
			}
				break;
		}

		com_requestbuffer[0U] = 0xFFU; // Invalidate this one
	}
	else
	{
		//SEGGER_RTT_printf(0, "Invalid MMDVM header byte %d\n",com_requestbuffer[0]);
	}
}
