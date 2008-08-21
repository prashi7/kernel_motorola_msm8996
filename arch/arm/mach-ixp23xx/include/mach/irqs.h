/*
 * arch/arm/mach-ixp23xx/include/mach/irqs.h
 *
 * IRQ definitions for IXP23XX based systems
 *
 * Author: Naeem Afzal <naeem.m.afzal@intel.com>
 *
 * Copyright (C) 2003-2004 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H

#define NR_IXP23XX_IRQS			IRQ_IXP23XX_INTB+1
#define IRQ_IXP23XX_EXTIRQS		NR_IXP23XX_IRQS


#define IRQ_IXP23XX_DBG0		0	/* Debug/Execution/MBox */
#define IRQ_IXP23XX_DBG1		1	/* Debug/Execution/MBox */
#define IRQ_IXP23XX_NPE_TRG		2	/* npe_trigger */
#define IRQ_IXP23XX_TIMER1		3	/* Timer[0] */
#define IRQ_IXP23XX_TIMER2		4	/* Timer[1] */
#define IRQ_IXP23XX_TIMESTAMP		5	/* Timer[2], Time-stamp */
#define IRQ_IXP23XX_WDOG		6	/* Time[3], Watchdog Timer */
#define IRQ_IXP23XX_PCI_DBELL		7	/* PCI Doorbell */
#define IRQ_IXP23XX_PCI_DMA1		8	/* PCI DMA Channel 1 */
#define IRQ_IXP23XX_PCI_DMA2		9	/* PCI DMA Channel 2 */
#define IRQ_IXP23XX_PCI_DMA3		10	/* PCI DMA Channel 3 */
#define IRQ_IXP23XX_PCI_INT_RPH		11	/* pcxg_pci_int_rph */
#define IRQ_IXP23XX_CPP_PMU		12	/* xpxg_pm_int_rpl */
#define IRQ_IXP23XX_SWINT0		13	/* S/W Interrupt0 */
#define IRQ_IXP23XX_SWINT1		14	/* S/W Interrupt1 */
#define IRQ_IXP23XX_UART2		15	/* UART1 Interrupt */
#define IRQ_IXP23XX_UART1		16	/* UART0 Interrupt */
#define IRQ_IXP23XX_XSI_PMU_ROLLOVER	17	/* AHB Performance M. Unit counter rollover */
#define IRQ_IXP23XX_XSI_AHB_PM0		18	/* intr_pm_o */
#define IRQ_IXP23XX_XSI_AHB_ECE0	19	/* intr_ece_o */
#define IRQ_IXP23XX_XSI_AHB_GASKET	20	/* gas_intr_o */
#define IRQ_IXP23XX_XSI_CPP		21	/* xsi2cpp_int */
#define IRQ_IXP23XX_CPP_XSI		22	/* cpp2xsi_int */
#define IRQ_IXP23XX_ME_ATTN0		23	/* ME_ATTN */
#define IRQ_IXP23XX_ME_ATTN1		24	/* ME_ATTN */
#define IRQ_IXP23XX_ME_ATTN2		25	/* ME_ATTN */
#define IRQ_IXP23XX_ME_ATTN3		26	/* ME_ATTN */
#define IRQ_IXP23XX_PCI_ERR_RPH		27	/* PCXG_PCI_ERR_RPH */
#define IRQ_IXP23XX_D0XG_ECC_CORR	28	/* D0XG_DRAM_ECC_CORR */
#define IRQ_IXP23XX_D0XG_ECC_UNCORR	29	/* D0XG_DRAM_ECC_UNCORR */
#define IRQ_IXP23XX_SRAM_ERR1		30	/* SRAM1_ERR */
#define IRQ_IXP23XX_SRAM_ERR0		31	/* SRAM0_ERR */
#define IRQ_IXP23XX_MEDIA_ERR		32	/* MEDIA_ERR */
#define IRQ_IXP23XX_STH_DRAM_ECC_MAJ	33	/* STH_DRAM0_ECC_MAJ */
#define IRQ_IXP23XX_GPIO6		34	/* GPIO0 interrupts */
#define IRQ_IXP23XX_GPIO7		35	/* GPIO1 interrupts */
#define IRQ_IXP23XX_GPIO8		36	/* GPIO2 interrupts */
#define IRQ_IXP23XX_GPIO9		37	/* GPIO3 interrupts */
#define IRQ_IXP23XX_GPIO10		38	/* GPIO4 interrupts */
#define IRQ_IXP23XX_GPIO11		39	/* GPIO5 interrupts */
#define IRQ_IXP23XX_GPIO12		40	/* GPIO6 interrupts */
#define IRQ_IXP23XX_GPIO13		41	/* GPIO7 interrupts */
#define IRQ_IXP23XX_GPIO14		42	/* GPIO8 interrupts */
#define IRQ_IXP23XX_GPIO15		43	/* GPIO9 interrupts */
#define IRQ_IXP23XX_SHAC_RING0		44	/* SHAC Ring Full */
#define IRQ_IXP23XX_SHAC_RING1		45	/* SHAC Ring Full */
#define IRQ_IXP23XX_SHAC_RING2		46	/* SHAC Ring Full */
#define IRQ_IXP23XX_SHAC_RING3		47	/* SHAC Ring Full */
#define IRQ_IXP23XX_SHAC_RING4		48	/* SHAC Ring Full */
#define IRQ_IXP23XX_SHAC_RING5		49	/* SHAC Ring Full */
#define IRQ_IXP23XX_SHAC_RING6		50	/* SHAC RING Full */
#define IRQ_IXP23XX_SHAC_RING7		51	/* SHAC Ring Full */
#define IRQ_IXP23XX_SHAC_RING8		52	/* SHAC Ring Full */
#define IRQ_IXP23XX_SHAC_RING9		53	/* SHAC Ring Full */
#define IRQ_IXP23XX_SHAC_RING10		54	/* SHAC Ring Full */
#define IRQ_IXP23XX_SHAC_RING11		55	/* SHAC Ring Full */
#define IRQ_IXP23XX_ME_THREAD_A0_ME0	56	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A1_ME0	57	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A2_ME0	58	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A3_ME0	59	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A4_ME0	60	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A5_ME0	61	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A6_ME0	62	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A7_ME0	63	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A8_ME1	64	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A9_ME1	65	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A10_ME1	66	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A11_ME1	67	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A12_ME1	68	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A13_ME1	69	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A14_ME1	70	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A15_ME1	71	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A16_ME2	72	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A17_ME2	73	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A18_ME2	74	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A19_ME2	75	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A20_ME2	76	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A21_ME2	77	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A22_ME2	78	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A23_ME2	79	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A24_ME3	80	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A25_ME3	81	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A26_ME3	82	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A27_ME3	83	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A28_ME3	84	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A29_ME3	85	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A30_ME3	86	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_A31_ME3	87	/* ME_THREAD_A */
#define IRQ_IXP23XX_ME_THREAD_B0_ME0	88	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B1_ME0	89	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B2_ME0	90	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B3_ME0	91	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B4_ME0	92	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B5_ME0	93	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B6_ME0	94	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B7_ME0	95	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B8_ME1	96	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B9_ME1	97	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B10_ME1	98	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B11_ME1	99	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B12_ME1	100	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B13_ME1	101	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B14_ME1	102	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B15_ME1	103	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B16_ME2	104	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B17_ME2	105	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B18_ME2	106	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B19_ME2	107	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B20_ME2	108	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B21_ME2	109	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B22_ME2	110	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B23_ME2	111	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B24_ME3	112	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B25_ME3	113	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B26_ME3	114	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B27_ME3	115	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B28_ME3	116	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B29_ME3	117	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B30_ME3	118	/* ME_THREAD_B */
#define IRQ_IXP23XX_ME_THREAD_B31_ME3	119	/* ME_THREAD_B */

