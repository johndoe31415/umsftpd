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
#include <stdarg.h>
#include "logging.h"

static enum loglevel_t current_llvl;
static const char *level_str[] = {
	[LLVL_CRITICAL] = "C",
	[LLVL_ERROR] = "E",
	[LLVL_WARN] = "W",
	[LLVL_INFO] = "I",
	[LLVL_DEBUG] = "D",
	[LLVL_TRACE] = "T",
};

enum loglevel_t llvl_get(void) {
	return current_llvl;
}

void llvl_set(enum loglevel_t loglevel) {
	current_llvl = loglevel;
}

void  __attribute__ ((format (printf, 2, 3))) logmsg(enum loglevel_t llvl, const char *msg, ...) {
	if (llvl <= current_llvl) {
		va_list ap;
		fprintf(stderr, "[%s]: ", level_str[llvl]);

		va_start(ap, msg);
		vfprintf(stderr, msg, ap);
		va_end(ap);
		fprintf(stderr, "\n");
	}
}
