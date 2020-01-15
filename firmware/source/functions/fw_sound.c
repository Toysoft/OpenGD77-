/*
 * Copyright (C)2019 Kai Ludwig, DG4KLU
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

#include <hardware/fw_HR-C6000.h>
#include "fw_sound.h"
#include "fw_settings.h"
typedef union byteSwap16
{
	int16_t byte16;
	uint8_t bytes8[2];
} byteSwap16_t;
byteSwap16_t swapper;

TaskHandle_t fwBeepTaskHandle;
static const int16_t sine_beep16[] =  {0,101,201,302,402,503,603,704,804,905,1005,1106,1206,1307,1407,1507,1608,1708,1809,1909,2009,2110,2210,2310,2410,2511,2611,2711,2811,2911,3012,3112,3212,3312,3412,3512,3612,3712,3811,3911,4011,4111,4210,4310,4410,4509,4609,4708,4808,4907,5007,5106,5205,5305,5404,5503,5602,5701,5800,5899,5998,6096,6195,6294,6393,6491,6590,6688,6786,6885,6983,7081,7179,7277,7375,7473,7571,7669,7767,7864,7962,8059,8157,8254,8351,8448,8545,8642,8739,8836,8933,9030,9126,9223,9319,9416,9512,9608,9704,9800,9896,9992,10087,10183,10278,10374,10469,10564,10659,10754,10849,10944,11039,11133,11228,11322,11417,11511,11605,11699,11793,11886,11980,12074,12167,12260,12353,12446,12539,12632,12725,12817,12910,13002,13094,13187,13279,13370,13462,13554,13645,13736,13828,13919,14010,14101,14191,14282,14372,14462,14553,14643,14732,14822,14912,15001,15090,15180,15269,15358,15446,15535,15623,15712,15800,15888,15976,16063,16151,16238,16325,16413,16499,16586,16673,16759,16846,16932,17018,17104,17189,17275,17360,17445,17530,17615,17700,17784,17869,17953,18037,18121,18204,18288,18371,18454,18537,18620,18703,18785,18868,18950,19032,19113,19195,19276,19357,19438,19519,19600,19680,19761,19841,19921,20000,20080,20159,20238,20317,20396,20475,20553,20631,20709,20787,20865,20942,21019,21096,21173,21250,21326,21403,21479,21554,21630,21705,21781,21856,21930,22005,22079,22154,22227,22301,22375,22448,22521,22594,22667,22739,22812,22884,22956,23027,23099,23170,23241,23311,23382,23452,23522,23592,23662,23731,23801,23870,23938,24007,24075,24143,24211,24279,24346,24413,24480,24547,24613,24680,24746,24811,24877,24942,25007,25072,25137,25201,25265,25329,25393,25456,25519,25582,25645,25708,25770,25832,25893,25955,26016,26077,26138,26198,26259,26319,26378,26438,26497,26556,26615,26674,26732,26790,26848,26905,26962,27019,27076,27133,27189,27245,27300,27356,27411,27466,27521,27575,27629,27683,27737,27790,27843,27896,27949,28001,28053,28105,28157,28208,28259,28310,28360,28411,28460,28510,28560,28609,28658,28706,28755,28803,28850,28898,28945,28992,29039,29085,29131,29177,29223,29268,29313,29358,29403,29447,29491,29534,29578,29621,29664,29706,29749,29791,29832,29874,29915,29956,29997,30037,30077,30117,30156,30195,30234,30273,30311,30349,30387,30424,30462,30498,30535,30571,30607,30643,30679,30714,30749,30783,30818,30852,30885,30919,30952,30985,31017,31050,31082,31113,31145,31176,31206,31237,31267,31297,31327,31356,31385,31414,31442,31470,31498,31526,31553,31580,31607,31633,31659,31685,31710,31736,31760,31785,31809,31833,31857,31880,31903,31926,31949,31971,31993,32014,32036,32057,32077,32098,32118,32137,32157,32176,32195,32213,32232,32250,32267,32285,32302,32318,32335,32351,32367,32382,32397,32412,32427,32441,32455,32469,32482,32495,32508,32521,32533,32545,32556,32567,32578,32589,32599,32609,32619,32628,32637,32646,32655,32663,32671,32678,32685,32692,32699,32705,32711,32717,32722,32728,32732,32737,32741,32745,32748,32752,32755,32757,32759,32761,32763,32765,32766,32766,32767,32767,32767,32766,32766,32765,32763,32761,32759,32757,32755,32752,32748,32745,32741,32737,32732,32728,32722,32717,32711,32705,32699,32692,32685,32678,32671,32663,32655,32646,32637,32628,32619,32609,32599,32589,32578,32567,32556,32545,32533,32521,32508,32495,32482,32469,32455,32441,32427,32412,32397,32382,32367,32351,32335,32318,32302,32285,32267,32250,32232,32213,32195,32176,32157,32137,32118,32098,32077,32057,32036,32014,31993,31971,31949,31926,31903,31880,31857,31833,31809,31785,31760,31736,31710,31685,31659,31633,31607,31580,31553,31526,31498,31470,31442,31414,31385,31356,31327,31297,31267,31237,31206,31176,31145,31113,31082,31050,31017,30985,30952,30919,30885,30852,30818,30783,30749,30714,30679,30643,30607,30571,30535,30498,30462,30424,30387,30349,30311,30273,30234,30195,30156,30117,30077,30037,29997,29956,29915,29874,29832,29791,29749,29706,29664,29621,29578,29534,29491,29447,29403,29358,29313,29268,29223,29177,29131,29085,29039,28992,28945,28898,28850,28803,28755,28706,28658,28609,28560,28510,28460,28411,28360,28310,28259,28208,28157,28105,28053,28001,27949,27896,27843,27790,27737,27683,27629,27575,27521,27466,27411,27356,27300,27245,27189,27133,27076,27019,26962,26905,26848,26790,26732,26674,26615,26556,26497,26438,26378,26319,26259,26198,26138,26077,26016,25955,25893,25832,25770,25708,25645,25582,25519,25456,25393,25329,25265,25201,25137,25072,25007,24942,24877,24811,24746,24680,24613,24547,24480,24413,24346,24279,24211,24143,24075,24007,23938,23870,23801,23731,23662,23592,23522,23452,23382,23311,23241,23170,23099,23027,22956,22884,22812,22739,22667,22594,22521,22448,22375,22301,22227,22154,22079,22005,21930,21856,21781,21705,21630,21554,21479,21403,21326,21250,21173,21096,21019,20942,20865,20787,20709,20631,20553,20475,20396,20317,20238,20159,20080,20000,19921,19841,19761,19680,19600,19519,19438,19357,19276,19195,19113,19032,18950,18868,18785,18703,18620,18537,18454,18371,18288,18204,18121,18037,17953,17869,17784,17700,17615,17530,17445,17360,17275,17189,17104,17018,16932,16846,16759,16673,16586,16499,16413,16325,16238,16151,16063,15976,15888,15800,15712,15623,15535,15446,15358,15269,15180,15090,15001,14912,14822,14732,14643,14553,14462,14372,14282,14191,14101,14010,13919,13828,13736,13645,13554,13462,13370,13279,13187,13094,13002,12910,12817,12725,12632,12539,12446,12353,12260,12167,12074,11980,11886,11793,11699,11605,11511,11417,11322,11228,11133,11039,10944,10849,10754,10659,10564,10469,10374,10278,10183,10087,9992,9896,9800,9704,9608,9512,9416,9319,9223,9126,9030,8933,8836,8739,8642,8545,8448,8351,8254,8157,8059,7962,7864,7767,7669,7571,7473,7375,7277,7179,7081,6983,6885,6786,6688,6590,6491,6393,6294,6195,6096,5998,5899,5800,5701,5602,5503,5404,5305,5205,5106,5007,4907,4808,4708,4609,4509,4410,4310,4210,4111,4011,3911,3811,3712,3612,3512,3412,3312,3212,3112,3012,2911,2811,2711,2611,2511,2410,2310,2210,2110,2009,1909,1809,1708,1608,1507,1407,1307,1206,1106,1005,905,804,704,603,503,402,302,201,101,0,-101,-201,-302,-402,-503,-603,-704,-804,-905,-1005,-1106,-1206,-1307,-1407,-1507,-1608,-1708,-1809,-1909,-2009,-2110,-2210,-2310,-2410,-2511,-2611,-2711,-2811,-2911,-3012,-3112,-3212,-3312,-3412,-3512,-3612,-3712,-3811,-3911,-4011,-4111,-4210,-4310,-4410,-4509,-4609,-4708,-4808,-4907,-5007,-5106,-5205,-5305,-5404,-5503,-5602,-5701,-5800,-5899,-5998,-6096,-6195,-6294,-6393,-6491,-6590,-6688,-6786,-6885,-6983,-7081,-7179,-7277,-7375,-7473,-7571,-7669,-7767,-7864,-7962,-8059,-8157,-8254,-8351,-8448,-8545,-8642,-8739,-8836,-8933,-9030,-9126,-9223,-9319,-9416,-9512,-9608,-9704,-9800,-9896,-9992,-10087,-10183,-10278,-10374,-10469,-10564,-10659,-10754,-10849,-10944,-11039,-11133,-11228,-11322,-11417,-11511,-11605,-11699,-11793,-11886,-11980,-12074,-12167,-12260,-12353,-12446,-12539,-12632,-12725,-12817,-12910,-13002,-13094,-13187,-13279,-13370,-13462,-13554,-13645,-13736,-13828,-13919,-14010,-14101,-14191,-14282,-14372,-14462,-14553,-14643,-14732,-14822,-14912,-15001,-15090,-15180,-15269,-15358,-15446,-15535,-15623,-15712,-15800,-15888,-15976,-16063,-16151,-16238,-16325,-16413,-16499,-16586,-16673,-16759,-16846,-16932,-17018,-17104,-17189,-17275,-17360,-17445,-17530,-17615,-17700,-17784,-17869,-17953,-18037,-18121,-18204,-18288,-18371,-18454,-18537,-18620,-18703,-18785,-18868,-18950,-19032,-19113,-19195,-19276,-19357,-19438,-19519,-19600,-19680,-19761,-19841,-19921,-20000,-20080,-20159,-20238,-20317,-20396,-20475,-20553,-20631,-20709,-20787,-20865,-20942,-21019,-21096,-21173,-21250,-21326,-21403,-21479,-21554,-21630,-21705,-21781,-21856,-21930,-22005,-22079,-22154,-22227,-22301,-22375,-22448,-22521,-22594,-22667,-22739,-22812,-22884,-22956,-23027,-23099,-23170,-23241,-23311,-23382,-23452,-23522,-23592,-23662,-23731,-23801,-23870,-23938,-24007,-24075,-24143,-24211,-24279,-24346,-24413,-24480,-24547,-24613,-24680,-24746,-24811,-24877,-24942,-25007,-25072,-25137,-25201,-25265,-25329,-25393,-25456,-25519,-25582,-25645,-25708,-25770,-25832,-25893,-25955,-26016,-26077,-26138,-26198,-26259,-26319,-26378,-26438,-26497,-26556,-26615,-26674,-26732,-26790,-26848,-26905,-26962,-27019,-27076,-27133,-27189,-27245,-27300,-27356,-27411,-27466,-27521,-27575,-27629,-27683,-27737,-27790,-27843,-27896,-27949,-28001,-28053,-28105,-28157,-28208,-28259,-28310,-28360,-28411,-28460,-28510,-28560,-28609,-28658,-28706,-28755,-28803,-28850,-28898,-28945,-28992,-29039,-29085,-29131,-29177,-29223,-29268,-29313,-29358,-29403,-29447,-29491,-29534,-29578,-29621,-29664,-29706,-29749,-29791,-29832,-29874,-29915,-29956,-29997,-30037,-30077,-30117,-30156,-30195,-30234,-30273,-30311,-30349,-30387,-30424,-30462,-30498,-30535,-30571,-30607,-30643,-30679,-30714,-30749,-30783,-30818,-30852,-30885,-30919,-30952,-30985,-31017,-31050,-31082,-31113,-31145,-31176,-31206,-31237,-31267,-31297,-31327,-31356,-31385,-31414,-31442,-31470,-31498,-31526,-31553,-31580,-31607,-31633,-31659,-31685,-31710,-31736,-31760,-31785,-31809,-31833,-31857,-31880,-31903,-31926,-31949,-31971,-31993,-32014,-32036,-32057,-32077,-32098,-32118,-32137,-32157,-32176,-32195,-32213,-32232,-32250,-32267,-32285,-32302,-32318,-32335,-32351,-32367,-32382,-32397,-32412,-32427,-32441,-32455,-32469,-32482,-32495,-32508,-32521,-32533,-32545,-32556,-32567,-32578,-32589,-32599,-32609,-32619,-32628,-32637,-32646,-32655,-32663,-32671,-32678,-32685,-32692,-32699,-32705,-32711,-32717,-32722,-32728,-32732,-32737,-32741,-32745,-32748,-32752,-32755,-32757,-32759,-32761,-32763,-32765,-32766,-32766,-32767,-32767,-32767,-32766,-32766,-32765,-32763,-32761,-32759,-32757,-32755,-32752,-32748,-32745,-32741,-32737,-32732,-32728,-32722,-32717,-32711,-32705,-32699,-32692,-32685,-32678,-32671,-32663,-32655,-32646,-32637,-32628,-32619,-32609,-32599,-32589,-32578,-32567,-32556,-32545,-32533,-32521,-32508,-32495,-32482,-32469,-32455,-32441,-32427,-32412,-32397,-32382,-32367,-32351,-32335,-32318,-32302,-32285,-32267,-32250,-32232,-32213,-32195,-32176,-32157,-32137,-32118,-32098,-32077,-32057,-32036,-32014,-31993,-31971,-31949,-31926,-31903,-31880,-31857,-31833,-31809,-31785,-31760,-31736,-31710,-31685,-31659,-31633,-31607,-31580,-31553,-31526,-31498,-31470,-31442,-31414,-31385,-31356,-31327,-31297,-31267,-31237,-31206,-31176,-31145,-31113,-31082,-31050,-31017,-30985,-30952,-30919,-30885,-30852,-30818,-30783,-30749,-30714,-30679,-30643,-30607,-30571,-30535,-30498,-30462,-30424,-30387,-30349,-30311,-30273,-30234,-30195,-30156,-30117,-30077,-30037,-29997,-29956,-29915,-29874,-29832,-29791,-29749,-29706,-29664,-29621,-29578,-29534,-29491,-29447,-29403,-29358,-29313,-29268,-29223,-29177,-29131,-29085,-29039,-28992,-28945,-28898,-28850,-28803,-28755,-28706,-28658,-28609,-28560,-28510,-28460,-28411,-28360,-28310,-28259,-28208,-28157,-28105,-28053,-28001,-27949,-27896,-27843,-27790,-27737,-27683,-27629,-27575,-27521,-27466,-27411,-27356,-27300,-27245,-27189,-27133,-27076,-27019,-26962,-26905,-26848,-26790,-26732,-26674,-26615,-26556,-26497,-26438,-26378,-26319,-26259,-26198,-26138,-26077,-26016,-25955,-25893,-25832,-25770,-25708,-25645,-25582,-25519,-25456,-25393,-25329,-25265,-25201,-25137,-25072,-25007,-24942,-24877,-24811,-24746,-24680,-24613,-24547,-24480,-24413,-24346,-24279,-24211,-24143,-24075,-24007,-23938,-23870,-23801,-23731,-23662,-23592,-23522,-23452,-23382,-23311,-23241,-23170,-23099,-23027,-22956,-22884,-22812,-22739,-22667,-22594,-22521,-22448,-22375,-22301,-22227,-22154,-22079,-22005,-21930,-21856,-21781,-21705,-21630,-21554,-21479,-21403,-21326,-21250,-21173,-21096,-21019,-20942,-20865,-20787,-20709,-20631,-20553,-20475,-20396,-20317,-20238,-20159,-20080,-20000,-19921,-19841,-19761,-19680,-19600,-19519,-19438,-19357,-19276,-19195,-19113,-19032,-18950,-18868,-18785,-18703,-18620,-18537,-18454,-18371,-18288,-18204,-18121,-18037,-17953,-17869,-17784,-17700,-17615,-17530,-17445,-17360,-17275,-17189,-17104,-17018,-16932,-16846,-16759,-16673,-16586,-16499,-16413,-16325,-16238,-16151,-16063,-15976,-15888,-15800,-15712,-15623,-15535,-15446,-15358,-15269,-15180,-15090,-15001,-14912,-14822,-14732,-14643,-14553,-14462,-14372,-14282,-14191,-14101,-14010,-13919,-13828,-13736,-13645,-13554,-13462,-13370,-13279,-13187,-13094,-13002,-12910,-12817,-12725,-12632,-12539,-12446,-12353,-12260,-12167,-12074,-11980,-11886,-11793,-11699,-11605,-11511,-11417,-11322,-11228,-11133,-11039,-10944,-10849,-10754,-10659,-10564,-10469,-10374,-10278,-10183,-10087,-9992,-9896,-9800,-9704,-9608,-9512,-9416,-9319,-9223,-9126,-9030,-8933,-8836,-8739,-8642,-8545,-8448,-8351,-8254,-8157,-8059,-7962,-7864,-7767,-7669,-7571,-7473,-7375,-7277,-7179,-7081,-6983,-6885,-6786,-6688,-6590,-6491,-6393,-6294,-6195,-6096,-5998,-5899,-5800,-5701,-5602,-5503,-5404,-5305,-5205,-5106,-5007,-4907,-4808,-4708,-4609,-4509,-4410,-4310,-4210,-4111,-4011,-3911,-3811,-3712,-3612,-3512,-3412,-3312,-3212,-3112,-3012,-2911,-2811,-2711,-2611,-2511,-2410,-2310,-2210,-2110,-2009,-1909,-1809,-1708,-1608,-1507,-1407,-1307,-1206,-1106,-1005,-905,-804,-704,-603,-503,-402,-302,-201,-101,};
volatile int sine_beep_freq;
volatile int sine_beep_duration;
volatile int micAudioSamplesTotal;

static volatile uint32_t runningMaxValue=0;
static const int MIC_AVERAGE_COUNTER_RELOAD = 10;
static volatile int micAudioAverageCounter = MIC_AVERAGE_COUNTER_RELOAD;

int melody_generic[512];// Note. As we don't play long melodies, I think this value can be made smaller.
#define DIT_LENGTH  60
#define DAH_LENGTH  3 * DIT_LENGTH
//const int melody_poweron[] = { 440, 300, 466, 300, 494, 300, -1, -1 };
const int melody_poweron[] = { 880, DAH_LENGTH,
								0, DIT_LENGTH,
								880, DIT_LENGTH,
								0, DIT_LENGTH,
								880, DAH_LENGTH,
								0, DIT_LENGTH,
								880, DIT_LENGTH,
								0, DIT_LENGTH,
								880, DAH_LENGTH,
								-1, -1 };
const int melody_private_call[] = {
								880, DIT_LENGTH,
								0, DIT_LENGTH,
								880, DAH_LENGTH,
								0, DIT_LENGTH,
								880, DAH_LENGTH,
								0, DIT_LENGTH,
								880, DIT_LENGTH,

								0, DAH_LENGTH,
								880, DAH_LENGTH,
								0, DIT_LENGTH,
								880, DIT_LENGTH,
								0, DIT_LENGTH,
								880, DAH_LENGTH,
								0, DIT_LENGTH,
								880, DIT_LENGTH

								-1, -1 };// Morse letters PC for Priavte Call
const int melody_key_beep[] = { 600, 60, -1, -1 };
const int melody_key_long_beep[] = { 880, 60, -1, -1 };
const int melody_sk1_beep[] = { 466, 60, 0, 60, 466, 60, -1, -1 };
const int melody_sk2_beep[] = { 494, 60, 0, 60, 494, 60, -1, -1 };
const int melody_orange_beep[] = { 440, 60, 494, 60, 440, 60, 494, 60, -1, -1 };
const int melody_ACK_beep[] = { 440, 120, 660, 120, 880, 120, -1, -1 };
const int melody_NACK_beep[] = { 494, 120, 466, 120, -1, -1 };
const int melody_ERROR_beep[] = { 440, 30, 0, 30, 440, 30, 0, 30, 440, 30, -1, -1 };
const int melody_tx_timeout_beep[] = { 440, 60, 494, 60, 440, 60, 494, 60, 440, 60, 494, 60, 440, 60, 494, 60, -1, -1 };
volatile int *melody_play = NULL;
volatile int melody_idx = 0;
int soundBeepVolumeDivider;
int currentPromptPosition=-1;
int currentPromptLength;

const uint8_t displayoptions_amb[] = {
  0x96, 0x4f, 0x2e, 0xb9, 0xee, 0xf1, 0x89, 0x5f, 0x90,
  0xc6, 0x28, 0xb2, 0x51, 0x51, 0x7d, 0x04, 0x50, 0xaf,
  0xbd, 0xa6, 0xd2, 0xd9, 0x05, 0x61, 0x30, 0xce, 0x5c,
  0xa3, 0x13, 0x1b, 0xbe, 0xb4, 0x26, 0xa7, 0x58, 0xd6,
  0xe3, 0x51, 0x0f, 0x8f, 0xef, 0x80, 0xc5, 0xc3, 0x34,
  0x7b, 0xf3, 0x39, 0xb1, 0xfa, 0xc1, 0xbe, 0x57, 0x1b,
  0x4b, 0x8b, 0x4c, 0x4c, 0xcf, 0x79, 0xb8, 0xe8, 0xe2,
  0x3d, 0x8b, 0x07, 0x4e, 0xc9, 0x50, 0xb3, 0xf4, 0xce,
  0x5b, 0xb9, 0x2c, 0x0d, 0x80, 0xa0, 0x0f, 0x50, 0xdf,
  0x91, 0x01, 0x08, 0xa8, 0xd6, 0x44, 0xc1, 0x7e, 0xe3,
  0xd7, 0x70, 0x48, 0xdd, 0x39, 0x5c, 0x28, 0xd1, 0x63,
  0xe0, 0x47, 0xa6, 0x57, 0x5f, 0xcf, 0x4c, 0x30, 0xbb,
  0x91, 0x83, 0x85, 0x26, 0x95, 0x01, 0x9b, 0x6d, 0x13,
  0xbd, 0x6e, 0x18, 0x8f, 0x17, 0xeb, 0x4d, 0x17, 0x26,
  0xaa, 0x49, 0x2d, 0xad, 0x72, 0x71, 0x49, 0x3b, 0xfc,
  0xfc, 0x0d, 0xc7, 0x27, 0x7e, 0x67, 0x0f, 0x16, 0x81,
  0xcd, 0x63, 0xc5, 0x62, 0x89, 0xf1, 0x68, 0xcd, 0xe9,
  0x7d, 0xa7, 0x3f, 0x8b, 0x30, 0xcc, 0x35, 0x11, 0x87,
  0x1e, 0xde, 0x4f, 0x1a, 0xd5, 0x40, 0xa9, 0x0a, 0xb8,
  0x1e, 0xb8, 0x04, 0x5f, 0x8c, 0x61, 0xb4, 0xb7, 0xad,
  0xf5, 0x47, 0x04, 0x6c, 0xe9, 0xb0, 0x10, 0x29, 0x4b,
  0xc2, 0x27, 0x03, 0x03, 0xef, 0x57, 0xc5, 0xdc, 0x52,
  0x90, 0x57, 0x66, 0x45, 0xbe, 0x7b, 0x50, 0xba, 0xcb,
  0xd1, 0x37, 0x03, 0x74, 0x8a, 0x23, 0xd5, 0xdc, 0x57,
  0xc1, 0x15, 0x03, 0x74, 0x9d, 0x31, 0x87, 0xca, 0x63,
  0xe3, 0x24, 0x02, 0x72, 0xbe, 0x11, 0xb3, 0xce, 0x27,
  0xd4, 0x74, 0x1c, 0x33, 0x89, 0x51, 0x3b, 0x94, 0xa2,
  0x1b, 0x02, 0xb2, 0x3e, 0xf1, 0x64, 0xd6, 0xfb, 0x4c,
  0xb5, 0xb1, 0x8c, 0xf1, 0x38, 0xdf, 0x41, 0x04, 0x9a,
  0xe0, 0xa7, 0xa6, 0xd3, 0xba, 0xe2, 0xe0, 0x22, 0x0c,
  0x06, 0x1a, 0xcf, 0xc5, 0x6f, 0xe6, 0xd9, 0x52, 0xe7,
  0xd7, 0xe6, 0x5f, 0x8c, 0xfb, 0x24, 0xfd, 0x34, 0x2b,
  0xf5, 0x23, 0x10, 0x9c, 0x8a, 0x87, 0x59, 0xa9, 0xa5,
  0xc0, 0x62, 0x41, 0x78, 0xa3, 0xd6, 0x85, 0xab, 0xfc,
  0x96, 0x35, 0x19, 0x12, 0xf0, 0xc1, 0x1d, 0x28, 0x27,
  0xe3, 0x63, 0x48, 0x55, 0xf6, 0x00, 0xf9, 0x56, 0x76,
  0xa6, 0x79, 0x0f, 0xd8, 0x67, 0x49, 0xf4, 0xdd, 0x13,
  0xfd, 0xe6, 0xb7, 0x88, 0x26, 0x6f, 0xc5, 0x9a, 0xe3,
  0x9a, 0xb5, 0xf9, 0x19, 0xac, 0xeb, 0xbf, 0xd3, 0x95,
  0xbb, 0x86, 0xa1, 0x60, 0xc6, 0xc0, 0x45, 0xa3, 0x03,
  0xef, 0xd0, 0xc2, 0x62, 0xc0, 0x50, 0xd7, 0xe9, 0x67,
  0xe8, 0xc5, 0xe4, 0x06, 0xe2, 0x88, 0xf1, 0xb3, 0x98,
  0xd6, 0x76, 0x81, 0x67, 0xc6, 0xe9, 0x03, 0x9d, 0xf6,
  0xa7, 0x07, 0x1d, 0xcf, 0xe0, 0xbf, 0x83, 0x52, 0x1d,
  0xd6, 0x3f, 0x4d, 0x61, 0xad, 0xd5, 0xc7, 0x56, 0x37,
  0xd4, 0x2c, 0x3f, 0x31, 0xe5, 0x1c, 0x14, 0xfd, 0x4a,
  0xd3, 0x86, 0x2c, 0x4c, 0x92, 0x73, 0x41, 0x57, 0x67,
  0xc0, 0xf5, 0x2d, 0x61, 0x2c, 0x80, 0xe8, 0x5e, 0x07,
  0xc4, 0xf3, 0x26, 0xf4, 0x5a, 0x80, 0x2c, 0xc6, 0x2f,
  0x90, 0xd0, 0x60, 0x33, 0x7a, 0x35, 0xa5, 0x0e, 0x96,
  0xc0, 0x9d, 0x6c, 0x40, 0x18, 0x34, 0x70, 0xef, 0xa3,
  0x80, 0x8a, 0x28, 0x2b, 0x48, 0x65, 0x35, 0x07, 0xa9,
  0xb0, 0x70, 0xf4, 0xc4, 0x48, 0xe2, 0xa4, 0x96, 0x9b,
  0xbb, 0xa1, 0x7a, 0xed, 0x3a, 0x40, 0x7d, 0xdb, 0xe3,
  0x8c, 0x2b, 0x2c, 0x48, 0x35, 0xf9, 0x75, 0xe6, 0xdf,
  0xdd, 0xa0, 0xfb, 0x11, 0x2d, 0x41, 0x50, 0x0f, 0xed,
  0xf5, 0x23, 0xa8, 0x56, 0x93, 0x64, 0xfd, 0xed, 0x8b,
  0x9a, 0x7a, 0x4d, 0xfd, 0x69, 0xc8, 0xdd, 0x96, 0xb4,
  0xaa, 0x6f, 0x6f, 0xf8, 0x80, 0x61, 0x80, 0x25, 0x74,
  0xc9, 0x29, 0xc3, 0x36, 0x2d, 0xec, 0x29, 0x1c, 0x4b,
  0xdf, 0xf1, 0xe2, 0x17, 0x92, 0x31, 0x97, 0xde, 0x44,
  0xce, 0xd2, 0xe3, 0x31, 0xb5, 0x21, 0xa0, 0xcd, 0x33,
  0xACU, 0xAAU, 0x40U, 0x20U, 0x00U, 0x44U, 0x40U, 0x80U, 0x80U
};
const unsigned int displayoptions_amb_len = 558 + 9;

void set_melody(const int *melody)
{
	taskENTER_CRITICAL();
	sine_beep_freq=0;
	sine_beep_duration=0;
	melody_play=(int *)melody;
	melody_idx=0;
	taskEXIT_CRITICAL();
}

// To calculate the pitch use a spreadsheet etc   =ROUND(98*POWER(2, (NOTE_NUMBER/12)),0)
static const int freqs[] = {0,104,110,117,123,131,139,147,156,165,175,185,196,208,220,233,247,262,277,294,311,330,349,370,392,415,440,466,494,523,554,587,622,659,698,740,784,831,880,932,988,1047,1109,1175,1245,1319,1397,1480};
int get_freq(int tone)
{
	return (freqs[tone]);
}

void create_song(const uint8_t *melody)
{
	int song_idx = 0;
	for (int i=0;i<256;i++)
	{
		if (melody[2*i+1]!=0)
		{
			melody_generic[song_idx++]=get_freq(melody[2*i]);
			melody_generic[song_idx++]=melody[2*i+1]*27;
		}
		else
		{
			melody_generic[song_idx++]=-1;
			melody_generic[song_idx++]=-1;
			song_idx=song_idx+2;
			break;
		}
	}
}

void fw_init_beep_task(void)
{
	taskENTER_CRITICAL();
	sine_beep_freq = 0;
	sine_beep_duration = 0;
	taskEXIT_CRITICAL();

	xTaskCreate(fw_beep_task,                        /* pointer to the task */
				"fw beep task",                      /* task name for kernel awareness debugging */
				1000L / sizeof(portSTACK_TYPE),      /* task stack size */
				NULL,                      			 /* optional task startup argument */
				5U,                                  /* initial priority */
				fwBeepTaskHandle					 /* optional task handle to create */
				);
}

