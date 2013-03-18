/*
 * arch/arm/mach-prima2/include/mach/irqs.h
 *
 * Copyright (c) 2011 Cambridge Silicon Radio Limited, a CSR plc group company.
 *
 * Licensed under GPLv2 or later.
 */

#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H

#define SIRFSOC_INTENAL_IRQ_START  0
#define SIRFSOC_INTENAL_IRQ_END    127
#define SIRFSOC_GPIO_IRQ_START     (SIRFSOC_INTENAL_IRQ_END + 1)
#define NR_IRQS	288

#endif
