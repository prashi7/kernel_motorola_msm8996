/*
 * s2mps11.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef __LINUX_MFD_S2MPS11_H
#define __LINUX_MFD_S2MPS11_H

/* S2MPS11 registers */
enum s2mps11_reg {
	S2MPS11_REG_ID,
	S2MPS11_REG_INT1,
	S2MPS11_REG_INT2,
	S2MPS11_REG_INT3,
	S2MPS11_REG_INT1M,
	S2MPS11_REG_INT2M,
	S2MPS11_REG_INT3M,
	S2MPS11_REG_ST1,
	S2MPS11_REG_ST2,
	S2MPS11_REG_OFFSRC,
	S2MPS11_REG_PWRONSRC,
	S2MPS11_REG_RTC_CTRL,
	S2MPS11_REG_CTRL1,
	S2MPS11_REG_ETC_TEST,
	S2MPS11_REG_RSVD3,
	S2MPS11_REG_BU_CHG,
	S2MPS11_REG_RAMP,
	S2MPS11_REG_RAMP_BUCK,
	S2MPS11_REG_LDO1_8,
	S2MPS11_REG_LDO9_16,
	S2MPS11_REG_LDO17_24,
	S2MPS11_REG_LDO25_32,
	S2MPS11_REG_LDO33_38,
	S2MPS11_REG_LDO1_8_1,
	S2MPS11_REG_LDO9_16_1,
	S2MPS11_REG_LDO17_24_1,
	S2MPS11_REG_LDO25_32_1,
	S2MPS11_REG_LDO33_38_1,
	S2MPS11_REG_OTP_ADRL,
	S2MPS11_REG_OTP_ADRH,
	S2MPS11_REG_OTP_DATA,
	S2MPS11_REG_MON1SEL,
	S2MPS11_REG_MON2SEL,
	S2MPS11_REG_LEE,
	S2MPS11_REG_RSVD_NO,
	S2MPS11_REG_UVLO,
	S2MPS11_REG_LEE_NO,
	S2MPS11_REG_B1CTRL1,
	S2MPS11_REG_B1CTRL2,
	S2MPS11_REG_B2CTRL1,
	S2MPS11_REG_B2CTRL2,
	S2MPS11_REG_B3CTRL1,
	S2MPS11_REG_B3CTRL2,
	S2MPS11_REG_B4CTRL1,
	S2MPS11_REG_B4CTRL2,
	S2MPS11_REG_B5CTRL1,
	S2MPS11_REG_BUCK5_SW,
	S2MPS11_REG_B5CTRL2,
	S2MPS11_REG_B5CTRL3,
	S2MPS11_REG_B5CTRL4,
	S2MPS11_REG_B5CTRL5,
	S2MPS11_REG_B6CTRL1,
	S2MPS11_REG_B6CTRL2,
	S2MPS11_REG_B7CTRL1,
	S2MPS11_REG_B7CTRL2,
	S2MPS11_REG_B8CTRL1,
	S2MPS11_REG_B8CTRL2,
	S2MPS11_REG_B9CTRL1,
	S2MPS11_REG_B9CTRL2,
	S2MPS11_REG_B10CTRL1,
	S2MPS11_REG_B10CTRL2,
	S2MPS11_REG_L1CTRL,
	S2MPS11_REG_L2CTRL,
	S2MPS11_REG_L3CTRL,
	S2MPS11_REG_L4CTRL,
	S2MPS11_REG_L5CTRL,
	S2MPS11_REG_L6CTRL,
	S2MPS11_REG_L7CTRL,
	S2MPS11_REG_L8CTRL,
	S2MPS11_REG_L9CTRL,
	S2MPS11_REG_L10CTRL,
	S2MPS11_REG_L11CTRL,
	S2MPS11_REG_L12CTRL,
	S2MPS11_REG_L13CTRL,
	S2MPS11_REG_L14CTRL,
	S2MPS11_REG_L15CTRL,
	S2MPS11_REG_L16CTRL,
	S2MPS11_REG_L17CTRL,
	S2MPS11_REG_L18CTRL,
	S2MPS11_REG_L19CTRL,
	S2MPS11_REG_L20CTRL,
	S2MPS11_REG_L21CTRL,
	S2MPS11_REG_L22CTRL,
	S2MPS11_REG_L23CTRL,
	S2MPS11_REG_L24CTRL,
	S2MPS11_REG_L25CTRL,
	S2MPS11_REG_L26CTRL,
	S2MPS11_REG_L27CTRL,
	S2MPS11_REG_L28CTRL,
	S2MPS11_REG_L29CTRL,
	S2MPS11_REG_L30CTRL,
	S2MPS11_REG_L31CTRL,
	S2MPS11_REG_L32CTRL,
	S2MPS11_REG_L33CTRL,
	S2MPS11_REG_L34CTRL,
	S2MPS11_REG_L35CTRL,
	S2MPS11_REG_L36CTRL,
	S2MPS11_REG_L37CTRL,
	S2MPS11_REG_L38CTRL,
};

