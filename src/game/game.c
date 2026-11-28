/* GAME: LameDuke's main module (obj1 1:00010-0f160): timer, keyboard ramps, master/slave packet sync and demo recording,
   descended from Ken Silverman's Build demo GAME.C (K94s Sep-94 getpackets/sync/playback; K95 Jun-95 timers, keytimerstuff, waitforeverybody),
   plus 3D Realms' HUD text, status bar, command line, main loop, bonus screen, saves and sector effectors,
   whose later forms are Duke Nukem 3D 1.5 GAME.C / PREMAP.C / ACTORS.C / MENUES.C (1996). */

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

extern void setvmode(long mode);
#pragma aux setvmode = \
    "int 0x10" \
    parm [eax];

extern char fta_quotes[13][32];

extern unsigned char scantoasc[128];
extern unsigned char scantoascwithshift[128];

void (__interrupt __far *oldtimerhandler)(void);
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
extern unsigned long syncbits[];
extern unsigned char synctics;
extern signed char recsyncangvel[16384], recsyncsvel[16384], recsyncvel[16384];
extern long recsyncbits[16384];
extern unsigned char recsynctics[16384];

extern char diz[];
/* These two literals belong to an earlier part of the shared GAME pool. */
#define lookup_filename "lookup.dat"
#define lookup_missing "Palette remaps not found."
/* The exit banner opens the shared GAME pool (2:00004); provisional like the two above. */
#define exit_banner "ÞßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßÝÞ DUKE DUKEM 3D Beta version 1.3.95 -- DO NOT DISTRIBUTE!!! That would be bad. ÝÞÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÝ"

typedef char user_defs_size_check[sizeof(struct user_defs) == 21 ? 1 : -1];

extern struct user_defs ud;
extern char score_names[12][33];
extern long high_scores[12], recstat;
#define userdefs_filename "userdefs.tmp"

typedef char player_size_check[sizeof(struct player_struct) == 176 ? 1 : -1];

extern struct player_struct ps[17];
#define demo_filename "mydemo__.dmo"
#define demo_missing "Demo not found."

