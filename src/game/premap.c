/* PREMAP: level setup (spawn, prelevel, newgame, enterlevel), player and level-state resets, the 96x96 camera/mirror tile renderer
   and the high-score entry helpers. Names follow Duke3D 1.5 PREMAP.C/GAME.C (1996) where the role matches; Ken's Sep-94 GAME.C prepareboard
   (K94s:640-959) is the ancestor of the reset/prelevel/enterlevel sequence. LameDuke's ps.startpos* is the respawn point; ps.opos* is the
   previous-tick position, as in Duke 1.5. */

#include <dos.h>
#include <fcntl.h>
#include <i86.h>
#include <io.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "build_v5.h"
#include "game.h"

extern unsigned outp(unsigned port, unsigned value);
extern void resetparallax(void);
extern void makepalookup(long palnum, char *remap, signed char red,
                        signed char green, signed char blue);
extern void bootleg_cache_pop(void);
extern void overwritesprite(long x, long y, long tile, long shade,
                            long orientation, long palette);
extern void getmousevalues(short *dx, short *dy, short *buttons);
extern int MUSIC_Shutdown(void);
extern int FX_Shutdown(void);
extern void sendlogoff(void);
extern void uninitmultiplayers(void);

/* PREMAP calls strget without a prototype (int arguments), as GAMEDEF does for its game calls. */
extern int strget();

extern void setvmode(long mode);
#pragma aux setvmode = \
    "int 0x10" \
    parm [eax];

extern char fta_quotes[13][32];

extern unsigned char scantoasc[128];
extern unsigned char scantoascwithshift[128];

extern void (__interrupt __far *oldtimerhandler)(void);
extern short hudpulse;
extern short sintable[2048];
extern unsigned char keys[17];
extern volatile unsigned char keystatus[256];
extern long turn_velocity, strafe_velocity, forward_velocity;
extern char tempbuf[];
extern long frameplace;
extern short screenpeek;
extern short myconnectindex;
extern short tilesizx[4096];
extern unsigned char typemessage[], typemessageleng;
extern short moustat, menu_mouse, menu_previous_buttons, menu_drag, menu_x, menu_y;

extern long reccnt;
extern signed char syncangvel[], syncsvel[], syncvel[];
extern long syncbits[];
extern unsigned char synctics;
extern signed char recsyncangvel[16384], recsyncsvel[16384], recsyncvel[16384];
extern long recsyncbits[16384];
extern unsigned char recsynctics[16384];

extern char diz[];
/* These two literals belong to an earlier part of the shared GAME pool. */
extern const char lookup_filename[];
extern char lookup_missing[];
/* The exit banner opens the shared GAME pool (2:00004); provisional like the two above. */
extern char exit_banner[];

typedef char user_defs_size_check[sizeof(struct user_defs) == 21 ? 1 : -1];

extern struct user_defs ud;
extern char score_names[12][33];
extern long high_scores[12], recstat;
extern const char userdefs_filename[];

typedef char player_size_check[sizeof(struct player_struct) == 176 ? 1 : -1];

extern struct player_struct ps[17];
extern char demo_filename[], demo_missing[];

extern long hittype[4096][5], msx[1024], msy[1024];
long tempwallptr;                     /* PREMAP _BSS at 2:0ac60 (gamesplit/dataown evidence) */
extern long *actorscrptr[4096];
extern short connecthead, connectpoint2[], randomstash;
extern short camerasprite, camsprite, gamepaused, numplayers, numsectors, numwalls;
extern unsigned char show2dsprite[512], show2dwall[1024];
extern unsigned char parallaxtype, visibility, option[];
extern long waloff[4096], totalclock, lockclock, randomseed;
extern long animatecnt;
extern short numanimwalls, tilesizy[4096];
extern char music_fn[4][10][13];

extern long krand(void);
extern long ksqrt(long square);
extern long getangle(long x, long y);
extern long getincangle(short angle, short target, short shift);
extern long setsprite(short i, long x, long y, long z);
extern int deletesprite(short i);
extern int changespritestat(short i, short stat);
extern short insertsprite(long sect, long stat);
extern void updatesector(long x, long y, short *sect);
extern long nextsectorneighborz(short sect, long z, short plane, short direction);
extern short findplayer(short i, long *distance);
extern long ifhitbyweapon(short i);
extern long getanimationgoal(long *goal);
extern long ifhitsectors(short i);
extern void ms(short i);
extern void nuke(long x, long y, long z, short i);
extern void operatesectors(short sect, short i);
extern void activatewarpelevators(short i, short direction);
extern void spritesound(long sound, short i);
extern void stopsound(long sound);
extern void rotatepoint(long x, long y, long pivot_x, long pivot_y, long ang,
    long *result_x, long *result_y);
extern int clipinsidebox(long x, long y, short w, long distance);
extern int cansee(long x1, long y1, long z1, short sect1,
    long x2, long y2, long z2, short sect2);
extern void clipmove(long *x, long *y, long *z, short *sect,
    long vx, long vy, long radius, long ceil_dist, long floor_dist, long cliptype);
extern void dragpoint(short w, long x, long y);

extern void setview(long left, long top, long right, long bottom);
extern void drawrooms(long x, long y, long z, long ang, long horizon, long sect);
extern void drawmasks(void);
extern void rotatesprite(long x, long y, long zoom, short ang, short tile);
extern void vscrn(short p);
extern void setgamemode(void);
extern void checkhitswitch(short w);
extern void loadtile(long tile);
extern void nextpage(void);
extern void FX_StopAllSounds(void);
extern int loadboard(char *name, long *x, long *y, long *z, short *ang, short *sect);
extern void precache(void);
extern void allignwarpelevators(void);
extern void playmusic(char *name);
extern void call_level_load_hook(char *name);
#pragma aux call_level_load_hook "level_load_hook_"
extern short disable_armor;
extern unsigned char level_tick_state;
extern signed char osyncvel[17], osyncsvel[17], osyncangvel[17];
extern long osyncbits[17];
extern struct sound_voice sound_voices[39];

