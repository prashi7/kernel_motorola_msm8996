/*
 * Copyright (c) 2008-2009 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef REGD_COMMON_H
#define REGD_COMMON_H

enum EnumRd {
	NO_ENUMRD = 0x00,
	NULL1_WORLD = 0x03,
	NULL1_ETSIB = 0x07,
	NULL1_ETSIC = 0x08,
	FCC1_FCCA = 0x10,
	FCC1_WORLD = 0x11,
	FCC4_FCCA = 0x12,
	FCC5_FCCA = 0x13,
	FCC6_FCCA = 0x14,

	FCC2_FCCA = 0x20,
	FCC2_WORLD = 0x21,
	FCC2_ETSIC = 0x22,
	FCC6_WORLD = 0x23,
	FRANCE_RES = 0x31,
	FCC3_FCCA = 0x3A,
	FCC3_WORLD = 0x3B,

	ETSI1_WORLD = 0x37,
	ETSI3_ETSIA = 0x32,
	ETSI2_WORLD = 0x35,
	ETSI3_WORLD = 0x36,
	ETSI4_WORLD = 0x30,
	ETSI4_ETSIC = 0x38,
	ETSI5_WORLD = 0x39,
	ETSI6_WORLD = 0x34,
	ETSI_RESERVED = 0x33,

	MKK1_MKKA = 0x40,
	MKK1_MKKB = 0x41,
	APL4_WORLD = 0x42,
	MKK2_MKKA = 0x43,
	APL_RESERVED = 0x44,
	APL2_WORLD = 0x45,
	APL2_APLC = 0x46,
	APL3_WORLD = 0x47,
	MKK1_FCCA = 0x48,
	APL2_APLD = 0x49,
	MKK1_MKKA1 = 0x4A,
	MKK1_MKKA2 = 0x4B,
	MKK1_MKKC = 0x4C,

	APL3_FCCA = 0x50,
	APL1_WORLD = 0x52,
	APL1_FCCA = 0x53,
	APL1_APLA = 0x54,
	APL1_ETSIC = 0x55,
	APL2_ETSIC = 0x56,
	APL5_WORLD = 0x58,
	APL6_WORLD = 0x5B,
	APL7_FCCA = 0x5C,
	APL8_WORLD = 0x5D,
	APL9_WORLD = 0x5E,

	WOR0_WORLD = 0x60,
	WOR1_WORLD = 0x61,
	WOR2_WORLD = 0x62,
	WOR3_WORLD = 0x63,
	WOR4_WORLD = 0x64,
	WOR5_ETSIC = 0x65,

	WOR01_WORLD = 0x66,
	WOR02_WORLD = 0x67,
	EU1_WORLD = 0x68,

	WOR9_WORLD = 0x69,
	WORA_WORLD = 0x6A,
	WORB_WORLD = 0x6B,

	MKK3_MKKB = 0x80,
	MKK3_MKKA2 = 0x81,
	MKK3_MKKC = 0x82,

	MKK4_MKKB = 0x83,
	MKK4_MKKA2 = 0x84,
	MKK4_MKKC = 0x85,

	MKK5_MKKB = 0x86,
	MKK5_MKKA2 = 0x87,
	MKK5_MKKC = 0x88,

	MKK6_MKKB = 0x89,
	MKK6_MKKA2 = 0x8A,
	MKK6_MKKC = 0x8B,

	MKK7_MKKB = 0x8C,
	MKK7_MKKA2 = 0x8D,
	MKK7_MKKC = 0x8E,

	MKK8_MKKB = 0x8F,
	MKK8_MKKA2 = 0x90,
	MKK8_MKKC = 0x91,

	MKK14_MKKA1 = 0x92,
	MKK15_MKKA1 = 0x93,

	MKK10_FCCA = 0xD0,
	MKK10_MKKA1 = 0xD1,
	MKK10_MKKC = 0xD2,
	MKK10_MKKA2 = 0xD3,

	MKK11_MKKA = 0xD4,
	MKK11_FCCA = 0xD5,
	MKK11_MKKA1 = 0xD6,
	MKK11_MKKC = 0xD7,
	MKK11_MKKA2 = 0xD8,

	MKK12_MKKA = 0xD9,
	MKK12_FCCA = 0xDA,
	MKK12_MKKA1 = 0xDB,
	MKK12_MKKC = 0xDC,
	MKK12_MKKA2 = 0xDD,

	MKK13_MKKB = 0xDE,

	MKK3_MKKA = 0xF0,
	MKK3_MKKA1 = 0xF1,
	MKK3_FCCA = 0xF2,
	MKK4_MKKA = 0xF3,
	MKK4_MKKA1 = 0xF4,
	MKK4_FCCA = 0xF5,
	MKK9_MKKA = 0xF6,
	MKK10_MKKA = 0xF7,
	MKK6_MKKA1 = 0xF8,
	MKK6_FCCA = 0xF9,
	MKK7_MKKA1 = 0xFA,
	MKK7_FCCA = 0xFB,
	MKK9_FCCA = 0xFC,
	MKK9_MKKA1 = 0xFD,
	MKK9_MKKC = 0xFE,
	MKK9_MKKA2 = 0xFF,

	WORLD = 0x0199,
	DEBUG_REG_DMN = 0x01ff,
};

/* Regpair to CTL band mapping */
static struct reg_dmn_pair_mapping regDomainPairs[] = {
	/* regpair, 5 GHz CTL, 2 GHz CTL */
	{NO_ENUMRD, DEBUG_REG_DMN, DEBUG_REG_DMN},
	{NULL1_WORLD, NO_CTL, CTL_ETSI},
	{NULL1_ETSIB, NO_CTL, CTL_ETSI},
	{NULL1_ETSIC, NO_CTL, CTL_ETSI},

