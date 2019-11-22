# unix-utils

My tiny unix utilities, use 'gcc-musl' for better results.

* [path++ man page](https://github.com/nereusx/unix-utils/blob/master/pathpp/path%2B%2B.pdf):
A utility to cleanup the path from duplicated or non-exists directories.

* [dof man page](https://github.com/nereusx/unix-utils/blob/master/dof/dof.pdf) (do-for):
A utility to run commands for each file/item of a list.
This is useful on c-shell where you cannot use one-line `foreach` statements.

* [isonline man page](https://github.com/nereusx/unix-utils/blob/master/isonline/isonline.pdf):
A utility to return 0 (which means OK) when the internet connection is up and running.

## installation

go to program's directory and
use `make && make install`
