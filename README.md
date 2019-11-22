# unix-utils

My tiny unix utilities, use 'gcc-musl' for better results.

* dof (do-for)
A utility to run commands for each file/item of a list.
This is useful on c-shell where you cannot use one-line `foreach` statements.

[dof man page](https://github.com/nereusx/unix-utils/blob/master/dof/dof.pdf)

* path++
A utility to cleanup the path from duplicated or non-exists directories.

[path++ man page](https://github.com/nereusx/unix-utils/blob/master/pathpp/path%2B%2B.pdf)

* isonline
A utility to return 0 (which means OK) when the internet connection is up and running.

[isonline man page](https://github.com/nereusx/unix-utils/blob/master/isonline/isonline.pdf)

## installation

go to program's directory and
use `make && make install`