	{FCC2_FCCA, CTL_FCC, CTL_FCC},
	{FCC2_WORLD, CTL_FCC, CTL_ETSI},
	{FCC2_ETSIC, CTL_FCC, CTL_ETSI},
	{FCC3_FCCA, CTL_FCC, CTL_FCC},
	{FCC3_WORLD, CTL_FCC, CTL_ETSI},
	{FCC4_FCCA, CTL_FCC, CTL_FCC},
	{FCC5_FCCA, CTL_FCC, CTL_FCC},
	{FCC6_FCCA, CTL_FCC, CTL_FCC},
	{FCC6_WORLD, CTL_FCC, CTL_ETSI},

	{ETSI1_WORLD, CTL_ETSI, CTL_ETSI},
	{ETSI2_WORLD, CTL_ETSI, CTL_ETSI},
	{ETSI3_WORLD, CTL_ETSI, CTL_ETSI},
	{ETSI4_WORLD, CTL_ETSI, CTL_ETSI},
	{ETSI5_WORLD, CTL_ETSI, CTL_ETSI},
	{ETSI6_WORLD, CTL_ETSI, CTL_ETSI},

	/* XXX: For ETSI3_ETSIA, Was NO_CTL meant for the 2 GHz band ? */
	{ETSI3_ETSIA, CTL_ETSI, CTL_ETSI},
	{FRANCE_RES, CTL_ETSI, CTL_ETSI},

	{FCC1_WORLD, CTL_FCC, CTL_ETSI},
	{FCC1_FCCA, CTL_FCC, CTL_FCC},
	{APL1_WORLD, CTL_FCC, CTL_ETSI},
	{APL2_WORLD, CTL_FCC, CTL_ETSI},
	{APL3_WORLD, CTL_FCC, CTL_ETSI},
	{APL4_WORLD, CTL_FCC, CTL_ETSI},
	{APL5_WORLD, CTL_FCC, CTL_ETSI},
	{APL6_WORLD, CTL_ETSI, CTL_ETSI},
	{APL8_WORLD, CTL_ETSI, CTL_ETSI},
	{APL9_WORLD, CTL_ETSI, CTL_ETSI},

	{APL3_FCCA, CTL_FCC, CTL_FCC},
	{APL1_ETSIC, CTL_FCC, CTL_ETSI},
	{APL2_ETSIC, CTL_FCC, CTL_ETSI},
	{APL2_APLD, CTL_FCC, NO_CTL},

	{MKK1_MKKA, CTL_MKK, CTL_MKK},
	{MKK1_MKKB, CTL_MKK, CTL_MKK},
	{MKK1_FCCA, CTL_MKK, CTL_FCC},
	{MKK1_MKKA1, CTL_MKK, CTL_MKK},
	{MKK1_MKKA2, CTL_MKK, CTL_MKK},
	{MKK1_MKKC, CTL_MKK, CTL_MKK},

	{MKK2_MKKA, CTL_MKK, CTL_MKK},
	{MKK3_MKKA, CTL_MKK, CTL_MKK},
	{MKK3_MKKB, CTL_MKK, CTL_MKK},
	{MKK3_MKKA1, CTL_MKK, CTL_MKK},
	{MKK3_MKKA2, CTL_MKK, CTL_MKK},
	{MKK3_MKKC, CTL_MKK, CTL_MKK},
	{MKK3_FCCA, CTL_MKK, CTL_FCC},

