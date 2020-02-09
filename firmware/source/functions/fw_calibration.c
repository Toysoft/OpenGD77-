/*
 * Copyright (C)2019 	Roger Clark, VK3KYY / G4KYF
 * 				and 	Kai Ludwig, DG4KLU
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

#include "fw_calibration.h"
#include "fw_trx.h"

// This lookup gives an approximate proportion of power needed for power settings from 0 to 5w in 0.25 W steps, where 32768 is 100% of whatever PA drive is required for 5W
const int PA_DRIVE_LOOKUP[] = {0,3315,5482,7395,8925,10455,11730,13005,14152,15300,16702,17977,19380,20655,22440,23715,25117,27030,28624,30855,32768};
const int NUM_PA_DRIVE_INDEXES = 20;// not including zero

/*
calibrationStruct_t calibrationVHF;
calibrationStruct_t calibrationUHF;
*/
void read_val_DACDATA_shift(int offset, uint8_t* val_shift)						//Sent to HRC6000 register 0x37. This sets the DMR received Audio Gain. (And Beep Volume)
{																				//Effective for DMR Receive Only.
	uint8_t buffer[1];															//only the low 4 bits are used. 1.5dB change per increment 0-31
	SPI_Flash_read(EXT_DACDATA_shift+offset,buffer,1);

	*val_shift=buffer[0]+1;
	if (*val_shift>31)
	{
		*val_shift=31;
	}
	*val_shift|=0x80;
}

void read_val_twopoint_mod(int offset, uint8_t* val_0x47, uint8_t* val_0x48)      // Sent to HRC6000 register 0x47 and 0x48 to set the DC level of the Mod1 output. This sets the frequency of the AT1846 Reference Oscillator.
{																				  // Effective for DMR and FM Receive and Transmit. Calibrates the receive and transmit frequencies.
	uint8_t buffer[2];															  // Only one setting should be required for all frequencies but the Cal table has separate entries for VHF and UHF.
	SPI_Flash_read(EXT_twopoint_mod+offset,buffer,2);
	*val_0x47=buffer[0];
	*val_0x48=buffer[1];
}

void read_val_Q_MOD2_offset(int offset, uint8_t* val_0x04)						  //Sent to the HRC6000 register 0x04 which is the offset value for the Mod 2 output.
{																				  //However according to the schematics the Mod 2 output is not connected.
	uint8_t buffer[1];															  //Therefore the function of this setting is unclear. (possible datasheet error?)
	SPI_Flash_read(EXT_Q_MOD2_offset+offset,buffer,1);							  //The Cal table has one entry for each band and this is almost identical to 0x47 above.
	*val_0x04=buffer[0];
}

void read_val_phase_reduce(int offset, uint8_t* val_0x46)						  //Sent to the HRC6000 register 0x46. This adjusts the level of the DMR Modulation on the MOD1 output.
{																				  //Mod 1 controls the AT1846 Reference oscillator so this is effectively the deviation setting for DMR.
	uint8_t buffer[1];															  //Because it modulates the reference oscillator the deviation will change depending on the frequency being used.
	SPI_Flash_read(EXT_phase_reduce+offset,buffer,1);							  //Only affects DMR Transmit. The Cal table has 8 calibration values for each band.
	*val_0x46=buffer[0];
}

void read_val_pga_gain(int offset, uint8_t* value)								  //Sent to AT1846S register 0x0a bits 6-10. Sets Voice Analogue Gain for FM Transmit.
{
	uint8_t buffer[1];
	SPI_Flash_read(EXT_pga_gain+offset,buffer,1);
	*value=buffer[0] & 0x1f;
}

void read_val_voice_gain_tx(int offset, uint8_t* value)							  //Sent to AT1846S register 0x41 bits 0-6. Sets Voice Digital Gain for FM Transmit.
{
	uint8_t buffer[1];
	SPI_Flash_read(EXT_voice_gain_tx+offset,buffer,1);
	*value=buffer[0] & 0x7f;
}

