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
 * File: aes_ccmp.c
 *
 * Purpose: AES_CCMP decryption
 *
 * Author: Warren Hsu
 *
 * Date: Feb 15, 2005
 *
 * Functions:
 *      AESbGenCCMP - Parsing RX-packet
 *
 * Revision History:
 */

#include "device.h"
#include "80211hdr.h"

/*---------------------  Static Definitions -------------------------*/

/*---------------------  Static Classes  ----------------------------*/

/*---------------------  Static Variables  --------------------------*/

/*
 * SBOX Table
 */

BYTE sbox_table[256] = {
	0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
	0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
	0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
	0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
	0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
	0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
	0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
	0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
	0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
	0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
	0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
	0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
	0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
	0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
	0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
	0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

BYTE dot2_table[256] = {
	0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e,
	0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e,
	0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e, 0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e,
	0x60, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6c, 0x6e, 0x70, 0x72, 0x74, 0x76, 0x78, 0x7a, 0x7c, 0x7e,
	0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e, 0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e,
	0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbe,
	0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce, 0xd0, 0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde,
	0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee, 0xf0, 0xf2, 0xf4, 0xf6, 0xf8, 0xfa, 0xfc, 0xfe,
	0x1b, 0x19, 0x1f, 0x1d, 0x13, 0x11, 0x17, 0x15, 0x0b, 0x09, 0x0f, 0x0d, 0x03, 0x01, 0x07, 0x05,
	0x3b, 0x39, 0x3f, 0x3d, 0x33, 0x31, 0x37, 0x35, 0x2b, 0x29, 0x2f, 0x2d, 0x23, 0x21, 0x27, 0x25,
	0x5b, 0x59, 0x5f, 0x5d, 0x53, 0x51, 0x57, 0x55, 0x4b, 0x49, 0x4f, 0x4d, 0x43, 0x41, 0x47, 0x45,
	0x7b, 0x79, 0x7f, 0x7d, 0x73, 0x71, 0x77, 0x75, 0x6b, 0x69, 0x6f, 0x6d, 0x63, 0x61, 0x67, 0x65,
	0x9b, 0x99, 0x9f, 0x9d, 0x93, 0x91, 0x97, 0x95, 0x8b, 0x89, 0x8f, 0x8d, 0x83, 0x81, 0x87, 0x85,
	0xbb, 0xb9, 0xbf, 0xbd, 0xb3, 0xb1, 0xb7, 0xb5, 0xab, 0xa9, 0xaf, 0xad, 0xa3, 0xa1, 0xa7, 0xa5,
	0xdb, 0xd9, 0xdf, 0xdd, 0xd3, 0xd1, 0xd7, 0xd5, 0xcb, 0xc9, 0xcf, 0xcd, 0xc3, 0xc1, 0xc7, 0xc5,
	0xfb, 0xf9, 0xff, 0xfd, 0xf3, 0xf1, 0xf7, 0xf5, 0xeb, 0xe9, 0xef, 0xed, 0xe3, 0xe1, 0xe7, 0xe5
};

BYTE dot3_table[256] = {
	0x00, 0x03, 0x06, 0x05, 0x0c, 0x0f, 0x0a, 0x09, 0x18, 0x1b, 0x1e, 0x1d, 0x14, 0x17, 0x12, 0x11,
	0x30, 0x33, 0x36, 0x35, 0x3c, 0x3f, 0x3a, 0x39, 0x28, 0x2b, 0x2e, 0x2d, 0x24, 0x27, 0x22, 0x21,
	0x60, 0x63, 0x66, 0x65, 0x6c, 0x6f, 0x6a, 0x69, 0x78, 0x7b, 0x7e, 0x7d, 0x74, 0x77, 0x72, 0x71,
	0x50, 0x53, 0x56, 0x55, 0x5c, 0x5f, 0x5a, 0x59, 0x48, 0x4b, 0x4e, 0x4d, 0x44, 0x47, 0x42, 0x41,
	0xc0, 0xc3, 0xc6, 0xc5, 0xcc, 0xcf, 0xca, 0xc9, 0xd8, 0xdb, 0xde, 0xdd, 0xd4, 0xd7, 0xd2, 0xd1,
	0xf0, 0xf3, 0xf6, 0xf5, 0xfc, 0xff, 0xfa, 0xf9, 0xe8, 0xeb, 0xee, 0xed, 0xe4, 0xe7, 0xe2, 0xe1,
	0xa0, 0xa3, 0xa6, 0xa5, 0xac, 0xaf, 0xaa, 0xa9, 0xb8, 0xbb, 0xbe, 0xbd, 0xb4, 0xb7, 0xb2, 0xb1,
	0x90, 0x93, 0x96, 0x95, 0x9c, 0x9f, 0x9a, 0x99, 0x88, 0x8b, 0x8e, 0x8d, 0x84, 0x87, 0x82, 0x81,
	0x9b, 0x98, 0x9d, 0x9e, 0x97, 0x94, 0x91, 0x92, 0x83, 0x80, 0x85, 0x86, 0x8f, 0x8c, 0x89, 0x8a,
	0xab, 0xa8, 0xad, 0xae, 0xa7, 0xa4, 0xa1, 0xa2, 0xb3, 0xb0, 0xb5, 0xb6, 0xbf, 0xbc, 0xb9, 0xba,
	0xfb, 0xf8, 0xfd, 0xfe, 0xf7, 0xf4, 0xf1, 0xf2, 0xe3, 0xe0, 0xe5, 0xe6, 0xef, 0xec, 0xe9, 0xea,
	0xcb, 0xc8, 0xcd, 0xce, 0xc7, 0xc4, 0xc1, 0xc2, 0xd3, 0xd0, 0xd5, 0xd6, 0xdf, 0xdc, 0xd9, 0xda,
	0x5b, 0x58, 0x5d, 0x5e, 0x57, 0x54, 0x51, 0x52, 0x43, 0x40, 0x45, 0x46, 0x4f, 0x4c, 0x49, 0x4a,
	0x6b, 0x68, 0x6d, 0x6e, 0x67, 0x64, 0x61, 0x62, 0x73, 0x70, 0x75, 0x76, 0x7f, 0x7c, 0x79, 0x7a,
	0x3b, 0x38, 0x3d, 0x3e, 0x37, 0x34, 0x31, 0x32, 0x23, 0x20, 0x25, 0x26, 0x2f, 0x2c, 0x29, 0x2a,
	0x0b, 0x08, 0x0d, 0x0e, 0x07, 0x04, 0x01, 0x02, 0x13, 0x10, 0x15, 0x16, 0x1f, 0x1c, 0x19, 0x1a
};

/*---------------------  Static Functions  --------------------------*/

/*---------------------  Export Variables  --------------------------*/

/*---------------------  Export Functions  --------------------------*/

void xor_128(BYTE *a, BYTE *b, BYTE *out)
{
	PDWORD dwPtrA = (PDWORD) a;
	PDWORD dwPtrB = (PDWORD) b;
	PDWORD dwPtrOut = (PDWORD) out;

	(*dwPtrOut++) = (*dwPtrA++) ^ (*dwPtrB++);
	(*dwPtrOut++) = (*dwPtrA++) ^ (*dwPtrB++);
	(*dwPtrOut++) = (*dwPtrA++) ^ (*dwPtrB++);
	(*dwPtrOut++) = (*dwPtrA++) ^ (*dwPtrB++);
}


void xor_32(BYTE *a, BYTE *b, BYTE *out)
{
	PDWORD dwPtrA = (PDWORD) a;
	PDWORD dwPtrB = (PDWORD) b;
	PDWORD dwPtrOut = (PDWORD) out;

	(*dwPtrOut++) = (*dwPtrA++) ^ (*dwPtrB++);
}

void AddRoundKey(BYTE *key, int round)
{
	BYTE sbox_key[4];
	BYTE rcon_table[10] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};

	sbox_key[0] = sbox_table[key[13]];
	sbox_key[1] = sbox_table[key[14]];
	sbox_key[2] = sbox_table[key[15]];
	sbox_key[3] = sbox_table[key[12]];

	key[0] = key[0] ^ rcon_table[round];
	xor_32(&key[0], sbox_key, &key[0]);

	xor_32(&key[4], &key[0], &key[4]);
	xor_32(&key[8], &key[4], &key[8]);
	xor_32(&key[12], &key[8], &key[12]);
}

