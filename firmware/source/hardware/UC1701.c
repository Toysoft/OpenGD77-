/*
 * Copyright (C)2019 Roger Clark, VK3KYY / G4KYF
 *
 * Development informed by work from  Rustem Iskuzhin (in 2014)
 * and https://github.com/bitbank2/uc1701/
 * and https://os.mbed.com/users/Anaesthetix/code/UC1701/file/7494bdca926b/
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

#include <hardware/UC1701.h>
#include <hardware/UC1701_charset.h>
#include "fw_display.h"
#include "fw_settings.h"

// number representing the maximum angle (e.g. if 100, then if you pass in start=0 and end=50, you get a half circle)
// this can be changed with setArcParams function at runtime
#define DEFAULT_ARC_ANGLE_MAX 360
// rotational offset in degrees defining position of value 0 (-90 will put it at the top of circle)
// this can be changed with setAngleOffset function at runtime
#define DEFAULT_ANGLE_OFFSET -90
static float _arcAngleMax = DEFAULT_ARC_ANGLE_MAX;
static float _angleOffset = DEFAULT_ANGLE_OFFSET;
#define DEG_TO_RAD  0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define swap(x, y) do { typeof(x) t = x; x = y; y = t; } while(0)


static uint8_t screenBuf[1024];
//#define DISPLAY_CHECK_BOUNDS

#ifdef DISPLAY_CHECK_BOUNDS
static const uint8_t *screenBufEnd = screenBuf + sizeof(screenBuf);
#endif
int activeBufNum=0;

void UC1701_setCommandMode(bool isCommand)
{
	GPIO_PinWrite(GPIO_Display_RS, Pin_Display_RS, !isCommand);
}

void UC1701_transfer(register uint8_t data1)
{
	for (register int i=0; i<8; i++)
	{
		GPIO_Display_SCK->PCOR = 1U << Pin_Display_SCK;

		if ((data1&0x80) == 0U)
		{
			GPIO_Display_SDA->PCOR = 1U << Pin_Display_SDA;// Hopefully the compiler will otimise this to a value rather than using a shift
		}
		else
		{
			GPIO_Display_SDA->PSOR = 1U << Pin_Display_SDA;// Hopefully the compiler will otimise this to a value rather than using a shift
		}
		GPIO_Display_SCK->PSOR = 1U << Pin_Display_SCK;// Hopefully the compiler will otimise this to a value rather than using a shift

		data1=data1<<1;
	}
}

int16_t UC1701_setPixel(int16_t x, int16_t y, bool color)
{
	int16_t i;

	i = ((y >> 3) << 7) + x;
	if (i < 0 || i > 1023)
	{
		return -1;// off the screen
	}

	if (color)
	{
		screenBuf[i] |= (0x1 << (y & 7));
	}
	else
	{
		screenBuf[i] &= ~(0x1 << (y & 7));
	}
	return 0;
}

void UC1701RenderRows(int16_t startRow, int16_t endRow)
{
	uint8_t *rowPos = (screenBuf + startRow*128);
	taskENTER_CRITICAL();
	for(int16_t row=startRow;row<endRow;row++)
	{
		UC1701_setCommandMode(true);
		UC1701_transfer(0xb0 | row); // set Y
		UC1701_transfer(0x10 | 0); // set X (high MSB)

// Note there are 4 pixels at the left which are no in the hardware of the LCD panel, but are in the RAM buffer of the controller
		UC1701_transfer(0x00 | 4); // set X (low MSB).

		UC1701_setCommandMode(false);
		uint8_t data1;
		for(int16_t line=0;line<128;line++)
		{
			//UC1701_transfer(*rowPos++);
			data1= *rowPos;
			for (register int i=0; i<8; i++)
			{
				//__asm volatile( "nop" );
				GPIO_Display_SCK->PCOR = 1U << Pin_Display_SCK;
				//__asm volatile( "nop" );
				if ((data1&0x80) == 0U)
				{
					GPIO_Display_SDA->PCOR = 1U << Pin_Display_SDA;// Hopefully the compiler will optimise this to a value rather than using a shift
				}
				else
				{
					GPIO_Display_SDA->PSOR = 1U << Pin_Display_SDA;// Hopefully the compiler will optimise this to a value rather than using a shift
				}
				//__asm volatile( "nop" );
				GPIO_Display_SCK->PSOR = 1U << Pin_Display_SCK;// Hopefully the compiler will optimise this to a value rather than using a shift

				data1=data1<<1;
			}
			rowPos++;
		}
	}
	taskEXIT_CRITICAL();
}

void UC1701_render()
{
	UC1701RenderRows(0,8);
}

//#define DISPLAY_CHECK_BOUNDS
#ifdef DISPLAY_CHECK_BOUNDS
static inline bool checkWritePos(uint8_t * writePos)
{
	if (writePos < screenBuf || writePos > screenBufEnd)
	{
		SEGGER_RTT_printf(0,"Display buffer error\n");
		return false;
	}
	return true;
}
#endif

int UC1701_printCore(int16_t x, int16_t y, char *szMsg, int16_t iSize, int16_t alignment, bool isInverted)
{
	int16_t i, sLen;
	uint8_t *currentCharData;
	int16_t charWidthPixels;
	int16_t charHeightPixels;
	int16_t bytesPerChar;
	int16_t startCode;
	uint8_t *currentFont;
	uint8_t *writePos;
	uint8_t *readPos;

    sLen = strlen(szMsg);

    switch(iSize)
    {
    	case UC1701_FONT_6X8:
    		currentFont = (uint8_t *) font_6x8;
    		break;
    	case UC1701_FONT_6X8_bold:
			currentFont = (uint8_t *) font_6x8_bold;
    		break;
    	case UC1701_FONT_8X8:
    		currentFont = (uint8_t *) font_8x8;
    		break;
    	case UC1701_FONT_GD77_8x16:
    		currentFont = (uint8_t *) font_gd77_8x16;
			break;
    	case UC1701_FONT_16x32:
    		currentFont = (uint8_t *) font_16x32;
			break;

    	default:
    		return -2;// Invalid font selected
    		break;
    }

    startCode   		= currentFont[2];  // get first defined character
    charWidthPixels   	= currentFont[4];  // width in pixel of one char
    charHeightPixels  	= currentFont[5];  // page count per char
    bytesPerChar 		= currentFont[7];  // bytes per char

    if ((charWidthPixels*sLen) + x > 128)
	{
    	sLen = (128-x)/charWidthPixels;
	}

	if (sLen < 0)
	{
		return -1;
	}

	switch(alignment)
	{
		case 0:
			// left aligned, do nothing.
			break;
		case 1:// Align centre
			x = (128 - (charWidthPixels * sLen))/2;
			break;
		case 2:// align right
			x = 128 - (charWidthPixels * sLen);
			break;
	}

	for (i=0; i<sLen; i++)
	{
		currentCharData = (uint8_t *)&currentFont[8 + ((szMsg[i] - startCode) * bytesPerChar)];

		for(int16_t row=0;row < charHeightPixels / 8 ;row++)
		{
			readPos = (currentCharData + row*charWidthPixels);
			writePos = (screenBuf + x + (i*charWidthPixels) + ((y>>3) + row)*128) ;

			if ((y&0x07)==0)
			{
				// y position is aligned to a row
				for(int16_t p=0;p<charWidthPixels;p++)
				{
					if (isInverted)
					{

						#ifdef DISPLAY_CHECK_BOUNDS
							checkWritePos(writePos);
						#endif
						*writePos++ &= ~(*readPos++);
					}
					else
					{
						#ifdef DISPLAY_CHECK_BOUNDS
							checkWritePos(writePos);
						#endif
						*writePos++ |= *readPos++;
					}
				}
			}
			else
			{
				int16_t shiftNum = y & 0x07;
				// y position is NOT aligned to a row

				for(int16_t p=0;p<charWidthPixels;p++)
				{
					if (isInverted)
					{
						#ifdef DISPLAY_CHECK_BOUNDS
							checkWritePos(writePos);
						#endif
						*writePos++ &= ~((*readPos++) << shiftNum);
					}
					else
					{
						#ifdef DISPLAY_CHECK_BOUNDS
							checkWritePos(writePos);
						#endif
						*writePos++ |= ((*readPos++) << shiftNum);
					}
				}

				readPos = (currentCharData + row*charWidthPixels);
				writePos = (screenBuf + x + (i*charWidthPixels) + ((y>>3) + row + 1)*128) ;

				for(int16_t p=0;p<charWidthPixels;p++)
				{
					if (isInverted)
					{
						#ifdef DISPLAY_CHECK_BOUNDS
							checkWritePos(writePos);
						#endif
						*writePos++ &= ~((*readPos++) >> (8 - shiftNum));
					}
					else
					{
						#ifdef DISPLAY_CHECK_BOUNDS
							checkWritePos(writePos);
						#endif
						*writePos++ |= ((*readPos++) >> (8 - shiftNum));
					}
				}
			}
		}
	}
	return 0;
}

void UC1701_setInverseVideo(bool isInverted)
{
	UC1701_setCommandMode(true);
	if (isInverted)
	{
		UC1701_transfer(0xA7); // Black background, white pixels
	}
	else
	{
		UC1701_transfer(0xA4); // White background, black pixels
	}

    UC1701_transfer(0xAF); // Set Display Enable
    UC1701_setCommandMode(false);
}

void UC1701_begin(bool isInverted)
{
	GPIO_PinWrite(GPIO_Display_CS, Pin_Display_CS, 0);// Enable CS permanently
    // Set the LCD parameters...
	UC1701_setCommandMode(true);
	UC1701_transfer(0xE2); // System Reset
	UC1701_transfer(0x2F); // Voltage Follower On
	UC1701_transfer(0x81); // Set Electronic Volume = 15
	UC1701_transfer(nonVolatileSettings.displayContrast); //
	UC1701_transfer(0xA2); // Set Bias = 1/9
	UC1701_transfer(0xA1); // A0 Set SEG Direction
	UC1701_transfer(0xC0); // Set COM Direction
	if (isInverted)
	{
		UC1701_transfer(0xA7); // Black background, white pixels
	}
	else
	{
		UC1701_transfer(0xA4); // White background, black pixels
	}

    UC1701_setCommandMode(true);
    UC1701_transfer(0xAF); // Set Display Enable
    UC1701_clearBuf();
    UC1701_render();
}

void UC1701_setContrast(uint8_t contrast)
{
	UC1701_setCommandMode(true);
	UC1701_transfer(0x81);              // command to set contrast
	UC1701_transfer(contrast);          // set contrast
	UC1701_setCommandMode(false);
}

void UC1701_clearBuf()
{
	memset(screenBuf,0x00,1024);
}

void UC1701_printCentered(uint8_t y, char *text, int16_t fontSize)
{
	UC1701_printCore(0, y, text, fontSize, 1, false);
}

void UC1701_printAt(uint8_t x, uint8_t y, char *text, int16_t fontSize)
{
	UC1701_printCore(x, y, text, fontSize, 0, false);
}

// Bresenham's algorithm - thx wikpedia
void UC1701_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color)
{
	int16_t steep = abs(y1 - y0) > abs(x1 - x0);

	if (steep)
	{
		swap(x0, y0);
		swap(x1, y1);
	}

	if (x0 > x1)
	{
		swap(x0, x1);
		swap(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1)
		ystep = 1;
	else
		ystep = -1;

	for (; x0<=x1; x0++)
	{
		if (steep)
			UC1701_setPixel(y0, x0, color);
		else
			UC1701_setPixel(x0, y0, color);

		err -= dy;
		if (err < 0)
		{
			y0 += ystep;
			err += dx;
		}
	}
}

void UC1701_drawFastVLine(int16_t x, int16_t y, int16_t h, bool color)
{
	UC1701_drawLine(x, y, x, y + h - 1, color);
}

void UC1701_drawFastHLine(int16_t x, int16_t y, int16_t w, bool color)
{
	UC1701_drawLine(x, y, x + w - 1, y, color);
}

// Draw a circle outline
void UC1701_drawCircle(int16_t x0, int16_t y0, int16_t r, bool color)
{
	int16_t f     = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x     = 0;
	int16_t y     = r;

	UC1701_setPixel(x0    , y0 + r, color);
	UC1701_setPixel(x0    , y0 - r, color);
	UC1701_setPixel(x0 + r, y0    , color);
	UC1701_setPixel(x0 - r, y0    , color);

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}

		x++;
		ddF_x += 2;
		f += ddF_x;

		UC1701_setPixel(x0 + x, y0 + y, color);
		UC1701_setPixel(x0 - x, y0 + y, color);
		UC1701_setPixel(x0 + x, y0 - y, color);
		UC1701_setPixel(x0 - x, y0 - y, color);
		UC1701_setPixel(x0 + y, y0 + x, color);
		UC1701_setPixel(x0 - y, y0 + x, color);
		UC1701_setPixel(x0 + y, y0 - x, color);
		UC1701_setPixel(x0 - y, y0 - x, color);
	}
}

void UC1701_drawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, bool color)
{
	int16_t f     = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x     = 0;
	int16_t y     = r;

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f     += ddF_y;
		}

		x++;
		ddF_x += 2;
		f     += ddF_x;

		if (cornername & 0x4)
		{
			UC1701_setPixel(x0 + x, y0 + y, color);
			UC1701_setPixel(x0 + y, y0 + x, color);
		}

		if (cornername & 0x2)
		{
			UC1701_setPixel(x0 + x, y0 - y, color);
			UC1701_setPixel(x0 + y, y0 - x, color);
		}

		if (cornername & 0x8)
		{
			UC1701_setPixel(x0 - y, y0 + x, color);
			UC1701_setPixel(x0 - x, y0 + y, color);
		}

		if (cornername & 0x1)
		{
			UC1701_setPixel(x0 - y, y0 - x, color);
			UC1701_setPixel(x0 - x, y0 - y, color);
		}
	}
}

/*
 * Used to do circles and roundrects
 */
