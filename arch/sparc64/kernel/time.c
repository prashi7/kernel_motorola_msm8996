/* time.c: UltraSparc timer and TOD clock support.
 *
 * Copyright (C) 1997, 2008 David S. Miller (davem@davemloft.net)
 * Copyright (C) 1998 Eddie C. Dost   (ecd@skynet.be)
 *
 * Based largely on code which is:
 *
 * Copyright (C) 1996 Thomas K. Dyas (tdyas@eden.rutgers.edu)
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/smp_lock.h>
#include <linux/kernel.h>
#include <linux/param.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/mc146818rtc.h>
#include <linux/delay.h>
#include <linux/profile.h>
#include <linux/bcd.h>
#include <linux/jiffies.h>
#include <linux/cpufreq.h>
#include <linux/percpu.h>
#include <linux/miscdevice.h>
#include <linux/rtc.h>
#include <linux/rtc/m48t59.h>
#include <linux/kernel_stat.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#include <asm/oplib.h>
#include <asm/timer.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/prom.h>
#include <asm/starfire.h>
#include <asm/smp.h>
#include <asm/sections.h>
#include <asm/cpudata.h>
#include <asm/uaccess.h>
#include <asm/irq_regs.h>

#include "entry.h"

DEFINE_SPINLOCK(rtc_lock);
static void __iomem *bq4802_regs;

static int set_rtc_mmss(unsigned long);

#define TICK_PRIV_BIT	(1UL << 63)
#define TICKCMP_IRQ_BIT	(1UL << 63)

#ifdef CONFIG_SMP
unsigned long profile_pc(struct pt_regs *regs)
{
	unsigned long pc = instruction_pointer(regs);

	if (in_lock_functions(pc))
		return regs->u_regs[UREG_RETPC];
	return pc;
}
EXPORT_SYMBOL(profile_pc);
#endif

static void tick_disable_protection(void)
{
	/* Set things up so user can access tick register for profiling
	 * purposes.  Also workaround BB_ERRATA_1 by doing a dummy
	 * read back of %tick after writing it.
	 */
	__asm__ __volatile__(
	"	ba,pt	%%xcc, 1f\n"
	"	 nop\n"
	"	.align	64\n"
	"1:	rd	%%tick, %%g2\n"
	"	add	%%g2, 6, %%g2\n"
	"	andn	%%g2, %0, %%g2\n"
	"	wrpr	%%g2, 0, %%tick\n"
	"	rdpr	%%tick, %%g0"
	: /* no outputs */
	: "r" (TICK_PRIV_BIT)
	: "g2");
}

static void tick_disable_irq(void)
{
	__asm__ __volatile__(
	"	ba,pt	%%xcc, 1f\n"
	"	 nop\n"
	"	.align	64\n"
	"1:	wr	%0, 0x0, %%tick_cmpr\n"
	"	rd	%%tick_cmpr, %%g0"
	: /* no outputs */
	: "r" (TICKCMP_IRQ_BIT));
}

static void tick_init_tick(void)
{
	tick_disable_protection();
	tick_disable_irq();
}

static unsigned long tick_get_tick(void)
{
	unsigned long ret;

	__asm__ __volatile__("rd	%%tick, %0\n\t"
			     "mov	%0, %0"
			     : "=r" (ret));

	return ret & ~TICK_PRIV_BIT;
}

static int tick_add_compare(unsigned long adj)
{
	unsigned long orig_tick, new_tick, new_compare;

	__asm__ __volatile__("rd	%%tick, %0"
			     : "=r" (orig_tick));

	orig_tick &= ~TICKCMP_IRQ_BIT;

	/* Workaround for Spitfire Errata (#54 I think??), I discovered
	 * this via Sun BugID 4008234, mentioned in Solaris-2.5.1 patch
	 * number 103640.
	 *
	 * On Blackbird writes to %tick_cmpr can fail, the
	 * workaround seems to be to execute the wr instruction
	 * at the start of an I-cache line, and perform a dummy
	 * read back from %tick_cmpr right after writing to it. -DaveM
	 */
	__asm__ __volatile__("ba,pt	%%xcc, 1f\n\t"
			     " add	%1, %2, %0\n\t"
			     ".align	64\n"
			     "1:\n\t"
			     "wr	%0, 0, %%tick_cmpr\n\t"
			     "rd	%%tick_cmpr, %%g0\n\t"
			     : "=r" (new_compare)
			     : "r" (orig_tick), "r" (adj));

	__asm__ __volatile__("rd	%%tick, %0"
			     : "=r" (new_tick));
	new_tick &= ~TICKCMP_IRQ_BIT;

	return ((long)(new_tick - (orig_tick+adj))) > 0L;
}

static unsigned long tick_add_tick(unsigned long adj)
{
	unsigned long new_tick;

	/* Also need to handle Blackbird bug here too. */
	__asm__ __volatile__("rd	%%tick, %0\n\t"
			     "add	%0, %1, %0\n\t"
			     "wrpr	%0, 0, %%tick\n\t"
			     : "=&r" (new_tick)
			     : "r" (adj));

	return new_tick;
}

static struct sparc64_tick_ops tick_operations __read_mostly = {
	.name		=	"tick",
	.init_tick	=	tick_init_tick,
	.disable_irq	=	tick_disable_irq,
	.get_tick	=	tick_get_tick,
	.add_tick	=	tick_add_tick,
	.add_compare	=	tick_add_compare,
	.softint_mask	=	1UL << 0,
};

