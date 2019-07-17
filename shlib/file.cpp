/*
 * file utilities
 */

#include <cstdio>
#include <cstdlib>
#include "str.cpp"

/*
 * returns the filename without the path
 */
const char *basename(const char *s)
{
	const char *p = strrchr(s, '/');
	return ( p ) ? p + 1 : s;
}

/*
 * returns the filename extention or null
 */
const char *file_ext(const char *s)
{
	const char *p = strrchr(basename(s), '.');
	return ( p ) ? p : NULL;
}

/*
 * change file name extention
 */
str set_file_ext(const char *source, const char *newext = NULL)
{
	str s(source);
	char *p = strrchr(s.ptr(), '.');
	if ( p ) *p = '\0';
	if ( newext ) {
		char *ns = s.ptr();
		ns = (char *) malloc(s.len() + strlen(newext) + 1);
		strcpy(ns, s.ptr());
		strcat(ns, newext);
		s.set(ns);
		free(ns);
		}
	return s;
}

/*
 * executes a command and captures its output to string
 * returns the process exit code
 */
int	sexec(const char* cmd, str& output)
{
	char buffer[LINE_MAX];
    FILE *fp = popen(cmd, "r");
    if ( !fp ) return 1; // failled
	while ( fgets(buffer, LINE_MAX, fp) )
		output += buffer;
	return pclose(fp); // exit code, 0 = success
}
