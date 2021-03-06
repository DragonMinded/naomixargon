// X_VOL2.C
//
// Xargon: Volume-specific Stuff
//
// by Allen W. Pilgrim

#include <stdlib.h>
#include "include/gr.h"
#include "include/keyboard.h"
#include "include/windows.h"
#include "include/gamectrl.h"
#include "include/x_obj.h"
#include "include/xargon.h"

char xshafile2[]="rom://graphics.xr2";
char xvocfile2[]="rom://audio.xr2";
char cfgfname2[]="rom://config.xr2";
char ext2[]=".xr2";
char tilefile2[]="rom://tiles.xr2";
char xintrosong2[]="rom://song_0.xr2";

char *demoboard2[1]={"intro"};
char demolvl2[1]={100};
char *demoname2[1]={"rom://demo_mac.xr0"};

char v_msg2[]="VOLUME TWO --- The Secret Chamber";

char *fidgetmsg2[4]={"Apogee never made games this fun","I can't imagine anyone being bored playing this","Don't just stand there looking stupid","Come on Malvineous get moving"};

char *leveltxt2[16]={
	"Malvineous returns\r\rto the safety of\r\rthe map level\r",
	"Malvineous continues\r\rhis quest to find\r\rXargon's Castle\r",
	"Malvineous cautiously\r\renters the\r\rmassive volcano\r",
	"Malvineous explores\r\rmore of this\r\rstrange land\r",
	"Malvineous finds\r\rthe cave of the\r\rdeadly xbat\r",
	"Malvineous is surrounded\r\rby strange\r\ralien creatures\r",
	"Malvineous sloshes\r\rinto the\r\rswamps of despair\r",
	"Malvineous begins\r\rto sweat in\r\rthe lava pits\r",
	"Malvineous gets\r\rlost in the\r\rsurreal forest\r",
	"Malvineous journeys\r\rbeneath\r\rthe earth\r",
	"Malvineous approaches\r\rthe\r\rgrunt's hideout\r",
	"Malvineous dives\r\rinto the\r\rdangerous waters\r",
	"Malvineous discovers\r\rsome\r\rancient ruins\r",
	"Malvineous enters\r\ra\r\rmysterious maze\r",
	"Malvineous storms\r\rinto the\r\rhidden fortress\r",
	"Malvineous creeps\r\rinto the\r\rSecret Chamber\r"
	};

void wait2 (void) {
	int x, y;
	int c,q;

	clrpal (); setpagemode (1);
	for (x=0; x<20; x++) {
		for (y=0; y<12; y++) {
			drawshape (&mainvp,0x7501+x+y*20,x*16,y*16);
			};
		};
	fontcolor (&mainvp,71,-1);
	wprint (&mainvp,168,8,1,"THE SECRET CHAMBER");
	pageflip (); setpagemode (0);	fadein ();
	do {
		gamecount++; checkctrl0(0);
		for (c=64; c<80; c++) {
			q=64+((c+gamecount)&15);
			setcolor (c,vgapal[q*3+0],vgapal[q*3+1],vgapal[q*3+2]);
			};
		} while ((key==0)&&(fire1==0)&&(fire2==0)&&(dx1==0)&&(dy1==0));
	};
