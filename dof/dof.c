/*
 * dof [file [file [...]] do command
 * if file = - then read from stdin
 * if command = - then read from stdin
 * -e = execute, default dof run in test mode (dryrun)
 * -f = force (no stop on error)
 * -p = regular files only
 * -d = directories only
 * %f = file
 * %b = basename
 * %d = dirname
 * %e = extension
 *
 * dof 1 2 3 do echo %f
 * dof *.txt do cp %f ${dir}/%b.%e \; chown root:root ${dir}/%b.%e
 *
 * Nicholas Christopoulos (nereus@freemail.gr)
 *
 * this software is released under GPLv3 or newer license
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

static list_t file_list;
static list_t cmds_list;

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
static void list_add(list_t *list, const char *str)
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

// initialize list
static void list_init(list_t *list)
{
	list->root = list->tail = NULL;
}

// clean up memory and reset the list
static void list_clear(list_t *list)
{
	node_t *pre, *cur = list->root;

	while ( cur ) {
		pre = cur;
		cur = cur->next;
		free(pre->str);
		free(pre);
		}
	list->root = list->tail = NULL;
}

// convert string list to string
static char *list_to_string(list_t *list, const char *delim)
{
	char	*ret;
	int		len = 0, delim_len = 0, first = 1;
	node_t	*cur = list->root;

	if ( delim )
		delim_len = strlen(delim);
	ret = (char *) malloc(1);
	*ret = '\0';
	while ( cur ) {
		len += delim_len + strlen(cur->str);
		ret = (char *) realloc(ret, len + 1);
		if ( first )
			first = 0;
		else if ( delim )
			strcat(ret, delim);
		strcat(ret, cur->str);
		cur = cur->next;
		}
	return ret;
}

// returns the name of the file without the directory and the extension
static char *basename(const char *source)
{
	static char buf[PATH_MAX];
	char		*b;
	const char	*p;

	if ( (p = strrchr(source, '/')) != NULL )
		strcpy(buf, p + 1);
	else
		strcpy(buf, source);
	if ( (b = (char *) strrchr(buf, '.')) != NULL )
		*b = '\0';
	return buf;
}

// returns the directory of the file without the trailing '/'
static char *dirname(const char *source)
{
	static char buf[PATH_MAX];
	const char	*p;

	if ( (p = strrchr(source, '/')) != NULL ) {
		strcpy(buf, source);
		buf[p - source] = '\0';
		}
	else
		buf[0] = '\0';
	return buf;
}

// returns the extension of the file without the '.'
static char *extname(const char *source)
{
	static char buf[PATH_MAX];
	char		*b;
	const char	*p;

	if ( (p = strrchr(source, '/')) != NULL )
		strcpy(buf, p + 1);
	else
		strcpy(buf, source);
	if ( (b = (char *) strrchr(buf, '.')) != NULL )
		return b + 1;
	else
		buf[0] = '\0';
	return buf;
}

// converts and returns the commands to run with 'system()'
// the buffer must be freed by the caller
static char *dof(const char *fmt, const char *data)
{
	const char *p, *v;
	char *d, *dest;
	int inside_sq = 0, count = 0, maxlen;

	// calculate a maximum length for the returned buffer
	p = fmt;
	while ( p ) {
		count ++;
		p = strchr(p + 1, '%');
		}
	maxlen = strlen(fmt) + strlen(data) * count + 1;

	// replace strings
	dest = (char *) malloc(maxlen);
	d = dest;
	p = fmt;
	while ( *p ) {

		// handle single-quoted parts
		if ( inside_sq ) {
			if ( *p == '\'' ) {
				inside_sq = 0;
				p ++;
				continue;
				}
			*d ++ = *p ++;
			continue;
			}
		if ( *p == '\'' ) {
			inside_sq = 1;
			p ++;
			continue;
			}

		// translate % expressions
		if ( *p == '%' ) {
			char block[LINE_MAX], *bp;
			
			p ++;
			switch ( *p ) {
			case 'f':		// full pathname
				v = data;
				while ( *v )	*d ++ = *v ++;
				break;
			case 'b':		// basename (no directory, no extension)
				v = basename(data);
				while ( *v )	*d ++ = *v ++;
				break;
			case 'd':		// directory name
				v = dirname(data);
				while ( *v )	*d ++ = *v ++;
				break;
			case 'e':		// extension
				v = extname(data);
				while ( *v )	*d ++ = *v ++;
				break;
			case '(':		// block
				p ++;
				bp = block;
				while ( *p ) {
					if ( *p == ')' )
						break;
					*bp ++ = *p ++;
					}
				*bp = '\0';
				break;			
			case '%':		// none
				*d ++ = *p;
				break;
			default:
				fprintf(stderr, "unknown element `%%%c'\n", *p);
				}
			}
		// not a command? just copy
		else
			*d ++ = *p;

		// next
		if ( *p )
			p ++;
		}

	// close destination string
	*d = '\0';

	return dest;
}

// --- main() ---

#define APP_DESCR \
"dof (do-for) run commands for each element of 'list'."

static const char *usage = "\
Usage: dof [list] do [commands]\n\
"APP_DESCR"\n\
\n\
Options:\n\
\t-e\texecute; dof displays what commands would be run, this option executes them.\n\
\t-f\tforce non-stop; dof stops on error, this option forces dof to ignore errors.\n\
\t-p\tplain files only; directories, devices, etc are ignored.\n\
\t-d\tdirectories only; plain files, devices, etc are ignored.\n\
\t-\tread from stdin\n\
\t-h\tthis screen\n\
\t-v\tversion and program information\n\
\n\
Variables:\n\
\t%f\tthe string (if file; the full path name)\n\
\t%b\tthe basename (no directory, no extension)\n\
\t%d\tthe directory (without trailing '/')\n\
\t%e\tthe extension (without '.')\n\
";

static const char *verss = "\
dof version 1.00\n\
"APP_DESCR"\n\
\n\
Copyright (C) 2017 Free Software Foundation, Inc.\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
Written by Nicholas Christopoulos <mailto:nereus@freemail.gr>\n\
";

// return a pointer to filename without the directory
const char *namep(const char *file)
{
	const char *p;
	if ( (p = strrchr(file, '/')) != NULL )
		return p + 1;
	return file;
}

//
int main(int argc, char **argv)
{
	int		flags = 0, state = 0, exit_status = 0;
	node_t	*cur;
	char	*cmds, *cmdbuf;
	struct stat st;

	list_init(&file_list);
	list_init(&cmds_list);
	for ( int i = 1; i < argc; i ++ ) {
		if ( argv[i][0] == '-' && state == 0 ) {

			// one minus, read from stdin
			if ( argv[i][1] == '\0' ) {
				char buf[LINE_MAX];

				while ( fgets(buf, LINE_MAX, stdin) )	{
					if ( state == 0 ) {
						if (
							strcmp(namep(argv[i]), ".") == 0 ||
							strcmp(namep(argv[i]), "..") == 0
							)
							; // ignore them
						else
							list_add(&file_list, buf);
						}
					else
						list_add(&cmds_list, buf);
					}
				}

			// check options
			for ( int j = 1; argv[i][j]; j ++ ) {
				switch ( argv[i][j] ) {
				case 'e': flags |= 0x01; break;
				case 'f': flags |= 0x02; break;
				case 'p': flags |= 0x04; break;
				case 'd': flags |= 0x08; break;
				case 'h': puts(usage); return 1;
				case 'v': puts(verss); return 1;
				default:  puts("unknown option."); return 1;
					}
				}
			}
		else if ( strcmp(argv[i], "do") == 0 && state == 0 )
			state ++;
		else {
			if ( state == 0 ) {
				if (
					strcmp(namep(argv[i]), ".") == 0 ||
					strcmp(namep(argv[i]), "..") == 0
					)
					; // ignore them
				else
					list_add(&file_list, argv[i]);
				}
			else
				list_add(&cmds_list, argv[i]);
			}
		}

	if ( state == 0 ) { // syntax error
		puts("`do' keyword missing; run `dof -h' for help.");
		return 1;
		}

	// execute
	cmds = list_to_string(&cmds_list, " ");
	cur = file_list.root;
	while ( cur ) {
		if ( flags & 0x04 || flags & 0x08 ) { // file attribute check
			if ( stat(cur->str, &st) == 0 ) {
				if ( flags & 0x04 ) { // plain files only
					if ( ! S_ISREG(st.st_mode) ) {
						cur = cur->next;
						continue;
						}
					}
				if ( flags & 0x08 ) { // directories only
					if ( ! S_ISDIR(st.st_mode) ) {
						cur = cur->next;
						continue;
						}
					}
				}
			}
		cmdbuf = dof(cmds, cur->str);
		if ( (flags & 0x01) == 0 ) // not execute-option
			fprintf(stdout, "%s\n", cmdbuf);
		else {
			if ( (exit_status = system(cmdbuf)) != 0 ) {
				if ( (flags & 0x02) == 0 ) { // not force-option
					free(cmdbuf);
					break;
					}
				}
			}
		free(cmdbuf);
		cur = cur->next;
		}

	// cleanup
	list_clear(&file_list);
	list_clear(&cmds_list);
	return exit_status;
}
