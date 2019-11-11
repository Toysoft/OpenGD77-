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
#include <user_interface/menuHotspot.h>
#include <user_interface/menuSystem.h>
#include "fw_usb_com.h"
#include "fw_settings.h"
#include "fw_wdog.h"
#include <stdarg.h>


static void handleCPSRequest();

volatile uint8_t com_buffer[COM_BUFFER_SIZE];
int com_buffer_write_idx = 0;
int com_buffer_read_idx = 0;
volatile int com_buffer_cnt = 0;

volatile int com_request = 0;
volatile uint8_t com_requestbuffer[COM_REQUESTBUFFER_SIZE];
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE) uint8_t usbComSendBuf[COM_BUFFER_SIZE];//DATA_BUFF_SIZE


int sector = -1;

//#define ENABLE_HOTSPOT_MODE

void tick_com_request()
{
		switch (settingsUsbMode)
		{
			case USB_MODE_CPS:
				if (com_request==1)
				{
#ifdef ENABLE_HOTSPOT_MODE
					if (com_requestbuffer[0]==0xE0)
					{
						settingsUsbMode = USB_MODE_HOTSPOT;
						menuSystemPushNewMenu(MENU_HOTSPOT_MODE);
						return;
					}
#endif
					taskENTER_CRITICAL();
					handleCPSRequest();
					taskEXIT_CRITICAL();
					com_request=0;
				}

				break;
			case USB_MODE_HOTSPOT:
				break;

	}

}

static void handleCPSRequest()
{
	//Handle read
	if (com_requestbuffer[0]=='R') // 'R' read data (com_requestbuffer[1]: 1 => external flash, 2 => EEPROM)
	{
		uint32_t address=(com_requestbuffer[2]<<24)+(com_requestbuffer[3]<<16)+(com_requestbuffer[4]<<8)+(com_requestbuffer[5]<<0);
		uint32_t length=(com_requestbuffer[6]<<8)+(com_requestbuffer[7]<<0);
		if (length>32)
		{
			length=32;
		}

		bool result=false;
		if (com_requestbuffer[1]==1)
		{
			taskEXIT_CRITICAL();
			result = SPI_Flash_read(address, &usbComSendBuf[3], length);
			taskENTER_CRITICAL();
		}
		else if (com_requestbuffer[1]==2)
		{
			taskEXIT_CRITICAL();
			result = EEPROM_Read(address, &usbComSendBuf[3], length);
			taskENTER_CRITICAL();
		}

		if (result)
		{
			usbComSendBuf[0] = com_requestbuffer[0];
			usbComSendBuf[1]=(length>>8)&0xFF;
			usbComSendBuf[2]=(length>>0)&0xFF;
			USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, usbComSendBuf, length+3);
		}
		else
		{
			usbComSendBuf[0] = '-';
			USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, usbComSendBuf, 1);
		}
	}
	// Handle Write
	else if (com_requestbuffer[0]=='W')
	{
		bool ok=false;
		if (com_requestbuffer[1]==1)
		{
			if (sector==-1)
			{
				sector=(com_requestbuffer[2]<<16)+(com_requestbuffer[3]<<8)+(com_requestbuffer[4]<<0);
				taskEXIT_CRITICAL();
				ok = SPI_Flash_read(sector*4096,SPI_Flash_sectorbuffer,4096);
				taskENTER_CRITICAL();
			}
		}
		else if (com_requestbuffer[1]==2)
		{
			if (sector>=0)
			{
				uint32_t address=(com_requestbuffer[2]<<24)+(com_requestbuffer[3]<<16)+(com_requestbuffer[4]<<8)+(com_requestbuffer[5]<<0);
				uint32_t length=(com_requestbuffer[6]<<8)+(com_requestbuffer[7]<<0);
				if (length>32)
				{
					length=32;
				}

				for (int i=0;i<length;i++)
				{
					if (sector==(address+i)/4096)
					{
						SPI_Flash_sectorbuffer[(address+i) % 4096]=com_requestbuffer[i+8];
					}
				}

				ok=true;
			}
		}
		else if (com_requestbuffer[1]==3)
		{
			if (sector>=0)
			{
				taskEXIT_CRITICAL();
				ok = SPI_Flash_eraseSector(sector*4096);
				taskENTER_CRITICAL();
				if (ok)
				{
					for (int i=0;i<16;i++)
					{
						taskEXIT_CRITICAL();
						ok = SPI_Flash_writePage(sector*4096+i*256,SPI_Flash_sectorbuffer+i*256);
						taskENTER_CRITICAL();
						if (!ok)
						{
							break;
						}
					}
				}
				sector=-1;
			}
		}
		else if (com_requestbuffer[1]==4)
		{
			uint32_t address=(com_requestbuffer[2]<<24)+(com_requestbuffer[3]<<16)+(com_requestbuffer[4]<<8)+(com_requestbuffer[5]<<0);
			uint32_t length=(com_requestbuffer[6]<<8)+(com_requestbuffer[7]<<0);
			if (length>32)
			{
				length=32;
			}

			ok = EEPROM_Write(address, (uint8_t*)com_requestbuffer+8, length);
		}

		if (ok)
		{
			usbComSendBuf[0] = com_requestbuffer[0];
			usbComSendBuf[1] = com_requestbuffer[1];
			USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, usbComSendBuf, 2);
		}
		else
		{
			sector=-1;
			usbComSendBuf[0] = '-';
			USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, usbComSendBuf, 1);
		}
	}
	// Handle a "Command"
	else if (com_requestbuffer[0]=='C')
	{
		int command = com_requestbuffer[1];
		switch(command)
		{
			case 0:
				// Show CPS screen
				menuSystemPushNewMenu(MENU_CPS);
				break;
			case 1:
				// Clear CPS screen
				menuCPSUpdate(0,0,0,0,0,0,NULL);
				break;
			case 2:
				// Write a line of text to CPS screen
				menuCPSUpdate(1,com_requestbuffer[2],com_requestbuffer[3],com_requestbuffer[4],com_requestbuffer[5],com_requestbuffer[6],(char *)&com_requestbuffer[7]);
				break;
			case 3:
				// Render CPS screen
				menuCPSUpdate(2,0,0,0,0,0,NULL);
				break;
			case 4:
				// Turn on the display backlight
				menuCPSUpdate(3,0,0,0,0,0,NULL);
				break;
			case 5:
				// Close
				menuCPSUpdate(6,0,0,0,0,0,NULL);
				break;
			case 6:
				{
					int subCommand= com_requestbuffer[2];
					// Do some other processing
					switch(subCommand)
					{
						case 0:
							// save current settings and reboot
							settingsSaveSettings();// Need to save these channels prior to reboot, as reboot does not save
							watchdogReboot();
						break;
						case 1:
							//reload VFO from codeplug
							settingsInitVFOChannel();
							settingsSaveSettings();// Need to save these channels prior to reboot, as reboot does not save
							watchdogReboot();
							break;
						case 2:
							// factory reset and reboot
							settingsRestoreDefaultSettings();// Also saves these new settings, so now need to call settingsSaveSettings
							watchdogReboot();
							break;
						case 3:
							// flash green LED
							menuCPSUpdate(4,0,0,0,0,0,NULL);
							break;
						case 4:
							// flash red LED
							menuCPSUpdate(5,0,0,0,0,0,NULL);
							break;
						default:
							break;
					}
				}
				break;
			default:
				break;
		}
		// Send something generic back.
		// Probably need to send a response code in the future
		usbComSendBuf[0] = '-';
		USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, usbComSendBuf, 1);
	}
	else
	{
		usbComSendBuf[0] = '-';
		USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, usbComSendBuf, 1);
	}
}
#if false
void send_packet(uint8_t val_0x82, uint8_t val_0x86, int ram)
{
	taskENTER_CRITICAL();
	if ((HR_C6000_datalogging) && ((com_buffer_cnt+8+(ram+1))<=COM_BUFFER_SIZE))
	{
		add_to_commbuffer((com_buffer_cnt >> 8) & 0xff);
		add_to_commbuffer((com_buffer_cnt >> 0) & 0xff);
		add_to_commbuffer(val_0x82);
		add_to_commbuffer(val_0x86);
		add_to_commbuffer(tmp_val_0x51);
		add_to_commbuffer(tmp_val_0x52);
		add_to_commbuffer(tmp_val_0x57);
		add_to_commbuffer(tmp_val_0x5f);
		for (int i=0;i<=ram;i++)
		{
			add_to_commbuffer(DMR_frame_buffer[i]);
		}
	}
	taskEXIT_CRITICAL();
}

