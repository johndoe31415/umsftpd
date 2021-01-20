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

#include "passdb.h"
#include "test_passdb.h"
#include "testbench.h"

void test_create_pbkdf2(void) {
	struct passdb_entry_t entry = { 0 };
	test_assert_true(passdb_create_entry(&entry, PASSDB_KDF_PBKDF2_SHA256, NULL, "foobar"));
	test_assert_true(passdb_validate(&entry, "foobar"));
	test_assert_false(passdb_validate(&entry, "foobar2"));
}

void test_create_srcypt(void) {
	struct passdb_entry_t entry = { 0 };
	test_assert_true(passdb_create_entry(&entry, PASSDB_KDF_SCRYPT, NULL, "foobar"));
	test_assert_true(passdb_validate(&entry, "foobar"));
	test_assert_false(passdb_validate(&entry, "foobar2"));
}
