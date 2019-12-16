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

static void *stack[256];	// the stack storage
static void **sp = stack;	// stack pointer (top)
#define push(p) *sp ++ = p
#define pop()   -- sp
#define ppop()  *pop()
#define peek()  sp[-1]

//
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
	
	pop();
	list_destroy(items);
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

#define APP_VER "1.6"

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
Copyright (C) 2017-2020 Free Software Foundation, Inc.\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
Written by Nicholas Christopoulos <mailto:nereus@freemail.gr>\n\
";

//
int main(int argc, char **argv)
{
	int		flags = 0, stage = 0, exit_status = 0;
	int		i, status;
	list_node_t	*cur;
	char buf[LINE_MAX];
	
	list_init(&cmds_list);
	list_init(&recp_list);
	list_init(&incl_list);
	list_init(&regx_list);
	list_init(&excl_list);

	readconf("dof", conf_parser);
	
	for ( i = 1; i < argc; i ++ ) {
		if ( argv[i][0] == '-' && (stage == 0 || stage > 1) ) {

			// one minus, read from stdin
			if ( argv[i][1] == '\0' ) {
				while ( fgets(buf, LINE_MAX, stdin) )	{
					switch ( stage ) {
					case 0:
						if ( strcmp(filename(argv[i]), ".") != 0 && strcmp(filename(argv[i]), "..") != 0 )
							list_add(&incl_list, buf);
						break;
					case 1:	list_add(&cmds_list, buf);	break;
					case 2:	list_add(&regx_list, buf);	break;
					case 3:	list_add(&excl_list, buf);	break;
						}
					}
				}

			// check options
			for ( int j = 1; argv[i][j]; j ++ ) {
				switch ( argv[i][j] ) {
				case 'e': flags |= OFL_EXEC; break;
				case 'f': flags |= OFL_FORCE; break;
				case 'p': flags |= OFL_PLAIN; break;
				case 'd': flags |= OFL_DIREC; break;
				case 'r': flags |= OFL_RECURS; break;
				case 'g': stage = 2; break;
				case 'x': stage = 3; break;
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
						p = parse_num(p, buf);
						start = last = atof(buf);

						// next number
						if ( (p = parse_const(p, "..")) == NULL )
							{ puts("example: dof -s:1..10"); return 1; }
						
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
							switch ( stage ) {
							case 0:	list_add(&incl_list, buf);	break;
							case 1:	list_add(&cmds_list, buf); break;
							case 2:	list_add(&regx_list, buf); break;
							case 3:	list_add(&excl_list, buf); break;
								}
							}

						j += p - pstart;
						}
						break;
				case '-':
						// execute recipe
						cur = recp_list.root;
						while ( cur ) {
							if ( strcmp(cur->key, argv[i]+2) == 0 ) {
								char cmd[LINE_MAX];
								char opt[64];
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
						return 1;
				case 'l':
						// list recipes
						cur = recp_list.root;
						while ( cur ) {
							printf("%s: (%s)\n", cur->key, (char *) cur->data);
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
		else if ( strcmp(argv[i], "do") == 0 && (stage == 0 || stage > 1) )
			stage = 1;
		else {
			switch ( stage ) {
			case 0:
				if ( strcmp(filename(argv[i]), ".") != 0 && strcmp(filename(argv[i]), "..") != 0 )
					list_add(&incl_list, argv[i]);
				break;
			case 1:	list_add(&cmds_list, argv[i]); break;
			case 2:	list_add(&regx_list, argv[i]); break;
			case 3:	list_add(&excl_list, argv[i]); break;
				}
			}
		}

	if ( stage == 0 || stage > 1 ) { // syntax error
		puts("`do' keyword missing; run `dof -h' for help.");
		return 1;
		}

	// build regex table
	cur = regx_list.root;
	while ( cur ) {
		cur->data = (void *) malloc(sizeof(regex_t));
		status = regcomp((regex_t *) (cur->data), cur->key, REG_EXTENDED|REG_NEWLINE);
		if ( status != 0 ) {
			char error_message[LINE_MAX];
			regerror(status, (regex_t *) (cur->data), error_message, LINE_MAX);
			printf("Regex error compiling '%s': %s\n", cur->key, error_message);
			return 1;
			}
		cur = cur->next;
		}

	// execute
	exit_status = execute(flags);

	// cleanup
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
	return exit_status;
}
