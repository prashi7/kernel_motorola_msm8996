/*
 * Copyright © 2008 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *    Keith Packard <keithp@keithp.com>
 *
 */

#include <linux/seq_file.h>
#include <linux/circ_buf.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <linux/list_sort.h>
#include <asm/msr-index.h>
#include <drm/drmP.h>
#include "intel_drv.h"
#include "intel_ringbuffer.h"
#include <drm/i915_drm.h>
#include "i915_drv.h"

enum {
	ACTIVE_LIST,
	INACTIVE_LIST,
	PINNED_LIST,
};

/* As the drm_debugfs_init() routines are called before dev->dev_private is
 * allocated we need to hook into the minor for release. */
static int
drm_add_fake_info_node(struct drm_minor *minor,
		       struct dentry *ent,
		       const void *key)
{
	struct drm_info_node *node;

	node = kmalloc(sizeof(*node), GFP_KERNEL);
	if (node == NULL) {
		debugfs_remove(ent);
		return -ENOMEM;
	}

	node->minor = minor;
	node->dent = ent;
	node->info_ent = (void *) key;

	mutex_lock(&minor->debugfs_lock);
	list_add(&node->list, &minor->debugfs_list);
	mutex_unlock(&minor->debugfs_lock);

	return 0;
}

static int i915_capabilities(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	const struct intel_device_info *info = INTEL_INFO(dev);

	seq_printf(m, "gen: %d\n", info->gen);
	seq_printf(m, "pch: %d\n", INTEL_PCH_TYPE(dev));
#define PRINT_FLAG(x)  seq_printf(m, #x ": %s\n", yesno(info->x))
#define SEP_SEMICOLON ;
	DEV_INFO_FOR_EACH_FLAG(PRINT_FLAG, SEP_SEMICOLON);
#undef PRINT_FLAG
#undef SEP_SEMICOLON

	return 0;
}

static const char *get_pin_flag(struct drm_i915_gem_object *obj)
{
	if (obj->pin_display)
		return "p";
	else
		return " ";
}

static const char *get_tiling_flag(struct drm_i915_gem_object *obj)
{
	switch (obj->tiling_mode) {
	default:
	case I915_TILING_NONE: return " ";
	case I915_TILING_X: return "X";
	case I915_TILING_Y: return "Y";
	}
}

static inline const char *get_global_flag(struct drm_i915_gem_object *obj)
{
	return i915_gem_obj_to_ggtt(obj) ? "g" : " ";
}

static u64 i915_gem_obj_total_ggtt_size(struct drm_i915_gem_object *obj)
{
	u64 size = 0;
	struct i915_vma *vma;

	list_for_each_entry(vma, &obj->vma_list, vma_link) {
		if (i915_is_ggtt(vma->vm) &&
		    drm_mm_node_allocated(&vma->node))
			size += vma->node.size;
	}

	return size;
}

static void
describe_obj(struct seq_file *m, struct drm_i915_gem_object *obj)
{
	struct drm_i915_private *dev_priv = to_i915(obj->base.dev);
	struct intel_engine_cs *ring;
	struct i915_vma *vma;
	int pin_count = 0;
	int i;

	seq_printf(m, "%pK: %s%s%s%s %8zdKiB %02x %02x [ ",
		   &obj->base,
		   obj->active ? "*" : " ",
		   get_pin_flag(obj),
		   get_tiling_flag(obj),
		   get_global_flag(obj),
		   obj->base.size / 1024,
		   obj->base.read_domains,
		   obj->base.write_domain);
	for_each_ring(ring, dev_priv, i)
		seq_printf(m, "%x ",
				i915_gem_request_get_seqno(obj->last_read_req[i]));
	seq_printf(m, "] %x %x%s%s%s",
		   i915_gem_request_get_seqno(obj->last_write_req),
		   i915_gem_request_get_seqno(obj->last_fenced_req),
		   i915_cache_level_str(to_i915(obj->base.dev), obj->cache_level),
		   obj->dirty ? " dirty" : "",
		   obj->madv == I915_MADV_DONTNEED ? " purgeable" : "");
	if (obj->base.name)
		seq_printf(m, " (name: %d)", obj->base.name);
	list_for_each_entry(vma, &obj->vma_list, vma_link) {
		if (vma->pin_count > 0)
			pin_count++;
	}
	seq_printf(m, " (pinned x %d)", pin_count);
	if (obj->pin_display)
		seq_printf(m, " (display)");
	if (obj->fence_reg != I915_FENCE_REG_NONE)
		seq_printf(m, " (fence: %d)", obj->fence_reg);
	list_for_each_entry(vma, &obj->vma_list, vma_link) {
		seq_printf(m, " (%sgtt offset: %08llx, size: %08llx",
			   i915_is_ggtt(vma->vm) ? "g" : "pp",
			   vma->node.start, vma->node.size);
		if (i915_is_ggtt(vma->vm))
			seq_printf(m, ", type: %u)", vma->ggtt_view.type);
		else
			seq_puts(m, ")");
	}
	if (obj->stolen)
		seq_printf(m, " (stolen: %08llx)", obj->stolen->start);
	if (obj->pin_display || obj->fault_mappable) {
		char s[3], *t = s;
		if (obj->pin_display)
			*t++ = 'p';
		if (obj->fault_mappable)
			*t++ = 'f';
		*t = '\0';
		seq_printf(m, " (%s mappable)", s);
	}
	if (obj->last_write_req != NULL)
		seq_printf(m, " (%s)",
			   i915_gem_request_get_ring(obj->last_write_req)->name);
	if (obj->frontbuffer_bits)
		seq_printf(m, " (frontbuffer: 0x%03x)", obj->frontbuffer_bits);
}

static void describe_ctx(struct seq_file *m, struct intel_context *ctx)
{
	seq_putc(m, ctx->legacy_hw_ctx.initialized ? 'I' : 'i');
	seq_putc(m, ctx->remap_slice ? 'R' : 'r');
	seq_putc(m, ' ');
}

static int i915_gem_object_list_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	uintptr_t list = (uintptr_t) node->info_ent->data;
	struct list_head *head;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct i915_address_space *vm = &dev_priv->gtt.base;
	struct i915_vma *vma;
	u64 total_obj_size, total_gtt_size;
	int count, ret;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	/* FIXME: the user of this interface might want more than just GGTT */
	switch (list) {
	case ACTIVE_LIST:
		seq_puts(m, "Active:\n");
		head = &vm->active_list;
		break;
	case INACTIVE_LIST:
		seq_puts(m, "Inactive:\n");
		head = &vm->inactive_list;
		break;
	default:
		mutex_unlock(&dev->struct_mutex);
		return -EINVAL;
	}

	total_obj_size = total_gtt_size = count = 0;
	list_for_each_entry(vma, head, mm_list) {
		seq_printf(m, "   ");
		describe_obj(m, vma->obj);
		seq_printf(m, "\n");
		total_obj_size += vma->obj->base.size;
		total_gtt_size += vma->node.size;
		count++;
	}
	mutex_unlock(&dev->struct_mutex);

	seq_printf(m, "Total %d objects, %llu bytes, %llu GTT size\n",
		   count, total_obj_size, total_gtt_size);
	return 0;
}

static int obj_rank_by_stolen(void *priv,
			      struct list_head *A, struct list_head *B)
{
	struct drm_i915_gem_object *a =
		container_of(A, struct drm_i915_gem_object, obj_exec_link);
	struct drm_i915_gem_object *b =
		container_of(B, struct drm_i915_gem_object, obj_exec_link);

	if (a->stolen->start < b->stolen->start)
		return -1;
	if (a->stolen->start > b->stolen->start)
		return 1;
	return 0;
}

static int i915_gem_stolen_list_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_i915_gem_object *obj;
	u64 total_obj_size, total_gtt_size;
	LIST_HEAD(stolen);
	int count, ret;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	total_obj_size = total_gtt_size = count = 0;
	list_for_each_entry(obj, &dev_priv->mm.bound_list, global_list) {
		if (obj->stolen == NULL)
			continue;

		list_add(&obj->obj_exec_link, &stolen);

		total_obj_size += obj->base.size;
		total_gtt_size += i915_gem_obj_total_ggtt_size(obj);
		count++;
	}
	list_for_each_entry(obj, &dev_priv->mm.unbound_list, global_list) {
		if (obj->stolen == NULL)
			continue;

		list_add(&obj->obj_exec_link, &stolen);

		total_obj_size += obj->base.size;
		count++;
	}
	list_sort(NULL, &stolen, obj_rank_by_stolen);
	seq_puts(m, "Stolen:\n");
	while (!list_empty(&stolen)) {
		obj = list_first_entry(&stolen, typeof(*obj), obj_exec_link);
		seq_puts(m, "   ");
		describe_obj(m, obj);
		seq_putc(m, '\n');
		list_del_init(&obj->obj_exec_link);
	}
	mutex_unlock(&dev->struct_mutex);

	seq_printf(m, "Total %d objects, %llu bytes, %llu GTT size\n",
		   count, total_obj_size, total_gtt_size);
	return 0;
}

#define count_objects(list, member) do { \
	list_for_each_entry(obj, list, member) { \
		size += i915_gem_obj_total_ggtt_size(obj); \
		++count; \
		if (obj->map_and_fenceable) { \
			mappable_size += i915_gem_obj_ggtt_size(obj); \
			++mappable_count; \
		} \
	} \
} while (0)

struct file_stats {
	struct drm_i915_file_private *file_priv;
	unsigned long count;
	u64 total, unbound;
	u64 global, shared;
	u64 active, inactive;
};

static int per_file_stats(int id, void *ptr, void *data)
{
	struct drm_i915_gem_object *obj = ptr;
	struct file_stats *stats = data;
	struct i915_vma *vma;

	stats->count++;
	stats->total += obj->base.size;

	if (obj->base.name || obj->base.dma_buf)
		stats->shared += obj->base.size;

	if (USES_FULL_PPGTT(obj->base.dev)) {
		list_for_each_entry(vma, &obj->vma_list, vma_link) {
			struct i915_hw_ppgtt *ppgtt;

			if (!drm_mm_node_allocated(&vma->node))
				continue;

			if (i915_is_ggtt(vma->vm)) {
				stats->global += obj->base.size;
				continue;
			}

			ppgtt = container_of(vma->vm, struct i915_hw_ppgtt, base);
			if (ppgtt->file_priv != stats->file_priv)
				continue;

			if (obj->active) /* XXX per-vma statistic */
				stats->active += obj->base.size;
			else
				stats->inactive += obj->base.size;

			return 0;
		}
	} else {
		if (i915_gem_obj_ggtt_bound(obj)) {
			stats->global += obj->base.size;
			if (obj->active)
				stats->active += obj->base.size;
			else
				stats->inactive += obj->base.size;
			return 0;
		}
	}

	if (!list_empty(&obj->global_list))
		stats->unbound += obj->base.size;

	return 0;
}

#define print_file_stats(m, name, stats) do { \
	if (stats.count) \
		seq_printf(m, "%s: %lu objects, %llu bytes (%llu active, %llu inactive, %llu global, %llu shared, %llu unbound)\n", \
			   name, \
			   stats.count, \
			   stats.total, \
			   stats.active, \
			   stats.inactive, \
			   stats.global, \
			   stats.shared, \
			   stats.unbound); \
} while (0)

static void print_batch_pool_stats(struct seq_file *m,
				   struct drm_i915_private *dev_priv)
{
	struct drm_i915_gem_object *obj;
	struct file_stats stats;
	struct intel_engine_cs *ring;
	int i, j;

	memset(&stats, 0, sizeof(stats));

	for_each_ring(ring, dev_priv, i) {
		for (j = 0; j < ARRAY_SIZE(ring->batch_pool.cache_list); j++) {
			list_for_each_entry(obj,
					    &ring->batch_pool.cache_list[j],
					    batch_pool_link)
				per_file_stats(0, obj, &stats);
		}
	}

	print_file_stats(m, "[k]batch pool", stats);
}

#define count_vmas(list, member) do { \
	list_for_each_entry(vma, list, member) { \
		size += i915_gem_obj_total_ggtt_size(vma->obj); \
		++count; \
		if (vma->obj->map_and_fenceable) { \
			mappable_size += i915_gem_obj_ggtt_size(vma->obj); \
			++mappable_count; \
		} \
	} \
} while (0)

static int i915_gem_object_info(struct seq_file *m, void* data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 count, mappable_count, purgeable_count;
	u64 size, mappable_size, purgeable_size;
	struct drm_i915_gem_object *obj;
	struct i915_address_space *vm = &dev_priv->gtt.base;
	struct drm_file *file;
	struct i915_vma *vma;
	int ret;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	seq_printf(m, "%u objects, %zu bytes\n",
		   dev_priv->mm.object_count,
		   dev_priv->mm.object_memory);

	size = count = mappable_size = mappable_count = 0;
	count_objects(&dev_priv->mm.bound_list, global_list);
	seq_printf(m, "%u [%u] objects, %llu [%llu] bytes in gtt\n",
		   count, mappable_count, size, mappable_size);

	size = count = mappable_size = mappable_count = 0;
	count_vmas(&vm->active_list, mm_list);
	seq_printf(m, "  %u [%u] active objects, %llu [%llu] bytes\n",
		   count, mappable_count, size, mappable_size);

	size = count = mappable_size = mappable_count = 0;
	count_vmas(&vm->inactive_list, mm_list);
	seq_printf(m, "  %u [%u] inactive objects, %llu [%llu] bytes\n",
		   count, mappable_count, size, mappable_size);

	size = count = purgeable_size = purgeable_count = 0;
	list_for_each_entry(obj, &dev_priv->mm.unbound_list, global_list) {
		size += obj->base.size, ++count;
		if (obj->madv == I915_MADV_DONTNEED)
			purgeable_size += obj->base.size, ++purgeable_count;
	}
	seq_printf(m, "%u unbound objects, %llu bytes\n", count, size);

	size = count = mappable_size = mappable_count = 0;
	list_for_each_entry(obj, &dev_priv->mm.bound_list, global_list) {
		if (obj->fault_mappable) {
			size += i915_gem_obj_ggtt_size(obj);
			++count;
		}
		if (obj->pin_display) {
			mappable_size += i915_gem_obj_ggtt_size(obj);
			++mappable_count;
		}
		if (obj->madv == I915_MADV_DONTNEED) {
			purgeable_size += obj->base.size;
			++purgeable_count;
		}
	}
	seq_printf(m, "%u purgeable objects, %llu bytes\n",
		   purgeable_count, purgeable_size);
	seq_printf(m, "%u pinned mappable objects, %llu bytes\n",
		   mappable_count, mappable_size);
	seq_printf(m, "%u fault mappable objects, %llu bytes\n",
		   count, size);

	seq_printf(m, "%llu [%llu] gtt total\n",
		   dev_priv->gtt.base.total,
		   (u64)dev_priv->gtt.mappable_end - dev_priv->gtt.base.start);

	seq_putc(m, '\n');
	print_batch_pool_stats(m, dev_priv);
	list_for_each_entry_reverse(file, &dev->filelist, lhead) {
		struct file_stats stats;
		struct task_struct *task;

		memset(&stats, 0, sizeof(stats));
		stats.file_priv = file->driver_priv;
		spin_lock(&file->table_lock);
		idr_for_each(&file->object_idr, per_file_stats, &stats);
		spin_unlock(&file->table_lock);
		/*
		 * Although we have a valid reference on file->pid, that does
		 * not guarantee that the task_struct who called get_pid() is
		 * still alive (e.g. get_pid(current) => fork() => exit()).
		 * Therefore, we need to protect this ->comm access using RCU.
		 */
		rcu_read_lock();
		task = pid_task(file->pid, PIDTYPE_PID);
		print_file_stats(m, task ? task->comm : "<unknown>", stats);
		rcu_read_unlock();
	}

	mutex_unlock(&dev->struct_mutex);

	return 0;
}

static int i915_gem_gtt_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	uintptr_t list = (uintptr_t) node->info_ent->data;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_i915_gem_object *obj;
	u64 total_obj_size, total_gtt_size;
	int count, ret;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	total_obj_size = total_gtt_size = count = 0;
	list_for_each_entry(obj, &dev_priv->mm.bound_list, global_list) {
		if (list == PINNED_LIST && !i915_gem_obj_is_pinned(obj))
			continue;

		seq_puts(m, "   ");
		describe_obj(m, obj);
		seq_putc(m, '\n');
		total_obj_size += obj->base.size;
		total_gtt_size += i915_gem_obj_total_ggtt_size(obj);
		count++;
	}

	mutex_unlock(&dev->struct_mutex);

	seq_printf(m, "Total %d objects, %llu bytes, %llu GTT size\n",
		   count, total_obj_size, total_gtt_size);

	return 0;
}

static int i915_gem_pageflip_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_crtc *crtc;
	int ret;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	for_each_intel_crtc(dev, crtc) {
		const char pipe = pipe_name(crtc->pipe);
		const char plane = plane_name(crtc->plane);
		struct intel_unpin_work *work;

		spin_lock_irq(&dev->event_lock);
		work = crtc->unpin_work;
		if (work == NULL) {
			seq_printf(m, "No flip due on pipe %c (plane %c)\n",
				   pipe, plane);
		} else {
			u32 addr;

			if (atomic_read(&work->pending) < INTEL_FLIP_COMPLETE) {
				seq_printf(m, "Flip queued on pipe %c (plane %c)\n",
					   pipe, plane);
			} else {
				seq_printf(m, "Flip pending (waiting for vsync) on pipe %c (plane %c)\n",
					   pipe, plane);
			}
			if (work->flip_queued_req) {
				struct intel_engine_cs *ring =
					i915_gem_request_get_ring(work->flip_queued_req);

				seq_printf(m, "Flip queued on %s at seqno %x, next seqno %x [current breadcrumb %x], completed? %d\n",
					   ring->name,
					   i915_gem_request_get_seqno(work->flip_queued_req),
					   dev_priv->next_seqno,
					   ring->get_seqno(ring, true),
					   i915_gem_request_completed(work->flip_queued_req, true));
			} else
				seq_printf(m, "Flip not associated with any ring\n");
			seq_printf(m, "Flip queued on frame %d, (was ready on frame %d), now %d\n",
				   work->flip_queued_vblank,
				   work->flip_ready_vblank,
				   drm_crtc_vblank_count(&crtc->base));
			if (work->enable_stall_check)
				seq_puts(m, "Stall check enabled, ");
			else
				seq_puts(m, "Stall check waiting for page flip ioctl, ");
			seq_printf(m, "%d prepares\n", atomic_read(&work->pending));

			if (INTEL_INFO(dev)->gen >= 4)
				addr = I915_HI_DISPBASE(I915_READ(DSPSURF(crtc->plane)));
			else
				addr = I915_READ(DSPADDR(crtc->plane));
			seq_printf(m, "Current scanout address 0x%08x\n", addr);

			if (work->pending_flip_obj) {
				seq_printf(m, "New framebuffer address 0x%08lx\n", (long)work->gtt_offset);
				seq_printf(m, "MMIO update completed? %d\n",  addr == work->gtt_offset);
			}
		}
		spin_unlock_irq(&dev->event_lock);
	}

	mutex_unlock(&dev->struct_mutex);

	return 0;
}

static int i915_gem_batch_pool_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_i915_gem_object *obj;
	struct intel_engine_cs *ring;
	int total = 0;
	int ret, i, j;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	for_each_ring(ring, dev_priv, i) {
		for (j = 0; j < ARRAY_SIZE(ring->batch_pool.cache_list); j++) {
			int count;

			count = 0;
			list_for_each_entry(obj,
					    &ring->batch_pool.cache_list[j],
					    batch_pool_link)
				count++;
			seq_printf(m, "%s cache[%d]: %d objects\n",
				   ring->name, j, count);

			list_for_each_entry(obj,
					    &ring->batch_pool.cache_list[j],
					    batch_pool_link) {
				seq_puts(m, "   ");
				describe_obj(m, obj);
				seq_putc(m, '\n');
			}

			total += count;
		}
	}

	seq_printf(m, "total: %d\n", total);

	mutex_unlock(&dev->struct_mutex);

	return 0;
}

static int i915_gem_request_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_engine_cs *ring;
	struct drm_i915_gem_request *req;
	int ret, any, i;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	any = 0;
	for_each_ring(ring, dev_priv, i) {
		int count;

		count = 0;
		list_for_each_entry(req, &ring->request_list, list)
			count++;
		if (count == 0)
			continue;

		seq_printf(m, "%s requests: %d\n", ring->name, count);
		list_for_each_entry(req, &ring->request_list, list) {
			struct task_struct *task;

			rcu_read_lock();
			task = NULL;
			if (req->pid)
				task = pid_task(req->pid, PIDTYPE_PID);
			seq_printf(m, "    %x @ %d: %s [%d]\n",
				   req->seqno,
				   (int) (jiffies - req->emitted_jiffies),
				   task ? task->comm : "<unknown>",
				   task ? task->pid : -1);
			rcu_read_unlock();
		}

		any++;
	}
	mutex_unlock(&dev->struct_mutex);

	if (any == 0)
		seq_puts(m, "No requests\n");

	return 0;
}

static void i915_ring_seqno_info(struct seq_file *m,
				 struct intel_engine_cs *ring)
{
	if (ring->get_seqno) {
		seq_printf(m, "Current sequence (%s): %x\n",
			   ring->name, ring->get_seqno(ring, false));
	}
}

static int i915_gem_seqno_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_engine_cs *ring;
	int ret, i;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;
	intel_runtime_pm_get(dev_priv);

	for_each_ring(ring, dev_priv, i)
		i915_ring_seqno_info(m, ring);

	intel_runtime_pm_put(dev_priv);
	mutex_unlock(&dev->struct_mutex);

	return 0;
}


