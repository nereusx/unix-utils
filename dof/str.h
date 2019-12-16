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
#include <stdarg.h>
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

char *stradd(char *str, const char *source);

// pascaloids
char *concat(const char *s1, ...);
char *copy(const char *source, int index, int count);
int	pos(const char *source, char c);
int	strpos(const char *source, const char *what);
char *insert(const char *source, int pos, const char *string);
char *delete(const char *source, int pos, int count);	

//
#define strtotr(b,s,r)\
	{ for(char *p=(b); *p; p ++) if (*p == (s)) *p = (r); }
#define strtomtr(b,s,r)\
	{ for(const char *p=(s); *p; p ++) strtotr((b), *p, (r)[p-(s)]); }

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
const char *parse_const(const char *src, const char *str);

#ifdef __cplusplus
}
#endif
	
#endif