union sharedDataBuffer audioAndHotspotDataBuffer;

volatile int  wavbuffer_read_idx;
volatile int  wavbuffer_write_idx;
volatile int wavbuffer_count;
uint8_t tmp_wavbuffer[WAV_BUFFER_SIZE];
uint8_t *currentWaveBuffer;

uint8_t spi_sound1[WAV_BUFFER_SIZE*2];
uint8_t spi_sound2[WAV_BUFFER_SIZE*2];
uint8_t spi_sound3[WAV_BUFFER_SIZE*2];
uint8_t spi_sound4[WAV_BUFFER_SIZE*2];

volatile bool g_TX_SAI_in_use = false;


uint8_t *spi_soundBuf;
sai_transfer_t xfer;

void init_sound(void)
{

    g_TX_SAI_in_use = false;
    SAI_TxSoftwareReset(I2S0, kSAI_ResetAll);
	SAI_TxEnable(I2S0, true);
    SAI_RxSoftwareReset(I2S0, kSAI_ResetAll);
	SAI_RxEnable(I2S0, true);
	spi_soundBuf=NULL;
	wavbuffer_read_idx = 0;
	wavbuffer_write_idx = 0;
	wavbuffer_count = 0;
}

void terminate_sound(void)
{
    SAI_TransferTerminateSendEDMA(I2S0, &g_SAI_TX_Handle);
    SAI_TransferTerminateSendEDMA(I2S0, &g_SAI_RX_Handle);
}