static int i915_interrupt_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_engine_cs *ring;
	int ret, i, pipe;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;
	intel_runtime_pm_get(dev_priv);

	if (IS_CHERRYVIEW(dev)) {
		seq_printf(m, "Master Interrupt Control:\t%08x\n",
			   I915_READ(GEN8_MASTER_IRQ));

		seq_printf(m, "Display IER:\t%08x\n",
			   I915_READ(VLV_IER));
		seq_printf(m, "Display IIR:\t%08x\n",
			   I915_READ(VLV_IIR));
		seq_printf(m, "Display IIR_RW:\t%08x\n",
			   I915_READ(VLV_IIR_RW));
		seq_printf(m, "Display IMR:\t%08x\n",
			   I915_READ(VLV_IMR));
		for_each_pipe(dev_priv, pipe)
			seq_printf(m, "Pipe %c stat:\t%08x\n",
				   pipe_name(pipe),
				   I915_READ(PIPESTAT(pipe)));

		seq_printf(m, "Port hotplug:\t%08x\n",
			   I915_READ(PORT_HOTPLUG_EN));
		seq_printf(m, "DPFLIPSTAT:\t%08x\n",
			   I915_READ(VLV_DPFLIPSTAT));
		seq_printf(m, "DPINVGTT:\t%08x\n",
			   I915_READ(DPINVGTT));

		for (i = 0; i < 4; i++) {
			seq_printf(m, "GT Interrupt IMR %d:\t%08x\n",
				   i, I915_READ(GEN8_GT_IMR(i)));
			seq_printf(m, "GT Interrupt IIR %d:\t%08x\n",
				   i, I915_READ(GEN8_GT_IIR(i)));
			seq_printf(m, "GT Interrupt IER %d:\t%08x\n",
				   i, I915_READ(GEN8_GT_IER(i)));
		}

		seq_printf(m, "PCU interrupt mask:\t%08x\n",
			   I915_READ(GEN8_PCU_IMR));
		seq_printf(m, "PCU interrupt identity:\t%08x\n",
			   I915_READ(GEN8_PCU_IIR));
		seq_printf(m, "PCU interrupt enable:\t%08x\n",
			   I915_READ(GEN8_PCU_IER));
	} else if (INTEL_INFO(dev)->gen >= 8) {
		seq_printf(m, "Master Interrupt Control:\t%08x\n",
			   I915_READ(GEN8_MASTER_IRQ));

		for (i = 0; i < 4; i++) {
			seq_printf(m, "GT Interrupt IMR %d:\t%08x\n",
				   i, I915_READ(GEN8_GT_IMR(i)));
			seq_printf(m, "GT Interrupt IIR %d:\t%08x\n",
				   i, I915_READ(GEN8_GT_IIR(i)));
			seq_printf(m, "GT Interrupt IER %d:\t%08x\n",
				   i, I915_READ(GEN8_GT_IER(i)));
		}

		for_each_pipe(dev_priv, pipe) {
			if (!intel_display_power_is_enabled(dev_priv,
						POWER_DOMAIN_PIPE(pipe))) {
				seq_printf(m, "Pipe %c power disabled\n",
					   pipe_name(pipe));
				continue;
			}
			seq_printf(m, "Pipe %c IMR:\t%08x\n",
				   pipe_name(pipe),
				   I915_READ(GEN8_DE_PIPE_IMR(pipe)));
			seq_printf(m, "Pipe %c IIR:\t%08x\n",
				   pipe_name(pipe),
				   I915_READ(GEN8_DE_PIPE_IIR(pipe)));
			seq_printf(m, "Pipe %c IER:\t%08x\n",
				   pipe_name(pipe),
				   I915_READ(GEN8_DE_PIPE_IER(pipe)));
		}

		seq_printf(m, "Display Engine port interrupt mask:\t%08x\n",
			   I915_READ(GEN8_DE_PORT_IMR));
		seq_printf(m, "Display Engine port interrupt identity:\t%08x\n",
			   I915_READ(GEN8_DE_PORT_IIR));
		seq_printf(m, "Display Engine port interrupt enable:\t%08x\n",
			   I915_READ(GEN8_DE_PORT_IER));

		seq_printf(m, "Display Engine misc interrupt mask:\t%08x\n",
			   I915_READ(GEN8_DE_MISC_IMR));
		seq_printf(m, "Display Engine misc interrupt identity:\t%08x\n",
			   I915_READ(GEN8_DE_MISC_IIR));
		seq_printf(m, "Display Engine misc interrupt enable:\t%08x\n",
			   I915_READ(GEN8_DE_MISC_IER));

		seq_printf(m, "PCU interrupt mask:\t%08x\n",
			   I915_READ(GEN8_PCU_IMR));
		seq_printf(m, "PCU interrupt identity:\t%08x\n",
			   I915_READ(GEN8_PCU_IIR));
		seq_printf(m, "PCU interrupt enable:\t%08x\n",
			   I915_READ(GEN8_PCU_IER));
	} else if (IS_VALLEYVIEW(dev)) {
		seq_printf(m, "Display IER:\t%08x\n",
			   I915_READ(VLV_IER));
		seq_printf(m, "Display IIR:\t%08x\n",
			   I915_READ(VLV_IIR));
		seq_printf(m, "Display IIR_RW:\t%08x\n",
			   I915_READ(VLV_IIR_RW));
		seq_printf(m, "Display IMR:\t%08x\n",
			   I915_READ(VLV_IMR));
		for_each_pipe(dev_priv, pipe)
			seq_printf(m, "Pipe %c stat:\t%08x\n",
				   pipe_name(pipe),
				   I915_READ(PIPESTAT(pipe)));

		seq_printf(m, "Master IER:\t%08x\n",
			   I915_READ(VLV_MASTER_IER));

		seq_printf(m, "Render IER:\t%08x\n",
			   I915_READ(GTIER));
		seq_printf(m, "Render IIR:\t%08x\n",
			   I915_READ(GTIIR));
		seq_printf(m, "Render IMR:\t%08x\n",
			   I915_READ(GTIMR));

		seq_printf(m, "PM IER:\t\t%08x\n",
			   I915_READ(GEN6_PMIER));
		seq_printf(m, "PM IIR:\t\t%08x\n",
			   I915_READ(GEN6_PMIIR));
		seq_printf(m, "PM IMR:\t\t%08x\n",
			   I915_READ(GEN6_PMIMR));

		seq_printf(m, "Port hotplug:\t%08x\n",
			   I915_READ(PORT_HOTPLUG_EN));
		seq_printf(m, "DPFLIPSTAT:\t%08x\n",
			   I915_READ(VLV_DPFLIPSTAT));
		seq_printf(m, "DPINVGTT:\t%08x\n",
			   I915_READ(DPINVGTT));

	} else if (!HAS_PCH_SPLIT(dev)) {
		seq_printf(m, "Interrupt enable:    %08x\n",
			   I915_READ(IER));
		seq_printf(m, "Interrupt identity:  %08x\n",
			   I915_READ(IIR));
		seq_printf(m, "Interrupt mask:      %08x\n",
			   I915_READ(IMR));
		for_each_pipe(dev_priv, pipe)
			seq_printf(m, "Pipe %c stat:         %08x\n",
				   pipe_name(pipe),
				   I915_READ(PIPESTAT(pipe)));
	} else {
		seq_printf(m, "North Display Interrupt enable:		%08x\n",
			   I915_READ(DEIER));
		seq_printf(m, "North Display Interrupt identity:	%08x\n",
			   I915_READ(DEIIR));
		seq_printf(m, "North Display Interrupt mask:		%08x\n",
			   I915_READ(DEIMR));
		seq_printf(m, "South Display Interrupt enable:		%08x\n",
			   I915_READ(SDEIER));
		seq_printf(m, "South Display Interrupt identity:	%08x\n",
			   I915_READ(SDEIIR));
		seq_printf(m, "South Display Interrupt mask:		%08x\n",
			   I915_READ(SDEIMR));
		seq_printf(m, "Graphics Interrupt enable:		%08x\n",
			   I915_READ(GTIER));
		seq_printf(m, "Graphics Interrupt identity:		%08x\n",
			   I915_READ(GTIIR));
		seq_printf(m, "Graphics Interrupt mask:		%08x\n",
			   I915_READ(GTIMR));
	}
	for_each_ring(ring, dev_priv, i) {
		if (INTEL_INFO(dev)->gen >= 6) {
			seq_printf(m,
				   "Graphics Interrupt mask (%s):	%08x\n",
				   ring->name, I915_READ_IMR(ring));
		}
		i915_ring_seqno_info(m, ring);
	}
	intel_runtime_pm_put(dev_priv);
	mutex_unlock(&dev->struct_mutex);

	return 0;
}

static int i915_gem_fence_regs_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	int i, ret;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	seq_printf(m, "Total fences = %d\n", dev_priv->num_fence_regs);
	for (i = 0; i < dev_priv->num_fence_regs; i++) {
		struct drm_i915_gem_object *obj = dev_priv->fence_regs[i].obj;

		seq_printf(m, "Fence %d, pin count = %d, object = ",
			   i, dev_priv->fence_regs[i].pin_count);
		if (obj == NULL)
			seq_puts(m, "unused");
		else
			describe_obj(m, obj);
		seq_putc(m, '\n');
	}

	mutex_unlock(&dev->struct_mutex);
	return 0;
}

static int i915_hws_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_engine_cs *ring;
	const u32 *hws;
	int i;

	ring = &dev_priv->ring[(uintptr_t)node->info_ent->data];
	hws = ring->status_page.page_addr;
	if (hws == NULL)
		return 0;

	for (i = 0; i < 4096 / sizeof(u32) / 4; i += 4) {
		seq_printf(m, "0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n",
			   i * 4,
			   hws[i], hws[i + 1], hws[i + 2], hws[i + 3]);
	}
	return 0;
}

static ssize_t
i915_error_state_write(struct file *filp,
		       const char __user *ubuf,
		       size_t cnt,
		       loff_t *ppos)
{
	struct i915_error_state_file_priv *error_priv = filp->private_data;
	struct drm_device *dev = error_priv->dev;
	int ret;

	DRM_DEBUG_DRIVER("Resetting error state\n");

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	i915_destroy_error_state(dev);
	mutex_unlock(&dev->struct_mutex);

	return cnt;
}

static int i915_error_state_open(struct inode *inode, struct file *file)
{
	struct drm_device *dev = inode->i_private;
	struct i915_error_state_file_priv *error_priv;

	error_priv = kzalloc(sizeof(*error_priv), GFP_KERNEL);
	if (!error_priv)
		return -ENOMEM;

	error_priv->dev = dev;

	i915_error_state_get(dev, error_priv);

	file->private_data = error_priv;

	return 0;
}

static int i915_error_state_release(struct inode *inode, struct file *file)
{
	struct i915_error_state_file_priv *error_priv = file->private_data;

	i915_error_state_put(error_priv);
	kfree(error_priv);

	return 0;
}

static ssize_t i915_error_state_read(struct file *file, char __user *userbuf,
				     size_t count, loff_t *pos)
{
	struct i915_error_state_file_priv *error_priv = file->private_data;
	struct drm_i915_error_state_buf error_str;
	loff_t tmp_pos = 0;
	ssize_t ret_count = 0;
	int ret;

	ret = i915_error_state_buf_init(&error_str, to_i915(error_priv->dev), count, *pos);
	if (ret)
		return ret;

	ret = i915_error_state_to_str(&error_str, error_priv);
	if (ret)
		goto out;

	ret_count = simple_read_from_buffer(userbuf, count, &tmp_pos,
					    error_str.buf,
					    error_str.bytes);

	if (ret_count < 0)
		ret = ret_count;
	else
		*pos = error_str.start + ret_count;
out:
	i915_error_state_buf_release(&error_str);
	return ret ?: ret_count;
}

static const struct file_operations i915_error_state_fops = {
	.owner = THIS_MODULE,
	.open = i915_error_state_open,
	.read = i915_error_state_read,
	.write = i915_error_state_write,
	.llseek = default_llseek,
	.release = i915_error_state_release,
};

static int
i915_next_seqno_get(void *data, u64 *val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	*val = dev_priv->next_seqno;
	mutex_unlock(&dev->struct_mutex);

	return 0;
}

static int
i915_next_seqno_set(void *data, u64 val)
{
	struct drm_device *dev = data;
	int ret;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	ret = i915_gem_set_seqno(dev, val);
	mutex_unlock(&dev->struct_mutex);

	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(i915_next_seqno_fops,
			i915_next_seqno_get, i915_next_seqno_set,
			"0x%llx\n");

static int i915_frequency_info(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret = 0;

	intel_runtime_pm_get(dev_priv);

	flush_delayed_work(&dev_priv->rps.delayed_resume_work);

	if (IS_GEN5(dev)) {
		u16 rgvswctl = I915_READ16(MEMSWCTL);
		u16 rgvstat = I915_READ16(MEMSTAT_ILK);

		seq_printf(m, "Requested P-state: %d\n", (rgvswctl >> 8) & 0xf);
		seq_printf(m, "Requested VID: %d\n", rgvswctl & 0x3f);
		seq_printf(m, "Current VID: %d\n", (rgvstat & MEMSTAT_VID_MASK) >>
			   MEMSTAT_VID_SHIFT);
		seq_printf(m, "Current P-state: %d\n",
			   (rgvstat & MEMSTAT_PSTATE_MASK) >> MEMSTAT_PSTATE_SHIFT);
	} else if (IS_GEN6(dev) || (IS_GEN7(dev) && !IS_VALLEYVIEW(dev)) ||
		   IS_BROADWELL(dev) || IS_GEN9(dev)) {
		u32 rp_state_limits;
		u32 gt_perf_status;
		u32 rp_state_cap;
		u32 rpmodectl, rpinclimit, rpdeclimit;
		u32 rpstat, cagf, reqf;
		u32 rpupei, rpcurup, rpprevup;
		u32 rpdownei, rpcurdown, rpprevdown;
		u32 pm_ier, pm_imr, pm_isr, pm_iir, pm_mask;
		int max_freq;

		rp_state_limits = I915_READ(GEN6_RP_STATE_LIMITS);
		if (IS_BROXTON(dev)) {
			rp_state_cap = I915_READ(BXT_RP_STATE_CAP);
			gt_perf_status = I915_READ(BXT_GT_PERF_STATUS);
		} else {
			rp_state_cap = I915_READ(GEN6_RP_STATE_CAP);
			gt_perf_status = I915_READ(GEN6_GT_PERF_STATUS);
		}

		/* RPSTAT1 is in the GT power well */
		ret = mutex_lock_interruptible(&dev->struct_mutex);
		if (ret)
			goto out;

		intel_uncore_forcewake_get(dev_priv, FORCEWAKE_ALL);

		reqf = I915_READ(GEN6_RPNSWREQ);
		if (IS_GEN9(dev))
			reqf >>= 23;
		else {
			reqf &= ~GEN6_TURBO_DISABLE;
			if (IS_HASWELL(dev) || IS_BROADWELL(dev))
				reqf >>= 24;
			else
				reqf >>= 25;
		}
		reqf = intel_gpu_freq(dev_priv, reqf);

		rpmodectl = I915_READ(GEN6_RP_CONTROL);
		rpinclimit = I915_READ(GEN6_RP_UP_THRESHOLD);
		rpdeclimit = I915_READ(GEN6_RP_DOWN_THRESHOLD);

		rpstat = I915_READ(GEN6_RPSTAT1);
		rpupei = I915_READ(GEN6_RP_CUR_UP_EI);
		rpcurup = I915_READ(GEN6_RP_CUR_UP);
		rpprevup = I915_READ(GEN6_RP_PREV_UP);
		rpdownei = I915_READ(GEN6_RP_CUR_DOWN_EI);
		rpcurdown = I915_READ(GEN6_RP_CUR_DOWN);
		rpprevdown = I915_READ(GEN6_RP_PREV_DOWN);
		if (IS_GEN9(dev))
			cagf = (rpstat & GEN9_CAGF_MASK) >> GEN9_CAGF_SHIFT;
		else if (IS_HASWELL(dev) || IS_BROADWELL(dev))
			cagf = (rpstat & HSW_CAGF_MASK) >> HSW_CAGF_SHIFT;
		else
			cagf = (rpstat & GEN6_CAGF_MASK) >> GEN6_CAGF_SHIFT;
		cagf = intel_gpu_freq(dev_priv, cagf);

		intel_uncore_forcewake_put(dev_priv, FORCEWAKE_ALL);
		mutex_unlock(&dev->struct_mutex);

		if (IS_GEN6(dev) || IS_GEN7(dev)) {
			pm_ier = I915_READ(GEN6_PMIER);
			pm_imr = I915_READ(GEN6_PMIMR);
			pm_isr = I915_READ(GEN6_PMISR);
			pm_iir = I915_READ(GEN6_PMIIR);
			pm_mask = I915_READ(GEN6_PMINTRMSK);
		} else {
			pm_ier = I915_READ(GEN8_GT_IER(2));
			pm_imr = I915_READ(GEN8_GT_IMR(2));
			pm_isr = I915_READ(GEN8_GT_ISR(2));
			pm_iir = I915_READ(GEN8_GT_IIR(2));
			pm_mask = I915_READ(GEN6_PMINTRMSK);
		}
		seq_printf(m, "PM IER=0x%08x IMR=0x%08x ISR=0x%08x IIR=0x%08x, MASK=0x%08x\n",
			   pm_ier, pm_imr, pm_isr, pm_iir, pm_mask);
		seq_printf(m, "GT_PERF_STATUS: 0x%08x\n", gt_perf_status);
		seq_printf(m, "Render p-state ratio: %d\n",
			   (gt_perf_status & (IS_GEN9(dev) ? 0x1ff00 : 0xff00)) >> 8);
		seq_printf(m, "Render p-state VID: %d\n",
			   gt_perf_status & 0xff);
		seq_printf(m, "Render p-state limit: %d\n",
			   rp_state_limits & 0xff);
		seq_printf(m, "RPSTAT1: 0x%08x\n", rpstat);
		seq_printf(m, "RPMODECTL: 0x%08x\n", rpmodectl);
		seq_printf(m, "RPINCLIMIT: 0x%08x\n", rpinclimit);
		seq_printf(m, "RPDECLIMIT: 0x%08x\n", rpdeclimit);
		seq_printf(m, "RPNSWREQ: %dMHz\n", reqf);
		seq_printf(m, "CAGF: %dMHz\n", cagf);
		seq_printf(m, "RP CUR UP EI: %dus\n", rpupei &
			   GEN6_CURICONT_MASK);
		seq_printf(m, "RP CUR UP: %dus\n", rpcurup &
			   GEN6_CURBSYTAVG_MASK);
		seq_printf(m, "RP PREV UP: %dus\n", rpprevup &
			   GEN6_CURBSYTAVG_MASK);
		seq_printf(m, "Up threshold: %d%%\n",
			   dev_priv->rps.up_threshold);

		seq_printf(m, "RP CUR DOWN EI: %dus\n", rpdownei &
			   GEN6_CURIAVG_MASK);
		seq_printf(m, "RP CUR DOWN: %dus\n", rpcurdown &
			   GEN6_CURBSYTAVG_MASK);
		seq_printf(m, "RP PREV DOWN: %dus\n", rpprevdown &
			   GEN6_CURBSYTAVG_MASK);
		seq_printf(m, "Down threshold: %d%%\n",
			   dev_priv->rps.down_threshold);

		max_freq = (IS_BROXTON(dev) ? rp_state_cap >> 0 :
			    rp_state_cap >> 16) & 0xff;
		max_freq *= (IS_SKYLAKE(dev) ? GEN9_FREQ_SCALER : 1);
		seq_printf(m, "Lowest (RPN) frequency: %dMHz\n",
			   intel_gpu_freq(dev_priv, max_freq));

		max_freq = (rp_state_cap & 0xff00) >> 8;
		max_freq *= (IS_SKYLAKE(dev) ? GEN9_FREQ_SCALER : 1);
		seq_printf(m, "Nominal (RP1) frequency: %dMHz\n",
			   intel_gpu_freq(dev_priv, max_freq));

		max_freq = (IS_BROXTON(dev) ? rp_state_cap >> 16 :
			    rp_state_cap >> 0) & 0xff;
		max_freq *= (IS_SKYLAKE(dev) ? GEN9_FREQ_SCALER : 1);
		seq_printf(m, "Max non-overclocked (RP0) frequency: %dMHz\n",
			   intel_gpu_freq(dev_priv, max_freq));
		seq_printf(m, "Max overclocked frequency: %dMHz\n",
			   intel_gpu_freq(dev_priv, dev_priv->rps.max_freq));

		seq_printf(m, "Current freq: %d MHz\n",
			   intel_gpu_freq(dev_priv, dev_priv->rps.cur_freq));
		seq_printf(m, "Actual freq: %d MHz\n", cagf);
		seq_printf(m, "Idle freq: %d MHz\n",
			   intel_gpu_freq(dev_priv, dev_priv->rps.idle_freq));
		seq_printf(m, "Min freq: %d MHz\n",
			   intel_gpu_freq(dev_priv, dev_priv->rps.min_freq));
		seq_printf(m, "Max freq: %d MHz\n",
			   intel_gpu_freq(dev_priv, dev_priv->rps.max_freq));
		seq_printf(m,
			   "efficient (RPe) frequency: %d MHz\n",
			   intel_gpu_freq(dev_priv, dev_priv->rps.efficient_freq));
	} else if (IS_VALLEYVIEW(dev)) {
		u32 freq_sts;

		mutex_lock(&dev_priv->rps.hw_lock);
		freq_sts = vlv_punit_read(dev_priv, PUNIT_REG_GPU_FREQ_STS);
		seq_printf(m, "PUNIT_REG_GPU_FREQ_STS: 0x%08x\n", freq_sts);
		seq_printf(m, "DDR freq: %d MHz\n", dev_priv->mem_freq);

		seq_printf(m, "actual GPU freq: %d MHz\n",
			   intel_gpu_freq(dev_priv, (freq_sts >> 8) & 0xff));

		seq_printf(m, "current GPU freq: %d MHz\n",
			   intel_gpu_freq(dev_priv, dev_priv->rps.cur_freq));

		seq_printf(m, "max GPU freq: %d MHz\n",
			   intel_gpu_freq(dev_priv, dev_priv->rps.max_freq));

		seq_printf(m, "min GPU freq: %d MHz\n",
			   intel_gpu_freq(dev_priv, dev_priv->rps.min_freq));

		seq_printf(m, "idle GPU freq: %d MHz\n",
			   intel_gpu_freq(dev_priv, dev_priv->rps.idle_freq));

		seq_printf(m,
			   "efficient (RPe) frequency: %d MHz\n",
			   intel_gpu_freq(dev_priv, dev_priv->rps.efficient_freq));
		mutex_unlock(&dev_priv->rps.hw_lock);
	} else {
		seq_puts(m, "no P-state info available\n");
	}

	seq_printf(m, "Current CD clock frequency: %d kHz\n", dev_priv->cdclk_freq);
	seq_printf(m, "Max CD clock frequency: %d kHz\n", dev_priv->max_cdclk_freq);
	seq_printf(m, "Max pixel clock frequency: %d kHz\n", dev_priv->max_dotclk_freq);

out:
	intel_runtime_pm_put(dev_priv);
	return ret;
}

static int i915_hangcheck_info(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_engine_cs *ring;
	u64 acthd[I915_NUM_RINGS];
	u32 seqno[I915_NUM_RINGS];
	int i;

	if (!i915.enable_hangcheck) {
		seq_printf(m, "Hangcheck disabled\n");
		return 0;
	}

	intel_runtime_pm_get(dev_priv);

	for_each_ring(ring, dev_priv, i) {
		seqno[i] = ring->get_seqno(ring, false);
		acthd[i] = intel_ring_get_active_head(ring);
	}

	intel_runtime_pm_put(dev_priv);

	if (delayed_work_pending(&dev_priv->gpu_error.hangcheck_work)) {
		seq_printf(m, "Hangcheck active, fires in %dms\n",
			   jiffies_to_msecs(dev_priv->gpu_error.hangcheck_work.timer.expires -
					    jiffies));
	} else
		seq_printf(m, "Hangcheck inactive\n");

	for_each_ring(ring, dev_priv, i) {
		seq_printf(m, "%s:\n", ring->name);
		seq_printf(m, "\tseqno = %x [current %x]\n",
			   ring->hangcheck.seqno, seqno[i]);
		seq_printf(m, "\tACTHD = 0x%08llx [current 0x%08llx]\n",
			   (long long)ring->hangcheck.acthd,
			   (long long)acthd[i]);
		seq_printf(m, "\tmax ACTHD = 0x%08llx\n",
			   (long long)ring->hangcheck.max_acthd);
		seq_printf(m, "\tscore = %d\n", ring->hangcheck.score);
		seq_printf(m, "\taction = %d\n", ring->hangcheck.action);
	}

	return 0;
}

static int ironlake_drpc_info(struct seq_file *m)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 rgvmodectl, rstdbyctl;
	u16 crstandvid;
	int ret;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;
	intel_runtime_pm_get(dev_priv);

	rgvmodectl = I915_READ(MEMMODECTL);
	rstdbyctl = I915_READ(RSTDBYCTL);
	crstandvid = I915_READ16(CRSTANDVID);

	intel_runtime_pm_put(dev_priv);
	mutex_unlock(&dev->struct_mutex);

	seq_printf(m, "HD boost: %s\n", yesno(rgvmodectl & MEMMODE_BOOST_EN));
	seq_printf(m, "Boost freq: %d\n",
		   (rgvmodectl & MEMMODE_BOOST_FREQ_MASK) >>
		   MEMMODE_BOOST_FREQ_SHIFT);
	seq_printf(m, "HW control enabled: %s\n",
		   yesno(rgvmodectl & MEMMODE_HWIDLE_EN));
	seq_printf(m, "SW control enabled: %s\n",
		   yesno(rgvmodectl & MEMMODE_SWMODE_EN));
	seq_printf(m, "Gated voltage change: %s\n",
		   yesno(rgvmodectl & MEMMODE_RCLK_GATE));
	seq_printf(m, "Starting frequency: P%d\n",
		   (rgvmodectl & MEMMODE_FSTART_MASK) >> MEMMODE_FSTART_SHIFT);
	seq_printf(m, "Max P-state: P%d\n",
		   (rgvmodectl & MEMMODE_FMAX_MASK) >> MEMMODE_FMAX_SHIFT);
	seq_printf(m, "Min P-state: P%d\n", (rgvmodectl & MEMMODE_FMIN_MASK));
	seq_printf(m, "RS1 VID: %d\n", (crstandvid & 0x3f));
	seq_printf(m, "RS2 VID: %d\n", ((crstandvid >> 8) & 0x3f));
	seq_printf(m, "Render standby enabled: %s\n",
		   yesno(!(rstdbyctl & RCX_SW_EXIT)));
	seq_puts(m, "Current RS state: ");
	switch (rstdbyctl & RSX_STATUS_MASK) {
	case RSX_STATUS_ON:
		seq_puts(m, "on\n");
		break;
	case RSX_STATUS_RC1:
		seq_puts(m, "RC1\n");
		break;
	case RSX_STATUS_RC1E:
		seq_puts(m, "RC1E\n");
		break;
	case RSX_STATUS_RS1:
		seq_puts(m, "RS1\n");
		break;
	case RSX_STATUS_RS2:
		seq_puts(m, "RS2 (RC6)\n");
		break;
	case RSX_STATUS_RS3:
		seq_puts(m, "RC3 (RC6+)\n");
		break;
	default:
		seq_puts(m, "unknown\n");
		break;
	}

	return 0;
}

