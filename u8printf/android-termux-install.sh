#!/bin/sh
cc u8printf.c -o u8printf
cp u8printf $PREFIX/bin
cp u8printf.man u8printf.1
gzip u8printf.1
cp u8printf.1.gz $PREFIX/share/man/man1/
rm u8printf u8printf.1.gz