void store_soundbuffer(void)
{
	taskENTER_CRITICAL();
	int tmp_wavbuffer_count = wavbuffer_count;
	taskEXIT_CRITICAL();

	if (tmp_wavbuffer_count<WAV_BUFFER_COUNT)
	{
		taskENTER_CRITICAL();
		for (int wav_idx=0;wav_idx<WAV_BUFFER_SIZE;wav_idx++)
		{
			audioAndHotspotDataBuffer.wavbuffer[wavbuffer_write_idx][wav_idx]=tmp_wavbuffer[wav_idx];
		}
		taskEXIT_CRITICAL();
		wavbuffer_write_idx++;
		if (wavbuffer_write_idx>=WAV_BUFFER_COUNT)
		{
			wavbuffer_write_idx=0;
		}

		taskENTER_CRITICAL();
		wavbuffer_count++;
		taskEXIT_CRITICAL();
	//	SEGGER_RTT_printf(0, "store_soundbuffer %d\n",wavbuffer_count);
	}
}

void retrieve_soundbuffer(void)
{
	taskENTER_CRITICAL();
	int tmp_wavbuffer_count = wavbuffer_count;
	taskEXIT_CRITICAL();

	if (tmp_wavbuffer_count>0)
	{

		currentWaveBuffer = (uint8_t *)audioAndHotspotDataBuffer.wavbuffer[wavbuffer_read_idx];// cast just to prevent compiler warning
		wavbuffer_read_idx++;
		if (wavbuffer_read_idx>=WAV_BUFFER_COUNT)
		{
			wavbuffer_read_idx=0;
		}

		taskENTER_CRITICAL();
		wavbuffer_count--;
		taskEXIT_CRITICAL();
	}
}