static int i915_forcewake_domains(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_uncore_forcewake_domain *fw_domain;
	int i;

	spin_lock_irq(&dev_priv->uncore.lock);
	for_each_fw_domain(fw_domain, dev_priv, i) {
		seq_printf(m, "%s.wake_count = %u\n",
			   intel_uncore_forcewake_domain_to_str(i),
			   fw_domain->wake_count);
	}
	spin_unlock_irq(&dev_priv->uncore.lock);

	return 0;
}

static int vlv_drpc_info(struct seq_file *m)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 rpmodectl1, rcctl1, pw_status;

	intel_runtime_pm_get(dev_priv);

	pw_status = I915_READ(VLV_GTLC_PW_STATUS);
	rpmodectl1 = I915_READ(GEN6_RP_CONTROL);
	rcctl1 = I915_READ(GEN6_RC_CONTROL);

	intel_runtime_pm_put(dev_priv);

	seq_printf(m, "Video Turbo Mode: %s\n",
		   yesno(rpmodectl1 & GEN6_RP_MEDIA_TURBO));
	seq_printf(m, "Turbo enabled: %s\n",
		   yesno(rpmodectl1 & GEN6_RP_ENABLE));
	seq_printf(m, "HW control enabled: %s\n",
		   yesno(rpmodectl1 & GEN6_RP_ENABLE));
	seq_printf(m, "SW control enabled: %s\n",
		   yesno((rpmodectl1 & GEN6_RP_MEDIA_MODE_MASK) ==
			  GEN6_RP_MEDIA_SW_MODE));
	seq_printf(m, "RC6 Enabled: %s\n",
		   yesno(rcctl1 & (GEN7_RC_CTL_TO_MODE |
					GEN6_RC_CTL_EI_MODE(1))));
	seq_printf(m, "Render Power Well: %s\n",
		   (pw_status & VLV_GTLC_PW_RENDER_STATUS_MASK) ? "Up" : "Down");
	seq_printf(m, "Media Power Well: %s\n",
		   (pw_status & VLV_GTLC_PW_MEDIA_STATUS_MASK) ? "Up" : "Down");

	seq_printf(m, "Render RC6 residency since boot: %u\n",
		   I915_READ(VLV_GT_RENDER_RC6));
	seq_printf(m, "Media RC6 residency since boot: %u\n",
		   I915_READ(VLV_GT_MEDIA_RC6));

	return i915_forcewake_domains(m, NULL);
}

static int gen6_drpc_info(struct seq_file *m)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 rpmodectl1, gt_core_status, rcctl1, rc6vids = 0;
	unsigned forcewake_count;
	int count = 0, ret;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;
	intel_runtime_pm_get(dev_priv);

	spin_lock_irq(&dev_priv->uncore.lock);
	forcewake_count = dev_priv->uncore.fw_domain[FW_DOMAIN_ID_RENDER].wake_count;
	spin_unlock_irq(&dev_priv->uncore.lock);

	if (forcewake_count) {
		seq_puts(m, "RC information inaccurate because somebody "
			    "holds a forcewake reference \n");
	} else {
		/* NB: we cannot use forcewake, else we read the wrong values */
		while (count++ < 50 && (I915_READ_NOTRACE(FORCEWAKE_ACK) & 1))
			udelay(10);
		seq_printf(m, "RC information accurate: %s\n", yesno(count < 51));
	}

	gt_core_status = readl(dev_priv->regs + GEN6_GT_CORE_STATUS);
	trace_i915_reg_rw(false, GEN6_GT_CORE_STATUS, gt_core_status, 4, true);

	rpmodectl1 = I915_READ(GEN6_RP_CONTROL);
	rcctl1 = I915_READ(GEN6_RC_CONTROL);
	mutex_unlock(&dev->struct_mutex);
	mutex_lock(&dev_priv->rps.hw_lock);
	sandybridge_pcode_read(dev_priv, GEN6_PCODE_READ_RC6VIDS, &rc6vids);
	mutex_unlock(&dev_priv->rps.hw_lock);

	intel_runtime_pm_put(dev_priv);

	seq_printf(m, "Video Turbo Mode: %s\n",
		   yesno(rpmodectl1 & GEN6_RP_MEDIA_TURBO));
	seq_printf(m, "HW control enabled: %s\n",
		   yesno(rpmodectl1 & GEN6_RP_ENABLE));
	seq_printf(m, "SW control enabled: %s\n",
		   yesno((rpmodectl1 & GEN6_RP_MEDIA_MODE_MASK) ==
			  GEN6_RP_MEDIA_SW_MODE));
	seq_printf(m, "RC1e Enabled: %s\n",
		   yesno(rcctl1 & GEN6_RC_CTL_RC1e_ENABLE));
	seq_printf(m, "RC6 Enabled: %s\n",
		   yesno(rcctl1 & GEN6_RC_CTL_RC6_ENABLE));
	seq_printf(m, "Deep RC6 Enabled: %s\n",
		   yesno(rcctl1 & GEN6_RC_CTL_RC6p_ENABLE));
	seq_printf(m, "Deepest RC6 Enabled: %s\n",
		   yesno(rcctl1 & GEN6_RC_CTL_RC6pp_ENABLE));
	seq_puts(m, "Current RC state: ");
	switch (gt_core_status & GEN6_RCn_MASK) {
	case GEN6_RC0:
		if (gt_core_status & GEN6_CORE_CPD_STATE_MASK)
			seq_puts(m, "Core Power Down\n");
		else
			seq_puts(m, "on\n");
		break;
	case GEN6_RC3:
		seq_puts(m, "RC3\n");
		break;
	case GEN6_RC6:
		seq_puts(m, "RC6\n");
		break;
	case GEN6_RC7:
		seq_puts(m, "RC7\n");
		break;
	default:
		seq_puts(m, "Unknown\n");
		break;
	}

	seq_printf(m, "Core Power Down: %s\n",
		   yesno(gt_core_status & GEN6_CORE_CPD_STATE_MASK));

	/* Not exactly sure what this is */
	seq_printf(m, "RC6 \"Locked to RPn\" residency since boot: %u\n",
		   I915_READ(GEN6_GT_GFX_RC6_LOCKED));
	seq_printf(m, "RC6 residency since boot: %u\n",
		   I915_READ(GEN6_GT_GFX_RC6));
	seq_printf(m, "RC6+ residency since boot: %u\n",
		   I915_READ(GEN6_GT_GFX_RC6p));
	seq_printf(m, "RC6++ residency since boot: %u\n",
		   I915_READ(GEN6_GT_GFX_RC6pp));

	seq_printf(m, "RC6   voltage: %dmV\n",
		   GEN6_DECODE_RC6_VID(((rc6vids >> 0) & 0xff)));
	seq_printf(m, "RC6+  voltage: %dmV\n",
		   GEN6_DECODE_RC6_VID(((rc6vids >> 8) & 0xff)));
	seq_printf(m, "RC6++ voltage: %dmV\n",
		   GEN6_DECODE_RC6_VID(((rc6vids >> 16) & 0xff)));
	return 0;
}

static int i915_drpc_info(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;

	if (IS_VALLEYVIEW(dev))
		return vlv_drpc_info(m);
	else if (INTEL_INFO(dev)->gen >= 6)
		return gen6_drpc_info(m);
	else
		return ironlake_drpc_info(m);
}

static int i915_frontbuffer_tracking(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	seq_printf(m, "FB tracking busy bits: 0x%08x\n",
		   dev_priv->fb_tracking.busy_bits);

	seq_printf(m, "FB tracking flip bits: 0x%08x\n",
		   dev_priv->fb_tracking.flip_bits);

	return 0;
}

static int i915_fbc_status(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (!HAS_FBC(dev)) {
		seq_puts(m, "FBC unsupported on this chipset\n");
		return 0;
	}

	intel_runtime_pm_get(dev_priv);
	mutex_lock(&dev_priv->fbc.lock);

	if (intel_fbc_enabled(dev_priv))
		seq_puts(m, "FBC enabled\n");
	else
		seq_printf(m, "FBC disabled: %s\n",
			  intel_no_fbc_reason_str(dev_priv->fbc.no_fbc_reason));

	if (INTEL_INFO(dev_priv)->gen >= 7)
		seq_printf(m, "Compressing: %s\n",
			   yesno(I915_READ(FBC_STATUS2) &
				 FBC_COMPRESSION_MASK));

	mutex_unlock(&dev_priv->fbc.lock);
	intel_runtime_pm_put(dev_priv);

	return 0;
}

static int i915_fbc_fc_get(void *data, u64 *val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (INTEL_INFO(dev)->gen < 7 || !HAS_FBC(dev))
		return -ENODEV;

	*val = dev_priv->fbc.false_color;

	return 0;
}

static int i915_fbc_fc_set(void *data, u64 val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 reg;

	if (INTEL_INFO(dev)->gen < 7 || !HAS_FBC(dev))
		return -ENODEV;

	mutex_lock(&dev_priv->fbc.lock);

	reg = I915_READ(ILK_DPFC_CONTROL);
	dev_priv->fbc.false_color = val;

	I915_WRITE(ILK_DPFC_CONTROL, val ?
		   (reg | FBC_CTL_FALSE_COLOR) :
		   (reg & ~FBC_CTL_FALSE_COLOR));

	mutex_unlock(&dev_priv->fbc.lock);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(i915_fbc_fc_fops,
			i915_fbc_fc_get, i915_fbc_fc_set,
			"%llu\n");

static int i915_ips_status(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (!HAS_IPS(dev)) {
		seq_puts(m, "not supported\n");
		return 0;
	}

	intel_runtime_pm_get(dev_priv);

	seq_printf(m, "Enabled by kernel parameter: %s\n",
		   yesno(i915.enable_ips));

	if (INTEL_INFO(dev)->gen >= 8) {
		seq_puts(m, "Currently: unknown\n");
	} else {
		if (I915_READ(IPS_CTL) & IPS_ENABLE)
			seq_puts(m, "Currently: enabled\n");
		else
			seq_puts(m, "Currently: disabled\n");
	}

	intel_runtime_pm_put(dev_priv);

	return 0;
}

static int i915_sr_status(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	bool sr_enabled = false;

	intel_runtime_pm_get(dev_priv);

	if (HAS_PCH_SPLIT(dev))
		sr_enabled = I915_READ(WM1_LP_ILK) & WM1_LP_SR_EN;
	else if (IS_CRESTLINE(dev) || IS_G4X(dev) ||
		 IS_I945G(dev) || IS_I945GM(dev))
		sr_enabled = I915_READ(FW_BLC_SELF) & FW_BLC_SELF_EN;
	else if (IS_I915GM(dev))
		sr_enabled = I915_READ(INSTPM) & INSTPM_SELF_EN;
	else if (IS_PINEVIEW(dev))
		sr_enabled = I915_READ(DSPFW3) & PINEVIEW_SELF_REFRESH_EN;
	else if (IS_VALLEYVIEW(dev))
		sr_enabled = I915_READ(FW_BLC_SELF_VLV) & FW_CSPWRDWNEN;

	intel_runtime_pm_put(dev_priv);

	seq_printf(m, "self-refresh: %s\n",
		   sr_enabled ? "enabled" : "disabled");

	return 0;
}

static int i915_emon_status(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	unsigned long temp, chipset, gfx;
	int ret;

	if (!IS_GEN5(dev))
		return -ENODEV;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	temp = i915_mch_val(dev_priv);
	chipset = i915_chipset_val(dev_priv);
	gfx = i915_gfx_val(dev_priv);
	mutex_unlock(&dev->struct_mutex);

	seq_printf(m, "GMCH temp: %ld\n", temp);
	seq_printf(m, "Chipset power: %ld\n", chipset);
	seq_printf(m, "GFX power: %ld\n", gfx);
	seq_printf(m, "Total power: %ld\n", chipset + gfx);

	return 0;
}

static int i915_ring_freq_table(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret = 0;
	int gpu_freq, ia_freq;
	unsigned int max_gpu_freq, min_gpu_freq;

	if (!HAS_CORE_RING_FREQ(dev)) {
		seq_puts(m, "unsupported on this chipset\n");
		return 0;
	}

	intel_runtime_pm_get(dev_priv);

	flush_delayed_work(&dev_priv->rps.delayed_resume_work);

	ret = mutex_lock_interruptible(&dev_priv->rps.hw_lock);
	if (ret)
		goto out;

	if (IS_SKYLAKE(dev)) {
		/* Convert GT frequency to 50 HZ units */
		min_gpu_freq =
			dev_priv->rps.min_freq_softlimit / GEN9_FREQ_SCALER;
		max_gpu_freq =
			dev_priv->rps.max_freq_softlimit / GEN9_FREQ_SCALER;
	} else {
		min_gpu_freq = dev_priv->rps.min_freq_softlimit;
		max_gpu_freq = dev_priv->rps.max_freq_softlimit;
	}

	seq_puts(m, "GPU freq (MHz)\tEffective CPU freq (MHz)\tEffective Ring freq (MHz)\n");

	for (gpu_freq = min_gpu_freq; gpu_freq <= max_gpu_freq; gpu_freq++) {
		ia_freq = gpu_freq;
		sandybridge_pcode_read(dev_priv,
				       GEN6_PCODE_READ_MIN_FREQ_TABLE,
				       &ia_freq);
		seq_printf(m, "%d\t\t%d\t\t\t\t%d\n",
			   intel_gpu_freq(dev_priv, (gpu_freq *
				(IS_SKYLAKE(dev) ? GEN9_FREQ_SCALER : 1))),
			   ((ia_freq >> 0) & 0xff) * 100,
			   ((ia_freq >> 8) & 0xff) * 100);
	}

	mutex_unlock(&dev_priv->rps.hw_lock);

out:
	intel_runtime_pm_put(dev_priv);
	return ret;
}

static int i915_opregion(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_opregion *opregion = &dev_priv->opregion;
	void *data = kmalloc(OPREGION_SIZE, GFP_KERNEL);
	int ret;

	if (data == NULL)
		return -ENOMEM;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		goto out;

	if (opregion->header) {
		memcpy(data, opregion->header, OPREGION_SIZE);
		seq_write(m, data, OPREGION_SIZE);
	}

	mutex_unlock(&dev->struct_mutex);

out:
	kfree(data);
	return 0;
}

static int i915_gem_framebuffer_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct intel_fbdev *ifbdev = NULL;
	struct intel_framebuffer *fb;
	struct drm_framebuffer *drm_fb;

#ifdef CONFIG_DRM_FBDEV_EMULATION
	struct drm_i915_private *dev_priv = dev->dev_private;

	ifbdev = dev_priv->fbdev;
	fb = to_intel_framebuffer(ifbdev->helper.fb);

	seq_printf(m, "fbcon size: %d x %d, depth %d, %d bpp, modifier 0x%llx, refcount %d, obj ",
		   fb->base.width,
		   fb->base.height,
		   fb->base.depth,
		   fb->base.bits_per_pixel,
		   fb->base.modifier[0],
		   atomic_read(&fb->base.refcount.refcount));
	describe_obj(m, fb->obj);
	seq_putc(m, '\n');
#endif

	mutex_lock(&dev->mode_config.fb_lock);
	drm_for_each_fb(drm_fb, dev) {
		fb = to_intel_framebuffer(drm_fb);
		if (ifbdev && &fb->base == ifbdev->helper.fb)
			continue;

		seq_printf(m, "user size: %d x %d, depth %d, %d bpp, modifier 0x%llx, refcount %d, obj ",
			   fb->base.width,
			   fb->base.height,
			   fb->base.depth,
			   fb->base.bits_per_pixel,
			   fb->base.modifier[0],
			   atomic_read(&fb->base.refcount.refcount));
		describe_obj(m, fb->obj);
		seq_putc(m, '\n');
	}
	mutex_unlock(&dev->mode_config.fb_lock);

	return 0;
}

static void describe_ctx_ringbuf(struct seq_file *m,
				 struct intel_ringbuffer *ringbuf)
{
	seq_printf(m, " (ringbuffer, space: %d, head: %u, tail: %u, last head: %d)",
		   ringbuf->space, ringbuf->head, ringbuf->tail,
		   ringbuf->last_retired_head);
}

static int i915_context_status(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_engine_cs *ring;
	struct intel_context *ctx;
	int ret, i;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	list_for_each_entry(ctx, &dev_priv->context_list, link) {
		if (!i915.enable_execlists &&
		    ctx->legacy_hw_ctx.rcs_state == NULL)
			continue;

		seq_puts(m, "HW context ");
		describe_ctx(m, ctx);
		for_each_ring(ring, dev_priv, i) {
			if (ring->default_context == ctx)
				seq_printf(m, "(default context %s) ",
					   ring->name);
		}

		if (i915.enable_execlists) {
			seq_putc(m, '\n');
			for_each_ring(ring, dev_priv, i) {
				struct drm_i915_gem_object *ctx_obj =
					ctx->engine[i].state;
				struct intel_ringbuffer *ringbuf =
					ctx->engine[i].ringbuf;

				seq_printf(m, "%s: ", ring->name);
				if (ctx_obj)
					describe_obj(m, ctx_obj);
				if (ringbuf)
					describe_ctx_ringbuf(m, ringbuf);
				seq_putc(m, '\n');
			}
		} else {
			describe_obj(m, ctx->legacy_hw_ctx.rcs_state);
		}

		seq_putc(m, '\n');
	}

	mutex_unlock(&dev->struct_mutex);

	return 0;
}

static void i915_dump_lrc_obj(struct seq_file *m,
			      struct intel_engine_cs *ring,
			      struct drm_i915_gem_object *ctx_obj)
{
	struct page *page;
	uint32_t *reg_state;
	int j;
	unsigned long ggtt_offset = 0;

	if (ctx_obj == NULL) {
		seq_printf(m, "Context on %s with no gem object\n",
			   ring->name);
		return;
	}

	seq_printf(m, "CONTEXT: %s %u\n", ring->name,
		   intel_execlists_ctx_id(ctx_obj));

	if (!i915_gem_obj_ggtt_bound(ctx_obj))
		seq_puts(m, "\tNot bound in GGTT\n");
	else
		ggtt_offset = i915_gem_obj_ggtt_offset(ctx_obj);

