#ifndef __BCM47XX_BOARD_H
#define __BCM47XX_BOARD_H

enum bcm47xx_board {
	BCM47XX_BOARD_ASUS_RTAC66U,
	BCM47XX_BOARD_ASUS_RTN10,
	BCM47XX_BOARD_ASUS_RTN10D,
	BCM47XX_BOARD_ASUS_RTN10U,
	BCM47XX_BOARD_ASUS_RTN12,
	BCM47XX_BOARD_ASUS_RTN12B1,
	BCM47XX_BOARD_ASUS_RTN12C1,
	BCM47XX_BOARD_ASUS_RTN12D1,
	BCM47XX_BOARD_ASUS_RTN12HP,
	BCM47XX_BOARD_ASUS_RTN15U,
	BCM47XX_BOARD_ASUS_RTN16,
	BCM47XX_BOARD_ASUS_RTN53,
	BCM47XX_BOARD_ASUS_RTN66U,
	BCM47XX_BOARD_ASUS_WL300G,
	BCM47XX_BOARD_ASUS_WL320GE,
	BCM47XX_BOARD_ASUS_WL330GE,
	BCM47XX_BOARD_ASUS_WL500G,
	BCM47XX_BOARD_ASUS_WL500GD,
	BCM47XX_BOARD_ASUS_WL500GPV1,
	BCM47XX_BOARD_ASUS_WL500GPV2,
	BCM47XX_BOARD_ASUS_WL500W,
	BCM47XX_BOARD_ASUS_WL520GC,
	BCM47XX_BOARD_ASUS_WL520GU,
	BCM47XX_BOARD_ASUS_WL700GE,
	BCM47XX_BOARD_ASUS_WLHDD,

	BCM47XX_BOARD_BELKIN_F7D3301,
	BCM47XX_BOARD_BELKIN_F7D3302,
	BCM47XX_BOARD_BELKIN_F7D4301,
	BCM47XX_BOARD_BELKIN_F7D4302,
	BCM47XX_BOARD_BELKIN_F7D4401,

	BCM47XX_BOARD_BUFFALO_WBR2_G54,
	BCM47XX_BOARD_BUFFALO_WHR2_A54G54,
	BCM47XX_BOARD_BUFFALO_WHR_G125,
	BCM47XX_BOARD_BUFFALO_WHR_G54S,
	BCM47XX_BOARD_BUFFALO_WHR_HP_G54,
	BCM47XX_BOARD_BUFFALO_WLA2_G54L,
	BCM47XX_BOARD_BUFFALO_WZR_G300N,
	BCM47XX_BOARD_BUFFALO_WZR_RS_G54,
	BCM47XX_BOARD_BUFFALO_WZR_RS_G54HP,

	BCM47XX_BOARD_CISCO_M10V1,
	BCM47XX_BOARD_CISCO_M20V1,

	BCM47XX_BOARD_DELL_TM2300,

	BCM47XX_BOARD_DLINK_DIR130,
	BCM47XX_BOARD_DLINK_DIR330,

	BCM47XX_BOARD_HUAWEI_E970,

	BCM47XX_BOARD_LINKSYS_E900V1,
	BCM47XX_BOARD_LINKSYS_E1000V1,
	BCM47XX_BOARD_LINKSYS_E1000V2,
	BCM47XX_BOARD_LINKSYS_E1000V21,
	BCM47XX_BOARD_LINKSYS_E1200V2,
	BCM47XX_BOARD_LINKSYS_E2000V1,
	BCM47XX_BOARD_LINKSYS_E3000V1,
	BCM47XX_BOARD_LINKSYS_E3200V1,
	BCM47XX_BOARD_LINKSYS_E4200V1,
	BCM47XX_BOARD_LINKSYS_WRT150NV1,
	BCM47XX_BOARD_LINKSYS_WRT150NV11,
	BCM47XX_BOARD_LINKSYS_WRT160NV1,
	BCM47XX_BOARD_LINKSYS_WRT160NV3,
	BCM47XX_BOARD_LINKSYS_WRT300NV11,
	BCM47XX_BOARD_LINKSYS_WRT310NV1,
	BCM47XX_BOARD_LINKSYS_WRT310NV2,
	BCM47XX_BOARD_LINKSYS_WRT54G3GV2,
	BCM47XX_BOARD_LINKSYS_WRT54G,
	BCM47XX_BOARD_LINKSYS_WRT610NV1,
	BCM47XX_BOARD_LINKSYS_WRT610NV2,
	BCM47XX_BOARD_LINKSYS_WRTSL54GS,

	BCM47XX_BOARD_MICROSOFT_MN700,

	BCM47XX_BOARD_MOTOROLA_WE800G,
	BCM47XX_BOARD_MOTOROLA_WR850GP,
	BCM47XX_BOARD_MOTOROLA_WR850GV2V3,

	BCM47XX_BOARD_NETGEAR_WGR614V8,
	BCM47XX_BOARD_NETGEAR_WGR614V9,
	BCM47XX_BOARD_NETGEAR_WNDR3300,
	BCM47XX_BOARD_NETGEAR_WNDR3400V1,
	BCM47XX_BOARD_NETGEAR_WNDR3400V2,
	BCM47XX_BOARD_NETGEAR_WNDR3400VCNA,
	BCM47XX_BOARD_NETGEAR_WNDR3700V3,
	BCM47XX_BOARD_NETGEAR_WNDR4000,
	BCM47XX_BOARD_NETGEAR_WNDR4500V1,
	BCM47XX_BOARD_NETGEAR_WNDR4500V2,
	BCM47XX_BOARD_NETGEAR_WNR2000,
	BCM47XX_BOARD_NETGEAR_WNR3500L,
	BCM47XX_BOARD_NETGEAR_WNR3500U,
	BCM47XX_BOARD_NETGEAR_WNR3500V2,
	BCM47XX_BOARD_NETGEAR_WNR3500V2VC,
	BCM47XX_BOARD_NETGEAR_WNR834BV2,

	BCM47XX_BOARD_PHICOMM_M1,

	BCM47XX_BOARD_SIEMENS_SE505V2,

	BCM47XX_BOARD_SIMPLETECH_SIMPLESHARE,

	BCM47XX_BOARD_ZTE_H218N,

	BCM47XX_BOARD_UNKNOWN,
	BCM47XX_BOARD_NO,
};

#define BCM47XX_BOARD_MAX_NAME 30

void bcm47xx_board_detect(void);
enum bcm47xx_board bcm47xx_board_get(void);
const char *bcm47xx_board_get_name(void);

#endif /* __BCM47XX_BOARD_H */
