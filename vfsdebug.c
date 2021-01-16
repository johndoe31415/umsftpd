/**
 *	umsftpd - User-mode SFTP application.
 *	Copyright (C) 2021-2021 Johannes Bauer
 *
 *	This file is part of umsftpd.
 *
 *	umsftpd is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; this program is ONLY licensed under
 *	version 3 of the License, later versions are explicitly excluded.
 *
 *	umsftpd is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with umsftpd; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	Johannes Bauer <JohannesBauer@gmx.de>
**/

#include "vfsdebug.h"

static void vfs_dump_flags(FILE *f, unsigned int flags) {
	if (flags & VFS_INODE_FLAG_RESET) {
		fprintf(f, " RESET");
	}
	if (flags & VFS_INODE_FLAG_READ_ONLY) {
		fprintf(f, " READ_ONLY");
	}
	if (flags & VFS_INODE_FLAG_FILTER_ALL) {
		fprintf(f, " FILTER_ALL");
	}
	if (flags & VFS_INODE_FLAG_FILTER_HIDDEN) {
		fprintf(f, " FILTER_HIDDEN");
	}
	if (flags & VFS_INODE_FLAG_DISALLOW_CREATE_FILE) {
		fprintf(f, " DISALLOW_CREATE_FILE");
	}
	if (flags & VFS_INODE_FLAG_DISALLOW_CREATE_DIR) {
		fprintf(f, " DISALLOW_CREATE_DIR");
	}
	if (flags & VFS_INODE_FLAG_DISALLOW_UNLINK) {
		fprintf(f, " DISALLOW_UNLINK");
	}
}

static void vfs_dump_inode_target(FILE *f, const struct vfs_inode_t *inode) {
	fprintf(f, "%s", inode->virtual_path);
	if (inode->target_path) {
		fprintf(f, " => %s", inode->target_path);
	}
	if (inode->flags) {
		fprintf(f, " [flags: 0x%x ", inode->flags);
		vfs_dump_flags(f, inode->flags);
		fprintf(f, "]");
	}
}

void vfs_dump(FILE *f, const struct vfs_t *vfs) {
	fprintf(f, "VFS details:\n");
	if (vfs->error.code) {
		fprintf(f, "   Last error: %d (%s)\n", vfs->error.code, vfs->error.string);
	}
	fprintf(f, "   Max handles: %d, Inodes: %d\n", vfs->handles.max_count, vfs->inode.count);
	fprintf(f, "   Base flags: 0x%x ", vfs->inode.base_flags);
	vfs_dump_flags(f, vfs->inode.base_flags);
	fprintf(f, "\n");
	for (unsigned int i = 0; i < vfs->inode.count; i++) {
		struct vfs_inode_t *inode = vfs->inode.data[i];
		fprintf(f, "   Inode %2d of %d: ", i + 1, vfs->inode.count);
		vfs_dump_inode_target(f, inode);
		fprintf(f, "\n");
	}
}

void vfs_dump_map_result(FILE *f, const struct vfs_lookup_result_t *map_result) {
	fprintf(f, "Map result flags: 0x%x ", map_result->flags);
	vfs_dump_flags(f, map_result->flags);
	fprintf(f, "\n");
	if (map_result->mountpoint) {
		fprintf(f, "Mountpoint: ");
		vfs_dump_inode_target(f, map_result->mountpoint);
		fprintf(f, "\n");
	} else {
		fprintf(f, "No mountpoint.\n");
	}
	fprintf(f, "Virtual directory: %s\n", map_result->virtual_directory ? "yes" : "no");
}