	if (i915_gem_object_get_pages(ctx_obj)) {
		seq_puts(m, "\tFailed to get pages for context object\n");
		return;
	}

	page = i915_gem_object_get_page(ctx_obj, LRC_STATE_PN);
	if (!WARN_ON(page == NULL)) {
		reg_state = kmap_atomic(page);

		for (j = 0; j < 0x600 / sizeof(u32) / 4; j += 4) {
			seq_printf(m, "\t[0x%08lx] 0x%08x 0x%08x 0x%08x 0x%08x\n",
				   ggtt_offset + 4096 + (j * 4),
				   reg_state[j], reg_state[j + 1],
				   reg_state[j + 2], reg_state[j + 3]);
		}
		kunmap_atomic(reg_state);
	}

	seq_putc(m, '\n');
}

static int i915_dump_lrc(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = (struct drm_info_node *) m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_engine_cs *ring;
	struct intel_context *ctx;
	int ret, i;

	if (!i915.enable_execlists) {
		seq_printf(m, "Logical Ring Contexts are disabled\n");
		return 0;
	}

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	list_for_each_entry(ctx, &dev_priv->context_list, link) {
		for_each_ring(ring, dev_priv, i) {
			if (ring->default_context != ctx)
				i915_dump_lrc_obj(m, ring,
						  ctx->engine[i].state);
		}
	}

	mutex_unlock(&dev->struct_mutex);

	return 0;
}

static int i915_execlists(struct seq_file *m, void *data)
{
	struct drm_info_node *node = (struct drm_info_node *)m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_engine_cs *ring;
	u32 status_pointer;
	u8 read_pointer;
	u8 write_pointer;
	u32 status;
	u32 ctx_id;
	struct list_head *cursor;
	int ring_id, i;
	int ret;

	if (!i915.enable_execlists) {
		seq_puts(m, "Logical Ring Contexts are disabled\n");
		return 0;
	}

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	intel_runtime_pm_get(dev_priv);

	for_each_ring(ring, dev_priv, ring_id) {
		struct drm_i915_gem_request *head_req = NULL;
		int count = 0;
		unsigned long flags;

		seq_printf(m, "%s\n", ring->name);

		status = I915_READ(RING_EXECLIST_STATUS_LO(ring));
		ctx_id = I915_READ(RING_EXECLIST_STATUS_HI(ring));
		seq_printf(m, "\tExeclist status: 0x%08X, context: %u\n",
			   status, ctx_id);

		status_pointer = I915_READ(RING_CONTEXT_STATUS_PTR(ring));
		seq_printf(m, "\tStatus pointer: 0x%08X\n", status_pointer);

		read_pointer = ring->next_context_status_buffer;
		write_pointer = status_pointer & 0x07;
		if (read_pointer > write_pointer)
			write_pointer += 6;
		seq_printf(m, "\tRead pointer: 0x%08X, write pointer 0x%08X\n",
			   read_pointer, write_pointer);

		for (i = 0; i < 6; i++) {
			status = I915_READ(RING_CONTEXT_STATUS_BUF_LO(ring, i));
			ctx_id = I915_READ(RING_CONTEXT_STATUS_BUF_HI(ring, i));

			seq_printf(m, "\tStatus buffer %d: 0x%08X, context: %u\n",
				   i, status, ctx_id);
		}

		spin_lock_irqsave(&ring->execlist_lock, flags);
		list_for_each(cursor, &ring->execlist_queue)
			count++;
		head_req = list_first_entry_or_null(&ring->execlist_queue,
				struct drm_i915_gem_request, execlist_link);
		spin_unlock_irqrestore(&ring->execlist_lock, flags);

		seq_printf(m, "\t%d requests in queue\n", count);
		if (head_req) {
			struct drm_i915_gem_object *ctx_obj;

			ctx_obj = head_req->ctx->engine[ring_id].state;
			seq_printf(m, "\tHead request id: %u\n",
				   intel_execlists_ctx_id(ctx_obj));
			seq_printf(m, "\tHead request tail: %u\n",
				   head_req->tail);
		}

		seq_putc(m, '\n');
	}

	intel_runtime_pm_put(dev_priv);
	mutex_unlock(&dev->struct_mutex);

	return 0;
}

static const char *swizzle_string(unsigned swizzle)
{
	switch (swizzle) {
	case I915_BIT_6_SWIZZLE_NONE:
		return "none";
	case I915_BIT_6_SWIZZLE_9:
		return "bit9";
	case I915_BIT_6_SWIZZLE_9_10:
		return "bit9/bit10";
	case I915_BIT_6_SWIZZLE_9_11:
		return "bit9/bit11";
	case I915_BIT_6_SWIZZLE_9_10_11:
		return "bit9/bit10/bit11";
	case I915_BIT_6_SWIZZLE_9_17:
		return "bit9/bit17";
	case I915_BIT_6_SWIZZLE_9_10_17:
		return "bit9/bit10/bit17";
	case I915_BIT_6_SWIZZLE_UNKNOWN:
		return "unknown";
	}

	return "bug";
}

static int i915_swizzle_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;
	intel_runtime_pm_get(dev_priv);

	seq_printf(m, "bit6 swizzle for X-tiling = %s\n",
		   swizzle_string(dev_priv->mm.bit_6_swizzle_x));
	seq_printf(m, "bit6 swizzle for Y-tiling = %s\n",
		   swizzle_string(dev_priv->mm.bit_6_swizzle_y));

	if (IS_GEN3(dev) || IS_GEN4(dev)) {
		seq_printf(m, "DDC = 0x%08x\n",
			   I915_READ(DCC));
		seq_printf(m, "DDC2 = 0x%08x\n",
			   I915_READ(DCC2));
		seq_printf(m, "C0DRB3 = 0x%04x\n",
			   I915_READ16(C0DRB3));
		seq_printf(m, "C1DRB3 = 0x%04x\n",
			   I915_READ16(C1DRB3));
	} else if (INTEL_INFO(dev)->gen >= 6) {
		seq_printf(m, "MAD_DIMM_C0 = 0x%08x\n",
			   I915_READ(MAD_DIMM_C0));
		seq_printf(m, "MAD_DIMM_C1 = 0x%08x\n",
			   I915_READ(MAD_DIMM_C1));
		seq_printf(m, "MAD_DIMM_C2 = 0x%08x\n",
			   I915_READ(MAD_DIMM_C2));
		seq_printf(m, "TILECTL = 0x%08x\n",
			   I915_READ(TILECTL));
		if (INTEL_INFO(dev)->gen >= 8)
			seq_printf(m, "GAMTARBMODE = 0x%08x\n",
				   I915_READ(GAMTARBMODE));
		else
			seq_printf(m, "ARB_MODE = 0x%08x\n",
				   I915_READ(ARB_MODE));
		seq_printf(m, "DISP_ARB_CTL = 0x%08x\n",
			   I915_READ(DISP_ARB_CTL));
	}

	if (dev_priv->quirks & QUIRK_PIN_SWIZZLED_PAGES)
		seq_puts(m, "L-shaped memory detected\n");

	intel_runtime_pm_put(dev_priv);
	mutex_unlock(&dev->struct_mutex);

	return 0;
}

static int per_file_ctx(int id, void *ptr, void *data)
{
	struct intel_context *ctx = ptr;
	struct seq_file *m = data;
	struct i915_hw_ppgtt *ppgtt = ctx->ppgtt;

	if (!ppgtt) {
		seq_printf(m, "  no ppgtt for context %d\n",
			   ctx->user_handle);
		return 0;
	}

	if (i915_gem_context_is_default(ctx))
		seq_puts(m, "  default context:\n");
	else
		seq_printf(m, "  context %d:\n", ctx->user_handle);
	ppgtt->debug_dump(ppgtt, m);

	return 0;
}

static void gen8_ppgtt_info(struct seq_file *m, struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_engine_cs *ring;
	struct i915_hw_ppgtt *ppgtt = dev_priv->mm.aliasing_ppgtt;
	int unused, i;

	if (!ppgtt)
		return;

	for_each_ring(ring, dev_priv, unused) {
		seq_printf(m, "%s\n", ring->name);
		for (i = 0; i < 4; i++) {
			u64 pdp = I915_READ(GEN8_RING_PDP_UDW(ring, i));
			pdp <<= 32;
			pdp |= I915_READ(GEN8_RING_PDP_LDW(ring, i));
			seq_printf(m, "\tPDP%d 0x%016llx\n", i, pdp);
		}
	}
}

static void gen6_ppgtt_info(struct seq_file *m, struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_engine_cs *ring;
	int i;

	if (INTEL_INFO(dev)->gen == 6)
		seq_printf(m, "GFX_MODE: 0x%08x\n", I915_READ(GFX_MODE));

	for_each_ring(ring, dev_priv, i) {
		seq_printf(m, "%s\n", ring->name);
		if (INTEL_INFO(dev)->gen == 7)
			seq_printf(m, "GFX_MODE: 0x%08x\n", I915_READ(RING_MODE_GEN7(ring)));
		seq_printf(m, "PP_DIR_BASE: 0x%08x\n", I915_READ(RING_PP_DIR_BASE(ring)));
		seq_printf(m, "PP_DIR_BASE_READ: 0x%08x\n", I915_READ(RING_PP_DIR_BASE_READ(ring)));
		seq_printf(m, "PP_DIR_DCLV: 0x%08x\n", I915_READ(RING_PP_DIR_DCLV(ring)));
	}
	if (dev_priv->mm.aliasing_ppgtt) {
		struct i915_hw_ppgtt *ppgtt = dev_priv->mm.aliasing_ppgtt;

		seq_puts(m, "aliasing PPGTT:\n");
		seq_printf(m, "pd gtt offset: 0x%08x\n", ppgtt->pd.base.ggtt_offset);

		ppgtt->debug_dump(ppgtt, m);
	}

	seq_printf(m, "ECOCHK: 0x%08x\n", I915_READ(GAM_ECOCHK));
}

static int i915_ppgtt_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_file *file;

	int ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;
	intel_runtime_pm_get(dev_priv);

	if (INTEL_INFO(dev)->gen >= 8)
		gen8_ppgtt_info(m, dev);
	else if (INTEL_INFO(dev)->gen >= 6)
		gen6_ppgtt_info(m, dev);

	list_for_each_entry_reverse(file, &dev->filelist, lhead) {
		struct drm_i915_file_private *file_priv = file->driver_priv;
		struct task_struct *task;

		task = get_pid_task(file->pid, PIDTYPE_PID);
		if (!task) {
			ret = -ESRCH;
			goto out_put;
		}
		seq_printf(m, "\nproc: %s\n", task->comm);
		put_task_struct(task);
		idr_for_each(&file_priv->context_idr, per_file_ctx,
			     (void *)(unsigned long)m);
	}

out_put:
	intel_runtime_pm_put(dev_priv);
	mutex_unlock(&dev->struct_mutex);

	return ret;
}

static int count_irq_waiters(struct drm_i915_private *i915)
{
	struct intel_engine_cs *ring;
	int count = 0;
	int i;

	for_each_ring(ring, i915, i)
		count += ring->irq_refcount;

	return count;
}

static int i915_rps_boost_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_file *file;

	seq_printf(m, "RPS enabled? %d\n", dev_priv->rps.enabled);
	seq_printf(m, "GPU busy? %d\n", dev_priv->mm.busy);
	seq_printf(m, "CPU waiting? %d\n", count_irq_waiters(dev_priv));
	seq_printf(m, "Frequency requested %d; min hard:%d, soft:%d; max soft:%d, hard:%d\n",
		   intel_gpu_freq(dev_priv, dev_priv->rps.cur_freq),
		   intel_gpu_freq(dev_priv, dev_priv->rps.min_freq),
		   intel_gpu_freq(dev_priv, dev_priv->rps.min_freq_softlimit),
		   intel_gpu_freq(dev_priv, dev_priv->rps.max_freq_softlimit),
		   intel_gpu_freq(dev_priv, dev_priv->rps.max_freq));
	spin_lock(&dev_priv->rps.client_lock);
	list_for_each_entry_reverse(file, &dev->filelist, lhead) {
		struct drm_i915_file_private *file_priv = file->driver_priv;
		struct task_struct *task;

		rcu_read_lock();
		task = pid_task(file->pid, PIDTYPE_PID);
		seq_printf(m, "%s [%d]: %d boosts%s\n",
			   task ? task->comm : "<unknown>",
			   task ? task->pid : -1,
			   file_priv->rps.boosts,
			   list_empty(&file_priv->rps.link) ? "" : ", active");
		rcu_read_unlock();
	}
	seq_printf(m, "Semaphore boosts: %d%s\n",
		   dev_priv->rps.semaphores.boosts,
		   list_empty(&dev_priv->rps.semaphores.link) ? "" : ", active");
	seq_printf(m, "MMIO flip boosts: %d%s\n",
		   dev_priv->rps.mmioflips.boosts,
		   list_empty(&dev_priv->rps.mmioflips.link) ? "" : ", active");
	seq_printf(m, "Kernel boosts: %d\n", dev_priv->rps.boosts);
	spin_unlock(&dev_priv->rps.client_lock);

	return 0;
}

static int i915_llc(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	/* Size calculation for LLC is a bit of a pain. Ignore for now. */
	seq_printf(m, "LLC: %s\n", yesno(HAS_LLC(dev)));
	seq_printf(m, "eLLC: %zuMB\n", dev_priv->ellc_size);

	return 0;
}

static int i915_guc_load_status_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_i915_private *dev_priv = node->minor->dev->dev_private;
	struct intel_guc_fw *guc_fw = &dev_priv->guc.guc_fw;
	u32 tmp, i;

	if (!HAS_GUC_UCODE(dev_priv->dev))
		return 0;

	seq_printf(m, "GuC firmware status:\n");
	seq_printf(m, "\tpath: %s\n",
		guc_fw->guc_fw_path);
	seq_printf(m, "\tfetch: %s\n",
		intel_guc_fw_status_repr(guc_fw->guc_fw_fetch_status));
	seq_printf(m, "\tload: %s\n",
		intel_guc_fw_status_repr(guc_fw->guc_fw_load_status));
	seq_printf(m, "\tversion wanted: %d.%d\n",
		guc_fw->guc_fw_major_wanted, guc_fw->guc_fw_minor_wanted);
	seq_printf(m, "\tversion found: %d.%d\n",
		guc_fw->guc_fw_major_found, guc_fw->guc_fw_minor_found);

	tmp = I915_READ(GUC_STATUS);

	seq_printf(m, "\nGuC status 0x%08x:\n", tmp);
	seq_printf(m, "\tBootrom status = 0x%x\n",
		(tmp & GS_BOOTROM_MASK) >> GS_BOOTROM_SHIFT);
	seq_printf(m, "\tuKernel status = 0x%x\n",
		(tmp & GS_UKERNEL_MASK) >> GS_UKERNEL_SHIFT);
	seq_printf(m, "\tMIA Core status = 0x%x\n",
		(tmp & GS_MIA_MASK) >> GS_MIA_SHIFT);
	seq_puts(m, "\nScratch registers:\n");
	for (i = 0; i < 16; i++)
		seq_printf(m, "\t%2d: \t0x%x\n", i, I915_READ(SOFT_SCRATCH(i)));

	return 0;
}

static void i915_guc_client_info(struct seq_file *m,
				 struct drm_i915_private *dev_priv,
				 struct i915_guc_client *client)
{
	struct intel_engine_cs *ring;
	uint64_t tot = 0;
	uint32_t i;

	seq_printf(m, "\tPriority %d, GuC ctx index: %u, PD offset 0x%x\n",
		client->priority, client->ctx_index, client->proc_desc_offset);
	seq_printf(m, "\tDoorbell id %d, offset: 0x%x, cookie 0x%x\n",
		client->doorbell_id, client->doorbell_offset, client->cookie);
	seq_printf(m, "\tWQ size %d, offset: 0x%x, tail %d\n",
		client->wq_size, client->wq_offset, client->wq_tail);

	seq_printf(m, "\tFailed to queue: %u\n", client->q_fail);
	seq_printf(m, "\tFailed doorbell: %u\n", client->b_fail);
	seq_printf(m, "\tLast submission result: %d\n", client->retcode);

	for_each_ring(ring, dev_priv, i) {
		seq_printf(m, "\tSubmissions: %llu %s\n",
				client->submissions[i],
				ring->name);
		tot += client->submissions[i];
	}
	seq_printf(m, "\tTotal: %llu\n", tot);
}

static int i915_guc_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_guc guc;
	struct i915_guc_client client = {};
	struct intel_engine_cs *ring;
	enum intel_ring_id i;
	u64 total = 0;

	if (!HAS_GUC_SCHED(dev_priv->dev))
		return 0;

	/* Take a local copy of the GuC data, so we can dump it at leisure */
	spin_lock(&dev_priv->guc.host2guc_lock);
	guc = dev_priv->guc;
	if (guc.execbuf_client) {
		spin_lock(&guc.execbuf_client->wq_lock);
		client = *guc.execbuf_client;
		spin_unlock(&guc.execbuf_client->wq_lock);
	}
	spin_unlock(&dev_priv->guc.host2guc_lock);

	seq_printf(m, "GuC total action count: %llu\n", guc.action_count);
	seq_printf(m, "GuC action failure count: %u\n", guc.action_fail);
	seq_printf(m, "GuC last action command: 0x%x\n", guc.action_cmd);
	seq_printf(m, "GuC last action status: 0x%x\n", guc.action_status);
	seq_printf(m, "GuC last action error code: %d\n", guc.action_err);

	seq_printf(m, "\nGuC submissions:\n");
	for_each_ring(ring, dev_priv, i) {
		seq_printf(m, "\t%-24s: %10llu, last seqno 0x%08x %9d\n",
			ring->name, guc.submissions[i],
			guc.last_seqno[i], guc.last_seqno[i]);
		total += guc.submissions[i];
	}
	seq_printf(m, "\t%s: %llu\n", "Total", total);

	seq_printf(m, "\nGuC execbuf client @ %p:\n", guc.execbuf_client);
	i915_guc_client_info(m, dev_priv, &client);

	/* Add more as required ... */

	return 0;
}

static int i915_guc_log_dump(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_i915_gem_object *log_obj = dev_priv->guc.log_obj;
	u32 *log;
	int i = 0, pg;

	if (!log_obj)
		return 0;

	for (pg = 0; pg < log_obj->base.size / PAGE_SIZE; pg++) {
		log = kmap_atomic(i915_gem_object_get_page(log_obj, pg));

		for (i = 0; i < PAGE_SIZE / sizeof(u32); i += 4)
			seq_printf(m, "0x%08x 0x%08x 0x%08x 0x%08x\n",
				   *(log + i), *(log + i + 1),
				   *(log + i + 2), *(log + i + 3));

		kunmap_atomic(log);
	}

	seq_putc(m, '\n');

	return 0;
}

static int i915_edp_psr_status(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 psrperf = 0;
	u32 stat[3];
	enum pipe pipe;
	bool enabled = false;

	if (!HAS_PSR(dev)) {
		seq_puts(m, "PSR not supported\n");
		return 0;
	}

	intel_runtime_pm_get(dev_priv);

	mutex_lock(&dev_priv->psr.lock);
	seq_printf(m, "Sink_Support: %s\n", yesno(dev_priv->psr.sink_support));
	seq_printf(m, "Source_OK: %s\n", yesno(dev_priv->psr.source_ok));
	seq_printf(m, "Enabled: %s\n", yesno((bool)dev_priv->psr.enabled));
	seq_printf(m, "Active: %s\n", yesno(dev_priv->psr.active));
	seq_printf(m, "Busy frontbuffer bits: 0x%03x\n",
		   dev_priv->psr.busy_frontbuffer_bits);
	seq_printf(m, "Re-enable work scheduled: %s\n",
		   yesno(work_busy(&dev_priv->psr.work.work)));

	if (HAS_DDI(dev))
		enabled = I915_READ(EDP_PSR_CTL(dev)) & EDP_PSR_ENABLE;
	else {
		for_each_pipe(dev_priv, pipe) {
			stat[pipe] = I915_READ(VLV_PSRSTAT(pipe)) &
				VLV_EDP_PSR_CURR_STATE_MASK;
			if ((stat[pipe] == VLV_EDP_PSR_ACTIVE_NORFB_UP) ||
			    (stat[pipe] == VLV_EDP_PSR_ACTIVE_SF_UPDATE))
				enabled = true;
		}
	}
	seq_printf(m, "HW Enabled & Active bit: %s", yesno(enabled));

	if (!HAS_DDI(dev))
		for_each_pipe(dev_priv, pipe) {
			if ((stat[pipe] == VLV_EDP_PSR_ACTIVE_NORFB_UP) ||
			    (stat[pipe] == VLV_EDP_PSR_ACTIVE_SF_UPDATE))
				seq_printf(m, " pipe %c", pipe_name(pipe));
		}
	seq_puts(m, "\n");

	/* CHV PSR has no kind of performance counter */
	if (HAS_DDI(dev)) {
		psrperf = I915_READ(EDP_PSR_PERF_CNT(dev)) &
			EDP_PSR_PERF_CNT_MASK;

		seq_printf(m, "Performance_Counter: %u\n", psrperf);
	}
	mutex_unlock(&dev_priv->psr.lock);

	intel_runtime_pm_put(dev_priv);
	return 0;
}

static int i915_sink_crc(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct intel_encoder *encoder;
	struct intel_connector *connector;
	struct intel_dp *intel_dp = NULL;
	int ret;
	u8 crc[6];

	drm_modeset_lock_all(dev);
	for_each_intel_connector(dev, connector) {

		if (connector->base.dpms != DRM_MODE_DPMS_ON)
			continue;

		if (!connector->base.encoder)
			continue;

		encoder = to_intel_encoder(connector->base.encoder);
		if (encoder->type != INTEL_OUTPUT_EDP)
			continue;

		intel_dp = enc_to_intel_dp(&encoder->base);

		ret = intel_dp_sink_crc(intel_dp, crc);
		if (ret)
			goto out;

		seq_printf(m, "%02x%02x%02x%02x%02x%02x\n",
			   crc[0], crc[1], crc[2],
			   crc[3], crc[4], crc[5]);
		goto out;
	}
	ret = -ENODEV;
out:
	drm_modeset_unlock_all(dev);
	return ret;
}

