/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MACH_MX3x_H__
#define __MACH_MX3x_H__

/*
 * MX31 memory map:
 *
 * Virt		Phys		Size	What
 * ---------------------------------------------------------------------------
 * FC000000	43F00000	1M	AIPS 1
 * FC100000	50000000	1M	SPBA
 * FC200000	53F00000	1M	AIPS 2
 * FC500000	60000000	128M	ROMPATCH
 * FC400000	68000000	128M	AVIC
 *         	70000000	256M	IPU (MAX M2)
 *         	80000000	256M	CSD0 SDRAM/DDR
 *         	90000000	256M	CSD1 SDRAM/DDR
 *         	A0000000	128M	CS0 Flash
 *         	A8000000	128M	CS1 Flash
 *         	B0000000	32M	CS2
 *         	B2000000	32M	CS3
 * F4000000	B4000000	32M	CS4
 *         	B6000000	32M	CS5
 * FC320000	B8000000	64K	NAND, SDRAM, WEIM, M3IF, EMI controllers
 *         	C0000000	64M	PCMCIA/CF
 */

/*
 * L2CC
 */
#define MX3x_L2CC_BASE_ADDR		0x30000000
#define MX3x_L2CC_SIZE			SZ_1M

/*
 * AIPS 1
 */
#define MX3x_AIPS1_BASE_ADDR		0x43f00000
#define MX3x_AIPS1_SIZE			SZ_1M
#define MX3x_MAX_BASE_ADDR			(MX3x_AIPS1_BASE_ADDR + 0x04000)
#define MX3x_EVTMON_BASE_ADDR			(MX3x_AIPS1_BASE_ADDR + 0x08000)
#define MX3x_CLKCTL_BASE_ADDR			(MX3x_AIPS1_BASE_ADDR + 0x0c000)
#define MX3x_ETB_SLOT4_BASE_ADDR		(MX3x_AIPS1_BASE_ADDR + 0x10000)
#define MX3x_ETB_SLOT5_BASE_ADDR		(MX3x_AIPS1_BASE_ADDR + 0x14000)
#define MX3x_ECT_CTIO_BASE_ADDR			(MX3x_AIPS1_BASE_ADDR + 0x18000)
#define MX3x_I2C_BASE_ADDR			(MX3x_AIPS1_BASE_ADDR + 0x80000)
#define MX3x_I2C3_BASE_ADDR			(MX3x_AIPS1_BASE_ADDR + 0x84000)
#define MX3x_UART1_BASE_ADDR			(MX3x_AIPS1_BASE_ADDR + 0x90000)
#define MX3x_UART2_BASE_ADDR			(MX3x_AIPS1_BASE_ADDR + 0x94000)
#define MX3x_I2C2_BASE_ADDR			(MX3x_AIPS1_BASE_ADDR + 0x98000)
#define MX3x_OWIRE_BASE_ADDR			(MX3x_AIPS1_BASE_ADDR + 0x9c000)
#define MX3x_SSI1_BASE_ADDR			(MX3x_AIPS1_BASE_ADDR + 0xa0000)
#define MX3x_CSPI1_BASE_ADDR			(MX3x_AIPS1_BASE_ADDR + 0xa4000)
#define MX3x_KPP_BASE_ADDR			(MX3x_AIPS1_BASE_ADDR + 0xa8000)
#define MX3x_IOMUXC_BASE_ADDR			(MX3x_AIPS1_BASE_ADDR + 0xac000)
#define MX3x_ECT_IP1_BASE_ADDR			(MX3x_AIPS1_BASE_ADDR + 0xb8000)
#define MX3x_ECT_IP2_BASE_ADDR			(MX3x_AIPS1_BASE_ADDR + 0xbc000)

/*
 * SPBA global module enabled #0
 */
#define MX3x_SPBA0_BASE_ADDR		0x50000000
#define MX3x_SPBA0_SIZE			SZ_1M
#define MX3x_UART3_BASE_ADDR			(MX3x_SPBA0_BASE_ADDR + 0x0c000)
#define MX3x_CSPI2_BASE_ADDR			(MX3x_SPBA0_BASE_ADDR + 0x10000)
#define MX3x_SSI2_BASE_ADDR			(MX3x_SPBA0_BASE_ADDR + 0x14000)
#define MX3x_ATA_DMA_BASE_ADDR			(MX3x_SPBA0_BASE_ADDR + 0x20000)
#define MX3x_MSHC1_BASE_ADDR			(MX3x_SPBA0_BASE_ADDR + 0x24000)
#define MX3x_SPBA_CTRL_BASE_ADDR		(MX3x_SPBA0_BASE_ADDR + 0x3c000)

