.PHONY: all test

vpath %.c ..

CFLAGS := -std=c11 -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=500 -Wall -Wmissing-prototypes -Wstrict-prototypes -Werror=implicit-function-declaration -O3 -I.. -g3
ifneq ($(USER),travis)
# On Travis-CI, gcc does not support "undefined" and "leak" sanitizers.
# Furthermore (and worse, actually), there seems to be a kernel < 4.12.8
# installed which causes the address sanitizer to cause spurious fails ("Shadow
# memory range interleaves with an existing memory mapping. ASan cannot proceed
# correctly. ABORTING."), leading to a broken build. Therefore we do not run
# sanitizers on Travis.
CFLAGS += -pie -fPIE -fsanitize=address -fsanitize=undefined -fsanitize=leak
endif
LDFLAGS :=

TEST_COMMON_OBJS := testbench.o
TEST_OBJS := \
	test_strings \
	test_vfs

all: $(TEST_COMMON_OBJS) $(TEST_OBJS)

test_strings: $(TEST_COMMON_OBJS) test_strings_main.o strings.o
test_vfs: $(TEST_COMMON_OBJS) test_vfs_main.o vfs.o vfsdebug.o strings.o

test_strings_main.c: test_strings.c
	./generate $< $@

test_vfs_main.c: test_vfs.c
	./generate $< $@

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
