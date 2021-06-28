#!/bin/sh
dirs="pathpp dof isonline hd"
for d in $dirs; do
	cd $d
	make clean
	cd ..
done
