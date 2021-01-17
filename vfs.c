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

struct vfs_lookup_traversal_t {
	struct vfs_t *vfs;
	struct vfs_lookup_result_t *map_result;
};

struct vfs_inode_adding_ctx_t {
	struct vfs_t *vfs;
	struct vfs_inode_t *previous;
	unsigned int leaf_set_flags, leaf_reset_flags;
	const char *leaf_target_path;
};

static void vfs_set_error(struct vfs_t *vfs, enum vfs_internal_error_t error_code, const char *msg, ...) {
	vfs->error.code = error_code;

	va_list ap;
	va_start(ap, msg);
	vsnprintf(vfs->error.string, VFS_MAX_ERROR_LENGTH - 1, msg, ap);
	va_end(ap);
}

static struct vfs_inode_t *vfs_find_inode(struct vfs_t *vfs, const char *virtual_path) {
	for (unsigned int i = 0; i < vfs->inode.count; i++) {
		struct vfs_inode_t *inode = vfs->inode.data[i];

		/* Search for match with trailing slash */
		if (pathcmp(inode->virtual_path, virtual_path)) {
			return inode;
		}
	}
	return NULL;
}

static bool vfs_set_cwd(struct vfs_t *vfs, const char *new_cwd) {
	if (!is_absolute_path(new_cwd)) {
		vfs_set_error(vfs, VFS_CWD_ILLEGAL, "working directory must be an absolute path");
		return false;
	}

	int len = strlen(new_cwd);
	if (len + 1 > vfs->cwd.alloced_size) {
		/* Realloc necessary */
		char *realloced_cwd = realloc(vfs->cwd.path, len + 1);
		if (!realloced_cwd) {
			vfs_set_error(vfs, VFS_CWD_OUT_OF_MEMORY, "out of memory when changing directory");
			return false;
		}
		vfs->cwd.path = realloced_cwd;
		vfs->cwd.length = len;
		vfs->cwd.alloced_size = len + 1;
	}
	strcpy(vfs->cwd.path, new_cwd);
	truncate_trailing_slash(vfs->cwd.path);
	return true;
}

static struct vfs_inode_t* vfs_add_single_inode(struct vfs_t *vfs, const char *virtual_path, const char *target_path, unsigned int set_flags, unsigned int reset_flags, struct vfs_inode_t *parent);

static bool vfs_add_inode_splitter(const char *path, bool is_full_path, void *vctx) {
	struct vfs_inode_adding_ctx_t *ctx = (struct vfs_inode_adding_ctx_t*)vctx;
	struct vfs_inode_t *inode = vfs_find_inode(ctx->vfs, path);
	if (!inode) {
		/* Create this inode */
		if (!is_full_path) {
			ctx->previous = vfs_add_single_inode(ctx->vfs, path, NULL, 0, 0, ctx->previous);
		} else {
			ctx->previous = vfs_add_single_inode(ctx->vfs, path, ctx->leaf_target_path, ctx->leaf_set_flags, ctx->leaf_reset_flags, ctx->previous);
		}
	} else {
		ctx->previous = inode;
	}
	return true;
}

static struct vfs_inode_t* vfs_add_single_inode(struct vfs_t *vfs, const char *virtual_path, const char *target_path, unsigned int set_flags, unsigned int reset_flags, struct vfs_inode_t *parent) {
	char *vpath_copy = strdup(virtual_path);
	if (!vpath_copy) {
		vfs_set_error(vfs, VFS_ADD_INODE_OUT_OF_MEMORY, "error dupping virtual path (%s)", strerror(errno));
		return NULL;
	}
	truncate_trailing_slash(vpath_copy);

	char *tpath_copy = target_path ? strdup(target_path) : NULL;
	if (target_path && (!tpath_copy)) {
		free(vpath_copy);
		vfs_set_error(vfs, VFS_ADD_INODE_OUT_OF_MEMORY, "error dupping real path (%s)", strerror(errno));
		return NULL;
	}
	if (tpath_copy) {
		truncate_trailing_slash(tpath_copy);
	}