	{MKK4_MKKA, CTL_MKK, CTL_MKK},
	{MKK4_MKKB, CTL_MKK, CTL_MKK},
	{MKK4_MKKA1, CTL_MKK, CTL_MKK},
	{MKK4_MKKA2, CTL_MKK, CTL_MKK},
	{MKK4_MKKC, CTL_MKK, CTL_MKK},
	{MKK4_FCCA, CTL_MKK, CTL_FCC},

	{MKK5_MKKB, CTL_MKK, CTL_MKK},
	{MKK5_MKKA2, CTL_MKK, CTL_MKK},
	{MKK5_MKKC, CTL_MKK, CTL_MKK},

	{MKK6_MKKB, CTL_MKK, CTL_MKK},
	{MKK6_MKKA1, CTL_MKK, CTL_MKK},
	{MKK6_MKKA2, CTL_MKK, CTL_MKK},
	{MKK6_MKKC, CTL_MKK, CTL_MKK},
	{MKK6_FCCA, CTL_MKK, CTL_FCC},

	{MKK7_MKKB, CTL_MKK, CTL_MKK},
	{MKK7_MKKA1, CTL_MKK, CTL_MKK},
	{MKK7_MKKA2, CTL_MKK, CTL_MKK},
	{MKK7_MKKC, CTL_MKK, CTL_MKK},
	{MKK7_FCCA, CTL_MKK, CTL_FCC},

	{MKK8_MKKB, CTL_MKK, CTL_MKK},
	{MKK8_MKKA2, CTL_MKK, CTL_MKK},
	{MKK8_MKKC, CTL_MKK, CTL_MKK},

	{MKK9_MKKA, CTL_MKK, CTL_MKK},
	{MKK9_FCCA, CTL_MKK, CTL_FCC},
	{MKK9_MKKA1, CTL_MKK, CTL_MKK},
	{MKK9_MKKA2, CTL_MKK, CTL_MKK},
	{MKK9_MKKC, CTL_MKK, CTL_MKK},

	{MKK10_MKKA, CTL_MKK, CTL_MKK},
	{MKK10_FCCA, CTL_MKK, CTL_FCC},
	{MKK10_MKKA1, CTL_MKK, CTL_MKK},
	{MKK10_MKKA2, CTL_MKK, CTL_MKK},
	{MKK10_MKKC, CTL_MKK, CTL_MKK},

	{MKK11_MKKA, CTL_MKK, CTL_MKK},
	{MKK11_FCCA, CTL_MKK, CTL_FCC},
	{MKK11_MKKA1, CTL_MKK, CTL_MKK},
	{MKK11_MKKA2, CTL_MKK, CTL_MKK},
	{MKK11_MKKC, CTL_MKK, CTL_MKK},

	{MKK12_MKKA, CTL_MKK, CTL_MKK},
	{MKK12_FCCA, CTL_MKK, CTL_FCC},
	{MKK12_MKKA1, CTL_MKK, CTL_MKK},
	{MKK12_MKKA2, CTL_MKK, CTL_MKK},
	{MKK12_MKKC, CTL_MKK, CTL_MKK},

	{MKK13_MKKB, CTL_MKK, CTL_MKK},
	{MKK14_MKKA1, CTL_MKK, CTL_MKK},
	{MKK15_MKKA1, CTL_MKK, CTL_MKK},

	{WOR0_WORLD, NO_CTL, NO_CTL},
	{WOR1_WORLD, NO_CTL, NO_CTL},
	{WOR2_WORLD, NO_CTL, NO_CTL},
	{WOR3_WORLD, NO_CTL, NO_CTL},
	{WOR4_WORLD, NO_CTL, NO_CTL},
	{WOR5_ETSIC, NO_CTL, NO_CTL},
	{WOR01_WORLD, NO_CTL, NO_CTL},
	{WOR02_WORLD, NO_CTL, NO_CTL},
	{EU1_WORLD, NO_CTL, NO_CTL},
	{WOR9_WORLD, NO_CTL, NO_CTL},
	{WORA_WORLD, NO_CTL, NO_CTL},
	{WORB_WORLD, NO_CTL, NO_CTL},
};