struct sparc64_tick_ops *tick_ops __read_mostly = &tick_operations;

static void stick_disable_irq(void)
{
	__asm__ __volatile__(
	"wr	%0, 0x0, %%asr25"
	: /* no outputs */
	: "r" (TICKCMP_IRQ_BIT));
}

static void stick_init_tick(void)
{
	/* Writes to the %tick and %stick register are not
	 * allowed on sun4v.  The Hypervisor controls that
	 * bit, per-strand.
	 */
	if (tlb_type != hypervisor) {
		tick_disable_protection();
		tick_disable_irq();

		/* Let the user get at STICK too. */
		__asm__ __volatile__(
		"	rd	%%asr24, %%g2\n"
		"	andn	%%g2, %0, %%g2\n"
		"	wr	%%g2, 0, %%asr24"
		: /* no outputs */
		: "r" (TICK_PRIV_BIT)
		: "g1", "g2");
	}

	stick_disable_irq();
}

static unsigned long stick_get_tick(void)
{
	unsigned long ret;

	__asm__ __volatile__("rd	%%asr24, %0"
			     : "=r" (ret));

	return ret & ~TICK_PRIV_BIT;
}

static unsigned long stick_add_tick(unsigned long adj)
{
	unsigned long new_tick;

	__asm__ __volatile__("rd	%%asr24, %0\n\t"
			     "add	%0, %1, %0\n\t"
			     "wr	%0, 0, %%asr24\n\t"
			     : "=&r" (new_tick)
			     : "r" (adj));

	return new_tick;
}

static int stick_add_compare(unsigned long adj)
{
	unsigned long orig_tick, new_tick;

	__asm__ __volatile__("rd	%%asr24, %0"
			     : "=r" (orig_tick));
	orig_tick &= ~TICKCMP_IRQ_BIT;

	__asm__ __volatile__("wr	%0, 0, %%asr25"
			     : /* no outputs */
			     : "r" (orig_tick + adj));

	__asm__ __volatile__("rd	%%asr24, %0"
			     : "=r" (new_tick));
	new_tick &= ~TICKCMP_IRQ_BIT;

	return ((long)(new_tick - (orig_tick+adj))) > 0L;
}

static struct sparc64_tick_ops stick_operations __read_mostly = {
	.name		=	"stick",
	.init_tick	=	stick_init_tick,
	.disable_irq	=	stick_disable_irq,
	.get_tick	=	stick_get_tick,
	.add_tick	=	stick_add_tick,
	.add_compare	=	stick_add_compare,
	.softint_mask	=	1UL << 16,
};

/* On Hummingbird the STICK/STICK_CMPR register is implemented
 * in I/O space.  There are two 64-bit registers each, the
 * first holds the low 32-bits of the value and the second holds
 * the high 32-bits.
 *
 * Since STICK is constantly updating, we have to access it carefully.
 *
 * The sequence we use to read is:
 * 1) read high
 * 2) read low
 * 3) read high again, if it rolled re-read both low and high again.
 *
 * Writing STICK safely is also tricky:
 * 1) write low to zero
 * 2) write high
 * 3) write low
 */
#define HBIRD_STICKCMP_ADDR	0x1fe0000f060UL
#define HBIRD_STICK_ADDR	0x1fe0000f070UL

static unsigned long __hbird_read_stick(void)
{
	unsigned long ret, tmp1, tmp2, tmp3;
	unsigned long addr = HBIRD_STICK_ADDR+8;

	__asm__ __volatile__("ldxa	[%1] %5, %2\n"
			     "1:\n\t"
			     "sub	%1, 0x8, %1\n\t"
			     "ldxa	[%1] %5, %3\n\t"
			     "add	%1, 0x8, %1\n\t"
			     "ldxa	[%1] %5, %4\n\t"
			     "cmp	%4, %2\n\t"
			     "bne,a,pn	%%xcc, 1b\n\t"
			     " mov	%4, %2\n\t"
			     "sllx	%4, 32, %4\n\t"
			     "or	%3, %4, %0\n\t"
			     : "=&r" (ret), "=&r" (addr),
			       "=&r" (tmp1), "=&r" (tmp2), "=&r" (tmp3)
			     : "i" (ASI_PHYS_BYPASS_EC_E), "1" (addr));

	return ret;
}

static void __hbird_write_stick(unsigned long val)
{
	unsigned long low = (val & 0xffffffffUL);
	unsigned long high = (val >> 32UL);
	unsigned long addr = HBIRD_STICK_ADDR;

	__asm__ __volatile__("stxa	%%g0, [%0] %4\n\t"
			     "add	%0, 0x8, %0\n\t"
			     "stxa	%3, [%0] %4\n\t"
			     "sub	%0, 0x8, %0\n\t"
			     "stxa	%2, [%0] %4"
			     : "=&r" (addr)
			     : "0" (addr), "r" (low), "r" (high),
			       "i" (ASI_PHYS_BYPASS_EC_E));
}

static void __hbird_write_compare(unsigned long val)
{
	unsigned long low = (val & 0xffffffffUL);
	unsigned long high = (val >> 32UL);
	unsigned long addr = HBIRD_STICKCMP_ADDR + 0x8UL;

	__asm__ __volatile__("stxa	%3, [%0] %4\n\t"
			     "sub	%0, 0x8, %0\n\t"
			     "stxa	%2, [%0] %4"
			     : "=&r" (addr)
			     : "0" (addr), "r" (low), "r" (high),
			       "i" (ASI_PHYS_BYPASS_EC_E));
}