void read_val_gain_tx(int offset, uint8_t* value)   							//Sent to AT1846S register 0x44 bits 8-11. Sets Voice Digital Gain after ADC for FM Transmit.
{
	uint8_t buffer[1];
	SPI_Flash_read(EXT_gain_tx+offset,buffer,1);
	*value=buffer[0] & 0x0f;
}

void read_val_padrv_ibit(int offset, uint8_t* value)							//Sent to AT1846S register 0x0a bits 11-14. Sets PA Power Control for DMR and FM Transmit.
{
	uint8_t buffer[1];
	SPI_Flash_read(EXT_padrv_ibit+offset,buffer,1);
	*value=buffer[0] & 0x0f;
}

void read_val_xmitter_dev_wideband(int offset, uint16_t* value)					//Sent to AT1846S register 0x59 bits 6-15. Sets Deviation for Wideband FM Transmit
{
	uint8_t buffer[2];
	SPI_Flash_read(EXT_xmitter_dev_wideband+offset,buffer,2);
	*value=buffer[0] + ((buffer[1] & 0x03) << 8);
}

void read_val_xmitter_dev_narrowband(int offset, uint16_t* value)				//Sent to AT1846S register 0x59 bits 6-15. Sets Deviation for NarrowBand FM Transmit
{
	uint8_t buffer[2];
	SPI_Flash_read(EXT_xmitter_dev_narrowband+offset,buffer,2);
	*value=buffer[0] + ((buffer[1] & 0x03) << 8);
}

void read_val_dev_tone(int index, uint8_t *value)
{
	int address;

	if (trxCurrentBand[TRX_TX_FREQ_BAND] == RADIO_BAND_UHF)                    //Sent to AT1846S register 0x59 bits 0-5. Sets Tones Deviation for FM Transmit
	{
		address = EXT_uhf_dev_tone+index;
	}
	else
	{
		address = EXT_vhf_dev_tone+index;
	}
	SPI_Flash_read(address, value, 1);
}

void read_val_dac_vgain_analog(int offset, uint8_t* value)					//Sent to AT1846S register 0x44 bits 0-3. Sets Digital Audio Gain for FM and DMR Receive.
{
	uint8_t buffer[1];
	SPI_Flash_read(EXT_dac_vgain_analog+offset,buffer,1);
	*value=buffer[0] & 0x0f;
}

void read_val_volume_analog(int offset, uint8_t* value)						//Sent to AT1846S register 0x44 bits 4-7. Sets Analogue Audio Gain for FM and DMR Receive.
{
	uint8_t buffer[1];
	SPI_Flash_read(EXT_volume_analog+offset,buffer,1);
	*value=buffer[0] & 0x0f;
}

void read_val_noise1_th_wideband(int offset, uint16_t* value)				//Sent to AT1846S register 0x48. Sets Noise 1 Threshold  for FM Wideband Receive.
{
	uint8_t buffer[2];
	SPI_Flash_read(EXT_noise1_th_wideband+offset,buffer,2);
	*value=((buffer[0] & 0x7f) << 7) + ((buffer[1] & 0x7f) << 0);
}

void read_val_noise2_th_wideband(int offset, uint16_t* value)				//Sent to AT1846S register 0x60. Sets Noise 2 Threshold  for FM  Wideband Receive.
{
	uint8_t buffer[2];
	SPI_Flash_read(EXT_noise2_th_wideband+offset,buffer,2);
	*value=((buffer[0] & 0x7f) << 7) + ((buffer[1] & 0x7f) << 0);
}

void read_val_rssi3_th_wideband(int offset, uint16_t* value)				//Sent to AT1846S register 0x3F. Sets RSSI3 Threshold  for  Wideband Receive.
{
	uint8_t buffer[2];
	SPI_Flash_read(EXT_rssi3_th_wideband+offset,buffer,2);
	*value=((buffer[0] & 0x7f) << 7) + ((buffer[1] & 0x7f) << 0);
}

