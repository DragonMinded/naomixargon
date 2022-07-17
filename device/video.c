#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <naomi/video.h>
#include <naomi/ta.h>
#include <naomi/thread.h>
#include <naomi/maple.h>
#include <naomi/timer.h>
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
static texture_description_t *outtex[2];
static byte *outbuf[2];
static int uvsize = 0;
static int drawbuf = 0;
static float xscale;
static float yscale;
static int yoff;
static int debugxoff = -1;
static int debugyoff = -1;
static int xargon_updates = 0;

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
    static int xargon_last_frame = -1;

#ifdef NAOMI_DEBUG
    double video_thread_fps = 0.0;
    double xargon_fps = 0.0;
    uint32_t elapsed = 0;
    int video_updates = 0;
    int xargon_last_reset = 0;
    task_scheduler_info_t sched;
    char task_info[2048] = "";
#endif

    while ( 1 )
    {
#ifdef NAOMI_DEBUG
        // Calculate instantaneous video thread FPS, should hover around 60 FPS.
        int fps = profile_start();
#endif

        // Draw the main screen if it is initialized.
        int drawn = 0;
        if (xargon_video_init)
        {
            if (!pagemode)
            {
                // Emulating direct-draw, so we need to refresh every update. We also need to draw directly
                // from the drawbuf instead of the backbuffer.
                ta_texture_load(outtex[drawbuf]->vram_location, outtex[drawbuf]->width, 8, outbuf[drawbuf]);
                ta_commit_begin();
                sprite_draw_scaled(0, yoff, xscale, yscale, outtex[drawbuf]);
                ta_commit_end();

                // We drew this frame.
                drawn = 1;
            }
            else
            {
                int needs_draw;

                ATOMIC({
                    needs_draw = xargon_last_frame != xargon_updates;
                    xargon_last_frame = xargon_updates;
                });

                if (needs_draw)
                {
                    // We need to draw from the back buffer since it was flipped.
                    ta_commit_begin();
                    sprite_draw_scaled(0, yoff, xscale, yscale, outtex[1 - drawbuf]);
                    ta_commit_end();

                    drawn = 1;
                }
            }

            if (drawn)
            {
                // Now, ask the TA to scale it for us
                ta_render();
            }
        }

#ifdef NAOMI_DEBUG
        if (drawn && debugxoff >= 0)
        {
            video_draw_debug_text(debugxoff, debugyoff + 0, rgb(0xFF, 0x6D, 0x0A), "Video FPS: %.01f, %dx%d", video_thread_fps, video_width(), video_height());
            video_draw_debug_text(debugxoff, debugyoff + 10, rgb(0xFF, 0x6D, 0x0A), "Xargon FPS: %.01f, %dx%d", xargon_fps, SCREEN_WIDTH, SCREEN_HEIGHT);
            video_draw_debug_text(debugxoff, debugyoff + 20, rgb(0xFF, 0x6D, 0x0A), "Page Mode: %s", pagemode ? "double buffered" : "single buffered");
            video_draw_debug_text(debugxoff, debugyoff + 30, rgb(0xFF, 0x6D, 0x0A), "IRQs: %lu", sched.interruptions);
            video_draw_debug_text(debugxoff, debugyoff + 40, rgb(0xFF, 0x6D, 0x0A), task_info);
            video_updates ++;
        }
#endif

        if (drawn)
        {
            // Draw console and game graphics.
            video_display_on_vblank();
        }
        else
        {
            // Just wait for a refresh.
            thread_wait_vblank_in();
        }
        
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

#ifdef NAOMI_DEBUG
        // Calculate instantaneous FPS.
        uint32_t uspf = profile_end(fps);
        video_thread_fps = 1000000.0 / (double)uspf;

        // Calculate Xargon and system FPS based on requested screen updates.
        elapsed += uspf;
        if (elapsed >= 1000000)
        {
            int frame_count;
            ATOMIC({
                frame_count = (xargon_updates - xargon_last_reset);
                xargon_last_reset = xargon_updates;
            });

            xargon_fps = (double)frame_count * ((double)elapsed / 1000000.0);
            elapsed = 0;
        }

        // Get task schduler info.
        task_scheduler_info(&sched);

        // Get info about various threads for performance tuning.
        task_info[0] = 0;
        for (int t = 0; t < sched.num_threads; t++)
        {
            thread_info_t info;
            if (thread_info(sched.thread_ids[t], &info))
            {
                char tmpBuf[1024];
                sprintf(tmpBuf, "%s: %0.1f%% CPU, %s\n", info.name, info.cpu_percentage * 100.0, info.running ? "running" : "parked");
                strcat(task_info, tmpBuf);
            }
        }
#endif
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

    outtex[0] = ta_texture_desc_malloc_paletted(uvsize, NULL, TA_PALETTE_CLUT8, 0);
    outtex[1] = ta_texture_desc_malloc_paletted(uvsize, NULL, TA_PALETTE_CLUT8, 0);
    outbuf[0] = malloc(uvsize * uvsize);
    outbuf[1] = malloc(uvsize * uvsize);

    // Wipe the textures so we don't have garbage on them.
    memset(outbuf[0], 0, uvsize * uvsize);
    memset(outbuf[1], 0, uvsize * uvsize);
    ta_texture_load(outtex[0]->vram_location, outtex[0]->width, 8, outbuf[0]);
    ta_texture_load(outtex[1]->vram_location, outtex[1]->width, 8, outbuf[1]);

    // Calculate the scaling factors and y offset. This is based off
    // of the assumption that xargon wants to be stretched to a 4:3 resolution.
    if (video_is_vertical())
    {
        float yheight = ((float)video_width() * 3.0) / 4.0;
        xscale = (float)video_width() / (float)mainvp.vpxl;
        yscale = yheight / (float)mainvp.vpyl;
        yoff = (video_height() - (int)yheight) / 2;
        debugxoff = 20;
        debugyoff = 20;
    }
    else
    {
        xscale = (float)video_width() / (float)mainvp.vpxl;
        yscale = (float)video_height() / (float)mainvp.vpyl;
        yoff = 0;
        debugxoff = 400;
        debugyoff = video_height() - (20 + (10 * 8));
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
    int xsrc, xdst, xamt;
    int ydir, ysrc, ydst, ystart, yend;

    if (xd >= 0)
    {
        // Going forwards, starting at offset 0, moving forward xd pixels,
        // and going for as many pixels as we are wide minus the offset.
        xsrc = vp->vpx + 0;
        xdst = vp->vpx + xd;
        xamt = vp->vpxl - xd;
    }
    else
    {
        // Going backwrds, starting at offset -xd, moving backwards xd pixels,
        // and going for as many pixels as we are wide minus the offset.
        xsrc = vp->vpx - xd;
        xdst = vp->vpx + 0;
        xamt = vp->vpxl + xd;
    }

    if (yd >= 0)
    {
        // Going backwards, starting at offset 0, moving forward yd pixels,
        // and going for as many pixels as we are tall minus the offset.
        ydir = -1;
        ysrc = vp->vpy + 0;
        ydst = vp->vpy + yd;
        ystart = vp->vpyl - yd - 1;
        yend = -1;
    }
    else
    {
        // Going forwards, starting at offset -yd, moving backwards yd pixels,
        // and going for as many pixels as we are tall minus the offset.
        ydir = 1;
        ysrc = vp->vpy - yd;
        ydst = vp->vpy + 0;
        ystart = 0;
        yend = vp->vpyl + yd;
    }

    for (int yoff = ystart; yoff != yend; yoff += ydir)
    {
        memmove(&outbuf[drawbuf][((ydst + yoff) * uvsize) + xdst], &outbuf[drawbuf][((ysrc + yoff) * uvsize) + xsrc], xamt);
    }
}

void scroll (vptype *vp, int x0, int y0, int x1, int y1, int xd, int yd)
{
    // TODO
}

void clrvp (vptype *vp, byte col)
{
    for (int yj = vp->vpy; yj < vp->vpy + vp->vpyl; yj++)
    {
        for (int xi = vp->vpx; xi < vp->vpx + vp->vpxl; xi++)
        {
            outbuf[drawbuf][xi + (yj * uvsize)] = col;
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

    thread_wait_vblank_in();
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
        // Load the texture itself.
        ta_texture_load(outtex[drawbuf]->vram_location, outtex[drawbuf]->width, 8, outbuf[drawbuf]);

        // Flip the page.
        ATOMIC({
            drawbuf = 1 - drawbuf;
            xargon_updates++;
        });
    }
    else
    {
        // Make sure we can calculate FPS in debug mode.
        ATOMIC(xargon_updates++);
    }

    // Force a vblank, otherwise two pageflips between a single display will net us the wrong screen update.
    thread_wait_vblank_in();
}

void vga_setpal(void)
{
    uint32_t *bank = ta_palette_bank(TA_PALETTE_CLUT8, 0);

    thread_wait_vblank_in();
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

            outbuf[drawbuf][actual_x + (actual_y * uvsize)] = palindex;
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
        // Must copy current buffer to the alt one, so that it's there for us
        // next frame.
        pagemode = 1;
    }
    else
    {
        if (pagemode != 0)
        {
            // Copy to the other buffer, so its available for the next blit.
            memcpy(outbuf[drawbuf], outbuf[1 - drawbuf], uvsize * uvsize);
        }

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
