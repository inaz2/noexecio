CC = gcc
CFLAGS = -O2 -Wall -Werror
PREFIX = /usr/local

all: noexecio

clean:
	rm -f noexecio

install:
	install -m 755 noexecio $(PREFIX)/bin/noexecio

uninstall:
	rm -f $(PREFIX)/bin/noexecio

noexecio: noexecio.c
	$(CC) $(CFLAGS) -o $@ $<