	struct vfs_inode_t *new_inode = calloc(1, sizeof(struct vfs_inode_t));
	if (!new_inode) {
		free(vpath_copy);
		free(tpath_copy);
		vfs_set_error(vfs, VFS_ADD_INODE_OUT_OF_MEMORY, "error creating inode (%s)", strerror(errno));
		return NULL;
	}
	new_inode->parent = parent;
	new_inode->set_flags = set_flags;
	new_inode->reset_flags = reset_flags;
	new_inode->virtual_path = vpath_copy;
	new_inode->target_path = tpath_copy;
	new_inode->vlen = strlen(virtual_path);
	new_inode->tlen = target_path ? strlen(target_path) : 0;

	struct vfs_inode_t **new_inodes = realloc(vfs->inode.data, sizeof(struct vfs_inode_t*) * (vfs->inode.count + 1));
	if (!new_inodes) {
		free(new_inode);
		free(tpath_copy);
		free(vpath_copy);
		vfs_set_error(vfs, VFS_ADD_INODE_OUT_OF_MEMORY, "error reallocating inode memory (%s)", strerror(errno));
		return NULL;
	}
	vfs->inode.data = new_inodes;
	vfs->inode.count++;
	vfs->inode.data[vfs->inode.count - 1] = new_inode;

	return new_inode;
}

bool vfs_add_inode(struct vfs_t *vfs, const char *virtual_path, const char *target_path, unsigned int set_flags, unsigned int reset_flags) {
	if (!is_absolute_path(virtual_path)) {
		vfs_set_error(vfs, VFS_ADD_INODE_PARAMETER_ERROR, "virtual path must start with a '/' character");
		return false;
	}

	if (target_path && !is_absolute_path(target_path)) {
		vfs_set_error(vfs, VFS_ADD_INODE_PARAMETER_ERROR, "target path must start with a '/' character");
		return false;
	}

	if (vfs_find_inode(vfs, virtual_path)) {
		vfs_set_error(vfs, VFS_ADD_INODE_ALREADY_EXISTS, "virtual path inode for '%s' is duplicate", virtual_path);
		return false;
	}

	struct vfs_inode_adding_ctx_t ctx = {
		.vfs = vfs,
		.previous = NULL,
		.leaf_set_flags = set_flags,
		.leaf_reset_flags = reset_flags,
		.leaf_target_path = target_path,
	};
	path_split(virtual_path, vfs_add_inode_splitter, &ctx);

	return true;
}

static bool vfs_lookup_splitter(const char *path, bool is_full_path, void *vctx) {
	struct vfs_lookup_traversal_t *ctx = (struct vfs_lookup_traversal_t*)vctx;
	struct vfs_inode_t *inode = vfs_find_inode(ctx->vfs, path);
	if (inode) {
		ctx->map_result->flags = (ctx->map_result->flags | inode->set_flags) & ~inode->reset_flags;
		if (is_full_path) {
			ctx->map_result->virtual_directory = true;
		}
		if (inode->target_path) {
			ctx->map_result->mountpoint = inode;
		}
	}
	return true;
}

bool vfs_lookup(struct vfs_t *vfs, struct vfs_lookup_result_t *result, const char *path) {
	if (!vfs->inode.frozen) {
		vfs_set_error(vfs, VFS_INODE_FINALIZATION_ERROR, "inodes not frozen");
		return false;
	}

	if (!is_absolute_path(path)) {
		vfs_set_error(vfs, VFS_LOOKUP_NON_ABSOLUTE, "can only look up absolute path in VFS");
		return false;
	}

	size_t len = strlen(path);
	size_t total_len = len;

	bool absolute_path = is_absolute_path(path);
	if (!absolute_path) {
		total_len += vfs->cwd.length;
	}

	char mpath[total_len + 1];
	if (absolute_path) {
		strcpy(mpath, path);
	} else {
		strcpy(mpath, vfs->cwd.path);
		strcpy(mpath + vfs->cwd.length, path);
	}

	memset(result, 0, sizeof(*result));
	result->flags = vfs->inode.base_flags;

	struct vfs_lookup_traversal_t ctx = {
		.vfs = vfs,
		.map_result = result,
	};
	path_split(mpath, vfs_lookup_splitter, &ctx);
	return true;
}

