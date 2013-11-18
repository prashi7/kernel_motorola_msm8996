/*
 * Copyright (c) 2013 Texas Instruments Inc.
 *
 * David Griego, <dagriego@biglakesoftware.com>
 * Dale Farnsworth, <dale@farnsworth.org>
 * Archit Taneja, <archit@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#ifndef __TI_VPE_REGS_H
#define __TI_VPE_REGS_H

/* VPE register offsets and field selectors */

/* VPE top level regs */
#define VPE_PID				0x0000
#define VPE_PID_MINOR_MASK		0x3f
#define VPE_PID_MINOR_SHIFT		0
#define VPE_PID_CUSTOM_MASK		0x03
#define VPE_PID_CUSTOM_SHIFT		6
#define VPE_PID_MAJOR_MASK		0x07
#define VPE_PID_MAJOR_SHIFT		8
#define VPE_PID_RTL_MASK		0x1f
#define VPE_PID_RTL_SHIFT		11
#define VPE_PID_FUNC_MASK		0xfff
#define VPE_PID_FUNC_SHIFT		16
#define VPE_PID_SCHEME_MASK		0x03
#define VPE_PID_SCHEME_SHIFT		30

#define VPE_SYSCONFIG			0x0010
#define VPE_SYSCONFIG_IDLE_MASK		0x03
#define VPE_SYSCONFIG_IDLE_SHIFT	2
#define VPE_SYSCONFIG_STANDBY_MASK	0x03
#define VPE_SYSCONFIG_STANDBY_SHIFT	4
#define VPE_FORCE_IDLE_MODE		0
#define VPE_NO_IDLE_MODE		1
#define VPE_SMART_IDLE_MODE		2
#define VPE_SMART_IDLE_WAKEUP_MODE	3
#define VPE_FORCE_STANDBY_MODE		0
#define VPE_NO_STANDBY_MODE		1
#define VPE_SMART_STANDBY_MODE		2
#define VPE_SMART_STANDBY_WAKEUP_MODE	3

#define VPE_INT0_STATUS0_RAW_SET	0x0020
#define VPE_INT0_STATUS0_RAW		VPE_INT0_STATUS0_RAW_SET
#define VPE_INT0_STATUS0_CLR		0x0028
#define VPE_INT0_STATUS0		VPE_INT0_STATUS0_CLR
#define VPE_INT0_ENABLE0_SET		0x0030
#define VPE_INT0_ENABLE0		VPE_INT0_ENABLE0_SET
#define VPE_INT0_ENABLE0_CLR		0x0038
#define VPE_INT0_LIST0_COMPLETE		(1 << 0)
#define VPE_INT0_LIST0_NOTIFY		(1 << 1)
#define VPE_INT0_LIST1_COMPLETE		(1 << 2)
#define VPE_INT0_LIST1_NOTIFY		(1 << 3)
#define VPE_INT0_LIST2_COMPLETE		(1 << 4)
#define VPE_INT0_LIST2_NOTIFY		(1 << 5)
#define VPE_INT0_LIST3_COMPLETE		(1 << 6)
#define VPE_INT0_LIST3_NOTIFY		(1 << 7)
#define VPE_INT0_LIST4_COMPLETE		(1 << 8)
#define VPE_INT0_LIST4_NOTIFY		(1 << 9)
#define VPE_INT0_LIST5_COMPLETE		(1 << 10)
#define VPE_INT0_LIST5_NOTIFY		(1 << 11)
#define VPE_INT0_LIST6_COMPLETE		(1 << 12)
#define VPE_INT0_LIST6_NOTIFY		(1 << 13)
#define VPE_INT0_LIST7_COMPLETE		(1 << 14)
#define VPE_INT0_LIST7_NOTIFY		(1 << 15)
#define VPE_INT0_DESCRIPTOR		(1 << 16)
#define VPE_DEI_FMD_INT			(1 << 18)

