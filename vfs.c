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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "vfs.h"
#include "logging.h"
#include "strings.h"

struct vfs_map_traversal_t {
	struct vfs_t *vfs;
	struct vfs_map_result_t *map_result;
};

static void vfs_set_error(struct vfs_t *vfs, enum vfs_error_code_t error_code, const char *msg, ...) {
	vfs->last_error = error_code;

	va_list ap;
	va_start(ap, msg);
	vsnprintf(vfs->error_str, VFS_MAX_ERROR_LENGTH - 1, msg, ap);
	va_end(ap);
}

static struct vfs_inode_t *vfs_find_inode(struct vfs_t *vfs, const char *virtual_path) {
	int vpath_len = strlen(virtual_path);

	for (unsigned int i = 0; i < vfs->inode_count; i++) {
		struct vfs_inode_t *inode = &vfs->inodes[i];

		/* Search for match with trailing slash */
		if (!strcmp(inode->virtual_path, virtual_path)) {
			return inode;
		}

		/* But also find it if there is no trailing slash */
		if ((vpath_len == inode->vlen - 1) && (!strncmp(inode->virtual_path, virtual_path, inode->vlen - 1))) {
			return inode;
		}

	}
	return NULL;
}

static bool vfs_set_cwd(struct vfs_t *vfs, const char *new_cwd) {
	if (!is_directory_string(new_cwd)) {
		vfs_set_error(vfs, VFS_CWD_ILLEGAL, "working directory must start and end with '/' character");
		return false;
	}

	int len = strlen(new_cwd);
	if (len + 1 > vfs->cwd_alloced_size) {
		/* Realloc necessary */
		char *realloced_cwd = realloc(vfs->cwd, len + 1);
		if (!realloced_cwd) {
			vfs_set_error(vfs, VFS_CWD_OUT_OF_MEMORY, "out of memory when changing directory");
			return false;
		}
		vfs->cwd = realloced_cwd;
		vfs->cwd_strlen = len;
		vfs->cwd_alloced_size = len + 1;
	}
	strcpy(vfs->cwd, new_cwd);
	return true;
}

bool vfs_add_inode(struct vfs_t *vfs, const char *virtual_path, const char *real_path, unsigned int flags) {
	if (!is_directory_string(virtual_path)) {
		vfs_set_error(vfs, VFS_ADD_INODE_PARAMETER_ERROR, "virtual path must start and end with a '/' character");
		return false;
	}

	size_t vlen = strlen(virtual_path);
	size_t rlen = 0;
	if (real_path) {
		rlen = strlen(real_path);
		if (!is_directory_string(real_path)) {
			vfs_set_error(vfs, VFS_ADD_INODE_PARAMETER_ERROR, "real path must start and end with a '/' character");
			return false;
		}
	}

	if (vfs_find_inode(vfs, virtual_path)) {
		vfs_set_error(vfs, VFS_ADD_INODE_ALREADY_EXISTS, "virtual path inode for '%s' is duplicate", virtual_path);
		return false;
	}

	char *vpath_copy = strdup(virtual_path);
	if (!vpath_copy) {
		vfs_set_error(vfs, VFS_ADD_INODE_OUT_OF_MEMORY, "error dupping virtual path (%s)", strerror(errno));
		return false;
	}

	char *rpath_copy = real_path ? strdup(real_path) : NULL;
	if (real_path && (!rpath_copy)) {
		free(vpath_copy);
		vfs_set_error(vfs, VFS_ADD_INODE_OUT_OF_MEMORY, "error dupping real path (%s)", strerror(errno));
		return false;
	}

	struct vfs_inode_t *new_inodes = realloc(vfs->inodes, sizeof(struct vfs_inode_t) * (vfs->inode_count + 1));
	if (!new_inodes) {
		vfs_set_error(vfs, VFS_ADD_INODE_OUT_OF_MEMORY, "error reallocating inode memory (%s)", strerror(errno));
		return false;
	}
	vfs->inodes = new_inodes;
	vfs->inode_count++;

	struct vfs_inode_t *new_inode = &vfs->inodes[vfs->inode_count - 1];
	memset(new_inode, 0, sizeof(*new_inode));
	new_inode->virtual_path = vpath_copy;
	new_inode->flags = flags;
	new_inode->target = rpath_copy;
	new_inode->vlen = vlen;
	new_inode->rlen = rlen;

	return true;
}

static bool vfs_map_splitter(const char *path, void *vctx) {
	struct vfs_map_traversal_t *ctx = (struct vfs_map_traversal_t*)vctx;
	struct vfs_inode_t *inode = vfs_find_inode(ctx->vfs, path);
	if (inode) {
		if (inode->flags & VFS_INODE_FLAG_RESET) {
			ctx->map_result->flags = inode->flags & ~VFS_INODE_FLAG_RESET;
		} else {
			ctx->map_result->flags |= inode->flags;
		}
		ctx->map_result->target = inode;
		if (inode->target) {
			ctx->map_result->mountpoint = inode;
		}
	}
	return true;
}