static void hbtick_disable_irq(void)
{
	__hbird_write_compare(TICKCMP_IRQ_BIT);
}

static void hbtick_init_tick(void)
{
	tick_disable_protection();

	/* XXX This seems to be necessary to 'jumpstart' Hummingbird
	 * XXX into actually sending STICK interrupts.  I think because
	 * XXX of how we store %tick_cmpr in head.S this somehow resets the
	 * XXX {TICK + STICK} interrupt mux.  -DaveM
	 */
	__hbird_write_stick(__hbird_read_stick());

	hbtick_disable_irq();
}

static unsigned long hbtick_get_tick(void)
{
	return __hbird_read_stick() & ~TICK_PRIV_BIT;
}

static unsigned long hbtick_add_tick(unsigned long adj)
{
	unsigned long val;

	val = __hbird_read_stick() + adj;
	__hbird_write_stick(val);

	return val;
}

static int hbtick_add_compare(unsigned long adj)
{
	unsigned long val = __hbird_read_stick();
	unsigned long val2;

	val &= ~TICKCMP_IRQ_BIT;
	val += adj;
	__hbird_write_compare(val);

	val2 = __hbird_read_stick() & ~TICKCMP_IRQ_BIT;

	return ((long)(val2 - val)) > 0L;
}

static struct sparc64_tick_ops hbtick_operations __read_mostly = {
	.name		=	"hbtick",
	.init_tick	=	hbtick_init_tick,
	.disable_irq	=	hbtick_disable_irq,
	.get_tick	=	hbtick_get_tick,
	.add_tick	=	hbtick_add_tick,
	.add_compare	=	hbtick_add_compare,
	.softint_mask	=	1UL << 0,
};

static unsigned long timer_ticks_per_nsec_quotient __read_mostly;

int update_persistent_clock(struct timespec now)
{
	struct rtc_device *rtc = rtc_class_open("rtc0");

	if (rtc)
		return rtc_set_mmss(rtc, now.tv_sec);

	return set_rtc_mmss(now.tv_sec);
}

/* Probe for the real time clock chip. */
static void __init set_system_time(void)
{
	unsigned int year, mon, day, hour, min, sec;
	void __iomem *bregs = bq4802_regs;
	unsigned char val = readb(bregs + 0x0e);
	unsigned int century;

	if (!bregs) {
		prom_printf("Something wrong, clock regs not mapped yet.\n");
		prom_halt();
	}		

	/* BQ4802 RTC chip. */

	writeb(val | 0x08, bregs + 0x0e);

	sec  = readb(bregs + 0x00);
	min  = readb(bregs + 0x02);
	hour = readb(bregs + 0x04);
	day  = readb(bregs + 0x06);
	mon  = readb(bregs + 0x09);
	year = readb(bregs + 0x0a);
	century = readb(bregs + 0x0f);

	writeb(val, bregs + 0x0e);

	BCD_TO_BIN(sec);
	BCD_TO_BIN(min);
	BCD_TO_BIN(hour);
	BCD_TO_BIN(day);
	BCD_TO_BIN(mon);
	BCD_TO_BIN(year);
	BCD_TO_BIN(century);

	year += (century * 100);

	xtime.tv_sec = mktime(year, mon, day, hour, min, sec);
	xtime.tv_nsec = (INITIAL_JIFFIES % HZ) * (NSEC_PER_SEC / HZ);
	set_normalized_timespec(&wall_to_monotonic,
 	                        -xtime.tv_sec, -xtime.tv_nsec);
}

/* davem suggests we keep this within the 4M locked kernel image */
static u32 starfire_get_time(void)
{
	static char obp_gettod[32];
	static u32 unix_tod;

	sprintf(obp_gettod, "h# %08x unix-gettod",
		(unsigned int) (long) &unix_tod);
	prom_feval(obp_gettod);

	return unix_tod;
}

static int starfire_set_time(u32 val)
{
	/* Do nothing, time is set using the service processor
	 * console on this platform.
	 */
	return 0;
}

static u32 hypervisor_get_time(void)
{
	unsigned long ret, time;
	int retries = 10000;

retry:
	ret = sun4v_tod_get(&time);
	if (ret == HV_EOK)
		return time;
	if (ret == HV_EWOULDBLOCK) {
		if (--retries > 0) {
			udelay(100);
			goto retry;
		}
		printk(KERN_WARNING "SUN4V: tod_get() timed out.\n");
		return 0;
	}
	printk(KERN_WARNING "SUN4V: tod_get() not supported.\n");
	return 0;
}

static int hypervisor_set_time(u32 secs)
{
	unsigned long ret;
	int retries = 10000;

retry:
	ret = sun4v_tod_set(secs);
	if (ret == HV_EOK)
		return 0;
	if (ret == HV_EWOULDBLOCK) {
		if (--retries > 0) {
			udelay(100);
			goto retry;
		}
		printk(KERN_WARNING "SUN4V: tod_set() timed out.\n");
		return -EAGAIN;
	}
	printk(KERN_WARNING "SUN4V: tod_set() not supported.\n");
	return -EOPNOTSUPP;
}

