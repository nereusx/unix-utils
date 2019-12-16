/*
 */

#ifndef NDC_LIST_H_
#define NDC_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdlib.h>

typedef struct {
	char *key;
	char *data;
	void *next;
	} list_node_t;

typedef struct {
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
list_node_t *list_addp(list_t *list, const char *key, const char *value);
void list_addwc(list_t *list, const char *pattern);

char *list_to_string(list_t *list, const char *delim);

#ifdef __cplusplus
}
#endif

#endif
