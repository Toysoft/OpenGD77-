/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
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
#include <user_interface/uiLocalisation.h>
#include <functions/fw_settings.h>
#include <interfaces/fw_wdog.h>
#include <user_interface/uiUtilities.h>

static void updateScreen(void);
static void handleEvent(uiEvent_t *ev);
static bool	doFactoryReset;
enum OPTIONS_MENU_LIST { OPTIONS_MENU_TIMEOUT_BEEP=0,OPTIONS_MENU_FACTORY_RESET,OPTIONS_MENU_USE_CALIBRATION,
							OPTIONS_MENU_TX_FREQ_LIMITS,OPTIONS_MENU_BEEP_VOLUME,OPTIONS_MIC_GAIN_DMR,
							OPTIONS_MENU_KEYPAD_TIMER_LONG, OPTIONS_MENU_KEYPAD_TIMER_REPEAT, OPTIONS_MENU_DMR_MONITOR_CAPTURE_TIMEOUT,
							OPTIONS_MENU_SCAN_DELAY,OPTIONS_MENU_SCAN_MODE,
							OPTIONS_MENU_SQUELCH_DEFAULT_VHF,OPTIONS_MENU_SQUELCH_DEFAULT_220MHz,OPTIONS_MENU_SQUELCH_DEFAULT_UHF,
							OPTIONS_MENU_PTT_TOGGLE, OPTIONS_MENU_HOTSPOT_TYPE, OPTIONS_MENU_TALKER_ALIAS_TX,
							OPTIONS_MENU_PRIVATE_CALLS,
							NUM_OPTIONS_MENU_ITEMS};

int menuOptions(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		doFactoryReset=false;
		// Store original settings, used on cancel event.
		memcpy(&originalNonVolatileSettings, &nonVolatileSettings, sizeof(settingsStruct_t));
		updateScreen();
	}
	else
	{
		if (ev->hasEvent)
			handleEvent(ev);
	}
	return 0;
}

