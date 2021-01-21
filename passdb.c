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

#include <stdbool.h>
#include <sys/random.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include "passdb.h"

typedef bool (*passdb_derivation_fnc_t)(const struct passdb_entry_t *entry, const char *passphrase, unsigned int passphrase_length, uint8_t digest[static PASSDB_PASS_SIZE_BYTES]);

static const union passdb_kdf_params_t default_params[] = {
	[PASSDB_KDF_PBKDF2_SHA256] = {
		.pbkdf2 = {
			.iterations = 140845,		/* 100ms on Intel Core i7-5930K CPU @ 3.50GHz */
		},
	},
	[PASSDB_KDF_SCRYPT] = {
		.scrypt = {
			.N = 32768,					/* 100ms on Intel Core i7-5930K CPU @ 3.50GHz */
			.r = 8,
			.p = 1,
			.maxmem_mib = 33,
		},
	},
};

static bool passdb_derive_pbkdf2_sha256(const struct passdb_entry_t *entry, const char *passphrase, unsigned int passphrase_length, uint8_t digest[static PASSDB_PASS_SIZE_BYTES]) {
	return PKCS5_PBKDF2_HMAC(passphrase, passphrase_length, entry->salt, PASSDB_SALT_SIZE_BYTES, entry->params.pbkdf2.iterations, EVP_sha256(), PASSDB_PASS_SIZE_BYTES, digest);
}

static bool passdb_derive_scrypt(const struct passdb_entry_t *entry, const char *passphrase, unsigned int passphrase_length, uint8_t digest[static PASSDB_PASS_SIZE_BYTES]) {
	EVP_PKEY_CTX *pctx = NULL;
	bool success = false;

	do {
		pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_SCRYPT, NULL);
		if (!pctx) {
			break;
		}
		if (EVP_PKEY_derive_init(pctx) <= 0) {
			break;
		}
		if (EVP_PKEY_CTX_set1_pbe_pass(pctx, passphrase, passphrase_length) <= 0) {
			break;
		}
		if (EVP_PKEY_CTX_set1_scrypt_salt(pctx, entry->salt, PASSDB_SALT_SIZE_BYTES) <= 0) {
			break;
		}
		if (EVP_PKEY_CTX_set_scrypt_N(pctx, entry->params.scrypt.N) <= 0) {
			break;
		}
		if (EVP_PKEY_CTX_set_scrypt_r(pctx, entry->params.scrypt.r) <= 0) {
			break;
		}
		if (EVP_PKEY_CTX_set_scrypt_p(pctx, entry->params.scrypt.p) <= 0) {
			break;
		}
		if (EVP_PKEY_CTX_set_scrypt_maxmem_bytes(pctx, entry->params.scrypt.maxmem_mib * 1024 * 1024) <= 0) {
			break;
		}
		size_t dklen = PASSDB_PASS_SIZE_BYTES;
		if (EVP_PKEY_derive(pctx, digest, &dklen) <= 0) {
			break;
		}
		success = true;
	} while (false);

	if (pctx) {
		EVP_PKEY_CTX_free(pctx);
	}
	return success;
}

static bool passdb_derive(const struct passdb_entry_t *entry, const char *passphrase, unsigned int passphrase_length, uint8_t digest[static PASSDB_PASS_SIZE_BYTES]) {
	const passdb_derivation_fnc_t derivation_functions[] = {
		[PASSDB_KDF_PBKDF2_SHA256] = passdb_derive_pbkdf2_sha256,
		[PASSDB_KDF_SCRYPT] = passdb_derive_scrypt,
	};
	return derivation_functions[entry->kdf](entry, passphrase, passphrase_length, digest);
}

bool passdb_create_entry(struct passdb_entry_t *entry, enum passdb_kdf_t kdf, const union passdb_kdf_params_t *params, const char *passphrase) {
	entry->kdf = kdf;
	if (params) {
		entry->params = *params;
	} else {
		entry->params = default_params[entry->kdf];
	}
	if (getrandom(entry->salt, PASSDB_SALT_SIZE_BYTES, 0) != PASSDB_SALT_SIZE_BYTES) {
		/* Failed to generate salt */
		return false;
	}
	return passdb_derive(entry, passphrase, strlen(passphrase), entry->hash);
}

