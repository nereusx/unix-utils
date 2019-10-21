#!/bin/sh
dirs="dof pathpp"
for d in $dirs; do
	cd $d
	make && make install
	cd ..
done
