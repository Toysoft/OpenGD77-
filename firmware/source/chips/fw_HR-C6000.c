/*
 * Copyright (C)2019 Kai Ludwig, DG4KLU
 *
 * Additional code by Roger Clark VK3KYY / G4KYF
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

#include "fw_HR-C6000.h"
#include "menu/menuUtilityQSOData.h"
#include "fw_settings.h"
#include "menu/menuHotspot.h"

#if defined(USE_SEGGER_RTT) && defined(DEBUG_DMR_DATA)
#include <SeggerRTT/RTT/SEGGER_RTT.h>
#endif

TaskHandle_t fwhrc6000TaskHandle;

const uint8_t TG_CALL_FLAG = 0x00;
const uint8_t PC_CALL_FLAG = 0x03;
const uint8_t SILENCE_AUDIO[27] = {	0xB9U, 0xE8U, 0x81U, 0x52U, 0x61U, 0x73U, 0x00U, 0x2AU, 0x6BU, 0xB9U, 0xE8U, 0x81U, 0x52U,
									0x61U, 0x73U, 0x00U, 0x2AU, 0x6BU, 0xB9U, 0xE8U, 0x81U, 0x52U, 0x61U, 0x73U, 0x00U, 0x2AU, 0x6BU };

// GD-77 FW V3.1.1 data from 0x76010 / length 0x06
static const uint8_t spi_init_values_1[] = { 0xd5, 0xd7, 0xf7, 0x7f, 0xd7, 0x57 };
// GD-77 FW V3.1.1 data from 0x75F70 / length 0x20
static const uint8_t spi_init_values_2[] = { 0x69, 0x69, 0x96, 0x96, 0x96, 0x99, 0x99, 0x99, 0xa5, 0xa5, 0xaa, 0xaa, 0xcc, 0xcc, 0x00, 0xf0, 0x01, 0xff, 0x01, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x10, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// GD-77 FW V3.1.1 data from 0x75F90 / length 0x10
static const uint8_t spi_init_values_3[] = { 0x00, 0x00, 0x14, 0x1e, 0x1a, 0xff, 0x3d, 0x50, 0x07, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// GD-77 FW V3.1.1 data from 0x75FA0 / length 0x07
static const uint8_t spi_init_values_4[] = { 0x00, 0x03, 0x01, 0x02, 0x05, 0x1e, 0xf0 };
// GD-77 FW V3.1.1 data from 0x75FA8 / length 0x05
static const uint8_t spi_init_values_5[] = { 0x00, 0x00, 0xeb, 0x78, 0x67 };
// GD-77 FW V3.1.1 data from 0x75FB0 / length 0x60
static const uint8_t spi_init_values_6[] = { 0x32, 0xef, 0x00, 0x31, 0xef, 0x00, 0x12, 0xef, 0x00, 0x13, 0xef, 0x00, 0x14, 0xef, 0x00, 0x15, 0xef, 0x00, 0x16, 0xef, 0x00, 0x17, 0xef, 0x00, 0x18, 0xef, 0x00, 0x19, 0xef, 0x00, 0x1a, 0xef, 0x00, 0x1b, 0xef, 0x00, 0x1c, 0xef, 0x00, 0x1d, 0xef, 0x00, 0x1e, 0xef, 0x00, 0x1f, 0xef, 0x00, 0x20, 0xef, 0x00, 0x21, 0xef, 0x00, 0x22, 0xef, 0x00, 0x23, 0xef, 0x00, 0x24, 0xef, 0x00, 0x25, 0xef, 0x00, 0x26, 0xef, 0x00, 0x27, 0xef, 0x00, 0x28, 0xef, 0x00, 0x29, 0xef, 0x00, 0x2a, 0xef, 0x00, 0x2b, 0xef, 0x00, 0x2c, 0xef, 0x00, 0x2d, 0xef, 0x00, 0x2e, 0xef, 0x00, 0x2f, 0xef, 0x00 };


volatile uint8_t tmp_val_0x82;
volatile uint8_t tmp_val_0x84;
volatile uint8_t tmp_val_0x86;
volatile uint8_t tmp_val_0x51;
volatile uint8_t tmp_val_0x52;
volatile uint8_t tmp_val_0x57;
volatile uint8_t tmp_val_0x5f;
volatile uint8_t tmp_val_0x90;
volatile uint8_t tmp_val_0x98;
volatile uint8_t DMR_frame_buffer[DRM_FRAME_BUFFER_SIZE];

volatile bool int_sys;
volatile bool int_ts;
volatile bool tx_required;
volatile bool int_rxtx;
volatile int int_timeout;

static uint32_t receivedTgOrPcId;
static uint32_t receivedSrcId;
volatile int slot_state;
int tick_cnt;
int skip_count;
volatile int tx_sequence;
uint8_t spi_tx[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


void SPI_HR_C6000_init()
{

    // C6000 interrupts
    PORT_SetPinMux(Port_INT_C6000_RF_RX, Pin_INT_C6000_RF_RX, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_INT_C6000_RF_TX, Pin_INT_C6000_RF_TX, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_INT_C6000_SYS, Pin_INT_C6000_SYS, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_INT_C6000_TS, Pin_INT_C6000_TS, kPORT_MuxAsGpio);
    GPIO_PinInit(GPIO_INT_C6000_RF_RX, Pin_INT_C6000_RF_RX, &pin_config_input);
    GPIO_PinInit(GPIO_INT_C6000_RF_TX, Pin_INT_C6000_RF_TX, &pin_config_input);
    GPIO_PinInit(GPIO_INT_C6000_SYS, Pin_INT_C6000_SYS, &pin_config_input);
    GPIO_PinInit(GPIO_INT_C6000_TS, Pin_INT_C6000_TS, &pin_config_input);

    // Connections with C6000
    PORT_SetPinMux(Port_INT_C6000_RESET, Pin_INT_C6000_RESET, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_INT_C6000_PWD, Pin_INT_C6000_PWD, kPORT_MuxAsGpio);
    GPIO_PinInit(GPIO_INT_C6000_RESET, Pin_INT_C6000_RESET, &pin_config_output);
    GPIO_PinInit(GPIO_INT_C6000_PWD, Pin_INT_C6000_PWD, &pin_config_output);
    GPIO_PinWrite(GPIO_INT_C6000_RESET, Pin_INT_C6000_RESET, 1);
    GPIO_PinWrite(GPIO_INT_C6000_PWD, Pin_INT_C6000_PWD, 1);

    // Wake up C6000
	vTaskDelay(portTICK_PERIOD_MS * 10);
    GPIO_PinWrite(GPIO_INT_C6000_PWD, Pin_INT_C6000_PWD, 0);
	vTaskDelay(portTICK_PERIOD_MS * 10);


	// --- start spi_init_daten_senden()
	write_SPI_page_reg_byte_SPI0(0x04, 0x0b, 0x40);    //Set PLL M Register
	write_SPI_page_reg_byte_SPI0(0x04, 0x0c, 0x32);    //Set PLL Dividers
	write_SPI_page_reg_byte_SPI0(0x04, 0xb9, 0x05);
	write_SPI_page_reg_byte_SPI0(0x04, 0x0a, 0x01);    //Set Clock Source Enable CLKOUT Pin

	write_SPI_page_reg_bytearray_SPI0(0x01, 0x04, spi_init_values_1, 0x06);
	write_SPI_page_reg_bytearray_SPI0(0x01, 0x10, spi_init_values_2, 0x20);
	write_SPI_page_reg_bytearray_SPI0(0x01, 0x30, spi_init_values_3, 0x10);
	write_SPI_page_reg_bytearray_SPI0(0x01, 0x40, spi_init_values_4, 0x07);
	write_SPI_page_reg_bytearray_SPI0(0x01, 0x51, spi_init_values_5, 0x05);
	write_SPI_page_reg_bytearray_SPI0(0x01, 0x60, spi_init_values_6, 0x60);

	write_SPI_page_reg_byte_SPI0(0x04, 0x00, 0x00);   //Clear all Reset Bits which forces a reset of all internal systems
	write_SPI_page_reg_byte_SPI0(0x04, 0x10, 0x6E);   //Set DMR,Tier2,Timeslot Mode, Layer 2, Repeater, Aligned, Slot1
	write_SPI_page_reg_byte_SPI0(0x04, 0x11, 0x80);   //Set LocalChanMode to Default Value 
	write_SPI_page_reg_byte_SPI0(0x04, 0x13, 0x00);   //Zero Cend_Band Timing advance
	write_SPI_page_reg_byte_SPI0(0x04, 0x1F, 0x10);   //Set LocalEMB  DMR Colour code in upper 4 bits - defaulted to 1, and is updated elsewhere in the code
	write_SPI_page_reg_byte_SPI0(0x04, 0x20, 0x00);   //Set LocalAccessPolicy to Impolite
	write_SPI_page_reg_byte_SPI0(0x04, 0x21, 0xA0);   //Set LocalAccessPolicy1 to Polite to Color Code  (unsure why there are two registers for this)   
	write_SPI_page_reg_byte_SPI0(0x04, 0x22, 0x26);   //Start Vocoder Decode, I2S mode
	write_SPI_page_reg_byte_SPI0(0x04, 0x22, 0x86);   //Start Vocoder Encode, I2S mode
	write_SPI_page_reg_byte_SPI0(0x04, 0x25, 0x0E);   //Undocumented Register 
	write_SPI_page_reg_byte_SPI0(0x04, 0x26, 0x7D);   //Undocumented Register 
	write_SPI_page_reg_byte_SPI0(0x04, 0x27, 0x40);   //Undocumented Register 
	write_SPI_page_reg_byte_SPI0(0x04, 0x28, 0x7D);   //Undocumented Register
	write_SPI_page_reg_byte_SPI0(0x04, 0x29, 0x40);   //Undocumented Register
	write_SPI_page_reg_byte_SPI0(0x04, 0x2A, 0x0B);   //Set spi_clk_cnt to default value
	write_SPI_page_reg_byte_SPI0(0x04, 0x2B, 0x0B);   //According to Datashhet this is a Read only register For FM Squelch
	write_SPI_page_reg_byte_SPI0(0x04, 0x2C, 0x17);   //According to Datashhet this is a Read only register For FM Squelch
	write_SPI_page_reg_byte_SPI0(0x04, 0x2D, 0x05);   //Set FM Compression and Decompression points (?)
	write_SPI_page_reg_byte_SPI0(0x04, 0x2E, 0x04);   //Set tx_pre_on (DMR Transmission advance) to 400us
	write_SPI_page_reg_byte_SPI0(0x04, 0x2F, 0x0B);   //Set I2S Clock Frequency
	write_SPI_page_reg_byte_SPI0(0x04, 0x32, 0x02);   //Set LRCK_CNT_H CODEC Operating Frequency to default value
	write_SPI_page_reg_byte_SPI0(0x04, 0x33, 0xFF);   //Set LRCK_CNT_L CODEC Operating Frequency to default value
	write_SPI_page_reg_byte_SPI0(0x04, 0x34, 0xF0);   //Set FM Filters on and bandwidth to 12.5Khz 
	write_SPI_page_reg_byte_SPI0(0x04, 0x35, 0x28);   //Set FM Modulation Coefficient
	write_SPI_page_reg_byte_SPI0(0x04, 0x3E, 0x28);   //Set FM Modulation Offset
	write_SPI_page_reg_byte_SPI0(0x04, 0x3F, 0x10);   //Set FM Modulation Limiter
	write_SPI_page_reg_byte_SPI0(0x04, 0x36, 0x00);   //Enable all clocks
	write_SPI_page_reg_byte_SPI0(0x04, 0x37, 0x00);   //Set mcu_control_shift to default. (codec under HRC-6000 control)
	write_SPI_page_reg_byte_SPI0(0x04, 0x4B, 0x1B);   //Set Data packet types to defaults
	write_SPI_page_reg_byte_SPI0(0x04, 0x4C, 0x00);   //Set Data packet types to defaults
	write_SPI_page_reg_byte_SPI0(0x04, 0x56, 0x00); 	//Undocumented Register
	write_SPI_page_reg_byte_SPI0(0x04, 0x5F, 0xC0); 	//Enable Sync detection for MS or BS orignated signals
	write_SPI_page_reg_byte_SPI0(0x04, 0x81, 0xFF); 	//Enable all Interrupts
	write_SPI_page_reg_byte_SPI0(0x04, 0xD1, 0xC4);   //According to Datasheet this register is for FM DTMF (?)

	// --- start subroutine spi_init_daten_senden_sub()
	write_SPI_page_reg_byte_SPI0(0x04, 0x01, 0x70); 	//set 2 point Mod, swap receive I and Q, receive mode IF (?)    (Presumably changed elsewhere)
	write_SPI_page_reg_byte_SPI0(0x04, 0x03, 0x00);   //zero Receive I Offset
	write_SPI_page_reg_byte_SPI0(0x04, 0x05, 0x00);   //Zero Receive Q Offset
	write_SPI_page_reg_byte_SPI0(0x04, 0x12, 0x15); 	//Set rf_pre_on Receive to transmit switching advance 
	write_SPI_page_reg_byte_SPI0(0x04, 0xA1, 0x80); 	//According to Datasheet this register is for FM Modulation Setting (?)
	write_SPI_page_reg_byte_SPI0(0x04, 0xC0, 0x0A);   //Set RF Signal Advance to 1ms (10x100us)
	write_SPI_page_reg_byte_SPI0(0x04, 0x06, 0x21);   //Use SPI vocoder under MCU control
	write_SPI_page_reg_byte_SPI0(0x04, 0x07, 0x0B);   //Set IF Frequency H to default 450KHz
	write_SPI_page_reg_byte_SPI0(0x04, 0x08, 0xB8);   //Set IF Frequency M to default 450KHz
	write_SPI_page_reg_byte_SPI0(0x04, 0x09, 0x00);   //Set IF Frequency L to default 450KHz
	write_SPI_page_reg_byte_SPI0(0x04, 0x0D, 0x10);   //Set Voice Superframe timeout value
	write_SPI_page_reg_byte_SPI0(0x04, 0x0E, 0x8E);   //Register Documented as Reserved 
	write_SPI_page_reg_byte_SPI0(0x04, 0x0F, 0xB8);   //FSK Error Count
	write_SPI_page_reg_byte_SPI0(0x04, 0xC2, 0x00);   //Disable Mic Gain AGC
	write_SPI_page_reg_byte_SPI0(0x04, 0xE0, 0x8B);   //CODEC under MCU Control, LineOut2 Enabled, Mic_p Enabled, I2S Slave Mode
	write_SPI_page_reg_byte_SPI0(0x04, 0xE1, 0x0F);   //Undocumented Register (Probably associated with CODEC)
	write_SPI_page_reg_byte_SPI0(0x04, 0xE2, 0x06);   //CODEC  Anti Pop Enabled, DAC Output Enabled
	write_SPI_page_reg_byte_SPI0(0x04, 0xE3, 0x52);   //CODEC Default Settings 
	write_SPI_page_reg_byte_SPI0(0x04, 0xE4, 0x4A);   //CODEC   LineOut Gain 2dB, Mic Stage 1 Gain 0dB, Mic Stage 2 Gain 30dB
	write_SPI_page_reg_byte_SPI0(0x04, 0xE5, 0x1A);   //CODEC Default Setting
	// --- end subroutine spi_init_daten_senden_sub()

	write_SPI_page_reg_byte_SPI0(0x04, 0x40, 0xC3);  	//Enable DMR Tx, DMR Rx, Passive Timing, Normal mode
	write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x40);   //Receive during next timeslot
	// --- end spi_init_daten_senden()

	// ------ start spi_more_init
	// --- start sub_1B5A4
	set_clear_SPI_page_reg_byte_with_mask_SPI0(0x04, 0x06, 0xFD, 0x02); // SET OpenMusic bit (play Boot sound and Call Prompts)
	// --- end sub_1B5A4

	// --- start sub_1B5DC
	// hard coded 128 * 0xAA
	uint8_t spi_values[128];
	for (int i=0; i<128;i++)
	{
		spi_values[i]=0xaa;
	}
	write_SPI_page_reg_bytearray_SPI0(0x03, 0x00, spi_values, 0x80);
	// --- end sub_1B5DC

	// --- start sub_1B5A4
	set_clear_SPI_page_reg_byte_with_mask_SPI0(0x04, 0x06, 0xFD, 0x00); // CLEAR OpenMusic bit (play Boot sound and Call Prompts)
	// --- end sub_1B5A4

	write_SPI_page_reg_byte_SPI0(0x04, 0x37, 0x9E); // MCU take control of CODEC
	set_clear_SPI_page_reg_byte_with_mask_SPI0(0x04, 0xE4, 0x3F, 0x00); // Set CODEC LineOut Gain to 0dB
	// ------ end spi_more_init
}

void SPI_C6000_postinit()
{
	write_SPI_page_reg_byte_SPI0(0x04, 0x04, 0xE8);  //Set Mod2 output offset
	write_SPI_page_reg_byte_SPI0(0x04, 0x46, 0x37);  //Set Mod1 Amplitude
	write_SPI_page_reg_byte_SPI0(0x04, 0x48, 0x03);  //Set 2 Point Mod Bias
	write_SPI_page_reg_byte_SPI0(0x04, 0x47, 0xE8);  //Set 2 Point Mod Bias

	write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x20);  //set sync fail bit (reset?)
	write_SPI_page_reg_byte_SPI0(0x04, 0x40, 0x03);  //Disable DMR Tx and Rx
	write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x00);  //Reset all bits. 
	write_SPI_page_reg_byte_SPI0(0x04, 0x00, 0x3F);  //Reset DMR Protocol and Physical layer modules.
	write_SPI_page_reg_bytearray_SPI0(0x01, 0x04, spi_init_values_1, 0x06);
	write_SPI_page_reg_byte_SPI0(0x04, 0x10, 0x6E);  //Set DMR, Tier2, Timeslot mode, Layer2, Repeater, Aligned, Slot 1
	write_SPI_page_reg_byte_SPI0(0x04, 0x1F, 0x10);  // Set Local EMB. DMR Colour code in upper 4 bits - defaulted to 1, and is updated elsewhere in the code
	write_SPI_page_reg_byte_SPI0(0x04, 0x26, 0x7D);  //Undocumented Register 
	write_SPI_page_reg_byte_SPI0(0x04, 0x27, 0x40);  //Undocumented Register 
	write_SPI_page_reg_byte_SPI0(0x04, 0x28, 0x7D);  //Undocumented Register 
	write_SPI_page_reg_byte_SPI0(0x04, 0x29, 0x40);  //Undocumented Register 
	write_SPI_page_reg_byte_SPI0(0x04, 0x2A, 0x0B);  //Set SPI Clock to default value
	write_SPI_page_reg_byte_SPI0(0x04, 0x2B, 0x0B);  //According to Datasheet this is a Read only register For FM Squelch
	write_SPI_page_reg_byte_SPI0(0x04, 0x2C, 0x17);  //According to Datasheet this is a Read only register For FM Squelch
	write_SPI_page_reg_byte_SPI0(0x04, 0x2D, 0x05);  //Set FM Compression and Decompression points (?)
	write_SPI_page_reg_byte_SPI0(0x04, 0x56, 0x00);  //Undocumented Register
	write_SPI_page_reg_byte_SPI0(0x04, 0x5F, 0xC0);  //Enable Sync detection for MS or BS orignated signals
	write_SPI_page_reg_byte_SPI0(0x04, 0x81, 0xFF);  //Enable all Interrupts
	write_SPI_page_reg_byte_SPI0(0x04, 0x01, 0x70);  //Set 2 Point Mod, Swap Rx I and Q, Rx Mode IF
	write_SPI_page_reg_byte_SPI0(0x04, 0x03, 0x00);  //Zero Receive I Offset
	write_SPI_page_reg_byte_SPI0(0x04, 0x05, 0x00);  //Zero Receive Q Offset
	write_SPI_page_reg_byte_SPI0(0x04, 0x12, 0x15);  //Set RF Switching Receive to Transmit Advance
	write_SPI_page_reg_byte_SPI0(0x04, 0xA1, 0x80);  //According to Datasheet this register is for FM Modulation Setting (?)
	write_SPI_page_reg_byte_SPI0(0x04, 0xC0, 0x0A);  //Set RF Signal Advance to 1ms (10x100us)
	write_SPI_page_reg_byte_SPI0(0x04, 0x06, 0x21);  //Use SPI vocoder under MCU control
	write_SPI_page_reg_byte_SPI0(0x04, 0x07, 0x0B);  //Set IF Frequency H to default 450KHz
	write_SPI_page_reg_byte_SPI0(0x04, 0x08, 0xB8);  //Set IF Frequency M to default 450KHz
	write_SPI_page_reg_byte_SPI0(0x04, 0x09, 0x00);  //Set IF Frequency l to default 450KHz
	write_SPI_page_reg_byte_SPI0(0x04, 0x0D, 0x10);  //Set Voice Superframe timeout value
	write_SPI_page_reg_byte_SPI0(0x04, 0x0E, 0x8E);  //Register Documented as Reserved 
	write_SPI_page_reg_byte_SPI0(0x04, 0x0F, 0xB8);  //FSK Error Count
	write_SPI_page_reg_byte_SPI0(0x04, 0xC2, 0x00);  //Disable Mic Gain AGC
	write_SPI_page_reg_byte_SPI0(0x04, 0xE0, 0x8B);  //CODEC under MCU Control, LineOut2 Enabled, Mic_p Enabled, I2S Slave Mode
	write_SPI_page_reg_byte_SPI0(0x04, 0xE1, 0x0F);  //Undocumented Register (Probably associated with CODEC)
	write_SPI_page_reg_byte_SPI0(0x04, 0xE2, 0x06);  //CODEC  Anti Pop Enabled, DAC Output Enabled
	write_SPI_page_reg_byte_SPI0(0x04, 0xE3, 0x52);  //CODEC Default Settings
	write_SPI_page_reg_byte_SPI0(0x04, 0xE4, 0x4A);  //CODEC   LineOut Gain 2dB, Mic Stage 1 Gain 0dB, Mic Stage 2 Gain 30dB
	write_SPI_page_reg_byte_SPI0(0x04, 0xE5, 0x1A);  //CODEC Default Setting
	write_SPI_page_reg_byte_SPI0(0x04, 0x26, 0x7D);  //Undocumented Register
	write_SPI_page_reg_byte_SPI0(0x04, 0x27, 0x40);  //Undocumented Register
	write_SPI_page_reg_byte_SPI0(0x04, 0x28, 0x7D);  //Undocumented Register
	write_SPI_page_reg_byte_SPI0(0x04, 0x29, 0x40);  //Undocumented Register
	write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x20);  //Set Sync Fail Bit  (Reset?)
	write_SPI_page_reg_byte_SPI0(0x04, 0x40, 0xC3);  //Enable DMR Tx and Rx, Passive Timing
	write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x40);  //Set Receive During Next Slot Bit
	write_SPI_page_reg_byte_SPI0(0x04, 0x01, 0x70);  //Set 2 Point Mod, Swap Rx I and Q, Rx Mode IF
	write_SPI_page_reg_byte_SPI0(0x04, 0x10, 0x6E);  //Set DMR, Tier2, Timeslot mode, Layer2, Repeater, Aligned, Slot 1
	write_SPI_page_reg_byte_SPI0(0x04, 0x00, 0x3F);  //Reset DMR Protocol and Physical layer modules.
	write_SPI_page_reg_byte_SPI0(0x04, 0xE4, 0x4B);  //CODEC   LineOut Gain 2dB, Mic Stage 1 Gain 0dB, Mic Stage 2 Gain 33dB
}


static void HRC6000SysInterruptHandler();
static void HRC6000TimeslotInterruptHandler();
static void HRC6000RxInterruptHandler();
static void HRC6000TxInterruptHandler();


void PORTC_IRQHandler(void)
{
    if ((1U << Pin_INT_C6000_SYS) & PORT_GetPinsInterruptFlags(Port_INT_C6000_SYS))
    {
    	HRC6000SysInterruptHandler();
        PORT_ClearPinsInterruptFlags(Port_INT_C6000_SYS, (1U << Pin_INT_C6000_SYS));
    }
    if ((1U << Pin_INT_C6000_TS) & PORT_GetPinsInterruptFlags(Port_INT_C6000_TS))
    {
    	HRC6000TimeslotInterruptHandler();
        PORT_ClearPinsInterruptFlags(Port_INT_C6000_TS, (1U << Pin_INT_C6000_TS));
    }
    if ((1U << Pin_INT_C6000_RF_RX) & PORT_GetPinsInterruptFlags(Port_INT_C6000_RF_RX))
    {
    	HRC6000RxInterruptHandler();
        PORT_ClearPinsInterruptFlags(Port_INT_C6000_RF_RX, (1U << Pin_INT_C6000_RF_RX));
    }
    if ((1U << Pin_INT_C6000_RF_TX) & PORT_GetPinsInterruptFlags(Port_INT_C6000_RF_TX))
    {
    	HRC6000TxInterruptHandler();

        PORT_ClearPinsInterruptFlags(Port_INT_C6000_RF_TX, (1U << Pin_INT_C6000_RF_TX));
    }

    int_timeout=0;

    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
    exception return operation might vector to incorrect interrupt */
    __DSB();
}

