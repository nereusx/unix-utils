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
	{ for(char *_=(b); *_; _ ++) if (*_ == (s)) *_ = (r); }
#define strtomtr(b,s,r)\
	{ for(const char *__=(s); *__; __ ++) strtotr((b), *__, (r)[__-(s)]); }

// constant list of words
cwords_t *cwords_create();
cwords_t *cwords_clear(cwords_t *list);
const cwords_t *cwords_destroy(cwords_t *list);
int cwords_add(cwords_t *list, const char *src);
cwords_t *strtocwords(char *buf);

// regex
int res_match(const char *pattern, const char *source);
int rex_match(regex_t *r, const char *source);
int res_replace(const char *pattern, char *source, const char *repl, size_t max_matches);
int rex_replace(regex_t *r, char *source, const char *repl, size_t max_matches);

// parsing
const char *parse_num(const char *src, char *buf);
const char *parse_const(const char *src, const char *str);

#ifdef __cplusplus
}
#endif
	
#endif