/* S2MPS11 regulator ids */
enum s2mps11_regulators {
	S2MPS11_LDO1,
	S2MPS11_LDO2,
	S2MPS11_LDO3,
	S2MPS11_LDO4,
	S2MPS11_LDO5,
	S2MPS11_LDO6,
	S2MPS11_LDO7,
	S2MPS11_LDO8,
	S2MPS11_LDO9,
	S2MPS11_LDO10,
	S2MPS11_LDO11,
	S2MPS11_LDO12,
	S2MPS11_LDO13,
	S2MPS11_LDO14,
	S2MPS11_LDO15,
	S2MPS11_LDO16,
	S2MPS11_LDO17,
	S2MPS11_LDO18,
	S2MPS11_LDO19,
	S2MPS11_LDO20,
	S2MPS11_LDO21,
	S2MPS11_LDO22,
	S2MPS11_LDO23,
	S2MPS11_LDO24,
	S2MPS11_LDO25,
	S2MPS11_LDO26,
	S2MPS11_LDO27,
	S2MPS11_LDO28,
	S2MPS11_LDO29,
	S2MPS11_LDO30,
	S2MPS11_LDO31,
	S2MPS11_LDO32,
	S2MPS11_LDO33,
	S2MPS11_LDO34,
	S2MPS11_LDO35,
	S2MPS11_LDO36,
	S2MPS11_LDO37,
	S2MPS11_LDO38,
	S2MPS11_BUCK1,
	S2MPS11_BUCK2,
	S2MPS11_BUCK3,
	S2MPS11_BUCK4,
	S2MPS11_BUCK5,
	S2MPS11_BUCK6,
	S2MPS11_BUCK7,
	S2MPS11_BUCK8,
	S2MPS11_BUCK9,
	S2MPS11_BUCK10,
	S2MPS11_AP_EN32KHZ,
	S2MPS11_CP_EN32KHZ,
	S2MPS11_BT_EN32KHZ,

	S2MPS11_REG_MAX,
};

#define S2MPS11_BUCK_MIN1	600000
#define S2MPS11_BUCK_MIN2	750000
#define S2MPS11_BUCK_MIN3	3000000
#define S2MPS11_LDO_MIN	800000
#define S2MPS11_BUCK_STEP1	6250
#define S2MPS11_BUCK_STEP2	12500
#define S2MPS11_BUCK_STEP3	25000
#define S2MPS11_LDO_STEP1	50000
#define S2MPS11_LDO_STEP2	25000
#define S2MPS11_LDO_VSEL_MASK	0x3F
#define S2MPS11_BUCK_VSEL_MASK	0xFF
#define S2MPS11_ENABLE_MASK	(0x03 << S2MPS11_ENABLE_SHIFT)
#define S2MPS11_ENABLE_SHIFT	0x06
#define S2MPS11_LDO_N_VOLTAGES	(S2MPS11_LDO_VSEL_MASK + 1)
#define S2MPS11_BUCK_N_VOLTAGES (S2MPS11_BUCK_VSEL_MASK + 1)
#define S2MPS11_RAMP_DELAY	25000		/* uV/us */


#define S2MPS11_BUCK2_RAMP_SHIFT	6
#define S2MPS11_BUCK34_RAMP_SHIFT	4
#define S2MPS11_BUCK5_RAMP_SHIFT	6
#define S2MPS11_BUCK16_RAMP_SHIFT	4
#define S2MPS11_BUCK7810_RAMP_SHIFT	2
#define S2MPS11_BUCK9_RAMP_SHIFT	0
#define S2MPS11_BUCK2_RAMP_EN_SHIFT	3
#define S2MPS11_BUCK3_RAMP_EN_SHIFT	2
#define S2MPS11_BUCK4_RAMP_EN_SHIFT	1
#define S2MPS11_BUCK6_RAMP_EN_SHIFT	0
#define S2MPS11_PMIC_EN_SHIFT	6
#define S2MPS11_REGULATOR_MAX (S2MPS11_REG_MAX - 3)

#endif /*  __LINUX_MFD_S2MPS11_H */
