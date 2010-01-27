/*
 * arch/arm/plat-omap/include/mach/clockdomain.h
 *
 * OMAP2/3 clockdomain framework functions
 *
 * Copyright (C) 2008 Texas Instruments, Inc.
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Written by Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARM_ARCH_OMAP_CLOCKDOMAIN_H
#define __ASM_ARM_ARCH_OMAP_CLOCKDOMAIN_H

#include <plat/powerdomain.h>
#include <plat/clock.h>
#include <plat/cpu.h>

/* Clockdomain capability flags */
#define CLKDM_CAN_FORCE_SLEEP			(1 << 0)
#define CLKDM_CAN_FORCE_WAKEUP			(1 << 1)
#define CLKDM_CAN_ENABLE_AUTO			(1 << 2)
#define CLKDM_CAN_DISABLE_AUTO			(1 << 3)

#define CLKDM_CAN_HWSUP		(CLKDM_CAN_ENABLE_AUTO | CLKDM_CAN_DISABLE_AUTO)
#define CLKDM_CAN_SWSUP		(CLKDM_CAN_FORCE_SLEEP | CLKDM_CAN_FORCE_WAKEUP)
#define CLKDM_CAN_HWSUP_SWSUP	(CLKDM_CAN_SWSUP | CLKDM_CAN_HWSUP)

/* OMAP24XX CM_CLKSTCTRL_*.AUTOSTATE_* register bit values */
#define OMAP24XX_CLKSTCTRL_DISABLE_AUTO		0x0
#define OMAP24XX_CLKSTCTRL_ENABLE_AUTO		0x1

/* OMAP3XXX CM_CLKSTCTRL_*.CLKTRCTRL_* register bit values */
#define OMAP34XX_CLKSTCTRL_DISABLE_AUTO		0x0
#define OMAP34XX_CLKSTCTRL_FORCE_SLEEP		0x1
#define OMAP34XX_CLKSTCTRL_FORCE_WAKEUP		0x2
#define OMAP34XX_CLKSTCTRL_ENABLE_AUTO		0x3

/*
 * struct clkdm_autodep - a clockdomain that should have wkdeps
 * and sleepdeps added when a clockdomain should stay active in hwsup mode;
 * and conversely, removed when the clockdomain should be allowed to go
 * inactive in hwsup mode.
 */
struct clkdm_autodep {

	union {
		/* Name of the clockdomain to add a wkdep/sleepdep on */
		const char *name;

		/* Clockdomain pointer (looked up at clkdm_init() time) */
		struct clockdomain *ptr;
	} clkdm;

	/* OMAP chip types that this clockdomain dep is valid on */
	const struct omap_chip_id omap_chip;

};

/* Encodes dependencies between clockdomains - statically defined */
struct clkdm_dep {

	/* Clockdomain name */
	const char *clkdm_name;

	/* Clockdomain pointer - resolved by the clockdomain code */
	struct clockdomain *clkdm;

	/* Flags to mark OMAP chip restrictions, etc. */
	const struct omap_chip_id omap_chip;

};

struct clockdomain {

	/* Clockdomain name */
	const char *name;

	union {
		/* Powerdomain enclosing this clockdomain */
		const char *name;

		/* Powerdomain pointer assigned at clkdm_register() */
		struct powerdomain *ptr;
	} pwrdm;

	/* CLKSTCTRL reg for the given clock domain*/
	void __iomem *clkstctrl_reg;

	/* CLKTRCTRL/AUTOSTATE field mask in CM_CLKSTCTRL reg */
	const u16 clktrctrl_mask;

	/* Clockdomain capability flags */
	const u8 flags;

	/* Bit shift of this clockdomain's PM_WKDEP/CM_SLEEPDEP bit */
	const u8 dep_bit;

	/* Clockdomains that can be told to wake this powerdomain up */
	struct clkdm_dep *wkdep_srcs;

	/* Clockdomains that can be told to keep this clkdm from inactivity */
	struct clkdm_dep *sleepdep_srcs;

	/* OMAP chip types that this clockdomain is valid on */
	const struct omap_chip_id omap_chip;

	/* Usecount tracking */
	atomic_t usecount;

	struct list_head node;

};

void clkdm_init(struct clockdomain **clkdms, struct clkdm_autodep *autodeps);
struct clockdomain *clkdm_lookup(const char *name);

int clkdm_for_each(int (*fn)(struct clockdomain *clkdm, void *user),
			void *user);
struct powerdomain *clkdm_get_pwrdm(struct clockdomain *clkdm);

int clkdm_add_wkdep(struct clockdomain *clkdm1, struct clockdomain *clkdm2);
int clkdm_del_wkdep(struct clockdomain *clkdm1, struct clockdomain *clkdm2);
int clkdm_read_wkdep(struct clockdomain *clkdm1, struct clockdomain *clkdm2);
int clkdm_add_sleepdep(struct clockdomain *clkdm1, struct clockdomain *clkdm2);
int clkdm_del_sleepdep(struct clockdomain *clkdm1, struct clockdomain *clkdm2);
int clkdm_read_sleepdep(struct clockdomain *clkdm1, struct clockdomain *clkdm2);

void omap2_clkdm_allow_idle(struct clockdomain *clkdm);
void omap2_clkdm_deny_idle(struct clockdomain *clkdm);

int omap2_clkdm_wakeup(struct clockdomain *clkdm);
int omap2_clkdm_sleep(struct clockdomain *clkdm);

int omap2_clkdm_clk_enable(struct clockdomain *clkdm, struct clk *clk);
int omap2_clkdm_clk_disable(struct clockdomain *clkdm, struct clk *clk);

#endif
