#include <stddef.h>
#include <string.h>
#include <naomi/video.h>
#include <naomi/ta.h>
#include <naomi/thread.h>
#include <naomi/maple.h>
#include <naomi/interrupt.h>
#include <naomi/sprite/sprite.h>
#include "include/gr.h"

// Screen stuff.
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

// Palette fixup stuff.
#define color6to8(x) (((x) << 2) | ((x) >> 4))

// Parameters to control the video update thread, so we can pretend
// to be a VGA card that updates on register writes.
static int video_thread;
static int xargon_video_init = 0;
static texture_description_t *outtex;
static byte *outbuf[2];
static int uvsize = 0;
static int whichbuf = 0;
static float xscale;
static float yscale;
static int yoff;
static int debugxoff;

// Shared with input.c
extern int controls_needed;
extern int controls_available;

// Shared with main.c
extern mutex_t control_mutex;

// Shared with graphics.c
extern void *LOST;
extern int pixelsperbyte;
extern vptype mainvp;
void initcolortabs_vga(void);
void fontcolor_vga(int hi, int lo, int back);

void *video(void *param)
{
    while ( 1 )
    {
        // Draw the main screen if it is initialized.
        if (xargon_video_init)
        {
            ta_commit_begin();
            ATOMIC(ta_texture_load(outtex->vram_location, outtex->width, 8, outbuf[whichbuf]));
            sprite_draw_scaled(0, yoff, xscale, yscale, outtex);
            ta_commit_end();

            // Now, ask the TA to scale it for us
            ta_render();
        }

#ifdef NAOMI_DEBUG
        video_draw_debug_text(debugxoff, 20, rgb(200, 200, 20), "Video FPS: %.01f, %dx%d", video_thread_fps, video_width(), video_height());
        video_draw_debug_text(debugxoff, 30, rgb(200, 200, 20), "DOOM FPS: %.01f, %dx%d", doom_fps, SCREENWIDTH, SCREENHEIGHT);
        video_draw_debug_text(debugxoff, 40, rgb(200, 200, 20), "Audio Buf Empty: %.01f%%", percent_empty * 100.0);
        video_draw_debug_text(debugxoff, 50, rgb(200, 200, 20), "Music Volume: %d/15", m_volume);
        video_draw_debug_text(debugxoff, 60, rgb(200, 200, 20), "IRQs: %lu", sched.interruptions);
        video_updates ++;
#endif

        // Draw console and game graphics.
        video_display_on_vblank();
        
        // Now, poll for buttons where it is safe.
        if (controls_needed)
        {
            mutex_lock(&control_mutex);
            if (controls_needed)
            {
                controls_needed = 0;
                controls_available = 1;
                maple_poll_buttons();
            }
            mutex_unlock(&control_mutex);
        }
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
    // Set up stuff that the old code would have.
    mainvp.vpx = 0;
    mainvp.vpy = 0;
    mainvp.vpxl = SCREEN_WIDTH;
    mainvp.vpyl = SCREEN_HEIGHT;
    mainvp.vpox = 0;
    mainvp.vpoy = 0;
    pixelsperbyte = 1;
    LOST = malloc(1);

    // Create a texture that we can use to render to to use hardware stretching.
    uvsize = ta_round_uvsize(mainvp.vpxl > mainvp.vpyl ? mainvp.vpxl : mainvp.vpyl);
    outtex = ta_texture_desc_malloc_paletted(uvsize, NULL, TA_PALETTE_CLUT8, 0);
    outbuf[0] = malloc(uvsize * uvsize);
    outbuf[1] = malloc(uvsize * uvsize);
    whichbuf = 0;

    // Wipe the textures so we don't have garbage on them.
    memset(outbuf[0], 0, uvsize * uvsize);
    memset(outbuf[1], 0, uvsize * uvsize);

    // Calculate the scaling factors and y offset. This is based off
    // of the assumption that doom wants to be stretched to a 4:3 resolution.
    if (video_is_vertical())
    {
        float yheight = ((float)video_width() * 3.0) / 4.0;
        xscale = (float)video_width() / (float)mainvp.vpxl;
        yscale = yheight / (float)mainvp.vpyl;
        yoff = (video_height() - (int)yheight) / 2;
        debugxoff = 20;
    }
    else
    {
        xscale = (float)video_width() / (float)mainvp.vpxl;
        yscale = (float)video_height() / (float)mainvp.vpyl;
        yoff = 0;
        debugxoff = 400;
    }

    // Set initial palette.
    initcolortabs_vga();
    fontcolor_vga (0x2a,0x22,0);
    vga_setpal();

    // Mark that the video thread has something to display.
    xargon_video_init = 1;
}

void gr_exit()
{
    xargon_video_init = 0;
}

void scrollvp (vptype *vp, int xd, int yd)
{
    // TODO
}

void scroll (vptype *vp, int x0, int y0, int x1, int y1, int xd, int yd)
{
    // TODO
}

void clrvp (vptype *vp, byte col)
{
    // If we are in double-buffered mode, draw on the back buffer. Otherwise,
    // draw directly to the screen buffer.
    int buf = pagemode ? (1 - whichbuf) : whichbuf;
    for (int yj = vp->vpy; yj < vp->vpy + vp->vpyl; yj++)
    {
        for (int xi = vp->vpx; xi < vp->vpx + vp->vpxl; xi++)
        {
            outbuf[buf][xi + (yj * uvsize)] = col;
        }
    }
}

void clrpal ()
{
    uint32_t *bank = ta_palette_bank(TA_PALETTE_CLUT8, 0);
    color_t color;

    color.r = 0;
    color.g = 0;
    color.b = 0;
    color.a = 255;

    for (int i = 0; i < 256; i++)
    {
        bank[i] = ta_palette_entry(color);
    }
}

void pageflip ()
{
    // We only need to do a flip if we are double-buffering.
    if (pagemode)
    {
        ATOMIC(whichbuf = 1 - whichbuf);
    }
}

void vga_setpal(void)
{
    uint32_t *bank = ta_palette_bank(TA_PALETTE_CLUT8, 0);

    for (int i = 0; i < 256; i++)
    {
        color_t color;

        color.r = color6to8(vgapal[(i * 3) + Red]);
        color.g = color6to8(vgapal[(i * 3) + Green]);
        color.b = color6to8(vgapal[(i * 3) + Blue]);
        color.a = 255;

        bank[i] = ta_palette_entry(color);
    }
}

void ldrawsh_vga (vptype *vp, int xpos, int ypos, int width, int height, char far *shape, int cmtable)
{
    // If we are in double-buffered mode, draw on the back buffer. Otherwise,
    // draw directly to the screen buffer.
    int buf = pagemode ? (1 - whichbuf) : whichbuf;
    for (int yj = 0; yj < height; yj++)
    {
        int actual_y = vp->vpy + ypos + yj;
        if (actual_y < vp->vpy) { continue; }
        if (actual_y >= (vp->vpy + vp->vpyl)) { break; }
        if (actual_y >= uvsize) { break; }

        for (int xi = 0; xi < width; xi++)
        {
            int actual_x = + vp->vpx + xpos + xi;
            if (actual_x < vp->vpx) { continue; }
            if (actual_x >= (vp->vpx + vp->vpxl)) { break; }
            if (actual_x >= uvsize) { break; }

            // Look up actual palette index, but index 255 is transparent.
            uint8_t pixel = shape[xi + (width * yj)];
            uint8_t palindex = cmtab[cmtable][pixel];
            if (palindex == 255) { continue; }

            outbuf[buf][actual_x + (actual_y * uvsize)] = palindex;
        }
    }
}

void fadein(void)
{
    p_rec currentpal;

    for (int cycle = 0; cycle < 64; cycle += 2)
    {
        for (int i = 0; i < (256 * 3); i++)
        {
            int temp = vgapal[i];
            temp = (temp * cycle) >> 6;
            currentpal[i] = temp;
        }

        thread_wait_vblank_in();
        uint32_t *bank = ta_palette_bank(TA_PALETTE_CLUT8, 0);

        for (int i = 0; i < 256; i++)
        {
            color_t color;

            color.r = color6to8(currentpal[(i * 3) + Red]);
            color.g = color6to8(currentpal[(i * 3) + Green]);
            color.b = color6to8(currentpal[(i * 3) + Blue]);
            color.a = 255;

            bank[i] = ta_palette_entry(color);
        }
    }
}

void fadeout(void)
{
    p_rec currentpal;

    for (int cycle = 63; cycle >= 0; cycle -= 2)
    {
        for (int i = 0; i < (256 * 3); i++)
        {
            int temp = vgapal[i];
            temp = (temp * cycle) >> 6;
            currentpal[i] = temp;
        }

        thread_wait_vblank_in();
        uint32_t *bank = ta_palette_bank(TA_PALETTE_CLUT8, 0);

        for (int i = 0; i < 256; i++)
        {
            color_t color;

            color.r = color6to8(currentpal[(i * 3) + Red]);
            color.g = color6to8(currentpal[(i * 3) + Green]);
            color.b = color6to8(currentpal[(i * 3) + Blue]);
            color.a = 255;

            bank[i] = ta_palette_entry(color);
        }
    }
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
    if (mode)
    {
        // Must copy current buffer to the alt one.
        memcpy(outbuf[1 - whichbuf], outbuf[whichbuf], uvsize * uvsize);
        pagemode = 1;
    }
    else
    {
        pagemode = 0;
    }
}

void setpages ()
{
    // I think its safe to leave this blank.
}

void setcolor (int c, int n1, int n2, int n3)
{
    uint32_t *bank = ta_palette_bank(TA_PALETTE_CLUT8, 0);

    color_t color;

    color.r = color6to8(n1);
    color.g = color6to8(n2);
    color.b = color6to8(n3);
    color.a = 255;

    bank[c] = ta_palette_entry(color);
}