static int i915_energy_uJ(struct seq_file *m, void *data)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u64 power;
	u32 units;

	if (INTEL_INFO(dev)->gen < 6)
		return -ENODEV;

	intel_runtime_pm_get(dev_priv);

	rdmsrl(MSR_RAPL_POWER_UNIT, power);
	power = (power & 0x1f00) >> 8;
	units = 1000000 / (1 << power); /* convert to uJ */
	power = I915_READ(MCH_SECP_NRG_STTS);
	power *= units;

	intel_runtime_pm_put(dev_priv);

	seq_printf(m, "%llu", (long long unsigned)power);

	return 0;
}

static int i915_runtime_pm_status(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (!HAS_RUNTIME_PM(dev)) {
		seq_puts(m, "not supported\n");
		return 0;
	}

	seq_printf(m, "GPU idle: %s\n", yesno(!dev_priv->mm.busy));
	seq_printf(m, "IRQs disabled: %s\n",
		   yesno(!intel_irqs_enabled(dev_priv)));
#ifdef CONFIG_PM
	seq_printf(m, "Usage count: %d\n",
		   atomic_read(&dev->dev->power.usage_count));
#else
	seq_printf(m, "Device Power Management (CONFIG_PM) disabled\n");
#endif

	return 0;
}

static const char *power_domain_str(enum intel_display_power_domain domain)
{
	switch (domain) {
	case POWER_DOMAIN_PIPE_A:
		return "PIPE_A";
	case POWER_DOMAIN_PIPE_B:
		return "PIPE_B";
	case POWER_DOMAIN_PIPE_C:
		return "PIPE_C";
	case POWER_DOMAIN_PIPE_A_PANEL_FITTER:
		return "PIPE_A_PANEL_FITTER";
	case POWER_DOMAIN_PIPE_B_PANEL_FITTER:
		return "PIPE_B_PANEL_FITTER";
	case POWER_DOMAIN_PIPE_C_PANEL_FITTER:
		return "PIPE_C_PANEL_FITTER";
	case POWER_DOMAIN_TRANSCODER_A:
		return "TRANSCODER_A";
	case POWER_DOMAIN_TRANSCODER_B:
		return "TRANSCODER_B";
	case POWER_DOMAIN_TRANSCODER_C:
		return "TRANSCODER_C";
	case POWER_DOMAIN_TRANSCODER_EDP:
		return "TRANSCODER_EDP";
	case POWER_DOMAIN_PORT_DDI_A_2_LANES:
		return "PORT_DDI_A_2_LANES";
	case POWER_DOMAIN_PORT_DDI_A_4_LANES:
		return "PORT_DDI_A_4_LANES";
	case POWER_DOMAIN_PORT_DDI_B_2_LANES:
		return "PORT_DDI_B_2_LANES";
	case POWER_DOMAIN_PORT_DDI_B_4_LANES:
		return "PORT_DDI_B_4_LANES";
	case POWER_DOMAIN_PORT_DDI_C_2_LANES:
		return "PORT_DDI_C_2_LANES";
	case POWER_DOMAIN_PORT_DDI_C_4_LANES:
		return "PORT_DDI_C_4_LANES";
	case POWER_DOMAIN_PORT_DDI_D_2_LANES:
		return "PORT_DDI_D_2_LANES";
	case POWER_DOMAIN_PORT_DDI_D_4_LANES:
		return "PORT_DDI_D_4_LANES";
	case POWER_DOMAIN_PORT_DDI_E_2_LANES:
		return "PORT_DDI_E_2_LANES";
	case POWER_DOMAIN_PORT_DSI:
		return "PORT_DSI";
	case POWER_DOMAIN_PORT_CRT:
		return "PORT_CRT";
	case POWER_DOMAIN_PORT_OTHER:
		return "PORT_OTHER";
	case POWER_DOMAIN_VGA:
		return "VGA";
	case POWER_DOMAIN_AUDIO:
		return "AUDIO";
	case POWER_DOMAIN_PLLS:
		return "PLLS";
	case POWER_DOMAIN_AUX_A:
		return "AUX_A";
	case POWER_DOMAIN_AUX_B:
		return "AUX_B";
	case POWER_DOMAIN_AUX_C:
		return "AUX_C";
	case POWER_DOMAIN_AUX_D:
		return "AUX_D";
	case POWER_DOMAIN_GMBUS:
		return "GMBUS";
	case POWER_DOMAIN_INIT:
		return "INIT";
	default:
		MISSING_CASE(domain);
		return "?";
	}
}

static int i915_power_domain_info(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct i915_power_domains *power_domains = &dev_priv->power_domains;
	int i;

	mutex_lock(&power_domains->lock);

	seq_printf(m, "%-25s %s\n", "Power well/domain", "Use count");
	for (i = 0; i < power_domains->power_well_count; i++) {
		struct i915_power_well *power_well;
		enum intel_display_power_domain power_domain;

		power_well = &power_domains->power_wells[i];
		seq_printf(m, "%-25s %d\n", power_well->name,
			   power_well->count);

		for (power_domain = 0; power_domain < POWER_DOMAIN_NUM;
		     power_domain++) {
			if (!(BIT(power_domain) & power_well->domains))
				continue;

			seq_printf(m, "  %-23s %d\n",
				 power_domain_str(power_domain),
				 power_domains->domain_use_count[power_domain]);
		}
	}

	mutex_unlock(&power_domains->lock);

	return 0;
}

static void intel_seq_print_mode(struct seq_file *m, int tabs,
				 struct drm_display_mode *mode)
{
	int i;

	for (i = 0; i < tabs; i++)
		seq_putc(m, '\t');

	seq_printf(m, "id %d:\"%s\" freq %d clock %d hdisp %d hss %d hse %d htot %d vdisp %d vss %d vse %d vtot %d type 0x%x flags 0x%x\n",
		   mode->base.id, mode->name,
		   mode->vrefresh, mode->clock,
		   mode->hdisplay, mode->hsync_start,
		   mode->hsync_end, mode->htotal,
		   mode->vdisplay, mode->vsync_start,
		   mode->vsync_end, mode->vtotal,
		   mode->type, mode->flags);
}

static void intel_encoder_info(struct seq_file *m,
			       struct intel_crtc *intel_crtc,
			       struct intel_encoder *intel_encoder)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_crtc *crtc = &intel_crtc->base;
	struct intel_connector *intel_connector;
	struct drm_encoder *encoder;

	encoder = &intel_encoder->base;
	seq_printf(m, "\tencoder %d: type: %s, connectors:\n",
		   encoder->base.id, encoder->name);
	for_each_connector_on_encoder(dev, encoder, intel_connector) {
		struct drm_connector *connector = &intel_connector->base;
		seq_printf(m, "\t\tconnector %d: type: %s, status: %s",
			   connector->base.id,
			   connector->name,
			   drm_get_connector_status_name(connector->status));
		if (connector->status == connector_status_connected) {
			struct drm_display_mode *mode = &crtc->mode;
			seq_printf(m, ", mode:\n");
			intel_seq_print_mode(m, 2, mode);
		} else {
			seq_putc(m, '\n');
		}
	}
}

static void intel_crtc_info(struct seq_file *m, struct intel_crtc *intel_crtc)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_crtc *crtc = &intel_crtc->base;
	struct intel_encoder *intel_encoder;
	struct drm_plane_state *plane_state = crtc->primary->state;
	struct drm_framebuffer *fb = plane_state->fb;

	if (fb)
		seq_printf(m, "\tfb: %d, pos: %dx%d, size: %dx%d\n",
			   fb->base.id, plane_state->src_x >> 16,
			   plane_state->src_y >> 16, fb->width, fb->height);
	else
		seq_puts(m, "\tprimary plane disabled\n");
	for_each_encoder_on_crtc(dev, crtc, intel_encoder)
		intel_encoder_info(m, intel_crtc, intel_encoder);
}

static void intel_panel_info(struct seq_file *m, struct intel_panel *panel)
{
	struct drm_display_mode *mode = panel->fixed_mode;

	seq_printf(m, "\tfixed mode:\n");
	intel_seq_print_mode(m, 2, mode);
}

static void intel_dp_info(struct seq_file *m,
			  struct intel_connector *intel_connector)
{
	struct intel_encoder *intel_encoder = intel_connector->encoder;
	struct intel_dp *intel_dp = enc_to_intel_dp(&intel_encoder->base);

	seq_printf(m, "\tDPCD rev: %x\n", intel_dp->dpcd[DP_DPCD_REV]);
	seq_printf(m, "\taudio support: %s\n", yesno(intel_dp->has_audio));
	if (intel_encoder->type == INTEL_OUTPUT_EDP)
		intel_panel_info(m, &intel_connector->panel);
}

static void intel_hdmi_info(struct seq_file *m,
			    struct intel_connector *intel_connector)
{
	struct intel_encoder *intel_encoder = intel_connector->encoder;
	struct intel_hdmi *intel_hdmi = enc_to_intel_hdmi(&intel_encoder->base);

	seq_printf(m, "\taudio support: %s\n", yesno(intel_hdmi->has_audio));
}

static void intel_lvds_info(struct seq_file *m,
			    struct intel_connector *intel_connector)
{
	intel_panel_info(m, &intel_connector->panel);
}

static void intel_connector_info(struct seq_file *m,
				 struct drm_connector *connector)
{
	struct intel_connector *intel_connector = to_intel_connector(connector);
	struct intel_encoder *intel_encoder = intel_connector->encoder;
	struct drm_display_mode *mode;

	seq_printf(m, "connector %d: type %s, status: %s\n",
		   connector->base.id, connector->name,
		   drm_get_connector_status_name(connector->status));
	if (connector->status == connector_status_connected) {
		seq_printf(m, "\tname: %s\n", connector->display_info.name);
		seq_printf(m, "\tphysical dimensions: %dx%dmm\n",
			   connector->display_info.width_mm,
			   connector->display_info.height_mm);
		seq_printf(m, "\tsubpixel order: %s\n",
			   drm_get_subpixel_order_name(connector->display_info.subpixel_order));
		seq_printf(m, "\tCEA rev: %d\n",
			   connector->display_info.cea_rev);
	}
	if (intel_encoder) {
		if (intel_encoder->type == INTEL_OUTPUT_DISPLAYPORT ||
		    intel_encoder->type == INTEL_OUTPUT_EDP)
			intel_dp_info(m, intel_connector);
		else if (intel_encoder->type == INTEL_OUTPUT_HDMI)
			intel_hdmi_info(m, intel_connector);
		else if (intel_encoder->type == INTEL_OUTPUT_LVDS)
			intel_lvds_info(m, intel_connector);
	}

	seq_printf(m, "\tmodes:\n");
	list_for_each_entry(mode, &connector->modes, head)
		intel_seq_print_mode(m, 2, mode);
}

static bool cursor_active(struct drm_device *dev, int pipe)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 state;

	if (IS_845G(dev) || IS_I865G(dev))
		state = I915_READ(CURCNTR(PIPE_A)) & CURSOR_ENABLE;
	else
		state = I915_READ(CURCNTR(pipe)) & CURSOR_MODE;

	return state;
}

static bool cursor_position(struct drm_device *dev, int pipe, int *x, int *y)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 pos;

	pos = I915_READ(CURPOS(pipe));

	*x = (pos >> CURSOR_X_SHIFT) & CURSOR_POS_MASK;
	if (pos & (CURSOR_POS_SIGN << CURSOR_X_SHIFT))
		*x = -*x;

	*y = (pos >> CURSOR_Y_SHIFT) & CURSOR_POS_MASK;
	if (pos & (CURSOR_POS_SIGN << CURSOR_Y_SHIFT))
		*y = -*y;

	return cursor_active(dev, pipe);
}

static int i915_display_info(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_crtc *crtc;
	struct drm_connector *connector;

	intel_runtime_pm_get(dev_priv);
	drm_modeset_lock_all(dev);
	seq_printf(m, "CRTC info\n");
	seq_printf(m, "---------\n");
	for_each_intel_crtc(dev, crtc) {
		bool active;
		struct intel_crtc_state *pipe_config;
		int x, y;

		pipe_config = to_intel_crtc_state(crtc->base.state);

		seq_printf(m, "CRTC %d: pipe: %c, active=%s (size=%dx%d)\n",
			   crtc->base.base.id, pipe_name(crtc->pipe),
			   yesno(pipe_config->base.active),
			   pipe_config->pipe_src_w, pipe_config->pipe_src_h);
		if (pipe_config->base.active) {
			intel_crtc_info(m, crtc);

			active = cursor_position(dev, crtc->pipe, &x, &y);
			seq_printf(m, "\tcursor visible? %s, position (%d, %d), size %dx%d, addr 0x%08x, active? %s\n",
				   yesno(crtc->cursor_base),
				   x, y, crtc->base.cursor->state->crtc_w,
				   crtc->base.cursor->state->crtc_h,
				   crtc->cursor_addr, yesno(active));
		}

		seq_printf(m, "\tunderrun reporting: cpu=%s pch=%s \n",
			   yesno(!crtc->cpu_fifo_underrun_disabled),
			   yesno(!crtc->pch_fifo_underrun_disabled));
	}

	seq_printf(m, "\n");
	seq_printf(m, "Connector info\n");
	seq_printf(m, "--------------\n");
	list_for_each_entry(connector, &dev->mode_config.connector_list, head) {
		intel_connector_info(m, connector);
	}
	drm_modeset_unlock_all(dev);
	intel_runtime_pm_put(dev_priv);

	return 0;
}

static int i915_semaphore_status(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = (struct drm_info_node *) m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_engine_cs *ring;
	int num_rings = hweight32(INTEL_INFO(dev)->ring_mask);
	int i, j, ret;

	if (!i915_semaphore_is_enabled(dev)) {
		seq_puts(m, "Semaphores are disabled\n");
		return 0;
	}

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;
	intel_runtime_pm_get(dev_priv);

	if (IS_BROADWELL(dev)) {
		struct page *page;
		uint64_t *seqno;

		page = i915_gem_object_get_page(dev_priv->semaphore_obj, 0);

		seqno = (uint64_t *)kmap_atomic(page);
		for_each_ring(ring, dev_priv, i) {
			uint64_t offset;

			seq_printf(m, "%s\n", ring->name);

			seq_puts(m, "  Last signal:");
			for (j = 0; j < num_rings; j++) {
				offset = i * I915_NUM_RINGS + j;
				seq_printf(m, "0x%08llx (0x%02llx) ",
					   seqno[offset], offset * 8);
			}
			seq_putc(m, '\n');

			seq_puts(m, "  Last wait:  ");
			for (j = 0; j < num_rings; j++) {
				offset = i + (j * I915_NUM_RINGS);
				seq_printf(m, "0x%08llx (0x%02llx) ",
					   seqno[offset], offset * 8);
			}
			seq_putc(m, '\n');

		}
		kunmap_atomic(seqno);
	} else {
		seq_puts(m, "  Last signal:");
		for_each_ring(ring, dev_priv, i)
			for (j = 0; j < num_rings; j++)
				seq_printf(m, "0x%08x\n",
					   I915_READ(ring->semaphore.mbox.signal[j]));
		seq_putc(m, '\n');
	}

	seq_puts(m, "\nSync seqno:\n");
	for_each_ring(ring, dev_priv, i) {
		for (j = 0; j < num_rings; j++) {
			seq_printf(m, "  0x%08x ", ring->semaphore.sync_seqno[j]);
		}
		seq_putc(m, '\n');
	}
	seq_putc(m, '\n');

	intel_runtime_pm_put(dev_priv);
	mutex_unlock(&dev->struct_mutex);
	return 0;
}

static int i915_shared_dplls_info(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = (struct drm_info_node *) m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	int i;

	drm_modeset_lock_all(dev);
	for (i = 0; i < dev_priv->num_shared_dpll; i++) {
		struct intel_shared_dpll *pll = &dev_priv->shared_dplls[i];

		seq_printf(m, "DPLL%i: %s, id: %i\n", i, pll->name, pll->id);
		seq_printf(m, " crtc_mask: 0x%08x, active: %d, on: %s\n",
			   pll->config.crtc_mask, pll->active, yesno(pll->on));
		seq_printf(m, " tracked hardware state:\n");
		seq_printf(m, " dpll:    0x%08x\n", pll->config.hw_state.dpll);
		seq_printf(m, " dpll_md: 0x%08x\n",
			   pll->config.hw_state.dpll_md);
		seq_printf(m, " fp0:     0x%08x\n", pll->config.hw_state.fp0);
		seq_printf(m, " fp1:     0x%08x\n", pll->config.hw_state.fp1);
		seq_printf(m, " wrpll:   0x%08x\n", pll->config.hw_state.wrpll);
	}
	drm_modeset_unlock_all(dev);

	return 0;
}

static int i915_wa_registers(struct seq_file *m, void *unused)
{
	int i;
	int ret;
	struct drm_info_node *node = (struct drm_info_node *) m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	intel_runtime_pm_get(dev_priv);

	seq_printf(m, "Workarounds applied: %d\n", dev_priv->workarounds.count);
	for (i = 0; i < dev_priv->workarounds.count; ++i) {
		u32 addr, mask, value, read;
		bool ok;

		addr = dev_priv->workarounds.reg[i].addr;
		mask = dev_priv->workarounds.reg[i].mask;
		value = dev_priv->workarounds.reg[i].value;
		read = I915_READ(addr);
		ok = (value & mask) == (read & mask);
		seq_printf(m, "0x%X: 0x%08X, mask: 0x%08X, read: 0x%08x, status: %s\n",
			   addr, value, mask, read, ok ? "OK" : "FAIL");
	}

	intel_runtime_pm_put(dev_priv);
	mutex_unlock(&dev->struct_mutex);

	return 0;
}

static int i915_ddb_info(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct skl_ddb_allocation *ddb;
	struct skl_ddb_entry *entry;
	enum pipe pipe;
	int plane;

	if (INTEL_INFO(dev)->gen < 9)
		return 0;

	drm_modeset_lock_all(dev);

	ddb = &dev_priv->wm.skl_hw.ddb;

	seq_printf(m, "%-15s%8s%8s%8s\n", "", "Start", "End", "Size");

	for_each_pipe(dev_priv, pipe) {
		seq_printf(m, "Pipe %c\n", pipe_name(pipe));

		for_each_plane(dev_priv, pipe, plane) {
			entry = &ddb->plane[pipe][plane];
			seq_printf(m, "  Plane%-8d%8u%8u%8u\n", plane + 1,
				   entry->start, entry->end,
				   skl_ddb_entry_size(entry));
		}

		entry = &ddb->plane[pipe][PLANE_CURSOR];
		seq_printf(m, "  %-13s%8u%8u%8u\n", "Cursor", entry->start,
			   entry->end, skl_ddb_entry_size(entry));
	}

	drm_modeset_unlock_all(dev);

	return 0;
}

static void drrs_status_per_crtc(struct seq_file *m,
		struct drm_device *dev, struct intel_crtc *intel_crtc)
{
	struct intel_encoder *intel_encoder;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct i915_drrs *drrs = &dev_priv->drrs;
	int vrefresh = 0;

	for_each_encoder_on_crtc(dev, &intel_crtc->base, intel_encoder) {
		/* Encoder connected on this CRTC */
		switch (intel_encoder->type) {
		case INTEL_OUTPUT_EDP:
			seq_puts(m, "eDP:\n");
			break;
		case INTEL_OUTPUT_DSI:
			seq_puts(m, "DSI:\n");
			break;
		case INTEL_OUTPUT_HDMI:
			seq_puts(m, "HDMI:\n");
			break;
		case INTEL_OUTPUT_DISPLAYPORT:
			seq_puts(m, "DP:\n");
			break;
		default:
			seq_printf(m, "Other encoder (id=%d).\n",
						intel_encoder->type);
			return;
		}
	}

	if (dev_priv->vbt.drrs_type == STATIC_DRRS_SUPPORT)
		seq_puts(m, "\tVBT: DRRS_type: Static");
	else if (dev_priv->vbt.drrs_type == SEAMLESS_DRRS_SUPPORT)
		seq_puts(m, "\tVBT: DRRS_type: Seamless");
	else if (dev_priv->vbt.drrs_type == DRRS_NOT_SUPPORTED)
		seq_puts(m, "\tVBT: DRRS_type: None");
	else
		seq_puts(m, "\tVBT: DRRS_type: FIXME: Unrecognized Value");

	seq_puts(m, "\n\n");

	if (to_intel_crtc_state(intel_crtc->base.state)->has_drrs) {
		struct intel_panel *panel;

		mutex_lock(&drrs->mutex);
		/* DRRS Supported */
		seq_puts(m, "\tDRRS Supported: Yes\n");

		/* disable_drrs() will make drrs->dp NULL */
		if (!drrs->dp) {
			seq_puts(m, "Idleness DRRS: Disabled");
			mutex_unlock(&drrs->mutex);
			return;
		}

		panel = &drrs->dp->attached_connector->panel;
		seq_printf(m, "\t\tBusy_frontbuffer_bits: 0x%X",
					drrs->busy_frontbuffer_bits);

		seq_puts(m, "\n\t\t");
		if (drrs->refresh_rate_type == DRRS_HIGH_RR) {
			seq_puts(m, "DRRS_State: DRRS_HIGH_RR\n");
			vrefresh = panel->fixed_mode->vrefresh;
		} else if (drrs->refresh_rate_type == DRRS_LOW_RR) {
			seq_puts(m, "DRRS_State: DRRS_LOW_RR\n");
			vrefresh = panel->downclock_mode->vrefresh;
		} else {
			seq_printf(m, "DRRS_State: Unknown(%d)\n",
						drrs->refresh_rate_type);
			mutex_unlock(&drrs->mutex);
			return;
		}
		seq_printf(m, "\t\tVrefresh: %d", vrefresh);

		seq_puts(m, "\n\t\t");
		mutex_unlock(&drrs->mutex);
	} else {
		/* DRRS not supported. Print the VBT parameter*/
		seq_puts(m, "\tDRRS Supported : No");
	}
	seq_puts(m, "\n");
}

static int i915_drrs_status(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = m->private;
	struct drm_device *dev = node->minor->dev;
	struct intel_crtc *intel_crtc;
	int active_crtc_cnt = 0;

	for_each_intel_crtc(dev, intel_crtc) {
		drm_modeset_lock(&intel_crtc->base.mutex, NULL);

		if (intel_crtc->base.state->active) {
			active_crtc_cnt++;
			seq_printf(m, "\nCRTC %d:  ", active_crtc_cnt);

			drrs_status_per_crtc(m, dev, intel_crtc);
		}

		drm_modeset_unlock(&intel_crtc->base.mutex);
	}

	if (!active_crtc_cnt)
		seq_puts(m, "No active crtc found\n");

	return 0;
}

