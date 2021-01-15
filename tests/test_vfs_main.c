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
#include "testbench.h"

void test_empty_vfs(void);
void test_vfs_lookup(void);
void test_vfs_ro_root(void);

static const char *test_name = "test_vfs.c";
static struct testcase_t testcases[] = {
	{
		.name = "empty_vfs",
		.entry = test_empty_vfs,
	},
	{
		.name = "vfs_lookup",
		.entry = test_vfs_lookup,
	},
	{
		.name = "vfs_ro_root",
		.entry = test_vfs_ro_root,
	},
};

int main(int argc, char **argv) {
	const unsigned int test_count = sizeof(testcases) / sizeof(struct testcase_t);
	tests_run_all(testcases, test_count);

	unsigned int pass_cnt = 0;
	unsigned int fail_cnt = 0;
	for (unsigned int i = 0; i < test_count; i++) {
		if (testcases[i].outcome == TEST_FAILED) {
			fail_cnt++;
		} else if (testcases[i].outcome == TEST_PASSED) {
			pass_cnt++;
		}
	}
	fprintf(stderr, "%s: %d tests run, %d pass and %d FAIL.\n", test_name, pass_cnt + fail_cnt, pass_cnt, fail_cnt);
	return (fail_cnt == 0) ? 0 : 1;
}
