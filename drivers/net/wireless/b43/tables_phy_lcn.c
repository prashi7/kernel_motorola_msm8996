/*

  Broadcom B43 wireless driver
  IEEE 802.11n LCN-PHY data tables

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
  Boston, MA 02110-1301, USA.

*/

#include "b43.h"
#include "tables_phy_lcn.h"
#include "phy_common.h"
#include "phy_lcn.h"

static const u16 b43_lcntab_0x02[] = {
	0x014d, 0x014d, 0x014d, 0x014d, 0x014d, 0x014d,
	0x014d, 0x014d, 0x014d, 0x014d, 0x014d, 0x014d,
	0x014d, 0x014d, 0x014d, 0x014d, 0x014d, 0x014d,
	0x014d, 0x014d, 0x014d, 0x014d, 0x014d, 0x014d,
	0x014d, 0x014d, 0x014d, 0x014d, 0x014d, 0x014d,
	0x014d, 0x014d, 0x014d, 0x014d, 0x014d, 0x014d,
	0x014d, 0x014d, 0x014d, 0x014d, 0x014d, 0x014d,
	0x014d, 0x014d, 0x014d, 0x014d, 0x014d, 0x014d,
	0x014d, 0x014d, 0x014d, 0x014d, 0x014d, 0x014d,
	0x014d, 0x014d, 0x014d, 0x014d, 0x014d, 0x014d,
	0x014d, 0x014d, 0x014d, 0x014d,
};

static const u16 b43_lcntab_0x01[] = {
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
};

static const u32 b43_lcntab_0x0b[] = {
	0x000141f8, 0x000021f8, 0x000021fb, 0x000041fb,
	0x0001fedb, 0x0000217b, 0x00002133, 0x000040eb,
	0x0001fea3, 0x0000024b,
};

static const u32 b43_lcntab_0x0c[] = {
	0x00100001, 0x00200010, 0x00300001, 0x00400010,
	0x00500022, 0x00600122, 0x00700222, 0x00800322,
	0x00900422, 0x00a00522, 0x00b00622, 0x00c00722,
	0x00d00822, 0x00f00922, 0x00100a22, 0x00200b22,
	0x00300c22, 0x00400d22, 0x00500e22, 0x00600f22,
};

static const u32 b43_lcntab_0x0d[] = {
	0x00000000, 0x00000000, 0x10000000, 0x00000000,
	0x20000000, 0x00000000, 0x30000000, 0x00000000,
	0x40000000, 0x00000000, 0x50000000, 0x00000000,
	0x60000000, 0x00000000, 0x70000000, 0x00000000,
	0x80000000, 0x00000000, 0x90000000, 0x00000008,
	0xa0000000, 0x00000008, 0xb0000000, 0x00000008,
	0xc0000000, 0x00000008, 0xd0000000, 0x00000008,
	0xe0000000, 0x00000008, 0xf0000000, 0x00000008,
	0x00000000, 0x00000009, 0x10000000, 0x00000009,
	0x20000000, 0x00000019, 0x30000000, 0x00000019,
	0x40000000, 0x00000019, 0x50000000, 0x00000019,
	0x60000000, 0x00000019, 0x70000000, 0x00000019,
	0x80000000, 0x00000019, 0x90000000, 0x00000019,
	0xa0000000, 0x00000019, 0xb0000000, 0x00000019,
	0xc0000000, 0x00000019, 0xd0000000, 0x00000019,
	0xe0000000, 0x00000019, 0xf0000000, 0x00000019,
	0x00000000, 0x0000001a, 0x10000000, 0x0000001a,
	0x20000000, 0x0000001a, 0x30000000, 0x0000001a,
	0x40000000, 0x0000001a, 0x50000000, 0x00000002,
	0x60000000, 0x00000002, 0x70000000, 0x00000002,
	0x80000000, 0x00000002, 0x90000000, 0x00000002,
	0xa0000000, 0x00000002, 0xb0000000, 0x00000002,
	0xc0000000, 0x0000000a, 0xd0000000, 0x0000000a,
	0xe0000000, 0x0000000a, 0xf0000000, 0x0000000a,
	0x00000000, 0x0000000b, 0x10000000, 0x0000000b,
	0x20000000, 0x0000000b, 0x30000000, 0x0000000b,
	0x40000000, 0x0000000b, 0x50000000, 0x0000001b,
	0x60000000, 0x0000001b, 0x70000000, 0x0000001b,
	0x80000000, 0x0000001b, 0x90000000, 0x0000001b,
	0xa0000000, 0x0000001b, 0xb0000000, 0x0000001b,
	0xc0000000, 0x0000001b, 0xd0000000, 0x0000001b,
	0xe0000000, 0x0000001b, 0xf0000000, 0x0000001b,
	0x00000000, 0x0000001c, 0x10000000, 0x0000001c,
	0x20000000, 0x0000001c, 0x30000000, 0x0000001c,
	0x40000000, 0x0000001c, 0x50000000, 0x0000001c,
	0x60000000, 0x0000001c, 0x70000000, 0x0000001c,
	0x80000000, 0x0000001c, 0x90000000, 0x0000001c,
};

