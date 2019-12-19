/*
 *	dof <list> do <command>
 *
 *	Executes <command> for each element of the <list>.
 * 
 *	This program was created because of the lack of one-line foreach of
 *	tcsh.
 * 
 *	Copyright (C) 2017-2020 Free Software Foundation, Inc.
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
 */

#include <time.h>
#include "panic.h"
#include "str.h"
#include "list.h"
#include <fnmatch.h>
#include "file.h"

#define BUFSZ		LINE_MAX

// options - flags
#define OFL_EXEC    0x01	// execute commands (otherwise it is just displays)
#define OFL_FORCE   0x02	// force: on error continue
#define OFL_PLAIN   0x04	// files, plain files only
#define OFL_DIREC   0x08	// files, directories only
#define OFL_RECURS  0x10	// recursive loop into subdirectories

static list_t cmds_list;	// list of commands
static list_t recp_list;	// recipes
static list_t incl_list;	// wc-patterns include list
static list_t regx_list;	// regex exclude list
static list_t excl_list;	// wc-patterns exclude list
static list_t dexc_list;	// wc-patterns exclude directories (recursive -X flag)

static int opt_flags;		// global version of execute's flags, too much passing in/out

// just a small stack to store a few pointers for recursive issues
static void *stack[256];	// the stack storage
static void **sp = stack;	// stack pointer (top)
#define push(p) *sp ++ = p
#define pop()   -- sp
#define ppop()  *pop()
#define peek()  sp[-1]

static int opt_unquote = 0;	// check single quotes in string

// variables/functions of '%' expressions
void	v_copyarg(const char *arg, char *rv)	{ strcpy(rv, arg); }
void	v_gethome(const char *arg, char *rv)	{ const char *p = getenv("HOME"); strcpy(rv, (p)? p : ""); }
void	v_basename(const char *arg, char *rv)	{ strcpy(rv, basename(arg)); }
void	v_dirname(const char *arg, char *rv)	{ strcpy(rv, dirname(arg)); }
void	v_extname(const char *arg, char *rv)	{ strcpy(rv, extname(arg)); }
void	v_getcwd(const char *arg, char *rv)		{ getcwd(rv, PATH_MAX); }
void	v_getdate(const char *arg, char *rv)
{
	time_t now; time(&now);
	struct tm *local = localtime(&now);
	sprintf(rv, "%d-%02d-%02d", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday);
}
void	v_gettime(const char *arg, char *rv)
{
	time_t now; time(&now);
	struct tm *local = localtime(&now);
	sprintf(rv, "%02d-%02d-%02d", local->tm_hour, local->tm_min, local->tm_sec);
}

typedef struct {
	const char *name;
	void (*func)(const char *, char *);
	const char *value;
	const char *desc;
	} dof_var_t;

dof_var_t dof_vars[] = {
	{ "f", v_copyarg,  NULL,   "the full string" },
	{ "b", v_basename, NULL,   "the basename of the file; no directory, no extension" },
	{ "d", v_dirname,  NULL,   "the directory of the filename" },
	{ "e", v_extname,  NULL,   "the extension of the filename (without dot)" },
	{ "h", v_gethome,  NULL,   "the home directory" },
	{ "home", v_gethome, NULL, "the home directory" },
	{ "cwd", v_getcwd, NULL,   "the current working directory" },
	{ "date", v_getdate, NULL, "the current date in the form YYYY-MM-DD" },
	{ "time", v_gettime, NULL, "the current time in the form HH-MM-SS" },
	{ "q",  NULL, "'",         "single quote character (')" },
	{ "dq", NULL, "\"",        "double quote character (\")" },
	{ "bq", NULL, "`",         "backquote character (`)" },
	{ NULL, NULL, NULL, NULL } // end-of-list
};

//
void	print_vars()
{
	for ( int i = 0; dof_vars[i].name; i ++ )
		printf("%16s %s\n", dof_vars[i].name, dof_vars[i].desc);
}

