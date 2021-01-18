#
# note: ps2pdf produces better results

APPNAME := hd
INSTALL := /bin
MANDIR  := /usr/share/man/man1
CFLAGS  := -Os -Wall -static

all: $(APPNAME)

clean:
	-rm $(APPNAME) $(APPNAME).1 $(APPNAME).1.gz > /dev/null

$(APPNAME): hd.c
	$(CC) $(CFLAGS) hd.c -o $(APPNAME)

$(APPNAME).1.gz: hd.man
	cp hd.man $(APPNAME).1
	gzip -f $(APPNAME).1

html: hd.man
	groff hd.man -Thtml -man > $(APPNAME).html

pdf: hd.man
	groff hd.man -Tpdf -man -P -e > $(APPNAME).pdf

install: $(APPNAME) $(APPNAME).1.gz
	install -m 755 -o root -g root -s $(APPNAME) $(INSTALL)
	install -m 644 -o root -g root $(APPNAME).1.gz $(MANDIR)

uninstall:
	rm $(INSTALL)/$(APPNAME) $(MANDIR)/$(APPNAME).1.gz

