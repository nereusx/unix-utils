# unix-utils

My tiny Unix utilities in C

---

* [path++ man page](https://github.com/nereusx/unix-utils/blob/master/pathpp/path%2B%2B.pdf):
A utility to cleanup the path from duplicated or non-existing directories.

```
# clean up path
export PATH=$(path++)
	
# make your path as you wish without mess it up
NEWPATH=/bin:/usr/bin:/usr/local/bin:$PATH
export PATH=$(path++ $NEWPATH)
```
---

* [dof man page](https://github.com/nereusx/unix-utils/blob/master/dof/dof.pdf) (do-for):
A utility to run commands for each file/item of a list.
I wrote this for C-Shell where you cannot use one-line `foreach` statements; but it is powerful with regex and you can use instead of `for`, `find` and more, even `ls`.


```
# convert all mp3s to ogg
dof -e *.mp3 do ffmpeg -i "%f" "%b.ogg"
	
# display colors on terminal
dof -e -s 0..15 do 'tput setaf %f; echo colour%f; tput op;'
	
# remove all files except *.c and *.tex
dof -e * -x *.{c,tex} do 'rm %f'
```
---

* [isonline man page](https://github.com/nereusx/unix-utils/blob/master/isonline/isonline.pdf):
A utility to return 0 (which means OK) when the internet connection is up and running.

```
$ isonline
yes
$ isonline www.example.com
no
```
---

## installation

**All:** just clone the whole directory, jump into it, and run `./install.sh`
```
git clone https://github.com/nereusx/unix-utils
cd unix-utils
./install.sh
```

* [hd man page](https://github.com/nereusx/unix-utils/blob/master/hd/hd.pdf):
Hex-dump

**Just one:** go to program's directory and type `make && make install`
