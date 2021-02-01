/*
 *	path++ check's directories (if exist, if is_dir, if !duplicate) and/or
 *	add directories to a path.
 *
 *	Copyright (C) 2017-2021 Nicholas Christopoulos.
 *
 *	This is free software: you can redistribute it and/or modify it under
 *	the terms of the GNU General Public License as published by the
 *	Free Software Foundation, either version 3 of the License, or (at your
 *	option) any later version.
 *
 *	It is distributed in the hope that it will be useful, but WITHOUT ANY
 *	WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *	for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with it. If not, see <http://www.gnu.org/licenses/>.
 *
 * 	Written by Nicholas Christopoulos <nereus@freemail.gr>
 * 
 *	usage: setenv PATH `path++ [[new-dir] ...]`
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef struct s_node node_t;
struct s_node {
	char *str;
	node_t *next;
	};

typedef struct s_list list_t;
struct s_list {
	node_t *root;
	node_t *tail;
	};

static list_t path_list;
static list_t new_dirs;

#ifdef STRDUP
// clone a string
static char *strdup(const char *src)
{
	char *buf = malloc(strlen(src) + 1);
    strcpy(buf, src);
	return buf;
}
#endif

// add node to list
static void node_add(list_t *list, const char *str)
{
	node_t *np = (node_t *) malloc(sizeof(node_t));
	np->str = strdup(str);
	np->next = NULL;
	if ( list->root ) {
		list->tail->next = np;
		list->tail = np;
		}
	else
		list->root = list->tail = np;
}

// add directory to list
static void dir_add(list_t *root, const char *dir)
{
	struct stat st;
	
	if ( stat(dir, &st) == 0 ) { // if directory exists
		if ( S_ISDIR(st.st_mode) ) { // if directory is directory (and not a file)
			const node_t *cur;
			for ( cur = path_list.root; cur; cur = cur->next ) {
				if ( strcmp(cur->str, dir) == 0 )
					break;
				}
			if ( cur == NULL ) // if directory is not already in the list
				node_add(&path_list, dir); // then add it
			}
		}
}

// add a list of directories to the list
static void list_add(list_t *root, const list_t *source)
{
	for ( const node_t *cur = source->root; cur; cur = cur->next )
		dir_add(root, cur->str);
}

// clear memory
static void delete_list(list_t *list)
{
	node_t *cur = list->root, *pre;
	while ( cur ) {
		pre = cur;
		cur = cur->next;
		free(pre->str);
		free(pre);
		}
}

// creates a $PATH string
static char *duppath()
{
	const node_t *cur;
	int l = 1;
	char *buf;
	
	for ( cur = path_list.root; cur; cur = cur->next )
		l += strlen(cur->str) + 1;
	buf = (char *) malloc(l);
	*buf = '\0';
	for ( cur = path_list.root; cur; cur = cur->next ) {
		strcat(buf, cur->str);
		if ( cur->next )
			strcat(buf, ":");
		}
	return buf;
}

#define APP_DESCR \
"Check directories of path removes non-existing, duplicated, and add directories\
 to system's PATH.\
 With few words, cleanups the PATH (or any PATH-like environment variable, see -e)."

static const char *usage = "\
Usage (csh): setenv PATH `path++ [-s|-u|-c|-b] [-e var] [dir ...]`\n\
OR   (bash): export PATH=$(path++ [-s|-u|-c|-b] [-e var] [dir ...])\n\
\n"APP_DESCR"\n\
Options:\n\
\t-e\tselect path-variable (PATH,MANPATH,etc), default is PATH\n\
\t-s\tadd new directories at the beginning; otherwise appends\n\
\t-b\tprint sh command\n\
\t-c\tprint csh command\n\
\t-h\tthis screen\n\
\t-v\tversion and program information\n\
\n\
\twith no arguments it just prints the corrected path.\n\
";

static const char *verss = "\
path++ version 1.3\n\
\n"APP_DESCR"\n\
\n\
Copyright (C) 2017-2019 Free Software Foundation, Inc.\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
Written by Nicholas Christopoulos <mailto:nereus@freemail.gr>\n\
";

int main(int argc, char **argv)
{
	int		flags = 0, mode = 0;
	const char *p, *cspath;
	char *d, *dest, *vname = NULL;

	for ( int i = 1; i < argc; i ++ ) {
		if ( argv[i][0] == '-' ) {	/* option, or error */
			for ( int j = 1; argv[i][j]; j ++ ) {
				switch ( argv[i][j] ) {
				case 'c': flags |= 0x02; break;
				case 'b': flags |= 0x04; break;
				case 'e': mode = 1;
				case 's': flags |= 0x08; break;
				case 'h': puts(usage); return 1;
				case 'v': puts(verss); return 1;
				default:  puts(usage); return 1;
					}
				}
			}
		else {
			switch ( mode )	{
			case 1: // this is the variable name
				vname = strdup(argv[i]);
				mode = 0;
				break;
			default:
				if ( strchr(argv[i], ':') ) { // bash format
					char *dest = strdup(argv[i]), *st = dest;
					char *p = dest;
					while ( *p ) {
						if ( *p == ':' ) {
							*p = '\0';
							if ( strlen(st) )
								node_add(&new_dirs, st);
							st = p + 1;
							}
						p ++;
						}
					if ( strlen(st) )
						node_add(&new_dirs, st);
					free(dest);
					}
				else
					node_add(&new_dirs, argv[i]);
				}
			}
		}

	// default variable = PATH
	if ( !vname )
		vname = strdup("PATH");

	// if not exist, create it
	cspath = getenv(vname);
	if ( !cspath ) {
		setenv(vname, "", 1);
		cspath = getenv(vname);
		}
	dest = strdup(cspath);

	//
	if ( new_dirs.root && (flags & 0x08) )
		list_add(&path_list, &new_dirs);

	// for each directory in path
	for ( p = cspath, d = dest; *p; p ++ ) {
		if ( *p == ':' ) {
			*d = '\0'; d = dest;
			if ( *dest )
				dir_add(&path_list, dest);
			}
		else
			*d ++ = *p;
		}
	*d = '\0';
	if ( *dest ) 
		dir_add(&path_list, dest);
	
	if ( new_dirs.root && (flags & 0x08) == 0 )
		list_add(&path_list, &new_dirs);

	// recreate path
	char *pathstr = duppath();

	/* ready... */
	if ( flags & 0x02 )	/* return c-shell text for eval */
		printf("setenv %s \'%s\'", vname, pathstr);
	else if ( flags & 0x04 )	/* return posix-shell text for eval */
		printf("export %s=\'%s\'", vname, pathstr);
	else
		puts(pathstr);

	/* clean-up */
	free(pathstr);
	free(vname);
	delete_list(&new_dirs);
	delete_list(&path_list);
	
	return 0;
}
