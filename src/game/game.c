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

extern char quote_text[13][32];

extern unsigned char scan_ascii[128];
extern unsigned char scan_ascii_shift[128];

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
extern unsigned char input_text[], input_text_length;
extern short moustat, menu_mouse, menu_previous_buttons, menu_drag, menu_x, menu_y;

extern long reccnt;
extern signed char player_avel[], player_vel[], player_svel[];
extern long player_bits[];
extern unsigned char script_tics;
extern signed char demo_avel[16384], demo_vel[16384], demo_svel[16384];
extern long demo_bits[16384];
extern unsigned char demo_tics[16384];

extern char file_id_record[];
/* These two literals belong to an earlier part of the shared GAME pool. */
#define lookup_filename "lookup.dat"
#define lookup_missing "Palette remaps not found."
/* The exit banner opens the shared GAME pool (2:00004); provisional like the two above. */
#define exit_banner "ÞßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßÝÞ DUKE DUKEM 3D Beta version 1.3.95 -- DO NOT DISTRIBUTE!!! That would be bad. ÝÞÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÝ"

#pragma pack(1)
struct user_settings {
    unsigned char visibility, quotes, brightness, sound, music;
    short debug, sound_volume, music_volume, paused, camera, mouse_sensitivity;
    unsigned char reserved[4];
};
#pragma pack()

typedef char user_settings_size_check[sizeof(struct user_settings) == 21 ? 1 : -1];

extern struct user_settings userdefs;
extern char score_names[12][33];
extern long high_scores[12], demomode;
#define userdefs_filename "userdefs.tmp"

/* The packed, 176-byte player record. Field names follow src/game/player.h; GAME's own
   names replace PLAYER's placeholders where GAME shows the meaning (mapzoom, quote_time and
   quote_index, parallax_sector, music, palette and the end-of-level tallies). */
#pragma pack(1)
struct player_view {
    short i;
    long display_mode, mapzoom;
    long posx, posy, posz, horiz;
    long previous_x, previous_y, previous_z, poszv;
    long oposx, oposy, oposz;
    unsigned long score;
    long bobcounter;
    short ang, oang, previous_ang, cursectnum, ocursectnum, lookang;
    short screen_size, ammo, batteryammo, remote_ammo, rocket_ammo, health;
    short newowner, flash_sprite, jumping_counter, airleft, last_weapon;
    short quote_time[13];
    short quote_index, kickback_pic, curr_weapon, state80, state82, parallax_sector;
    char *music;
    unsigned char gm, weapon_clock, skill, level, volume;
    unsigned char on_ground, remote_active, knee_sequence, state92;
    unsigned char scuba_on, jetpack_on, state95, armor;
    unsigned char inventory[5], state9c[5];
    unsigned char weapon_pos, statea2, raise_frame, knee_flash, footstep;
    unsigned char statea6, palette, hard_landing, max_secret_rooms, secret_rooms;
    unsigned char muzzle_flash, max_squashed, actors_squashed, max_actors_killed, actors_killed;
};
#pragma pack()

typedef char player_size_check[sizeof(struct player_view) == 176 ? 1 : -1];

extern struct player_view ps[17];
#define demo_filename "mydemo__.dmo"
#define demo_missing "Demo not found."

extern long hittype[4096][5], msx[1024], msy[1024], mspos;
extern long *actorscrptr[4096];
extern short connecthead, connectpoint2[], randomstash;
extern short camerasprite, viewscreen, gamepaused, numplayers, numsectors, numwalls;
extern unsigned char show2dsprite[512], show2dwall[1024];
extern unsigned char parallaxtype, visibility, option[];
extern long waloff[4096], totalclock, lockclock, randomseed;
extern long animatecnt;
extern short numanimwalls, tilesizy[4096];
extern char music_files[4][10][13];

extern long krand(void);
extern long ksqrt(long square);
extern long getangle(long x, long y);
extern long getincangle_shift(short angle, short target, short shift);
extern long setsprite(short i, long x, long y, long z);
extern int deletesprite(short i);
extern int changespritestat(short i, short stat);
extern short insertsprite(long sect, long stat);
extern void updatesector(long x, long y, short *sect);
extern short nextsectorneighborz(short sect, long z, short plane, short direction);
extern short findplayer(short i, long *distance);
extern long ifhitbyweapon(short i);
extern long getanimationgoal(long *goal);
extern long effectinsect(short i);
extern void movesector(short i);
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
extern signed char pending_svel[17], pending_vel[17], pending_avel[17];
extern long pending_bits[17];
struct game_sound_voice {
    long x, y, z;
    short sprite;
    long handle;
    unsigned char active;
};
extern struct game_sound_voice sound_voices[39];

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


extern short msgtime;
extern unsigned char *msgtext;
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
extern void moveeffects(void);
extern void playerkeys(short p);
extern void processinput(short p);
extern void checksectors(short p);
extern void moveplayers(void);
extern void moveweapons(void);
extern void movestandables(void);
extern void wakeactors(void);
extern void moveactors(void);
extern void movemusic(void);

struct animwall_entry {
    short wallnum;
    long tag;
};
extern struct animwall_entry animwall[256];

/* Declarations used by the functions in 1:04cd0-0ba30. */
char level_warp;
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
extern struct { long MaxVoices, MaxSampleBits, MaxChannels; } fx_device;
extern char *FX_ErrorString(int error), *MUSIC_ErrorString(int error);
extern int FX_SetupCard(int card, void *device);
extern int FX_Init(int card, int voices, int channels, int bits, unsigned rate);
extern int FX_SetCallBack(void (*function)(unsigned long));
extern void FX_SetVolume(int volume), MUSIC_SetVolume(int volume);
extern int MUSIC_Init(int card, int address);
extern void TestCallBack(unsigned long number);
extern void draw_fade_transition(void);
/* Bonus-screen tally marks, x/y pairs, eight per episode (2:02304, GAME _DATA). */
short bonus_marks[3][16] = {
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


/* dataown: one object, one name. GLOBAL defines the timer ramps as avel/vel/svel and the 21-byte settings
   block as userdefs; the separate spellings used here are its fields. */
extern long avel, vel, svel;
#define turn_velocity avel
#define strafe_velocity vel
#define forward_velocity svel
#define disable_armor userdefs.debug
#define gamepaused userdefs.paused
#define camerasprite userdefs.camera
#define facing_sprite userdefs.camera

/* The status bar drawing, written out through the helper calls. */

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

void update_timer_input(void)
{
    if (!keystatus[keys[5]]) {
        if (held(2, 0x4b)) {
            turn_velocity -= 16;
            if (turn_velocity < -128)
                turn_velocity = -128;
        }
        if (held(3, 0x4d)) {
            turn_velocity += 16;
            if (turn_velocity > 127)
                turn_velocity = 127;
        }
    } else {
        if (held(2, 0x4b)) {
            strafe_velocity += 8;
            if (strafe_velocity > 127)
                strafe_velocity = 127;
        }
        if (held(3, 0x4d)) {
            strafe_velocity -= 8;
            if (strafe_velocity < -128)
                strafe_velocity = -128;
        }
    }
    if (held(0, 0x48)) {
        forward_velocity += 8;
        if (forward_velocity > 127)
            forward_velocity = 127;
    }
    if (held(1, 0x50)) {
        forward_velocity -= 8;
        if (forward_velocity < -128)
            forward_velocity = -128;
    }

    if (turn_velocity < 0) {
        turn_velocity += 10;
        if (turn_velocity > 0)
            turn_velocity = 0;
    }
    if (turn_velocity > 0) {
        turn_velocity -= 10;
        if (turn_velocity < 0)
            turn_velocity = 0;
    }
    if (strafe_velocity < 0) {
        strafe_velocity += 2;
        if (strafe_velocity > 0)
            strafe_velocity = 0;
    }
    if (strafe_velocity > 0) {
        strafe_velocity -= 2;
        if (strafe_velocity < 0)
            strafe_velocity = 0;
    }
    if (forward_velocity < 0) {
        forward_velocity += 2;
        if (forward_velocity > 0)
            forward_velocity = 0;
    }
    if (forward_velocity > 0) {
        forward_velocity -= 2;
        if (forward_velocity < 0)
            forward_velocity = 0;
    }
}

#undef held

void level_load_hook(void)
{
}

#define held(binding, scan) \
    ((keystatus[keys[binding]] | keystatus[scan] | keystatus[(scan) + 128]) > 0)

void __interrupt __far timerhandler(void)
{
    totalclock++;
    update_timer_input();
    if (option[2] != 0) {
        totalclock += 5;
        update_timer_input();
        update_timer_input();
        update_timer_input();
        update_timer_input();
        update_timer_input();
    }
    outp(0x20, 0x20);
}

#undef held

void gameexit(char *message)
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
    puts(message);
    exit(0);
}