static const u16 b43_lcntab_0x0e[] = {
	0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406,
	0x0407, 0x0408, 0x0409, 0x040a, 0x058b, 0x058c,
	0x058d, 0x058e, 0x058f, 0x0090, 0x0091, 0x0092,
	0x0193, 0x0194, 0x0195, 0x0196, 0x0197, 0x0198,
	0x0199, 0x019a, 0x019b, 0x019c, 0x019d, 0x019e,
	0x019f, 0x01a0, 0x01a1, 0x01a2, 0x01a3, 0x01a4,
	0x01a5, 0x0000,
};

static const u16 b43_lcntab_0x0f[] = {
	0x000a, 0x0009, 0x0006, 0x0005, 0x000a, 0x0009,
	0x0006, 0x0005, 0x000a, 0x0009, 0x0006, 0x0005,
	0x000a, 0x0009, 0x0006, 0x0005, 0x000a, 0x0009,
	0x0006, 0x0005, 0x000a, 0x0009, 0x0006, 0x0005,
	0x000a, 0x0009, 0x0006, 0x0005, 0x000a, 0x0009,
	0x0006, 0x0005, 0x000a, 0x0009, 0x0006, 0x0005,
	0x000a, 0x0009, 0x0006, 0x0005, 0x000a, 0x0009,
	0x0006, 0x0005, 0x000a, 0x0009, 0x0006, 0x0005,
	0x000a, 0x0009, 0x0006, 0x0005, 0x000a, 0x0009,
	0x0006, 0x0005, 0x000a, 0x0009, 0x0006, 0x0005,
	0x000a, 0x0009, 0x0006, 0x0005,
};

static const u16 b43_lcntab_0x10[] = {
	0x005f, 0x0036, 0x0029, 0x001f, 0x005f, 0x0036,
	0x0029, 0x001f, 0x005f, 0x0036, 0x0029, 0x001f,
	0x005f, 0x0036, 0x0029, 0x001f,
};

static const u16 b43_lcntab_0x11[] = {
	0x0009, 0x000f, 0x0014, 0x0018, 0x00fe, 0x0007,
	0x000b, 0x000f, 0x00fb, 0x00fe, 0x0001, 0x0005,
	0x0008, 0x000b, 0x000e, 0x0011, 0x0014, 0x0017,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0003, 0x0006, 0x0009, 0x000c, 0x000f,
	0x0012, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0003,
	0x0006, 0x0009, 0x000c, 0x000f, 0x0012, 0x0015,
	0x0018, 0x001b, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0003, 0x00eb, 0x0000, 0x0000,
};

