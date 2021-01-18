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

#include <string.h>
#include <sys/stat.h>
#include "strings.h"
#include "vfsdebug.h"

static void vfs_dump_flags(FILE *f, unsigned int flags) {
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
	if (inode->set_flags || inode->reset_flags) {
		fprintf(f, " [set = 0x%x ", inode->set_flags);
		vfs_dump_flags(f, inode->set_flags);
		fprintf(f, ", reset = 0x%x ", inode->reset_flags);
		vfs_dump_flags(f, inode->reset_flags);
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

static void vfs_shell_ls_printent_color(const struct vfs_dirent_t *vfs_dirent, bool start) {
	unsigned int color = 0;
	if (!vfs_dirent->is_file) {
		color = 4;		/* directory, blue */
	} else if (vfs_dirent->permissions & (S_IXUSR | S_IXGRP | S_IXOTH)) {
		color = 2;		/* executable, green */
	}
	if (color) {
		if (start) {
			printf("\e[01;3%dm", color);
		} else {
			printf("\e[0m");
		}
	}
}

static void vfs_shell_ls_printent(const struct vfs_dirent_t *vfs_dirent) {
	printf("%s", vfs_dirent->is_file ? "-" : "d");
	for (unsigned int bit = 9; bit > 0; bit -= 3) {
		printf("%s%s%s", ((vfs_dirent->permissions >> (bit - 1)) & 1) ? "r" : "-", ((vfs_dirent->permissions >> (bit - 2)) & 1) ? "w" : "-", ((vfs_dirent->permissions >> (bit - 3)) & 1) ? "x" : "-");
	}

	printf("   %4d %4d", vfs_dirent->uid, vfs_dirent->gid);
	printf("   %8lu", vfs_dirent->filesize);

	struct tm time_struct;
	localtime_r(&vfs_dirent->mtime.tv_sec, &time_struct);
	char time_format[64];
	strftime(time_format, sizeof(time_format), "%Y-%m-%d %H:%M:%S", &time_struct);
	printf("  %s", time_format);

	printf("  ");
	vfs_shell_ls_printent_color(vfs_dirent, true);
	printf("%s", vfs_dirent->filename);
	vfs_shell_ls_printent_color(vfs_dirent, false);
	printf("\n");
}

static void vfs_shell_ls(struct vfs_t *vfs, const char *path) {
	struct vfs_handle_t *handle;
	enum vfs_error_t result = vfs_opendir(vfs, path, &handle);
	if (result != VFS_OK) {
		fprintf(stderr, "diropen %s: %s\n", path, vfs_error_str(result));
		return;
	}

	struct vfs_dirent_t vfs_dirent;
	while (true) {
		result = vfs_readdir(handle, &vfs_dirent);
		if (result != VFS_OK) {
			fprintf(stderr, "vfs_readdir %s: %s\n", path, vfs_error_str(result));
			break;
		}
		if (vfs_dirent.eof) {
			break;
		}
		vfs_shell_ls_printent(&vfs_dirent);
	}

	vfs_close_handle(handle);
}

void vfs_shell(struct vfs_t *vfs) {
	while (true) {
		printf("vfs [");
		printf("%s", (vfs->cwd.path[0] == 0) ? "/" : vfs->cwd.path);
		printf("]: ");
		fflush(stdout);

		char line[256];
		if (!fgets(line, sizeof(line) - 1, stdin)) {
			return;
		}
		strip_crlf(line);

		char *command = strtok(line, " ");
		if (!command) {
			continue;
		} else if (!strcmp(command, "ls")) {
			char *path = strtok(NULL, " ");
			if (!path) {
				vfs_shell_ls(vfs, ".");
			} else {
				vfs_shell_ls(vfs, path);
			}
		} else if (!strcmp(command, "cd")) {
			char *path = strtok(NULL, " ");
			if (!path) {
				printf("Error: 'cd' command requires path as argument\n");
			} else {
				enum vfs_error_t result = vfs_chdir(vfs, path);
				if (result != VFS_OK) {
					printf("Error: %s\n", vfs_error_str(result));
				}
			}
		} else if (!strcmp(command, "cat")) {
		} else if (!strcmp(command, "put")) {
		} else if (!strcmp(command, "?") || !strcmp(command, "help")) {
		} else {
			printf("Unknown command: %s\n", command);
		}
	}
}

#ifdef __VFS_SHELL__
#include "logging.h"

int main(int argc, char **argv) {
	llvl_set(LLVL_TRACE);

	struct vfs_t *vfs = vfs_init();
	vfs_add_inode(vfs, "/", "/tmp", VFS_INODE_FLAG_READ_ONLY, 0);
	vfs_add_inode(vfs, "/virt", NULL, VFS_INODE_FLAG_READ_ONLY, 0);
	vfs_freeze_inodes(vfs);
	vfs_shell(vfs);
	vfs_free(vfs);
	return 0;
}
#endif
