#ifndef GAMEDEF_H
#define GAMEDEF_H

#define MAXTILES 4096
#define MAXSPRITES 4096
#define LABELSIZE 64
#define MAXLABELS 512

enum con_opcode {
    CON_END, CON_ACTOR, CON_ADDBATTERYAMMO, CON_IFRND, CON_ENDA,
    CON_IFCANSEE, CON_IFHITWEAPON, CON_ACTION, CON_IFPDISTL, CON_IFPDISTG,
    CON_ELSE, CON_STRENGTH, CON_BREAK, CON_SHOOT, CON_ADDSCORE,
    CON_SOUND, CON_FALL, CON_STATE, CON_ENDS, CON_DEFINE,
    CON_LINECOMMENT, CON_NUKE, CON_KILLIT, CON_ADDWEAPON, CON_WEAPON,
    CON_ADDHEALTH, CON_IFDEAD, CON_IFSQUISHED, CON_CACTOR, CON_LBRACE,
    CON_RBRACE, CON_SPAWN, CON_MOVE, CON_IFWASWEAPON, CON_IFACTION,
    CON_IFACTIONCOUNT, CON_RESETACTIONCOUNT, CON_DEBRIS, CON_IFSKILL,
    CON_BLOCKCOMMENT, CON_CSTAT, CON_IFMOVE, CON_RESETPLAYER, CON_IFONWATER,
    CON_IFINWATER, CON_IFPLAYERSWEAPON, CON_IFMOVECOUNT, CON_RESETMOVECOUNT,
    CON_ADDINVENTORY, CON_ADDAMMO, CON_HITRADIUS, CON_IFP, CON_ADDRAMMO,
    CON_SINK, CON_MUSIC, CON_INCLUDE, CON_IFSTRENGTH, CON_BLOOD, CON_GUTS,
    CON_IFSPAWNEDBY, NUMKEYWORDS
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

struct actorstate {
    long movecount;
    long *move;
    long actioncount, actionframe;
    long *action;
};

struct playerstate {
    unsigned char prefix[10];
    long posx, posy, posz;
    unsigned char motion[32];
    long score;
    unsigned char view[10];
    short cursectnum;
    unsigned char state46[2];
    short horiz;
    unsigned char state4a[2];
    short ammo, batteryammo;
    unsigned char state50[2];
    short rammo, health;
    unsigned char state56[4];
    short poszv;
    unsigned char state5c[34];
    short curr_weapon;
    unsigned char state80[12];
    unsigned char skill;
    unsigned char state8d[2];
    unsigned char on_ground;
    unsigned char state90[4];
    unsigned char jetpack_on;
    unsigned char state95;
    unsigned char armor, inventory[5];
    unsigned char state9c[15];
    unsigned char shooting;
    unsigned char tail[4];
};

extern struct spritetype sprite[MAXSPRITES];
extern struct sectortype sector[1024];
extern struct actorstate hittype[MAXSPRITES];
extern struct playerstate ps[17];
extern short sintable[2048];
extern char tempbuf[];
extern char *textptr;
extern char *label;
extern long labelcnt, *labelcode, *scriptptr;
extern long script[];
extern long *actorscrptr[MAXTILES];
extern unsigned char warning, error;
extern char *last_used_text;
extern short line_number, total_lines, hit_sprite;
extern long *actor_header;
extern char parsing_state;
extern signed char brace_depth;
extern char *sound_names[39];
extern char music_files[4][10][13], env_music[10][13];
extern long *insptr;
extern char killit_flag;
extern short cached_cansee;
extern unsigned char checking_ifelse;
extern unsigned char script_tics;
extern short disable_armor, facing_sprite;
extern signed char player_vel[], player_svel[];
extern unsigned long player_bits[];

extern void *bootleg_cache_alloc(long size);
extern void bootleg_cache_pop(void);
extern void hitscan(long x, long y, long z, short sectnum,
    long vx, long vy, long vz, short *hitsect, short *hitwall,
    short *hitsprite, long *hitx, long *hity, long *hitz);
extern short getangle(long x, long y);
extern short ssp(short i, long cliptype);
extern long krand(void);
extern int cansee(long x1, long y1, long z1, short sect1,
    long x2, long y2, long z2, short sect2);
extern long ifhitbyweapon(short i);
extern long ifsquished(short i, short p);
extern void shoot(short i, short tile);
extern void spritesound(short sound, short i);
extern void nuke(long x, long y, long z, short i);
extern void addweapon(short p, short tile, short amount);
extern short spawn(short i, short tile);
extern void resetplayer(short p);
extern void hitradius(short i, long radius, long damage);
extern void guts(short i);
extern void gameexit(char *message);
extern int deletesprite(short i);
extern short EGS(short sect, long x, long y, long z, unsigned char cstat, short picnum,
    signed char shade, unsigned char xrepeat, unsigned char yrepeat,
    short ang, short xvel, short zvel, short owner, short statnum);

/* Build's original inline helper, present in the June 1994 PRAGMAS.H. */
extern long klabs(long value);
#pragma aux klabs = \
    "test eax,eax" \
    "jns skipnegate" \
    "neg eax" \
    "skipnegate:" \
    parm nomemory [eax];

/* Watcom CLIB3R dword fill entry and register ABI. */
extern void fill_dwords(void *dest, unsigned long value, unsigned long count);
#pragma aux fill_dwords "__STOSD" parm [eax] [edx] [ecx] modify exact [eax ecx edx];

short getincangle_shift(short a, short na, short shift);
char ispecial(char c);
char isaltok(char c);
void getlabel(void);
long keyword(void);
long transword(void);
void transnum(void);
char parsecommand(void);
void passone(void);
void loadefs(void);
short furthestangle(short i, short angs);
void move_actor(short i, short p, long x);
char parse(short i, short p, long x);
void execute(short i, short p, long x);

#endif
