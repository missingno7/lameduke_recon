#ifndef SECTOR_H
#define SECTOR_H

#include "records.h"

#define MAXANIMATES 512
#define MAXANIMWALLS 256
#define MAXSPRITES 4096
#define MAXSECTORS 1024
#define MAXWALLS 8192
#define MASTERSWITCH 51
#define SECTOREFFECTOR 175
#define FORCEFIELD 1184
#define SOUND_LIFT_START 0
#define SOUND_LIFT_STOP 35
#define SOUND_LIFT_MOVING 36

/* Short names for the records.h layouts (the typedefs also hold this unit's symbol slots). */
typedef struct spritetype spritetype;
typedef struct sectortype sectortype;
typedef struct walltype walltype;
typedef struct player_struct playertype;
typedef struct animwalltype animwalltype;

extern spritetype sprite[MAXSPRITES];
extern sectortype sector[MAXSECTORS];
extern walltype wall[MAXWALLS];
extern playertype ps[17];
extern long hittype[MAXSPRITES][5];
extern short connecthead, connectpoint2[];
extern short headspritestat[], nextspritestat[];
extern short headspritesect[], nextspritesect[];
extern short numsectors, screenpeek, camsprite;
extern char gotpic[];
extern long waloff[];
extern short sintable[];
extern unsigned long syncbits[];
extern char option[];
extern volatile char keystatus[];
extern short gamepaused, disable_armor;
extern long menupage;

extern void spritesound(short sound, short i);
extern void stopsound(short sound);
extern void xyzsound(short sound, short i, long x, long y, long z);
extern void render_camera_tile(long x, long y, long z, short ang, short sn, short tile);
extern void render_mirror(short w);
extern void movelava(long tile_address);
extern long nextsectorneighborz(short sectnum, long z, short topbottom, short direction);
extern void updatesector(long x, long y, short *sectnum);
extern int deletesprite(short i);
extern short EGS(short sect, long x, long y, long z, unsigned char cstat, short picnum,
    signed char shade, unsigned char xrepeat, unsigned char yrepeat,
    short ang, short xvel, short zvel, short owner, short statnum);
extern void FTA(short quote);
extern void setview(long x1, long y1, long x2, long y2);
extern void setgamemode(void);
extern void qsetmode640480(void);
extern void vscrn(short p);
extern void displayrooms(short p);
extern void addweapon(short p, short tile, short amount);
extern void neartag(long x, long y, long z, short sectnum, short ang, short *tagsector,
    short *tagwall, short *tagsprite, long *distance, long range);

/* Original Build absolute-value intrinsic, from BUIL0615/PRAGMAS.H. */
extern long klabs(long value);
#pragma aux klabs = \
    "test eax,eax" \
    "jns skipnegate" \
    "neg eax" \
    "skipnegate:" \
    parm nomemory [eax];

extern long animatecnt, *animateptr[MAXANIMATES];
extern long animategoal[MAXANIMATES], animatevel[MAXANIMATES];
extern short animatesect[MAXANIMATES];
extern animwalltype animwall[MAXANIMWALLS];
extern short numanimwalls;

short checkcursectnums(short sn);
short findplayer(short sn, long *d);
void doanimations(long tics);
short getanimationgoal(long *position);
int setanimation(short sn, long *position, long goal, long velocity);
void animatecamsprite(short p);
void animatewalls(void);
void activatewarpelevators(short i, short direction);
void operatesectors(short sn, short source);
void operateactivators(short lotag);
void operatemasterswitches(short lotag);
void operateforcefields(short lotag);
void cheatkeys(short p);
void checkhitswitch(long w);
void checkhitwall(long w, long x, long y, long z, short weapon);
char checkhitceiling(short sn);
void checksectors(short p);
void allignwarpelevators(void);

#endif