extern long hittype[4096][5], msx[1024], msy[1024], tempwallptr;
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
/* GAME passes the full getangle result as the target. */
extern long getincangle();
/* COMPILER_STEERED: Watcom passes the promoted sprite index in EAX. */
extern long setsprite();
extern int deletesprite(short i);
extern int changespritestat(short i, short stat);
extern short insertsprite(long sect, long stat);
extern void updatesector(long x, long y, short *sect);
extern short nextsectorneighborz(short sect, long z, short plane, short direction);
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
/* COMPILER_STEERED: wall indices reach EBX at their promoted width. */
extern int clipinsidebox();
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
extern unsigned long osyncbits[17];
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
char warp_on;
extern char output_path[];
#define savegame_filename "SAVE___0.SAV"
#define saveboard_filename "SAVE_000.SAV"
#define saveboard_missing "Board not found."
#define help_banner "\n *  Here's a little command line help..."
#define help_usage "    %s [flags...]\n"
#define help_help "\t/?        This help message.\n"
#define help_level "\t/l#       Go to level #."
#define help_volume "\t/v#       Go to volume #."
#define help_skill "\t/s#       Skill # (0-3)"
#define help_quotes "\t/q        Turn on help quotes."
#define help_debug "\t/d        Activate DEBUG MODEs."
#define help_record "\t/r        Record a demo."
#define help_output "\t/oDIR     Reroute game output to DIR."
#define help_encode "\t/e        Encode maps..."
#define unknown_parameter "Unknown command line paramater '%s'\n"
#define unknown_parameter2 "Unknown command line paramater '%s'\n"
#define demorec_on "Demorec mode on..."
#define output_rerouted "I/O path rerouted to %s.\n"
#define encode_mapname "L?.MAP"
#define encode_message "Encoding %s\n"
extern int saveboard(char *filename, long *x, long *y, long *z, short *ang, short *sectnum);
#define main_banner "ÞßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßÝÞ DUKE DUKEM 3D Beta version 1.3.95 -- DO NOT DISTRIBUTE!!! That would be bad. ÝÞÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÝ"
#define main_loading_stats "Loading user stats..."
#define main_init_cache "Init BootLeg CacheMaster..."
#define main_checking "Checking program integrety..."
#define main_no_good "No good."
#define main_init_engine "Init engine..."
#define main_multiplayer "Checking for multiplayer..."
#define main_loading_art "Loading art..."
#define main_tiles_file "tiles000.art"
#define main_init_remaps "Init colorize/remaps..."
#define main_init_fx "Init sound FX..."
#define main_reading_sounds "Reading sounds..."
#define main_init_music "Init music..."
#define main_locating "Locating other players..."
#define main_master "Master"
#define main_slave "Slave"
#define main_game_over "Game Over, or is it?"
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
short bonus_xy[3][16] = {
    { 9, 60, 61, 82, 84, 57, 85, 112, 300, 64, 261, 83, 223, 97, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};
#define bonus_press_space "Press SPACE to continue"
#define bonus_title "BONUS"
#define bonus_secrets "Secret rooms Found"
#define bonus_percent "%, ="
#define bonus_points "points."
#define bonus_squashed "Bad guys squashed"
#define bonus_killed "Bad guys killed"
extern long current_menu;
extern void displayweapon(short p), menus(void), pan3dsound(void);
extern void sendlogon(void);
#define warp_title "Warp Mode"
#define warp_episode "Episode"
#define warp_level "Level"
extern void screencapture(void);
extern void setbrightness(unsigned char brightness);
extern void MUSIC_Pause(void), MUSIC_Continue(void);
extern void qsetmode640350(void);
#define score_rank_title "#"
#define score_quote_title "Hero's Quote"
#define score_score_title "Score"


/* dataown: one object, one name. GLOBAL defines the timer ramps as angvel/svel/vel and the 21-byte settings
   block as ud, and code uses those names. The unused externs turn_velocity, strafe_velocity,
   forward_velocity, camerasprite, gamepaused, disable_armor and facing_sprite above only keep this
   unit's symbol slots (docs/BSS_LAYOUT.md). */
extern long angvel, svel, vel;

/* The status bar drawing, written out through the helper calls. */

/* Set the PIT to 120 Hz and hook IRQ0 (INT 8) with timerhandler, saving the old vector. Ken Silverman, Build GAME.C (1995). */
void inittimer(void)
{
    outp(0x43, 0x36);
    outp(0x40, 0xd6);
    outp(0x40, 0x26);
    oldtimerhandler = _dos_getvect(8);
    _disable();
    _dos_setvect(8, timerhandler);
    _enable();
}

/* Restore the BIOS 18.2 Hz timer and the original IRQ0 handler. Ken Silverman, Build GAME.C (1995). */
void uninittimer(void)
{
    outp(0x43, 0x36);
    outp(0x40, 0xff);
    outp(0x40, 0xff);
    _disable();
    _dos_setvect(8, oldtimerhandler);
    _enable();
}

#define held(binding, scan) \
    ((keystatus[keys[binding]] | keystatus[scan] | keystatus[(scan) + 128]) > 0)

/* Per-tick keyboard ramps for angvel, svel (strafe) and vel (forward). After Ken Silverman's keytimerstuff, Build GAME.C (1995). */
void keytimerstuff(void)
{
    if (!keystatus[keys[5]]) {
        if (held(2, 0x4b)) {
            angvel -= 16;
            if (angvel < -128)
                angvel = -128;
        }
        if (held(3, 0x4d)) {
            angvel += 16;
            if (angvel > 127)
                angvel = 127;
        }
    } else {
        if (held(2, 0x4b)) {
            svel += 8;
            if (svel > 127)
                svel = 127;
        }
        if (held(3, 0x4d)) {
            svel -= 8;
            if (svel < -128)
                svel = -128;
        }
    }
    if (held(0, 0x48)) {
        vel += 8;
        if (vel > 127)
            vel = 127;
    }
    if (held(1, 0x50)) {
        vel -= 8;
        if (vel < -128)
            vel = -128;
    }

    if (angvel < 0) {
        angvel += 10;
        if (angvel > 0)
            angvel = 0;
    }
    if (angvel > 0) {
        angvel -= 10;
        if (angvel < 0)
            angvel = 0;
    }
    if (svel < 0) {
        svel += 2;
        if (svel > 0)
            svel = 0;
    }
    if (svel > 0) {
        svel -= 2;
        if (svel < 0)
            svel = 0;
    }
    if (vel < 0) {
        vel += 2;
        if (vel > 0)
            vel = 0;
    }
    if (vel > 0) {
        vel -= 2;
        if (vel < 0)
            vel = 0;
    }
}

#undef held

/* Empty hook called around loadboard (the caller passes the map name in EAX). */
void level_load_hook(void)
{
}

#define held(binding, scan) \
    ((keystatus[keys[binding]] | keystatus[scan] | keystatus[(scan) + 128]) > 0)

/* 120 Hz timer interrupt. After Ken Silverman's timerhandler, Build GAME.C (1995). */
void __interrupt __far timerhandler(void)
{
    totalclock++;
    keytimerstuff();
    if (option[2] != 0) {
        totalclock += 5;
        keytimerstuff();
        keytimerstuff();
        keytimerstuff();
        keytimerstuff();
        keytimerstuff();
    }
    outp(0x20, 0x20);
}

#undef held

/* Shut everything down and exit to DOS with a message. Duke Nukem 3D 1.5 keeps this name. */
void gameexit(char *t)
{
    if (option[2] != 0)
        MUSIC_Shutdown();
    if (option[1] != 0)
        FX_Shutdown();
    if (numplayers > 1 && option[4] != 0) {
        sendlogoff();
        uninitmultiplayers();
    }
    uninittimer();
    setvmode(3);
    puts(exit_banner);
    puts(t);
    exit(0);
}

/* Read and apply all pending network packets. After Ken Silverman's getpackets, Build GAME.C (1994). */
void getpackets(void)
{
    short otherconnectindex, tempbufleng, p;
    long i, j, k, shift;

    while ((tempbufleng = getpacket(&otherconnectindex, (unsigned char *)tempbuf)) > 0) {
        switch (tempbuf[0]) {
            case 2:
                synctics = tempbuf[1];
                j = ((numplayers + 1) >> 1) + 2;
                k = (2 << 3);
                for (i = connecthead; i >= 0; i = connectpoint2[i]) {
                    if ((tempbuf[k >> 3] & (1 << (k & 7))) > 0)
                        syncvel[i] = tempbuf[j++];
                    if ((tempbuf[k >> 3] & (2 << (k & 7))) > 0)
                        syncsvel[i] = tempbuf[j++];
                    if (((4 << (k & 7)) & tempbuf[k >> 3]) > 0)
                        syncangvel[i] = tempbuf[j++];
                    /* COMPILER_STEERED: keep the final flag shift separate from the byte index. */
                    shift = k & 7;
                    if ((tempbuf[k >> 3] & (8 << shift)) > 0) {
                        syncbits[i] = tempbuf[j] +
                                         (tempbuf[j + 1] << 8) +
                                         (tempbuf[j + 2] << 16) +
                                         (tempbuf[j + 3] << 24);
                        j += 4;
                    }
                    k += 4;
                }
                domovethings();
                break;
            case 3:
                j = 2;
                if ((tempbuf[1] & 1) > 0)
                    syncvel[otherconnectindex] = tempbuf[j++];
                if ((tempbuf[1] & 2) > 0)
                    syncsvel[otherconnectindex] = tempbuf[j++];
                if ((tempbuf[1] & 4) > 0)
                    syncangvel[otherconnectindex] = tempbuf[j++];
                if ((tempbuf[1] & 8) > 0) {
                    syncbits[otherconnectindex] &= 0xffffff00;
                    syncbits[otherconnectindex] |= tempbuf[j++];
                }
                if ((tempbuf[1] & 16) > 0) {
                    syncbits[otherconnectindex] &= 0xffff00ff;
                    syncbits[otherconnectindex] |= tempbuf[j++] << 8;
                }
                if ((tempbuf[1] & 32) > 0) {
                    syncbits[otherconnectindex] &= 0xff00ffff;
                    syncbits[otherconnectindex] |= tempbuf[j++] << 16;
                }
                if ((tempbuf[1] & 64) > 0) {
                    syncbits[otherconnectindex] &= 0x00ffffff;
                    syncbits[otherconnectindex] |= tempbuf[j++] << 24;
                }
                break;
            case 4:
                typemessageleng = 0; /* 9.5b keeps only the xor, as in the oracle */
                for (j = 1; j < tempbufleng; j++)
                    typemessage[typemessageleng++] = tempbuf[j];
                user_quote = typemessage;
                user_quote_time = typemessageleng << 4;
                typemessage[0] = 0;
                break;
            case 5:
                playerreadyflag[otherconnectindex] = 1;
                break;
            case 255:
                ps[myconnectindex].gm = MODE_END;
                break;
        }
    }
}

/* Wait until every player is ready. After Ken Silverman's waitforeverybody, Build GAME.C (1995); Duke 1.5 PREMAP.C keeps the name. */
void waitforeverybody(void)
{
    long player; /* COMPILER_STEERED: keep the send index separate from the ready-count walk. */
    long i, j;
    long start;

    if (numplayers < 2)
        return;
    for (j = connecthead; j >= 0; j = connectpoint2[j])
        playerreadyflag[j] = 0;

    start = totalclock;
    if (myconnectindex == connecthead) {
        long ready, oldtotalclock;

        oldtotalclock = start - 120;
        playerreadyflag[connecthead] = 1;
        do {
            if (totalclock >= oldtotalclock + 120) {
                oldtotalclock = totalclock;
                tempbuf[0] = 5;
                for (player = connecthead; player >= 0; player = connectpoint2[player])
                    if (player != connecthead)
                        sendpacket(player, (unsigned char *)tempbuf, 1);
            }
            getpackets();
            ready = 0;
            for (i = connecthead; i >= 0; i = connectpoint2[i])
                ready += playerreadyflag[i];
        } while ((long)ready < (long)numplayers);
    } else {
        long oldtotalclock;

        oldtotalclock = start - 120;
        do {
            if (totalclock >= oldtotalclock + 120) {
                oldtotalclock = totalclock;
                tempbuf[0] = 5;
                sendpacket(connecthead, (unsigned char *)tempbuf, 1);
            }
            getpackets();
        } while (playerreadyflag[connecthead] == 0);
    }
}

/* Master/slave input synchronisation for one frame. After Ken Silverman's sync, Build GAME.C (1994). */
void sync(void)
{
    long i, j, k;
    short p;

    if (myconnectindex == connecthead) {
        getinput(myconnectindex);
        getpackets();
        if (synctics > 0 && getoutputcirclesize() < 16) {
            short packet_length; /* COMPILER_STEERED: save the master packet length in a word. */

            tempbuf[0] = 2;
            tempbuf[1] = synctics;
            j = ((numplayers + 1) >> 1) + 2;
            for (k = 2; k < j; k++)
                tempbuf[k] = 0;
            k = (2 << 3);
            for (i = connecthead; i >= 0; i = connectpoint2[i]) {
                if (syncvel[i] != osyncvel[i]) {
                    tempbuf[j++] = syncvel[i];
                    osyncvel[i] = syncvel[i];
                    tempbuf[k >> 3] |= (1 << (k & 7));
                }
                if (syncsvel[i] != osyncsvel[i]) {
                    tempbuf[j++] = syncsvel[i];
                    osyncsvel[i] = syncsvel[i];
                    tempbuf[k >> 3] |= (2 << (k & 7));
                }
                if (syncangvel[i] != osyncangvel[i]) {
                    tempbuf[j++] = syncangvel[i];
                    osyncangvel[i] = syncangvel[i];
                    tempbuf[k >> 3] |= (4 << (k & 7));
                }
                if (syncbits[i] != osyncbits[i]) {
                    tempbuf[j++] = (syncbits[i] & 255);
                    tempbuf[j++] = ((syncbits[i] >> 8) & 255);
                    tempbuf[j++] = ((syncbits[i] >> 16) & 255);
                    tempbuf[j++] = ((syncbits[i] >> 24) & 255);
                    osyncbits[i] = syncbits[i];
                    tempbuf[k >> 3] |= (8 << (k & 7));
                }
                k += 4;
            }
            packet_length = j;
            for (i = connecthead; i >= 0; i = connectpoint2[i])
                if (i != myconnectindex)
                    sendpacket(i, (unsigned char *)tempbuf, packet_length);
            domovethings();
        }
    } else {
        getinput(16);
        if (getoutputcirclesize() < 16) {
            tempbuf[0] = 3;
            tempbuf[1] = 0;
            j = 2;
            if (syncvel[16] != osyncvel[16]) {
                tempbuf[j++] = syncvel[16];
                osyncvel[16] = syncvel[16];
                tempbuf[1] |= 1;
            }
            if (syncsvel[16] != osyncsvel[16]) {
                tempbuf[j++] = syncsvel[16];
                osyncsvel[16] = syncsvel[16];
                tempbuf[1] |= 2;
            }
            if (syncangvel[16] != osyncangvel[16]) {
                tempbuf[j++] = syncangvel[16];
                osyncangvel[16] = syncangvel[16];
                tempbuf[1] |= 4;
            }
            if ((syncbits[16] & 0xff) != (osyncbits[16] & 0xff)) {
                tempbuf[j++] = syncbits[16];
                tempbuf[1] |= 8;
            }
            if ((syncbits[16] | 0xff) != (osyncbits[16] | 0xff)) {
                tempbuf[j++] = (syncbits[16] >> 8) & 255;
                tempbuf[1] |= 16;
            }
            if ((syncbits[16] & 0xff0000) != (osyncbits[16] & 0xff0000)) {
                tempbuf[j++] = (syncbits[16] >> 16) & 255;
                tempbuf[1] |= 32;
            }
            if ((syncbits[16] | 0xff0000) != (osyncbits[16] | 0xff0000)) {
                tempbuf[j++] = (syncbits[16] >> 24) & 255;
                tempbuf[1] |= 64;
            }
            osyncbits[16] = syncbits[16];
            sendpacket(connecthead, (unsigned char *)tempbuf, j);
        }
        getpackets();
    }
}

/* Pulsing text shade for menus and HUD captions; each call advances the pulse. */
long next_text_shade(void)
{
    short phase;

    phase = hudpulse += 128;
    return (sintable[phase & 2047] >> 10) + 16;
}

extern int getscorename();
extern short newscore(void);

/* Draw the chat message and the active help quote, decrementing their timers. Later Duke: operatefta(). */
void operatefta(short snum)
{
    long offset = 0;

    if (user_quote_time > 0) {
        user_quote_time--;
        minitext(8, 162, user_quote, 0);
    }
    if (ps[snum].fta[ps[snum].ftq] > 0) {
        ps[snum].fta[ps[snum].ftq]--;
        if (ps[snum].screen_size >= 0)
            offset -= 23;
        minitext(24, offset + 189, (unsigned char *)fta_quotes[ps[snum].ftq], 0);
    }
}

/* Show help quote q ('first time active' message). Duke Nukem 3D 1.5 keeps the name. */
void FTA(short q)
{
    if (ud.quotes == 1 && ps[screenpeek].fta[q] == 0) {
        ps[screenpeek].fta[q] = strlen(fta_quotes[q]) << 2;
        ps[screenpeek].ftq = q;
    }
}

/* Draw an overlay tile in the viewed player's sector palette. Later Duke: myos()/myospal(). */
void myos(short x, short y, short tilenum, unsigned char shade,
                      unsigned char orientation)
{
    unsigned char p;

    if (ps[screenpeek].dimensionmode == 3) {
        p = sector[ps[screenpeek].cursectnum].floorpal;
        if (p)
            overwritesprite(x, y, tilenum, shade, orientation, p);
        else
            overwritesprite(x, y, tilenum, shade, orientation, ps[screenpeek].palookup);
    }
}

/* Big-font text; returns the next x. Duke Nukem 3D 1.5 keeps the name. */
long gametext(long x, long y, unsigned char *t, unsigned char s)
{
    /* COMPILER_STEERED: the glyph and line origin persist across characters. */
    short ac;
    short left;

    left = x;
    if (ps[screenpeek].dimensionmode == 2)
        return 0;
    while (*t) {
        long advance;
        short descender;

        descender = 2;
        if (*t == '\n') {
            x = left;
            y += 16;
            t++;
            continue;
        }
        if (*t == ' ') {
            x += 5;
            t++;
            continue;
        }
        if (*t >= '0' && *t <= '9')
            ac = *t + 101;
        if (*t >= 'a' && *t <= 'z')
            ac = *t + 26;
        if (*t >= 'A' && *t <= 'Z') {
            ac = *t + 32;
            descender = 0;
        }
        overwritesprite(x, y + descender, ac, s, 0, 0);
        advance = tilesizx[ac];
        x += advance + 1;
        t++;
    }
    return x;
}

/* Small-font text; returns the next x. Duke Nukem 3D 1.5 keeps the name. */
long minitext(long x, long y, unsigned char *t, unsigned char s)
{
    if (ps[screenpeek].dimensionmode == 2)
        return 0;
    {
        /* COMPILER_STEERED: only the line origin lives across glyphs. */
        short left = x;

        while (*t) {
            long advance;
            short descender; /* COMPILER_STEERED: declare the vertical offset before the glyph. */
            short ac;

            descender = 0;
            if (*t == '\n') {
                x = left;
                y += 8;
                t++;
                continue;
            }
            if (*t == ' ') {
                x += 2;
                t++;
                continue;
            }
            ac = *t + 991;
            if (*t >= 'a' && *t <= 'z')
                descender = 2;
            overwritesprite(x, y + descender, ac, s, 0, 0);
            t++;
            advance = tilesizx[ac];
            x += advance + 1;
        }
    }
    return x;
}

/* Big-font number. Later Duke: gamenumber(). */
void gamenumber(long x, long sy, long n, unsigned char s)
{
    ltoa(n, tempbuf, 10);
    gametext(x, sy - 1, (unsigned char *)tempbuf, s);
}

/* Line editor for chat and high-score names; returns nonzero on Enter. Typing loop after Ken Silverman's getinput typemode, Build GAME.C (1994). */
unsigned char strget(short x, short y, unsigned char input_shade, unsigned char max_length)
{
    enum {
        KEY_BACKSPACE = 0x0e,
        KEY_LEFT_SHIFT = 0x2a,
        KEY_RIGHT_SHIFT = 0x36,
        KEY_ACCEPT = 0x1c,
        KEY_EXTENDED_ACCEPT = 0x9c,
        REPEAT_THRESHOLD = 60,
        LARGE_FONT_FLAG = 128
    };
    /* COMPILER_STEERED: keep separate byte locals for the editor's shade and length limit. */
    unsigned char shade = input_shade;
    unsigned char dalen = max_length;
    short ink; /* COMPILER_STEERED: save the masked small-font shade in a word. */
    short i;

    for (i = 0; i < 128; i++)
        if (keystatus[i] > 0)
        {
            if ((keystatus[0x2a] | keystatus[0x36]) != 0)
            {
                if (scantoascwithshift[i] != 0)
                {
                    if (typemessageleng < 79)
                        typemessage[typemessageleng++] = scantoascwithshift[i];
                    keystatus[i] = 0;
                }
            }
            else
            {
                if (scantoasc[i] != 0)
                {
                    if (typemessageleng < 159)
                        typemessage[typemessageleng++] = scantoasc[i];
                    keystatus[i] = 0;
                }
            }

            if (typemessageleng > dalen)
                typemessageleng = dalen;

            if (keystatus[0xe] > 0)   /* Backspace */
            {
                    /* Here's a trick of making key repeat after a 1/2 second */
                if ((keystatus[0xe] == 1) || (keystatus[0xe] == 60))
                {
                    if (typemessageleng > 0)
                        typemessageleng--;
                    else
                        typemessageleng = 0;
                }
                typemessage[typemessageleng] = 0;
                if (keystatus[0xe] < 60)
                {
                    keystatus[0xe] += synctics;
                    if (keystatus[0xe] > 60) keystatus[0xe] = 60;
                }
            }
        }

    if (shade < 128) {
        ink = shade & 127;
        x = minitext(x, y, typemessage, ink);
        myos(x, y, 1023, shade, 0);
    } else {
        x = gametext(x, y, typemessage, shade & 127);
        myos(x, y, 1022, shade, 0);
    }
    return (keystatus[KEY_ACCEPT] | keystatus[KEY_EXTENDED_ACCEPT]) != 0;
}

/* Find locator n (status 7) in sector sn (-1 = anywhere). Later Duke: LocateTheLocator(). */
short LocateTheLocator(short n, short sn)
{
    short i = headspritestat[STAT_LOCATOR];

    while (i >= 0) {
        if ((sn == -1 || sn == sprite[i].sectnum) && n == sprite[i].lotag)
            return i;
        i = nextspritestat[i];
    }
    return -1;
}

/* Spawn a sprite with explicit fields (Duke's 'EGS'). Duke Nukem 3D 1.5 keeps the name. */
short EGS(short sect, long x, long y, long z, unsigned char cstat, short picnum,
          signed char shade, unsigned char xrepeat, unsigned char yrepeat,
          short ang, short xvel, short zvel, short owner, signed char stat)
{
    short i;

    i = insertsprite(sect, stat);
    sprite[i].y = y;
    sprite[i].cstat = cstat;
    sprite[i].picnum = picnum;
    sprite[i].shade = shade;
    sprite[i].xrepeat = xrepeat;
    sprite[i].yrepeat = yrepeat;
    sprite[i].ang = ang;
    sprite[i].xvel = xvel;
    sprite[i].zvel = zvel;
    sprite[i].owner = owner;
    sprite[i].yvel = 0;
    hittype[i][3] = 0;
    hittype[i][2] = 0;
    hittype[i][0] = 0;
    sprite[i].z = z;
    sprite[i].x = x;
    if (actorscrptr[picnum]) {
        sprite[i].extra = *actorscrptr[picnum];
        hittype[i][4] = *(actorscrptr[picnum] + 1);
        hittype[i][1] = *(actorscrptr[picnum] + 2);
        if (picnum != 57)
            sprite[i].hitag = *(actorscrptr[picnum] + 3);
    } else {
        hittype[i][4] = 0;
        hittype[i][1] = 0;
    }
    return i;
}

/* Precache the tiles a level needs. Later Duke split it into cachespritenum() and cachegoodsprites(). */
void cacheit(void)
{
    short i, j, maxc;
    long tile;

    for (i = 0; i < 4096; i++) {
        if (sprite[i].statnum >= 1024)
            continue;
        maxc = 0;
        switch (sprite[i].picnum) {
            case 0x460:
                maxc = 1;
                for (j = 0x46a; j < 0x46c; j++)
                    if (waloff[j] == -1)
                        loadtile(j);
                break;
            case 0x3c:
            case 0x9f:
            case 0x109:
            case 0x3ac:
                maxc = 5;
                break;
            case 0x131:
            case 0x143:
            case 0x147:
            case 0x4b0:
                maxc = 4;
                break;
            case 0x39:
                for (j = 0x2e; j < 0x30; j++)
                    if (waloff[j] == -1)
                        loadtile(j);
                maxc = 1;
                break;
            case 0x461:
                for (j = 0x233; j < 0x238; j++)
                    if (waloff[j] == -1)
                        loadtile(j);
                for (j = 0x283; j < 0x288; j++)
                    if (waloff[j] == -1)
                        loadtile(j);
                maxc = 1;
                break;
            case 0x497:
                for (j = 0x496; j < 0x499; j++)
                    if (waloff[j] == -1)
                        loadtile(j);
                break;
            case 0x389:
                for (j = 0x3ce; j < 0x3d2; j++)
                    if (waloff[j] == -1)
                        loadtile(j);
            case 0x274:
            case 0x2e1:
                maxc = 3;
                break;
            case 0x26:
                if (waloff[0x26] == -1) {
                    for (j = 0x372; j < 0x377; j++)
                        if (waloff[j] == -1)
                            loadtile(j);
                    maxc = 1;
                }
                break;
            case 0x465:
                if (waloff[0x465] == -1) {
                    for (j = 0x47e; j < 0x481; j++)
                        if (waloff[j] == -1)
                            loadtile(j);
                    for (j = 0x488; j < 0x48d; j++)
                        if (waloff[j] == -1)
                            loadtile(j);
                    if (waloff[0x48d] == -1)
                        loadtile(0x48d);
                    maxc = 1;
                }
                break;
            case 0x273:
                maxc = 1;
                if (waloff[0x137] == -1)
                    loadtile(j);
                if (waloff[0x138] == -1)
                    loadtile(j);
                break;
            case 0x71c:
                if (waloff[0x71c] == -1)
                    maxc = 38;
                break;
            case 0x243:
                if (waloff[0x243] == -1) {
                    maxc = 10;
                    for (j = 0x39d; j < 0x3a2; j++)
                        if (waloff[j] == -1)
                            loadtile(j);
                }
                break;
            case 0x198:
                if (waloff[0x198] == -1)
                    maxc = 69;
                break;
            case 0x600:
                if (waloff[0x600] == -1)
                    maxc = 69;
                break;
            case 0x690:
                if (waloff[0x690] == -1)
                    maxc = 50;
                break;
            case 0x500:
                if (waloff[0x500] == -1)
                    maxc = 15;
                break;
            case 0x3c1:
                maxc = 7;
                break;
            case 0x26c:
                maxc = 6;
                break;
            case 0xf7:
                maxc = 10;
                break;
            default:
                maxc = 1;
                break;
        }
        for (j = 0; j < maxc; j++) {
            tile = i + j;
            if (waloff[tile] == -1)
                loadtile(tile);
        }
    }

    if (waloff[0x61] == -1)
        for (j = 0x61; j < 0x9e; j++)
            loadtile(j);
    if (waloff[0x166] == -1)
        for (j = 0x166; j < 0x16a; j++)
            loadtile(j);
    if (waloff[0xbf] == -1)
        for (j = 0xbf; j < 0xc3; j++)
            loadtile(j);
    if (waloff[0x2ae] == -1)
        for (j = 0x2ae; j < 0x2b2; j++)
            loadtile(j);
    if (waloff[0x4ba] == -1)
        for (j = 0x4ba; j < 0x4bf; j++)
            loadtile(j);
    if (waloff[0x400] == -1)
        for (j = 0x400; j < 0x460; j++)
            loadtile(j);
    if (waloff[0xeb] == -1)
        for (j = 0xeb; j < 0xf7; j++)
            loadtile(j);
    if (waloff[0x55a] == -1)
        for (j = 0x55a; j < 0x564; j++)
            loadtile(j);
    if (waloff[0x550] == -1)
        for (j = 0x550; j < 0x55a; j++)
            loadtile(j);
    if (waloff[0x185] == -1)
        for (j = 0x185; j < 0x187; j++)
            loadtile(j);
    if (waloff[0x366] == -1)
        for (j = 0x366; j < 0x372; j++)
            loadtile(j);
    if (waloff[0x4ce] == -1)
        for (j = 0x4ce; j < 0x4dd; j++)
            loadtile(j);
    if (waloff[0x23d] == -1)
        for (j = 0x23d; j < 0x243; j++)
            loadtile(j);
    if (waloff[0x564] == -1)
        for (j = 0x564; j < 0x57c; j++)
            loadtile(j);
    if (waloff[0x2a9] == -1)
        for (j = 0x2a9; j < 0x2ad; j++)
            loadtile(j);
    if (waloff[0x528] == -1)
        for (j = 0x528; j < 0x532; j++)
            loadtile(j);
    if (waloff[0x266] == -1)
        loadtile(0x266);
    if (waloff[0x473] == -1)
        loadtile(0x473);
    if (waloff[0x57c] == -1) {
        loadtile(0x57c);
        loadtile(0x57d);
    }
    if (waloff[0x298] == -1)
        loadtile(0x298);
    if (waloff[0x299] == -1)
        loadtile(0x299);
    if (waloff[0x29e] == -1)
        loadtile(0x29e);
    if (waloff[0x29f] == -1)
        loadtile(0x29f);
    if (waloff[0xae] == -1)
        loadtile(0xae);
    if (waloff[0xd6] == -1)
        loadtile(0xd6);
    if (waloff[0x470] == -1)
        loadtile(0x470);
    if (waloff[0x469] == -1)
        loadtile(0x469);
    if (waloff[0x11b] == -1) {
        for (j = 0x11b; j < 0x120; j++)
            if (waloff[j] == -1)
                loadtile(j);
        for (j = 0x58c; j < 0x5b4; j++)
            if (waloff[j] == -1)
                loadtile(j);
        for (j = 0x6f4; j < 0x712; j++)
            if (waloff[j] == -1)
                loadtile(j);
    }
}

/* Load sprite colour remaps from LOOKUP.DAT. Later Duke: genspriteremaps(). */
void genspriteremaps(void)
{
    unsigned char numl, look_pos;
    short j;
    int fp;

    resetparallax();
    fp = open(lookup_filename, O_RDWR | O_BINARY, S_IREAD);
    if (fp != -1)
        read(fp, &numl, 1);
    else
        gameexit(lookup_missing);

    for (j = 0; j < numl; j++) {
        read(fp, &look_pos, 1);
        read(fp, tempbuf, 256);
        makepalookup(look_pos, tempbuf, 0, 0, 0);
    }
    close(fp);
}

/* Small-font number (minitext counterpart of gamenumber). */
void numtext(long x, long sy, long n, unsigned char s)
{
    ltoa(n, tempbuf, 10);
    minitext(x, sy, (unsigned char *)tempbuf, s);
}

/* Status-bar digits, right-aligned. Role of Duke 1.5's digitalnumber(). */
void numtext_fixed(long x, long y, long n, unsigned char s)
{
    short i, offset;

    ltoa(n, tempbuf, 10);
    i = strlen(tempbuf);
    offset = 0;
    while (i != 0) {
        overwritesprite(x - offset, y, tempbuf[i - 1] + 0x536, s, 0, 0);
        offset += 6;
        i--;
    }
}

/* Tally marks in groups of five. Survives commented out as scratchmarks() in Duke 1.5 GAME.C. */
void scratchmarks(long x, long y, long n, unsigned char s)
{
    enum { FIRST_METER_TILE = 0x570, FULL_METER_TILE = FIRST_METER_TILE + 5 };
    long i, ni;

    ni = n / 5;
    for (i = 0; i < ni; i++) {
        overwritesprite(x - 2, y, FULL_METER_TILE, s, 0, 0);
        x += tilesizx[FULL_METER_TILE] - 1;
    }
    if (n % 5)
        overwritesprite(x, y, FIRST_METER_TILE + n % 5, s, 0, 0);
}

/* Choose rotation/animation frames for visible sprites. Duke Nukem 3D 1.5 keeps the name (Ken's analyzesprites). */
void animatesprites(short daang)
{
    short j, k, sect;
    long l;

    for (j = 0; j < spritesortcnt; j++) {
        short i; /* COMPILER_STEERED: the sprite index belongs to this iteration. */

        i = thesprite[j];
        if ((sprite[i].xrepeat | sprite[i].yrepeat) == 0)
            continue;
        sect = sprite[i].sectnum;
        if (sprite[i].picnum == 389)
            spritepal[j] = 0;

        if (actorscrptr[sprite[i].picnum] != 0 && hittype[i][4] != 0) {
            l = ((long *)hittype[i][4])[2];
            if (l < 0) {
                sprite[i].cstat |= 16;
                l = -l;
            } else {
                sprite[i].cstat &= ~16;
            }
            switch (l) {
                case 2:
                    k = (((sprite[i].ang + 3200 - daang) & 2047) >> 8) & 1;
                    break;
                case 3:
                    k = (((sprite[i].ang + 3200 - daang) & 2047) / 192) % 5;
                    if (k > 2) {
                        sprite[i].cstat |= 4;
                        k = 4 - k;
                    } else {
                        sprite[i].cstat &= ~4;
                    }
                    break;
                case 5:
                    k = (((sprite[i].ang + 3200 - daang) & 2047) >> 8) & 7;
                    if (k > 4) {
                        k = 8 - k;
                        sprite[i].cstat |= 4;
                    } else {
                        sprite[i].cstat &= ~4;
                    }
                    break;
                case 7:
                    k = ((sprite[i].ang + 3200 - daang) & 2047) / 170;
                    if (k > 6) {
                        k = 12 - k;
                        sprite[i].cstat |= 4;
                    } else {
                        sprite[i].cstat &= ~4;
                    }
                    break;
                case 8:
                    k = (((sprite[i].ang + 3200 - daang) & 2047) >> 8) & 7;
                    break;
                default:
                    k = 0;
                    break;
            }
            spritepicnum[j] += k + *(long *)hittype[i][4] + l * hittype[i][3];
            continue;
        }

        if (sprite[i].picnum == 1165) {
            spritepicnum[j] = 1165 + (hittype[i][0] & 1);
            hittype[i][0]++;
            hittype[i][0] &= 3;
            sprite[i].cstat |= 12;
            if (hittype[i][0] > 1)
                sprite[i].cstat &= ~4;
            if (hittype[i][0] > 2)
                sprite[i].cstat &= ~12;
        } else if (sprite[i].picnum == 1280) {
            spritepicnum[j] = 1280;
            k = (((sprite[i].ang + 3200 - daang) & 2047) >> 8) & 7;
            l = hittype[i][0];
            if (k > 4) {
                if (l == 2)
                    l--;
                else if (l == 1)
                    l++;
                k = 8 - k;
                sprite[i].cstat |= 4;
            } else {
                sprite[i].cstat &= ~4;
            }
            spritepicnum[j] += k + l * 5;
        } else if (sprite[i].picnum == 283) {
            if (sector[sect].floorpal != 2) {
                if (sprite[i].owner >= 0)
                    spritepal[j] = ps[sprite[i].yvel].palookup;
                else
                    spritepal[j] = 0;
            }
        } else if (sprite[i].picnum == 263 || sprite[i].picnum == 264) {
            if (hittype[i][0] != 0) {
                k = (((sprite[i].ang + 3200 - daang) & 2047) >> 8) & 7;
                if (k > 4) {
                    k = 8 - k;
                    sprite[i].cstat |= 4;
                } else {
                    sprite[i].cstat &= ~4;
                }
                if (sprite[i].zvel < 0)
                    k += (hittype[i][1] & 1) * 5;
                spritepicnum[j] = k + 247;
            }
        } else if (sprite[i].picnum >= 930 && sprite[i].picnum <= 931) {
            k = (((sprite[i].ang + 3200 - daang) & 2047) >> 8) & 7;
            if (k > 4) {
                k = 8 - k;
                sprite[i].cstat |= 4;
            } else {
                sprite[i].cstat &= ~4;
            }
            if (sprite[i].picnum == 931)
                k--;
            if (hittype[i][4] == 1)
                k += 5;
            spritepicnum[j] = k + sprite[i].picnum;
        }
    }
}

/* Reflective floor effect for tile 572: mirror the view into the floor's marker pixels. */
void draw_water_distortion(void)
{
    long x, row, run = 0;
    unsigned char c, *pixel, *src;

    for (x = 0; x < 320; x++) {
        pixel = (unsigned char *)frameplace + x + 99 * 320;
        for (row = 99; row < 200; row++, pixel += 320) {
            if (*pixel == 255) {
                if (run == 0) {
                    run = 1;
                    src = pixel - 320;
                }
                c = *src & 31;
                c = c + 96 - (run >> 1);
                if (run > 63 || c < 96 || c > 128)
                    c = 96;
                src -= 320;
                run++;
                *pixel = c;
            } else {
                run = 0;
            }
        }
    }
}

/* Draw the 2D map or the 3D view (player or camera). Duke Nukem 3D 1.5 keeps the name (Ken's drawscreen). */
void displayrooms(short snum)
{
    short s;

    if (ps[snum].dimensionmode == 2) {
        draw2dscreen(ps[snum].posx, ps[snum].posy, ps[snum].ang + ps[snum].look_ang, ps[snum].zoom, 0);
        return;
    }
    if ((syncbits[snum] & SB_ENLARGE_VIEW) || (syncbits[snum] & SB_SHRINK_VIEW) || (syncbits[snum] & SB_MAP))
        permanentwritesprite(0, 0, 0x492, 0, 0, 0, 319, 199);
    animatewalls();
    animatecamsprite(snum);
    if (ud.camerasprite >= 0) {
        s = ud.camerasprite;
        if (sprite[s].yvel < 0)
            sprite[s].yvel = 0;
        else if (sprite[s].yvel > 199)
            sprite[s].yvel = 199;
        drawrooms(sprite[s].x, sprite[s].y, sprite[s].z, sprite[s].ang,
                  sprite[s].yvel, sprite[s].sectnum);
        animatesprites(sprite[s].ang);
    } else {
        drawrooms(ps[snum].posx, ps[snum].posy, ps[snum].posz, ps[snum].ang + ps[snum].look_ang,
                  ps[snum].horiz, ps[snum].cursectnum);
        animatesprites(ps[snum].ang + ps[snum].look_ang);
    }
    drawmasks();
    if ((gotpic[572 >> 3] & (1 << (572 & 7))) > 0 && waloff[572] != -1) {
        draw_water_distortion();
    }
}

/* myos's body, written out where the overlay draws. */

/* minitext's small-font loop, written out for the score table. */

/* The view drawing of displayrooms, written out where a loop redraws the view. */
/* gametext's large-font loop, written out where screens draw fixed captions. */

/* Local (non-synchronised) function keys and the debug warp screen. Duke Nukem 3D 1.5 keeps the name. */
void nonsharedkeys(void)
{
    /* COMPILER_STEERED: read each ordinary and extended arrow before combining them. */
    int normal_key, extended_key;

    if (keystatus[0x58]) {
        keystatus[0x58] = 0;
        screencapture();
    }
    if (keystatus[0x44]) {
        keystatus[0x44] = 0;
        ud.brightness++;
        if (ud.brightness > 4)
            ud.brightness = 0;
        setbrightness(ud.brightness);
    }
    if (keystatus[0x14] > 0) {
        keystatus[0x14] = 0;
        ps[myconnectindex].gm |= MODE_TYPE;
    }
    if (keystatus[0x20]) {
        keystatus[0x20] = 0;
        ud.debug ^= 1;
    }
    if (option[2] && keystatus[0x32]) {
        keystatus[0x32] = 0;
        ud.MusicToggle ^= 1;
        if (ud.MusicToggle == 0)
            MUSIC_Pause();
        else
            MUSIC_Continue();
    }
    if (option[1] && keystatus[0x1f]) {
        keystatus[0x1f] = 0;
        ud.SoundToggle ^= 1;
        if (ud.SoundToggle == 0)
            FX_StopAllSounds();
    }
    if (keystatus[0x2f]) {
        keystatus[0x2f] = 0;
        ud.visibility--;
        if (ud.visibility < 7)
            ud.visibility = 16;
    }
    if (keystatus[0x25]) {
        screenpeek = connectpoint2[screenpeek];
        if (screenpeek == -1)
            screenpeek = connecthead;
        if (ps[screenpeek].dimensionmode == 3) {
            overwritesprite(0, 0, 1170, 0, 0, 0);
            nextpage();
            overwritesprite(0, 0, 1170, 0, 0, 0);
            nextpage();
            setgamemode();
        } else
            qsetmode640350();
        vscrn(screenpeek);
        keystatus[0x25] = 0;
    }
    if (keystatus[0x11] && ud.debug && (ps[myconnectindex].gm & MODE_TYPE) == 0) {
        keystatus[0x11] = 0;
        while ((keystatus[0x1c] | keystatus[0x9c] | keystatus[0x39] | keystatus[1]) == 0) {
            displayrooms(screenpeek);
            if (keystatus[0xc9]) {
                ps[0].volume++;
                keystatus[0xc9] = 0;
            }
            if (keystatus[0xd1]) {
                ps[0].volume--;
                keystatus[0xd1] = 0;
            }
            normal_key = keystatus[0x48];
            extended_key = keystatus[0xc8];
            if (normal_key | extended_key) {
                ps[0].level++;
                keystatus[0x48] = 0;
                keystatus[0xc8] = 0;
            }
            normal_key = keystatus[0x50];
            extended_key = keystatus[0xd0];
            if (normal_key | extended_key) {
                ps[0].level--;
                keystatus[0x50] = 0;
                keystatus[0xd0] = 0;
            }
            ps[0].level &= 15;
            ps[0].volume &= 3;
            gametext(112, 50, warp_title, 0);
            gametext(100, 69, warp_episode, 0);
            gametext(100, 88, warp_level, 0);
            gamenumber(210, 68, ps[0].volume + 1, 0);
            gamenumber(210, 87, ps[0].level + 1, 0);
            nextpage();
        }
        if (keystatus[1] == 0)
            enterlevel(MODE_GAME);
        keystatus[0x1c] = keystatus[0x9c] = keystatus[0x39] = keystatus[1] = 0;
    }
}

/* Command-line help. Later Duke: comlinehelp(). */
void comlinehelp(char **argv)
{
    puts(help_banner);
    printf(help_usage, argv[0]);
    puts(help_help);
    puts(help_level);
    puts(help_volume);
    puts(help_skill);
    puts(help_quotes);
    puts(help_debug);
    puts(help_record);
    puts(help_output);
    puts(help_encode);
}

/* Parse the command line. Duke Nukem 3D 1.5 keeps the name. */
void checkcommandline(int argc, char **argv)
{
    short i, j;
    char *c, *mapname;

    i = 1;
    ud.quotes = 0;
    warp_on = 0;
    if (argc > 1) {
    while (i < argc) {
        c = argv[i];
        if (*c == '/') {
            c++;
            switch (*c) {
            default:
                printf(unknown_parameter, argv[i]);
            case '?':
                comlinehelp(argv);
                exit(0);
            case 'q':
            case 'Q':
                ud.quotes = 1;
                break;
            case 'r':
            case 'R':
                recstat = 1;
                puts(demorec_on);
                break;
            case 'o':
            case 'O':
                c++;
                j = 0;
                while (*c) {
                    output_path[j] = *c;
                    j++;
                    c++;
                }
                output_path[j] = 0;
                printf(output_rerouted, output_path);
                break;
            case 's':
            case 'S':
                c++;
                ps[0].skill = atoi(c) - 1;
                break;
            case 'd':
            case 'D':
                ud.debug = 1;
                break;
            case 'l':
            case 'L':
                warp_on = 1;
                c++;
                ps[0].level = atoi(c) - 1;
                break;
            case 'v':
            case 'V':
                c++;
                ps[0].volume = atoi(c) - 1;
                break;
            case 'e':
            case 'E':
                mapname = encode_mapname;
                for (j = 0; j < 3; j++)
                    for (i = 0; i < 9; i++) {
                        mapname[0] = 'L' + j;
                        mapname[1] = '1' + i;
                        printf(encode_message, mapname);
                    }
                break;
            }
            i++;
        } else {
            printf(unknown_parameter2, c);
            comlinehelp(argv);
            exit(0);
        }
    }
    }
}

/* Wait for the other players to log on. From the waitplayers loop of Ken Silverman's main, Build GAME.C (1994). */
/* numtext_fixed's body, written out for the status bar. */
void wait_for_players(void)
{
    short waitplayers;

    if (option[4] >= 1 && option[4] <= 4)
        waitplayers = 2;
    else
        waitplayers = option[4] - 3;
    sendlogon();
    while (numplayers < waitplayers && keystatus[1] == 0) {
        getpackets();
    }
    keystatus[1] = 0;
}

/* Program entry: initialise, run demos and the game loop, save scores and exit. */
void main(int argc, char **argv)
{
    unsigned char which;
    short status;
    short i, k;
    short p, offset;
    unsigned char shade;
    short x, y;

    setvmode(3);
    puts(main_banner);
    puts(main_loading_stats);
    read_userdefs();
    puts(main_init_cache);
    init_bootleg_cache();
    puts(main_checking);
    if (check_file_id()) {
        puts(main_no_good);
        unlink(argv[0]);
        return;
    }
    checkcommandline(argc, argv);
    loadefs();
    puts(main_init_engine);
    initmenus();
    initengine();
    /* PIT and vector setup follows Ken's buil0723 GAME.C inittimer (1995, lines 4128-4133). */
    outp(0x43, 54);
    outp(0x40, 9942 & 255);
    outp(0x40, 9942 >> 8);
    oldtimerhandler = _dos_getvect(8);
    _disable();
    _dos_setvect(8, timerhandler);
    _enable();
    puts(main_multiplayer);
    initmultiplayers(option[4], option[5]);
    puts(main_loading_art);
    loadpics(main_tiles_file);
    puts(main_init_remaps);
    initlava();
    genspriteremaps();
    if (option[1]) {
        puts(main_init_fx);
        status = FX_SetupCard(sound_device_map[option[1]], &fx_device);
        if (status)
            gameexit(FX_ErrorString(status));
        puts(main_reading_sounds);
        loadsounds();
        status = FX_Init(sound_device_map[option[1]], 4, 2, 16, 11000);
        if (status)
            gameexit(FX_ErrorString(status));
        FX_SetCallBack(TestCallBack);
        FX_SetVolume(ud.sound_volume);
    }
    if (option[2]) {
        puts(main_init_music);
        status = MUSIC_Init(sound_device_map[option[1]], 0);
        if (status)
            gameexit(MUSIC_ErrorString(status));
        MUSIC_SetVolume(ud.music_volume);
    }
    if (option[4]) {
        puts(main_locating);
        wait_for_players();
    }
    setgamemode();
    if (warp_on) {
        newgame(ps[0].volume, ps[0].level, ps[0].skill);
        enterlevel(MODE_GAME);
    }

attract:
    if (warp_on == 0)
        playback();
    else
        warp_on = 0;

play:
    while ((ps[myconnectindex].gm & MODE_END) == 0) {
        nonsharedkeys();
        i = screenpeek;
        displayrooms(i);
        if (ps[myconnectindex].gm & MODE_TYPE) {
            short write_index; /* COMPILER_STEERED: scope the packet index to typing mode. */

            if (strget(10, 140, next_text_shade(), 79)) {
                short k;

                tempbuf[0] = 4;
                write_index = 1;
                for (p = 0; p < typemessageleng; p++)
                    tempbuf[write_index++] = typemessage[p];
                for (k = connecthead; k >= 0; k = connectpoint2[k])
                    if (k != myconnectindex)
                        sendpacket(k, (unsigned char *)tempbuf, p);
                ps[myconnectindex].gm ^= MODE_TYPE;
            }
        }
        if (ud.gamepaused && ps[screenpeek].dimensionmode == 3)
            draw_pause_sign();
        if ((ps[myconnectindex].gm & MODE_GAME) &&
            (((ps[myconnectindex].gm & MODE_MENU) == 0 && numplayers < 2) ||
             ((ps[myconnectindex].gm & MODE_MENU) && numplayers >= 2))) {
            if (totalclock < 360 && numplayers > 1 && ud.debug) {
                if (myconnectindex == connecthead) {
                    gametext(120, 48, main_master, 0);
                } else {
                    gametext(130, 48, main_slave, 0);
                }
            }
            if (ps[screenpeek].newowner >= 0)
                cameratext();
            else {
                short view = screenpeek; /* COMPILER_STEERED: use a local view index for the weapon. */

                displayweapon(view);
                i = screenpeek;
                displaystatus(i);
                i = screenpeek;
                operatefta(i);
            }
            if (numplayers < 2) {
                getinput(0);
                domovethings();
            } else
                sync();
            if (ps[myconnectindex].gm & MODE_EOL) {
                ps[myconnectindex].level++;
                enterlevel(MODE_GAME);
                continue;
            }
        }
        if (ps[myconnectindex].gm & MODE_MENU) {
            ps[myconnectindex].gm &= ~MODE_TYPE;
            menus();
            if (ps[myconnectindex].gm & MODE_DEMO)
                goto attract;
        }
        nextpage();
        pan3dsound();
    }
    if (ud.debug == 0 && (ps[myconnectindex].gm & MODE_DEMO) == 0) {
        p = newscore();
        if (p >= 0) {
            while (getscorename(p) == 0) {
                nextpage();
                i = screenpeek;
                displayrooms(i);
                displayhighscores();
            }
            memcpy(score_names[p], typemessage, 31);
        }
        do {
            i = screenpeek;
            displayrooms(i);
            displayhighscores();
            nextpage();
        } while (keystatus[1] == 0 && keystatus[0x39] == 0 && keystatus[0x1c] == 0);
    }
    closedemowrite();
    write_userdefs();
    gameexit(main_game_over);
}

/* Status bar and inventory panel for player snum (3D view only). */
void displaystatus(short snum)
{
    short i;

    if (ps[snum].dimensionmode != 3)
        return;
    if ((sector[ps[snum].cursectnum].lotag & 0x7fff) == 15)
        myos(160, 120, 1136, 0, 1);
    if (ps[snum].screen_size >= 0) {
        overwritesprite(0, 0, 1380, 0, 0, 0);
        overwritesprite(160, 189, 1381, 0, 1, 0);
        numtext_fixed(82, 6, ps[snum].score, 0);
        numtext_fixed(119, 186, ps[snum].health, 0);
        for (i = 0; i < 25; i++)
            if (i < ps[snum].health >> 2)
                overwritesprite(i * 3 + 30, 186, 1392, 0, 0, 0);
        switch (ps[snum].current_weapon) {
        case 389:
        case 1230:
            numtext_fixed(209, 186, ps[snum].ammo_bat_amount, 0);
            overwritesprite(160, 192, 1400, 0, 1, 0);
            for (i = 0; i < 25; i++)
                if (i < ps[snum].ammo_bat_amount >> 2)
                    overwritesprite(i * 3 + 216, 186, 1392, 0, 0, 0);
            break;
        case 191:
            numtext_fixed(209, 186, ps[snum].ammo_amount, 0);
            overwritesprite(160, 192, 1401, 0, 1, 0);
            for (i = 0; i < 25; i++)
                if (i < ps[snum].ammo_amount >> 2)
                    overwritesprite(i * 3 + 216, 186, 1392, 0, 0, 0);
            break;
        case 721:
        case 1140:
            numtext_fixed(209, 186, ps[snum].ammo_h_amount, 0);
            overwritesprite(160, 192, 1402, 0, 1, 0);
            for (i = 0; i < 25; i++)
                if (i < ps[snum].ammo_h_amount >> 2)
                    overwritesprite(i * 3 + 216, 186, 1392, 0, 0, 0);
            break;
        case 563:
            numtext_fixed(209, 186, ps[snum].ammo_r_amount, 0);
            overwritesprite(160, 192, 1403, 0, 1, 0);
            for (i = 0; i < 25; i++)
                if (i < ps[snum].ammo_r_amount >> 2)
                    overwritesprite(i * 3 + 216, 186, 1392, 0, 0, 0);
            break;
        }
    }
    if (ps[snum].show_inventory) {
        overwritesprite(0, 28, 1398, 0, 0, 0);
        if (ps[snum].steroids_on)
            overwritesprite(39, 41, 1399, 0, 0, 0);
        if (ps[snum].shield_amount)
            overwritesprite(39, 66, 1399, 0, 0, 0);
        if (ps[snum].scuba_on)
            overwritesprite(39, 93, 1399, 0, 0, 0);
        if (ps[snum].holoduke_on)
            overwritesprite(39, 119, 1399, 0, 0, 0);
        if (ps[snum].jetpack_on)
            overwritesprite(39, 145, 1399, 0, 0, 0);
        if (ps[snum].inventory[2])
            overwritesprite(13, 39, 717, 0, 0, 0);
        if (ps[snum].inventory[3])
            overwritesprite(14, 64, 719, 0, 0, 0);
        if (ps[snum].inventory[0])
            overwritesprite(13, 90, 716, 0, 0, 0);
        if (ps[snum].inventory[4])
            overwritesprite(13, 117, 718, 0, 0, 0);
        if (ps[snum].inventory[1])
            overwritesprite(13, 141, 713, 0, 0, 0);
    }
}

/* Demo playback / attract loop. After Ken Silverman's playback, Build GAME.C (1994); Duke 1.5 keeps the name. */
void playback(void)
{
    unsigned char which_demo = 1;
    long i;
    short snum; /* COMPILER_STEERED: capture the view index at the displayrooms call. */

restart:
    opendemoread(which_demo);
    which_demo++;
    if (which_demo == 8)
        which_demo = 0;
    enterlevel(MODE_DEMO);
    recstat = 2;
    reccnt--;
    i = 0;
    while (i < reccnt) {
        nonsharedkeys();
        while (totalclock > lockclock) {
            syncvel[0] = recsyncvel[i];
            syncsvel[0] = recsyncsvel[i];
            syncangvel[0] = recsyncangvel[i];
            syncbits[0] = recsyncbits[i];
            synctics = recsynctics[i];
            syncbits[0] &= ~0x180000;
            domovethings();
            i++;
        }
        while (totalclock <= lockclock) {
            snum = screenpeek;
            displayrooms(snum);
            if (ps[screenpeek].newowner >= 0)
                cameratext();
            else if (ud.camerasprite == -1) {
                displayweapon(screenpeek);
                displaystatus(screenpeek);
                operatefta(screenpeek);
            }
            if (keystatus[1]) {
                keystatus[1] = 0;
                current_menu = 0;
                ps[myconnectindex].gm |= MODE_MENU;
            }
            if (ps[myconnectindex].gm & MODE_MENU) {
                ps[myconnectindex].gm &= ~MODE_TYPE;
                menus();
            }
            if (ud.camerasprite >= 0) {
                long camera_x = 268; /* COMPILER_STEERED: keep this inlined draw coordinate wide. */

                overwritesprite(camera_x, 156, 481, 0, 4, 0);
            }
            pan3dsound();
            nextpage();
            if (ps[myconnectindex].gm == MODE_END)
                return;
            if (ps[myconnectindex].gm & MODE_GAME) {
                if (ps[myconnectindex].gm & MODE_DEMO)
                    goto restart;
                recstat = 0;
                return;
            }
        }
    }
    goto restart;
}

/* Record one tick of input into the demo buffers. Ken Silverman's recording code, Build GAME.C (1994). */
void record(short snum)
{
    recsyncvel[reccnt] = syncvel[snum];
    recsyncsvel[reccnt] = syncsvel[snum];
    recsyncangvel[reccnt] = syncangvel[snum];
    recsyncbits[reccnt] = syncbits[snum];
    recsynctics[reccnt] = synctics;
    if (++reccnt > 16383)
        reccnt = 16383;
}

/* Water and teleporter pairs (status 9); domovethings writes the same walk out. */


/* Advance the whole game by one tick. Duke Nukem 3D 1.5 keeps the name (Ken's movethings). */
void domovethings(void)
{
    short i;

    randomstash = krand();
    ud.camerasprite = -1;
    lockclock += synctics;
    doanimations(synctics);
    if (ud.gamepaused == 0) {
        if (recstat == 1)
            record(0);
        moveexplosions();
        movetransports();
    }
    for (i = connecthead; i >= 0; i = connectpoint2[i]) {
        cheatkeys(i);
        if (ud.gamepaused == 0) {
            processinput(i);
            checksectors(i);
        }
    }
    if (ud.gamepaused == 0) {
        moveplayers();
        moveweapons();
        movestandables();
        moveeffectors();
        movefta();
        moveactors();
        movefx();
    }
    synctics = 0;
}


/* Bonus/episode-map screen between levels. Duke Nukem 3D 1.5 keeps the name. */
void dobonus(void)
{
    /* COMPILER_STEERED: declaration order selects the stack homes for these screen values. */
    long pct;
    short shade;
    short start;
    short pulse;

    start = totalclock;
    menu_drag = 0;
    while (menu_drag == 0 &&
           (keystatus[0x1c] | keystatus[0x9c] | keystatus[0x39] | keystatus[1]) == 0 &&
           start + 2048 > totalclock) {
        pulse = next_text_shade();
        shade = (unsigned char)pulse;
        if (keystatus[0x39])
            start = totalclock - 2400;
        switch (ps[0].volume) {
        case 0:
            overwritesprite(0, 0, 1132, 0, 0, 0);
            break;
        case 1:
            overwritesprite(0, 0, 1135, 0, 0, 0);
            break;
        }
        gametext(36, 160, bonus_press_space, shade);
        {
            short i;

            for (i = 0; i < ps[0].level; i++) {
                short row = bonus_xy[ps[0].volume][i * 2 + 1];

                overwritesprite(bonus_xy[ps[0].volume][i * 2], row, 1133, shade, 0, 0);
            }
            {
                short row = bonus_xy[ps[0].volume][i * 2 + 1];
                short col = bonus_xy[ps[0].volume][i * 2];

                overwritesprite(col, row, 1134, shade, 0, 0);
            }
        }
        if (ps[myconnectindex].gm == MODE_EOL) {
            gametext(32, 10, bonus_title, 0);
            if (start + 240 < totalclock) {
                if (ps[0].max_secret_rooms)
                    pct = (ps[0].secret_rooms * 100) / ps[0].max_secret_rooms;
                minitext(60, 50, bonus_secrets, shade);
                numtext(60, 60, (short)pct, shade);
                minitext(72, 60, bonus_percent, shade);
                numtext(100, 60, (short)pct * 10000, shade);
                minitext(140, 60, bonus_points, 0);
                ps[screenpeek].score += (short)pct * 10000;
            }
            if (start + 480 < totalclock) {
                if (ps[0].max_squashable)
                    pct = (ps[0].squashable * 100) / ps[0].max_squashable;
                minitext(60, 80, bonus_squashed, shade);
                numtext(60, 90, (short)pct, shade);
                minitext(72, 90, bonus_percent, shade);
                numtext(100, 90, (short)pct * 1000, shade);
                minitext(140, 90, bonus_points, shade);
                ps[screenpeek].score += (short)pct * 1000;
            }
            if (start + 720 < totalclock) {
                if (ps[0].max_actors_killed)
                    pct = (ps[0].actors_killed * 100) / ps[0].max_actors_killed;
                minitext(60, 110, bonus_killed, shade);
                numtext(60, 120, (short)pct, shade);
                minitext(72, 120, bonus_percent, shade);
                numtext(100, 120, (short)pct * 1000, shade);
                minitext(140, 120, bonus_points, shade);
                ps[screenpeek].score += (short)pct * 1000;
            }
        }
        nextpage();
    }
}

/* Camera-view frame overlay. Later Duke: cameratext(). */
void cameratext(void)
{
    short shade = (unsigned char)next_text_shade();

    myos(10, 10, 275, shade, 2);
    myos(274, 10, 276, shade, 2);
    myos(10, 160, 277, shade, 2);
    myos(274, 160, 278, shade, 2);
    if (totalclock & 16)
        myos(18, 20, 279, shade, 2);
}

/* Update and draw the menu mouse pointer. */
void update_menu_cursor(short tile)
{
    short mousx, mousy;
    short shade = (unsigned char)next_text_shade();
    long scale;

    menu_previous_buttons = menu_drag;
    getmousevalues(&mousx, &mousy, &menu_drag);

    if (menu_drag == 0 && menu_previous_buttons != 0)
        menu_mouse = menu_previous_buttons;
    if (menu_drag == 0 && menu_previous_buttons == 0)
        menu_mouse = menu_drag;

    if (moustat != 0) {
        scale = ud.mouse_sensitivity + 129;
        menu_x += (mousx * scale) >> 9;
        {
            long delta = scale;

            delta *= mousy;

            menu_y = menu_y + (delta >> 9);
        }
        {
            short pos_x = menu_x;

            if (pos_x >= 0)
                goto x_upper;
            menu_x = 0;
            goto x_clamped;
        x_upper:
            if (pos_x > 319)
                menu_x = 319;
        }
    x_clamped:
        {
            short pos_y = menu_y;

            if (pos_y < 0)
                menu_y = 0;
            else if (pos_y > 199)
                menu_y = 199;
        }
    }

    {
        short x, y;

        y = menu_y;
        x = menu_x;
        myos(x, y, tile, shade, 1);
    }
}

/* High-score table. */
void displayhighscores(void)
{
    short shade;
    short value; /* COMPILER_STEERED: reuse the word holding the pulse for the table index. */

    value = next_text_shade();
    shade = (unsigned char)value;

    minitext(30, 36, score_rank_title, 0);
    minitext(50, 36, score_quote_title, 0);
    minitext(270, 36, score_score_title, 0);
    {
        short i;

        for (i = 0; i < 12; i++) {
            if (i)
                shade = 0;
            value = i;
            numtext(30, i * 11 + 50, value, 0);
            minitext(50, i * 11 + 50, score_names[i], shade);
            numtext(270, i * 11 + 50, high_scores[i], shade);
        }
    }
}



/* Throw glass shards from sprite i. Later Duke: lotsofglass() (which adds a wall parameter). */
void lotsofglass(short i, short n)
{
    short j;
    long z;

    for (j = 0; j < n; j++) {
        z = sprite[i].z - (krand() & 4095);
        if (z < sector[sprite[i].sectnum].ceilingz)
            z = sprite[i].z;
        {
            short velocity;

            /* COMPILER_STEERED: keep the velocity draw inside the EGS argument. */

            EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, z, 0, 970 + (j & 3), -32, 18, 18,
                sprite[i].ang - 256 + (krand() & 511), 64,
                (velocity = 1024 - (krand() & 2047), velocity), sprite[i].owner, 5);
        }
    }
}

