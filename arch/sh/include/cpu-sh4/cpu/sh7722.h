#ifndef __ASM_SH7722_H__
#define __ASM_SH7722_H__

/* Boot Mode Pins:
 *
 * MD0: CPG - Clock Mode 0->3
 * MD1: CPG - Clock Mode 0->3
 * MD2: CPG - Reserved (L: Normal operation)
 * MD3: BSC - Area0 Bus Width (16/32-bit) [CS0BCR.9,10]
 * MD5: BSC - Endian Mode (L: Big, H: Little) [CMNCR.3]
 * MD8: Test Mode
 */

/* Pin Function Controller:
 * GPIO_FN_xx - GPIO used to select pin function
 * GPIO_Pxx - GPIO mapped to real I/O pin on CPU
 */
enum {
	/* PTA */
	GPIO_PTA7, GPIO_PTA6, GPIO_PTA5, GPIO_PTA4,
	GPIO_PTA3, GPIO_PTA2, GPIO_PTA1, GPIO_PTA0,

	/* PTB */
	GPIO_PTB7, GPIO_PTB6, GPIO_PTB5, GPIO_PTB4,
	GPIO_PTB3, GPIO_PTB2, GPIO_PTB1, GPIO_PTB0,

	/* PTC */
	GPIO_PTC7, GPIO_PTC5, GPIO_PTC4, GPIO_PTC3,
	GPIO_PTC2, GPIO_PTC0,

	/* PTD */
	GPIO_PTD7, GPIO_PTD6, GPIO_PTD5, GPIO_PTD4,
	GPIO_PTD3, GPIO_PTD2, GPIO_PTD1, GPIO_PTD0,

	/* PTE */
	GPIO_PTE7, GPIO_PTE6, GPIO_PTE5, GPIO_PTE4,
	GPIO_PTE1, GPIO_PTE0,

	/* PTF */
	GPIO_PTF6, GPIO_PTF5, GPIO_PTF4, GPIO_PTF3,
	GPIO_PTF2, GPIO_PTF1, GPIO_PTF0,

	/* PTG */
	GPIO_PTG4, GPIO_PTG3, GPIO_PTG2, GPIO_PTG1, GPIO_PTG0,

	/* PTH */
	GPIO_PTH7, GPIO_PTH6, GPIO_PTH5, GPIO_PTH4,
	GPIO_PTH3, GPIO_PTH2, GPIO_PTH1, GPIO_PTH0,

	/* PTJ */
	GPIO_PTJ7, GPIO_PTJ6, GPIO_PTJ5, GPIO_PTJ1, GPIO_PTJ0,

	/* PTK */
	GPIO_PTK6, GPIO_PTK5, GPIO_PTK4, GPIO_PTK3,
	GPIO_PTK2, GPIO_PTK1, GPIO_PTK0,

	/* PTL */
	GPIO_PTL7, GPIO_PTL6, GPIO_PTL5, GPIO_PTL4,
	GPIO_PTL3, GPIO_PTL2, GPIO_PTL1, GPIO_PTL0,

	/* PTM */
	GPIO_PTM7, GPIO_PTM6, GPIO_PTM5, GPIO_PTM4,
	GPIO_PTM3, GPIO_PTM2, GPIO_PTM1, GPIO_PTM0,

	/* PTN */
	GPIO_PTN7, GPIO_PTN6, GPIO_PTN5, GPIO_PTN4,
	GPIO_PTN3, GPIO_PTN2, GPIO_PTN1, GPIO_PTN0,

	/* PTQ */
	GPIO_PTQ7, GPIO_PTQ6, GPIO_PTQ5, GPIO_PTQ4,
	GPIO_PTQ3, GPIO_PTQ2, GPIO_PTQ1, GPIO_PTQ0,

	/* PTR */
	GPIO_PTR4, GPIO_PTR3, GPIO_PTR2, GPIO_PTR1, GPIO_PTR0,

	/* PTS */
	GPIO_PTS4, GPIO_PTS3, GPIO_PTS2, GPIO_PTS1, GPIO_PTS0,

