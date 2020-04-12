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

#include <codeplug.h>
#include <main.h>
#include <settings.h>
#include <ticks.h>
#include <user_interface/menuSystem.h>
#include <user_interface/uiUtilities.h>
#include <user_interface/uiLocalisation.h>


#if defined(USE_SEGGER_RTT)
#include <SeggerRTT/RTT/SEGGER_RTT.h>
#endif

#ifdef NDEBUG
#error A firmware compiled in Releae mode will not work, yet
#error Change target build to Debug then Clean the build and recompile
#endif

bool PTTToggledDown = false; // PTT toggle feature

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

void fw_powerOffFinalStage(void)
{
	uint32_t m;

	// If user was in a private call when they turned the radio off we need to restore the last Tg prior to stating the Private call.
	// to the nonVolatile Setting overrideTG, otherwise when the radio is turned on again it be in PC mode to that station.
	if ((trxTalkGroupOrPcId>>24) == PC_CALL_FLAG)
	{
		nonVolatileSettings.overrideTG = menuUtilityTgBeforePcMode;
	}

	menuHotspotRestoreSettings();

	m = fw_millis();
	settingsSaveSettings(true);

	// Give it a bit of time before pulling the plug as DM-1801 EEPROM looks slower
	// than GD-77 to write, then quickly power cycling triggers settings reset.
	while (1U)
	{
		if ((fw_millis() - m) > 50)
		{
			break;
		}
	}

#if !defined(PLATFORM_RD5R)
	// This turns the power off to the CPU.
	GPIO_PinWrite(GPIO_Keep_Power_On, Pin_Keep_Power_On, 0);

	// Death trap
	while(1U) {}
#endif
}


static void show_lowbattery(void)
{
	ucClearBuf();
	ucPrintCentered(32, currentLanguage->low_battery, FONT_SIZE_3);
	ucRender();
}

static void showErrorMessage(char *message)
{
	ucClearBuf();
	ucPrintCentered(32, message, FONT_SIZE_3);
	ucRender();
}