/* Transporters and water-level crossings (status 9). Later Duke: movetransports(). */
void movetransports(void)
{
    long p, k;
    short i, j, m, sect;
    long otherlotag, sectlotag;

    {
        i = headspritestat[STAT_TRANSPORT];
        while (i >= 0) {
            sect = sprite[i].sectnum;
            if (sprite[i].z > sector[sect].floorz)
                sprite[i].z = sector[sect].floorz;
            if (sector[sect].lotag == 0 && sprite[i].z == sector[sect].floorz) {
                j = headspritesect[sect];
                while (j >= 0) {
                    if (hittype[i][1]) {
                        hittype[i][1]--;
                        break;
                    }
                    if (sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_TRANSPORT &&
                        sprite[j].picnum != 215 && sprite[j].picnum != 283) {
                        k = headspritestat[STAT_TRANSPORT];
                        while (k >= 0) {
                            if (i != k && sprite[i].hitag == sprite[k].hitag) {
                                sprite[j].x = sprite[k].x;
                                sprite[j].y = sprite[k].y;
                                sprite[j].z = sprite[k].z;
                                setsprite(j, sprite[j].x, sprite[j].y, sprite[j].z);
                                hittype[k][1] = 10;
                                hittype[i][1] = 10;
                            }
                            k = nextspritestat[k];
                        }
                    }
                    j = nextspritesect[j];
                }
            }

            p = connecthead;
            while (p >= 0) {
                if (ps[p].cursectnum == sect) {
                    sectlotag = sector[sect].lotag & 0x7fff;
                    if (sectlotag == 0) {
                        if (klabs(sprite[i].z - ps[p].posz) > 4096 &&
                            sprite[i].z != sector[sect].floorz)
                            goto nextplayer;
                    } else if (sectlotag == 2) {
                        if (sector[sect].ceilingz + 0x500 < ps[p].posz)
                            goto nextplayer;
                    } else if (sectlotag == 1) {
                        if (sector[sect].floorz - 0x500 > ps[p].posz)
                            goto nextplayer;
                    }
                    if (hittype[i][0] == 0) {
                    m = headspritestat[STAT_TRANSPORT];
                    while (m >= 0) {
                        if (m != i && sprite[i].hitag == sprite[m].hitag) {
                            otherlotag = sectlotag;
                            sectlotag = sector[sprite[m].sectnum].lotag;
                            if (!(otherlotag == 1 && sectlotag == 0) &&
                                !(otherlotag == 1 && sectlotag == 1) &&
                                !(otherlotag == 2 && sectlotag == 2)) {
                                if (sectlotag == 0) {
                                    ps[p].posx = sprite[m].x;
                                    ps[p].posy = sprite[m].y;
                                    if (sprite[m].z != sprite[i].z)
                                        ps[p].posz = sprite[m].z;
                                    else
                                        ps[p].posz = sector[sprite[m].sectnum].floorz - 0x2400;
                                    hittype[m][0] = 20;
                                    hittype[i][0] = 20;
                                }
                                if (sectlotag == 1) {
                                    if (ps[p].on_ground)
                                        spritesound(28, ps[p].i);
                                    ps[p].posx += sprite[m].x - sprite[i].x;
                                    ps[p].posy += sprite[m].y - sprite[i].y;
                                    ps[p].posz = sprite[m].z - 0x600;
                                    hittype[m][0] = 1;
                                    hittype[i][0] = 1;
                                }
                                if (sectlotag == 2) {
                                    spritesound(10, ps[p].i);
                                    ps[p].posx += sprite[m].x - sprite[i].x;
                                    ps[p].posy += sprite[m].y - sprite[i].y;
                                    ps[p].posz = sector[sprite[m].sectnum].ceilingz + 0x600;
                                    hittype[m][0] = 1;
                                    hittype[i][0] = 1;
                                }
                                ps[p].cursectnum = sprite[m].sectnum;
                                if (sector[ps[p].cursectnum].lotag & 0x7fff) {
                                    for (k = 0; k < 20; k++)
                                        spawn(ps[p].i, 215);
                                    spawn(ps[p].i, 1190);
                                }
                                break;
                            }
                        }
                        m = nextspritestat[m];
                    }
                    } else
                        hittype[i][0]--;
                }
        nextplayer:
                p = connectpoint2[p];
            }
            i = nextspritestat[i];
        }
    }
}