void HRC6000SysInterruptHandler()
{
	read_SPI_page_reg_byte_SPI0(0x04, 0x82, &tmp_val_0x82);  //Read Interrupt Flag Register1


	if (tmp_val_0x82 & 0x80)
	{
		/*
			Bit7: In DMR mode: indicates that the transmission request rejects the interrupt without a sub-status register.
			In DMR mode, it indicates that this transmission request is rejected because the channel is busy;
		 */
	}

	if (tmp_val_0x82 & 0x40)
	{
		/*
		 	Bit6: In DMR mode: indicates the start of transmission; in MSK mode: indicates that the ping-pong buffer is half-full interrupted.
			In DMR mode, the sub-status register 0x84 is transmitted at the beginning, and the corresponding interrupt can be masked by 0x85.
		 */

		read_SPI_page_reg_byte_SPI0(0x04, 0x84, &tmp_val_0x84);  //Read sub status register
		/*
			The sub-status registers indicate
			seven interrupts that initiate the transmission, including:
			Bit7: Voice transmission starts

			Bit6: OACSU requests to send interrupts, including first-time send and resend requests.

			Bit5: End-to-end voice enhanced encryption interrupt, including EMB72bits update interrupt
			and voice 216bits key update interrupt, which are distinguished by Bit5~Bit4 of Register
			0x88, where 01 indicates EMB72bits update interrupt and 10 indicates voice 216bits key update interrupt.

			Bit4: 	The Vocoder configuration returns an interrupt (this interrupt is sent by the HR_C6000
					to the MCU when the MCU manually configures the AMBE3000). This interrupt is only
					valid when using the external AMBE3000 vocoder.

			Bit3: Data transmission starts

			Bit2: Data partial retransmission

			Bit1: Data retransmission

			Bit0: The vocoder is initialized to an interrupt. This interrupt is only valid when using an external AMBE3000 or AMBE1000 vocoder.

			In MSK mode, there is no sub-interrupt status.
		*/
	}
	else
	{
		tmp_val_0x84=0x00;
	}

	if (tmp_val_0x82 & 0x20)
	{
		/*
			Bit5: In DMR mode: indicates the end of transmission; in MSK mode: indicates the end of	transmission.
		*/

		read_SPI_page_reg_byte_SPI0(0x04, 0x86, &tmp_val_0x86);  //Read Interrupt Flag Register2

		/*
			In DMR mode, there is a sub-status register 0x86 at the end of the transmission, and the
			corresponding interrupt can be masked by 0x87. The sub-status register indicates six interrupts that
			generate the end of the transmission, including:

			Bit7: 	Indicates that the service transmission is completely terminated, including voice and data.
					The MCU distinguishes whether the voice or data is sent this time. Confirming that the
					data service is received is the response packet that receives the correct feedback.

			Bit6: 	Indicates that a Fragment length confirmation packet is sent in the sliding window data
					service without immediate feedback.

			Bit5: VoiceOACSU wait timeout

			Bit4: 	The Layer 2 mode handles the interrupt. The MCU sends the configuration information
					to the last processing timing of the chip to control the interrupt. If after the interrupt, the
					MCU has not written all the information to be sent in the next frame to the chip, the next
					time slot cannot be Configured to send time slots. This interrupt is only valid when the chip
					is operating in Layer 2 mode.

			Bit3: 	indicates that a Fragment that needs to be fed back confirms the completion of the data
					packet transmission. The interrupt is mainly applied to the acknowledgment message after
					all the data packets have been sent or the data packet that needs to be fed back in the sliding
					window data service is sent to the MCU to start waiting for the timing of the Response
					packet. Device.

			Bit2 : ShortLC Receive Interrupt

			Bit1: BS activation timeout interrupt

			In MSK mode, there is no substate interrupt.
		*/
	}
	else
	{
		tmp_val_0x86=0x00;
	}

	if (tmp_val_0x82 & 0x10)
	{
		/*
		Bit4:
			In DMR mode: indicates the access interruption;
			In MSK mode: indicates that the response response is interrupted.

			In DMR mode, the access interrupt has no sub-status register.
			After receiving the interrupt, it indicates that the access voice communication mode is post-access. the way.

			In MSK mode, this interrupt has no substatus registers.
		*/
	}

	if (tmp_val_0x82 & 0x08)
	{
		/*
			Bit3: 	In DMR mode: indicates that the control frame parsing completion interrupt;
					In MSK mode: indicates the receive interrupt.

			In DMR mode, this interrupt has no sub-status register, but the error and receive type of its received
			data is given by the 0x51 register. The DLLRecvDataType, DLLRecvCRC are used to indicate the
			received data type and the error status, and the MCU accordingly performs the corresponding status.

			Display, you can also block the corresponding interrupt.
			In MSK mode, this interrupt has no substate interrupts.

			The FMB frame's EMB information parsing completion prompt is also the completion of the
			interrupt, which is distinguished by judging the 0x51 register SyncClass=0.

		*/

		read_SPI_page_reg_byte_SPI0(0x04, 0x51, &tmp_val_0x51);
	}
	else
	{
		tmp_val_0x51 = 0x00;
	}

	if (tmp_val_0x82 & 0x04)
	{
		/*
			Bit2:
			In DMR mode: indicates service data reception interrupt; in FM mode: indicates FM function detection interrupt.
			In DMR mode, this interrupt has sub-status register 0x90, which has three types:
			1. 0x80 indicates that the entire information is received and verified. After the service data is
			verified, the MCU extracts the data after the address 0x30 in the RX terminal 1.2KRAM
			through the SPI port. The length of the data is defined by the corresponding field of the
			received frame header.
			2. 0x00 indicates the entire information reception check error;
			3. 0x40 indicates that a non-confirmed SMS abnormal interrupt is generated;

			In FM mode, the interrupt has sub-status register 0x90, and the sub-status register has 1 type:
			1. 0x10 indicates that the FM function detection interrupt is matched. When the FM interrupt is
			detected in the FM mode, the corresponding analog sound output is turned on.
		*/
		read_SPI_page_reg_byte_SPI0(0x04, 0x90, &tmp_val_0x90);

	}
	else
	{
		tmp_val_0x90 = 0x00;
	}


	if (tmp_val_0x82 & 0x02)
	{
		/*
			Bit1: In DMR mode: indicates that the voice is abnormally exited;
			In DMR mode, the cause of the abnormality in DMR mode is the unexpected abnormal voice
			interrupt generated inside the state machine. The corresponding voice exception type is obtained
			through Bit2~Bit0 of register address 0x98.
		*/
		read_SPI_page_reg_byte_SPI0(0x04, 0x98, &tmp_val_0x98);
	}
	else
	{
		tmp_val_0x98 = 0x00;
	}

	if (tmp_val_0x82 & 0x02)
	{
	/*
		Bit0: physical layer separate work reception interrupt
		The physical layer works independently to receive interrupts without a sub-status register. The
		interrupt is generated in the physical layer single working mode. After receiving the data, the
		interrupt is generated, and the MCU is notified to read the corresponding register to obtain the
		received data. This interrupt is typically tested in bit error rate or other performance in physical
		layer mode.
	*/
	}

	read_SPI_page_reg_byte_SPI0(0x04, 0x52, &tmp_val_0x52);  //Read Received CC and CACH Register
//	read_SPI_page_reg_byte_SPI0(0x04, 0x57, &tmp_val_0x57);  //Undocumented register
	read_SPI_page_reg_byte_SPI0(0x04, 0x5f, &tmp_val_0x5f);  //Read Received Sync type register
	read_SPI_page_reg_bytearray_SPI0(0x02, 0x00, DMR_frame_buffer, DRM_FRAME_BUFFER_SIZE);

	int_sys=true;
	timer_hrc6000task=0;
}

