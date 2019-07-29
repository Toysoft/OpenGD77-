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

#include <SeggerRTT/RTT/SEGGER_RTT.h>

uint32_t freq_rx;
uint32_t freq_tx;
uint8_t rf_power;


static void updateScreen();
static void handleEvent(int buttons, int keys, int events);

int menuHotspotMode(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
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
		settingsUsbMode = USB_MODE_CPS;
		menuSystemPopAllAndDisplayRootMenu();
		return;
	}
}



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

static void sendACK(uint8_t* s_ComBuf)
{
  SEGGER_RTT_printf(0, "sendACK\r\n");
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

static int dmrDMOTX_getSpace()
{
#warning DONT KNOW WHAT VALUE IS NEEDED HERE
	return 64;
}

static void getStatus(uint8_t* s_ComBuf)
{
//	SEGGER_RTT_printf(0, "getStatus\r\n");

 // io.resetWatchdog();

#warning NOT SURE WHAT MODEM STATE DOES
	uint8_t m_modemState=0x00;

  // Send all sorts of interesting internal values
	s_ComBuf[0U]  = MMDVM_FRAME_START;
	s_ComBuf[1U]  = 13U;
	s_ComBuf[2U]  = MMDVM_GET_STATUS;

	s_ComBuf[3U]  = 0x00U;

	s_ComBuf[3U] |= 0x02U;// DMR ENABLED


	s_ComBuf[4U]  = m_modemState;

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
	s_ComBuf[8U] = 	dmrDMOTX_getSpace();
	s_ComBuf[9U] = 	0U;// No YSF
	s_ComBuf[10U] = 0U;// No P25
	s_ComBuf[11U] = 0U;// no NXDN
	s_ComBuf[12U] = 0U;// no POCSAG
	USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, s_ComBuf, s_ComBuf[1]);
}

static uint8_t setConfig(const uint8_t* data, uint8_t length)
{
	SEGGER_RTT_printf(0, "setConfig\r\n");

/*
  if (length < 13U)
    return 4U;

 */
  /*
  bool ysfLoDev  = (data[0U] & 0x08U) == 0x08U;
  bool simplex   = (data[0U] & 0x80U) == 0x80U;

  m_debug = (data[0U] & 0x10U) == 0x10U;

  bool dstarEnable  = (data[1U] & 0x01U) == 0x01U;
  bool dmrEnable    = (data[1U] & 0x02U) == 0x02U;
  bool ysfEnable    = (data[1U] & 0x04U) == 0x04U;
  bool p25Enable    = (data[1U] & 0x08U) == 0x08U;
  bool nxdnEnable   = (data[1U] & 0x10U) == 0x10U;
  bool pocsagEnable = (data[1U] & 0x20U) == 0x20U;

  uint8_t txDelay = data[2U];
  if (txDelay > 50U)
    return 4U;

  MMDVM_STATE modemState = MMDVM_STATE(data[3U]);

  if (modemState != STATE_IDLE && modemState != STATE_DSTAR && modemState != STATE_DMR && modemState != STATE_YSF && modemState != STATE_P25 && modemState != STATE_NXDN && modemState != STATE_POCSAG && modemState != STATE_DSTARCAL && modemState != STATE_DMRCAL && modemState != STATE_DMRDMO1K && modemState != STATE_INTCAL && modemState != STATE_RSSICAL && modemState != STATE_POCSAGCAL)
    return 4U;
  if (modemState == STATE_DSTAR && !dstarEnable)
    return 4U;
  if (modemState == STATE_DMR && !dmrEnable)
    return 4U;
  if (modemState == STATE_YSF && !ysfEnable)
    return 4U;
  if (modemState == STATE_P25 && !p25Enable)
    return 4U;
  if (modemState == STATE_NXDN && !nxdnEnable)
    return 4U;
  if (modemState == STATE_POCSAG && !pocsagEnable)
    return 4U;

  uint8_t colorCode = data[6U];
  if (colorCode > 15U)
    return 4U;

#if defined(DUPLEX)
  uint8_t dmrDelay = data[7U];
#endif

  m_cwIdTXLevel = data[5U]>>2;

  uint8_t dstarTXLevel  = data[9U];
  uint8_t dmrTXLevel    = data[10U];
  uint8_t ysfTXLevel    = data[11U];
  uint8_t p25TXLevel    = data[12U];
  uint8_t nxdnTXLevel   = 128U;
  uint8_t pocsagTXLevel = 128U;

  if (length >= 16U)
    nxdnTXLevel = data[15U];

  if (length >= 18U)
    pocsagTXLevel = data[17U];

  io.setDeviations(dstarTXLevel, dmrTXLevel, ysfTXLevel, p25TXLevel, nxdnTXLevel, pocsagTXLevel, ysfLoDev);

  m_dstarEnable  = dstarEnable;
  m_dmrEnable    = dmrEnable;
  m_ysfEnable    = ysfEnable;
  m_p25Enable    = p25Enable;
  m_nxdnEnable   = nxdnEnable;
  m_pocsagEnable = pocsagEnable;

  if (modemState == STATE_DMRCAL || modemState == STATE_DMRDMO1K || modemState == STATE_RSSICAL || modemState == STATE_INTCAL) {
    m_dmrEnable = true;
    m_modemState = STATE_DMR;
    m_calState = modemState;
    if (m_firstCal)
      io.updateCal();
    if (modemState == STATE_RSSICAL)
      io.ifConf(STATE_DMR, true);
  } else if (modemState == STATE_POCSAGCAL) {
    m_pocsagEnable = true;
    m_modemState = STATE_POCSAG;
    m_calState = modemState;
    if (m_firstCal)
      io.updateCal();
  }
  else {
    m_modemState = modemState;
    m_calState = STATE_IDLE;
  }

  m_duplex      = !simplex;

#if !defined(DUPLEX)
  if (m_duplex && m_calState == STATE_IDLE && modemState != STATE_DSTARCAL) {
    DEBUG1("Full duplex not supported with this firmware");
    return 6U;
  }
#endif

  dstarTX.setTXDelay(txDelay);
  ysfTX.setTXDelay(txDelay);
  p25TX.setTXDelay(txDelay);
  nxdnTX.setTXDelay(txDelay);
  pocsagTX.setTXDelay(txDelay);
  dmrDMOTX.setTXDelay(txDelay);

#if defined(DUPLEX)
  dmrTX.setColorCode(colorCode);
  dmrRX.setColorCode(colorCode);
  dmrRX.setDelay(dmrDelay);
  dmrIdleRX.setColorCode(colorCode);
#endif

  dmrDMORX.setColorCode(colorCode);

  io.setLoDevYSF(ysfLoDev);

  if (!m_firstCal || (modemState != STATE_DMRCAL && modemState != STATE_DMRDMO1K && modemState != STATE_RSSICAL && modemState != STATE_INTCAL && modemState != STATE_POCSAGCAL)) {
    if(m_dstarEnable)
      io.ifConf(STATE_DSTAR, true);
    else if(m_dmrEnable)
      io.ifConf(STATE_DMR, true);
    else if(m_ysfEnable)
      io.ifConf(STATE_YSF, true);
    else if(m_p25Enable)
      io.ifConf(STATE_P25, true);
    else if(m_nxdnEnable)
      io.ifConf(STATE_NXDN, true);
    else if(m_pocsagEnable)
      io.ifConf(STATE_POCSAG, true);
  }

  io.start();
#if defined(ENABLE_DEBUG)
  io.printConf();
#endif

  if (modemState == STATE_DMRCAL || modemState == STATE_DMRDMO1K || modemState == STATE_RSSICAL || modemState == STATE_INTCAL || modemState == STATE_POCSAGCAL)
    m_firstCal = true;
*/
  return 0U;
}

