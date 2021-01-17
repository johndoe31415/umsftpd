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
#include <string.h>
#include "strings.h"

void path_split_mutable(char *path, path_split_callback_t callback, void *vctx) {
	size_t len = strlen(path);
	for (size_t i = 0; i < len; i++) {
		bool is_full_path = (i == (len - 1));
		bool do_continue = true;
		if (path[i] == '/') {
			if (i == 0) {
				char saved = path[1];
				path[1] = 0;
				do_continue = callback(path, is_full_path, vctx);
				path[1] = saved;
			} else {
				path[i] = 0;
				do_continue = callback(path, is_full_path, vctx);
				path[i] = '/';
			}
		} else if (is_full_path) {
			callback(path, is_full_path, vctx);
		}
		if (!do_continue) {
			break;
		}
	}
}

void path_split(const char *path, path_split_callback_t callback, void *vctx) {
	size_t len = strlen(path);
	char copy[len + 1];
	strcpy(copy, path);
	path_split_mutable(copy, callback, vctx);
}

bool pathcmp(const char *path1, const char *path2) {
	int len1, len2;
	len1 = strlen(path1);
	len2 = strlen(path2);
	if (len1 && (path1[len1 - 1] == '/')) {
		len1 -= 1;
	}
	if (len2 && (path2[len2 - 1] == '/')) {
		len2 -= 1;
	}
	if (len1 != len2) {
		return false;
	}
	return !strncmp(path1, path2, len1);
}

void truncate_trailing_slash(char *path) {
	int len = strlen(path);
	while (len && (path[len - 1] == '/')) {
		path[--len] = 0;
	}
}

bool is_valid_path(const char *path) {
	int len = strlen(path);
	return (len > 0) && (path[0] == '/') && (path[len - 1] != '/');
}

bool is_absolute_path(const char *path) {
	return path[0] == '/';
}

static char *sanitize_absolute_path(const char *path) {
	if (!is_absolute_path(path)) {
		return NULL;
	}

	int pathlen = strlen(path);
	char *result = malloc(pathlen + 1);
	if (!result) {
		return NULL;
	}

	unsigned int result_len = 1;
	result[0] = '/';

	unsigned int token_len = 0;
	const char *token = path + 1;
	for (unsigned int i = 1; i < pathlen + 1; i++) {
		char next_char = path[i];

		if ((next_char == '/') || (next_char == 0)) {
			//printf("Token: %20.*s (len %2d) ", token_len, token, token_len);
			if ((token_len == 0) || (!strncmp(token, ".", token_len))) {
				/* Entirely disregard this addition */
			} else if (!strncmp(token, "..", token_len)) {
				/* Backtrack! */
				while ((result_len > 1) && (result[result_len - 1] != '/')) {
					result_len--;
				}
				result_len--;
				if (result_len == 0) {
					/* Never cut out the initial '/' */
					result_len = 1;
				}
			} else {
				/* Just a regular appendage, copy over */
				if (result[result_len - 1] != '/') {
					result[result_len++] = '/';
				}
				memcpy(result + result_len, token, token_len);
				result_len += token_len;
			}
			token = path + i + 1;
			token_len = 0;
			//printf("-> %.*s\n", result_len, result);
		} else {
			token_len++;
		}
	}
	result[result_len] = 0;
	return result;
}

char* sanitize_path(const char *cwd, const char *path) {
	if (!is_absolute_path(path)) {
		int cwd_len = strlen(cwd);
		int path_len = strlen(path);
		char absolute_path[cwd_len + 1 + path_len + 1];
		strcpy(absolute_path, cwd);
		strcpy(absolute_path + cwd_len, "/");
		strcpy(absolute_path + cwd_len + 1, path);
		return sanitize_absolute_path(absolute_path);
	} else {
		return sanitize_absolute_path(path);
	}
}

/* Functions only on sanitized paths, i.e, '/foo/./bar' or '/foo/..' will show
 * up as 'hidden'. */
bool path_contains_hidden(const char *path) {
	bool seen_slash = true;
	while (*path) {
		char next_char = *path;
		if (next_char == '/') {
			seen_slash = true;
		} else {
			if (seen_slash && (next_char == '.')) {
				return true;
			}
			seen_slash = false;
		}
		path++;
	}
	return false;
}
