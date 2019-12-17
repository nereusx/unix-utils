/*
 *	Error handling in C :P
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

#ifndef NDC_PANIC_H_
#define NDC_PANIC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void warning(const char *fmt, ...);
void error(const char *fmt, ...);
void panic(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
