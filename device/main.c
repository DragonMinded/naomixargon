#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <naomi/video.h>
#include <naomi/console.h>
#include <naomi/thread.h>
#include <naomi/romfs.h>
#include <naomi/tmpfs/tmpfs.h>

// Defined in xargon.c
void xargon_main (int argc, char *argv[]);

// Defined in video.c
void video_thread_start();
void video_thread_end();

// To share the load between the video thread (polling controls) and the main
// thread (reacting to polled controls).
mutex_t control_mutex;

// To allow episode selection within a single binary instead of having 3 separate executables.
char *xshafile;
char *xvocfile;
char *cfgfname;
char *ext;
char *tilefile;
char *xintrosong;
char *demoboard[1];
char demolvl[1];
char *demoname[1];
char *v_msg;
char *fidgetmsg[4];
char *leveltxt[16];
static int episode = 0;

// Wait functions in various episodes.
void wait1(void);
void wait2(void);
void wait3(void);

void wait()
{
    switch(episode)
    {
        case 1:
            wait1();
            break;
        case 2:
            wait2();
            break;
        case 3:
            wait3();
            break;
    }
}

// Selecct episode 1 contents.
extern char xshafile1[],xvocfile1[],cfgfname1[],ext1[],xintrosong1[];
extern char v_msg1[];
extern char *demoboard1[1];
extern char demolvl1[1];
extern char *demoname1[1];
extern char tilefile1[];
extern char *leveltxt1[16];
extern char *fidgetmsg1[4];

void episode_1()
{
    xshafile = xshafile1;
    xvocfile = xvocfile1;
    cfgfname = cfgfname1;
    ext = ext1;
    tilefile = tilefile1;
    xintrosong = xintrosong1;

    demoboard[0] = demoboard1[0];
    demolvl[0] = demolvl1[0];
    demoname[0] = demoname1[0];

    v_msg = v_msg1;

    for (int i = 0; i < 4; i++)
    {
        fidgetmsg[i] = fidgetmsg1[i];
    }

    for (int i = 0; i < 16; i++)
    {
        leveltxt[i] = leveltxt1[i];
    }

    episode = 1;
}

// Selecct episode 2 contents.
extern char xshafile2[],xvocfile2[],cfgfname2[],ext2[],xintrosong2[];
extern char v_msg2[];
extern char *demoboard2[1];
extern char demolvl2[1];
extern char *demoname2[1];
extern char tilefile2[];
extern char *leveltxt2[16];
extern char *fidgetmsg2[4];

void episode_2()
{
    xshafile = xshafile2;
    xvocfile = xvocfile2;
    cfgfname = cfgfname2;
    ext = ext2;
    tilefile = tilefile2;
    xintrosong = xintrosong2;

    demoboard[0] = demoboard2[0];
    demolvl[0] = demolvl2[0];
    demoname[0] = demoname2[0];

    v_msg = v_msg2;

    for (int i = 0; i < 4; i++)
    {
        fidgetmsg[i] = fidgetmsg2[i];
    }

    for (int i = 0; i < 16; i++)
    {
        leveltxt[i] = leveltxt2[i];
    }

    episode = 2;
}

// Selecct episode 3 contents.
extern char xshafile3[],xvocfile3[],cfgfname3[],ext3[],xintrosong3[];
extern char v_msg3[];
extern char *demoboard3[1];
extern char demolvl3[1];
extern char *demoname3[1];
extern char tilefile3[];
extern char *leveltxt3[16];
extern char *fidgetmsg3[4];

void episode_3()
{
    xshafile = xshafile3;
    xvocfile = xvocfile3;
    cfgfname = cfgfname3;
    ext = ext3;
    tilefile = tilefile3;
    xintrosong = xintrosong3;

    demoboard[0] = demoboard3[0];
    demolvl[0] = demolvl3[0];
    demoname[0] = demoname3[0];

    v_msg = v_msg3;

    for (int i = 0; i < 4; i++)
    {
        fidgetmsg[i] = fidgetmsg3[i];
    }

    for (int i = 0; i < 16; i++)
    {
        leveltxt[i] = leveltxt3[i];
    }

    episode = 3;
}

void main()
{
    // Fake arguments for the original main.
    char *argv[] = { "xargon", NULL };

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

    // Init TMPFS so the game has a place to store temp files.
    tmpfs_init_default(0, 256 * 1024);

    // Select episode.
    episode_1();

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
    // TODO: Calculate how much memory is left
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

int _creat(char * name, int mode)
{
    // Create a file, with the given mode. Xargon seems to only ever pass "0" as the mode so
    // we don't really worry about it.
    return open(name, O_CREAT|O_WRONLY|O_TRUNC, mode);
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
