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

void test_passdb_custom_params_pbkdf2(void) {
	/* Passphrase: 31AnNyAi6z */
	struct passdb_entry_t entry = {
		.kdf = PASSDB_KDF_PBKDF2_SHA256,
		.params.pbkdf2.iterations = 281,
		.salt = { 0xed, 0x01, 0xcf, 0xea, 0x2e, 0x2f, 0x3d, 0xd8, 0x30, 0x19, 0x7b, 0xbc, 0x87, 0x78, 0x7b, 0x63 },
		.hash = { 0x54, 0x22, 0x32, 0x18, 0xca, 0x0e, 0xa8, 0x29, 0x16, 0x5c, 0x81, 0x92, 0x7d, 0x60, 0x4f, 0x22, 0x84, 0xf1, 0x82, 0x49, 0x88, 0xa6, 0xf5, 0x84, 0x60, 0x1a, 0x48, 0x31, 0x8d, 0x40, 0xcd, 0xe1 },
	};
	test_assert_true(passdb_validate(&entry, "31AnNyAi6z"));
	test_assert_false(passdb_validate(&entry, "31AnNyAi6y"));
	test_assert_false(passdb_validate(&entry, "foobar"));
}

void test_passdb_totp_pbkdf2(void) {
	/* Passphrase: DDcA4l2j3OVvmtWeLSpX_U01F(yHmBR4/qw) */
	struct passdb_entry_t entry = {
		.kdf = PASSDB_KDF_PBKDF2_SHA256,
		.params.pbkdf2.iterations = 215,
		.salt = { 0x6a, 0xc2, 0xe2, 0x0d, 0x74, 0x91, 0xdd, 0x2c, 0xae, 0xc8, 0xbe, 0x8a, 0xc4, 0x2c, 0xd7, 0x83 },
		.hash = { 0xa0, 0x55, 0x13, 0xc2, 0x6e, 0xef, 0x9b, 0x51, 0x02, 0xde, 0x0f, 0x88, 0xed, 0x6f, 0xec, 0xf4, 0x0e, 0x4b, 0x41, 0x16, 0xc6, 0xb6, 0x75, 0xd1, 0x68, 0xcb, 0x9d, 0xbb, 0xb4, 0x94, 0x00, 0x31 },
	};
	test_assert_false(passdb_validate(&entry, ""));
	test_assert_true(passdb_validate(&entry, "DDcA4l2j3OVvmtWeLSpX_U01F(yHmBR4/qw)"));

	const char *secret = "Johannes Bauer";
	struct rfc6238_config_t *totp = rfc6238_new(secret, strlen(secret), RFC6238_DIGEST_SHA1, 30, 6);
	passdb_attach_totp(&entry, totp, 0);

	time_t now = 1611267965;	/* Token 350301 ~25 secs left validity */
	test_assert_false(passdb_validate(&entry, ""));
	test_assert_false(passdb_validate(&entry, "DDcA4l2j3OVvmtWeLSpX_U01F(yHmBR4/qw)"));
	test_assert_false(passdb_validate_around(&entry, "DDcA4l2j3OVvmtWeLSpX_U01F(yHmBR4/qw)", now));
	test_assert_true(passdb_validate_around(&entry, "DDcA4l2j3OVvmtWeLSpX_U01F(yHmBR4/qw)350301", now));

	/* Token is too old or too new */
	test_assert_false(passdb_validate_around(&entry, "DDcA4l2j3OVvmtWeLSpX_U01F(yHmBR4/qw)350301", now + 27));
	test_assert_false(passdb_validate_around(&entry, "DDcA4l2j3OVvmtWeLSpX_U01F(yHmBR4/qw)350301", now - 7));

	/* With increased window size, token validates */
	passdb_attach_totp(&entry, totp, 1);
	test_assert_true(passdb_validate_around(&entry, "DDcA4l2j3OVvmtWeLSpX_U01F(yHmBR4/qw)350301", now + 27));
	test_assert_true(passdb_validate_around(&entry, "DDcA4l2j3OVvmtWeLSpX_U01F(yHmBR4/qw)350301", now - 7));

	rfc6238_free(totp);
}

