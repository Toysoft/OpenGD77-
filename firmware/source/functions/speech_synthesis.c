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
#include <io/LEDs.h>
#include <math.h>
#include <stdio.h>
#include <usb_com.h>



#if defined(PLATFORM_GD77S)

__attribute__((section(".data.$RAM4"))) static const struct
{
	EPL003_Sequence_t	Word;
	uint32_t			Duration;
} SpeechSequences[] =
{
		{ SPEECH_SYNTHESIS_ZERO,						 5680 },
		{ SPEECH_SYNTHESIS_ONE,							 4620 },
		{ SPEECH_SYNTHESIS_TWO,							 4400 },
		{ SPEECH_SYNTHESIS_THREE,						 4320 },
		{ SPEECH_SYNTHESIS_FOUR,						 4360 },
		{ SPEECH_SYNTHESIS_FIVE,						 6920 },
		{ SPEECH_SYNTHESIS_SIX,							 5300 },
		{ SPEECH_SYNTHESIS_SEVEN,						 5460 },
		{ SPEECH_SYNTHESIS_HEIGHT,						 4080 },
		{ SPEECH_SYNTHESIS_NINE,						 5340 },
		{ SPEECH_SYNTHESIS_TEN,							 4880 },
		{ SPEECH_SYNTHESIS_ELEVEN,						 6260 },
		{ SPEECH_SYNTHESIS_TWELVE,						 5980 },
		{ SPEECH_SYNTHESIS_THIRTEEN,					 6660 },
		{ SPEECH_SYNTHESIS_FOURTEEN,					 5880 },
		{ SPEECH_SYNTHESIS_FIFTEEN,						 6440 },
		{ SPEECH_SYNTHESIS_SIXTEEN,						 7460 },
		{ SPEECH_SYNTHESIS_SEVENTEEN,					 7540 },
		{ SPEECH_SYNTHESIS_HEIGHTEEN,					 6600 },
		{ SPEECH_SYNTHESIS_NINETEEN,					 7320 },
		{ SPEECH_SYNTHESIS_TWENTY,						 6920 },
		{ SPEECH_SYNTHESIS_THIRTY,						 6120 },
		{ SPEECH_SYNTHESIS_FOURTY,						 6320 },
		{ SPEECH_SYNTHESIS_FIFTY,						 6680 },
		{ SPEECH_SYNTHESIS_SIXTY,						 7220 },
		{ SPEECH_SYNTHESIS_SEVENTY,						 8060 },
		{ SPEECH_SYNTHESIS_EIGHTY,						 6380 },
		{ SPEECH_SYNTHESIS_NINETY,						 6560 },
		{ SPEECH_SYNTHESIS_HUNDRED,						 5420 },
		{ SPEECH_SYNTHESIS_POWER_ON,					 7960 },
		{ SPEECH_SYNTHESIS_PLEASE_CHARGE_THE_BATTERY,	13920 },
		{ SPEECH_SYNTHESIS_CLONING,						 6000 },
		{ SPEECH_SYNTHESIS_POWER_OFF,					 8340 },
		{ SPEECH_SYNTHESIS_ALARM,						 6100 },
		{ SPEECH_SYNTHESIS_SQUELCH,						 6400 },
		{ SPEECH_SYNTHESIS_VOX,							 6560 },
		{ SPEECH_SYNTHESIS_LEVEL,						 5440 },
		{ SPEECH_SYNTHESIS_SCRAMBLE,					 6880 },
		{ SPEECH_SYNTHESIS_SCAN,						 7360 },
		{ SPEECH_SYNTHESIS_ON,							 4700 },
		{ SPEECH_SYNTHESIS_OFF,							 4040 },
		{ SPEECH_SYNTHESIS_FREQUENCY,					 7360 },
		{ SPEECH_SYNTHESIS_CHANNEL,						 5080 },
		{ SPEECH_SYNTHESIS_POWER,						 4820 },
		{ SPEECH_SYNTHESIS_KEY,							 4500 },
		{ SPEECH_SYNTHESIS_LOCK,						 4900 },
		{ SPEECH_SYNTHESIS_UNLOCK,						 5460 },
		{ SPEECH_SYNTHESIS_MODE,						 5560 },
		{ SPEECH_SYNTHESIS_SHIFT,						 5460 },
		{ SPEECH_SYNTHESIS_PLUS,						 5500 },
		{ SPEECH_SYNTHESIS_MINUS,						 5840 },
		{ SPEECH_SYNTHESIS_REVERSE,						 6240 },
		{ SPEECH_SYNTHESIS_PRIORITY,					 6440 },
		{ SPEECH_SYNTHESIS_TONE,						 5800 },
		{ SPEECH_SYNTHESIS_RECEIVE,						 5620 },
		{ SPEECH_SYNTHESIS_TRANSMIT,					 6840 },
		{ SPEECH_SYNTHESIS_STORE,						 5820 },
		{ SPEECH_SYNTHESIS_ID_CODE,						 8820 },
		{ SPEECH_SYNTHESIS_CTCSS,						15320 },
		{ SPEECH_SYNTHESIS_DCS,							 8660 },
		{ SPEECH_SYNTHESIS_DTMF,						11560 },
		{ SPEECH_SYNTHESIS_STEP,						 4780 },
		{ SPEECH_SYNTHESIS_WIDE,						 5260 },
		{ SPEECH_SYNTHESIS_NARROW,						 5840 },
		{ SPEECH_SYNTHESIS_LIGHT,						 4360 },
		{ SPEECH_SYNTHESIS_MENU,						 6180 },
		{ SPEECH_SYNTHESIS_FUNCTION,					 5600 },
		{ SPEECH_SYNTHESIS_SET,							 4060 },
		{ SPEECH_SYNTHESIS_NAME,						 5240 },
		{ SPEECH_SYNTHESIS_EXIT,						 4180 },
		{ SPEECH_SYNTHESIS_ERROR,						 4240 },
		{ SPEECH_SYNTHESIS_AUTO,						 4280 },
		{ SPEECH_SYNTHESIS_OKAY,						 7440 },
		{ SPEECH_SYNTHESIS_ENTER,						 4440 },
		{ SPEECH_SYNTHESIS_DELETE,						 5140 },
		{ SPEECH_SYNTHESIS_HIGH,						 5100 },
		{ SPEECH_SYNTHESIS_MIDDLE,						 5120 },
		{ SPEECH_SYNTHESIS_LOW,							 4920 },
		{ SPEECH_SYNTHESIS_BATTERY,						 5760 },
		{ SPEECH_SYNTHESIS_POINT,						 5740 },
		{ SPEECH_SYNTHESIS_SAVE,						 5320 },
		{ SPEECH_SYNTHESIS_RADIO,						 5840 },
		{ SPEECH_SYNTHESIS_T_O_T,						 8140 },
		{ SPEECH_SYNTHESIS_REVERSE_FUNCTION,			10740 },
		{ SPEECH_SYNTHESIS_COMM_PENDING,				 7340 },
		{ SPEECH_SYNTHESIS_WHISPER,						 6100 },
		{ SPEECH_SYNTHESIS_TONE_DUPLICATE,				 5800 },
		{ SPEECH_SYNTHESIS_END,							 4320 },
		{ SPEECH_SYNTHESIS_SEQUENCE_SEPARATOR,           2000 }, // This slot is empty in the IC, we use it as a gap
		{ SPEECH_SYNTHESIS_DISPLAY_COLOR,				 9800 },
		{ SPEECH_SYNTHESIS_ORANGE,						 6520 },
		{ SPEECH_SYNTHESIS_GREEN,						 5960 },
		{ SPEECH_SYNTHESIS_BLUE,						 5940 },
		{ SPEECH_SYNTHESIS_RED,							 5860 },
		{ SPEECH_SYNTHESIS_YELLOW,						 6100 },
		{ SPEECH_SYNTHESIS_PURPLE,						 6060 },
		{ SPEECH_SYNTHESIS_WHITE,						 5280 }
};

