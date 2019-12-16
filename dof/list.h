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

#ifndef NDC_LIST_H_
#define NDC_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdlib.h>
#include "str.h"

typedef struct {
	char *key;
	void *data;
	void *next;
	} list_node_t;

typedef struct {
	int	flags;
	list_node_t *root;
	list_node_t *tail;
	} list_t;

list_t *list_init(list_t *list);
void list_clear(list_t *list);

#ifndef list_create
#define list_create()		list_init(NULL)
#define list_destroy(l)		{ list_clear(l); free(l); l = NULL; }
#endif

list_node_t *list_add (list_t *list, const char *key);
void list_remove(list_t *list, const char *key);
list_node_t *list_addp(list_t *list, const char *key, const char *value);
void list_delrec(list_t *list, regex_t *re);

char *list_to_string(list_t *list, const char *delim);

#define list_append(l,k)	list_add((l),(k))
#define list_del(l,k)		list_remove((l),(k))

#ifdef __cplusplus
}
#endif

#endif
