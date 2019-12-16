/*
 *	File-related utilities
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

#include <stdio.h>
#include <glob.h>
#include "str.h"
#include "file.h"

#define IF_DOTS(s) if((s)[0]=='.' && ((s)[1]=='\0' || ((s)[1]=='.' && (s)[2]=='\0')))

/*
 * returns true if the "filename" has wildcards
 */
int iswcpat(const char *filename)
{
	const char *p = filename;
	
	while ( *p ) {
		if ( *p == '*' || *p == '?' || *p == '['
	#ifdef GLOB_TILDE
			 || *p == '~'
	#endif
	#ifdef GLOB_BRACE
			 || *p == '{'
	#endif
		   )
			return 1;
		p ++;
		}
	return 0;
}

/*
 * wildcard matches
 */
void wclist(const char *pattern, int (*callback)(const char *))
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
			IF_DOTS(name) continue;
			if ( callback(name) )
				break;
			}
		globfree(&globbuf);
		}
}

/*
 * returns the name of the file without the directory and the extension
 */
char *basename(const char *source)
{
	static char buf[PATH_MAX];
	char		*b;
	const char	*p;

	if ( (p = strrchr(source, '/')) != NULL )
		strcpy(buf, p + 1);
	else
		strcpy(buf, source);
	if ( (b = (char *) strrchr(buf, '.')) != NULL )
		*b = '\0';
	return buf;
}

/*
 * returns the directory of the file without the trailing '/'
 */
char *dirname(const char *source)
{
	static char buf[PATH_MAX];
	const char	*p;

	if ( (p = strrchr(source, '/')) != NULL ) {
		strcpy(buf, source);
		buf[p - source] = '\0';
		}
	else
		buf[0] = '\0';
	return buf;
}

/*
 * returns the extension of the file without the '.'
 */
char *extname(const char *source)
{
	static char buf[PATH_MAX];
	char		*b;
	const char	*p;

	if ( (p = strrchr(source, '/')) != NULL )
		strcpy(buf, p + 1);
	else
		strcpy(buf, source);
	if ( (b = (char *) strrchr(buf, '.')) != NULL )
		return b + 1;
	else
		buf[0] = '\0';
	return buf;
}

// return a pointer to filename without the directory
const char *filename(const char *file)
{
	const char *p;
	if ( (p = strrchr(file, '/')) != NULL )
		return p + 1;
	return file;
}

// read configuration files
int	readconf(const char *appname, int (*parser)(char *))
{
	FILE	*fp;
	char	buf[LINE_MAX], *p;
	char	file[PATH_MAX];
	int		retval = 0;
	
	for ( int i = 0; i < 2; i ++ ) {
		switch ( i ) {
		case 0: sprintf(file, "/etc/%s.conf", appname); break;
		case 1: sprintf(file, "%s/.%src", getenv("HOME"), appname); break;
			}
		if ( (fp = fopen(file, "rt")) != NULL ) {
			while ( fgets(buf, LINE_MAX, fp) ) {
				p = buf;
				while ( *p == ' ' || *p == '\t' ) p ++;
				if ( *p == '\n' || *p == '\0' || *p == '#' )
					continue;
				if ( buf[strlen(buf)-1] == '\n' )
					buf[strlen(buf)-1] = '\0';
				if ( (retval = parser(p)) != 0 )
					break;
				}
			fclose(fp);
			}
		}
	return retval;
}