unsigned long cmos_regs;
EXPORT_SYMBOL(cmos_regs);

struct resource rtc_cmos_resource;

static struct platform_device rtc_cmos_device = {
	.name		= "rtc_cmos",
	.id		= -1,
	.resource	= &rtc_cmos_resource,
	.num_resources	= 1,
};

static int __devinit rtc_probe(struct of_device *op, const struct of_device_id *match)
{
	struct resource *r;

	printk(KERN_INFO "%s: RTC regs at 0x%lx\n",
	       op->node->full_name, op->resource[0].start);

	/* The CMOS RTC driver only accepts IORESOURCE_IO, so cons
	 * up a fake resource so that the probe works for all cases.
	 * When the RTC is behind an ISA bus it will have IORESOURCE_IO
	 * already, whereas when it's behind EBUS is will be IORESOURCE_MEM.
	 */

	r = &rtc_cmos_resource;
	r->flags = IORESOURCE_IO;
	r->name = op->resource[0].name;
	r->start = op->resource[0].start;
	r->end = op->resource[0].end;

	cmos_regs = op->resource[0].start;
	return platform_device_register(&rtc_cmos_device);
}

static struct of_device_id rtc_match[] = {
	{
		.name = "rtc",
		.compatible = "m5819",
	},
	{
		.name = "rtc",
		.compatible = "isa-m5819p",
	},
	{
		.name = "rtc",
		.compatible = "isa-m5823p",
	},
	{
		.name = "rtc",
		.compatible = "ds1287",
	},
	{},
};

static struct of_platform_driver rtc_driver = {
	.match_table	= rtc_match,
	.probe		= rtc_probe,
	.driver		= {
		.name	= "rtc",
	},
};

static int __devinit bq4802_probe(struct of_device *op, const struct of_device_id *match)
{
	struct device_node *dp = op->node;
	unsigned long flags;

	bq4802_regs = of_ioremap(&op->resource[0], 0, resource_size(&op->resource[0]), "bq4802");
	if (!bq4802_regs)
		return -ENOMEM;

	printk(KERN_INFO "%s: Clock regs at %p\n", dp->full_name, bq4802_regs);

	local_irq_save(flags);

	set_system_time();
	
	local_irq_restore(flags);

	return 0;
}

static struct of_device_id bq4802_match[] = {
	{
		.name = "rtc",
		.compatible = "bq4802",
	},
};

static struct of_platform_driver bq4802_driver = {
	.match_table	= bq4802_match,
	.probe		= bq4802_probe,
	.driver		= {
		.name	= "bq4802",
	},
};

static unsigned char mostek_read_byte(struct device *dev, u32 ofs)
{
	struct platform_device *pdev = to_platform_device(dev);
	void __iomem *regs;
	unsigned char val;

	regs = (void __iomem *) pdev->resource[0].start;
	val = readb(regs + ofs);

	/* the year 0 is 1968 */
	if (ofs == M48T59_YEAR) {
		val += 0x68;
		if ((val & 0xf) > 9)
			val += 6;
	}
	return val;
}

static void mostek_write_byte(struct device *dev, u32 ofs, u8 val)
{
	struct platform_device *pdev = to_platform_device(dev);
	void __iomem *regs;

	regs = (void __iomem *) pdev->resource[0].start;
	if (ofs == M48T59_YEAR) {
		if (val < 0x68)
			val += 0x32;
		else
			val -= 0x68;
		if ((val & 0xf) > 9)
			val += 6;
		if ((val & 0xf0) > 0x9A)
			val += 0x60;
	}
	writeb(val, regs + ofs);
}

static struct m48t59_plat_data m48t59_data = {
	.read_byte	= mostek_read_byte,
	.write_byte	= mostek_write_byte,
};

static struct platform_device m48t59_rtc = {
	.name		= "rtc-m48t59",
	.id		= 0,
	.num_resources	= 1,
	.dev	= {
		.platform_data = &m48t59_data,
	},
};

static int __devinit mostek_probe(struct of_device *op, const struct of_device_id *match)
{
	struct device_node *dp = op->node;

	/* On an Enterprise system there can be multiple mostek clocks.
	 * We should only match the one that is on the central FHC bus.
	 */
	if (!strcmp(dp->parent->name, "fhc") &&
	    strcmp(dp->parent->parent->name, "central") != 0)
		return -ENODEV;

	printk(KERN_INFO "%s: Mostek regs at 0x%lx\n",
	       dp->full_name, op->resource[0].start);

	m48t59_rtc.resource = &op->resource[0];
	return platform_device_register(&m48t59_rtc);
}

static struct of_device_id mostek_match[] = {
	{
		.name = "eeprom",
	},
	{},
};

static struct of_platform_driver mostek_driver = {
	.match_table	= mostek_match,
	.probe		= mostek_probe,
	.driver		= {
		.name	= "mostek",
	},
};

static int __init clock_init(void)
{
	if (this_is_starfire) {
		xtime.tv_sec = starfire_get_time();
		xtime.tv_nsec = (INITIAL_JIFFIES % HZ) * (NSEC_PER_SEC / HZ);
		set_normalized_timespec(&wall_to_monotonic,
		                        -xtime.tv_sec, -xtime.tv_nsec);
		return 0;
	}
	if (tlb_type == hypervisor) {
		xtime.tv_sec = hypervisor_get_time();
		xtime.tv_nsec = (INITIAL_JIFFIES % HZ) * (NSEC_PER_SEC / HZ);
		set_normalized_timespec(&wall_to_monotonic,
		                        -xtime.tv_sec, -xtime.tv_nsec);
		return 0;
	}

	(void) of_register_driver(&rtc_driver, &of_platform_bus_type);
	(void) of_register_driver(&mostek_driver, &of_platform_bus_type);
	(void) of_register_driver(&bq4802_driver, &of_platform_bus_type);

	return 0;
}

