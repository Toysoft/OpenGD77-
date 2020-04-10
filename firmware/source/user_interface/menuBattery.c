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

#define VOLTAGE_BUFFER_LEN 128
static const float BATTERY_CRITICAL_VOLTAGE = 66.7f;

typedef struct
{
	int32_t  buffer[VOLTAGE_BUFFER_LEN];
	int32_t *head;
	int32_t *tail;
	int32_t *end;
	bool     modified;
} voltageCircularBuffer_t;

__attribute__((section(".data.$RAM2"))) voltageCircularBuffer_t batteryVoltageHistory;

enum { BATTERY_LEVEL = 0, BATTERY_GRAPH };
enum { GRAPH_FILL = 0, GRAPH_LINE };

static int displayMode = BATTERY_LEVEL;
static int graphStyle = GRAPH_FILL;
static int battery_stack_iter = 0;
static const int BATTERY_ITER_PUSHBACK = 20;


static void updateScreen(bool forceRedraw);
static void handleEvent(uiEvent_t *ev);


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
		ucRenderRows(0, 2);

		updateScreen(true);
	}
	else
	{
		if ((ev->time - m) > 500)
		{
			m = ev->time;
			updateScreen(false);// update the screen each 500ms to show any changes to the battery voltage or low battery
		}

		if (ev->hasEvent)
			handleEvent(ev);
	}
	return 0;
}

