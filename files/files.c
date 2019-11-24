/*
 *	advanced file selection
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <glob.h>

char	*myname;
int		opt_recurs = 0;

/* --- classic list --- */

typedef struct node_s {
	char *file;
	struct node_s *next;
	} node_t;

typedef struct list_s {
	node_t *head, *tail;
	} list_t;

void stk_init(list_t *list)
{
	list->head = list->tail = NULL;
}

list_t *stk_create()
{
	list_t *list = (list_t *) malloc(sizeof(list_t));
	stk_init(list);
	return list;
}

void stk_append(list_t *list, const char *file)
{
	node_t	*node = (node_t *) malloc(sizeof(node_t));
	node->file = strdup(file);
	node->next = NULL;
	if ( list->head == NULL )
		list->head = list->tail = node;
	else {
		list->tail->next = node;
		list->tail = node;
		}
}

void stk_remove(list_t *list, const char *file)
{
	node_t *cur = list->head, *pre = NULL, *tmp;
	
	while ( cur ) {
		if ( strcmp(cur->file, file) == 0 ) {
			free(cur->file);
			if ( cur == list->tail )	list->tail = pre;
			if ( pre )
				pre->next = cur->next;
			else if ( cur == list->head )
				list->head = cur->next;
			tmp = cur;
			cur = cur->next;
			free(tmp);
			continue;
			}
		pre = cur;
		cur = cur->next;
		}
}

void stk_clear(list_t *list)
{
	node_t	*cur = list->head, *pre;
	
	while ( cur ) {
		pre = cur;
		cur = cur->next;
		free(pre->file);
		free(pre);
		}
	list->head = list->tail = NULL;
}

void stk_destroy(list_t *list)
{
	stk_clear(list);
	free(list);
}

void stk_print(list_t *list)
{
	node_t	*cur = list->head;
	
	while ( cur ) {
		printf("%s\n", cur->file);
		cur = cur->next;
		}
}

/* --- classic list --- */

int globerr(const char *path, int eerrno)
{
	fprintf(stderr, "%s: %s: %s\n", myname, path, strerror(eerrno));
	return 0;	/* let glob() keep going */
}

void	print_help()	{ exit(1); }
void	print_version()	{ exit(1); }

#define IF_OPT(s,a,b)	if((strcmp((s),(a))==0)||(strcmp((s),(b))==0))
int main(int argc, char **argv)
{
	int		i, ret, flags = 0, mode = 0;
	list_t	*toremove, *toexclude, *toexecute, *files;
	node_t	*cur;

	toremove  = stk_create();
	toexclude = stk_create();
	toexecute = stk_create();
	for ( i = 1; i < argc; i ++ ) {
		if ( argv[i][0] == '-' ) {
			IF_OPT(argv[i], "-h", "--help")				print_help();
			else IF_OPT(argv[i], "-v", "--version")		print_version();
			else IF_OPT(argv[i], "-x", "--exclude")		mode = 1;
			else IF_OPT(argv[i], "-e", "--execute")		mode = 2;
			else IF_OPT(argv[i], "-r", "--recursive")	opt_recurs = 1;
			else {
				fprintf(stderr, "unknown option %s\n", argv[i]);
				exit(1);
				}
			}
		else {
			switch ( mode ) {
			case 0:	stk_append(toremove,  argv[i]);	break;
			case 1:	stk_append(toexclude, argv[i]);	break;
			case 2:	stk_append(toexecute, argv[i]);	break;
				}
			}
		}
	
	myname = argv[0];	/* for globerr() */
	files = stk_create();

	/* fill list of files to select */
	cur = toremove->head;
	while ( cur ) {
		glob_t results;
		flags = GLOB_BRACE | GLOB_TILDE_CHECK;
		ret = glob(cur->file, flags, globerr, &results);
		if ( ret != 0 ) {
			if ( ret != GLOB_NOMATCH ) {
				const char *desc;
				switch ( ret ) {
				case GLOB_ABORTED: desc = "filesystem problem"; break;
				case GLOB_NOSPACE: desc = "no dynamic memory";  break;
				default: desc = "unknown problem"; }
				fprintf(stderr, "%s: problem with %s (%s), stopping early\nglob() error code %d\n", myname, cur->file, desc, ret);
				exit(1);
				}
			}
		else {
			for ( i = 0; i < results.gl_pathc; i ++ )
				stk_append(files, results.gl_pathv[i]);
			}
		globfree(&results);
		cur = cur->next;
		}

	/* remove files from the list */
	cur = toexclude->head;
	while ( cur ) {
		glob_t results;
		flags = GLOB_BRACE | GLOB_TILDE_CHECK;
		ret = glob(cur->file, flags, globerr, &results);
		if ( ret != 0 ) {
			if ( ret != GLOB_NOMATCH ) {
				const char *desc;
				switch ( ret ) {
				case GLOB_ABORTED: desc = "filesystem problem"; break;
				case GLOB_NOSPACE: desc = "no dynamic memory";  break;
				default: desc = "unknown problem"; }
				fprintf(stderr, "%s: problem with %s (%s), stopping early\nglob() error code %d\n", myname, cur->file, desc, ret);
				exit(1);
				}
			}
		else {
			for ( i = 0; i < results.gl_pathc; i ++ )
				stk_remove(files, results.gl_pathv[i]);
			}
		globfree(&results);
		cur = cur->next;
		}

	/* cleanup */
	stk_destroy(toremove);
	stk_destroy(toexclude);

	/* */
	if ( toexecute->head == NULL )
		stk_print(files);
	else {
		char *buf = (char *) malloc(LINE_MAX);
		cur = toexecute->head;
		while ( cur ) {
			node_t	*fc = files->head;
			while ( fc ) {
				snprintf(buf, LINE_MAX, "%s '%s'", cur->file, fc->file);
				system(buf);
				fc = fc->next;
				}
			cur = cur->next;
			}
		free(buf);
		}

	/* cleanup */
	stk_destroy(toexecute);
	stk_destroy(files);
	return 0;
}
