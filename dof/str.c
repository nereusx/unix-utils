/*
 *	Additional C String library
 * 
 *	Copyright (C) 2017-2021 Nicholas Christopoulos.
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

#include <assert.h>
#include "str.h"

// append source to string base
char *stradd(char *base, const char *source)
{
	char *str = (char *) realloc(base, strlen(base) + strlen(source) + 1);
	strcat(str, source);
	return str;
}

// concatenate strings
char *concat(const char *s1, ...)
{
	va_list ap;
	const char *s;
	int len = strlen(s1) + 1;
	char *str = (char *) malloc(len);

	strcpy(str, s1);
	va_start(ap, s1);
	while ( (s = va_arg(ap, const char *)) != NULL ) {
		len += strlen(s);
		str = (char *) realloc(str, len);
		strcat(str, s);
		}
	va_end(ap);
	return str;
}

// returns 'count' bytes at 'index' position of 'source'
char *copy(const char *source, int index, int count)
{
	if ( count > 0 ) {
		char *str = (char *) malloc(count + 1);
		int len = strlen(source);
	
		if ( index >= len )
			str[0] = '\0';
		else {
			strncpy(str, source + index, count);
			str[count] = '\0';
			}
		return str;
		}
	if ( count == 0 ) return strdup("");
	assert(count >= 0);
	return NULL;
}

// returns the position of character c in string source or -1
int	pos(const char *source, char c)
{
	assert(source);
	for ( int i = 0; source[i]; i ++ )
		if ( source[i] == c )
			return i;
	return -1;
}

// returns the position of what string in source or -1
int	strpos(const char *source, const char *what)
{
	assert(source);
	char *p = strstr(source, what);
	if ( p )
		return p - source;
	return -1;
}

// inserts string in position pos of source and returns 
char *insert(const char *source, int pos, const char *string)
{
	int		len = strlen(source);
	
	char *str = (char *) malloc(len + strlen(string) + 1);
	if ( pos == 0 ) {
		strcpy(str, string);
		strcat(str, source);
		}
	else {
		strncpy(str, source, pos);
		str[pos] = '\0';
		strcat(str, string);
		if ( pos < len )
			strcat(str, source + pos);
		}
	return str;
}

// deletes count bytes of source at pos position
char *delete(const char *source, int pos, int count)
{
	int	len = strlen(source);
	assert(pos < len);
	if ( count < 0 )
		count = len;	
	if ( pos < len ) {
		char *str = (char *) malloc((len - count) + 1);
		if ( pos == 0 )
			strcpy(str, source + count);
		else {
			strncpy(str, source, pos);
			str[pos] = '\0';
			if ( pos + count < len )
				strcat(str, source + pos + count);
			}
		return str;
		}
	return NULL;
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
const char *parse_const(const char *src, const char *str)
{
	size_t	len = strlen(str);
	if ( strncmp(src, str, len) == 0 )
		return src + len;
	return NULL;
}

//
int rex_match(regex_t *r, const char *source)
{
	if ( regexec(r, source, 0, NULL, 0) == 0 )
		return 1;
	return 0;
}

//
int res_match(const char *pattern, const char *source)
{
	regex_t r;
	int status = regcomp(&r, pattern, REG_EXTENDED|REG_NEWLINE|REG_NOSUB);
	if ( status == 0 ) {
		status = regexec(&r, source, 0, NULL, 0);
		regfree(&r);
		return (status == 0);
		}
	return 0;
}

//
int rex_replace(regex_t *r, char *source, const char *repl, size_t max_matches)
{
	size_t max_groups = 1, m, beg, end, rplen = strlen(repl);
	regmatch_t groups[max_groups];
	char *p = source, *buf = (char *) malloc(strlen(source) + rplen * max_matches + 1);
	char *bp = buf;

	p = source;
	for ( m = 0; *p && m < max_matches; m ++ ) {
		if ( regexec(r, p, max_groups, groups, 0) )
			break;  // No more matches
		beg = groups[0].rm_so;
		end = groups[0].rm_eo;
		if ( beg ) {
			strncpy(bp, p, beg);
			bp += beg;
			}
		if ( rplen ) {
			strcpy(bp, repl);
			bp += rplen;
			}
		p += end;
		}
	*bp = '\0';
	strcat(bp, p);
	strcpy(source, buf);
	free(buf);
	return 1;
}

//
int res_replace(const char *pattern, char *source, const char *repl, size_t max_matches)
{
	regex_t r;
	int		status;

	if ( regcomp(&r, pattern, REG_EXTENDED|REG_NEWLINE) )
		return 0;
	status = rex_replace(&r, source, repl, max_matches);
	regfree(&r);
	return status;
}
