/* The industrial I/O core
 *
 * Copyright (c) 2008 Jonathan Cameron
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * Handling of ring allocation / resizing.
 *
 *
 * Things to look at here.
 * - Better memory allocation techniques?
 * - Alternative access techniques?
 */
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/poll.h>

#include "iio.h"
#include "ring_generic.h"

int iio_push_ring_event(struct iio_ring_buffer *ring_buf,
		       int event_code,
		       s64 timestamp)
{
	return __iio_push_event(&ring_buf->ev_int,
				event_code,
				timestamp);
}
EXPORT_SYMBOL(iio_push_ring_event);

/**
 * iio_ring_open() - chrdev file open for ring buffer access
 *
 * This function relies on all ring buffer implementations having an
 * iio_ring_buffer as their first element.
 **/
static int iio_ring_open(struct inode *inode, struct file *filp)
{
	struct iio_handler *hand
		= container_of(inode->i_cdev, struct iio_handler, chrdev);
	struct iio_ring_buffer *rb = hand->private;

	filp->private_data = hand->private;
	if (rb->access.mark_in_use)
		rb->access.mark_in_use(rb);

	return 0;
}

/**
 * iio_ring_release() - chrdev file close ring buffer access
 *
 * This function relies on all ring buffer implementations having an
 * iio_ring_buffer as their first element.
 **/
static int iio_ring_release(struct inode *inode, struct file *filp)
{
	struct cdev *cd = inode->i_cdev;
	struct iio_handler *hand = iio_cdev_to_handler(cd);
	struct iio_ring_buffer *rb = hand->private;

	clear_bit(IIO_BUSY_BIT_POS, &rb->access_handler.flags);
	if (rb->access.unmark_in_use)
		rb->access.unmark_in_use(rb);

	return 0;
}

/**
 * iio_ring_read_first_n_outer() - chrdev read for ring buffer access
 *
 * This function relies on all ring buffer implementations having an
 * iio_ring _bufer as their first element.
 **/
static ssize_t iio_ring_read_first_n_outer(struct file *filp, char __user *buf,
				  size_t n, loff_t *f_ps)
{
	struct iio_ring_buffer *rb = filp->private_data;
	int ret, dead_offset;

	/* rip lots must exist. */
	if (!rb->access.read_first_n)
		return -EINVAL;
	ret = rb->access.read_first_n(rb, n, buf, &dead_offset);

	return ret;
}

/**
 * iio_ring_poll() - poll the ring to find out if it has data
 */
static unsigned int iio_ring_poll(struct file *filp,
				  struct poll_table_struct *wait)
{
	struct iio_ring_buffer *rb = filp->private_data;
	int ret = 0;

	poll_wait(filp, &rb->pollq, wait);
	if (rb->stufftoread)
		return POLLIN | POLLRDNORM;
	/* need a way of knowing if there may be enough data... */
	return ret;
}

static const struct file_operations iio_ring_fileops = {
	.read = iio_ring_read_first_n_outer,
	.release = iio_ring_release,
	.open = iio_ring_open,
	.poll = iio_ring_poll,
	.owner = THIS_MODULE,
	.llseek = noop_llseek,
};

/**
 * __iio_request_ring_buffer_event_chrdev() - allocate ring event chrdev
 * @buf:	ring buffer whose event chrdev we are allocating
 * @id:		id of this ring buffer (typically 0)
 * @owner:	the module who owns the ring buffer (for ref counting)
 * @dev:	device with which the chrdev is associated
 **/
static inline int
__iio_request_ring_buffer_event_chrdev(struct iio_ring_buffer *buf,
				       int id,
				       struct module *owner,
				       struct device *dev)
{
	int ret;

	snprintf(buf->ev_int._name, sizeof(buf->ev_int._name),
		 "%s:event%d",
		 dev_name(&buf->dev),
		 id);
	ret = iio_setup_ev_int(&(buf->ev_int),
			       buf->ev_int._name,
			       owner,
			       dev);
	if (ret)
		goto error_ret;
	return 0;

error_ret:
	return ret;
}

static inline void
__iio_free_ring_buffer_event_chrdev(struct iio_ring_buffer *buf)
{
	iio_free_ev_int(&(buf->ev_int));
}

