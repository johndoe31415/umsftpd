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

struct symlink_check_response_t {
	bool critical_error;
	bool file_not_found;
	bool contains_symlink;
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
void __attribute__((nonnull (1, 2))) path_split_mutable(char *path, path_split_callback_t callback, void *vctx);
void __attribute__((nonnull (1, 2))) path_split(const char *path, path_split_callback_t callback, void *vctx);
bool __attribute__((nonnull (1, 2))) pathcmp(const char *path1, const char *path2);
void __attribute__((nonnull (1))) truncate_trailing_slash(char *path);
bool __attribute__((nonnull (1))) is_valid_path(const char *path);
bool __attribute__((nonnull (1))) is_absolute_path(const char *path);
char* __attribute__((nonnull (1, 2))) sanitize_path(const char *cwd, const char *path);
bool __attribute__((nonnull (1))) path_contains_hidden(const char *path);
struct symlink_check_response_t __attribute__((nonnull (1))) path_contains_symlink(const char *path);
void __attribute__((nonnull (1))) strip_crlf(char *string);
const char *const_basename(const char *path);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
