/*
 * AppArmor security module
 *
 * This file contains AppArmor filesystem definitions.
 *
 * Copyright (C) 1998-2008 Novell/SUSE
 * Copyright 2009-2010 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 */

#ifndef __AA_APPARMORFS_H
#define __AA_APPARMORFS_H

enum aa_fs_type {
	AA_FS_TYPE_FOPS,
	AA_FS_TYPE_DIR,
};

struct aa_fs_entry;

struct aa_fs_entry {
	const char *name;
	struct dentry *dentry;
	umode_t mode;
	enum aa_fs_type v_type;
	union {
		struct aa_fs_entry *files;
	} v;
	const struct file_operations *file_ops;
};

#define AA_FS_FILE_FOPS(_name, _mode, _fops) \
	{ .name = (_name), .v_type = AA_FS_TYPE_FOPS, \
	  .mode = (_mode), .file_ops = (_fops) }
#define AA_FS_DIR(_name, _value) \
	{ .name = (_name), .v_type = AA_FS_TYPE_DIR, .v.files = (_value) }

extern void __init aa_destroy_aafs(void);

#endif /* __AA_APPARMORFS_H */
