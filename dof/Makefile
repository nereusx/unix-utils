#

MANDIR  := /usr/share/man/man1
INSTALL := /usr/bin
CFLAGS  := -O -Wall -Wno-unused-result -static

# select compiler
MUSLCC  := $(shell which musl-gcc)
ifeq "$(MUSLCC)" ""
	CC=gcc
else
	CC=musl-gcc
endif

all: dof

clean:
	rm dof

dof: dof.c
	$(CC) $(CFLAGS) dof.c -o dof

install: dof
	install --mode=755 --owner=root --group=root -s dof $(INSTALL)
	install --mode=755 --owner=root --group=root dof.1 $(MANDIR)

