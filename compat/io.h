#include <stdio.h>
#include <unistd.h>

#define _open open
#define _close close
#define _read read
#define _write write
#define _creat creat
