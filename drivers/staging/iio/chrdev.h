/* The industrial I/O core - character device related
 *
 * Copyright (c) 2008 Jonathan Cameron
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#ifndef _IIO_CHRDEV_H_
#define _IIO_CHRDEV_H_

/**
 * struct iio_handler - Structure used to specify file operations
 *			for a particular chrdev
 * @chrdev:	character device structure
 * @id:		the location in the handler table - used for deallocation.
 * @flags:	file operations related flags including busy flag.
 * @private:	handler specific data used by the fileops registered with
 *		the chrdev.
 */
struct iio_handler {
	struct cdev	chrdev;
	int		id;
	unsigned long	flags;
	void		*private;
};

#define iio_cdev_to_handler(cd)				\
	container_of(cd, struct iio_handler, chrdev)

/**
 * struct iio_event_data - The actual event being pushed to userspace
 * @id:		event identifier
 * @timestamp:	best estimate of time of event occurrence (often from
 *		the interrupt handler)
 */
struct iio_event_data {
	int	id;
	s64	timestamp;
};

#endif
