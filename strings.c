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
#include "strings.h"

void path_split(const char *path, path_split_callback_t callback, void *vctx) {
	size_t len = strlen(path);
	char copy[len + 1];
	strcpy(copy, path);

	for (size_t i = 0; i < len; i++) {
		bool is_full_path = (i == (len - 1));
		if (copy[i] == '/') {
			if (i == 0) {
				char root[2] = "/";
				if (!callback(root, is_full_path, vctx)) {
					break;
				}
			} else {
				copy[i] = 0;
				if (!callback(copy, is_full_path, vctx)) {
					break;
				}
				copy[i] = '/';
			}
		} else if (is_full_path) {
			callback(copy, is_full_path, vctx);
		}
	}
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