static void iio_ring_access_release(struct device *dev)
{
	struct iio_ring_buffer *buf
		= access_dev_to_iio_ring_buffer(dev);
	cdev_del(&buf->access_handler.chrdev);
	iio_device_free_chrdev_minor(MINOR(dev->devt));
}

static struct device_type iio_ring_access_type = {
	.release = iio_ring_access_release,
};

static inline int
__iio_request_ring_buffer_access_chrdev(struct iio_ring_buffer *buf,
					int id,
					struct module *owner)
{
	int ret, minor;

	buf->access_handler.flags = 0;

	buf->access_dev.parent = &buf->dev;
	buf->access_dev.bus = &iio_bus_type;
	buf->access_dev.type = &iio_ring_access_type;
	device_initialize(&buf->access_dev);

	minor = iio_device_get_chrdev_minor();
	if (minor < 0) {
		ret = minor;
		goto error_device_put;
	}
	buf->access_dev.devt = MKDEV(MAJOR(iio_devt), minor);


	buf->access_id = id;

	dev_set_name(&buf->access_dev, "%s:access%d",
		     dev_name(&buf->dev),
		     buf->access_id);
	ret = device_add(&buf->access_dev);
	if (ret < 0) {
		printk(KERN_ERR "failed to add the ring access dev\n");
		goto error_device_put;
	}

	cdev_init(&buf->access_handler.chrdev, &iio_ring_fileops);
	buf->access_handler.chrdev.owner = owner;

	ret = cdev_add(&buf->access_handler.chrdev, buf->access_dev.devt, 1);
	if (ret) {
		printk(KERN_ERR "failed to allocate ring access chrdev\n");
		goto error_device_unregister;
	}
	return 0;

error_device_unregister:
	device_unregister(&buf->access_dev);
error_device_put:
	put_device(&buf->access_dev);

	return ret;
}

static void __iio_free_ring_buffer_access_chrdev(struct iio_ring_buffer *buf)
{
	device_unregister(&buf->access_dev);
}

void iio_ring_buffer_init(struct iio_ring_buffer *ring,
			  struct iio_dev *dev_info)
{
	if (ring->access.mark_param_change)
		ring->access.mark_param_change(ring);
	ring->indio_dev = dev_info;
	ring->ev_int.private = ring;
	ring->access_handler.private = ring;
	init_waitqueue_head(&ring->pollq);
}
EXPORT_SYMBOL(iio_ring_buffer_init);

static ssize_t iio_show_scan_index(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct iio_dev_attr *this_attr = to_iio_dev_attr(attr);
	return sprintf(buf, "%u\n", this_attr->c->scan_index);
}

static ssize_t iio_show_fixed_type(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct iio_dev_attr *this_attr = to_iio_dev_attr(attr);
	return sprintf(buf, "%c%d/%d>>%u\n",
		       this_attr->c->scan_type.sign,
		       this_attr->c->scan_type.realbits,
		       this_attr->c->scan_type.storagebits,
		       this_attr->c->scan_type.shift);
}

static int __iio_add_chan_scan_elattr(const char *postfix,
				      const char *group,
				      const struct iio_chan_spec *chan,
				      struct device *dev,
				      struct list_head *attr_list)
{
	int ret;
	struct iio_scan_el *scan_el;

	scan_el = kzalloc(sizeof *scan_el, GFP_KERNEL);
	if (scan_el == NULL) {
		ret = -ENOMEM;
		goto error_ret;
	}
	if (chan->type != IIO_TIMESTAMP)
		ret = __iio_device_attr_init(&scan_el->dev_attr, postfix, chan,
					     iio_scan_el_show,
					     iio_scan_el_store, 0);
	else /*
	      * Timestamp handled separately because it simplifies a lot of
	      * drivers by ensuring they don't have to know its magic index
	      */
		ret = __iio_device_attr_init(&scan_el->dev_attr, postfix, chan,
					     iio_scan_el_ts_show,
					     iio_scan_el_ts_store, 0);
	if (ret)
		goto error_free_scan_el;

	scan_el->number = chan->scan_index;

	ret = sysfs_add_file_to_group(&dev->kobj,
				      &scan_el->dev_attr.attr,
				      group);
	if (ret < 0)
		goto error_device_attr_deinit;

	list_add(&scan_el->l, attr_list);

