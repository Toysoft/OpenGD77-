/*
 * Copyright (C)2020	Kai Ludwig, DG4KLU
 *               and	Roger Clark, VK3KYY / G4KYF
 *               and	Daniel Caujolle-Bert, F1RMB
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

#include <EPL003.h>
#include <settings.h>


#if defined(PLATFORM_GD77S)
extern volatile uint32_t PITCounter;

void EPL003_init(void)
{
	PORT_SetPinMux(Port_SpeechSynth_CLK, Pin_SpeechSynth_CLK, kPORT_MuxAsGpio);
	GPIO_PinInit(GPIO_SpeechSynth_CLK, Pin_SpeechSynth_CLK, &pin_config_output);

	PORT_SetPinMux(Port_SpeechSynth_DAT, Pin_SpeechSynth_DAT, kPORT_MuxAsGpio);
	GPIO_PinInit(GPIO_SpeechSynth_DAT, Pin_SpeechSynth_DAT, &pin_config_output);

	GPIO_SpeechSynth_CLK->PCOR = 1U << Pin_SpeechSynth_CLK;
	GPIO_SpeechSynth_DAT->PCOR = 1U << Pin_SpeechSynth_DAT;
}

void EPL003_speak(EPL003_Sequence_t seq)
{
	uint32_t m;

	seq += 128; // English words are starting at offset 128, otherwise it's the Chinese version

	// Preamble
	GPIO_SpeechSynth_CLK->PSOR = 1U << Pin_SpeechSynth_CLK;
	m = PITCounter + (170); // 17ms
	while (PITCounter < m) {}

	for (uint8_t bit = 0; bit < 8; bit++)
	{
		GPIO_SpeechSynth_CLK->PCOR = 1U << Pin_SpeechSynth_CLK;

		if ((seq & 0x80) == 0U)
		{
			GPIO_SpeechSynth_DAT->PCOR = 1U << Pin_SpeechSynth_DAT;
		}
		else
		{
			GPIO_SpeechSynth_DAT->PSOR = 1U << Pin_SpeechSynth_DAT;
		}
		m = PITCounter + 8; // 0.8 ms
		while (PITCounter < m) {}

		GPIO_SpeechSynth_CLK->PSOR = 1U << Pin_SpeechSynth_CLK;
		m = PITCounter + 8; // 0.8 ms
		while (PITCounter < m) {}

		seq <<= 1;
	}

	GPIO_SpeechSynth_CLK->PCOR = 1U << Pin_SpeechSynth_CLK;
	GPIO_SpeechSynth_DAT->PCOR = 1U << Pin_SpeechSynth_DAT;
}
#endif
