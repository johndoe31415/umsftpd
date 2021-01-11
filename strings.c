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

void path_split(char *path, path_split_callback_t callback, void *vctx) {
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

bool is_directory_string(const char *path) {
	int len = strlen(path);
	return (len > 0) && (path[0] == '/') && (path[len - 1] == '/');
}
