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

#ifndef __STRINGS_H__
#define __STRINGS_H__

#include <stdbool.h>

typedef bool (*path_split_callback_t)(const char *path, bool is_full_path, void *vctx);

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
void path_split_mutable(char *path, path_split_callback_t callback, void *vctx);
void path_split(const char *path, path_split_callback_t callback, void *vctx);
bool pathcmp(const char *path1, const char *path2);
void truncate_trailing_slash(char *path);
bool is_valid_path(const char *path);
bool is_absolute_path(const char *path);
char* sanitize_path(const char *cwd, const char *path);
bool path_contains_hidden(const char *path);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