void fw_main_task(void *data)
{
	keyboardCode_t keys;
	int key_event;
	int keyFunction;
	uint32_t buttons;
	int button_event;
	uint32_t rotary;
	int rotary_event;
	int function_event;
	uiEvent_t ev = { .buttons = 0, .keys = NO_KEYCODE, .rotary = 0, .function = 0, .events = NO_EVENT, .hasEvent = false, .time = 0 };
	bool keyOrButtonChanged = false;

    USB_DeviceApplicationInit();

    // Init I2C
    init_I2C0a();
    setup_I2C0();
	fw_init_common();
	fw_init_buttons();
	fw_init_LEDs();
	fw_init_keyboard();
	init_rotary_switch();

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

    if (!checkAndCopyCalibrationToCommonLocation())
	{
		showErrorMessage("CAL DATA ERROR");
		while(1U)
		{
			tick_com_request();
		};
	}

    // Init AT1846S
    I2C_AT1846S_init();

    // Init HR-C6000
    SPI_HR_C6000_init();

    // Additional init stuff
    SPI_C6000_postinit();
    I2C_AT1846_Postinit();

    // Init HR-C6000 interrupts
    init_HR_C6000_interrupts();

    // Speech Synthesis (GD77S Only)
    speechSynthesisInit();


    // Small startup delay after initialization to stabilize system
  //  vTaskDelay(portTICK_PERIOD_MS * 500);

	init_pit();

	trx_measure_count = 0;

	if (get_battery_voltage()<CUTOFF_VOLTAGE_UPPER_HYST)
	{
		show_lowbattery();
#if !defined(PLATFORM_RD5R)
		GPIO_PinWrite(GPIO_Keep_Power_On, Pin_Keep_Power_On, 0);
#endif
		while(1U) {};
	}

	init_hrc6000_task();

	menuBatteryInit(); // Initialize circular buffer
	init_watchdog(menuBatteryPushBackVoltage);

    fw_init_beep_task();

#if defined(USE_SEGGER_RTT)
    SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
    SEGGER_RTT_printf(0,"Segger RTT initialised\n");
#endif

    lastheardInitList();
    codeplugInitContactsCache();
    dmrIDCacheInit();
    menuInitMenuSystem();

#if defined(PLATFORM_GD77S)
    // Change hotspot modem type setting
	if (buttons & BUTTON_SK1)
	{
		nonVolatileSettings.hotspotType = (buttons & BUTTON_PTT) ? HOTSPOT_TYPE_BLUEDV : HOTSPOT_TYPE_MMDVM;
	}
#endif

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

			check_rotary_switch_event(&rotary, &rotary_event); // Rotary switch state and event (GD-77S only)

			// EVENT_*_CHANGED can be cleared later, so check this now as hasEvent has to be set anyway.
			keyOrButtonChanged = ((key_event != NO_EVENT) || (button_event != NO_EVENT) || (rotary_event != NO_EVENT));

			if (batteryVoltageHasChanged == true)
			{
				int currentMenu = menuSystemGetCurrentMenuNumber();

				if ((currentMenu == UI_CHANNEL_MODE) || (currentMenu == UI_VFO_MODE))
				{
#if defined(PLATFORM_RD5R)
					ucFillRect(0, 0, 128, 8, true);
#else
					ucClearRows(0, 2, false);
#endif
					menuUtilityRenderHeader();
					ucRenderRows(0, 2);
				}

				batteryVoltageHasChanged = false;
			}

			if (keypadLocked || PTTLocked)
			{
				if (keypadLocked && (buttons & BUTTON_PTT) == 0)
				{
					if (key_event == EVENT_KEY_CHANGE)
					{
						if ((PTTToggledDown == false) && (menuSystemGetCurrentMenuNumber() != UI_LOCK_SCREEN))
						{
							menuSystemPushNewMenu(UI_LOCK_SCREEN);
						}

						key_event = EVENT_KEY_NONE;

						if (nonVolatileSettings.pttToggle && PTTToggledDown)
						{
							PTTToggledDown = false;
						}
					}

					// Lockout ORANGE AND BLUE (BLACK stay active regardless lock status, useful to trigger backlight)
#if defined(PLATFORM_RD5R)
					if (button_event == EVENT_BUTTON_CHANGE && (buttons & BUTTON_SK2))
#else
					if (button_event == EVENT_BUTTON_CHANGE && ((buttons & BUTTON_ORANGE) || (buttons & BUTTON_SK2)))
#endif
					{
						if ((PTTToggledDown == false) && (menuSystemGetCurrentMenuNumber() != UI_LOCK_SCREEN))
						{
							menuSystemPushNewMenu(UI_LOCK_SCREEN);
						}

						button_event = EVENT_BUTTON_NONE;

						if (nonVolatileSettings.pttToggle && PTTToggledDown)
						{
							PTTToggledDown = false;
						}
					}
				}
				else if (PTTLocked)
				{
					if ((buttons & BUTTON_PTT) && (button_event == EVENT_BUTTON_CHANGE))
					{
						set_melody(melody_ERROR_beep);

						if (menuSystemGetCurrentMenuNumber() != UI_LOCK_SCREEN)
						{
							menuSystemPushNewMenu(UI_LOCK_SCREEN);
						}

						button_event = EVENT_BUTTON_NONE;
						// Clear PTT button
						buttons &= ~BUTTON_PTT;
					}
					else if ((buttons & BUTTON_SK2) && KEYCHECK_DOWN(keys, KEY_STAR))
					{
						if (menuSystemGetCurrentMenuNumber() != UI_LOCK_SCREEN)
						{
							menuSystemPushNewMenu(UI_LOCK_SCREEN);
						}
					}
				}
			}

#if ! defined(PLATFORM_GD77S)
		if ((key_event == EVENT_KEY_CHANGE) && ((buttons & BUTTON_PTT) == 0) && (keys.key != 0))
			{
				// Do not send any beep while scanning, otherwise enabling the AMP will be handled as a valid signal detection.
				if (keys.event & KEY_MOD_PRESS)
				{
					if ((PTTToggledDown == false) && (((menuSystemGetCurrentMenuNumber() == UI_VFO_MODE) && uiVFOModeIsScanning()) == false))
						set_melody(melody_key_beep);
				}
				else if ((keys.event & (KEY_MOD_LONG | KEY_MOD_DOWN)) == (KEY_MOD_LONG | KEY_MOD_DOWN))
				{
					if ((PTTToggledDown == false) && (((menuSystemGetCurrentMenuNumber() == UI_VFO_MODE) && uiVFOModeIsScanning()) == false))
						set_melody(melody_key_long_beep);
				}

				if (KEYCHECK_LONGDOWN(keys, KEY_RED) &&
						(((menuSystemGetCurrentMenuNumber() == UI_VFO_MODE) && uiVFOModeIsScanning()) == false))
				{
					contactListContactIndex = 0;
					menuSystemPopAllAndDisplayRootMenu();
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

			//
			// PTT toggle feature
			//
			// PTT is locked down, but any button but SK1 is pressed, virtually release PTT
#if defined(PLATFORM_RD5R)
			if ((nonVolatileSettings.pttToggle && PTTToggledDown) &&
							(((buttons & BUTTON_SK2)) ||
									((keys.key != 0) && (keys.event & KEY_MOD_UP))))
#else
			if ((nonVolatileSettings.pttToggle && PTTToggledDown) &&
					(((button_event & EVENT_BUTTON_CHANGE) && ((buttons & BUTTON_ORANGE) || (buttons & BUTTON_SK2))) ||
							((keys.key != 0) && (keys.event & KEY_MOD_UP))))
#endif
			{
				PTTToggledDown = false;
				button_event = EVENT_BUTTON_CHANGE;
				buttons = BUTTON_NONE;
				key_event = NO_EVENT;
				keys.key = 0;
			}
			// PTT toggle action
			if (nonVolatileSettings.pttToggle)
			{
				if (button_event == EVENT_BUTTON_CHANGE)
				{
					if (buttons & BUTTON_PTT)
					{
						if (PTTToggledDown == false)
						{
							// PTT toggle works only if a TOT value is defined.
							if (currentChannelData->tot != 0)
							{
								PTTToggledDown = true;
							}
						}
						else
						{
							PTTToggledDown = false;
						}
					}
				}

				if (PTTToggledDown && ((buttons & BUTTON_PTT) == 0))
				{
					buttons |= BUTTON_PTT;
				}
			}
			else
			{
				if (PTTToggledDown)
					PTTToggledDown = false;
			}
#endif

			if (button_event == EVENT_BUTTON_CHANGE)
			{
				displayLightTrigger();
				if ((buttons & BUTTON_PTT) != 0)
				{
					int currentMenu = menuSystemGetCurrentMenuNumber();

					/*
					 * This code would prevent transmission on simplex if the radio is receiving a DMR signal.
					 * if ((slot_state == DMR_STATE_IDLE || trxDMRMode == DMR_MODE_PASSIVE)  &&
					 *
					 */
					if (	trxGetMode() != RADIO_MODE_NONE &&
							settingsUsbMode != USB_MODE_HOTSPOT &&
							currentMenu != UI_POWER_OFF &&
							currentMenu != UI_SPLASH_SCREEN &&
							currentMenu != UI_TX_SCREEN )
					{
						bool wasScanning = false;
						if (scanActive)
						{
							if (currentMenu == UI_VFO_MODE)
							{
								uiVFOModeStopScanning();
							}
							else
							{
								uiChannelModeStopScanning();
							}
							wasScanning = true;
						}
						else
						{
							if (currentMenu == UI_LOCK_SCREEN)
							{
								menuLockScreenPop();
							}
						}
						if (!wasScanning)
						{
							menuSystemPushNewMenu(UI_TX_SCREEN);
						}
					}
				}

        		if (buttons & BUTTON_SK1 && buttons & BUTTON_SK2)
        		{
        			settingsSaveSettings(true);
        		}

    			// Toggle backlight
        		if ((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_MANUAL) && (buttons & BUTTON_SK1))
        		{
        			fw_displayEnableBacklight(! fw_displayIsBacklightLit());
        		}
        	}

    		if (!trxIsTransmitting && updateLastHeard==true)
    		{
    			lastHeardListUpdate((uint8_t *)DMR_frame_buffer, false);
    			updateLastHeard=false;
    		}

    		if ((nonVolatileSettings.hotspotType == HOTSPOT_TYPE_OFF) ||
    				((nonVolatileSettings.hotspotType != HOTSPOT_TYPE_OFF) && (settingsUsbMode != USB_MODE_HOTSPOT))) // Do not filter anything in HS mode.
    		{
				if ((uiPrivateCallState == PRIVATE_CALL_DECLINED) &&
						(slot_state == DMR_STATE_IDLE))
				{
					menuClearPrivateCall();
				}
				if (!trxIsTransmitting && menuDisplayQSODataState == QSO_DISPLAY_CALLER_DATA && nonVolatileSettings.privateCalls == true)
    			{
    				if (HRC6000GetReceivedTgOrPcId() == (trxDMRID | (PC_CALL_FLAG<<24)))
    				{
    					if ((uiPrivateCallState == NOT_IN_CALL) &&
    							(trxTalkGroupOrPcId != (HRC6000GetReceivedSrcId() | (PC_CALL_FLAG<<24))) &&
								(HRC6000GetReceivedSrcId() != uiPrivateCallLastID))
    					{
    						if ((HRC6000GetReceivedSrcId() & 0xFFFFFF) >= 1000000)
    						{
    							menuSystemPushNewMenu(UI_PRIVATE_CALL);
    						}
    					}
    				}
    			}
    		}

			ev.function = 0;
			function_event = NO_EVENT;
			if (buttons & BUTTON_SK2)
			{
//				keyFunction = codeplugGetQuickkeyFunctionID(keys.key);
				switch (keys.key)
				{
				case '1':
					keyFunction = START_SCANNING;
					break;
				case '2':
					keyFunction = (MENU_BATTERY << 8);
					break;
				case '3':
					keyFunction = ( MENU_LAST_HEARD << 8);
					break;
				case '4':
					keyFunction = ( MENU_CHANNEL_DETAILS << 8) | 2;
					break;
#if defined(PLATFORM_RD5R)
				case '5':
					keyFunction = TOGGLE_TORCH;
					break;
#endif
				case '7':
					keyFunction = (MENU_DISPLAY <<8) + DEC_BRIGHTNESS;
					break;
				case '8':
					keyFunction = (MENU_DISPLAY <<8) + INC_BRIGHTNESS;
					break;
				default:
					keyFunction = 0;
					break;
				}

				if (keyFunction > 0)
				{
					int menuFunction = (keyFunction >> 8) & 0xff;

					if (menuFunction > 0 && menuFunction < NUM_MENU_ENTRIES)
					{
						int currentMenu = menuSystemGetCurrentMenuNumber();
						if (currentMenu != menuFunction)
						{
							menuSystemPushNewMenu(menuFunction);
						}
					}
					ev.function = keyFunction & 0xff;
					buttons = BUTTON_NONE;
					rotary = 0;
					key_event = EVENT_KEY_NONE;
					button_event = EVENT_BUTTON_NONE;
					rotary_event = EVENT_ROTARY_NONE;
					keys.key = 0;
					keys.event = 0;
					function_event = FUNCTION_EVENT;
					fw_reset_keyboard();
				}
			}
    		ev.buttons = buttons;
    		ev.keys = keys;
    		ev.rotary = rotary;
    		ev.events = function_event | (button_event << 1) | (rotary_event << 3) | key_event;
    		ev.hasEvent = keyOrButtonChanged || function_event;
    		ev.time = fw_millis();

        	menuSystemCallCurrentMenuTick(&ev);

#if defined(PLATFORM_RD5R)
        	if (keyFunction == TOGGLE_TORCH)
        	{
        		toggle_torch();
        	}
#endif

#if defined(PLATFORM_RD5R)
        	if ((battery_voltage < CUTOFF_VOLTAGE_LOWER_HYST)
        			&& (menuSystemGetCurrentMenuNumber() != UI_POWER_OFF))
#else
        	if (((GPIO_PinRead(GPIO_Power_Switch, Pin_Power_Switch) != 0)
        			|| (battery_voltage < CUTOFF_VOLTAGE_LOWER_HYST))
        			&& (menuSystemGetCurrentMenuNumber() != UI_POWER_OFF))
#endif
        	{
        		if (battery_voltage < CUTOFF_VOLTAGE_LOWER_HYST)
        		{
        			show_lowbattery();
#if defined(PLATFORM_RD5R)
        			fw_powerOffFinalStage();
#else
        			if (GPIO_PinRead(GPIO_Power_Switch, Pin_Power_Switch) != 0)
        			{
        				fw_powerOffFinalStage();
        			}
#endif
        		}
        		else
        		{
        			menuSystemPushNewMenu(UI_POWER_OFF);
        		}
        		GPIO_PinWrite(GPIO_audio_amp_enable, Pin_audio_amp_enable, 0);
        		set_melody(NULL);
        	}

    		if ((nonVolatileSettings.backlightMode == BACKLIGHT_MODE_AUTO) && (menuDisplayLightTimer > 0))
    		{
    			menuDisplayLightTimer--;
    			if (menuDisplayLightTimer==0)
    			{
    				fw_displayEnableBacklight(false);
    			}
    		}
    		tick_melody();
    		speechSynthesisTick();
    	}
		vTaskDelay(0);
    }
}
