/*
 *	hd [<file>]
 *
 *	hex dump, dump files in hexadecimal format
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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define OFL_NL			0x01	// new line
#define OFL_SPC			0x02	// space after each byte
#define OFL_CHR			0x04	// character table
#define OFL_ADDR		0x08	// print address
int opt_flags = OFL_NL | OFL_SPC | OFL_CHR | OFL_ADDR;

typedef char * str_t;
#define MAX_FLS		128
str_t	files[MAX_FLS];
int		fl_count = 0;

// print line
void hexpl(int address, const char *source, int len) {
	if ( opt_flags & OFL_ADDR )
		printf("%08X: ", address);
	for ( int i = 0; i < len; i ++ ) {
		printf("%02X", source[i] & 0xFF);
		if ( opt_flags & OFL_SPC )
			printf(" ");
		}
	if ( opt_flags & OFL_CHR ) {
		printf("\t");
		for ( int j = 0; j < len; j ++ ) {
			if ( isprint(source[j]) )
				printf("%c", source[j] & 0xFF);
			else
				printf(".");
			}
		}
	if ( opt_flags & OFL_NL )
		printf("\n");
	}

// print file
void hexpf(FILE *fp) {
	char buf[16];
	int	n, address = 0;
	
	do {
		n = fread(buf, 1, 16, fp);
		if ( n > 0 )
			hexpl(address, buf, n);
		address += n;
		} while ( n == 16 );
	}

// --- main() ---

#define APP_DESCR \
"hd (hex-dump) dump files in hexadecimal format."

#define APP_VER "1.0"

static const char *usage = "\
Usage: hd [-s] [file]\n\
"APP_DESCR"\n\
\n\
Options:\n\
\t-p\tsimple string lines\n\
\t-s\tone-line string\n\
\t-\tread from stdin\n\
\t-h\tthis screen\n\
\t-v\tversion and program information\n\
";

static const char *verss = "\
dof version "APP_VER"\n\
"APP_DESCR"\n\
\n\
Copyright (C) 2017-2020 Free Software Foundation, Inc.\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
Written by Nicholas Christopoulos <mailto:nereus@freemail.gr>\n\
";

// main()
int main(int argc, char **argv) {
	int		i, j;

	// parsing arguments
	for ( i = 1; i < argc; i ++ ) {
		if ( argv[i][0] == '-' ) {

			if ( argv[i][1] == '\0' ) {	// one minus, read from stdin
				files[fl_count ++] = strdup("-");
				continue; // we finished with this argv
				}

			// check options
			for ( j = 1; argv[i][j]; j ++ ) {
				switch ( argv[i][j] ) {
				case 'p': opt_flags = OFL_NL; break;
				case 's': opt_flags = 0; break;
				case 'h': puts(usage); return 1;
				case 'v': puts(verss); return 1;
				case '-': // -- double minus
					if ( strcmp(argv[i], "--help") == 0 )    { puts(usage); return 1; }
					if ( strcmp(argv[i], "--version") == 0 ) { puts(verss); return 1; }
					return 1;
				default:
					fprintf(stderr, "unknown option [%c]", argv[i][j]);
					return 1;
					}
				}
			}
		else
			files[fl_count ++] = strdup(argv[i]);
		}

	//
	if ( fl_count == 0 )
		files[fl_count ++] = strdup("-"); // stdin

	//
	for ( i = 0; i < fl_count; i ++ ) {
		if ( files[i][0] == '-' )
			hexpf(stdin);
		else if ( access(files[i], R_OK) == 0 ) {
			printf("%s:\n", files[i]);
			FILE *fp = fopen(files[i], "rb");
			if ( fp ) {
				hexpf(fp);
				fclose(fp);
				}
			else
				fprintf(stderr, "cannot open %s\n", files[i]);
			}
		else
			fprintf(stderr, "cannot open %s\n", files[i]);
		}

	//
	return EXIT_SUCCESS;
	}
