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
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>
#include "stringlist.h"

#define VFS_MAX_ERROR_LENGTH					128
#define VFS_MAX_FILENAME_LENGTH					256

#define VFS_INODE_FLAG_READ_ONLY				(1 << 0)
#define VFS_INODE_FLAG_FILTER_ALL				(1 << 1)
#define VFS_INODE_FLAG_FILTER_HIDDEN			(1 << 2)
#define VFS_INODE_FLAG_DISALLOW_CREATE_FILE		(1 << 3)
#define VFS_INODE_FLAG_DISALLOW_CREATE_DIR		(1 << 4)
#define VFS_INODE_FLAG_DISALLOW_UNLINK			(1 << 5)
#define VFS_INODE_FLAG_ALLOW_SYMLINKS			(1 << 6)

struct vfs_inode_t {
	struct vfs_inode_t *parent;
	unsigned int flags_set, flags_reset;
	char *virtual_path;
	char *target_path;
	size_t vlen, tlen;
	struct stringlist_t *virtual_subdirs;
};

struct vfs_lookup_result_t {
	unsigned int flags;
	struct vfs_inode_t *inode;
	struct vfs_inode_t *mountpoint;
};

enum vfs_handle_type_t {
	FILE_HANDLE,
	DIR_HANDLE,
};

enum vfs_filemode_t {
	FILEMODE_READ,
	FILEMODE_WRITE,
	FILEMODE_APPEND,
};

struct vfs_handle_t {
	enum vfs_handle_type_t type;
	char *virtual_path;
	char *mapped_path;
	const struct vfs_inode_t *inode;
	unsigned int flags;
	union {
		struct {
			DIR *dir;
			unsigned int internal_node_index;
		} dir;
		struct {
			FILE *file;
		} file;
	};
};

struct vfs_dirent_t {
	char filename[VFS_MAX_FILENAME_LENGTH];
	bool eof;
	bool is_file;
	uint16_t uid, gid;
	uint64_t filesize;
	uint16_t permissions;
	struct timespec mtime, ctime, atime;
};

enum vfs_internal_error_t {
	VFS_ADD_INODE_PARAMETER_ERROR,
	VFS_ADD_INODE_ALREADY_EXISTS,
	VFS_ADD_INODE_OUT_OF_MEMORY,
	VFS_MAP_EMPTY_PATH,
	VFS_CWD_OUT_OF_MEMORY,
	VFS_INODE_FINALIZATION_ERROR,
	VFS_CWD_ILLEGAL,
	VFS_LOOKUP_NON_ABSOLUTE,
	VFS_ILLEGAL_PATH,
	VFS_SANITIZE_PATH_ERROR,
	VFS_INODE_LOOKUP_ERROR,
	VFS_PATH_MAP_ERROR,
	VFS_MISSING_ARGUMENT,
	VFS_NOT_MOUNTED,
	VFS_PATH_NOT_ABSOLUTE,
	VFS_OUT_OF_MEMORY,
	VFS_OPENDIR_FAILED,
};

enum vfs_error_t {
	VFS_OK,
	VFS_OUT_OF_HANDLES,
	VFS_PERMISSION_DENIED,
	VFS_NO_SUCH_FILE_OR_DIRECTORY,
	VFS_NOT_A_DIRECTORY,
	VFS_NOT_A_FILE,
	VFS_INTERNAL_ERROR,
	VFS_IO_ERROR,
};

struct vfs_t {
	struct {
		char string[VFS_MAX_ERROR_LENGTH];
		enum vfs_internal_error_t code;
	} error;
	struct {
		unsigned int current_count;
		unsigned int max_count;
	} handles;
	struct {
		unsigned int base_flags;
		unsigned int count;
		struct vfs_inode_t **data;
		bool frozen;
	} inode;
	struct {
		unsigned int alloced_size;
		unsigned int length;
		char *path;
	} cwd;
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
const char *vfs_error_str(enum vfs_error_t error_code);
bool vfs_add_inode(struct vfs_t *vfs, const char *virtual_path, const char *target_path, unsigned int flags_set, unsigned int flags_reset);
bool vfs_lookup(struct vfs_t *vfs, struct vfs_lookup_result_t *result, const char *path);
void vfs_freeze_inodes(struct vfs_t *vfs);
struct vfs_t *vfs_init(void);
void vfs_free(struct vfs_t *vfs);
enum vfs_error_t vfs_chdir(struct vfs_t *vfs, const char *path);
enum vfs_error_t vfs_opendir(struct vfs_t *vfs, const char *path, struct vfs_handle_t **handle_ptr);
enum vfs_error_t vfs_open(struct vfs_t *vfs, const char *path, enum vfs_filemode_t mode, struct vfs_handle_t **handle_ptr);
enum vfs_error_t vfs_stat(struct vfs_t *vfs, const char *path, struct vfs_dirent_t *vfs_dirent);
enum vfs_error_t vfs_read(struct vfs_handle_t *handle, void *ptr, size_t *length);
enum vfs_error_t vfs_write(struct vfs_handle_t *handle, const void *ptr, size_t *length);
enum vfs_error_t vfs_readdir(struct vfs_handle_t *handle, struct vfs_dirent_t *vfs_dirent);
void vfs_close_handle(struct vfs_handle_t *handle);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