void UC1701_fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, bool color)
{
	int16_t f     = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x     = 0;
	int16_t y     = r;

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f     += ddF_y;
		}

		x++;
		ddF_x += 2;
		f     += ddF_x;

		if (cornername & 0x1)
		{
			UC1701_drawFastVLine(x0 + x, y0 - y, 2 * y + 1 + delta, color);
			UC1701_drawFastVLine(x0 + y, y0 - x, 2 * x + 1 + delta, color);
		}

		if (cornername & 0x2)
		{
			UC1701_drawFastVLine(x0 - x, y0 - y, 2 * y + 1 + delta, color);
			UC1701_drawFastVLine(x0 - y, y0 - x, 2 * x + 1 + delta, color);
		}
	}
}

void UC1701_fillCircle(int16_t x0, int16_t y0, int16_t r, bool color)
{
	UC1701_drawFastVLine(x0, y0 - r, 2 * r + 1, color);
	UC1701_fillCircleHelper(x0, y0, r, 3, 0, color);
}

/*
 * ***** Arc related functions *****
 */
static float cosDegrees(float angle)
{
	return cos(angle * DEG_TO_RAD);
}

static float sinDegrees(float angle)
{
	return sin(angle * DEG_TO_RAD);
}

/*
 * DrawArc function thanks to Jnmattern and his Arc_2.0 (https://github.com/Jnmattern)
 */
