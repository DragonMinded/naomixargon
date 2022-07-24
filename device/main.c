#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <naomi/video.h>
#include <naomi/ta.h>
#include <naomi/console.h>
#include <naomi/thread.h>
#include <naomi/romfs.h>
#include <naomi/maple.h>
#include <naomi/sprite/sprite.h>
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

void pcx_load_info(const char * const filename, int *width, int *height)
{
    // Start with defaults to make error handling easier.
    *width = 0;
    *height = 0;

    // Load the file, read the header.
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        return;
    }

    uint8_t version;
    fread(&version, sizeof(version), 1, fp);
    if (version != 0x0A)
    {
        fclose(fp);
        return;
    }

    // Grab the low/high x and y, subtract, add 1, return.
    uint16_t xmin, xmax, ymin, ymax;
    fseek(fp, 4, SEEK_SET);
    fread(&xmin, sizeof(xmin), 1, fp);
    fread(&ymin, sizeof(xmax), 1, fp);
    fread(&xmax, sizeof(ymin), 1, fp);
    fread(&ymax, sizeof(ymax), 1, fp);
    fclose(fp);

    *width = (xmax - xmin) + 1;
    *height = (ymax - ymin) + 1;
}

void pcx_load(const char * const filename, uint8_t *buffer, int palbank, int buf_width, int buf_height)
{
    // Start with defaults to make error handling easier.
    int width = 0;
    int height = 0;

    // Load the file, read the header.
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        return;
    }

    uint8_t version;
    fread(&version, sizeof(version), 1, fp);
    if (version != 0x0A)
    {
        fclose(fp);
        return;
    }

    uint8_t rle;
    uint8_t bpp;
    fseek(fp, 2, SEEK_SET);
    fread(&rle, sizeof(rle), 1, fp);
    fread(&bpp, sizeof(bpp), 1, fp);

    // Grab the low/high x and y, subtract, add 1, return.
    uint16_t xmin, xmax, ymin, ymax;
    fseek(fp, 4, SEEK_SET);
    fread(&xmin, sizeof(xmin), 1, fp);
    fread(&ymin, sizeof(xmax), 1, fp);
    fread(&xmax, sizeof(ymin), 1, fp);
    fread(&ymax, sizeof(ymax), 1, fp);

    // Grab color plane information.
    uint8_t planes;
    uint16_t bytes_per_plane;
    fseek(fp, 65, SEEK_SET);
    fread(&planes, sizeof(planes), 1, fp);
    fread(&bytes_per_plane, sizeof(bytes_per_plane), 1, fp);

    width = (xmax - xmin) + 1;
    height = (ymax - ymin) + 1;

    if (planes != 1 || rle != 1 || bytes_per_plane != width)
    {
        // We don't support this.
        fclose(fp);
        return;
    }

    // Now, get to the image data and start RLE decompressing.
    int pos = 0;
    int total = width * height;
    fseek(fp, 128, SEEK_SET);
    while (pos < total)
    {
        uint8_t control;
        uint8_t data;
        uint8_t length;
        fread(&control, sizeof(control), 1, fp);

        if ((control & 0xC0) == 0xC0)
        {
            // It is a length byte.
            length = control & 0x3F;
            fread(&data, sizeof(data), 1, fp);
        }
        else
        {
            // It is a direct copy byte.
            length = 1;
            data = control;
        }

        // Now, unroll that data.
        if (pos + length > total)
        {
            length = total - pos;
        }
        for(int i = 0; i < length; i++)
        {
            int xpos = pos % width;
            int ypos = pos / width;

            switch(bpp)
            {
                case 8:
                {
                    // Only output if there's room in the buffer.
                    if (xpos < buf_width && ypos < buf_height)
                    {
                        buffer[xpos + (ypos * buf_width)] = data;
                    }

                    // 8bpp means each byte goes to one pixel, so increment and wrap.
                    pos++;
                    break;
                }
                default:
                {
                    // We don't support other depths right now.
                    fclose(fp);
                    return;
                }
            }
        }
    }

    // Now, figure out if there's palette data after.
    uint32_t *bank = ta_palette_bank(TA_PALETTE_CLUT8, palbank);
    uint8_t palette_header = 0;
    fread(&palette_header, sizeof(palette_header), 1, fp);
    if (palette_header == 0x0C)
    {
        // Figure out if there's a VGA palette here.
        long loc = ftell(fp);
        fseek(fp, 0, SEEK_END);
        long end = ftell(fp);
        fseek(fp, loc, SEEK_SET);

        if (end - loc >= 768)
        {
            for (int i = 0; i < 256; i++)
            {
                color_t color;
                uint8_t pal[3];
                fread(pal, sizeof(pal[0]), 3, fp);

                color.r = pal[0];
                color.g = pal[1];
                color.b = pal[2];
                color.a = 255;

                bank[i] = ta_palette_entry(color);
            }
        }
        else
        {
            palette_header = 0;
        }
    }

    if (palette_header != 0x0C)
    {
        // Technically the spec says to look at the header in position 16-48 for 16
        // palette entries. However, the file we're reading doesn't need this so I
        // ignored that part of the spec.
    }

    fclose(fp);
}