/* Read USERDEFS.TMP (settings + high scores). */
void read_userdefs(void)
{
    int fp;

    recstat = 0;
    fp = open(userdefs_filename, O_RDWR | O_BINARY, S_IREAD);
    if (fp == -1) {
        ud.mouse_sensitivity = 128;
        ud.visibility = 13;
        ud.sound_volume = 255;
        ud.music_volume = 255;
        ud.brightness = 1;
        ud.SoundToggle = 1;
        ud.MusicToggle = 1;
        return;
    }
    read(fp, &ud, sizeof(ud));
    read(fp, score_names, sizeof(score_names));
    read(fp, high_scores, sizeof(high_scores));
    close(fp);
}

/* Write USERDEFS.TMP (settings + high scores). */
void write_userdefs(void)
{
    int fp;

    /* Both text and binary flag bits are present in the original call. */
    fp = open(userdefs_filename, O_WRONLY | O_CREAT | O_TRUNC | O_TEXT | O_BINARY, 0xb0);
    write(fp, &ud, sizeof(ud));
    write(fp, score_names, sizeof(score_names));
    write(fp, high_scores, sizeof(high_scores));
    close(fp);
}

/* Save a game to slot spot. Duke Nukem 3D 1.5 keeps the name. */
void saveplayer(char spot)
{
    int fil;
    char *gamename = savegame_filename, *boardname = saveboard_filename;

    spot += '1';
    boardname[5] = '1' + ps[0].volume;
    boardname[6] = '1' + ps[0].level;
    boardname[7] = spot;
    gamename[7] = spot;
    fil = open(gamename, O_WRONLY | O_CREAT | O_TRUNC | O_TEXT | O_BINARY, 0xb0);
    write(fil, ps, sizeof(ps));
    write(fil, hittype, sizeof(hittype));
    write(fil, animwall, sizeof(animwall));
    write(fil, msx, sizeof(msx));
    write(fil, msy, sizeof(msy));
    close(fil);
    saveboard(boardname, &ps[0].posx, &ps[0].posy, &ps[0].posz,
              &ps[0].ang, &ps[0].cursectnum);
}