// This function is used when receiving
void send_sound_data(void)
{
	if (wavbuffer_count>0)
	{
		switch(g_SAI_TX_Handle.queueUser)
		{
		case 0:
			spi_soundBuf = spi_sound1;
			break;
		case 1:
			spi_soundBuf = spi_sound2;
			break;
		case 2:
			spi_soundBuf = spi_sound3;
			break;
		case 3:
			spi_soundBuf = spi_sound4;
			break;
		default:
			spi_soundBuf=spi_sound1;// just put in to please the compiler
			break;
		}

		for (int i=0; i<(WAV_BUFFER_SIZE/2); i++)
		{
			*(spi_soundBuf +4*i +3) = audioAndHotspotDataBuffer.wavbuffer[wavbuffer_read_idx][2*i+1];
			*(spi_soundBuf +4*i +2) = audioAndHotspotDataBuffer.wavbuffer[wavbuffer_read_idx][2*i];
		}

		xfer.data = spi_soundBuf;
		xfer.dataSize = WAV_BUFFER_SIZE*2;

		SAI_TransferSendEDMA(I2S0, &g_SAI_TX_Handle, &xfer);

		g_TX_SAI_in_use = true;

		wavbuffer_read_idx++;
		if (wavbuffer_read_idx>=WAV_BUFFER_COUNT)
		{
			wavbuffer_read_idx=0;
		}
		wavbuffer_count--;
		//SEGGER_RTT_printf(0, "send_sound_data %d \n",wavbuffer_count);
	}
}