static struct country_code_to_enum_rd allCountries[] = {
	{CTRY_DEBUG, NO_ENUMRD, "DB"},
	{CTRY_DEFAULT, FCC1_FCCA, "CO"},
	{CTRY_ALBANIA, NULL1_WORLD, "AL"},
	{CTRY_ALGERIA, NULL1_WORLD, "DZ"},
	{CTRY_ARGENTINA, APL3_WORLD, "AR"},
	{CTRY_ARMENIA, ETSI4_WORLD, "AM"},
	{CTRY_AUSTRALIA, FCC2_WORLD, "AU"},
	{CTRY_AUSTRALIA2, FCC6_WORLD, "AU"},
	{CTRY_AUSTRIA, ETSI1_WORLD, "AT"},
	{CTRY_AZERBAIJAN, ETSI4_WORLD, "AZ"},
	{CTRY_BAHRAIN, APL6_WORLD, "BH"},
	{CTRY_BELARUS, ETSI1_WORLD, "BY"},
	{CTRY_BELGIUM, ETSI1_WORLD, "BE"},
	{CTRY_BELGIUM2, ETSI4_WORLD, "BL"},
	{CTRY_BELIZE, APL1_ETSIC, "BZ"},
	{CTRY_BOLIVIA, APL1_ETSIC, "BO"},
	{CTRY_BOSNIA_HERZ, ETSI1_WORLD, "BA"},
	{CTRY_BRAZIL, FCC3_WORLD, "BR"},
	{CTRY_BRUNEI_DARUSSALAM, APL1_WORLD, "BN"},
	{CTRY_BULGARIA, ETSI6_WORLD, "BG"},
	{CTRY_CANADA, FCC2_FCCA, "CA"},
	{CTRY_CANADA2, FCC6_FCCA, "CA"},
	{CTRY_CHILE, APL6_WORLD, "CL"},
	{CTRY_CHINA, APL1_WORLD, "CN"},
	{CTRY_COLOMBIA, FCC1_FCCA, "CO"},
	{CTRY_COSTA_RICA, FCC1_WORLD, "CR"},
	{CTRY_CROATIA, ETSI3_WORLD, "HR"},
	{CTRY_CYPRUS, ETSI1_WORLD, "CY"},
	{CTRY_CZECH, ETSI3_WORLD, "CZ"},
	{CTRY_DENMARK, ETSI1_WORLD, "DK"},
	{CTRY_DOMINICAN_REPUBLIC, FCC1_FCCA, "DO"},
	{CTRY_ECUADOR, FCC1_WORLD, "EC"},
	{CTRY_EGYPT, ETSI3_WORLD, "EG"},
	{CTRY_EL_SALVADOR, FCC1_WORLD, "SV"},
	{CTRY_ESTONIA, ETSI1_WORLD, "EE"},
	{CTRY_FINLAND, ETSI1_WORLD, "FI"},
	{CTRY_FRANCE, ETSI1_WORLD, "FR"},
	{CTRY_GEORGIA, ETSI4_WORLD, "GE"},
	{CTRY_GERMANY, ETSI1_WORLD, "DE"},
	{CTRY_GREECE, ETSI1_WORLD, "GR"},
	{CTRY_GUATEMALA, FCC1_FCCA, "GT"},
	{CTRY_HONDURAS, NULL1_WORLD, "HN"},
	{CTRY_HONG_KONG, FCC2_WORLD, "HK"},
	{CTRY_HUNGARY, ETSI1_WORLD, "HU"},
	{CTRY_ICELAND, ETSI1_WORLD, "IS"},
	{CTRY_INDIA, APL6_WORLD, "IN"},
	{CTRY_INDONESIA, APL1_WORLD, "ID"},
	{CTRY_IRAN, APL1_WORLD, "IR"},
	{CTRY_IRELAND, ETSI1_WORLD, "IE"},
	{CTRY_ISRAEL, NULL1_WORLD, "IL"},
	{CTRY_ITALY, ETSI1_WORLD, "IT"},
	{CTRY_JAMAICA, ETSI1_WORLD, "JM"},

