/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
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

#ifndef _SPI_FLASH_H_
#define _SPI_FLASH_H_

#include "FreeRTOS.h"
#include "task.h"

#include "common.h"


#if defined(PLATFORM_GD77) || defined(PLATFORM_GD77S)

#define Port_SPI_FLASH_CS_U  	PORTA
#define GPIO_SPI_FLASH_CS_U  	GPIOA
#define Pin_SPI_FLASH_CS_U   	19

#define Port_SPI_FLASH_CLK_U 	PORTE
#define GPIO_SPI_FLASH_CLK_U 	GPIOE
#define Pin_SPI_FLASH_CLK_U  	5

#define Port_SPI_FLASH_DI_U  	PORTE
#define GPIO_SPI_FLASH_DI_U  	GPIOE
#define Pin_SPI_FLASH_DI_U   	6

#define Port_SPI_FLASH_DO_U  	PORTE
#define GPIO_SPI_FLASH_DO_U  	GPIOE
#define Pin_SPI_FLASH_DO_U   	4

#elif defined(PLATFORM_DM1801)

#define Port_SPI_FLASH_CS_U PORTE
#define GPIO_SPI_FLASH_CS_U GPIOE
#define Pin_SPI_FLASH_CS_U 6

#define Port_SPI_FLASH_CLK_U PORTE
#define GPIO_SPI_FLASH_CLK_U GPIOE
#define Pin_SPI_FLASH_CLK_U 5

#define Port_SPI_FLASH_DI_U PORTA
#define GPIO_SPI_FLASH_DI_U GPIOA
#define Pin_SPI_FLASH_DI_U 19

#define Port_SPI_FLASH_DO_U PORTE
#define GPIO_SPI_FLASH_DO_U GPIOE
#define Pin_SPI_FLASH_DO_U 4

#elif defined(PLATFORM_RD5R)

#define Port_SPI_FLASH_CS_U  	PORTE
#define GPIO_SPI_FLASH_CS_U  	GPIOE
#define Pin_SPI_FLASH_CS_U   	2

#define Port_SPI_FLASH_CLK_U 	PORTE
#define GPIO_SPI_FLASH_CLK_U 	GPIOE
#define Pin_SPI_FLASH_CLK_U  	5

#define Port_SPI_FLASH_DI_U  	PORTE
#define GPIO_SPI_FLASH_DI_U  	GPIOE
#define Pin_SPI_FLASH_DI_U   	3

#define Port_SPI_FLASH_DO_U  	PORTE
#define GPIO_SPI_FLASH_DO_U  	GPIOE
#define Pin_SPI_FLASH_DO_U   	4

#endif




extern uint8_t SPI_Flash_sectorbuffer[4096];

// Public functions
bool SPI_Flash_init(void);
bool SPI_Flash_read(uint32_t addrress,uint8_t *buf,int size);
bool SPI_Flash_write(uint32_t addr, uint8_t *dataBuf, int size);
bool SPI_Flash_writePage(uint32_t address,uint8_t *dataBuf);// page is 256 bytes
bool SPI_Flash_eraseSector(uint32_t address);// sector is 16 pages  = 4k bytes
int SPI_Flash_readManufacturer(void);// Not necessarily Winbond !
int SPI_Flash_readPartID(void);// Should be 4014 for 1M or 4017 for 8M
int SPI_Flash_readStatusRegister(void);// May come in handy

#endif /* _SPI_FLASH_H_ */