	/* PTT */
	GPIO_PTT4, GPIO_PTT3, GPIO_PTT2, GPIO_PTT1, GPIO_PTT0,

	/* PTU */
	GPIO_PTU4, GPIO_PTU3, GPIO_PTU2, GPIO_PTU1, GPIO_PTU0,

	/* PTV */
	GPIO_PTV4, GPIO_PTV3, GPIO_PTV2, GPIO_PTV1, GPIO_PTV0,

	/* PTW */
	GPIO_PTW6, GPIO_PTW5, GPIO_PTW4, GPIO_PTW3,
	GPIO_PTW2, GPIO_PTW1, GPIO_PTW0,

	/* PTX */
	GPIO_PTX6, GPIO_PTX5, GPIO_PTX4, GPIO_PTX3,
	GPIO_PTX2, GPIO_PTX1, GPIO_PTX0,

	/* PTY */
	GPIO_PTY5, GPIO_PTY4, GPIO_PTY3, GPIO_PTY2,
	GPIO_PTY1, GPIO_PTY0,

	/* PTZ */
	GPIO_PTZ5, GPIO_PTZ4, GPIO_PTZ3, GPIO_PTZ2, GPIO_PTZ1,

	/* SCIF0 */
	GPIO_FN_SCIF0_TXD, GPIO_FN_SCIF0_RXD,
	GPIO_FN_SCIF0_RTS, GPIO_FN_SCIF0_CTS, GPIO_FN_SCIF0_SCK,

	/* SCIF1 */
	GPIO_FN_SCIF1_TXD, GPIO_FN_SCIF1_RXD,
	GPIO_FN_SCIF1_RTS, GPIO_FN_SCIF1_CTS, GPIO_FN_SCIF1_SCK,

	/* SCIF2 */
	GPIO_FN_SCIF2_TXD, GPIO_FN_SCIF2_RXD,
	GPIO_FN_SCIF2_RTS, GPIO_FN_SCIF2_CTS, GPIO_FN_SCIF2_SCK,

	/* SIO */
	GPIO_FN_SIOTXD, GPIO_FN_SIORXD,
	GPIO_FN_SIOD, GPIO_FN_SIOSTRB0, GPIO_FN_SIOSTRB1,
	GPIO_FN_SIOSCK, GPIO_FN_SIOMCK,

	/* CEU */
	GPIO_FN_VIO_D15, GPIO_FN_VIO_D14, GPIO_FN_VIO_D13, GPIO_FN_VIO_D12,
	GPIO_FN_VIO_D11, GPIO_FN_VIO_D10, GPIO_FN_VIO_D9, GPIO_FN_VIO_D8,
	GPIO_FN_VIO_D7, GPIO_FN_VIO_D6, GPIO_FN_VIO_D5, GPIO_FN_VIO_D4,
	GPIO_FN_VIO_D3, GPIO_FN_VIO_D2, GPIO_FN_VIO_D1, GPIO_FN_VIO_D0,
	GPIO_FN_VIO_FLD, GPIO_FN_VIO_CKO, GPIO_FN_VIO_STEX, GPIO_FN_VIO_STEM,
	GPIO_FN_VIO_VD, GPIO_FN_VIO_HD, GPIO_FN_VIO_CLK,
	GPIO_FN_VIO_VD2, GPIO_FN_VIO_HD2, GPIO_FN_VIO_CLK2,