/* Inline Build helpers from the historical PRAGMAS.H. */
extern long ksgn(long value);
#pragma aux ksgn = "add ebx,0x80000000" "sbb eax,eax" "add ebx,0x7fffffff" "adc eax,0" \
    parm nomemory [ebx] modify exact [eax ebx];
extern long klabs(long value);
#pragma aux klabs = "test eax,eax" "jns skipnegate" "neg eax" "skipnegate:" \
    parm nomemory [eax];
extern void copybuf(void *source, void *dest, long count);
/* Ken's 1994 copybuf (BUIL0422) has no modify clause: the callers reload their loop state. */
#pragma aux copybuf = "rep movsd" parm [esi] [edi] [ecx];


extern short user_quote_time;
extern unsigned char *user_quote;
extern short thesprite[], spritepicnum[];
extern char spritepal[];
extern long spritesortcnt;
extern unsigned char gotpic[];
extern short facing_sprite;
extern char playerreadyflag[16];
extern void draw2dscreen(long x, long y, long ang, long zoom, long grid);
extern void permanentwritesprite(long x, long y, long tile, long shade,
                                 long x1, long y1, long x2, long y2);
extern void animatewalls(void);
extern void animatecamsprite(short p);
extern short getpacket(short *other, unsigned char *buffer);
extern void sendpacket(long other, unsigned char *buffer, long length);
extern long getoutputcirclesize(void);
extern void getinput(short p);
extern void doanimations(long tics);
extern void moveexplosions(void);
extern void cheatkeys(short p);
extern void processinput(short p);
extern void checksectors(short p);
extern void moveplayers(void);
extern void moveweapons(void);
extern void movestandables(void);
extern void movefta(void);
extern void moveactors(void);
extern void movefx(void);

extern struct animwalltype animwall[256];

/* Declarations used by the functions in 1:04cd0-0ba30. */
extern char warp_on;
extern char output_path[];
extern char savegame_filename[], saveboard_filename[], saveboard_missing[];
extern char help_banner[], help_usage[], help_help[], help_level[], help_volume[];
extern char help_skill[], help_quotes[], help_debug[], help_record[], help_output[];
extern char help_encode[], unknown_parameter[], unknown_parameter2[];
extern char demorec_on[], output_rerouted[], encode_mapname[], encode_message[];
extern int saveboard(char *filename, long *x, long *y, long *z, short *ang, short *sectnum);
extern char main_banner[], main_loading_stats[], main_init_cache[], main_checking[];
extern char main_no_good[], main_init_engine[], main_multiplayer[], main_loading_art[];
extern char main_tiles_file[], main_init_remaps[], main_init_fx[], main_reading_sounds[];
extern char main_init_music[], main_locating[], main_master[], main_slave[], main_game_over[];
extern void init_bootleg_cache(void), loadefs(void), initmenus(void), initengine(void);
extern void initmultiplayers(unsigned char option, unsigned char rate);
extern void loadpics(char *filename), initlava(void), loadsounds(void);
extern int sound_device_map[];
extern struct fx_device fx_device;
extern char *FX_ErrorString(int error), *MUSIC_ErrorString(int error);
extern int FX_SetupCard(int card, void *device);
extern int FX_Init(int card, int voices, int channels, int bits, unsigned rate);
extern int FX_SetCallBack(void (*function)(unsigned long));
extern void FX_SetVolume(int volume), MUSIC_SetVolume(int volume);
extern int MUSIC_Init(int card, int address);
extern void TestCallBack(unsigned long number);
extern void draw_pause_sign(void);
/* Bonus-screen tally marks, x/y pairs, eight per episode (2:02304, GAME _DATA). */
extern short bonus_xy[3][16];
extern char bonus_press_space[], bonus_title[], bonus_secrets[], bonus_percent[];
extern char bonus_points[], bonus_squashed[], bonus_killed[];
extern long current_menu;
extern void displayweapon(short p), menus(void), pan3dsound(void);
extern void sendlogon(void);
extern char warp_title[], warp_episode[], warp_level[];
extern void screencapture(void);
extern void setbrightness(unsigned char brightness);
extern void MUSIC_Pause(void), MUSIC_Continue(void);
extern void qsetmode640480(void);
extern char score_rank_title[], score_quote_title[], score_score_title[];


/* dataown: one object, one name. GLOBAL defines the timer ramps as angvel/svel/vel and the 21-byte settings
   block as ud, and code uses those names. The unused externs turn_velocity, strafe_velocity,
   forward_velocity, camerasprite, gamepaused, disable_armor and facing_sprite above only keep this
   unit's symbol slots (docs/BSS_LAYOUT.md). */
extern long angvel, svel, vel;

/* The status bar drawing, written out through the helper calls. */

