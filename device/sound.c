#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <naomi/audio.h>
#include <naomi/thread.h>
#include <mpg123.h>
#include "include/music.h"

#define BUFSIZE 8192

typedef struct {
    void *voc;
    int handle;
} raw_t;

static raw_t *loaded_vocs[num_samps];
static int loaded_voc_count = 0;
static volatile int looping = 0;

typedef struct {
    uint32_t thread;
    volatile int exit;
    uint8_t *data;
    int size;
    int loc;
} instructions_t;

static instructions_t *cursong = 0;

// Forward definitions.
void StopSequence(void);

mpg123_ssize_t audiothread_read(void *handle, void *data, size_t size)
{
    instructions_t *song = (instructions_t *)handle;
    if (!song) { return 0; }
    if (!data) { return 0; }
    if (size == 0) { return 0; }

    size_t actual = song->size - song->loc;
    if (size > actual) { size = actual; }
    if (size > 0)
    {
        memcpy(data, &song->data[song->loc], size);
        song->loc += size;
    }

    return size;
}

off_t audiothread_seek(void *handle, off_t offset, int whence)
{
    instructions_t *song = (instructions_t *)handle;
    if (!song) { return -1; }

    off_t actual;
    switch (whence)
    {
        case SEEK_SET:
            actual = offset;
            break;
        case SEEK_END:
            actual = song->size + offset;
            break;
        case SEEK_CUR:
            actual = song->loc + offset;
            break;
        default:
            return -1;
    }

    if (actual < 0) { actual = 0; }
    if (actual > song->size) { actual = song->size; }
    song->loc = actual;

    return actual;
}

void audiothread_cleanup(void *handle)
{
    instructions_t *song = (instructions_t *)handle;

    if (!song) { return; }
    if (!song->data) { return; }

    free(song->data);
    song->data = 0;
    song->size = 0;
    song->loc = 0;
}

void *audiothread(void *param)
{
    instructions_t *song = (instructions_t *)param;

    // Now, get a handle and start setting up.
    int err = 0;
    mpg123_handle *mh = mpg123_new(NULL, &err);
    if (err != MPG123_OK)
    {
        mpg123_exit();
        return 0;
    }

    // Now, point mpg123 at our loaded data.
    err = mpg123_replace_reader_handle(mh, &audiothread_read, &audiothread_seek, &audiothread_cleanup);
    if (err != MPG123_OK)
    {
        mpg123_delete(mh);
        mpg123_exit();
        return 0;
    }

    // Now, open and get the info from the file.
    err = mpg123_open_handle(mh, song);
    if (err != MPG123_OK)
    {
        mpg123_delete(mh);
        mpg123_exit();
        return 0;
    }

    // Get the info of the file so we can set up streaming for it.
    long samplerate;
    int channels;
    int encoding;
    err = mpg123_getformat(mh, &samplerate, &channels, &encoding);
    if (err != MPG123_OK)
    {
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
        return 0;
    }

    // Ensure that we've got the correct encoding (we control this so let's not be too generic).
    int encbits = mpg123_encsize(encoding) * 8;
    if (samplerate < 6000 || samplerate > 48000 || channels != 2 || encbits != 16)
    {
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
        return 0;
    }

    // Finally, based on the file's info, set up the encoding and samplerate.
    audio_register_ringbuffer(AUDIO_FORMAT_16BIT, samplerate, BUFSIZE);

    // Now, start decoding!
    size_t bytes_read;
    uint32_t *buffer = malloc(BUFSIZE);
    while (song->exit == 0)
    {
        while (song->exit == 0 && (mpg123_read(mh, buffer, BUFSIZE, &bytes_read) == MPG123_OK))
        {
            int numsamples = bytes_read / 4;

            uint32_t *samples = buffer;
            while (numsamples > 0)
            {
                int actual_written = audio_write_stereo_data(samples, numsamples);
                if (actual_written < 0)
                {
                    song->exit = 1;
                    break;
                }
                if (actual_written < numsamples)
                {
                    numsamples -= actual_written;
                    samples += actual_written;

                    // Sleep for the time it takes to play half our buffer so we can wake up and
                    // fill it again.
                    thread_sleep((int)(1000000.0 * (((float)BUFSIZE / 4.0) / (float)samplerate)));
                }
                else
                {
                    numsamples = 0;
                }
            }
        }

        if (!looping || song->exit != 0)
        {
            // We're done, no looping requested!
            break;
        }
        else
        {
            // Rewind the song, start playing again!
            mpg123_seek(mh, 0, SEEK_SET);
        }
    }

    // Done streaming, don't need the audio streaming interface anymore.
    free(buffer);
    audio_unregister_ringbuffer();

    // Also don't need mpg123 anymore.
    mpg123_close(mh);
    mpg123_delete(mh);
    return 0;
}

