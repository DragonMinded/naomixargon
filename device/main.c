#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <naomi/video.h>
#include <naomi/console.h>
#include <naomi/thread.h>
#include <naomi/romfs.h>

// Defined in xargon.c
void xargon_main (int argc, char *argv[]);

// Defined in video.c
void video_thread_start();
void video_thread_end();

// To share the load between the video thread (polling controls) and the main
// thread (reacting to polled controls).
mutex_t control_mutex;

void main()
{
    // Fake arguments for the original main.
    char *argv[] = { "xargon", "/NOSND", NULL };

    // Make sure we have a mutex for control input ready.
    mutex_init(&control_mutex);

    // Set up video so we get at least something on the screen even on failure.
    video_init(VIDEO_COLOR_1555);
    video_set_background_color(rgb(0, 0, 0));
    console_init(16);

    // Set up the video thread so that even if we don't get to the main game loop we still
    // see console messages.
    video_thread_start();

    // Init ROMFS so we can find files.
    romfs_init_default();

    // Now, run the main executable.
    xargon_main((sizeof(argv) / sizeof(argv[0])) - 1, argv);

    // Display an error on getting here.
    video_thread_end();
    console_set_visible(0);
    video_set_background_color(rgb(48, 0, 0));

    // Should never happen!
    video_draw_debug_text(16, 16, rgb(255, 255, 255), "Program unexpectedly exit!\n");
    video_display_on_vblank();

    while ( 1 ) { ; }
}

int getclock()
{
    struct timeval time;
    if (gettimeofday(&time, NULL) == 0)
    {
        uint64_t us = (((uint64_t)time.tv_sec) * 1000000) + time.tv_usec;

        return us / 54924;
    }

    return 0;
}

int coreleft()
{
    // TODO: How much memory is left
    return 8 * 1024 * 1024;
}

int randomrange(int max)
{
    return (int)(((double)rand() / (double)RAND_MAX) * (double)max);
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
    // Sleep for amount milliseconds.
    thread_sleep(amount * 1000);
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
