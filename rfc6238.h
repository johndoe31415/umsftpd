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

#ifndef __RFC6238_H__
#define __RFC6238_H__

#include <stdint.h>
#include <stdbool.h>
#include <openssl/evp.h>

enum rfc6238_digest_t {
	RFC6238_DIGEST_SHA1,
	RFC6238_DIGEST_SHA256,
	RFC6238_DIGEST_SHA384,
	RFC6238_DIGEST_SHA512,
};

struct rfc6238_config_t {
	const EVP_MD *md;
	enum rfc6238_digest_t digest;
	uint32_t modulo;
	unsigned int digits;
	unsigned int secret_length;
	unsigned int slice_time_seconds;
	uint8_t secret[];
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
struct rfc6238_config_t *rfc6238_new(const void *secret, unsigned int secret_length, enum rfc6238_digest_t digest, unsigned int slice_time_seconds, unsigned int digits);
bool rfc6238_generate(const struct rfc6238_config_t *config, char output[static 9], uint64_t timecode);
bool rfc6238_generate_at(const struct rfc6238_config_t *config, char output[static 9], time_t t);
bool rfc6238_generate_now(const struct rfc6238_config_t *config, char output[static 9]);
void rfc6238_free(struct rfc6238_config_t *config);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
