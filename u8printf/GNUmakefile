#
# note: ps2pdf produces better results

APPNAME := u8printf
INSTALL := /usr/bin
MANDIR  := /usr/share/man/man1
CFLAGS  := -Os -Wall

all: $(APPNAME)

clean:
	-rm $(APPNAME) $(APPNAME).1 $(APPNAME).1.gz > /dev/null

$(APPNAME): $(APPNAME).c
	$(CC) $(CFLAGS) $(APPNAME).c -o $(APPNAME)

$(APPNAME).1.gz: $(APPNAME).man
	cp $(APPNAME).man $(APPNAME).1
	gzip -f $(APPNAME).1

html: $(APPNAME).man
	groff $(APPNAME).man -Thtml -man > $(APPNAME).html

pdf: $(APPNAME).man
	groff $(APPNAME).man -Tpdf -man -P -e > $(APPNAME).pdf

install: $(APPNAME) $(APPNAME).1.gz
	install -m 755 -o root -g root -s $(APPNAME) $(INSTALL)
	install -m 644 -o root -g root $(APPNAME).1.gz $(MANDIR)

uninstall:
	rm $(INSTALL)/$(APPNAME) $(MANDIR)/$(APPNAME).1.gz
