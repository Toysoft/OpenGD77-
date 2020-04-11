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

#include <common.h>

gpio_pin_config_t pin_config_input =
{
	kGPIO_DigitalInput,
	0
};

gpio_pin_config_t pin_config_output =
{
	kGPIO_DigitalOutput,
	0
};

void fw_init_common(void)
{
    CLOCK_EnableClock(kCLOCK_PortA);
    CLOCK_EnableClock(kCLOCK_PortB);
    CLOCK_EnableClock(kCLOCK_PortC);
    CLOCK_EnableClock(kCLOCK_PortD);
    CLOCK_EnableClock(kCLOCK_PortE);

#if !defined(PLATFORM_RD5R)
    // Power On/Off logic
    PORT_SetPinMux(Port_Keep_Power_On, Pin_Keep_Power_On, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_Power_Switch, Pin_Power_Switch, kPORT_MuxAsGpio);

    // Power On/Off logic
    GPIO_PinInit(GPIO_Keep_Power_On, Pin_Keep_Power_On, &pin_config_output);
    GPIO_PinInit(GPIO_Power_Switch, Pin_Power_Switch, &pin_config_input);

    // Power On/Off logic
	GPIO_PinWrite(GPIO_Keep_Power_On, Pin_Keep_Power_On, 1);
#endif


    // Speaker mute and RX/TX mux init
    PORT_SetPinMux(Port_audio_amp_enable, Pin_audio_amp_enable, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_RX_audio_mux, Pin_RX_audio_mux, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_TX_audio_mux, Pin_TX_audio_mux, kPORT_MuxAsGpio);
    GPIO_PinInit(GPIO_audio_amp_enable, Pin_audio_amp_enable, &pin_config_output);
    GPIO_PinInit(GPIO_RX_audio_mux, Pin_RX_audio_mux, &pin_config_output);
    GPIO_PinInit(GPIO_TX_audio_mux, Pin_TX_audio_mux, &pin_config_output);
    GPIO_PinWrite(GPIO_audio_amp_enable, Pin_audio_amp_enable, 0);
    GPIO_PinWrite(GPIO_RX_audio_mux, Pin_RX_audio_mux, 0);
    GPIO_PinWrite(GPIO_TX_audio_mux, Pin_TX_audio_mux, 0);

    // UHF/VHF RX/TX amp init
    PORT_SetPinMux(Port_VHF_RX_amp_power, Pin_VHF_RX_amp_power, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_UHF_RX_amp_power, Pin_UHF_RX_amp_power, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_UHF_TX_amp_power, Pin_UHF_TX_amp_power, kPORT_MuxAsGpio);
    PORT_SetPinMux(Port_VHF_TX_amp_power, Pin_VHF_TX_amp_power, kPORT_MuxAsGpio);

    GPIO_PinInit(GPIO_VHF_RX_amp_power, Pin_VHF_RX_amp_power, &pin_config_output);
    GPIO_PinInit(GPIO_UHF_RX_amp_power, Pin_UHF_RX_amp_power, &pin_config_output);

    GPIO_PinInit(GPIO_UHF_TX_amp_power, Pin_UHF_TX_amp_power, &pin_config_output);
    GPIO_PinInit(GPIO_VHF_TX_amp_power, Pin_VHF_TX_amp_power, &pin_config_output);

    GPIO_PinWrite(GPIO_VHF_RX_amp_power, Pin_VHF_RX_amp_power, 0);
    GPIO_PinWrite(GPIO_UHF_RX_amp_power, Pin_UHF_RX_amp_power, 0);

    GPIO_PinWrite(GPIO_UHF_TX_amp_power, Pin_UHF_TX_amp_power, 0);
    GPIO_PinWrite(GPIO_VHF_TX_amp_power, Pin_VHF_TX_amp_power, 0);
}
