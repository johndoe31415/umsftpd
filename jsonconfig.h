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

#ifndef __JSONCONFIG_H__
#define __JSONCONFIG_H__

#include <stdbool.h>

struct json_base_config_t {
	const char *bind_addr;
	unsigned int bind_port;
	const char *server_key_filename;
	const char *loglevel;
};

struct json_config_t {
	struct json_base_config_t base;
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
struct json_config_t *jsonconfig_parse(const char *filename);
void jsonconfig_free(struct json_config_t *config);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
