// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2000-2003,2005 Silicon Graphics, Inc.
 * All Rights Reserved.
 */
#ifndef __XFS_IOPS_H__
#define __XFS_IOPS_H__

struct xfs_inode;

extern ssize_t xfs_vn_listxattr(struct dentry *, char *data, size_t size);

int xfs_vn_setattr_size(struct user_namespace *mnt_userns,
		struct dentry *dentry, struct iattr *vap);

int xfs_inode_init_security(struct inode *inode, struct inode *dir,
		const struct qstr *qstr);

#endif /* __XFS_IOPS_H__ */