	return 0;
error_device_attr_deinit:
	__iio_device_attr_deinit(&scan_el->dev_attr);
error_free_scan_el:
	kfree(scan_el);
error_ret:
	return ret;
}

static int iio_ring_add_channel_sysfs(struct iio_ring_buffer *ring,
				      const struct iio_chan_spec *chan)
{
	int ret;

	ret = __iio_add_chan_devattr("index", "scan_elements",
				     chan,
				     &iio_show_scan_index,
				     NULL,
				     0,
				     0,
				     &ring->dev,
				     &ring->scan_el_dev_attr_list);
	if (ret)
		goto error_ret;

	ret = __iio_add_chan_devattr("type", "scan_elements",
				     chan,
				     &iio_show_fixed_type,
				     NULL,
				     0,
				     0,
				     &ring->dev,
				     &ring->scan_el_dev_attr_list);

	if (ret)
		goto error_ret;

	ret = __iio_add_chan_scan_elattr("en", "scan_elements",
					 chan, &ring->dev,
					 &ring->scan_el_en_attr_list);

error_ret:
	return ret;
}

static void iio_ring_remove_and_free_scan_el_attr(struct iio_ring_buffer *ring,
						  struct iio_scan_el *p)
{
	sysfs_remove_file_from_group(&ring->dev.kobj,
				     &p->dev_attr.attr, "scan_elements");
	kfree(p->dev_attr.attr.name);
	kfree(p);
}

static void iio_ring_remove_and_free_scan_dev_attr(struct iio_ring_buffer *ring,
						   struct iio_dev_attr *p)
{
	sysfs_remove_file_from_group(&ring->dev.kobj,
				     &p->dev_attr.attr, "scan_elements");
	kfree(p->dev_attr.attr.name);
	kfree(p);
}

static struct attribute *iio_scan_el_dummy_attrs[] = {
	NULL
};

static struct attribute_group iio_scan_el_dummy_group = {
	.name = "scan_elements",
	.attrs = iio_scan_el_dummy_attrs
};

static void __iio_ring_attr_cleanup(struct iio_ring_buffer *ring)
{
	struct iio_dev_attr *p, *n;
	struct iio_scan_el *q, *m;
	int anydynamic = !(list_empty(&ring->scan_el_dev_attr_list) &&
			   list_empty(&ring->scan_el_en_attr_list));
	list_for_each_entry_safe(p, n,
				 &ring->scan_el_dev_attr_list, l)
		iio_ring_remove_and_free_scan_dev_attr(ring, p);
	list_for_each_entry_safe(q, m,
				 &ring->scan_el_en_attr_list, l)
		iio_ring_remove_and_free_scan_el_attr(ring, q);

	if (ring->scan_el_attrs)
		sysfs_remove_group(&ring->dev.kobj,
				   ring->scan_el_attrs);
	else if (anydynamic)
		sysfs_remove_group(&ring->dev.kobj,
				   &iio_scan_el_dummy_group);
}

int iio_ring_buffer_register_ex(struct iio_ring_buffer *ring, int id,
				const struct iio_chan_spec *channels,
				int num_channels)
{
	int ret, i;

	ring->id = id;

	dev_set_name(&ring->dev, "%s:buffer%d",
		     dev_name(ring->dev.parent),
		     ring->id);
	ret = device_add(&ring->dev);
	if (ret)
		goto error_ret;

	ret = __iio_request_ring_buffer_event_chrdev(ring,
						     0,
						     ring->owner,
						     &ring->dev);
	if (ret)
		goto error_remove_device;

	ret = __iio_request_ring_buffer_access_chrdev(ring,
						      0,
						      ring->owner);

	if (ret)
		goto error_ret;

	if (ring->scan_el_attrs) {
		ret = sysfs_create_group(&ring->dev.kobj,
					 ring->scan_el_attrs);
		if (ret) {
			dev_err(&ring->dev,
				"Failed to add sysfs scan elements\n");
			goto error_free_ring_buffer_access_chrdev;
		}
	} else if (channels) {
		ret = sysfs_create_group(&ring->dev.kobj,
					 &iio_scan_el_dummy_group);
		if (ret)
			goto error_free_ring_buffer_access_chrdev;
	}


	INIT_LIST_HEAD(&ring->scan_el_dev_attr_list);
	INIT_LIST_HEAD(&ring->scan_el_en_attr_list);
	if (channels) {
		/* new magic */
		for (i = 0; i < num_channels; i++) {
			ret = iio_ring_add_channel_sysfs(ring, &channels[i]);
			if (ret < 0)
				goto error_cleanup_dynamic;
		}
	}

	return 0;
error_cleanup_dynamic:
	__iio_ring_attr_cleanup(ring);
error_free_ring_buffer_access_chrdev:
	__iio_free_ring_buffer_access_chrdev(ring);
error_remove_device:
	device_del(&ring->dev);
error_ret:
	return ret;
}
EXPORT_SYMBOL(iio_ring_buffer_register_ex);

