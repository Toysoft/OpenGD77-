/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
 *
 * Using some code ported from MMDVM_HS by Andy CA6JAU
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
#include <user_interface/uiLocalisation.h>

#include <user_interface/languages/english.h>
#include <user_interface/languages/french.h>
#include <user_interface/languages/german.h>
#include <user_interface/languages/portuguese.h>
#include <user_interface/languages/catalan.h>
#include <user_interface/languages/spanish.h>

const stringsTable_t languages[NUM_LANGUAGES]= { englishLanguage,frenchLanguage,catalanLanguage,germanLanguage,portuguesLanguage,spanishLanguage};
const stringsTable_t *currentLanguage;