/* Set up map sprite pn (j < 0) or spawn tile pn at sprite j; per-picnum defaults and sector-effector setup. */
void spawn(long j, short pn)
{
    short sect, i, endwall, startwall, s, clostest;
    long x, y, d;

    if (j >= 0) {
        i = EGS(sprite[j].sectnum, sprite[j].x, sprite[j].y, sprite[j].z,
                0, pn, 0, 0, 0, 0, 0, 0, sprite[j].picnum, 0);
    } else {
        i = pn;
        s = sprite[i].picnum;
        hittype[i][3] = 0;
        hittype[i][2] = 0;
        hittype[i][0] = 0;
        if (s >= 265 && s <= 269)
            s = 265;
        if (actorscrptr[s]) {
            sprite[i].extra = *(actorscrptr[s]);
            hittype[i][4] = *(actorscrptr[s] + 1);
            hittype[i][1] = *(actorscrptr[s] + 2);
            if (s != 57)
                sprite[i].hitag = *(actorscrptr[s] + 3);
        } else {
            hittype[i][4] = 0;
            hittype[i][1] = 0;
        }
    }
    sect = sprite[i].sectnum;
    switch (sprite[i].picnum) {
    case 1408:
        sprite[i].owner = -1;
        changespritestat(i, STAT_ACTOR);
        break;
    case 1165:
        if (j >= 0) {
            short a;

            if (sprite[j].picnum == 283) {
                short p = sprite[j].yvel;

                a = (short)(ps[p].ang - (krand() & 0x3F));
                hittype[i][0] = krand() & 1;
                sprite[i].x = sprite[j].x + (sintable[(a + 512) & 2047] >> 6);
                sprite[i].y = (sintable[a & 2047] >> 6) + sprite[j].y;
                sprite[i].z = sprite[j].z - 32 * ps[p].horiz + 4608;
                sprite[i].zvel = (short)-(krand() & 0xFF);
            } else {
                a = sprite[i].ang;
                sprite[i].x = sprite[j].x + (sintable[(a + 512) & 2047] >> 6);
                sprite[i].y = (sintable[a & 2047] >> 6) + sprite[j].y;
                sprite[i].z = sprite[j].z - 9216;
            }
            sprite[i].shade = -8;
            sprite[i].ang = (short)(a - 512);
            sprite[i].zvel = -192;
            sprite[i].xvel = 48;
            sprite[i].xrepeat = sprite[i].yrepeat = 5;
            changespritestat(i, STAT_MISC);
        }
        break;
    case 1406:
        if (j >= 0) {
            sprite[i].xrepeat = sprite[i].yrepeat = 16;
            sprite[i].shade = -15;
            sprite[i].cstat |= rand() & 0xE;
            sprite[i].ang = sprite[j].ang;
            sprite[i].xvel = sprite[j].xvel;
            sprite[i].zvel = sprite[j].zvel;
            sprite[i].owner = j;
            sprite[i].z -= 0x2000;
            changespritestat(i, STAT_PROJECTILE);
        }
        break;
    case 5:
        sprite[i].xrepeat = sprite[i].yrepeat = 0;
        changespritestat(i, STAT_FX);
        break;
    case 1404:
        if (j >= 0) {
            sprite[i].owner = j;
            sprite[i].xrepeat = (unsigned char)(tilesizx[sprite[j].picnum] / 3);
            sprite[i].yrepeat = (unsigned char)(tilesizy[sprite[j].picnum] / 3);
            sprite[i].shade = -127;
        }
        changespritestat(i, STAT_MISC);
        break;
    case 1359:
        sprite[i].xrepeat = sprite[i].yrepeat = 32;
        sprite[i].cstat |= 0x22;
        sprite[i].ang = (short)(krand() & 2047);
        sprite[i].shade = 0;
        sprite[i].z = sector[sect].floorz;
        changespritestat(i, STAT_MISC);
        break;
    case 1360:
        if (j >= 0 && sprite[j].statnum == STAT_PLAYER)
            sprite[i].z += 4096;
        /* fall through */
    case 686: case 1370:
        if (sprite[i].picnum == 1360)
            sprite[i].xrepeat = sprite[i].yrepeat = 8;
        else
            sprite[i].xrepeat = sprite[i].yrepeat = 28;
        sprite[i].cstat = (unsigned char)(krand() & 0xC);
        sprite[i].shade = -127;
        sprite[i].ang = sprite[j].ang;
        changespritestat(i, STAT_MISC);
        break;
    case 283:
        sprite[i].xrepeat = sprite[i].yrepeat = 0;
        sprite[i].shade = -32;
        changespritestat(i, STAT_PLAYER);
        break;
    case 215:
        sprite[i].xrepeat = sprite[i].yrepeat = 10;
        sprite[i].shade = -16;
        changespritestat(i, STAT_MISC);
        break;
    case 1280:
        sprite[i].xrepeat = sprite[i].yrepeat = 64;
        sprite[i].shade = -127;
        sprite[i].cstat |= 1;
        changespritestat(i, STAT_ZOMBIEACTOR);
        break;
    case 1175: {
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].cstat |= 0x40;
        sprite[i].picnum += 2;
        sprite[i].z = sector[sect].ceilingz + 12288;
        hittype[i][4] = tempwallptr;
        msx[tempwallptr] = sprite[i].x;
        msy[tempwallptr] = sprite[i].y;
        msx[tempwallptr + 2] = sprite[i].z;
        for (s = headspritestat[STAT_DEFAULT]; s >= 0; s = nextspritestat[s]) {
            if (sprite[s].picnum == 1174 && sprite[s].hitag == sprite[i].hitag) {
                hittype[i][1] = sprite[s].sectnum;
                sprite[s].xrepeat = 48;
                sprite[s].yrepeat = 128;
                msx[tempwallptr + 1] = sprite[s].x;
                msy[tempwallptr + 1] = sprite[s].y;
                sprite[s].x = sprite[i].x;
                sprite[s].y = sprite[i].y;
                sprite[s].z = sprite[i].z;
                sprite[s].shade = sprite[i].shade;
                msy[tempwallptr + 2] = s;
                setsprite(s, sprite[s].x, sprite[s].y, sprite[s].z);
                break;
            }
        }
        sprite[i].owner = -1;
        tempwallptr += 3;
        changespritestat(i, STAT_STANDABLE);
        break;
    }
    case 1190:
        sprite[i].xrepeat = sprite[i].yrepeat = 20;
        sprite[i].shade = -16;
        changespritestat(i, STAT_MISC);
        break;
    case 960:
        sprite[i].xrepeat = 16;
        sprite[i].yrepeat = 0;
        hittype[i][1] = krand() & 0x1F;
        sprite[i].z = sector[sect].ceilingz + 2048;
        changespritestat(i, STAT_ZOMBIEACTOR);
        break;
    case 962:
        sprite[i].z = sector[sect].ceilingz + 6144;
        hittype[i][0] = 3;
        /* fall through */
    case 961:
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].shade = -16;
        sprite[i].xrepeat = 32;
        sprite[i].yrepeat = 16;
        sprite[i].cstat |= 1;
        changespritestat(i, STAT_ZOMBIEACTOR);
        break;
    case 305:
        hittype[i][0] = sprite[i].xrepeat;
        hittype[i][1] = sprite[i].yrepeat;
        /* fall through */
    case 32: case 51: case 216:
        sprite[i].xrepeat = sprite[i].yrepeat = 0;
        sprite[i].yvel = 0;
        changespritestat(i, STAT_ACTOR);
        break;
    case 408: case 490: case 579: case 1536: case 1680: case 1820:
        ps[0].max_squashable++;
        ps[0].max_actors_killed++;
        /* fall through */
    case 631:
        sprite[i].shade = -24;
        sprite[i].yrepeat = 43;
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].xrepeat = 52;
        /* COMPILER_STEERED: the explicit assignment affects Watcom register allocation. */
        if (sprite[i].picnum == 408)
            sprite[i].xrepeat = sprite[i].xrepeat - 9;
        changespritestat(i, STAT_ZOMBIEACTOR);
        sprite[i].cstat |= 1;
        break;
    case 34:
        changespritestat(i, STAT_LOCATOR);
        sprite[i].xrepeat = sprite[i].yrepeat = 0;
        break;
    case 23: case 31:
        sprite[i].xrepeat = sprite[i].yrepeat = 0;
        if (sprite[i].picnum == 23)
            sector[sect].lotag ^= 0x4000;
        changespritestat(i, STAT_ACTIVATOR);
        break;
    case 30: case 313: case 314: case 1123:
        sprite[i].cstat |= 1;
        /* fall through */
    case 1122:
        sprite[i].shade = -12;
        show2dsprite[i >> 3] |= 1 << (i & 7);
        changespritestat(i, STAT_ACTOR);
        break;
    case 38: case 60: case 70: case 80: case 159: case 263: case 315: case 316: case 381: case 620:
    case 677: case 720: case 721: case 722: case 723: case 724: case 726: case 727: case 728:
    case 1119: case 1120: case 1121: case 1125:
        sprite[i].shade = -17;
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].cstat |= 1;
        changespritestat(i, STAT_ZOMBIEACTOR);
        break;
    case 931:
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].xrepeat = sprite[i].yrepeat = 32;
        hittype[i][3] = sector[sect].ceilingz;
        sprite[i].z = hittype[i][3] + 10752;
        sprite[i].cstat |= 9;
        ps[0].max_actors_killed++;
        changespritestat(i, STAT_ZOMBIEACTOR);
        break;
    case 930:
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].xrepeat = sprite[i].yrepeat = 32;
        hittype[i][3] = sector[sect].floorz;
        sprite[i].z = hittype[i][3] + 3072;
        sprite[i].cstat |= 1;
        ps[0].max_actors_killed++;
        changespritestat(i, STAT_ZOMBIEACTOR);
        break;
    case 940:
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].owner = i;
        sprite[i].xrepeat = sprite[i].yrepeat = 24;
        sprite[i].cstat |= 1;
        ps[0].max_actors_killed++;
        changespritestat(i, STAT_PROJECTILE);
        break;
    case 323:
        sprite[i].z = sector[sect].floorz - (sprite[i].yrepeat << 7);
        break;
    case 265: case 266: case 267: case 268: case 269:
        sprite[i].picnum = 265;
        /* fall through */
    case 327:
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].shade = -16;
        changespritestat(i, STAT_ACTOR);
        break;
    case 175:
        sprite[i].yvel = sector[sect].extra;
        sprite[i].xrepeat = sprite[i].yrepeat = 0;
        switch (sprite[i].lotag) {
        case 27:
            if (recstat == 1)
                sprite[i].xrepeat = sprite[i].yrepeat = 64;
            break;
        case 12:
            hittype[i][1] = sector[sect].floorshade;
            hittype[i][2] = sector[sect].ceilingshade;
            break;
        case 13:
            hittype[i][0] = sector[sect].ceilingz;
            hittype[i][1] = sector[sect].floorz;
            if (klabs(hittype[i][0] - sprite[i].z) < klabs(hittype[i][1] - sprite[i].z))
                sprite[i].owner = 1;
            else
                sprite[i].owner = 0;
            if (sprite[i].ang == 512) {
                if (sprite[i].owner)
                    sector[sect].ceilingz = sprite[i].z;
                else
                    sector[sect].floorz = sprite[i].z;
            } else
                sector[sect].ceilingz = sector[sect].floorz = sprite[i].z;
            if (sector[sect].ceilingstat & 1) {
                sector[sect].ceilingstat ^= 1;
                hittype[i][3] = 1;
                if (!sprite[i].owner && sprite[i].ang == 512) {
                    sector[sect].ceilingstat ^= 1;
                    hittype[i][3] = 0;
                }
                sector[sect].ceilingshade = sector[sect].floorshade;
                if (sprite[i].ang == 512) {
                    startwall = sector[sect].wallptr;
                    endwall = startwall + sector[sect].wallnum;
                    for (j = startwall; j < endwall; j++) {
                        x = wall[j].nextsector1;
                        if (x >= 0)
                            if (!(sector[x].ceilingstat & 1)) {
                                sector[sect].ceilingpicnum = sector[x].ceilingpicnum;
                                sector[sect].ceilingshade = sector[x].ceilingshade;
                                break;
                            }
                    }
                }
            }
            break;
        case 17:
            hittype[i][2] = sector[sect].floorz;
            j = nextsectorneighborz(sect, sector[sect].floorz, -1, -1);
            hittype[i][3] = sector[j].ceilingz;
            j = nextsectorneighborz(sect, sector[sect].ceilingz, 1, 1);
            hittype[i][4] = sector[j].floorz;
            break;
        case 24:
            sprite[i].yvel <<= 1;
            sector[sect].floorstat |= 64;
            break;
        case 23:
            sprite[i].xrepeat = sprite[i].yrepeat = 64;
            sprite[i].shade = 0;
            hittype[i][1] = tempwallptr;
            x = tempwallptr;
            for (s = 0; s < numsectors; s++)
                if (sector[s].hitag == sprite[i].hitag) {
                    msx[x] = s;
                    x++;
                    tempwallptr++;
                    hittype[i][0]++;
                }
            break;
        case 20: {
            long q;

            startwall = sector[sect].wallptr;
            endwall = startwall + sector[sect].wallnum;
            q = 0x7fffffff;
            for (s = startwall; s < endwall; s++) {
                x = wall[s].x;
                y = wall[s].y;
                d = ksqrt((sprite[i].y - y) * (sprite[i].y - y) + (sprite[i].x - x) * (sprite[i].x - x));
                if (d < q) {
                    q = d;
                    clostest = s;
                }
            }
            hittype[i][1] = clostest;
            q = 0x7fffffff;
            for (s = startwall; s < endwall; s++) {
                x = wall[s].x;
                y = wall[s].y;
                d = ksqrt((sprite[i].y - y) * (sprite[i].y - y) + (sprite[i].x - x) * (sprite[i].x - x));
                if (d < q && s != hittype[i][1]) {
                    q = d;
                    clostest = s;
                }
            }
            hittype[i][2] = clostest;
        }
            break;
        case 3:
            hittype[i][3] = sector[sect].floorshade;
            sector[sect].floorshade = sprite[i].shade;
            sector[sect].ceilingshade = sprite[i].shade;
            sprite[i].owner = sector[sect].ceilingpal << 8;
            sprite[i].owner |= sector[sect].floorpal;
            startwall = sector[sect].wallptr;
            endwall = startwall + sector[sect].wallnum;
            for (s = startwall; s < endwall; s++)
                if (!(wall[s].hitag & 1))
                    wall[s].shade = sprite[i].shade;
            break;
        case 4:
            hittype[i][2] = sector[sect].floorshade;
            startwall = sector[sect].wallptr;
            endwall = startwall + sector[sect].wallnum;
            sprite[i].owner = sector[sect].ceilingpal << 8;
            sprite[i].owner |= sector[sect].floorpal;
            for (s = startwall; s < endwall; s++)
                if (wall[s].shade > hittype[i][3])
                    hittype[i][3] = wall[s].shade;
            break;
        case 9:
            if ((sector[sect].lotag & 32767) && labs(sector[sect].ceilingz - sprite[i].z) > 1024)
                sector[sect].lotag |= 32768;
            /* fall through */
        case 8:
            hittype[i][0] = sector[sect].floorshade;
            hittype[i][1] = sector[sect].ceilingshade;
            startwall = sector[sect].wallptr;
            endwall = startwall + sector[sect].wallnum;
            for (s = startwall; s < endwall; s++)
                if (wall[s].shade > hittype[i][2])
                    hittype[i][2] = wall[s].shade;
            hittype[i][3] = (sprite[i].yvel >> 8) + 2;
            if (hittype[i][3] < 8)
                hittype[i][3] = 1;
            hittype[i][3] = 8; /* Final shade step in the executable. */
            break;
        case 11:
            if (sprite[i].ang > 1024)
                hittype[i][3] = 2;
            else
                hittype[i][3] = -2;
            /* fall through */
        case 0: case 2: case 5: case 6: case 14: case 15: case 16: case 25: case 26:
            startwall = sector[sect].wallptr;
            endwall = startwall + sector[sect].wallnum - 1;
            hittype[i][1] = tempwallptr;
            for (s = startwall; s <= endwall; s++) {
                msx[tempwallptr] = wall[s].x - sprite[i].x;
                msy[tempwallptr] = wall[s].y - sprite[i].y;
                tempwallptr++;
            }
            if (sprite[i].lotag == 6 || sprite[i].lotag == 14 || sprite[i].lotag == 5) {
                if (sector[sprite[i].sectnum].hitag != -1) {
                    sector[sect].floorstat |= 64;
                    sector[sect].ceilingstat |= 64;
                }
                /* s still holds the wall loop's end index: the first sector hint. */
                for (j = 0; j < 10; j++) {
                    updatesector(sprite[i].x + j * (sintable[(sprite[i].ang + 512) & 2047] >> 5),
                        sprite[i].y + j * (sintable[sprite[i].ang & 2047] >> 5), &s);
                    if (sector[s].hitag == 0)
                        break;
                    if (sector[s].hitag != -1) {
                        sector[s].ceilingstat |= 64;
                        sector[s].floorstat |= 64;
                    }
                }
                sprite[i].owner = -1;
                hittype[i][0] = s;
                hittype[i][3] = sprite[i].hitag;
            }
            if (sprite[i].lotag == 16)
                hittype[i][3] = sector[sect].ceilingz;
            if (sprite[i].lotag == 25) {
                hittype[i][4] = sector[sect].ceilingz;
                sector[sect].ceilingz = sprite[i].z;
            }
            if (sprite[i].lotag == 26) {
                hittype[i][3] = sprite[i].x;
                hittype[i][4] = sprite[i].y;
                if (sprite[i].shade == sector[sect].floorshade)
                    sprite[i].zvel = -256;
                else
                    sprite[i].zvel = 256;
                sector[sect].floorstat |= 64;
                sprite[i].shade = 0;
            }
            break;
        }
        if (sprite[i].lotag == 7)
            changespritestat(i, 9);
        else
            changespritestat(i, 3);
        break;
    case 57:
        sprite[i].owner = i;
        sprite[i].shade = -16;
        if (sprite[i].xrepeat <= 8)
            sprite[i].xrepeat = sprite[i].yrepeat = 0;
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].cstat |= 1;
        changespritestat(i, STAT_STANDABLE);
        break;
    case 606: case 627: case 905: case 1200:
        sprite[i].owner = i;
        sprite[i].shade = -16;
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].yrepeat = sprite[i].xrepeat = 38;
        sprite[i].cstat |= 1;
        changespritestat(i, STAT_STANDABLE);
        break;
    }
}

