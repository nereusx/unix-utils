#!/bin/sh
cc dof.c list.c str.c panic.c file.c -o dof
cp dof $PREFIX/bin
cp dof.man dof.1
gzip dof.1
cp dof.1.gz $PREFIX/share/man/man1/
rm dof.1 dof.1.gz