typedef struct
{
	uint8_t  Buffer[SPEECH_SYNTHESIS_BUFFER_SIZE];
	uint8_t  Pos;
	uint8_t  Length;
	uint32_t EndsAt;
} SpeechCurrentSequence_t;

static SpeechCurrentSequence_t ssSeq;

#endif

void speechSynthesisInit(void)
{
#if defined(PLATFORM_GD77S)
	EPL003_init();

	// We don't care if Buffer is clean or not.
	ssSeq.Pos = 0;
	ssSeq.Length = 0;
	ssSeq.EndsAt = 0;
#endif
}

void speechSynthesisSpeak(uint8_t *sentence)
{
#if defined(PLATFORM_GD77S)
	if (!trxIsTransmitting && ((sentence[0] > 0) && (sentence[0] <= sizeof(ssSeq.Buffer))))
	{
		ssSeq.Pos = 0;
		ssSeq.Length = sentence[0];

		memcpy(&(ssSeq.Buffer[0]), &(sentence[1]), ssSeq.Length);
	}
#endif
}

bool speechSynthesisIsSpeaking(void)
{
#if defined(PLATFORM_GD77S)
	return (ssSeq.Length != 0);
#else
	return false;
#endif
}

void speechSynthesisTick(void)
{
#if defined(PLATFORM_GD77S)
	if (ssSeq.Length == 0)
	{
		return;
	}

	if (ssSeq.Pos == 0)
	{
		// Turn on Audio Amp
		enableAudioAmp(AUDIO_AMP_MODE_BEEP);

		if (trxGetMode() == RADIO_MODE_ANALOG)
		{
			GPIO_PinWrite(GPIO_RX_audio_mux, Pin_RX_audio_mux, 0);// set the audio mux HR-C6000 -> audio amp
		}

		GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 1);
		ssSeq.EndsAt = PITCounter - 10; // Force to start immediately
	}

	if (PITCounter > ssSeq.EndsAt)
	{
		// We have reached the end of the speech sequence
		if (ssSeq.Pos >= ssSeq.Length)
		{
			if (trxGetMode() == RADIO_MODE_ANALOG)
			{
				/*
				 *  The DM-1801 audio amplifier seems very slow to respond to the control signal
				 *  probably because the amp does not have an integrated enabled / disable pin,
				 *  and instead the power to the amp is turned on and and off via a transistor.
				 *  In FM mode when there is no signal, this results in the unsquelched hiss being heard at the end of the beep,
				 *  in the time between the beep ending and the amp turning off.
				 *  To resolve this problem, the audio mux control to the amp input, is left set to the output of the C6000
				 *  unless there is a RF audio signal.
				 *
				 *  Note. Its quicker just to read the Green LED pin to see if there is an RF signal, but its safer to get he audio amp status
				 *  to see if the amp is already on because of an RF signal.
				 *
				 *  On the GD-77. A click also seems to be induced at the end of the beep when the mux is changed back to the RF chip (AT1846)
				 *  So this fix seems to slighly improve the GD-77 beep on FM
				 */
				if (getAudioAmpStatus() & AUDIO_AMP_MODE_RF)
				{
					GPIO_PinWrite(GPIO_RX_audio_mux, Pin_RX_audio_mux, 1);// Set the audio path to AT1846 -> audio amp.
				}
			}

			disableAudioAmp(AUDIO_AMP_MODE_BEEP);
			if (((getAudioAmpStatus() & AUDIO_AMP_MODE_RF) == 0) && (trxCarrierDetected() == 0))
			{
				GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
			}
			ssSeq.Length = 0;
		}
		else
		{
			// Send the current sequence to the chip
			EPL003_speak(SpeechSequences[ssSeq.Buffer[ssSeq.Pos]].Word);
			ssSeq.EndsAt = PITCounter + SpeechSequences[ssSeq.Buffer[ssSeq.Pos]].Duration;
			ssSeq.Pos++;
		}
	}