void SubBytes(BYTE *in, BYTE *out)
{
	int i;

	for (i = 0; i < 16; i++)
		out[i] = sbox_table[in[i]];
}

void ShiftRows(BYTE *in, BYTE *out)
{
	out[0]  = in[0];
	out[1]  = in[5];
	out[2]  = in[10];
	out[3]  = in[15];
	out[4]  = in[4];
	out[5]  = in[9];
	out[6]  = in[14];
	out[7]  = in[3];
	out[8]  = in[8];
	out[9]  = in[13];
	out[10] = in[2];
	out[11] = in[7];
	out[12] = in[12];
	out[13] = in[1];
	out[14] = in[6];
	out[15] = in[11];
}

void MixColumns(BYTE *in, BYTE *out)
{

	out[0] = dot2_table[in[0]] ^ dot3_table[in[1]] ^ in[2] ^ in[3];
	out[1] = in[0] ^ dot2_table[in[1]] ^ dot3_table[in[2]] ^ in[3];
	out[2] = in[0] ^ in[1] ^ dot2_table[in[2]] ^ dot3_table[in[3]];
	out[3] = dot3_table[in[0]] ^ in[1] ^ in[2] ^ dot2_table[in[3]];
}

void AESv128(BYTE *key, BYTE *data, BYTE *ciphertext)
{
	int  i;
	int  round;
	BYTE TmpdataA[16];
	BYTE TmpdataB[16];
	BYTE abyRoundKey[16];

	for (i = 0; i < 16; i++)
		abyRoundKey[i] = key[i];

	for (round = 0; round < 11; round++) {
		if (round == 0) {
			xor_128(abyRoundKey, data, ciphertext);
			AddRoundKey(abyRoundKey, round);
		} else if (round == 10) {
			SubBytes(ciphertext, TmpdataA);
			ShiftRows(TmpdataA, TmpdataB);
			xor_128(TmpdataB, abyRoundKey, ciphertext);
		} else { /* round 1 ~ 9 */
			SubBytes(ciphertext, TmpdataA);
			ShiftRows(TmpdataA, TmpdataB);
			MixColumns(&TmpdataB[0], &TmpdataA[0]);
			MixColumns(&TmpdataB[4], &TmpdataA[4]);
			MixColumns(&TmpdataB[8], &TmpdataA[8]);
			MixColumns(&TmpdataB[12], &TmpdataA[12]);
			xor_128(TmpdataA, abyRoundKey, ciphertext);
			AddRoundKey(abyRoundKey, round);
		}
	}

}

