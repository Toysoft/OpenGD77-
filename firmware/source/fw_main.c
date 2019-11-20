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

#include <user_interface/menuSystem.h>
#include <user_interface/menuUtilityQSOData.h>
#include "fw_main.h"
#include "fw_settings.h"

#if defined(USE_SEGGER_RTT)
#include <SeggerRTT/RTT/SEGGER_RTT.h>
#endif

void fw_main_task(void *data);

const char *FIRMWARE_VERSION_STRING = "VK3KYY";//"V0.3.5";
TaskHandle_t fwMainTaskHandle;

void fw_init(void)
{
	xTaskCreate(fw_main_task,                        /* pointer to the task */
				"fw main task",                      /* task name for kernel awareness debugging */
				5000L / sizeof(portSTACK_TYPE),      /* task stack size */
				NULL,                      			 /* optional task startup argument */
				6U,                                  /* initial priority */
				fwMainTaskHandle					 /* optional task handle to create */
				);

    vTaskStartScheduler();
}

static void show_lowbattery(void)
{
	UC1701_clearBuf();
	UC1701_printCentered(32, "LOW BATTERY !!!", UC1701_FONT_8x16);
	UC1701_render();
}

void fw_main_task(void *data)
{
	uint32_t keys;
	int key_event;
	uint32_t buttons;
	int button_event;
	
    USB_DeviceApplicationInit();

    // Init I2C
    init_I2C0a();
    setup_I2C0();
	fw_init_common();
	fw_init_buttons();
	fw_init_LEDs();
	fw_init_keyboard();

	fw_check_button_event(&buttons, &button_event);// Read button state and event
	if (buttons & BUTTON_SK2)
	{
		settingsRestoreDefaultSettings();
	}
    settingsLoadSettings();

	fw_init_display(nonVolatileSettings.displayInverseVideo);

    // Init SPI
    init_SPI();
    setup_SPI0();
    setup_SPI1();

    // Init I2S
    init_I2S();
    setup_I2S();

    // Init ADC
    adc_init();

    // Init DAC
    dac_init();

    SPI_Flash_init();

    // Init AT1846S
    I2C_AT1846S_init();

    // Init HR-C6000
    SPI_HR_C6000_init();

    // Additional init stuff
    SPI_C6000_postinit();
    I2C_AT1846_Postinit();

    // Init HR-C6000 interrupts
    init_HR_C6000_interrupts();

    // Small startup delay after initialization to stabilize system
    vTaskDelay(portTICK_PERIOD_MS * 500);

	init_pit();

	trx_measure_count = 0;

	if (get_battery_voltage()<CUTOFF_VOLTAGE_UPPER_HYST)
	{
		show_lowbattery();
		GPIO_PinWrite(GPIO_Keep_Power_On, Pin_Keep_Power_On, 0);
		while(1U) {};
	}

	init_hrc6000_task();

	init_watchdog();

    fw_init_beep_task();

    set_melody(melody_poweron);

#if defined(USE_SEGGER_RTT)
    SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
    SEGGER_RTT_printf(0,"Segger RTT initialised\n");
#endif

    lastheardInitList();
    menuInitMenuSystem();

    if (buttons & BUTTON_SK1)
    {
    	enableHotspot = true;
    }

    while (1U)
    {
    	taskENTER_CRITICAL();
    	uint32_t tmp_timer_maintask=timer_maintask;
    	taskEXIT_CRITICAL();
    	if (tmp_timer_maintask==0)
    	{
        	taskENTER_CRITICAL();
    		timer_maintask=10;
    	    alive_maintask=true;
        	taskEXIT_CRITICAL();

			tick_com_request();

			fw_check_button_event(&buttons, &button_event); // Read button state and event
			fw_check_key_event(&keys, &key_event); // Read keyboard state and event

			if (keypadLocked)
			{
				if (key_event == EVENT_KEY_CHANGE)
				{
					key_event = EVENT_KEY_NONE;
					if (menuSystemGetCurrentMenuNumber() != MENU_LOCK_SCREEN)
					{
						menuSystemPushNewMenu(MENU_LOCK_SCREEN);
					}
				}
				if (button_event == EVENT_BUTTON_CHANGE
						&& (buttons & BUTTON_ORANGE) != 0)
				{
					button_event = EVENT_BUTTON_NONE;
					if (menuSystemGetCurrentMenuNumber() != MENU_LOCK_SCREEN)
					{
						menuSystemPushNewMenu(MENU_LOCK_SCREEN);
					}
				}
			}

			if (key_event == EVENT_KEY_CHANGE && (keys & KEY_MOD_PRESS) == 0) {   // Remove after changing key handling in user interface
				key_event = EVENT_KEY_NONE;
			}

			if (keypadLocked)
			{
				if (key_event == EVENT_KEY_CHANGE)
				{
					key_event = EVENT_KEY_NONE;
					keys = 0;
					if (menuSystemGetCurrentMenuNumber() != MENU_LOCK_SCREEN)
					{
						menuSystemPushNewMenu(MENU_LOCK_SCREEN);
					}
				}
				if (button_event == EVENT_BUTTON_CHANGE
						&& (buttons & BUTTON_ORANGE) != 0)
				{
					button_event = EVENT_BUTTON_NONE;
					buttons = 0;
					if (menuSystemGetCurrentMenuNumber() != MENU_LOCK_SCREEN)
					{
						menuSystemPushNewMenu(MENU_LOCK_SCREEN);
					}
				}
			}
			if (key_event == EVENT_KEY_CHANGE && KEYCHECK_PRESS(keys))
			{
				if (keys != 0 && (buttons & BUTTON_PTT) == 0)
				{
					set_melody(melody_key_beep);
				}
			}
/*
 * This code ignores the keypress if the display is not lit, however this functionality is proving to be a problem for things like entering a TG
 * I think it needs to be removed until a better solution can be found
 *
			if (menuDisplayLightTimer == 0
					&& nonVolatileSettings.backLightTimeout != 0)
			{
				if (key_event == EVENT_KEY_CHANGE)
				{
					key_event = EVENT_KEY_NONE;
					keys = 0;
					displayLightTrigger();
				}
				if (button_event == EVENT_BUTTON_CHANGE && (buttons & BUTTON_ORANGE) != 0)
				{
					button_event = EVENT_BUTTON_NONE;
					buttons = 0;
					displayLightTrigger();
				}
			}
*/



			if (button_event == EVENT_BUTTON_CHANGE)
			{
				displayLightTrigger();
        		if ((	(buttons & BUTTON_PTT)!=0) &&
        				(slot_state==DMR_STATE_IDLE || trxDMRMode == DMR_MODE_PASSIVE) &&
						trxGetMode()!=RADIO_MODE_NONE &&
						settingsUsbMode != USB_MODE_HOTSPOT &&
						menuSystemGetCurrentMenuNumber() != MENU_POWER_OFF &&
						menuSystemGetCurrentMenuNumber() != MENU_SPLASH_SCREEN &&
						menuSystemGetCurrentMenuNumber() != MENU_TX_SCREEN )
        		{
        			menuSystemPushNewMenu(MENU_TX_SCREEN);
        		}
        	}

    		if (!trxIsTransmitting && updateLastHeard==true)
    		{
    			lastHeardListUpdate((uint8_t *)DMR_frame_buffer);
    			updateLastHeard=false;
    		}

        	menuSystemCallCurrentMenuTick(buttons,keys,(button_event<<1) | key_event);

        	if (((GPIO_PinRead(GPIO_Power_Switch, Pin_Power_Switch)!=0)
        			|| (battery_voltage<CUTOFF_VOLTAGE_LOWER_HYST))
        			&& (menuSystemGetCurrentMenuNumber() != MENU_POWER_OFF))
        	{
        		// If user was in a private call when they turned the radio off we need to restore the last Tg prior to stating the Private call.
        		// to the nonVolatile Setting overrideTG, otherwise when the radio is turned on again it be in PC mode to that station.
        		if (menuUtilityTgBeforePcMode!=0)
        		{
        			nonVolatileSettings.overrideTG = menuUtilityTgBeforePcMode;
        		}
				settingsSaveSettings();

        		if (battery_voltage<CUTOFF_VOLTAGE_LOWER_HYST)
        		{
        			show_lowbattery();

                	if (GPIO_PinRead(GPIO_Power_Switch, Pin_Power_Switch)!=0)
					{
        				// This turns the power off to the CPU.
        				GPIO_PinWrite(GPIO_Keep_Power_On, Pin_Keep_Power_On, 0);
        			}
        		}
        		else
        		{
					menuSystemPushNewMenu(MENU_POWER_OFF);
        		}
    		    GPIO_PinWrite(GPIO_audio_amp_enable, Pin_audio_amp_enable, 0);
    		    set_melody(NULL);
        	}

    		if (menuDisplayLightTimer > 0)
    		{
    			menuDisplayLightTimer--;
    			if (menuDisplayLightTimer==0)
    			{
    				fw_displayEnableBacklight(false);
    			}
    		}
    		tick_melody();
    	}
		vTaskDelay(0);
    }
}
