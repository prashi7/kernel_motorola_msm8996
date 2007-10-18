#ifndef _LINUX_SUSPEND_H
#define _LINUX_SUSPEND_H

#if defined(CONFIG_X86) || defined(CONFIG_FRV) || defined(CONFIG_PPC32) || defined(CONFIG_PPC64)
#include <asm/suspend.h>
#endif
#include <linux/swap.h>
#include <linux/notifier.h>
#include <linux/init.h>
#include <linux/pm.h>
#include <linux/mm.h>
#include <asm/errno.h>

#if defined(CONFIG_PM_SLEEP) && defined(CONFIG_VT) && defined(CONFIG_VT_CONSOLE)
extern int pm_prepare_console(void);
extern void pm_restore_console(void);
#else
static inline int pm_prepare_console(void) { return 0; }
static inline void pm_restore_console(void) {}
#endif

typedef int __bitwise suspend_state_t;

#define PM_SUSPEND_ON		((__force suspend_state_t) 0)
#define PM_SUSPEND_STANDBY	((__force suspend_state_t) 1)
#define PM_SUSPEND_MEM		((__force suspend_state_t) 3)
#define PM_SUSPEND_MAX		((__force suspend_state_t) 4)

/**
 * struct platform_suspend_ops - Callbacks for managing platform dependent
 *	system sleep states.
 *
 * @valid: Callback to determine if given system sleep state is supported by
 *	the platform.
 *	Valid (ie. supported) states are advertised in /sys/power/state.  Note
 *	that it still may be impossible to enter given system sleep state if the
 *	conditions aren't right.
 *	There is the %suspend_valid_only_mem function available that can be
 *	assigned to this if the platform only supports mem sleep.
 *
 * @set_target: Tell the platform which system sleep state is going to be
 *	entered.
 *	@set_target() is executed right prior to suspending devices.  The
 *	information conveyed to the platform code by @set_target() should be
 *	disregarded by the platform as soon as @finish() is executed and if
 *	@prepare() fails.  If @set_target() fails (ie. returns nonzero),
 *	@prepare(), @enter() and @finish() will not be called by the PM core.
 *	This callback is optional.  However, if it is implemented, the argument
 *	passed to @prepare(), @enter() and @finish() is meaningless and should
 *	be ignored.
 *
 * @prepare: Prepare the platform for entering the system sleep state indicated
 *	by @set_target() or represented by the argument if @set_target() is not
 *	implemented.
 *	@prepare() is called right after devices have been suspended (ie. the
 *	appropriate .suspend() method has been executed for each device) and
 *	before the nonboot CPUs are disabled (it is executed with IRQs enabled).
 *	This callback is optional.  It returns 0 on success or a negative
 *	error code otherwise, in which case the system cannot enter the desired
 *	sleep state (@enter() and @finish() will not be called in that case).
 *
 * @enter: Enter the system sleep state indicated by @set_target() or
 *	represented by the argument if @set_target() is not implemented.
 *	This callback is mandatory.  It returns 0 on success or a negative
 *	error code otherwise, in which case the system cannot enter the desired
 *	sleep state.
 *
 * @finish: Called when the system has just left a sleep state, right after
 *	the nonboot CPUs have been enabled and before devices are resumed (it is
 *	executed with IRQs enabled).  If @set_target() is not implemented, the
 *	argument represents the sleep state being left.
 *	This callback is optional, but should be implemented by the platforms
 *	that implement @prepare().  If implemented, it is always called after
 *	@enter() (even if @enter() fails).
 */
struct platform_suspend_ops {
	int (*valid)(suspend_state_t state);
	int (*set_target)(suspend_state_t state);
	int (*prepare)(suspend_state_t state);
	int (*enter)(suspend_state_t state);
	int (*finish)(suspend_state_t state);
};

#ifdef CONFIG_SUSPEND
extern struct platform_suspend_ops *suspend_ops;

/**
 * suspend_set_ops - set platform dependent suspend operations
 * @ops: The new suspend operations to set.
 */
extern void suspend_set_ops(struct platform_suspend_ops *ops);
extern int suspend_valid_only_mem(suspend_state_t state);

/**
 * arch_suspend_disable_irqs - disable IRQs for suspend
 *
 * Disables IRQs (in the default case). This is a weak symbol in the common
 * code and thus allows architectures to override it if more needs to be
 * done. Not called for suspend to disk.
 */
