#ifndef MENUES_H
#define MENUES_H

#include "records.h"

extern struct player_struct ps[17];
extern struct sectortype sector[1024];
extern short screenpeek, myconnectindex;
extern short tilesizx[4096], tilesizy[4096], pskyoff[], pskybits;
extern volatile unsigned char keystatus[256];

extern void overwritesprite(long x, long y, short tile, long shade,
    long orientation, long palette);
extern void rotatesprite(long x, long y, long zoom, short angle, short tile);
extern void setview(long x1, long y1, long x2, long y2);
extern long ksqrt(long square);
extern long next_text_shade(void);
extern long gametext(short x, short y, unsigned char *text, unsigned char shade);
extern void update_menu_cursor(short tile);
extern void displayhighscores(void);
extern void saveplayer(long slot);
extern void loadplayer(long slot);
extern void newgame(unsigned char episode, unsigned char level, unsigned char skill);
extern void enterlevel(unsigned char mode);

/* Build's original inline helper from the June 1994 PRAGMAS.H. */
extern long klabs(long value);
#pragma aux klabs = \
    "test eax,eax" \
    "jns skipnegate" \
    "neg eax" \
    "skipnegate:" \
    parm nomemory [eax];

/* Ken's 1994 copybuf (BUIL0422 ENGINE.C/BUILD.C, also buil1995 PRAGMAS.H) has no modify clause; movelava's
   register allocation depends on it (kengame worker). */
extern void copybuf(void *source, void *dest, long count);
#pragma aux copybuf = \
    "rep movsd" \
    parm [esi] [edi] [ecx];

#endif