void domovethings_multiplayer(void)
{
    short other, packbufleng, p;
    long i, j, k;

    while ((packbufleng = getpacket(&other, (unsigned char *)tempbuf)) > 0) {
        switch (tempbuf[0]) {
            case 2:
                script_tics = tempbuf[1];
                j = ((numplayers + 1) >> 1) + 2;
                k = (2 << 3);
                for (i = connecthead; i >= 0; i = connectpoint2[i]) {
                    if (((1 << (k & 7)) & tempbuf[k >> 3]) > 0)
                        player_svel[i] = tempbuf[j++];
                    if (((2 << (k & 7)) & tempbuf[k >> 3]) > 0)
                        player_vel[i] = tempbuf[j++];
                    if ((tempbuf[k >> 3] & (4 << (k & 7))) > 0)
                        player_avel[i] = tempbuf[j++];
                    if (((8 << (k & 7)) & tempbuf[k >> 3]) > 0) {
                        player_bits[i] = tempbuf[j] +
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
                    player_svel[other] = tempbuf[j++];
                if ((tempbuf[1] & 2) > 0)
                    player_vel[other] = tempbuf[j++];
                if ((tempbuf[1] & 4) > 0)
                    player_avel[other] = tempbuf[j++];
                if ((tempbuf[1] & 8) > 0) {
                    player_bits[other] &= 0xffffff00;
                    player_bits[other] |= tempbuf[j++];
                }
                if ((tempbuf[1] & 16) > 0) {
                    player_bits[other] &= 0xffff00ff;
                    player_bits[other] |= tempbuf[j++] << 8;
                }
                if ((tempbuf[1] & 32) > 0) {
                    player_bits[other] &= 0xff00ffff;
                    player_bits[other] |= tempbuf[j++] << 16;
                }
                if ((tempbuf[1] & 64) > 0) {
                    player_bits[other] &= 0x00ffffff;
                    player_bits[other] |= tempbuf[j++] << 24;
                }
                break;
            case 4:
                input_text_length = 0; /* 9.5b keeps only the xor, as in the oracle */
                for (j = 1; j < packbufleng; j++)
                    input_text[input_text_length++] = tempbuf[j];
                msgtext = input_text;
                msgtime = input_text_length << 4;
                input_text[0] = 0;
                break;
            case 5:
                playerreadyflag[other] = 1;
                break;
            case 255:
                ps[myconnectindex].gm = 0x80;
                break;
        }
    }
}

void moveloop(void)
{
    long i, j, clock;

    if (numplayers < 2)
        return;
    for (j = connecthead; j >= 0; j = connectpoint2[j])
        playerreadyflag[j] = 0;
    clock = totalclock - 120;
    if (myconnectindex == connecthead) {
        playerreadyflag[connecthead] = 1;
        do {
            if (totalclock >= clock + 120) {
                clock = totalclock;
                tempbuf[0] = 5;
                for (i = connecthead; i >= 0; i = connectpoint2[i])
                    if (i != connecthead)
                        sendpacket(i, (unsigned char *)tempbuf, 1);
            }
            domovethings_multiplayer();
            j = 0;
            for (i = connecthead; i >= 0; i = connectpoint2[i])
                j += playerreadyflag[i];
        } while (j < numplayers);
    } else {
        do {
            if (totalclock >= clock + 120) {
                clock = totalclock;
                tempbuf[0] = 5;
                sendpacket(connecthead, (unsigned char *)tempbuf, 1);
            }
            domovethings_multiplayer();
        } while (playerreadyflag[connecthead] == 0);
    }
}

void network_move_loop(void)
{
    long i, j, k;
    short p;

    if (myconnectindex == connecthead) {
        getinput(myconnectindex);
        domovethings_multiplayer();
        if (script_tics > 0 && getoutputcirclesize() < 16) {
            tempbuf[0] = 2;
            tempbuf[1] = script_tics;
            j = ((numplayers + 1) >> 1) + 2;
            for (k = 2; k < j; k++)
                tempbuf[k] = 0;
            k = (2 << 3);
            for (i = connecthead; i >= 0; i = connectpoint2[i]) {
                if (player_svel[i] != pending_svel[i]) {
                    tempbuf[j++] = player_svel[i];
                    pending_svel[i] = player_svel[i];
                    tempbuf[k >> 3] |= (1 << (k & 7));
                }
                if (player_vel[i] != pending_vel[i]) {
                    tempbuf[j++] = player_vel[i];
                    pending_vel[i] = player_vel[i];
                    tempbuf[k >> 3] |= (2 << (k & 7));
                }
                if (player_avel[i] != pending_avel[i]) {
                    tempbuf[j++] = player_avel[i];
                    pending_avel[i] = player_avel[i];
                    tempbuf[k >> 3] |= (4 << (k & 7));
                }
                if (player_bits[i] != pending_bits[i]) {
                    tempbuf[j] = player_bits[i];
                    tempbuf[j + 1] = (player_bits[i] >> 8) & 255;
                    tempbuf[j + 2] = (player_bits[i] >> 16) & 255;
                    tempbuf[j + 3] = (player_bits[i] >> 24) & 255;
                    pending_bits[i] = player_bits[i];
                    j += 4;
                    tempbuf[k >> 3] |= (8 << (k & 7));
                }
                k += 4;
            }
            for (i = connecthead; i >= 0; i = connectpoint2[i])
                if (i != myconnectindex)
                    sendpacket(i, (unsigned char *)tempbuf, j);
        domovethings();
            script_tics = 0;
        }
    } else {
        getinput(16);
        if (getoutputcirclesize() < 16) {
            tempbuf[0] = 3;
            tempbuf[1] = 0;
            j = 2;
            if (player_svel[16] != pending_svel[16]) {
                tempbuf[j++] = player_svel[16];
                pending_svel[16] = player_svel[16];
                tempbuf[1] |= 1;
            }
            if (player_vel[16] != pending_vel[16]) {
                tempbuf[j++] = player_vel[16];
                pending_vel[16] = player_vel[16];
                tempbuf[1] |= 2;
            }
            if (player_avel[16] != pending_avel[16]) {
                tempbuf[j++] = player_avel[16];
                pending_avel[16] = player_avel[16];
                tempbuf[1] |= 4;
            }
            if ((player_bits[16] & 0xff) != (pending_bits[16] & 0xff)) {
                tempbuf[j++] = player_bits[16];
                tempbuf[1] |= 8;
            }
            if ((player_bits[16] | 0xff) != (pending_bits[16] | 0xff)) {
                tempbuf[j++] = (player_bits[16] >> 8) & 255;
                tempbuf[1] |= 16;
            }
            if ((player_bits[16] & 0xff0000) != (pending_bits[16] & 0xff0000)) {
                tempbuf[j++] = (player_bits[16] >> 16) & 255;
                tempbuf[1] |= 32;
            }
            if ((player_bits[16] | 0xff0000) != (pending_bits[16] | 0xff0000)) {
                tempbuf[j++] = (player_bits[16] >> 24) & 255;
                tempbuf[1] |= 64;
            }
            pending_bits[16] = player_bits[16];
            sendpacket(connecthead, (unsigned char *)tempbuf, j);
        }
        domovethings_multiplayer();
    }
}

long next_text_shade(void)
{
    short phase = hudpulse + 128;
    long shade = sintable[phase & 2047] >> 10;

    hudpulse = phase;
    return shade + 16;
}

extern unsigned char getscorename(short row);
extern short newscore(void);

void displayquotes(short p)
{
    long offset = 0;

    if (msgtime > 0) {
        msgtime--;
        minitext(8, 162, msgtext, 0);
    }
    if (ps[p].quote_time[ps[p].quote_index] > 0) {
        ps[p].quote_time[ps[p].quote_index]--;
        if (ps[p].screen_size >= 0)
            offset -= 23;
        minitext(24, offset + 189, (unsigned char *)quote_text[ps[p].quote_index], 0);
    }
}

void FTA(short q)
{
    if (userdefs.quotes == 1 && ps[screenpeek].quote_time[q] == 0) {
        ps[screenpeek].quote_time[q] = strlen(quote_text[q]) << 2;
        ps[screenpeek].quote_index = q;
    }
}

void draw_weapon_tile(short x, short y, short tile, unsigned char shade,
                      unsigned char flags)
{
    unsigned char palette;

    if (ps[screenpeek].display_mode == 3) {
        palette = sector[ps[screenpeek].cursectnum].floorpal;
        if (palette)
            overwritesprite(x, y, tile, shade, flags, palette);
        else
            overwritesprite(x, y, tile, shade, flags, ps[screenpeek].palette);
    }
}

long gametext(long sx, long y, unsigned char *text, unsigned char shade)
{
    long x = sx;
    short left;
    short tile, descender;

    left = x;
    if (ps[screenpeek].display_mode == 2)
        return 0;
    while (*text) {
        descender = 2;
        if (*text == '\n') {
            x = left;
            y += 16;
            text++;
            continue;
        }
        if (*text == ' ') {
            x += 5;
            text++;
            continue;
        }
        if (*text >= '0' && *text <= '9')
            tile = *text + 101;
        if (*text >= 'a' && *text <= 'z')
            tile = *text + 26;
        if (*text >= 'A' && *text <= 'Z') {
            tile = *text + 32;
            descender = 0;
        }
        overwritesprite(x, y + descender, tile, shade, 0, 0);
        x += tilesizx[tile] + 1;
        text++;
    }
    return x;
}

long minitext(long x, long y, unsigned char *text, unsigned char shade)
{
    if (ps[screenpeek].display_mode == 2)
        return 0;

#define TEXT_ROW y
/* Small glyph rendering shared with the decimal printer. */
{
    short left = x;
    short tile, descender;

    while (*text) {
        descender = 0;
        if (*text == '\n') {
            x = left;
            TEXT_ROW += 8;
            text++;
            continue;
        }
        if (*text == ' ') {
            x += 2;
            text++;
            continue;
        }
        tile = *text + 991;
        if (*text >= 'a' && *text <= 'z')
            descender = 2;
        overwritesprite(x, TEXT_ROW + descender, tile, shade, 0, 0);
        text++;
        x += tilesizx[tile] + 1;
    }
}
#undef TEXT_ROW
    return x;
}

void numtext_relative(long sx, long sy, long value, unsigned char shade)
{
    ltoa(value, tempbuf, 10);
    gametext(sx, sy - 1, (unsigned char *)tempbuf, shade);
}

int strget(long x, long y, long shade, long limit)
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
    unsigned char maxlen = limit, drawshade = shade;
    unsigned char ch;
    short scan;

    for (scan = 0; scan < 128; scan++)
        if (keystatus[scan] > 0)
        {
            if ((keystatus[0x2a] | keystatus[0x36]) != 0)
            {
                if (scan_ascii_shift[scan] != 0)
                {
                    if (input_text_length < 79)
                        input_text[input_text_length++] = scan_ascii_shift[scan];
                    keystatus[scan] = 0;
                }
            }
            else
            {
                if (scan_ascii[scan] != 0)
                {
                    if (input_text_length < 159)
                        input_text[input_text_length++] = scan_ascii[scan];
                    keystatus[scan] = 0;
                }
            }

            if (input_text_length > maxlen)
                input_text_length = maxlen;

            if (keystatus[0xe] > 0)   /* Backspace */
            {
                    /* Here's a trick of making key repeat after a 1/2 second */
                if ((keystatus[0xe] == 1) || (keystatus[0xe] == 60))
                {
                    if (input_text_length > 0)
                        input_text_length--;
                    else
                        input_text_length = 0;
                }
                input_text[input_text_length] = 0;
                if (keystatus[0xe] < 60)
                {
                    keystatus[0xe] += script_tics;
                    if (keystatus[0xe] > 60) keystatus[0xe] = 60;
                }
            }
        }

    if (drawshade < 128) {
        x = minitext(x, y, input_text, drawshade & 127);
        draw_weapon_tile(x, y, 1023, drawshade, 0);
    } else {
        x = gametext(x, y, input_text, drawshade & 127);
        draw_weapon_tile(x, y, 1022, drawshade, 0);
    }
    return (keystatus[KEY_ACCEPT] | keystatus[KEY_EXTENDED_ACCEPT]) != 0;
}

short findlocator(short tag, short sector)
{
    short i = headspritestat[7];

    while (i >= 0) {
        if ((sector == -1 || sector == sprite[i].sectnum) && tag == sprite[i].lotag)
            return i;
        i = nextspritestat[i];
    }
    return -1;
}

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

void loadlookup(void)
{
    unsigned char count, palette;
    short i;
    int file;

    resetparallax();
    file = open(lookup_filename, O_RDWR | O_BINARY, S_IREAD);
    if (file != -1)
        read(file, &count, 1);
    else
        gameexit(lookup_missing);

    for (i = 0; i < count; i++) {
        read(file, &palette, 1);
        read(file, tempbuf, 256);
        makepalookup(palette, tempbuf, 0, 0, 0);
    }
    close(file);
}

void numtext(long sx, long sy, long value, unsigned char shade)
{
    ltoa(value, tempbuf, 10);
    minitext(sx, sy, (unsigned char *)tempbuf, shade);
}

void numtext_fixed(long x, long y, long value, unsigned char shade)
{
    short i, offset;

    ltoa(value, tempbuf, 10);
    i = strlen(tempbuf);
    offset = 0;
    while (i != 0) {
        overwritesprite(x - offset, y, tempbuf[i - 1] + 0x536, shade, 0, 0);
        offset += 6;
        i--;
    }
}

void draw_meter(long x, long y, long units, unsigned char shade)
{
    enum { FIRST_METER_TILE = 0x570, FULL_METER_TILE = FIRST_METER_TILE + 5 };
    long i, full_cells;

    full_cells = units / 5;
    for (i = 0; i < full_cells; i++) {
        overwritesprite(x - 2, y, FULL_METER_TILE, shade, 0, 0);
        x += tilesizx[FULL_METER_TILE] - 1;
    }
    if (units % 5)
        overwritesprite(x, y, FIRST_METER_TILE + units % 5, shade, 0, 0);
}

void animatesprites(short ang)
{
    short i, j, k, sect;
    long l;

    for (j = 0; j < spritesortcnt; j++) {
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
                    k = (((sprite[i].ang + 3200 - ang) & 2047) >> 8) & 1;
                    break;
                case 3:
                    k = (((sprite[i].ang + 3200 - ang) & 2047) / 192) % 5;
                    if (k > 2) {
                        sprite[i].cstat |= 4;
                        k = 4 - k;
                    } else {
                        sprite[i].cstat &= ~4;
                    }
                    break;
                case 5:
                    k = (((sprite[i].ang + 3200 - ang) & 2047) >> 8) & 7;
                    if (k > 4) {
                        k = 8 - k;
                        sprite[i].cstat |= 4;
                    } else {
                        sprite[i].cstat &= ~4;
                    }
                    break;
                case 7:
                    k = ((sprite[i].ang + 3200 - ang) & 2047) / 170;
                    if (k > 6) {
                        k = 12 - k;
                        sprite[i].cstat |= 4;
                    } else {
                        sprite[i].cstat &= ~4;
                    }
                    break;
                case 8:
                    k = (((sprite[i].ang + 3200 - ang) & 2047) >> 8) & 7;
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
            k = (((sprite[i].ang + 3200 - ang) & 2047) >> 8) & 7;
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
                    spritepal[j] = ps[sprite[i].yvel].palette;
                else
                    spritepal[j] = 0;
            }
        } else if (sprite[i].picnum == 263 || sprite[i].picnum == 264) {
            if (hittype[i][0] != 0) {
                k = (((sprite[i].ang + 3200 - ang) & 2047) >> 8) & 7;
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
            k = (((sprite[i].ang + 3200 - ang) & 2047) >> 8) & 7;
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

void draw_water_distortion(void)
{
    long column, row, depth = 0;
    unsigned char shade, *pixel, *reflection;

    for (column = 0; column < 320; column++) {
        pixel = (unsigned char *)frameplace + column + 99 * 320;
        for (row = 99; row < 200; row++, pixel += 320) {
            if (*pixel == 255) {
                if (depth == 0) {
                    depth = 1;
                    reflection = pixel - 320;
                }
                shade = *reflection & 31;
                shade = shade + 96 - (depth >> 1);
                if (depth > 63 || shade < 96 || shade > 128)
                    shade = 96;
                reflection -= 320;
                depth++;
                *pixel = shade;
            } else {
                depth = 0;
            }
        }
    }
}

void displayrooms(short p)
{
    short s;

    if (ps[p].display_mode == 2) {
        draw2dscreen(ps[p].posx, ps[p].posy, ps[p].ang + ps[p].lookang, ps[p].mapzoom, 0);
        return;
    }
    if ((player_bits[p] & 0x80000) || (player_bits[p] & 0x100000) || (player_bits[p] & 0x400000))
        permanentwritesprite(0, 0, 0x492, 0, 0, 0, 319, 199);
    animatewalls();
    animatecamsprite(p);
    if (facing_sprite >= 0) {
        s = facing_sprite;
        if (sprite[s].yvel < 0)
            sprite[s].yvel = 0;
        else if (sprite[s].yvel > 199)
            sprite[s].yvel = 199;
        drawrooms(sprite[s].x, sprite[s].y, sprite[s].z, sprite[s].ang,
                  sprite[s].yvel, sprite[s].sectnum);
        animatesprites(sprite[s].ang);
    } else {
        drawrooms(ps[p].posx, ps[p].posy, ps[p].posz, ps[p].ang + ps[p].lookang,
                  ps[p].horiz, ps[p].cursectnum);
        animatesprites(ps[p].ang + ps[p].lookang);
    }
    drawmasks();
    if ((gotpic[572 >> 3] & (1 << (572 & 7))) > 0 && waloff[572] != -1) {
        draw_water_distortion();
    }
}

/* draw_weapon_tile's body, written out where the overlay draws. */

/* minitext's small-font loop, written out for the score table. */

/* The view drawing of displayrooms, written out where a loop redraws the view. */
/* gametext's large-font loop, written out where screens draw fixed captions. */

void nonsharedkeys(void)
{
    if (keystatus[0x58]) {
        keystatus[0x58] = 0;
        screencapture();
    }
    if (keystatus[0x44]) {
        keystatus[0x44] = 0;
        userdefs.brightness++;
        if (userdefs.brightness > 4)
            userdefs.brightness = 0;
        setbrightness(userdefs.brightness);
    }
    if (keystatus[0x14] > 0) {
        keystatus[0x14] = 0;
        ps[myconnectindex].gm |= 16;
    }
    if (keystatus[0x20]) {
        keystatus[0x20] = 0;
        userdefs.debug ^= 1;
    }
    if (option[2] && keystatus[0x32]) {
        keystatus[0x32] = 0;
        userdefs.music ^= 1;
        if (userdefs.music == 0)
            MUSIC_Pause();
        else
            MUSIC_Continue();
    }
    if (option[1] && keystatus[0x1f]) {
        keystatus[0x1f] = 0;
        userdefs.sound ^= 1;
        if (userdefs.sound == 0)
            FX_StopAllSounds();
    }
    if (keystatus[0x2f]) {
        keystatus[0x2f] = 0;
        userdefs.visibility--;
        if (userdefs.visibility < 7)
            userdefs.visibility = 16;
    }
    if (keystatus[0x25]) {
        screenpeek = connectpoint2[screenpeek];
        if (screenpeek == -1)
            screenpeek = connecthead;
        if (ps[screenpeek].display_mode == 3) {
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
    if (keystatus[0x11] && userdefs.debug && (ps[myconnectindex].gm & 16) == 0) {
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
            if (keystatus[0x48] | keystatus[0xc8]) {
                ps[0].level++;
                keystatus[0x48] = 0;
                keystatus[0xc8] = 0;
            }
            if (keystatus[0x50] | keystatus[0xd0]) {
                ps[0].level--;
                keystatus[0x50] = 0;
                keystatus[0xd0] = 0;
            }
            ps[0].level &= 15;
            ps[0].volume &= 3;
            gametext(112, 50, warp_title, 0);
            gametext(100, 69, warp_episode, 0);
            gametext(100, 88, warp_level, 0);
            numtext_relative(210, 68, ps[0].volume + 1, 0);
            numtext_relative(210, 87, ps[0].level + 1, 0);
            nextpage();
        }
        if (keystatus[1] == 0)
            enterlevel(4);
        keystatus[0x1c] = keystatus[0x9c] = keystatus[0x39] = keystatus[1] = 0;
    }
}

void show_help(char **argv)
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

void checkcommandline(int argc, char **argv)
{
    short i, j;
    char *c, *mapname;

    userdefs.quotes = 0;
    level_warp = 0;
    i = 1;
    if (argc > 1) {
    while (i < argc) {
        c = argv[i];
        if (*c == '/') {
            c++;
            switch (*c) {
            default:
                printf(unknown_parameter, argv[i]);
            case '?':
                show_help(argv);
                exit(0);
            case 'q':
            case 'Q':
                userdefs.quotes = 1;
                break;
            case 'r':
            case 'R':
                demomode = 1;
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
                userdefs.debug = 1;
                break;
            case 'l':
            case 'L':
                level_warp = 1;
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
            show_help(argv);
            exit(0);
        }
    }
    }
}

/* numtext_fixed's body, written out for the status bar. */
void wait_for_players(void)
{
    short numwanted;

    if (option[4] >= 1 && option[4] <= 4)
        numwanted = 2;
    else
        numwanted = option[4] - 3;
    sendlogon();
    while (numplayers < numwanted && keystatus[1] == 0) {
        domovethings_multiplayer();
    }
    keystatus[1] = 0;
}

void main(int argc, char **argv)
{
    unsigned char which;
    short status;
    long i, j, k;
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
    inittimer();
    puts(main_multiplayer);
    initmultiplayers(option[4], option[5]);
    puts(main_loading_art);
    loadpics(main_tiles_file);
    puts(main_init_remaps);
    initlava();
    loadlookup();
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
        FX_SetVolume(userdefs.sound_volume);
    }
    if (option[2]) {
        puts(main_init_music);
        status = MUSIC_Init(sound_device_map[option[1]], 0);
        if (status)
            gameexit(MUSIC_ErrorString(status));
        MUSIC_SetVolume(userdefs.music_volume);
    }
    if (option[4]) {
        puts(main_locating);
        wait_for_players();
    }
    setgamemode();
    if (level_warp) {
        newgame(ps[0].volume, ps[0].level, ps[0].skill);
        enterlevel(4);
    }

attract:
    if (level_warp == 0)
        playback_demo();
    level_warp = 0;

play:
    while ((ps[myconnectindex].gm & 128) == 0) {
        nonsharedkeys();
        displayrooms(screenpeek);
        if (ps[myconnectindex].gm & 16) {
            if (strget(10, 140, next_text_shade(), 79)) {
                short k;

                tempbuf[0] = 4;
                j = 1;
                for (p = 0; p < input_text_length; p++)
                    tempbuf[j++] = input_text[p];
                for (k = connecthead; k >= 0; k = connectpoint2[k])
                    if (k != myconnectindex)
                        sendpacket(k, (unsigned char *)tempbuf, p);
                ps[myconnectindex].gm ^= 16;
            }
        }
        if (userdefs.paused && ps[screenpeek].display_mode == 3)
            draw_fade_transition();
        if ((ps[myconnectindex].gm & 4) &&
            (((ps[myconnectindex].gm & 1) == 0 && numplayers < 2) ||
             ((ps[myconnectindex].gm & 1) && numplayers >= 2))) {
            if (totalclock < 360 && numplayers > 1 && userdefs.debug) {
                if (myconnectindex == connecthead) {
                    gametext(120, 48, main_master, 0);
                } else {
                    gametext(130, 48, main_slave, 0);
                }
            }
            if (ps[screenpeek].newowner >= 0)
                draw_monitor_overlay();
            else {
                displayweapon(screenpeek);
                displaystatus(screenpeek);
                displayquotes(screenpeek);
            }
            if (numplayers < 2) {
                getinput(0);
                domovethings();
            } else
                network_move_loop();
            if (ps[myconnectindex].gm & 8) {
                ps[myconnectindex].level++;
                enterlevel(4);
                continue;
            }
        }
        if (ps[myconnectindex].gm & 1) {
            ps[myconnectindex].gm &= ~16;
            menus();
            if (ps[myconnectindex].gm & 2)
                goto attract;
        }
        nextpage();
        pan3dsound();
    }
    if (userdefs.debug == 0 && (ps[myconnectindex].gm & 2) == 0) {
        p = newscore();
        if (p >= 0) {
            while (getscorename(p) == 0) {
                nextpage();
                displayrooms(screenpeek);
                displayhighscores();
            }
            memcpy(score_names[p], input_text, 31);
        }
        do {
            displayrooms(screenpeek);
            displayhighscores();
            nextpage();
        } while (keystatus[1] == 0 && keystatus[0x39] == 0 && keystatus[0x1c] == 0);
    }
    write_demo();
    write_userdefs();
    gameexit(main_game_over);
}

void displaystatus(short snum)
{
    short i;

    if (ps[snum].display_mode != 3)
        return;
    if ((sector[ps[snum].cursectnum].lotag & 0x7fff) == 15)
        draw_weapon_tile(160, 120, 1136, 0, 1);
    if (ps[snum].screen_size >= 0) {
        overwritesprite(0, 0, 1380, 0, 0, 0);
        overwritesprite(160, 189, 1381, 0, 1, 0);
        numtext_fixed(82, 6, ps[snum].score, 0);
        numtext_fixed(119, 186, ps[snum].health, 0);
        for (i = 0; i < 25; i++)
            if (i < ps[snum].health >> 2)
                overwritesprite(i * 3 + 30, 186, 1392, 0, 0, 0);
        switch (ps[snum].curr_weapon) {
        case 389:
        case 1230:
            numtext_fixed(209, 186, ps[snum].batteryammo, 0);
            overwritesprite(160, 192, 1400, 0, 1, 0);
            for (i = 0; i < 25; i++)
                if (i < ps[snum].batteryammo >> 2)
                    overwritesprite(i * 3 + 216, 186, 1392, 0, 0, 0);
            break;
        case 191:
            numtext_fixed(209, 186, ps[snum].ammo, 0);
            overwritesprite(160, 192, 1401, 0, 1, 0);
            for (i = 0; i < 25; i++)
                if (i < ps[snum].ammo >> 2)
                    overwritesprite(i * 3 + 216, 186, 1392, 0, 0, 0);
            break;
        case 721:
        case 1140:
            numtext_fixed(209, 186, ps[snum].remote_ammo, 0);
            overwritesprite(160, 192, 1402, 0, 1, 0);
            for (i = 0; i < 25; i++)
                if (i < ps[snum].remote_ammo >> 2)
                    overwritesprite(i * 3 + 216, 186, 1392, 0, 0, 0);
            break;
        case 563:
            numtext_fixed(209, 186, ps[snum].rocket_ammo, 0);
            overwritesprite(160, 192, 1403, 0, 1, 0);
            for (i = 0; i < 25; i++)
                if (i < ps[snum].rocket_ammo >> 2)
                    overwritesprite(i * 3 + 216, 186, 1392, 0, 0, 0);
            break;
        }
    }
    if (ps[snum].state92) {
        overwritesprite(0, 28, 1398, 0, 0, 0);
        if (ps[snum].state95)
            overwritesprite(39, 41, 1399, 0, 0, 0);
        if (ps[snum].armor)
            overwritesprite(39, 66, 1399, 0, 0, 0);
        if (ps[snum].scuba_on)
            overwritesprite(39, 93, 1399, 0, 0, 0);
        if (ps[snum].flash_sprite)
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

void playback_demo(void)
{
    unsigned char which = 1;
    long i;

restart:
    read_demo(which);
    which++;
    if (which == 8)
        which = 0;
    enterlevel(2);
    demomode = 2;
    reccnt--;
    i = 0;
    while (i < reccnt) {
        nonsharedkeys();
        while (totalclock > lockclock) {
            player_svel[0] = demo_svel[i];
            player_vel[0] = demo_vel[i];
            player_avel[0] = demo_avel[i];
            player_bits[0] = demo_bits[i];
            script_tics = demo_tics[i];
            player_bits[0] &= ~0x180000;
            domovethings();
            i++;
        }
        while (totalclock <= lockclock) {
            displayrooms(screenpeek);
            if (ps[screenpeek].newowner >= 0)
                draw_monitor_overlay();
            else if (userdefs.camera == -1) {
                displayweapon(screenpeek);
                displaystatus(screenpeek);
                displayquotes(screenpeek);
            }
            if (keystatus[1]) {
                keystatus[1] = 0;
                ps[myconnectindex].gm |= 1;
                current_menu = 0;
            }
            if (ps[myconnectindex].gm & 1) {
                ps[myconnectindex].gm &= ~16;
                menus();
            }
            if (userdefs.camera >= 0)
                overwritesprite(268, 156, 481, 0, 4, 0);
            pan3dsound();
            nextpage();
            if (ps[myconnectindex].gm == 128)
                return;
            if (ps[myconnectindex].gm & 4) {
                if (ps[myconnectindex].gm & 2)
                    goto restart;
                demomode = 0;
                return;
            }
        }
    }
    goto restart;
}

void record(short player)
{
    demo_svel[reccnt] = player_svel[player];
    demo_vel[reccnt] = player_vel[player];
    demo_avel[reccnt] = player_avel[player];
    demo_bits[reccnt] = player_bits[player];
    demo_tics[reccnt] = script_tics;
    if (++reccnt > 16383)
        reccnt = 16383;
}

/* Water and teleporter pairs (status 9); domovethings writes the same walk out. */


void domovethings(void)
{
    short p;

    randomstash = krand();
    userdefs.camera = -1;
    lockclock += script_tics;
    doanimations(script_tics);
    if (userdefs.paused == 0) {
        if (demomode == 1)
            record(0);
        moveeffects();
        move_status9();
    }
    for (p = connecthead; p >= 0; p = connectpoint2[p]) {
        playerkeys(p);
        if (userdefs.paused == 0) {
            processinput(p);
            checksectors(p);
        }
    }
    if (userdefs.paused == 0) {
        moveplayers();
        moveweapons();
        movestandables();
        moveeffectors();
        wakeactors();
        moveactors();
        movemusic();
    }
    script_tics = 0;
}


void dobonus(void)
{
    short i, start, shade;
    long pct;

    start = totalclock;
    menu_drag = 0;
    while (menu_drag == 0 &&
           (keystatus[0x1c] | keystatus[0x9c] | keystatus[0x39] | keystatus[1]) == 0 &&
           start + 2048 > totalclock) {
        shade = (unsigned char)next_text_shade();
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
        for (i = 0; i < ps[0].level; i++)
            overwritesprite(bonus_marks[ps[0].volume][i * 2], bonus_marks[ps[0].volume][i * 2 + 1],
                            1133, shade, 0, 0);
        overwritesprite(bonus_marks[ps[0].volume][i * 2], bonus_marks[ps[0].volume][i * 2 + 1],
                        1134, shade, 0, 0);
        if (ps[myconnectindex].gm == 8) {
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
                if (ps[0].max_squashed)
                    pct = (ps[0].actors_squashed * 100) / ps[0].max_squashed;
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

void draw_monitor_overlay(void)
{
    short shade = (unsigned char)next_text_shade();

    draw_weapon_tile(10, 10, 275, shade, 2);
    draw_weapon_tile(274, 10, 276, shade, 2);
    draw_weapon_tile(10, 160, 277, shade, 2);
    draw_weapon_tile(274, 160, 278, shade, 2);
    if (totalclock & 16)
        draw_weapon_tile(18, 20, 279, shade, 2);
}

void update_menu_cursor(short tile)
{
    short dx, dy;
    unsigned char shade = next_text_shade();
    long scale;

    menu_previous_buttons = menu_drag;
    getmousevalues(&dx, &dy, &menu_drag);

    if (menu_drag == 0 && menu_previous_buttons != 0)
        menu_mouse = menu_previous_buttons;
    else if (menu_drag == 0 && menu_previous_buttons == 0)
        menu_mouse = menu_drag;

    if (moustat != 0) {
        scale = userdefs.mouse_sensitivity + 129;
        menu_x += (dx * scale) >> 9;
        menu_y += (dy * scale) >> 9;
        if (menu_x < 0)
            menu_x = 0;
        else if (menu_x > 319)
            menu_x = 319;
        if (menu_y < 0)
            menu_y = 0;
        else if (menu_y > 199)
            menu_y = 199;
    }

    draw_weapon_tile(menu_x, menu_y, tile, shade, 1);
}

void displayhighscores(void)
{
    short i;
    short shade = (unsigned char)next_text_shade();

    minitext(30, 36, score_rank_title, 0);
    minitext(50, 36, score_quote_title, 0);
    minitext(270, 36, score_score_title, 0);
    for (i = 0; i < 12; i++) {
        if (i)
            shade = 0;
        numtext(30, i * 11 + 50, i, 0);
        minitext(50, i * 11 + 50, score_names[i], shade);
        numtext(270, i * 11 + 50, high_scores[i], shade);
    }
}



void spawn_random_effects(short i, short count)
{
    short j;
    long z;

    for (j = 0; j < count; j++) {
        z = sprite[i].z - (krand() & 4095);
        if (z < sector[sprite[i].sectnum].ceilingz)
            z = sprite[i].z;
        EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, z, 0, 970 + (j & 3), -32, 18, 18,
            sprite[i].ang - 256 + (krand() & 511), 64, 1024 - (krand() & 2047), sprite[i].owner, 5);
    }
}

void move_status9(void)
{
    short i, j, m, sect;
    long k, p;
    long lotag, otherlotag;

    {
        i = headspritestat[9];
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
                    if (sprite[j].statnum != 3 && sprite[j].statnum != 9 &&
                        sprite[j].picnum != 215 && sprite[j].picnum != 283) {
                        k = headspritestat[9];
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
                    lotag = sector[sect].lotag & 0x7fff;
                    if (lotag == 0) {
                        if (klabs(sprite[i].z - ps[p].posz) > 4096 &&
                            sprite[i].z != sector[sect].floorz)
                            goto nextplayer;
                    } else if (lotag == 2) {
                        if (sector[sect].ceilingz + 0x500 < ps[p].posz)
                            goto nextplayer;
                    } else if (lotag == 1) {
                        if (sector[sect].floorz - 0x500 > ps[p].posz)
                            goto nextplayer;
                    }
                    if (hittype[i][0] == 0) {
                    m = headspritestat[9];
                    while (m >= 0) {
                        if (m != i && sprite[i].hitag == sprite[m].hitag) {
                            otherlotag = lotag;
                            lotag = sector[sprite[m].sectnum].lotag;
                            if (!(otherlotag == 1 && lotag == 0) &&
                                !(otherlotag == 1 && lotag == 1) &&
                                !(otherlotag == 2 && lotag == 2)) {
                                if (lotag == 0) {
                                    ps[p].posx = sprite[m].x;
                                    ps[p].posy = sprite[m].y;
                                    if (sprite[m].z != sprite[i].z)
                                        ps[p].posz = sprite[m].z;
                                    else
                                        ps[p].posz = sector[sprite[m].sectnum].floorz - 0x2400;
                                    hittype[m][0] = 20;
                                    hittype[i][0] = 20;
                                }
                                if (lotag == 1) {
                                    if (ps[p].on_ground)
                                        spritesound(28, ps[p].i);
                                    ps[p].posx += sprite[m].x - sprite[i].x;
                                    ps[p].posy += sprite[m].y - sprite[i].y;
                                    ps[p].posz = sprite[m].z - 0x600;
                                    hittype[m][0] = 1;
                                    hittype[i][0] = 1;
                                }
                                if (lotag == 2) {
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

void read_userdefs(void)
{
    int file;

    demomode = 0;
    file = open(userdefs_filename, O_RDWR | O_BINARY, S_IREAD);
    if (file == -1) {
        userdefs.mouse_sensitivity = 128;
        userdefs.visibility = 13;
        userdefs.sound_volume = 255;
        userdefs.music_volume = 255;
        userdefs.brightness = 1;
        userdefs.sound = 1;
        userdefs.music = 1;
        return;
    }
    read(file, &userdefs, sizeof(userdefs));
    read(file, score_names, sizeof(score_names));
    read(file, high_scores, sizeof(high_scores));
    close(file);
}

void write_userdefs(void)
{
    int file;

    /* Both text and binary flag bits are present in the original call. */
    file = open(userdefs_filename, O_WRONLY | O_CREAT | O_TRUNC | O_TEXT | O_BINARY, 0xb0);
    write(file, &userdefs, sizeof(userdefs));
    write(file, score_names, sizeof(score_names));
    write(file, high_scores, sizeof(high_scores));
    close(file);
}

void saveplayer(char slot)
{
    int file;
    char *gamename = savegame_filename, *boardname = saveboard_filename;

    slot += '1';
    boardname[5] = '1' + ps[0].volume;
    boardname[6] = '1' + ps[0].level;
    boardname[7] = slot;
    gamename[7] = slot;
    file = open(gamename, O_WRONLY | O_CREAT | O_TRUNC | O_TEXT | O_BINARY, 0xb0);
    write(file, ps, sizeof(ps));
    write(file, hittype, sizeof(hittype));
    write(file, animwall, sizeof(animwall));
    write(file, msx, sizeof(msx));
    write(file, msy, sizeof(msy));
    close(file);
    saveboard(boardname, &ps[0].posx, &ps[0].posy, &ps[0].posz,
              &ps[0].ang, &ps[0].cursectnum);
}

void loadplayer(char slot)
{
    int file;
    char *gamename = savegame_filename, *boardname = saveboard_filename;

    slot += '1';
    boardname[5] = '1' + ps[0].volume;
    boardname[6] = '1' + ps[0].level;
    boardname[7] = slot;
    gamename[7] = slot;
    file = open(gamename, O_RDWR | O_BINARY, S_IREAD);
    if (file != -1) {
        read(file, ps, sizeof(ps));
        read(file, hittype, sizeof(hittype));
        read(file, animwall, sizeof(animwall));
        read(file, msx, sizeof(msx));
        read(file, msy, sizeof(msy));
        close(file);
    }
    if (loadboard(boardname, &ps[0].posx, &ps[0].posy, &ps[0].posz,
                  &ps[0].ang, &ps[0].cursectnum) == -1)
        gameexit(saveboard_missing);
    ps[myconnectindex].gm = 4;
}

void read_demo(unsigned char slot)
{
    int file;

    demo_filename[7] = '0' + slot;
    file = open(demo_filename, O_RDWR | O_BINARY, S_IREAD);
    if (file != -1) {
        read(file, &ps[0].volume, sizeof(ps[0].volume));
        read(file, &ps[0].level, sizeof(ps[0].level));
        read(file, &ps[0].skill, sizeof(ps[0].skill));
        newgame(ps[0].volume, ps[0].level, ps[0].skill);
        read(file, &reccnt, sizeof(reccnt));
        read(file, demo_svel, reccnt);
        read(file, demo_vel, reccnt);
        read(file, demo_avel, reccnt);
        read(file, demo_tics, reccnt);
        read(file, demo_bits, reccnt * sizeof(demo_bits[0]));
        close(file);
    } else {
        gameexit(demo_missing);
    }
}

void write_demo(void)
{
    int file;
    char *filename = demo_filename;

    if (demomode != 1)
        return;

    reccnt--;
    filename[7] = '0' + (ps[0].level != 0 || ps[0].volume != 0);

    file = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_TEXT | O_BINARY, 0xb0);
    if (file != -1) {
        write(file, &ps[0].volume, sizeof(ps[0].volume));
        write(file, &ps[0].level, sizeof(ps[0].level));
        write(file, &ps[0].skill, sizeof(ps[0].skill));
        write(file, &reccnt, sizeof(reccnt));
        write(file, demo_svel, reccnt);
        write(file, demo_vel, reccnt);
        write(file, demo_avel, reccnt);
        write(file, demo_tics, reccnt);
        write(file, demo_bits, reccnt * sizeof(demo_bits[0]));
        close(file);
    }
}

/* The dispatch follows LameDuke's status-3 actor and sector conventions, written
 * in the Duke 1.3d moveeffectors style (shared q/l/m/x scratch, st, long j).
 * FTA is expanded in place where the compiler inlined it. */
void moveeffectors(void)
{
    long q, l, m, x, st, j, d, *t;
    short i, k, nexti, p, sect;

    i = headspritestat[3];
    while (i >= 0) {
        nexti = nextspritestat[i];
        sect = sprite[i].sectnum;
        t = &hittype[i][0];
        st = sprite[i].lotag;

        switch (st) {
        case 0:
            for (j = headspritestat[3]; j >= 0; j = nextspritestat[j])
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
            movesector(i);
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
                        sprite[j].x += (sintable[(sprite[i].ang + 512) & 2047] * sprite[i].xvel) >> 14;
                        sprite[j].y += (sprite[i].xvel * sintable[sprite[i].ang & 2047]) >> 14;
                        setsprite(j, sprite[j].x, sprite[j].y, sprite[j].z);
                    }
                movesector(i);
            }
            break;
        case 3:
            if (t[4] == 0)
                break;
            if ((randomstash / (sprite[i].hitag + 1)) & 1) {
                sector[sect].ceilingpal = sprite[i].owner >> 8;
                sector[sect].floorpal = sprite[i].owner;
                t[0] = sprite[i].shade;
            } else {
                sector[sect].ceilingpal = 0;
                sector[sect].floorpal = 0;
                t[0] = t[3];
            }
            sector[sect].floorshade = sector[sect].ceilingshade = t[0];
            k = sector[sect].wallptr + sector[sect].wallnum;
            for (x = sector[sect].wallptr; x < k; x++)
                if (!(wall[x].hitag & 1))
                    wall[x].shade = t[0];
            break;
        case 4:
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
            k = sector[sect].wallptr + sector[sect].wallnum;
            for (x = sector[sect].wallptr; x < k; x++)
                if (!(wall[x].hitag & 1))
                    wall[x].shade = t[0];
            break;
        case 5:
            p = findplayer(i, &x);
            if (sprite[i].owner == -1) {
                t[4] = 0;
                l = 0x7fffffff;
                while (1) {
                    sprite[i].owner = findlocator(t[4], -1);
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
            q = getincangle_shift(sprite[i].ang, x, 3);
            sprite[i].ang += q;
            if ((krand() >> 8) >= 223) {
                t[2] += q;
                sector[sect].ceilingshade = 127;
            } else {
                t[2] += getincangle_shift(t[2] + 512, getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y), 2);
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
            movesector(i);
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
            for (j = headspritestat[3]; j >= 0; j = nextspritestat[j])
                if (sprite[j].lotag == 14 && sprite[j].hitag == sprite[i].hitag && hittype[j][0] == t[0]) {
                    sprite[j].xvel = sprite[i].xvel;
                    x++;
                    hittype[j][4] = t[4];
                }
        case 14:
            if (sprite[i].owner == -1)
                sprite[i].owner = findlocator(t[3], t[0]);
            d = klabs(sprite[sprite[i].owner].x - sprite[i].x) + klabs(sprite[sprite[i].owner].y - sprite[i].y);
            if (d < 1024) {
                if (st == 6 && (sprite[sprite[i].owner].hitag & 1))
                    t[4] = 256;
                t[3]++;
                sprite[i].owner = findlocator(t[3], t[0]);
                if (sprite[i].owner == -1) {
                    t[3] = 0;
                    sprite[i].owner = findlocator(0, t[0]);
                }
            }
            if (sprite[i].xvel) {
                x = getangle(sprite[sprite[i].owner].x - sprite[i].x, sprite[sprite[i].owner].y - sprite[i].y);
                q = getincangle_shift(sprite[i].ang, x, 3);
                t[2] += q;
                sprite[i].ang += q;
                if (sprite[i].xvel == 256 && sound_voices[32].active == 0) {
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
                movesector(i);
            }
            break;
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
            for (j = headspritestat[3]; j >= 0; j = nextspritestat[j]) {
                k = sprite[j].sectnum;
                m = sprite[j].shade;
                if (sprite[j].lotag == st && sprite[j].hitag == sprite[i].hitag) {
                    p = sector[k].wallptr + sector[k].wallnum;
                    for (l = sector[k].wallptr; l < p; l++)
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
                    if (sprite[i].hitag < ++t[0]) {
                        operatesectors(sect, i);
                        t[0] = 0;
                    }
                }
            } else if (sector[sect].floorz > sector[sect].ceilingz) {
                if (t[1] == 0) {
                    if (sprite[i].hitag < ++t[0]) {
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
                short step;

                step = (sprite[i].yvel >> 3) * t[3];
                t[2] += step;
                t[4] += step;
                movesector(i);
                for (j = sector[sect].wallptr; j < (short)(sector[sect].wallptr + sector[sect].wallnum); j++)
                    for (p = connecthead; p >= 0; p = connectpoint2[p])
                        if (clipinsidebox(ps[p].posx, ps[p].posy, j, 128) == 1) {
                            t[4] -= step;
                            t[2] -= step;
                            movesector(i);
                        }
                if (t[4] <= -511 || t[4] >= 512) {
                    t[4] = 0;
                    t[2] &= 0xffffff00;
                    movesector(i);
                }
            }
            break;
        case 12:
            if (t[0] == 1) {
                k = sector[sect].wallptr + sector[sect].wallnum;
                for (j = sector[sect].wallptr; j < k; j++)
                    wall[j].shade = t[1];
                sector[sect].floorshade = t[1];
                sector[sect].ceilingshade = t[2];
                t[0] = 0;
            } else if (t[0] == 2) {
                if (sector[sect].floorshade > sprite[i].shade) {
                    sector[sect].floorshade -= 2;
                    sector[sect].ceilingshade -= 2;
                    for (j = sector[sect].wallptr; j < (short)(sector[sect].wallptr + sector[sect].wallnum); j++)
                        wall[j].shade -= 2;
                } else
                    t[0] = 0;
            }
            break;
        case 13:
            if (t[2] == 0) {
                for (j = headspritestat[6]; j >= 0; j = nextspritestat[j])
                    if (sprite[j].picnum == 57 && sprite[j].shade == -32 && sprite[j].hitag == sprite[i].hitag) {
                        for (j = headspritestat[3]; j >= 0; j = nextspritestat[j])
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
                        for (j = sector[sect].wallptr; j < k; j++)
                            wall[j].shade = sprite[i].shade;
                        sector[sect].floorshade = sprite[i].shade;
                        if (ps[0].parallax_sector >= 0) {
                            sector[sect].ceilingpicnum = sector[ps[0].parallax_sector].ceilingpicnum;
                            sector[sect].ceilingshade = sector[ps[0].parallax_sector].ceilingshade;
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
                nuke(sprite[i].x, sprite[i].y, x, i);
                for (x = 0; x < 30; x += 3) {
                    long sx = sprite[i].x, sy = sprite[i].y;
                    long sz = sector[sect].floorz - (krand() & 0x1000);
                    short pic = krand() % 11 + 235, a = krand() & 2047;
                    short xv = (krand() & 0x80) + 64;
                    long zv = -(krand() & 511) - 512;
                    long *actor;

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
                movesector(i);
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
            movesector(i);
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
                        for (j = headspritestat[3]; j >= 0; j = nextspritestat[j])
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
            } else if (effectinsect(i) >= 0) {
                FTA(8);
                for (l = headspritestat[3]; l >= 0; l = nextspritestat[l])
                    if ((sprite[l].lotag & 0x7fff) == 19 && sprite[l].hitag == sprite[i].hitag && hittype[l][0] == 0)
                        hittype[l][0] = 1;
            }
            break;
        case 20:
            if (t[0] == 0)
                break;
            if (t[0] == 1)
                sprite[i].xvel = 128;
            else
                sprite[i].xvel = -128;
            if (sprite[i].xvel) {
                long dx, dy;

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
                    if (sprite[j].statnum != 3) {
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
        case 21:
            if (t[0] == 0)
                break;
            if (sprite[i].ang == 1536)
                l = (long)&sector[sect].ceilingz;
            else
                l = (long)&sector[sect].floorz;
            if (t[0] == 1) {
                sprite[i].xvel = ksgn(sprite[i].z - *(long *)l) * (sprite[i].yvel << 4);
                t[0]++;
            }
            if (sector[sect].extra == 0) {
                *(long *)l += sprite[i].xvel;
                if (klabs(*(long *)l - sprite[i].z) < 1024) {
                    *(long *)l = sprite[i].z;
                    deletesprite(i);
                    goto BOLT;
                }
            } else
                sector[sect].extra--;
            break;
        case 22:
            if (t[1]) {
                if (getanimationgoal(&sector[t[0]].ceilingz) >= 0)
                    sector[sect].ceilingz += sprite[i].yvel * script_tics;
                else
                    t[1] = 0;
            }
            break;
        case 23:
            if (sprite[i].shade == 0) {
                if (effectinsect(i) >= 0)
                    sprite[i].shade = 1;
            } else if (sprite[i].shade == 1) {
                /* The initial sector comes from the shared l slot. */
                for (j = sector[l].wallptr; j < (short)(sector[l].wallptr + sector[l].wallnum); j++)
                    if (sector[wall[j].nextsector].hitag == sector[l].hitag)
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
            x = (sprite[i].yvel * sintable[(sprite[i].ang + 512) & 2047]) >> 18;
            l = (sprite[i].yvel * sintable[sprite[i].ang & 2047]) >> 18;
            for (j = headspritesect[sect]; j >= 0; j = nextspritesect[j])
                if (sprite[j].statnum != 9 && sprite[j].statnum != 0 && sprite[j].statnum != 3 && sprite[j].statnum != 10)
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
            if (effectinsect(i) >= 0) {
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
            movesector(i);
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
                if (sprite[j].statnum != 3 && sprite[j].statnum != 10) {
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
            k = sector[sect].wallptr + sector[sect].wallnum;
            for (j = sector[sect].wallptr; j < k; j++)
                if (wall[j].lotag == 0)
                    wall[j].xpanning += 4;
            movesector(i);
            break;
        case 27:
            p = findplayer(i, &x);
            if (ps[p].health > 0) {
                if (t[0] < 0) {
                    t[0]++;
                    camerasprite = i;
                } else if (demomode == 2 && ps[p].newowner == -1) {
                    if (cansee(sprite[i].x, sprite[i].y, sprite[i].z, sprite[i].sectnum, ps[p].posx, ps[p].posy, ps[p].posz, ps[p].cursectnum)) {
                        if (x < (unsigned)sprite[i].hitag) {
                            t[0] = 999;
                            camerasprite = i;
                            sprite[i].ang = getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y);
                            sprite[i].yvel = 100 + ((sprite[i].z - ps[p].posz) / 257);
                        } else if (t[0] == 999) {
                            if (camerasprite == i)
                                t[0] = 0;
                            else
                                t[0] = -20;
                            camerasprite = i;
                        }
                    } else if (t[0] == 999) {
                        if (camerasprite == i)
                            t[0] = 0;
                        else
                            t[0] = -20;
                        camerasprite = i;
                    }
                }
            }
            break;
        case 128:
            if (wall[t[2]].cstat | 32) {
                wall[t[2]].cstat &= (255 - 32);
                wall[t[2]].cstat |= 16;
                wall[wall[t[2]].nextwall].cstat &= (255 - 32);
                wall[wall[t[2]].nextwall].cstat |= 16;
            }
            wall[t[2]].overpicnum++;
            wall[wall[t[2]].nextwall].overpicnum++;
            if (t[0] == 0)
                spawn_random_effects(i, 15);
            if (t[0] < t[1])
                t[0]++;
            else {
                wall[t[2]].cstat &= (128 + 32 + 8 + 4 + 2);
                wall[wall[t[2]].nextwall].cstat &= (128 + 32 + 8 + 4 + 2);
                deletesprite(i);
                goto BOLT;
            }
            break;
        case 1: /* rotation pivots and transporters have no per-tic work here; */
        case 7: /* the labels still shape the dispatch tree (split at 15) */
            break;
        case 129: {
            unsigned char frames[12] = { 1, 0, 2, 0, 1, 0, 2, 0, 1, 0, 2, 0 };

            wall[t[2]].overpicnum = frames[t[0]] + 790;
            wall[wall[t[2]].nextwall].overpicnum = frames[t[0] + 2] + 790;
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
