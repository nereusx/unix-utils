/*
 *
 */

#include <stdio.h>

enum crt_color_t {
  Black        =   0,
  Blue         =   1,
  Green        =   2,
  Cyan         =   3,
  Red          =   4,
  Magenta      =   5,
  Brown        =   6,
  LightGray    =   7,
  DarkGray     =   8,
  LightBlue    =   9,
  LightGreen   =  10,
  LightCyan    =  11,
  LightRed     =  12,
  LightMagenta =  13,
  Yellow       =  14,
  White        =  15,
};

void gotoxy(int x, int y)
{
	printf("\033[%d;%dH", y, x);
}

void clrscr()
{
	printf("\033[0m\033[2J\033[1;1H");
}

void textcolor(enum crt_color_t color)
{
	if ( color > 7 )
		printf("\033[1;%dm", color + 30);
	else
		printf("\033[22;%dm", color + 30);
}

void textbackground(enum crt_color_t color)
{
	if ( color > 7 )
		printf("\033[1;%dm", color + 40);
	else
		printf("\033[22;%dm", color + 40);
}

void delay(int ms)
{
	usleep(ms * 1000);
}

