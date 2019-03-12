#

INSTALL := /bin
MANDIR  := /usr/share/man/man1
CFLAGS  := -O -Wall -Wno-unused-result -static

# select compiler
MUSLCC  := $(shell which musl-gcc)
ifeq "$(MUSLCC)" ""
	CC=gcc
else
	CC=musl-gcc
endif

all: path++

clean:
	rm path++

path++: pathpp.c
	$(CC) $(CFLAGS) pathpp.c -o path++

install: path++
	install --mode=755 --owner=root --group=root -s path++ $(INSTALL)
	install --mode=755 --owner=root --group=root path++.1 $(MANDIR)

