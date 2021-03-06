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
#include <sys/stat.h>
#include "testbench.h"
#include "vfs.h"
#include "vfsdebug.h"
#include "test_vfs.h"

void test_empty_vfs(void) {
	struct vfs_t *vfs = vfs_init();
	vfs_freeze_inodes(vfs);
	vfs_free(vfs);
}

struct vfs_lookup_data_t {
	struct {
		const char *path;
	} input;
	struct {
		bool success;
		struct vfs_lookup_result_t result;
	} expect;
};

static void verify_vfs_lookups(struct vfs_t *vfs, const struct vfs_lookup_data_t *expected_outcome, unsigned int testcase_count) {
	for (unsigned int i = 0; i < testcase_count; i++) {
		const struct vfs_lookup_data_t *testcase = &expected_outcome[i];
		struct vfs_lookup_result_t result = { 0 };
		test_debug("Testing lookup of: %s", testcase->input.path);
		bool success = vfs_lookup(vfs, &result, testcase->input.path);
		test_assert_bool_eq(success, testcase->expect.success);
		if (success) {
			if (is_testing_verbose()) {
				vfs_dump_map_result(stderr, &result);
			}
			test_assert_int_eq(result.flags, testcase->expect.result.flags);
			if (testcase->expect.result.mountpoint) {
				test_assert(result.mountpoint != NULL);
				test_assert_str_eq(result.mountpoint->virtual_path, testcase->expect.result.mountpoint->virtual_path);
				test_assert_str_eq(result.mountpoint->target_path, testcase->expect.result.mountpoint->target_path);
			}
			if (testcase->expect.result.inode) {
				test_assert(result.inode != NULL);
				test_assert_str_eq(result.inode->virtual_path, testcase->expect.result.inode->virtual_path);
				test_assert_str_eq(result.inode->target_path, testcase->expect.result.inode->target_path);
			}
		}
	}
}

void test_vfs_lookup(void) {
	struct vfs_t *vfs = vfs_init();

	vfs_add_inode(vfs, "/", NULL, VFS_INODE_FLAG_READ_ONLY, 0);
	vfs_add_inode(vfs, "/pics/", "/home/joe/pics/", 0, 0);
	vfs_add_inode(vfs, "/pics/foo/neu/", NULL, 0, 0);
	vfs_add_inode(vfs, "/this/is/", NULL, VFS_INODE_FLAG_DISALLOW_CREATE_DIR | VFS_INODE_FLAG_DISALLOW_UNLINK, 0);
	vfs_add_inode(vfs, "/this/is/deeply/nested/", "/home/joe/nested/", VFS_INODE_FLAG_ALLOW_SYMLINKS, 0);
	vfs_add_inode(vfs, "/zeug/", "/tmp/zeug/", 0, 0);
	vfs_add_inode(vfs, "/incoming/", "/tmp/write/", VFS_INODE_FLAG_DISALLOW_UNLINK, 0);
	vfs_freeze_inodes(vfs);
	if (is_testing_verbose()) {
		vfs_dump(stderr, vfs);
	}


	const struct vfs_lookup_data_t expected_outcome[] = {
		{
			.input.path = "/",
			.expect.success = true,
			.expect.result.flags = VFS_INODE_FLAG_READ_ONLY,
			.expect.result.mountpoint = NULL,
			.expect.result.inode = &(struct vfs_inode_t){ .virtual_path = "", .target_path = NULL },
		},
		{
			.input.path = "/pics/foo.jpg",
			.expect.success = true,
			.expect.result.flags = VFS_INODE_FLAG_READ_ONLY,
			.expect.result.mountpoint = &(struct vfs_inode_t){ .virtual_path = "/pics", .target_path = "/home/joe/pics" },
			.expect.result.inode = NULL,
		},
		{
			.input.path = "/pics/x/y/z.jpg",
			.expect.success = true,
			.expect.result.flags = VFS_INODE_FLAG_READ_ONLY,
			.expect.result.mountpoint = &(struct vfs_inode_t){ .virtual_path = "/pics", .target_path = "/home/joe/pics" },
			.expect.result.inode = NULL,
		},
		{
			.input.path = "/this",
			.expect.success = true,
			.expect.result.flags = VFS_INODE_FLAG_READ_ONLY,
			.expect.result.mountpoint = NULL,
			.expect.result.inode = &(struct vfs_inode_t){ .virtual_path = "/this", .target_path = NULL },
		},
		{
			.input.path = "/incoming/foo/blubb.jpg",
			.expect.success = true,
			.expect.result.flags = VFS_INODE_FLAG_READ_ONLY | VFS_INODE_FLAG_DISALLOW_UNLINK,
			.expect.result.mountpoint = &(struct vfs_inode_t){ .virtual_path = "/incoming", .target_path = "/tmp/write" },
			.expect.result.inode = NULL,
		},
		{
			.input.path = "/this/is/deeply/nested/blah/mookoo/xyz.jpg",
			.expect.success = true,
			.expect.result.flags = VFS_INODE_FLAG_READ_ONLY | VFS_INODE_FLAG_DISALLOW_UNLINK | VFS_INODE_FLAG_DISALLOW_CREATE_DIR | VFS_INODE_FLAG_DISALLOW_UNLINK | VFS_INODE_FLAG_ALLOW_SYMLINKS,
			.expect.result.mountpoint = &(struct vfs_inode_t){ .virtual_path = "/this/is/deeply/nested", .target_path = "/home/joe/nested" },
			.expect.result.inode = NULL,
		}
	};
	verify_vfs_lookups(vfs, expected_outcome, sizeof(expected_outcome) / sizeof(struct vfs_lookup_data_t));

#if 0

	struct vfs_lookup_result_t result;
	const char *lookup_nodes[] = {
		"/",
		"/pics/foo/neu/bar",
		"/foo/bar",
		"/this",
		"/pics/DCIM/12345.jpg",
		"foo"
	};
	for (unsigned int i = 0; i < sizeof(lookup_nodes) / sizeof(const char*); i++) {
		const char *lookup = lookup_nodes[i];
		printf("%s\n", lookup);
		vfs_lookup(vfs, &result, lookup);
		vfs_dump_map_result(stderr, &result);
		printf("\n");
	}
#endif
	vfs_free(vfs);
}

void test_vfs_ro_root(void) {
	struct vfs_t *vfs = vfs_init();

	vfs_add_inode(vfs, "/", NULL, VFS_INODE_FLAG_READ_ONLY, 0);
	vfs_add_inode(vfs, "/pics/", "/home/joe/pics/", 0, 0);

	vfs_free(vfs);
}

void test_vfs_opendir(void) {
	mkdir("/tmp/umsftpd_test", 0755);

	struct vfs_t *vfs = vfs_init();

	vfs_add_inode(vfs, "/", "/tmp", VFS_INODE_FLAG_READ_ONLY, 0);
	vfs_freeze_inodes(vfs);

	struct vfs_handle_t *handle = NULL;
	enum vfs_error_t result = vfs_opendir(vfs, "/foo/.fhud/../../umsftpd_test", &handle);
	if (result != VFS_OK) {
		test_debug("VFS error %d: %s", vfs->error.code, vfs->error.string);
	}
	test_assert_int_eq(result, VFS_OK);
	test_assert(handle);
	vfs_close_handle(handle);
	vfs_free(vfs);
}