	{CTRY_JAPAN, MKK1_MKKA, "JP"},
	{CTRY_JAPAN1, MKK1_MKKB, "JP"},
	{CTRY_JAPAN2, MKK1_FCCA, "JP"},
	{CTRY_JAPAN3, MKK2_MKKA, "JP"},
	{CTRY_JAPAN4, MKK1_MKKA1, "JP"},
	{CTRY_JAPAN5, MKK1_MKKA2, "JP"},
	{CTRY_JAPAN6, MKK1_MKKC, "JP"},
	{CTRY_JAPAN7, MKK3_MKKB, "JP"},
	{CTRY_JAPAN8, MKK3_MKKA2, "JP"},
	{CTRY_JAPAN9, MKK3_MKKC, "JP"},
	{CTRY_JAPAN10, MKK4_MKKB, "JP"},
	{CTRY_JAPAN11, MKK4_MKKA2, "JP"},
	{CTRY_JAPAN12, MKK4_MKKC, "JP"},
	{CTRY_JAPAN13, MKK5_MKKB, "JP"},
	{CTRY_JAPAN14, MKK5_MKKA2, "JP"},
	{CTRY_JAPAN15, MKK5_MKKC, "JP"},
	{CTRY_JAPAN16, MKK6_MKKB, "JP"},
	{CTRY_JAPAN17, MKK6_MKKA2, "JP"},
	{CTRY_JAPAN18, MKK6_MKKC, "JP"},
	{CTRY_JAPAN19, MKK7_MKKB, "JP"},
	{CTRY_JAPAN20, MKK7_MKKA2, "JP"},
	{CTRY_JAPAN21, MKK7_MKKC, "JP"},
	{CTRY_JAPAN22, MKK8_MKKB, "JP"},
	{CTRY_JAPAN23, MKK8_MKKA2, "JP"},
	{CTRY_JAPAN24, MKK8_MKKC, "JP"},
	{CTRY_JAPAN25, MKK3_MKKA, "JP"},
	{CTRY_JAPAN26, MKK3_MKKA1, "JP"},
	{CTRY_JAPAN27, MKK3_FCCA, "JP"},
	{CTRY_JAPAN28, MKK4_MKKA1, "JP"},
	{CTRY_JAPAN29, MKK4_FCCA, "JP"},
	{CTRY_JAPAN30, MKK6_MKKA1, "JP"},
	{CTRY_JAPAN31, MKK6_FCCA, "JP"},
	{CTRY_JAPAN32, MKK7_MKKA1, "JP"},
	{CTRY_JAPAN33, MKK7_FCCA, "JP"},
	{CTRY_JAPAN34, MKK9_MKKA, "JP"},
	{CTRY_JAPAN35, MKK10_MKKA, "JP"},
	{CTRY_JAPAN36, MKK4_MKKA, "JP"},
	{CTRY_JAPAN37, MKK9_FCCA, "JP"},
	{CTRY_JAPAN38, MKK9_MKKA1, "JP"},
	{CTRY_JAPAN39, MKK9_MKKC, "JP"},
	{CTRY_JAPAN40, MKK9_MKKA2, "JP"},
	{CTRY_JAPAN41, MKK10_FCCA, "JP"},
	{CTRY_JAPAN42, MKK10_MKKA1, "JP"},
	{CTRY_JAPAN43, MKK10_MKKC, "JP"},
	{CTRY_JAPAN44, MKK10_MKKA2, "JP"},
	{CTRY_JAPAN45, MKK11_MKKA, "JP"},
	{CTRY_JAPAN46, MKK11_FCCA, "JP"},
	{CTRY_JAPAN47, MKK11_MKKA1, "JP"},
	{CTRY_JAPAN48, MKK11_MKKC, "JP"},
	{CTRY_JAPAN49, MKK11_MKKA2, "JP"},
	{CTRY_JAPAN50, MKK12_MKKA, "JP"},
	{CTRY_JAPAN51, MKK12_FCCA, "JP"},
	{CTRY_JAPAN52, MKK12_MKKA1, "JP"},
	{CTRY_JAPAN53, MKK12_MKKC, "JP"},
	{CTRY_JAPAN54, MKK12_MKKA2, "JP"},
	{CTRY_JAPAN57, MKK13_MKKB, "JP"},
	{CTRY_JAPAN58, MKK14_MKKA1, "JP"},
	{CTRY_JAPAN59, MKK15_MKKA1, "JP"},

