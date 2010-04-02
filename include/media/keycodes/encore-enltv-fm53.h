/* encore-enltv-fm53.h - Keytable for encore_enltv_fm53 Remote Controller
 *
 * Imported from ir-keymaps.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* Encore ENLTV-FM v5.3
   Mauro Carvalho Chehab <mchehab@infradead.org>
 */

#ifdef IR_KEYMAPS
static struct ir_scancode encore_enltv_fm53[] = {
	{ 0x10, KEY_POWER2},
	{ 0x06, KEY_MUTE},

	{ 0x09, KEY_1},
	{ 0x1d, KEY_2},
	{ 0x1f, KEY_3},
	{ 0x19, KEY_4},
	{ 0x1b, KEY_5},
	{ 0x11, KEY_6},
	{ 0x17, KEY_7},
	{ 0x12, KEY_8},
	{ 0x16, KEY_9},
	{ 0x48, KEY_0},

	{ 0x04, KEY_LIST},		/* -/-- */
	{ 0x40, KEY_LAST},		/* recall */

	{ 0x02, KEY_MODE},		/* TV/AV */
	{ 0x05, KEY_CAMERA},		/* SNAPSHOT */

	{ 0x4c, KEY_CHANNELUP},		/* UP */
	{ 0x00, KEY_CHANNELDOWN},	/* DOWN */
	{ 0x0d, KEY_VOLUMEUP},		/* RIGHT */
	{ 0x15, KEY_VOLUMEDOWN},	/* LEFT */
	{ 0x49, KEY_ENTER},		/* OK */

	{ 0x54, KEY_RECORD},
	{ 0x4d, KEY_PLAY},		/* pause */

	{ 0x1e, KEY_MENU},		/* video setting */
	{ 0x0e, KEY_RIGHT},		/* <- */
	{ 0x1a, KEY_LEFT},		/* -> */

	{ 0x0a, KEY_CLEAR},		/* video default */
	{ 0x0c, KEY_ZOOM},		/* hide pannel */
	{ 0x47, KEY_SLEEP},		/* shutdown */
};
DEFINE_LEGACY_IR_KEYTABLE(encore_enltv_fm53);
#else
DECLARE_IR_KEYTABLE(encore_enltv_fm53);
#endif
