/*
 *	Dynamic Single-linked List module
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glob.h>
#include "list.h"

#define IF_DOTS(s) if((s)[0]=='.' && ((s)[1]=='\0' || ((s)[1]=='.' && (s)[2]=='\0')))

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
 * remove node by key from list
 */
void list_remove(list_t *list, const char *key)
{
	list_node_t *pre = NULL, *cur = list->root;
    
	while ( cur ) {
		if ( strcmp(cur->key, key) == 0 ) {
			if ( pre ) {
				if ( cur == list->tail ) // last node
					list->tail = pre;
				pre->next = cur->next;
				}
			else { // first node
				if ( list->root == list->tail ) // the only one node
					list->tail = cur->next;
				list->root = cur->next;
				}
			free(cur->key);
			free(cur);
			break;
			}
		pre = cur;
		cur = cur->next;
		}
}

/*
 * find and returns node
 */
list_node_t *list_find(list_t *list, const char *key)
{
	for ( list_node_t *cur = list->root; cur; cur = cur->next )
		if ( strcmp(key, cur->key) == 0 )
			return cur;
	return NULL;
}

/*
 * find and returns a node by using regex
 */
list_node_t *list_find_re(list_t *list, regex_t *re)
{
	for ( list_node_t *cur = list->root; cur; cur = cur->next )
		if ( rex_match(re, cur->key) )
			return cur;
	return NULL;
}

/*
 * remove node from list
 * this removes regex compiled matches
 */
void list_remove_re(list_t *list, regex_t *re)
{
	list_node_t	*cur = list->root, *next;
	
	while ( cur ) {
		if ( rex_match(re, cur->key) ) {
			next = cur->next;
			list_remove(list, cur->key);
			cur = next;
			continue;
			}
		cur = cur->next;
		}
}

/*
 *	convert string list to string
 */
char *list_to_string(list_t *list, const char *delim)
{
	list_node_t *cur;
	int		count, size;
	char	*str;

	for ( cur = list->root, count = size = 0; cur; cur = cur->next, count ++ ) size += strlen(cur->key);
	str = (char *) malloc(size + strlen(delim) * count + 1);
	str[0] = '\0';
	for ( cur = list->root; cur; cur = cur->next ) {
		strcat(str, cur->key);
		if ( cur->next )
			strcat(str, delim);
		}
	return str;
}

/*
 * print outs the list
 */
void list_print(list_t *list, FILE *fp)
{
	list_node_t *cur = list->root;
	while ( cur ) {
		fprintf(fp, "%s: %s\n", cur->key, (char *) cur->data);
		cur = cur->next;
		}
}

/*
 * convert list to a newly allocated 1D string table
 */
char **list_to_str1D(list_t *list)
{
	list_node_t *cur;
	int	count;
	char **table;

	for ( cur = list->root, count = 0; cur; cur = cur->next ) count ++;
	table = (char **) malloc(sizeof(char *) * count);
	for ( cur = list->root, count = 0; cur; cur = cur->next, count ++ )
		table[count] = cur->key;
	return table;
}
