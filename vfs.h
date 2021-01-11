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

#ifndef __VFS_H__
#define __VFS_H__

#include <stdio.h>
#include <stdbool.h>
#include "stringlist.h"

#define VFS_MAX_ERROR_LENGTH		64

#define VFS_MAPPING_FLAG_RESET					(1 << 0)
#define VFS_MAPPING_FLAG_READ_ONLY				(1 << 1)
#define VFS_MAPPING_FLAG_FILTER_ALL				(1 << 2)
#define VFS_MAPPING_FLAG_FILTER_HIDDEN			(1 << 3)
#define VFS_MAPPING_FLAG_DISALLOW_CREATE_FILE	(1 << 4)
#define VFS_MAPPING_FLAG_DISALLOW_CREATE_DIR	(1 << 5)
#define VFS_MAPPING_FLAG_DISALLOW_UNLINK		(1 << 6)

struct vfs_mapping_target_t {
	char *virtual_path;
	unsigned int flags;
	char *target;
	size_t vlen, rlen;
};

struct vfs_map_result_t {
	unsigned int flags;
	struct vfs_mapping_target_t *mountpoint;
	struct vfs_mapping_target_t *target;
};

enum vfs_handle_type_t {
	FILE_HANDLE,
	DIR_HANDLE,
};

struct vfs_handle_t {
	enum vfs_handle_type_t type;
};

enum vfs_error_code_t {
	VFS_ADD_MAPPING_PARAMETER_ERROR,
	VFS_ADD_MAPPING_ALREADY_EXISTS,
	VFS_ADD_MAPPING_OUT_OF_MEMORY,
	VFS_MAP_EMPTY_PATH,
	VFS_CWD_OUT_OF_MEMORY,
	VFS_MAPPING_FINALIZATION_ERROR,
	VFS_CWD_ILLEGAL,
};

struct vfs_t {
	char error_str[VFS_MAX_ERROR_LENGTH];
	enum vfs_error_code_t last_error;
	unsigned int max_handle_count;
	unsigned int mapping_count;
	unsigned int base_flags;
	unsigned int cwd_alloced_size;
	unsigned int cwd_strlen;
	char *cwd;
	struct vfs_mapping_target_t *mappings;
	bool mappings_finalized;
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
bool vfs_add_mapping(struct vfs_t *vfs, const char *virtual_path, const char *real_path, unsigned int flags);
bool vfs_map(struct vfs_t *vfs, struct vfs_map_result_t *result, const char *path);
void vfs_finalize_mappings(struct vfs_t *vfs);
struct vfs_handle_t* vfs_opendir(const struct vfs_t *vfs, const char *virtual_path);
struct vfs_t *vfs_init(void);
void vfs_handle_free(struct vfs_handle_t *handle);
void vfs_free(struct vfs_t *vfs);
void vfs_dump(FILE *f, const struct vfs_t *vfs);
void vfs_dump_map_result(FILE *f, const struct vfs_map_result_t *map_result);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif