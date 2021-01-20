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
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <byteswap.h>
#include <openssl/crypto.h>
#include <openssl/hmac.h>
#include "rfc6238.h"



struct rfc6238_config_t *rfc6238_new(const void *secret, unsigned int secret_length, enum rfc6238_digest_t digest, unsigned int slice_time_seconds, unsigned int digits) {
	if ((digits < 1) || (digits > 8)) {
		return NULL;
	}

	struct rfc6238_config_t *config = malloc(sizeof(struct rfc6238_config_t) + secret_length);
	if (!config) {
		return NULL;
	}
	switch (digest) {
		case RFC6238_DIGEST_SHA1: config->md = EVP_sha1(); break;
		case RFC6238_DIGEST_SHA256: config->md = EVP_sha256(); break;
		case RFC6238_DIGEST_SHA384: config->md = EVP_sha384(); break;
		case RFC6238_DIGEST_SHA512: config->md = EVP_sha512(); break;
	}
	config->digits = digits;
	config->modulo = 1;
	for (unsigned int i = 0; i < digits; i++) {
		config->modulo *= 10;
	}
	config->slice_time_seconds = slice_time_seconds;
	config->secret_length = secret_length;
	memcpy(config->secret, secret, secret_length);
	return config;
}

static void rfc6238_present(const struct rfc6238_config_t *config, char output[static 9], uint32_t value) {
	value %= config->modulo;
	snprintf(output, 9, "%0*d", config->digits, value);
}

bool rfc6238_generate(const struct rfc6238_config_t *config, char output[static 9], uint64_t timecode) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	/* Timecode needs to be big endian */
	timecode = bswap_64(timecode);
#endif

	const unsigned int digest_len = EVP_MD_size(config->md);
	unsigned char digest[digest_len];
	const unsigned char *mac_data = (const unsigned char*)&timecode;
	unsigned char *result = HMAC(config->md, config->secret, config->secret_length, mac_data, 8, digest, NULL);
	if (!result) {
		return false;
	}

	unsigned int offset = digest[digest_len - 1] & 0xf;

	uint32_t value = ((uint32_t)digest[offset + 0] << 24) | ((uint32_t)digest[offset + 1] << 16) | ((uint32_t)digest[offset + 2] << 8) | ((uint32_t)digest[offset + 3] << 0);
	value = value & 0x7fffffff;
	rfc6238_present(config, output, value);
	return true;
}

bool rfc6238_generate_at(const struct rfc6238_config_t *config, char output[static 9], time_t t) {
	uint64_t timecode = t / config->slice_time_seconds;
	return rfc6238_generate(config, output, timecode);
}

bool rfc6238_generate_now(const struct rfc6238_config_t *config, char output[static 9]) {
	return rfc6238_generate(config, output, time(NULL));
}

void rfc6238_free(struct rfc6238_config_t *config) {
	OPENSSL_cleanse(config, sizeof(struct rfc6238_config_t));
	free(config);
}