void UC1701_fillArcOffsetted(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t thickness, float start, float end, bool color)
{
	int16_t xmin = 65535, xmax = -32767, ymin = 32767, ymax = -32767;
	float cosStart, sinStart, cosEnd, sinEnd;
	float r, t;
	float startAngle, endAngle;

	startAngle = (start / _arcAngleMax) * 360;	// 252
	endAngle = (end / _arcAngleMax) * 360;		// 807

	while (startAngle < 0) startAngle += 360;
	while (endAngle < 0) endAngle += 360;
	while (startAngle > 360) startAngle -= 360;
	while (endAngle > 360) endAngle -= 360;

	if (startAngle > endAngle)
	{
		UC1701_fillArcOffsetted(cx, cy, radius, thickness, ((startAngle) / (float)360) * _arcAngleMax, _arcAngleMax, color);
		UC1701_fillArcOffsetted(cx, cy, radius, thickness, 0, ((endAngle) / (float)360) * _arcAngleMax, color);
	}
	else
	{
		// Calculate bounding box for the arc to be drawn
		cosStart = cosDegrees(startAngle);
		sinStart = sinDegrees(startAngle);
		cosEnd = cosDegrees(endAngle);
		sinEnd = sinDegrees(endAngle);

		r = radius;
		// Point 1: radius & startAngle
		t = r * cosStart;
		if (t < xmin) xmin = t;
		if (t > xmax) xmax = t;
		t = r * sinStart;
		if (t < ymin) ymin = t;
		if (t > ymax) ymax = t;

		// Point 2: radius & endAngle
		t = r * cosEnd;
		if (t < xmin) xmin = t;
		if (t > xmax) xmax = t;
		t = r * sinEnd;
		if (t < ymin) ymin = t;
		if (t > ymax) ymax = t;

		r = radius - thickness;
		// Point 3: radius-thickness & startAngle
		t = r * cosStart;
		if (t < xmin) xmin = t;
		if (t > xmax) xmax = t;
		t = r * sinStart;
		if (t < ymin) ymin = t;
		if (t > ymax) ymax = t;

		// Point 4: radius-thickness & endAngle
		t = r * cosEnd;
		if (t < xmin) xmin = t;
		if (t > xmax) xmax = t;
		t = r * sinEnd;
		if (t < ymin) ymin = t;
		if (t > ymax) ymax = t;

		// Corrections if arc crosses X or Y axis
		if ((startAngle < 90) && (endAngle > 90))
		{
			ymax = radius;
		}

		if ((startAngle < 180) && (endAngle > 180))
		{
			xmin = -radius;
		}

		if ((startAngle < 270) && (endAngle > 270))
		{
			ymin = -radius;
		}

		// Slopes for the two sides of the arc
		float sslope = (float)cosStart / (float)sinStart;
		float eslope = (float)cosEnd / (float)sinEnd;

		if (endAngle == 360) eslope = -1000000;

		int ir2 = (radius - thickness) * (radius - thickness);
		int or2 = radius * radius;

		for (int x = xmin; x <= xmax; x++)
		{
			bool y1StartFound = false, y2StartFound = false;
			bool y1EndFound = false, y2EndSearching = false;
			int y1s = 0, y1e = 0, y2s = 0;
			for (int y = ymin; y <= ymax; y++)
			{
				int x2 = x * x;
				int y2 = y * y;

				if (
					(x2 + y2 < or2 && x2 + y2 >= ir2) && (
					(y > 0 && startAngle < 180 && x <= y * sslope) ||
					(y < 0 && startAngle > 180 && x >= y * sslope) ||
					(y < 0 && startAngle <= 180) ||
					(y == 0 && startAngle <= 180 && x < 0) ||
					(y == 0 && startAngle == 0 && x > 0)
					) && (
					(y > 0 && endAngle < 180 && x >= y * eslope) ||
					(y < 0 && endAngle > 180 && x <= y * eslope) ||
					(y > 0 && endAngle >= 180) ||
					(y == 0 && endAngle >= 180 && x < 0) ||
					(y == 0 && startAngle == 0 && x > 0)))
				{
					if (!y1StartFound)	//start of the higher line found
					{
						y1StartFound = true;
						y1s = y;
					}
					else if (y1EndFound && !y2StartFound) //start of the lower line found
					{
						y2StartFound = true;
						//drawPixel_cont(cx+x, cy+y, ILI9341_BLUE);
						y2s = y;
						y += y1e - y1s - 1;	// calculate the most probable end of the lower line (in most cases the length of lower line is equal to length of upper line), in the next loop we will validate if the end of line is really there
						if (y > ymax - 1) // the most probable end of line 2 is beyond ymax so line 2 must be shorter, thus continue with pixel by pixel search
						{
							y = y2s;	// reset y and continue with pixel by pixel search
							y2EndSearching = true;
						}
					}
					else if (y2StartFound && !y2EndSearching)
					{
						// we validated that the probable end of the lower line has a pixel, continue with pixel by pixel search, in most cases next loop with confirm the end of lower line as it will not find a valid pixel
						y2EndSearching = true;
					}
				}
				else
				{
					if (y1StartFound && !y1EndFound) //higher line end found
					{
						y1EndFound = true;
						y1e = y - 1;
						UC1701_drawFastVLine(cx + x, cy + y1s, y - y1s, color);
						if (y < 0)
						{
							y = abs(y); // skip the empty middle
						}
						else
							break;
					}
					else if (y2StartFound)
					{
						if (y2EndSearching)
						{
							// we found the end of the lower line after pixel by pixel search
							UC1701_drawFastVLine(cx + x, cy + y2s, y - y2s, color);
							y2EndSearching = false;
							break;
						}
						else
						{
							// the expected end of the lower line is not there so the lower line must be shorter
							y = y2s;	// put the y back to the lower line start and go pixel by pixel to find the end
							y2EndSearching = true;
						}
					}
				}
			}
			if (y1StartFound && !y1EndFound)
			{
				y1e = ymax;
				UC1701_drawFastVLine(cx + x, cy + y1s, y1e - y1s + 1, color);
			}
			else if (y2StartFound && y2EndSearching)	// we found start of lower line but we are still searching for the end
			{										// which we haven't found in the loop so the last pixel in a column must be the end
				UC1701_drawFastVLine(cx + x, cy + y2s, ymax - y2s + 1, color);
			}
		}
	}
}

