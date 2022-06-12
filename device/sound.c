void StartWorx(void)
{

}

void CloseWorx(void)
{

}

int AdlibDetect(void)
{
    return 0;
}

int DSPReset(void)
{
    return 0;
}

int SetMasterVolume(unsigned char left,unsigned char right)
{
    return 0;
}

int SetFMVolume(unsigned char left,unsigned char right)
{
    return 0;
}

void DSPClose(void)
{

}

int VOCPlaying(void)
{
    return 0;
}

void SetLoopMode(int m)
{

}

void StopSequence(void)
{

}

void nosound()
{

}

void timerset (int numero, int moodi, unsigned int arvo)
{

}

int PlayVOCBlock(char far *voc,int volume)
{
    return 0;
}

void PlayCMFBlock(char far *seq)
{

}

char far *GetSequence(char *f_name)
{
    return 0;
}

void setvect(void *vect)
{
    // Blank
}

void *getvect()
{
    // Blank
    return 0;
}

void interrupt WorxBugInt8 (void)
{

}

void interrupt spkr_intr (void)
{

}