/*
 * AIPS 2
 */
#define MX3x_AIPS2_BASE_ADDR		0x53f00000
#define MX3x_AIPS2_SIZE			SZ_1M
#define MX3x_CCM_BASE_ADDR			(MX3x_AIPS2_BASE_ADDR + 0x80000)
#define MX3x_GPT1_BASE_ADDR			(MX3x_AIPS2_BASE_ADDR + 0x90000)
#define MX3x_EPIT1_BASE_ADDR			(MX3x_AIPS2_BASE_ADDR + 0x94000)
#define MX3x_EPIT2_BASE_ADDR			(MX3x_AIPS2_BASE_ADDR + 0x98000)
#define MX3x_GPIO3_BASE_ADDR			(MX3x_AIPS2_BASE_ADDR + 0xa4000)
#define MX3x_SCC_BASE_ADDR			(MX3x_AIPS2_BASE_ADDR + 0xac000)
#define MX3x_RNGA_BASE_ADDR			(MX3x_AIPS2_BASE_ADDR + 0xb0000)
#define MX3x_IPU_CTRL_BASE_ADDR			(MX3x_AIPS2_BASE_ADDR + 0xc0000)
#define MX3x_AUDMUX_BASE_ADDR			(MX3x_AIPS2_BASE_ADDR + 0xc4000)
#define MX3x_GPIO1_BASE_ADDR			(MX3x_AIPS2_BASE_ADDR + 0xcc000)
#define MX3x_GPIO2_BASE_ADDR			(MX3x_AIPS2_BASE_ADDR + 0xd0000)
#define MX3x_SDMA_BASE_ADDR			(MX3x_AIPS2_BASE_ADDR + 0xd4000)
#define MX3x_RTC_BASE_ADDR			(MX3x_AIPS2_BASE_ADDR + 0xd8000)
#define MX3x_WDOG_BASE_ADDR			(MX3x_AIPS2_BASE_ADDR + 0xdc000)
#define MX3x_PWM_BASE_ADDR			(MX3x_AIPS2_BASE_ADDR + 0xe0000)
#define MX3x_RTIC_BASE_ADDR			(MX3x_AIPS2_BASE_ADDR + 0xec000)

/*
 * ROMP and AVIC
 */
#define MX3x_ROMP_BASE_ADDR		0x60000000
#define MX3x_ROMP_SIZE			SZ_1M

#define MX3x_AVIC_BASE_ADDR		0x68000000
#define MX3x_AVIC_SIZE			SZ_1M

/*
 * Memory regions and CS
 */
#define MX3x_IPU_MEM_BASE_ADDR		0x70000000
#define MX3x_CSD0_BASE_ADDR		0x80000000
#define MX3x_CSD1_BASE_ADDR		0x90000000

#define MX3x_CS0_BASE_ADDR		0xa0000000
#define MX3x_CS1_BASE_ADDR		0xa8000000
#define MX3x_CS2_BASE_ADDR		0xb0000000
#define MX3x_CS3_BASE_ADDR		0xb2000000

#define MX3x_CS4_BASE_ADDR		0xb4000000
#define MX3x_CS4_BASE_ADDR_VIRT		0xf6000000
#define MX3x_CS4_SIZE			SZ_32M

#define MX3x_CS5_BASE_ADDR		0xb6000000
#define MX3x_CS5_BASE_ADDR_VIRT		0xf8000000
#define MX3x_CS5_SIZE			SZ_32M

/*
 * NAND, SDRAM, WEIM, M3IF, EMI controllers
 */
