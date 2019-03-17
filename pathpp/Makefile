#

APPNAME := path++
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

all: $(APPNAME)

clean:
	rm $(APPNAME) $(APPNAME).1

$(APPNAME): pathpp.c
	$(CC) $(CFLAGS) pathpp.c -o $(APPNAME)

$(APPNAME).1: pathpp.man
	cp pathpp.man $(APPNAME).1

html: $(APPNAME).1
	groff $(APPNAME).1 -Thtml -man > $(APPNAME).1.html

pdf: $(APPNAME).1
	groff $(APPNAME).1 -Tpdf -man > $(APPNAME).1.pdf

install: $(APPNAME) $(APPNAME).1
	install --mode=755 --owner=root --group=root -s $(APPNAME) $(INSTALL)
	install --mode=755 --owner=root --group=root $(APPNAME).1 $(MANDIR)

