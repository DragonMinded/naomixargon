#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <naomi/video.h>
#include <naomi/console.h>

// Defined in xargon.c
void xargon_main (int argc, char *argv[]);

// Defined in video.c
void video_thread_start();
void video_thread_end();

void main()
{
    // Fake arguments for the original main.
    char *argv[] = { "xargon", "/NOSND", NULL };

    // Set up video so we get at least something on the screen even on failure.
    video_init(VIDEO_COLOR_1555);
    console_init(16);

    // Set up the video thread so that even if we don't get to the main game loop we still
    // see console messages.
    video_thread_start();

    // Now, run the main executable.
    xargon_main((sizeof(argv) / sizeof(argv[0])) - 1, argv);

    // TODO: Display an error on getting here.

    while( 1 ) { ; }
}

int coreleft()
{
    // TODO: How much memory is left
    return 8 * 1024 * 1024;
}

int randomrange(int max)
{
    return random() * max;
}

char *ltoa(long int l, char * out, int outlen)
{
    sprintf(out, "%ld", l);
    return out;
}

char *ultoa(unsigned long int l, char * out, int outlen)
{
    sprintf(out, "%lu", l);
    return out;
}

int min(int a, int b)
{
    return a < b ? a : b;
}

int max(int a, int b)
{
    return a > b ? a : b;
}

void setmem(void *dst, unsigned long len, char val)
{
    memset(dst, val, len);
}

unsigned long filelength(int fd)
{
    struct stat buf;
    fstat(fd, &buf);
    return buf.st_size;
}

void delay(int amount)
{
    // Blank
}

void enable()
{
    // Blank
}

void disable()
{
    // Blank
}

// TODO: Investigate having to implement this
int _creat(char * name, int mode)
{
    return 0;
}

int outportb(int thing)
{
    // Blank
    assert(0);
}

int inportb()
{
    // Blank
    assert(0);
}
