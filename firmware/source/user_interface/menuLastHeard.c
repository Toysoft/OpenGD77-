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
#include <user_interface/uiUtilities.h>
#include <user_interface/uiLocalisation.h>
#include <functions/fw_ticks.h>

static const int LAST_HEARD_NUM_LINES_ON_DISPLAY = 3;
static bool displayLHDetails = false;

static void handleEvent(uiEvent_t *ev);
static void menuLastHeardDisplayTA(uint8_t y, char *text, uint32_t time, uint32_t now, uint32_t TGorPC, size_t maxLen, bool displayDetails);

int menuLastHeard(uiEvent_t *ev, bool isFirstRun)
{
	static uint32_t m = 0;

	if (isFirstRun)
	{
		gMenusStartIndex = LinkHead->id;// reuse this global to store the ID of the first item in the list
		gMenusEndIndex = 0;
		displayLHDetails = false;
		menuLastHeardUpdateScreen(true, displayLHDetails);
		m = ev->time;
	}
	else
	{
		// do live update by checking if the item at the top of the list has changed
		if ((gMenusStartIndex != LinkHead->id) || (menuDisplayQSODataState == QSO_DISPLAY_CALLER_DATA))
		{
			gMenusStartIndex = LinkHead->id;
			gMenusCurrentItemIndex = 0;
			gMenusEndIndex = 0;
			menuLastHeardUpdateScreen(true, displayLHDetails);
		}

		if (ev->hasEvent)
		{
			m = ev->time;
			handleEvent(ev);
		}
		else
		{
			// Just refresh the list while displaying details, it's all about elapsed time
			if (displayLHDetails && ((ev->time - m) > (1000U * 60U)))
			{
				m = ev->time;
				menuLastHeardUpdateScreen(true, true);
			}
		}

	}
	return 0;
}

void menuLastHeardUpdateScreen(bool showTitleOrHeader, bool displayDetails)
{
	dmrIdDataStruct_t foundRecord;
	int numDisplayed = 0;
	LinkItem_t *item = LinkHead;
	uint32_t now = fw_millis();

	ucClearBuf();
	if (showTitleOrHeader)
	{
		menuDisplayTitle(currentLanguage->last_heard);
	}
	else
	{
		menuUtilityRenderHeader();
	}

	// skip over the first gMenusCurrentItemIndex in the listing
	for(int i = 0; i < gMenusCurrentItemIndex; i++)
	{
		item = item->next;
	}

	while((item != NULL) && (item->id != 0))
	{
		if (dmrIDLookup(item->id, &foundRecord))
		{
			menuLastHeardDisplayTA(16 + (numDisplayed * 16), foundRecord.text, item->time, now, item->talkGroupOrPcId, 20, displayDetails);
		}
		else
		{
			if (item->talkerAlias[0] != 0x00)
			{
				menuLastHeardDisplayTA(16 + (numDisplayed * 16), item->talkerAlias, item->time, now, item->talkGroupOrPcId, 32, displayDetails);
			}
			else
			{
				char buffer[17];

				snprintf(buffer, 17, "ID:%d", item->id);
				buffer[16] = 0;
				menuLastHeardDisplayTA(16 + (numDisplayed * 16), buffer, item->time, now, item->talkGroupOrPcId, 17, displayDetails);
			}
		}

		numDisplayed++;

		item = item->next;
		if (numDisplayed > (LAST_HEARD_NUM_LINES_ON_DISPLAY - 1))
		{
			if ((item != NULL) && (item->id != 0))
			{
				gMenusEndIndex = 1;
			}
			else
			{
				gMenusEndIndex = 0;
			}
			break;
		}
	}
	ucRender();
	menuDisplayQSODataState = QSO_DISPLAY_IDLE;
}