#define SELECT_SCREEN "rom://screen_1.xr0"

void select_fadein(texture_description_t *outtex, int yoff, float xscale, float yscale)
{
    uint32_t *bank1 = ta_palette_bank(TA_PALETTE_CLUT8, 0);
    uint32_t *bank3 = ta_palette_bank(TA_PALETTE_CLUT8, 2);

    // Preserve the current bank since we used that to select the right episode.
    for (int i = 0; i < 256; i++)
    {
        bank3[i] = bank1[i];
    }

    // Do the fade!
    for (int cycle = 0; cycle < 64; cycle += 2)
    {
        for (int i = 0; i < 256; i++)
        {
            color_t unfaded = ta_palette_reverse_entry(bank3[i]);

            unfaded.r = (unfaded.r * cycle) >> 6;
            unfaded.g = (unfaded.g * cycle) >> 6;
            unfaded.b = (unfaded.b * cycle) >> 6;

            bank1[i] = ta_palette_entry(unfaded);
        }

        ta_commit_begin();
        sprite_draw_scaled(0, yoff, xscale, yscale, outtex);
        ta_commit_end();

        ta_render();

        video_display_on_vblank();
    }

    // Should be equal, but lets restore it anyway.
    for (int i = 0; i < 256; i++)
    {
        bank1[i] = bank3[i];
    }
}

void select_fadeout(texture_description_t *outtex, int yoff, float xscale, float yscale)
{
    uint32_t *bank1 = ta_palette_bank(TA_PALETTE_CLUT8, 0);
    uint32_t *bank3 = ta_palette_bank(TA_PALETTE_CLUT8, 2);

    // Preserve the current bank since we used that to select the right episode.
    for (int i = 0; i < 256; i++)
    {
        bank3[i] = bank1[i];
    }

    // Do the fade!
    for (int cycle = 62; cycle >= 0; cycle -= 2)
    {
        for (int i = 0; i < 256; i++)
        {
            color_t unfaded = ta_palette_reverse_entry(bank3[i]);

            unfaded.r = (unfaded.r * cycle) >> 6;
            unfaded.g = (unfaded.g * cycle) >> 6;
            unfaded.b = (unfaded.b * cycle) >> 6;

            bank1[i] = ta_palette_entry(unfaded);
        }

        ta_commit_begin();
        sprite_draw_scaled(0, yoff, xscale, yscale, outtex);
        ta_commit_end();

        ta_render();

        video_display_on_vblank();
    }

    // Nuke the colors, since we faded out entirely.
    for (int i = 0; i < 256; i++)
    {
        bank1[i] = ta_palette_entry(rgb(0, 0, 0));
    }
}

