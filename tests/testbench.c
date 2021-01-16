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
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include "testbench.h"

static const char *current_testcase;
static struct test_options_t current_options;

bool is_testing_verbose(void) {
	return current_options.verbose;
}

void  __attribute__ ((format (printf, 1, 2))) test_debug(const char *msg, ...) {
	if (is_testing_verbose()) {
		fprintf(stderr, "[%s] ", current_testcase);
		va_list ap;
		va_start(ap, msg);
		vfprintf(stderr, msg, ap);
		va_end(ap);
		fprintf(stderr, "\n");
	}
}

void test_fail_ext(const char *file, int line, const char *fncname, const char *reason, failfnc_t failfnc, const void *lhs, const void *rhs) {
	fprintf(stderr, "FAILED %s %s:%d %s: %.80s\n", current_testcase, file, line, fncname, reason);
	if (failfnc != NULL) {
		char *extended_reason = failfnc(lhs, rhs);
		fprintf(stderr, "   %.120s\n", extended_reason);
		free(extended_reason);
	}
	exit(EXIT_FAILURE);
}

void test_fail(const char *file, int line, const char *fncname, const char *reason) {
	test_fail_ext(file, line, fncname, reason, NULL, NULL, NULL);
}

char *testbed_failfnc_int(const void *vlhs, const void *vrhs) {
	const int lhs = *((const int*)vlhs);
	const int rhs = *((const int*)vrhs);
	char *result = calloc(1, 64);
	sprintf(result, "LHS = %d, RHS = %d", lhs, rhs);
	return result;
}

char *testbed_failfnc_str(const void *vlhs, const void *vrhs) {
	const char *lhs = (const char*)vlhs;
	const char *rhs = (const char*)vrhs;
	char *result = calloc(1, strlen(lhs) + strlen(rhs) + 32);
	sprintf(result, "LHS = \"%s\", RHS = \"%s\"", lhs, rhs);
	return result;
}

char *testbed_failfnc_bool(const void *vlhs, const void *vrhs) {
	const bool lhs = *((const bool*)vlhs);
	const bool rhs = *((const bool*)vrhs);
	char *result = calloc(1, 32);
	sprintf(result, "LHS = %s, RHS = %s", lhs ? "true" : "false", rhs ? "true" : "false");
	return result;
}

static void test_run(struct testcase_t *testcase, const struct test_options_t *options) {
	pid_t pid = fork();
	current_testcase = testcase->name;
	current_options = *options;
	if (pid > 0) {
		/* Parent, overwatch. */
		int wstatus;
		waitpid(pid, &wstatus, 0);

		int exit_status = WEXITSTATUS(wstatus);
		if (exit_status == 0) {
			testcase->outcome = TEST_PASSED;
		} else {
			testcase->outcome = TEST_FAILED;
		}
	} else if (pid == 0) {
		/* Child, run the testcase */
		testcase->entry();
		exit(EXIT_SUCCESS);
	} else if (pid < 0) {
		fprintf(stderr, "Fatal error at fork(): %s\n", strerror(errno));
		return;
	}
}

void tests_run_all(struct testcase_t *testcases, unsigned int testcase_count) {
	for (unsigned int i = 0; i < testcase_count; i++) {
		struct test_options_t options = {
			.verbose = false,
		};
		test_run(&testcases[i], &options);
		if (testcases[i].outcome == TEST_FAILED) {
			/* Rerun verbosely */
			fprintf(stderr, "====== Rerun of failed testcase: %s ====================================================\n", testcases[i].name);
			options.verbose = true;
			test_run(&testcases[i], &options);
			fprintf(stderr, "========================================================================================\n");
		}
	}
}
