/* -*- coding: binary; -*- */
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
 * Rev: 2.2
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
.LANGUAGE_NAME 			= "estina", // MaxLen: 16
.language				= "Jazyk", // MaxLen: 16
.menu					= "Menu", // MaxLen: 16
.credits				= "PõispùvatelÈ", // MaxLen: 16
.zone					= "ZÛna", // MaxLen: 16
.rssi					= "SÌla sign·lu", // MaxLen: 16
.battery				= "Baterie", // MaxLen: 16
.contacts				= "Kontakty", // MaxLen: 16
.firmware_info			= "Firmware Info", // MaxLen: 16
.last_heard				= "PoslednÌKdoVolal", // MaxLen: 16
.options				= "NastavenÌ", // MaxLen: 16
.display_options		= "NastavenÌDisplay", // MaxLen: 16
.sound_options				= "NastavenÌ zvuku", // MaxLen: 16
.channel_details		= "Kan·l Detail", // MaxLen: 16
.new_contact			= "Nov˝ Kontakt", // MaxLen: 16
.new_channel			= "Nov˝ Kanal", // MaxLen: 16, leave room for a space and four channel digits after
.contact_list			= "Kontakt list", // MaxLen: 16
.hotspot_mode			= "Hotspot-MÛd", // MaxLen: 16
.contact_details		= "Kontakt Detail", // MaxLen: 16
.built					= "SestavenÌ", // MaxLen: 16
.zones					= "ZÛny", // MaxLen: 16
.keypad					= "Kl·vesa", // MaxLen: 12 (with .ptt)
.ptt					= "PTT", // MaxLen: 12 (with .keypad)
.locked					= "ZamÄen˝", // MaxLen: 15
.press_blue_plus_star	= "StlaÄit ModrÈ + *", // MaxLen: 19
.to_unlock				= "Odemknout", // MaxLen: 19
.unlocked				= "Odemknuto", // MaxLen: 15
.power_off				= "VypÌnanÌ...", // MaxLen: 16
.error					= "CHYBA", // MaxLen: 8
.rx_only				= "Jenom Rx", // MaxLen: 16
.out_of_band			= "MIMO P¡SMO", // MaxLen: 16
.timeout				= "asLimit", // MaxLen: 8
.tg_entry				= "TG Zad·nÌ", // MaxLen: 15
.pc_entry				= "PC Zad·nÌ", // MaxLen: 15
.user_dmr_id			= "Uûiv. DMR ID", // MaxLen: 15
.contact 				= "Kontakty", // MaxLen: 15
.accept_call			= "Põijmout hovor?", // MaxLen: 16
.private_call			= "Seznam kontakt°", // MaxLen: 16
.squelch				= "Squelch",  // MaxLen: 8
.quick_menu 			= "RychlÈ menu", // MaxLen: 16
.filter					= "Filtr", // MaxLen: 7 (with ':' + settings: "None", "CC", "CC,TS", "CC,TS,TG")
.all_channels			= "Vöechny Kan·ly", // MaxLen: 16
.gotoChannel			= "DalöÌ kan·l",  // MaxLen: 11 (" 1024")
.scan					= "Sken", // MaxLen: 16
.channelToVfo			= "Kan·l-->VFO", // MaxLen: 16
.vfoToChannel			= "VFO-->Kan·l", // MaxLen: 16
.vfoToNewChannel		= "VFO-->Nov˝ Kan·l", // MaxLen: 16
.group					= "Group", // MaxLen: 16 (with .type)
.private				= "SoukromÈ", // MaxLen: 16 (with .type)
.all					= "Vöechno", // MaxLen: 16 (with .type)
.type					= "Typ", // MaxLen: 16 (with .type)
.timeSlot				= "TimeSlot", // MaxLen: 16 (plus ':' and  .none, '1' or '2')
.none					= "NenÌ", // MaxLen: 16 (with .timeSlot, "Rx CTCSS:" and ""Tx CTCSS:")
.contact_saved			= "Kontakt uloûen", // MaxLen: 16
.duplicate				=  "Duplik·t", // MaxLen: 16
.tg						= "TG", // MaxLen: 8
.pc						= "PC", // MaxLen: 8
.ts						= "TS", // MaxLen: 8
.mode					= "MÛd",  // MaxLen: 12
.colour_code			= "Color Code", // MaxLen: 16 (with ':' * .n_a)
.n_a					= "N/A", // MaxLen: 16 (with ':' * .colour_code)
.bandwidth				= "äÌõ.P·sma", // MaxLen: 16 (with ':' + .n_a, "25kHz" or "12.5kHz")
.stepFreq				= "Krok", // MaxLen: 7 (with ':' + xx.xxkHz fitted)
.tot					= "TOT", // MaxLen: 16 (with ':' + .off or 15..3825)
.off					= "Vyp", // MaxLen: 16 (with ':' + .timeout_beep, .calibration or .band_limits)
.zone_skip				= "Skip v ZÛnù", // MaxLen: 16 (with ':' + .yes or .no) 
.all_skip				= "Skip ve Vöe", // MaxLen: 16 (with ':' + .yes or .no)
.yes					= "Ano", // MaxLen: 16 (with ':' + .zone_skip, .all_skip or .factory_reset)
.no						= "Ne", // MaxLen: 16 (with ':' + .zone_skip, .all_skip or .factory_reset)
.rx_group				= "Rx Group", // MaxLen: 16 (with ':' and codeplug group name)
.on						= "Zap", // MaxLen: 16 (with ':' + .calibration or .band_limits)
.timeout_beep			= "TÛnLimit", // MaxLen: 16 (with ':' + .off or 5..20)
.factory_reset			= "⁄pln˝-Reset", // MaxLen: 16 (with ':' + .yes or .no)
.calibration			= "Kalibrace", // MaxLen: 16 (with ':' + .on or .off)
.band_limits			= "OmezitP·smo", // MaxLen: 16 (with ':' + .on or .off)
.beep_volume			= "ZvukKl·ves", // MaxLen: 16 (with ':' + -24..6 + 'dB')
.dmr_mic_gain			= "DMR mic", // MaxLen: 16 (with ':' + -33..12 + 'dB')
.key_long				= "Drûet", // MaxLen: 11 (with ':' + x.xs fitted)
.key_repeat				= "Znovu", // MaxLen: 11 (with ':' + x.xs fitted)
.dmr_filter_timeout		= "asDMRFiltru", // MaxLen: 16 (with ':' + 1..90 + 's')
.brightness				= "Jas", // MaxLen: 16 (with ':' + 0..100 + '%')
.brightness_off			= "Min. Jas", // MaxLen: 16 (with ':' + 0..100 + '%')
.contrast				= "Kontrast", // MaxLen: 16 (with ':' + 12..30)
.colour_invert			= "Barva:ern·", // MaxLen: 16
.colour_normal			= "Barva:BÌl·", // MaxLen: 16
.backlight_timeout		= "as SvÌcenÌ", // MaxLen: 16 (with ':' + .no to 30s)
.scan_delay				= "Sken pauza", // MaxLen: 16 (with ':' + 1..30 + 's')
.YES					= "ANO", // MaxLen: 8 (choice above green/red buttons)
.NO						= "NE", // MaxLen: 8 (choice above green/red buttons)
.DISMISS				= "ODMÕTNI", // MaxLen: 8 (choice above green/red buttons)
.scan_mode				= "SkenMÛd", // MaxLen: 16 (with ':' + .hold or .pause)
.hold					= "Zastavit", // MaxLen: 16 (with ':' + .scan_mode)
.pause					= "Pauza", // MaxLen: 16 (with ':' + .scan_mode)
.empty_list				= "Pr·zdn˝ seznam", // MaxLen: 16
.delete_contact_qm		= "Smazat kontakt?", // MaxLen: 16
.contact_deleted		= "Kontakt smaz·n", // MaxLen: 16
.contact_used			= "PouûÌt kontakt", // MaxLen: 16
.in_rx_group			= "v RX Group", // MaxLen: 16
.select_tx				= "Vybrat TX", // MaxLen: 16
.edit_contact			= "Upravit kontakt", // MaxLen: 16
.delete_contact			= "Smazat Kontakt", // MaxLen: 16
.group_call				= "Seznam Group", // MaxLen: 16
.all_call				= "Vöechny ÄÌsla", // MaxLen: 16
.tone_scan				= "CTCSS Sken",//// MaxLen: 16
.cc_scan				= "CC Sken",//// MaxLen: 16
.low_battery			= "SLAB¡ BATERIE!",//// MaxLen: 16
.Auto					= "Automat.", // MaxLen 16 (with .mode + ':') 
.manual					= "ManualnÌ",  // MaxLen 16 (with .mode + ':') 
.ptt_toggle				= "PTT põepnout", // MaxLen 16 (with ':' + .on or .off)
.private_call_handling		= "Soukr.Vol·nÌ", // MaxLen 16 (with ':' + .on ot .off)
.stop					= "stop", // Maxlen 16 (with ':' + .scan_mode)
.one_line				= "1 õ·dek", // MaxLen 16 (with ':' + .contact)
.two_lines				= "2 õ·dky", // MaxLen 16 (with ':' + .contact)
.priority_order				= "Ìst ID", // MaxLen 16 (with ':' + 'Cc/DB/TA')
.dmr_beep				= "pÌpDMR", // MaxLen 16 (with ':' + .star/.stop/.both/.none)
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