// This function is used during transmission.
void receive_sound_data(void)
{
	if (trxIsTransmitting==false)
	{
		return;
	}

	if (wavbuffer_count<WAV_BUFFER_COUNT)
	{
		// spi_soundBuf == NULL  happens the first time through there is no previously sampled buffer to load into the wave buffer
		if (spi_soundBuf!=NULL)
		{
			for (int i=0; i<(WAV_BUFFER_SIZE/2); i++)
			{
				swapper.bytes8[1] = *(spi_soundBuf +4*i +3);
				audioAndHotspotDataBuffer.wavbuffer[wavbuffer_write_idx][2*i+1] = swapper.bytes8[1];
				swapper.bytes8[0] = *(spi_soundBuf +4*i +2);
				audioAndHotspotDataBuffer.wavbuffer[wavbuffer_write_idx][2*i] = swapper.bytes8[0];
				if (abs(swapper.byte16) > runningMaxValue)
				{
					runningMaxValue = abs(swapper.byte16);
				}
			}
			if (micAudioAverageCounter-- == 0)
			{
				micAudioAverageCounter=MIC_AVERAGE_COUNTER_RELOAD;
				micAudioSamplesTotal = runningMaxValue;
				runningMaxValue=0;
			}

			wavbuffer_write_idx++;

			if (wavbuffer_write_idx>=WAV_BUFFER_COUNT)
			{
				wavbuffer_write_idx=0;
			}
			wavbuffer_count++;
		}

		switch(g_SAI_RX_Handle.queueUser)
		{
		case 0:
			spi_soundBuf = spi_sound1;
			break;
		case 1:
			spi_soundBuf = spi_sound2;
			break;
		case 2:
			spi_soundBuf = spi_sound3;
			break;
		case 3:
			spi_soundBuf = spi_sound4;
			break;
		default:
			spi_soundBuf=spi_sound1;// just put in to please the compiler
			break;
		}

		xfer.data = spi_soundBuf;
		xfer.dataSize = WAV_BUFFER_SIZE*2;

		SAI_TransferReceiveEDMA(I2S0, &g_SAI_RX_Handle, &xfer);
	}
}

