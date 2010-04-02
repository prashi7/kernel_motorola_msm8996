/* iodata-bctv7e.h - Keytable for iodata_bctv7e Remote Controller
 *
 * Imported from ir-keymaps.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* IO-DATA BCTV7E Remote */

#ifdef IR_KEYMAPS
static struct ir_scancode iodata_bctv7e[] = {
	{ 0x40, KEY_TV },
	{ 0x20, KEY_RADIO },		/* FM */
	{ 0x60, KEY_EPG },
	{ 0x00, KEY_POWER },

	/* Keys 0 to 9 */
	{ 0x44, KEY_0 },		/* 10 */
	{ 0x50, KEY_1 },
	{ 0x30, KEY_2 },
	{ 0x70, KEY_3 },
	{ 0x48, KEY_4 },
	{ 0x28, KEY_5 },
	{ 0x68, KEY_6 },
	{ 0x58, KEY_7 },
	{ 0x38, KEY_8 },
	{ 0x78, KEY_9 },

	{ 0x10, KEY_L },		/* Live */
	{ 0x08, KEY_TIME },		/* Time Shift */

	{ 0x18, KEY_PLAYPAUSE },	/* Play */

	{ 0x24, KEY_ENTER },		/* 11 */
	{ 0x64, KEY_ESC },		/* 12 */
	{ 0x04, KEY_M },		/* Multi */

	{ 0x54, KEY_VIDEO },
	{ 0x34, KEY_CHANNELUP },
	{ 0x74, KEY_VOLUMEUP },
	{ 0x14, KEY_MUTE },

	{ 0x4c, KEY_VCR },		/* SVIDEO */
	{ 0x2c, KEY_CHANNELDOWN },
	{ 0x6c, KEY_VOLUMEDOWN },
	{ 0x0c, KEY_ZOOM },

	{ 0x5c, KEY_PAUSE },
	{ 0x3c, KEY_RED },		/* || (red) */
	{ 0x7c, KEY_RECORD },		/* recording */
	{ 0x1c, KEY_STOP },

	{ 0x41, KEY_REWIND },		/* backward << */
	{ 0x21, KEY_PLAY },
	{ 0x61, KEY_FASTFORWARD },	/* forward >> */
	{ 0x01, KEY_NEXT },		/* skip >| */
};
DEFINE_LEGACY_IR_KEYTABLE(iodata_bctv7e);
#else
DECLARE_IR_KEYTABLE(iodata_bctv7e);
#endif
