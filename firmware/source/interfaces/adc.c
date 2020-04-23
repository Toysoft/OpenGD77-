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

#include <adc.h>

static volatile uint32_t adc_channel;
volatile uint32_t adc0_dp1;
volatile uint32_t adc0_dp3;


const int CUTOFF_VOLTAGE_UPPER_HYST = 64;
const int CUTOFF_VOLTAGE_LOWER_HYST = 62;
const int BATTERY_MAX_VOLTAGE = 82;

void trigger_adc(void)
{
    adc16_channel_config_t adc16ChannelConfigStruct;

    adc16ChannelConfigStruct.channelNumber = adc_channel;
    adc16ChannelConfigStruct.enableInterruptOnConversionCompleted = true;
    adc16ChannelConfigStruct.enableDifferentialConversion = false;
    ADC16_SetChannelConfig(ADC0, 0, &adc16ChannelConfigStruct);
}

void adc_init(void)
{
	adc16_config_t adc16ConfigStruct;

	taskENTER_CRITICAL();
	adc_channel = 1;
	adc0_dp1 = 0;
	adc0_dp3 = 0;

	taskEXIT_CRITICAL();

    ADC16_GetDefaultConfig(&adc16ConfigStruct);
    ADC16_Init(ADC0, &adc16ConfigStruct);
    ADC16_EnableHardwareTrigger(ADC0, false);
    ADC16_DoAutoCalibration(ADC0);

    EnableIRQ(ADC0_IRQn);

    trigger_adc();
}

void ADC0_IRQHandler(void)
{
	uint32_t result = ADC16_GetChannelConversionValue(ADC0, 0);

    switch (adc_channel)
    {
    case 1:
    	adc0_dp1 = result;
    	adc_channel = 3;// get channel 3 next
    	break;
    case 3:
    	adc0_dp3 = result;
    	adc_channel = 1;// get channel 1 next
    	break;
    }

    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
    exception return operation might vector to incorrect interrupt */
    __DSB();
}

// result of conversion is rounded voltage*10 as integer
int get_battery_voltage(void)
{
	int tmp_voltage = adc0_dp1/41.6f+0.5f;
	return tmp_voltage;
}

int getVOX(void)
{
	return adc0_dp3;
}