/* Load a game from slot spot. Duke Nukem 3D 1.5 keeps the name. */
void loadplayer(char spot)
{
    int fil;
    char *gamename = savegame_filename, *boardname = saveboard_filename;

    spot += '1';
    boardname[5] = '1' + ps[0].volume;
    boardname[6] = '1' + ps[0].level;
    boardname[7] = spot;
    gamename[7] = spot;
    fil = open(gamename, O_RDWR | O_BINARY, S_IREAD);
    if (fil != -1) {
        read(fil, ps, sizeof(ps));
        read(fil, hittype, sizeof(hittype));
        read(fil, animwall, sizeof(animwall));
        read(fil, msx, sizeof(msx));
        read(fil, msy, sizeof(msy));
        close(fil);
    }
    if (loadboard(boardname, &ps[0].posx, &ps[0].posy, &ps[0].posz,
                  &ps[0].ang, &ps[0].cursectnum) == -1)
        gameexit(saveboard_missing);
    ps[myconnectindex].gm = MODE_GAME;
}

/* Open and load demo which_demo. Later Duke: opendemoread(). */
void opendemoread(unsigned char which_demo)
{
    int fp;

    demo_filename[7] = '0' + which_demo;
    fp = open(demo_filename, O_RDWR | O_BINARY, S_IREAD);
    if (fp != -1) {
        read(fp, &ps[0].volume, sizeof(ps[0].volume));
        read(fp, &ps[0].level, sizeof(ps[0].level));
        read(fp, &ps[0].skill, sizeof(ps[0].skill));
        newgame(ps[0].volume, ps[0].level, ps[0].skill);
        read(fp, &reccnt, sizeof(reccnt));
        read(fp, recsyncvel, reccnt);
        read(fp, recsyncsvel, reccnt);
        read(fp, recsyncangvel, reccnt);
        read(fp, recsynctics, reccnt);
        read(fp, recsyncbits, reccnt * sizeof(recsyncbits[0]));
        close(fp);
    } else {
        gameexit(demo_missing);
    }
}

