/*
 * Copyright (c) 1996, 2003 VIA Networking Technologies, Inc.
 * All rights reserved.
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * File: rxtx.h
 *
 * Purpose:
 *
 * Author: Jerry Chen
 *
 * Date: Jun. 27, 2002
 *
 */

#ifndef __RXTX_H__
#define __RXTX_H__

#include "device.h"
#include "wcmd.h"

/* RsvTime buffer header */
struct vnt_rrv_time_rts {
	u16 wRTSTxRrvTime_ba;
	u16 wRTSTxRrvTime_aa;
	u16 wRTSTxRrvTime_bb;
	u16 wReserved;
	u16 wTxRrvTime_b;
	u16 wTxRrvTime_a;
} __packed;

struct vnt_rrv_time_cts {
	u16 wCTSTxRrvTime_ba;
	u16 wReserved;
	u16 wTxRrvTime_b;
	u16 wTxRrvTime_a;
} __packed;

/* RTS buffer header */
struct vnt_rts_g {
	u8 bySignalField_b;
	u8 byServiceField_b;
	u16 wTransmitLength_b;
	u8 bySignalField_a;
	u8 byServiceField_a;
	u16 wTransmitLength_a;
	u16 wDuration_ba;
	u16 wDuration_aa;
	u16 wDuration_bb;
	u16 wReserved;
	struct ieee80211_rts data;
} __packed;

struct vnt_rts_g_fb {
	u8 bySignalField_b;
	u8 byServiceField_b;
	u16 wTransmitLength_b;
	u8 bySignalField_a;
	u8 byServiceField_a;
	u16 wTransmitLength_a;
	u16 wDuration_ba;
	u16 wDuration_aa;
	u16 wDuration_bb;
	u16 wReserved;
	u16 wRTSDuration_ba_f0;
	u16 wRTSDuration_aa_f0;
	u16 wRTSDuration_ba_f1;
	u16 wRTSDuration_aa_f1;
	struct ieee80211_rts data;
} __packed;

struct vnt_rts_ab {
	u8 bySignalField;
	u8 byServiceField;
	u16 wTransmitLength;
	u16 wDuration;
	u16 wReserved;
	struct ieee80211_rts data;
} __packed;

struct vnt_rts_a_fb {
	u8 bySignalField;
	u8 byServiceField;
	u16 wTransmitLength;
	u16 wDuration;
	u16 wReserved;
	u16 wRTSDuration_f0;
	u16 wRTSDuration_f1;
	struct ieee80211_rts data;
} __packed;

/* CTS buffer header */
struct vnt_cts {
	u8 bySignalField_b;
	u8 byServiceField_b;
	u16 wTransmitLength_b;
	u16 wDuration_ba;
	u16 wReserved;
	struct ieee80211_cts data;
	u16 reserved2;
} __packed;

struct vnt_cts_fb {
	u8 bySignalField_b;
	u8 byServiceField_b;
	u16 wTransmitLength_b;
	u16 wDuration_ba;
	u16 wReserved;
	u16 wCTSDuration_ba_f0;
	u16 wCTSDuration_ba_f1;
	struct ieee80211_cts data;
	u16 reserved2;
} __packed;

struct vnt_tx_buffer {
	u8 byType;
	u8 byPKTNO;
	u16 wTxByteCount;
	u32 adwTxKey[4];
	u16 wFIFOCtl;
	u16 wTimeStamp;
	u16 wFragCtl;
	u16 wReserved;
} __packed;

struct vnt_beacon_buffer {
	u8 byType;
	u8 byPKTNO;
	u16 wTxByteCount;
	u16 wFIFOCtl;
	u16 wTimeStamp;
} __packed;

void vDMA0_tx_80211(struct vnt_private *, struct sk_buff *skb);
int nsDMA_tx_packet(struct vnt_private *, u32 uDMAIdx, struct sk_buff *skb);
CMD_STATUS csMgmt_xmit(struct vnt_private *, struct vnt_tx_mgmt *);
CMD_STATUS csBeacon_xmit(struct vnt_private *, struct vnt_tx_mgmt *);
int bRelayPacketSend(struct vnt_private *, u8 *pbySkbData, u32 uDataLen,
	u32 uNodeIndex);

#endif /* __RXTX_H__ */