void StartWorx(void)
{
    audio_init();
    mpg123_init();

    loaded_voc_count = 0;

    for (int i = 0; i < sizeof(loaded_vocs) / sizeof(loaded_vocs[0]); i++)
    {
        loaded_vocs[i] = 0;
    }
}

void CloseWorx(void)
{
    for (int i = 0; i < sizeof(loaded_vocs) / sizeof(loaded_vocs[0]); i++)
    {
        if (loaded_vocs[i] != 0)
        {
            audio_unregister_sound(loaded_vocs[i]->handle);
            free(loaded_vocs[i]);
            loaded_vocs[i] = 0;
        }
    }

    loaded_voc_count = 0;

    mpg123_exit();
    audio_free();
}

int AdlibDetect(void)
{
    // Force audio detection if at all possible, since we have a working sound system.
    return 1;
}

int DSPReset(void)
{
    // Claim that this worked fine so audio will work.
    return 1;
}

int SetMasterVolume(unsigned char left,unsigned char right)
{
    // TODO
    return 0;
}

int SetFMVolume(unsigned char left,unsigned char right)
{
    // TODO
    return 0;
}

void DSPClose(void)
{
    // TODO
}

int VOCPlaying(void)
{
    // We support playing 62 sounds at once, so lie about VOCs playing
    // in order to overlay sounds.
    return 0;
}

void SetLoopMode(int m)
{
    // Set whether the background audio should loop or not.
    looping = m != 0;
}

void nosound()
{
    // This kills existing sounds in the system, but it could possibly nuke sound effects
    // that are being layered, so we choose to ignore it.
}

void timerset (int numero, int moodi, unsigned int arvo)
{
    // I'm not sure if we need this, since its just a direct call to the sound card?
}

raw_t *voc2raw(char far *voc)
{
    // First, see if we've already created this structure.
    for (int i = 0; i < loaded_voc_count; i++)
    {
        if (loaded_vocs[i]->voc == voc)
        {
            return loaded_vocs[i];
        }
    }

    if (loaded_voc_count == (sizeof(loaded_vocs) / sizeof(loaded_vocs[0])))
    {
        // No room!
        return 0;
    }

    // We must parse it ourselves!
    if (memcmp(&voc[0], "Creative Voice File", 19) != 0)
    {
        // Unsure what this file is.
        return 0;
    }

    if (voc[19] != 0x1A)
    {
        // Unsure what this file is.
        return 0;
    }

    // Grab the size, so we can index to the first chunk.
    uint16_t size;
    memcpy(&size, &voc[20], 2);

    // Where we put the output data.
    uint8_t *finaldata = 0;
    unsigned int finalsize = 0;
    int finalfrequency = -1;

    int offset = size;
    int end = 0;
    while( !end )
    {
        switch(voc[offset])
        {
            case 1:
            {
                // Sound data with type block.
                unsigned int audiosize = (
                    (0xFF & (unsigned int)voc[offset + 1]) |
                    (0xFF00 & ((unsigned int)voc[offset + 2] << 8)) |
                    (0xFF0000 & ((unsigned int)voc[offset + 3] << 16))
                );

                uint8_t *data = (uint8_t *)&voc[offset + 4];
                int frequency = 1000000.0 / (256.0 - (float)data[0]);
                int codec = (int)data[1];
                void *audio = &data[2];

                if (codec != 0)
                {
                    // We don't have support for this!
                    if (finaldata) { free(finaldata); }
                    return 0;
                }

                if (finalfrequency != -1 && finalfrequency != frequency)
                {
                    // We don't support frequency adjustments!
                    if (finaldata) { free(finaldata); }
                    return 0;
                }

                // Register this chunk of data.
                finalfrequency = frequency;
                finaldata = realloc(finaldata, finalsize + audiosize);
                memcpy(&finaldata[finalsize], audio, audiosize);
                finalsize += audiosize;

                // Add 4 bytes for the header, and 2 for the frequency/codec.
                // This seems to be the format style for this version of the file.
                offset += audiosize + 6;

                break;
            }
            case 0:
            {
                // End of sound block.
                end = 1;
                break;
            }
            default:
            {
                if (finaldata) { free(finaldata); }
                return 0;
            }
        }
    }

    if (!finaldata || !finalsize || finalfrequency == -1)
    {
        if (finaldata) { free(finaldata); }
        return 0;
    }

    // Convert from 8-bit unsigned to 8-bit signed.
    for (int i = 0; i < finalsize; i++)
    {
        finaldata[i] ^= 0x80;
    }

    // Now we must register it!
    int soundid = audio_register_sound(AUDIO_FORMAT_8BIT, finalfrequency, finaldata, finalsize);
    if (soundid <= 0)
    {
        free(finaldata);
        return 0;
    }

    // Finally, create a structure for this!
    raw_t *data = malloc(sizeof(raw_t));
    data->handle = soundid;
    data->voc = voc;
    loaded_vocs[loaded_voc_count++] = data;
    return data;
}

