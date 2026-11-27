#ifndef ACTORS_H
#define ACTORS_H

#include "records.h"

#define MAXSPRITES 4096
#define MAXSECTORS 1024
#define MAXPLAYERS 17

/* Short names for the records.h layouts (the typedefs also hold this unit's symbol slots). */
typedef struct spritetype spritetype;
typedef struct sectortype sectortype;
typedef struct player_struct playertype;

typedef char sprite_stride[(sizeof(spritetype) == 38) ? 1 : -1];
typedef char sector_stride[(sizeof(sectortype) == 37) ? 1 : -1];
typedef char player_stride[(sizeof(playertype) == 176) ? 1 : -1];

extern spritetype sprite[MAXSPRITES];
extern sectortype sector[MAXSECTORS];
extern playertype ps[MAXPLAYERS];
extern short sintable[2048];
extern short headspritestat[], nextspritestat[], headspritesect[], nextspritesect[];
extern short screenpeek, camsprite, music_volume;
extern short randomstash, disable_armor;
extern unsigned long syncbits[];
extern unsigned char synctics, parallaxtype;
extern char music_fn[4][10][13], env_music_fn[10][13];

extern long hittype[MAXSPRITES][5], msy[1024], msx[1024];

extern short EGS(short sect, long x, long y, long z, unsigned char cstat,
    short picnum, signed char shade, unsigned char xrepeat, unsigned char yrepeat,
    long ang, long xvel, long zvel, short owner, short statnum);
extern short spawn(short i, short tile);
extern long setsprite(short i, long x, long y, long z);
extern int deletesprite(short i);
extern int changespritestat(short i, short stat);
extern long movesprite(short i, long dx, long dy, long dz, long radius,
    long ceilingclearance, long floorclearance, short cliptype, long tics);
extern void rotatepoint(long pivotx, long pivoty, long x, long y, long angle,
    long *outx, long *outy);
extern void dragpoint(short w, long x, long y);
extern int getangle(long x, long y);
extern short getincangle();
extern long krand(void);
extern short findplayer(short i, long *dist);
extern short checkcursectnums(short sectnum);
extern int cansee(long x1, long y1, long z1, short sect1,
    long x2, long y2, long z2, short sect2);
extern void FTA(short quote);
extern void spritesound(long sound, short i);
extern void stopsound(long sound);
extern void playmusic(char *name);
extern void MUSIC_SetVolume(long volume);
extern void shoot(short i, short tile, unsigned char trace);
extern void kickview(short p, short i);
extern void execute(short i, short p, long dist);
extern void myos(long x, long y, short tile, short shade, long flags);
extern void operatesectors(short sect, short i);
extern void operateactivators(short lotag);
extern void operatemasterswitches(short lotag);

extern void clipmove(long *x, long *y, long *z, short *sectnum,
    long xv, long yv, long radius, long ceilingclearance, long floorclearance, long cliptype);

/* Original Build intrinsic from Ken Silverman's BUIL0615/PRAGMAS.H. */
extern long klabs(long x);
#pragma aux klabs = \
    "test eax,eax" \
    "jns skipnegate" \
    "neg eax" \
    "skipnegate:" \
    parm nomemory [eax];

#endif