struct pipe_crc_info {
	const char *name;
	struct drm_device *dev;
	enum pipe pipe;
};

static int i915_dp_mst_info(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = (struct drm_info_node *) m->private;
	struct drm_device *dev = node->minor->dev;
	struct drm_encoder *encoder;
	struct intel_encoder *intel_encoder;
	struct intel_digital_port *intel_dig_port;
	drm_modeset_lock_all(dev);
	list_for_each_entry(encoder, &dev->mode_config.encoder_list, head) {
		intel_encoder = to_intel_encoder(encoder);
		if (intel_encoder->type != INTEL_OUTPUT_DISPLAYPORT)
			continue;
		intel_dig_port = enc_to_dig_port(encoder);
		if (!intel_dig_port->dp.can_mst)
			continue;

		drm_dp_mst_dump_topology(m, &intel_dig_port->dp.mst_mgr);
	}
	drm_modeset_unlock_all(dev);
	return 0;
}

static int i915_pipe_crc_open(struct inode *inode, struct file *filep)
{
	struct pipe_crc_info *info = inode->i_private;
	struct drm_i915_private *dev_priv = info->dev->dev_private;
	struct intel_pipe_crc *pipe_crc = &dev_priv->pipe_crc[info->pipe];

	if (info->pipe >= INTEL_INFO(info->dev)->num_pipes)
		return -ENODEV;

	spin_lock_irq(&pipe_crc->lock);

	if (pipe_crc->opened) {
		spin_unlock_irq(&pipe_crc->lock);
		return -EBUSY; /* already open */
	}

	pipe_crc->opened = true;
	filep->private_data = inode->i_private;

	spin_unlock_irq(&pipe_crc->lock);

	return 0;
}

static int i915_pipe_crc_release(struct inode *inode, struct file *filep)
{
	struct pipe_crc_info *info = inode->i_private;
	struct drm_i915_private *dev_priv = info->dev->dev_private;
	struct intel_pipe_crc *pipe_crc = &dev_priv->pipe_crc[info->pipe];

	spin_lock_irq(&pipe_crc->lock);
	pipe_crc->opened = false;
	spin_unlock_irq(&pipe_crc->lock);

	return 0;
}

/* (6 fields, 8 chars each, space separated (5) + '\n') */
#define PIPE_CRC_LINE_LEN	(6 * 8 + 5 + 1)
/* account for \'0' */
#define PIPE_CRC_BUFFER_LEN	(PIPE_CRC_LINE_LEN + 1)

static int pipe_crc_data_count(struct intel_pipe_crc *pipe_crc)
{
	assert_spin_locked(&pipe_crc->lock);
	return CIRC_CNT(pipe_crc->head, pipe_crc->tail,
			INTEL_PIPE_CRC_ENTRIES_NR);
}

static ssize_t
i915_pipe_crc_read(struct file *filep, char __user *user_buf, size_t count,
		   loff_t *pos)
{
	struct pipe_crc_info *info = filep->private_data;
	struct drm_device *dev = info->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_pipe_crc *pipe_crc = &dev_priv->pipe_crc[info->pipe];
	char buf[PIPE_CRC_BUFFER_LEN];
	int n_entries;
	ssize_t bytes_read;

	/*
	 * Don't allow user space to provide buffers not big enough to hold
	 * a line of data.
	 */
	if (count < PIPE_CRC_LINE_LEN)
		return -EINVAL;

	if (pipe_crc->source == INTEL_PIPE_CRC_SOURCE_NONE)
		return 0;

	/* nothing to read */
	spin_lock_irq(&pipe_crc->lock);
	while (pipe_crc_data_count(pipe_crc) == 0) {
		int ret;

		if (filep->f_flags & O_NONBLOCK) {
			spin_unlock_irq(&pipe_crc->lock);
			return -EAGAIN;
		}

		ret = wait_event_interruptible_lock_irq(pipe_crc->wq,
				pipe_crc_data_count(pipe_crc), pipe_crc->lock);
		if (ret) {
			spin_unlock_irq(&pipe_crc->lock);
			return ret;
		}
	}

	/* We now have one or more entries to read */
	n_entries = count / PIPE_CRC_LINE_LEN;

	bytes_read = 0;
	while (n_entries > 0) {
		struct intel_pipe_crc_entry *entry =
			&pipe_crc->entries[pipe_crc->tail];
		int ret;

		if (CIRC_CNT(pipe_crc->head, pipe_crc->tail,
			     INTEL_PIPE_CRC_ENTRIES_NR) < 1)
			break;

		BUILD_BUG_ON_NOT_POWER_OF_2(INTEL_PIPE_CRC_ENTRIES_NR);
		pipe_crc->tail = (pipe_crc->tail + 1) & (INTEL_PIPE_CRC_ENTRIES_NR - 1);

		bytes_read += snprintf(buf, PIPE_CRC_BUFFER_LEN,
				       "%8u %8x %8x %8x %8x %8x\n",
				       entry->frame, entry->crc[0],
				       entry->crc[1], entry->crc[2],
				       entry->crc[3], entry->crc[4]);

		spin_unlock_irq(&pipe_crc->lock);

		ret = copy_to_user(user_buf, buf, PIPE_CRC_LINE_LEN);
		if (ret == PIPE_CRC_LINE_LEN)
			return -EFAULT;

		user_buf += PIPE_CRC_LINE_LEN;
		n_entries--;

		spin_lock_irq(&pipe_crc->lock);
	}

	spin_unlock_irq(&pipe_crc->lock);

	return bytes_read;
}

static const struct file_operations i915_pipe_crc_fops = {
	.owner = THIS_MODULE,
	.open = i915_pipe_crc_open,
	.read = i915_pipe_crc_read,
	.release = i915_pipe_crc_release,
};

static struct pipe_crc_info i915_pipe_crc_data[I915_MAX_PIPES] = {
	{
		.name = "i915_pipe_A_crc",
		.pipe = PIPE_A,
	},
	{
		.name = "i915_pipe_B_crc",
		.pipe = PIPE_B,
	},
	{
		.name = "i915_pipe_C_crc",
		.pipe = PIPE_C,
	},
};

static int i915_pipe_crc_create(struct dentry *root, struct drm_minor *minor,
				enum pipe pipe)
{
	struct drm_device *dev = minor->dev;
	struct dentry *ent;
	struct pipe_crc_info *info = &i915_pipe_crc_data[pipe];

	info->dev = dev;
	ent = debugfs_create_file(info->name, S_IRUGO, root, info,
				  &i915_pipe_crc_fops);
	if (!ent)
		return -ENOMEM;

	return drm_add_fake_info_node(minor, ent, info);
}

static const char * const pipe_crc_sources[] = {
	"none",
	"plane1",
	"plane2",
	"pf",
	"pipe",
	"TV",
	"DP-B",
	"DP-C",
	"DP-D",
	"auto",
};

static const char *pipe_crc_source_name(enum intel_pipe_crc_source source)
{
	BUILD_BUG_ON(ARRAY_SIZE(pipe_crc_sources) != INTEL_PIPE_CRC_SOURCE_MAX);
	return pipe_crc_sources[source];
}

static int display_crc_ctl_show(struct seq_file *m, void *data)
{
	struct drm_device *dev = m->private;
	struct drm_i915_private *dev_priv = dev->dev_private;
	int i;

	for (i = 0; i < I915_MAX_PIPES; i++)
		seq_printf(m, "%c %s\n", pipe_name(i),
			   pipe_crc_source_name(dev_priv->pipe_crc[i].source));

	return 0;
}

static int display_crc_ctl_open(struct inode *inode, struct file *file)
{
	struct drm_device *dev = inode->i_private;

	return single_open(file, display_crc_ctl_show, dev);
}

static int i8xx_pipe_crc_ctl_reg(enum intel_pipe_crc_source *source,
				 uint32_t *val)
{
	if (*source == INTEL_PIPE_CRC_SOURCE_AUTO)
		*source = INTEL_PIPE_CRC_SOURCE_PIPE;

	switch (*source) {
	case INTEL_PIPE_CRC_SOURCE_PIPE:
		*val = PIPE_CRC_ENABLE | PIPE_CRC_INCLUDE_BORDER_I8XX;
		break;
	case INTEL_PIPE_CRC_SOURCE_NONE:
		*val = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int i9xx_pipe_crc_auto_source(struct drm_device *dev, enum pipe pipe,
				     enum intel_pipe_crc_source *source)
{
	struct intel_encoder *encoder;
	struct intel_crtc *crtc;
	struct intel_digital_port *dig_port;
	int ret = 0;

	*source = INTEL_PIPE_CRC_SOURCE_PIPE;

	drm_modeset_lock_all(dev);
	for_each_intel_encoder(dev, encoder) {
		if (!encoder->base.crtc)
			continue;

		crtc = to_intel_crtc(encoder->base.crtc);

		if (crtc->pipe != pipe)
			continue;

		switch (encoder->type) {
		case INTEL_OUTPUT_TVOUT:
			*source = INTEL_PIPE_CRC_SOURCE_TV;
			break;
		case INTEL_OUTPUT_DISPLAYPORT:
		case INTEL_OUTPUT_EDP:
			dig_port = enc_to_dig_port(&encoder->base);
			switch (dig_port->port) {
			case PORT_B:
				*source = INTEL_PIPE_CRC_SOURCE_DP_B;
				break;
			case PORT_C:
				*source = INTEL_PIPE_CRC_SOURCE_DP_C;
				break;
			case PORT_D:
				*source = INTEL_PIPE_CRC_SOURCE_DP_D;
				break;
			default:
				WARN(1, "nonexisting DP port %c\n",
				     port_name(dig_port->port));
				break;
			}
			break;
		default:
			break;
		}
	}
	drm_modeset_unlock_all(dev);

	return ret;
}

static int vlv_pipe_crc_ctl_reg(struct drm_device *dev,
				enum pipe pipe,
				enum intel_pipe_crc_source *source,
				uint32_t *val)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	bool need_stable_symbols = false;

	if (*source == INTEL_PIPE_CRC_SOURCE_AUTO) {
		int ret = i9xx_pipe_crc_auto_source(dev, pipe, source);
		if (ret)
			return ret;
	}

	switch (*source) {
	case INTEL_PIPE_CRC_SOURCE_PIPE:
		*val = PIPE_CRC_ENABLE | PIPE_CRC_SOURCE_PIPE_VLV;
		break;
	case INTEL_PIPE_CRC_SOURCE_DP_B:
		*val = PIPE_CRC_ENABLE | PIPE_CRC_SOURCE_DP_B_VLV;
		need_stable_symbols = true;
		break;
	case INTEL_PIPE_CRC_SOURCE_DP_C:
		*val = PIPE_CRC_ENABLE | PIPE_CRC_SOURCE_DP_C_VLV;
		need_stable_symbols = true;
		break;
	case INTEL_PIPE_CRC_SOURCE_DP_D:
		if (!IS_CHERRYVIEW(dev))
			return -EINVAL;
		*val = PIPE_CRC_ENABLE | PIPE_CRC_SOURCE_DP_D_VLV;
		need_stable_symbols = true;
		break;
	case INTEL_PIPE_CRC_SOURCE_NONE:
		*val = 0;
		break;
	default:
		return -EINVAL;
	}

	/*
	 * When the pipe CRC tap point is after the transcoders we need
	 * to tweak symbol-level features to produce a deterministic series of
	 * symbols for a given frame. We need to reset those features only once
	 * a frame (instead of every nth symbol):
	 *   - DC-balance: used to ensure a better clock recovery from the data
	 *     link (SDVO)
	 *   - DisplayPort scrambling: used for EMI reduction
	 */
	if (need_stable_symbols) {
		uint32_t tmp = I915_READ(PORT_DFT2_G4X);

		tmp |= DC_BALANCE_RESET_VLV;
		switch (pipe) {
		case PIPE_A:
			tmp |= PIPE_A_SCRAMBLE_RESET;
			break;
		case PIPE_B:
			tmp |= PIPE_B_SCRAMBLE_RESET;
			break;
		case PIPE_C:
			tmp |= PIPE_C_SCRAMBLE_RESET;
			break;
		default:
			return -EINVAL;
		}
		I915_WRITE(PORT_DFT2_G4X, tmp);
	}

	return 0;
}

static int i9xx_pipe_crc_ctl_reg(struct drm_device *dev,
				 enum pipe pipe,
				 enum intel_pipe_crc_source *source,
				 uint32_t *val)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	bool need_stable_symbols = false;

	if (*source == INTEL_PIPE_CRC_SOURCE_AUTO) {
		int ret = i9xx_pipe_crc_auto_source(dev, pipe, source);
		if (ret)
			return ret;
	}

	switch (*source) {
	case INTEL_PIPE_CRC_SOURCE_PIPE:
		*val = PIPE_CRC_ENABLE | PIPE_CRC_SOURCE_PIPE_I9XX;
		break;
	case INTEL_PIPE_CRC_SOURCE_TV:
		if (!SUPPORTS_TV(dev))
			return -EINVAL;
		*val = PIPE_CRC_ENABLE | PIPE_CRC_SOURCE_TV_PRE;
		break;
	case INTEL_PIPE_CRC_SOURCE_DP_B:
		if (!IS_G4X(dev))
			return -EINVAL;
		*val = PIPE_CRC_ENABLE | PIPE_CRC_SOURCE_DP_B_G4X;
		need_stable_symbols = true;
		break;
	case INTEL_PIPE_CRC_SOURCE_DP_C:
		if (!IS_G4X(dev))
			return -EINVAL;
		*val = PIPE_CRC_ENABLE | PIPE_CRC_SOURCE_DP_C_G4X;
		need_stable_symbols = true;
		break;
	case INTEL_PIPE_CRC_SOURCE_DP_D:
		if (!IS_G4X(dev))
			return -EINVAL;
		*val = PIPE_CRC_ENABLE | PIPE_CRC_SOURCE_DP_D_G4X;
		need_stable_symbols = true;
		break;
	case INTEL_PIPE_CRC_SOURCE_NONE:
		*val = 0;
		break;
	default:
		return -EINVAL;
	}

	/*
	 * When the pipe CRC tap point is after the transcoders we need
	 * to tweak symbol-level features to produce a deterministic series of
	 * symbols for a given frame. We need to reset those features only once
	 * a frame (instead of every nth symbol):
	 *   - DC-balance: used to ensure a better clock recovery from the data
	 *     link (SDVO)
	 *   - DisplayPort scrambling: used for EMI reduction
	 */
	if (need_stable_symbols) {
		uint32_t tmp = I915_READ(PORT_DFT2_G4X);

		WARN_ON(!IS_G4X(dev));

		I915_WRITE(PORT_DFT_I9XX,
			   I915_READ(PORT_DFT_I9XX) | DC_BALANCE_RESET);

		if (pipe == PIPE_A)
			tmp |= PIPE_A_SCRAMBLE_RESET;
		else
			tmp |= PIPE_B_SCRAMBLE_RESET;

		I915_WRITE(PORT_DFT2_G4X, tmp);
	}

	return 0;
}

static void vlv_undo_pipe_scramble_reset(struct drm_device *dev,
					 enum pipe pipe)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	uint32_t tmp = I915_READ(PORT_DFT2_G4X);

	switch (pipe) {
	case PIPE_A:
		tmp &= ~PIPE_A_SCRAMBLE_RESET;
		break;
	case PIPE_B:
		tmp &= ~PIPE_B_SCRAMBLE_RESET;
		break;
	case PIPE_C:
		tmp &= ~PIPE_C_SCRAMBLE_RESET;
		break;
	default:
		return;
	}
	if (!(tmp & PIPE_SCRAMBLE_RESET_MASK))
		tmp &= ~DC_BALANCE_RESET_VLV;
	I915_WRITE(PORT_DFT2_G4X, tmp);

}

static void g4x_undo_pipe_scramble_reset(struct drm_device *dev,
					 enum pipe pipe)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	uint32_t tmp = I915_READ(PORT_DFT2_G4X);

	if (pipe == PIPE_A)
		tmp &= ~PIPE_A_SCRAMBLE_RESET;
	else
		tmp &= ~PIPE_B_SCRAMBLE_RESET;
	I915_WRITE(PORT_DFT2_G4X, tmp);

	if (!(tmp & PIPE_SCRAMBLE_RESET_MASK)) {
		I915_WRITE(PORT_DFT_I9XX,
			   I915_READ(PORT_DFT_I9XX) & ~DC_BALANCE_RESET);
	}
}

static int ilk_pipe_crc_ctl_reg(enum intel_pipe_crc_source *source,
				uint32_t *val)
{
	if (*source == INTEL_PIPE_CRC_SOURCE_AUTO)
		*source = INTEL_PIPE_CRC_SOURCE_PIPE;

	switch (*source) {
	case INTEL_PIPE_CRC_SOURCE_PLANE1:
		*val = PIPE_CRC_ENABLE | PIPE_CRC_SOURCE_PRIMARY_ILK;
		break;
	case INTEL_PIPE_CRC_SOURCE_PLANE2:
		*val = PIPE_CRC_ENABLE | PIPE_CRC_SOURCE_SPRITE_ILK;
		break;
	case INTEL_PIPE_CRC_SOURCE_PIPE:
		*val = PIPE_CRC_ENABLE | PIPE_CRC_SOURCE_PIPE_ILK;
		break;
	case INTEL_PIPE_CRC_SOURCE_NONE:
		*val = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void hsw_trans_edp_pipe_A_crc_wa(struct drm_device *dev, bool enable)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_crtc *crtc =
		to_intel_crtc(dev_priv->pipe_to_crtc_mapping[PIPE_A]);
	struct intel_crtc_state *pipe_config;
	struct drm_atomic_state *state;
	int ret = 0;

	drm_modeset_lock_all(dev);
	state = drm_atomic_state_alloc(dev);
	if (!state) {
		ret = -ENOMEM;
		goto out;
	}

	state->acquire_ctx = drm_modeset_legacy_acquire_ctx(&crtc->base);
	pipe_config = intel_atomic_get_crtc_state(state, crtc);
	if (IS_ERR(pipe_config)) {
		ret = PTR_ERR(pipe_config);
		goto out;
	}

	pipe_config->pch_pfit.force_thru = enable;
	if (pipe_config->cpu_transcoder == TRANSCODER_EDP &&
	    pipe_config->pch_pfit.enabled != enable)
		pipe_config->base.connectors_changed = true;

	ret = drm_atomic_commit(state);
out:
	drm_modeset_unlock_all(dev);
	WARN(ret, "Toggling workaround to %i returns %i\n", enable, ret);
	if (ret)
		drm_atomic_state_free(state);
}

static int ivb_pipe_crc_ctl_reg(struct drm_device *dev,
				enum pipe pipe,
				enum intel_pipe_crc_source *source,
				uint32_t *val)
{
	if (*source == INTEL_PIPE_CRC_SOURCE_AUTO)
		*source = INTEL_PIPE_CRC_SOURCE_PF;

	switch (*source) {
	case INTEL_PIPE_CRC_SOURCE_PLANE1:
		*val = PIPE_CRC_ENABLE | PIPE_CRC_SOURCE_PRIMARY_IVB;
		break;
	case INTEL_PIPE_CRC_SOURCE_PLANE2:
		*val = PIPE_CRC_ENABLE | PIPE_CRC_SOURCE_SPRITE_IVB;
		break;
	case INTEL_PIPE_CRC_SOURCE_PF:
		if (IS_HASWELL(dev) && pipe == PIPE_A)
			hsw_trans_edp_pipe_A_crc_wa(dev, true);

		*val = PIPE_CRC_ENABLE | PIPE_CRC_SOURCE_PF_IVB;
		break;
	case INTEL_PIPE_CRC_SOURCE_NONE:
		*val = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int pipe_crc_set_source(struct drm_device *dev, enum pipe pipe,
			       enum intel_pipe_crc_source source)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_pipe_crc *pipe_crc = &dev_priv->pipe_crc[pipe];
	struct intel_crtc *crtc = to_intel_crtc(intel_get_crtc_for_pipe(dev,
									pipe));
	u32 val = 0; /* shut up gcc */
	int ret;

	if (pipe_crc->source == source)
		return 0;

	/* forbid changing the source without going back to 'none' */
	if (pipe_crc->source && source)
		return -EINVAL;

	if (!intel_display_power_is_enabled(dev_priv, POWER_DOMAIN_PIPE(pipe))) {
		DRM_DEBUG_KMS("Trying to capture CRC while pipe is off\n");
		return -EIO;
	}

	if (IS_GEN2(dev))
		ret = i8xx_pipe_crc_ctl_reg(&source, &val);
	else if (INTEL_INFO(dev)->gen < 5)
		ret = i9xx_pipe_crc_ctl_reg(dev, pipe, &source, &val);
	else if (IS_VALLEYVIEW(dev))
		ret = vlv_pipe_crc_ctl_reg(dev, pipe, &source, &val);
	else if (IS_GEN5(dev) || IS_GEN6(dev))
		ret = ilk_pipe_crc_ctl_reg(&source, &val);
	else
		ret = ivb_pipe_crc_ctl_reg(dev, pipe, &source, &val);

	if (ret != 0)
		return ret;

	/* none -> real source transition */
	if (source) {
		struct intel_pipe_crc_entry *entries;

		DRM_DEBUG_DRIVER("collecting CRCs for pipe %c, %s\n",
				 pipe_name(pipe), pipe_crc_source_name(source));

		entries = kcalloc(INTEL_PIPE_CRC_ENTRIES_NR,
				  sizeof(pipe_crc->entries[0]),
				  GFP_KERNEL);
		if (!entries)
			return -ENOMEM;

		/*
		 * When IPS gets enabled, the pipe CRC changes. Since IPS gets
		 * enabled and disabled dynamically based on package C states,
		 * user space can't make reliable use of the CRCs, so let's just
		 * completely disable it.
		 */
		hsw_disable_ips(crtc);

		spin_lock_irq(&pipe_crc->lock);
		kfree(pipe_crc->entries);
		pipe_crc->entries = entries;
		pipe_crc->head = 0;
		pipe_crc->tail = 0;
		spin_unlock_irq(&pipe_crc->lock);
	}

	pipe_crc->source = source;

	I915_WRITE(PIPE_CRC_CTL(pipe), val);
	POSTING_READ(PIPE_CRC_CTL(pipe));

	/* real source -> none transition */
	if (source == INTEL_PIPE_CRC_SOURCE_NONE) {
		struct intel_pipe_crc_entry *entries;
		struct intel_crtc *crtc =
			to_intel_crtc(dev_priv->pipe_to_crtc_mapping[pipe]);

		DRM_DEBUG_DRIVER("stopping CRCs for pipe %c\n",
				 pipe_name(pipe));

		drm_modeset_lock(&crtc->base.mutex, NULL);
		if (crtc->base.state->active)
			intel_wait_for_vblank(dev, pipe);
		drm_modeset_unlock(&crtc->base.mutex);

		spin_lock_irq(&pipe_crc->lock);
		entries = pipe_crc->entries;
		pipe_crc->entries = NULL;
		pipe_crc->head = 0;
		pipe_crc->tail = 0;
		spin_unlock_irq(&pipe_crc->lock);

		kfree(entries);

		if (IS_G4X(dev))
			g4x_undo_pipe_scramble_reset(dev, pipe);
		else if (IS_VALLEYVIEW(dev))
			vlv_undo_pipe_scramble_reset(dev, pipe);
		else if (IS_HASWELL(dev) && pipe == PIPE_A)
			hsw_trans_edp_pipe_A_crc_wa(dev, false);

		hsw_enable_ips(crtc);
	}

	return 0;
}

/*
 * Parse pipe CRC command strings:
 *   command: wsp* object wsp+ name wsp+ source wsp*
 *   object: 'pipe'
 *   name: (A | B | C)
 *   source: (none | plane1 | plane2 | pf)
 *   wsp: (#0x20 | #0x9 | #0xA)+
 *
 * eg.:
 *  "pipe A plane1"  ->  Start CRC computations on plane1 of pipe A
 *  "pipe A none"    ->  Stop CRC
 */
static int display_crc_ctl_tokenize(char *buf, char *words[], int max_words)
{
	int n_words = 0;

	while (*buf) {
		char *end;

		/* skip leading white space */
		buf = skip_spaces(buf);
		if (!*buf)
			break;	/* end of buffer */

		/* find end of word */
		for (end = buf; *end && !isspace(*end); end++)
			;

		if (n_words == max_words) {
			DRM_DEBUG_DRIVER("too many words, allowed <= %d\n",
					 max_words);
			return -EINVAL;	/* ran out of words[] before bytes */
		}

		if (*end)
			*end++ = '\0';
		words[n_words++] = buf;
		buf = end;
	}

	return n_words;
}

enum intel_pipe_crc_object {
	PIPE_CRC_OBJECT_PIPE,
};

static const char * const pipe_crc_objects[] = {
	"pipe",
};

static int
display_crc_ctl_parse_object(const char *buf, enum intel_pipe_crc_object *o)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pipe_crc_objects); i++)
		if (!strcmp(buf, pipe_crc_objects[i])) {
			*o = i;
			return 0;
		    }

	return -EINVAL;
}

