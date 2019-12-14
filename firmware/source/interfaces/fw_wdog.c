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

#include "fw_wdog.h"
#include "fw_pit.h"

TaskHandle_t fwwatchdogTaskHandle;

static WDOG_Type *wdog_base = WDOG;
int watchdog_refresh_tick=0;

volatile bool alive_maintask;
volatile bool alive_beeptask;
volatile bool alive_hrc6000task;

float averageBatteryVoltage;
int battery_voltage = 0;
int battery_voltage_tick = 0;
static bool reboot=false;
static const int BATTERY_VOLTAGE_TICK_RELOAD = 2000;
static const int AVERAGE_BATTERY_VOLTAGE_SAMPLE_WINDOW = 60.0f;// 120 secs = Sample window * BATTERY_VOLTAGE_TICK_RELOAD in milliseconds

#define VOLTAGE_BUFFER_LEN 128 // At one sample each 2 minutes: ~ 4.3 hours

typedef struct
{
	int32_t  buffer[VOLTAGE_BUFFER_LEN];
	int32_t *buffer_end;
	size_t   capacity;
	int32_t *head;
	int32_t *tail;
} voltageCircularBuffer_t;

voltageCircularBuffer_t batteryVoltageHistory;

static void circularBufferInit(voltageCircularBuffer_t *cb, size_t capacity)
{
	cb->buffer_end = &cb->buffer[VOLTAGE_BUFFER_LEN - 1];
	cb->capacity = capacity;
	cb->head = cb->buffer;
	cb->tail = cb->buffer;
}

static void circularBufferPushBack(voltageCircularBuffer_t *cb, const int32_t item)
{
	*cb->head = item;
	cb->head++;

    if(cb->head == cb->buffer_end)
    	cb->head = cb->buffer;

    if (cb->tail == cb->head)
    {
    	cb->tail++;

    	if(cb->tail == cb->buffer_end)
    		cb->tail = cb->buffer;
    }
}

static size_t circularBufferGetData(voltageCircularBuffer_t *cb, int32_t *data, size_t dataLen)
{
     size_t  count = 0;
     int32_t *p = cb->tail;

     while ((p != cb->head) && (count < dataLen))
     {
    	 *(data + count) = *p;

    	 p++;
    	 count++;

    	 if (p == cb->buffer_end)
    		 p = cb->buffer;
     }

     return count;
}

size_t batteryGetHistoryData(int32_t *data, size_t dataLen)
{
	size_t l = 0;

	taskENTER_CRITICAL();
	l = circularBufferGetData(&batteryVoltageHistory, data, dataLen);
	taskEXIT_CRITICAL();

	return l;
}


void init_watchdog(void)
{
    wdog_config_t config;
    WDOG_GetDefaultConfig(&config);
    config.timeoutValue = 0x3ffU;
    WDOG_Init(wdog_base, &config);
    for (uint32_t i = 0; i < 256; i++)
    {
    	wdog_base->RSTCNT;
    }

    watchdog_refresh_tick=0;

    alive_maintask = false;
    alive_beeptask = false;
    alive_hrc6000task = false;

    battery_voltage=get_battery_voltage();
	averageBatteryVoltage = battery_voltage;
	battery_voltage_tick=0;

	circularBufferInit(&batteryVoltageHistory, (sizeof(batteryVoltageHistory.buffer) / sizeof(batteryVoltageHistory.buffer[0])));
	circularBufferPushBack(&batteryVoltageHistory, battery_voltage);

	xTaskCreate(fw_watchdog_task,                        /* pointer to the task */
				"fw watchdog task",                      /* task name for kernel awareness debugging */
				1000L / sizeof(portSTACK_TYPE),      /* task stack size */
				NULL,                      			 /* optional task startup argument */
				5U,                                  /* initial priority */
				fwwatchdogTaskHandle					 /* optional task handle to create */
				);
}

void fw_watchdog_task(void *data)
{
    while (1U)
    {
    	taskENTER_CRITICAL();
    	uint32_t tmp_timer_watchdogtask=timer_watchdogtask;
    	taskEXIT_CRITICAL();
    	if (tmp_timer_watchdogtask==0)
    	{
        	taskENTER_CRITICAL();
        	timer_watchdogtask=10;
        	taskEXIT_CRITICAL();

        	tick_watchdog();
    	}

		vTaskDelay(0);
    }
}


void tick_watchdog(void)
{

	watchdog_refresh_tick++;
	if (watchdog_refresh_tick==200)
	{
		if (alive_maintask && alive_beeptask && alive_hrc6000task && !reboot)
		{
	    	WDOG_Refresh(wdog_base);
		}
	    alive_maintask = false;
	    alive_beeptask = false;
	    alive_hrc6000task = false;
    	watchdog_refresh_tick=0;
	}

	battery_voltage_tick++;
	if (battery_voltage_tick == BATTERY_VOLTAGE_TICK_RELOAD)
	{
		int tmp_battery_voltage = get_battery_voltage();

		circularBufferPushBack(&batteryVoltageHistory, tmp_battery_voltage);

		if (battery_voltage!=tmp_battery_voltage)
		{
			battery_voltage=tmp_battery_voltage;
			averageBatteryVoltage = (averageBatteryVoltage * (AVERAGE_BATTERY_VOLTAGE_SAMPLE_WINDOW-1) + battery_voltage) / AVERAGE_BATTERY_VOLTAGE_SAMPLE_WINDOW;
		}
		battery_voltage_tick=0;
	}
	trigger_adc();// need the ADC value next time though, so request conversion now, so that its ready by the time we need it
}

void watchdogReboot(void)
{
	reboot=true;
}