int PlayVOCBlock(char far *voc, int volume)
{
    // First, convert the VOC file.
    raw_t *data = voc2raw(voc);
    if (data)
    {
        audio_play_registered_sound(data->handle, SPEAKER_LEFT | SPEAKER_RIGHT, (float)volume / 127.0);
        return 1;
    }

    // This is never checked by the code but I assume its meant to return truthy/falsy.
    return 0;
}

void PlayCMFBlock(char far *seq)
{
    // Make sure we kill any existing songs (the game takes care of this, but we're careful).
    StopSequence();

    // Play a song that was returned from GetSequence.
    if (seq == 0) { return; }
    instructions_t *newsong = (instructions_t *)seq;
    newsong->exit = 0;

    // Schedule the audio thread to decode for us while we do game loop work.
    newsong->thread = thread_create("audio", &audiothread, newsong);
    thread_priority(newsong->thread, 1);
    thread_start(newsong->thread);

    // Remember the current song.
    cursong = newsong;
}

char far *GetSequence(char *f_name)
{
    // Load a song that can be passed to PlayCMFBlock.
    instructions_t *newsong = malloc(sizeof(instructions_t));
    if (!newsong)
    {
        return 0;
    }

    // Mark that this song isn't exiting.
    newsong->exit = 0;
    newsong->size = 0;
    newsong->loc = 0;

    // Load the file, get its size.
    FILE *fp;
    if (strncmp(f_name, "rom://", 6) != 0)
    {
        char tmp[512];
        strcpy(tmp, "rom://");
        strcat(tmp, f_name);
        fp = fopen(tmp, "r");
    }
    else
    {
        fp = fopen(f_name, "r");
    }

    if (!fp)
    {
        free(newsong);
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    newsong->size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Load the data.
    newsong->data = malloc(newsong->size);
    if (!newsong->data)
    {
        fclose(fp);
        free(newsong);
        return 0;
    }

    int actual = fread(newsong->data, 1, newsong->size, fp);
    fclose(fp);

    if (actual != newsong->size)
    {
        free(newsong);
        return 0;
    }

    // Now, return this pointer.
    return (char *)newsong;
}

void StopSequence(void)
{
    // Kill any existing background audio
    if (cursong)
    {
        cursong->exit = 1;
        thread_join(cursong->thread);
        thread_destroy(cursong->thread);

        if (cursong->data) { free(cursong->data); }
    }

    // Make sure we know there's no song playing.
    cursong = 0;
}


void setvect(void *vect)
{
    // Blank, we don't need interrupt vector support.
}

void *getvect()
{
    // Blank, we don't need interrupt vector support.
    return 0;
}

void interrupt WorxBugInt8 (void)
{
    // Blank, we don't need interrupt vector support.
}

void interrupt spkr_intr (void)
{
    // Blank, we don't need interrupt vector support.
}
