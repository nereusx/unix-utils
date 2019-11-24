#

MANDIR  := /usr/share/man/man1
INSTALL := /usr/bin
CFLAGS  := -O -Wall -Wno-unused-result -static

all: dof

clean:
	-rm dof dof.1 dof.1.gz

dof: dof.c
	$(CC) $(CFLAGS) dof.c -o dof

dof.1.gz: dof.man
	cp dof.man dof.1
	gzip -f dof.1

html: dof.man
	groff dof.man -Thtml -man > dof.html

pdf: dof.man
	groff dof.man -Tpdf -man > dof.pdf

install: dof dof.1.gz
	install -m 755 -o root -g root -s dof $(INSTALL)
	install -m 755 -o root -g root dof.1.gz $(MANDIR)

uninstall:
	rm $(INSTALL)/dof $(MANDIR)/dof.1.gz