/* The window is rendered into the framebuffer, copied to the tile, then
 * drawn back at 1:1 before a second copy establishes the tile's orientation. */
/* Offsets advance by eight rows per iteration in each buffer. */
#define COPY_CAMERA_WINDOW(tile, row, dest)                                                  \
    for (; row < 320 * 95; row += 320 * 8, dest += 96 * 8) {                                 \
        copybuf((void *)(frameplace + row), (void *)(waloff[tile] + dest), 24);              \
        copybuf((void *)(frameplace + row + 320), (void *)(waloff[tile] + dest + 96), 24);   \
        copybuf((void *)(frameplace + row + 640), (void *)(waloff[tile] + dest + 192), 24);  \
        copybuf((void *)(frameplace + row + 960), (void *)(waloff[tile] + dest + 288), 24);  \
        copybuf((void *)(frameplace + row + 1280), (void *)(waloff[tile] + dest + 384), 24); \
        copybuf((void *)(frameplace + row + 1600), (void *)(waloff[tile] + dest + 480), 24); \
        copybuf((void *)(frameplace + row + 1920), (void *)(waloff[tile] + dest + 576), 24); \
        copybuf((void *)(frameplace + row + 2240), (void *)(waloff[tile] + dest + 672), 24); \
    }

/* Render a 96x96 view from (x,y,z,ang,sect) into tile tilenume (viewscreens, mirror). */
void render_camera_tile(long x, long y, long z, short ang, short sect, short tilenume)
{
    short row, dest, row2, dest2;

    setview(0, 0, 95, 95);
    drawrooms(x, y, z, ang, 100, sect);
    animatesprites(ang);
    drawmasks();
    row = dest = 0;
    COPY_CAMERA_WINDOW(tilenume, row, dest)
    row2 = dest2 = 0;
    rotatesprite(47L << 16, 47L << 16, 1L << 16, 0, tilenume);
    COPY_CAMERA_WINDOW(tilenume, row2, dest2)
    vscrn(screenpeek);
}