static void updateScreen(void)
{
	int mNum = 0;
	static const int bufferLen = 17;
	char buf[bufferLen];

	ucClearBuf();
	menuDisplayTitle(currentLanguage->options);

	// Can only display 3 of the options at a time menu at -1, 0 and +1
	for(int i = -1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_OPTIONS_MENU_ITEMS, i);
		buf[0] = 0;

		switch(mNum)
		{
			case OPTIONS_MENU_TIMEOUT_BEEP:
				if (nonVolatileSettings.txTimeoutBeepX5Secs != 0)
				{
					snprintf(buf, bufferLen, "%s:%d", currentLanguage->timeout_beep, nonVolatileSettings.txTimeoutBeepX5Secs * 5);
				}
				else
				{
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->timeout_beep, currentLanguage->off);
				}
				break;
			case OPTIONS_MENU_FACTORY_RESET:
				if (doFactoryReset == true)
				{
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->factory_reset, currentLanguage->yes);
				}
				else
				{
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->factory_reset, currentLanguage->no);
				}
				break;
			case OPTIONS_MENU_USE_CALIBRATION:
				if (nonVolatileSettings.useCalibration)
				{
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->calibration, currentLanguage->on);
				}
				else
				{
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->calibration, currentLanguage->off);
				}
				break;
			case OPTIONS_MENU_TX_FREQ_LIMITS:// Tx Freq limits
				if (nonVolatileSettings.txFreqLimited)
				{
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->band_limits, currentLanguage->on);
				}
				else
				{
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->band_limits, currentLanguage->off);
				}
				break;
			case OPTIONS_MENU_BEEP_VOLUME:// Beep volume reduction
				snprintf(buf, bufferLen, "%s:%ddB", currentLanguage->beep_volume, (2 - nonVolatileSettings.beepVolumeDivider) * 3);
				soundBeepVolumeDivider = nonVolatileSettings.beepVolumeDivider;
				break;
			case OPTIONS_MIC_GAIN_DMR:// DMR Mic gain
				snprintf(buf, bufferLen, "%s:%ddB", currentLanguage->dmr_mic_gain, (nonVolatileSettings.micGainDMR - 11) * 3);
				break;
			case OPTIONS_MENU_KEYPAD_TIMER_LONG:// Timer longpress
				snprintf(buf, bufferLen, "%s:%1d.%1ds", currentLanguage->key_long, nonVolatileSettings.keypadTimerLong / 10, nonVolatileSettings.keypadTimerLong % 10);
				break;
			case OPTIONS_MENU_KEYPAD_TIMER_REPEAT:// Timer repeat
				snprintf(buf, bufferLen, "%s:%1d.%1ds", currentLanguage->key_repeat, nonVolatileSettings.keypadTimerRepeat/10, nonVolatileSettings.keypadTimerRepeat % 10);
				break;
			case OPTIONS_MENU_DMR_MONITOR_CAPTURE_TIMEOUT:// DMR filtr timeout repeat
				snprintf(buf, bufferLen, "%s:%ds", currentLanguage->dmr_filter_timeout, nonVolatileSettings.dmrCaptureTimeout);
				break;
			case OPTIONS_MENU_SCAN_DELAY:// Scan hold and pause time
				snprintf(buf, bufferLen, "%s:%ds", currentLanguage->scan_delay, nonVolatileSettings.scanDelay);
				break;
			case OPTIONS_MENU_SCAN_MODE:// scanning mode
				{
					const char *scanModes[] = { currentLanguage->hold, currentLanguage->pause, currentLanguage->stop };
					snprintf(buf, bufferLen, "%s:%s", currentLanguage->scan_mode, scanModes[nonVolatileSettings.scanModePause]);
				}
				break;
			case OPTIONS_MENU_SQUELCH_DEFAULT_VHF:
				snprintf(buf, bufferLen, "%s VHF:%d%%", currentLanguage->squelch, (nonVolatileSettings.squelchDefaults[RADIO_BAND_VHF] - 1) * 5);// 5% steps
				break;
			case OPTIONS_MENU_SQUELCH_DEFAULT_220MHz:
				snprintf(buf, bufferLen, "%s 220:%d%%", currentLanguage->squelch, (nonVolatileSettings.squelchDefaults[RADIO_BAND_220MHz] - 1) * 5);// 5% steps
				break;
			case OPTIONS_MENU_SQUELCH_DEFAULT_UHF:
				snprintf(buf, bufferLen, "%s UHF:%d%%", currentLanguage->squelch, (nonVolatileSettings.squelchDefaults[RADIO_BAND_UHF] - 1) * 5);// 5% steps
				break;
			case OPTIONS_MENU_PTT_TOGGLE:
				snprintf(buf, bufferLen, "%s:%s", currentLanguage->ptt_toggle, (nonVolatileSettings.pttToggle ? currentLanguage->on : currentLanguage->off));
				break;
			case OPTIONS_MENU_HOTSPOT_TYPE:
				{
					const char *hsTypes[] = { currentLanguage->off, "MMDVM", "BlueDV" };
					snprintf(buf, bufferLen, "Hotspot:%s", hsTypes[nonVolatileSettings.hotspotType]);
				}
				break;
			case OPTIONS_MENU_TALKER_ALIAS_TX:
				snprintf(buf, bufferLen, "TA Tx:%s",(nonVolatileSettings.transmitTalkerAlias ? currentLanguage->on : currentLanguage->off));
				break;
			case OPTIONS_MENU_PRIVATE_CALLS:
				snprintf(buf, bufferLen, "%s:%s", currentLanguage->private_call_handling, (nonVolatileSettings.privateCalls ? currentLanguage->on : currentLanguage->off));
				break;
		}

		buf[bufferLen - 1] = 0;
		menuDisplayEntry(i, mNum, buf);
	}

	ucRender();
	displayLightTrigger();
}