#define VPE_INT0_STATUS1_RAW_SET	0x0024
#define VPE_INT0_STATUS1_RAW		VPE_INT0_STATUS1_RAW_SET
#define VPE_INT0_STATUS1_CLR		0x002c
#define VPE_INT0_STATUS1		VPE_INT0_STATUS1_CLR
#define VPE_INT0_ENABLE1_SET		0x0034
#define VPE_INT0_ENABLE1		VPE_INT0_ENABLE1_SET
#define VPE_INT0_ENABLE1_CLR		0x003c
#define VPE_INT0_CHANNEL_GROUP0		(1 << 0)
#define VPE_INT0_CHANNEL_GROUP1		(1 << 1)
#define VPE_INT0_CHANNEL_GROUP2		(1 << 2)
#define VPE_INT0_CHANNEL_GROUP3		(1 << 3)
#define VPE_INT0_CHANNEL_GROUP4		(1 << 4)
#define VPE_INT0_CHANNEL_GROUP5		(1 << 5)
#define VPE_INT0_CLIENT			(1 << 7)
#define VPE_DEI_ERROR_INT		(1 << 16)
#define VPE_DS1_UV_ERROR_INT		(1 << 22)

#define VPE_INTC_EOI			0x00a0

#define VPE_CLK_ENABLE			0x0100
#define VPE_VPEDMA_CLK_ENABLE		(1 << 0)
#define VPE_DATA_PATH_CLK_ENABLE	(1 << 1)

#define VPE_CLK_RESET			0x0104
#define VPE_VPDMA_CLK_RESET_MASK	0x1
#define VPE_VPDMA_CLK_RESET_SHIFT	0
#define VPE_DATA_PATH_CLK_RESET_MASK	0x1
#define VPE_DATA_PATH_CLK_RESET_SHIFT	1
#define VPE_MAIN_RESET_MASK		0x1
#define VPE_MAIN_RESET_SHIFT		31

#define VPE_CLK_FORMAT_SELECT		0x010c
#define VPE_CSC_SRC_SELECT_MASK		0x03
#define VPE_CSC_SRC_SELECT_SHIFT	0
#define VPE_RGB_OUT_SELECT		(1 << 8)
#define VPE_DS_SRC_SELECT_MASK		0x07
#define VPE_DS_SRC_SELECT_SHIFT		9
#define VPE_DS_BYPASS			(1 << 16)
#define VPE_COLOR_SEPARATE_422		(1 << 18)

#define VPE_DS_SRC_DEI_SCALER		(5 << VPE_DS_SRC_SELECT_SHIFT)
#define VPE_CSC_SRC_DEI_SCALER		(3 << VPE_CSC_SRC_SELECT_SHIFT)

#define VPE_CLK_RANGE_MAP		0x011c
#define VPE_RANGE_RANGE_MAP_Y_MASK	0x07
#define VPE_RANGE_RANGE_MAP_Y_SHIFT	0
#define VPE_RANGE_RANGE_MAP_UV_MASK	0x07
#define VPE_RANGE_RANGE_MAP_UV_SHIFT	3
#define VPE_RANGE_MAP_ON		(1 << 6)
#define VPE_RANGE_REDUCTION_ON		(1 << 28)

/* VPE chrominance upsampler regs */
#define VPE_US1_R0			0x0304
#define VPE_US2_R0			0x0404
#define VPE_US3_R0			0x0504
#define VPE_US_C1_MASK			0x3fff
#define VPE_US_C1_SHIFT			2
#define VPE_US_C0_MASK			0x3fff
#define VPE_US_C0_SHIFT			18
#define VPE_US_MODE_MASK		0x03
#define VPE_US_MODE_SHIFT		16
#define VPE_ANCHOR_FID0_C1_MASK		0x3fff
#define VPE_ANCHOR_FID0_C1_SHIFT	2
#define VPE_ANCHOR_FID0_C0_MASK		0x3fff
#define VPE_ANCHOR_FID0_C0_SHIFT	18

#define VPE_US1_R1			0x0308
#define VPE_US2_R1			0x0408
#define VPE_US3_R1			0x0508
#define VPE_ANCHOR_FID0_C3_MASK		0x3fff
#define VPE_ANCHOR_FID0_C3_SHIFT	2
#define VPE_ANCHOR_FID0_C2_MASK		0x3fff
#define VPE_ANCHOR_FID0_C2_SHIFT	18

#define VPE_US1_R2			0x030c
#define VPE_US2_R2			0x040c
#define VPE_US3_R2			0x050c
#define VPE_INTERP_FID0_C1_MASK		0x3fff
#define VPE_INTERP_FID0_C1_SHIFT	2
#define VPE_INTERP_FID0_C0_MASK		0x3fff
#define VPE_INTERP_FID0_C0_SHIFT	18

#define VPE_US1_R3			0x0310
#define VPE_US2_R3			0x0410
#define VPE_US3_R3			0x0510
#define VPE_INTERP_FID0_C3_MASK		0x3fff
#define VPE_INTERP_FID0_C3_SHIFT	2
#define VPE_INTERP_FID0_C2_MASK		0x3fff
#define VPE_INTERP_FID0_C2_SHIFT	18