/* Render sprite i's viewpoint into its own tile. */
void render_sprite_camera(short i)
{
    render_camera_tile(sprite[i].x, sprite[i].y, sprite[i].z, sprite[i].ang, sprite[i].sectnum,
                       sprite[i].picnum);
}

/* Draw the fake mirror: the view from wall w's midpoint toward the viewer, into MIRROR (570). */
void render_mirror(short w)
{
    long x, y, z, x1, y1, x2, y2, player_x;
    short ang;

    x1 = wall[w].x;
    y1 = wall[w].y;
    x2 = wall[wall[w].point2].x;
    y2 = wall[wall[w].point2].y;
    player_x = ps[screenpeek].posx;
    x = (x1 + x2) / 2;
    y = (y1 + y2) / 2;
    z = ps[screenpeek].posz;
    ang = getangle(player_x - x, ps[screenpeek].posy - y);
    render_camera_tile(x, y, z, ang, wall[w].nextsector1, 570);
}

#undef COPY_CAMERA_WINDOW

/* Respawn player snum at its start point with fresh view, motion and weapon-raise state (CON 'resetplayer'). */
void resetplayer(short snum)
{
    short look;

    ps[snum].horiz = 100;
    ps[snum].posx = ps[snum].startposx;
    ps[snum].posy = ps[snum].startposy;
    ps[snum].posz = ps[snum].startposz;
    ps[snum].ang = ps[snum].startang;
    ps[snum].cursectnum = ps[snum].startsectnum;
    ps[snum].on_ground = 0;
    ps[snum].bobcounter = 0;
    ps[snum].knee_flash = 0;
    look = 256 - (krand() & 512);
    ps[snum].look_ang = look;
    ps[snum].newowner = -1;
    ps[snum].jumping_counter = 0;
    ps[snum].hard_landing = 0;
    ps[snum].poszv = 0;
    ps[snum].somethingonplayer = -1;
    ps[snum].holoduke_on = -1;
    ps[snum].weapon_clock = 0;
    if (actorscrptr[283] != NULL)
        ps[snum].health = (short)*actorscrptr[283];
    else
        ps[snum].health = 100;
    ps[snum].on_crane = -1;
    ps[snum].hbomb_on = 0;
    ps[snum].kickback_pic = 5;
    ps[snum].weapon_pos = 6;
    ps[snum].knuckle_incs = 1;
    ps[snum].zoom = 768;
}