	/* LCDC */
	GPIO_FN_LCDD23, GPIO_FN_LCDD22, GPIO_FN_LCDD21, GPIO_FN_LCDD20,
	GPIO_FN_LCDD19, GPIO_FN_LCDD18, GPIO_FN_LCDD17, GPIO_FN_LCDD16,
	GPIO_FN_LCDD15, GPIO_FN_LCDD14, GPIO_FN_LCDD13, GPIO_FN_LCDD12,
	GPIO_FN_LCDD11, GPIO_FN_LCDD10, GPIO_FN_LCDD9, GPIO_FN_LCDD8,
	GPIO_FN_LCDD7, GPIO_FN_LCDD6, GPIO_FN_LCDD5, GPIO_FN_LCDD4,
	GPIO_FN_LCDD3, GPIO_FN_LCDD2, GPIO_FN_LCDD1, GPIO_FN_LCDD0,
	GPIO_FN_LCDLCLK,
	/* Main LCD */
	GPIO_FN_LCDDON, GPIO_FN_LCDVCPWC, GPIO_FN_LCDVEPWC, GPIO_FN_LCDVSYN,
	/* Main LCD - RGB Mode */
	GPIO_FN_LCDDCK, GPIO_FN_LCDHSYN, GPIO_FN_LCDDISP,
	/* Main LCD - SYS Mode */
	GPIO_FN_LCDRS, GPIO_FN_LCDCS, GPIO_FN_LCDWR, GPIO_FN_LCDRD,
	/* Sub LCD - SYS Mode */
	GPIO_FN_LCDDON2, GPIO_FN_LCDVCPWC2, GPIO_FN_LCDVEPWC2,
	GPIO_FN_LCDVSYN2, GPIO_FN_LCDCS2,

	/* BSC */
	GPIO_FN_IOIS16, GPIO_FN_A25, GPIO_FN_A24, GPIO_FN_A23, GPIO_FN_A22,
	GPIO_FN_BS, GPIO_FN_CS6B_CE1B, GPIO_FN_WAIT, GPIO_FN_CS6A_CE2B,

	/* SBSC */
	GPIO_FN_HPD63, GPIO_FN_HPD62, GPIO_FN_HPD61, GPIO_FN_HPD60,
	GPIO_FN_HPD59, GPIO_FN_HPD58, GPIO_FN_HPD57, GPIO_FN_HPD56,
	GPIO_FN_HPD55, GPIO_FN_HPD54, GPIO_FN_HPD53, GPIO_FN_HPD52,
	GPIO_FN_HPD51, GPIO_FN_HPD50, GPIO_FN_HPD49, GPIO_FN_HPD48,
	GPIO_FN_HPDQM7, GPIO_FN_HPDQM6, GPIO_FN_HPDQM5, GPIO_FN_HPDQM4,

	/* IRQ */
	GPIO_FN_IRQ0, GPIO_FN_IRQ1, GPIO_FN_IRQ2, GPIO_FN_IRQ3,
	GPIO_FN_IRQ4, GPIO_FN_IRQ5, GPIO_FN_IRQ6, GPIO_FN_IRQ7,

	/* SDHI */
	GPIO_FN_SDHICD, GPIO_FN_SDHIWP, GPIO_FN_SDHID3, GPIO_FN_SDHID2,
	GPIO_FN_SDHID1, GPIO_FN_SDHID0, GPIO_FN_SDHICMD, GPIO_FN_SDHICLK,

	/* SIU - Port A */
	GPIO_FN_SIUAOLR, GPIO_FN_SIUAOBT, GPIO_FN_SIUAISLD, GPIO_FN_SIUAILR,
	GPIO_FN_SIUAIBT, GPIO_FN_SIUAOSLD, GPIO_FN_SIUMCKA, GPIO_FN_SIUFCKA,

	/* SIU - Port B */
	GPIO_FN_SIUBOLR, GPIO_FN_SIUBOBT, GPIO_FN_SIUBISLD, GPIO_FN_SIUBILR,
	GPIO_FN_SIUBIBT, GPIO_FN_SIUBOSLD, GPIO_FN_SIUMCKB, GPIO_FN_SIUFCKB,

	/* AUD */
	GPIO_FN_AUDSYNC, GPIO_FN_AUDATA3, GPIO_FN_AUDATA2, GPIO_FN_AUDATA1,
	GPIO_FN_AUDATA0,

	/* DMAC */
	GPIO_FN_DACK, GPIO_FN_DREQ0,

