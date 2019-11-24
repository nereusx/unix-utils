#!/bin/sh
dirs="pathpp dof isonline files"
for d in $dirs; do
	cd $d
	make && make install
	cd ..
done
