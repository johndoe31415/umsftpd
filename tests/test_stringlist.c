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
#include "testbench.h"
#include "stringlist.h"
#include "test_stringlist.h"

void test_stringlist_create_destroy(void) {
	struct stringlist_t* list = stringlist_new();
	test_assert(list);
	test_assert_true(list->sorted);
	test_assert_int_eq(list->count, 0);
	stringlist_free(list);
}

void test_stringlist_insert(void) {
	struct stringlist_t* list = stringlist_new();
	test_assert(list);
	test_assert_int_eq(list->count, 0);
	stringlist_insert(list, "foo");
	test_assert_int_eq(list->count, 1);
	test_assert_str_eq(list->strings[0], "foo");
	stringlist_insert(list, "bar");
	test_assert_int_eq(list->count, 2);
	test_assert_str_eq(list->strings[1], "bar");
	stringlist_free(list);
}

void test_stringlist_sort(void) {
	struct stringlist_t* list = stringlist_new();
	stringlist_insert(list, "foo");
	test_assert_false(list->sorted);
	stringlist_insert(list, "bar");
	test_assert_int_eq(list->count, 2);
	stringlist_sort(list);
	test_assert_true(list->sorted);
	test_assert_str_eq(list->strings[0], "bar");
	test_assert_str_eq(list->strings[1], "foo");
	stringlist_free(list);
}