void UC1701_fillArc(uint16_t x, uint16_t y, uint16_t radius, uint16_t thickness, float start, float end, bool color)
{
	if (start == 0 && end == _arcAngleMax)
		UC1701_fillArcOffsetted(x, y, radius, thickness, 0, _arcAngleMax, color);
	else
		UC1701_fillArcOffsetted(x, y, radius, thickness, start + (_angleOffset / (float)360)*_arcAngleMax, end + (_angleOffset / (float)360)*_arcAngleMax, color);
}
/*
 * ***** End of Arc related functions *****
 */

void UC1701_drawellipse(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color)
{
  int16_t a = abs(x1 - x0), b = abs(y1 - y0), b1 = b & 1; /* values of diameter */
  long dx = 4 * (1 - a) * b * b, dy = 4 * (b1 + 1) * a * a; /* error increment */
  long err = dx + dy + b1 * a * a, e2; /* error of 1.step */

  if (x0 > x1) { x0 = x1; x1 += a; } /* if called with swapped points */
  if (y0 > y1) y0 = y1; /* .. exchange them */
  y0 += (b + 1) / 2; /* starting pixel */
  y1 = y0 - b1;
  a *= 8 * a;
  b1 = 8 * b * b;

  do {
	  UC1701_setPixel(x1, y0, color); /*   I. Quadrant */
	  UC1701_setPixel(x0, y0, color); /*  II. Quadrant */
	  UC1701_setPixel(x0, y1, color); /* III. Quadrant */
	  UC1701_setPixel(x1, y1, color); /*  IV. Quadrant */
    e2 = 2 * err;
    if (e2 >= dx) { x0++; x1--; err += dx += b1; } /* x step */
    if (e2 <= dy) { y0++; y1--; err += dy += a; }  /* y step */
  } while (x0 <= x1);

  while (y0 - y1 < b) /* too early stop of flat ellipses a=1 */
  {
	  UC1701_setPixel(x0 - 1, ++y0, color); /* -> complete tip of ellipse */
	  UC1701_setPixel(x0 - 1, --y1, color);
  }
}