/* Must be after subsys_initcall() so that busses are probed.  Must
 * be before device_initcall() because things like the RTC driver
 * need to see the clock registers.
 */
fs_initcall(clock_init);

/* This is gets the master TICK_INT timer going. */
static unsigned long sparc64_init_timers(void)
{
	struct device_node *dp;
	unsigned long clock;

	dp = of_find_node_by_path("/");
	if (tlb_type == spitfire) {
		unsigned long ver, manuf, impl;

		__asm__ __volatile__ ("rdpr %%ver, %0"
				      : "=&r" (ver));
		manuf = ((ver >> 48) & 0xffff);
		impl = ((ver >> 32) & 0xffff);
		if (manuf == 0x17 && impl == 0x13) {
			/* Hummingbird, aka Ultra-IIe */
			tick_ops = &hbtick_operations;
			clock = of_getintprop_default(dp, "stick-frequency", 0);
		} else {
			tick_ops = &tick_operations;
			clock = local_cpu_data().clock_tick;
		}
	} else {
		tick_ops = &stick_operations;
		clock = of_getintprop_default(dp, "stick-frequency", 0);
	}

	return clock;
}

struct freq_table {
	unsigned long clock_tick_ref;
	unsigned int ref_freq;
};
static DEFINE_PER_CPU(struct freq_table, sparc64_freq_table) = { 0, 0 };

unsigned long sparc64_get_clock_tick(unsigned int cpu)
{
	struct freq_table *ft = &per_cpu(sparc64_freq_table, cpu);

	if (ft->clock_tick_ref)
		return ft->clock_tick_ref;
	return cpu_data(cpu).clock_tick;
}

#ifdef CONFIG_CPU_FREQ

static int sparc64_cpufreq_notifier(struct notifier_block *nb, unsigned long val,
				    void *data)
{
	struct cpufreq_freqs *freq = data;
	unsigned int cpu = freq->cpu;
	struct freq_table *ft = &per_cpu(sparc64_freq_table, cpu);

	if (!ft->ref_freq) {
		ft->ref_freq = freq->old;
		ft->clock_tick_ref = cpu_data(cpu).clock_tick;
	}
	if ((val == CPUFREQ_PRECHANGE  && freq->old < freq->new) ||
	    (val == CPUFREQ_POSTCHANGE && freq->old > freq->new) ||
	    (val == CPUFREQ_RESUMECHANGE)) {
		cpu_data(cpu).clock_tick =
			cpufreq_scale(ft->clock_tick_ref,
				      ft->ref_freq,
				      freq->new);
	}

	return 0;
}

static struct notifier_block sparc64_cpufreq_notifier_block = {
	.notifier_call	= sparc64_cpufreq_notifier
};

static int __init register_sparc64_cpufreq_notifier(void)
{

	cpufreq_register_notifier(&sparc64_cpufreq_notifier_block,
				  CPUFREQ_TRANSITION_NOTIFIER);
	return 0;
}

core_initcall(register_sparc64_cpufreq_notifier);

#endif /* CONFIG_CPU_FREQ */

static int sparc64_next_event(unsigned long delta,
			      struct clock_event_device *evt)
{
	return tick_ops->add_compare(delta) ? -ETIME : 0;
}

static void sparc64_timer_setup(enum clock_event_mode mode,
				struct clock_event_device *evt)
{
	switch (mode) {
	case CLOCK_EVT_MODE_ONESHOT:
	case CLOCK_EVT_MODE_RESUME:
		break;

	case CLOCK_EVT_MODE_SHUTDOWN:
		tick_ops->disable_irq();
		break;

	case CLOCK_EVT_MODE_PERIODIC:
	case CLOCK_EVT_MODE_UNUSED:
		WARN_ON(1);
		break;
	};
}

static struct clock_event_device sparc64_clockevent = {
	.features	= CLOCK_EVT_FEAT_ONESHOT,
	.set_mode	= sparc64_timer_setup,
	.set_next_event	= sparc64_next_event,
	.rating		= 100,
	.shift		= 30,
	.irq		= -1,
};
static DEFINE_PER_CPU(struct clock_event_device, sparc64_events);

void timer_interrupt(int irq, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);
	unsigned long tick_mask = tick_ops->softint_mask;
	int cpu = smp_processor_id();
	struct clock_event_device *evt = &per_cpu(sparc64_events, cpu);

	clear_softint(tick_mask);

	irq_enter();

	kstat_this_cpu.irqs[0]++;

	if (unlikely(!evt->event_handler)) {
		printk(KERN_WARNING
		       "Spurious SPARC64 timer interrupt on cpu %d\n", cpu);
	} else
		evt->event_handler(evt);

	irq_exit();

	set_irq_regs(old_regs);
}

