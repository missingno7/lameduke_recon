#ifndef MENUES_H
#define MENUES_H

/* Watcom's game profile packs these records to one byte. */
struct menu {
    short x, y, lines;
    short unused;
    short tile;
    unsigned char *text;
    unsigned char flags;
};

struct menu_player {
    unsigned char prefix[2];
    long display_mode;
    unsigned char movement[62];
    short cursectnum;
    unsigned char view[4];
    short screen_size;
    unsigned char controls[62];
    unsigned char game_mode;
    unsigned char unknown_8b;
    unsigned char skill, level, episode;
    unsigned char remainder[33];
};

struct menu_sector {
    unsigned char prefix[29];
    unsigned char floorpal;
    unsigned char tags[7];
};

extern struct menu_player ps[17];
extern struct menu_sector sector[1024];
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

