//	Uncrunch header

#define scrnaddr ((char far *)0xB8000000)
extern void uncrunch (unsigned char far *sourceptr,char far *destptr,int length);
