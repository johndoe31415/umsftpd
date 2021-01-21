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

#ifndef __PASSDB_H__
#define __PASSDB_H__

#include <stdint.h>
#include <stdbool.h>

#include "rfc6238.h"

#define PASSDB_SALT_SIZE_BYTES		16
#define PASSDB_PASS_SIZE_BYTES		32

enum passdb_kdf_t {
	PASSDB_KDF_PBKDF2_SHA256,
	PASSDB_KDF_SCRYPT,
};

struct passdb_kdf_params_pbkdf2_t {
	unsigned int iterations;
};

struct passdb_kdf_params_scrypt_t {
	unsigned int N, r, p;
	unsigned int maxmem_mib;
};

union passdb_kdf_params_t {
	struct passdb_kdf_params_pbkdf2_t pbkdf2;
	struct passdb_kdf_params_scrypt_t scrypt;
};

struct passdb_entry_t {
	enum passdb_kdf_t kdf;
	union passdb_kdf_params_t params;
	uint8_t salt[PASSDB_SALT_SIZE_BYTES];
	uint8_t hash[PASSDB_PASS_SIZE_BYTES];
	struct rfc6238_config_t *totp;
	unsigned int totp_window_size;
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
bool passdb_create_entry(struct passdb_entry_t *entry, enum passdb_kdf_t kdf, const union passdb_kdf_params_t *params, const char *passphrase);
void passdb_entry_dump(const struct passdb_entry_t *entry);
void passdb_attach_totp(struct passdb_entry_t *entry, struct rfc6238_config_t *totp, unsigned int window_size_seconds);
bool passdb_validate_totp(const struct passdb_entry_t *entry, const char *totp_provided, time_t timestamp, int offset);
bool passdb_validate_around(const struct passdb_entry_t *entry, const char *passphrase, time_t timestamp);
bool passdb_validate(const struct passdb_entry_t *entry, const char *passphrase);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
