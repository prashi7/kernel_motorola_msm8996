/* em-terratec.h - Keytable for em_terratec Remote Controller
 *
 * Imported from ir-keymaps.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#ifdef IR_KEYMAPS
static struct ir_scancode em_terratec[] = {
	{ 0x01, KEY_CHANNEL },
	{ 0x02, KEY_SELECT },
	{ 0x03, KEY_MUTE },
	{ 0x04, KEY_POWER },
	{ 0x05, KEY_1 },
	{ 0x06, KEY_2 },
	{ 0x07, KEY_3 },
	{ 0x08, KEY_CHANNELUP },
	{ 0x09, KEY_4 },
	{ 0x0a, KEY_5 },
	{ 0x0b, KEY_6 },
	{ 0x0c, KEY_CHANNELDOWN },
	{ 0x0d, KEY_7 },
	{ 0x0e, KEY_8 },
	{ 0x0f, KEY_9 },
	{ 0x10, KEY_VOLUMEUP },
	{ 0x11, KEY_0 },
	{ 0x12, KEY_MENU },
	{ 0x13, KEY_PRINT },
	{ 0x14, KEY_VOLUMEDOWN },
	{ 0x16, KEY_PAUSE },
	{ 0x18, KEY_RECORD },
	{ 0x19, KEY_REWIND },
	{ 0x1a, KEY_PLAY },
	{ 0x1b, KEY_FORWARD },
	{ 0x1c, KEY_BACKSPACE },
	{ 0x1e, KEY_STOP },
	{ 0x40, KEY_ZOOM },
};
DEFINE_LEGACY_IR_KEYTABLE(em_terratec);
#else
DECLARE_IR_KEYTABLE(em_terratec);
#endif
