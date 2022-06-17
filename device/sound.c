#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <naomi/audio.h>
#include "include/music.h"

typedef struct {
    void *voc;
    int handle;
} raw_t;

static raw_t *loaded_vocs[num_samps];
static int loaded_voc_count = 0;

void StartWorx(void)
{
    audio_init();
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
    // TODO
}

void StopSequence(void)
{
    // TODO
}

void nosound()
{
    // TODO
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
    // TODO
}

char far *GetSequence(char *f_name)
{
    // TODO
    return 0;
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