/* Write the recorded demo at exit. Later Duke split this into opendemowrite()/closedemowrite(). */
void closedemowrite(void)
{
    int fp;
    char *filename = demo_filename;

    if (recstat != 1)
        return;

    reccnt--;
    /* COMPILER_STEERED: do/while(0) models a basic-block boundary after reccnt-- (the oracle keeps
     * `sub [reccnt],edx` as a memory RMW; in one block the 9.5 scheduler splits it). */
    do {
        if (ps[0].level == 0 && ps[0].volume == 0)
            filename[7] = '0';
        else
            filename[7] = '1';
    } while (0);

    fp = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_TEXT | O_BINARY, 0xb0);
    if (fp != -1) {
        write(fp, &ps[0].volume, sizeof(ps[0].volume));
        write(fp, &ps[0].level, sizeof(ps[0].level));
        write(fp, &ps[0].skill, sizeof(ps[0].skill));
        write(fp, &reccnt, sizeof(reccnt));
        write(fp, recsyncvel, reccnt);
        write(fp, recsyncsvel, reccnt);
        write(fp, recsyncangvel, reccnt);
        write(fp, recsynctics, reccnt);
        write(fp, recsyncbits, reccnt * sizeof(recsyncbits[0]));
        close(fp);
    }
}

/* Sector effector logic (status 3). Duke Nukem 3D 1.5 keeps the name. */
/* The dispatch follows LameDuke's status-3 actor and sector conventions, written
 * in the Duke 1.3d moveeffectors style (shared q/l/m/x scratch, st, long j).
 * FTA is expanded in place where the compiler inlined it. sector[].wallptr is unsigned (BUILD.H); the
 * (short) casts on the wall-loop starts keep this NEAR reconstruction's signed loads (COMPILER_STEERED). */
