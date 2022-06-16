void StartWorx(void)
{
    // TODO: Do we need to implement this, or just treat it as our own sound init?
}

void CloseWorx(void)
{
    // TODO: Do we need to implement this, or just treat it as our own sound free?
}

int AdlibDetect(void)
{
    // Force audio detection if at all possible, since we have a working sound system.
    return 1;
}

int DSPReset(void)
{
    // TODO
    return 0;
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
    // TODO
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

int PlayVOCBlock(char far *voc,int volume)
{
    // TODO
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
