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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Translators: EA3IGM, EA5SW, EB3AM, EA3BIL
 *
 *
 * Rev: 5
 */
#ifndef USER_INTERFACE_LANGUAGES_CATALAN_H_
#define USER_INTERFACE_LANGUAGES_CATALAN_H_
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
const stringsTable_t catalanLanguage=
{
.LANGUAGE_NAME			= "Català",
.language				= "Idioma",
.menu					= "Menú",
.credits				= "Crèdits",
.zone					= "Zona",
.rssi					= "Senyal",
.battery				= "Batería",
.contacts				= "Contactes",
.firmware_info			= "Info firmware",
.last_heard				= "Escoltats",
.options				= "Opcions",
.display_options		= "Opcions pantalla",
.sound_options			= "Opcions de só", // MaxLen: 16
.channel_details		= "Detalls Canal",
.new_contact			= "Nou contacte",
.new_channel			= "Nou canal", // MaxLen: 16, leave room for a space and four channel digits after
.contact_list			= "Contactes",
.hotspot_mode			= "Mode Hotspot",
.contact_details		= "Detall Ctte",
.built					= "Compilat",
.zones					= "Zones",
.keypad					= "Teclat",
.ptt					= "PTT",
.locked					= "blocat",
.press_blue_plus_star	= "Prem Blau + (*)",
.to_unlock				= "per desblocar",
.unlocked				= "Desblocat",
.power_off				= "Apagant...",
.error					= "ERROR",
.rx_only				= "Només RX",
.out_of_band			= "FORA DE BANDA",
.timeout				= "TIMEOUT",
.tg_entry				= "Entreu TG",
.pc_entry				= "Entreu ID",
.user_dmr_id			= "ID DMR d'Usuari",
.contact				= "Contacte",
.accept_call			= "Acceptar trucada?",
.private_call			= "Trucada privada",
.squelch				= "Squelch",
.quick_menu				= "Menú Ràpid",
.filter					= "Filtre",
.all_channels			= "Llista total",
.gotoChannel			= "Anar al canal",
.scan					= "Scan",
.channelToVfo			= "Canal -> VFO",
.vfoToChannel			= "VFO -> Canal",
.vfoToNewChannel		= "VFO -> Nou Canal", // MaxLen: 16
.group					= "Grup",
.private				= "Privada",
.all					= "Tots",
.type					= "Tipus",
.timeSlot				= "TS",
.none					= "Cap",
.contact_saved			= "Contacte Desat",
.duplicate				= "Duplicat",
.tg						= "TG",
.pc						= "ID",
.ts						= "TS",
.mode					= "Mode",
.colour_code			= "Codi Color",
.n_a					= "N/D",
.bandwidth				= "Ample banda",
.stepFreq				= "Salt",
.tot					= "TOT",
.off					= "NO",
.zone_skip				= "Saltar zona",
.all_skip				= "Saltar tot",
.yes					= "Sí",
.no						= "No",
.rx_group				= "Llista TG/ID",
.on						= "Sí",
.timeout_beep			= "Avís T.O.T.",
.factory_reset			= "Reinici fabr.",
.calibration			= "Calibració",
.band_limits			= "Límit bandes",
.beep_volume			= "Volum tons",
.dmr_mic_gain			= "Mic DMR",
.key_long				= "Prem llarg",
.key_repeat				= "Prem rpt",
.dmr_filter_timeout		= "Filtre temps",
.brightness				= "Brillantor",
.brightness_off			= "Brillan. min",
.contrast				= "Contrast",
.colour_invert			= "Color:Invers",
.colour_normal			= "Color:Normal",
.backlight_timeout		= "Temps llum",
.scan_delay				= "Temps Scan",
.YES					= "Sí",
.NO						= "No",
.DISMISS				= "PASSAR",
.scan_mode				= "Mode Scan",
.hold					= "Parar",
.pause					= "Pausa",
.empty_list				= "Llista buida",
.delete_contact_qm		= "Esborrar ctte?",
.contact_deleted		= "Ctte esborrat",
.contact_used			= "Ctte en ús a",
.in_rx_group			= "la Llista TG/ID",
.select_tx				= "Selec. TX",
.edit_contact			= "Editar Ctte",
.delete_contact			= "Esborrar Ctte",
.group_call				= "Cridar a Grup",
.all_call				= "Cridar a Tots",
.tone_scan				= "Scan CCTCS",//// MaxLen: 16
.cc_scan				= "Scan CColor",//// MaxLen: 16
.low_battery			= "BATERIA BAIXA !!",//// MaxLen: 16
.Auto					= "Auto", // MaxLen 16 (with .mode + ':') 
.manual					= "Manual",  // MaxLen 16 (with .mode + ':') 
.ptt_toggle				= "Enclavar PTT", // MaxLen 16 (with ':' + .on or .off)
.private_call_handling	= "Gestió PC", // MaxLen 16 (with ':' + .on ot .off)
.stop					= "Stop", // Maxlen 16 (with ':' + .scan_mode)
.one_line				= "1 línia", // MaxLen 16 (with ':' + .contact)
.two_lines				= "2 línies", // MaxLen 16 (with ':' + .contact)
.priority_order			= "Prio.", // MaxLen 16 (with ':' + 'Cc/DB/TA')
.dmr_beep				= "DMR Beep", // MaxLen 16 (with ':' + .star/.stop/.both/.none)
.start					= "Inici", // MaxLen 16 (with ':' + .dmr_beep)
.both					= "Tots" // MaxLen 16 (with ':' + .dmr_beep)
};
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
#endif /* USER_INTERFACE_LANGUAGES_CATALAN_H_ */
