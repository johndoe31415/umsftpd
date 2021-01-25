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
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "vfs.h"
#include "logging.h"
#include "strings.h"

static const char *mode_string_mapping[] = {
	[FILEMODE_READ] = "r",
	[FILEMODE_WRITE] = "w",
	[FILEMODE_APPEND] = "a",
};

static struct vfs_inode_t* vfs_add_single_inode(struct vfs_t *vfs, const char *virtual_path, const char *target_path, unsigned int flags_set, unsigned int flags_reset, struct vfs_inode_t *parent);

struct vfs_lookup_traversal_t {
	struct vfs_t *vfs;
	struct vfs_lookup_result_t *map_result;
};

struct vfs_inode_adding_ctx_t {
	struct vfs_t *vfs;
	struct vfs_inode_t *previous;
	unsigned int leaf_flags_set, leaf_flags_reset;
	const char *leaf_target_path;
};

struct vfs_validate_constraints_ctx_t {
	bool constraints_fulfilled;
	unsigned int flags;
};

const char *vfs_error_str(enum vfs_error_t error_code) {
	switch (error_code) {
		case VFS_OK: return "success";
		case VFS_OUT_OF_HANDLES: return "out of handles";
		case VFS_PERMISSION_DENIED: return "permission denied";
		case VFS_NO_SUCH_FILE_OR_DIRECTORY: return "no such file or directory";
		case VFS_INTERNAL_ERROR: return "internal error";
		case VFS_NOT_A_DIRECTORY: return "not a directory";
		case VFS_NOT_A_FILE: return "not a file";
		case VFS_IO_ERROR: return "I/O error";
	}
	return "unknown error";
}