void test_passdb_totp_scrypt(void) {
	/* Passphrase: WQ2ys-GFJSfws2 */
	struct passdb_entry_t entry = {
		.kdf = PASSDB_KDF_SCRYPT,
		.params.scrypt.N = 2,
		.params.scrypt.r = 3,
		.params.scrypt.p = 1,
		.params.scrypt.maxmem_mib = 128,
		.salt = { 0x97, 0xfe, 0x61, 0x34, 0xcf, 0x37, 0xb5, 0xb1, 0x18, 0x25, 0x99, 0x37, 0xc0, 0xd7, 0xed, 0x2a },
		.hash = { 0xe0, 0x65, 0x28, 0x5d, 0x87, 0x05, 0xa3, 0x82, 0xd7, 0x2b, 0xa1, 0x55, 0xc4, 0xe2, 0x1a, 0x0b, 0x84, 0x3e, 0xfe, 0x56, 0x95, 0x79, 0x5e, 0x34, 0x5f, 0x4d, 0x10, 0x93, 0xdd, 0xff, 0x3d, 0x8d },
	};
	test_assert_false(passdb_validate(&entry, ""));
	test_assert_true(passdb_validate(&entry, "WQ2ys-GFJSfws2"));

	const char *secret = "Johannes Bauer";
	struct rfc6238_config_t *totp = rfc6238_new(secret, strlen(secret), RFC6238_DIGEST_SHA1, 30, 6);
	passdb_attach_totp(&entry, totp, 0);

	time_t now = 1611267965;	/* Token 350301 ~25 secs left validity */
	test_assert_false(passdb_validate(&entry, "WQ2ys-GFJSfws2"));
	test_assert_false(passdb_validate(&entry, "WQ2ys-GFJSfws2"));
	test_assert_false(passdb_validate_around(&entry, "WQ2ys-GFJSfws2", now));
	test_assert_true(passdb_validate_around(&entry, "WQ2ys-GFJSfws2350301", now));

	/* Token is too old or too new */
	test_assert_false(passdb_validate_around(&entry, "WQ2ys-GFJSfws2350301", now + 27));
	test_assert_false(passdb_validate_around(&entry, "WQ2ys-GFJSfws2350301", now - 7));

	/* With increased window size, token validates */
	passdb_attach_totp(&entry, totp, 1);
	test_assert_true(passdb_validate_around(&entry, "WQ2ys-GFJSfws2350301", now + 27));
	test_assert_true(passdb_validate_around(&entry, "WQ2ys-GFJSfws2350301", now - 7));

	rfc6238_free(totp);
}

void test_passdb_no_password(void) {
	struct passdb_entry_t entry = {
		.kdf = PASSDB_KDF_NONE,
	};

	test_assert_true(passdb_validate_around(&entry, "fsdjkiofjdsoifsd", 4378947));
	test_assert_true(passdb_validate_around(&entry, "", 2389847));
	test_assert_true(passdb_validate_around(&entry, "UJOFIDJSAOIJFOIJ39839", 0));
}

void test_passdb_totp_only(void) {
	struct passdb_entry_t entry = {
		.kdf = PASSDB_KDF_NONE,
	};
	const char *secret = "Johannes Bauer";
	struct rfc6238_config_t *totp = rfc6238_new(secret, strlen(secret), RFC6238_DIGEST_SHA1, 30, 6);
	passdb_attach_totp(&entry, totp, 0);

	test_assert_false(passdb_validate_around(&entry, "fsdjkiofjdsoifsd", 4378947));
	test_assert_false(passdb_validate_around(&entry, "", 2389847));
	test_assert_false(passdb_validate_around(&entry, "UJOFIDJSAOIJFOIJ39839", 0));

	time_t now = 1611267965;	/* Token 350301 ~25 secs left validity */
	test_assert_true(passdb_validate_around(&entry, "WQ2ys-GFJSfws2350301", now));
	test_assert_true(passdb_validate_around(&entry, "350301", now));

	rfc6238_free(totp);
}