void __devinit setup_sparc64_timer(void)
{
	struct clock_event_device *sevt;
	unsigned long pstate;

	/* Guarantee that the following sequences execute
	 * uninterrupted.
	 */
	__asm__ __volatile__("rdpr	%%pstate, %0\n\t"
			     "wrpr	%0, %1, %%pstate"
			     : "=r" (pstate)
			     : "i" (PSTATE_IE));

	tick_ops->init_tick();

	/* Restore PSTATE_IE. */
	__asm__ __volatile__("wrpr	%0, 0x0, %%pstate"
			     : /* no outputs */
			     : "r" (pstate));

	sevt = &__get_cpu_var(sparc64_events);

	memcpy(sevt, &sparc64_clockevent, sizeof(*sevt));
	sevt->cpumask = cpumask_of_cpu(smp_processor_id());

	clockevents_register_device(sevt);
}

#define SPARC64_NSEC_PER_CYC_SHIFT	10UL

static struct clocksource clocksource_tick = {
	.rating		= 100,
	.mask		= CLOCKSOURCE_MASK(64),
	.shift		= 16,
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
};

static void __init setup_clockevent_multiplier(unsigned long hz)
{
	unsigned long mult, shift = 32;

	while (1) {
		mult = div_sc(hz, NSEC_PER_SEC, shift);
		if (mult && (mult >> 32UL) == 0UL)
			break;

		shift--;
	}

	sparc64_clockevent.shift = shift;
	sparc64_clockevent.mult = mult;
}

static unsigned long tb_ticks_per_usec __read_mostly;

void __delay(unsigned long loops)
{
	unsigned long bclock, now;

	bclock = tick_ops->get_tick();
	do {
		now = tick_ops->get_tick();
	} while ((now-bclock) < loops);
}
EXPORT_SYMBOL(__delay);

void udelay(unsigned long usecs)
{
	__delay(tb_ticks_per_usec * usecs);
}
EXPORT_SYMBOL(udelay);

void __init time_init(void)
{
	unsigned long clock = sparc64_init_timers();

	tb_ticks_per_usec = clock / USEC_PER_SEC;

	timer_ticks_per_nsec_quotient =
		clocksource_hz2mult(clock, SPARC64_NSEC_PER_CYC_SHIFT);

	clocksource_tick.name = tick_ops->name;
	clocksource_tick.mult =
		clocksource_hz2mult(clock,
				    clocksource_tick.shift);
	clocksource_tick.read = tick_ops->get_tick;

	printk("clocksource: mult[%x] shift[%d]\n",
	       clocksource_tick.mult, clocksource_tick.shift);

	clocksource_register(&clocksource_tick);

	sparc64_clockevent.name = tick_ops->name;

	setup_clockevent_multiplier(clock);

	sparc64_clockevent.max_delta_ns =
		clockevent_delta2ns(0x7fffffffffffffffUL, &sparc64_clockevent);
	sparc64_clockevent.min_delta_ns =
		clockevent_delta2ns(0xF, &sparc64_clockevent);

	printk("clockevent: mult[%lx] shift[%d]\n",
	       sparc64_clockevent.mult, sparc64_clockevent.shift);

	setup_sparc64_timer();
}

unsigned long long sched_clock(void)
{
	unsigned long ticks = tick_ops->get_tick();

	return (ticks * timer_ticks_per_nsec_quotient)
		>> SPARC64_NSEC_PER_CYC_SHIFT;
}

static int set_rtc_mmss(unsigned long nowtime)
{
	int real_seconds, real_minutes, chip_minutes;
	void __iomem *bregs = bq4802_regs;
	unsigned long flags;
	unsigned char val;
	int retval = 0;

	/* 
	 * Not having a register set can lead to trouble.
	 * Also starfire doesn't have a tod clock.
	 */
	if (!bregs)
		return -1;

	spin_lock_irqsave(&rtc_lock, flags);

	val = readb(bregs + 0x0e);

	/* BQ4802 RTC chip. */

	writeb(val | 0x08, bregs + 0x0e);

	chip_minutes = readb(bregs + 0x02);
	BCD_TO_BIN(chip_minutes);
	real_seconds = nowtime % 60;
	real_minutes = nowtime / 60;
	if (((abs(real_minutes - chip_minutes) + 15)/30) & 1)
		real_minutes += 30;
	real_minutes %= 60;

	if (abs(real_minutes - chip_minutes) < 30) {
		BIN_TO_BCD(real_seconds);
		BIN_TO_BCD(real_minutes);
		writeb(real_seconds, bregs + 0x00);
		writeb(real_minutes, bregs + 0x02);
	} else {
		printk(KERN_WARNING
		       "set_rtc_mmss: can't update from %d to %d\n",
		       chip_minutes, real_minutes);
		retval = -1;
	}

	writeb(val, bregs + 0x0e);

	spin_unlock_irqrestore(&rtc_lock, flags);

	return retval;
}

#define RTC_IS_OPEN		0x01	/* means /dev/rtc is in use	*/
static unsigned char mini_rtc_status;	/* bitmapped status byte.	*/

#define FEBRUARY	2
#define	STARTOFTIME	1970
#define SECDAY		86400L
#define SECYR		(SECDAY * 365)
#define	leapyear(year)		((year) % 4 == 0 && \
				 ((year) % 100 != 0 || (year) % 400 == 0))
#define	days_in_year(a) 	(leapyear(a) ? 366 : 365)
#define	days_in_month(a) 	(month_days[(a) - 1])