static void vfs_set_error(struct vfs_t *vfs, enum vfs_internal_error_t error_code, const char *msg, ...) {
	vfs->error.code = error_code;

	va_list ap;
	va_start(ap, msg);
	vsnprintf(vfs->error.string, VFS_MAX_ERROR_LENGTH - 1, msg, ap);
	va_end(ap);

	logmsg(LLVL_ERROR, "VFS error %d: %s", vfs->error.code, vfs->error.string);
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

static bool vfs_add_inode_splitter(const char *path, bool is_full_path, void *vctx) {
	struct vfs_inode_adding_ctx_t *ctx = (struct vfs_inode_adding_ctx_t*)vctx;
	struct vfs_inode_t *inode = vfs_find_inode(ctx->vfs, path);
	if (!inode) {
		/* Create this inode */
		if (!is_full_path) {
			ctx->previous = vfs_add_single_inode(ctx->vfs, path, NULL, 0, 0, ctx->previous);
		} else {
			ctx->previous = vfs_add_single_inode(ctx->vfs, path, ctx->leaf_target_path, ctx->leaf_flags_set, ctx->leaf_flags_reset, ctx->previous);
		}
	} else {
		ctx->previous = inode;
	}
	return true;
}

static struct vfs_inode_t* vfs_add_single_inode(struct vfs_t *vfs, const char *virtual_path, const char *target_path, unsigned int flags_set, unsigned int flags_reset, struct vfs_inode_t *parent) {
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
	if (parent) {
		const char *virt_basename = const_basename(vpath_copy);
		stringlist_insert(parent->virtual_subdirs, virt_basename);
	}
	new_inode->parent = parent;
	new_inode->flags_set = flags_set;
	new_inode->flags_reset = flags_reset;
	new_inode->virtual_path = vpath_copy;
	new_inode->target_path = tpath_copy;
	new_inode->vlen = strlen(vpath_copy);
	new_inode->tlen = target_path ? strlen(tpath_copy) : 0;
	new_inode->virtual_subdirs = stringlist_new();

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

bool vfs_add_inode(struct vfs_t *vfs, const char *virtual_path, const char *target_path, unsigned int flags_set, unsigned int flags_reset) {
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
		.leaf_flags_set = flags_set,
		.leaf_flags_reset = flags_reset,
		.leaf_target_path = target_path,
	};
	path_split(virtual_path, vfs_add_inode_splitter, &ctx);

	return true;
}

static bool vfs_lookup_splitter(const char *path, bool is_full_path, void *vctx) {
	struct vfs_lookup_traversal_t *ctx = (struct vfs_lookup_traversal_t*)vctx;
	struct vfs_inode_t *inode = vfs_find_inode(ctx->vfs, path);
	if (inode) {
		ctx->map_result->flags = (ctx->map_result->flags | inode->flags_set) & ~inode->flags_reset;
		if (is_full_path) {
			ctx->map_result->inode = inode;
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

void vfs_free(struct vfs_t *vfs) {
	if (!vfs) {
		return;
	}
	free(vfs->cwd.path);
	for (unsigned int i = 0; i < vfs->inode.count; i++) {
		free(vfs->inode.data[i]->virtual_path);
		free(vfs->inode.data[i]->target_path);
		stringlist_free(vfs->inode.data[i]->virtual_subdirs);
		free(vfs->inode.data[i]);
	}
	free(vfs->inode.data);
	free(vfs);
}

static char *vfs_map_path(struct vfs_t *vfs, const struct vfs_lookup_result_t *lookup, const char *virtual_path) {
	if (!lookup) {
		vfs_set_error(vfs, VFS_MISSING_ARGUMENT, "vfs_map_path() recevied NULL lookup");
		return NULL;
	}
	if (!virtual_path) {
		vfs_set_error(vfs, VFS_MISSING_ARGUMENT, "vfs_map_path() recevied NULL virtual_path");
		return NULL;
	}
	if (!is_absolute_path(virtual_path)) {
		vfs_set_error(vfs, VFS_PATH_NOT_ABSOLUTE, "vfs_map_path() recevied non-absolute virtual_path");
		return NULL;
	}
	if (!lookup->mountpoint) {
		vfs_set_error(vfs, VFS_NOT_MOUNTED, "vfs_map_path() has non-mounted lookup");
		return NULL;
	}

	size_t virtual_path_length = strlen(virtual_path);
	if (virtual_path_length < lookup->mountpoint->vlen) {
		vfs_set_error(vfs, VFS_ILLEGAL_PATH, "vfs_map_path() has received shorter virtual path (%s) than mountpoint (%s); something is wrong.", virtual_path, lookup->mountpoint->virtual_path);
		return NULL;
	}

	char *result = NULL;
	if (virtual_path_length > lookup->mountpoint->vlen) {
		const char *virtual_path_suffix = virtual_path + lookup->mountpoint->vlen + 1;
		size_t suffix_length = virtual_path_length - lookup->mountpoint->vlen - 1;

		size_t memory_requirement = lookup->mountpoint->tlen + 1 + suffix_length + 1;
		result = malloc(memory_requirement);
		if (!result) {
			vfs_set_error(vfs, VFS_OUT_OF_MEMORY, "vfs_map_path() could not allocate memory for mapped path");
			return NULL;
		}

		memcpy(result, lookup->mountpoint->target_path, lookup->mountpoint->tlen);
		result[lookup->mountpoint->tlen] = '/';
		memcpy(result + lookup->mountpoint->tlen + 1, virtual_path_suffix, suffix_length);
		result[lookup->mountpoint->tlen + 1 + suffix_length] = 0;
	} else if (virtual_path_length == lookup->mountpoint->vlen) {
		size_t memory_requirement = lookup->mountpoint->tlen + 1;
		result = malloc(memory_requirement);
		if (!result) {
			vfs_set_error(vfs, VFS_OUT_OF_MEMORY, "vfs_map_path() could not allocate memory for mapped path");
			return NULL;
		}
		strcpy(result, lookup->mountpoint->target_path);
	}

	return result;
}

static enum vfs_error_t vfs_open_node(struct vfs_t *vfs, const char *path, struct vfs_handle_t **handle_ptr) {
	*handle_ptr = NULL;

	if (!path) {
		vfs_set_error(vfs, VFS_ILLEGAL_PATH, "vfs_opendir() recevied NULL path");
		return VFS_INTERNAL_ERROR;
	}

	if (vfs->handles.current_count >= vfs->handles.max_count) {
		logmsg(LLVL_ERROR, "vfs_open_node() ran out of handles (%d maximum).", vfs->handles.max_count);
		return VFS_OUT_OF_HANDLES;
	}

	struct vfs_handle_t *handle = calloc(1, sizeof(struct vfs_handle_t));
	if (!handle) {
		vfs_set_error(vfs, VFS_OUT_OF_MEMORY, "vfs_opendir() could not allocate handle memory");
		return VFS_INTERNAL_ERROR;
	}

	handle->virtual_path = sanitize_path(vfs->cwd.path, path);
	if (!handle->virtual_path) {
		vfs_set_error(vfs, VFS_SANITIZE_PATH_ERROR, "vfs_opendir() could not sanitize path successfully");
		vfs_close_handle(handle);
		return VFS_INTERNAL_ERROR;
	}

	struct vfs_lookup_result_t lookup;
	if (!vfs_lookup(vfs, &lookup, handle->virtual_path)) {
		vfs_set_error(vfs, VFS_INODE_LOOKUP_ERROR, "vfs_opendir() could not lookup path successfully");
		vfs_close_handle(handle);
		return VFS_INTERNAL_ERROR;
	}

	handle->inode = lookup.inode;
	handle->flags = lookup.flags;

	if (lookup.flags & VFS_INODE_FLAG_FILTER_ALL) {
		logmsg(LLVL_DEBUG, "vfs_open_node() returning 'no such file or directory' because virtual path \"%s\" is filtered.", handle->virtual_path);
		vfs_close_handle(handle);
		return VFS_NO_SUCH_FILE_OR_DIRECTORY;
	}

	if (lookup.flags & VFS_INODE_FLAG_FILTER_HIDDEN) {
		if (path_contains_hidden(handle->virtual_path)) {
			logmsg(LLVL_DEBUG, "vfs_open_node() returning 'permission denied' because virtual path \"%s\" contains hidden elements.", handle->virtual_path);
			vfs_close_handle(handle);
			return VFS_PERMISSION_DENIED;
		}
	}

	if ((!handle->inode) && (!lookup.mountpoint)) {
		logmsg(LLVL_DEBUG, "vfs_open_node() returning 'no such file or directory' because no mountpoint exists for \"%s\".", handle->virtual_path);
		vfs_close_handle(handle);
		return VFS_NO_SUCH_FILE_OR_DIRECTORY;
	}

	if (lookup.mountpoint) {
		handle->mapped_path = vfs_map_path(vfs, &lookup, handle->virtual_path);
		if (!handle->mapped_path) {
			vfs_set_error(vfs, VFS_PATH_MAP_ERROR, "vfs_opendir() could not map path successfully");
			vfs_close_handle(handle);
			return VFS_INTERNAL_ERROR;
		}

		if (!(lookup.flags & VFS_INODE_FLAG_ALLOW_SYMLINKS)) {
			struct symlink_check_response_t symlink = path_contains_symlink(handle->mapped_path);
			if (symlink.critical_error) {
				/* Error checking for symlinks, better reject */
				logmsg(LLVL_ERROR, "vfs_open_node() failed to check symlinks of %s: %s", handle->mapped_path, strerror(errno));
				vfs_close_handle(handle);
				return VFS_INTERNAL_ERROR;
			}
			if (symlink.contains_symlink) {
				/* Symlinks disallowed, but somewhere in real path symlinks are
				 * present -> pretend this node does not exist */
				logmsg(LLVL_DEBUG, "vfs_open_node() returning 'no such file or directory' because disallowed symlinks present in \"%s\".", handle->virtual_path);
				vfs_close_handle(handle);
				return VFS_NO_SUCH_FILE_OR_DIRECTORY;
			}
		}
	}

	*handle_ptr = handle;
	return VFS_OK;
}

static enum vfs_error_t vfs_errno_to_vfs_error(int errnum) {
	switch (errnum) {
		case EACCES:
			return VFS_PERMISSION_DENIED;

		case ENOENT:
			return VFS_NO_SUCH_FILE_OR_DIRECTORY;

		default:
			return VFS_INTERNAL_ERROR;
	}
}

enum vfs_error_t vfs_chdir(struct vfs_t *vfs, const char *path) {
	struct vfs_handle_t *handle;
	enum vfs_error_t result = vfs_open_node(vfs, path, &handle);
	if (result != VFS_OK) {
		return result;
	}

	if (handle->inode) {
		/* We always allow chdir to a virtual directory */
		bool success = vfs_set_cwd(vfs, handle->virtual_path);
		vfs_close_handle(handle);
		return success ? VFS_OK : VFS_INTERNAL_ERROR;
	} else {
		struct stat statbuf;
		if (stat(handle->mapped_path, &statbuf)) {
			enum vfs_error_t error_code = vfs_errno_to_vfs_error(errno);
			logmsg(LLVL_WARN, "vfs_chdir() refused to change directory to mapped %s; stat failed", handle->mapped_path);
			vfs_close_handle(handle);
			return error_code;
		}

		if (!S_ISDIR(statbuf.st_mode)) {
			logmsg(LLVL_WARN, "vfs_chdir() refused to change directory to mapped %s; not a directory", handle->mapped_path);
			vfs_close_handle(handle);
			return VFS_NOT_A_DIRECTORY;
		}

		if (!vfs_set_cwd(vfs, handle->virtual_path)) {
			logmsg(LLVL_ERROR, "vfs_chdir() failed to change directory to %s", handle->virtual_path);
			vfs_close_handle(handle);
			return VFS_INTERNAL_ERROR;
		}
	}

	vfs_close_handle(handle);
	return VFS_OK;
}

enum vfs_error_t vfs_opendir(struct vfs_t *vfs, const char *path, struct vfs_handle_t **handle_ptr) {
	enum vfs_error_t result = vfs_open_node(vfs, path, handle_ptr);
	if (result != VFS_OK) {
		return result;
	}

	struct vfs_handle_t *handle = *handle_ptr;
	handle->type = DIR_HANDLE;

	handle->dir.dir = opendir(handle->mapped_path);
	if (!handle->dir.dir) {
		logmsg(LLVL_DEBUG, "vfs_opendir() cannot open %s (%s), but is a virtual directory at %p", handle->mapped_path, strerror(errno), handle->inode);

//		logmsg(LLVL_WARN, "vfs_opendir() got invalid handle type %u", handle->type);
//		vfs_set_error(vfs, VFS_OPENDIR_FAILED, "vfs_opendir() failed to call opendir(3) on \"%s\": %s", handle->mapped_path, strerror(errno));
//		vfs_close_handle(handle);
//		return vfs_errno_to_vfs_error(errno);
	}

	return VFS_OK;
}

enum vfs_error_t vfs_open(struct vfs_t *vfs, const char *path, enum vfs_filemode_t mode, struct vfs_handle_t **handle_ptr) {
	enum vfs_error_t result = vfs_open_node(vfs, path, handle_ptr);
	if (result != VFS_OK) {
		return result;
	}

	struct vfs_handle_t *handle = *handle_ptr;
	handle->type = FILE_HANDLE;

	struct stat statbuf;
	int stat_result = stat(handle->mapped_path, &statbuf);
	if (stat_result == -1) {
		/* stat failed; this is only okay if we're writing and the stat failed
		 * because the file did not exist. */
		if ((errno != ENOENT) || ((mode == FILEMODE_READ) && (errno != ENOENT))) {
			enum vfs_error_t error_code = vfs_errno_to_vfs_error(errno);
			logmsg(LLVL_DEBUG, "vfs_open() had error when running stat(): %s", strerror(errno));
			vfs_close_handle(handle);
			return error_code;
		}
	} else {
		/* stat successful, then we require that it's actually a file */
		if (!S_ISREG(statbuf.st_mode)) {
			/* not a file */
			logmsg(LLVL_DEBUG, "vfs_open() refusing to open non-file");
			vfs_close_handle(handle);
			return VFS_NOT_A_FILE;
		}
	}

	if ((handle->flags & VFS_INODE_FLAG_READ_ONLY) && (mode != FILEMODE_READ)) {
		/* Requesting writing on a readonly file */
		logmsg(LLVL_DEBUG, "vfs_open() refusing to open file in write mode when flags indicate read-only");
		vfs_close_handle(handle);
		return VFS_PERMISSION_DENIED;
	}

	handle->file.file = fopen(handle->mapped_path, mode_string_mapping[mode]);
	if (!handle->file.file) {
		/* e.g., permission denied */
		enum vfs_error_t error_code = vfs_errno_to_vfs_error(errno);
		logmsg(LLVL_DEBUG, "vfs_open() got error when calling fopen()");
		vfs_close_handle(handle);
		return error_code;
	}
	return VFS_OK;
}

static void vfs_stat_virtual_directory(const char *virtual_dirname, struct vfs_dirent_t *vfs_dirent, unsigned int flags) {
	*vfs_dirent = (struct vfs_dirent_t) {
		.permissions = (flags & VFS_INODE_FLAG_READ_ONLY) ? 0555 : 0755,
	};
	strncpy(vfs_dirent->filename, virtual_dirname, VFS_MAX_FILENAME_LENGTH - 1);
	vfs_dirent->filename[VFS_MAX_FILENAME_LENGTH - 1] = 0;
}

static void vfs_stat_statbuf(const struct stat *statbuf, struct vfs_dirent_t *vfs_dirent, unsigned int flags) {
	vfs_dirent->eof = false;
	vfs_dirent->is_file = S_ISREG(statbuf->st_mode);
	vfs_dirent->uid = statbuf->st_uid;
	vfs_dirent->gid = statbuf->st_gid;
	vfs_dirent->filesize = statbuf->st_size;
	vfs_dirent->permissions = statbuf->st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
	vfs_dirent->mtime = statbuf->st_mtim;
	vfs_dirent->ctime = statbuf->st_ctim;
	vfs_dirent->atime = statbuf->st_atim;
	if (flags & VFS_INODE_FLAG_READ_ONLY) {
		/* Strip write permission flags if read-only handle */
		vfs_dirent->permissions &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
	}
}

enum vfs_error_t vfs_stat(struct vfs_t *vfs, const char *path, struct vfs_dirent_t *vfs_dirent) {
	struct vfs_handle_t *handle;
	enum vfs_error_t result = vfs_open_node(vfs, path, &handle);
	if (result != VFS_OK) {
		return result;
	}

	if (handle->inode) {
		/* Virtual directory */
		vfs_stat_virtual_directory(const_basename(handle->virtual_path), vfs_dirent, handle->flags);
		vfs_close_handle(handle);
		return VFS_OK;
	}

	/* Mapped directory */
	errno = 0;
	struct stat statbuf;
	int stat_result = stat(handle->mapped_path, &statbuf);
	if (stat_result == -1) {
		/* stat failed */
		enum vfs_error_t error_code = vfs_errno_to_vfs_error(errno);
		vfs_close_handle(handle);
		return error_code;
	}

	strncpy(vfs_dirent->filename, const_basename(handle->virtual_path), VFS_MAX_FILENAME_LENGTH - 1);
	vfs_dirent->filename[VFS_MAX_FILENAME_LENGTH - 1] = 0;
	vfs_stat_statbuf(&statbuf, vfs_dirent, handle->flags);
	vfs_close_handle(handle);
	return VFS_OK;
}

enum vfs_error_t vfs_read(struct vfs_handle_t *handle, void *ptr, size_t *length) {
	if (handle->type != FILE_HANDLE) {
		logmsg(LLVL_WARN, "vfs_read() got invalid handle type %u", handle->type);
		return VFS_INTERNAL_ERROR;
	}

	errno = 0;
	*length = fread(ptr, 1, *length, handle->file.file);
	int fread_errno = errno;

	if (errno) {
		logmsg(LLVL_ERROR, "vfs_read() had I/O error when reading from file: %s", strerror(fread_errno));
	}
	return (fread_errno == 0) ? VFS_OK : VFS_IO_ERROR;
}

enum vfs_error_t vfs_write(struct vfs_handle_t *handle, const void *ptr, size_t *length) {
	if (handle->type != FILE_HANDLE) {
		logmsg(LLVL_WARN, "vfs_write() got invalid handle type %u", handle->type);
		return VFS_INTERNAL_ERROR;
	}

	errno = 0;
	*length = fwrite(ptr, 1, *length, handle->file.file);
	int fwrite_errno = errno;

	if (errno) {
		logmsg(LLVL_ERROR, "vfs_write() had I/O error when writing to file: %s", strerror(fwrite_errno));
	}
	return (fwrite_errno == 0) ? VFS_OK : VFS_IO_ERROR;
}

enum vfs_error_t vfs_readdir(struct vfs_handle_t *handle, struct vfs_dirent_t *vfs_dirent) {
	if (handle->type != DIR_HANDLE) {
		logmsg(LLVL_WARN, "vfs_readdir() got invalid handle type %u", handle->type);
		return VFS_INTERNAL_ERROR;
	}

	if (!handle->inode && !handle->dir.dir) {
		logmsg(LLVL_ERROR, "vfs_readdir() has neither inode nor open directory");
		return VFS_INTERNAL_ERROR;
	}

	if (handle->inode) {
		if (handle->dir.internal_node_index < handle->inode->virtual_subdirs->count) {
			const char *virtual_dirname = handle->inode->virtual_subdirs->strings[handle->dir.internal_node_index];
			handle->dir.internal_node_index++;
			vfs_stat_virtual_directory(virtual_dirname, vfs_dirent, handle->flags);
			return VFS_OK;
		}
	}

	while (handle->dir.dir) {
		errno = 0;
		struct dirent *dirent = readdir(handle->dir.dir);
		if ((dirent == NULL) && (errno != 0)) {
			/* An error occurred while trying to read the directory */
			logmsg(LLVL_ERROR, "vfs_readdir() encountered an error while trying to read directory: %s", strerror(errno));
			return VFS_INTERNAL_ERROR;
		}

		if (dirent == NULL) {
			break;
		}

		if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, "..")) {
			/* We're omitting the '.' and '..' nodes in this listing */
			continue;
		}

		if ((dirent->d_type != DT_REG) && (dirent->d_type != DT_DIR) && (dirent->d_type != DT_LNK)) {
			/* No need to stat if there's a file node which we do not support
			 * at all. */
			continue;
		}

		if (handle->inode && stringlist_contains(handle->inode->virtual_subdirs, dirent->d_name)) {
			/* Provided by directory listing, but overridden by virtual directory */
			continue;
		}

		strncpy(vfs_dirent->filename, dirent->d_name, VFS_MAX_FILENAME_LENGTH - 1);
		vfs_dirent->filename[VFS_MAX_FILENAME_LENGTH - 1] = 0;

		errno = 0;
		struct stat statbuf;
		int stat_result = fstatat(dirfd(handle->dir.dir), vfs_dirent->filename, &statbuf, 0);
		if (stat_result == -1) {
			/* Possibly truncated filename, hence ENOENT. Also could be missing
			 * permissions. In either case, do not return the node if we're not
			 * allowed to stat it. */
			logmsg(LLVL_WARN, "vfs_readdir() encountered error in fstatat: %s", strerror(errno));
			continue;
		}

		unsigned int file_type = statbuf.st_mode & S_IFMT;
		if ((file_type != S_IFDIR) && (file_type != S_IFREG)) {
			/* Special file (block device, char device, FIFO, unknown) */
			continue;
		}
		vfs_stat_statbuf(&statbuf, vfs_dirent, handle->flags);
		return VFS_OK;
	}

	vfs_dirent->eof = true;
	return VFS_OK;
}

void vfs_close_handle(struct vfs_handle_t *handle) {
	if (!handle) {
		return;
	}
	free(handle->mapped_path);
	free(handle->virtual_path);
	if (handle->type == DIR_HANDLE) {
		if (handle->dir.dir) {
			closedir(handle->dir.dir);
		}
	} else if (handle->type == FILE_HANDLE) {
		if (handle->file.file) {
			fclose(handle->file.file);
		}
	}
	free(handle);
}
