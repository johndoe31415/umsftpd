.PHONY: all test

vpath %.c ..

CFLAGS := -std=c11 -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=500 -Wall -Werror -Wmissing-prototypes -Wstrict-prototypes -Werror=implicit-function-declaration -O3 -I.. -g3
CFLAGS += -pie -fPIE -fsanitize=address -fsanitize=undefined -fsanitize=leak
LDFLAGS :=

TEST_COMMON_OBJS := testbench.o testmain.o
TEST_OBJS := \
	test_strings \
	test_vfs

all: $(TEST_COMMON_OBJS) $(TEST_OBJS)

test_strings: $(TEST_COMMON_OBJS) test_strings_entry.o strings.o
test_vfs: $(TEST_COMMON_OBJS) test_vfs_entry.o vfs.o vfsdebug.o strings.o

%_entry.c: %.c
	./generate_entry $< $@

test: all
	for testname in $(TEST_OBJS); do ./$$testname; done

clean:
	rm -f $(TEST_COMMON_OBJS) $(TEST_OBJS) ../*.o *.o

.c:
	$(CC) $(CFLAGS) -o $@ $+ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -include testbench.h -c -o $@ $<

.s.o:
	$(CC) $(CFLAGS) -c -o $@ $<
