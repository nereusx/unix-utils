#!/bin/sh
cc isonline.c -o isonline
cp isonline $PREFIX/bin
cp isonline.man isonline.1
gzip isonline.1
cp isonline.1.gz $PREFIX/share/man/man1/
rm isonline isonline.1.gz
