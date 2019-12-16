#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glob.h>
#include "list.h"

/*
 * add node to list
 * this just stores a string in the list
 */
list_node_t *list_add(list_t *list, const char *key)
{
	list_node_t *np = (list_node_t *) malloc(sizeof(list_node_t));
	np->key  = strdup(key);
	np->data = NULL;
	np->next = NULL;
	if ( list->root ) {
		list->tail->next = np;
		list->tail = np;
		}
	else
		list->root = list->tail = np;
	return np;
}

/*
 * add node to list
 * this stores a key and a value to the list
 */
list_node_t *list_addp(list_t *list, const char *key, const char *value)
{
	list_node_t *np = (list_node_t *) malloc(sizeof(list_node_t));
	np->key  = strdup(key);
	np->data = strdup(value);
	np->next = NULL;
	if ( list->root ) {
		list->tail->next = np;
		list->tail = np;
		}
	else
		list->root = list->tail = np;
	return np;
}

/*
 * Initialize list
 * if list = null then creates a new list and returns its pointer
 */
list_t *list_init(list_t *list)
{
	if ( list == NULL )
		list = (list_t *) malloc(sizeof(list_t));
	list->root = list->tail = NULL;
	return list;
}

/*
 *	Clean up memory and reset the list
 */
void list_clear(list_t *list)
{
	list_node_t *pre, *cur = list->root;

	while ( cur ) {
		pre = cur;
		cur = cur->next;
		free(pre->key);
		free(pre);
		}
	list->root = list->tail = NULL;
}

/*
 * add node to list
 * this stores wildcard matches
 */
void list_addwc(list_t *list, const char *pattern)
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

/*
 *	convert string list to string
 */
char *list_to_string(list_t *list, const char *delim)
{
	char	*ret;
	int		len = 0, delim_len = 0, first = 1;
	list_node_t	*cur = list->root;

	if ( delim )
		delim_len = strlen(delim);
	ret = (char *) malloc(1);
	*ret = '\0';
	while ( cur ) {
		len += delim_len + strlen(cur->key);
		ret = (char *) realloc(ret, len + 1);
		if ( first )
			first = 0;
		else if ( delim )
			strcat(ret, delim);
		strcat(ret, cur->key);
		cur = cur->next;
		}
	return ret;
}
