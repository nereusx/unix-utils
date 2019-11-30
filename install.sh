#!/bin/sh
dirs="pathpp dof isonline"
for d in $dirs; do
	cd $d
	make && make install
	cd ..
done