#define VPE_US1_R4			0x0314
#define VPE_US2_R4			0x0414
#define VPE_US3_R4			0x0514
#define VPE_ANCHOR_FID1_C1_MASK		0x3fff
#define VPE_ANCHOR_FID1_C1_SHIFT	2
#define VPE_ANCHOR_FID1_C0_MASK		0x3fff
#define VPE_ANCHOR_FID1_C0_SHIFT	18

#define VPE_US1_R5			0x0318
#define VPE_US2_R5			0x0418
#define VPE_US3_R5			0x0518
#define VPE_ANCHOR_FID1_C3_MASK		0x3fff
#define VPE_ANCHOR_FID1_C3_SHIFT	2
#define VPE_ANCHOR_FID1_C2_MASK		0x3fff
#define VPE_ANCHOR_FID1_C2_SHIFT	18

#define VPE_US1_R6			0x031c
#define VPE_US2_R6			0x041c
#define VPE_US3_R6			0x051c
#define VPE_INTERP_FID1_C1_MASK		0x3fff
#define VPE_INTERP_FID1_C1_SHIFT	2
#define VPE_INTERP_FID1_C0_MASK		0x3fff
#define VPE_INTERP_FID1_C0_SHIFT	18

#define VPE_US1_R7			0x0320
#define VPE_US2_R7			0x0420
#define VPE_US3_R7			0x0520
#define VPE_INTERP_FID0_C3_MASK		0x3fff
#define VPE_INTERP_FID0_C3_SHIFT	2
#define VPE_INTERP_FID0_C2_MASK		0x3fff
#define VPE_INTERP_FID0_C2_SHIFT	18

/* VPE de-interlacer regs */
#define VPE_DEI_FRAME_SIZE		0x0600
#define VPE_DEI_WIDTH_MASK		0x07ff
#define VPE_DEI_WIDTH_SHIFT		0
#define VPE_DEI_HEIGHT_MASK		0x07ff
#define VPE_DEI_HEIGHT_SHIFT		16
#define VPE_DEI_INTERLACE_BYPASS	(1 << 29)
#define VPE_DEI_FIELD_FLUSH		(1 << 30)
#define VPE_DEI_PROGRESSIVE		(1 << 31)

#define VPE_MDT_BYPASS			0x0604
#define VPE_MDT_TEMPMAX_BYPASS		(1 << 0)
#define VPE_MDT_SPATMAX_BYPASS		(1 << 1)

#define VPE_MDT_SF_THRESHOLD		0x0608
#define VPE_MDT_SF_SC_THR1_MASK		0xff
#define VPE_MDT_SF_SC_THR1_SHIFT	0
#define VPE_MDT_SF_SC_THR2_MASK		0xff
#define VPE_MDT_SF_SC_THR2_SHIFT	0
#define VPE_MDT_SF_SC_THR3_MASK		0xff
#define VPE_MDT_SF_SC_THR3_SHIFT	0

#define VPE_EDI_CONFIG			0x060c
#define VPE_EDI_INP_MODE_MASK		0x03
#define VPE_EDI_INP_MODE_SHIFT		0
#define VPE_EDI_ENABLE_3D		(1 << 2)
#define VPE_EDI_ENABLE_CHROMA_3D	(1 << 3)
#define VPE_EDI_CHROMA3D_COR_THR_MASK	0xff
#define VPE_EDI_CHROMA3D_COR_THR_SHIFT	8
#define VPE_EDI_DIR_COR_LOWER_THR_MASK	0xff
#define VPE_EDI_DIR_COR_LOWER_THR_SHIFT	16
#define VPE_EDI_COR_SCALE_FACTOR_MASK	0xff
#define VPE_EDI_COR_SCALE_FACTOR_SHIFT	23

#define VPE_DEI_EDI_LUT_R0		0x0610
#define VPE_EDI_LUT0_MASK		0x1f
#define VPE_EDI_LUT0_SHIFT		0
#define VPE_EDI_LUT1_MASK		0x1f
#define VPE_EDI_LUT1_SHIFT		8
#define VPE_EDI_LUT2_MASK		0x1f
#define VPE_EDI_LUT2_SHIFT		16
#define VPE_EDI_LUT3_MASK		0x1f
#define VPE_EDI_LUT3_SHIFT		24