void passdb_entry_dump(const struct passdb_entry_t *entry) {
	switch (entry->kdf) {
		case PASSDB_KDF_PBKDF2_SHA256: printf("PBKDF2-SHA256"); break;
		case PASSDB_KDF_SCRYPT: printf("scrypt"); break;
	}
	printf("(");
	switch (entry->kdf) {
		case PASSDB_KDF_PBKDF2_SHA256: printf("iterations %u", entry->params.pbkdf2.iterations); break;
		case PASSDB_KDF_SCRYPT: printf("N %u, r %u, p %u, maxmem_mib %u", entry->params.scrypt.N, entry->params.scrypt.r, entry->params.scrypt.p, entry->params.scrypt.maxmem_mib); break;
	}
	printf(")\n");
	printf("Salt [%2d]: ", PASSDB_SALT_SIZE_BYTES);
	for (unsigned int i = 0; i < PASSDB_SALT_SIZE_BYTES; i++) {
		printf("%02x", entry->salt[i]);
	}
	printf("\n");
	printf("Hash [%2d]: ", PASSDB_PASS_SIZE_BYTES);
	for (unsigned int i = 0; i < PASSDB_PASS_SIZE_BYTES; i++) {
		printf("%02x", entry->hash[i]);
	}
	printf("\n");
	if (entry->totp) {
		printf("TOTP enabled; window size +-%d seconds\n", entry->totp->slice_time_seconds * entry->totp_window_size);
	}
}

void passdb_attach_totp(struct passdb_entry_t *entry, struct rfc6238_config_t *totp, unsigned int window_size_seconds) {
	unsigned int totp_window_size = (window_size_seconds + totp->slice_time_seconds - 1) / totp->slice_time_seconds;
	entry->totp = totp;
	entry->totp_window_size = totp_window_size;
}

bool passdb_validate_totp(const struct passdb_entry_t *entry, const char *totp_provided, time_t timestamp, int offset) {
	if (!entry->totp) {
		return true;
	}

	time_t effective_timestamp = timestamp + (offset * (int)entry->totp->slice_time_seconds);
	char totp_expected[12];
	if (!rfc6238_generate_at(entry->totp, totp_expected, effective_timestamp)) {
		return false;
	}

	bool totp_correct = !strcmp(totp_provided, totp_expected);
	return totp_correct;
}

bool passdb_validate_around(const struct passdb_entry_t *entry, const char *passphrase, time_t timestamp) {
	size_t passlen = strlen(passphrase);
	const char *totp_provided = "";
	if (entry->totp) {
		if (passlen >= entry->totp->digits) {
			totp_provided = passphrase + passlen - entry->totp->digits;
			passlen -= entry->totp->digits;
		}
	}

	uint8_t digest[PASSDB_PASS_SIZE_BYTES];
	if (!passdb_derive(entry, passphrase, passlen, digest)) {
		return false;
	}
	bool digest_correct = !memcmp(entry->hash, digest, PASSDB_PASS_SIZE_BYTES);

	/* Always validate the TOTP to keep impact of timing side channels as low
	 * as possible */
	if (passdb_validate_totp(entry, totp_provided, timestamp, 0)) {
		return digest_correct;
	}
	for (int offset = 1; offset <= entry->totp_window_size; offset++) {
		if (passdb_validate_totp(entry, totp_provided, timestamp, -offset)) {
			return digest_correct;
		}
		if (passdb_validate_totp(entry, totp_provided, timestamp, offset)) {
			return digest_correct;
		}
	}

	return false;
}

bool passdb_validate(const struct passdb_entry_t *entry, const char *passphrase) {
	return passdb_validate_around(entry, passphrase, time(NULL));
}
