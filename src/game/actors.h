#ifndef ACTORS_H
#define ACTORS_H

#define MAXSPRITES 4096
#define MAXSECTORS 1024
#define MAXPLAYERS 17

#pragma pack(1)
typedef struct {
    long x, y, z;
    unsigned char cstat;
    signed char shade;
    unsigned char xrepeat, yrepeat;
    short picnum, ang, xvel, yvel, zvel, owner;
    short sectnum, statnum, lotag, hitag, extra;
} spritetype;

typedef struct {
    unsigned short wallptr, wallnum;
    short ceilingpicnum, floorpicnum, ceilingheinum, floorheinum;
    long ceilingz, floorz;
    signed char ceilingshade, floorshade;
    unsigned char ceilingxpanning, floorxpanning, ceilingypanning, floorypanning;
    unsigned char ceilingstat, floorstat, ceilingpal, floorpal, visibility;
    short lotag, hitag, extra;
} sectortype;

typedef struct {
    short i;
    long viewmode, mapzoom, posx, posy, posz, horiz;
    long oldx, oldy, oldz, poszv;
    long oposx, oposy, oposz, score, bobcounter;
    short ang, oang, oldang, cursectnum, ocursectnum, lookang;
    short screen_size, ammo, batteryammo, remote_ammo, rocket_ammo, health;
    short newowner, holoduke, jumping_counter, airleft, last_weapon;
    short weapon_delay[13];
    short weapon_slot, kickback_pic, curr_weapon, face_parasite, crane, state84;
    char *music;
    unsigned char gm, weapon_clock, skill, level, volume, on_ground;
    unsigned char remote_active, state91[5], armor, state97[5];
    unsigned char gotweapon[5], weapon_pos;
    unsigned char statea2[9];
    unsigned char muzzle_flash, kills_left, kills, total_kills, turret_kills;
} playertype;
#pragma pack()

typedef char sprite_stride[(sizeof(spritetype) == 38) ? 1 : -1];
typedef char sector_stride[(sizeof(sectortype) == 37) ? 1 : -1];
typedef char player_stride[(sizeof(playertype) == 176) ? 1 : -1];

extern spritetype sprite[MAXSPRITES];
extern sectortype sector[MAXSECTORS];
extern playertype ps[MAXPLAYERS];
extern short sintable[2048];
extern short headspritestat[], nextspritestat[], headspritesect[], nextspritesect[];
extern short screenpeek, viewscreen, music_volume;
extern short randomstash, disable_armor;
extern unsigned long syncbits[];
extern unsigned char synctics, parallaxtype;
extern char music_files[4][10][13], env_music[10][13];

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
extern short getangle(long x, long y);
extern short getincangle_shift(short ang, short goal, short shift);
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
extern void draw_weapon_tile(long x, long y, short tile, short shade, long flags);
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