void HRC6000TimeslotInterruptHandler()
{

	int_ts=false;
	// RX/TX state machine
	switch (slot_state)
	{
		case DMR_STATE_RX_1: // Start RX (first step)
			write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x00);     //No Transmit or receive in next timeslot
			GPIO_PinWrite(GPIO_speaker_mute, Pin_speaker_mute, 1);
			//GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 1);
			slot_state = DMR_STATE_RX_2;
			break;
		case DMR_STATE_RX_2: // Start RX (second step)
			write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x50);   //  Receive during Next Timeslot And Layer2 Access success Bit
			slot_state = DMR_STATE_RX_1;
			break;
		case DMR_STATE_RX_END: // Stop RX
			init_digital_DMR_RX();
			GPIO_PinWrite(GPIO_speaker_mute, Pin_speaker_mute, 0);
			GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
			slot_state = DMR_STATE_IDLE;
			break;
		case DMR_STATE_TX_START_1: // Start TX (second step)
			write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x80);    //Transmit during next Timeslot
			write_SPI_page_reg_byte_SPI0(0x04, 0x50, 0x10);    //Set Data Type to 0001 (Voice LC Header), Data, LCSS=00
			slot_state = DMR_STATE_TX_START_2;
			break;
		case DMR_STATE_TX_START_2: // Start TX (third step)
			write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x40); 		//Receive during next Timeslot
			slot_state = DMR_STATE_TX_START_3;
			break;
		case DMR_STATE_TX_START_3: // Start TX (fourth step)
			write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x80);     //Transmit during Next Timeslot
			write_SPI_page_reg_byte_SPI0(0x04, 0x50, 0x10);     //Set Data Type to 0001 (Voice LC Header), Data, LCSS=00
			slot_state = DMR_STATE_TX_START_4;
			break;
		case DMR_STATE_TX_START_4: // Start TX (fifth step)
			write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x40); 	//Receive during Next Timeslot
			slot_state = DMR_STATE_TX_START_5;
			break;
		/*
		 * TX_START_5 etc
		 * SOME OTHER CASES IN HERE PROCESSED BY THE RTOS task function
		 */

		case DMR_STATE_TX_1: // Ongoing TX (inactive timeslot)
			if ((trxIsTransmitting==false) && (tx_sequence==0))
			{
				slot_state = DMR_STATE_TX_END_1; // only exit here to ensure staying in the correct timeslot
			}
			else
			{
				write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x40); // Receive during next Timeslot
				slot_state = DMR_STATE_TX_2;
			}
			break;

		case DMR_STATE_TX_END_1: // Stop TX (first step)
			write_SPI_page_reg_bytearray_SPI1(0x03, 0x00, SILENCE_AUDIO, 27);// send silence audio bytes
			write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x80);  //Transmit during Next Timeslot
			write_SPI_page_reg_byte_SPI0(0x04, 0x50, 0x20);  // Data Type =0010 (Terminator with LC), Data, LCSS=0
			slot_state = DMR_STATE_TX_END_2;
			break;
		case DMR_STATE_TX_END_2: // Stop TX (second step)
			write_SPI_page_reg_byte_SPI0(0x04, 0x40, 0xC3);  //Enable DMR Tx and Rx, Passive Timing
			init_digital_DMR_RX();
			txstopdelay=30;
			slot_state = DMR_STATE_IDLE;
			break;

		default:
			int_ts=true;// tell the RTOS based state machine that there is something to do
			break;
	}


	timer_hrc6000task=0;// I don't think this actually does anything. Its probably redundant legacy code

}

