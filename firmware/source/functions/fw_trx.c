/*
 * Copyright (C)2019 Kai Ludwig, DG4KLU
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

#include <hardware/fw_AT1846S.h>
#include <hardware/fw_HR-C6000.h>
#include <user_interface/menuSystem.h>
#include "fw_trx.h"
#include "fw_settings.h"
#include "fw_calibration.h"


int trx_measure_count = 0;
volatile bool trxIsTransmitting = false;
uint32_t trxTalkGroupOrPcId = 9;// Set to local TG just in case there is some problem with it not being loaded
uint32_t trxDMRID = 0;// Set ID to 0. Not sure if its valid. This value needs to be loaded from the codeplug.
int txstopdelay = 0;
volatile bool trxIsTransmittingTone = false;
static uint16_t txPower;
static bool analogSignalReceived = false;

const int RADIO_VHF_MIN			=	13400000;
const int RADIO_VHF_MAX			=	17400000;
const int RADIO_UHF_MIN			=	40000000;
const int RADIO_UHF_MAX			=	52000000;

static int currentMode = RADIO_MODE_NONE;
static bool currentBandWidthIs25kHz = BANDWIDTH_12P5KHZ;
static int currentRxFrequency = 14400000;
static int currentTxFrequency = 14400000;
static int currentCC = 1;
static uint8_t squelch = 0x00;
static bool rxCTCSSactive = false;
static uint8_t ANTENNA_SWITCH_RX = 0;
static uint8_t ANTENNA_SWITCH_TX = 1;

// AT-1846 native values for Rx
static uint8_t rx_fl_l;
static uint8_t rx_fl_h;
static uint8_t rx_fh_l;
static uint8_t rx_fh_h;

// AT-1846 native values for Tx
static uint8_t tx_fl_l;
static uint8_t tx_fl_h;
static uint8_t tx_fh_l;
static uint8_t tx_fh_h;

volatile uint8_t trxRxSignal;
volatile uint8_t trxRxNoise;

static uint8_t trxSaveVoiceGainTx = 0xff;
static uint16_t trxSaveDeviation = 0xff;

int trxDMRMode = DMR_MODE_ACTIVE;// Active is for simplex
static volatile bool txPAEnabled=false;

static int trxCurrentDMRTimeSlot;

const int trxDTMFfreq1[] = { 1336, 1209, 1336, 1477, 1209, 1336, 1477, 1209, 1336, 1477, 1633, 1633, 1633, 1633, 1209, 1477 };
const int trxDTMFfreq2[] = {  941,  697,  697,  697,  770,  770,  770,  852,  852,  852,  697,  770,  852,  941,  941,  941 };

calibrationPowerValues_t trxPowerSettings;

int	trxGetMode()
{
	return currentMode;
}

int	trxGetBandwidthIs25kHz()
{
	return currentBandWidthIs25kHz;
}

void trxSetModeAndBandwidth(int mode, bool bandwidthIs25kHz)
{
	if ((mode != currentMode) || (bandwidthIs25kHz != currentBandWidthIs25kHz))
	{
		currentMode=mode;

		currentBandWidthIs25kHz=bandwidthIs25kHz;

		switch(mode)
		{
		case RADIO_MODE_NONE:
			// not truely off
			GPIO_PinWrite(GPIO_RX_audio_mux, Pin_RX_audio_mux, 0); // connect AT1846S audio to HR_C6000
			terminate_sound();
			terminate_digital();

			I2C_AT1846_SetMode();
			I2C_AT1846_SetBandwidth();
			trxUpdateC6000Calibration();
			trxUpdateAT1846SCalibration();
			break;
		case RADIO_MODE_ANALOG:
			GPIO_PinWrite(GPIO_TX_audio_mux, Pin_TX_audio_mux, 0); // Connect mic to mic input of AT-1846
			GPIO_PinWrite(GPIO_RX_audio_mux, Pin_RX_audio_mux, 1); // connect AT1846S audio to speaker
			terminate_sound();
			terminate_digital();

			I2C_AT1846_SetMode();
			I2C_AT1846_SetBandwidth();
			trxUpdateC6000Calibration();
			trxUpdateAT1846SCalibration();
			break;
		case RADIO_MODE_DIGITAL:
			I2C_AT1846_SetMode();
			I2C_AT1846_SetBandwidth();
			trxUpdateC6000Calibration();
			trxUpdateAT1846SCalibration();
			GPIO_PinWrite(GPIO_TX_audio_mux, Pin_TX_audio_mux, 1); // Connect mic to MIC_P input of HR-C6000
			GPIO_PinWrite(GPIO_RX_audio_mux, Pin_RX_audio_mux, 0); // connect AT1846S audio to HR_C6000
			init_sound();
			init_digital();
			break;
		}
	}
}

bool trxCheckFrequencyIsSupportedByTheRadioHardware(int frequency)
{
	return (((frequency >= RADIO_VHF_MIN) && (frequency < RADIO_VHF_MAX)) || ((frequency >= RADIO_UHF_MIN) && (frequency < RADIO_UHF_MAX)));
}

bool trxCheckFrequencyIsVHF(int frequency)
{
	return ((frequency >= RADIO_VHF_MIN) && (frequency < RADIO_VHF_MAX));
}

bool trxCheckFrequencyIsUHF(int frequency)
{
	return ((frequency >= RADIO_UHF_MIN) && (frequency < RADIO_UHF_MAX));
}

bool trxCheckFrequencyInAmateurBand(int tmp_frequency)
{
	return ((tmp_frequency>=BAND_VHF_MIN) && (tmp_frequency<=BAND_VHF_MAX)) || ((tmp_frequency>=BAND_UHF_MIN) && (tmp_frequency<=BAND_UHF_MAX));
}

void trxReadRSSIAndNoise()
{
	read_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x1b,(uint8_t *)&trxRxSignal,(uint8_t *)&trxRxNoise);
}

int trx_carrier_detected()
{
	uint8_t squelch=45;

	trxReadRSSIAndNoise();

	// check for variable squelch control
	if (currentChannelData->sql!=0)
	{
		if (currentChannelData->sql==1)
		{
			//open_squelch = true;
		}
		else
		{
			squelch =  70 - (((currentChannelData->sql-1)*11)>>2);
			//open_squelch = false;
		}
	}

	if (trxRxNoise < squelch)
	{
		return 1;
	}
	else
	{
		return 0;
	}

}

void trx_check_analog_squelch()
{
	trx_measure_count++;
	if (trx_measure_count==50)
	{
		uint8_t squelch=45;

		trxReadRSSIAndNoise();

		// check for variable squelch control
		if (currentChannelData->sql!=0)
		{
			if (currentChannelData->sql==1)
			{
				//open_squelch = true;
			}
			else
			{
				squelch =  70 - (((currentChannelData->sql-1)*11)>>2);
				//open_squelch = false;
			}
		}

		if (trxRxNoise < squelch)
		{
			if (!analogSignalReceived)
			{
				analogSignalReceived = true;
				displayLightTrigger();
				GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 1);
			}
			if(!rxCTCSSactive || (rxCTCSSactive & trxCheckCTCSSFlag()))
			{
				GPIO_PinWrite(GPIO_audio_amp_enable, Pin_audio_amp_enable, 1); // speaker on
			}
		}
		else
		{
			analogSignalReceived = false;
			if (trxIsTransmittingTone == false)
			{
				GPIO_PinWrite(GPIO_audio_amp_enable, Pin_audio_amp_enable, 0); // speaker off
				GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
			}
		}

    	trx_measure_count=0;
	}
}

void trxSetFrequency(int fRx,int fTx, int dmrMode)
{
	if (currentRxFrequency!=fRx || currentTxFrequency!=fTx)
	{
		calibrationGetPowerForFrequency(fTx, &trxPowerSettings);
		trxSetPowerFromLevel(nonVolatileSettings.txPowerLevel);

		currentRxFrequency=fRx;
		currentTxFrequency=fTx;

		if (dmrMode==DMR_MODE_AUTO)
		{
			// Most DMR radios determine whether to use Active or Passive DMR depending on whether the Tx and Rx freq are the same
			// This prevents split simplex operation, but since no other radio appears to support split freq simplex
			// Its easier to do things the same way as othe radios, and revisit this again in the future if split freq simplex is required.
			if (currentRxFrequency == currentTxFrequency)
			{
				trxDMRMode = DMR_MODE_ACTIVE;
			}
			else
			{
				trxDMRMode = DMR_MODE_PASSIVE;
			}
		}
		else
		{
			trxDMRMode = dmrMode;
		}

		uint32_t f = currentRxFrequency * 0.16f;
		rx_fl_l = (f & 0x000000ff) >> 0;
		rx_fl_h = (f & 0x0000ff00) >> 8;
		rx_fh_l = (f & 0x00ff0000) >> 16;
		rx_fh_h = (f & 0xff000000) >> 24;

		f = currentTxFrequency * 0.16f;
		tx_fl_l = (f & 0x000000ff) >> 0;
		tx_fl_h = (f & 0x0000ff00) >> 8;
		tx_fh_l = (f & 0x00ff0000) >> 16;
		tx_fh_h = (f & 0xff000000) >> 24;

		if (currentMode==RADIO_MODE_DIGITAL)
		{
			terminate_digital();
		}

		if (currentBandWidthIs25kHz)
		{
			// 25 kHz settings
			write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x30, 0x70, 0x06 | squelch); // RX off
		}
		else
		{
			// 12.5 kHz settings
			write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x30, 0x40, 0x06 | squelch); // RX off
		}
		write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x05, 0x87, 0x63); // select 'normal' frequency mode

		write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x29, rx_fh_h, rx_fh_l);
		write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x2a, rx_fl_h, rx_fl_l);
		write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x49, 0x0C, 0x15); // setting SQ open and shut threshold

		if (currentBandWidthIs25kHz)
		{
			// 25 kHz settings
			write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x30, 0x70, 0x26 | squelch); // RX on
		}
		else
		{
			// 12.5 kHz settings
			write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x30, 0x40, 0x26 | squelch); // RX on
		}

		trxUpdateC6000Calibration();
		trxUpdateAT1846SCalibration();

		if (!txPAEnabled)
		{
			if (trxCheckFrequencyIsUHF(currentRxFrequency))
			{
				GPIO_PinWrite(GPIO_VHF_RX_amp_power, Pin_VHF_RX_amp_power, 0);
				GPIO_PinWrite(GPIO_UHF_RX_amp_power, Pin_UHF_RX_amp_power, 1);
			}
			else
			{
				GPIO_PinWrite(GPIO_VHF_RX_amp_power, Pin_VHF_RX_amp_power, 1);
				GPIO_PinWrite(GPIO_UHF_RX_amp_power, Pin_UHF_RX_amp_power, 0);
			}
		}
		else
		{
			//SEGGER_RTT_printf(0, "ERROR Cant enable Rx when PA active\n");
		}

		if (currentMode==RADIO_MODE_DIGITAL)
		{
			init_digital();
		}
	}
}

int trxGetFrequency()
{
	if (trxIsTransmitting)
	{
		return currentTxFrequency;
	}
	else
	{
		return currentRxFrequency;
	}
}

void trx_setRX()
{
//	set_clear_I2C_reg_2byte_with_mask(0x30, 0xFF, 0x1F, 0x00, 0x00);
	if (currentMode == RADIO_MODE_ANALOG)
	{
		trx_activateRx();
	}

}

void trx_setTX()
{
	trxIsTransmitting=true;

	// RX pre-amp off
	GPIO_PinWrite(GPIO_VHF_RX_amp_power, Pin_VHF_RX_amp_power, 0);
	GPIO_PinWrite(GPIO_UHF_RX_amp_power, Pin_UHF_RX_amp_power, 0);

//	set_clear_I2C_reg_2byte_with_mask(0x30, 0xFF, 0x1F, 0x00, 0x00);
	if (currentMode == RADIO_MODE_ANALOG)
	{
		trx_activateTx();
	}

}

void trx_activateRx()
{
	//SEGGER_RTT_printf(0, "trx_activateRx\n");
    DAC_SetBufferValue(DAC0, 0U, 0U);// PA drive power to zero

    // Possibly quicker to turn them both off, than to check which on is on and turn that one off
	GPIO_PinWrite(GPIO_VHF_TX_amp_power, Pin_VHF_TX_amp_power, 0);// VHF PA off
	GPIO_PinWrite(GPIO_UHF_TX_amp_power, Pin_UHF_TX_amp_power, 0);// UHF PA off

    GPIO_PinWrite(GPIO_RF_ant_switch, Pin_RF_ant_switch, ANTENNA_SWITCH_RX);
	txPAEnabled=false;


    if (trxCheckFrequencyIsUHF(currentRxFrequency))
	{
		GPIO_PinWrite(GPIO_VHF_RX_amp_power, Pin_VHF_RX_amp_power, 0);// VHF pre-amp off
		GPIO_PinWrite(GPIO_UHF_RX_amp_power, Pin_UHF_RX_amp_power, 1);// UHF pre-amp on
	}
    else
	{
		GPIO_PinWrite(GPIO_VHF_RX_amp_power, Pin_VHF_RX_amp_power, 1);// VHF pre-amp on
		GPIO_PinWrite(GPIO_UHF_RX_amp_power, Pin_UHF_RX_amp_power, 0);// UHF pre-amp off
	}

	if (currentBandWidthIs25kHz)
	{
		write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x30, 0x70, 0x06); 		// 25 kHz settings // RX off
	}
	else
	{
		write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x30, 0x40, 0x06); 		// 12.5 kHz settings // RX off
	}

	write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x29, rx_fh_h, rx_fh_l);
	write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x2a, rx_fl_h, rx_fl_l);

	if (currentBandWidthIs25kHz)
	{
		write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x30, 0x70, 0x26); // 25 kHz settings // RX on
	}
	else
	{
		write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x30, 0x40, 0x26); // 12.5 kHz settings // RX on
	}
}

void trx_activateTx()
{
	txPAEnabled=true;
	write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x29, tx_fh_h, tx_fh_l);
	write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x2a, tx_fl_h, tx_fl_l);

	set_clear_I2C_reg_2byte_with_mask(0x30, 0xFF, 0x1F, 0x00, 0x00); // Clear Tx and Rx bits
	if (currentMode == RADIO_MODE_ANALOG)
	{
		set_clear_I2C_reg_2byte_with_mask(0x30, 0xFF, 0x1F, 0x00, 0x40); // analog TX
		trxSelectVoiceChannel(AT1846_VOICE_CHANNEL_MIC);// For 1750 tone burst
	}
	else
	{
		set_clear_I2C_reg_2byte_with_mask(0x30, 0xFF, 0x1F, 0x00, 0xC0); // digital TX
	}

    GPIO_PinWrite(GPIO_RF_ant_switch, Pin_RF_ant_switch, ANTENNA_SWITCH_TX);

	// TX PA on
	if (trxCheckFrequencyIsUHF(currentTxFrequency))
	{
		GPIO_PinWrite(GPIO_VHF_TX_amp_power, Pin_VHF_TX_amp_power, 0);// I can't see why this would be needed. Its probably just for safety.
		GPIO_PinWrite(GPIO_UHF_TX_amp_power, Pin_UHF_TX_amp_power, 1);
	}
	else
	{
		GPIO_PinWrite(GPIO_UHF_TX_amp_power, Pin_UHF_TX_amp_power, 0);// I can't see why this would be needed. Its probably just for safety.
		GPIO_PinWrite(GPIO_VHF_TX_amp_power, Pin_VHF_TX_amp_power, 1);
	}
    DAC_SetBufferValue(DAC0, 0U, txPower);	// PA drive to appropriate level
}

void trxSetPowerFromLevel(int powerLevel)
{
	uint16_t powerVal=1400;// Default to around 1W
	int stepPerWatt = (trxPowerSettings.highPower - trxPowerSettings.lowPower)/4;

	switch(powerLevel)
	{
		case 0:// 250mW
			powerVal = trxPowerSettings.lowPower - 500;
			break;
		case 1:// 500mW
			powerVal = trxPowerSettings.lowPower - 300;
			break;
		case 2:// 750mW
			powerVal = trxPowerSettings.lowPower - 150;
			break;
		case 3:// 1W
		case 4:// 2W
		case 5:// 3W
		case 6:// 4W
		case 7:// 5W
			powerVal = ((powerLevel - 3) * stepPerWatt) + trxPowerSettings.lowPower;
			break;
		case 8:// 5W+
			powerVal=4095;
			break;
	}

	if (powerVal>4095)
	{
		powerVal=4095;
	}
	txPower = powerVal;
}

uint16_t trxGetPower()
{
	return txPower;
}

void trxCalcBandAndFrequencyOffset(int *band_offset, int *freq_offset)
{
// NOTE. For crossband duplex DMR, the calibration potentially needs to be changed every time the Tx/Rx is switched over on each 30ms cycle
// But at the moment this is an unnecessary complication and I'll just use the Rx frequency to get the calibration offsets

	if (trxCheckFrequencyIsUHF(currentRxFrequency))
	{
		*band_offset=0x00000000;
		if (currentRxFrequency<4100000)
		{
			*freq_offset=0x00000000;
		}
		else if (currentRxFrequency<4200000)
		{
			*freq_offset=0x00000001;
		}
		else if (currentRxFrequency<4300000)
		{
			*freq_offset=0x00000002;
		}
		else if (currentRxFrequency<4400000)
		{
			*freq_offset=0x00000003;
		}
		else if (currentRxFrequency<4500000)
		{
			*freq_offset=0x00000004;
		}
		else if (currentRxFrequency<4600000)
		{
			*freq_offset=0x00000005;
		}
		else if (currentRxFrequency<4700000)
		{
			*freq_offset=0x00000006;
		}
		else
		{
			*freq_offset=0x00000007;
		}
	}
	else
	{
		*band_offset=0x00000070;
		if (currentRxFrequency<1380000)
		{
			*freq_offset=0x00000000;
		}
		else if (currentRxFrequency<1425000)
		{
			*freq_offset=0x00000001;
		}
		else if (currentRxFrequency<1475000)
		{
			*freq_offset=0x00000002;
		}
		else if (currentRxFrequency<1525000)
		{
			*freq_offset=0x00000003;
		}
		else if (currentRxFrequency<1575000)
		{
			*freq_offset=0x00000004;
		}
		else if (currentRxFrequency<1625000)
		{
			*freq_offset=0x00000005;
		}
		else if (currentRxFrequency<1685000)
		{
			*freq_offset=0x00000006;
		}
		else
		{
			*freq_offset=0x00000007;
		}
	}
}

void trxUpdateC6000Calibration()
{
	int band_offset=0x00000000;
	int freq_offset=0x00000000;

	if (nonVolatileSettings.useCalibration==false)
	{
		return;
	}

	trxCalcBandAndFrequencyOffset(&band_offset, &freq_offset);

	write_SPI_page_reg_byte_SPI0(0x04, 0x00, 0x3F); // Reset HR-C6000 state

	uint8_t val_shift;
	read_val_DACDATA_shift(band_offset,&val_shift);
	write_SPI_page_reg_byte_SPI0(0x04, 0x37, val_shift); // DACDATA shift (LIN_VOL)

	uint8_t val_0x04;
	read_val_Q_MOD2_offset(band_offset,&val_0x04);
	write_SPI_page_reg_byte_SPI0(0x04, 0x04, val_0x04); // MOD2 offset

	uint8_t val_0x46;
	read_val_phase_reduce(band_offset+freq_offset,&val_0x46);
	write_SPI_page_reg_byte_SPI0(0x04, 0x46, val_0x46); // phase reduce

	uint8_t val_0x47;
	uint8_t val_0x48;
	read_val_twopoint_mod(band_offset,&val_0x47, &val_0x48);
	write_SPI_page_reg_byte_SPI0(0x04, 0x48, val_0x48); // bit 0 to 1 = upper 2 bits of 10-bit twopoint mod
	write_SPI_page_reg_byte_SPI0(0x04, 0x47, val_0x47); // bit 0 to 7 = lower 8 bits of 10-bit twopoint mod
}

void I2C_AT1846_set_register_with_mask(uint8_t reg, uint16_t mask, uint16_t value, uint8_t shift)
{
	set_clear_I2C_reg_2byte_with_mask(reg, (mask & 0xff00) >> 8, (mask & 0x00ff) >> 0, ((value << shift) & 0xff00) >> 8, ((value << shift) & 0x00ff) >> 0);
}

void trxUpdateAT1846SCalibration()
{
	int band_offset=0x00000000;
	int freq_offset=0x00000000;

	if (nonVolatileSettings.useCalibration==false)
	{
		return;
	}

	trxCalcBandAndFrequencyOffset(&band_offset, &freq_offset);

	uint8_t val_pga_gain;
	uint8_t voice_gain_tx;
	uint8_t gain_tx;
	uint8_t padrv_ibit;

	uint16_t xmitter_dev;

	uint8_t dac_vgain_analog;
	uint8_t volume_analog;

	uint16_t noise1_th;
	uint16_t noise2_th;
	uint16_t rssi3_th;

	uint16_t squelch_th;

	read_val_pga_gain(band_offset, &val_pga_gain);
	read_val_voice_gain_tx(band_offset, &voice_gain_tx);
	read_val_gain_tx(band_offset, &gain_tx);
	read_val_padrv_ibit(band_offset, &padrv_ibit);

	if (currentBandWidthIs25kHz)
	{
		// 25 kHz settings
		read_val_xmitter_dev_wideband(band_offset, &xmitter_dev);
	}
	else
	{
		// 12.5 kHz settings
		read_val_xmitter_dev_narrowband(band_offset, &xmitter_dev);
	}

	if (currentMode == RADIO_MODE_ANALOG)
	{
		read_val_dac_vgain_analog(band_offset, &dac_vgain_analog);
		read_val_volume_analog(band_offset, &volume_analog);
	}
	else
	{
		dac_vgain_analog = 0x0C;
		volume_analog = 0x0C;
	}

	if (currentBandWidthIs25kHz)
	{
		// 25 kHz settings
		read_val_noise1_th_wideband(band_offset, &noise1_th);
		read_val_noise2_th_wideband(band_offset, &noise2_th);
		read_val_rssi3_th_wideband(band_offset, &rssi3_th);

		read_val_squelch_th(band_offset+freq_offset, 0, &squelch_th);
	}
	else
	{
		// 12.5 kHz settings
		read_val_noise1_th_narrowband(band_offset, &noise1_th);
		read_val_noise2_th_narrowband(band_offset, &noise2_th);
		read_val_rssi3_th_narrowband(band_offset, &rssi3_th);

		read_val_squelch_th(band_offset+freq_offset, 3, &squelch_th);
	}

	I2C_AT1846_set_register_with_mask(0x0A, 0xF83F, val_pga_gain, 6);
	I2C_AT1846_set_register_with_mask(0x41, 0xFF80, voice_gain_tx, 0);
	I2C_AT1846_set_register_with_mask(0x44, 0xF0FF, gain_tx, 8);

	I2C_AT1846_set_register_with_mask(0x59, 0x003f, xmitter_dev, 6);
	I2C_AT1846_set_register_with_mask(0x44, 0xFF0F, dac_vgain_analog, 4);
	I2C_AT1846_set_register_with_mask(0x44, 0xFFF0, volume_analog, 0);

	I2C_AT1846_set_register_with_mask(0x48, 0x0000, noise1_th, 0);
	I2C_AT1846_set_register_with_mask(0x60, 0x0000, noise2_th, 0);
	I2C_AT1846_set_register_with_mask(0x3f, 0x0000, rssi3_th, 0);

	I2C_AT1846_set_register_with_mask(0x0A, 0x87FF, padrv_ibit, 11);

	I2C_AT1846_set_register_with_mask(0x49, 0x0000, squelch_th, 0);
}

void trxSetDMRColourCode(int colourCode)
{
	write_SPI_page_reg_byte_SPI0(0x04, 0x1F, (colourCode << 4)); // DMR Colour code in upper 4 bits.
	currentCC = colourCode;
}

int trxGetDMRColourCode()
{
	return currentCC;
}

int trxGetDMRTimeSlot()
{
	return trxCurrentDMRTimeSlot;
	//return ((currentChannelData->flag2 & 0x40)!=0);
}

void trxSetDMRTimeSlot(int timeslot)
{
	trxCurrentDMRTimeSlot = timeslot;
}

void trxUpdateTsForCurrentChannelWithSpecifiedContact(struct_codeplugContact_t *contactData)
{
	bool hasManualTsOverride = false;

	// nonVolatileSettings.tsManualOverride stores separate TS overrides for VFO and Channel mode
	// Lower nibble is the Channel screen override and upper nibble if the VFO
	if (nonVolatileSettings.initialMenuNumber == MENU_CHANNEL_MODE)
	{
		if ((nonVolatileSettings.tsManualOverride & 0x0F) != 0)
		{
			hasManualTsOverride = true;
		}
	}
	else
	{
		if ((nonVolatileSettings.tsManualOverride & 0xF0) != 0)
		{
			hasManualTsOverride = true;
		}
	}

	if (!hasManualTsOverride)
	{
		if ((contactData->reserve1 & 0x01) == 0x00)
		{
			if ( (contactData->reserve1 & 0x02) !=0 )
			{
				trxCurrentDMRTimeSlot = 1;
			}
			else
			{
				trxCurrentDMRTimeSlot = 0;
			}
		}
		else
		{
			trxCurrentDMRTimeSlot = ((currentChannelData->flag2 & 0x40)!=0);
		}
	}
}

void trxSetTxCTCSS(int toneFreqX10)
{
	if (toneFreqX10 == 0xFFFF)
	{
		// tone value of 0xffff in the codeplug seem to be a flag that no tone has been selected
        write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x4a, 0x00,0x00); //Zero the CTCSS1 Register
		set_clear_I2C_reg_2byte_with_mask(0x4e,0xF9,0xFF,0x00,0x00);    //disable the transmit CTCSS
	}
	else
	{
		toneFreqX10 = toneFreqX10*10;// value that is stored is 100 time the tone freq but its stored in the codeplug as freq times 10
		write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT,	0x4a, (toneFreqX10 >> 8) & 0xff,	(toneFreqX10 & 0xff));
		write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT,	0x4b, 0x00, 0x00); // init cdcss_code
		write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT,	0x4c, 0x0A, 0xE3); // init cdcss_code
		set_clear_I2C_reg_2byte_with_mask(0x4e,0xF9,0xFF,0x06,0x00);    //enable the transmit CTCSS
	}
}

void trxSetRxCTCSS(int toneFreqX10)
{
	if (toneFreqX10 == 0xFFFF)
	{
		// tone value of 0xffff in the codeplug seem to be a flag that no tone has been selected
        write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x4d, 0x00,0x00); //Zero the CTCSS2 Register
        rxCTCSSactive=false;
	}
	else
	{
		int threshold=(2500-toneFreqX10)/100;   //adjust threshold value to match tone frequency.
		toneFreqX10 = toneFreqX10*10;// value that is stored is 100 time the tone freq but its stored in the codeplug as freq times 10
		write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT,	0x4d, (toneFreqX10 >> 8) & 0xff,	(toneFreqX10 & 0xff));
		write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x5b,(threshold & 0xFF),(threshold & 0xFF)); //set the detection thresholds
		set_clear_I2C_reg_2byte_with_mask(0x3a,0xFF,0xE0,0x00,0x08);    //set detection to CTCSS2
		rxCTCSSactive=true;
	}
}

bool trxCheckCTCSSFlag()
{
	uint8_t FlagsH;
	uint8_t FlagsL;

	read_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x1c,&FlagsH,&FlagsL);

	return (FlagsH & 0x01);
}

void trxUpdateDeviation(int channel)
{
	uint8_t deviation;

	if (nonVolatileSettings.useCalibration == false)
	{
		return;
	}

	switch (channel)
	{
	case AT1846_VOICE_CHANNEL_TONE1:
	case AT1846_VOICE_CHANNEL_TONE2:
		read_val_dev_tone(currentTxFrequency, CAL_DEV_TONE, &deviation);
		deviation &= 0x7f;
		I2C_AT1846_set_register_with_mask(0x59, 0x003f, deviation, 6); // Tone deviation value
		break;
	case AT1846_VOICE_CHANNEL_DTMF:
		read_val_dev_tone(currentTxFrequency, CAL_DEV_DTMF, &deviation);
		deviation &= 0x7f;
		I2C_AT1846_set_register_with_mask(0x59, 0x003f, deviation, 6); // Tone deviation value
		break;
	}
}

void trxSelectVoiceChannel(uint8_t channel) {
	uint8_t valh;
	uint8_t vall;

	switch (channel)
	{
	case AT1846_VOICE_CHANNEL_TONE1:
	case AT1846_VOICE_CHANNEL_TONE2:
	case AT1846_VOICE_CHANNEL_DTMF:
		set_clear_I2C_reg_2byte_with_mask(0x79, 0xff, 0xff, 0xc0, 0x00); // Select single tone
		set_clear_I2C_reg_2byte_with_mask(0x57, 0xff, 0xfe, 0x00, 0x01); // Audio feedback on

		read_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x41, &valh, &trxSaveVoiceGainTx);
		trxSaveVoiceGainTx &= 0x7f;

		read_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x59, &valh, &vall);
		trxSaveDeviation = (vall + (valh<<8)) >> 6;

		trxUpdateDeviation(channel);

		set_clear_I2C_reg_2byte_with_mask(0x41, 0xff, 0x80, 0x00, 0x05);
		break;
	default:
		set_clear_I2C_reg_2byte_with_mask(0x57, 0xff, 0xfe, 0x00, 0x00); // Audio feedback off
		if (trxSaveVoiceGainTx != 0xff)
		{
			I2C_AT1846_set_register_with_mask(0x41, 0xFF80, trxSaveVoiceGainTx, 0);
		}
		if (trxSaveDeviation != 0xFF)
		{
			I2C_AT1846_set_register_with_mask(0x59, 0x003f, trxSaveDeviation, 6);
		}
		break;
	}
	set_clear_I2C_reg_2byte_with_mask(0x3a, 0x8f, 0xff, channel, 0x00);
}

void trxSetTone1(int toneFreq)
{
	toneFreq = toneFreq * 10;
	write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x35, (toneFreq >> 8) & 0xff,	(toneFreq & 0xff));   // tone1_freq
}

void trxSetTone2(int toneFreq)
{
	toneFreq = toneFreq * 10;
	write_I2C_reg_2byte(I2C_MASTER_SLAVE_ADDR_7BIT, 0x36, (toneFreq >> 8) & 0xff,	(toneFreq & 0xff));   // tone2_freq
}

void trxSetDTMF(int code)
{
	if (code < 16)
	{
		trxSetTone1(trxDTMFfreq1[code]);
		trxSetTone2(trxDTMFfreq2[code]);
	}
}