	{CTRY_JORDAN, ETSI2_WORLD, "JO"},
	{CTRY_KAZAKHSTAN, NULL1_WORLD, "KZ"},
	{CTRY_KOREA_NORTH, APL9_WORLD, "KP"},
	{CTRY_KOREA_ROC, APL9_WORLD, "KR"},
	{CTRY_KOREA_ROC2, APL2_WORLD, "K2"},
	{CTRY_KOREA_ROC3, APL9_WORLD, "K3"},
	{CTRY_KUWAIT, NULL1_WORLD, "KW"},
	{CTRY_LATVIA, ETSI1_WORLD, "LV"},
	{CTRY_LEBANON, NULL1_WORLD, "LB"},
	{CTRY_LIECHTENSTEIN, ETSI1_WORLD, "LI"},
	{CTRY_LITHUANIA, ETSI1_WORLD, "LT"},
	{CTRY_LUXEMBOURG, ETSI1_WORLD, "LU"},
	{CTRY_MACAU, FCC2_WORLD, "MO"},
	{CTRY_MACEDONIA, NULL1_WORLD, "MK"},
	{CTRY_MALAYSIA, APL8_WORLD, "MY"},
	{CTRY_MALTA, ETSI1_WORLD, "MT"},
	{CTRY_MEXICO, FCC1_FCCA, "MX"},
	{CTRY_MONACO, ETSI4_WORLD, "MC"},
	{CTRY_MOROCCO, NULL1_WORLD, "MA"},
	{CTRY_NEPAL, APL1_WORLD, "NP"},
	{CTRY_NETHERLANDS, ETSI1_WORLD, "NL"},
	{CTRY_NETHERLANDS_ANTILLES, ETSI1_WORLD, "AN"},
	{CTRY_NEW_ZEALAND, FCC2_ETSIC, "NZ"},
	{CTRY_NORWAY, ETSI1_WORLD, "NO"},
	{CTRY_OMAN, APL6_WORLD, "OM"},
	{CTRY_PAKISTAN, NULL1_WORLD, "PK"},
	{CTRY_PANAMA, FCC1_FCCA, "PA"},
	{CTRY_PAPUA_NEW_GUINEA, FCC1_WORLD, "PG"},
	{CTRY_PERU, APL1_WORLD, "PE"},
	{CTRY_PHILIPPINES, APL1_WORLD, "PH"},
	{CTRY_POLAND, ETSI1_WORLD, "PL"},
	{CTRY_PORTUGAL, ETSI1_WORLD, "PT"},
	{CTRY_PUERTO_RICO, FCC1_FCCA, "PR"},
	{CTRY_QATAR, NULL1_WORLD, "QA"},
	{CTRY_ROMANIA, NULL1_WORLD, "RO"},
	{CTRY_RUSSIA, NULL1_WORLD, "RU"},
	{CTRY_SAUDI_ARABIA, NULL1_WORLD, "SA"},
	{CTRY_SERBIA_MONTENEGRO, ETSI1_WORLD, "CS"},
	{CTRY_SINGAPORE, APL6_WORLD, "SG"},
	{CTRY_SLOVAKIA, ETSI1_WORLD, "SK"},
	{CTRY_SLOVENIA, ETSI1_WORLD, "SI"},
	{CTRY_SOUTH_AFRICA, FCC3_WORLD, "ZA"},
	{CTRY_SPAIN, ETSI1_WORLD, "ES"},
	{CTRY_SRI_LANKA, FCC3_WORLD, "LK"},
	{CTRY_SWEDEN, ETSI1_WORLD, "SE"},
	{CTRY_SWITZERLAND, ETSI1_WORLD, "CH"},
	{CTRY_SYRIA, NULL1_WORLD, "SY"},
	{CTRY_TAIWAN, APL3_FCCA, "TW"},
	{CTRY_THAILAND, FCC3_WORLD, "TH"},
	{CTRY_TRINIDAD_Y_TOBAGO, ETSI4_WORLD, "TT"},
	{CTRY_TUNISIA, ETSI3_WORLD, "TN"},
	{CTRY_TURKEY, ETSI3_WORLD, "TR"},
	{CTRY_UKRAINE, NULL1_WORLD, "UA"},
	{CTRY_UAE, NULL1_WORLD, "AE"},
	{CTRY_UNITED_KINGDOM, ETSI1_WORLD, "GB"},
	{CTRY_UNITED_STATES, FCC3_FCCA, "US"},
	/* This "PS" is for US public safety actually... to support this we
	 * would need to assign new special alpha2 to CRDA db as with the world
	 * regdomain and use another alpha2 */
	{CTRY_UNITED_STATES_FCC49, FCC4_FCCA, "PS"},
	{CTRY_URUGUAY, APL2_WORLD, "UY"},
	{CTRY_UZBEKISTAN, FCC3_FCCA, "UZ"},
	{CTRY_VENEZUELA, APL2_ETSIC, "VE"},
	{CTRY_VIET_NAM, NULL1_WORLD, "VN"},
	{CTRY_YEMEN, NULL1_WORLD, "YE"},
	{CTRY_ZIMBABWE, NULL1_WORLD, "ZW"},
};

#endif