void HRC6000RxInterruptHandler()
{
	trx_deactivateTX();
}

void HRC6000TxInterruptHandler()
{
	trx_activateTX();
}


void init_HR_C6000_interrupts()
{
	init_digital_state();

    PORT_SetPinInterruptConfig(Port_INT_C6000_SYS, Pin_INT_C6000_SYS, kPORT_InterruptFallingEdge);
    PORT_SetPinInterruptConfig(Port_INT_C6000_TS, Pin_INT_C6000_TS, kPORT_InterruptFallingEdge);
    PORT_SetPinInterruptConfig(Port_INT_C6000_RF_RX, Pin_INT_C6000_RF_RX, kPORT_InterruptFallingEdge);
    PORT_SetPinInterruptConfig(Port_INT_C6000_RF_TX, Pin_INT_C6000_RF_TX, kPORT_InterruptFallingEdge);

    NVIC_SetPriority(PORTC_IRQn, 3);
}

void init_digital_state()
{
	taskENTER_CRITICAL();
	int_sys=false;
	int_ts=false;
	int_timeout=0;
	taskEXIT_CRITICAL();
	slot_state = DMR_STATE_IDLE;
	tick_cnt=0;
	skip_count=0;
	qsodata_timer = 0;
}

void init_digital_DMR_RX()
{
	write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x20);  //Set Sync Fail Bit (Reset?))
	write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x00);  //Reset
	write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x20);  //Set Sync Fail Bit (Reset?)
	write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x40);  //Receive during next Timeslot
}