/* Reset level tallies and global level state before a level starts (Duke3D 1.5 resetprestat), then resetplayer. */
void resetprestat(short snum)
{
    ps[snum].toggle_key_flag = 0;
    ps[snum].secret_rooms = 0;
    ps[snum].max_secret_rooms = 0;
    ps[snum].squashable = 0;
    ps[snum].max_squashable = 0;
    ps[snum].actors_killed = 0;
    ps[snum].max_actors_killed = 0;
    ps[snum].walking_snd_toggle = 0;
    if (ps[snum].dimensionmode == 2)
        setgamemode();
    ps[snum].dimensionmode = 3;
    ps[snum].startposx = ps[snum].posx;
    ps[snum].startposy = ps[snum].posy;
    ps[snum].startposz = ps[snum].posz;
    ps[snum].startang = ps[snum].ang;
    ps[snum].startsectnum = ps[snum].cursectnum;
    vel = 0;
    svel = 0;
    angvel = 0;
    ps[snum].one_parallax_sectnum = -1;
    screenpeek = myconnectindex;
    visibility = ud.visibility;
    numanimwalls = 0;
    animatecnt = 0;
    parallaxtype = 0;
    randomseed = 17;
    ud.gamepaused = 0;
    ud.camerasprite = -1;
    tempwallptr = 0;
    camsprite = -1;
    resetplayer(snum);
}