// expand '%' expressions
char *expand_expr(char *dest, const char *source, const char *data)
{
	const char *p = source, *pn;
	char *d = dest, *tp;
	char *buf = (char *) malloc(BUFSZ);
	char name[32], *n;
	int  i, found;

	// get variable name
	n = name;
	while ( isalnum(*p) )
		*n ++ = *p ++;
	*n = '\0';

	// find and copy variable's data
	for ( i = 0, found = 0; dof_vars[i].name; i ++ ) {
		if ( strcmp(dof_vars[i].name, name) == 0 ) {
			if ( dof_vars[i].func )
				dof_vars[i].func(data, buf);
			else if ( dof_vars[i].value )
				strcpy(buf, dof_vars[i].value);
			found ++;
			break;
			}
		}

	if ( found ) {
		// modifiers
		while ( *p == ':' ) {
			p ++;

			switch ( *p ) {
			case 'l':	// l[{f|l}]<c> the left part of first|last occurrence of 'c'
				switch ( p[1] ) {
				case 'f': tp = strchr (buf, p[2]); p += 3; break;
				case 'l': tp = strrchr(buf, p[2]); p += 3; break;
				case 's':
					tp = strstr (buf, p +2);
					p = ((pn = strchr(p, ':')) == NULL) ? p + strlen(p) : pn;
					break;
				default:  tp = strchr (buf, p[1]); p += 2; }
				if ( tp ) *tp = '\0';
				break;
			case 'r':	// r[{f|l}]<c> the right part of first|last occurrence of 'c'
				switch ( p[1] ) {
				case 'f': tp = strchr (buf, p[2]); p += 3; break;
				case 'l': tp = strrchr(buf, p[2]); p += 3; break;
				case 's':
					tp = strstr (buf, p +2);
					p = ((pn = strchr(p, ':')) == NULL) ? p + strlen(p) : pn;
					break;
				default:  tp = strchr (buf, p[1]); p += 2; }
				if ( tp ) {
					char *tmp = strdup(tp+1);
					strcpy(buf, tmp);
					free(tmp);
					}
				break;
			case 't':	// t<a><b> replaces all occurrences of 'a' to 'b' (character)
				strtotr(buf, p[1], p[2]);
				p += 3;
				break;
			case 's':	// s/str/str/[g]
				p ++;
				if ( *p ) {
					char mark = *p ++; // first mark
					int  len = strlen(p);
					char str1[len], str2[len];
					for ( tp = str1; *p && *p != mark; *tp ++ = *p ++ );
					*tp = '\0';
					if ( *p == mark ) { // middle
						p ++;
						for ( tp = str2; *p && *p != mark; *tp ++ = *p ++ );
						*tp = '\0';
						if ( *p == mark ) { // final
							int gflag = 0;
							p ++;
							if ( *p == 'g' ) { gflag = 1; p ++; }
							// now replace str1 on buf with str2
							if ( gflag )
								res_replace(str1, buf, str2, 32);
							else
								res_replace(str1, buf, str2, 1);
							}
						}
					}
				break;
				}
			}

		// copy result to dest
		p = buf;
		while ( *p )
			*d ++ = *p ++;
		}
	else
		error("unknown variable '%%%s'", name);

	free(buf);
	return d;
}

// converts and returns the commands to run with 'system()'
// the buffer must be freed by the caller
char *expand(const char *source, const char *data)
{
	const char *p;
	char *d, *dest;
	int inside_sq = 0;
	int count = 0, maxlen;

	// calculate a maximum length for the returned buffer
	p = source;
	while ( p ) {
		count ++;
		p = strchr(p + 1, '%');
		}
	maxlen = BUFSZ;

	// replace strings
	dest = (char *) malloc(maxlen);
	d = dest;
	p = source;
	while ( *p ) {

		if ( opt_unquote ) {
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
			}

		// translate '%' expressions
		if ( *p == '%' ) {
			p ++;
			if ( *p == '%' || *p == '\'' || *p == '"' || *p == '\\' ) // few special chars
				*d ++ = *p ++;
			else if ( *p == '~' ) {
				d = expand_expr(d, "h", data);
				p ++;
				}
			else {
				char *block = (char *) malloc(BUFSZ), *bp;
				char mark = ' ';

				if ( ispunct(*p) ) { // %{form} or %(form) or %/form/
					mark = *p;
					if ( mark == '{' )	mark = '}';
					if ( mark == '(' )	mark = ')';
					if ( mark == '[' )	mark = ']';

					// copy internal block
					p ++;
					bp = block;
					while ( *p ) {
						if ( *p == mark ) {
							p ++;
							break;
							}
						*bp ++ = *p ++;
						}
					*bp = '\0';
					d = expand_expr(d, block, data);
					}
				else if ( isalnum(*p) ) {
					bp = block;
					while ( isalnum(*p) )
						*bp ++ = *p ++;
					if ( *p == ':' ) { // modifier follows
						while ( *p && !isspace(*p) )
							*bp ++ = *p ++;
						}
					*bp = '\0';
					d = expand_expr(d, block, data);
					}
				else { // actually, this is error; but I ll pass..
					*d ++ = '%';
					if ( *p )
						*d ++ = *p ++;
					}
				free(block);
				}
			}
		else // copy
			*d ++ = *p ++;
		}

	*d = '\0';	// close string
	return dest;
}