static int display_crc_ctl_parse_pipe(const char *buf, enum pipe *pipe)
{
	const char name = buf[0];

	if (name < 'A' || name >= pipe_name(I915_MAX_PIPES))
		return -EINVAL;

	*pipe = name - 'A';

	return 0;
}

static int
display_crc_ctl_parse_source(const char *buf, enum intel_pipe_crc_source *s)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pipe_crc_sources); i++)
		if (!strcmp(buf, pipe_crc_sources[i])) {
			*s = i;
			return 0;
		    }

	return -EINVAL;
}

static int display_crc_ctl_parse(struct drm_device *dev, char *buf, size_t len)
{
#define N_WORDS 3
	int n_words;
	char *words[N_WORDS];
	enum pipe pipe;
	enum intel_pipe_crc_object object;
	enum intel_pipe_crc_source source;

	n_words = display_crc_ctl_tokenize(buf, words, N_WORDS);
	if (n_words != N_WORDS) {
		DRM_DEBUG_DRIVER("tokenize failed, a command is %d words\n",
				 N_WORDS);
		return -EINVAL;
	}

	if (display_crc_ctl_parse_object(words[0], &object) < 0) {
		DRM_DEBUG_DRIVER("unknown object %s\n", words[0]);
		return -EINVAL;
	}

	if (display_crc_ctl_parse_pipe(words[1], &pipe) < 0) {
		DRM_DEBUG_DRIVER("unknown pipe %s\n", words[1]);
		return -EINVAL;
	}

	if (display_crc_ctl_parse_source(words[2], &source) < 0) {
		DRM_DEBUG_DRIVER("unknown source %s\n", words[2]);
		return -EINVAL;
	}

	return pipe_crc_set_source(dev, pipe, source);
}

static ssize_t display_crc_ctl_write(struct file *file, const char __user *ubuf,
				     size_t len, loff_t *offp)
{
	struct seq_file *m = file->private_data;
	struct drm_device *dev = m->private;
	char *tmpbuf;
	int ret;

	if (len == 0)
		return 0;

	if (len > PAGE_SIZE - 1) {
		DRM_DEBUG_DRIVER("expected <%lu bytes into pipe crc control\n",
				 PAGE_SIZE);
		return -E2BIG;
	}

	tmpbuf = kmalloc(len + 1, GFP_KERNEL);
	if (!tmpbuf)
		return -ENOMEM;

	if (copy_from_user(tmpbuf, ubuf, len)) {
		ret = -EFAULT;
		goto out;
	}
	tmpbuf[len] = '\0';

	ret = display_crc_ctl_parse(dev, tmpbuf, len);

out:
	kfree(tmpbuf);
	if (ret < 0)
		return ret;

	*offp += len;
	return len;
}

static const struct file_operations i915_display_crc_ctl_fops = {
	.owner = THIS_MODULE,
	.open = display_crc_ctl_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = display_crc_ctl_write
};

static ssize_t i915_displayport_test_active_write(struct file *file,
					    const char __user *ubuf,
					    size_t len, loff_t *offp)
{
	char *input_buffer;
	int status = 0;
	struct drm_device *dev;
	struct drm_connector *connector;
	struct list_head *connector_list;
	struct intel_dp *intel_dp;
	int val = 0;

	dev = ((struct seq_file *)file->private_data)->private;

	connector_list = &dev->mode_config.connector_list;

	if (len == 0)
		return 0;

	input_buffer = kmalloc(len + 1, GFP_KERNEL);
	if (!input_buffer)
		return -ENOMEM;

	if (copy_from_user(input_buffer, ubuf, len)) {
		status = -EFAULT;
		goto out;
	}

	input_buffer[len] = '\0';
	DRM_DEBUG_DRIVER("Copied %d bytes from user\n", (unsigned int)len);

	list_for_each_entry(connector, connector_list, head) {

		if (connector->connector_type !=
		    DRM_MODE_CONNECTOR_DisplayPort)
			continue;

		if (connector->status == connector_status_connected &&
		    connector->encoder != NULL) {
			intel_dp = enc_to_intel_dp(connector->encoder);
			status = kstrtoint(input_buffer, 10, &val);
			if (status < 0)
				goto out;
			DRM_DEBUG_DRIVER("Got %d for test active\n", val);
			/* To prevent erroneous activation of the compliance
			 * testing code, only accept an actual value of 1 here
			 */
			if (val == 1)
				intel_dp->compliance_test_active = 1;
			else
				intel_dp->compliance_test_active = 0;
		}
	}
out:
	kfree(input_buffer);
	if (status < 0)
		return status;

	*offp += len;
	return len;
}

static int i915_displayport_test_active_show(struct seq_file *m, void *data)
{
	struct drm_device *dev = m->private;
	struct drm_connector *connector;
	struct list_head *connector_list = &dev->mode_config.connector_list;
	struct intel_dp *intel_dp;

	list_for_each_entry(connector, connector_list, head) {

		if (connector->connector_type !=
		    DRM_MODE_CONNECTOR_DisplayPort)
			continue;

		if (connector->status == connector_status_connected &&
		    connector->encoder != NULL) {
			intel_dp = enc_to_intel_dp(connector->encoder);
			if (intel_dp->compliance_test_active)
				seq_puts(m, "1");
			else
				seq_puts(m, "0");
		} else
			seq_puts(m, "0");
	}

	return 0;
}

static int i915_displayport_test_active_open(struct inode *inode,
				       struct file *file)
{
	struct drm_device *dev = inode->i_private;

	return single_open(file, i915_displayport_test_active_show, dev);
}

static const struct file_operations i915_displayport_test_active_fops = {
	.owner = THIS_MODULE,
	.open = i915_displayport_test_active_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = i915_displayport_test_active_write
};

static int i915_displayport_test_data_show(struct seq_file *m, void *data)
{
	struct drm_device *dev = m->private;
	struct drm_connector *connector;
	struct list_head *connector_list = &dev->mode_config.connector_list;
	struct intel_dp *intel_dp;

	list_for_each_entry(connector, connector_list, head) {

		if (connector->connector_type !=
		    DRM_MODE_CONNECTOR_DisplayPort)
			continue;

		if (connector->status == connector_status_connected &&
		    connector->encoder != NULL) {
			intel_dp = enc_to_intel_dp(connector->encoder);
			seq_printf(m, "%lx", intel_dp->compliance_test_data);
		} else
			seq_puts(m, "0");
	}

	return 0;
}
static int i915_displayport_test_data_open(struct inode *inode,
				       struct file *file)
{
	struct drm_device *dev = inode->i_private;

	return single_open(file, i915_displayport_test_data_show, dev);
}