#define NUM_IXP23XX_RAW_IRQS		120

#define IRQ_IXP23XX_INTA		120	/* Indirect pcxg_pci_int_rph */
#define IRQ_IXP23XX_INTB		121	/* Indirect pcxg_pci_int_rph */

#define NR_IXP23XX_IRQ			(IRQ_IXP23XX_INTB + 1)

/*
 * We default to 32 per-board IRQs. Increase this number if you need
 * more, but keep it realistic.
 */
#define NR_IXP23XX_MACH_IRQS 		32

#define NR_IRQS				(NR_IXP23XX_IRQS + NR_IXP23XX_MACH_IRQS)

#define IXP23XX_MACH_IRQ(irq) 		(NR_IXP23XX_IRQ + (irq))


/*
 * IXDP2351-specific interrupts
 */

/*
 * External PCI interrupts signaled through INTB
 *
 */
#define IXDP2351_INTB_IRQ_BASE 		0
#define IRQ_IXDP2351_INTA_82546		IXP23XX_MACH_IRQ(0)
#define IRQ_IXDP2351_INTB_82546		IXP23XX_MACH_IRQ(1)
#define IRQ_IXDP2351_SPCI_DB_0		IXP23XX_MACH_IRQ(2)
#define IRQ_IXDP2351_SPCI_DB_1		IXP23XX_MACH_IRQ(3)
#define IRQ_IXDP2351_SPCI_PMC_INTA	IXP23XX_MACH_IRQ(4)
#define IRQ_IXDP2351_SPCI_PMC_INTB	IXP23XX_MACH_IRQ(5)
#define IRQ_IXDP2351_SPCI_PMC_INTC	IXP23XX_MACH_IRQ(6)
#define IRQ_IXDP2351_SPCI_PMC_INTD	IXP23XX_MACH_IRQ(7)
#define IRQ_IXDP2351_SPCI_FIC		IXP23XX_MACH_IRQ(8)

