/*
 * OMAP3430 Power/Reset Management register bits
 *
 * Copyright (C) 2007-2008 Texas Instruments, Inc.
 * Copyright (C) 2007-2008 Nokia Corporation
 *
 * Written by Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ARCH_ARM_MACH_OMAP2_PRM_REGBITS_34XX_H
#define __ARCH_ARM_MACH_OMAP2_PRM_REGBITS_34XX_H


#include "prm3xxx.h"

#define OMAP3430_ERROROFFSET_MASK			(0xff << 24)
#define OMAP3430_ERRORGAIN_MASK				(0xff << 16)
#define OMAP3430_INITVOLTAGE_MASK			(0xff << 8)
#define OMAP3430_TIMEOUTEN_MASK				(1 << 3)
#define OMAP3430_INITVDD_MASK				(1 << 2)
#define OMAP3430_FORCEUPDATE_MASK			(1 << 1)
#define OMAP3430_VPENABLE_MASK				(1 << 0)
#define OMAP3430_SMPSWAITTIMEMIN_SHIFT			8
#define OMAP3430_VSTEPMIN_SHIFT				0
#define OMAP3430_SMPSWAITTIMEMAX_SHIFT			8
#define OMAP3430_VSTEPMAX_SHIFT				0
#define OMAP3430_VDDMAX_SHIFT				24
#define OMAP3430_VDDMIN_SHIFT				16
#define OMAP3430_TIMEOUT_SHIFT				0
#define OMAP3430_VPVOLTAGE_MASK				(0xff << 0)
#define OMAP3430_EN_PER_SHIFT				7
#define OMAP3430_LOGICSTATEST_MASK			(1 << 2)
#define OMAP3430_LASTLOGICSTATEENTERED_MASK		(1 << 2)
#define OMAP3430_LASTPOWERSTATEENTERED_MASK		(0x3 << 0)
#define OMAP3630_GRPSEL_UART4_MASK			(1 << 18)
#define OMAP3430_GRPSEL_GPIO6_MASK			(1 << 17)
#define OMAP3430_GRPSEL_GPIO5_MASK			(1 << 16)
#define OMAP3430_GRPSEL_GPIO4_MASK			(1 << 15)
#define OMAP3430_GRPSEL_GPIO3_MASK			(1 << 14)
#define OMAP3430_GRPSEL_GPIO2_MASK			(1 << 13)
#define OMAP3430_GRPSEL_UART3_MASK			(1 << 11)
#define OMAP3430_GRPSEL_MCBSP4_MASK			(1 << 2)
#define OMAP3430_GRPSEL_MCBSP3_MASK			(1 << 1)
#define OMAP3430_GRPSEL_MCBSP2_MASK			(1 << 0)
#define OMAP3430_GRPSEL_GPIO1_MASK			(1 << 3)
#define OMAP3430_GRPSEL_GPT12_MASK			(1 << 1)
#define OMAP3430_GRPSEL_GPT1_MASK			(1 << 0)
#define OMAP3430_RST3_IVA2_MASK				(1 << 2)
#define OMAP3430_RST2_IVA2_MASK				(1 << 1)
#define OMAP3430_RST1_IVA2_MASK				(1 << 0)
#define OMAP3430_L2FLATMEMONSTATE_MASK			(0x3 << 22)
#define OMAP3430_SHAREDL2CACHEFLATONSTATE_MASK		(0x3 << 20)
#define OMAP3430_L1FLATMEMONSTATE_MASK			(0x3 << 18)
#define OMAP3430_SHAREDL1CACHEFLATONSTATE_MASK		(0x3 << 16)
#define OMAP3430_L2FLATMEMRETSTATE_MASK			(1 << 11)
#define OMAP3430_SHAREDL2CACHEFLATRETSTATE_MASK		(1 << 10)
#define OMAP3430_L1FLATMEMRETSTATE_MASK			(1 << 9)
#define OMAP3430_SHAREDL1CACHEFLATRETSTATE_MASK		(1 << 8)
#define OMAP3430_L2FLATMEMSTATEST_MASK			(0x3 << 10)
#define OMAP3430_SHAREDL2CACHEFLATSTATEST_MASK		(0x3 << 8)
#define OMAP3430_L1FLATMEMSTATEST_MASK			(0x3 << 6)
#define OMAP3430_SHAREDL1CACHEFLATSTATEST_MASK		(0x3 << 4)
#define OMAP3430_LASTL2FLATMEMSTATEENTERED_MASK			(0x3 << 10)
#define OMAP3430_LASTSHAREDL2CACHEFLATSTATEENTERED_MASK		(0x3 << 8)
#define OMAP3430ES2_SND_PERIPH_DPLL_ST_SHIFT		25
#define OMAP3430_VP2_TRANXDONE_ST_MASK			(1 << 21)
#define OMAP3430_VP1_TRANXDONE_ST_MASK			(1 << 15)
#define OMAP3430_PRM_IRQSTATUS_MPU_IVA2_DPLL_ST_SHIFT	8
#define OMAP3430_MPU_DPLL_ST_SHIFT			7
#define OMAP3430_PERIPH_DPLL_ST_SHIFT			6
#define OMAP3430_CORE_DPLL_ST_SHIFT			5
#define OMAP3430ES2_SND_PERIPH_DPLL_RECAL_EN_SHIFT		25
#define OMAP3430_PRM_IRQENABLE_MPU_IVA2_DPLL_RECAL_EN_SHIFT	8
#define OMAP3430_MPU_DPLL_RECAL_EN_SHIFT			7
#define OMAP3430_PERIPH_DPLL_RECAL_EN_SHIFT			6
#define OMAP3430_CORE_DPLL_RECAL_EN_SHIFT			5
#define OMAP3430_PM_WKDEP_MPU_EN_DSS_SHIFT		5
#define OMAP3430_PM_WKDEP_MPU_EN_IVA2_SHIFT		2
#define OMAP3430_RM_RSTCTRL_CORE_MODEM_SW_RSTPWRON_MASK		(1 << 1)
#define OMAP3430_RM_RSTCTRL_CORE_MODEM_SW_RST_MASK		(1 << 0)
#define OMAP3430_LASTMEM2STATEENTERED_MASK		(0x3 << 6)
#define OMAP3430_LASTMEM1STATEENTERED_MASK		(0x3 << 4)
#define OMAP3430_EN_IO_CHAIN_MASK			(1 << 16)
#define OMAP3430_EN_IO_MASK				(1 << 8)
#define OMAP3430_EN_GPIO1_MASK				(1 << 3)
#define OMAP3430_ST_IO_CHAIN_MASK			(1 << 16)
#define OMAP3430_ST_IO_MASK				(1 << 8)
#define OMAP3430_SYS_CLKIN_SEL_SHIFT			0
#define OMAP3430_SYS_CLKIN_SEL_WIDTH			3
#define OMAP3430_CLKOUT_EN_SHIFT			7
#define OMAP3430_PM_WKEN_DSS_EN_DSS_MASK		(1 << 0)
#define OMAP3430ES2_SAVEANDRESTORE_SHIFT		4
#define OMAP3430_PRM_VC_SMPS_SA_SA1_SHIFT		16
#define OMAP3430_PRM_VC_SMPS_SA_SA1_MASK		(0x7f << 16)
#define OMAP3430_PRM_VC_SMPS_SA_SA0_SHIFT		0
#define OMAP3430_PRM_VC_SMPS_SA_SA0_MASK		(0x7f << 0)
#define OMAP3430_VOLRA1_MASK				(0xff << 16)
#define OMAP3430_VOLRA0_MASK				(0xff << 0)
#define OMAP3430_CMDRA1_MASK				(0xff << 16)
#define OMAP3430_CMDRA0_MASK				(0xff << 0)
#define OMAP3430_VC_CMD_ON_SHIFT			24
#define OMAP3430_VC_CMD_ON_MASK				(0xFF << 24)
#define OMAP3430_VC_CMD_ONLP_SHIFT			16
#define OMAP3430_VC_CMD_RET_SHIFT			8
#define OMAP3430_VC_CMD_OFF_SHIFT			0
#define OMAP3430_HSEN_MASK				(1 << 3)
#define OMAP3430_MCODE_MASK				(0x7 << 0)
#define OMAP3430_VALID_MASK				(1 << 24)
#define OMAP3430_DATA_SHIFT				16
#define OMAP3430_REGADDR_SHIFT				8
#define OMAP3430_SLAVEADDR_SHIFT			0
#define OMAP3430_ICECRUSHER_RST_SHIFT			10
#define OMAP3430_ICEPICK_RST_SHIFT			9
#define OMAP3430_VDD2_VOLTAGE_MANAGER_RST_SHIFT		8
#define OMAP3430_VDD1_VOLTAGE_MANAGER_RST_SHIFT		7
#define OMAP3430_EXTERNAL_WARM_RST_SHIFT		6
#define OMAP3430_SECURE_WD_RST_SHIFT			5
#define OMAP3430_MPU_WD_RST_SHIFT			4
#define OMAP3430_SECURITY_VIOL_RST_SHIFT		3
#define OMAP3430_GLOBAL_SW_RST_SHIFT			1
#define OMAP3430_GLOBAL_COLD_RST_SHIFT			0
#define OMAP3430_GLOBAL_COLD_RST_MASK			(1 << 0)
#define OMAP3430_SEL_OFF_MASK				(1 << 3)
#define OMAP3430_AUTO_OFF_MASK				(1 << 2)
#define OMAP3430_SETUP_TIME2_MASK			(0xffff << 16)
#define OMAP3430_SETUP_TIME1_MASK			(0xffff << 0)
#endif
