/*
 *	Writes `yes' and returns exit-succeed, if it is connected to internet.
 *
 *	Copyright (C) 2017-2020 Free Software Foundation, Inc.
 *
 *	This is free software: you can redistribute it and/or modify it under
 *	the terms of the GNU General Public License as published by the Free
 *	Software Foundation, either version 3 of the License, or (at your
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
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>

int		opt_print = 0, opt_quiet = 0;

// calculating the checksum
unsigned short checksum(void *b, int len)
{
	unsigned short *buf = b;
	unsigned int sum = 0;
	unsigned short result;

	for ( sum = 0; len > 1; len -= 2 )
		sum += *buf++;
	
	if ( len == 1 )
		sum += *(unsigned char*)buf;
	
	sum  = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

//
int	ping_it(struct in_addr *dst)
{
    struct icmphdr icmp_hdr;
    struct sockaddr_in addr;
	unsigned char data[4096];
	int		rc, sock, msec;
	fd_set read_set;
	socklen_t slen;
	struct icmphdr rcv_hdr;
	struct timeval timeout = { 1, 0 }; // wait max 1 second for a reply

    sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_ICMP);
    if ( sock < 0 ) {
		perror("socket");
		return 2;
		}

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr = *dst;

    memset(&icmp_hdr, 0, sizeof(icmp_hdr));
    icmp_hdr.type = ICMP_ECHO;
    icmp_hdr.un.echo.id = getpid(); // an ID
	icmp_hdr.un.echo.sequence = 0;
	icmp_hdr.checksum = checksum(&icmp_hdr, sizeof(icmp_hdr)); // ICMP header + data (if any)
	
	// send
	rc = sendto(sock, &icmp_hdr, sizeof(icmp_hdr), 0, (struct sockaddr*)&addr, sizeof(addr));
	if ( rc <= 0 ) {
		perror("sendto");
		return 3;
		}

	// receive		
	memset(&read_set, 0, sizeof(read_set));
	FD_SET(sock, &read_set);

	// wait for a reply with a timeout
	rc = select(sock + 1, &read_set, NULL, NULL, &timeout);

	if ( rc < 0 ) { // error
		perror("select");
		return 4;
		}

	if ( rc == 0 ) // timeout
		return 1;

	// time of response
	msec = 1000 - ((timeout.tv_sec * 1000) + (timeout.tv_usec / 1000));
	if ( opt_print )
		printf("Get response in %d ms\n", msec);

	//
	slen = 0;
	rc = recvfrom(sock, data, sizeof(data), 0, NULL, &slen);
	if ( rc <= 0 ) {
		perror("recvfrom");
		return 5;
		}
	
	if ( rc < sizeof(rcv_hdr) ) {
		printf("Error, got short ICMP packet, %d bytes\n", rc);
		return 6;
		}
	
	memcpy(&rcv_hdr, data, sizeof(rcv_hdr));
	if ( rcv_hdr.type == ICMP_ECHOREPLY )
		return 0;
	
	printf("ICMP packet with type 0x%x ?!?\n", rcv_hdr.type);
	return 7;
}

// --------------------------------------------------------------------------

#define APP_DESCR \
"Returns the internet connection status."

static const char *helps = "\
Usage: isonline [-p|-q] [<ip-address>|<hostname>]\n\
\n"APP_DESCR"\n\n\
Options:\n\
\t-p\tdisplays additional information as IP of a host and the respond time\n\
\t-q\tquiet, displays nothing, just returns the exit status, 0 for online\n\
\t-h\tthis screen\n\
\t-v\tversion and program information\n\
\n\
\twith no arguments it just prints yes or no.\n\
";

static const char *verss = "\
isonline version 1.0\n\
\n"APP_DESCR"\n\
\n\
Copyright (C) 2017-2019 Free Software Foundation, Inc.\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
Written by Nicholas Christopoulos <mailto:nereus@freemail.gr>\n\
";

void print_version()	{ puts(verss); exit(1); }
void print_help()		{ puts(helps); exit(1); }

#define IF_OPT(p,s,l)	if((strcmp((p),(s))==0)||(strcmp((p),(l))==0))
int main(int argc, char *argv[]) 
{ 
    struct in_addr dst;
	int		i, rv, has_dest = 0;
	// gnu.org = 209.51.188.148, linux.org = 104.27.167.219, ntua = 147.102.222.210
	// cloudflare abuse 1.0.0.*
	static char *default_address = "1.0.0.1";

	for ( i = 1; i < argc; i ++ ) {
		if ( argv[i][0] == '-' ) {
			IF_OPT(argv[i], "-v", "--version")	print_version();
			IF_OPT(argv[i], "-h", "--help")		print_help();
			IF_OPT(argv[i], "-p", "--print")	opt_print = 1;
			IF_OPT(argv[i], "-q", "--quiet")	opt_quiet = 1;
			}
		else {
			if ( has_dest ) {
				printf("usage: isonline --help\n");
				return 1;
				}
			if ( isdigit(argv[i][0]) ) {
			    if ( inet_aton(argv[i], &dst) == 0 ) {
			        printf("%s isn't a valid IP address\n", argv[i]);
			        return 15;
					}
				}
			else {
				struct hostent *host_entity;
				
				if ( (host_entity = gethostbyname(argv[i])) == NULL )
					return 16;	// No ip found for hostname
				if ( opt_print )
					printf("IP %s\n", inet_ntoa(*(struct in_addr *) host_entity->h_addr));
			    dst = *(struct in_addr *) host_entity->h_addr;
				}
			has_dest ++;
			}
		}

	if ( !has_dest )
		inet_aton(default_address, &dst);

	//
	rv = ping_it(&dst);
	if ( !opt_quiet ) {
		if ( rv == 0 )
			printf("yes\n");
		else
			printf("no\n");
		}
	return rv;
} 