void read_val_noise1_th_narrowband(int offset, uint16_t* value)				//Sent to AT1846S register 0x48. Sets Noise 1 Threshold  for FM Narrowband Receive.
{
	uint8_t buffer[2];
	SPI_Flash_read(EXT_noise1_th_narrowband+offset,buffer,2);
	*value=((buffer[0] & 0x7f) << 7) + ((buffer[1] & 0x7f) << 0);
}

void read_val_noise2_th_narrowband(int offset, uint16_t* value)				//Sent to AT1846S register 0x60. Sets Noise 2 Threshold  for FM  Narrowband  Receive.
{
	uint8_t buffer[2];
	SPI_Flash_read(EXT_noise2_th_narrowband+offset,buffer,2);
	*value=((buffer[0] & 0x7f) << 7) + ((buffer[1] & 0x7f) << 0);
}

void read_val_rssi3_th_narrowband(int offset, uint16_t* value)				//Sent to AT1846S register 0x3F. Sets RSSI3 Threshold  for  Narrowband Receive.
{
	uint8_t buffer[2];
	SPI_Flash_read(EXT_rssi3_th_narrowband+offset,buffer,2);
	*value=((buffer[0] & 0x7f) << 7) + ((buffer[1] & 0x7f) << 0);
}

void read_val_squelch_th(int offset, int mod, uint16_t* value)				//Sent to At1846S register 0x49. sets squelch open threshold for FM Receive.
{																			//8 Cal Table Entries for each band. Frequency Dependent.
	uint8_t buffer[1];
	SPI_Flash_read(EXT_squelch_th+offset,buffer,1);
	uint8_t v1 = buffer[0]-mod;
	uint8_t v2 = buffer[0]-mod-3;
	if ( v1 >= 127 || v2 >= 127 || v1 < v2 )
	{
	  v1 = 24;
	  v2 = 21;
	}
	*value=(v1 << 7) + (v2 << 0);
}

bool calibrationGetPowerForFrequency(int freq, calibrationPowerValues_t *powerSettings)				//Power Settings for Transmit
{																							        //16 Entries of two bytes each.
	const uint32_t POWER_CALIBRATION_ADDRESS_UHF_400MHZ = 0x8F00B;									//UHF is in 5Mhz bands starting at 400Mhz.
	const uint32_t POWER_CALIBRATION_ADDRESS_VHF_135MHZ = 0x8F07B;									//VHF is in 5Mhz bands from 135Mhz (Only first 8 entries are used)
	int address;																					//first byte of each entry  is the low power and the second byte is the high power value
	uint8_t buffer[2];


	if (trxCurrentBand[TRX_TX_FREQ_BAND] == RADIO_BAND_UHF)
	{
		address =  (freq - RADIO_FREQUENCY_BANDS[RADIO_BAND_UHF].minFreq) / 500000;

		if (address < 0)
		{
			address=0;
		}
		else
		{
			if (address > 15)
			{
				address = 15;
			}
		}
		address = POWER_CALIBRATION_ADDRESS_UHF_400MHZ + ( address * 2);

	}
	else
	{
		address = (freq - RADIO_FREQUENCY_BANDS[RADIO_BAND_VHF].minFreq) / 500000;

		if (address < 0)
		{
			address=0;
		}
		else
		{
			if (address > 7)
			{
				address = 7;
			}
		}
		address = POWER_CALIBRATION_ADDRESS_VHF_135MHZ +   (address * 2);
	}

	SPI_Flash_read(address,buffer,2);
	powerSettings->lowPower 	= buffer[0] * 16;
	powerSettings->highPower 	= buffer[1] * 16;

	return true;
}

bool calibrationGetRSSIMeterParams(calibrationRSSIMeter_t *rssiMeterValues)							//Signal Level Meter setting
{																									//two bytes per band
	int address;																					//Low Byte is low end of meter range
																									//High Byte is High End of Meter Range
	if (trxCurrentBand[TRX_RX_FREQ_BAND] == RADIO_BAND_UHF)
	{
		address = 0x8F053;
	}
	else
	{
		address = 0x8F0C3;
	}

	if (SPI_Flash_read(address,(uint8_t *)rssiMeterValues,2))
	{
		return true;
	}
	else
	{
		return false;
	}
}
