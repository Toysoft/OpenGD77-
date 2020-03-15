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

#define NUM_LANGUAGES 12
#define LANGUAGE_TEXTS_LENGTH 17

typedef struct stringsTable
{
// Fixed length texts used in the menu system. These are treated as an array and MUST be fixed length
   const char LANGUAGE_NAME[LANGUAGE_TEXTS_LENGTH];// Menu number 0
   const char menu[LANGUAGE_TEXTS_LENGTH];// Menu number  1
   const char credits[LANGUAGE_TEXTS_LENGTH];// Menu number  2
   const char zone[LANGUAGE_TEXTS_LENGTH];// Menu number 3
   const char rssi[LANGUAGE_TEXTS_LENGTH];// Menu number  4
   const char battery[LANGUAGE_TEXTS_LENGTH];// Menu number  5
   const char contacts[LANGUAGE_TEXTS_LENGTH];// Menu number  6
   const char last_heard[LANGUAGE_TEXTS_LENGTH];// Menu number  7
   const char firmware_info[LANGUAGE_TEXTS_LENGTH];// Menu number  8
   const char options[LANGUAGE_TEXTS_LENGTH];// Menu number  9
   const char display_options[LANGUAGE_TEXTS_LENGTH];// Menu number  10
   const char sound_options[LANGUAGE_TEXTS_LENGTH];// Menu number  11
   const char channel_details[LANGUAGE_TEXTS_LENGTH];// Menu number  12
   const char language[LANGUAGE_TEXTS_LENGTH];// Menu number  13
   const char new_contact[LANGUAGE_TEXTS_LENGTH];// Menu number  14
   const char contact_list[LANGUAGE_TEXTS_LENGTH];// Menu number  15
   const char contact_details[LANGUAGE_TEXTS_LENGTH];// Menu number 16
   const char hotspot_mode[LANGUAGE_TEXTS_LENGTH];// Menu number 17

// Variable length texts
   const char *built;//
   const char *zones;//
   const char *keypad;//
   const char *ptt;//
   const char *locked;//
   const char *press_blue_plus_star;//
   const char *to_unlock;//
   const char *unlocked;//
   const char *power_off; //
   const char *error;//
   const char *rx_only;//
   const char *out_of_band;//
   const char *timeout;//
   const char *tg_entry;//
   const char *pc_entry;//
   const char *user_dmr_id;//
   const char *contact;//
   const char *accept_call;//  "Accept call?"
   const char *private_call;// "Private call"
   const char *squelch;// "Squelch"
   const char *quick_menu;//"Quick Menu"
   const char *filter;//"Filter:%s"
   const char *all_channels;//"All Channels"
   const char *gotoChannel;//"Goto %d"
   const char *scan;// "Scan"
   const char *channelToVfo;// "Channel --> VFO",
   const char *vfoToChannel;// "VFO --> Channel",
   const char *vfoToNewChannel;// "VFO --> New Chan",
   const char *group;//"Group",
   const char *private;//"Private",
   const char *all;//"All",
   const char *type;//"Type:"
   const char *timeSlot;//"Timeslot"
   const char *none;//"none"
   const char *contact_saved;// "Contact saved",
   const char *duplicate;//"Duplicate"
   const char *tg;//"TG"
   const char *pc;//"PC"
   const char *ts;//"TS"
   const char *mode;//"Mode"
   const char *colour_code;//"Color Code"
   const char *n_a;//"N/A"
   const char *bandwidth;
   const char *stepFreq;
   const char *tot;
   const char *off;
   const char *zone_skip;
   const char *all_skip;
   const char *yes;
   const char *no;
   const char *rx_group;
   const char *on;
   const char *timeout_beep;
   const char *factory_reset;
   const char *calibration;
   const char *band_limits;
   const char *beep_volume;
   const char *dmr_mic_gain;
   const char *key_long;
   const char *key_repeat;
   const char *dmr_filter_timeout;
   const char *brightness;
   const char *brightness_off;
   const char *contrast;
   const char *colour_invert;
   const char *colour_normal;
   const char *backlight_timeout;
   const char *scan_delay;
   const char *YES;
   const char *NO;
   const char *DISMISS;
   const char *scan_mode;
   const char *hold;
   const char *pause;
   const char *empty_list;
   const char *delete_contact_qm;
   const char *contact_deleted;
   const char *contact_used;
   const char *in_rx_group;
   const char *select_tx;
   const char *edit_contact;
   const char *delete_contact;
   const char *group_call;
   const char *all_call;
   const char *tone_scan;
   const char *cc_scan;
   const char *low_battery;
   const char *Auto;
   const char *manual;
   const char *ptt_toggle;
   const char *private_call_handling;
   const char *stop;
   const char *one_line;
   const char *two_lines;
   const char *new_channel;
   const char *priority_order;
   const char *dmr_beep;
   const char *start;
   const char *both;
} stringsTable_t;

extern const stringsTable_t languages[];
extern const stringsTable_t *currentLanguage;
#endif