/*
 * Description: AES decryption
 *
 * Parameters:
 *  In:
 *      pbyRxKey            - The key used to decrypt
 *      pbyFrame            - Starting address of packet header
 *      wFrameSize          - Total packet size including CRC
 *  Out:
 *      none
 *
 * Return Value: MIC compare result
 *
 */

BOOL AESbGenCCMP(PBYTE pbyRxKey, PBYTE pbyFrame, WORD wFrameSize)
{
	BYTE            abyNonce[13];
	BYTE            MIC_IV[16];
	BYTE            MIC_HDR1[16];
	BYTE            MIC_HDR2[16];
	BYTE            abyMIC[16];
	BYTE            abyCTRPLD[16];
	BYTE            abyTmp[16];
	BYTE            abyPlainText[16];
	BYTE            abyLastCipher[16];

	PS802_11Header  pMACHeader = (PS802_11Header) pbyFrame;
	PBYTE           pbyIV;
	PBYTE           pbyPayload;
	WORD            wHLen = 22;
	/* 8 is IV, 8 is MIC, 4 is CRC */
	WORD            wPayloadSize = wFrameSize - 8 - 8 - 4 - WLAN_HDR_ADDR3_LEN;
	BOOL            bA4 = FALSE;
	BYTE            byTmp;
	WORD            wCnt;
	int             ii, jj, kk;

	pbyIV = pbyFrame + WLAN_HDR_ADDR3_LEN;
	if (WLAN_GET_FC_TODS(*(PWORD) pbyFrame) &&
	    WLAN_GET_FC_FROMDS(*(PWORD) pbyFrame)) {
		bA4 = TRUE;
		pbyIV += 6;             /* 6 is 802.11 address4 */
		wHLen += 6;
		wPayloadSize -= 6;
	}
	pbyPayload = pbyIV + 8; /* IV-length */

	abyNonce[0]  = 0x00; /* now is 0, if Qos here will be priority */
	memcpy(&(abyNonce[1]), pMACHeader->abyAddr2, ETH_ALEN);
	abyNonce[7]  = pbyIV[7];
	abyNonce[8]  = pbyIV[6];
	abyNonce[9]  = pbyIV[5];
	abyNonce[10] = pbyIV[4];
	abyNonce[11] = pbyIV[1];
	abyNonce[12] = pbyIV[0];

	/* MIC_IV */
	MIC_IV[0] = 0x59;
	memcpy(&(MIC_IV[1]), &(abyNonce[0]), 13);
	MIC_IV[14] = (BYTE)(wPayloadSize >> 8);
	MIC_IV[15] = (BYTE)(wPayloadSize & 0xff);

	/* MIC_HDR1 */
	MIC_HDR1[0] = (BYTE)(wHLen >> 8);
	MIC_HDR1[1] = (BYTE)(wHLen & 0xff);
	byTmp = (BYTE)(pMACHeader->wFrameCtl & 0xff);
	MIC_HDR1[2] = byTmp & 0x8f;
	byTmp = (BYTE)(pMACHeader->wFrameCtl >> 8);
	byTmp &= 0x87;
	MIC_HDR1[3] = byTmp | 0x40;
	memcpy(&(MIC_HDR1[4]), pMACHeader->abyAddr1, ETH_ALEN);
	memcpy(&(MIC_HDR1[10]), pMACHeader->abyAddr2, ETH_ALEN);

	/* MIC_HDR2 */
	memcpy(&(MIC_HDR2[0]), pMACHeader->abyAddr3, ETH_ALEN);
	byTmp = (BYTE)(pMACHeader->wSeqCtl & 0xff);
	MIC_HDR2[6] = byTmp & 0x0f;
	MIC_HDR2[7] = 0;

	if (bA4) {
		memcpy(&(MIC_HDR2[8]), pMACHeader->abyAddr4, ETH_ALEN);
	} else {
		MIC_HDR2[8]  = 0x00;
		MIC_HDR2[9]  = 0x00;
		MIC_HDR2[10] = 0x00;
		MIC_HDR2[11] = 0x00;
		MIC_HDR2[12] = 0x00;
		MIC_HDR2[13] = 0x00;
	}
	MIC_HDR2[14] = 0x00;
	MIC_HDR2[15] = 0x00;

	/* CCMP */
	AESv128(pbyRxKey, MIC_IV, abyMIC);
	for (kk = 0; kk < 16; kk++)
		abyTmp[kk] = MIC_HDR1[kk] ^ abyMIC[kk];

	AESv128(pbyRxKey, abyTmp, abyMIC);
	for (kk = 0; kk < 16; kk++)
		abyTmp[kk] = MIC_HDR2[kk] ^ abyMIC[kk];

	AESv128(pbyRxKey, abyTmp, abyMIC);

	wCnt = 1;
	abyCTRPLD[0] = 0x01;
	memcpy(&(abyCTRPLD[1]), &(abyNonce[0]), 13);

	for (jj = wPayloadSize; jj > 16; jj = jj-16) {

		abyCTRPLD[14] = (BYTE) (wCnt >> 8);
		abyCTRPLD[15] = (BYTE) (wCnt & 0xff);

		AESv128(pbyRxKey, abyCTRPLD, abyTmp);

		for (kk = 0; kk < 16; kk++)
			abyPlainText[kk] = abyTmp[kk] ^ pbyPayload[kk];

		for (kk = 0; kk < 16; kk++)
			abyTmp[kk] = abyMIC[kk] ^ abyPlainText[kk];

		AESv128(pbyRxKey, abyTmp, abyMIC);

		memcpy(pbyPayload, abyPlainText, 16);
		wCnt++;
		pbyPayload += 16;
	} /* for wPayloadSize */

	/* last payload */
	memcpy(&(abyLastCipher[0]), pbyPayload, jj);
	for (ii = jj; ii < 16; ii++)
		abyLastCipher[ii] = 0x00;

	abyCTRPLD[14] = (BYTE) (wCnt >> 8);
	abyCTRPLD[15] = (BYTE) (wCnt & 0xff);

	AESv128(pbyRxKey, abyCTRPLD, abyTmp);
	for (kk = 0; kk < 16; kk++)
		abyPlainText[kk] = abyTmp[kk] ^ abyLastCipher[kk];

	memcpy(pbyPayload, abyPlainText, jj);
	pbyPayload += jj;

	/* for MIC calculation */
	for (ii = jj; ii < 16; ii++)
		abyPlainText[ii] = 0x00;
	for (kk = 0; kk < 16; kk++)
		abyTmp[kk] = abyMIC[kk] ^ abyPlainText[kk];

	AESv128(pbyRxKey, abyTmp, abyMIC);

	/* => above is the calculated MIC */

	wCnt = 0;
	abyCTRPLD[14] = (BYTE) (wCnt >> 8);
	abyCTRPLD[15] = (BYTE) (wCnt & 0xff);
	AESv128(pbyRxKey, abyCTRPLD, abyTmp);

	for (kk = 0; kk < 8; kk++)
		abyTmp[kk] = abyTmp[kk] ^ pbyPayload[kk];

	/* => above is the packet dec-MIC */

	if (!memcmp(abyMIC, abyTmp, 8))
		return TRUE;
	else
		return FALSE;
}