/*
 * Draw a triangle
 */
void UC1701_drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool color)
{
	UC1701_drawLine(x0, y0, x1, y1, color);
	UC1701_drawLine(x1, y1, x2, y2, color);
	UC1701_drawLine(x2, y2, x0, y0, color);
}

/*
 * Fill a triangle
 */
void UC1701_fillTriangle ( int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool color)
{
	int16_t a, b, y, last;

	// Sort coordinates by Y order (y2 >= y1 >= y0)
	if (y0 > y1)
	{
		swap(y0, y1); swap(x0, x1);
	}
	if (y1 > y2)
	{
		swap(y2, y1); swap(x2, x1);
	}
	if (y0 > y1)
	{
		swap(y0, y1); swap(x0, x1);
	}

	if(y0 == y2) // Handle awkward all-on-same-line case as its own thing
	{
		a = b = x0;
		if(x1 < a)      a = x1;
		else if(x1 > b) b = x1;

		if(x2 < a)      a = x2;
		else if(x2 > b) b = x2;

		UC1701_drawFastHLine(a, y0, b - a + 1, color);
		return;
	}

	int16_t dx01 = x1 - x0,
			dy01 = y1 - y0,
			dx02 = x2 - x0,
			dy02 = y2 - y0,
			dx12 = x2 - x1,
			dy12 = y2 - y1,
			sa   = 0,
			sb   = 0;

	// For upper part of triangle, find scanline crossings for segments
	// 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
	// is included here (and second loop will be skipped, avoiding a /0
	// error there), otherwise scanline y1 is skipped here and handled
	// in the second loop...which also avoids a /0 error here if y0=y1
	// (flat-topped triangle).
	if(y1 == y2) last = y1;   // Include y1 scanline
	else         last = y1-1; // Skip it

	for(y=y0; y<=last; y++) {
		a   = x0 + sa / dy01;
		b   = x0 + sb / dy02;
		sa += dx01;
		sb += dx02;
		/* longhand:
		a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
		b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
		*/
		if(a > b)
			swap(a,b);

		UC1701_drawFastHLine(a, y, b - a + 1, color);
	}

	// For lower part of triangle, find scanline crossings for segments
	// 0-2 and 1-2.  This loop is skipped if y1=y2.
	sa = dx12 * (y - y1);
	sb = dx02 * (y - y0);

	for(; y<=y2; y++)
	{
		a   = x1 + sa / dy12;
		b   = x0 + sb / dy02;
		sa += dx12;
		sb += dx02;
		/* longhand:
		a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
		b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
		*/
		if(a > b)
			swap(a,b);

		UC1701_drawFastHLine(a, y, b - a + 1, color);
	}
}

