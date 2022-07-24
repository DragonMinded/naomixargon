#ifndef __EXTRA_H
#define __EXTRA_H

// A bunch of stuff that was in stdlib.h a long time ago but we implement
// by bridging over to modern functions. Since the code assumes its in stdlib.h
// we must define it here to not get warnings.
char *ltoa(long int l, char * out, int outlen);
char *ultoa(unsigned long int l, char * out, int outlen);
int min(int a, int b);
int max(int a, int b);

int randomrange(int max);
int getclock();

void setmem(void *dst, unsigned long len, char val);

// Stuff in input libraries that somehow is not foward defined.
void readjoy (int *x, int *y);

// Stuff in video libraries that somehow is not forward defined.
void dim(void);
void undim(void);

// Stuff in audio libraries that somehow is not forward defined.
void nosound();
void *getvect();
void setvect(int vno, void *vect);

// Stuff in main source file that somehow is not forward defined.
void drawstats(void);
void loadboard (char *fname);
void saveboard (char *fname);
void moddrawboard (void);
void printhi (int newhi);
void enable();
void delay(int amount);
unsigned long filelength(int fd);

#endif
