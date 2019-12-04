CC=gcc
CFLAGS=
MANDIR=$(shell test -d /usr/local/share/man && echo /usr/local/share/man || echo /usr/local/man)

all: isonline

isonline: isonline.c
	$(CC) $(CFLAGS) isonline.c -o isonline

clean:
	-rm isonline isonline.1.gz

isonline.1.gz: isonline.man
	cp isonline.man isonline.1
	gzip -f isonline.1

html: isonline.man
	groff isonline.man -Thtml -man > isonline.html

pdf: isonline.man
	groff isonline.man -Tps -man | ps2pdf -dPDFSETTINGS=/screen - isonline.pdf
#	groff isonline.man -Tpdf -man > isonline.pdf

install: isonline isonline.1.gz
	install -m 755 -o root -g root isonline /usr/local/bin
	install -m 755 -o root -g root isonline.1.gz $(MANDIR)/man1

uninstall:
	-@rm /usr/local/bin/isonline $(MANDIR)/man1/isonline.1.gz
