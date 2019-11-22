CC=gcc
CFLAGS=

all: isonline

isonline: isonline.c
	$(CC) $(CFLAGS) isonline.c -o isonline

clean:
	-rm isonline

install:
	install -m 0755 isonline /usr/local/bin
