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
extern unsigned char keys[17];
extern volatile unsigned char keystatus[256];
extern unsigned char input_text[], input_text_length;
extern short moustat, menu_mouse, menu_previous_buttons, menu_drag, menu_x, menu_y;

extern signed char demo_avel[16384], demo_vel[16384], demo_svel[16384];
extern long demo_bits[16384];
extern unsigned char demo_tics[16384];

extern long getincangle_shift(short angle, short target, short shift);
extern short insertsprite(long sect, long stat);
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

struct game_sound_voice {
    long x, y, z;
    short sprite;
    long handle;
    unsigned char active;
};
extern struct game_sound_voice sound_voices[39];

#define FTA_INLINE(quote)                                                \
    if (userdefs.quotes == 1 && ps[screenpeek].quote_time[quote] == 0) { \
        ps[screenpeek].quote_time[quote] = (short)(strlen(quote_text[quote]) << 2); \
        ps[screenpeek].quote_index = (quote);                            \
    }

extern short msgtime;
extern unsigned char *msgtext;
extern short thesprite[], spritepicnum[];
extern char spritepal[];
extern long spritesortcnt;
extern unsigned char gotpic[];
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

/* Declarations used by the functions in 1:04cd0-0ba30. */
char level_warp;
extern char output_path[];
extern int saveboard(char *filename, long *x, long *y, long *z, short *ang, short *sectnum);
extern void init_bootleg_cache(void), loadefs(void), initmenus(void), initengine(void);
extern void initmultiplayers(unsigned char option, unsigned char rate);
extern void loadpics(char *filename), init_water_ripples(void), loadsounds(void);
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
extern long current_menu;
extern void displayweapon(short p), menus(void), pan3dsound(void);
extern void sendlogon(void);
extern void screencapture(void);
extern void setbrightness(unsigned char brightness);
extern void MUSIC_Pause(void), MUSIC_Continue(void);
extern void qsetmode640480(void);

/* The status bar drawing, written out through the helper calls. */
#define STATUS_BAR(P)                                                     \
{                                                                         \
    short snum = (P);                                                     \
    short j;                                                              \
                                                                          \
    if (ps[snum].display_mode == 3) {                                     \
        if ((sector[ps[snum].cursectnum].lotag & 0x7fff) == 15)           \
            draw_weapon_tile(160, 120, 1136, 0, 1);                       \
        if (ps[snum].screen_size >= 0) {                                  \
            overwritesprite(0, 0, 1380, 0, 0, 0);                         \
            overwritesprite(160, 189, 1381, 0, 1, 0);                     \
            numtext_fixed(82, 6, ps[snum].score, 0);                      \
            numtext_fixed(119, 186, ps[snum].health, 0);                  \
            for (j = 0; j < 25; j++)                                      \
                if (j < ps[snum].health >> 2)                             \
                    overwritesprite(j * 3 + 30, 186, 1392, 0, 0, 0);      \
            switch (ps[snum].curr_weapon) {                               \
            case 389:                                                     \
            case 1230:                                                    \
                numtext_fixed(209, 186, ps[snum].batteryammo, 0);         \
                overwritesprite(160, 192, 1400, 0, 1, 0);                 \
                for (j = 0; j < 25; j++)                                  \
                    if (j < ps[snum].batteryammo >> 2)                    \
                        overwritesprite(j * 3 + 216, 186, 1392, 0, 0, 0); \
                break;                                                    \
            case 191:                                                     \
                numtext_fixed(209, 186, ps[snum].ammo, 0);                \
                overwritesprite(160, 192, 1401, 0, 1, 0);                 \
                for (j = 0; j < 25; j++)                                  \
                    if (j < ps[snum].ammo >> 2)                           \
                        overwritesprite(j * 3 + 216, 186, 1392, 0, 0, 0); \
                break;                                                    \
            case 721:                                                     \
            case 1140:                                                    \
                numtext_fixed(209, 186, ps[snum].remote_ammo, 0);         \
                overwritesprite(160, 192, 1402, 0, 1, 0);                 \
                for (j = 0; j < 25; j++)                                  \
                    if (j < ps[snum].remote_ammo >> 2)                    \
                        overwritesprite(j * 3 + 216, 186, 1392, 0, 0, 0); \
                break;                                                    \
            case 563:                                                     \
                numtext_fixed(209, 186, ps[snum].rocket_ammo, 0);         \
                overwritesprite(160, 192, 1403, 0, 1, 0);                 \
                for (j = 0; j < 25; j++)                                  \
                    if (j < ps[snum].rocket_ammo >> 2)                    \
                        overwritesprite(j * 3 + 216, 186, 1392, 0, 0, 0); \
                break;                                                    \
            }                                                             \
        }                                                                 \
        if (ps[snum].state92) {                                           \
            overwritesprite(0, 28, 1398, 0, 0, 0);                        \
            if (ps[snum].state95)                                         \
                overwritesprite(39, 41, 1399, 0, 0, 0);                   \
            if (ps[snum].armor)                                           \
                overwritesprite(39, 66, 1399, 0, 0, 0);                   \
            if (ps[snum].scuba_on)                                        \
                overwritesprite(39, 93, 1399, 0, 0, 0);                   \
            if (ps[snum].flash_sprite)                                    \
                overwritesprite(39, 119, 1399, 0, 0, 0);                  \
            if (ps[snum].jetpack_on)                                      \
                overwritesprite(39, 145, 1399, 0, 0, 0);                  \
            if (ps[snum].inventory[2])                                    \
                overwritesprite(13, 39, 717, 0, 0, 0);                    \
            if (ps[snum].inventory[3])                                    \
                overwritesprite(14, 64, 719, 0, 0, 0);                    \
            if (ps[snum].inventory[0])                                    \
                overwritesprite(13, 90, 716, 0, 0, 0);                    \
            if (ps[snum].inventory[4])                                    \
                overwritesprite(13, 117, 718, 0, 0, 0);                   \
            if (ps[snum].inventory[1])                                    \
                overwritesprite(13, 141, 713, 0, 0, 0);                   \
        }                                                                 \
    }                                                                     \
}                                                                         \
/* The message and quote lines, written out through minitext. */
#define SHOW_QUOTES(P)                                                                 \
{                                                                                      \
    offset = 0;                                                                        \
    if (msgtime > 0) {                                                                 \
        msgtime--;                                                                     \
        minitext(8, 162, msgtext, 0);                                                  \
    }                                                                                  \
    if (ps[P].quote_time[ps[P].quote_index] > 0) {                                     \
        ps[P].quote_time[ps[P].quote_index]--;                                         \
        if (ps[P].screen_size >= 0)                                                    \
            offset -= 23;                                                              \
        minitext(24, offset + 189, (unsigned char *)quote_text[ps[P].quote_index], 0); \
    }                                                                                  \
}

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

/* Each do/while (0) block is update_timer_input() as the compiler inlines it (COMPILER_STEERED: the
   block boundary keeps totalclock++ / += 5 as memory inc/add, as the inline expansion does). */
