/*
 * cfgdefs.h
 *
 * DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *
 * Global CFG constants and types, shared between DSP API and Bridge driver.
 *
 * Copyright (C) 2005-2006 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef CFGDEFS_
#define CFGDEFS_

/* Host Resources: */
#define CFG_MAXMEMREGISTERS     9

/* IRQ flag */
#define CFG_IRQSHARED           0x01	/* IRQ can be shared */

/* A platform-related device handle: */
struct cfg_devnode;

/*
 *  Host resource structure.
 */
struct cfg_hostres {
	u32 num_mem_windows;	/* Set to default */
	/* This is the base.memory */
	u32 dw_mem_base[CFG_MAXMEMREGISTERS];	/* shm virtual address */
	u32 dw_mem_length[CFG_MAXMEMREGISTERS];	/* Length of the Base */
	u32 dw_mem_phys[CFG_MAXMEMREGISTERS];	/* shm Physical address */
	u8 birq_registers;	/* IRQ Number */
	u8 birq_attrib;		/* IRQ Attribute */
	u32 dw_offset_for_monitor;	/* The Shared memory starts from
					 * dw_mem_base + this offset */
	/*
	 *  Info needed by NODE for allocating channels to communicate with RMS:
	 *      dw_chnl_offset:       Offset of RMS channels. Lower channels are
	 *                          reserved.
	 *      dw_chnl_buf_size:      Size of channel buffer to send to RMS
	 *      dw_num_chnls:		Total number of channels
	 *      			(including reserved).
	 */
	u32 dw_chnl_offset;
	u32 dw_chnl_buf_size;
	u32 dw_num_chnls;
	void __iomem *dw_per_base;
	u32 dw_per_pm_base;
	u32 dw_core_pm_base;
	void __iomem *dw_dmmu_base;
};

#endif /* CFGDEFS_ */