#endif
}

// Helper functions
uint8_t speechSynthesisBuildFromNumberInString(uint8_t *dest, uint8_t destSize, const char *str, bool enumerate)
{
	uint8_t   *pBuf = dest;
#if defined(PLATFORM_GD77S)
	char      *p = (char *)str;

	// Just enumerates all numbers, plus '.', and sign
	if (enumerate)
	{
		while ((*p != 0) && ((p - str) < destSize))
		{
			if ((*p >= '0') && (*p <= '9'))
			{
				*pBuf++ = (*p - 48) + SPEECH_SYNTHESIS_ZERO;
			}
			else if (*p == '.')
			{
				*pBuf++ = SPEECH_SYNTHESIS_POINT;
			}
			else if (*p == '+')
			{
				*pBuf++ = SPEECH_SYNTHESIS_PLUS;
			}
			else if (*p == '-')
			{
				*pBuf++ = SPEECH_SYNTHESIS_MINUS;
			}

			p++;
		}
	}
	else // Builds a real spoken number (less the 'and')
	{
		uint8_t    len = strlen(p);
		bool       sign = ((*str == '+') || (*str == '-'));

		if (len > destSize)
		{
			return 0;
		}

		if (sign)
		{
			*pBuf++ = (*str == '+') ? SPEECH_SYNTHESIS_PLUS : SPEECH_SYNTHESIS_MINUS;

			p++;
			len--;
		}

		// Got a float number
		char *point;
		if ((point = strchr(p, '.')))
		{
			char buffer[17];

			snprintf(buffer, 15, "%s", p);
			buffer[16] = 0;
			point = buffer + ((point - p));
			*point++ = 0;

			// First part
			pBuf += speechSynthesisBuildFromNumberInString(pBuf, destSize, buffer, true);

			// Insert the point
			*pBuf++ = SPEECH_SYNTHESIS_POINT;

			// Second part
			pBuf += speechSynthesisBuildFromNumberInString(pBuf, (destSize - (pBuf - dest)), point, true);

			return (pBuf - dest);
		}

		if (len > 3)
		{
			// Not that clean, but hey ! ;-)
			*(p + 3) = 0;
			len = 3;
		}

		if ((*p == '0') && (*(p + 1) == '0') && (*(p + 2) == '0'))
		{
			*pBuf++ = SPEECH_SYNTHESIS_ZERO;
			*pBuf++ = SPEECH_SYNTHESIS_ZERO;
			*pBuf++ = SPEECH_SYNTHESIS_ZERO;
			return (pBuf - dest);
		}

		switch (len)
		{
			case 3: // First digit
			{
				*pBuf++ = (*p - 48) + SPEECH_SYNTHESIS_ZERO;

				if (*p != '0')
				{
					*pBuf++ = SPEECH_SYNTHESIS_HUNDRED;
				}

				p++;
			}

			case 2: // Second digit
			{
				if (*p == '1')
				{
					*pBuf++ = (*(p + 1) - 48) + SPEECH_SYNTHESIS_TEN;
					break;
				}
				else if (*p >= '2')
				{
					*pBuf++ = (*p - 50) + SPEECH_SYNTHESIS_TWENTY;

					if (*(p + 1) == '0')
					{
						break;
					}
				}
				else // zero
				{
					if (((len == 3) && (*(pBuf - 1) != SPEECH_SYNTHESIS_HUNDRED) && (*p == '0'))
							|| ((len == 2) && (*p == '0')))
					{
						*pBuf++ = SPEECH_SYNTHESIS_ZERO;
					}
				}

				p++;
			}

			case 1: // Third digit
				if (((len > 1) && (*p != '0')) || (len == 1))
				{
					*pBuf++ = (*p - 48) + SPEECH_SYNTHESIS_ZERO;
				}
				break;

			default:
				// Noop
				break;
		}
	}
#endif

	return (pBuf - dest);
}

