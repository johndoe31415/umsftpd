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
	rfc6238_free(totp);
}