void select_episode()
{
    int width, height;
    pcx_load_info(SELECT_SCREEN, &width, &height);

    // Create textures for us to manipulate.
    int uvsize = ta_round_uvsize(width > height ? width : height);
    texture_description_t *outtex = ta_texture_desc_malloc_paletted(uvsize, NULL, TA_PALETTE_CLUT8, 0);
    uint8_t *outbuf = malloc(uvsize * uvsize);

    // Load the PCX image into our buffer, with 8bit palette entry 0.
    pcx_load(SELECT_SCREEN, outbuf, 0, uvsize, uvsize);

    // Calculate the scaling factors and y offset. This is based off
    // of the assumption that xargon wants to be stretched to a 4:3 resolution.
    float xscale;
    float yscale;
    int yoff;
    if (video_is_vertical())
    {
        float yheight = ((float)video_width() * 3.0) / 4.0;
        xscale = (float)video_width() / (float)width;
        yscale = yheight / (float)height;
        yoff = (video_height() - (int)yheight) / 2;
    }
    else
    {
        xscale = (float)video_width() / (float)width;
        yscale = (float)video_height() / (float)height;
        yoff = 0;
    }

    // Save the original palette.
    uint32_t *bank1 = ta_palette_bank(TA_PALETTE_CLUT8, 0);
    uint32_t *bank2 = ta_palette_bank(TA_PALETTE_CLUT8, 1);
    for (int i = 0; i < 256; i++)
    {
        bank2[i] = bank1[i];
    }

    // Handle definitions for where episode palettes are.
    int cursor_loc[4] = { 33, 65, 97, 129 };
    int tstart[4] = { 32, 64, 96, 128 };
    int tend[4] = { 64, 96, 128, 160 };

    // Gray out all of the episode selection bits.
    int cursor = 0;
    color_t gray = rgb(160, 160, 160);
    for (int i = 0; i < 4; i++)
    {
        bank1[cursor_loc[i]] = ta_palette_entry(gray);
    }
    bank1[cursor_loc[cursor]] = bank2[cursor_loc[cursor]];

    // Now, adjust the palette for each of the episodes.
    for (int p = tstart[0]; p < tend[3]; p++)
    {
        // Don't do anything to cursors.
        if (p == cursor_loc[cursor]) { continue; }

        color_t unfaded = ta_palette_reverse_entry(bank2[p]);
        unfaded.r = (unfaded.r * 17) >> 5;
        unfaded.g = (unfaded.g * 17) >> 5;
        unfaded.b = (unfaded.b * 17) >> 5;
        bank1[p] = ta_palette_entry(unfaded);
    }

    // Load the data, since its palette-cycled only we have to do this just once.
    ta_texture_load(outtex->vram_location, outtex->width, 8, outbuf);

    // Fade in the screen, like the original did.
    select_fadein(outtex, yoff, xscale, yscale);

    // Draw it! Xargon conveniently separated the palette for the display image
    // so that the different episodes were all in one chunk of palette. It means
    // that we can use palette cycling in certain regions to fade based on the
    // cursor, which is probably how the original game did it. The following
    // are rough chunks of what goes where.
    //
    // Episode 1: 32-63, border is 33
    // Episode 2: 64-95, border is 65
    // Episode 3: 96-127, border is 97
    // Order info: 128-159, border is 129
    uint32_t count = 0;
    while( 1 )
    {
        // Calculate current "throb" value. We want it to go through a full cycle
        // every half second, so grab a half second of frames, and then center it
        // on zero where half the frames are negative and half are positive. Then
        // just take the absolute value of that which gives us a ping-ponging value
        // within 0-15 inclusive. It doesn't really matter where we start, but since
        // we begin with all episodes faded out it makes sense to fade in instead of
        // out for our first chunk, so finally we subtract ourselves from 15.
        int throb = count % 30;
        throb = throb - 15;
        throb = throb < 0 ? -throb : throb;
        throb = 15 - throb;

        // Now, adjust the palette.
        for (int p = tstart[0]; p < tend[3]; p++)
        {
            // Don't do anything to cursors.
            if (p == cursor_loc[cursor]) { continue; }

            color_t unfaded = ta_palette_reverse_entry(bank2[p]);

            if (p >= tstart[cursor] && p < tend[cursor])
            {
                // Fade current selection with a throb.
                unfaded.r = (unfaded.r * (throb + 17)) >> 5;
                unfaded.g = (unfaded.g * (throb + 17)) >> 5;
                unfaded.b = (unfaded.b * (throb + 17)) >> 5;
            }
            else
            {
                // Fade non-current selection completely so its not highlighted.
                unfaded.r = (unfaded.r * 17) >> 5;
                unfaded.g = (unfaded.g * 17) >> 5;
                unfaded.b = (unfaded.b * 17) >> 5;
            }

            bank1[p] = ta_palette_entry(unfaded);
        }

        maple_poll_buttons();
        jvs_buttons_t pressed = maple_buttons_pressed();

        if (pressed.player1.up)
        {
            if (cursor > 0)
            {
                // Gray out the old entry, ungray the current.
                bank1[cursor_loc[cursor]] = ta_palette_entry(gray);
                cursor--;
                bank1[cursor_loc[cursor]] = bank2[cursor_loc[cursor]];
            }
        }
        if (pressed.player1.down)
        {
            if (cursor < 3)
            {
                // Gray out the old entry, ungray the current.
                bank1[cursor_loc[cursor]] = ta_palette_entry(gray);
                cursor++;
                bank1[cursor_loc[cursor]] = bank2[cursor_loc[cursor]];
            }
        }
        if (pressed.player1.start)
        {
            if (cursor == 3)
            {
                // TODO, display some sort of something? Dunno, can't really order it
                // for the Sega Naomi or anything.
            }
            else
            {
                break;
            }
        }

        ta_commit_begin();
        sprite_draw_scaled(0, yoff, xscale, yscale, outtex);
        ta_commit_end();

        ta_render();

        video_display_on_vblank();
        count++;
    }

    // Fade out the screen, like the original did.
    select_fadeout(outtex, yoff, xscale, yscale);

    free(outbuf);
    ta_texture_desc_free(outtex);

    switch(cursor)
    {
        case 0:
            episode_1();
            break;
        case 1:
            episode_2();
            break;
        case 2:
            episode_3();
            break;
    }
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
#ifdef NAOMI_CONSOLE
    console_init(16);
#endif

    // Init ROMFS so we can find files.
    romfs_init_default();

    // Init TMPFS so the game has a place to store temp files.
    tmpfs_init_default(0, 256 * 1024);

    // Select episode.
    select_episode();

    // Set up the video thread so that even if we don't get to the main game loop we still
    // see console messages.
    video_thread_start();

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