void tick_RXsoundbuffer(void)
{
    if (!g_TX_SAI_in_use && wavbuffer_count>6)
    {
    	send_sound_data();
    }
}


void tick_melody(void)
{
	taskENTER_CRITICAL();
	if (melody_play!=NULL)
	{
		if (sine_beep_duration==0)
		{
			if (melody_play[melody_idx]==-1)
			{
				if (trxGetMode() == RADIO_MODE_ANALOG)
				{
					GPIO_PinWrite(GPIO_audio_amp_enable, Pin_audio_amp_enable, 0);// Mute the speaker, otherwise there will be a burst of squelch noise until the next tick in HR-C6000
				    GPIO_PinWrite(GPIO_RX_audio_mux, Pin_RX_audio_mux, 1);// Set the audio path to AT1846 -> audio amp.
				}
				else
				{
					// only mute the speaker if in DMR and not receiving
					if (slot_state !=DMR_STATE_RX_1 && slot_state !=DMR_STATE_RX_2)
					{
						GPIO_PinWrite(GPIO_audio_amp_enable, Pin_audio_amp_enable, 0);
					}
				}
			    set_melody(NULL);
			}
			else
			{
				if (melody_idx==0)
				{
				    GPIO_PinWrite(GPIO_audio_amp_enable, Pin_audio_amp_enable, 1);// enable the speaker (audio amplifier)
					if (trxGetMode() == RADIO_MODE_ANALOG)
					{
					    GPIO_PinWrite(GPIO_RX_audio_mux, Pin_RX_audio_mux, 0);// set the audio mux   HR-C6000 -> audio amp
					}
				}
				sine_beep_freq=melody_play[melody_idx];
				sine_beep_duration=melody_play[melody_idx+1];
				melody_idx=melody_idx+2;
			}
		}
	}
	taskEXIT_CRITICAL();
}


