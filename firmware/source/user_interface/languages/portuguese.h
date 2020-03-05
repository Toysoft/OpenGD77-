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
 * Translators: CT4TX, CT1HSN
 *
 *
 * Rev: 2
 */
#ifndef USER_INTERFACE_LANGUAGES_PORTUGUESE_H_
#define USER_INTERFACE_LANGUAGES_PORTUGUESE_H_
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
const stringsTable_t portuguesLanguage =
{
.LANGUAGE_NAME 			= "Portugues",
.language				= "Lingua",
.menu					= "Menu",
.credits				= "Creditos",
.zone					= "Zona",
.rssi					= "RSSI",
.battery				= "Bateria",
.contacts				= "Contactos",
.firmware_info			= "Versao Firmware",
.last_heard				= "Ultima escutada",
.options				= "Opcoes",
.display_options		= "Opcoes Visor",
.sound_options				= "Sound options", // MaxLen: 16
.channel_details		= "Detalhes Canal",
.new_contact			= "Contacto Novo",
.new_channel			= "Canal novo", // MaxLen: 16, leave room for a space and four channel digits after
.contact_list			= "Lista Contactos",
.hotspot_mode			= "Modo Hotspot",
.contact_details		= "Detalhes Contato",
.built					= "Built",
.zones					= "Zonas",
.keypad					= "Teclado",
.ptt 					= "PTT",
.locked 				= "Bloqueado",
.press_blue_plus_star	="Prima Azul + *",
.to_unlock				= "Para Desbloquear",
.unlocked				= "Desbloqueado",
.power_off				= "Desligar...",
.error					= "ERRO",
.rx_only				= "Apenas Rx",
.out_of_band			= "FORA DA BANDA",
.timeout				= "TEMPO ESGOTADO",
.tg_entry				= "Entrada TG",
.pc_entry				= "Entrada PC",
.user_dmr_id			= "DMRID Utilizador",
.contact 				= "Contacto",
.accept_call			= "Aceitar Chamada?",
.private_call			= "Chamada Privada",
.squelch				= "Squelch",
.quick_menu 			= "Menu Rápido",
.filter					= "Filtro",
.all_channels			= "Todos os Canais",
.gotoChannel			= "Ir para",
.scan					= "Busca",
.channelToVfo			= "Canal --> VFO",
.vfoToChannel			= "VFO --> Canal",
.vfoToNewChannel		= "VFO --> Novo Can",
.group					= "Grupo",
.private				= "Privado",
.all					= "Todos",
.type					= "Tipo",
.timeSlot				= "Intervalo Tempo", // Too long
.none					= "Nenhum",
.contact_saved			= "Contacto Gravado",
.duplicate				= "Duplicado",
.tg						= "TG",
.pc						= "PC",
.ts						= "TS",
.mode					= "Mode",
.colour_code			= "Codigo de cores",
.n_a					= "N/A",
.bandwidth				= "Largura de banda",
.stepFreq				= "Passo",
.tot					= "TOT",
.off					= "Off",
.zone_skip				= "Z Ignorar",
.all_skip				= "A Ignorar",
.yes					= "Sim",
.no						= "Nao",
.rx_group				= "Rx Grp",
.on						= "On",
.timeout_beep			= "Beep timeout",
.factory_reset			= "Fact Reset",
.calibration			= "Calibracao",
.band_limits			= "Limites banda",
.beep_volume			= "Volume beep",
.dmr_mic_gain			= "Micro DMR",
.key_long				= "Key long",
.key_repeat				= "Key rpt",
.dmr_filter_timeout		= "Filtro DMR",
.brightness				= "Brilho",
.brightness_off				= "Min bright",
.contrast				= "Contraste",
.colour_invert			= "Color:Invertido",
.colour_normal			= "Color:Normal",
.backlight_timeout		= "Timeout",
.scan_delay				= "Scan delay",
.YES					= "SIM",
.NO						= "NÃO",
.DISMISS				= "DISPENSAR",
.scan_mode				= "Scan mode",
.hold					= "Hold",
.pause					= "Pause",
.empty_list				= "Empty List",
.delete_contact_qm			= "Delete contact?",
.contact_deleted			= "Contact deleted",
.contact_used				= "Contact used",
.in_rx_group				= "in RX group",
.select_tx				= "Select TX",
.edit_contact				= "Edit Contact",
.delete_contact				= "Delete Contact",
.group_call				= "Group Call",
.all_call				= "All Call",
.tone_scan				= "Tone scan",//// MaxLen: 16
.cc_scan				= "CC scan",//// MaxLen: 16
.low_battery			= "LOW BATTERY !!!",//// MaxLen: 16
.Auto					= "Auto", // MaxLen 16 (with .mode + ':') 
.manual					= "Manual",  // MaxLen 16 (with .mode + ':') 
.ptt_toggle				= "PTT toggle", // MaxLen 16 (with ':' + .on or .off)
.private_call_handling			= "Handle PC", // MaxLen 16 (with ':' + .on ot .off)
.stop					= "Stop", // Maxlen 16 (with ':' + .scan_mode)
.one_line				= "1 line", // MaxLen 16 (with ':' + .contact)
.two_lines				= "2 lines", // MaxLen 16 (with ':' + .contact)
.priority_order				= "Order", // MaxLen 16 (with ':' + 'Cc/DB/TA')
.dmr_beep				= "DMR beep", // MaxLen 16 (with ':' + .star/.stop/.both/.none)
.start					= "Start", // MaxLen 16 (with ':' + .dmr_beep)
.both					= "Both" // MaxLen 16 (with ':' + .dmr_beep)
};
/********************************************************************
 *
 * VERY IMPORTANT.
 * This file should not be saved with UTF-8 encoding
 * Use Notepad++ on Windows with ANSI encoding
 * or emacs on Linux with windows-1252-unix encoding
 *
 ********************************************************************/
#endif /* USER_INTERFACE_LANGUAGES_PORTUGUESE_H_ */
