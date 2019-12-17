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

#include "panic.h"
#include "str.h"
#include "list.h"
#include "file.h"

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

// just a small stack to store a few pointers for recursive issues
static void *stack[256];	// the stack storage
static void **sp = stack;	// stack pointer (top)
#define push(p) *sp ++ = p
#define pop()   -- sp
#define ppop()  *pop()
#define peek()  sp[-1]

// expand `%' variables
static const char *expand_prc(const char *source, const char *data)
{
	static char buf[LINE_MAX];
	char args[LINE_MAX], *tp, *ap;
	const char *p = source, *next;

	buf[0] = '\0';
	switch ( *p ) {
	case 'f': strcpy(buf, data); break;
	case 'b': strcpy(buf, basename(data)); break;
	case 'd': strcpy(buf, dirname(data)); break;
	case 'e': strcpy(buf, extname(data)); break;
	case '%': strcpy(buf, "%"); break;
	case '\'':
	case 'q':  strcpy(buf, "'"); break;
	case '\"': strcpy(buf, "\""); break;
	default:
		fprintf(stderr, "unknown element `%%%c'\n", *p);
		return buf;
		}
	
	// modifiers
	p ++;
	while ( p && *p == ':' ) {
		p ++;
		if ( (next = strchr(p, ':')) != NULL ) {
			strcpy(args, p+1);
			if ( (tp = strchr(args, ':')) != NULL )
				*tp = '\0';
			}
		else
			strcpy(args, p+1);
		ap = args;
			
		switch ( *p ) {
		case 'l':
			while ( *ap ) {
				if ( (tp = strchr(buf, *ap)) != NULL )
					*tp = '\0';
				ap ++;
				}
			break;
		case 'r':
			while ( *ap ) {
				if ( (tp = strrchr(buf, *ap)) != NULL )
					*tp = '\0';
				ap ++;
				}
			break;
		case 'i':
			if ( (tp = strstr(buf, args)) != NULL )
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
				v = expand_prc(block, data);
				while ( *v )	*d ++ = *v ++;
				}
			else {
				block[0] = *p;
				block[1] = '\0';
				v = expand_prc(block, data);
				while ( *v )	*d ++ = *v ++;
				}
			}
		else // not a command? just copy
			*d ++ = *p;
		if ( *p ) p ++; // next p
		}
	*d = '\0';	// close string
	return dest;
}

// wclist callback; append file to the item list
int fl_append(const char *name)
{
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
	char	*cmds, *cmdbuf;
	char	*cwd = (char *) malloc(PATH_MAX);
	int		ignore, exit_status = 0;
	list_node_t	*cur, *reptr;
	struct stat st;
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
			if ( match_regex((regex_t *) reptr->data, cur->key) ) {
				ignore ++;
				break;
				}
			reptr = reptr->next;
			}
		
		// recursive loop into subdirectories
		if ( !ignore && (flags & OFL_RECURS) ) { 
			if ( stat(cur->key, &st) == 0 ) {
				if ( S_ISDIR(st.st_mode) ) {
					push(cwd);
					if ( chdir(cur->key) == 0 ) {
						exit_status = execute(flags);
						if ( exit_status && (flags & OFL_FORCE) == 0 ) { // not force-option
							pop();
							break;
							}
						}
					else
						fprintf(stderr, "change working directory to '%s' failed.\n", cur->key);
					chdir(ppop());
					}
				}
			}
			
		// check file attributes
		if ( !ignore && (flags & (OFL_PLAIN | OFL_DIREC)) ) { // file attribute check
			if ( stat(cur->key, &st) == 0 )
				ignore = ( (flags & OFL_PLAIN) && (!S_ISREG(st.st_mode)) ) ||
						 ( (flags & OFL_DIREC) && (!S_ISDIR(st.st_mode)) );
			}
			
		//
		if ( ignore )
			{ cur = cur->next; continue; }
			
		// execute
		cmdbuf = dof(cmds, cur->key);
		if ( (flags & OFL_EXEC) == 0 ) // not execute-option
			fprintf(stdout, "%s\n", cmdbuf);
		else {
			if ( (exit_status = system(cmdbuf)) != 0 ) {
				if ( (flags & OFL_FORCE) == 0 ) { // not force-option
					free(cmdbuf);
					break;
					}
				}
			}
		free(cmdbuf);
		
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

#define APP_VER "1.7"

static const char *usage = "\
Usage: dof [list] [-x patterns] do [commands]\n\
"APP_DESCR"\n\
\n\
Options:\n\
\t-e\texecute; dof displays what commands would be run, this option executes them.\n\
\t-x\texclude glob patterns; the excluded list always has priority.\n\
\t-g\texclude regex patterns; the excluded list always has priority.\n\
\t-r\trecursive execution of commands into sub-directories.\n\
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
Copyright (C) 2017-2020 Free Software Foundation, Inc.\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
Written by Nicholas Christopoulos <mailto:nereus@freemail.gr>\n\
";

// parsing arguments stages
typedef enum stage_e { Items = 0, Commands, ExcludeRE, ExcludeWC } stage_t;

// initialize globals
void dof_init()
{
	list_init(&cmds_list);
	list_init(&recp_list);
	list_init(&incl_list);
	list_init(&regx_list);
	list_init(&excl_list);
	
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
	char message[LINE_MAX];
	
	cur = regx_list.root;
	while ( cur ) {
		cur->data = (void *) malloc(sizeof(regex_t));
		status = regcomp((regex_t *) (cur->data), cur->key, REG_EXTENDED|REG_NEWLINE);
		if ( status != 0 ) {
			regerror(status, (regex_t *) (cur->data), message, LINE_MAX);
			panic("Regex error compiling '%s': %s\n", cur->key, message);
			}
		cur = cur->next;
		}
}

// execute recipe
int execute_recipe(const char *key, int flags)
{
	list_node_t	*cur;
	
	if ( strcmp(key, "help") == 0 )    { puts(usage); return 1; }
	if ( strcmp(key, "version") == 0 ) { puts(verss); return 1; }
	
	cur = recp_list.root;
	while ( cur ) {
		if ( strcmp(cur->key, key) == 0 ) {
			char cmd[LINE_MAX];
			char opt[32];
			opt[0] = '\0';
			if ( flags & OFL_EXEC   ) strcat(opt, "-e ");
			if ( flags & OFL_FORCE  ) strcat(opt, "-f ");
			if ( flags & OFL_PLAIN  ) strcat(opt, "-p ");
			if ( flags & OFL_DIREC  ) strcat(opt, "-d ");
			if ( flags & OFL_RECURS ) strcat(opt, "-r ");
			snprintf(cmd, LINE_MAX, "dof %s %s", opt, (char *) cur->data);
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
		}
}

// add sequence of numbers
int dof_addseq(stage_t stage, const char *src)
{
	const char *p = src;
	double	last, start, step = 1.0;
	char	buf[LINE_MAX];

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

// main()
int main(int argc, char **argv)
{
	int		i, j, flags = 0, opt_seq = 0;
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
				char	buf[LINE_MAX];
				while ( fgets(buf, LINE_MAX, stdin) )
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
				case 'h': puts(usage); return 1;
				case 'v': puts(verss); return 1;
				case 's': opt_seq = 1; break;
				case '-': return execute_recipe(argv[i]+2, flags);
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
	return execute(flags);
}