bool vfs_map(struct vfs_t *vfs, struct vfs_map_result_t *result, const char *path) {
	if (!vfs->inodes_finalized) {
		vfs_set_error(vfs, VFS_INODE_FINALIZATION_ERROR, "inodes not finalized");
		return false;
	}

	size_t len = strlen(path);
	size_t total_len = len;

	bool relative_path = (len == 0) || (path[0] != '/');
	if (relative_path) {
		total_len += vfs->cwd_strlen;
	}

	/* One character more than needed so we can append '/' */
	char mpath[total_len + 1 + 1];
	if (relative_path) {
		strcpy(mpath, vfs->cwd);
		strcpy(mpath + vfs->cwd_strlen, path);
	} else {
		strcpy(mpath, path);
	}

	memset(result, 0, sizeof(*result));
	result->flags = vfs->base_flags;

	struct vfs_map_traversal_t ctx = {
		.vfs = vfs,
		.map_result = result,
	};
	path_split(mpath, vfs_map_splitter, &ctx);
	return true;
}

static bool vfs_finalization_splitter(const char *path, void *vctx) {
	struct vfs_t *vfs = (struct vfs_t*)vctx;
	struct vfs_inode_t *inode = vfs_find_inode(vfs, path);
	if (!inode) {
		vfs_add_inode(vfs, path, NULL, 0);
	}
	return true;
}

static int vfs_inode_comparator(const void *velem1, const void *velem2) {
	const struct vfs_inode_t *elem1 = (const struct vfs_inode_t *)velem1;
	const struct vfs_inode_t *elem2 = (const struct vfs_inode_t *)velem2;
	return strcmp(elem1->virtual_path, elem2->virtual_path);
}

void vfs_finalize_inodes(struct vfs_t *vfs) {
	if (vfs->inodes_finalized) {
		vfs_set_error(vfs, VFS_INODE_FINALIZATION_ERROR, "inodes already finalized");
	}

	unsigned int old_inode_count = vfs->inode_count;
	for (unsigned int i = 0; i < old_inode_count; i++) {
		struct vfs_inode_t *inode = &vfs->inodes[i];
		char mpath[inode->vlen + 1];
		strcpy(mpath, inode->virtual_path);
		path_split(mpath, vfs_finalization_splitter, vfs);
	}
	qsort(vfs->inodes, vfs->inode_count, sizeof(struct vfs_inode_t), vfs_inode_comparator);
	vfs->inodes_finalized = true;
}

struct vfs_handle_t* vfs_opendir(const struct vfs_t *vfs, const char *virtual_path) {
	struct vfs_handle_t *handle = calloc(1, sizeof(*handle));
	if (!handle) {
		return NULL;
	}

	return handle;
}

struct vfs_t *vfs_init(void) {
	struct vfs_t *vfs = calloc(1, sizeof(*vfs));
	if (!vfs) {
		return NULL;
	}

	if (!vfs_set_cwd(vfs, "/")) {
		vfs_free(vfs);
		return NULL;
	}

	vfs->max_handle_count = 10;

	return vfs;
}

void vfs_handle_free(struct vfs_handle_t *handle) {
	free(handle);
}

void vfs_free(struct vfs_t *vfs) {
	if (!vfs) {
		return;
	}
	free(vfs->cwd);
	for (unsigned int i = 0; i < vfs->inode_count; i++) {
		free(vfs->inodes[i].target);
		free(vfs->inodes[i].virtual_path);
	}
	free(vfs->inodes);
	free(vfs);
}

#ifdef __VFS_TEST__
#include "vfsdebug.h"

int main(void) {
	struct vfs_t *vfs = vfs_init();
	vfs_add_inode(vfs, "/pics/", "/home/joe/pics/", 0);
	vfs_add_inode(vfs, "/pics/foo/neu/", "/home/joe/pics_neu/", VFS_INODE_FLAG_READ_ONLY);
	vfs_add_inode(vfs, "/this/is/", NULL, VFS_INODE_FLAG_DISALLOW_CREATE_DIR);
	vfs_add_inode(vfs, "/this/is/deeply/nested/", "/home/joe/nested/", VFS_INODE_FLAG_READ_ONLY);
	vfs_add_inode(vfs, "/zeug/", "/tmp/zeug/", 0);
	vfs_add_inode(vfs, "/incoming/", "/tmp/write/", VFS_INODE_FLAG_DISALLOW_UNLINK);
	fprintf(stderr, "=========================\n");
	vfs_finalize_inodes(vfs);
	vfs_dump(stderr, vfs);

	struct vfs_map_result_t result;
	//vfs_map(vfs, &result, "/pics/DCIM/12345.jpg");
//	vfs_map(vfs, &result, "/pics/foo/bar");
//	vfs_map(vfs, &result, "/pics/foo/neu/bar");
	//vfs_map(vfs, &result, "/");

	vfs_dump_map_result(stderr, &result);

	vfs_free(vfs);
	return 0;
}
#endif