uint8_t setMode(const uint8_t* data, uint8_t length)
{
	SEGGER_RTT_printf(0, "setMode\r\n");
	/*
  if (length < 1U)
  {
    return 4U;
  }

  MMDVM_STATE modemState = MMDVM_STATE(data[0U]);
  MMDVM_STATE tmpState;

  if (modemState == m_modemState)
    return 0U;

  if (modemState != STATE_IDLE && modemState != STATE_DSTAR && modemState != STATE_DMR && modemState != STATE_YSF && modemState != STATE_P25 && modemState != STATE_NXDN && modemState != STATE_POCSAG && modemState != STATE_DSTARCAL && modemState != STATE_DMRCAL && modemState != STATE_DMRDMO1K && modemState != STATE_RSSICAL && modemState != STATE_INTCAL && modemState != STATE_POCSAGCAL)
    return 4U;
  if (modemState == STATE_DSTAR && !m_dstarEnable)
    return 4U;
  if (modemState == STATE_DMR && !m_dmrEnable)
    return 4U;
  if (modemState == STATE_YSF && !m_ysfEnable)
    return 4U;
  if (modemState == STATE_P25 && !m_p25Enable)
    return 4U;
  if (modemState == STATE_NXDN && !m_nxdnEnable)
    return 4U;
  if (modemState == STATE_POCSAG && !m_pocsagEnable)
    return 4U;

  if (modemState == STATE_DMRCAL || modemState == STATE_DMRDMO1K || modemState == STATE_RSSICAL || modemState == STATE_INTCAL) {
    m_dmrEnable = true;
    tmpState = STATE_DMR;
    m_calState = modemState;
    if (m_firstCal)
      io.updateCal();
  } else if (modemState == STATE_POCSAGCAL) {
    m_pocsagEnable = true;
    tmpState = STATE_POCSAG;
    m_calState = modemState;
    if (m_firstCal)
      io.updateCal();
  }
  else {
    tmpState  = modemState;
    m_calState = STATE_IDLE;
  }

  setMode(tmpState);
*/
  return 0U;
}

static void getVersion(uint8_t s_ComBuf[])
{
	SEGGER_RTT_printf(0, "getVersion\r\n");

	const char HOTSPOT_NAME[] = "OpenGD77";
	s_ComBuf[1]= 4 + strlen(HOTSPOT_NAME) - 1;// minus 1 because there is no terminator
	s_ComBuf[3]= PROTOCOL_VERSION;
	strcpy((char *)&s_ComBuf[4],HOTSPOT_NAME);
	USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, s_ComBuf, s_ComBuf[1]);
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
				SEGGER_RTT_printf(0, "MMDVM_DMR_DATA1\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_DMR_LOST1:
				SEGGER_RTT_printf(0, "MMDVM_DMR_LOST1\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_DMR_DATA2:
				SEGGER_RTT_printf(0, "MMDVM_DMR_DATA2\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_DMR_LOST2:
				SEGGER_RTT_printf(0, "MMDVM_DMR_LOST2\r\n");
				sendACK(s_ComBuf);
				break;
			case MMDVM_DMR_SHORTLC:
				SEGGER_RTT_printf(0, "MMDVM_DMR_SHORTLC\r\n");
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
				sendACK(s_ComBuf);
				break;
		}
	}
	else
	{
		s_ComBuf[0] = '?';
		USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, s_ComBuf, 1);
	}
}