/*
 * Draw a rounded rectangle
 */
void UC1701_drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool color)
{
	// smarter version
	UC1701_drawFastHLine(x + r    , y        , w - 2 * r, color); // Top
	UC1701_drawFastHLine(x + r    , y + h - 1, w - 2 * r, color); // Bottom
	UC1701_drawFastVLine(x        , y + r    , h - 2 * r, color); // Left
	UC1701_drawFastVLine(x + w - 1, y + r    , h - 2 * r, color); // Right
	// draw four corners
	UC1701_drawCircleHelper(x + r        , y + r        , r, 1, color);
	UC1701_drawCircleHelper(x + w - r - 1, y + r        , r, 2, color);
	UC1701_drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
	UC1701_drawCircleHelper(x + r        , y + h - r - 1, r, 8, color);
}

/*
 * Fill a rounded rectangle
 */
void UC1701_fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, bool color)
{
	UC1701_fillRect(x + r, y, w - 2 * r, h, !color);

	// draw four corners
	UC1701_fillCircleHelper(x+w-r-1, y + r, r, 1, h - 2 * r - 1, color);
	UC1701_fillCircleHelper(x+r    , y + r, r, 2, h - 2 * r - 1, color);
}

/*
 * Draw a rectangle
 */
void UC1701_drawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool color)
{
	UC1701_drawFastHLine(x        , y        , w, color);
	UC1701_drawFastHLine(x        , y + h - 1, w, color);
	UC1701_drawFastVLine(x        , y        , h, color);
	UC1701_drawFastVLine(x + w - 1, y        , h, color);
}