// wclist callback; append file to the item list
int fl_append(const char *name)
{
	if ( opt_flags & (OFL_PLAIN | OFL_DIREC) ) {
		struct stat st;
		if ( lstat(name, &st) == 0 ) {
			if ( opt_flags & OFL_PLAIN )
				if ( S_ISREG(st.st_mode) )
					list_append((list_t *) peek(), name);
			if ( opt_flags & OFL_DIREC )
				if ( S_ISDIR(st.st_mode) )
					list_append((list_t *) peek(), name);
			}
		}
	else
		list_append((list_t *) peek(), name);
	return 0;
}

// wclist callback; remove file from the item list
int fl_remove(const char *name)
{
	list_remove((list_t *) peek(), name);
	return 0;
}

// execute
int execute(int flags)
{
	char	*cmds;
	int		ignore = 0, exit_status = 0;
	list_node_t	*cur, *reptr;
	struct stat st;
	char	*cwd = (char *) malloc(PATH_MAX);
	list_t	*items = list_create();

	push(items);
	getcwd(cwd, PATH_MAX);
//	printf("\nDIR: %s\n", cwd);

	// select files
	cur = incl_list.root;
	while ( cur ) {
		if ( iswcpat(cur->key) )
			wclist(cur->key, fl_append);
		else
			fl_append(cur->key);
		cur = cur->next;
		}

	// exclude files
	cur = excl_list.root;
	while ( cur ) {
		if ( iswcpat(cur->key) )
			wclist(cur->key, fl_remove);
		else
			fl_remove(cur->key);
		cur = cur->next;
		}

	// for each item in the list
	cur  = items->root;
	cmds = list_to_string(&cmds_list, " ");
	while ( cur ) {
		ignore = 0;

		// exclude items by regex
		reptr = regx_list.root;
		while ( reptr ) {
			if ( rex_match((regex_t *) reptr->data, cur->key) ) {
				ignore ++;
				break;
				}
			reptr = reptr->next;
			}

		// check file attributes
		if ( !ignore && (flags & (OFL_PLAIN | OFL_DIREC)) ) { // file attribute check
			if ( stat(cur->key, &st) == 0 )
				ignore = ( (flags & OFL_PLAIN) && (!S_ISREG(st.st_mode)) ) ||
						 ( (flags & OFL_DIREC) && (!S_ISDIR(st.st_mode)) );
			}

		// execute
		if ( !ignore && (exit_status == 0) ) {
			char *command_line = expand(cmds, cur->key);
			if ( (flags & OFL_EXEC) == 0 ) // not execute-option
				fprintf(stdout, "%s\n", command_line);
			else
				exit_status = system(command_line);
			free(command_line);
			if (exit_status && ((flags & OFL_FORCE) == 0)) break;
			}

		// next
		cur = cur->next;
		}
	
	list_destroy(items);
	pop();
	free(cwd);
	return exit_status;
}

// read_conf() callback
int conf_parser(char *source)
{
	char *p = source;

	while ( *p && *p != ':' )	p ++;
	if ( *p == ':' ) {
		*p ++ = '\0';
		while ( *p == ' ' || *p == '\t' ) p ++;
		list_addp(&recp_list, source, p);
		}
	return 0;
}

// --- main() ---

#define APP_DESCR \
"dof (do-for) run commands for each element of 'list'."

#define APP_VER "1.10"

