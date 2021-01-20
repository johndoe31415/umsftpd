.PHONY: test pgmopts install vfsshell tests

CFLAGS := -O3 -std=c11 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=500 -D_DEFAULT_SOURCE -march=native
CFLAGS += -Wall -Wmissing-prototypes -Wstrict-prototypes -Werror=implicit-function-declaration -Wno-stringop-truncation -Werror=format -Wno-stringop-truncation -Wshadow -Wswitch -pthread
CFLAGS += -DDEBUG -ggdb3 -pie -fPIE -fsanitize=address -fsanitize=undefined -fsanitize=leak
CFLAGS += -DWITH_SERVER

CFLAGS += `pkg-config --cflags libssh` `pkg-config --cflags openssl`
LDFLAGS += `pkg-config --libs libssh` `pkg-config --libs openssl`

OBJS := \
	logging.o \
	main.o \
	passdb.o \
	rfc4648.o \
	rfc6238.o \
	stringlist.o \
	strings.o \
	vfs.o

BINARIES := umsftpd vfsshell

all: umsftpd

tests:
	make -C tests tests

umsftpd: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

vfsshell: vfs.c stringlist.c strings.c vfsdebug.c logging.c
	$(CC) $(CFLAGS) -D__VFS_SHELL__ -o $@ $^ $(LDFLAGS)
	./vfsshell

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
