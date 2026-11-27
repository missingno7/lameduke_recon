#ifndef GAMEDEF_H
#define GAMEDEF_H

#include "records.h"

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

/* Native actor scratch row. CON stores script addresses in the move and action cells. */
enum actor_cell {
    ACTOR_MOVE_COUNT, ACTOR_MOVE_PTR, ACTOR_ACTION_COUNT,
    ACTOR_ACTION_FRAME, ACTOR_ACTION_PTR
};

extern struct spritetype sprite[MAXSPRITES];
extern struct sectortype sector[1024];
extern long hittype[MAXSPRITES][5];
extern struct player_struct ps[17];
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
extern long *parsing_actor;
extern char parsing_state;
extern signed char brace_depth;
extern char *sound_names[39];
extern char music_fn[4][10][13], env_music_fn[10][13];
extern long *insptr;
extern char killit_flag;
extern short cached_cansee;
extern unsigned char checking_ifelse;
extern unsigned char synctics;
extern short disable_armor, facing_sprite;
extern signed char syncsvel[], syncvel[];
extern unsigned long syncbits[];

extern void *bootleg_cache_alloc(long size);
extern void bootleg_cache_pop(void);
/* Old-style declarations: GAMEDEF passes these arguments default-promoted (the oracle loads *insptr as a
   long for shoot/spawn/addweapon and computes EGS arguments in 32 bits without narrowing). */
extern void hitscan();
extern short getangle();
extern short ssp();
extern long krand();
extern int cansee();
extern long ifhitbyweapon();
extern long ifsquished();
extern void shoot();
extern void spritesound();
extern void nuke();
extern void addweapon();
extern short spawn();
extern void resetplayer();
extern void hitradius();
extern void guts();
extern void gameexit();
extern int deletesprite();
extern short EGS();

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

short getincangle(short a, short na, short shift);
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