/*
 * Fill a rectangle
 */
void UC1701_fillRect(int16_t x, int16_t y, int16_t width, int16_t height, bool isInverted)
{
	uint8_t *addPtr;
	int16_t endStripe 	= x+width;
	int16_t startRow 	= y>>3;
	int16_t endRow 		= ((y+height)>>3);
	uint8_t bitPatten;
	int16_t shiftNum;

	if (startRow==endRow)
	{
		addPtr = screenBuf + (startRow << 7);
		bitPatten = (0xff >> (8-(height&0x07))) << (y&0x07);
		//bitPatten = bitPatten ;
		for(int16_t stripe=x;stripe < endStripe;stripe++)
		{
			if (isInverted)
			{
				*(addPtr + stripe) &= ~ bitPatten;
			}
			else
			{
				*(addPtr + stripe) |= bitPatten;
			}
		}
	}
	else
	{
		for(int16_t row=startRow;row<=endRow;row++)
		{
			if (row==startRow)
			{
				shiftNum = y & 0x07;
				bitPatten = (0xff << shiftNum);
			}
			else
			{
				if (row == endRow)
				{
					shiftNum = (y+height) & 0x07;
					bitPatten = (0xff >> (8 - shiftNum));
				}
				else
				{
					// middle rows
					bitPatten = 0xff;
				}
			}
			addPtr = screenBuf + (row << 7);
			for(int16_t stripe=x;stripe < endStripe;stripe++)
			{
				if (isInverted)
				{
					*(addPtr + stripe) &= ~ bitPatten;
				}
				else
				{
					*(addPtr + stripe) |= bitPatten;
				}
			}
		}
	}
}

/*
 * Draw a 1-bit image at the specified (x,y) position.
*/
void UC1701_drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, bool color)
{
    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    for(int16_t j = 0; j < h; j++, y++)
    {
        for(int16_t i = 0; i < w; i++)
        {
            if(i & 7)
            	byte <<= 1;
            else
            	byte = *(bitmap + (j * byteWidth + i / 8));

            if(byte & 0x80)
            	UC1701_setPixel(x + i, y, color);
        }
    }
}

/*
 * Draw XBitMap Files (*.xbm), e.g. exported from GIMP.
*/
void UC1701_drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, bool color)
{
    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    for(int16_t j = 0; j < h; j++, y++)
    {
        for(int16_t i = 0; i < w; i++)
        {
            if(i & 7)
            	byte >>= 1;
            else
            	byte = *(bitmap + (j * byteWidth + i / 8));
            // Nearly identical to drawBitmap(), only the bit order
            // is reversed here (left-to-right = LSB to MSB):
            if(byte & 0x01)
            	UC1701_setPixel(x + i, y, color);
        }
    }
}