uint8_t tmp_ram1[256];
uint8_t tmp_ram2[256];

void send_packet_big(uint8_t val_0x82, uint8_t val_0x86, int ram1, int ram2)
{
	taskENTER_CRITICAL();
	if ((HR_C6000_datalogging) && ((com_buffer_cnt+8+(ram1+1)+(ram2+1))<=COM_BUFFER_SIZE))
	{
		add_to_commbuffer((com_buffer_cnt >> 8) & 0xff);
		add_to_commbuffer((com_buffer_cnt >> 0) & 0xff);
		add_to_commbuffer(val_0x82);
		add_to_commbuffer(val_0x86);
		add_to_commbuffer(tmp_val_0x51);
		add_to_commbuffer(tmp_val_0x52);
		add_to_commbuffer(tmp_val_0x57);
		add_to_commbuffer(tmp_val_0x5f);
		for (int i=0;i<=ram1;i++)
		{
			add_to_commbuffer(tmp_ram1[i]);
		}
		for (int i=0;i<=ram2;i++)
		{
			add_to_commbuffer(tmp_ram2[i]);
		}
	}
	taskEXIT_CRITICAL();
}

void add_to_commbuffer(uint8_t value)
{
	com_buffer[com_buffer_write_idx]=value;
	com_buffer_cnt++;
	com_buffer_write_idx++;
	if (com_buffer_write_idx==COM_BUFFER_SIZE)
	{
		com_buffer_write_idx=0;
	}
}
#endif
void USB_DEBUG_PRINT(char *str)
{
	strcpy((char*)usbComSendBuf,str);
	USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, usbComSendBuf, strlen(str));
}

void USB_DEBUG_printf(const char *format, ...) {
	  char buf[80];

	  va_list params;

	  va_start(params, format);
	  vsnprintf(buf, 77, format, params);
	  strcat(buf, "\n");
	  va_end(params);
	  USB_DEBUG_PRINT(buf);
}
