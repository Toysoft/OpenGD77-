/*
 * Copyright (C)2019 Kai Ludwig, DG4KLU
 *
 * PWM modifications by Roger Clark VK3KYY / G4KYF
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

#include <display.h>
#include <hardware/UC1701.h>
#include <settings.h>
#include "FreeRTOS.h"
#include "task.h"


#if ! defined(PLATFORM_GD77S)
#ifdef DISPLAY_LED_PWM
	#include "fsl_ftm.h"
#endif
#endif // ! PLATFORM_GD77S

void displayInit(bool isInverseColour)
{
#if ! defined(PLATFORM_GD77S)
	PORT_SetPinMux(Port_Display_CS, Pin_Display_CS, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_Display_RST, Pin_Display_RST, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_Display_RS, Pin_Display_RS, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_Display_SCK, Pin_Display_SCK, kPORT_MuxAsGpio);
	PORT_SetPinMux(Port_Display_SDA, Pin_Display_SDA, kPORT_MuxAsGpio);

#ifdef DISPLAY_LED_PWM

#if defined(PLATFORM_RD5R)
    PORT_SetPinMux(Port_Display_Light, Pin_Display_Light, kPORT_MuxAlt7);/* Configured as PWM FTM0_CH2 */
#else
    PORT_SetPinMux(Port_Display_Light, Pin_Display_Light, kPORT_MuxAlt4);/* Configured as PWM FTM0_CH3 */
#endif

	ftm_config_t ftmInfo;
	ftm_chnl_pwm_signal_param_t ftmParam;

	ftmParam.chnlNumber = BOARD_FTM_CHANNEL;
	ftmParam.level = kFTM_HighTrue;
	ftmParam.dutyCyclePercent = 0U;// initially 0%
	ftmParam.firstEdgeDelayPercent = 0U;
	FTM_GetDefaultConfig(&ftmInfo);
	FTM_Init(BOARD_FTM_BASEADDR, &ftmInfo);/* Initialize FTM module */
	FTM_SetupPwm(BOARD_FTM_BASEADDR, &ftmParam, 1U, kFTM_CenterAlignedPwm, 10000U, CLOCK_GetFreq(kCLOCK_BusClk));   /* Configure ftm params with frequency 10kHZ */
	FTM_StartTimer(BOARD_FTM_BASEADDR, kFTM_SystemClock);

#else // ! DISPLAY_LED_PWM

	PORT_SetPinMux(Port_Display_Light, Pin_Display_Light, kPORT_MuxAsGpio);
	GPIO_PinInit(GPIO_Display_Light, Pin_Display_Light, &pin_config_output);
	GPIO_PinWrite(GPIO_Display_Light, Pin_Display_Light, 0);

#endif // DISPLAY_LED_PWM

	GPIO_PinInit(GPIO_Display_CS, Pin_Display_CS, &pin_config_output);
	GPIO_PinInit(GPIO_Display_RST, Pin_Display_RST, &pin_config_output);
	GPIO_PinInit(GPIO_Display_RS, Pin_Display_RS, &pin_config_output);
	GPIO_PinInit(GPIO_Display_SCK, Pin_Display_SCK, &pin_config_output);
	GPIO_PinInit(GPIO_Display_SDA, Pin_Display_SDA, &pin_config_output);

	// Init pins
	GPIO_PinWrite(GPIO_Display_CS, Pin_Display_CS, 1);
	GPIO_PinWrite(GPIO_Display_RST, Pin_Display_RST, 1);
	GPIO_PinWrite(GPIO_Display_RS, Pin_Display_RS, 1);
	GPIO_PinWrite(GPIO_Display_SCK, Pin_Display_SCK, 1);
	GPIO_PinWrite(GPIO_Display_SDA, Pin_Display_SDA, 1);

	// Reset LCD
	GPIO_PinWrite(GPIO_Display_RST, Pin_Display_RST, 0);
	vTaskDelay(portTICK_PERIOD_MS * 1);
	GPIO_PinWrite(GPIO_Display_RST, Pin_Display_RST, 1);
	vTaskDelay(portTICK_PERIOD_MS * 5);

	ucBegin(isInverseColour);
#endif // ! PLATFORM_GD77S
}

void displayEnableBacklight(bool enable)
{
#if ! defined(PLATFORM_GD77S)
	if (enable)
	{
#ifdef DISPLAY_LED_PWM
		displaySetBacklightIntensityPercentage(nonVolatileSettings.displayBacklightPercentage);
#else
		GPIO_PinWrite(GPIO_Display_Light, Pin_Display_Light, 1);
#endif
	}
	else
	{
#ifdef DISPLAY_LED_PWM

		displaySetBacklightIntensityPercentage(((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_NONE) ? 0 : nonVolatileSettings.displayBacklightPercentageOff));
#else
		GPIO_PinWrite(GPIO_Display_Light, Pin_Display_Light, 0);
#endif
	}
#endif // ! PLATFORM_GD77S
}

bool displayIsBacklightLit(void)
{
#if defined(PLATFORM_GD77S)
	return false;
#else
#ifdef DISPLAY_LED_PWM
	uint32_t cnv = BOARD_FTM_BASEADDR->CONTROLS[BOARD_FTM_CHANNEL].CnV;
	uint32_t mod = BOARD_FTM_BASEADDR->MOD;
	uint32_t dutyCyclePercent = 0;

	// Calculate dutyCyclePercent value
	if (cnv == (mod + 1))
	{
		dutyCyclePercent = 100;
	}
	else
	{
		dutyCyclePercent = (uint32_t)((((float)cnv / (float)mod) * 100.0) + 0.5);
	}
	return (dutyCyclePercent != nonVolatileSettings.displayBacklightPercentageOff);
#else
	return (GPIO_PinRead(GPIO_Display_Light, Pin_Display_Light) == 1);
#endif
#endif // PLATFORM_GD77S
}

void displaySetBacklightIntensityPercentage(uint8_t intensityPercentage)
{
#if ! defined(PLATFORM_GD77S)
#ifdef DISPLAY_LED_PWM
	static uint8_t prevPercentage = 255;

	if (prevPercentage != intensityPercentage)
	{
		prevPercentage = intensityPercentage;
		FTM_UpdateChnlEdgeLevelSelect(BOARD_FTM_BASEADDR, BOARD_FTM_CHANNEL, kFTM_NoPwmSignal); // Disable channel output before updating the dutycycle
		FTM_UpdatePwmDutycycle(BOARD_FTM_BASEADDR, BOARD_FTM_CHANNEL, kFTM_CenterAlignedPwm, intensityPercentage); // Update PWM duty cycle
		FTM_SetSoftwareTrigger(BOARD_FTM_BASEADDR, true); // Software trigger to update registers
		FTM_UpdateChnlEdgeLevelSelect(BOARD_FTM_BASEADDR, BOARD_FTM_CHANNEL, kFTM_HighTrue);    // Start channel output with updated dutycycle
	}
#endif
#endif // ! PLATFORM_GD77S
}