int iio_ring_buffer_register(struct iio_ring_buffer *ring, int id)
{
	return iio_ring_buffer_register_ex(ring, id, NULL, 0);
}
EXPORT_SYMBOL(iio_ring_buffer_register);

void iio_ring_buffer_unregister(struct iio_ring_buffer *ring)
{
	__iio_ring_attr_cleanup(ring);
	__iio_free_ring_buffer_access_chrdev(ring);
	__iio_free_ring_buffer_event_chrdev(ring);
	device_del(&ring->dev);
}
EXPORT_SYMBOL(iio_ring_buffer_unregister);

ssize_t iio_read_ring_length(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	int len = 0;
	struct iio_ring_buffer *ring = dev_get_drvdata(dev);

	if (ring->access.get_length)
		len = sprintf(buf, "%d\n",
			      ring->access.get_length(ring));

	return len;
}
EXPORT_SYMBOL(iio_read_ring_length);

ssize_t iio_write_ring_length(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf,
			       size_t len)
{
	int ret;
	ulong val;
	struct iio_ring_buffer *ring = dev_get_drvdata(dev);
	ret = strict_strtoul(buf, 10, &val);
	if (ret)
		return ret;

	if (ring->access.get_length)
		if (val == ring->access.get_length(ring))
			return len;

	if (ring->access.set_length) {
		ring->access.set_length(ring, val);
		if (ring->access.mark_param_change)
			ring->access.mark_param_change(ring);
	}

	return len;
}
EXPORT_SYMBOL(iio_write_ring_length);

ssize_t iio_read_ring_bytes_per_datum(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	int len = 0;
	struct iio_ring_buffer *ring = dev_get_drvdata(dev);

	if (ring->access.get_bytes_per_datum)
		len = sprintf(buf, "%d\n",
			      ring->access.get_bytes_per_datum(ring));

	return len;
}
EXPORT_SYMBOL(iio_read_ring_bytes_per_datum);

ssize_t iio_store_ring_enable(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf,
			      size_t len)
{
	int ret;
	bool requested_state, current_state;
	int previous_mode;
	struct iio_ring_buffer *ring = dev_get_drvdata(dev);
	struct iio_dev *dev_info = ring->indio_dev;

	mutex_lock(&dev_info->mlock);
	previous_mode = dev_info->currentmode;
	requested_state = !(buf[0] == '0');
	current_state = !!(previous_mode & INDIO_ALL_RING_MODES);
	if (current_state == requested_state) {
		printk(KERN_INFO "iio-ring, current state requested again\n");
		goto done;
	}
	if (requested_state) {
		if (ring->preenable) {
			ret = ring->preenable(dev_info);
			if (ret) {
				printk(KERN_ERR
				       "Buffer not started:"
				       "ring preenable failed\n");
				goto error_ret;
			}
		}
		if (ring->access.request_update) {
			ret = ring->access.request_update(ring);
			if (ret) {
				printk(KERN_INFO
				       "Buffer not started:"
				       "ring parameter update failed\n");
				goto error_ret;
			}
		}
		if (ring->access.mark_in_use)
			ring->access.mark_in_use(ring);
		/* Definitely possible for devices to support both of these.*/
		if (dev_info->modes & INDIO_RING_TRIGGERED) {
			if (!dev_info->trig) {
				printk(KERN_INFO
				       "Buffer not started: no trigger\n");
				ret = -EINVAL;
				if (ring->access.unmark_in_use)
					ring->access.unmark_in_use(ring);
				goto error_ret;
			}
			dev_info->currentmode = INDIO_RING_TRIGGERED;
		} else if (dev_info->modes & INDIO_RING_HARDWARE_BUFFER)
			dev_info->currentmode = INDIO_RING_HARDWARE_BUFFER;
		else { /* should never be reached */
			ret = -EINVAL;
			goto error_ret;
		}

		if (ring->postenable) {

			ret = ring->postenable(dev_info);
			if (ret) {
				printk(KERN_INFO
				       "Buffer not started:"
				       "postenable failed\n");
				if (ring->access.unmark_in_use)
					ring->access.unmark_in_use(ring);
				dev_info->currentmode = previous_mode;
				if (ring->postdisable)
					ring->postdisable(dev_info);
				goto error_ret;
			}
		}
	} else {
		if (ring->predisable) {
			ret = ring->predisable(dev_info);
			if (ret)
				goto error_ret;
		}
		if (ring->access.unmark_in_use)
			ring->access.unmark_in_use(ring);
		dev_info->currentmode = INDIO_DIRECT_MODE;
		if (ring->postdisable) {
			ret = ring->postdisable(dev_info);
			if (ret)
				goto error_ret;
		}
	}
done:
	mutex_unlock(&dev_info->mlock);
	return len;

error_ret:
	mutex_unlock(&dev_info->mlock);
	return ret;
}
EXPORT_SYMBOL(iio_store_ring_enable);
ssize_t iio_show_ring_enable(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct iio_ring_buffer *ring = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", !!(ring->indio_dev->currentmode
				       & INDIO_ALL_RING_MODES));
}
EXPORT_SYMBOL(iio_show_ring_enable);