void init_digital()
{
	GPIO_PinWrite(GPIO_speaker_mute, Pin_speaker_mute, 0);
    GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
	init_digital_DMR_RX();
	init_digital_state();
    NVIC_EnableIRQ(PORTC_IRQn);
	init_codec();
}

void terminate_digital()
{
	GPIO_PinWrite(GPIO_speaker_mute, Pin_speaker_mute, 0);
    GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
	init_digital_state();
    NVIC_DisableIRQ(PORTC_IRQn);
}

void store_qsodata()
{
	// If this is the start of a newly received signal, we always need to trigger the display to show this, even if its the same station calling again.
	// Of if the display is holding on the PC accept text and the incoming call is not a PC
	if (qsodata_timer == 0 || (menuUtilityReceivedPcId!=0 && DMR_frame_buffer[0]==TG_CALL_FLAG))
	{
		menuDisplayQSODataState = QSO_DISPLAY_CALLER_DATA;
	}
	// check if this is a valid data frame, including Talker Alias data frames (0x04 - 0x07)
	// Not sure if its necessary to check byte [1] for 0x00 but I'm doing this
	if (DMR_frame_buffer[1] == 0x00  && (DMR_frame_buffer[0]==TG_CALL_FLAG || DMR_frame_buffer[0]==PC_CALL_FLAG  || (DMR_frame_buffer[0]>=0x04 && DMR_frame_buffer[0]<=0x7)))
	{
		// Needs to enter critical because the LastHeard is accessed by other threads
    	taskENTER_CRITICAL();
    	lastHeardListUpdate((uint8_t *)DMR_frame_buffer);
		taskEXIT_CRITICAL();
		qsodata_timer=QSO_TIMER_TIMEOUT;
	}
}

