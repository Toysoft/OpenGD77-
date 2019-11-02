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

#include "fw_EEPROM.h"

const uint8_t EEPROM_ADDRESS 	= 0x50;
const uint8_t EEPROM_PAGE_SIZE 	= 128;

// !!! Wait 5ms between consecutive calls !!!
bool EEPROM_Write(int address,uint8_t *buf, int size)
{
	const int COMMAND_SIZE = 2;
	int transferSize;
	uint8_t tmpBuf[COMMAND_SIZE];
    i2c_master_transfer_t masterXfer;
    status_t status;

	taskENTER_CRITICAL();
    while(size > 0)
    {
		transferSize = size>EEPROM_PAGE_SIZE?EEPROM_PAGE_SIZE:size;
		tmpBuf[0] = address >> 8;
		tmpBuf[1] = address & 0xff;

		memset(&masterXfer, 0, sizeof(masterXfer));
		masterXfer.slaveAddress = EEPROM_ADDRESS;
		masterXfer.direction = kI2C_Write;
		masterXfer.subaddress = 0;
		masterXfer.subaddressSize = 0;
		masterXfer.data = tmpBuf;
		masterXfer.dataSize = COMMAND_SIZE;
		masterXfer.flags = kI2C_TransferNoStopFlag;//kI2C_TransferDefaultFlag;

		// EEPROM Will not respond if it is busy completing the previous write.
		// So repeat the write command until it responds or timeout after 50
		// attempts 1mS apart

		int timoutCount=50;
		status=kStatus_Success;
		do
		{
			if(status!=kStatus_Success)
			{
				vTaskDelay(portTICK_PERIOD_MS * 1);
			}

		status = I2C_MasterTransferBlocking(I2C0, &masterXfer);

		}while((status!=kStatus_Success)& (timoutCount-- >0));

		if (status != kStatus_Success)
		{
	    	taskEXIT_CRITICAL();
			return false;
		}

		memset(&masterXfer, 0, sizeof(masterXfer));
		masterXfer.slaveAddress = EEPROM_ADDRESS;
		masterXfer.direction = kI2C_Write;
		masterXfer.subaddress = 0;
		masterXfer.subaddressSize = 0;
		masterXfer.data = buf;
		masterXfer.dataSize = transferSize;
		masterXfer.flags = kI2C_TransferNoStartFlag;//kI2C_TransferDefaultFlag;

		status = I2C_MasterTransferBlocking(I2C0, &masterXfer);
		if (status != kStatus_Success)
		{
	    	taskEXIT_CRITICAL();
			return status;
		}
		address += transferSize;
		size -= transferSize;
    }
	taskEXIT_CRITICAL();
	return true;
}

bool EEPROM_Read(int address,uint8_t *buf, int size)
{
	const int COMMAND_SIZE = 2;
	uint8_t tmpBuf[COMMAND_SIZE];
    i2c_master_transfer_t masterXfer;
    status_t status;

	taskENTER_CRITICAL();
    tmpBuf[0] = address >> 8;
    tmpBuf[1] = address & 0xff;

    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = EEPROM_ADDRESS;
    masterXfer.direction = kI2C_Write;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = tmpBuf;
    masterXfer.dataSize = COMMAND_SIZE;
    masterXfer.flags = kI2C_TransferNoStopFlag;

	int timoutCount=50;
	status=kStatus_Success;
	do
	{
		if(status!=kStatus_Success)
		{
			vTaskDelay(portTICK_PERIOD_MS * 1);
		}

	status = I2C_MasterTransferBlocking(I2C0, &masterXfer);

	}while((status!=kStatus_Success)& (timoutCount-- >0));

	if (status != kStatus_Success)
	{
    	taskEXIT_CRITICAL();
		return false;
	}

    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = EEPROM_ADDRESS;
    masterXfer.direction = kI2C_Read;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = buf;
    masterXfer.dataSize = size;
    masterXfer.flags = kI2C_TransferRepeatedStartFlag;

    status = I2C_MasterTransferBlocking(I2C0, &masterXfer);
    if (status != kStatus_Success)
    {
    	taskEXIT_CRITICAL();
    	return false;
    }

	taskEXIT_CRITICAL();
	return true;
}
