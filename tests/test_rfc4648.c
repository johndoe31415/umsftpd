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

#include "rfc4648.h"
#include "test_rfc4648.h"
#include "testbench.h"

void test_base32(void) {
	char buffer[16] = { 0 };
	test_assert_true(rfc4648_decode_base32(buffer, sizeof(buffer), ""));
	test_assert_str_eq(buffer, "");

	test_assert_true(rfc4648_decode_base32(buffer, sizeof(buffer), "MY======"));
	test_assert_str_eq(buffer, "f");

	test_assert_true(rfc4648_decode_base32(buffer, sizeof(buffer), "MZXQ===="));
	test_assert_str_eq(buffer, "fo");

	test_assert_true(rfc4648_decode_base32(buffer, sizeof(buffer), "MZXW6==="));
	test_assert_str_eq(buffer, "foo");

	test_assert_true(rfc4648_decode_base32(buffer, sizeof(buffer), "MZXW6YQ="));
	test_assert_str_eq(buffer, "foob");

	test_assert_true(rfc4648_decode_base32(buffer, sizeof(buffer), "MZXW6YTB"));
	test_assert_str_eq(buffer, "fooba");

	test_assert_true(rfc4648_decode_base32(buffer, sizeof(buffer), "MZXW6YTBOI======"));
	test_assert_str_eq(buffer, "foobar");
}

void test_base32hex(void) {
	char buffer[16] = { 0 };
	test_assert_true(rfc4648_decode_base32hex(buffer, sizeof(buffer), ""));
	test_assert_str_eq(buffer, "");

	test_assert_true(rfc4648_decode_base32hex(buffer, sizeof(buffer), "CO======"));
	test_assert_str_eq(buffer, "f");

	test_assert_true(rfc4648_decode_base32hex(buffer, sizeof(buffer), "CPNG===="));
	test_assert_str_eq(buffer, "fo");

	test_assert_true(rfc4648_decode_base32hex(buffer, sizeof(buffer), "CPNMU==="));
	test_assert_str_eq(buffer, "foo");

	test_assert_true(rfc4648_decode_base32hex(buffer, sizeof(buffer), "CPNMUOG="));
	test_assert_str_eq(buffer, "foob");

	test_assert_true(rfc4648_decode_base32hex(buffer, sizeof(buffer), "CPNMUOJ1"));
	test_assert_str_eq(buffer, "fooba");

	test_assert_true(rfc4648_decode_base32hex(buffer, sizeof(buffer), "CPNMUOJ1E8======"));
	test_assert_str_eq(buffer, "foobar");
}

void test_base32_illegal_char(void) {
	char buffer[16];
	test_assert_false(rfc4648_decode_base32(buffer, sizeof(buffer), "MZXW6xTBOI======"));
	test_assert_false(rfc4648_decode_base32hex(buffer, sizeof(buffer), "MZXW6T@BOI======"));
}

void test_base32_short(void) {
	char buffer[6] = { 0 };
	test_assert_true(rfc4648_decode_base32(buffer, 6, "MZXW6YTBOI======"));
	test_assert_false(rfc4648_decode_base32(buffer, 5, "MZXW6YTBOI======"));
}