/* Set up a freshly loaded board: reset state, scan sector tags, spawn all map sprites, collect animated walls. */
void prelevel(void)
{
    short i, k, sect, nexti;
    long j;
    long *actor;

    resetprestat(0);

    for (i = 0; i < 13; i++)
        ps[0].fta[i] = 0;
    for (i = 0; i < 512; i++)
        show2dwall[i] = 255;
    for (i = 0; i < 512; i++)
        show2dsprite[i] = 255 * ud.debug;

    for (i = 0; i < numsectors; i++) {
        sector[i].extra = 256;
        if (ps[0].one_parallax_sectnum == -1 && (sector[i].ceilingstat & 1))
            ps[0].one_parallax_sectnum = i;
        if ((sector[i].lotag & 32767) == 32767)
            ps[0].max_secret_rooms++;
        else if (sector[i].lotag == 2)
            sector[i].ceilingpal = sector[i].floorpal = 1;
    }
    for (i = headspritestat[STAT_DEFAULT]; i >= 0; i = nexti) {
        nexti = nextspritestat[i];
        if (sprite[i].picnum == 310) {
            sector[sprite[i].sectnum].extra = sprite[i].lotag;
            deletesprite(i);
        }
    }

    /* Initialize the map sprites in index order. */
    for (i = 0; i < 4096; i++)
        if (sprite[i].statnum < 1024)
            spawn(-1, i);

    for (i = 0; i < numwalls; i++) {
        if (numanimwalls >= 256)
            gameexit("Maxed out on anim walls\n");
        switch (wall[i].overpicnum) {
        case 1184:
            if (waloff[1184] == -1)
                for (k = 0; k < 3; k++)
                    loadtile(1184 + k);
            /* fall through */
        case 1185: case 1186:
            if (wall[i].shade > 31)
                wall[i].cstat = 0;
            else
                wall[i].cstat |= 1;
            wall[i].shade = 0;
            if (wall[i].lotag)
                wall[wall[i].nextwall1].lotag = wall[i].lotag;
            animwall[numanimwalls].wallnum = i;
            numanimwalls++;
            continue;
        }
        switch (wall[i].picnum) {
        case 819: case 823: case 860: case 864:
            if (waloff[wall[i].picnum] == -1)
                for (k = 0; k < 4; k++)
                    loadtile(wall[i].picnum + k);
            /* fall through */
        case 570:
            animwall[numanimwalls].wallnum = i;
            numanimwalls++;
            break;
        case 761:
            if (waloff[761] == -1)
                for (k = 761; k < 765; k++)
                    loadtile(k);
            /* fall through */
        case 764: case 765: case 766: case 767:
            animwall[numanimwalls].wallnum = i;
            animwall[numanimwalls++].tag = wall[i].picnum;
            break;
        case 406:
            wall[i].picnum = 405;
            checkhitswitch(wall[i].lotag);
            /* fall through */
        case 405:
            animwall[numanimwalls].tag = 0;
            animwall[numanimwalls].wallnum = i;
            numanimwalls++;
            break;
        case 395: case 396:
            animwall[numanimwalls].tag = 0;
            animwall[numanimwalls].wallnum = i;
            numanimwalls++;
            break;
        }
    }
}

