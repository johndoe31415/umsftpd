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

#ifndef __LOGGING_H__
#define __LOGGING_H__

enum loglevel_t {
	LLVL_CRITICAL = 0,
	LLVL_ERROR = 1,
	LLVL_WARN = 2,
	LLVL_INFO = 3,
	LLVL_DEBUG = 4,
	LLVL_TRACE = 5
};

/*************** AUTO GENERATED SECTION FOLLOWS ***************/
enum loglevel_t llvl_get(void);
void llvl_set(enum loglevel_t loglevel);
void  __attribute__ ((format (printf, 2, 3))) logmsg(enum loglevel_t llvl, const char *msg, ...);
/***************  AUTO GENERATED SECTION ENDS   ***************/

#endif
