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
#ifndef _UI_LOCALISATION_H_
#define _UI_LOCALISATION_H_

#define NUM_LANGUAGES 10
#define LANGUAGE_TEXTS_LENGTH 24

typedef struct stringsTable
{
   const char LANGUAGE_NAME[LANGUAGE_TEXTS_LENGTH];//0
   const char menu[LANGUAGE_TEXTS_LENGTH];// 1
   const char credits[LANGUAGE_TEXTS_LENGTH];// 2
   const char zone[LANGUAGE_TEXTS_LENGTH];//3
   const char rssi[LANGUAGE_TEXTS_LENGTH];// 4
   const char battery[LANGUAGE_TEXTS_LENGTH];// 5
   const char contacts[LANGUAGE_TEXTS_LENGTH];// 6
   const char last_heard[LANGUAGE_TEXTS_LENGTH];// 7
   const char firmware_info[LANGUAGE_TEXTS_LENGTH];// 8
   const char options[LANGUAGE_TEXTS_LENGTH];// 9
   const char display_options[LANGUAGE_TEXTS_LENGTH];// 10
   const char channel_details[LANGUAGE_TEXTS_LENGTH];// 11
   const char language[LANGUAGE_TEXTS_LENGTH];// 12
   const char new_contact[LANGUAGE_TEXTS_LENGTH];// 13
   const char new_channel[LANGUAGE_TEXTS_LENGTH];// 14
   const char contact_list[LANGUAGE_TEXTS_LENGTH];// 15
   const char contact_details[LANGUAGE_TEXTS_LENGTH];//16
   const char hotspot_mode[LANGUAGE_TEXTS_LENGTH];//
   const char built[LANGUAGE_TEXTS_LENGTH];//
   const char zones[LANGUAGE_TEXTS_LENGTH];//
   const char keypad[LANGUAGE_TEXTS_LENGTH];//
   const char ptt[LANGUAGE_TEXTS_LENGTH];//
   const char locked[LANGUAGE_TEXTS_LENGTH];//
   const char press_blue_plus_star[LANGUAGE_TEXTS_LENGTH];//
   const char to_unlock[LANGUAGE_TEXTS_LENGTH];//
   const char unlocked[LANGUAGE_TEXTS_LENGTH];//
   const char power_off[LANGUAGE_TEXTS_LENGTH]; //
   const char error[LANGUAGE_TEXTS_LENGTH];//
   const char rx_only[LANGUAGE_TEXTS_LENGTH];//
   const char out_of_band[LANGUAGE_TEXTS_LENGTH];//
   const char timeout[LANGUAGE_TEXTS_LENGTH];//
   const char tg_entry[LANGUAGE_TEXTS_LENGTH];//
   const char pc_entry[LANGUAGE_TEXTS_LENGTH];//
   const char user_dmr_id[LANGUAGE_TEXTS_LENGTH];//
   const char contact[LANGUAGE_TEXTS_LENGTH];//
   const char accept_call[LANGUAGE_TEXTS_LENGTH];//  "Accept call?"
   const char private_call[LANGUAGE_TEXTS_LENGTH];// "Private call"
   const char squelch[LANGUAGE_TEXTS_LENGTH];// "Squelch"
   const char quick_menu[LANGUAGE_TEXTS_LENGTH];//"Quick Menu"
   const char filter[LANGUAGE_TEXTS_LENGTH];//"Filter:%s"
   const char all_channels[LANGUAGE_TEXTS_LENGTH];//"All Channels"
   const char gotoChannel[LANGUAGE_TEXTS_LENGTH];//"Goto %d"
   const char scan[LANGUAGE_TEXTS_LENGTH];// "Scan"
   const char channelToVfo[LANGUAGE_TEXTS_LENGTH];// "Channel --> VFO",
   const char vfoToChannel[LANGUAGE_TEXTS_LENGTH];// "VFO --> Channel",
   const char vfoToNewChannel[LANGUAGE_TEXTS_LENGTH];// "VFO --> New Chan",
   const char group[LANGUAGE_TEXTS_LENGTH];//"Group",
   const char private[LANGUAGE_TEXTS_LENGTH];//"Private",
   const char all[LANGUAGE_TEXTS_LENGTH];//"All",
   const char type[LANGUAGE_TEXTS_LENGTH];//"Type:"
   const char timeSlot[LANGUAGE_TEXTS_LENGTH];//"Timeslot"
   const char none[LANGUAGE_TEXTS_LENGTH];//"none"
   const char contact_saved[LANGUAGE_TEXTS_LENGTH];// "Contact saved",
   const char duplicate[LANGUAGE_TEXTS_LENGTH];//"Duplicate"
   const char tg[LANGUAGE_TEXTS_LENGTH];//"TG"
   const char pc[LANGUAGE_TEXTS_LENGTH];//"PC"
   const char ts[LANGUAGE_TEXTS_LENGTH];//"TS"
   const char mode[LANGUAGE_TEXTS_LENGTH];//"Mode"
   const char colour_code[LANGUAGE_TEXTS_LENGTH];//"Color Code"
   const char n_a[LANGUAGE_TEXTS_LENGTH];//"N/A"
   const char bandwidth[LANGUAGE_TEXTS_LENGTH];
   const char stepFreq[LANGUAGE_TEXTS_LENGTH];
   const char tot[LANGUAGE_TEXTS_LENGTH];
   const char off[LANGUAGE_TEXTS_LENGTH];
   const char zone_skip[LANGUAGE_TEXTS_LENGTH];
   const char all_skip[LANGUAGE_TEXTS_LENGTH];
   const char yes[LANGUAGE_TEXTS_LENGTH];
   const char no[LANGUAGE_TEXTS_LENGTH];
   const char rx_group[LANGUAGE_TEXTS_LENGTH];
   const char on[LANGUAGE_TEXTS_LENGTH];
   const char timeout_beep[LANGUAGE_TEXTS_LENGTH];
   const char factory_reset[LANGUAGE_TEXTS_LENGTH];
   const char calibration[LANGUAGE_TEXTS_LENGTH];
   const char band_limits[LANGUAGE_TEXTS_LENGTH];
   const char beep_volume[LANGUAGE_TEXTS_LENGTH];
   const char dmr_mic_gain[LANGUAGE_TEXTS_LENGTH];
   const char key_long[LANGUAGE_TEXTS_LENGTH];
   const char key_repeat[LANGUAGE_TEXTS_LENGTH];
   const char dmr_filter_timeout[LANGUAGE_TEXTS_LENGTH];
   const char brightness[LANGUAGE_TEXTS_LENGTH];
   const char brightness_off[LANGUAGE_TEXTS_LENGTH];
   const char contrast[LANGUAGE_TEXTS_LENGTH];
   const char colour_invert[LANGUAGE_TEXTS_LENGTH];
   const char colour_normal[LANGUAGE_TEXTS_LENGTH];
   const char backlight_timeout[LANGUAGE_TEXTS_LENGTH];
   const char scan_delay[LANGUAGE_TEXTS_LENGTH];
   const char YES[LANGUAGE_TEXTS_LENGTH];
   const char NO[LANGUAGE_TEXTS_LENGTH];
   const char DISMISS[LANGUAGE_TEXTS_LENGTH];
   const char scan_mode[LANGUAGE_TEXTS_LENGTH];
   const char hold[LANGUAGE_TEXTS_LENGTH];
   const char pause[LANGUAGE_TEXTS_LENGTH];
   const char empty_list[LANGUAGE_TEXTS_LENGTH];
   const char delete_contact_qm[LANGUAGE_TEXTS_LENGTH];
   const char contact_deleted[LANGUAGE_TEXTS_LENGTH];
   const char contact_used[LANGUAGE_TEXTS_LENGTH];
   const char in_rx_group[LANGUAGE_TEXTS_LENGTH];
   const char select_tx[LANGUAGE_TEXTS_LENGTH];
   const char edit_contact[LANGUAGE_TEXTS_LENGTH];
   const char delete_contact[LANGUAGE_TEXTS_LENGTH];
   const char group_call[LANGUAGE_TEXTS_LENGTH];
   const char all_call[LANGUAGE_TEXTS_LENGTH];
   const char tone_scan[LANGUAGE_TEXTS_LENGTH];
   const char cc_scan[LANGUAGE_TEXTS_LENGTH];
   const char low_battery[LANGUAGE_TEXTS_LENGTH];
   const char Auto[LANGUAGE_TEXTS_LENGTH];
   const char manual[LANGUAGE_TEXTS_LENGTH];
   const char ptt_toggle[LANGUAGE_TEXTS_LENGTH];
   const char private_call_handling[LANGUAGE_TEXTS_LENGTH];
   const char stop[LANGUAGE_TEXTS_LENGTH];
   const char one_line[LANGUAGE_TEXTS_LENGTH];
   const char two_lines[LANGUAGE_TEXTS_LENGTH];
} stringsTable_t;

extern const stringsTable_t languages[];
extern const stringsTable_t *currentLanguage;
#endif
