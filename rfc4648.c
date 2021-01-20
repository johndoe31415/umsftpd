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

#include <string.h>
#include <stdint.h>
#include "rfc4648.h"

static const char rfc4648_padding_char = '=';

unsigned int rfc4648_base32_size(const char *input_string) {
	int length = strlen(input_string);
	while ((length > 0) && (input_string[length - 1] == rfc4648_padding_char)) {
		length--;
	}
	return length;
}

/*
  0      1      2      3      4      5      6      7
43210  43210  43210  43210  43210  43210  43210  43210
76543  21076  54321  07654  32107  65432  10765  43210
0         1           2         3           4

76543210   76543210   76543210   76543210   76543210
43210432   10432104   32104321   04321043   21043210
*/
static bool rfc4648_decode_base32_multi(void *output, unsigned int bufsize, const char *input_string, const char *alphabet) {
	unsigned int src_offset = 0;
	unsigned int dst_offset = 0;
	bool finished = false;
	while (!finished) {
		int mapped_chars[8];
		for (unsigned int i = 0; i < 8; i++) {
			mapped_chars[i] = -1;
		}
		for (unsigned int i = 0; i < 8; i++) {
			if ((input_string[src_offset + i] == 0) || (input_string[src_offset + i] == rfc4648_padding_char)) {
				break;
			}
			char *sub = strchr(alphabet, input_string[src_offset + i]);
			if (!sub) {
				return false;
			}
			mapped_chars[i] = sub - alphabet;
		}

		for (unsigned int i = 0; i < 5; i++) {
			uint8_t next;
			if ((i == 0) && (mapped_chars[0] != -1) && (mapped_chars[1] != -1)) {
				next = (mapped_chars[0] << 3) | (mapped_chars[1] >> 2);
			} else if ((i == 1) && (mapped_chars[1] != -1) && (mapped_chars[2] != -1) && (mapped_chars[3] != -1)) {
				next = (mapped_chars[1] << 6) | (mapped_chars[2] << 1) | (mapped_chars[3] >> 4);
			} else if ((i == 2) && (mapped_chars[3] != -1) && (mapped_chars[4] != -1)) {
				next = (mapped_chars[3] << 4) | (mapped_chars[4] >> 1);
			} else if ((i == 3) && (mapped_chars[4] != -1) && (mapped_chars[5] != -1) && (mapped_chars[6] != -1)) {
				next = (mapped_chars[4] << 7) | (mapped_chars[5] << 2) | (mapped_chars[6] >> 3);
			} else if ((i == 4) && (mapped_chars[6] != -1) && (mapped_chars[7] != -1)) {
				next = (mapped_chars[6] << 5) | (mapped_chars[7] >> 0);
			} else {
				/* End of string */
				finished = true;
				break;
			}

			if (dst_offset >= bufsize) {
				/* No more room to convert, return error */
				return false;
			}
			((uint8_t*)output)[dst_offset++] = next;
		}
		src_offset += 8;
	}
	return true;
}

bool rfc4648_decode_base32hex(void *output, unsigned int bufsize, const char *input_string) {
	return rfc4648_decode_base32_multi(output, bufsize, input_string, "0123456789ABCDEFGHIJKLMNOPQRSTUV");
}

bool rfc4648_decode_base32(void *output, unsigned int bufsize, const char *input_string) {
	return rfc4648_decode_base32_multi(output, bufsize, input_string, "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567");
}
