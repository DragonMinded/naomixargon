// This file intentionally left blank.
#include <stdio.h>
#include <stdlib.h>

#define ESCAPE_CHARACTER 0x1B

#define clrscr() printf("%c[2J", ESCAPE_CHARACTER)
#define cputs(x) printf(x)
