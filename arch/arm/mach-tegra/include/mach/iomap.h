/*
 * arch/arm/mach-tegra/include/mach/iomap.h
 *
 * Copyright (C) 2010 Google, Inc.
 *
 * Author:
 *	Colin Cross <ccross@google.com>
 *	Erik Gilling <konkers@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MACH_TEGRA_IOMAP_H
#define __MACH_TEGRA_IOMAP_H

#include <asm/sizes.h>

#define TEGRA_IRAM_BASE			0x40000000
#define TEGRA_IRAM_SIZE			SZ_256K

#define TEGRA_ARM_PERIF_BASE		0x50040000
#define TEGRA_ARM_PERIF_SIZE		SZ_8K

#define TEGRA_ARM_PL310_BASE		0x50043000
#define TEGRA_ARM_PL310_SIZE		SZ_4K

#define TEGRA_ARM_INT_DIST_BASE		0x50041000
#define TEGRA_ARM_INT_DIST_SIZE		SZ_4K

#define TEGRA_DISPLAY_BASE		0x54200000
#define TEGRA_DISPLAY_SIZE		SZ_256K

#define TEGRA_DISPLAY2_BASE		0x54240000
#define TEGRA_DISPLAY2_SIZE		SZ_256K

#define TEGRA_PRIMARY_ICTLR_BASE	0x60004000
#define TEGRA_PRIMARY_ICTLR_SIZE	SZ_64

#define TEGRA_SECONDARY_ICTLR_BASE	0x60004100
#define TEGRA_SECONDARY_ICTLR_SIZE	SZ_64

#define TEGRA_TERTIARY_ICTLR_BASE	0x60004200
#define TEGRA_TERTIARY_ICTLR_SIZE	SZ_64

#define TEGRA_QUATERNARY_ICTLR_BASE	0x60004300
#define TEGRA_QUATERNARY_ICTLR_SIZE	SZ_64

#define TEGRA_TMR1_BASE			0x60005000
#define TEGRA_TMR1_SIZE			SZ_8

#define TEGRA_TMR2_BASE			0x60005008
#define TEGRA_TMR2_SIZE			SZ_8

#define TEGRA_TMRUS_BASE		0x60005010
#define TEGRA_TMRUS_SIZE		SZ_64

#define TEGRA_TMR3_BASE			0x60005050
#define TEGRA_TMR3_SIZE			SZ_8

#define TEGRA_TMR4_BASE			0x60005058
#define TEGRA_TMR4_SIZE			SZ_8

#define TEGRA_CLK_RESET_BASE		0x60006000
#define TEGRA_CLK_RESET_SIZE		SZ_4K

#define TEGRA_FLOW_CTRL_BASE		0x60007000
#define TEGRA_FLOW_CTRL_SIZE		20

#define TEGRA_AHB_DMA_BASE		0x60008000
#define TEGRA_AHB_DMA_SIZE		SZ_4K

#define TEGRA_AHB_DMA_CH0_BASE		0x60009000
#define TEGRA_AHB_DMA_CH0_SIZE		32

#define TEGRA_APB_DMA_BASE		0x6000A000
#define TEGRA_APB_DMA_SIZE		SZ_4K

#define TEGRA_APB_DMA_CH0_BASE		0x6000B000
#define TEGRA_APB_DMA_CH0_SIZE		32

#define TEGRA_AHB_GIZMO_BASE		0x6000C004
#define TEGRA_AHB_GIZMO_SIZE		0x10C

#define TEGRA_STATMON_BASE		0x6000C400
#define TEGRA_STATMON_SIZE		SZ_1K

#define TEGRA_GPIO_BASE			0x6000D000
#define TEGRA_GPIO_SIZE			SZ_4K

#define TEGRA_EXCEPTION_VECTORS_BASE    0x6000F000
#define TEGRA_EXCEPTION_VECTORS_SIZE    SZ_4K

#define TEGRA_APB_MISC_BASE		0x70000000
#define TEGRA_APB_MISC_SIZE		SZ_4K

#define TEGRA_AC97_BASE			0x70002000
#define TEGRA_AC97_SIZE			SZ_512

#define TEGRA_SPDIF_BASE		0x70002400
#define TEGRA_SPDIF_SIZE		SZ_512

#define TEGRA_I2S1_BASE			0x70002800
#define TEGRA_I2S1_SIZE			SZ_256

#define TEGRA_I2S2_BASE			0x70002A00
#define TEGRA_I2S2_SIZE			SZ_256

#define TEGRA_UARTA_BASE		0x70006000
#define TEGRA_UARTA_SIZE		SZ_64

#define TEGRA_UARTB_BASE		0x70006040
#define TEGRA_UARTB_SIZE		SZ_64

#define TEGRA_UARTC_BASE		0x70006200
#define TEGRA_UARTC_SIZE		SZ_256

#define TEGRA_UARTD_BASE		0x70006300
#define TEGRA_UARTD_SIZE		SZ_256

#define TEGRA_UARTE_BASE		0x70006400
#define TEGRA_UARTE_SIZE		SZ_256

#define TEGRA_NAND_BASE			0x70008000
#define TEGRA_NAND_SIZE			SZ_256

#define TEGRA_HSMMC_BASE		0x70008500
#define TEGRA_HSMMC_SIZE		SZ_256

#define TEGRA_SNOR_BASE			0x70009000
#define TEGRA_SNOR_SIZE			SZ_4K

#define TEGRA_PWFM_BASE			0x7000A000
#define TEGRA_PWFM_SIZE			SZ_256

#define TEGRA_MIPI_BASE			0x7000B000
#define TEGRA_MIPI_SIZE			SZ_256

#define TEGRA_I2C_BASE			0x7000C000
#define TEGRA_I2C_SIZE			SZ_256

#define TEGRA_TWC_BASE			0x7000C100
#define TEGRA_TWC_SIZE			SZ_256

#define TEGRA_SPI_BASE			0x7000C380
#define TEGRA_SPI_SIZE			48

#define TEGRA_I2C2_BASE			0x7000C400
#define TEGRA_I2C2_SIZE			SZ_256

#define TEGRA_I2C3_BASE			0x7000C500
#define TEGRA_I2C3_SIZE			SZ_256

#define TEGRA_OWR_BASE			0x7000C600
#define TEGRA_OWR_SIZE			80

#define TEGRA_DVC_BASE			0x7000D000
#define TEGRA_DVC_SIZE			SZ_512

#define TEGRA_SPI1_BASE			0x7000D400
#define TEGRA_SPI1_SIZE			SZ_512

#define TEGRA_SPI2_BASE			0x7000D600
#define TEGRA_SPI2_SIZE			SZ_512

#define TEGRA_SPI3_BASE			0x7000D800
#define TEGRA_SPI3_SIZE			SZ_512

#define TEGRA_SPI4_BASE			0x7000DA00
#define TEGRA_SPI4_SIZE			SZ_512

#define TEGRA_RTC_BASE			0x7000E000
#define TEGRA_RTC_SIZE			SZ_256

#define TEGRA_KBC_BASE			0x7000E200
#define TEGRA_KBC_SIZE			SZ_256

#define TEGRA_PMC_BASE			0x7000E400
#define TEGRA_PMC_SIZE			SZ_256

#define TEGRA_MC_BASE			0x7000F000
#define TEGRA_MC_SIZE			SZ_1K

#define TEGRA_EMC_BASE			0x7000F400
#define TEGRA_EMC_SIZE			SZ_1K

#define TEGRA_FUSE_BASE			0x7000F800
#define TEGRA_FUSE_SIZE			SZ_1K

#define TEGRA_KFUSE_BASE		0x7000FC00
#define TEGRA_KFUSE_SIZE		SZ_1K

#define TEGRA_CSITE_BASE		0x70040000
#define TEGRA_CSITE_SIZE		SZ_256K

#define TEGRA_USB_BASE			0xC5000000
#define TEGRA_USB_SIZE			SZ_16K

#define TEGRA_USB2_BASE			0xC5004000
#define TEGRA_USB2_SIZE			SZ_16K

#define TEGRA_USB3_BASE			0xC5008000
#define TEGRA_USB3_SIZE			SZ_16K

#define TEGRA_SDMMC1_BASE		0xC8000000
#define TEGRA_SDMMC1_SIZE		SZ_512

#define TEGRA_SDMMC2_BASE		0xC8000200
#define TEGRA_SDMMC2_SIZE		SZ_512

#define TEGRA_SDMMC3_BASE		0xC8000400
#define TEGRA_SDMMC3_SIZE		SZ_512

#define TEGRA_SDMMC4_BASE		0xC8000600
#define TEGRA_SDMMC4_SIZE		SZ_512

#endif
