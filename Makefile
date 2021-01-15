.PHONY: test pgmopts install vfstest tests

CFLAGS := -O3 -std=c11 -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=500 -march=native
CFLAGS += -Wall -Wmissing-prototypes -Wstrict-prototypes -Werror=implicit-function-declaration -Werror=format -Wshadow -Wswitch -pthread
CFLAGS += -DDEBUG -ggdb3 -pie -fPIE -fsanitize=address -fsanitize=undefined -fsanitize=leak
CFLAGS += -DWITH_SERVER

CFLAGS += `pkg-config --cflags libssh`
LDFLAGS += `pkg-config --libs libssh`

OBJS := \
	main.o \
	logging.o \
	vfs.o \
	stringlist.o \
	strings.o

BINARIES := umsftpd vfstest

all: umsftpd

tests:
	make -C tests test

umsftpd: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

vfstest: vfs.c stringlist.c strings.c vfsdebug.c
	$(CC) $(CFLAGS) -D__VFS_TEST__ -o $@ $^ $(LDFLAGS)
	./vfstest

clean:
	rm -f $(OBJS)
	rm -f $(BINARIES)
	make -C tests clean

install: all
	strip umsftpd
	cp umsftpd /usr/local/bin
	chown root:root /usr/local/bin/umsftpd
	chmod 755 /usr/local/bin/umsftpd

pgmopts:
	../Python/pypgmopts/pypgmopts parser.py

test: all
	./umsftpd -vv

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
