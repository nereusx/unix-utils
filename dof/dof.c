/*
 * dof [file [file [...]] do command
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
#include <glob.h>
#include <dirent.h>
#include <regex.h>

typedef struct s_node node_t;
struct s_node {
	char *str;
	char *data;
	node_t *next;
	};

typedef struct s_list list_t;
struct s_list {
	node_t *root;
	node_t *tail;
	};

static list_t file_list;
static list_t cmds_list;
static list_t rcpt_list;
static list_t patt_list;
static list_t excl_list;

static int		re_count = 0;
static regex_t	**re_table;

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
// this just stores a string in the list
static void list_add(list_t *list, const char *str)
{
	node_t *np = (node_t *) malloc(sizeof(node_t));
	np->str  = strdup(str);
	np->data = NULL;
	np->next = NULL;
	if ( list->root ) {
		list->tail->next = np;
		list->tail = np;
		}
	else
		list->root = list->tail = np;
}

// add node to list
// this stores a key and a value to the list
static void list_addpair(list_t *list, const char *key, const char *value)
{
	node_t *np = (node_t *) malloc(sizeof(node_t));
	np->str  = strdup(key);
	np->data = strdup(value);
	np->next = NULL;
	if ( list->root ) {
		list->tail->next = np;
		list->tail = np;
		}
	else
		list->root = list->tail = np;
}

// add node to list
// this stores wildcard matches
static void list_addwc(list_t *list, const char *pattern)
{
	glob_t globbuf;
	int flags = GLOB_DOOFFS;
	#ifdef GLOB_TILDE
	flags |= GLOB_TILDE;
	#endif
	#ifdef GLOB_BRACE
	flags |= GLOB_BRACE;
	#endif

	globbuf.gl_offs = 0;
	if ( glob(pattern, flags, NULL, &globbuf) == 0 ) {
		for ( int i = 0; globbuf.gl_pathv[i]; i ++ ) {
			const char *name = globbuf.gl_pathv[i];
			if ( ! (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) )
				list_add(list, name);
			}
		globfree(&globbuf);
		}
}

// returns true if the "filename" has wildcards
static int has_wildcards(const char *filename)
{
	const char *p = filename;
	
	while ( *p ) {
		if ( *p == '*' || *p == '?' || *p == '['
	#ifdef GLOB_TILDE
			 || *p == '~'
	#endif
	#ifdef GLOB_BRACE
			 || *p == '{'
	#endif
		   )
			return 1;
		p ++;
		}
	return 0;
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

//
static int match_regex(regex_t *r, const char *to_match)
{
	const char *p = to_match;
	const int n_matches = 1;
	regmatch_t m[n_matches];

	int nomatch = regexec(r, p, n_matches, m, 0);
	if ( nomatch )
		return 0;
	return 1;
}

//
static const char *exec_expr(const char *source, const char *data)
{
	static char buf[LINE_MAX];
	char args[LINE_MAX], *tp, *ap;
	const char *p = source, *next;
	
	buf[0] = '\0';
	switch ( *p ) {
	case 'f':		// full pathname
		strcpy(buf, data);
		break;
	case 'b':		// basename (no directory, no extension)
		strcpy(buf, basename(data));
		break;
	case 'd':		// directory name
		strcpy(buf, dirname(data));
		break;
	case 'e':		// extension
		strcpy(buf, extname(data));
		break;
	case '%':		// none
		strcpy(buf, "%");
		break;
	case '\'':
	case 'q':
		strcpy(buf, "'");
		break;
	case '\"':
		strcpy(buf, "\"");
		break;
	default:
		fprintf(stderr, "unknown element `%%%c'\n", *p);
		return buf;
		}
	
	// modifiers
	p ++;
	while ( p && *p == ':' ) {
		p ++;
		next = strchr(p, ':');
		if ( next )	{
			strcpy(args, p+1);
			tp = strchr(args, ':');
			if ( tp )
				*tp = '\0';
			}
		else
			strcpy(args, p+1);
		ap = args;
			
		switch ( *p ) {
		case 'l':
			while ( *ap ) {
				tp = strchr(buf, *ap);
				if ( tp )
					*tp = '\0';
				ap ++;
				}
			break;
		case 'r':
			while ( *ap ) {
				tp = strrchr(buf, *ap);
				if ( tp )
					*tp = '\0';
				ap ++;
				}
			break;
		case 'i':
			tp = strstr(buf, args);
			if ( tp )
				*tp = '\0';
			break;
			};
			
		p = next;
		}

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
			char mark = ' ';
			
			p ++;
			if ( *p == '{' || *p == '(' ) {
				mark = *p;			
				if ( mark == '{' )
					{ mark = '}'; p ++; }
				else if ( mark == '(' )
					{ mark = ')'; p ++; }
				else 
					mark = ' ';
	
				bp = block;
				while ( *p ) {
					if ( *p == mark )
						break;
					*bp ++ = *p ++;
					if ( mark == ' ' ) // patch for one-char
						break;
					}
				*bp = '\0';
				v = exec_expr(block, data);
				while ( *v )	*d ++ = *v ++;
				}
			else {
				block[0] = *p;
				block[1] = '\0';
				v = exec_expr(block, data);
				while ( *v )	*d ++ = *v ++;
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

// read configuration files
static void	read_conf()
{
	FILE	*fp;
	char	buf[LINE_MAX], *p, *key, *data;
	char	file[PATH_MAX];
	static	int readconf_init = 0;

	if ( readconf_init ) return;
	readconf_init = 1;
	
	for ( int i = 0; i < 2; i ++ ) {
		switch ( i ) {
		case 0: strcpy(file, "/etc/dof.conf"); break;
		case 1: strcpy(file, getenv("HOME")); strcat(file, "/.dof"); break;
			}
		if ( (fp = fopen(file, "rt")) != NULL ) {
			while ( fgets(buf, LINE_MAX, fp) ) {
				p = buf;
				while ( *p == ' ' || *p == '\t' ) p ++;
				if ( *p == '\n' || *p == '\0' || *p == '#' )
					continue;
				key = p;
				while ( *p ) {
					if ( *p == ':' )
						break;
					p ++;
					}
				if ( *p == ':' ) {
					*p = '\0';
					p ++;
					while ( *p == ' ' || *p == '\t' ) p ++;
					data = p;
					if ( data[strlen(data)-1] == '\n' )
						data[strlen(data)-1] = '\0';
					list_addpair(&rcpt_list, key, data);
					}
				}
			fclose(fp);
			}
		}
}

// --- main() ---

#define APP_DESCR \
"dof (do-for) run commands for each element of 'list'."

#define APP_VER "1.4"

static const char *usage = "\
Usage: dof [list] [-x patterns] do [commands]\n\
"APP_DESCR"\n\
\n\
Options:\n\
\t-e\texecute; dof displays what commands would be run, this option executes them.\n\
\t-x\texclude regex patterns; the excluded list always has priority.\n\
\t-r\trecursive execution of commands into sub-directories.\n\
\t-f\tforce non-stop; dof stops on error, this option forces dof to ignore errors.\n\
\t-p\tplain files only; directories, devices, etc are ignored.\n\
\t-d\tdirectories only; plain files, devices, etc are ignored.\n\
\t-l\tprint recipes (/etc/dof.conf, ~/.dof)\n\
\t--<recipe>\n\t\texecute recipe (ex: dof --to-ogg)\n\
\t-\tread from stdin\n\
\t-h\tthis screen\n\
\t-v\tversion and program information\n\
\n\
Variables:\n\
\t%f\tthe string (if file; the full path name)\n\
\t%b\tthe basename (no directory, no extension)\n\
\t%d\tthe directory (without trailing '/')\n\
\t%e\tthe extension (without '.')\n\
\n\
Modifiers:\n\
modifiers defined by ':' that follows a variable and modifies the result string. You can have unlimited number of modifiers.\n\
\tlc\treturns the string until the the first occurence of 'c'\n\
\trc\treturns the string until the last occurence of 'c'\n\
\tis\treturns the string until the occurence of string 's'\n\
";

static const char *verss = "\
dof version "APP_VER"\n\
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
static const char *namep(const char *file)
{
	const char *p;
	if ( (p = strrchr(file, '/')) != NULL )
		return p + 1;
	return file;
}

//
static int pass_exclude_list(const char *src)
{
	int	i;
	
	for ( i = 0; i < re_count; i ++ ) {
		if ( match_regex(re_table[i], src) )
			return 0;
		}
	return 1;
}

// execute
int execute(int flags)
{
	char	*cmds, *cmdbuf;
	node_t	*cur;
	struct stat st;
	int exit_status = 0;
	
	cmds = list_to_string(&cmds_list, " ");

	// select files
	list_clear(&file_list);
	cur = patt_list.root;
	while ( cur ) {
		if ( has_wildcards(cur->str) )
			list_addwc(&file_list, cur->str);
		else
			list_add(&file_list, cur->str);
		cur = cur->next;
		}

	// for each file
	cur = file_list.root;
	while ( cur ) {

		// check file attributes
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

		if ( pass_exclude_list(cur->str) )	{
			// execute
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
			}
		cur = cur->next;
		}
	
	return exit_status;
}

// execute the commands for each subdirectory
int execute_indir(int flags)
{
	struct dirent *p_dirent;
	DIR	*p_dir;
	int exit_status = 0;
	struct stat st;
		
	if ( (exit_status = execute(flags)) != 0 ) {
		if ( (flags & 0x02) == 0 ) // not force-option
			return exit_status;
		}
	if ( (p_dir = opendir(".")) == NULL )
		return exit_status;
	while ( (p_dirent = readdir(p_dir)) != NULL ) {
		if ( strcmp(p_dirent->d_name, ".") == 0 || strcmp(p_dirent->d_name, "..") == 0 )
			continue;
		if ( stat(p_dirent->d_name, &st) == 0 ) {
			if ( S_ISDIR(st.st_mode) ) {
				if ( pass_exclude_list(p_dirent->d_name) ) {
					if ( chdir(p_dirent->d_name) == 0 ) {
						exit_status = execute_indir(flags);
						chdir("..");
						if ( exit_status && (flags & 0x02) == 0 ) // not force-option
							break;
						}
					else
						fprintf(stderr, "change dir to %s failed.\n", p_dirent->d_name);					
					}
				}
			}
		}
	closedir(p_dir);
	return exit_status;
}

//
int main(int argc, char **argv)
{
	int		flags = 0, state = 0, exit_status = 0;
	int		i;
	node_t	*cur;

	list_init(&file_list);
	list_init(&cmds_list);
	list_init(&rcpt_list);
	list_init(&patt_list);
	list_init(&excl_list);

	for ( i = 1; i < argc; i ++ ) {
		if ( argv[i][0] == '-' && (state == 0 || state == 2) ) {

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
					else if ( state == 1 ) 
						list_add(&cmds_list, buf);
					else if ( state == 2 ) 
						list_add(&excl_list, buf);
					}
				}

			// check options
			for ( int j = 1; argv[i][j]; j ++ ) {
				switch ( argv[i][j] ) {
				case 'e': flags |= 0x01; break;
				case 'f': flags |= 0x02; break;
				case 'p': flags |= 0x04; break;
				case 'd': flags |= 0x08; break;
				case 'r': flags |= 0x10; break;
				case 'x': state = 2; break;
				case 'h': puts(usage); return 1;
				case 'v': puts(verss); return 1;
				case '-':
						// execute recipe
						read_conf();
						cur = rcpt_list.root;
						while ( cur ) {
							if ( strcmp(cur->str, argv[i]+2) == 0 ) {
								char cmd[LINE_MAX];
								char opt[64];
								opt[0] = '\0';
								if ( flags & 0x01 ) strcat(opt, "-e ");
								if ( flags & 0x02 ) strcat(opt, "-f ");
								if ( flags & 0x04 ) strcat(opt, "-p ");
								if ( flags & 0x08 ) strcat(opt, "-d ");
								if ( flags & 0x10 ) strcat(opt, "-r ");
								snprintf(cmd, LINE_MAX, "dof %s %s", opt, cur->data);
								return system(cmd);
								}
							cur = cur->next;
							}
						return 1;
				case 'l':
						// list recipes
						read_conf();
						cur = rcpt_list.root;
						while ( cur ) {
							printf("%s: (%s)\n", cur->str, cur->data);
							cur = cur->next;
							}
						return 0;
				default:
					// error
					puts("unknown option.");
					return 1;
					}
				}
			}
		else if ( strcmp(argv[i], "do") == 0 && (state == 0 || state == 2) )
			state = 1;
		else {
			switch ( state ) {
			case 0:
				if (
					strcmp(namep(argv[i]), ".") == 0 ||
					strcmp(namep(argv[i]), "..") == 0
					)
					;
				else
					list_add(&patt_list, argv[i]);
				break;
			case 1:
				list_add(&cmds_list, argv[i]);
				break;
			case 2:
				list_add(&excl_list, argv[i]);
				break;
				}
			}
		}

	if ( state == 0 || state == 2 ) { // syntax error
		puts("`do' keyword missing; run `dof -h' for help.");
		return 1;
		}

	// build regex table
	if ( excl_list.root ) {
		int		i, status;
		node_t	*cur;

		re_count = 0;
		cur = excl_list.root;
		while ( cur ) {
			re_count ++;
			cur = cur->next;
			}
		re_table = (regex_t **) malloc(sizeof(regex_t*) * re_count);
		for ( i = 0, cur = excl_list.root; i < re_count; i ++, cur = cur->next ) {
			re_table[i] = (regex_t *) malloc(sizeof(regex_t));
		    status = regcomp(re_table[i], cur->str, REG_EXTENDED|REG_NEWLINE);
		    if ( status != 0 ) {
				char error_message[LINE_MAX];
				regerror(status, re_table[i], error_message, LINE_MAX);
		        printf("Regex error compiling '%s': %s\n", cur->str, error_message);
        		return 1;
				}
			}
	    }

	if ( flags & 0x10 )
		exit_status = execute_indir(flags);
	else
		exit_status = execute(flags);

	// cleanup
	for ( i = 0; i < re_count; i ++ ) {
		regfree(re_table[i]);
		free(re_table[i]);
		}
	free(re_table);
	
	list_clear(&excl_list);
	list_clear(&file_list);
	list_clear(&cmds_list);
	list_clear(&rcpt_list);
	list_clear(&patt_list);
	return exit_status;
}
