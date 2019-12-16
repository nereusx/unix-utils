/*
 */

#ifndef NDC_STR_H_
#define NDC_STR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <regex.h>

/*
 *	constant words list
 */
typedef struct {
	const char **ptr;	// table of words
	int	count;			// number of items
	int	alloc;			// allocation size (used for realloc)
	} cwords_t;

char *strnew(const char *source);
const char *strfree(char *ptr);
char *strset(char *base, const char *source);
char *stradd(char *base, const char *source);
char *strsize(char *base, int newsize);
void strtolwr(char *buf);
void strtoupr(char *buf);
void strtotr(char *buf, char what, char with);
void strtomtr(char *buf, const char *what, const char *with);

// constant list of words
cwords_t *cwords_create();
cwords_t *cwords_clear(cwords_t *list);
const cwords_t *cwords_destroy(cwords_t *list);
int cwords_add(cwords_t *list, const char *src);
cwords_t *strtocwords(char *buf);

// regex
int match_regex(regex_t *r, const char *to_match);

// parsing
const char *parse_num(const char *src, char *buf);

#ifdef __cplusplus
}
#endif
	
#endif

