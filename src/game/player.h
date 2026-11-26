#ifndef PLAYER_H
#define PLAYER_H

#define MAXPLAYERS 17
#define MAXSPRITES 4096
#define MAXSECTORS 1024

struct playerstate {
    short i;
    long display_mode, unused_06;
    long posx, posy, posz, horiz;
    long previous_x, previous_y, previous_z, poszv;
    long oposx, oposy, oposz, score, bobcounter;
    short ang, oang, previous_ang, cursectnum, ocursectnum, lookang;
    short screen_size, ammo, batteryammo, remote_ammo, rocket_ammo, health;
    short newowner, flash_sprite, jumping_counter, airleft, last_weapon;
    short weapon_delay[13];
    short weapon_slot, kickback_pic, curr_weapon, state80, state82, state84;
    long music_offset;
    unsigned char gm, weapon_clock, skill, level, volume;
    unsigned char on_ground, remote_active, knee_sequence, state92;
    unsigned char scuba_on, jetpack_on, state95, armor;
    unsigned char inventory[5], state9c[5];
    unsigned char weapon_pos, statea2, raise_frame, knee_flash, footstep;
    unsigned char statea6, statea7, hard_landing, statea9, stateaa;
    unsigned char muzzle_flash, stateac, statead, stateae, stateaf;
};

struct spritetype {
    long x, y, z;
    unsigned char cstat;
    signed char shade;
    unsigned char xrepeat, yrepeat;
    short picnum, ang, xvel, yvel, zvel, owner;
    short sectnum, statnum, lotag, hitag, extra;
};

struct sectortype {
    unsigned short wallptr, wallnum;
    short ceilingpicnum, floorpicnum, ceilingheinum, floorheinum;
    long ceilingz, floorz;
    signed char ceilingshade, floorshade;
    unsigned char ceilingxpanning, floorxpanning, ceilingypanning, floorypanning;
    unsigned char ceilingstat, floorstat, ceilingpal, floorpal, visibility;
    short lotag, hitag, extra;
};

typedef char player_stride[(sizeof(struct playerstate) == 176) ? 1 : -1];
typedef char sprite_stride[(sizeof(struct spritetype) == 38) ? 1 : -1];
typedef char sector_stride[(sizeof(struct sectortype) == 37) ? 1 : -1];

extern struct playerstate ps[MAXPLAYERS];
extern struct spritetype sprite[MAXSPRITES];
extern struct sectortype sector[MAXSECTORS];
extern short sintable[2048];
extern volatile unsigned char keystatus[256];
extern unsigned char script_tics, user_visibility, visibility;
extern signed char player_avel[], player_vel[], player_svel[];
extern unsigned long player_bits[];
extern long totalclock, lockclock, avel, vel, svel;
extern short screenpeek, myconnectindex, moustat;
extern short menu_x, menu_y, menu_drag, mouse_sensitivity;
extern short sound_volume, disable_armor;
extern long *actorscrptr[4096];

extern int getangle(long x, long y);
extern int getincangle_shift(short a, long b, short shift);
extern long ksqrt(long square);
extern long krand(void);
extern long FX_SetVolume(long volume);
extern void spritesound(short sound, short i);
extern void draw_weapon_tile(long x, long y, long tile, short shade, long flags);
extern void getmousevalues(short *x, short *y, short *buttons);
extern void hitscan(long x, long y, long z, short sectnum, long xv, long yv, long zv,
    short *hitsect, short *hitwall, short *hitsprite, long *hitx, long *hity, long *hitz);
extern short EGS(short sect, long x, long y, long z, unsigned char cstat, short picnum,
    signed char shade, unsigned char xrepeat, unsigned char yrepeat,
    short ang, short xvel, short zvel, short owner, short statnum);
extern long setsprite(short i, long x, long y, long z);
extern long checkhitceiling(short sect);
extern void checkhitswitch(short wall);
extern void checkhitwall(short wall, long x, long y, long z, short weapon);
extern short findplayer(short i, long *distance);
extern void updatesector(long x, long y, short *sectnum);
extern void clipmove(long *x, long *y, long *z, short *sectnum, long xv, long yv,
    long radius, long ceiling_dist, long floor_dist, long cliptype);
extern void getzrange(long x, long y, long z, short sectnum, long *cz, long *ch,
    long *fz, long *fh, long distance, long cliptype);
extern void stopsound(short sound);
extern short spawn(short i, short tile);
extern void addweapon(short p, short tile, short amount);

/* Original Build inline helper, from the June 1994 PRAGMAS.H. */
extern long klabs(long value);
#pragma aux klabs = \
    "test eax,eax" \
    "jns skipnegate" \
    "neg eax" \
    "skipnegate:" \
    parm nomemory [eax];

void kickview(short p, short i);
void spawn_pistol_tracer(short p);
void shoot(short i, short tile, unsigned char move_to_hit);
unsigned char displayraise(short shade, short p);
void displayweapon(short p);
void getinput(short p);
void update_air(short p);
void processinput(short p);

#endif

