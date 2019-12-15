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
#include <user_interface/menuSystem.h>
#include <user_interface/uiLocalisation.h>


SemaphoreHandle_t battSemaphore = NULL;

#define VOLTAGE_BUFFER_LEN 128 // At one sample each 2 secs (BATTERY_VOLTAGE_TICK_RELOAD): ~ 4.3 minutes ATM

typedef struct
{
	int32_t  buffer[VOLTAGE_BUFFER_LEN];
	int32_t *head;
	int32_t *tail;
	int32_t *end;
	bool     modified;
} voltageCircularBuffer_t;

voltageCircularBuffer_t batteryVoltageHistory;


enum { BATTERY_LEVEL = 0, BATTERY_GRAPH };

static void updateScreen(bool forceRedraw);
static void handleEvent(uiEvent_t *ev);

static int displayMode = BATTERY_LEVEL;

static void circularBufferInit(voltageCircularBuffer_t *cb)
{
	cb->end = &cb->buffer[VOLTAGE_BUFFER_LEN - 1];
	cb->head = cb->buffer;
	cb->tail = cb->buffer;
	cb->modified = false;
}

static void circularBufferPushBack(voltageCircularBuffer_t *cb, const int32_t item)
{
	cb->modified = true;

	*cb->head = item;
	cb->head++;

    if(cb->head == cb->end)
    	cb->head = cb->buffer;

    if (cb->tail == cb->head)
    {
    	cb->tail++;

    	if(cb->tail == cb->end)
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

    	 if (p == cb->end)
    		 p = cb->buffer;
     }

     return count;
}

int menuBattery(uiEvent_t *ev, bool isFirstRun)
{
	static uint32_t m = 0;

	if (isFirstRun)
	{
		ucClearBuf();
		menuDisplayTitle(currentLanguage->battery);

		updateScreen(true);
	}
	else
	{
		if ((ev->ticks - m) > 2000)
		{
			m = ev->ticks;
			updateScreen(false);// update the screen each two seconds to show any changes to the battery voltage
		}

		if (ev->hasEvent)
			handleEvent(ev);
	}
	return 0;
}

static void updateScreen(bool forceRedraw)
{
	static bool blink = false;
	bool renderArrow = true;

	switch (displayMode)
	{
		case BATTERY_LEVEL:
		{
			static float prevAverageBatteryVoltage = 0.0f;

			if ((prevAverageBatteryVoltage != averageBatteryVoltage) || forceRedraw)
			{
				const int MAX_BATTERY_BAR_HEIGHT = 36;
				char buffer[17];
				int val1 = averageBatteryVoltage / 10;
				int val2 = averageBatteryVoltage - (val1 * 10);

				prevAverageBatteryVoltage = averageBatteryVoltage;

				snprintf(buffer, 17, "%1d.%1dV", val1, val2);
				buffer[16] = 0;

				renderArrow = false;

				if (forceRedraw)
				{
					// Clear drawing region
					ucFillRect(0, 14, 128, 64 - 14, true);

					// Draw...
					// Inner body frame
					ucDrawRoundRect(97, 20, 26, 42, 3, true);
					// Outer body frame
					ucDrawRoundRect(96, 19, 28, 44, 3, true);
					// Positive pole frame
					ucFillRoundRect(96+9, 15, 10, 6, 2, true);
				}
				else
				{
					// Clear voltage area
					ucFillRect(20, 22, (4 * 16), 32, true);
					// Clear level area
					ucFillRoundRect(100, 23, 20, MAX_BATTERY_BAR_HEIGHT, 2, false);
				}

				ucPrintAt(20, 22, buffer, FONT_16x32);

				uint32_t h = (uint32_t)(((averageBatteryVoltage - CUTOFF_VOLTAGE_UPPER_HYST) * MAX_BATTERY_BAR_HEIGHT) / (BATTERY_MAX_VOLTAGE - CUTOFF_VOLTAGE_UPPER_HYST));
				if (h > MAX_BATTERY_BAR_HEIGHT)
				{
					h = MAX_BATTERY_BAR_HEIGHT;
				}

				// Draw Level
				ucFillRoundRect(100, 23 + MAX_BATTERY_BAR_HEIGHT - h , 20, h, 2, (h < 6) /* < ~16.7%: 6.6V */ ? blink : true);
			}

			// low blinking arrow
			ucFillTriangle(63, 63, 59, 59, 67, 59, blink);
		}
		break;

		// Handle small redraw when possible
		// only redraw blinking arrow
		case BATTERY_GRAPH:
		{
			int32_t hist[120];
			size_t histLen = 0;
			bool newHistAvailable = false;
			char buf[17];

			if (xSemaphoreTake(battSemaphore, (TickType_t)10) == pdTRUE)
			{
				if ((newHistAvailable = batteryVoltageHistory.modified) == true)
				{
					histLen = circularBufferGetData(&batteryVoltageHistory, hist, (sizeof(hist) / sizeof(hist[0])));
					batteryVoltageHistory.modified = false;
				}
				xSemaphoreGive(battSemaphore);
			}

			if (newHistAvailable || forceRedraw)
			{
				renderArrow = false;

				// Update
				sprintf(buf, "%ld", histLen);

				// Clear drawing region
				ucFillRect(0, 14, 128, 64 - 14, true);

				ucPrintCentered(32, buf, FONT_8x16);
			}

			// low blinking arrow
			ucFillTriangle(63, 59, 59, 63, 67, 63, blink);
		}
		break;
	}

	//ucPrintCore(0, 56, "x", FONT_8x8, TEXT_ALIGN_LEFT, blink);
	blink = !blink;

	if (renderArrow)
		ucRenderRows(7, 8);
	else
		ucRender();

	displayLightTrigger();
}

static void handleEvent(uiEvent_t *ev)
{
	if (KEYCHECK_SHORTUP(ev->keys,KEY_RED))
	{
		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_GREEN))
	{
		menuSystemPopAllAndDisplayRootMenu();
		return;
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_DOWN))
	{
		if (displayMode == BATTERY_LEVEL)
		{
			displayMode = BATTERY_GRAPH;
			updateScreen(true);
		}
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_UP))
	{
		if (displayMode == BATTERY_GRAPH)
		{
			displayMode = BATTERY_LEVEL;
			updateScreen(true);
		}
	}
}

void menuBatteryInit(void)
{
	battSemaphore = xSemaphoreCreateMutex();

	if (battSemaphore == NULL)
	{
		while(true); // Something better maybe ?
	}

	circularBufferInit(&batteryVoltageHistory);
}

void menuBatteryPushBackVoltage(int32_t voltage)
{
	if (xSemaphoreTake(battSemaphore, (TickType_t)10) == pdTRUE)
	{
		circularBufferPushBack(&batteryVoltageHistory, voltage);
		xSemaphoreGive(battSemaphore);
	}
}

/*
size_t batteryGetHistoryData(int32_t *data, size_t dataLen)
{
	size_t l = 0;

	taskENTER_CRITICAL();
	l = circularBufferGetData(&batteryVoltageHistory, data, dataLen);
	taskEXIT_CRITICAL();

	return l;
}
*/