static const struct file_operations i915_displayport_test_data_fops = {
	.owner = THIS_MODULE,
	.open = i915_displayport_test_data_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static int i915_displayport_test_type_show(struct seq_file *m, void *data)
{
	struct drm_device *dev = m->private;
	struct drm_connector *connector;
	struct list_head *connector_list = &dev->mode_config.connector_list;
	struct intel_dp *intel_dp;

	list_for_each_entry(connector, connector_list, head) {

		if (connector->connector_type !=
		    DRM_MODE_CONNECTOR_DisplayPort)
			continue;

		if (connector->status == connector_status_connected &&
		    connector->encoder != NULL) {
			intel_dp = enc_to_intel_dp(connector->encoder);
			seq_printf(m, "%02lx", intel_dp->compliance_test_type);
		} else
			seq_puts(m, "0");
	}

	return 0;
}

static int i915_displayport_test_type_open(struct inode *inode,
				       struct file *file)
{
	struct drm_device *dev = inode->i_private;

	return single_open(file, i915_displayport_test_type_show, dev);
}

static const struct file_operations i915_displayport_test_type_fops = {
	.owner = THIS_MODULE,
	.open = i915_displayport_test_type_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static void wm_latency_show(struct seq_file *m, const uint16_t wm[8])
{
	struct drm_device *dev = m->private;
	int level;
	int num_levels;

	if (IS_CHERRYVIEW(dev))
		num_levels = 3;
	else if (IS_VALLEYVIEW(dev))
		num_levels = 1;
	else
		num_levels = ilk_wm_max_level(dev) + 1;

	drm_modeset_lock_all(dev);

	for (level = 0; level < num_levels; level++) {
		unsigned int latency = wm[level];

		/*
		 * - WM1+ latency values in 0.5us units
		 * - latencies are in us on gen9/vlv/chv
		 */
		if (INTEL_INFO(dev)->gen >= 9 || IS_VALLEYVIEW(dev))
			latency *= 10;
		else if (level > 0)
			latency *= 5;

		seq_printf(m, "WM%d %u (%u.%u usec)\n",
			   level, wm[level], latency / 10, latency % 10);
	}

	drm_modeset_unlock_all(dev);
}

static int pri_wm_latency_show(struct seq_file *m, void *data)
{
	struct drm_device *dev = m->private;
	struct drm_i915_private *dev_priv = dev->dev_private;
	const uint16_t *latencies;

	if (INTEL_INFO(dev)->gen >= 9)
		latencies = dev_priv->wm.skl_latency;
	else
		latencies = to_i915(dev)->wm.pri_latency;

	wm_latency_show(m, latencies);

	return 0;
}

static int spr_wm_latency_show(struct seq_file *m, void *data)
{
	struct drm_device *dev = m->private;
	struct drm_i915_private *dev_priv = dev->dev_private;
	const uint16_t *latencies;

	if (INTEL_INFO(dev)->gen >= 9)
		latencies = dev_priv->wm.skl_latency;
	else
		latencies = to_i915(dev)->wm.spr_latency;

	wm_latency_show(m, latencies);

	return 0;
}

static int cur_wm_latency_show(struct seq_file *m, void *data)
{
	struct drm_device *dev = m->private;
	struct drm_i915_private *dev_priv = dev->dev_private;
	const uint16_t *latencies;

	if (INTEL_INFO(dev)->gen >= 9)
		latencies = dev_priv->wm.skl_latency;
	else
		latencies = to_i915(dev)->wm.cur_latency;

	wm_latency_show(m, latencies);

	return 0;
}

static int pri_wm_latency_open(struct inode *inode, struct file *file)
{
	struct drm_device *dev = inode->i_private;

	if (INTEL_INFO(dev)->gen < 5)
		return -ENODEV;

	return single_open(file, pri_wm_latency_show, dev);
}

static int spr_wm_latency_open(struct inode *inode, struct file *file)
{
	struct drm_device *dev = inode->i_private;

	if (HAS_GMCH_DISPLAY(dev))
		return -ENODEV;

	return single_open(file, spr_wm_latency_show, dev);
}

static int cur_wm_latency_open(struct inode *inode, struct file *file)
{
	struct drm_device *dev = inode->i_private;

	if (HAS_GMCH_DISPLAY(dev))
		return -ENODEV;

	return single_open(file, cur_wm_latency_show, dev);
}

static ssize_t wm_latency_write(struct file *file, const char __user *ubuf,
				size_t len, loff_t *offp, uint16_t wm[8])
{
	struct seq_file *m = file->private_data;
	struct drm_device *dev = m->private;
	uint16_t new[8] = { 0 };
	int num_levels;
	int level;
	int ret;
	char tmp[32];

	if (IS_CHERRYVIEW(dev))
		num_levels = 3;
	else if (IS_VALLEYVIEW(dev))
		num_levels = 1;
	else
		num_levels = ilk_wm_max_level(dev) + 1;

	if (len >= sizeof(tmp))
		return -EINVAL;

	if (copy_from_user(tmp, ubuf, len))
		return -EFAULT;

	tmp[len] = '\0';

	ret = sscanf(tmp, "%hu %hu %hu %hu %hu %hu %hu %hu",
		     &new[0], &new[1], &new[2], &new[3],
		     &new[4], &new[5], &new[6], &new[7]);
	if (ret != num_levels)
		return -EINVAL;

	drm_modeset_lock_all(dev);

	for (level = 0; level < num_levels; level++)
		wm[level] = new[level];

	drm_modeset_unlock_all(dev);

	return len;
}


static ssize_t pri_wm_latency_write(struct file *file, const char __user *ubuf,
				    size_t len, loff_t *offp)
{
	struct seq_file *m = file->private_data;
	struct drm_device *dev = m->private;
	struct drm_i915_private *dev_priv = dev->dev_private;
	uint16_t *latencies;

	if (INTEL_INFO(dev)->gen >= 9)
		latencies = dev_priv->wm.skl_latency;
	else
		latencies = to_i915(dev)->wm.pri_latency;

	return wm_latency_write(file, ubuf, len, offp, latencies);
}

static ssize_t spr_wm_latency_write(struct file *file, const char __user *ubuf,
				    size_t len, loff_t *offp)
{
	struct seq_file *m = file->private_data;
	struct drm_device *dev = m->private;
	struct drm_i915_private *dev_priv = dev->dev_private;
	uint16_t *latencies;

	if (INTEL_INFO(dev)->gen >= 9)
		latencies = dev_priv->wm.skl_latency;
	else
		latencies = to_i915(dev)->wm.spr_latency;

	return wm_latency_write(file, ubuf, len, offp, latencies);
}

static ssize_t cur_wm_latency_write(struct file *file, const char __user *ubuf,
				    size_t len, loff_t *offp)
{
	struct seq_file *m = file->private_data;
	struct drm_device *dev = m->private;
	struct drm_i915_private *dev_priv = dev->dev_private;
	uint16_t *latencies;

	if (INTEL_INFO(dev)->gen >= 9)
		latencies = dev_priv->wm.skl_latency;
	else
		latencies = to_i915(dev)->wm.cur_latency;

	return wm_latency_write(file, ubuf, len, offp, latencies);
}

static const struct file_operations i915_pri_wm_latency_fops = {
	.owner = THIS_MODULE,
	.open = pri_wm_latency_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = pri_wm_latency_write
};

static const struct file_operations i915_spr_wm_latency_fops = {
	.owner = THIS_MODULE,
	.open = spr_wm_latency_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = spr_wm_latency_write
};

static const struct file_operations i915_cur_wm_latency_fops = {
	.owner = THIS_MODULE,
	.open = cur_wm_latency_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = cur_wm_latency_write
};

static int
i915_wedged_get(void *data, u64 *val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;

	*val = atomic_read(&dev_priv->gpu_error.reset_counter);

	return 0;
}

static int
i915_wedged_set(void *data, u64 val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;

	/*
	 * There is no safeguard against this debugfs entry colliding
	 * with the hangcheck calling same i915_handle_error() in
	 * parallel, causing an explosion. For now we assume that the
	 * test harness is responsible enough not to inject gpu hangs
	 * while it is writing to 'i915_wedged'
	 */

	if (i915_reset_in_progress(&dev_priv->gpu_error))
		return -EAGAIN;

	intel_runtime_pm_get(dev_priv);

	i915_handle_error(dev, val,
			  "Manually setting wedged to %llu", val);

	intel_runtime_pm_put(dev_priv);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(i915_wedged_fops,
			i915_wedged_get, i915_wedged_set,
			"%llu\n");

static int
i915_ring_stop_get(void *data, u64 *val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;

	*val = dev_priv->gpu_error.stop_rings;

	return 0;
}

static int
i915_ring_stop_set(void *data, u64 val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret;

	DRM_DEBUG_DRIVER("Stopping rings 0x%08llx\n", val);

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	dev_priv->gpu_error.stop_rings = val;
	mutex_unlock(&dev->struct_mutex);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(i915_ring_stop_fops,
			i915_ring_stop_get, i915_ring_stop_set,
			"0x%08llx\n");

static int
i915_ring_missed_irq_get(void *data, u64 *val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;

	*val = dev_priv->gpu_error.missed_irq_rings;
	return 0;
}

static int
i915_ring_missed_irq_set(void *data, u64 val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret;

	/* Lock against concurrent debugfs callers */
	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;
	dev_priv->gpu_error.missed_irq_rings = val;
	mutex_unlock(&dev->struct_mutex);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(i915_ring_missed_irq_fops,
			i915_ring_missed_irq_get, i915_ring_missed_irq_set,
			"0x%08llx\n");

static int
i915_ring_test_irq_get(void *data, u64 *val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;

	*val = dev_priv->gpu_error.test_irq_rings;

	return 0;
}

static int
i915_ring_test_irq_set(void *data, u64 val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret;

	DRM_DEBUG_DRIVER("Masking interrupts on rings 0x%08llx\n", val);

	/* Lock against concurrent debugfs callers */
	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	dev_priv->gpu_error.test_irq_rings = val;
	mutex_unlock(&dev->struct_mutex);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(i915_ring_test_irq_fops,
			i915_ring_test_irq_get, i915_ring_test_irq_set,
			"0x%08llx\n");

#define DROP_UNBOUND 0x1
#define DROP_BOUND 0x2
#define DROP_RETIRE 0x4
#define DROP_ACTIVE 0x8
#define DROP_ALL (DROP_UNBOUND | \
		  DROP_BOUND | \
		  DROP_RETIRE | \
		  DROP_ACTIVE)
static int
i915_drop_caches_get(void *data, u64 *val)
{
	*val = DROP_ALL;

	return 0;
}

static int
i915_drop_caches_set(void *data, u64 val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret;

	DRM_DEBUG("Dropping caches: 0x%08llx\n", val);

	/* No need to check and wait for gpu resets, only libdrm auto-restarts
	 * on ioctls on -EAGAIN. */
	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	if (val & DROP_ACTIVE) {
		ret = i915_gpu_idle(dev);
		if (ret)
			goto unlock;
	}

	if (val & (DROP_RETIRE | DROP_ACTIVE))
		i915_gem_retire_requests(dev);

	if (val & DROP_BOUND)
		i915_gem_shrink(dev_priv, LONG_MAX, I915_SHRINK_BOUND);

	if (val & DROP_UNBOUND)
		i915_gem_shrink(dev_priv, LONG_MAX, I915_SHRINK_UNBOUND);

unlock:
	mutex_unlock(&dev->struct_mutex);

	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(i915_drop_caches_fops,
			i915_drop_caches_get, i915_drop_caches_set,
			"0x%08llx\n");

static int
i915_max_freq_get(void *data, u64 *val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret;

	if (INTEL_INFO(dev)->gen < 6)
		return -ENODEV;

	flush_delayed_work(&dev_priv->rps.delayed_resume_work);

	ret = mutex_lock_interruptible(&dev_priv->rps.hw_lock);
	if (ret)
		return ret;

	*val = intel_gpu_freq(dev_priv, dev_priv->rps.max_freq_softlimit);
	mutex_unlock(&dev_priv->rps.hw_lock);

	return 0;
}

static int
i915_max_freq_set(void *data, u64 val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 hw_max, hw_min;
	int ret;

	if (INTEL_INFO(dev)->gen < 6)
		return -ENODEV;

	flush_delayed_work(&dev_priv->rps.delayed_resume_work);

	DRM_DEBUG_DRIVER("Manually setting max freq to %llu\n", val);

	ret = mutex_lock_interruptible(&dev_priv->rps.hw_lock);
	if (ret)
		return ret;

	/*
	 * Turbo will still be enabled, but won't go above the set value.
	 */
	val = intel_freq_opcode(dev_priv, val);

	hw_max = dev_priv->rps.max_freq;
	hw_min = dev_priv->rps.min_freq;

	if (val < hw_min || val > hw_max || val < dev_priv->rps.min_freq_softlimit) {
		mutex_unlock(&dev_priv->rps.hw_lock);
		return -EINVAL;
	}

	dev_priv->rps.max_freq_softlimit = val;

	intel_set_rps(dev, val);

	mutex_unlock(&dev_priv->rps.hw_lock);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(i915_max_freq_fops,
			i915_max_freq_get, i915_max_freq_set,
			"%llu\n");

static int
i915_min_freq_get(void *data, u64 *val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret;

	if (INTEL_INFO(dev)->gen < 6)
		return -ENODEV;

	flush_delayed_work(&dev_priv->rps.delayed_resume_work);

	ret = mutex_lock_interruptible(&dev_priv->rps.hw_lock);
	if (ret)
		return ret;

	*val = intel_gpu_freq(dev_priv, dev_priv->rps.min_freq_softlimit);
	mutex_unlock(&dev_priv->rps.hw_lock);

	return 0;
}

static int
i915_min_freq_set(void *data, u64 val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 hw_max, hw_min;
	int ret;

	if (INTEL_INFO(dev)->gen < 6)
		return -ENODEV;

	flush_delayed_work(&dev_priv->rps.delayed_resume_work);

	DRM_DEBUG_DRIVER("Manually setting min freq to %llu\n", val);

	ret = mutex_lock_interruptible(&dev_priv->rps.hw_lock);
	if (ret)
		return ret;

	/*
	 * Turbo will still be enabled, but won't go below the set value.
	 */
	val = intel_freq_opcode(dev_priv, val);

	hw_max = dev_priv->rps.max_freq;
	hw_min = dev_priv->rps.min_freq;

	if (val < hw_min || val > hw_max || val > dev_priv->rps.max_freq_softlimit) {
		mutex_unlock(&dev_priv->rps.hw_lock);
		return -EINVAL;
	}

	dev_priv->rps.min_freq_softlimit = val;

	intel_set_rps(dev, val);

	mutex_unlock(&dev_priv->rps.hw_lock);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(i915_min_freq_fops,
			i915_min_freq_get, i915_min_freq_set,
			"%llu\n");

static int
i915_cache_sharing_get(void *data, u64 *val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 snpcr;
	int ret;

	if (!(IS_GEN6(dev) || IS_GEN7(dev)))
		return -ENODEV;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;
	intel_runtime_pm_get(dev_priv);

	snpcr = I915_READ(GEN6_MBCUNIT_SNPCR);

	intel_runtime_pm_put(dev_priv);
	mutex_unlock(&dev_priv->dev->struct_mutex);

	*val = (snpcr & GEN6_MBC_SNPCR_MASK) >> GEN6_MBC_SNPCR_SHIFT;

	return 0;
}

static int
i915_cache_sharing_set(void *data, u64 val)
{
	struct drm_device *dev = data;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 snpcr;

	if (!(IS_GEN6(dev) || IS_GEN7(dev)))
		return -ENODEV;

	if (val > 3)
		return -EINVAL;

	intel_runtime_pm_get(dev_priv);
	DRM_DEBUG_DRIVER("Manually setting uncore sharing to %llu\n", val);

	/* Update the cache sharing policy here as well */
	snpcr = I915_READ(GEN6_MBCUNIT_SNPCR);
	snpcr &= ~GEN6_MBC_SNPCR_MASK;
	snpcr |= (val << GEN6_MBC_SNPCR_SHIFT);
	I915_WRITE(GEN6_MBCUNIT_SNPCR, snpcr);

	intel_runtime_pm_put(dev_priv);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(i915_cache_sharing_fops,
			i915_cache_sharing_get, i915_cache_sharing_set,
			"%llu\n");

struct sseu_dev_status {
	unsigned int slice_total;
	unsigned int subslice_total;
	unsigned int subslice_per_slice;
	unsigned int eu_total;
	unsigned int eu_per_subslice;
};

static void cherryview_sseu_device_status(struct drm_device *dev,
					  struct sseu_dev_status *stat)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ss_max = 2;
	int ss;
	u32 sig1[ss_max], sig2[ss_max];

	sig1[0] = I915_READ(CHV_POWER_SS0_SIG1);
	sig1[1] = I915_READ(CHV_POWER_SS1_SIG1);
	sig2[0] = I915_READ(CHV_POWER_SS0_SIG2);
	sig2[1] = I915_READ(CHV_POWER_SS1_SIG2);

	for (ss = 0; ss < ss_max; ss++) {
		unsigned int eu_cnt;

		if (sig1[ss] & CHV_SS_PG_ENABLE)
			/* skip disabled subslice */
			continue;

		stat->slice_total = 1;
		stat->subslice_per_slice++;
		eu_cnt = ((sig1[ss] & CHV_EU08_PG_ENABLE) ? 0 : 2) +
			 ((sig1[ss] & CHV_EU19_PG_ENABLE) ? 0 : 2) +
			 ((sig1[ss] & CHV_EU210_PG_ENABLE) ? 0 : 2) +
			 ((sig2[ss] & CHV_EU311_PG_ENABLE) ? 0 : 2);
		stat->eu_total += eu_cnt;
		stat->eu_per_subslice = max(stat->eu_per_subslice, eu_cnt);
	}
	stat->subslice_total = stat->subslice_per_slice;
}

static void gen9_sseu_device_status(struct drm_device *dev,
				    struct sseu_dev_status *stat)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int s_max = 3, ss_max = 4;
	int s, ss;
	u32 s_reg[s_max], eu_reg[2*s_max], eu_mask[2];

	/* BXT has a single slice and at most 3 subslices. */
	if (IS_BROXTON(dev)) {
		s_max = 1;
		ss_max = 3;
	}

	for (s = 0; s < s_max; s++) {
		s_reg[s] = I915_READ(GEN9_SLICE_PGCTL_ACK(s));
		eu_reg[2*s] = I915_READ(GEN9_SS01_EU_PGCTL_ACK(s));
		eu_reg[2*s + 1] = I915_READ(GEN9_SS23_EU_PGCTL_ACK(s));
	}

	eu_mask[0] = GEN9_PGCTL_SSA_EU08_ACK |
		     GEN9_PGCTL_SSA_EU19_ACK |
		     GEN9_PGCTL_SSA_EU210_ACK |
		     GEN9_PGCTL_SSA_EU311_ACK;
	eu_mask[1] = GEN9_PGCTL_SSB_EU08_ACK |
		     GEN9_PGCTL_SSB_EU19_ACK |
		     GEN9_PGCTL_SSB_EU210_ACK |
		     GEN9_PGCTL_SSB_EU311_ACK;

	for (s = 0; s < s_max; s++) {
		unsigned int ss_cnt = 0;

		if ((s_reg[s] & GEN9_PGCTL_SLICE_ACK) == 0)
			/* skip disabled slice */
			continue;

		stat->slice_total++;

		if (IS_SKYLAKE(dev))
			ss_cnt = INTEL_INFO(dev)->subslice_per_slice;

		for (ss = 0; ss < ss_max; ss++) {
			unsigned int eu_cnt;

			if (IS_BROXTON(dev) &&
			    !(s_reg[s] & (GEN9_PGCTL_SS_ACK(ss))))
				/* skip disabled subslice */
				continue;

			if (IS_BROXTON(dev))
				ss_cnt++;

			eu_cnt = 2 * hweight32(eu_reg[2*s + ss/2] &
					       eu_mask[ss%2]);
			stat->eu_total += eu_cnt;
			stat->eu_per_subslice = max(stat->eu_per_subslice,
						    eu_cnt);
		}

		stat->subslice_total += ss_cnt;
		stat->subslice_per_slice = max(stat->subslice_per_slice,
					       ss_cnt);
	}
}

static void broadwell_sseu_device_status(struct drm_device *dev,
					 struct sseu_dev_status *stat)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int s;
	u32 slice_info = I915_READ(GEN8_GT_SLICE_INFO);

	stat->slice_total = hweight32(slice_info & GEN8_LSLICESTAT_MASK);

	if (stat->slice_total) {
		stat->subslice_per_slice = INTEL_INFO(dev)->subslice_per_slice;
		stat->subslice_total = stat->slice_total *
				       stat->subslice_per_slice;
		stat->eu_per_subslice = INTEL_INFO(dev)->eu_per_subslice;
		stat->eu_total = stat->eu_per_subslice * stat->subslice_total;

		/* subtract fused off EU(s) from enabled slice(s) */
		for (s = 0; s < stat->slice_total; s++) {
			u8 subslice_7eu = INTEL_INFO(dev)->subslice_7eu[s];

			stat->eu_total -= hweight8(subslice_7eu);
		}
	}
}

static int i915_sseu_status(struct seq_file *m, void *unused)
{
	struct drm_info_node *node = (struct drm_info_node *) m->private;
	struct drm_device *dev = node->minor->dev;
	struct sseu_dev_status stat;

	if (INTEL_INFO(dev)->gen < 8)
		return -ENODEV;

	seq_puts(m, "SSEU Device Info\n");
	seq_printf(m, "  Available Slice Total: %u\n",
		   INTEL_INFO(dev)->slice_total);
	seq_printf(m, "  Available Subslice Total: %u\n",
		   INTEL_INFO(dev)->subslice_total);
	seq_printf(m, "  Available Subslice Per Slice: %u\n",
		   INTEL_INFO(dev)->subslice_per_slice);
	seq_printf(m, "  Available EU Total: %u\n",
		   INTEL_INFO(dev)->eu_total);
	seq_printf(m, "  Available EU Per Subslice: %u\n",
		   INTEL_INFO(dev)->eu_per_subslice);
	seq_printf(m, "  Has Slice Power Gating: %s\n",
		   yesno(INTEL_INFO(dev)->has_slice_pg));
	seq_printf(m, "  Has Subslice Power Gating: %s\n",
		   yesno(INTEL_INFO(dev)->has_subslice_pg));
	seq_printf(m, "  Has EU Power Gating: %s\n",
		   yesno(INTEL_INFO(dev)->has_eu_pg));

	seq_puts(m, "SSEU Device Status\n");
	memset(&stat, 0, sizeof(stat));
	if (IS_CHERRYVIEW(dev)) {
		cherryview_sseu_device_status(dev, &stat);
	} else if (IS_BROADWELL(dev)) {
		broadwell_sseu_device_status(dev, &stat);
	} else if (INTEL_INFO(dev)->gen >= 9) {
		gen9_sseu_device_status(dev, &stat);
	}
	seq_printf(m, "  Enabled Slice Total: %u\n",
		   stat.slice_total);
	seq_printf(m, "  Enabled Subslice Total: %u\n",
		   stat.subslice_total);
	seq_printf(m, "  Enabled Subslice Per Slice: %u\n",
		   stat.subslice_per_slice);
	seq_printf(m, "  Enabled EU Total: %u\n",
		   stat.eu_total);
	seq_printf(m, "  Enabled EU Per Subslice: %u\n",
		   stat.eu_per_subslice);

	return 0;
}

static int i915_forcewake_open(struct inode *inode, struct file *file)
{
	struct drm_device *dev = inode->i_private;
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (INTEL_INFO(dev)->gen < 6)
		return 0;

	intel_runtime_pm_get(dev_priv);
	intel_uncore_forcewake_get(dev_priv, FORCEWAKE_ALL);

	return 0;
}

static int i915_forcewake_release(struct inode *inode, struct file *file)
{
	struct drm_device *dev = inode->i_private;
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (INTEL_INFO(dev)->gen < 6)
		return 0;

	intel_uncore_forcewake_put(dev_priv, FORCEWAKE_ALL);
	intel_runtime_pm_put(dev_priv);

	return 0;
}

static const struct file_operations i915_forcewake_fops = {
	.owner = THIS_MODULE,
	.open = i915_forcewake_open,
	.release = i915_forcewake_release,
};

static int i915_forcewake_create(struct dentry *root, struct drm_minor *minor)
{
	struct drm_device *dev = minor->dev;
	struct dentry *ent;

	ent = debugfs_create_file("i915_forcewake_user",
				  S_IRUSR,
				  root, dev,
				  &i915_forcewake_fops);
	if (!ent)
		return -ENOMEM;

	return drm_add_fake_info_node(minor, ent, &i915_forcewake_fops);
}

static int i915_debugfs_create(struct dentry *root,
			       struct drm_minor *minor,
			       const char *name,
			       const struct file_operations *fops)
{
	struct drm_device *dev = minor->dev;
	struct dentry *ent;

	ent = debugfs_create_file(name,
				  S_IRUGO | S_IWUSR,
				  root, dev,
				  fops);
	if (!ent)
		return -ENOMEM;

	return drm_add_fake_info_node(minor, ent, fops);
}

static const struct drm_info_list i915_debugfs_list[] = {
	{"i915_capabilities", i915_capabilities, 0},
	{"i915_gem_objects", i915_gem_object_info, 0},
	{"i915_gem_gtt", i915_gem_gtt_info, 0},
	{"i915_gem_pinned", i915_gem_gtt_info, 0, (void *) PINNED_LIST},
	{"i915_gem_active", i915_gem_object_list_info, 0, (void *) ACTIVE_LIST},
	{"i915_gem_inactive", i915_gem_object_list_info, 0, (void *) INACTIVE_LIST},
	{"i915_gem_stolen", i915_gem_stolen_list_info },
	{"i915_gem_pageflip", i915_gem_pageflip_info, 0},
	{"i915_gem_request", i915_gem_request_info, 0},
	{"i915_gem_seqno", i915_gem_seqno_info, 0},
	{"i915_gem_fence_regs", i915_gem_fence_regs_info, 0},
	{"i915_gem_interrupt", i915_interrupt_info, 0},
	{"i915_gem_hws", i915_hws_info, 0, (void *)RCS},
	{"i915_gem_hws_blt", i915_hws_info, 0, (void *)BCS},
	{"i915_gem_hws_bsd", i915_hws_info, 0, (void *)VCS},
	{"i915_gem_hws_vebox", i915_hws_info, 0, (void *)VECS},
	{"i915_gem_batch_pool", i915_gem_batch_pool_info, 0},
	{"i915_guc_info", i915_guc_info, 0},
	{"i915_guc_load_status", i915_guc_load_status_info, 0},
	{"i915_guc_log_dump", i915_guc_log_dump, 0},
	{"i915_frequency_info", i915_frequency_info, 0},
	{"i915_hangcheck_info", i915_hangcheck_info, 0},
	{"i915_drpc_info", i915_drpc_info, 0},
	{"i915_emon_status", i915_emon_status, 0},
	{"i915_ring_freq_table", i915_ring_freq_table, 0},
	{"i915_frontbuffer_tracking", i915_frontbuffer_tracking, 0},
	{"i915_fbc_status", i915_fbc_status, 0},
	{"i915_ips_status", i915_ips_status, 0},
	{"i915_sr_status", i915_sr_status, 0},
	{"i915_opregion", i915_opregion, 0},
	{"i915_gem_framebuffer", i915_gem_framebuffer_info, 0},
	{"i915_context_status", i915_context_status, 0},
	{"i915_dump_lrc", i915_dump_lrc, 0},
	{"i915_execlists", i915_execlists, 0},
	{"i915_forcewake_domains", i915_forcewake_domains, 0},
	{"i915_swizzle_info", i915_swizzle_info, 0},
	{"i915_ppgtt_info", i915_ppgtt_info, 0},
	{"i915_llc", i915_llc, 0},
	{"i915_edp_psr_status", i915_edp_psr_status, 0},
	{"i915_sink_crc_eDP1", i915_sink_crc, 0},
	{"i915_energy_uJ", i915_energy_uJ, 0},
	{"i915_runtime_pm_status", i915_runtime_pm_status, 0},
	{"i915_power_domain_info", i915_power_domain_info, 0},
	{"i915_display_info", i915_display_info, 0},
	{"i915_semaphore_status", i915_semaphore_status, 0},
	{"i915_shared_dplls_info", i915_shared_dplls_info, 0},
	{"i915_dp_mst_info", i915_dp_mst_info, 0},
	{"i915_wa_registers", i915_wa_registers, 0},
	{"i915_ddb_info", i915_ddb_info, 0},
	{"i915_sseu_status", i915_sseu_status, 0},
	{"i915_drrs_status", i915_drrs_status, 0},
	{"i915_rps_boost_info", i915_rps_boost_info, 0},
};
#define I915_DEBUGFS_ENTRIES ARRAY_SIZE(i915_debugfs_list)

static const struct i915_debugfs_files {
	const char *name;
	const struct file_operations *fops;
} i915_debugfs_files[] = {
	{"i915_wedged", &i915_wedged_fops},
	{"i915_max_freq", &i915_max_freq_fops},
	{"i915_min_freq", &i915_min_freq_fops},
	{"i915_cache_sharing", &i915_cache_sharing_fops},
	{"i915_ring_stop", &i915_ring_stop_fops},
	{"i915_ring_missed_irq", &i915_ring_missed_irq_fops},
	{"i915_ring_test_irq", &i915_ring_test_irq_fops},
	{"i915_gem_drop_caches", &i915_drop_caches_fops},
	{"i915_error_state", &i915_error_state_fops},
	{"i915_next_seqno", &i915_next_seqno_fops},
	{"i915_display_crc_ctl", &i915_display_crc_ctl_fops},
	{"i915_pri_wm_latency", &i915_pri_wm_latency_fops},
	{"i915_spr_wm_latency", &i915_spr_wm_latency_fops},
	{"i915_cur_wm_latency", &i915_cur_wm_latency_fops},
	{"i915_fbc_false_color", &i915_fbc_fc_fops},
	{"i915_dp_test_data", &i915_displayport_test_data_fops},
	{"i915_dp_test_type", &i915_displayport_test_type_fops},
	{"i915_dp_test_active", &i915_displayport_test_active_fops}
};

void intel_display_crc_init(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	enum pipe pipe;

	for_each_pipe(dev_priv, pipe) {
		struct intel_pipe_crc *pipe_crc = &dev_priv->pipe_crc[pipe];

		pipe_crc->opened = false;
		spin_lock_init(&pipe_crc->lock);
		init_waitqueue_head(&pipe_crc->wq);
	}
}

int i915_debugfs_init(struct drm_minor *minor)
{
	int ret, i;

	ret = i915_forcewake_create(minor->debugfs_root, minor);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(i915_pipe_crc_data); i++) {
		ret = i915_pipe_crc_create(minor->debugfs_root, minor, i);
		if (ret)
			return ret;
	}

	for (i = 0; i < ARRAY_SIZE(i915_debugfs_files); i++) {
		ret = i915_debugfs_create(minor->debugfs_root, minor,
					  i915_debugfs_files[i].name,
					  i915_debugfs_files[i].fops);
		if (ret)
			return ret;
	}

	return drm_debugfs_create_files(i915_debugfs_list,
					I915_DEBUGFS_ENTRIES,
					minor->debugfs_root, minor);
}

void i915_debugfs_cleanup(struct drm_minor *minor)
{
	int i;

	drm_debugfs_remove_files(i915_debugfs_list,
				 I915_DEBUGFS_ENTRIES, minor);

	drm_debugfs_remove_files((struct drm_info_list *) &i915_forcewake_fops,
				 1, minor);

	for (i = 0; i < ARRAY_SIZE(i915_pipe_crc_data); i++) {
		struct drm_info_list *info_list =
			(struct drm_info_list *)&i915_pipe_crc_data[i];

		drm_debugfs_remove_files(info_list, 1, minor);
	}

	for (i = 0; i < ARRAY_SIZE(i915_debugfs_files); i++) {
		struct drm_info_list *info_list =
			(struct drm_info_list *) i915_debugfs_files[i].fops;

		drm_debugfs_remove_files(info_list, 1, minor);
	}
}

struct dpcd_block {
	/* DPCD dump start address. */
	unsigned int offset;
	/* DPCD dump end address, inclusive. If unset, .size will be used. */
	unsigned int end;
	/* DPCD dump size. Used if .end is unset. If unset, defaults to 1. */
	size_t size;
	/* Only valid for eDP. */
	bool edp;
};

static const struct dpcd_block i915_dpcd_debug[] = {
	{ .offset = DP_DPCD_REV, .size = DP_RECEIVER_CAP_SIZE },
	{ .offset = DP_PSR_SUPPORT, .end = DP_PSR_CAPS },
	{ .offset = DP_DOWNSTREAM_PORT_0, .size = 16 },
	{ .offset = DP_LINK_BW_SET, .end = DP_EDP_CONFIGURATION_SET },
	{ .offset = DP_SINK_COUNT, .end = DP_ADJUST_REQUEST_LANE2_3 },
	{ .offset = DP_SET_POWER },
	{ .offset = DP_EDP_DPCD_REV },
	{ .offset = DP_EDP_GENERAL_CAP_1, .end = DP_EDP_GENERAL_CAP_3 },
	{ .offset = DP_EDP_DISPLAY_CONTROL_REGISTER, .end = DP_EDP_BACKLIGHT_FREQ_CAP_MAX_LSB },
	{ .offset = DP_EDP_DBC_MINIMUM_BRIGHTNESS_SET, .end = DP_EDP_DBC_MAXIMUM_BRIGHTNESS_SET },
};

static int i915_dpcd_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct intel_dp *intel_dp =
		enc_to_intel_dp(&intel_attached_encoder(connector)->base);
	uint8_t buf[16];
	ssize_t err;
	int i;

	if (connector->status != connector_status_connected)
		return -ENODEV;

	for (i = 0; i < ARRAY_SIZE(i915_dpcd_debug); i++) {
		const struct dpcd_block *b = &i915_dpcd_debug[i];
		size_t size = b->end ? b->end - b->offset + 1 : (b->size ?: 1);

		if (b->edp &&
		    connector->connector_type != DRM_MODE_CONNECTOR_eDP)
			continue;

		/* low tech for now */
		if (WARN_ON(size > sizeof(buf)))
			continue;

		err = drm_dp_dpcd_read(&intel_dp->aux, b->offset, buf, size);
		if (err <= 0) {
			DRM_ERROR("dpcd read (%zu bytes at %u) failed (%zd)\n",
				  size, b->offset, err);
			continue;
		}

		seq_printf(m, "%04x: %*ph\n", b->offset, (int) size, buf);
	}

	return 0;
}

static int i915_dpcd_open(struct inode *inode, struct file *file)
{
	return single_open(file, i915_dpcd_show, inode->i_private);
}

static const struct file_operations i915_dpcd_fops = {
	.owner = THIS_MODULE,
	.open = i915_dpcd_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/**
 * i915_debugfs_connector_add - add i915 specific connector debugfs files
 * @connector: pointer to a registered drm_connector
 *
 * Cleanup will be done by drm_connector_unregister() through a call to
 * drm_debugfs_connector_remove().
 *
 * Returns 0 on success, negative error codes on error.
 */
int i915_debugfs_connector_add(struct drm_connector *connector)
{
	struct dentry *root = connector->debugfs_entry;

	/* The connector must have been registered beforehands. */
	if (!root)
		return -ENODEV;

	if (connector->connector_type == DRM_MODE_CONNECTOR_DisplayPort ||
	    connector->connector_type == DRM_MODE_CONNECTOR_eDP)
		debugfs_create_file("i915_dpcd", S_IRUGO, root, connector,
				    &i915_dpcd_fops);

	return 0;
}