static void handleEvent(uiEvent_t *ev)
{
	if (KEYCHECK_PRESS(ev->keys,KEY_DOWN) && gMenusEndIndex!=0)
	{
		MENU_INC(gMenusCurrentItemIndex, NUM_OPTIONS_MENU_ITEMS);
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_UP))
	{
		MENU_DEC(gMenusCurrentItemIndex, NUM_OPTIONS_MENU_ITEMS);
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_RIGHT))
	{
		switch(gMenusCurrentItemIndex)
		{
			case OPTIONS_MENU_TIMEOUT_BEEP:
				if (nonVolatileSettings.txTimeoutBeepX5Secs < 4)
				{
					nonVolatileSettings.txTimeoutBeepX5Secs++;
				}
				break;
			case OPTIONS_MENU_FACTORY_RESET:
				doFactoryReset = true;
				break;
			case OPTIONS_MENU_USE_CALIBRATION:
				nonVolatileSettings.useCalibration=true;
				break;
			case OPTIONS_MENU_TX_FREQ_LIMITS:
				nonVolatileSettings.txFreqLimited=true;
				break;
			case OPTIONS_MENU_BEEP_VOLUME:
				if (nonVolatileSettings.beepVolumeDivider>0)
				{
					nonVolatileSettings.beepVolumeDivider--;
				}
				break;
			case OPTIONS_MIC_GAIN_DMR:// DMR Mic gain
				if (nonVolatileSettings.micGainDMR<15 )
				{
					nonVolatileSettings.micGainDMR++;
					setMicGainDMR(nonVolatileSettings.micGainDMR);
				}
				break;
			case OPTIONS_MENU_KEYPAD_TIMER_LONG:
				if (nonVolatileSettings.keypadTimerLong<90)
				{
					nonVolatileSettings.keypadTimerLong++;
				}
				break;
			case OPTIONS_MENU_KEYPAD_TIMER_REPEAT:
				if (nonVolatileSettings.keypadTimerRepeat<90)
				{
					nonVolatileSettings.keypadTimerRepeat++;
				}
				break;
			case OPTIONS_MENU_DMR_MONITOR_CAPTURE_TIMEOUT:
				if (nonVolatileSettings.dmrCaptureTimeout<90)
				{
					nonVolatileSettings.dmrCaptureTimeout++;
				}
				break;
			case OPTIONS_MENU_SCAN_DELAY:
				if (nonVolatileSettings.scanDelay<30)
				{
					nonVolatileSettings.scanDelay++;
				}
				break;
			case OPTIONS_MENU_SCAN_MODE:
				if (nonVolatileSettings.scanModePause < SCAN_MODE_STOP)
				{
					nonVolatileSettings.scanModePause++;
				}
				break;
			case OPTIONS_MENU_SQUELCH_DEFAULT_VHF:
				if (nonVolatileSettings.squelchDefaults[RADIO_BAND_VHF] < CODEPLUG_MAX_VARIABLE_SQUELCH)
				{
					nonVolatileSettings.squelchDefaults[RADIO_BAND_VHF]++;
				}
				break;
			case OPTIONS_MENU_SQUELCH_DEFAULT_220MHz:
				if (nonVolatileSettings.squelchDefaults[RADIO_BAND_220MHz] < CODEPLUG_MAX_VARIABLE_SQUELCH)
				{
					nonVolatileSettings.squelchDefaults[RADIO_BAND_220MHz]++;
				}
				break;
			case OPTIONS_MENU_SQUELCH_DEFAULT_UHF:
				if (nonVolatileSettings.squelchDefaults[RADIO_BAND_UHF] < CODEPLUG_MAX_VARIABLE_SQUELCH)
				{
					nonVolatileSettings.squelchDefaults[RADIO_BAND_UHF]++;
				}
				break;
			case OPTIONS_MENU_PTT_TOGGLE:
				nonVolatileSettings.pttToggle = true;
				break;
			case OPTIONS_MENU_HOTSPOT_TYPE:
				if (nonVolatileSettings.hotspotType < HOTSPOT_TYPE_BLUEDV)
				{
					nonVolatileSettings.hotspotType++;
				}
				break;
			case OPTIONS_MENU_TALKER_ALIAS_TX:
				nonVolatileSettings.transmitTalkerAlias = true;
				break;
			case OPTIONS_MENU_PRIVATE_CALLS:
				nonVolatileSettings.privateCalls = true;
				break;
		}
	}
	else if (KEYCHECK_PRESS(ev->keys,KEY_LEFT))
	{

		switch(gMenusCurrentItemIndex)
		{
			case OPTIONS_MENU_TIMEOUT_BEEP:
				if (nonVolatileSettings.txTimeoutBeepX5Secs>0)
				{
					nonVolatileSettings.txTimeoutBeepX5Secs--;
				}
				break;
			case OPTIONS_MENU_FACTORY_RESET:
				doFactoryReset = false;
				break;
			case OPTIONS_MENU_USE_CALIBRATION:
				nonVolatileSettings.useCalibration=false;
				break;
			case OPTIONS_MENU_TX_FREQ_LIMITS:
				nonVolatileSettings.txFreqLimited=false;
				break;
			case OPTIONS_MENU_BEEP_VOLUME:
				if (nonVolatileSettings.beepVolumeDivider<10)
				{
					nonVolatileSettings.beepVolumeDivider++;
				}
				break;
			case OPTIONS_MIC_GAIN_DMR:// DMR Mic gain
				if (nonVolatileSettings.micGainDMR>0)
				{
					nonVolatileSettings.micGainDMR--;
					setMicGainDMR(nonVolatileSettings.micGainDMR);
				}
				break;
			case OPTIONS_MENU_KEYPAD_TIMER_LONG:
				if (nonVolatileSettings.keypadTimerLong>1)
				{
					nonVolatileSettings.keypadTimerLong--;
				}
				break;
			case OPTIONS_MENU_KEYPAD_TIMER_REPEAT:
				if (nonVolatileSettings.keypadTimerRepeat>0)
				{
					nonVolatileSettings.keypadTimerRepeat--;
				}
				break;
			case OPTIONS_MENU_DMR_MONITOR_CAPTURE_TIMEOUT:
				if (nonVolatileSettings.dmrCaptureTimeout>1)
				{
					nonVolatileSettings.dmrCaptureTimeout--;
				}
				break;
			case OPTIONS_MENU_SCAN_DELAY:
				if (nonVolatileSettings.scanDelay>1)
				{
					nonVolatileSettings.scanDelay--;
				}
				break;
			case OPTIONS_MENU_SCAN_MODE:
				if (nonVolatileSettings.scanModePause > SCAN_MODE_HOLD)
				{
					nonVolatileSettings.scanModePause--;
				}
				break;
			case OPTIONS_MENU_SQUELCH_DEFAULT_VHF:
				if (nonVolatileSettings.squelchDefaults[RADIO_BAND_VHF] > 1)
				{
					nonVolatileSettings.squelchDefaults[RADIO_BAND_VHF]--;
				}
				break;
			case OPTIONS_MENU_SQUELCH_DEFAULT_220MHz:
				if (nonVolatileSettings.squelchDefaults[RADIO_BAND_220MHz] > 1)
				{
					nonVolatileSettings.squelchDefaults[RADIO_BAND_220MHz]--;
				}
				break;
			case OPTIONS_MENU_SQUELCH_DEFAULT_UHF:
				if (nonVolatileSettings.squelchDefaults[RADIO_BAND_UHF] > 1)
				{
					nonVolatileSettings.squelchDefaults[RADIO_BAND_UHF]--;
				}
				break;
			case OPTIONS_MENU_PTT_TOGGLE:
				nonVolatileSettings.pttToggle = false;
				break;
			case OPTIONS_MENU_HOTSPOT_TYPE:
				if (nonVolatileSettings.hotspotType > HOTSPOT_TYPE_OFF)
				{
					nonVolatileSettings.hotspotType--;
				}
				break;
			case OPTIONS_MENU_TALKER_ALIAS_TX:
				nonVolatileSettings.transmitTalkerAlias = false;
				break;
			case OPTIONS_MENU_PRIVATE_CALLS:
				nonVolatileSettings.privateCalls = false;
				break;
		}
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_GREEN))
	{
		if (doFactoryReset==true)
		{
			settingsRestoreDefaultSettings();
			watchdogReboot();
		}
		menuSystemPopAllAndDisplayRootMenu();
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys,KEY_RED))
	{
		// Restore original settings.
		memcpy(&nonVolatileSettings, &originalNonVolatileSettings, sizeof(settingsStruct_t));
		setMicGainDMR(nonVolatileSettings.micGainDMR);
		menuSystemPopPreviousMenu();
		return;
	}
	updateScreen();
}
