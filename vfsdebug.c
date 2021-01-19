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

typedef bool (*vfs_shell_callback_t)(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments);

struct vfs_shell_command_t {
	const char *command;
	unsigned int min_argument_count;
	unsigned int max_argument_count;
	const char *arg_text;
	const char *help_text;
	vfs_shell_callback_t handler;
};

static bool vfs_shell_ls(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments);
static bool vfs_shell_find(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments);
static bool vfs_shell_cd(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments);
static bool vfs_shell_stat(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments);
static bool vfs_shell_cat(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments);
static bool vfs_shell_mod(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments);
static bool vfs_shell_put(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments);
static bool vfs_shell_rm(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments);
static bool vfs_shell_mkdir(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments);
static bool vfs_shell_rmdir(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments);
static bool vfs_shell_help(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments);

static const struct vfs_shell_command_t vfs_shell_commands[] = {
	{ .command = "ls", .min_argument_count = 0, .max_argument_count = 1, .arg_text = "([path])", .help_text = "list a given path (or cwd)", .handler = vfs_shell_ls },
	{ .command = "find", .min_argument_count = 0, .max_argument_count = 1, .arg_text = "([path])", .help_text = "recursively traverse paths", .handler = vfs_shell_find },
	{ .command = "cd", .min_argument_count = 1, .max_argument_count = 1, .arg_text = "[path]", .help_text = "change directory to given path", .handler = vfs_shell_cd },
	{ .command = "stat", .min_argument_count = 1, .max_argument_count = 1, .arg_text = "[path/file]", .help_text = "stat the given path", .handler = vfs_shell_stat },
	{ .command = "cat", .min_argument_count = 1, .max_argument_count = 1, .arg_text = "[file]", .help_text = "show contents of the given file", .handler = vfs_shell_cat },
	{ .command = "mod", .min_argument_count = 1, .max_argument_count = 1, .arg_text = "[file]", .help_text = "modify the given file", .handler = vfs_shell_mod },
	{ .command = "put", .min_argument_count = 1, .max_argument_count = 1, .arg_text = "[file]", .help_text = "create a given file", .handler = vfs_shell_put },
	{ .command = "rm", .min_argument_count = 1, .max_argument_count = 1, .arg_text = "[file]", .help_text = "unlink the given file", .handler = vfs_shell_rm },
	{ .command = "mkdir", .min_argument_count = 1, .max_argument_count = 1, .arg_text = "[path]", .help_text = "create a directory", .handler = vfs_shell_mkdir },
	{ .command = "rmdir", .min_argument_count = 1, .max_argument_count = 1, .arg_text = "[path]", .help_text = "remove a directory", .handler = vfs_shell_rmdir },
	{ .command = "help", .help_text = "show this help page", .handler = vfs_shell_help },
	{ .command = "?", .handler = vfs_shell_help },
	{ 0 },
};

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
	fprintf(f, "Virtual directory: %s\n", map_result->inode ? "yes" : "no");
}