static void updateScreen(bool forceRedraw)
{
	static bool blink = false;
	bool renderArrowOnly = true;

	switch (displayMode)
	{
		case BATTERY_LEVEL:
		{
			static float prevAverageBatteryVoltage = 0.0f;

			if ((prevAverageBatteryVoltage != averageBatteryVoltage) || (averageBatteryVoltage < BATTERY_CRITICAL_VOLTAGE) || forceRedraw)
			{
				char buffer[17];
				int val1 = averageBatteryVoltage / 10;
				int val2 = averageBatteryVoltage - (val1 * 10);

				prevAverageBatteryVoltage = averageBatteryVoltage;

				snprintf(buffer, 17, "%1d.%1dV", val1, val2);
				buffer[16] = 0;

				renderArrowOnly = false;

				if (forceRedraw)
				{
					// Clear whole drawing region
					ucFillRect(0, 14, 128, DISPLAY_SIZE_Y - 14, true);
					// Draw...
					// Inner body frame
					ucDrawRoundRect(97, 20, 26, DISPLAY_SIZE_Y-22, 3, true);
					// Outer body frame
					ucDrawRoundRect(96, 19, 28, DISPLAY_SIZE_Y - 20, 3, true);
					// Positive pole frame
					ucFillRoundRect(96+9, 15, 10, 6, 2, true);
				}
				else
				{
					// Clear voltage area
					ucFillRect(20, 22, (4 * 16), 32, true);
					// Clear level area
					ucFillRoundRect(100, 23, 20, DISPLAY_SIZE_Y - 28, 2, false);
				}

				ucPrintAt(20, 22, buffer, FONT_SIZE_4);

				uint32_t h = (uint32_t)(((averageBatteryVoltage - CUTOFF_VOLTAGE_UPPER_HYST) * DISPLAY_SIZE_Y - 28) / (BATTERY_MAX_VOLTAGE - CUTOFF_VOLTAGE_UPPER_HYST));
				if (h > DISPLAY_SIZE_Y - 28)
				{
					h = DISPLAY_SIZE_Y - 28;
				}

				// Draw Level
				ucFillRoundRect(100, 23 + DISPLAY_SIZE_Y - 28 - h , 20, h, 2, (averageBatteryVoltage < BATTERY_CRITICAL_VOLTAGE) ? blink : true);
			}

			// Low blinking arrow
			ucFillTriangle(63, DISPLAY_SIZE_Y -1 , 59, (DISPLAY_SIZE_Y -5), 67, (DISPLAY_SIZE_Y -5), blink);
		}
		break;

		case BATTERY_GRAPH:
		{
#define  chartWidth 104
			static int32_t hist[chartWidth];
			static size_t histLen = 0;
			bool newHistAvailable = false;

			// Grab history values.
			// There is a 10 ticks timeout, if it kicks in, history length will be 0, then
			// redraw will be done on the next run
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
				static const uint8_t chartX = 2 + (2 * 6) + 3 + 2;
				static const uint8_t chartY = 14 + 1 + 2;
				const int chartHeight = DISPLAY_SIZE_Y - 26;

				// Min is 6.4V, Max is 8.2V
				// Pick: MIN @ 7V, MAX @ 8V
				uint32_t minVH = (uint32_t)(((70 - CUTOFF_VOLTAGE_UPPER_HYST) * chartHeight) / (BATTERY_MAX_VOLTAGE - CUTOFF_VOLTAGE_UPPER_HYST));
				uint32_t maxVH = (uint32_t)(((80 - CUTOFF_VOLTAGE_UPPER_HYST) * chartHeight) / (BATTERY_MAX_VOLTAGE - CUTOFF_VOLTAGE_UPPER_HYST));

				renderArrowOnly = false;

				// Redraw chart's axes, ticks and so on
				if (forceRedraw)
				{
					// Clear whole drawing region
					ucFillRect(0, 14, 128, DISPLAY_SIZE_Y - 14, true);

					// 2 axis chart
					ucDrawFastVLine(chartX - 3, chartY - 2, chartHeight + 2 + 3, true);
					ucDrawFastVLine(chartX - 2, chartY - 2, chartHeight + 2 + 2, true);
					ucDrawFastHLine(chartX - 3, chartY + chartHeight + 2, chartWidth + 3 + 3, true);
					ucDrawFastHLine(chartX - 2, chartY + chartHeight + 1, chartWidth + 3 + 2, true);

					// Min/Max Voltage ticks and values
					ucDrawFastHLine(chartX - 6, (chartY + chartHeight) - minVH, 3, true);
					ucPrintAt(chartX - 3 - 12 - 3, ((chartY + chartHeight) - minVH) - 3, "7V", FONT_SIZE_1);
					ucDrawFastHLine(chartX - 6, (chartY + chartHeight) - maxVH, 3, true);
					ucPrintAt(chartX - 3 - 12 - 3, ((chartY + chartHeight) - maxVH) - 3, "8V", FONT_SIZE_1);

					// Time ticks
					for (uint8_t i = 0; i < chartWidth + 2; i += 22 /* ~ 15 minutes */)
					{
						ucSetPixel(chartX + i, (chartY + chartHeight) + 3, true);
					}
				}
				else
				{
					ucFillRect(chartX, chartY, chartWidth, chartHeight, true);
				}

				// Draw chart values, according to style
				for (size_t i = 0; i < histLen; i++)
				{
					uint32_t y = (uint32_t)(((hist[i] - CUTOFF_VOLTAGE_UPPER_HYST) * chartHeight) / (BATTERY_MAX_VOLTAGE - CUTOFF_VOLTAGE_UPPER_HYST));

					if (graphStyle == GRAPH_FILL)
						ucDrawFastVLine(chartX + i, ((chartY + chartHeight) - y), y, true);
					else
						ucSetPixel(chartX + i, ((chartY + chartHeight) - y), true);
				}

				// Min/Max dot lines
				for (uint8_t i = 0; i < chartWidth + 2; i++)
				{
					ucSetPixel(chartX + i, ((chartY + chartHeight) - minVH), (i % 2) ? false : true);
					ucSetPixel(chartX + i, ((chartY + chartHeight) - maxVH), (i % 2) ? false : true);
				}
			}

			// Upwards blinking arrow
			ucFillTriangle(63,(DISPLAY_SIZE_Y - 5), 59,(DISPLAY_SIZE_Y - 1), 67,(DISPLAY_SIZE_Y - 1), blink);
		}
		break;
	}

	blink = !blink;

	ucRenderRows((renderArrowOnly ? 7 : 1), 8);
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
			graphStyle = GRAPH_FILL;
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
	else if (KEYCHECK_PRESS(ev->keys,KEY_LEFT))
	{
		if (displayMode == BATTERY_GRAPH)
		{
			if (graphStyle == GRAPH_LINE)
			{
				graphStyle = GRAPH_FILL;
				updateScreen(true);
			}
		}
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_RIGHT))
	{
		if (displayMode == BATTERY_GRAPH)
		{
			if (graphStyle == GRAPH_FILL)
			{
				graphStyle = GRAPH_LINE;
				updateScreen(true);
			}
		}
	}

	displayLightTrigger();
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

// called every 2000 ticks
void menuBatteryPushBackVoltage(int32_t voltage)
{
	// Store value each 40k ticks
	if ((battery_stack_iter == 0) || (battery_stack_iter > BATTERY_ITER_PUSHBACK))
	{
		if (xSemaphoreTake(battSemaphore, (TickType_t)10) == pdTRUE)
		{
			circularBufferPushBack(&batteryVoltageHistory, voltage);
			xSemaphoreGive(battSemaphore);
		}

		battery_stack_iter = 0;
	}

	battery_stack_iter++;
}

