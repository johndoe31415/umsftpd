#include "testbench.h"

%for testcase in testcases:
void ${testcase.function_name}(void);
%endfor

const char *test_name = "${test_name}";
struct testcase_t testcases[] = {
%for testcase in testcases:
	{
		.name = "${testcase.name}",
		.entry = ${testcase.function_name},
	},
%endfor
};
const unsigned int testcase_count = ${len(testcases)};
