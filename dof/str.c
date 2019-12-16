/*
 */

#include "str.h"

// create a new string
char *strnew(const char *source)
{
	if ( source )
		return strdup(source);
	return strdup("");
}

// delete a string
const char *strfree(char *ptr)
{
	free(ptr);
	return NULL;
}

// set string value
char *strset(char *base, const char *source)
{
	if ( base )
		free(base);
	if ( source )
		return strdup(source);
	return strdup("");
}

// append source to string base
char *stradd(char *base, const char *source)
{
	char *str = (char *) realloc(base, strlen(base) + strlen(source) + 1);
	strcat(str, source);
	return str;
}

// resize string
char *strsize(char *base, int newsize)
{
	char *str = (char *) realloc(base, newsize + 1);
	str[newsize] = '\0';
	return str;
}

// convert to lowercase
void strtolwr(char *buf)
{
	char *p = buf;
	
	while ( *p ) {
		*p = tolower(*p);
		p ++;
		}
}

// convert to uppercase
void strtoupr(char *buf)
{
	char *p = buf;
	
	while ( *p ) {
		*p = toupper(*p);
		p ++;
		}
}

// 
void strtotr(char *buf, char what, char with)
{
	char *p = buf;
	
	while ( *p ) {
		if ( *p == what )
			*p = with;
		p ++;
		}
}

//
void strtomtr(char *buf, const char *what, const char *with)
{
	const char *p = what;
	while ( *p ) {
		strtotr(buf, *p, with[p-what]);
		p ++;
		}
}

//
int cwords_add(cwords_t *list, const char *src)
{
	if ( list->count == list->alloc ) {
		list->alloc += 16;
		list->ptr = (const char **) realloc(list->ptr, list->alloc);
		}
	list->ptr[list->count ++] = src;
	return list->count;
}

//
cwords_t *cwords_create()
{
	cwords_t *list = (cwords_t *) malloc(sizeof(cwords_t));
	list->alloc = 16;
	list->count = 0;
	list->ptr = (const char **) malloc(sizeof(const char *) * list->alloc);
	return list;
}

//
const cwords_t *cwords_destroy(cwords_t *list)
{
	if ( list ) {
		if ( list->alloc )
			free(list->ptr);
		free(list);
		}
	return NULL;
}

//
cwords_t *cwords_clear(cwords_t *list)
{
	if ( list ) {
		if ( list->alloc )
			free(list->ptr);
		list->alloc = 0;
		}
	return list;
}

//
cwords_t *strtocwords(char *buf)
{
	char *p = buf;
	char *ps;
	cwords_t *list = cwords_create();
	
	while ( *p == ' ' || *p == '\t' ) p ++;
	ps = p;
	while ( *p ) {
		if ( *p == ' ' || *p == '\t' ) {
			*p ++ = '\0';
			cwords_add(list, ps);
			while ( *p == ' ' || *p == '\t' ) p ++;
			ps = p;
			}
		p ++;
		}
	if ( strlen(ps) )
		cwords_add(list, ps);
	return list;
}

//
const char *parse_num(const char *src, char *buf)
{
	const char *p = src;
	char *d = buf;
	while ( *p == ' ' || *p == '\t' ) p ++;
	if ( *p == '+' )	p ++;
	if ( *p == '-' )	*d ++ = *p ++;
	
	while ( isdigit(*p) )	*d ++ = *p ++;
	if ( p[0] == '.' && isdigit(p[1]) ) {
		*d ++ = *p ++;
		while ( isdigit(*p) )	*d ++ = *p ++;
		}
	*d = '\0';
	return p;
}

//
int match_regex(regex_t *r, const char *to_match)
{
	const char *p = to_match;
	const int n_matches = 1;
	regmatch_t m[n_matches];

	int nomatch = regexec(r, p, n_matches, m, 0);
	if ( nomatch )
		return 0;
	return 1;
}