#define VPE_DEI_EDI_LUT_R1		0x0614
#define VPE_EDI_LUT0_MASK		0x1f
#define VPE_EDI_LUT0_SHIFT		0
#define VPE_EDI_LUT1_MASK		0x1f
#define VPE_EDI_LUT1_SHIFT		8
#define VPE_EDI_LUT2_MASK		0x1f
#define VPE_EDI_LUT2_SHIFT		16
#define VPE_EDI_LUT3_MASK		0x1f
#define VPE_EDI_LUT3_SHIFT		24

#define VPE_DEI_EDI_LUT_R2		0x0618
#define VPE_EDI_LUT4_MASK		0x1f
#define VPE_EDI_LUT4_SHIFT		0
#define VPE_EDI_LUT5_MASK		0x1f
#define VPE_EDI_LUT5_SHIFT		8
#define VPE_EDI_LUT6_MASK		0x1f
#define VPE_EDI_LUT6_SHIFT		16
#define VPE_EDI_LUT7_MASK		0x1f
#define VPE_EDI_LUT7_SHIFT		24

#define VPE_DEI_EDI_LUT_R3		0x061c
#define VPE_EDI_LUT8_MASK		0x1f
#define VPE_EDI_LUT8_SHIFT		0
#define VPE_EDI_LUT9_MASK		0x1f
#define VPE_EDI_LUT9_SHIFT		8
#define VPE_EDI_LUT10_MASK		0x1f
#define VPE_EDI_LUT10_SHIFT		16
#define VPE_EDI_LUT11_MASK		0x1f
#define VPE_EDI_LUT11_SHIFT		24

#define VPE_DEI_FMD_WINDOW_R0		0x0620
#define VPE_FMD_WINDOW_MINX_MASK	0x07ff
#define VPE_FMD_WINDOW_MINX_SHIFT	0
#define VPE_FMD_WINDOW_MAXX_MASK	0x07ff
#define VPE_FMD_WINDOW_MAXX_SHIFT	16
#define VPE_FMD_WINDOW_ENABLE		(1 << 31)

#define VPE_DEI_FMD_WINDOW_R1		0x0624
#define VPE_FMD_WINDOW_MINY_MASK	0x07ff
#define VPE_FMD_WINDOW_MINY_SHIFT	0
#define VPE_FMD_WINDOW_MAXY_MASK	0x07ff
#define VPE_FMD_WINDOW_MAXY_SHIFT	16

#define VPE_DEI_FMD_CONTROL_R0		0x0628
#define VPE_FMD_ENABLE			(1 << 0)
#define VPE_FMD_LOCK			(1 << 1)
#define VPE_FMD_JAM_DIR			(1 << 2)
#define VPE_FMD_BED_ENABLE		(1 << 3)
#define VPE_FMD_CAF_FIELD_THR_MASK	0xff
#define VPE_FMD_CAF_FIELD_THR_SHIFT	16
#define VPE_FMD_CAF_LINE_THR_MASK	0xff
#define VPE_FMD_CAF_LINE_THR_SHIFT	24

#define VPE_DEI_FMD_CONTROL_R1		0x062c
#define VPE_FMD_CAF_THR_MASK		0x000fffff
#define VPE_FMD_CAF_THR_SHIFT		0

#define VPE_DEI_FMD_STATUS_R0		0x0630
#define VPE_FMD_CAF_MASK		0x000fffff
#define VPE_FMD_CAF_SHIFT		0
#define VPE_FMD_RESET			(1 << 24)

#define VPE_DEI_FMD_STATUS_R1		0x0634
#define VPE_FMD_FIELD_DIFF_MASK		0x0fffffff
#define VPE_FMD_FIELD_DIFF_SHIFT	0

#define VPE_DEI_FMD_STATUS_R2		0x0638
#define VPE_FMD_FRAME_DIFF_MASK		0x000fffff
#define VPE_FMD_FRAME_DIFF_SHIFT	0

/* VPE scaler regs */
#define VPE_SC_MP_SC0			0x0700
#define VPE_INTERLACE_O			(1 << 0)
#define VPE_LINEAR			(1 << 1)
#define VPE_SC_BYPASS			(1 << 2)
#define VPE_INVT_FID			(1 << 3)
#define VPE_USE_RAV			(1 << 4)
#define VPE_ENABLE_EV			(1 << 5)
#define VPE_AUTO_HS			(1 << 6)
#define VPE_DCM_2X			(1 << 7)
#define VPE_DCM_4X			(1 << 8)
#define VPE_HP_BYPASS			(1 << 9)
#define VPE_INTERLACE_I			(1 << 10)
#define VPE_ENABLE_SIN2_VER_INTP	(1 << 11)
#define VPE_Y_PK_EN			(1 << 14)
#define VPE_TRIM			(1 << 15)
#define VPE_SELFGEN_FID			(1 << 16)