	/* VOU */
	GPIO_FN_DV_CLKI, GPIO_FN_DV_CLK, GPIO_FN_DV_HSYNC, GPIO_FN_DV_VSYNC,
	GPIO_FN_DV_D15, GPIO_FN_DV_D14, GPIO_FN_DV_D13, GPIO_FN_DV_D12,
	GPIO_FN_DV_D11, GPIO_FN_DV_D10, GPIO_FN_DV_D9, GPIO_FN_DV_D8,
	GPIO_FN_DV_D7, GPIO_FN_DV_D6, GPIO_FN_DV_D5, GPIO_FN_DV_D4,
	GPIO_FN_DV_D3, GPIO_FN_DV_D2, GPIO_FN_DV_D1, GPIO_FN_DV_D0,

	/* CPG */
	GPIO_FN_STATUS0, GPIO_FN_PDSTATUS,

	/* SIOF0 */
	GPIO_FN_SIOF0_MCK, GPIO_FN_SIOF0_SCK,
	GPIO_FN_SIOF0_SYNC, GPIO_FN_SIOF0_SS1, GPIO_FN_SIOF0_SS2,
	GPIO_FN_SIOF0_TXD, GPIO_FN_SIOF0_RXD,

	/* SIOF1 */
	GPIO_FN_SIOF1_MCK, GPIO_FN_SIOF1_SCK,
	GPIO_FN_SIOF1_SYNC, GPIO_FN_SIOF1_SS1, GPIO_FN_SIOF1_SS2,
	GPIO_FN_SIOF1_TXD, GPIO_FN_SIOF1_RXD,

	/* SIM */
	GPIO_FN_SIM_D, GPIO_FN_SIM_CLK, GPIO_FN_SIM_RST,

	/* TSIF */
	GPIO_FN_TS_SDAT, GPIO_FN_TS_SCK, GPIO_FN_TS_SDEN, GPIO_FN_TS_SPSYNC,

	/* IRDA */
	GPIO_FN_IRDA_IN, GPIO_FN_IRDA_OUT,

	/* TPU */
	GPIO_FN_TPUTO,

	/* FLCTL */
	GPIO_FN_FCE, GPIO_FN_NAF7, GPIO_FN_NAF6, GPIO_FN_NAF5, GPIO_FN_NAF4,
	GPIO_FN_NAF3, GPIO_FN_NAF2, GPIO_FN_NAF1, GPIO_FN_NAF0, GPIO_FN_FCDE,
	GPIO_FN_FOE, GPIO_FN_FSC, GPIO_FN_FWE, GPIO_FN_FRB,

	/* KEYSC */
	GPIO_FN_KEYIN0, GPIO_FN_KEYIN1, GPIO_FN_KEYIN2, GPIO_FN_KEYIN3,
	GPIO_FN_KEYIN4, GPIO_FN_KEYOUT0, GPIO_FN_KEYOUT1, GPIO_FN_KEYOUT2,
	GPIO_FN_KEYOUT3, GPIO_FN_KEYOUT4_IN6, GPIO_FN_KEYOUT5_IN5,
};

enum {
	HWBLK_UNKNOWN = 0,
	HWBLK_TLB, HWBLK_IC, HWBLK_OC, HWBLK_URAM, HWBLK_XYMEM,
	HWBLK_INTC, HWBLK_DMAC, HWBLK_SHYWAY, HWBLK_HUDI,
	HWBLK_UBC, HWBLK_TMU, HWBLK_CMT, HWBLK_RWDT, HWBLK_FLCTL,
	HWBLK_SCIF0, HWBLK_SCIF1, HWBLK_SCIF2, HWBLK_SIO,
	HWBLK_SIOF0, HWBLK_SIOF1, HWBLK_IIC, HWBLK_RTC,
	HWBLK_TPU, HWBLK_IRDA, HWBLK_SDHI, HWBLK_SIM, HWBLK_KEYSC,
	HWBLK_TSIF, HWBLK_USBF, HWBLK_2DG, HWBLK_SIU, HWBLK_VOU,
	HWBLK_JPU, HWBLK_BEU, HWBLK_CEU, HWBLK_VEU, HWBLK_VPU,
	HWBLK_LCDC,
	HWBLK_NR,
};

#endif /* __ASM_SH7722_H__ */
