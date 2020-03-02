#!/bin/sh
cc pathpp.c -o path++
cp path++ $PREFIX/bin
cp pathpp.man path++.1
gzip path++.1
cp path++.1.gz $PREFIX/share/man/man1/
rm path++ path++.1.gz
