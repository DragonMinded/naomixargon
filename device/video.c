#include <stddef.h>
#include <naomi/video.h>
#include <naomi/thread.h>
#include "include/gr.h"

static int video_thread;
static int xargon_video_init = 0;

void *video(void *param)
{
    while ( 1 )
    {
        video_display_on_vblank();
    }
}

void video_thread_start()
{
    video_thread = thread_create("video", video, NULL);
    thread_priority(video_thread, 1);
    thread_start(video_thread);
}

void video_thread_end()
{
    thread_stop(video_thread);
    thread_destroy(video_thread);
}

void gr_init()
{
    xargon_video_init = 1;
}

void gr_exit()
{
    xargon_video_init = 0;
}

void scrollvp (vptype *vp, int xd, int yd)
{

}

void scroll (vptype *vp, int x0, int y0, int x1, int y1, int xd, int yd)
{

}

void clrvp (vptype *vp, byte col)
{

}

void clrpal ()
{

}

void pageflip ()
{
    // TODO
}

void vga_setpal(void)
{
    // TODO
}

void ldrawsh_vga (vptype *vp, int xpos, int ypos, int width, int height, char far *shape, int cmtable)
{
    // TODO
}

void fntcolor (int hi,int lo,int back)
{
    // TODO
}

void fadein(void)
{
    // TODO
}

void fadeout(void)
{
    // TODO
}

void dim(void)
{
    // TODO
}

void undim(void)
{
    // TODO
}

void uncrunch (char far *sourceptr,char far *destptr,int length)
{
    // TODO
}

void setpagemode (int mode)
{
    // TODO
}

void setpages ()
{
    // TODO
}

void setcolor (int c, int n1, int n2, int n3)
{
    // TODO
}
