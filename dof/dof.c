/*
 * dof [file [file [...]] do command
 *
 * Nicholas Christopoulos (nereus@freemail.gr)
 *
 * this software is released under GPLv3 or newer license
 */

/* Enable GNU extensions in fnmatch.h.  */
//#ifndef _GNU_SOURCE
//	#define _GNU_SOURCE        1
//#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <dirent.h>
#include <regex.h>
#include <fnmatch.h>
#include <glob.h>
#include "list.c"
#include "file.c"

static list_t file_list;
static list_t cmds_list;
static list_t rcpt_list;
static list_t patt_list;
static list_t excl_list;
static list_t excg_list;

typedef char * charp_t;

static int		re_count = 0;
static regex_t	**re_table;
static int		ge_count = 0;
static charp_t	*ge_table;

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

#define APP_VER "1.5"

static const char *usage = "\
Usage: dof [list] [-x patterns] do [commands]\n\
"APP_DESCR"\n\
\n\
Options:\n\
\t-e\texecute; dof displays what commands would be run, this option executes them.\n\
\t-x\texclude regex patterns; the excluded list always has priority.\n\
\t-g\texclude glob patterns; the excluded list always has priority.\n\
\t-r\trecursive execution of commands into sub-directories.\n\
\t-f\tforce non-stop; dof stops on error, this option forces dof to ignore errors.\n\
\t-p\tplain files only; directories, devices, etc are ignored.\n\
\t-d\tdirectories only; plain files, devices, etc are ignored.\n\
\t-s:fist..last[..step]\tadd sequence of numbers (float or integer).\n\
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
Copyright (C) 2017-2019 Free Software Foundation, Inc.\n\
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
	for ( i = 0; i < ge_count; i ++ ) {
//		if ( fnmatch(ge_table[i], src, FNM_PATHNAME | FNM_EXTMATCH) == 0 )
		if ( fnmatch(ge_table[i], src, FNM_PATHNAME | FNM_PERIOD) == 0 )
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
const char *getnum(const char *src, char *buf)
{
	const char *p = src;
	char *d = buf;
	while ( *p == ' ' || *p == '\t' ) p ++;
	if ( *p == '+' )	p ++;
	if ( *p == '-' )	*d ++ = *p ++;
	
	while ( isdigit(*p) )	*d ++ = *p ++;
	if ( p[0] == '.' && isdigit(p[1]) ) {
		*d ++ = *p ++;
		while ( isdigit(*p) )	*d ++ = *p ++;
		}
	*d = '\0';
	return p;
}

//
int main(int argc, char **argv)
{
	int		flags = 0, state = 0, exit_status = 0;
	int		i;
	node_t	*cur;
	char buf[LINE_MAX];

	list_init(&file_list);
	list_init(&cmds_list);
	list_init(&rcpt_list);
	list_init(&patt_list);
	list_init(&excl_list);
	list_init(&excg_list);

	for ( i = 1; i < argc; i ++ ) {
		if ( argv[i][0] == '-' && (state == 0 || state > 1) ) {

			// one minus, read from stdin
			if ( argv[i][1] == '\0' ) {
				while ( fgets(buf, LINE_MAX, stdin) )	{
					switch ( state ) {
					case 0:
						if ( strcmp(namep(argv[i]), ".") != 0 && strcmp(namep(argv[i]), "..") != 0 )
							list_add(&file_list, buf);
						break;
					case 1:	list_add(&cmds_list, buf);	break;
					case 2:	list_add(&excl_list, buf);	break;
					case 3:	list_add(&excg_list, buf);	break;
						}
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
				case 'g': state = 3; break;
				case 'h': puts(usage); return 1;
				case 'v': puts(verss); return 1;
				case 's': // add sequence of numbers
						{
						const char *p, *pstart;
						double last, start, step = 1.0;

						p = pstart = (argv[i]+j+1);
						if ( *p != ':' ) { puts("example: dof -s:1..10"); return 1; }
						p ++;

						// get 'first'
						p = getnum(p, buf);
						start = last = atof(buf);

						// next number
						if ( p[0] == '.' && p[1] == '.' )		p += 2;
						else { puts("example: dof -s:1..10"); return 1; }
						
						// get 'last'
						p = getnum(p, buf);
						last = atof(buf);

						// get 'step'
						if ( p[0] == '.' && p[1] == '.' ) {
							p += 2;
							p = getnum(p, buf);
							step = atof(buf);
							}
							
						// add
						for ( double f = start; f <= last; f += step ) {
							sprintf(buf, "%g", f);
							switch ( state ) {
							case 0:	list_add(&patt_list, buf);	break;
							case 1:	list_add(&cmds_list, buf); break;
							case 2:	list_add(&excl_list, buf); break;
							case 3:	list_add(&excg_list, buf); break;
								}
							}

						j += p - pstart;
						}
						break;
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
					printf("unknown option [%c].\n", argv[i][j]);
					return 1;
					}
				}
			}
		else if ( strcmp(argv[i], "do") == 0 && (state == 0 || state > 1) )
			state = 1;
		else {
			switch ( state ) {
			case 0:
				if ( strcmp(namep(argv[i]), ".") != 0 && strcmp(namep(argv[i]), "..") != 0 )
					list_add(&patt_list, argv[i]);
				break;
			case 1:	list_add(&cmds_list, argv[i]); break;
			case 2:	list_add(&excl_list, argv[i]); break;
			case 3:	list_add(&excg_list, argv[i]); break;
				}
			}
		}

	if ( state == 0 || state > 1 ) { // syntax error
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

	// build fnmatch table
	if ( excg_list.root ) {
		int		i;
		node_t	*cur;

		ge_count = 0;
		cur = excg_list.root;
		while ( cur ) {
			ge_count ++;
			cur = cur->next;
			}
		ge_table = (charp_t *) malloc(sizeof(charp_t) * ge_count);
		for ( i = 0, cur = excg_list.root; i < ge_count; i ++, cur = cur->next ) {
			ge_table[i] = (charp_t) malloc(sizeof(charp_t));
		    ge_table[i] = strdup(cur->str);
			}
	    }

	// execute
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

	for ( i = 0; i < ge_count; i ++ )
		free(ge_table[i]);
	free(ge_table);

	list_clear(&excg_list);
	list_clear(&excl_list);
	list_clear(&file_list);
	list_clear(&cmds_list);
	list_clear(&rcpt_list);
	list_clear(&patt_list);
	return exit_status;
}
