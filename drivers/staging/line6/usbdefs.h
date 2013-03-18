/*
 * Line6 Linux USB driver - 0.9.1beta
 *
 * Copyright (C) 2005-2008 Markus Grabner (grabner@icg.tugraz.at)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 */

#ifndef USBDEFS_H
#define USBDEFS_H

#define LINE6_VENDOR_ID  0x0e41

#define USB_INTERVALS_PER_SECOND 1000

/*
	Device ids.
*/
#define LINE6_DEVID_BASSPODXT     0x4250
#define LINE6_DEVID_BASSPODXTLIVE 0x4642
#define LINE6_DEVID_BASSPODXTPRO  0x4252
#define LINE6_DEVID_GUITARPORT    0x4750
#define LINE6_DEVID_POCKETPOD     0x5051
#define LINE6_DEVID_PODHD300      0x5057
#define LINE6_DEVID_PODHD500      0x414D
#define LINE6_DEVID_PODSTUDIO_GX  0x4153
#define LINE6_DEVID_PODSTUDIO_UX1 0x4150
#define LINE6_DEVID_PODSTUDIO_UX2 0x4151
#define LINE6_DEVID_PODX3         0x414a
#define LINE6_DEVID_PODX3LIVE     0x414b
#define LINE6_DEVID_PODXT         0x5044
#define LINE6_DEVID_PODXTLIVE     0x4650
#define LINE6_DEVID_PODXTPRO      0x5050
#define LINE6_DEVID_TONEPORT_GX   0x4147
#define LINE6_DEVID_TONEPORT_UX1  0x4141
#define LINE6_DEVID_TONEPORT_UX2  0x4142
#define LINE6_DEVID_VARIAX        0x534d

#define LINE6_BIT(x) LINE6_BIT_ ## x = 1 << LINE6_INDEX_ ## x

enum {
	LINE6_INDEX_BASSPODXT,
	LINE6_INDEX_BASSPODXTLIVE,
	LINE6_INDEX_BASSPODXTPRO,
	LINE6_INDEX_GUITARPORT,
	LINE6_INDEX_POCKETPOD,
	LINE6_INDEX_PODHD300,
	LINE6_INDEX_PODHD500,
	LINE6_INDEX_PODSTUDIO_GX,
	LINE6_INDEX_PODSTUDIO_UX1,
	LINE6_INDEX_PODSTUDIO_UX2,
	LINE6_INDEX_PODX3,
	LINE6_INDEX_PODX3LIVE,
	LINE6_INDEX_PODXT,
	LINE6_INDEX_PODXTLIVE,
	LINE6_INDEX_PODXTPRO,
	LINE6_INDEX_TONEPORT_GX,
	LINE6_INDEX_TONEPORT_UX1,
	LINE6_INDEX_TONEPORT_UX2,
	LINE6_INDEX_VARIAX,

	LINE6_BIT(BASSPODXT),
	LINE6_BIT(BASSPODXTLIVE),
	LINE6_BIT(BASSPODXTPRO),
	LINE6_BIT(GUITARPORT),
	LINE6_BIT(POCKETPOD),
	LINE6_BIT(PODHD300),
	LINE6_BIT(PODHD500),
	LINE6_BIT(PODSTUDIO_GX),
	LINE6_BIT(PODSTUDIO_UX1),
	LINE6_BIT(PODSTUDIO_UX2),
	LINE6_BIT(PODX3),
	LINE6_BIT(PODX3LIVE),
	LINE6_BIT(PODXT),
	LINE6_BIT(PODXTLIVE),
	LINE6_BIT(PODXTPRO),
	LINE6_BIT(TONEPORT_GX),
	LINE6_BIT(TONEPORT_UX1),
	LINE6_BIT(TONEPORT_UX2),
	LINE6_BIT(VARIAX),

	LINE6_BITS_PRO = LINE6_BIT_BASSPODXTPRO | LINE6_BIT_PODXTPRO,
	LINE6_BITS_LIVE = LINE6_BIT_BASSPODXTLIVE | LINE6_BIT_PODXTLIVE |
			  LINE6_BIT_PODX3LIVE,
	LINE6_BITS_PODXTALL = LINE6_BIT_PODXT | LINE6_BIT_PODXTLIVE |
			      LINE6_BIT_PODXTPRO,
	LINE6_BITS_PODX3ALL = LINE6_BIT_PODX3 | LINE6_BIT_PODX3LIVE,
	LINE6_BITS_PODHDALL = LINE6_BIT_PODHD300 | LINE6_BIT_PODHD500,
	LINE6_BITS_BASSPODXTALL	= LINE6_BIT_BASSPODXT |
				  LINE6_BIT_BASSPODXTLIVE |
				  LINE6_BIT_BASSPODXTPRO
};

/* device supports settings parameter via USB */
#define LINE6_BIT_CONTROL (1 << 0)
/* device supports PCM input/output via USB */
#define LINE6_BIT_PCM (1 << 1)
/* device support hardware monitoring */
#define LINE6_BIT_HWMON (1 << 2)

#define LINE6_BIT_CONTROL_PCM_HWMON	(LINE6_BIT_CONTROL |	\
					 LINE6_BIT_PCM |	\
					 LINE6_BIT_HWMON)

#define LINE6_FALLBACK_INTERVAL 10
#define LINE6_FALLBACK_MAXPACKETSIZE 16

#endif