static const char *usage = "\
Usage: dof [list] [-x patterns] [do [commands]]\n\
"APP_DESCR"\n\
\n\
Options:\n\
\t-e\texecute; dof displays what commands would be run, this option executes them.\n\
\t-x\texclude glob patterns; the excluded list always has priority.\n\
\t-g\texclude regex patterns; the excluded list always has priority.\n\
\t-r\trecursive execution of commands into sub-directories (beta).\n\
\t-X\texclude glob patterns for directories only in recursive mode only; the excluded list always has priority.\n\
\t-f\tforce non-stop; dof stops on error, this option forces dof to ignore errors.\n\
\t-p\tplain files only; directories, devices, etc are ignored.\n\
\t-d\tdirectories only; plain files, devices, etc are ignored.\n\
\t-s fist..last[..step]\tadd sequence of numbers (float or integer).\n\
\t-l\tprint recipes (/etc/dof.conf, ~/.dof)\n\
\t--<recipe>\n\t\texecute recipe (ex: dof --to-ogg)\n\
\t-\tread from stdin\n\
\t-h\tthis screen\n\
\t-v\tversion and program information\n\
\n\
Variables (use: dof --vars):\n\
\t%f\tthe string (if file; the full path name)\n\
\t%b\tthe basename (no directory, no extension)\n\
\t%d\tthe directory (without trailing '/')\n\
\t%e\tthe extension (without '.')\n\
\n\
Modifiers:\n\
modifiers defined by ':' that follows a variable and modifies the result string. You can have unlimited number of modifiers.\n\
\tl[{f|l}]c\treturns the string until the the first|last occurence of 'c'\n\
\tr[{f|l}]c\treturns the right part of the string from the first|last occurence of 'c'\n\
\ttab\treplaces all 'a' characters with 'b' character\n\
\tis\treturns the string until the occurence of string 's'\n\
";

static const char *verss = "\
dof version "APP_VER"\n\
"APP_DESCR"\n\
\n\
Copyright (C) 2017-2020 Free Software Foundation, Inc.\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
Written by Nicholas Christopoulos <mailto:nereus@freemail.gr>\n\
";

// parsing arguments stages
typedef enum stage_e { Items = 0, Commands, ExcludeRE, ExcludeWC, ExcludeDirWC } stage_t;

// initialize globals
void dof_init()
{
	list_init(&cmds_list);
	list_init(&recp_list);
	list_init(&incl_list);
	list_init(&regx_list);
	list_init(&excl_list);
	list_init(&dexc_list);

	void dof_done();
	atexit(dof_done);

	readconf("dof", conf_parser);
}

// closing program (atexit)
void dof_done()
{
	list_node_t	*cur;

	cur = regx_list.root;
	while ( cur ) {
		regfree((regex_t *) (cur->data));
		cur = cur->next;
		}

	list_clear(&dexc_list);
	list_clear(&excl_list);
	list_clear(&regx_list);
	list_clear(&cmds_list);
	list_clear(&recp_list);
	list_clear(&incl_list);
}

// build regex_t table
void dof_build_regex()
{
	list_node_t	*cur;
	int status;
	char message[BUFSZ];

	cur = regx_list.root;
	while ( cur ) {
		cur->data = (void *) malloc(sizeof(regex_t));
		status = regcomp((regex_t *) (cur->data), cur->key, REG_EXTENDED|REG_NEWLINE|REG_NOSUB);
		if ( status != 0 ) {
			regerror(status, (regex_t *) (cur->data), message, BUFSZ);
			panic("Regex error compiling '%s': %s\n", cur->key, message);
			}
		cur = cur->next;
		}
}

// execute recipe
int execute_recipe(const char *key, int flags)
{
	list_node_t	*cur;

	cur = recp_list.root;
	while ( cur ) {
		if ( strcmp(cur->key, key) == 0 ) {
			char cmd[BUFSZ];
			char opt[32];
			opt[0] = '\0';
			if ( flags & OFL_EXEC   ) strcat(opt, "-e ");
			if ( flags & OFL_FORCE  ) strcat(opt, "-f ");
			if ( flags & OFL_PLAIN  ) strcat(opt, "-p ");
			if ( flags & OFL_DIREC  ) strcat(opt, "-d ");
			if ( flags & OFL_RECURS ) strcat(opt, "-r ");
			snprintf(cmd, BUFSZ, "dof %s %s", opt, (char *) cur->data);
			return system(cmd);
			}
		cur = cur->next;
		}

	error("recipe '%s' not found", key);
	return 1;
}

// put an item in the correct list
void dof_additem(stage_t stage, const char *data)
{
	switch ( stage ) {
	case Items:
		if ( !isdots(filename(data)) )
			list_add(&incl_list, data);
		break;
	case Commands:	list_add(&cmds_list, data);	break;
	case ExcludeRE:	list_add(&regx_list, data);	break;
	case ExcludeWC:	list_add(&excl_list, data);	break;
	case ExcludeDirWC:	list_add(&dexc_list, data);	break;
		}
}

