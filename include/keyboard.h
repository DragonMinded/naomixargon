//	Keyboard.H

extern char k_rshift, k_lshift, k_shift, k_ctrl, k_alt, k_numlock;
extern unsigned char keydown[2][256];

void k_status (void);
int k_pressed (void);
int k_read (void);

void installhandler (unsigned char status);
void removehandler(void);
void enablebios(void);
void disablebios(void);
int biosstatus (void);

//	Characters
#define enter 13
#define esc 27
#define escape 27
#define k_tab 9
#define k_bs 8
#define k_up 200
#define k_down 208
#define k_left 203
#define k_right 205
#define k_pgup 201
#define k_pgdown 209
#define k_f1 187
#define k_f2 188
#define k_f3 189
#define k_f4 190
#define k_f5 191
#define k_f6 192
#define k_f7 193
#define k_f8 194
#define k_f9 195
#define k_f10 196

#define scan_esc 0x01
#define scan_ctrl 0x1D
#define scan_lshift 0x2A
#define scan_rshift 0x36
#define scan_alt 0x38
#define scan_space 0x39
#define scan_f1 0x3B
#define scan_cursorup 0x48
#define scan_cursorleft 0x4B
#define scan_cursorright 0x4D
#define scan_cursordown 0x50
