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

#ifndef _SPEECH_SYNTHESIS_H_
#define _SPEECH_SYNTHESIS_H_

#include <common.h>
#include <EPL003.h>

#define SPEECH_SYNTHESIS_BUFFER_SIZE 32U

void speechSynthesisInit(void);
bool speechSynthesisIsSpeaking(void);
void speechSynthesisSpeak(uint8_t *sentence);
void speechSynthesisTick(void);

uint8_t speechSynthesisBuildFromNumberInString(uint8_t *dest, uint8_t destSize, const char *str, bool enumerate);
uint8_t speechSynthesisBuildNumerical(uint8_t *dest, uint8_t destSize, float value, uint8_t numberOfDecimals, bool enumerate);


#endif /* _SPEECH_SYNTHESIS_H_ */