// add sequence of numbers
int dof_addseq(stage_t stage, const char *src)
{
	const char *p = src;
	double	last, start, step = 1.0;
	char	buf[BUFSZ];

	// get 'first'
	p = parse_num(p, buf);
	start = last = atof(buf);

	// next number
	if ( (p = parse_const(p, "..")) == NULL )
		{ error("example: dof -s 1..10"); return 1; }

	// get 'last'
	p = parse_num(p, buf);
	last = atof(buf);

	// get 'step'
	if ( p[0] == '.' && p[1] == '.' ) {
		p += 2;
		p = parse_num(p, buf);
		step = atof(buf);
		}

	// add
	for ( double f = start; f <= last; f += step ) {
		sprintf(buf, "%g", f);
		dof_additem(stage, buf);
		}
	return 0;
}

//
int recurs_exec_cb(const char *file, const char *cwd, int dtype, void *pars)
{
//	printf("%02X [%s] [%s]\n", dtype, cwd, file); return 0;

	if ( dexc_list.root ) { // exclude directories list is defined
		list_node_t *cur;
		int found = 0;
		char *path = (char *) malloc(PATH_MAX);
		strcpy(path, cwd);
		if ( path[strlen(path)-1] != '/' )
			strcat(path, "/");
		strcat(path, file);
		
		// exclude directories
		cur = dexc_list.root;
		while ( cur ) {
			if ( fnmatch(cur->key, path, 0) == 0 ) {
				found ++;
				break;
				}
			cur = cur->next;
			}
		free(path);
		if ( found ) return 0; // ignore this directory
		}
	
	return execute(opt_flags);
}

// main()
int main(int argc, char **argv)
{
	int		i, j, flags = 0, opt_seq = 0, status = 0;
	stage_t	stage = Items;

	dof_init();

	// parsing arguments
	for ( i = 1; i < argc; i ++ ) {
		if ( opt_seq ) { // this arg is the parameter of '-s'
			if ( dof_addseq(stage, argv[i]) )
				return 1;
			opt_seq = 0;
			}
		else if ( (argv[i][0] == '-') && (stage != Commands) ) {

			if ( argv[i][1] == '\0' ) {	// one minus, read from stdin
				char	buf[BUFSZ];
				while ( fgets(buf, BUFSZ, stdin) )
					dof_additem(stage, buf);
				continue; // we finished with this argv
				}

			// check options
			for ( j = 1; argv[i][j]; j ++ ) {
				switch ( argv[i][j] ) {
				case 'e': flags |= OFL_EXEC; break;
				case 'f': flags |= OFL_FORCE; break;
				case 'p': flags |= OFL_PLAIN; break;
				case 'd': flags |= OFL_DIREC; break;
				case 'r': flags |= OFL_RECURS; break;
				case 'g': stage = ExcludeRE; break;
				case 'x': stage = ExcludeWC; break;
				case 'X': stage = ExcludeDirWC; break;
				case 'u': opt_unquote = !opt_unquote; break;
				case 'h': puts(usage); return 1;
				case 'v': puts(verss); return 1;
				case 's': opt_seq = 1; break;
				case '-': // -- double minus
					if ( strcmp(argv[i], "--help") == 0 )    { puts(usage); return 1; }
					if ( strcmp(argv[i], "--version") == 0 ) { puts(verss); return 1; }
					if ( strcmp(argv[i], "--vars") == 0 )    { print_vars(); return 1; }
					return execute_recipe(argv[i]+2, flags);
				case 'l': list_print(&recp_list, stdout); return 0;
				default:
					error("unknown option [%c]", argv[i][j]);
					return 1;
					}
				}
			}
		else if ( (strcmp(argv[i], "do") == 0) && (stage != Commands) )
			stage = Commands;
		else
			dof_additem(stage, argv[i]);
		}

	if ( stage != Commands ) { // no commands specified
		// error("`do' keyword missing; usage: dof <items> do <commands>\nrun `dof -h' for help"); return 1;
		stage = Commands;
		dof_additem(stage, "%f");
		}

	dof_build_regex();
	opt_flags = flags;
	if ( flags & OFL_RECURS )
		status = dirwalk(".", recurs_exec_cb, DIRWALK_RECURSIVE, NULL);
	else
		status = execute(flags);
	return status;
}
