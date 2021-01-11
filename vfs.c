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

typedef bool (*path_split_callback_t)(const char *path, void *vctx);

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

static struct vfs_mapping_target_t *vfs_find_mapping(struct vfs_t *vfs, const char *virtual_path) {
	int vpath_len = strlen(virtual_path);

	for (unsigned int i = 0; i < vfs->mapping_count; i++) {
		struct vfs_mapping_target_t *mapping = &vfs->mappings[i];

		/* Search for match with trailing slash */
		if (!strcmp(mapping->virtual_path, virtual_path)) {
			return mapping;
		}

		/* But also find it if there is no trailing slash */
		if ((vpath_len == mapping->vlen - 1) && (!strncmp(mapping->virtual_path, virtual_path, mapping->vlen - 1))) {
			return mapping;
		}

	}
	return NULL;
}

static bool vfs_set_cwd(struct vfs_t *vfs, const char *new_cwd) {
	if (new_cwd[0] != '/') {
		vfs_set_error(vfs, VFS_CWD_ILLEGAL, "working directory must start with '/' character");
		return false;
	}

	int len = strlen(new_cwd);
	if (new_cwd[len - 1] != '/') {
		vfs_set_error(vfs, VFS_CWD_ILLEGAL, "working directory must end with '/' character");
		return false;
	}

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

bool vfs_add_mapping(struct vfs_t *vfs, const char *virtual_path, const char *real_path, unsigned int flags) {
	size_t vlen = strlen(virtual_path);
	if (vlen == 0) {
		vfs_set_error(vfs, VFS_ADD_MAPPING_PARAMETER_ERROR, "empty virtual path is disallowed");
		return false;
	}
	if (virtual_path[0] != '/') {
		vfs_set_error(vfs, VFS_ADD_MAPPING_PARAMETER_ERROR, "virtual path must start with a '/' character");
		return false;
	}
	if (virtual_path[vlen - 1] != '/') {
		vfs_set_error(vfs, VFS_ADD_MAPPING_PARAMETER_ERROR, "virtual path must end with a '/' character");
		return false;
	}

	size_t rlen = 0;
	if (real_path) {
		rlen = strlen(real_path);
		if (rlen == 0) {
			vfs_set_error(vfs, VFS_ADD_MAPPING_PARAMETER_ERROR, "empty real path is disallowed");
			return false;
		}
		if (real_path[0] != '/') {
			vfs_set_error(vfs, VFS_ADD_MAPPING_PARAMETER_ERROR, "real path must begin with a '/' character");
			return false;
		}
		if (real_path[rlen - 1] != '/') {
			vfs_set_error(vfs, VFS_ADD_MAPPING_PARAMETER_ERROR, "real path must end with a '/' character");
			return false;
		}
	}

	if (vfs_find_mapping(vfs, virtual_path)) {
		vfs_set_error(vfs, VFS_ADD_MAPPING_ALREADY_EXISTS, "virtual path mapping for '%s' is duplicate", virtual_path);
		return false;
	}

	char *vpath_copy = strdup(virtual_path);
	if (!vpath_copy) {
		vfs_set_error(vfs, VFS_ADD_MAPPING_OUT_OF_MEMORY, "error dupping virtual path (%s)", strerror(errno));
		return false;
	}

	char *rpath_copy = real_path ? strdup(real_path) : NULL;
	if (real_path && (!rpath_copy)) {
		free(vpath_copy);
		vfs_set_error(vfs, VFS_ADD_MAPPING_OUT_OF_MEMORY, "error dupping real path (%s)", strerror(errno));
		return false;
	}

	struct vfs_mapping_target_t *new_mappings = realloc(vfs->mappings, sizeof(struct vfs_mapping_target_t) * (vfs->mapping_count + 1));
	if (!new_mappings) {
		vfs_set_error(vfs, VFS_ADD_MAPPING_OUT_OF_MEMORY, "error reallocating mapping memory (%s)", strerror(errno));
		return false;
	}
	vfs->mappings = new_mappings;
	vfs->mapping_count++;

	struct vfs_mapping_target_t *new_mapping = &vfs->mappings[vfs->mapping_count - 1];
	memset(new_mapping, 0, sizeof(*new_mapping));
	new_mapping->virtual_path = vpath_copy;
	new_mapping->flags = flags;
	new_mapping->target = rpath_copy;
	new_mapping->vlen = vlen;
	new_mapping->rlen = rlen;

	return true;
}

static bool vfs_map_splitter(const char *path, void *vctx) {
	struct vfs_map_traversal_t *ctx = (struct vfs_map_traversal_t*)vctx;
	struct vfs_mapping_target_t *mapping = vfs_find_mapping(ctx->vfs, path);
	if (mapping) {
		if (mapping->flags & VFS_MAPPING_FLAG_RESET) {
			ctx->map_result->flags = mapping->flags & ~VFS_MAPPING_FLAG_RESET;
		} else {
			ctx->map_result->flags |= mapping->flags;
		}
		ctx->map_result->target = mapping;
		if (mapping->target) {
			ctx->map_result->mountpoint = mapping;
		}
	}
	return true;
}

static void path_split(char *path, path_split_callback_t callback, void *vctx) {
	size_t len = strlen(path);
	for (size_t i = 0; i < len; i++) {
		if (path[i] == '/') {
			char saved = path[i + 1];
			path[i + 1] = 0;
			bool continue_splitting = callback(path, vctx);
			path[i + 1] = saved;
			if (!continue_splitting) {
				break;
			}
		}
	}
}

bool vfs_map(struct vfs_t *vfs, struct vfs_map_result_t *result, const char *path) {
	if (!vfs->mappings_finalized) {
		vfs_set_error(vfs, VFS_MAPPING_FINALIZATION_ERROR, "mappings not finalized");
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
	struct vfs_mapping_target_t *mapping = vfs_find_mapping(vfs, path);
	if (!mapping) {
		vfs_add_mapping(vfs, path, NULL, 0);
	}
	return true;
}

static int vfs_mapping_comparator(const void *velem1, const void *velem2) {
	const struct vfs_mapping_target_t *elem1 = (const struct vfs_mapping_target_t *)velem1;
	const struct vfs_mapping_target_t *elem2 = (const struct vfs_mapping_target_t *)velem2;
	return strcmp(elem1->virtual_path, elem2->virtual_path);
}

void vfs_finalize_mappings(struct vfs_t *vfs) {
	if (vfs->mappings_finalized) {
		vfs_set_error(vfs, VFS_MAPPING_FINALIZATION_ERROR, "mappings already finalized");
	}

	unsigned int old_mapping_count = vfs->mapping_count;
	for (unsigned int i = 0; i < old_mapping_count; i++) {
		struct vfs_mapping_target_t *mapping = &vfs->mappings[i];
		char mpath[mapping->vlen + 1];
		strcpy(mpath, mapping->virtual_path);
		path_split(mpath, vfs_finalization_splitter, vfs);
	}
	qsort(vfs->mappings, vfs->mapping_count, sizeof(struct vfs_mapping_target_t), vfs_mapping_comparator);
	vfs->mappings_finalized = true;
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
	for (unsigned int i = 0; i < vfs->mapping_count; i++) {
		free(vfs->mappings[i].target);
		free(vfs->mappings[i].virtual_path);
	}
	free(vfs->mappings);
	free(vfs);
}

static void vfs_dump_flags(FILE *f, unsigned int flags) {
	if (flags & VFS_MAPPING_FLAG_RESET) {
		fprintf(f, " RESET");
	}
	if (flags & VFS_MAPPING_FLAG_READ_ONLY) {
		fprintf(f, " READ_ONLY");
	}
	if (flags & VFS_MAPPING_FLAG_FILTER_ALL) {
		fprintf(f, " FILTER_ALL");
	}
	if (flags & VFS_MAPPING_FLAG_FILTER_HIDDEN) {
		fprintf(f, " FILTER_HIDDEN");
	}
	if (flags & VFS_MAPPING_FLAG_DISALLOW_CREATE_FILE) {
		fprintf(f, " DISALLOW_CREATE_FILE");
	}
	if (flags & VFS_MAPPING_FLAG_DISALLOW_CREATE_DIR) {
		fprintf(f, " DISALLOW_CREATE_DIR");
	}
	if (flags & VFS_MAPPING_FLAG_DISALLOW_UNLINK) {
		fprintf(f, " DISALLOW_UNLINK");
	}
}

static void vfs_dump_mapping_target(FILE *f, const struct vfs_mapping_target_t *mapping) {
	fprintf(f, "%s", mapping->virtual_path);
	if (mapping->target) {
		fprintf(f, " => %s", mapping->target);
	}
	if (mapping->flags) {
		fprintf(f, " [flags: 0x%x ", mapping->flags);
		vfs_dump_flags(f, mapping->flags);
		fprintf(f, "]");
	}
}

void vfs_dump(FILE *f, const struct vfs_t *vfs) {
	fprintf(f, "VFS details:\n");
	if (vfs->last_error) {
		fprintf(f, "   Last error: %d (%s)\n", vfs->last_error, vfs->error_str);
	}
	fprintf(f, "   Max handles: %d, Mappings: %d\n", vfs->max_handle_count, vfs->mapping_count);
	fprintf(f, "   Base flags: 0x%x ", vfs->base_flags);
	vfs_dump_flags(f, vfs->base_flags);
	fprintf(f, "\n");
	for (unsigned int i = 0; i < vfs->mapping_count; i++) {
		struct vfs_mapping_target_t *mapping = &vfs->mappings[i];
		fprintf(f, "   Mapping %2d of %d: ", i + 1, vfs->mapping_count);
		vfs_dump_mapping_target(f, mapping);
		fprintf(f, "\n");
	}
}

void vfs_dump_map_result(FILE *f, const struct vfs_map_result_t *map_result) {
	fprintf(f, "Map result flags: 0x%x ", map_result->flags);
	vfs_dump_flags(f, map_result->flags);
	fprintf(f, "\n");
	if (map_result->mountpoint) {
		fprintf(f, "Mountpoint: ");
		vfs_dump_mapping_target(f, map_result->mountpoint);
		fprintf(f, "\n");
	} else {
		fprintf(f, "No mountpoint.\n");
	}
	if (map_result->target) {
		fprintf(f, "Target: ");
		vfs_dump_mapping_target(f, map_result->target);
		fprintf(f, "\n");
	} else {
		fprintf(f, "No target.\n");
	}
}

#ifdef __VFS_TEST__
int main(void) {
	struct vfs_t *vfs = vfs_init();
	vfs_add_mapping(vfs, "/pics/", "/home/joe/pics/", 0);
	vfs_add_mapping(vfs, "/pics/foo/neu/", "/home/joe/pics_neu/", VFS_MAPPING_FLAG_READ_ONLY);
	vfs_add_mapping(vfs, "/this/is/", NULL, VFS_MAPPING_FLAG_DISALLOW_CREATE_DIR);
	vfs_add_mapping(vfs, "/this/is/deeply/nested/", "/home/joe/nested/", VFS_MAPPING_FLAG_READ_ONLY);
	vfs_add_mapping(vfs, "/zeug/", "/tmp/zeug/", 0);
	vfs_add_mapping(vfs, "/incoming/", "/tmp/write/", VFS_MAPPING_FLAG_DISALLOW_UNLINK);
	fprintf(stderr, "=========================\n");
	vfs_finalize_mappings(vfs);
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