static void vfs_print_dirent_color(const struct vfs_dirent_t *vfs_dirent, bool start) {
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

static void vfs_print_dirent(const struct vfs_dirent_t *vfs_dirent) {
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
	vfs_print_dirent_color(vfs_dirent, true);
	printf("%s", vfs_dirent->filename);
	vfs_print_dirent_color(vfs_dirent, false);
	printf("\n");
}

static bool vfs_shell_ls(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments) {
	struct vfs_handle_t *handle;
	const char *path = (argument_count == 0) ? "." : arguments[0];
	enum vfs_error_t result = vfs_opendir(vfs, path, &handle);
	if (result != VFS_OK) {
		fprintf(stderr, "diropen %s: %s\n", path, vfs_error_str(result));
		return false;
	}

	bool success = true;
	struct vfs_dirent_t vfs_dirent;
	while (true) {
		result = vfs_readdir(handle, &vfs_dirent);
		if (result != VFS_OK) {
			fprintf(stderr, "vfs_readdir %s: %s\n", path, vfs_error_str(result));
			success = false;
			break;
		}
		if (vfs_dirent.eof) {
			break;
		}
		vfs_print_dirent(&vfs_dirent);
	}

	vfs_close_handle(handle);
	return success;
}

static bool vfs_shell_find(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments) {
	/* TODO implement me */
	return true;
}

static bool vfs_shell_cd(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments) {
	enum vfs_error_t result = vfs_chdir(vfs, arguments[0]);
	if (result != VFS_OK) {
		printf("Error: %s\n", vfs_error_str(result));
	}
	return result == VFS_OK;
}

static bool vfs_shell_stat(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments) {
	struct vfs_dirent_t dirent;
	enum vfs_error_t result = vfs_stat(vfs, arguments[0], &dirent);
	if (result != VFS_OK) {
		printf("Error: %s\n", vfs_error_str(result));
	} else {
		vfs_print_dirent(&dirent);
	}
	return result == VFS_OK;
}

static bool vfs_shell_cat(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments) {
	struct vfs_handle_t *handle;
	enum vfs_error_t result = vfs_open(vfs, arguments[0], FILEMODE_READ, &handle);
	if (result != VFS_OK) {
		printf("Error: %s\n", vfs_error_str(result));
		return false;
	} else {
		uint8_t buffer[1024];
		while (result == VFS_OK) {
			size_t length = sizeof(buffer);
			result = vfs_read(handle, buffer, &length);
			if (length == 0) {
				/* EOF */
				break;
			}
			if (fwrite(buffer, 1, length, stdout) != length) {
				fprintf(stderr, "Short write to stdout.\n");
				break;
			}
		}
	}
	vfs_close_handle(handle);
	return result == VFS_OK;
}

static bool vfs_shell_mod(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments) {
	/* TODO implement me */
	return true;
}

static bool vfs_shell_put(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments) {
	struct vfs_handle_t *handle;
	enum vfs_error_t result = vfs_open(vfs, arguments[0], FILEMODE_WRITE, &handle);
	if (result != VFS_OK) {
		printf("Error: %s\n", vfs_error_str(result));
		return false;
	} else {
		const char *put_string = "This is now in a file.\n";
		size_t length = strlen(put_string);
		result = vfs_write(handle, put_string, &length);
	}
	vfs_close_handle(handle);
	return result == VFS_OK;
}

static bool vfs_shell_rm(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments) {
	/* TODO implement me */
	return true;
}

static bool vfs_shell_mkdir(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments) {
	/* TODO implement me */
	return true;
}

static bool vfs_shell_rmdir(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments) {
	/* TODO implement me */
	return true;
}

static bool vfs_shell_help(struct vfs_t *vfs, const char *cmd, unsigned int argument_count, const char **arguments) {
	const struct vfs_shell_command_t *cmds = vfs_shell_commands;
	fprintf(stderr, "help:\n");
	while (cmds->command) {
		if (cmds->help_text) {
			char lhs[64];
			snprintf(lhs, sizeof(lhs), "%s %s", cmds->command, cmds->arg_text ? cmds->arg_text : "");
			fprintf(stderr, "  %-20s %s\n", lhs, cmds->help_text);
		}
		cmds++;
	}
	return true;
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

		const char *tokens[8];
		unsigned int token_count = 0;
		char *input = line;
		char *token;
		while ((token = strtok(input, " ")) != NULL) {
			input = NULL;
			if (token_count < 8) {
				tokens[token_count] = token;
				token_count++;
			} else {
				fprintf(stderr, "Too many tokens. Not executing command.\n");
				continue;
			}
		}

		if (token_count == 0) {
			continue;
		}
		const unsigned int argument_count = token_count - 1;
		const struct vfs_shell_command_t *cmd = vfs_shell_commands;
		bool found = false;
		while (cmd->command) {
			if (!strcmp(cmd->command, tokens[0])) {
				/* Command found */
				found = true;
				if ((argument_count >= cmd->min_argument_count) && (argument_count <= cmd->max_argument_count)) {
					/* Argument count correct */
					cmd->handler(vfs, tokens[0], argument_count, &tokens[1]);
				} else {
					fprintf(stderr, "Invalid amount of argument for: \"%s\". Expected %d to %d. Type \"help\" for a help page.\n", tokens[0], cmd->min_argument_count, cmd->max_argument_count);
				}
				break;
			}
			cmd++;
		}
		if (!found) {
			fprintf(stderr, "No such command: \"%s\". Type \"help\" for a help page.\n", tokens[0]);
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
	vfs_add_inode(vfs, "/virt/sub1", NULL, VFS_INODE_FLAG_READ_ONLY, 0);
	vfs_add_inode(vfs, "/virt/sub2", NULL, VFS_INODE_FLAG_READ_ONLY, 0);
	vfs_add_inode(vfs, "/virt/abcd", NULL, VFS_INODE_FLAG_READ_ONLY, 0);
	vfs_add_inode(vfs, "/b", "/bin", VFS_INODE_FLAG_READ_ONLY, 0);
	vfs_add_inode(vfs, "/incoming", "/tmp/incoming", 0, VFS_INODE_FLAG_READ_ONLY);
	vfs_freeze_inodes(vfs);
	vfs_shell(vfs);
	vfs_free(vfs);
	return 0;
}
#endif