void init_hrc6000_task()
{
	xTaskCreate(fw_hrc6000_task,                        /* pointer to the task */
				"fw hrc6000 task",                      /* task name for kernel awareness debugging */
				5000L / sizeof(portSTACK_TYPE),      /* task stack size */
				NULL,                      			 /* optional task startup argument */
				5U,                                  /* initial priority */
				fwhrc6000TaskHandle					 /* optional task handle to create */
				);
}

void fw_hrc6000_task()
{
    while (1U)
    {
    	taskENTER_CRITICAL();
    	uint32_t tmp_timer_hrc6000task=timer_hrc6000task;
    	taskEXIT_CRITICAL();
    	if (tmp_timer_hrc6000task==0)
    	{
        	taskENTER_CRITICAL();
        	timer_hrc6000task=10;
        	alive_hrc6000task=true;
        	taskEXIT_CRITICAL();

        	if (trxGetMode() == RADIO_MODE_DIGITAL)
			{
				tick_HR_C6000();
			}
			else
			{
				if (trxGetMode() == RADIO_MODE_ANALOG && melody_play==NULL)
				{
					trx_check_analog_squelch();
				}
			}
    	}

		vTaskDelay(0);
    }
}

void buildLCDataFromParams(uint8_t *data,uint8_t FLCO,uint32_t srcId,uint32_t dstId)
{
	data[0] = FLCO;
	data[1] = 0x00;
	data[2] = 0x00;
	data[3] = (dstId >> 16) & 0xFF;
	data[4] = (dstId >> 8) & 0xFF;
	data[5] = (dstId >> 0) & 0xFF;
	data[6] = (srcId >> 16) & 0xFF;
	data[7] = (srcId >> 8) & 0xFF;
	data[8] = (srcId >> 0) & 0xFF;
	data[9] = 0x00;
	data[10] = 0x00;
	data[11] = 0x00;
}

void buildLC_DataFromLD_Data(uint8_t *outData,uint8_t *LC_DataBytes)
{
	memcpy(outData,LC_DataBytes,9);
	outData[9] = 0x00;
	outData[10] = 0x00;
	outData[11] = 0x00;
}

void setupPcOrTGHeader()
{
	spi_tx[0] = (trxTalkGroupOrPcId >> 24) & 0xFF;
	spi_tx[1] = 0x00;
	spi_tx[2] = 0x00;
	spi_tx[3] = (trxTalkGroupOrPcId >> 16) & 0xFF;
	spi_tx[4] = (trxTalkGroupOrPcId >> 8) & 0xFF;
	spi_tx[5] = (trxTalkGroupOrPcId >> 0) & 0xFF;
	spi_tx[6] = (trxDMRID >> 16) & 0xFF;
	spi_tx[7] = (trxDMRID >> 8) & 0xFF;
	spi_tx[8] = (trxDMRID >> 0) & 0xFF;
	spi_tx[9] = 0x00;
	spi_tx[10] = 0x00;
	spi_tx[11] = 0x00;
	write_SPI_page_reg_bytearray_SPI0(0x02, 0x00, spi_tx, 0x0c);
}

bool callAcceptFilter()
{
	 if (settingsUsbMode == USB_MODE_HOTSPOT)
	 {
  		    //In Hotspot mode, we need to accept all incoming traffic, otherwise private calls won't work
			if (DMR_frame_buffer[0]==TG_CALL_FLAG || DMR_frame_buffer[0]==PC_CALL_FLAG)
			{
				return true;
			}
			else
			{
				return false;// Not a PC or TG call
			}
	 }
	 else
	 {
		 return ( (DMR_frame_buffer[0]==TG_CALL_FLAG) || (DMR_frame_buffer[0]==PC_CALL_FLAG && receivedTgOrPcId == trxDMRID));
	 }
}

