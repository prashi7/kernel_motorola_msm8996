/*
 * zsmalloc memory allocator
 *
 * Copyright (C) 2011  Nitin Gupta
 *
 * This code is released using a dual license strategy: BSD/GPL
 * You can choose the license that better fits your requirements.
 *
 * Released under the terms of 3-clause BSD License
 * Released under the terms of GNU General Public License Version 2.0
 */

#ifndef _ZS_MALLOC_INT_H_
#define _ZS_MALLOC_INT_H_

#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/types.h>

/*
 * This must be power of 2 and greater than of equal to sizeof(link_free).
 * These two conditions ensure that any 'struct link_free' itself doesn't
 * span more than 1 page which avoids complex case of mapping 2 pages simply
 * to restore link_free pointer values.
 */
#define ZS_ALIGN		8

/* ZS_MIN_ALLOC_SIZE must be multiple of ZS_ALIGN */
#define ZS_MIN_ALLOC_SIZE	32
#define ZS_MAX_ALLOC_SIZE	PAGE_SIZE

/*
 * On systems with 4K page size, this gives 254 size classes! There is a
 * trader-off here:
 *  - Large number of size classes is potentially wasteful as free page are
 *    spread across these classes
 *  - Small number of size classes causes large internal fragmentation
 *  - Probably its better to use specific size classes (empirically
 *    determined). NOTE: all those class sizes must be set as multiple of
 *    ZS_ALIGN to make sure link_free itself never has to span 2 pages.
 *
 *  ZS_MIN_ALLOC_SIZE and ZS_SIZE_CLASS_DELTA must be multiple of ZS_ALIGN
 *  (reason above)
 */
#define ZS_SIZE_CLASS_DELTA	16
#define ZS_SIZE_CLASSES		((ZS_MAX_ALLOC_SIZE - ZS_MIN_ALLOC_SIZE) / \
					ZS_SIZE_CLASS_DELTA + 1)

/*
 * A single 'zspage' is composed of N discontiguous 0-order (single) pages.
 * This defines upper limit on N.
 */
static const int max_zspage_order = 4;

/*
 * We do not maintain any list for completely empty or full pages
 */
enum fullness_group {
	ZS_ALMOST_FULL,
	ZS_ALMOST_EMPTY,
	_ZS_NR_FULLNESS_GROUPS,

	ZS_EMPTY,
	ZS_FULL
};

/*
 * We assign a page to ZS_ALMOST_EMPTY fullness group when:
 *	n <= N / f, where
 * n = number of allocated objects
 * N = total number of objects zspage can store
 * f = 1/fullness_threshold_frac
 *
 * Similarly, we assign zspage to:
 *	ZS_ALMOST_FULL	when n > N / f
 *	ZS_EMPTY	when n == 0
 *	ZS_FULL		when n == N
 *
 * (see: fix_fullness_group())
 */
static const int fullness_threshold_frac = 4;

struct mapping_area {
	struct vm_struct *vm;
	pte_t *vm_ptes[2];
	char *vm_addr;
};

struct size_class {
	/*
	 * Size of objects stored in this class. Must be multiple
	 * of ZS_ALIGN.
	 */
	int size;
	unsigned int index;

	/* Number of PAGE_SIZE sized pages to combine to form a 'zspage' */
	int zspage_order;

	spinlock_t lock;

	/* stats */
	u64 pages_allocated;

	struct page *fullness_list[_ZS_NR_FULLNESS_GROUPS];
};

/*
 * Placed within free objects to form a singly linked list.
 * For every zspage, first_page->freelist gives head of this list.
 *
 * This must be power of 2 and less than or equal to ZS_ALIGN
 */
struct link_free {
	/* Handle of next free chunk (encodes <PFN, obj_idx>) */
	void *next;
};

struct zs_pool {
	struct size_class size_class[ZS_SIZE_CLASSES];

	gfp_t flags;	/* allocation flags used when growing pool */
	const char *name;
};

#endif
