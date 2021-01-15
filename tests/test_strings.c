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
#include "strings.h"
#include "test_strings.h"
#include "testbench.h"

void test_pathcmp(void) {
	test_assert(pathcmp("", "/"));
	test_assert(pathcmp("", ""));
	test_assert(!pathcmp("", "x"));
	test_assert(pathcmp("x", "x"));
	test_assert(!pathcmp("x", "y"));
	test_assert(!pathcmp("/x", "y"));
	test_assert(!pathcmp("x/", "y"));
	test_assert(pathcmp("x/", "x"));
	test_assert(!pathcmp("/foo", "/bar"));
	test_assert(pathcmp("/foo", "/foo"));
	test_assert(pathcmp("/foo", "/foo/"));
	test_assert(pathcmp("/foo/", "/foo/"));
}

struct pathsplit_t {
	int count;
	struct {
		char path[128];
		bool is_full_path;
	} result[10];
};

static bool pathsplit_callback(const char *path, bool is_full_path, void *vctx) {
	struct pathsplit_t *ctx = (struct pathsplit_t*)vctx;
	strcpy(ctx->result[ctx->count].path, path);
	ctx->result[ctx->count].is_full_path = is_full_path;
	ctx->count++;
	return true;
}

void test_path_split(void) {
	{
		struct pathsplit_t splitctx = { 0 };
		path_split("/foo/bar/moo/koo", pathsplit_callback, &splitctx);

		test_assert_int_eq(splitctx.count, 5);
		test_assert_str_eq(splitctx.result[0].path, "/");
		test_assert_str_eq(splitctx.result[1].path, "/foo");
		test_assert_str_eq(splitctx.result[2].path, "/foo/bar");
		test_assert_str_eq(splitctx.result[3].path, "/foo/bar/moo");
		test_assert_str_eq(splitctx.result[4].path, "/foo/bar/moo/koo");
		test_assert(!splitctx.result[0].is_full_path);
		test_assert(!splitctx.result[1].is_full_path);
		test_assert(!splitctx.result[2].is_full_path);
		test_assert(!splitctx.result[3].is_full_path);
		test_assert(splitctx.result[4].is_full_path);
	}

	{
		struct pathsplit_t splitctx = { 0 };
		path_split("/foo/bar/", pathsplit_callback, &splitctx);

		test_assert_int_eq(splitctx.count, 3);
		test_assert_str_eq(splitctx.result[0].path, "/");
		test_assert_str_eq(splitctx.result[1].path, "/foo");
		test_assert_str_eq(splitctx.result[2].path, "/foo/bar");
		test_assert(!splitctx.result[0].is_full_path);
		test_assert(!splitctx.result[1].is_full_path);
		test_assert(splitctx.result[2].is_full_path);
	}

	{
		struct pathsplit_t splitctx = { 0 };
		path_split("foo/bar", pathsplit_callback, &splitctx);

		test_assert_int_eq(splitctx.count, 2);
		test_assert_str_eq(splitctx.result[0].path, "foo");
		test_assert_str_eq(splitctx.result[1].path, "foo/bar");
		test_assert(!splitctx.result[0].is_full_path);
		test_assert(splitctx.result[1].is_full_path);
	}

	{
		struct pathsplit_t splitctx = { 0 };
		path_split("foo/bar/", pathsplit_callback, &splitctx);

		test_assert_int_eq(splitctx.count, 2);
		test_assert_str_eq(splitctx.result[0].path, "foo");
		test_assert_str_eq(splitctx.result[1].path, "foo/bar");
		test_assert(!splitctx.result[0].is_full_path);
		test_assert(splitctx.result[1].is_full_path);
	}

	{
		struct pathsplit_t splitctx = { 0 };
		path_split("/foo///", pathsplit_callback, &splitctx);

		test_assert_int_eq(splitctx.count, 4);
		test_assert_str_eq(splitctx.result[0].path, "/");
		test_assert_str_eq(splitctx.result[1].path, "/foo");
		test_assert_str_eq(splitctx.result[2].path, "/foo/");
		test_assert_str_eq(splitctx.result[3].path, "/foo//");
		test_assert(!splitctx.result[0].is_full_path);
		test_assert(!splitctx.result[1].is_full_path);
		test_assert(!splitctx.result[2].is_full_path);
		test_assert(splitctx.result[3].is_full_path);
	}

	{
		struct pathsplit_t splitctx = { 0 };
		path_split("", pathsplit_callback, &splitctx);
		test_assert(splitctx.count == 0);
	}
}