void __interrupt __far timerhandler(void)
{
    totalclock++;
    do {
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
    } while (0);
    if (option[2] != 0) {
        totalclock += 5;
        do {
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
        } while (0);
        do {
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
        } while (0);
        do {
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
        } while (0);
        do {
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
        } while (0);
        do {
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
        } while (0);
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
    puts("ÞßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßÝÞ DUKE DUKEM 3D Beta version 1.3.95 -- DO NOT DISTRIBUTE!!! That would be bad. ÝÞÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÝ");
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
                randomstash = krand();
                facing_sprite = -1;
                lockclock += script_tics;
                doanimations(script_tics);
                if (gamepaused == 0) {
                    if (demomode == 1)
                        record(0);
                    moveeffects();
                    move_status9();
                }
                for (p = connecthead; p >= 0; p = connectpoint2[p]) {
                    playerkeys(p);
                    if (gamepaused == 0) {
                        processinput(p);
                        checksectors(p);
                    }
                }
                if (gamepaused == 0) {
                    moveplayers();
                    moveweapons();
                    movestandables();
                    moveeffectors();
                    wakeactors();
                    moveactors();
                    movemusic();
                }
                script_tics = 0;
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
            /* Packet handling shared with domovethings_multiplayer. */
            {
                short other, packbufleng;
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
                                if ((tempbuf[k >> 3] & (2 << (k & 7))) > 0)
                                    player_vel[i] = tempbuf[j++];
                                if (((4 << (k & 7)) & tempbuf[k >> 3]) > 0)
                                    player_avel[i] = tempbuf[j++];
                                if ((tempbuf[k >> 3] & (8 << (k & 7))) > 0) {
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
                            input_text_length = 0;
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
            /* Packet handling shared with domovethings_multiplayer. */
            {
                short other, packbufleng;
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
                            input_text_length = 0;
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
        } while (playerreadyflag[connecthead] == 0);
    }
}

void network_move_loop(void)
{
    long i, j, k;
    short p;

    if (myconnectindex == connecthead) {
        getinput(myconnectindex);
        /* Packet handling shared with domovethings_multiplayer. */
        {
            short other, packbufleng;
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
                        input_text_length = 0;
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
        randomstash = krand();
        facing_sprite = -1;
        lockclock += script_tics;
        doanimations(script_tics);
        if (gamepaused == 0) {
            if (demomode == 1)
                record(0);
            moveeffects();
            move_status9();
        }
        for (p = connecthead; p >= 0; p = connectpoint2[p]) {
            playerkeys(p);
            if (gamepaused == 0) {
                processinput(p);
                checksectors(p);
            }
        }
        if (gamepaused == 0) {
            moveplayers();
            moveweapons();
            movestandables();
            moveeffectors();
            wakeactors();
            moveactors();
            movemusic();
        }
        script_tics = 0;
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
        /* Packet handling shared with domovethings_multiplayer. */
        {
            short other, packbufleng;
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
                        input_text_length = 0;
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
    }
}

long next_text_shade(void)
{
    short phase = hudpulse + 128;
    long shade = sintable[phase & 2047] >> 10;

    hudpulse = phase;
    return shade + 16;
}

/* The same routine where the original calls it instead of expanding it in place. */
extern long score_text_shade(void);
#pragma aux score_text_shade "next_text_shade_"
/* main tests the file check as a full int result. */
extern long file_id_check(void);
#pragma aux file_id_check "check_file_id_"

void displayquotes(short p)
{
    long ofs = 0;

    if (msgtime > 0) {
        msgtime--;
        {
        char *m = msgtext;
        long x = 8, y = 162;
        unsigned char *text = (unsigned char *)m;
        unsigned char shade_ = 0;
        short left, tile, descender;

        if (ps[screenpeek].display_mode != 2) {
            left = x;
            while (*text) {
                descender = 0;
                if (*text == '\n') {
                    x = left;
                    y += 8;
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
                overwritesprite(x, y + descender, tile, shade_, 0, 0);
                text++;
                x += tilesizx[tile] + 1;
            }
        }
        }
    }

    if (ps[p].quote_time[ps[p].quote_index] > 0) {
        ps[p].quote_time[ps[p].quote_index]--;
        if (ps[p].screen_size >= 0)
            ofs -= 23;
        {
        long x = 24, y = ofs + 189;
        unsigned char *text = (unsigned char *)quote_text[ps[p].quote_index];
        unsigned char shade_ = 0;
        short left, tile, descender;

        if (ps[screenpeek].display_mode != 2) {
            left = x;
            while (*text) {
                descender = 0;
                if (*text == '\n') {
                    x = left;
                    y += 8;
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
                overwritesprite(x, y + descender, tile, shade_, 0, 0);
                text++;
                x += tilesizx[tile] + 1;
            }
        }
        }
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
    {
    long x = sx, y = sy - 1;
    unsigned char *text = (unsigned char *)tempbuf;
    unsigned char shade_ = shade;
    short left, tile, descender;

    left = x;
    if (ps[screenpeek].display_mode != 2) {
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
            overwritesprite(x, y + descender, tile, shade_, 0, 0);
            text++;
            x += tilesizx[tile] + 1;
        }
    }
    }
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
    unsigned char palette, ch;
    short scan;
    short left = x, top = y;
    short tile, descender;
    long column = left, row = top;
    unsigned char *text;

    for (scan = 0; scan < 128; scan++) {
        if (keystatus[scan] != 0) {
            if (keystatus[KEY_LEFT_SHIFT] | keystatus[KEY_RIGHT_SHIFT]) {
                ch = scan_ascii_shift[scan];
                if (ch != 0) {
                    if (input_text_length < 79)
                        input_text[input_text_length++] = ch;
                    keystatus[scan] = 0;
                }
            } else if (scan_ascii[scan] != 0) {
                if (input_text_length < 159)
                    input_text[input_text_length++] = scan_ascii[scan];
                keystatus[scan] = 0;
            }

            if (input_text_length > maxlen)
                input_text_length = maxlen;
            if (keystatus[KEY_BACKSPACE] != 0) {
                if (keystatus[KEY_BACKSPACE] == 1 ||
                    keystatus[KEY_BACKSPACE] == REPEAT_THRESHOLD) {
                    if (input_text_length != 0)
                        input_text_length--;
                }
                input_text[input_text_length] = 0;
                if (keystatus[KEY_BACKSPACE] < REPEAT_THRESHOLD) {
                    keystatus[KEY_BACKSPACE] += script_tics;
                    if (keystatus[KEY_BACKSPACE] > REPEAT_THRESHOLD)
                        keystatus[KEY_BACKSPACE] = REPEAT_THRESHOLD;
                }
            }
        }
    }

    text = input_text;
    if (drawshade & LARGE_FONT_FLAG) {
        if (ps[screenpeek].display_mode == 2) {
            column = 0;
        } else {
            tile = -1;
            while (*text) {
                descender = 2;
                if (*text == '\n') {
                    column = left;
                    row += 16;
                } else if (*text == ' ') {
                    column += 5;
                } else {
                    if (*text >= '0' && *text <= '9')
                        tile = *text + 101;
                    if (*text >= 'a' && *text <= 'z')
                        tile = *text + 26;
                    if (*text >= 'A' && *text <= 'Z') {
                        tile = *text + 32;
                        descender = 0;
                    }
                    if (tile >= 0) {
                        overwritesprite(column, row + descender, tile, drawshade & 127, 0, 0);
                        column += tilesizx[tile] + 1;
                    }
                }
                text++;
            }
        }
        if (ps[screenpeek].display_mode == 3) {
            palette = sector[ps[screenpeek].cursectnum].floorpal;
            if (palette)
                overwritesprite((short)column, top, 1022, drawshade, 0, palette);
            else
                overwritesprite((short)column, top, 1022, drawshade, 0, ps[screenpeek].palette);
        }
    } else {
        if (ps[screenpeek].display_mode == 2) {
            column = 0;
        } else {
            while (*text) {
                if (*text == '\n') {
                    column = left;
                    row += 8;
                } else if (*text == ' ') {
                    column += 2;
                } else {
                    tile = *text + 991;
                    descender = 0;
                    if (*text >= 'a' && *text <= 'z')
                        descender = 2;
                    overwritesprite(column, row + descender, tile, drawshade & 127, 0, 0);
                    column += tilesizx[tile] + 1;
                }
                text++;
            }
        }
        if (ps[screenpeek].display_mode == 3) {
            palette = sector[ps[screenpeek].cursectnum].floorpal;
            if (palette)
                overwritesprite((short)column, top, 1023, drawshade, 0, palette);
            else
                overwritesprite((short)column, top, 1023, drawshade, 0, ps[screenpeek].palette);
        }
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
    file = open("lookup.dat", O_RDWR | O_BINARY, S_IREAD);
    if (file != -1)
        read(file, &count, 1);
    else
        gameexit("Palette remaps not found.");

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
    {
    long x = sx, y = sy;
    unsigned char *text = (unsigned char *)tempbuf;
    unsigned char shade_ = shade;
    short left, tile, descender;

    if (ps[screenpeek].display_mode != 2) {
        left = x;
        while (*text) {
            descender = 0;
            if (*text == '\n') {
                x = left;
                y += 8;
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
            overwritesprite(x, y + descender, tile, shade_, 0, 0);
            text++;
            x += tilesizx[tile] + 1;
        }
    }
    }
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
        /* Sprite frame selection shared with animatesprites. */
        {
            short ang = sprite[s].ang;
            long l;
            short i, j, k, sect;

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
    } else {
        drawrooms(ps[p].posx, ps[p].posy, ps[p].posz, ps[p].ang + ps[p].lookang,
                  ps[p].horiz, ps[p].cursectnum);
        /* Sprite frame selection shared with animatesprites. */
        {
            short ang = ps[p].ang + ps[p].lookang;
            long l;
            short i, j, k, sect;

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
    }
    drawmasks();
    if ((gotpic[572 >> 3] & (1 << (572 & 7))) > 0 && waloff[572] != -1) {
        /* Floor reflection shared with draw_water_distortion. */
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
    }
}

/* draw_weapon_tile's body, written out where the overlay draws. */
#define WEAPON_TILE(X, Y, TILE, SHADE, FLAGS)                                       \
{                                                                                   \
    short x_ = (X), y_ = (Y), tile_ = (TILE);                                       \
    unsigned char shade_ = (SHADE), flags_ = (FLAGS), palette_;                     \
                                                                                    \
    if (ps[screenpeek].display_mode == 3) {                                         \
        palette_ = sector[ps[screenpeek].cursectnum].floorpal;                      \
        if (palette_)                                                               \
            overwritesprite(x_, y_, tile_, shade_, flags_, palette_);               \
        else                                                                        \
            overwritesprite(x_, y_, tile_, shade_, flags_, ps[screenpeek].palette); \
    }                                                                               \
}

/* minitext's small-font loop, written out for the score table. */
#define SMALL_TEXT(X, Y, TEXT, SHADE) \
{                       \
    long x = (X), y = (Y); \
    unsigned char *t_ = (unsigned char *)(TEXT); \
    unsigned char shade_ = (SHADE); \
    unsigned char *text = t_; \
    short left, tile, descender; \
\
    if (ps[screenpeek].display_mode != 2) { \
        left = x;       \
        while (*text) { \
            descender = 0; \
            if (*text == '\n') { \
                x = left; \
                y += 8; \
                text++; \
                continue; \
            }           \
            if (*text == ' ') { \
                x += 2; \
                text++; \
                continue; \
            }           \
            tile = *text + 991; \
            if (*text >= 'a' && *text <= 'z') \
                descender = 2; \
            overwritesprite(x, y + descender, tile, shade_, 0, 0); \
            text++;     \
            x += tilesizx[tile] + 1; \
        }               \
    }                   \
}

/* The view drawing of displayrooms, written out where a loop redraws the view. */
#define DRAW_VIEW(P)                                                                         \
{                                                                                            \
    short p_ = (P);                                                                          \
    short p = p_;                                                                            \
                                                                                             \
    if (ps[p].display_mode == 2)                                                             \
        draw2dscreen(ps[p].posx, ps[p].posy, ps[p].ang + ps[p].lookang, ps[p].mapzoom, 0);   \
    else {                                                                                   \
        if ((player_bits[p] & 0x80000) || (player_bits[p] & 0x100000) ||                     \
            (player_bits[p] & 0x400000))                                                     \
            permanentwritesprite(0, 0, 1170, 0, 0, 0, 319, 199);                             \
        animatewalls();                                                                      \
        animatecamsprite(p);                                                                 \
        if (userdefs.camera >= 0) {                                                          \
            short s = userdefs.camera;                                                       \
                                                                                             \
            if (sprite[s].yvel < 0)                                                          \
                sprite[s].yvel = 0;                                                          \
            else if (sprite[s].yvel > 199)                                                   \
                sprite[s].yvel = 199;                                                        \
            drawrooms(sprite[s].x, sprite[s].y, sprite[s].z, sprite[s].ang,                  \
                      sprite[s].yvel, sprite[s].sectnum);                                    \
            animatesprites(sprite[s].ang);                                                   \
        } else {                                                                             \
            drawrooms(ps[p].posx, ps[p].posy, ps[p].posz, ps[p].ang + ps[p].lookang,         \
                      ps[p].horiz, ps[p].cursectnum);                                        \
            animatesprites(ps[p].ang + ps[p].lookang);                                       \
        }                                                                                    \
        drawmasks();                                                                         \
        if ((gotpic[0x47] & 16) > 0 && waloff[572] != -1)                                    \
            draw_water_distortion();                                                         \
    }                                                                                        \
}
/* gametext's large-font loop, written out where screens draw fixed captions. */
#define BIG_TEXT(X, Y, TEXT, SHADE)                                                         \
{                                                                                           \
    long x = (X), y = (Y);                                                                  \
    unsigned char *text = (unsigned char *)(TEXT);                                          \
    unsigned char shade_ = (SHADE);                                                         \
    short left, tile, descender;                                                            \
                                                                                            \
    left = x;                                                                               \
    if (ps[screenpeek].display_mode != 2) {                                                 \
        while (*text) {                                                                     \
            descender = 2;                                                                  \
            if (*text == '\n') {                                                            \
                x = left;                                                                   \
                y += 16;                                                                    \
                text++;                                                                     \
                continue;                                                                   \
            }                                                                               \
            if (*text == ' ') {                                                             \
                x += 5;                                                                     \
                text++;                                                                     \
                continue;                                                                   \
            }                                                                               \
            if (*text >= '0' && *text <= '9')                                               \
                tile = *text + 101;                                                         \
            if (*text >= 'a' && *text <= 'z')                                               \
                tile = *text + 26;                                                          \
            if (*text >= 'A' && *text <= 'Z') {                                             \
                tile = *text + 32;                                                          \
                descender = 0;                                                              \
            }                                                                               \
            overwritesprite(x, y + descender, tile, shade_, 0, 0);                          \
            text++;                                                                         \
            x += tilesizx[tile] + 1;                                                        \
        }                                                                                   \
    }                                                                                       \
}

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
            qsetmode640480();
        vscrn(screenpeek);
        keystatus[0x25] = 0;
    }
    if (keystatus[0x11] && userdefs.debug && (ps[myconnectindex].gm & 16) == 0) {
        keystatus[0x11] = 0;
        while ((keystatus[0x1c] | keystatus[0x9c] | keystatus[0x39] | keystatus[1]) == 0) {
            DRAW_VIEW(screenpeek);
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
            BIG_TEXT(112, 50, "Warp Mode", 0);
            BIG_TEXT(100, 69, "Episode", 0);
            BIG_TEXT(100, 88, "Level", 0);
            ltoa(ps[0].volume + 1, tempbuf, 10);
            gametext(210, 67, (unsigned char *)tempbuf, 0);
            ltoa(ps[0].level + 1, tempbuf, 10);
            gametext(210, 86, (unsigned char *)tempbuf, 0);
            nextpage();
        }
        if (keystatus[1] == 0)
            enterlevel(4);
        keystatus[0x1c] = keystatus[0x9c] = keystatus[0x39] = keystatus[1] = 0;
    }
}

void show_help(char **argv)
{
    puts("\n *  Here's a little command line help...");
    printf("    %s [flags...]\n", argv[0]);
    puts("\t/?        This help message.\n");
    puts("\t/l#       Go to level #.");
    puts("\t/v#       Go to volume #.");
    puts("\t/s#       Skill # (0-3)");
    puts("\t/q        Turn on help quotes.");
    puts("\t/d        Activate DEBUG MODEs.");
    puts("\t/r        Record a demo.");
    puts("\t/oDIR     Reroute game output to DIR.");
    puts("\t/e        Encode maps...");
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
                printf("Unknown command line paramater '%s'\n", argv[i]);
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
                puts("Demorec mode on...");
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
                printf("I/O path rerouted to %s.\n", output_path);
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
                mapname = "L?.MAP";
                for (j = 0; j < 3; j++)
                    for (i = 0; i < 9; i++) {
                        mapname[0] = 'L' + j;
                        mapname[1] = '1' + i;
                        printf("Encoding %s\n", mapname);
                    }
                break;
            }
        } else {
            printf("Unknown command line paramater '%s'\n", c);
            show_help(argv);
            exit(0);
        }
        i++;
    }
    }
}

/* numtext_fixed's body, written out for the status bar. */
#define NUMTEXT_FIXED(X, Y, VALUE, SHADE)                                        \
{                                                                                \
    short i_, offset_;                                                           \
                                                                                 \
    ltoa(VALUE, tempbuf, 10);                                                    \
    i_ = strlen(tempbuf);                                                        \
    offset_ = 0;                                                                 \
    while (i_ != 0) {                                                            \
        overwritesprite((X) - offset_, Y, tempbuf[i_ - 1] + 0x536, SHADE, 0, 0); \
        offset_ += 6;                                                            \
        i_--;                                                                    \
    }                                                                            \
}
void wait_for_players(void)
{
    short other, packbufleng, numwanted;
    long i, j, k;

    if (option[4] >= 1 && option[4] <= 4)
        numwanted = 2;
    else
        numwanted = option[4] - 3;
    sendlogon();
    while (numplayers < numwanted && keystatus[1] == 0) {
        while ((packbufleng = getpacket(&other, (unsigned char *)tempbuf)) > 0) {
        switch (tempbuf[0]) {
        case 2:
            script_tics = tempbuf[1];
            j = ((numplayers + 1) >> 1) + 2;
            k = 16;
            for (i = connecthead; i >= 0; i = connectpoint2[i]) {
                if ((tempbuf[k >> 3] & (1 << (k & 7))) > 0)
                    player_svel[i] = tempbuf[j++];
                if ((tempbuf[k >> 3] & (2 << (k & 7))) > 0)
                    player_vel[i] = tempbuf[j++];
                if ((tempbuf[k >> 3] & (4 << (k & 7))) > 0)
                    player_avel[i] = tempbuf[j++];
                if ((tempbuf[k >> 3] & (8 << (k & 7))) > 0) {
                    player_bits[i] = tempbuf[j] + (tempbuf[j + 1] << 8) +
                                     (tempbuf[j + 2] << 16) + (tempbuf[j + 3] << 24);
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
                player_bits[other] |= ((long)tempbuf[j++] << 8);
            }
            if ((tempbuf[1] & 32) > 0) {
                player_bits[other] &= 0xff00ffff;
                player_bits[other] |= ((long)tempbuf[j++] << 16);
            }
            if ((tempbuf[1] & 64) > 0) {
                player_bits[other] &= 0x00ffffff;
                player_bits[other] |= ((long)tempbuf[j++] << 24);
            }
            break;
        case 4:
            input_text_length = 0;
            for (i = 1; i < packbufleng; i++)
                input_text[input_text_length++] = tempbuf[i];
            msgtext = input_text;
            msgtime = input_text_length << 4;
            input_text[0] = 0;
            break;
        case 5:
            playerreadyflag[other] = 1;
            break;
        case 255:
            ps[myconnectindex].gm = 128;
            break;
        }
        }
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
    puts("ÞßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßÝÞ DUKE DUKEM 3D Beta version 1.3.95 -- DO NOT DISTRIBUTE!!! That would be bad. ÝÞÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÝ");
    puts("Loading user stats...");
    read_userdefs();
    puts("Init BootLeg CacheMaster...");
    init_bootleg_cache();
    puts("Checking program integrety...");
    if (file_id_check()) {
        puts("No good.");
        unlink(argv[0]);
        return;
    }
    checkcommandline(argc, argv);
    loadefs();
    puts("Init engine...");
    initmenus();
    initengine();
    outp(0x43, 0x36);
    outp(0x40, 0xd6);
    outp(0x40, 0x26);
    oldtimerhandler = _dos_getvect(8);
    _disable();
    _dos_setvect(8, timerhandler);
    _enable();
    puts("Checking for multiplayer...");
    initmultiplayers(option[4], option[5]);
    puts("Loading art...");
    loadpics("tiles000.art");
    puts("Init colorize/remaps...");
    init_water_ripples();
    loadlookup();
    if (option[1]) {
        puts("Init sound FX...");
        status = FX_SetupCard(sound_device_map[option[1]], &fx_device);
        if (status)
            gameexit(FX_ErrorString(status));
        puts("Reading sounds...");
        loadsounds();
        status = FX_Init(sound_device_map[option[1]], 4, 2, 16, 11000);
        if (status)
            gameexit(FX_ErrorString(status));
        FX_SetCallBack(TestCallBack);
        FX_SetVolume(userdefs.sound_volume);
    }
    if (option[2]) {
        puts("Init music...");
        status = MUSIC_Init(sound_device_map[option[1]], 0);
        if (status)
            gameexit(MUSIC_ErrorString(status));
        MUSIC_SetVolume(userdefs.music_volume);
    }
    if (option[4]) {
        puts("Locating other players...");
        if (option[4] >= 1 && option[4] <= 4)
            p = 2;
        else
            p = option[4] - 3;
        sendlogon();
        while (numplayers < p && keystatus[1] == 0)
            domovethings_multiplayer();
        keystatus[1] = 0;
    }
    setgamemode();
    if (level_warp) {
        newgame(ps[0].volume, ps[0].level, ps[0].skill);
        enterlevel(4);
    }

attract:
    if (level_warp == 0) {
        which = 1;
nextdemo:
        read_demo(which++);
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
                    current_menu = 0;
                    ps[myconnectindex].gm |= 1;
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
                    goto play;
                if (ps[myconnectindex].gm & 4) {
                    if (ps[myconnectindex].gm & 2)
                        goto nextdemo;
                    demomode = 0;
                    goto play;
                }
            }
        }
        goto nextdemo;
    }
    level_warp = 0;

play:
    while ((ps[myconnectindex].gm & 128) == 0) {
        nonsharedkeys();
        DRAW_VIEW(screenpeek);
        if (ps[myconnectindex].gm & 16) {
            x = 10;
            y = 140;
            shade = next_text_shade();
            for (p = 0; p < 128; p++) {
                if (keystatus[p] > 0) {
                    if (keystatus[0x2a] | keystatus[0x36]) {
                        if (scan_ascii_shift[p]) {
                            if (input_text_length < 79)
                                input_text[input_text_length++] = scan_ascii_shift[p];
                            keystatus[p] = 0;
                        }
                    } else if (scan_ascii[p]) {
                        if (input_text_length < 159)
                            input_text[input_text_length++] = scan_ascii[p];
                        keystatus[p] = 0;
                    }
                    if (input_text_length > 79)
                        input_text_length = 79;
                    if (keystatus[0x0e] > 0) {
                        if (keystatus[0x0e] == 1 || keystatus[0x0e] == 60) {
                            if (input_text_length > 0)
                                input_text_length--;
                            else
                                input_text_length = 0;
                        }
                        input_text[input_text_length] = 0;
                        if (keystatus[0x0e] < 60) {
                            keystatus[0x0e] += script_tics;
                            if (keystatus[0x0e] > 60)
                                keystatus[0x0e] = 60;
                        }
                    }
                }
            }
            if (shade < 128) {
                x = minitext(x, y, input_text, shade & 127);
                draw_weapon_tile(x, y, 1023, shade, 0);
            } else {
                x = gametext(x, y, input_text, shade & 127);
                draw_weapon_tile(x, y, 1022, shade, 0);
            }
            if (keystatus[0x1c] | keystatus[0x9c]) {
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
                    BIG_TEXT(120, 48, "Master", 0);
                } else {
                    BIG_TEXT(130, 48, "Slave", 0);
                }
            }
            if (ps[screenpeek].newowner >= 0) {
                shade = score_text_shade();
                draw_weapon_tile(10, 10, 275, shade, 2);
                draw_weapon_tile(274, 10, 276, shade, 2);
                draw_weapon_tile(10, 160, 277, shade, 2);
                draw_weapon_tile(274, 160, 278, shade, 2);
                if (totalclock & 16)
                    draw_weapon_tile(18, 20, 279, shade, 2);
            } else {
                displayweapon(screenpeek);
                p = screenpeek;
                STATUS_BAR(p);
                p = screenpeek;
                SHOW_QUOTES(p);
            }
            if (numplayers < 2) {
                getinput(0);
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
            } else if (myconnectindex == connecthead) {
                getinput(myconnectindex);
                domovethings_multiplayer();
                if (script_tics > 0 && getoutputcirclesize() < 16) {
                    tempbuf[0] = 2;
                    tempbuf[1] = script_tics;
                    j = ((numplayers + 1) >> 1) + 2;
                    for (k = 2; k < j; k++)
                        tempbuf[k] = 0;
                    k = 16;
                    for (i = connecthead; i >= 0; i = connectpoint2[i]) {
                        if (player_svel[i] != pending_svel[i]) {
                            tempbuf[j++] = player_svel[i];
                            pending_svel[i] = player_svel[i];
                            tempbuf[k >> 3] |= 1 << (k & 7);
                        }
                        if (player_vel[i] != pending_vel[i]) {
                            tempbuf[j++] = player_vel[i];
                            pending_vel[i] = player_vel[i];
                            tempbuf[k >> 3] |= 2 << (k & 7);
                        }
                        if (player_avel[i] != pending_avel[i]) {
                            tempbuf[j++] = player_avel[i];
                            pending_avel[i] = player_avel[i];
                            tempbuf[k >> 3] |= 4 << (k & 7);
                        }
                        if (player_bits[i] != pending_bits[i]) {
                            tempbuf[j++] = player_bits[i] & 255;
                            tempbuf[j++] = (player_bits[i] >> 8) & 255;
                            tempbuf[j++] = (player_bits[i] >> 16) & 255;
                            tempbuf[j++] = (player_bits[i] >> 24) & 255;
                            pending_bits[i] = player_bits[i];
                            tempbuf[k >> 3] |= 8 << (k & 7);
                        }
                        k += 4;
                    }
                    for (i = connecthead; i >= 0; i = connectpoint2[i])
                        if (i != myconnectindex)
                            sendpacket(i, (unsigned char *)tempbuf, j);
                    domovethings();
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
                        tempbuf[j++] = player_bits[16] & 255;
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
                DRAW_VIEW(screenpeek);
                displayhighscores();
            }
            memcpy(score_names[p], input_text, 31);
        }
        do {
            DRAW_VIEW(screenpeek);
            displayhighscores();
            nextpage();
        } while (keystatus[1] == 0 && keystatus[0x39] == 0 && keystatus[0x1c] == 0);
    }
    write_demo();
    write_userdefs();
    gameexit("Game Over, or is it?");
}

void displaystatus(short snum)
{
    short i;

    if (ps[snum].display_mode != 3)
        return;
    if ((sector[ps[snum].cursectnum].lotag & 0x7fff) == 15)
        WEAPON_TILE(160, 120, 1136, 0, 1);
    if (ps[snum].screen_size >= 0) {
        overwritesprite(0, 0, 1380, 0, 0, 0);
        overwritesprite(160, 189, 1381, 0, 1, 0);
        NUMTEXT_FIXED(82, 6, ps[snum].score, 0);
        NUMTEXT_FIXED(119, 186, ps[snum].health, 0);
        for (i = 0; i < 25; i++)
            if (i < ps[snum].health >> 2)
                overwritesprite(i * 3 + 30, 186, 1392, 0, 0, 0);
        switch (ps[snum].curr_weapon) {
        case 389:
        case 1230:
            NUMTEXT_FIXED(209, 186, ps[snum].batteryammo, 0);
            overwritesprite(160, 192, 1400, 0, 1, 0);
            for (i = 0; i < 25; i++)
                if (i < ps[snum].batteryammo >> 2)
                    overwritesprite(i * 3 + 216, 186, 1392, 0, 0, 0);
            break;
        case 191:
            NUMTEXT_FIXED(209, 186, ps[snum].ammo, 0);
            overwritesprite(160, 192, 1401, 0, 1, 0);
            for (i = 0; i < 25; i++)
                if (i < ps[snum].ammo >> 2)
                    overwritesprite(i * 3 + 216, 186, 1392, 0, 0, 0);
            break;
        case 721:
        case 1140:
            NUMTEXT_FIXED(209, 186, ps[snum].remote_ammo, 0);
            overwritesprite(160, 192, 1402, 0, 1, 0);
            for (i = 0; i < 25; i++)
                if (i < ps[snum].remote_ammo >> 2)
                    overwritesprite(i * 3 + 216, 186, 1392, 0, 0, 0);
            break;
        case 563:
            NUMTEXT_FIXED(209, 186, ps[snum].rocket_ammo, 0);
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
    short p, j, offset;
    unsigned char shade;

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
            i++;
        }
        while (totalclock <= lockclock) {
            DRAW_VIEW(screenpeek);
            if (ps[screenpeek].newowner >= 0) {
                shade = score_text_shade();
                draw_weapon_tile(10, 10, 275, shade, 2);
                draw_weapon_tile(274, 10, 276, shade, 2);
                draw_weapon_tile(10, 160, 277, shade, 2);
                draw_weapon_tile(274, 160, 278, shade, 2);
                if (totalclock & 16)
                    draw_weapon_tile(18, 20, 279, shade, 2);
            } else if (userdefs.camera == -1) {
                displayweapon(screenpeek);
                p = screenpeek;
                STATUS_BAR(p);
                p = screenpeek;
                SHOW_QUOTES(p);
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
#define MOVE_STATUS9()                                                                      \
{                                                                                           \
    i = headspritestat[9];                                                                  \
    while (i >= 0) {                                                                        \
        sect = sprite[i].sectnum;                                                           \
        if (sprite[i].z > sector[sect].floorz)                                              \
            sprite[i].z = sector[sect].floorz;                                              \
        if (sector[sect].lotag == 0 && sector[sect].floorz == sprite[i].z) {                \
            j = headspritesect[sect];                                                       \
            while (j >= 0) {                                                                \
                if (hittype[i][1]) {                                                        \
                    hittype[i][1]--;                                                        \
                    break;                                                                  \
                }                                                                           \
                if (sprite[j].statnum != 3 && sprite[j].statnum != 9 &&                     \
                    sprite[j].picnum != 215 && sprite[j].picnum != 283) {                   \
                    k = headspritestat[9];                                                  \
                    while (k >= 0) {                                                        \
                        if (i != k && sprite[i].hitag == sprite[k].hitag) {                 \
                            sprite[j].x = sprite[k].x;                                      \
                            sprite[j].y = sprite[k].y;                                      \
                            sprite[j].z = sprite[k].z;                                      \
                            setsprite(j, sprite[j].x, sprite[j].y, sprite[j].z);            \
                            hittype[k][1] = 10;                                             \
                            hittype[i][1] = 10;                                             \
                        }                                                                   \
                        k = nextspritestat[k];                                              \
                    }                                                                       \
                }                                                                           \
                j = nextspritesect[j];                                                      \
            }                                                                               \
        }                                                                                   \
                                                                                            \
        p = connecthead;                                                                    \
        while (p >= 0) {                                                                    \
            if (ps[p].cursectnum == sect) {                                                 \
                lotag = sector[sect].lotag & 0x7fff;                                        \
                if (lotag == 0) {                                                           \
                    if (klabs(sprite[i].z - ps[p].posz) > 4096 &&                           \
                        sprite[i].z != sector[sect].floorz)                                 \
                        goto nextplayer;                                                    \
                } else if (lotag == 2) {                                                    \
                    if (sector[sect].ceilingz + 0x500 < ps[p].posz)                         \
                        goto nextplayer;                                                    \
                } else if (lotag == 1) {                                                    \
                    if (sector[sect].floorz - 0x500 > ps[p].posz)                           \
                        goto nextplayer;                                                    \
                }                                                                           \
                if (hittype[i][0] == 0) {                                                   \
                m = headspritestat[9];                                                      \
                while (m >= 0) {                                                            \
                    if (m != i && sprite[i].hitag == sprite[m].hitag) {                     \
                        otherlotag = lotag;                                                 \
                        lotag = sector[sprite[m].sectnum].lotag;                            \
                        if (!(otherlotag == 1 && lotag == 0) &&                             \
                            !(otherlotag == 1 && lotag == 1) &&                             \
                            !(otherlotag == 2 && lotag == 2)) {                             \
                            if (lotag == 0) {                                               \
                                ps[p].posx = sprite[m].x;                                   \
                                ps[p].posy = sprite[m].y;                                   \
                                if (sprite[m].z != sprite[i].z)                             \
                                    ps[p].posz = sprite[m].z;                               \
                                else                                                        \
                                    ps[p].posz = sector[sprite[m].sectnum].floorz - 0x2400; \
                                hittype[m][0] = 20;                                         \
                                hittype[i][0] = 20;                                         \
                            }                                                               \
                            if (lotag == 1) {                                               \
                                if (ps[p].on_ground)                                        \
                                    spritesound(28, ps[p].i);                               \
                                ps[p].posx += sprite[m].x - sprite[i].x;                    \
                                ps[p].posy += sprite[m].y - sprite[i].y;                    \
                                ps[p].posz = sprite[m].z - 0x600;                           \
                                hittype[m][0] = 1;                                          \
                                hittype[i][0] = 1;                                          \
                            }                                                               \
                            if (lotag == 2) {                                               \
                                spritesound(10, ps[p].i);                                   \
                                ps[p].posx += sprite[m].x - sprite[i].x;                    \
                                ps[p].posy += sprite[m].y - sprite[i].y;                    \
                                ps[p].posz = sector[sprite[m].sectnum].ceilingz + 0x600;    \
                                hittype[m][0] = 1;                                          \
                                hittype[i][0] = 1;                                          \
                            }                                                               \
                            ps[p].cursectnum = sprite[m].sectnum;                           \
                            if (sector[ps[p].cursectnum].lotag & 0x7fff) {                  \
                                for (k = 0; k < 20; k++)                                    \
                                    spawn(ps[p].i, 215);                                    \
                                spawn(ps[p].i, 1190);                                       \
                            }                                                               \
                            break;                                                          \
                        }                                                                   \
                    }                                                                       \
                    m = nextspritestat[m];                                                  \
                }                                                                           \
                } else                                                                      \
                    hittype[i][0]--;                                                        \
            }                                                                               \
    nextplayer:                                                                             \
            p = connectpoint2[p];                                                           \
        }                                                                                   \
        i = nextspritestat[i];                                                              \
    }                                                                                       \
}


void domovethings(void)
{
    short i, j, m, sect;
    long k, p;
    long lotag, otherlotag;

    randomstash = krand();
    userdefs.camera = -1;
    lockclock += script_tics;
    doanimations(script_tics);
    if (userdefs.paused == 0) {
        if (demomode == 1) {
demo_svel[reccnt] = player_svel[0];
            demo_vel[reccnt] = player_vel[0];
            demo_avel[reccnt] = player_avel[0];
            demo_bits[reccnt] = player_bits[0];
            demo_tics[reccnt] = script_tics;
            if (++reccnt > 16383)
                reccnt = 16383;
        }
        moveeffects();
        MOVE_STATUS9();
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
        BIG_TEXT(36, 160, "Press SPACE to continue", shade);
        for (i = 0; i < ps[0].level; i++)
            overwritesprite(bonus_marks[ps[0].volume][i * 2], bonus_marks[ps[0].volume][i * 2 + 1],
                            1133, shade, 0, 0);
        overwritesprite(bonus_marks[ps[0].volume][i * 2], bonus_marks[ps[0].volume][i * 2 + 1],
                        1134, shade, 0, 0);
        if (ps[myconnectindex].gm == 8) {
            BIG_TEXT(32, 10, "BONUS", 0);
            if (start + 240 < totalclock) {
                if (ps[0].max_secret_rooms)
                    pct = (ps[0].secret_rooms * 100) / ps[0].max_secret_rooms;
                SMALL_TEXT(60, 50, "Secret rooms Found", shade);
                ltoa((short)pct, tempbuf, 10);
                minitext(60, 60, (unsigned char *)tempbuf, shade);
                SMALL_TEXT(72, 60, "%, =", shade);
                ltoa((short)pct * 10000, tempbuf, 10);
                minitext(100, 60, (unsigned char *)tempbuf, shade);
                SMALL_TEXT(140, 60, "points.", 0);
                ps[screenpeek].score += (short)pct * 10000;
            }
            if (start + 480 < totalclock) {
                if (ps[0].max_squashed)
                    pct = (ps[0].actors_squashed * 100) / ps[0].max_squashed;
                SMALL_TEXT(60, 80, "Bad guys squashed", shade);
                ltoa((short)pct, tempbuf, 10);
                minitext(60, 90, (unsigned char *)tempbuf, shade);
                SMALL_TEXT(72, 90, "%, =", shade);
                ltoa((short)pct * 1000, tempbuf, 10);
                minitext(100, 90, (unsigned char *)tempbuf, shade);
                SMALL_TEXT(140, 90, "points.", shade);
                ps[screenpeek].score += (short)pct * 1000;
            }
            if (start + 720 < totalclock) {
                if (ps[0].max_actors_killed)
                    pct = (ps[0].actors_killed * 100) / ps[0].max_actors_killed;
                SMALL_TEXT(60, 110, "Bad guys killed", shade);
                ltoa((short)pct, tempbuf, 10);
                minitext(60, 120, (unsigned char *)tempbuf, shade);
                SMALL_TEXT(72, 120, "%, =", shade);
                ltoa((short)pct * 1000, tempbuf, 10);
                minitext(100, 120, (unsigned char *)tempbuf, shade);
                SMALL_TEXT(140, 120, "points.", shade);
                ps[screenpeek].score += (short)pct * 1000;
            }
        }
        nextpage();
    }
}

void draw_monitor_overlay(void)
{
    short shade = (unsigned char)next_text_shade();

    WEAPON_TILE(10, 10, 275, shade, 2);
    WEAPON_TILE(274, 10, 276, shade, 2);
    WEAPON_TILE(10, 160, 277, shade, 2);
    WEAPON_TILE(274, 160, 278, shade, 2);
    if (totalclock & 16)
        WEAPON_TILE(18, 20, 279, shade, 2);
}

void update_menu_cursor(short tile)
{
    short dx, dy, draw_x, draw_y;
    short phase = hudpulse + 128;
    unsigned char shade = (sintable[phase & 2047] >> 10) + 16;
    unsigned char palette;
    long scale;

    hudpulse = phase;
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

    draw_y = menu_y;
    draw_x = menu_x;
    if (ps[screenpeek].display_mode == 3) {
        palette = sector[ps[screenpeek].cursectnum].floorpal;
        if (palette)
            overwritesprite(draw_x, draw_y, tile, shade, 1, palette);
        else
            overwritesprite(draw_x, draw_y, tile, shade, 1, ps[screenpeek].palette);
    }
}

void displayhighscores(void)
{
    short i;
    short shade = (unsigned char)next_text_shade();

    SMALL_TEXT(30, 36, "#", 0);
    SMALL_TEXT(50, 36, "Hero's Quote", 0);
    SMALL_TEXT(270, 36, "Score", 0);
    for (i = 0; i < 12; i++) {
        if (i)
            shade = 0;
        ltoa(i, tempbuf, 10);
        minitext(30, i * 11 + 50, tempbuf, 0);
        SMALL_TEXT(50, i * 11 + 50, score_names[i], shade);
        {
            long y = i * 11 + 50;
            unsigned char s = shade;

            ltoa(high_scores[i], tempbuf, 10);
            minitext(270, y, tempbuf, s);
        }
    }
}



void spawn_random_effects(short i, short count)
{
    short j, k, sect, ang, zvel, owner;
    long x, y, z;

    for (j = 0; j < count; j++) {
        z = sprite[i].z - (krand() & 4095);
        if (z < sector[sprite[i].sectnum].ceilingz)
            z = sprite[i].z;
        sect = sprite[i].sectnum;
        x = sprite[i].x;
        y = sprite[i].y;
        ang = sprite[i].ang - 256 + (krand() & 511);
        zvel = 1024 - (krand() & 2047);
        owner = sprite[i].owner;
        k = insertsprite(sect, 5);
        sprite[k].x = x;
        sprite[k].y = y;
        sprite[k].z = z;
        sprite[k].cstat = 0;
        sprite[k].picnum = 970 + (j & 3);
        sprite[k].shade = -32;
        sprite[k].xrepeat = 18;
        sprite[k].yrepeat = 18;
        sprite[k].ang = ang;
        sprite[k].xvel = 64;
        sprite[k].zvel = zvel;
        sprite[k].owner = owner;
        sprite[k].yvel = 0;
        hittype[k][0] = 0;
        hittype[k][2] = 0;
        hittype[k][3] = 0;
        if (actorscrptr[sprite[k].picnum]) {
            sprite[k].extra = *actorscrptr[sprite[k].picnum];
            hittype[k][4] = actorscrptr[sprite[k].picnum][1];
            hittype[k][1] = actorscrptr[sprite[k].picnum][2];
            if (sprite[k].picnum != 57)
                sprite[k].hitag = actorscrptr[sprite[k].picnum][3];
        } else {
            hittype[k][4] = 0;
            hittype[k][1] = 0;
        }
    }
}

void move_status9(void)
{
    short i, j, m, sect;
    long k, p;
    long lotag, otherlotag;

    MOVE_STATUS9();
}

void read_userdefs(void)
{
    int file;

    demomode = 0;
    file = open("userdefs.tmp", O_RDWR | O_BINARY, S_IREAD);
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
    file = open("userdefs.tmp", O_WRONLY | O_CREAT | O_TRUNC | O_TEXT | O_BINARY, 0xb0);
    write(file, &userdefs, sizeof(userdefs));
    write(file, score_names, sizeof(score_names));
    write(file, high_scores, sizeof(high_scores));
    close(file);
}

void saveplayer(char slot)
{
    int file;
    char *gamename = "SAVE___0.SAV", *boardname = "SAVE_000.SAV";

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
    char *gamename = "SAVE___0.SAV", *boardname = "SAVE_000.SAV";

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
        gameexit("Board not found.");
    ps[myconnectindex].gm = 4;
}

void read_demo(unsigned char slot)
{
    int file;
    char *filename = "mydemo__.dmo";

    filename[7] = '0' + slot;
    file = open(filename, O_RDWR | O_BINARY, S_IREAD);
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
        gameexit("Demo not found.");
    }
}

void write_demo(void)
{
    int file;
    char *filename = "mydemo__.dmo";

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
                    FTA_INLINE(7)
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
                {
                    short q = 8;

                    if (userdefs.quotes == 1 && ps[screenpeek].quote_time[q] == 0) {
                        ps[screenpeek].quote_time[q] = strlen(quote_text[q]) << 2;
                        ps[screenpeek].quote_index = q;
                    }
                }
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
                for (k = 0; k < 15; k++) {
                    x = sprite[i].z - (krand() & 4095);
                    if (x < sector[sprite[i].sectnum].ceilingz)
                        x = sprite[i].z;
                    EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, x, 0, 970 + (k & 3), -32, 18, 18,
                        sprite[i].ang - 256 + (krand() & 511), 64, 1024 - (krand() & 2047), sprite[i].owner, 5);
                }
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
void spawn(long j, short pn)
{
    short i, sect, k, wall_end;
    long *actor;

    if (j >= 0) {
        i = EGS(sprite[j].sectnum, sprite[j].x, sprite[j].y, sprite[j].z,
                0, pn, 0, 0, 0, 0, 0, 0, sprite[j].picnum, 0);
    } else {
        i = pn;
        k = sprite[i].picnum;
        hittype[i][3] = 0;
        hittype[i][2] = 0;
        hittype[i][0] = 0;
        if (k >= 265 && k <= 269)
            k = 265;
        actor = actorscrptr[k];
        if (actor != NULL) {
            sprite[i].extra = (short)actor[0];
            hittype[i][4] = actor[1];
            hittype[i][1] = actor[2];
            if (k != 57)
                sprite[i].hitag = (short)actor[3];
        } else {
            hittype[i][4] = 0;
            hittype[i][1] = 0;
        }
    }
    sect = sprite[i].sectnum;
    switch (sprite[i].picnum) {
    case 1408:
        sprite[i].owner = -1;
        changespritestat(i, 1);
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
            changespritestat(i, 5);
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
            changespritestat(i, 4);
        }
        break;
    case 5:
        sprite[i].xrepeat = sprite[i].yrepeat = 0;
        changespritestat(i, 11);
        break;
    case 1404:
        if (j >= 0) {
            sprite[i].owner = j;
            sprite[i].xrepeat = (unsigned char)(tilesizx[sprite[j].picnum] / 3);
            sprite[i].yrepeat = (unsigned char)(tilesizy[sprite[j].picnum] / 3);
            sprite[i].shade = -127;
        }
        changespritestat(i, 5);
        break;
    case 1359:
        sprite[i].xrepeat = sprite[i].yrepeat = 32;
        sprite[i].cstat |= 0x22;
        sprite[i].ang = (short)(krand() & 2047);
        sprite[i].shade = 0;
        sprite[i].z = sector[sect].floorz;
        changespritestat(i, 5);
        break;
    case 1360:
        if (j >= 0 && sprite[j].statnum == 10)
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
        changespritestat(i, 5);
        break;
    case 283:
        sprite[i].xrepeat = sprite[i].yrepeat = 0;
        sprite[i].shade = -32;
        changespritestat(i, 10);
        break;
    case 215:
        sprite[i].xrepeat = sprite[i].yrepeat = 10;
        sprite[i].shade = -16;
        changespritestat(i, 5);
        break;
    case 1280:
        sprite[i].xrepeat = sprite[i].yrepeat = 64;
        sprite[i].shade = -127;
        sprite[i].cstat |= 1;
        changespritestat(i, 2);
        break;
    case 1175: {
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].cstat |= 0x40;
        sprite[i].picnum += 2;
        sprite[i].z = sector[sect].ceilingz + 12288;
        hittype[i][4] = mspos;
        msx[mspos] = sprite[i].x;
        msy[mspos] = sprite[i].y;
        msx[mspos + 2] = sprite[i].z;
        for (k = headspritestat[0]; k >= 0; k = nextspritestat[k]) {
            if (sprite[k].picnum == 1174 && sprite[k].hitag == sprite[i].hitag) {
                hittype[i][1] = sprite[k].sectnum;
                sprite[k].xrepeat = 48;
                sprite[k].yrepeat = 128;
                msx[mspos + 1] = sprite[k].x;
                msy[mspos + 1] = sprite[k].y;
                sprite[k].x = sprite[i].x;
                sprite[k].y = sprite[i].y;
                sprite[k].z = sprite[i].z;
                sprite[k].shade = sprite[i].shade;
                msy[mspos + 2] = k;
                setsprite(k, sprite[k].x, sprite[k].y, sprite[k].z);
                break;
            }
        }
        sprite[i].owner = -1;
        mspos += 3;
        changespritestat(i, 6);
        break;
    }
    case 1190:
        sprite[i].xrepeat = sprite[i].yrepeat = 20;
        sprite[i].shade = -16;
        changespritestat(i, 5);
        break;
    case 960:
        sprite[i].xrepeat = 16;
        sprite[i].yrepeat = 0;
        hittype[i][1] = krand() & 0x1F;
        sprite[i].z = sector[sect].ceilingz + 2048;
        changespritestat(i, 2);
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
        changespritestat(i, 2);
        break;
    case 305:
        hittype[i][0] = sprite[i].xrepeat;
        hittype[i][1] = sprite[i].yrepeat;
        /* fall through */
    case 32: case 51: case 216:
        sprite[i].xrepeat = sprite[i].yrepeat = 0;
        sprite[i].yvel = 0;
        changespritestat(i, 1);
        break;
    case 408: case 490: case 579: case 1536: case 1680: case 1820:
        ps[0].max_squashed++;
        ps[0].max_actors_killed++;
        /* fall through */
    case 631:
        sprite[i].shade = -24;
        sprite[i].yrepeat = 43;
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].xrepeat = 52;
        if (sprite[i].picnum == 408)
            sprite[i].xrepeat -= 9;
        changespritestat(i, 2);
        sprite[i].cstat |= 1;
        break;
    case 34:
        changespritestat(i, 7);
        sprite[i].xrepeat = sprite[i].yrepeat = 0;
        break;
    case 23: case 31:
        sprite[i].xrepeat = sprite[i].yrepeat = 0;
        if (sprite[i].picnum == 23)
            sector[sect].lotag ^= 0x4000;
        changespritestat(i, 8);
        break;
    case 30: case 313: case 314: case 1123:
        sprite[i].cstat |= 1;
        /* fall through */
    case 1122:
        sprite[i].shade = -12;
        show2dsprite[i >> 3] |= 1 << (i & 7);
        changespritestat(i, 1);
        break;
    case 38: case 60: case 70: case 80: case 159: case 263: case 315: case 316: case 381: case 620:
    case 677: case 720: case 721: case 722: case 723: case 724: case 726: case 727: case 728:
    case 1119: case 1120: case 1121: case 1125:
        sprite[i].shade = -17;
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].cstat |= 1;
        changespritestat(i, 2);
        break;
    case 931:
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].xrepeat = sprite[i].yrepeat = 32;
        hittype[i][3] = sector[sect].ceilingz;
        sprite[i].z = hittype[i][3] + 10752;
        sprite[i].cstat |= 9;
        ps[0].max_actors_killed++;
        changespritestat(i, 2);
        break;
    case 930:
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].xrepeat = sprite[i].yrepeat = 32;
        hittype[i][3] = sector[sect].floorz;
        sprite[i].z = hittype[i][3] + 3072;
        sprite[i].cstat |= 1;
        ps[0].max_actors_killed++;
        changespritestat(i, 2);
        break;
    case 940:
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].owner = i;
        sprite[i].xrepeat = sprite[i].yrepeat = 24;
        sprite[i].cstat |= 1;
        ps[0].max_actors_killed++;
        changespritestat(i, 4);
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
        changespritestat(i, 1);
        break;
    case 175: {
        sprite[i].yvel = sector[sect].extra;
        sprite[i].xrepeat = 0;
        sprite[i].yrepeat = 0;
        switch (sprite[i].lotag) {
        case 27:
            if (demomode == 1) {
                sprite[i].xrepeat = 64;
                sprite[i].yrepeat = 64;
            }
            break;
        case 12:
            hittype[i][1] = sector[sect].floorshade;
            hittype[i][2] = sector[sect].ceilingshade;
            break;
        case 13:
            hittype[i][0] = sector[sect].ceilingz;
            hittype[i][1] = sector[sect].floorz;
            sprite[i].owner = klabs(hittype[i][0] - sprite[i].z) < klabs(hittype[i][1] - sprite[i].z);
            if (sprite[i].ang == 512) {
                if (sprite[i].owner)
                    sector[sect].ceilingz = sprite[i].z;
                else
                    sector[sect].floorz = sprite[i].z;
            } else {
                sector[sect].floorz = sprite[i].z;
                sector[sect].ceilingz = sprite[i].z;
            }
            if (!(sector[sect].ceilingstat & 1))
                break;
            sector[sect].ceilingstat ^= 1;
            hittype[i][3] = 1;
            if (sprite[i].owner == 0 && sprite[i].ang == 512) {
                sector[sect].ceilingstat ^= 1;
                hittype[i][3] = 0;
            }
            sector[sect].ceilingshade = sector[sect].floorshade;
            if (sprite[i].ang == 512) {
                wall_end = sector[sect].wallptr + sector[sect].wallnum;
                for (k = sector[sect].wallptr; k < wall_end; k++) {
                    short ns = wall[k].nextsector;

                    if (ns >= 0 && !(sector[ns].ceilingstat & 1)) {
                        sector[sect].ceilingpicnum = sector[ns].ceilingpicnum;
                        sector[sect].ceilingshade = sector[ns].ceilingshade;
                        break;
                    }
                }
            }
            break;
        case 17:
            hittype[i][2] = sector[sect].floorz;
            hittype[i][3] = sector[nextsectorneighborz(sect, sector[sect].floorz, -1, -1)].ceilingz;
            hittype[i][4] = sector[nextsectorneighborz(sect, sector[sect].ceilingz, 1, 1)].floorz;
            break;
        case 24:
            sprite[i].yvel *= 2;
            sector[sect].floorstat |= 0x40;
            break;
        case 23:
            sprite[i].xrepeat = 64;
            sprite[i].yrepeat = 64;
            sprite[i].shade = 0;
            hittype[i][1] = mspos;
            for (k = 0; k < numsectors; k++) {
                if (sector[k].hitag == sprite[i].hitag) {
                    msx[mspos++] = k;
                    hittype[i][0]++;
                }
            }
            break;
        case 20: {
            long best = 0x7FFFFFFF, w0 = -1;

            wall_end = sector[sect].wallptr + sector[sect].wallnum;
            for (k = sector[sect].wallptr; k < wall_end; k++) {
                long d = ksqrt((sprite[i].y - wall[k].y) * (sprite[i].y - wall[k].y) + (sprite[i].x - wall[k].x) * (sprite[i].x - wall[k].x));

                if (d < best) {
                    best = d;
                    w0 = k;
                }
            }
            hittype[i][1] = w0;
            best = 0x7FFFFFFF;
            wall_end = sector[sect].wallptr + sector[sect].wallnum;
            for (k = sector[sect].wallptr; k < wall_end; k++) {
                long d = ksqrt((sprite[i].y - wall[k].y) * (sprite[i].y - wall[k].y) + (sprite[i].x - wall[k].x) * (sprite[i].x - wall[k].x));

                if (d < best && k != hittype[i][1]) {
                    best = d;
                    w0 = k;
                }
            }
            hittype[i][2] = w0;
            break;
        }
        case 3:
            hittype[i][3] = sector[sect].floorshade;
            sector[sect].floorshade = sprite[i].shade;
            sector[sect].ceilingshade = sprite[i].shade;
            sprite[i].owner = (short)(sector[sect].ceilingpal << 8);
            sprite[i].owner |= sector[sect].floorpal;
            wall_end = sector[sect].wallptr + sector[sect].wallnum;
            for (k = sector[sect].wallptr; k < wall_end; k++)
                if (!(wall[k].hitag & 1))
                    wall[k].shade = sprite[i].shade;
            break;
        case 4:
            hittype[i][2] = sector[sect].floorshade;
            sprite[i].owner = (short)(sector[sect].ceilingpal << 8);
            sprite[i].owner |= sector[sect].floorpal;
            wall_end = sector[sect].wallptr + sector[sect].wallnum;
            for (k = sector[sect].wallptr; k < wall_end; k++)
                if (wall[k].shade > hittype[i][3])
                    hittype[i][3] = wall[k].shade;
            break;
        case 9:
            if ((sector[sect].lotag & 0x7FFF) && klabs(sector[sect].ceilingz - sprite[i].z) > 1024)
                sector[sect].lotag |= 32768U;
            /* fall through */
        case 8:
            hittype[i][0] = sector[sect].floorshade;
            hittype[i][1] = sector[sect].ceilingshade;
            wall_end = sector[sect].wallptr + sector[sect].wallnum;
            for (k = sector[sect].wallptr; k < wall_end; k++)
                if (wall[k].shade > hittype[i][2])
                    hittype[i][2] = wall[k].shade;
            hittype[i][3] = (sprite[i].yvel >> 8) + 2;
            if (hittype[i][3] < 8)
                hittype[i][3] = 1;
            hittype[i][3] = 8; /* Final shade step in the executable. */
            break;
        case 11:
            hittype[i][3] = sprite[i].ang <= 1024 ? -2 : 2;
            /* fall through */
        case 0: case 2: case 5: case 6: case 14: case 15: case 16: case 25: case 26: {
            short s;

            hittype[i][1] = mspos;
            wall_end = sector[sect].wallptr + sector[sect].wallnum;
            for (k = sector[sect].wallptr; k < wall_end; k++) {
                msx[mspos] = wall[k].x - sprite[i].x;
                msy[mspos] = wall[k].y - sprite[i].y;
                mspos++;
            }
            if (sprite[i].lotag == 6 || sprite[i].lotag == 14 || sprite[i].lotag == 5) {
                if (sector[sprite[i].sectnum].hitag != -1) {
                    sector[sect].floorstat |= 0x40;
                    sector[sect].ceilingstat |= 0x40;
                }
                /* The exe passes the wall loop's end index as the first sector hint. */
                s = k;
                for (k = 0; k < 10; k++) {
                    updatesector(k * (sintable[(sprite[i].ang + 512) & 2047] >> 5) + sprite[i].x,
                        sprite[i].y + k * (sintable[sprite[i].ang & 2047] >> 5), &s);
                    if (sector[s].hitag == 0)
                        break;
                    if (sector[s].hitag != -1) {
                        sector[s].ceilingstat |= 0x40;
                        sector[s].floorstat |= 0x40;
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
                sprite[i].zvel = sprite[i].shade == sector[sect].floorshade ? -256 : 256;
                sector[sect].floorstat |= 0x40;
                sprite[i].shade = 0;
            }
            break;
        }
        }
        changespritestat(i, sprite[i].lotag == 7 ? 9 : 3);
        break;
    }
    case 57:
        sprite[i].owner = i;
        sprite[i].shade = -16;
        if (sprite[i].xrepeat <= 8)
            sprite[i].xrepeat = sprite[i].yrepeat = 0;
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].cstat |= 1;
        changespritestat(i, 6);
        break;
    case 606: case 627: case 905: case 1200:
        sprite[i].owner = i;
        sprite[i].shade = -16;
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].yrepeat = sprite[i].xrepeat = 38;
        sprite[i].cstat |= 1;
        changespritestat(i, 6);
        break;
    }
}
