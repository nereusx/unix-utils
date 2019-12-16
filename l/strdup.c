#include <stdlib.h>
#include <string.h>

/* clone a string */
char *strdup(const char *src)
{
	char *buf = malloc(strlen(src) + 1);
    strcpy(buf, src);
	return buf;
}

