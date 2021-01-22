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
#include <string.h>
#include <json.h>
#include "jsonconfig.h"

#define JSON_PARSE_ERROR_MAXLEN		64

struct json_parse_ctx_t {
	struct json_config_t *config;
	char error_string[JSON_PARSE_ERROR_MAXLEN];
	bool parse_error;
};

static bool jsonconfig_parse_base(struct json_parse_ctx_t *ctx, struct json_object *base) {
	if (!json_object_is_type(base, json_type_object)) {
		snprintf(ctx->error_string, JSON_PARSE_ERROR_MAXLEN, "config[\"base\"] element not a dictionary");
		return false;
	}
	json_object_object_foreach(base, key, value) {
		if (!strcmp(key, "bind_addr")) {
			if (!json_object_is_type(value, json_type_string)) {
				snprintf(ctx->error_string, JSON_PARSE_ERROR_MAXLEN, "config[\"base\"][\"bind_addr\"] element not a string");
				return false;
			}
			ctx->config->base.bind_addr = json_object_get_string(value);
		} else if (!strcmp(key, "bind_port")) {
			if (!json_object_is_type(value, json_type_int)) {
				snprintf(ctx->error_string, JSON_PARSE_ERROR_MAXLEN, "config[\"base\"][\"bind_port\"] element not a integer");
				return false;
			}
			ctx->config->base.bind_port = json_object_get_int(value);
		} else if (!strcmp(key, "server_key_filename")) {
			if (!json_object_is_type(value, json_type_string)) {
				snprintf(ctx->error_string, JSON_PARSE_ERROR_MAXLEN, "config[\"base\"][\"server_key_filename\"] element not a string");
				return false;
			}
			ctx->config->base.server_key_filename = json_object_get_string(value);
		} else if (!strcmp(key, "loglevel")) {
			if (!json_object_is_type(value, json_type_string)) {
				snprintf(ctx->error_string, JSON_PARSE_ERROR_MAXLEN, "config[\"base\"][\"loglevel\"] element not a string");
				return false;
			}
			ctx->config->base.loglevel = json_object_get_string(value);
		}
	}
	return true;
}

static bool jsonconfig_parse_auth_user(struct json_parse_ctx_t *ctx, const char *user_name, struct json_object *auth_dict) {
	if (!json_object_is_type(auth_dict, json_type_object)) {
		snprintf(ctx->error_string, JSON_PARSE_ERROR_MAXLEN, "config[\"auth\"][\"%s\"] element not a dictionary", user_name);
		return false;
	}

//	json_object_object_foreach(auth, key, value) {
//	}

	return true;
}

static bool jsonconfig_parse_auth(struct json_parse_ctx_t *ctx, struct json_object *auth) {
	if (!json_object_is_type(auth, json_type_object)) {
		snprintf(ctx->error_string, JSON_PARSE_ERROR_MAXLEN, "config[\"auth\"] element not a dictionary");
		return false;
	}

	json_object_object_foreach(auth, user_name, auth_dict) {
		if (!jsonconfig_parse_auth_user(ctx, user_name, auth_dict)) {
			return false;
		}
	}

	return true;
}

static bool jsonconfig_parse_vfs(struct json_parse_ctx_t *ctx, struct json_object *vfs) {
	if (!json_object_is_type(vfs, json_type_object)) {
		snprintf(ctx->error_string, JSON_PARSE_ERROR_MAXLEN, "config[\"vfs\"] element not a dictionary");
		return false;
	}
	return true;
}

static bool jsonconfig_parse_root(struct json_parse_ctx_t *ctx, struct json_object *root) {
	if (!json_object_is_type(root, json_type_object)) {
		snprintf(ctx->error_string, JSON_PARSE_ERROR_MAXLEN, "root element not a dictionary");
		return false;
	}

	json_object_object_foreach(root, key, value) {
		if (!strcmp(key, "base")) {
			if (!jsonconfig_parse_base(ctx, value)) {
				return false;
			}
		} else if (!strcmp(key, "auth")) {
			if (!jsonconfig_parse_auth(ctx, value)) {
				return false;
			}
		} else if (!strcmp(key, "vfs")) {
			if (!jsonconfig_parse_vfs(ctx, value)) {
				return false;
			}
		}
	}

	return true;
}

struct json_config_t *jsonconfig_parse(const char *filename) {
	struct json_parse_ctx_t ctx = { 0 };

	ctx.config = calloc(1, sizeof(struct json_config_t));
	if (!ctx.config) {
		return NULL;
	}

	struct json_object* root = json_object_from_file(filename);
	if (!root) {
		jsonconfig_free(ctx.config);
		return NULL;
	}

	if (!jsonconfig_parse_root(&ctx, root)) {
		jsonconfig_free(ctx.config);
		ctx.config = NULL;
		fprintf(stderr, "parse error: %s\n", ctx.error_string);
	}

	json_object_put(root);
	return ctx.config;
}

void jsonconfig_free(struct json_config_t *config) {
	free(config);
}
