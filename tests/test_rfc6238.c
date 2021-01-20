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

#include "rfc6238.h"
#include "test_rfc6238.h"
#include "testbench.h"

void test_sha1(void) {
	const uint8_t secret[] = {
		0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30,
		0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30
	};
	char output[16];
	struct rfc6238_config_t *totp = rfc6238_new(secret, sizeof(secret), RFC6238_DIGEST_SHA1, 30, 8);
	test_assert_true(rfc6238_generate_at(totp, output, 59));
	test_assert_str_eq(output, "94287082");
	test_assert_true(rfc6238_generate_at(totp, output, 1111111109));
	test_assert_str_eq(output, "07081804");
	test_assert_true(rfc6238_generate_at(totp, output, 1111111111));
	test_assert_str_eq(output, "14050471");
	test_assert_true(rfc6238_generate_at(totp, output, 1234567890));
	test_assert_str_eq(output, "89005924");
	test_assert_true(rfc6238_generate_at(totp, output, 2000000000));
	test_assert_str_eq(output, "69279037");
	test_assert_true(rfc6238_generate_at(totp, output, 20000000000));
	test_assert_str_eq(output, "65353130");
	test_assert_true(rfc6238_generate_at(totp, output, 96262));
	test_assert_str_eq(output, "00044814");
	rfc6238_free(totp);
}

void test_sha256(void) {
	const uint8_t secret[] = {
		0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
		0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32
	};
	char output[16];
	struct rfc6238_config_t *totp = rfc6238_new(secret, sizeof(secret), RFC6238_DIGEST_SHA256, 30, 8);
	test_assert_true(rfc6238_generate_at(totp, output, 59));
	test_assert_str_eq(output, "46119246");
	test_assert_true(rfc6238_generate_at(totp, output, 1111111109));
	test_assert_str_eq(output, "68084774");
	test_assert_true(rfc6238_generate_at(totp, output, 1111111111));
	test_assert_str_eq(output, "67062674");
	test_assert_true(rfc6238_generate_at(totp, output, 1234567890));
	test_assert_str_eq(output, "91819424");
	test_assert_true(rfc6238_generate_at(totp, output, 2000000000));
	test_assert_str_eq(output, "90698825");
	test_assert_true(rfc6238_generate_at(totp, output, 20000000000));
	test_assert_str_eq(output, "77737706");
	rfc6238_free(totp);
}

void test_sha512(void) {
	const uint8_t secret[] = {
		0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
		0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32,
		0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
		0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34
	};
	char output[16];
	struct rfc6238_config_t *totp = rfc6238_new(secret, sizeof(secret), RFC6238_DIGEST_SHA512, 30, 8);
	test_assert_true(rfc6238_generate_at(totp, output, 59));
	test_assert_str_eq(output, "90693936");
	test_assert_true(rfc6238_generate_at(totp, output, 1111111109));
	test_assert_str_eq(output, "25091201");
	test_assert_true(rfc6238_generate_at(totp, output, 1111111111));
	test_assert_str_eq(output, "99943326");
	test_assert_true(rfc6238_generate_at(totp, output, 1234567890));
	test_assert_str_eq(output, "93441116");
	test_assert_true(rfc6238_generate_at(totp, output, 2000000000));
	test_assert_str_eq(output, "38618901");
	test_assert_true(rfc6238_generate_at(totp, output, 20000000000));
	test_assert_str_eq(output, "47863826");
	rfc6238_free(totp);
}

void test_vanilla(void) {
	const uint8_t secret[] = {
		0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30
	};
	char output[16];
	struct rfc6238_config_t *totp = rfc6238_new(secret, sizeof(secret), RFC6238_DIGEST_SHA1, 30, 6);
	test_assert_true(rfc6238_generate_at(totp, output, 0));
	test_assert_str_eq(output, "755224");
	rfc6238_free(totp);
}
