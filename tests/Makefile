.PHONY: all tests

vpath %.c ..

CFLAGS := -std=c11 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=500 -D_DEFAULT_SOURCE -Wall -Werror -Wmissing-prototypes -Wstrict-prototypes -Werror=implicit-function-declaration -Wno-stringop-truncation -O0 -I.. -g3
CFLAGS += -pie -fPIE -fsanitize=address -fsanitize=undefined -fsanitize=leak
CFLAGS += `pkg-config --cflags libssh` `pkg-config --cflags openssl` `pkg-config --cflags json-c`
LDFLAGS += `pkg-config --libs libssh` `pkg-config --libs openssl` `pkg-config --libs json-c`

TEST_COMMON_OBJS := testbench.o testmain.o
TEST_OBJS := \
	test_jsonconfig \
	test_passdb \
	test_rfc4648 \
	test_rfc6238 \
	test_stringlist \
	test_strings \
	test_vfs

all: $(TEST_COMMON_OBJS) $(TEST_OBJS)

test_jsonconfig: $(TEST_COMMON_OBJS) test_jsonconfig_entry.o jsonconfig.o
test_passdb: $(TEST_COMMON_OBJS) test_passdb_entry.o passdb.o rfc6238.o
test_rfc4648: $(TEST_COMMON_OBJS) test_rfc4648_entry.o rfc4648.o
test_rfc6238: $(TEST_COMMON_OBJS) test_rfc6238_entry.o rfc6238.o
test_stringlist: $(TEST_COMMON_OBJS) test_stringlist_entry.o stringlist.o
test_strings: $(TEST_COMMON_OBJS) test_strings_entry.o strings.o
test_vfs: $(TEST_COMMON_OBJS) test_vfs_entry.o vfs.o vfsdebug.o strings.o logging.o stringlist.o

%_entry.c: %.c
	./generate_entry $< $@

tests: all
	for testname in $(TEST_OBJS); do ./$$testname || exit 1; done

clean:
	rm -f $(TEST_COMMON_OBJS) $(TEST_OBJS) ../*.o *.o

.c:
	$(CC) $(CFLAGS) -o $@ $+ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -include testbench.h -c -o $@ $<

.s.o:
	$(CC) $(CFLAGS) -c -o $@ $<
