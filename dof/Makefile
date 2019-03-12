#

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

install: path++
	install --mode=755 --owner=root --group=root -s dof $(INSTALL)

