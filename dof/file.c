/*
 */

#include <stdio.h>
#include <glob.h>
#include "str.h"
#include "file.h"

/*
 * returns true if the "filename" has wildcards
 */
int has_wildcards(const char *filename)
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
int	read_conf(const char *appname, int (*parser)(char *))
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