void tick_HR_C6000()
{
	bool tmp_int_sys=false;
	bool tmp_int_ts=false;
	taskENTER_CRITICAL();
	if (int_sys)
	{
		tmp_int_sys=true;
		int_sys=false;
	}
	if (int_ts)
	{
		tmp_int_ts=true;
		int_ts=false;
	}
	taskEXIT_CRITICAL();

	if (trxIsTransmitting==true && (slot_state == DMR_STATE_IDLE)) // Start TX (first step)
	{
		if (settingsUsbMode != USB_MODE_HOTSPOT)
		{
			init_codec();
			setupPcOrTGHeader();
		}
		else
		{
			write_SPI_page_reg_bytearray_SPI0(0x02, 0x00, (uint8_t *)&audioAndHotspotDataBuffer.hotspotBuffer[wavbuffer_read_idx], 0x0c);// put LC into hardware
		}

		write_SPI_page_reg_byte_SPI0(0x04, 0x40, 0xE3); // TX and RX enable, Active Timing
		write_SPI_page_reg_byte_SPI0(0x04, 0x21, 0xA2); // Set Polite to Color Code and Reset vocoder encodingbuffer
		write_SPI_page_reg_byte_SPI0(0x04, 0x22, 0x86); // Start Vocoder Encode, I2S mode

		slot_state = DMR_STATE_TX_START_1;
	}

	// Timeout interrupt
	if (slot_state != DMR_STATE_IDLE)
	{
		if (int_timeout<200)
		{
			int_timeout++;
			if (int_timeout==200)
			{
	            	init_digital();
	            	slot_state = DMR_STATE_IDLE;
	            	int_timeout=0;
#if defined(USE_SEGGER_RTT) && defined(DEBUG_DMR_DATA)
            	SEGGER_RTT_printf(0, ">>> INTERRUPT TIMEOUT\r\n");
#endif
			}
		}
	}
	else
	{
		int_timeout=0;
	}

	if (tmp_int_ts)
	{
		// RX/TX state machine
		switch (slot_state)
		{
		/*
		 * MOVED TO ISR
		case DMR_STATE_RX_1: // Start RX (first step)
			write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x00);     //No Transmit or receive in next timeslot
			GPIO_PinWrite(GPIO_speaker_mute, Pin_speaker_mute, 1);
		    //GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 1);
			slot_state = DMR_STATE_RX_2;
			break;
		case DMR_STATE_RX_2: // Start RX (second step)
			write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x50);   //  Receive during Next Timeslot And Layer2 Access success Bit
			slot_state = DMR_STATE_RX_1;
			break;
		case DMR_STATE_RX_END: // Stop RX
			init_digital_DMR_RX();
			GPIO_PinWrite(GPIO_speaker_mute, Pin_speaker_mute, 0);
		    GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
			slot_state = DMR_STATE_IDLE;
			break;
		case DMR_STATE_TX_START_1: // Start TX (second step)
			write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x80);    //Transmit during next Timeslot
			write_SPI_page_reg_byte_SPI0(0x04, 0x50, 0x10);    //Set Data Type to 0001 (Voice LC Header), Data, LCSS=00
			slot_state = DMR_STATE_TX_START_2;
			break;
		case DMR_STATE_TX_START_2: // Start TX (third step)
			write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x40); 		//Receive during next Timeslot
			slot_state = DMR_STATE_TX_START_3;
			break;
		case DMR_STATE_TX_START_3: // Start TX (fourth step)
			write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x80);     //Transmit during Next Timeslot
			write_SPI_page_reg_byte_SPI0(0x04, 0x50, 0x10);     //Set Data Type to 0001 (Voice LC Header), Data, LCSS=00
			slot_state = DMR_STATE_TX_START_4;
			break;
		case DMR_STATE_TX_START_4: // Start TX (fifth step)
			write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x40); 	//Receive during Next Timeslot
			slot_state = DMR_STATE_TX_START_5;
			break;
			*/
		case DMR_STATE_TX_START_5: // Start TX (sixth step)
			write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x80);   //Transmit during next Timeslot
			write_SPI_page_reg_byte_SPI0(0x04, 0x50, 0x10);   //Set Data Type to 0001 (Voice LC Header), Data, LCSS=00
			tx_sequence=0;
			slot_state = DMR_STATE_TX_1;

            if (settingsUsbMode != USB_MODE_HOTSPOT)
            {
            	tick_TXsoundbuffer();
            }

			break;
			/* Moved to TS ISR
		case DMR_STATE_TX_1: // Ongoing TX (inactive timeslot)
			if ((trxIsTransmitting==false) && (tx_sequence==0))
			{
				slot_state = DMR_STATE_TX_END_1; // only exit here to ensure staying in the correct timeslot
			}
			else
			{
				write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x40); // Receive during next Timeslot
				slot_state = DMR_STATE_TX_2;
			}
			break;
			*/
		case DMR_STATE_TX_2: // Ongoing TX (active timeslot)

			memset((uint8_t *)DMR_frame_buffer, 0, 27 + 0x0C);// fills the LC and  audio buffer with zeros in case there is no real audio

			if (trxIsTransmitting)
			{

                if (settingsUsbMode != USB_MODE_HOTSPOT)
                {
                	tick_TXsoundbuffer();
    				tick_codec_encode((uint8_t *)DMR_frame_buffer+0x0C);
                }
                else
                {
                	if (wavbuffer_count > 0)
                	{
						memcpy((uint8_t *)DMR_frame_buffer,(uint8_t *)&audioAndHotspotDataBuffer.hotspotBuffer[wavbuffer_read_idx],27+0x0C);

						if(tx_sequence==0)
						{
							write_SPI_page_reg_bytearray_SPI0(0x02, 0x00, (uint8_t*)DMR_frame_buffer, 0x0c);// put LC into hardware
						}

						wavbuffer_read_idx++;
						if (wavbuffer_read_idx > (HOTSPOT_BUFFER_COUNT-1))
						{
							wavbuffer_read_idx=0;
						}

						if (wavbuffer_count>0)
						{
							wavbuffer_count--;
						}

                	}
                }
    			write_SPI_page_reg_bytearray_SPI1(0x03, 0x00, (uint8_t*)(DMR_frame_buffer+0x0C), 27);// send the audio bytes to the hardware
			}
			else
			{
				write_SPI_page_reg_bytearray_SPI1(0x03, 0x00, (uint8_t *)SILENCE_AUDIO, 27);// send silence audio bytes
			}


			write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x80); // Transmit during next Timeslot
			switch (tx_sequence)
			{
			case 0:
				write_SPI_page_reg_byte_SPI0(0x04, 0x50, 0x08); // Data Type=0000 (Voice Frame A) , Voice, LCSS = 0
				break;
			case 1:
				write_SPI_page_reg_byte_SPI0(0x04, 0x50, 0x18); // Data Type=0001 (Voice Frame B) , Voice, LCSS = 0
				break;
			case 2:
				write_SPI_page_reg_byte_SPI0(0x04, 0x50, 0x28); // Data Type=0010 (Voice Frame C) , Voice, LCSS = 0
				break;
			case 3:
				write_SPI_page_reg_byte_SPI0(0x04, 0x50, 0x38); // Data Type=0011 (Voice Frame D) , Voice, LCSS = 0
				break;
			case 4:
				write_SPI_page_reg_byte_SPI0(0x04, 0x50, 0x48); // Data Type=0100 (Voice Frame E) , Voice, LCSS = 0
				break;
			case 5:
				write_SPI_page_reg_byte_SPI0(0x04, 0x50, 0x58); // Data Type=0101 (Voice Frame F) , Voice, LCSS = 0
				break;
			}
			tx_sequence++;
			if (tx_sequence>5)
			{
				tx_sequence=0;
			}
			slot_state = DMR_STATE_TX_1;
			break;

		/*
		 * Moved to the TS ISR
		case DMR_STATE_TX_END_1: // Stop TX (first step)
			write_SPI_page_reg_bytearray_SPI1(0x03, 0x00, SILENCE_AUDIO, 27);// send silence audio bytes
			write_SPI_page_reg_byte_SPI0(0x04, 0x41, 0x80);  //Transmit during Next Timeslot
			write_SPI_page_reg_byte_SPI0(0x04, 0x50, 0x20);  // Data Type =0010 (Terminator with LC), Data, LCSS=0
			slot_state = DMR_STATE_TX_END_2;
			break;
		case DMR_STATE_TX_END_2: // Stop TX (second step)
			write_SPI_page_reg_byte_SPI0(0x04, 0x40, 0xC3);  //Enable DMR Tx and Rx, Passive Timing
			init_digital_DMR_RX();
			txstopdelay=30;
			slot_state = DMR_STATE_IDLE;
			break;
			*/
		}

		// Timeout interrupted RX
    	if ((slot_state < DMR_STATE_TX_START_1) && (tick_cnt<10))
    	{
    		tick_cnt++;
            if (tick_cnt==10)
            {
            	slot_state = DMR_STATE_RX_END;
#if defined(USE_SEGGER_RTT) && defined(DEBUG_DMR_DATA)
            	SEGGER_RTT_printf(0, ">>> TIMEOUT\r\n");
#endif
            }
    	}
	}

	if (tmp_int_sys)
	{
		// Check for correct received packet
		int rcrc = (tmp_val_0x51 >> 2) & 0x01;
		int rpi = (tmp_val_0x51 >> 3) & 0x01;
		int cc 	= (tmp_val_0x52 >> 4) & 0x0f;
        if ((rcrc==0) && (rpi==0) && (cc == trxGetDMRColourCode()) && (slot_state < DMR_STATE_TX_START_1))
        {
		    GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 1);// Turn the LED on as soon as a DMR signal is detected.
    		if (tmp_val_0x82 & 0x20) // InterSendStop interrupt
    		{
    			write_SPI_page_reg_byte_SPI0(0x04, 0x83, 0x20);
    		}

    		receivedTgOrPcId 	= (DMR_frame_buffer[3]<<16)+(DMR_frame_buffer[4]<<8)+(DMR_frame_buffer[5]<<0);// used by the call accept filter
    		receivedSrcId 		= (DMR_frame_buffer[6]<<16)+(DMR_frame_buffer[7]<<8)+(DMR_frame_buffer[8]<<0);// used by the call accept filter

    		if (tmp_val_0x82 & 0x10) // InterLateEntry interrupt
    		{
    			// Late entry into ongoing RX
                if (slot_state == DMR_STATE_IDLE && callAcceptFilter())
                {
                	slot_state = DMR_STATE_RX_1;
                	store_qsodata();
                	init_codec();
                	skip_count = 2;

#if defined(USE_SEGGER_RTT) && defined(DEBUG_DMR_DATA)
					SEGGER_RTT_printf(0, ">>> RX START LATE\r\n");
#endif
					if (settingsUsbMode == USB_MODE_HOTSPOT)
					{
						DMR_frame_buffer[27 + 0x0c] = HOTSPOT_RX_START_LATE;
						hotspotRxFrameHandler((uint8_t *)DMR_frame_buffer);
					}
                }

    			write_SPI_page_reg_byte_SPI0(0x04, 0x83, 0x10);   //Clear Late Entry Interrupt Flag
    		}

    		if (tmp_val_0x82 & 0x08) // InterRecvData
    		{
    			// Reset RX timeout
    			tick_cnt = 0;

    			// Start RX
    			int rxdt = (tmp_val_0x51 >> 4) & 0x0f;   //Recieved Data Type  
    			int sc = (tmp_val_0x51 >> 0) & 0x03;     //Received Sync Class  0=Sync Header 1=Voice 2=data 3=RC
                if ((slot_state == DMR_STATE_IDLE) && (sc==2) && (rxdt==1) &&  callAcceptFilter())       //Voice LC Header
                {
                	slot_state = DMR_STATE_RX_1;
                	store_qsodata();
                	init_codec();
                	skip_count = 0;
#if defined(USE_SEGGER_RTT) && defined(DEBUG_DMR_DATA)
            	SEGGER_RTT_printf(0, ">>> RX START\r\n");
#endif
					if (settingsUsbMode == USB_MODE_HOTSPOT)
					{
						DMR_frame_buffer[27 + 0x0c] = HOTSPOT_RX_START;
						hotspotRxFrameHandler((uint8_t *)DMR_frame_buffer);
					}
                }
    			// Stop RX
                if ((sc==2) && (rxdt==2) && callAcceptFilter())        //Terminator with LC
                {
                	slot_state = DMR_STATE_RX_END;
#if defined(USE_SEGGER_RTT) && defined(DEBUG_DMR_DATA)
            	SEGGER_RTT_printf(0, ">>> RX STOP\r\n");
#endif

					if (settingsUsbMode == USB_MODE_HOTSPOT)
					{
						DMR_frame_buffer[27 + 0x0c] = HOTSPOT_RX_STOP;
						hotspotRxFrameHandler((uint8_t *)DMR_frame_buffer);
					}
                }

            	if ((slot_state!=0) && (skip_count>0) && (sc!=2) && ((rxdt & 0x07) == 0x01))
            	{
            		skip_count--;
            	}

                // Detect/decode voice packet and transfer it into the output soundbuffer
                if ((slot_state != DMR_STATE_IDLE) && (skip_count==0) && (sc!=2) && ((rxdt & 0x07) >= 0x01) && ((rxdt & 0x07) <= 0x06))
                {
#if false
                	SEGGER_RTT_printf(0, ">>> Audio frame %02x\r\n",rxdt & 0x07);
#endif
                	store_qsodata();
                    read_SPI_page_reg_bytearray_SPI1(0x03, 0x00, DMR_frame_buffer+0x0C, 27);
                    if (settingsUsbMode == USB_MODE_HOTSPOT)
                    {
   						DMR_frame_buffer[27 + 0x0c] = HOTSPOT_RX_AUDIO_FRAME;
   						DMR_frame_buffer[27 + 0x0c + 1] = (rxdt & 0x07);// audio sequence number
   						hotspotRxFrameHandler((uint8_t *)DMR_frame_buffer);
                    }
                    else
                    {
                    	if (settingsPrivateCallMuteMode == false)
                    	{
                    		tick_codec_decode((uint8_t *)DMR_frame_buffer+0x0C);
                    		tick_RXsoundbuffer();
                    	}
                    }
                }
                else
                {
#if false
                    if (settingsUsbMode == USB_MODE_HOTSPOT)
                    {
   						DMR_frame_buffer[27 + 0x0c] = HOTSPOT_RX_IDLE_OR_REPEAT;
   						hotspotRxFrameHandler(DMR_frame_buffer);
                    }

                	SEGGER_RTT_printf(0, ">>> Not valid data (perhaps Idle) or another Voice header LC frame\r\n");
#endif
                }

#if defined(USE_SEGGER_RTT) && defined(DEBUG_DMR_DATA)
                SEGGER_RTT_printf(0, "DATA %02x [%02x %02x] %02x %02x %02x %02x SC:%02x RCRC:%02x RPI:%02x RXDT:%02x LCSS:%02x TC:%02x AT:%02x CC:%02x ??:%02x ST:%02x RAM:", slot_state, tmp_val_0x82, tmp_val_0x86, tmp_val_0x51, tmp_val_0x52, tmp_val_0x57, tmp_val_0x5f, (tmp_val_0x51 >> 0) & 0x03, (tmp_val_0x51 >> 2) & 0x01, (tmp_val_0x51 >> 3) & 0x01, (tmp_val_0x51 >> 4) & 0x0f, (tmp_val_0x52 >> 0) & 0x03, (tmp_val_0x52 >> 2) & 0x01, (tmp_val_0x52 >> 3) & 0x01, (tmp_val_0x52 >> 4) & 0x0f, (tmp_val_0x57 >> 2) & 0x01, (tmp_val_0x5f >> 0) & 0x03);
				for (int i=0;i<0x0c;i++)
				{
	            	SEGGER_RTT_printf(0, " %02x", DMR_frame_buffer[i]);
				}
            	SEGGER_RTT_printf(0, "\r\n");
#endif

    			write_SPI_page_reg_byte_SPI0(0x04, 0x83, 0x08);     // Clear Data Received Interrupt Flag
    		}

    		if (tmp_val_0x82 & 0x01) // InterPHYOnly flag
    		{
    			write_SPI_page_reg_byte_SPI0(0x04, 0x83, 0x01);  //Clear  InterPHYOnly flag
    		}

    		if (tmp_val_0x82 & 0xC6)   //Remaining Interrupt Flags
    		{
    			write_SPI_page_reg_byte_SPI0(0x04, 0x83, 0xC6);  //Clear remaining Interrupt Flags
    		}
        }
        else if (slot_state >= DMR_STATE_TX_START_1)
        {
        	uint8_t tmp_val_0x42;
			read_SPI_page_reg_byte_SPI0(0x04, 0x42, &tmp_val_0x42);   //Read Current Timeslot Sent, Current Timeslot Received and Current Timeslot Active Status bits
#if defined(USE_SEGGER_RTT) && defined(DEBUG_DMR_DATA)
            	SEGGER_RTT_printf(0, "TXTX %02x [%02x %02x] %02x\r\n", slot_state, tmp_val_0x82, tmp_val_0x86, tmp_val_0x42);
#endif
    		write_SPI_page_reg_byte_SPI0(0x04, 0x83, 0xFF);  //Clear all Interrupt Flags
        }
        else
        {
#if defined(USE_SEGGER_RTT) && defined(DEBUG_DMR_DATA)
            	SEGGER_RTT_printf(0, "---- %02x [%02x %02x] %02x %02x %02x %02x SC:%02x RCRC:%02x RPI:%02x RXDT:%02x LCSS:%02x TC:%02x AT:%02x CC:%02x ??:%02x ST:%02x RAM:", slot_state, tmp_val_0x82, tmp_val_0x86, tmp_val_0x51, tmp_val_0x52, tmp_val_0x57, tmp_val_0x5f, (tmp_val_0x51 >> 0) & 0x03, (tmp_val_0x51 >> 2) & 0x01, (tmp_val_0x51 >> 3) & 0x01, (tmp_val_0x51 >> 4) & 0x0f, (tmp_val_0x52 >> 0) & 0x03, (tmp_val_0x52 >> 2) & 0x01, (tmp_val_0x52 >> 3) & 0x01, (tmp_val_0x52 >> 4) & 0x0f, (tmp_val_0x57 >> 2) & 0x01, (tmp_val_0x5f >> 0) & 0x03);
				for (int i=0;i<0x0c;i++)
				{
	            	SEGGER_RTT_printf(0, " %02x", DMR_frame_buffer[i]);
				}
            	SEGGER_RTT_printf(0, "\r\n");
#endif
        }
	}

	if (qsodata_timer>0)
	{
		// Only timeout the QSO data display if not displaying the Private Call Accept Yes/No text
		// if menuUtilityReceivedPcId is non zero the Private Call Accept text is being displayed
		if (menuUtilityReceivedPcId == 0)
		{
			qsodata_timer--;

			if (qsodata_timer==0)
			{
				menuDisplayQSODataState= QSO_DISPLAY_DEFAULT_SCREEN;
			}
		}

	}
}