uint8_t speechSynthesisBuildNumerical(uint8_t *dest, uint8_t destSize, float value, uint8_t numberOfDecimals, bool enumerate)
{
#if defined(PLATFORM_GD77S)
	char str[16];
	char *p = str;

	// Is it an integer
	if (ceilf(value) == value)
	{
		int iValue = (int)value;

		snprintf(&str[0], 16, "%d", iValue);

		return speechSynthesisBuildFromNumberInString(dest, destSize, p, enumerate);
	}
	else // Or a float
	{
		int     intValue = (int)value;
		double  decValue = value - intValue;
		uint8_t destPos;

		if (numberOfDecimals == 0)
		{
			numberOfDecimals++;
		}

		// Int part
		snprintf(&str[0], 16, "%d", intValue);

		destPos = speechSynthesisBuildFromNumberInString(dest, destSize, p, enumerate);

		if ((destPos + 2) <= destSize)
		{
			// Add a point
			*(dest + (destPos++)) = SPEECH_SYNTHESIS_POINT;

			// Then the decimal part
#if 0 // With printf's float capable
			snprintf(&str[0], 16, "%.*f", numberOfDecimals, decValue);
			// starts at the first decimal digit
			p = &str[2];
#else // integer version
			snprintf(&str[0], 16, "%03d", (int)(decValue * (value < -0.0 ? -1000.00 : 1000.00)));
			if (numberOfDecimals < 3)
			{
				*(str + numberOfDecimals) = 0;
			}
			p = str;
#endif

			return (destPos + speechSynthesisBuildFromNumberInString((dest + destPos), (destSize - destPos), p, enumerate));
		}
	}
#endif

	return 0U;
}