#define MX3x_X_MEMC_BASE_ADDR		0xb8000000
#define MX3x_X_MEMC_SIZE		SZ_64K
#define MX3x_ESDCTL_BASE_ADDR			(MX3x_X_MEMC_BASE_ADDR + 0x1000)
#define MX3x_WEIM_BASE_ADDR			(MX3x_X_MEMC_BASE_ADDR + 0x2000)
#define MX3x_M3IF_BASE_ADDR			(MX3x_X_MEMC_BASE_ADDR + 0x3000)
#define MX3x_EMI_CTL_BASE_ADDR			(MX3x_X_MEMC_BASE_ADDR + 0x4000)
#define MX3x_PCMCIA_CTL_BASE_ADDR		MX3x_EMI_CTL_BASE_ADDR

#define MX3x_PCMCIA_MEM_BASE_ADDR	0xbc000000

/*
 * Interrupt numbers
 */
#include <asm/irq.h>
#define MX3x_INT_I2C3		(NR_IRQS_LEGACY + 3)
#define MX3x_INT_I2C2		(NR_IRQS_LEGACY + 4)
#define MX3x_INT_RTIC		(NR_IRQS_LEGACY + 6)
#define MX3x_INT_I2C		(NR_IRQS_LEGACY + 10)
#define MX3x_INT_CSPI2		(NR_IRQS_LEGACY + 13)
#define MX3x_INT_CSPI1		(NR_IRQS_LEGACY + 14)
#define MX3x_INT_ATA		(NR_IRQS_LEGACY + 15)
#define MX3x_INT_UART3		(NR_IRQS_LEGACY + 18)
#define MX3x_INT_IIM		(NR_IRQS_LEGACY + 19)
#define MX3x_INT_RNGA		(NR_IRQS_LEGACY + 22)
#define MX3x_INT_EVTMON		(NR_IRQS_LEGACY + 23)
#define MX3x_INT_KPP		(NR_IRQS_LEGACY + 24)
#define MX3x_INT_RTC		(NR_IRQS_LEGACY + 25)
#define MX3x_INT_PWM		(NR_IRQS_LEGACY + 26)
#define MX3x_INT_EPIT2		(NR_IRQS_LEGACY + 27)
#define MX3x_INT_EPIT1		(NR_IRQS_LEGACY + 28)
#define MX3x_INT_GPT		(NR_IRQS_LEGACY + 29)
#define MX3x_INT_POWER_FAIL	(NR_IRQS_LEGACY + 30)
#define MX3x_INT_UART2		(NR_IRQS_LEGACY + 32)
#define MX3x_INT_NANDFC		(NR_IRQS_LEGACY + 33)
#define MX3x_INT_SDMA		(NR_IRQS_LEGACY + 34)
#define MX3x_INT_MSHC1		(NR_IRQS_LEGACY + 39)
#define MX3x_INT_IPU_ERR	(NR_IRQS_LEGACY + 41)
#define MX3x_INT_IPU_SYN	(NR_IRQS_LEGACY + 42)
#define MX3x_INT_UART1		(NR_IRQS_LEGACY + 45)
#define MX3x_INT_ECT		(NR_IRQS_LEGACY + 48)
#define MX3x_INT_SCC_SCM	(NR_IRQS_LEGACY + 49)
#define MX3x_INT_SCC_SMN	(NR_IRQS_LEGACY + 50)
#define MX3x_INT_GPIO2		(NR_IRQS_LEGACY + 51)
#define MX3x_INT_GPIO1		(NR_IRQS_LEGACY + 52)
#define MX3x_INT_WDOG		(NR_IRQS_LEGACY + 55)
#define MX3x_INT_GPIO3		(NR_IRQS_LEGACY + 56)
#define MX3x_INT_EXT_POWER	(NR_IRQS_LEGACY + 58)
#define MX3x_INT_EXT_TEMPER	(NR_IRQS_LEGACY + 59)
#define MX3x_INT_EXT_SENSOR60	(NR_IRQS_LEGACY + 60)
#define MX3x_INT_EXT_SENSOR61	(NR_IRQS_LEGACY + 61)
#define MX3x_INT_EXT_WDOG	(NR_IRQS_LEGACY + 62)
#define MX3x_INT_EXT_TV		(NR_IRQS_LEGACY + 63)

#define MX3x_PROD_SIGNATURE		0x1	/* For MX31 */

/* Mandatory defines used globally */

#if !defined(__ASSEMBLY__) && !defined(__MXC_BOOT_UNCOMPRESS)
extern int mx35_revision(void);
extern int mx31_revision(void);
#endif

#endif /* ifndef __MACH_MX3x_H__ */
