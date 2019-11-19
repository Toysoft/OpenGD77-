/*
 * Initial work for port to MK22FN512xxx12 Copyright (C)2019 Kai Ludwig, DG4KLU
 *
 * Code mainly re-written by Roger Clark. VK3KYY / G4KYF
 * based on information and code references from various sources, including
 * https://github.com/bitbank2/uc1701 and
 * https://os.mbed.com/users/Anaesthetix/code/UC1701/file/7494bdca926b/
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

#ifndef __UC1701_H__
#define __UC1701_H__

#include "FreeRTOS.h"
#include "task.h"
#include <math.h>
#include "fw_common.h"

typedef enum
{
	UC1701_FONT_6x8 = 0,
	UC1701_FONT_6x8_BOLD,
	UC1701_FONT_8x8,
	UC1701_FONT_8x16,
	UC1701_FONT_16x32
} UC1701_Font_t;

typedef enum
{
	UC1701_TEXT_ALIGN_LEFT = 0,
	UC1701_TEXT_ALIGN_CENTER,
	UC1701_TEXT_ALIGN_RIGHT
} UC1701_Text_Align_t;

void UC1701_begin(bool isInverted);
void UC1701_clearBuf();
void UC1701_render();
void UC1701RenderRows(int16_t startRow, int16_t endRow);
void UC1701_printCentered(uint8_t y, char *text, UC1701_Font_t fontSize);
void UC1701_printAt(uint8_t x, uint8_t y, char *text, UC1701_Font_t fontSize);
int UC1701_printCore(int16_t x, int16_t y, char *szMsg, UC1701_Font_t fontSize, UC1701_Text_Align_t alignment, bool isInverted);

int16_t UC1701_setPixel(int16_t x, int16_t y, bool color);

void UC1701_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color);
void UC1701_drawFastVLine(int16_t x, int16_t y, int16_t h, bool color);
void UC1701_drawFastHLine(int16_t x, int16_t y, int16_t w, bool color);

void UC1701_drawCircle(int16_t x0, int16_t y0, int16_t r, bool color);
void UC1701_fillCircle(int16_t x0, int16_t y0, int16_t r, bool color);

void UC1701_drawellipse(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color);

void UC1701_drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool color);
void UC1701_fillTriangle ( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool color);

void UC1701_fillArc(uint16_t x, uint16_t y, uint16_t radius, uint16_t thickness, float start, float end, bool color);

void UC1701_drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool color);
void UC1701_fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool color);

void UC1701_drawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool color);
void UC1701_fillRect(int16_t x, int16_t y, int16_t width, int16_t height, bool isInverted);

void UC1701_drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, bool color);
void UC1701_drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, bool color);


void UC1701_setContrast(uint8_t contrast);
void UC1701_setInverseVideo(bool isInverted);

#endif /* __UC1701_H__ */
