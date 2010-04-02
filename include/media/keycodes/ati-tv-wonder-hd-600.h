/* ati-tv-wonder-hd-600.h - Keytable for ati_tv_wonder_hd_600 Remote Controller
 *
 * Imported from ir-keymaps.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* ATI TV Wonder HD 600 USB
   Devin Heitmueller <devin.heitmueller@gmail.com>
 */

#ifdef IR_KEYMAPS
static struct ir_scancode ati_tv_wonder_hd_600[] = {
	{ 0x00, KEY_RECORD},		/* Row 1 */
	{ 0x01, KEY_PLAYPAUSE},
	{ 0x02, KEY_STOP},
	{ 0x03, KEY_POWER},
	{ 0x04, KEY_PREVIOUS},	/* Row 2 */
	{ 0x05, KEY_REWIND},
	{ 0x06, KEY_FORWARD},
	{ 0x07, KEY_NEXT},
	{ 0x08, KEY_EPG},		/* Row 3 */
	{ 0x09, KEY_HOME},
	{ 0x0a, KEY_MENU},
	{ 0x0b, KEY_CHANNELUP},
	{ 0x0c, KEY_BACK},		/* Row 4 */
	{ 0x0d, KEY_UP},
	{ 0x0e, KEY_INFO},
	{ 0x0f, KEY_CHANNELDOWN},
	{ 0x10, KEY_LEFT},		/* Row 5 */
	{ 0x11, KEY_SELECT},
	{ 0x12, KEY_RIGHT},
	{ 0x13, KEY_VOLUMEUP},
	{ 0x14, KEY_LAST},		/* Row 6 */
	{ 0x15, KEY_DOWN},
	{ 0x16, KEY_MUTE},
	{ 0x17, KEY_VOLUMEDOWN},
};
DEFINE_LEGACY_IR_KEYTABLE(ati_tv_wonder_hd_600);
#else
DECLARE_IR_KEYTABLE(ati_tv_wonder_hd_600);
#endif
