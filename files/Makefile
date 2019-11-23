CC=gcc
CFLAGS=
MANDIR=/usr/local/share/man/man1

all: files

rmx: files.c
	$(CC) $(CFLAGS) files.c -o files

clean:
	-rm files files.1.gz

files.1.gz: files.man
	cp files.man files.1
	gzip -f files.1

html: files.man
	groff files.man -Thtml -man > files.html

pdf: files.man
	groff files.man -Tpdf -man > files.pdf

install: files files.1.gz
	install -m 755 -o root -g root files /usr/local/bin
	install -m 755 -o root -g root files.1.gz $(MANDIR)

uninstall:
	rm /usr/local/bin/files $(MANDIR)/files.1.gz