void fw_beep_task(void *data)
{
	// beep mode stuff
	uint8_t tmp_val;
	int beep_idx = 0;
	bool beep = false;
	uint8_t spi_sound[32];

    while (1U)
    {
    	taskENTER_CRITICAL();
    	uint32_t tmp_timer_beeptask=timer_beeptask;
    	taskEXIT_CRITICAL();
    	if (tmp_timer_beeptask==0)
    	{
        	taskENTER_CRITICAL();
        	timer_beeptask=10;
        	alive_beeptask=true;

    		if (sine_beep_duration>0)
    		{
    			if (!beep)
    			{
    				set_clear_SPI_page_reg_byte_with_mask_SPI0(0x04, 0x06, 0xFD, 0x02); // SET
    				beep = true;
    			}

    			read_SPI_page_reg_byte_SPI0(0x04, 0x88, &tmp_val);
    			if ( !(tmp_val & 1) )
    			{
    				for (int i=0; i<16 ;i++)
    				{
    					swapper.byte16 = ((int)sine_beep16[beep_idx])>>soundBeepVolumeDivider;
    					spi_sound[2*i+1]=swapper.bytes8[0];// low byte
    					spi_sound[2*i]=swapper.bytes8[1];// high byte
    					if (sine_beep_freq!=0)
    					{
							beep_idx = beep_idx + (int)(sine_beep_freq/3.915f);
    						if (beep_idx>=0x0800)
    						{
    							beep_idx=beep_idx-0x0800;
    						}
    					}
    				}
    				write_SPI_page_reg_bytearray_SPI0(0x03, 0x00, spi_sound, 0x20);
    			}

    			sine_beep_duration--;
    		}
    		else
    		{
    			if (beep)
    			{
    				set_clear_SPI_page_reg_byte_with_mask_SPI0(0x04, 0x06, 0xFD, 0x00); // CLEAR
    				beep = false;
    			}
    		}
    		taskEXIT_CRITICAL();
    	}

		vTaskDelay(0);
    }
}

