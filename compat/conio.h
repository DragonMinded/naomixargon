// This file intentionally left blank.
#include <stdio.h>
#include <stdlib.h>

#define ESCAPE_CHARACTER 0x1B

#define clrscr() printf("%c[2J", ESCAPE_CHARACTER)
#define cputs(x) printf(x)

void textcolor(int color);
void textbackground(int color);
void gotoxy(int x, int y);
void window(int a, int b, int c, int d);
