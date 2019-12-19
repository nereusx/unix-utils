#
# note: ps2pdf produces better results

APPNAME := path++
INSTALL := /bin
MANDIR  := /usr/share/man/man1
CFLAGS  := -Os -Wall

all: $(APPNAME)

clean:
	-rm $(APPNAME) $(APPNAME).1 $(APPNAME).1.gz > /dev/null

$(APPNAME): pathpp.c
	$(CC) $(CFLAGS) pathpp.c -o $(APPNAME)

$(APPNAME).1.gz: pathpp.man
	cp pathpp.man $(APPNAME).1
	gzip -f $(APPNAME).1

html: pathpp.man
	groff pathpp.man -Thtml -man > $(APPNAME).html

pdf: pathpp.man
	groff pathpp.man -Tpdf -man -P -e > $(APPNAME).pdf

install: $(APPNAME) $(APPNAME).1.gz
	install -m 755 -o root -g root -s $(APPNAME) $(INSTALL)
	install -m 755 -o root -g root $(APPNAME).1.gz $(MANDIR)

uninstall:
	rm $(INSTALL)/$(APPNAME) $(MANDIR)/$(APPNAME).1.gz
