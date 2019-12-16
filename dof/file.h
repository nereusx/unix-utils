/*
 */

#ifndef NDC_FILE_H_
#define NDC_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <fnmatch.h>
	
int has_wildcards(const char *filename);
char *basename(const char *source);
const char *filename(const char *source);
char *dirname(const char *source);
char *extname(const char *source);

int	read_conf(const char *appname, int (*parser)(char *));

#ifdef __cplusplus
}
#endif

#endif