#define VPE_SC_MP_SC1			0x0704
#define VPE_ROW_ACC_INC_MASK		0x07ffffff
#define VPE_ROW_ACC_INC_SHIFT		0

#define VPE_SC_MP_SC2			0x0708
#define VPE_ROW_ACC_OFFSET_MASK		0x0fffffff
#define VPE_ROW_ACC_OFFSET_SHIFT	0

#define VPE_SC_MP_SC3			0x070c
#define VPE_ROW_ACC_OFFSET_B_MASK	0x0fffffff
#define VPE_ROW_ACC_OFFSET_B_SHIFT	0

#define VPE_SC_MP_SC4			0x0710
#define VPE_TAR_H_MASK			0x07ff
#define VPE_TAR_H_SHIFT			0
#define VPE_TAR_W_MASK			0x07ff
#define VPE_TAR_W_SHIFT			12
#define VPE_LIN_ACC_INC_U_MASK		0x07
#define VPE_LIN_ACC_INC_U_SHIFT		24
#define VPE_NLIN_ACC_INIT_U_MASK	0x07
#define VPE_NLIN_ACC_INIT_U_SHIFT	28

#define VPE_SC_MP_SC5			0x0714
#define VPE_SRC_H_MASK			0x07ff
#define VPE_SRC_H_SHIFT			0
#define VPE_SRC_W_MASK			0x07ff
#define VPE_SRC_W_SHIFT			12
#define VPE_NLIN_ACC_INC_U_MASK		0x07
#define VPE_NLIN_ACC_INC_U_SHIFT	24

#define VPE_SC_MP_SC6			0x0718
#define VPE_ROW_ACC_INIT_RAV_MASK	0x03ff
#define VPE_ROW_ACC_INIT_RAV_SHIFT	0
#define VPE_ROW_ACC_INIT_RAV_B_MASK	0x03ff
#define VPE_ROW_ACC_INIT_RAV_B_SHIFT	10

#define VPE_SC_MP_SC8			0x0720
#define VPE_NLIN_LEFT_MASK		0x07ff
#define VPE_NLIN_LEFT_SHIFT		0
#define VPE_NLIN_RIGHT_MASK		0x07ff
#define VPE_NLIN_RIGHT_SHIFT		12

#define VPE_SC_MP_SC9			0x0724
#define VPE_LIN_ACC_INC			VPE_SC_MP_SC9

#define VPE_SC_MP_SC10			0x0728
#define VPE_NLIN_ACC_INIT		VPE_SC_MP_SC10

#define VPE_SC_MP_SC11			0x072c
#define VPE_NLIN_ACC_INC		VPE_SC_MP_SC11

#define VPE_SC_MP_SC12			0x0730
#define VPE_COL_ACC_OFFSET_MASK		0x01ffffff
#define VPE_COL_ACC_OFFSET_SHIFT	0

#define VPE_SC_MP_SC13			0x0734
#define VPE_SC_FACTOR_RAV_MASK		0x03ff
#define VPE_SC_FACTOR_RAV_SHIFT		0
#define VPE_CHROMA_INTP_THR_MASK	0x03ff
#define VPE_CHROMA_INTP_THR_SHIFT	12
#define VPE_DELTA_CHROMA_THR_MASK	0x0f
#define VPE_DELTA_CHROMA_THR_SHIFT	24

#define VPE_SC_MP_SC17			0x0744
#define VPE_EV_THR_MASK			0x03ff
#define VPE_EV_THR_SHIFT		12
#define VPE_DELTA_LUMA_THR_MASK		0x0f
#define VPE_DELTA_LUMA_THR_SHIFT	24
#define VPE_DELTA_EV_THR_MASK		0x0f
#define VPE_DELTA_EV_THR_SHIFT		28

#define VPE_SC_MP_SC18			0x0748
#define VPE_HS_FACTOR_MASK		0x03ff
#define VPE_HS_FACTOR_SHIFT		0
#define VPE_CONF_DEFAULT_MASK		0x01ff
#define VPE_CONF_DEFAULT_SHIFT		16