void moveeffectors(void)
{
    long q;
    long l;
    long m;
    short nexti;
    short i;
    short startwall;
    short endwall;
    short k;
    long *height;
    short p;
    long *t;
    long x;
    long j;
    long st;

    i = headspritestat[STAT_EFFECTOR];
    while (i >= 0) {
        short sect;

        nexti = nextspritestat[i];
        sect = sprite[i].sectnum;
        t = &hittype[i][0];
        st = sprite[i].lotag;

        switch (st) {
        case 0:
            for (j = headspritestat[STAT_EFFECTOR]; j >= 0; j = nextspritestat[j])
                if (sprite[i].hitag == sprite[j].hitag && (sprite[j].lotag == 1 || sprite[j].lotag == 5)) {
                    q = sector[sect].extra >> 3;
                    if (sprite[j].ang > 1024)
                        l = -1;
                    else
                        l = 1;
                    break;
                }
            if (j >= 0) {
                if (t[3] == 0)
                    t[3] = klabs(sprite[i].x - sprite[j].x) + klabs(sprite[i].y - sprite[j].y);
                sprite[i].xvel = t[3];
                sprite[i].x = sprite[j].x;
                sprite[i].y = sprite[j].y;
                sprite[i].ang += l * q;
                t[2] += l * q;
            }
            ms(i);
            break;
        case 2:
            if (t[4] > 0 && t[0] == 0) {
                if (sprite[i].hitag > t[4])
                    t[4]++;
                else {
                    t[0] = 1;
                    sprite[i].xvel = 16;
                }
            }
            if (t[0] > 0) {
                t[0]++;
                if (t[0] > 24) {
                    t[0] = -1;
                    t[4] = -1;
                    break;
                }
                sprite[i].xvel++;
                for (p = connecthead; p >= 0; p = connectpoint2[p])
                    if (ps[p].cursectnum == sect && ps[p].on_ground) {
                        ps[p].posx += (sintable[(sprite[i].ang + 512) & 2047] * sprite[i].xvel) >> 14;
                        ps[p].posy += (sintable[sprite[i].ang & 2047] * sprite[i].xvel) >> 14;
                    }
                for (j = headspritesect[sect]; j >= 0; j = nextspritesect[j])
                    if (sprite[j].picnum != 175) {
                        /* COMPILER_STEERED: both products read the speed first. */
                        sprite[j].x += (sprite[i].xvel * sintable[(sprite[i].ang + 512) & 2047]) >> 14;
                        sprite[j].y += (sprite[i].xvel * sintable[sprite[i].ang & 2047]) >> 14;
                        setsprite(j, sprite[j].x, sprite[j].y, sprite[j].z);
                    }
                ms(i);
            }
            break;
        case 3: {
            short palette;
            short endwall;

            if (t[4] == 0)
                break;
            if ((randomstash / (sprite[i].hitag + 1)) & 1) {
                palette = sprite[i].owner;
                sector[sect].ceilingpal = palette >> 8;
                sector[sect].floorpal = palette;
                t[0] = sprite[i].shade;
            } else {
                sector[sect].ceilingpal = 0;
                sector[sect].floorpal = 0;
                t[0] = t[3];
            }
            sector[sect].floorshade = sector[sect].ceilingshade = t[0];
            endwall = sector[sect].wallptr + sector[sect].wallnum;
            for (x = (short)sector[sect].wallptr; x < endwall; x++)
                if (!(wall[x].hitag & 1))
                    wall[x].shade = t[0];
            break;
        }
        case 4: {
            short endwall;

            if ((randomstash / (sprite[i].hitag + 1)) & 1) {
                t[1] = sprite[i].shade;
                t[0] = sprite[i].shade;
                sector[sect].ceilingpal = sprite[i].owner >> 8;
                sector[sect].floorpal = sprite[i].owner;
            } else {
                t[1] = t[2];
                t[0] = t[3];
                sector[sect].ceilingpal = 0;
                sector[sect].floorpal = 0;
            }
            sector[sect].ceilingshade = sector[sect].floorshade = t[1];
            endwall = (short)sector[sect].wallptr;
            x = endwall;
            endwall += sector[sect].wallnum;
            for (; x < endwall; x++)
                if (!(wall[x].hitag & 1))
                    wall[x].shade = t[0];
            break;
        }
        case 5:
            p = findplayer(i, &x);
            if (sprite[i].owner == -1) {
                t[4] = 0;
                l = 0x7fffffff;
                while (1) {
                    sprite[i].owner = LocateTheLocator(t[4], -1);
                    if (sprite[i].owner == -1)
                        break;
                    if (klabs(sprite[ps[p].i].x - sprite[sprite[i].owner].x) + klabs(sprite[ps[p].i].y - sprite[sprite[i].owner].y) < l) {
                        q = sprite[i].owner;
                        l = klabs(sprite[ps[p].i].x - sprite[q].x) + klabs(sprite[ps[p].i].y - sprite[q].y);
                    }
                    t[4]++;
                }
                sprite[i].owner = q;
                sprite[i].zvel = ksgn(sprite[q].z - sprite[i].z) << 4;
            }
            if (klabs(sprite[sprite[i].owner].x - sprite[i].x) + klabs(sprite[sprite[i].owner].y - sprite[i].y) < 1024) {
                short ta;

                ta = sprite[i].ang;
                sprite[i].ang = getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y);
                sprite[i].ang = ta;
                sprite[i].owner = -1;
                break;
            }
            sprite[i].xvel = 256;
            x = getangle(sprite[sprite[i].owner].x - sprite[i].x, sprite[sprite[i].owner].y - sprite[i].y);
            q = getincangle(sprite[i].ang, x, 3);
            sprite[i].ang += q;
            if ((krand() >> 8) >= 223) {
                t[2] += q;
                sector[sect].ceilingshade = 127;
            } else {
                t[2] += getincangle(t[2] + 512, getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y), 2);
                sector[sect].ceilingshade = 0;
            }
            if (ifhitbyweapon(i) >= 0) {
                t[3]++;
                ps[p].score += 10000;
                if (t[3] == 5) {
                    sprite[i].zvel += 1024;
                    ps[p].score += 1000000;
                    FTA(7);
                }
            }
            sprite[i].z += sprite[i].zvel;
            sector[sect].ceilingz += sprite[i].zvel;
            sector[t[0]].ceilingz += sprite[i].zvel;
            ms(i);
            break;
        case 6:
            if (t[4] > 0) {
                t[4]--;
                if (t[4] >= 224)
                    sprite[i].xvel -= 8;
                if (t[4] > 127 && t[4] < 224)
                    sprite[i].xvel = 0;
                if (t[4] < 128)
                    sprite[i].xvel += 8;
                if (t[4] < 96) {
                    t[4] = 0;
                    sprite[i].xvel = 256;
                }
            } else
                sprite[i].xvel = 256;
            x = 0;
            for (j = headspritestat[STAT_EFFECTOR]; j >= 0; j = nextspritestat[j])
                if (sprite[j].lotag == 14 && sprite[j].hitag == sprite[i].hitag && hittype[j][0] == t[0]) {
                    sprite[j].xvel = sprite[i].xvel;
                    x++;
                    hittype[j][4] = t[4];
                }
        case 14: {
            long d;

            if (sprite[i].owner == -1)
                sprite[i].owner = LocateTheLocator(t[3], t[0]);
            d = klabs(sprite[sprite[i].owner].x - sprite[i].x) + klabs(sprite[sprite[i].owner].y - sprite[i].y);
            if (d < 1024) {
                if (st == 6 && (sprite[sprite[i].owner].hitag & 1))
                    t[4] = 256;
                t[3]++;
                sprite[i].owner = LocateTheLocator(t[3], t[0]);
                if (sprite[i].owner == -1) {
                    t[3] = 0;
                    sprite[i].owner = LocateTheLocator(0, t[0]);
                }
            }
            if (sprite[i].xvel) {
                x = getangle(sprite[sprite[i].owner].x - sprite[i].x, sprite[sprite[i].owner].y - sprite[i].y);
                q = getincangle(sprite[i].ang, x, 3);
                t[2] += q;
                sprite[i].ang += q;
                if (sprite[i].xvel == 256 && sound_voices[32].num == 0) {
                    d = 1;
                    if (!(sector[sect].floorstat & 1) && !(sector[sect].ceilingstat & 1))
                        spritesound(32, i);
                }
                if (sprite[i].xvel <= 64 && !(sector[sect].floorstat & 1) && !(sector[sect].ceilingstat & 1))
                    stopsound(32);
                for (p = connecthead; p >= 0; p = connectpoint2[p])
                    if (sector[ps[p].cursectnum].hitag == sector[sect].hitag && sector[ps[p].cursectnum].hitag != -1) {
                        rotatepoint(sprite[i].x, sprite[i].y, ps[p].posx, ps[p].posy, q, &ps[p].posx, &ps[p].posy);
                        if (d)
                            ps[p].posz -= 1024;
                        ps[p].posx += (sintable[(sprite[i].ang + 512) & 2047] * sprite[i].xvel) >> 14;
                        ps[p].posy += (sintable[sprite[i].ang & 2047] * sprite[i].xvel) >> 14;
                        ps[p].ang += q;
                        setsprite(ps[p].i, ps[p].posx, ps[p].posy, ps[p].posz + 9216);
                    }
                for (j = headspritesect[sect]; j >= 0; j = nextspritesect[j])
                    if (sprite[j].picnum != 175 && sprite[j].picnum != 34 && sprite[j].picnum != 283
                        && sector[sect].hitag != 65535) {
                        rotatepoint(sprite[i].x, sprite[i].y, sprite[j].x, sprite[j].y, q, &sprite[j].x, &sprite[j].y);
                        sprite[j].x += (sintable[(sprite[i].ang + 512) & 2047] * sprite[i].xvel) >> 14;
                        sprite[j].y += (sintable[sprite[i].ang & 2047] * sprite[i].xvel) >> 14;
                        sprite[j].ang += q;
                        setsprite(j, sprite[j].x, sprite[j].y, sprite[j].z);
                    }
                ms(i);
            }
            break;
        }
        case 8:
        case 9:
            if (getanimationgoal(&sector[sect].ceilingz) < 0)
                break;
            if (sector[sect].lotag & 0x8000)
                x = -t[3];
            else
                x = t[3];
            if (st == 9)
                x = -x;
            for (j = headspritestat[STAT_EFFECTOR]; j >= 0; j = nextspritestat[j]) {
                k = sprite[j].sectnum;
                m = sprite[j].shade;
                if (sprite[j].lotag == st && sprite[j].hitag == sprite[i].hitag) {
                    startwall = sector[k].wallptr;
                    endwall = startwall + sector[k].wallnum;

                    for (l = startwall; l < endwall; l++)
                        if (!(wall[l].hitag & 1)) {
                            wall[l].shade += x;
                            if (wall[l].shade < m)
                                wall[l].shade = m;
                            else if (wall[l].shade > hittype[j][2])
                                wall[l].shade = hittype[j][2];
                        }
                    sector[k].floorshade += x;
                    sector[k].ceilingshade += x;
                    if (sector[k].floorshade < m)
                        sector[k].floorshade = m;
                    else if (sector[k].floorshade > hittype[j][0])
                        sector[k].floorshade = hittype[j][0];
                    if (sector[k].ceilingshade < m)
                        sector[k].ceilingshade = m;
                    else if (sector[k].ceilingshade > hittype[j][1])
                        sector[k].ceilingshade = hittype[j][0];
                }
            }
            break;
        case 10:
            if ((sector[sect].lotag & 0x7fff) == 23) {
                if (sector[sect].lotag & 0x8000) {
                    t[0]++;
                    if (t[0] > sprite[i].hitag) {
                        operatesectors(sect, i);
                        t[0] = 0;
                    }
                }
            } else if (sector[sect].floorz > sector[sect].ceilingz) {
                if (t[1] == 0) {
                    t[0]++;
                    if (t[0] > sprite[i].hitag) {
                        operatesectors(sect, i);
                        t[1] = 1;
                    }
                }
            } else {
                t[0] = 0;
                t[1] = 0;
            }
            break;
        case 11:
            if (t[4]) {
                long j;
                short step;
                short last;

                step = (sprite[i].yvel >> 3) * t[3];
                t[2] += step;
                t[4] += step;
                ms(i);
                j = (short)sector[sect].wallptr;
                last = (short)(j + sector[sect].wallnum);
                for (; j < last; j++)
                    for (p = connecthead; p >= 0; p = connectpoint2[p])
                        if (clipinsidebox(ps[p].posx, ps[p].posy, j, 128) == 1) {
                            t[4] -= step;
                            t[2] -= step;
                            ms(i);
                        }
                if (t[4] <= -511 || t[4] >= 512) {
                    t[4] = 0;
                    t[2] &= 0xffffff00;
                    ms(i);
                }
            }
            break;
        case 12: {
            short k;

            if (t[0] == 1) {
                short firstwall = sector[sect].wallptr;

                k = firstwall + sector[sect].wallnum;
                for (j = firstwall; j < k; j++)
                    wall[j].shade = t[1];
                sector[sect].floorshade = t[1];
                sector[sect].ceilingshade = t[2];
                t[0] = 0;
            } else if (t[0] == 2) {
                if (sector[sect].floorshade > sprite[i].shade) {
                    short startwall, endwall;

                    sector[sect].floorshade -= 2;
                    sector[sect].ceilingshade -= 2;
                    startwall = sector[sect].wallptr;
                    endwall = startwall + sector[sect].wallnum;
                    for (j = startwall; j < endwall; j++)
                        wall[j].shade -= 2;
                } else
                    t[0] = 0;
            }
            break;
        }
        case 13: {
            long sx, sy;

            if (t[2] == 0) {
                for (j = headspritestat[STAT_STANDABLE]; j >= 0; j = nextspritestat[j])
                    if (sprite[j].picnum == 57 && sprite[j].shade == -32 && sprite[j].hitag == sprite[i].hitag) {
                        for (j = headspritestat[STAT_EFFECTOR]; j >= 0; j = nextspritestat[j])
                            if (sprite[j].lotag == 13 && sprite[j].hitag == sprite[i].hitag)
                                hittype[j][2] = 1;
                        break;
                    }
            } else {
                if (sprite[i].ang == 512) {
                    if (sprite[i].owner)
                        sector[sect].ceilingz += (t[0] - sector[sect].ceilingz) >> 1;
                    else
                        sector[sect].floorz += (t[1] - sector[sect].floorz) >> 1;
                } else {
                    sector[sect].ceilingz += (t[0] - sector[sect].ceilingz) >> 1;
                    sector[sect].floorz += (t[1] - sector[sect].floorz) >> 1;
                }
                if (t[3] == 1) {
                    t[3]++;
                    sector[sect].ceilingstat ^= 1;
                    if (sprite[i].ang == 512) {
                        k = sector[sect].wallptr + sector[sect].wallnum;
                        for (j = (short)sector[sect].wallptr; j < k; j++)
                            wall[j].shade = sprite[i].shade;
                        sector[sect].floorshade = sprite[i].shade;
                        if (ps[0].one_parallax_sectnum >= 0) {
                            sector[sect].ceilingpicnum = sector[ps[0].one_parallax_sectnum].ceilingpicnum;
                            sector[sect].ceilingshade = sector[ps[0].one_parallax_sectnum].ceilingshade;
                        }
                    }
                }
                t[2]++;
                if (t[2] > 7) {
                    if (t[3] != 2)
                        sector[sect].ceilingshade = 127;
                    if (sprite[i].ang != 512)
                        sector[sect].floorshade = 127;
                    deletesprite(i);
                    goto BOLT;
                }
            }
            if (t[2] == 4 && sprite[i].ang != 512) {
                x = sector[sect].floorz - 12288 - ((krand() & 32) << 6);
                sx = sprite[i].x;
                sy = sprite[i].y;
                nuke(sx, sy, x, i);
                for (x = 0; x < 30; x += 3) {
                    long sz, drop;
                    short pic;
                    short a;
                    short xv;
                    long *actor;
                    short zv;

                    sx = sprite[i].x;
                    sy = sprite[i].y;
                    drop = krand() & 0x1000;
                    sz = sector[sect].floorz - drop;
                    pic = krand() % 11;
                    a = krand() & 2047;
                    xv = (krand() & 0x80) + 64;
                    zv = -(krand() & 511) - 512;
                    pic += 235;
                    k = insertsprite(sect, 5);
                    sprite[k].x = sx;
                    sprite[k].y = sy;
                    sprite[k].z = sz;
                    sprite[k].picnum = pic;
                    sprite[k].xrepeat = 24;
                    sprite[k].yrepeat = 24;
                    sprite[k].ang = a;
                    sprite[k].xvel = xv;
                    sprite[k].zvel = zv;
                    sprite[k].owner = i;
                    sprite[k].yvel = 0;
                    sprite[k].cstat = 0;
                    hittype[k][3] = 0;
                    sprite[k].shade = -8;
                    hittype[k][2] = 0;
                    actor = actorscrptr[pic];
                    hittype[k][0] = 0;
                    if (actor != NULL) {
                        sprite[k].extra = actor[0];
                        hittype[k][4] = actor[1];
                        hittype[k][1] = actor[2];
                        if (pic != 57)
                            sprite[k].hitag = actor[3];
                    } else {
                        hittype[k][4] = 0;
                        hittype[k][1] = 0;
                    }
                }
            }
            break;
        }
        case 15:
            if (t[4]) {
                sprite[i].xvel = 16;
                if (t[4] == 1) {
                    if ((sprite[i].yvel >> 3) < t[3]) {
                        t[4] = 0;
                        break;
                    }
                    t[3]++;
                }
                if (t[4] == 2) {
                    t[3]--;
                    if (t[3] < 1)
                        t[4] = 0;
                }
                ms(i);
            }
            break;
        case 16:
            t[2] += 32;
            if (sector[sect].floorz < sector[sect].ceilingz)
                sprite[i].shade = 0;
            else if (sector[sect].ceilingz < t[3]) {
                if (nextspritesect[headspritesect[sect]] == -1) {
                    deletesprite(i);
                    goto BOLT;
                }
                sprite[i].shade = 1;
            }
            if (sprite[i].shade)
                sector[sect].ceilingz += 1024;
            else
                sector[sect].ceilingz -= 512;
            ms(i);
            break;
        case 17:
            q = t[0] * (sprite[i].yvel << 2);
            sector[sect].ceilingz += q;
            sector[sect].floorz += q;
            for (j = headspritesect[sect]; j >= 0; j = nextspritesect[j])
                if (sprite[j].statnum < 3)
                    sprite[j].z += q;
            if (t[0] == 0)
                break;
            if (t[0]) {
                if (klabs(sector[sect].floorz - t[2]) <= sprite[i].yvel) {
                    activatewarpelevators(i, 0);
                    break;
                }
                if (t[0] == -1) {
                    sprite[i].ang += 128;
                    if (sector[sect].floorz > t[3])
                        break;
                } else {
                    sprite[i].ang -= 128;
                    if (sector[sect].ceilingz < t[4])
                        break;
                }
                if (t[1] == 0)
                    break;
                t[1] = 0;
                for (p = connecthead; p >= 0; p = connectpoint2[p])
                    if (ps[p].cursectnum == sect)
                        for (j = headspritestat[STAT_EFFECTOR]; j >= 0; j = nextspritestat[j])
                            if (sprite[j].lotag == 17 && sector[sect].hitag - t[0] == sector[sprite[j].sectnum].hitag
                                && sprite[j].hitag == sprite[i].hitag) {
                                ps[p].posx += sprite[j].x - sprite[i].x;
                                ps[p].cursectnum = sprite[j].sectnum;
                                ps[p].posy += sprite[j].y - sprite[i].y;
                                setsprite(ps[p].i, ps[p].posx, ps[p].posy, ps[p].posz + 9216);
                            }
            }
            break;
        case 18:
            if (t[0]) {
                unsigned char statlist[3] = { 1, 4, 5 };

                m = sprite[i].yvel << 4;
                for (x = 0; x < 3; x++)
                    for (j = headspritestat[statlist[x]]; j >= 0; j = nextspritestat[j]) {
                        l = klabs(sprite[i].x - sprite[j].x) + klabs(sprite[i].y - sprite[j].y);
                        if (l < m) {
                            sprite[j].x += ksgn(sprite[i].x - sprite[j].x) << 7;
                            sprite[j].y += ksgn(sprite[i].y - sprite[j].y) << 7;
                            sprite[j].z += ksgn(sprite[i].z - sprite[j].z) << 12;
                            sprite[j].xvel = 0;
                            sprite[j].ang = getangle(sprite[i].x - sprite[j].x, sprite[i].y - sprite[j].y);
                        }
                    }
                /* The original skips the body of the connection walk and uses
                 * the terminating index for this access. */
                for (p = connecthead; p >= 0; p = connectpoint2[p])
                    ;
                if (klabs(sprite[ps[p].i].x - sprite[i].x) + klabs(sprite[ps[p].i].y - sprite[i].y) < m) {
                    ps[p].posx += ksgn(sprite[i].x - ps[p].posx) << 8;
                    ps[p].posy += ksgn(sprite[i].y - ps[p].posy) << 8;
                    clipmove(&ps[p].posx, &ps[p].posy, &ps[p].posz, &ps[p].cursectnum, 0, 0, 192, 1024, 1024, 0);
                }
            }
            break;
        case 19:
            if (t[0]) {
                if (sector[sect].ceilingz >= sector[sect].floorz) {
                    sector[sect].ceilingz = sector[sect].floorz;
                    deletesprite(i);
                    goto BOLT;
                }
                sector[sect].ceilingz += sprite[i].yvel;
            } else if (ifhitsectors(i) >= 0) {
                FTA(8);
                for (l = headspritestat[STAT_EFFECTOR]; l >= 0; l = nextspritestat[l])
                    if ((sprite[l].lotag & 0x7fff) == 19 && sprite[l].hitag == sprite[i].hitag && hittype[l][0] == 0)
                        hittype[l][0] = 1;
            }
            break;
        case 20: {
            short p;

            if (t[0] == 0)
                break;
            if (t[0] == 1)
                sprite[i].xvel = 128;
            else
                sprite[i].xvel = -128;
            if (sprite[i].xvel) {
                long dy;
                long dx;

                dx = (sintable[(sprite[i].ang + 512) & 2047] * sprite[i].xvel) >> 14;
                dy = (sintable[sprite[i].ang & 2047] * sprite[i].xvel) >> 14;
                t[3] += sprite[i].xvel;
                sprite[i].x += dx;
                sprite[i].y += dy;
                if (t[3] <= 0 || (t[3] >> 6) == (sprite[i].yvel >> 6)) {
                    sprite[i].x -= dx;
                    sprite[i].y -= dy;
                    setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
                    t[0] = 0;
                    break;
                }
                for (j = headspritesect[sect]; j >= 0; j = nextspritesect[j])
                    if (sprite[j].statnum != STAT_EFFECTOR) {
                        sprite[j].x += dx;
                        sprite[j].y += dy;
                    }
                dragpoint(t[1], wall[t[1]].x + dx, wall[t[1]].y + dy);
                dragpoint(t[2], wall[t[2]].x + dx, wall[t[2]].y + dy);
                for (p = connecthead; p >= 0; p = connectpoint2[p])
                    if (ps[p].cursectnum == sect && ps[p].on_ground) {
                        ps[p].posx += dx;
                        ps[p].posy += dy;
                    }
                sector[sect].floorxpanning -= dx >> 3;
                sector[sect].floorypanning -= dy >> 3;
                sector[sect].ceilingxpanning -= dx >> 3;
                sector[sect].ceilingypanning -= dy >> 3;
            }
            break;
        }
        case 21:
            if (t[0] == 0)
                break;
            if (sprite[i].ang == 1536)
                height = &sector[sect].ceilingz;
            else
                height = &sector[sect].floorz;
            if (t[0] == 1) {
                sprite[i].xvel = ksgn(sprite[i].z - *height) * (sprite[i].yvel << 4);
                t[0]++;
            }
            if (sector[sect].extra == 0) {
                *height += sprite[i].xvel;
                if (klabs(*height - sprite[i].z) < 1024) {
                    *height = sprite[i].z;
                    deletesprite(i);
                    goto BOLT;
                }
            } else
                sector[sect].extra--;
            break;
        case 22:
            if (t[1]) {
                if (getanimationgoal(&sector[t[0]].ceilingz) >= 0)
                    sector[sect].ceilingz += synctics * sprite[i].yvel;
                else
                    t[1] = 0;
            }
            break;
        case 23:
            if (sprite[i].shade == 0) {
                if (ifhitsectors(i) >= 0)
                    sprite[i].shade = 1;
            } else if (sprite[i].shade == 1) {
                short startwall, endwall;

                /* The initial sector comes from the shared l slot. */
                startwall = sector[l].wallptr;
                endwall = startwall + sector[l].wallnum;
                for (j = startwall; j < endwall; j++)
                    if (sector[wall[j].nextsector1].hitag == sector[l].hitag)
                        wall[j].picnum = 49;
                q = 0x80000000;
                for (x = 0; x < t[0]; x++) {
                    l = msx[t[1] + x];
                    if (q < sector[l].floorz && (sector[l].lotag & 1)) {
                        q = sector[l].floorz;
                        t[2] = l;
                    }
                }
                for (x = 0; x < t[0]; x++) {
                    l = msx[t[1] + x];
                    if (sector[l].floorpicnum != 49)
                        sector[l].floorpicnum = 49;
                    if ((sector[l].lotag & 0x7fff) == 2)
                        sector[l].ceilingz = sector[t[2]].floorz;
                    else {
                        sector[l].lotag |= 1;
                        if (sector[l].floorz >= sector[t[2]].floorz)
                            sector[t[2]].floorz = sector[l].floorz -= sprite[i].yvel;
                        if (sector[l].floorz < sector[l].ceilingz)
                            sector[l].floorz = sector[l].ceilingz;
                    }
                }
                if (sector[sect].floorz < sprite[i].z) {
                    deletesprite(i);
                    goto BOLT;
                }
            }
            break;
        case 24:
            x = (sintable[(sprite[i].ang + 512) & 2047] * sprite[i].yvel) >> 18;
            l = (sintable[sprite[i].ang & 2047] * sprite[i].yvel) >> 18;
            for (j = headspritesect[sect]; j >= 0; j = nextspritesect[j])
                if (sprite[j].statnum != STAT_TRANSPORT && sprite[j].statnum != STAT_DEFAULT && sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_PLAYER)
                    if (sprite[j].picnum < 1175 || sprite[j].picnum > 1178) {
                        sprite[j].x += x;
                        sprite[j].y += l;
                        setsprite(j, sprite[j].x, sprite[j].y, sprite[j].z);
                        if (sprite[j].sectnum != sect) {
                            sprite[j].x += x << 2;
                            sprite[j].y += l << 2;
                        }
                    }
            for (p = connecthead; p >= 0; p = connectpoint2[p])
                if (ps[p].cursectnum == sect && ps[p].on_ground) {
                    ps[p].posx += x;
                    ps[p].posy += l;
                    clipmove(&ps[p].posx, &ps[p].posy, &ps[p].posz, &ps[p].cursectnum, 0, 0, 192, 1024, 1024, 0);
                    setsprite(ps[p].i, ps[p].posx, ps[p].posy, ps[p].posz);
                }
            sector[sect].floorxpanning += l >> 2;
            sector[sect].floorypanning += x >> 2;
            break;
        case 25:
            if (sprite[i].ang)
                t[2] += 64;
            if (ifhitsectors(i) >= 0) {
                t[3]++;
                if (t[3] == 4) {
                    deletesprite(i);
                    goto BOLT;
                }
            }
            if (sector[sect].floorz <= sector[sect].ceilingz)
                sprite[i].shade = 0;
            else if (sector[sect].ceilingz <= t[4])
                sprite[i].shade = 1;
            if (sprite[i].shade) {
                sector[sect].ceilingz += sprite[i].yvel << 4;
                if (sector[sect].ceilingz > sector[sect].floorz)
                    sector[sect].ceilingz = sector[sect].floorz;
            } else {
                sector[sect].ceilingz -= sprite[i].yvel << 4;
                if (sector[sect].ceilingz < t[4])
                    sector[sect].ceilingz = t[4];
            }
            ms(i);
            break;
        case 26:
            sprite[i].xvel = 32;
            l = (sintable[(sprite[i].ang + 512) & 2047] * sprite[i].xvel) >> 14;
            x = (sprite[i].xvel * sintable[sprite[i].ang & 2047]) >> 14;
            sprite[i].shade++;
            if (sprite[i].shade > 7) {
                sprite[i].x = t[3];
                sprite[i].y = t[4];
                sector[sect].floorz -= (sprite[i].zvel * sprite[i].shade) - sprite[i].zvel;
                sprite[i].shade = 0;
            } else
                sector[sect].floorz += sprite[i].zvel;
            for (j = headspritesect[sect]; j >= 0; j = nextspritesect[j])
                if (sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_PLAYER) {
                    sprite[j].x += l;
                    sprite[j].y += x;
                    setsprite(j, sprite[j].x, sprite[j].y, sprite[j].z + sprite[i].zvel);
                }
            for (p = connecthead; p >= 0; p = connectpoint2[p])
                if (ps[p].cursectnum == sect && ps[p].on_ground) {
                    ps[p].posx += l;
                    ps[p].posy += x;
                    ps[p].posz += sprite[i].zvel;
                    setsprite(ps[p].i, ps[p].posx, ps[p].posy, ps[p].posz + 9216);
                }
            startwall = (short)sector[sect].wallptr;
            endwall = startwall + sector[sect].wallnum;
            for (j = startwall; j < endwall; j++)
                if (wall[j].lotag == 0)
                    wall[j].xpanning += 4;
            ms(i);
            break;
        case 27:
            p = findplayer(i, &x);
            if (ps[p].health > 0) {
                if (t[0] < 0) {
                    t[0]++;
                    ud.camerasprite = i;
                } else if (recstat == 2 && ps[p].newowner == -1) {
                    if (cansee(sprite[i].x, sprite[i].y, sprite[i].z, sprite[i].sectnum, ps[p].posx, ps[p].posy, ps[p].posz, ps[p].cursectnum)) {
                        if (x < (unsigned)sprite[i].hitag) {
                            t[0] = 999;
                            ud.camerasprite = i;
                            sprite[i].ang = getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y);
                            sprite[i].yvel = 100 + ((sprite[i].z - ps[p].posz) / 257);
                        } else if (t[0] == 999) {
                            if (ud.camerasprite == i)
                                t[0] = 0;
                            else
                                t[0] = -20;
                            ud.camerasprite = i;
                        }
                    } else if (t[0] == 999) {
                        if (ud.camerasprite == i)
                            t[0] = 0;
                        else
                            t[0] = -20;
                        ud.camerasprite = i;
                    }
                }
            }
            break;
        case 128: {
            short cstat;

            if (wall[t[2]].cstat | 32) {
                wall[t[2]].cstat &= (255 - 32);
                wall[t[2]].cstat |= 16;
                wall[wall[t[2]].nextwall1].cstat &= (255 - 32);
                wall[wall[t[2]].nextwall1].cstat |= 16;
            }
            wall[t[2]].overpicnum++;
            wall[wall[t[2]].nextwall1].overpicnum++;
            if (t[0] == 0)
                lotsofglass(i, 15);
            if (t[0] < t[1])
                t[0]++;
            else {
                cstat = wall[t[2]].cstat;
                wall[t[2]].cstat = cstat & (128 + 32 + 8 + 4 + 2);
                cstat = wall[wall[t[2]].nextwall1].cstat;
                wall[wall[t[2]].nextwall1].cstat = cstat & (128 + 32 + 8 + 4 + 2);
                deletesprite(i);
                goto BOLT;
            }
            break;
        }
        case 1: /* rotation pivots and transporters have no per-tic work here; */
        case 7: /* the labels still shape the dispatch tree (split at 15) */
            break;
        case 129: {
            unsigned short step;
            unsigned char frames[12] = { 1, 0, 2, 0, 1, 0, 2, 0, 1, 0, 2, 0 };

            step = t[0];
            wall[t[2]].overpicnum = 790 + frames[step];
            wall[wall[t[2]].nextwall1].overpicnum = 790 + frames[step + 2];
            t[0]++;
            if (t[0] > 9) {
                deletesprite(i);
                goto BOLT;
            }
            break;
        }
        }
    BOLT:
        i = nexti;
    }
}

/* A negative source selects an existing map sprite by index in pn. */
