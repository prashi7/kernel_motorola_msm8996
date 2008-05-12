#ifndef _LINUX_CPUPRI_H
#define _LINUX_CPUPRI_H

#include <linux/sched.h>

#define CPUPRI_NR_PRIORITIES 2+MAX_RT_PRIO
#define CPUPRI_NR_PRI_WORDS CPUPRI_NR_PRIORITIES/BITS_PER_LONG

#define CPUPRI_INVALID -1
#define CPUPRI_IDLE     0
#define CPUPRI_NORMAL   1
/* values 2-101 are RT priorities 0-99 */

struct cpupri_vec {
	spinlock_t lock;
	int        count;
	cpumask_t  mask;
};

struct cpupri {
	struct cpupri_vec pri_to_cpu[CPUPRI_NR_PRIORITIES];
	long              pri_active[CPUPRI_NR_PRI_WORDS];
	int               cpu_to_pri[NR_CPUS];
};

#ifdef CONFIG_SMP
int  cpupri_find(struct cpupri *cp,
		 struct task_struct *p, cpumask_t *lowest_mask);
void cpupri_set(struct cpupri *cp, int cpu, int pri);
void cpupri_init(struct cpupri *cp);
#else
#define cpupri_set(cp, cpu, pri) do { } while (0)
#define cpupri_init() do { } while (0)
#endif

#endif /* _LINUX_CPUPRI_H */