static void handleEvent(uiEvent_t *ev)
{
	displayLightTrigger();

	if (KEYCHECK_PRESS(ev->keys, KEY_DOWN) && (gMenusEndIndex != 0))
	{
		gMenusCurrentItemIndex++;
	}
	else if (KEYCHECK_PRESS(ev->keys, KEY_UP))
	{
		if (gMenusCurrentItemIndex > 0)
		{
			gMenusCurrentItemIndex--;
		}
	}
	else if (KEYCHECK_SHORTUP(ev->keys, KEY_RED))
	{
		menuSystemPopPreviousMenu();
		return;
	}
	else if (KEYCHECK_SHORTUP(ev->keys, KEY_GREEN))
	{
		menuSystemPopAllAndDisplayRootMenu();
		return;
	}

	// Toggles LH simple/details view on SK2 press
	if (ev->buttons & BUTTON_SK2)
	{
		displayLHDetails = true;//!displayLHDetails;
	}
	else
	{
		displayLHDetails = false;
	}

	menuLastHeardUpdateScreen(true, displayLHDetails);
}

static void menuLastHeardDisplayTA(uint8_t y, char *text, uint32_t time, uint32_t now, uint32_t TGorPC, size_t maxLen, bool displayDetails)
{
	char buffer[37]; // Max: TA 27 (in 7bit format) + ' [' + 6 (Maidenhead)  + ']' + NULL

	if (displayDetails)
	{
		uint32_t diffTimeInMins = (((now - time) / 1000U) / 60U);
		uint32_t tg = TGorPC & 0xFFFFFF;

		// PC or TG
		sprintf(buffer, "%s %u", (((TGorPC >> 24) == PC_CALL_FLAG) ? "PC" : "TG"), tg);
		ucPrintAt(0, y, buffer, FONT_8x16);

		// Time
		snprintf(buffer, 5, "%d", diffTimeInMins);
		buffer[5] = 0;

		ucPrintAt((128 - (3 * 6)), (y + 6), "min", FONT_6x8);
		ucPrintAt((128 - (strlen(buffer) * 8) - (3 * 6) - 1), y, buffer, FONT_8x16);

	}
	else // search for callsign + first name
	{
		if (strlen(text) >= 5)
		{
			int32_t  cpos;

			if ((cpos = getFirstSpacePos(text)) != -1) // Callsign found
			{
				// Check if second part is 'DMR ID:'
				// In this case, don't go further
				if (strncmp((text + cpos + 1), "DMR ID:", 7) == 0)
				{
					if (cpos > 15)
						cpos = 16;

					memcpy(buffer, text, cpos);
					buffer[cpos] = 0;

					ucPrintCentered(y, chomp(buffer), FONT_8x16);
				}
				else // Nope, look for first name
				{
					uint32_t npos;
					char nameBuf[17];
					char outputBuf[17];

					memset(nameBuf, 0, sizeof(nameBuf));

					// Look for the second part, aka first name
					if ((npos = getFirstSpacePos(text + cpos + 1)) != -1)
					{
						// Store callsign
						memcpy(buffer, text, cpos);
						buffer[cpos] = 0;

						// Store 'first name'
						memcpy(nameBuf, (text + cpos + 1), npos);
						nameBuf[npos] = 0;

						snprintf(outputBuf, 16, "%s %s", chomp(buffer), chomp(nameBuf));
						outputBuf[16] = 0;

						ucPrintCentered(y, chomp(outputBuf), FONT_8x16);
					}
					else
					{
						// Store callsign
						memcpy(buffer, text, cpos);
						buffer[cpos] = 0;

						memcpy(nameBuf, (text + cpos + 1), strlen(text) - cpos - 1);
						nameBuf[16] = 0;

						snprintf(outputBuf, 16, "%s %s", chomp(buffer), chomp(nameBuf));
						outputBuf[16] = 0;

						ucPrintCentered(y, chomp(outputBuf), FONT_8x16);
					}
				}
			}
			else
			{
				// No space found, use a chainsaw
				memcpy(buffer, text, 16);
				buffer[16] = 0;

				ucPrintCentered(y, chomp(buffer), FONT_8x16);
			}
		}
		else // short callsign
		{
			memcpy(buffer, text, strlen(text));
			buffer[strlen(text)] = 0;

			ucPrintCentered(y, chomp(buffer), FONT_8x16);
		}
	}
}

