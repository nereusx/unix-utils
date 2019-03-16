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

html: path++.1
	groff path++.1 -Thtml -man > path++.1.html

pdf:
	groff path++.1 -Tpdf -man > path++.1.pdf

install: path++
	install --mode=755 --owner=root --group=root -s path++ $(INSTALL)
	install --mode=755 --owner=root --group=root path++.1 $(MANDIR)