void handlePromptAudio(void)
{

	SEGGER_RTT_printf(0, "handlePromptAudio\n");
	if (currentPromptPosition < currentPromptLength)
	{
		currentPromptPosition+=27;
		tick_codec_decode((uint8_t *)&displayoptions_amb[currentPromptPosition]);
	}
	else
	{
		currentPromptPosition=-1;
	    GPIO_PinWrite(GPIO_audio_amp_enable, Pin_audio_amp_enable, 0);// enable the speaker (audio amplifier)
		if (trxGetMode() != RADIO_MODE_ANALOG)
		{
		    GPIO_PinWrite(GPIO_RX_audio_mux, Pin_RX_audio_mux, 1);// set the audio mux   HR-C6000 -> audio amp
		}
	}
}

void playAMBEPrompt(int promptNumber)
{
	currentPromptPosition=0;
	currentPromptLength = displayoptions_amb_len-1;
    GPIO_PinWrite(GPIO_audio_amp_enable, Pin_audio_amp_enable, 1);// enable the speaker (audio amplifier)
	if (trxGetMode() == RADIO_MODE_ANALOG)
	{
	    GPIO_PinWrite(GPIO_RX_audio_mux, Pin_RX_audio_mux, 0);// set the audio mux   HR-C6000 -> audio amp
	}
	tick_codec_decode((uint8_t *)&displayoptions_amb[currentPromptPosition]);
	tick_RXsoundbuffer();
}