ssize_t iio_scan_el_show(struct device *dev,
			 struct device_attribute *attr,
			 char *buf)
{
	int ret;
	struct iio_ring_buffer *ring = dev_get_drvdata(dev);
	struct iio_scan_el *this_el = to_iio_scan_el(attr);

	ret = iio_scan_mask_query(ring, this_el->number);
	if (ret < 0)
		return ret;
	return sprintf(buf, "%d\n", ret);
}
EXPORT_SYMBOL(iio_scan_el_show);

ssize_t iio_scan_el_store(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf,
			  size_t len)
{
	int ret = 0;
	bool state;
	struct iio_ring_buffer *ring = dev_get_drvdata(dev);
	struct iio_dev *indio_dev = ring->indio_dev;
	struct iio_scan_el *this_el = to_iio_scan_el(attr);

	state = !(buf[0] == '0');
	mutex_lock(&indio_dev->mlock);
	if (indio_dev->currentmode == INDIO_RING_TRIGGERED) {
		ret = -EBUSY;
		goto error_ret;
	}
	ret = iio_scan_mask_query(ring, this_el->number);
	if (ret < 0)
		goto error_ret;
	if (!state && ret) {
		ret = iio_scan_mask_clear(ring, this_el->number);
		if (ret)
			goto error_ret;
	} else if (state && !ret) {
		ret = iio_scan_mask_set(ring, this_el->number);
		if (ret)
			goto error_ret;
	}
	if (this_el->set_state)
		ret = this_el->set_state(this_el, indio_dev, state);
error_ret:
	mutex_unlock(&indio_dev->mlock);

	return ret ? ret : len;

}
EXPORT_SYMBOL(iio_scan_el_store);

ssize_t iio_scan_el_ts_show(struct device *dev,
			    struct device_attribute *attr,
			    char *buf)
{
	struct iio_ring_buffer *ring = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", ring->scan_timestamp);
}
EXPORT_SYMBOL(iio_scan_el_ts_show);

ssize_t iio_scan_el_ts_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf,
			     size_t len)
{
	int ret = 0;
	struct iio_ring_buffer *ring = dev_get_drvdata(dev);
	struct iio_dev *indio_dev = ring->indio_dev;
	bool state;
	state = !(buf[0] == '0');
	mutex_lock(&indio_dev->mlock);
	if (indio_dev->currentmode == INDIO_RING_TRIGGERED) {
		ret = -EBUSY;
		goto error_ret;
	}
	ring->scan_timestamp = state;
error_ret:
	mutex_unlock(&indio_dev->mlock);

	return ret ? ret : len;
}
EXPORT_SYMBOL(iio_scan_el_ts_store);