static int vfs_inode_comparator(const void *velem1, const void *velem2) {
	const struct vfs_inode_t **elem1 = (const struct vfs_inode_t **)velem1;
	const struct vfs_inode_t **elem2 = (const struct vfs_inode_t **)velem2;
	const struct vfs_inode_t *inode1 = *elem1;
	const struct vfs_inode_t *inode2 = *elem2;
	return strcmp(inode1->virtual_path, inode2->virtual_path);
}

void vfs_freeze_inodes(struct vfs_t *vfs) {
	if (vfs->inode.frozen) {
		vfs_set_error(vfs, VFS_INODE_FINALIZATION_ERROR, "inodes already frozen");
	}
	if (vfs->inode.count) {
		qsort(vfs->inode.data, vfs->inode.count, sizeof(struct vfs_inode_t*), vfs_inode_comparator);
	}
	vfs->inode.frozen = true;
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

	vfs->handles.max_count = 10;

	return vfs;
}

void vfs_handle_free(struct vfs_handle_t *handle) {
	free(handle);
}

void vfs_free(struct vfs_t *vfs) {
	if (!vfs) {
		return;
	}
	free(vfs->cwd.path);
	for (unsigned int i = 0; i < vfs->inode.count; i++) {
		free(vfs->inode.data[i]->virtual_path);
		free(vfs->inode.data[i]->target_path);
		free(vfs->inode.data[i]);
	}
	free(vfs->inode.data);
	free(vfs);
}

static char *vfs_map_path(const struct vfs_lookup_result_t *lookup, const char *virtual_path) {
	return NULL;
}

enum vfs_error_t vfs_opendir(struct vfs_t *vfs, const char *path, struct vfs_handle_t **handle_ptr) {
	*handle_ptr = NULL;
	if (!path) {
		vfs_set_error(vfs, VFS_ILLEGAL_PATH, "vfs_opendir() recevied NULL path");
		return VFS_INTERNAL_ERROR;
	}

	if (vfs->handles.current_count >= vfs->handles.max_count) {
		return VFS_OUT_OF_HANDLES;
	}

	char *virtual_path = sanitize_path(vfs->cwd.path, path);
	if (!virtual_path) {
		vfs_set_error(vfs, VFS_SANITIZE_PATH_ERROR, "vfs_opendir() could not sanitize path successfully");
		return VFS_INTERNAL_ERROR;
	}

	struct vfs_lookup_result_t lookup;
	if (!vfs_lookup(vfs, &lookup, virtual_path)) {
		free(virtual_path);
		vfs_set_error(vfs, VFS_INODE_LOOKUP_ERROR, "vfs_opendir() could not lookup path successfully");
		return VFS_INTERNAL_ERROR;
	}

	if (!lookup.mountpoint) {
		free(virtual_path);
		return VFS_NO_SUCH_FILE_OR_DIRECTORY;
	}

	if (lookup.flags & VFS_INODE_FLAG_FILTER_ALL) {
		free(virtual_path);
		return VFS_NO_SUCH_FILE_OR_DIRECTORY;
	}

	if (lookup.flags & VFS_INODE_FLAG_FILTER_HIDDEN) {
		if (path_contains_hidden(virtual_path)) {
			free(virtual_path);
			return VFS_NO_SUCH_FILE_OR_DIRECTORY;
		}
	}

	char *mapped_path = vfs_map_path(&lookup, virtual_path);
	if (!mapped_path) {
		free(virtual_path);
		vfs_set_error(vfs, VFS_PATH_MAP_ERROR, "vfs_opendir() could not map path successfully");
		return VFS_INTERNAL_ERROR;
	}

	free(mapped_path);
	free(virtual_path);
	return VFS_OK;
}