#define IXDP2351_INTB_IRQ_BIT(irq)	(irq - IXP23XX_MACH_IRQ(0))
#define IXDP2351_INTB_IRQ_MASK(irq)	(1 << IXDP2351_INTB_IRQ_BIT(irq))
#define IXDP2351_INTB_IRQ_VALID		0x01FF
#define IXDP2351_INTB_IRQ_NUM 		16

/*
 * Other external interrupts signaled through INTA
 */
#define IXDP2351_INTA_IRQ_BASE 		16
#define IRQ_IXDP2351_IPMI_FROM		IXP23XX_MACH_IRQ(16)
#define IRQ_IXDP2351_125US		IXP23XX_MACH_IRQ(17)
#define IRQ_IXDP2351_DB_0_ADD		IXP23XX_MACH_IRQ(18)
#define IRQ_IXDP2351_DB_1_ADD		IXP23XX_MACH_IRQ(19)
#define IRQ_IXDP2351_DEBUG1		IXP23XX_MACH_IRQ(20)
#define IRQ_IXDP2351_ADD_UART		IXP23XX_MACH_IRQ(21)
#define IRQ_IXDP2351_FIC_ADD		IXP23XX_MACH_IRQ(24)
#define IRQ_IXDP2351_CS8900		IXP23XX_MACH_IRQ(25)
#define IRQ_IXDP2351_BBSRAM		IXP23XX_MACH_IRQ(26)
#define IRQ_IXDP2351_CONFIG_MEDIA	IXP23XX_MACH_IRQ(27)
#define IRQ_IXDP2351_CLOCK_REF		IXP23XX_MACH_IRQ(28)
#define IRQ_IXDP2351_A10_NP		IXP23XX_MACH_IRQ(29)
#define IRQ_IXDP2351_A11_NP		IXP23XX_MACH_IRQ(30)
#define IRQ_IXDP2351_DEBUG_NP		IXP23XX_MACH_IRQ(31)

#define IXDP2351_INTA_IRQ_BIT(irq) 	(irq - IXP23XX_MACH_IRQ(16))
#define IXDP2351_INTA_IRQ_MASK(irq) 	(1 << IXDP2351_INTA_IRQ_BIT(irq))
#define IXDP2351_INTA_IRQ_VALID 	0xFF3F
#define IXDP2351_INTA_IRQ_NUM 		16


/*
 * ADI RoadRunner IRQs
 */
#define IRQ_ROADRUNNER_PCI_INTA 	IRQ_IXP23XX_INTA
#define IRQ_ROADRUNNER_PCI_INTB 	IRQ_IXP23XX_INTB
#define IRQ_ROADRUNNER_PCI_INTC 	IRQ_IXP23XX_GPIO11
#define IRQ_ROADRUNNER_PCI_INTD 	IRQ_IXP23XX_GPIO12

/*
 * Put new board definitions here
 */


#endif
