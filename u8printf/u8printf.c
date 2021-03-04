/*
 * printf utility with utf8 support
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
 * 	
 */

#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <locale.h>

// convert utf8 string to wchar string
wchar_t *u8towcs(const char *str) {
	size_t  wlen = mbstowcs(NULL, str, 0);
	wchar_t *wcs;

	if ( wlen == (size_t) -1 )
		wlen = 0;
	wcs = (wchar_t *) malloc(sizeof(wchar_t) * (wlen + 1));
	wcs[wlen] = L'\0';
	if ( wlen ) {
		if ( mbstowcs(wcs, str, wlen+1) == (size_t) -1 )
			wcs[0] = L'\0';
		}
	return wcs;
	}

// printf "%<width><stype>", text
void uprint(const char *stype, const char *width, const char *text) {
	int len = strlen(stype), type;
	char fmt[32];
	
	sprintf(fmt, "%%%s%s", width, stype);
	type = stype[len-1];
	switch ( type ) {
	case 'i': case 'd':
		if ( stype[0] == 'q' || (stype[0] == 'l' && stype[1] == 'l') ) {
			long long n = strtoll(text, NULL, 10);
			printf(fmt, n);
			}
		else if ( stype[0] == 'l' ) {
			long n = strtol(text, NULL, 10);
			printf(fmt, n);
			}
		else if ( stype[0] == 'h' && stype[1] == 'h' ) {
			char n = atoi(text);
			printf(fmt, n);
			}
		else if ( stype[0] == 'h' ) {
			short n = atoi(text);
			printf(fmt, n);
			}
		else {
			int n = atoi(text);
			printf(fmt, n);
			}
		break;
	case 'u': case 'x': case 'X': case 'o': case 'z': case 'Z':
		if ( stype[0] == 'q' || (stype[0] == 'l' && stype[1] == 'l') ) {
			unsigned long long n = strtoll(text, NULL, 10);
			printf(fmt, n);
			}
		else if ( stype[0] == 'l' ) {
			unsigned long n = strtol(text, NULL, 10);
			printf(fmt, n);
			}
		else if ( stype[0] == 'h' && stype[1] == 'h' ) {
			unsigned char n = atoi(text);
			printf(fmt, n);
			}
		else if ( stype[0] == 'h' ) {
			unsigned short n = atoi(text);
			printf(fmt, n);
			}
		else {
			unsigned int n = atoi(text);
			printf(fmt, n);
			}
		break;
	case 'f': case 'F':
	case 'e': case 'E': case 'g': case 'G':
	case 'a': case 'A':
		if ( stype[0] == 'l' || stype[0] == 'L' ) {
			long double f = strtold(text, NULL);
			printf(fmt, f);
			}
		else {
			double f = strtold(text, NULL);
			printf(fmt, f);
			}
		break;
	case 's': {
			if ( stype[0] != 'l' )
				sprintf(fmt, "%%%sls", width);
			wchar_t	*wcs = u8towcs(text);
			printf(fmt, wcs);
			free(wcs);
			}
		break;
		}
	}

#define APP_VER	"1.0"

#define APP_DESCR \
"printf utility with utf8 support"

static const char *usage = "\
Usage: u8printf format ...\n\
\n"APP_DESCR"\n\
";

static const char *verss = "\
path++ version "APP_VER"\n\
\n"APP_DESCR"\n\
\n\
Copyright (C) 2017-2021 Nicholas Christopoulos <mailto:nereus@freemail.gr>.\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
";

#define ARG_MAX	128

int main(int argc, char *argv[]) {
	int		idx = 0, count = 0;
	char	*aptr[ARG_MAX];	// pointers to arguments (not options)
	char	dest[16], *d;	// format prefix buffer
	char	cmd[16], *c;	// type buffer
	const char	*p;			// later, pointer to format

	setlocale(LC_ALL, "");
	for ( int i = 1; i < argc; i ++ ) {
		if ( argv[i][0] == '-' ) {	/* option, or error */
			for ( int j = 1; argv[i][j]; j ++ ) {
				switch ( argv[i][j] ) {
				case 'h': puts(usage); return 1;
				case 'v': puts(verss); return 1;
				default:  puts(usage); return 1;
					}
				}
			}
		else {
			aptr[count ++] = argv[i];
			if ( count == ARG_MAX ) {
				fprintf(stderr, "u8printf: maximum number of parameters reached.\n");
				return 1;
				}
			}
		}

	if ( count ) {
		p = aptr[idx ++];
		while ( *p ) {
			if ( *p == '%' ) {
				p ++;
				if ( *p == '%' )
					break;
				else {
					d = dest;
					c = cmd;
					while ( *p && !isalpha(*p) && d - dest < 16 )
						*d ++ = *p ++;
					while ( *p && strchr("lLhqz", *p) && c - cmd < 15 )
						*c ++ = *p ++;
					if ( *p )
						*c ++ = *p ++;
					*d = '\0';
					*c = '\0';
					
					// print the argument
					if ( idx < count ) 
						uprint(cmd, dest, aptr[idx ++]);
					}
				continue;
				}
			if ( *p == '\\' ) {
				p ++;
				switch ( *p ) {
				case 't': printf("\t"); p ++; break;
				case 'v': printf("\v"); p ++; break;
				case 'f': printf("\f"); p ++; break;
				case 'n': printf("\n"); p ++; break;
				case 'r': printf("\r"); p ++; break;
				case 'a': printf("\a"); p ++; break;
				case 'b': printf("\b"); p ++; break;
				case 'e': printf("\033"); p ++; break;
				case '0': case '1': case '2': case '3':
				case '4': case '5': case '6': case '7': { // octal
						int o = *p ++ - '0';
						if ( *p >= '0' && *p <= '7' ) {
							o = (o << 3) | (*p ++ - '0');
							if ( *p >= '0' && *p <= '7' )
								o = (o << 3) | (*p ++ - '0');
							}
						printf("%c", o);
						}
					break;
				case 'x': { // hexadecimal
						int n = 0;
						p ++;
						if ( isxdigit(p[0]) && isxdigit(p[1]) ) {
							n = (( *p >= 'A' ) ? (toupper(*p) - 'A') + 10 : *p - '0') << 4;
							p ++;
							n = n | (( *p >= 'A' ) ? (toupper(*p) - 'A') + 10 : *p - '0');
							p ++;
							printf("%c", n);
							}
						}
					break;
				default:
					printf("%c", *p ++);
					}
				continue;
				}
			printf("%c", *p ++);
			}
		}
	
	return 0;
	}