static const u32 b43_lcntab_0x12[] = {
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000004, 0x00000000, 0x00000004, 0x00000008,
	0x00000001, 0x00000005, 0x00000009, 0x0000000d,
	0x0000004d, 0x0000008d, 0x0000000d, 0x0000004d,
	0x0000008d, 0x000000cd, 0x0000004f, 0x0000008f,
	0x000000cf, 0x000000d3, 0x00000113, 0x00000513,
	0x00000913, 0x00000953, 0x00000d53, 0x00001153,
	0x00001193, 0x00005193, 0x00009193, 0x0000d193,
	0x00011193, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000004,
	0x00000000, 0x00000004, 0x00000008, 0x00000001,
	0x00000005, 0x00000009, 0x0000000d, 0x0000004d,
	0x0000008d, 0x0000000d, 0x0000004d, 0x0000008d,
	0x000000cd, 0x0000004f, 0x0000008f, 0x000000cf,
	0x000000d3, 0x00000113, 0x00000513, 0x00000913,
	0x00000953, 0x00000d53, 0x00001153, 0x00005153,
	0x00009153, 0x0000d153, 0x00011153, 0x00015153,
	0x00019153, 0x0001d153, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

static const u16 b43_lcntab_0x14[] = {
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0002, 0x0003, 0x0001, 0x0003, 0x0002, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0002, 0x0003,
	0x0001, 0x0003, 0x0002, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
	0x0001, 0x0001,
};

static const u16 b43_lcntab_0x17[] = {
	0x001a, 0x0034, 0x004e, 0x0068, 0x009c, 0x00d0,
	0x00ea, 0x0104, 0x0034, 0x0068, 0x009c, 0x00d0,
	0x0138, 0x01a0, 0x01d4, 0x0208, 0x004e, 0x009c,
	0x00ea, 0x0138, 0x01d4, 0x0270, 0x02be, 0x030c,
	0x0068, 0x00d0, 0x0138, 0x01a0, 0x0270, 0x0340,
	0x03a8, 0x0410, 0x0018, 0x009c, 0x00d0, 0x0104,
	0x00ea, 0x0138, 0x0186, 0x00d0, 0x0104, 0x0104,
	0x0138, 0x016c, 0x016c, 0x01a0, 0x0138, 0x0186,
	0x0186, 0x01d4, 0x0222, 0x0222, 0x0270, 0x0104,
	0x0138, 0x016c, 0x0138, 0x016c, 0x01a0, 0x01d4,
	0x01a0, 0x01d4, 0x0208, 0x0208, 0x023c, 0x0186,
	0x01d4, 0x0222, 0x01d4, 0x0222, 0x0270, 0x02be,
	0x0270, 0x02be, 0x030c, 0x030c, 0x035a, 0x0036,
	0x006c, 0x00a2, 0x00d8, 0x0144, 0x01b0, 0x01e6,
	0x021c, 0x006c, 0x00d8, 0x0144, 0x01b0, 0x0288,
	0x0360, 0x03cc, 0x0438, 0x00a2, 0x0144, 0x01e6,
	0x0288, 0x03cc, 0x0510, 0x05b2, 0x0654, 0x00d8,
	0x01b0, 0x0288, 0x0360, 0x0510, 0x06c0, 0x0798,
	0x0870, 0x0018, 0x0144, 0x01b0, 0x021c, 0x01e6,
	0x0288, 0x032a, 0x01b0, 0x021c, 0x021c, 0x0288,
	0x02f4, 0x02f4, 0x0360, 0x0288, 0x032a, 0x032a,
	0x03cc, 0x046e, 0x046e, 0x0510, 0x021c, 0x0288,
	0x02f4, 0x0288, 0x02f4, 0x0360, 0x03cc, 0x0360,
	0x03cc, 0x0438, 0x0438, 0x04a4, 0x032a, 0x03cc,
	0x046e, 0x03cc, 0x046e, 0x0510, 0x05b2, 0x0510,
	0x05b2, 0x0654, 0x0654, 0x06f6,
};

static const u16 b43_lcntab_0x00[] = {
	0x0200, 0x0300, 0x0400, 0x0600, 0x0800, 0x0b00,
	0x1000, 0x1001, 0x1002, 0x1003, 0x1004, 0x1005,
	0x1006, 0x1007, 0x1707, 0x2007, 0x2d07, 0x4007,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0200, 0x0300, 0x0400, 0x0600,
	0x0800, 0x0b00, 0x1000, 0x1001, 0x1002, 0x1003,
	0x1004, 0x1005, 0x1006, 0x1007, 0x1707, 0x2007,
	0x2d07, 0x4007, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x4000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

static const u32 b43_lcntab_0x18[] = {
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
	0x00080000, 0x00080000, 0x00080000, 0x00080000,
};

const u16 b43_lcntab_sw_ctl_4313_epa_rev0[] = {
	0x0002, 0x0008, 0x0004, 0x0001, 0x0002, 0x0008,
	0x0004, 0x0001, 0x0002, 0x0008, 0x0004, 0x0001,
	0x0002, 0x0008, 0x0004, 0x0001, 0x0002, 0x0008,
	0x0004, 0x0001, 0x0002, 0x0008, 0x0004, 0x0001,
	0x0002, 0x0008, 0x0004, 0x0001, 0x0002, 0x0008,
	0x0004, 0x0001, 0x0002, 0x0008, 0x0004, 0x0001,
	0x0002, 0x0008, 0x0004, 0x0001, 0x0002, 0x0008,
	0x0004, 0x0001, 0x0002, 0x0008, 0x0004, 0x0001,
	0x0002, 0x0008, 0x0004, 0x0001, 0x0002, 0x0008,
	0x0004, 0x0001, 0x0002, 0x0008, 0x0004, 0x0001,
	0x0002, 0x0008, 0x0004, 0x0001,
};

/**************************************************
 * R/W ops.
 **************************************************/

u32 b43_lcntab_read(struct b43_wldev *dev, u32 offset)
{
	u32 type, value;

	type = offset & B43_LCNTAB_TYPEMASK;
	offset &= ~B43_LCNTAB_TYPEMASK;
	B43_WARN_ON(offset > 0xFFFF);

	switch (type) {
	case B43_LCNTAB_8BIT:
		b43_phy_write(dev, B43_PHY_LCN_TABLE_ADDR, offset);
		value = b43_phy_read(dev, B43_PHY_LCN_TABLE_DATALO) & 0xFF;
		break;
	case B43_LCNTAB_16BIT:
		b43_phy_write(dev, B43_PHY_LCN_TABLE_ADDR, offset);
		value = b43_phy_read(dev, B43_PHY_LCN_TABLE_DATALO);
		break;
	case B43_LCNTAB_32BIT:
		b43_phy_write(dev, B43_PHY_LCN_TABLE_ADDR, offset);
		value = b43_phy_read(dev, B43_PHY_LCN_TABLE_DATALO);
		value |= (b43_phy_read(dev, B43_PHY_LCN_TABLE_DATAHI) << 16);
		break;
	default:
		B43_WARN_ON(1);
		value = 0;
	}

	return value;
}

void b43_lcntab_read_bulk(struct b43_wldev *dev, u32 offset,
			  unsigned int nr_elements, void *_data)
{
	u32 type;
	u8 *data = _data;
	unsigned int i;

	type = offset & B43_LCNTAB_TYPEMASK;
	offset &= ~B43_LCNTAB_TYPEMASK;
	B43_WARN_ON(offset > 0xFFFF);

	b43_phy_write(dev, B43_PHY_LCN_TABLE_ADDR, offset);

	for (i = 0; i < nr_elements; i++) {
		switch (type) {
		case B43_LCNTAB_8BIT:
			*data = b43_phy_read(dev,
					     B43_PHY_LCN_TABLE_DATALO) & 0xFF;
			data++;
			break;
		case B43_LCNTAB_16BIT:
			*((u16 *)data) = b43_phy_read(dev,
						      B43_PHY_LCN_TABLE_DATALO);
			data += 2;
			break;
		case B43_LCNTAB_32BIT:
			*((u32 *)data) = b43_phy_read(dev,
						B43_PHY_LCN_TABLE_DATALO);
			*((u32 *)data) |= (b43_phy_read(dev,
					   B43_PHY_LCN_TABLE_DATAHI) << 16);
			data += 4;
			break;
		default:
			B43_WARN_ON(1);
		}
	}
}

void b43_lcntab_write(struct b43_wldev *dev, u32 offset, u32 value)
{
	u32 type;

	type = offset & B43_LCNTAB_TYPEMASK;
	offset &= 0xFFFF;

	switch (type) {
	case B43_LCNTAB_8BIT:
		B43_WARN_ON(value & ~0xFF);
		b43_phy_write(dev, B43_PHY_LCN_TABLE_ADDR, offset);
		b43_phy_write(dev, B43_PHY_LCN_TABLE_DATALO, value);
		break;
	case B43_LCNTAB_16BIT:
		B43_WARN_ON(value & ~0xFFFF);
		b43_phy_write(dev, B43_PHY_LCN_TABLE_ADDR, offset);
		b43_phy_write(dev, B43_PHY_LCN_TABLE_DATALO, value);
		break;
	case B43_LCNTAB_32BIT:
		b43_phy_write(dev, B43_PHY_LCN_TABLE_ADDR, offset);
		b43_phy_write(dev, B43_PHY_LCN_TABLE_DATAHI, value >> 16);
		b43_phy_write(dev, B43_PHY_LCN_TABLE_DATALO, value & 0xFFFF);
		break;
	default:
		B43_WARN_ON(1);
	}

	return;
}

void b43_lcntab_write_bulk(struct b43_wldev *dev, u32 offset,
			   unsigned int nr_elements, const void *_data)
{
	u32 type, value;
	const u8 *data = _data;
	unsigned int i;

	type = offset & B43_LCNTAB_TYPEMASK;
	offset &= ~B43_LCNTAB_TYPEMASK;
	B43_WARN_ON(offset > 0xFFFF);

	b43_phy_write(dev, B43_PHY_LCN_TABLE_ADDR, offset);

	for (i = 0; i < nr_elements; i++) {
		switch (type) {
		case B43_LCNTAB_8BIT:
			value = *data;
			data++;
			B43_WARN_ON(value & ~0xFF);
			b43_phy_write(dev, B43_PHY_LCN_TABLE_DATALO, value);
			break;
		case B43_LCNTAB_16BIT:
			value = *((u16 *)data);
			data += 2;
			B43_WARN_ON(value & ~0xFFFF);
			b43_phy_write(dev, B43_PHY_LCN_TABLE_DATALO, value);
			break;
		case B43_LCNTAB_32BIT:
			value = *((u32 *)data);
			data += 4;
			b43_phy_write(dev, B43_PHY_LCN_TABLE_DATAHI,
				      value >> 16);
			b43_phy_write(dev, B43_PHY_LCN_TABLE_DATALO,
				      value & 0xFFFF);
			break;
		default:
			B43_WARN_ON(1);
		}
	}
}

/**************************************************
 * Tables ops.
 **************************************************/

#define lcntab_upload(dev, offset, data) do { \
		b43_lcntab_write_bulk(dev, offset, ARRAY_SIZE(data), data); \
	} while (0)
static void b43_phy_lcn_upload_static_tables(struct b43_wldev *dev)
{
	lcntab_upload(dev, B43_LCNTAB16(0x02, 0), b43_lcntab_0x02);
	lcntab_upload(dev, B43_LCNTAB16(0x01, 0), b43_lcntab_0x01);
	lcntab_upload(dev, B43_LCNTAB32(0x0b, 0), b43_lcntab_0x0b);
	lcntab_upload(dev, B43_LCNTAB32(0x0c, 0), b43_lcntab_0x0c);
	lcntab_upload(dev, B43_LCNTAB32(0x0d, 0), b43_lcntab_0x0d);
	lcntab_upload(dev, B43_LCNTAB16(0x0e, 0), b43_lcntab_0x0e);
	lcntab_upload(dev, B43_LCNTAB16(0x0f, 0), b43_lcntab_0x0f);
	lcntab_upload(dev, B43_LCNTAB16(0x10, 0), b43_lcntab_0x10);
	lcntab_upload(dev, B43_LCNTAB16(0x11, 0), b43_lcntab_0x11);
	lcntab_upload(dev, B43_LCNTAB32(0x12, 0), b43_lcntab_0x12);
	lcntab_upload(dev, B43_LCNTAB16(0x14, 0), b43_lcntab_0x14);
	lcntab_upload(dev, B43_LCNTAB16(0x17, 0), b43_lcntab_0x17);
	lcntab_upload(dev, B43_LCNTAB16(0x00, 0), b43_lcntab_0x00);
	lcntab_upload(dev, B43_LCNTAB32(0x18, 0), b43_lcntab_0x18);
}

/* Not implemented in brcmsmac, noticed in wl in MMIO dump */
static void b43_phy_lcn_rewrite_tables(struct b43_wldev *dev)
{
	int i;
	u32 tmp;
	for (i = 0; i < 128; i++) {
		tmp = b43_lcntab_read(dev, B43_LCNTAB32(0x7, 0x240 + i));
		b43_lcntab_write(dev, B43_LCNTAB32(0x7, 0x240 + i), tmp);
	}
}

/* wlc_lcnphy_clear_papd_comptable */
static void b43_phy_lcn_clean_papd_comp_table(struct b43_wldev *dev)
{
	u8 i;

	for (i = 0; i < 0x80; i++)
		b43_lcntab_write(dev, B43_LCNTAB32(0x18, i), 0x80000);
}

void b43_phy_lcn_tables_init(struct b43_wldev *dev)
{
	b43_phy_lcn_upload_static_tables(dev);
	/* TODO: various tables ops here */
	b43_lcntab_write_bulk(dev, B43_LCNTAB16(0xf, 0),
			ARRAY_SIZE(b43_lcntab_sw_ctl_4313_epa_rev0),
			b43_lcntab_sw_ctl_4313_epa_rev0);
	/* TODO: various tables ops here */
	b43_phy_lcn_rewrite_tables(dev);
	b43_phy_lcn_clean_papd_comp_table(dev);
}
