/* -*- coding: windows-1252-unix; -*- */
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
/*
 * Translators: OK2HAD
 *
 *
 * Rev: 1.1
 */
#ifndef USER_INTERFACE_LANGUAGES_CZECH_H_
#define USER_INTERFACE_LANGUAGES_CZECH_H_
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
const stringsTable_t czechLanguage =
{
.LANGUAGE_NAME 			= "Cestina", // MaxLen: 16
.language				= "Jazyk", // MaxLen: 16
.menu					= "Menu", // MaxLen: 16
.credits				= "Prispevatele", // MaxLen: 16
.zone					= "Zona", // MaxLen: 16
.rssi					= "Sila signalu", // MaxLen: 16
.battery				= "Baterie", // MaxLen: 16
.contacts				= "Kontakt", // MaxLen: 16
.firmware_info			= "Firmware Info", // MaxLen: 16
.last_heard				= "Posledni volajici", // MaxLen: 16
.options				= "Nastaveni", // MaxLen: 16
.display_options		= "NastaveniDisplay", // MaxLen: 16
.sound_options				= "Nastaveni zvuku", // MaxLen: 16
.channel_details		= "Kanal Detail", // MaxLen: 16
.new_contact			= "Novy Kontakt", // MaxLen: 16
.new_channel			= "Novy Kanal", // MaxLen: 16, leave room for a space and four channel digits after
.contact_list			= "Kontakt list", // MaxLen: 16
.hotspot_mode			= "Hotspot-Mod", // MaxLen: 16
.contact_details		= "Kontakt Detail", // MaxLen: 16
.built					= "Sestaveni", // MaxLen: 16
.zones					= "Zony", // MaxLen: 16
.keypad					= "Klavesa", // MaxLen: 12 (with .ptt)
.ptt					= "PTT", // MaxLen: 12 (with .keypad)
.locked					= "Zamceny", // MaxLen: 15
.press_blue_plus_star	= "Stlacit Modre + *", // MaxLen: 19
.to_unlock				= "odemknout", // MaxLen: 19
.unlocked				= "Odemknuto", // MaxLen: 15
.power_off				= "Vypinani...", // MaxLen: 16
.error					= "CHYBA", // MaxLen: 8
.rx_only				= "Jenom Rx", // MaxLen: 16
.out_of_band			= "MIMO PASMO", // MaxLen: 16
.timeout				= "CasLimit", // MaxLen: 8
.tg_entry				= "TG Zadani", // MaxLen: 15
.pc_entry				= "PC Zadani", // MaxLen: 15
.user_dmr_id			= "Uziv. DMR ID", // MaxLen: 15
.contact 				= "Kontakt", // MaxLen: 15
.accept_call			= "Prijmout hovor?", // MaxLen: 16
.private_call			= "Soukromy Hovor", // MaxLen: 16
.squelch				= "Squelch",  // MaxLen: 8
.quick_menu 			= "Rychle menu", // MaxLen: 16
.filter					= "Filtr", // MaxLen: 7 (with ':' + settings: "None", "CC", "CC,TS", "CC,TS,TG")
.all_channels			= "Vsechny Kanaly", // MaxLen: 16
.gotoChannel			= "Dalsi kanal",  // MaxLen: 11 (" 1024")
.scan					= "Sken", // MaxLen: 16
.channelToVfo			= "Kanal-->VFO", // MaxLen: 16
.vfoToChannel			= "VFO-->Kanal", // MaxLen: 16
.vfoToNewChannel		= "VFO-->Novy Kanal", // MaxLen: 16
.group					= "Group", // MaxLen: 16 (with .type)
.private				= "Soukrome", // MaxLen: 16 (with .type)
.all					= "Vsechno", // MaxLen: 16 (with .type)
.type					= "Typ", // MaxLen: 16 (with .type)
.timeSlot				= "TimeSlot", // MaxLen: 16 (plus ':' and  .none, '1' or '2')
.none					= "Neni", // MaxLen: 16 (with .timeSlot, "Rx CTCSS:" and ""Tx CTCSS:")
.contact_saved			= "Kontakt ulozen", // MaxLen: 16
.duplicate				=  "Duplikat", // MaxLen: 16
.tg						= "TG", // MaxLen: 8
.pc						= "PC", // MaxLen: 8
.ts						= "TS", // MaxLen: 8
.mode					= "Mod",  // MaxLen: 12
.colour_code			= "Color Code", // MaxLen: 16 (with ':' * .n_a)
.n_a					= "N/A", // MaxLen: 16 (with ':' * .colour_code)
.bandwidth				= "Sir.Pasma", // MaxLen: 16 (with ':' + .n_a, "25kHz" or "12.5kHz")
.stepFreq				= "Krok", // MaxLen: 7 (with ':' + xx.xxkHz fitted)
.tot					= "TOT", // MaxLen: 16 (with ':' + .off or 15..3825)
.off					= "Vyp", // MaxLen: 16 (with ':' + .timeout_beep, .calibration or .band_limits)
.zone_skip				= "Skip v Zone", // MaxLen: 16 (with ':' + .yes or .no) 
.all_skip				= "Skip ve Vse", // MaxLen: 16 (with ':' + .yes or .no)
.yes					= "Ano", // MaxLen: 16 (with ':' + .zone_skip, .all_skip or .factory_reset)
.no						= "Ne", // MaxLen: 16 (with ':' + .zone_skip, .all_skip or .factory_reset)
.rx_group				= "Rx Group", // MaxLen: 16 (with ':' and codeplug group name)
.on						= "Zap", // MaxLen: 16 (with ':' + .calibration or .band_limits)
.timeout_beep			= "TonLimit", // MaxLen: 16 (with ':' + .off or 5..20)
.factory_reset			= "Uplny-Reset", // MaxLen: 16 (with ':' + .yes or .no)
.calibration			= "Kalibrace", // MaxLen: 16 (with ':' + .on or .off)
.band_limits			= "OmezitPasmo", // MaxLen: 16 (with ':' + .on or .off)
.beep_volume			= "ZvukKlaves", // MaxLen: 16 (with ':' + -24..6 + 'dB')
.dmr_mic_gain			= "DMR mic", // MaxLen: 16 (with ':' + -33..12 + 'dB')
.key_long				= "Drz", // MaxLen: 11 (with ':' + x.xs fitted)
.key_repeat				= "Znovu", // MaxLen: 11 (with ':' + x.xs fitted)
.dmr_filter_timeout		= "CasDMRFiltru", // MaxLen: 16 (with ':' + 1..90 + 's')
.brightness				= "Jas", // MaxLen: 16 (with ':' + 0..100 + '%')
.brightness_off			= "Min. Jas", // MaxLen: 16 (with ':' + 0..100 + '%')
.contrast				= "Kontrast", // MaxLen: 16 (with ':' + 12..30)
.colour_invert			= "Barva:Cerna", // MaxLen: 16
.colour_normal			= "Barva:Bila", // MaxLen: 16
.backlight_timeout		= "Cas Sviceni", // MaxLen: 16 (with ':' + .no to 30s)
.scan_delay				= "Sken pauza", // MaxLen: 16 (with ':' + 1..30 + 's')
.YES					= "ANO", // MaxLen: 8 (choice above green/red buttons)
.NO						= "NE", // MaxLen: 8 (choice above green/red buttons)
.DISMISS				= "ODMITNI", // MaxLen: 8 (choice above green/red buttons)
.scan_mode				= "SkenMod", // MaxLen: 16 (with ':' + .hold or .pause)
.hold					= "Zastavit", // MaxLen: 16 (with ':' + .scan_mode)
.pause					= "Pauza", // MaxLen: 16 (with ':' + .scan_mode)
.empty_list				= "Prazdny seznam", // MaxLen: 16
.delete_contact_qm		= "Smazat kontakt?", // MaxLen: 16
.contact_deleted		= "Kontakt smazan", // MaxLen: 16
.contact_used			= "Pouzit kontakt", // MaxLen: 16
.in_rx_group			= "v RX Group", // MaxLen: 16
.select_tx				= "Vybrat TX", // MaxLen: 16
.edit_contact			= "Upravit kontakt", // MaxLen: 16
.delete_contact			= "Smazat Kontakt", // MaxLen: 16
.group_call				= "Group cisla", // MaxLen: 16
.all_call				= "Vsechny cisla", // MaxLen: 16
.tone_scan				= "CTCSS Sken",//// MaxLen: 16
.cc_scan				= "CC Sken",//// MaxLen: 16
.low_battery			= "SLABA BATERIE!",//// MaxLen: 16
.Auto					= "Automaticky", // MaxLen 16 (with .mode + ':') 
.manual					= "Manualni",  // MaxLen 16 (with .mode + ':') 
.ptt_toggle				= "PTT prepnout", // MaxLen 16 (with ':' + .on or .off)
.private_call_handling		= "Soukr.Volani", // MaxLen 16 (with ':' + .on ot .off)
.stop					= "stop", // Maxlen 16 (with ':' + .scan_mode)
.one_line				= "1 radek", // MaxLen 16 (with ':' + .contact)
.two_lines				= "2 radky", // MaxLen 16 (with ':' + .contact)
.priority_order				= "Cist ID", // MaxLen 16 (with ':' + 'Cc/DB/TA')
.dmr_beep				= "pipDMR", // MaxLen 16 (with ':' + .star/.stop/.both/.none)
.start					= "start", // MaxLen 16 (with ':' + .dmr_beep)
.both					= "StartStop" // MaxLen 16 (with ':' + .dmr_beep)
};
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
#endif /* USER_INTERFACE_LANGUAGES_CZECH_H  */
