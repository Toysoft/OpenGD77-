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
 * Translators: F1CXG, F1RMB
 *
 *
 * Rev: 3
 */
#ifndef USER_INTERFACE_LANGUAGES_FRENCH_H_
#define USER_INTERFACE_LANGUAGES_FRENCH_H_
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
const stringsTable_t frenchLanguage =
{
.LANGUAGE_NAME			= "Fran�ais",
.language				= "Langue",
.menu					= "Menu",
.credits				= "Cr�dits",
.zone					= "Zone",
.rssi					= "RSSI",
.battery				= "Batterie",
.contacts				= "Contacts",
.firmware_info			= "Info Firmware",
.last_heard				= "Derniers re�us",
.options				= "Options",
.display_options		= "Options aff.",
.sound_options				= "Options son", // MaxLen: 16
.channel_details		= "D�tails canal",
.new_contact			= "Nouv. contact",
.new_channel			= "New channel", // MaxLen: 16, leave room for a space and four channel digits after
.contact_list			= "Liste contacts",
.hotspot_mode			= "Mode hotspot",
.contact_details		= "D�tails contact",
.built					= "Cr��",
.zones					= "Zones",
.keypad					= "Clavier",
.ptt					= "PTT",
.locked					= "Verrouill�",
.press_blue_plus_star	= "Pressez Bleu + *",
.to_unlock				= "pour d�verrouiller",
.unlocked				= "D�verrouill�",
.power_off				= "Extinction...",
.error					= "ERREUR",
.rx_only				= "Rx uniqmnt.",
.out_of_band			= "HORS BANDE",
.timeout				= "TIMEOUT",
.tg_entry				= "Entrez TG",
.pc_entry				= "Entrez PC",
.user_dmr_id			= "DMR ID perso.",
.contact 				= "Contact",
.accept_call			= "Accepter Appel ?",
.private_call			= "Appel Priv�",
.squelch				= "Squelch",
.quick_menu 			= "Menu rapide",
.filter					= "Filtre",
.all_channels			= "Tous Canaux",
.gotoChannel			= "Aller",
.scan					= "Scan",
.channelToVfo			= "Canal --> VFO",
.vfoToChannel			= "VFO --> Canal",
.vfoToNewChannel		= "VFO --> New Chan", // MaxLen: 16
.group					= "Groupe",
.private				= "Priv�",
.all					= "Tous",
.type					= "Type",
.timeSlot				= "Timeslot",
.none					= "Aucun",
.contact_saved			= "Contact sauv�",
.duplicate				= "Dupliqu�",
.tg						= "TG",
.pc						= "PC",
.ts						= "TS",
.mode					= "Mode",
.colour_code			= "Code Couleur",
.n_a					= "ND",
.bandwidth				= "Larg. bde",
.stepFreq				= "Pas",
.tot					= "TOT",
.off					= "Off",
.zone_skip				= "Saut Zone",
.all_skip				= "Saut Compl.",
.yes					= "Oui",
.no						= "Non",
.rx_group				= "Grp Rx",
.on						= "On",
.timeout_beep			= "Son timeout",
.factory_reset			= "R�z Usine",
.calibration			= "�talonnage",
.band_limits			= "Lim. Bandes",
.beep_volume			= "Vol. bip",
.dmr_mic_gain			= "DMR mic",
.key_long				= "Appui long",
.key_repeat				= "R�p�t�",
.dmr_filter_timeout		= "Tps filtre",
.brightness				= "R�tro �cl.",
.brightness_off				= "�cl. min",
.contrast				= "Contraste",
.colour_invert			= "Couleur:Inverse",
.colour_normal			= "Couleur:Normale",
.backlight_timeout		= "Timeout",
.scan_delay				= "D�lai scan",
.YES					= "OUI",
.NO						= "NON",
.DISMISS				= "CACHER",
.scan_mode				= "Mode scan",
.hold					= "Bloque",
.pause					= "Pause",
.empty_list				= "Liste Vide",
.delete_contact_qm			= "Efface contact ?",
.contact_deleted			= "Contact effac�",
.contact_used				= "Contact utilis�",
.in_rx_group				= "ds le groupe RX",
.select_tx				= "Select� TX",
.edit_contact				= "�dite Contact",
.delete_contact				= "Efface Contact",
.group_call				= "Appel de Groupe",
.all_call				= "All Call",
.tone_scan				= "Scan tons",
.cc_scan				= "Scan CC",
.low_battery			        = "BATT. FAIBLE !!!",//// MaxLen: 16
.Auto					= "Auto", // MaxLen 16 (with .mode + ':') 
.manual					= "Manuel",  // MaxLen 16 (with .mode + ':') }
.ptt_toggle				= "Bascule PTT", // MaxLen 16 (with ':' + .on or .off)
.private_call_handling			= "Filtre PC", // MaxLen 16 (with ':' + .on ot .off)
.stop					= "Arr�t", // Maxlen 16 (with ':' + .scan_mode)
.one_line				= "1 ligne", // MaxLen 16 (with ':' + .contact)
.two_lines				= "2 lignes", // MaxLen 16 (with ':' + .contact)
.priority_order				= "Ordre", // MaxLen 16 (with ':' + 'Cc/DB/TA')
.dmr_beep				= "Bip TX", // MaxLen 16 (with ':' + .star/.stop/.both/.none)
.start					= "D�but", // MaxLen 16 (with ':' + .dmr_beep)
.both					= "Les Deux" // MaxLen 16 (with ':' + .dmr_beep)
};
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
#endif /* USER_INTERFACE_LANGUAGES_FRENCH_H_ */