/* Start a new game: episode vn, level ln, skill sk; fresh inventory, pistol and ammo for player 0. */
void newgame(unsigned char vn, unsigned char ln, unsigned char sk)
{
    ps[0].level = ln;
    ps[0].volume = vn;
    ps[0].skill = sk;
    ps[0].scuba_on = 0;
    ps[0].inventory[0] = 0;
    ps[0].jetpack_on = 0;
    ps[0].inventory[1] = 0;
    ps[0].steroids_on = 0;
    ps[0].inventory[2] = 0;
    ps[0].shield_amount = 0;
    ps[0].inventory[3] = 0;
    ps[0].show_inventory = 0;
    ps[0].gotweapon[0] = 0;
    ps[0].gotweapon[2] = 0;
    ps[0].gotweapon[3] = 0;
    ps[0].gotweapon[4] = 0;
    ps[0].screen_size = -4;
    ps[0].gotweapon[1] = 1;
    ps[0].score = 0;
    if (ud.debug) {
        ps[0].ammo_r_amount = 99;
        ps[0].ammo_h_amount = 696;
        ps[0].ammo_amount = 144;
        ps[0].ammo_bat_amount = 120;
    } else {
        ps[0].ammo_r_amount = 5;
        ps[0].ammo_h_amount = 2;
        ps[0].ammo_amount = 24;
        ps[0].ammo_bat_amount = 9;
    }
    ps[0].last_weapon = -1;
    ps[0].current_weapon = 191;
    reccnt = 0;
}

/* Load and start the current level; g becomes the game mode (MODE_GAME / MODE_DEMO). */
void enterlevel(unsigned char g)
{
    char *levname = "L1.MAP";
    short i, p, load_result;
    unsigned char which_palookup;

    if (ud.debug == 0 && (g & 4)) {
        dobonus();
    } else {
        overwritesprite(0, 0, 482, 0, 0, 0);
        minitext(170, 50, (unsigned char *)"Entering...", 0);
        minitext(174, 62, (unsigned char *)"Episode", 0);
        minitext(174, 74, (unsigned char *)"Level", 0);
        numtext(232, 62, ps[0].volume + 1, 0);
        numtext(232, 74, ps[0].level + 1, 0);
        nextpage();
    }
    levname[0] = 'L' + ps[0].volume;
    levname[1] = '1' + ps[0].level;
    FX_StopAllSounds();
    call_level_load_hook(levname);
    load_result = loadboard(levname, &ps[0].posx, &ps[0].posy, &ps[0].posz,
                            &ps[0].ang, &ps[0].cursectnum);
    call_level_load_hook(levname);
    if (load_result == -1)
        gameexit("Board not found.");
    prelevel();
    EGS(ps[0].cursectnum, ps[0].posx, ps[0].posy, ps[0].posz, 0, 283, 0, 0, 0,
        ps[0].ang, 0, 0, 0, 10);
    precache();
    cacheit();
    allignwarpelevators();
    for (i = 0; i < 17; i++) {
        memcpy(&ps[i], &ps[0], sizeof(ps[0]));
        syncvel[i] = osyncvel[i] = 0;
        syncsvel[i] = osyncsvel[i] = 0;
        syncangvel[i] = osyncangvel[i] = 0;
        syncbits[i] = osyncbits[i] = 0;
    }
    waitforeverybody();
    if (option[0] == 7)
        setview(0, 0, 639, 479);
    else
        vscrn(0);

    which_palookup = 0;
    i = headspritestat[STAT_PLAYER];
    for (p = connecthead; p >= 0; p = connectpoint2[p]) {
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].cstat = 1;
        sprite[i].shade = -16;
        sprite[i].owner = i;
        sprite[i].yvel = p;
        ps[p].palookup = which_palookup;
        if (which_palookup != 0)
            which_palookup++;
        else
            which_palookup = 3;
        sprite[i].xrepeat = sprite[i].yrepeat = 42;
        ps[p].i = i;
        if (numplayers > 1) {
            ps[p].startposx = ps[p].posx = sprite[i].x;
            ps[p].startposy = ps[p].posy = sprite[i].y;
            ps[p].startposz = ps[p].posz = sprite[i].z;
            ps[p].startang = ps[p].ang = sprite[i].ang;
            ps[p].startsectnum = ps[p].cursectnum = sprite[i].sectnum;
        }
        updatesector(sprite[i].x, sprite[i].y, &ps[p].cursectnum);
        i = nextspritestat[i];
    }
    ps[myconnectindex].music = music_fn[ps[myconnectindex].volume][0] + 13 * ps[myconnectindex].level;
    playmusic(ps[myconnectindex].music);
    displaystatus(myconnectindex);
    lockclock = totalclock = 0;
    level_tick_state = synctics = 0;
    ps[myconnectindex].gm = g;
}

/* Check FILE_ID.DIZ against the embedded notice; nonzero = missing or modified. */
unsigned char check_file_id(void)
{
    int fil;
    long recpos, bufpos;

    fil = open("FILE_ID.DIZ", O_RDWR | O_BINARY, S_IREAD);
    if (fil == -1)
        return 1;

    read(fil, tempbuf, 450);
    bufpos = 0;
    for (recpos = 0; recpos < 400; recpos++, bufpos++) {
        if (diz[recpos] == '*') {
            recpos++;
            bufpos += 2;
        }
        if (tempbuf[bufpos] != diz[recpos]) {
            close(fil);
            return 1;
        }
    }
    close(fil);
    bootleg_cache_pop();
    return 0;
}


/* Place the current score in the high-score table; returns its row or -1. */
short newscore(void)
{
    short i, j;

    next_text_shade();
    for (i = 0; i < 12; i++) {
        if (ps[myconnectindex].score > (unsigned long)high_scores[i]) {
            for (j = 10; j > i; j--) {
                high_scores[j + 1] = high_scores[j];
                memcpy(score_names[j + 1], score_names[j], 31);
            }
            score_names[i][0] = 0;
            high_scores[i] = ps[myconnectindex].score;
            return i;
        }
    }
    return -1;
}

/* Edit the high-score name on row; nonzero when entry is finished. */
unsigned char getscorename(short row)
{
    return strget(50, row * 11 + 50, next_text_shade(), 31) != 0;
}