#define VPE_SC_MP_SC19			0x074c
#define VPE_HPF_COEFF0_MASK		0xff
#define VPE_HPF_COEFF0_SHIFT		0
#define VPE_HPF_COEFF1_MASK		0xff
#define VPE_HPF_COEFF1_SHIFT		8
#define VPE_HPF_COEFF2_MASK		0xff
#define VPE_HPF_COEFF2_SHIFT		16
#define VPE_HPF_COEFF3_MASK		0xff
#define VPE_HPF_COEFF3_SHIFT		23

#define VPE_SC_MP_SC20			0x0750
#define VPE_HPF_COEFF4_MASK		0xff
#define VPE_HPF_COEFF4_SHIFT		0
#define VPE_HPF_COEFF5_MASK		0xff
#define VPE_HPF_COEFF5_SHIFT		8
#define VPE_HPF_NORM_SHIFT_MASK		0x07
#define VPE_HPF_NORM_SHIFT_SHIFT	16
#define VPE_NL_LIMIT_MASK		0x1ff
#define VPE_NL_LIMIT_SHIFT		20

#define VPE_SC_MP_SC21			0x0754
#define VPE_NL_LO_THR_MASK		0x01ff
#define VPE_NL_LO_THR_SHIFT		0
#define VPE_NL_LO_SLOPE_MASK		0xff
#define VPE_NL_LO_SLOPE_SHIFT		16

#define VPE_SC_MP_SC22			0x0758
#define VPE_NL_HI_THR_MASK		0x01ff
#define VPE_NL_HI_THR_SHIFT		0
#define VPE_NL_HI_SLOPE_SH_MASK		0x07
#define VPE_NL_HI_SLOPE_SH_SHIFT	16

#define VPE_SC_MP_SC23			0x075c
#define VPE_GRADIENT_THR_MASK		0x07ff
#define VPE_GRADIENT_THR_SHIFT		0
#define VPE_GRADIENT_THR_RANGE_MASK	0x0f
#define VPE_GRADIENT_THR_RANGE_SHIFT	12
#define VPE_MIN_GY_THR_MASK		0xff
#define VPE_MIN_GY_THR_SHIFT		16
#define VPE_MIN_GY_THR_RANGE_MASK	0x0f
#define VPE_MIN_GY_THR_RANGE_SHIFT	28

#define VPE_SC_MP_SC24			0x0760
#define VPE_ORG_H_MASK			0x07ff
#define VPE_ORG_H_SHIFT			0
#define VPE_ORG_W_MASK			0x07ff
#define VPE_ORG_W_SHIFT			16

#define VPE_SC_MP_SC25			0x0764
#define VPE_OFF_H_MASK			0x07ff
#define VPE_OFF_H_SHIFT			0
#define VPE_OFF_W_MASK			0x07ff
#define VPE_OFF_W_SHIFT			16

/* VPE color space converter regs */
#define VPE_CSC_CSC00			0x5700
#define VPE_CSC_A0_MASK			0x1fff
#define VPE_CSC_A0_SHIFT		0
#define VPE_CSC_B0_MASK			0x1fff
#define VPE_CSC_B0_SHIFT		16

#define VPE_CSC_CSC01			0x5704
#define VPE_CSC_C0_MASK			0x1fff
#define VPE_CSC_C0_SHIFT		0
#define VPE_CSC_A1_MASK			0x1fff
#define VPE_CSC_A1_SHIFT		16

#define VPE_CSC_CSC02			0x5708
#define VPE_CSC_B1_MASK			0x1fff
#define VPE_CSC_B1_SHIFT		0
#define VPE_CSC_C1_MASK			0x1fff
#define VPE_CSC_C1_SHIFT		16

#define VPE_CSC_CSC03			0x570c
#define VPE_CSC_A2_MASK			0x1fff
#define VPE_CSC_A2_SHIFT		0
#define VPE_CSC_B2_MASK			0x1fff
#define VPE_CSC_B2_SHIFT		16

#define VPE_CSC_CSC04			0x5710
#define VPE_CSC_C2_MASK			0x1fff
#define VPE_CSC_C2_SHIFT		0
#define VPE_CSC_D0_MASK			0x0fff
#define VPE_CSC_D0_SHIFT		16

#define VPE_CSC_CSC05			0x5714
#define VPE_CSC_D1_MASK			0x0fff
#define VPE_CSC_D1_SHIFT		0
#define VPE_CSC_D2_MASK			0x0fff
#define VPE_CSC_D2_SHIFT		16
#define VPE_CSC_BYPASS			(1 << 28)

#endif