static int month_days[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/*
 * This only works for the Gregorian calendar - i.e. after 1752 (in the UK)
 */
static void GregorianDay(struct rtc_time * tm)
{
	int leapsToDate;
	int lastYear;
	int day;
	int MonthOffset[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

	lastYear = tm->tm_year - 1;

	/*
	 * Number of leap corrections to apply up to end of last year
	 */
	leapsToDate = lastYear / 4 - lastYear / 100 + lastYear / 400;

	/*
	 * This year is a leap year if it is divisible by 4 except when it is
	 * divisible by 100 unless it is divisible by 400
	 *
	 * e.g. 1904 was a leap year, 1900 was not, 1996 is, and 2000 was
	 */
	day = tm->tm_mon > 2 && leapyear(tm->tm_year);

	day += lastYear*365 + leapsToDate + MonthOffset[tm->tm_mon-1] +
		   tm->tm_mday;

	tm->tm_wday = day % 7;
}

static void to_tm(int tim, struct rtc_time *tm)
{
	register int    i;
	register long   hms, day;

	day = tim / SECDAY;
	hms = tim % SECDAY;

	/* Hours, minutes, seconds are easy */
	tm->tm_hour = hms / 3600;
	tm->tm_min = (hms % 3600) / 60;
	tm->tm_sec = (hms % 3600) % 60;

	/* Number of years in days */
	for (i = STARTOFTIME; day >= days_in_year(i); i++)
		day -= days_in_year(i);
	tm->tm_year = i;

	/* Number of months in days left */
	if (leapyear(tm->tm_year))
		days_in_month(FEBRUARY) = 29;
	for (i = 1; day >= days_in_month(i); i++)
		day -= days_in_month(i);
	days_in_month(FEBRUARY) = 28;
	tm->tm_mon = i;

	/* Days are what is left over (+1) from all that. */
	tm->tm_mday = day + 1;

	/*
	 * Determine the day of week
	 */
	GregorianDay(tm);
}

/* Both Starfire and SUN4V give us seconds since Jan 1st, 1970,
 * aka Unix time.  So we have to convert to/from rtc_time.
 */
static void starfire_get_rtc_time(struct rtc_time *time)
{
	u32 seconds = starfire_get_time();

	to_tm(seconds, time);
	time->tm_year -= 1900;
	time->tm_mon -= 1;
}

static int starfire_set_rtc_time(struct rtc_time *time)
{
	u32 seconds = mktime(time->tm_year + 1900, time->tm_mon + 1,
			     time->tm_mday, time->tm_hour,
			     time->tm_min, time->tm_sec);

	return starfire_set_time(seconds);
}

static void hypervisor_get_rtc_time(struct rtc_time *time)
{
	u32 seconds = hypervisor_get_time();

	to_tm(seconds, time);
	time->tm_year -= 1900;
	time->tm_mon -= 1;
}

static int hypervisor_set_rtc_time(struct rtc_time *time)
{
	u32 seconds = mktime(time->tm_year + 1900, time->tm_mon + 1,
			     time->tm_mday, time->tm_hour,
			     time->tm_min, time->tm_sec);

	return hypervisor_set_time(seconds);
}

static void bq4802_get_rtc_time(struct rtc_time *time)
{
	unsigned char val = readb(bq4802_regs + 0x0e);
	unsigned int century;

	writeb(val | 0x08, bq4802_regs + 0x0e);

	time->tm_sec = readb(bq4802_regs + 0x00);
	time->tm_min = readb(bq4802_regs + 0x02);
	time->tm_hour = readb(bq4802_regs + 0x04);
	time->tm_mday = readb(bq4802_regs + 0x06);
	time->tm_mon = readb(bq4802_regs + 0x09);
	time->tm_year = readb(bq4802_regs + 0x0a);
	time->tm_wday = readb(bq4802_regs + 0x08);
	century = readb(bq4802_regs + 0x0f);

	writeb(val, bq4802_regs + 0x0e);

	BCD_TO_BIN(time->tm_sec);
	BCD_TO_BIN(time->tm_min);
	BCD_TO_BIN(time->tm_hour);
	BCD_TO_BIN(time->tm_mday);
	BCD_TO_BIN(time->tm_mon);
	BCD_TO_BIN(time->tm_year);
	BCD_TO_BIN(time->tm_wday);
	BCD_TO_BIN(century);

	time->tm_year += (century * 100);
	time->tm_year -= 1900;

	time->tm_mon--;
}

static int bq4802_set_rtc_time(struct rtc_time *time)
{
	unsigned char val = readb(bq4802_regs + 0x0e);
	unsigned char sec, min, hrs, day, mon, yrs, century;
	unsigned int year;

	year = time->tm_year + 1900;
	century = year / 100;
	yrs = year % 100;

	mon = time->tm_mon + 1;   /* tm_mon starts at zero */
	day = time->tm_mday;
	hrs = time->tm_hour;
	min = time->tm_min;
	sec = time->tm_sec;

	BIN_TO_BCD(sec);
	BIN_TO_BCD(min);
	BIN_TO_BCD(hrs);
	BIN_TO_BCD(day);
	BIN_TO_BCD(mon);
	BIN_TO_BCD(yrs);
	BIN_TO_BCD(century);

	writeb(val | 0x08, bq4802_regs + 0x0e);

	writeb(sec, bq4802_regs + 0x00);
	writeb(min, bq4802_regs + 0x02);
	writeb(hrs, bq4802_regs + 0x04);
	writeb(day, bq4802_regs + 0x06);
	writeb(mon, bq4802_regs + 0x09);
	writeb(yrs, bq4802_regs + 0x0a);
	writeb(century, bq4802_regs + 0x0f);

	writeb(val, bq4802_regs + 0x0e);

	return 0;
}

struct mini_rtc_ops {
	void (*get_rtc_time)(struct rtc_time *);
	int (*set_rtc_time)(struct rtc_time *);
};

static struct mini_rtc_ops starfire_rtc_ops = {
	.get_rtc_time = starfire_get_rtc_time,
	.set_rtc_time = starfire_set_rtc_time,
};

static struct mini_rtc_ops hypervisor_rtc_ops = {
	.get_rtc_time = hypervisor_get_rtc_time,
	.set_rtc_time = hypervisor_set_rtc_time,
};

static struct mini_rtc_ops bq4802_rtc_ops = {
	.get_rtc_time = bq4802_get_rtc_time,
	.set_rtc_time = bq4802_set_rtc_time,
};

static struct mini_rtc_ops *mini_rtc_ops;

static inline void mini_get_rtc_time(struct rtc_time *time)
{
	unsigned long flags;

	spin_lock_irqsave(&rtc_lock, flags);
	mini_rtc_ops->get_rtc_time(time);
	spin_unlock_irqrestore(&rtc_lock, flags);
}

static inline int mini_set_rtc_time(struct rtc_time *time)
{
	unsigned long flags;
	int err;

	spin_lock_irqsave(&rtc_lock, flags);
	err = mini_rtc_ops->set_rtc_time(time);
	spin_unlock_irqrestore(&rtc_lock, flags);

	return err;
}

static int mini_rtc_ioctl(struct inode *inode, struct file *file,
			  unsigned int cmd, unsigned long arg)
{
	struct rtc_time wtime;
	void __user *argp = (void __user *)arg;

	switch (cmd) {

	case RTC_PLL_GET:
		return -EINVAL;

	case RTC_PLL_SET:
		return -EINVAL;

	case RTC_UIE_OFF:	/* disable ints from RTC updates.	*/
		return 0;

	case RTC_UIE_ON:	/* enable ints for RTC updates.	*/
	        return -EINVAL;

	case RTC_RD_TIME:	/* Read the time/date from RTC	*/
		/* this doesn't get week-day, who cares */
		memset(&wtime, 0, sizeof(wtime));
		mini_get_rtc_time(&wtime);

		return copy_to_user(argp, &wtime, sizeof(wtime)) ? -EFAULT : 0;

	case RTC_SET_TIME:	/* Set the RTC */
	    {
		int year, days;

		if (!capable(CAP_SYS_TIME))
			return -EACCES;

		if (copy_from_user(&wtime, argp, sizeof(wtime)))
			return -EFAULT;

		year = wtime.tm_year + 1900;
		days = month_days[wtime.tm_mon] +
		       ((wtime.tm_mon == 1) && leapyear(year));

		if ((wtime.tm_mon < 0 || wtime.tm_mon > 11) ||
		    (wtime.tm_mday < 1))
			return -EINVAL;

		if (wtime.tm_mday < 0 || wtime.tm_mday > days)
			return -EINVAL;

		if (wtime.tm_hour < 0 || wtime.tm_hour >= 24 ||
		    wtime.tm_min < 0 || wtime.tm_min >= 60 ||
		    wtime.tm_sec < 0 || wtime.tm_sec >= 60)
			return -EINVAL;

		return mini_set_rtc_time(&wtime);
	    }
	}

	return -EINVAL;
}

static int mini_rtc_open(struct inode *inode, struct file *file)
{
	lock_kernel();
	if (mini_rtc_status & RTC_IS_OPEN) {
		unlock_kernel();
		return -EBUSY;
	}

	mini_rtc_status |= RTC_IS_OPEN;
	unlock_kernel();

	return 0;
}

static int mini_rtc_release(struct inode *inode, struct file *file)
{
	mini_rtc_status &= ~RTC_IS_OPEN;
	return 0;
}


static const struct file_operations mini_rtc_fops = {
	.owner		= THIS_MODULE,
	.ioctl		= mini_rtc_ioctl,
	.open		= mini_rtc_open,
	.release	= mini_rtc_release,
};

static struct miscdevice rtc_mini_dev =
{
	.minor		= RTC_MINOR,
	.name		= "rtc",
	.fops		= &mini_rtc_fops,
};

static int __init rtc_mini_init(void)
{
	int retval;

	if (tlb_type == hypervisor)
		mini_rtc_ops = &hypervisor_rtc_ops;
	else if (this_is_starfire)
		mini_rtc_ops = &starfire_rtc_ops;
	else if (bq4802_regs)
		mini_rtc_ops = &bq4802_rtc_ops;
	else
		return -ENODEV;

	printk(KERN_INFO "Mini RTC Driver\n");

	retval = misc_register(&rtc_mini_dev);
	if (retval < 0)
		return retval;

	return 0;
}

static void __exit rtc_mini_exit(void)
{
	misc_deregister(&rtc_mini_dev);
}

int __devinit read_current_timer(unsigned long *timer_val)
{
	*timer_val = tick_ops->get_tick();
	return 0;
}

module_init(rtc_mini_init);
module_exit(rtc_mini_exit);
