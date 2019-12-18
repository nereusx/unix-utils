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

#define isdots(s) ((s)[0]=='.' && ((s)[1]=='\0' || ((s)[1]=='.' && (s)[2]=='\0')))

int		iswcpat(const char *filename);
const char *basename(const char *source);
const char *filename(const char *source);
const char *dirname(const char *source);
const char *extname(const char *source);

void	wclist(const char *pattern, int (*callback)(const char *));
int		readconf(const char *appname, int (*parser)(char *));

#ifdef __cplusplus
}
#endif

#endif