extern void arch_suspend_disable_irqs(void);

/**
 * arch_suspend_enable_irqs - enable IRQs after suspend
 *
 * Enables IRQs (in the default case). This is a weak symbol in the common
 * code and thus allows architectures to override it if more needs to be
 * done. Not called for suspend to disk.
 */
extern void arch_suspend_enable_irqs(void);

extern int pm_suspend(suspend_state_t state);
#else /* !CONFIG_SUSPEND */
#define suspend_valid_only_mem	NULL

static inline void suspend_set_ops(struct platform_suspend_ops *ops) {}
static inline int pm_suspend(suspend_state_t state) { return -ENOSYS; }
#endif /* !CONFIG_SUSPEND */

/* struct pbe is used for creating lists of pages that should be restored
 * atomically during the resume from disk, because the page frames they have
 * occupied before the suspend are in use.
 */
struct pbe {
	void *address;		/* address of the copy */
	void *orig_address;	/* original address of a page */
	struct pbe *next;
};

/* mm/page_alloc.c */
extern void drain_local_pages(void);
extern void mark_free_pages(struct zone *zone);

/**
 * struct hibernation_ops - hibernation platform support
 *
 * The methods in this structure allow a platform to override the default
 * mechanism of shutting down the machine during a hibernation transition.
 *
 * All three methods must be assigned.
 *
 * @prepare: prepare system for hibernation
 * @enter: shut down system after state has been saved to disk
 * @finish: finish/clean up after state has been reloaded
 * @pre_restore: prepare system for the restoration from a hibernation image
 * @restore_cleanup: clean up after a failing image restoration
 */
struct hibernation_ops {
	int (*prepare)(void);
	int (*enter)(void);
	void (*finish)(void);
	int (*pre_restore)(void);
	void (*restore_cleanup)(void);
};

#ifdef CONFIG_HIBERNATION
/* kernel/power/snapshot.c */
extern void __register_nosave_region(unsigned long b, unsigned long e, int km);
static inline void register_nosave_region(unsigned long b, unsigned long e)
{
	__register_nosave_region(b, e, 0);
}
static inline void register_nosave_region_late(unsigned long b, unsigned long e)
{
	__register_nosave_region(b, e, 1);
}
extern int swsusp_page_is_forbidden(struct page *);
extern void swsusp_set_page_free(struct page *);
extern void swsusp_unset_page_free(struct page *);
extern unsigned long get_safe_page(gfp_t gfp_mask);

extern void hibernation_set_ops(struct hibernation_ops *ops);
extern int hibernate(void);
#else /* CONFIG_HIBERNATION */
static inline int swsusp_page_is_forbidden(struct page *p) { return 0; }
static inline void swsusp_set_page_free(struct page *p) {}
static inline void swsusp_unset_page_free(struct page *p) {}

static inline void hibernation_set_ops(struct hibernation_ops *ops) {}
static inline int hibernate(void) { return -ENOSYS; }
#endif /* CONFIG_HIBERNATION */

#ifdef CONFIG_PM_SLEEP
void save_processor_state(void);
void restore_processor_state(void);
struct saved_context;
void __save_processor_state(struct saved_context *ctxt);
void __restore_processor_state(struct saved_context *ctxt);

/* kernel/power/main.c */
extern struct blocking_notifier_head pm_chain_head;

static inline int register_pm_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&pm_chain_head, nb);
}

static inline int unregister_pm_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&pm_chain_head, nb);
}

#define pm_notifier(fn, pri) {				\
	static struct notifier_block fn##_nb =			\
		{ .notifier_call = fn, .priority = pri };	\
	register_pm_notifier(&fn##_nb);			\
}
#else /* !CONFIG_PM_SLEEP */

static inline int register_pm_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline int unregister_pm_notifier(struct notifier_block *nb)
{
	return 0;
}

#define pm_notifier(fn, pri)	do { (void)(fn); } while (0)
#endif /* !CONFIG_PM_SLEEP */

#ifndef CONFIG_HIBERNATION
static inline void register_nosave_region(unsigned long b, unsigned long e)
{
}
static inline void register_nosave_region_late(unsigned long b, unsigned long e)
{
}
#endif

#endif /* _LINUX_SUSPEND_H */
