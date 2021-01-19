.PHONY: all tests

vpath %.c ..

CFLAGS := -std=c11 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=500 -D_DEFAULT_SOURCE -Wall -Werror -Wmissing-prototypes -Wstrict-prototypes -Werror=implicit-function-declaration -Wno-stringop-truncation -O3 -I.. -g3
CFLAGS += -pie -fPIE -fsanitize=address -fsanitize=undefined -fsanitize=leak
LDFLAGS :=

TEST_COMMON_OBJS := testbench.o testmain.o
TEST_OBJS := \
	test_stringlist \
	test_strings \
	test_vfs

all: $(TEST_COMMON_OBJS) $(TEST_OBJS)

test_stringlist: $(TEST_COMMON_OBJS) test_stringlist_entry.o stringlist.o
test_strings: $(TEST_COMMON_OBJS) test_strings_entry.o strings.o
test_vfs: $(TEST_COMMON_OBJS) test_vfs_entry.o vfs.o vfsdebug.o strings.o logging.o stringlist.o

%_entry.c: %.c
	./generate_entry $< $@

tests: all
	for testname in $(TEST_OBJS); do ./$$testname; done

clean:
	rm -f $(TEST_COMMON_OBJS) $(TEST_OBJS) ../*.o *.o

.c:
	$(CC) $(CFLAGS) -o $@ $+ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -include testbench.h -c -o $@ $<

.s.o:
	$(CC) $(CFLAGS) -c -o $@ $<
