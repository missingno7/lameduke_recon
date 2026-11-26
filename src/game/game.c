#include <dos.h>
#include <fcntl.h>
#include <i86.h>
#include <io.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "build_v5.h"
#include "game.h"

extern void __interrupt __far timerhandler(void);
extern unsigned outp(unsigned port, unsigned value);
extern void resetparallax(void);
extern void gameexit(char *message);
extern void makepalookup(long palnum, char *remap, signed char red,
                        signed char green, signed char blue);
extern void bootleg_cache_pop(void);
extern void newgame(unsigned char episode, unsigned char level, unsigned char skill);
extern void overwritesprite(long x, long y, long tile, long shade,
                            long orientation, long palette);
extern void getmousevalues(short *dx, short *dy, short *buttons);

/* US PC set-1 printable keys, with zero entries for non-text keys. */
unsigned char scan_ascii[128] =
    "\0\0" "1234567890-=" "\0\0"                 /* 00-0f */
    "qwertyuiop[]" "\0\0" "as"                    /* 10-1f */
    "dfghjkl;'\x60" "\0" "\\zxcv"                  /* 20-2f */
    "bnm,./\0*\0 " "\0\0\0\0\0\0"                  /* 30-3f */
    "\0\0\0\0\0\0\0" "789-456+1"                   /* 40-4f */
    "230.";                                       /* 50-53 */

unsigned char scan_ascii_shift[128] =
    "\0\0" "!@#$%^&*()_+" "\0\0"                 /* 00-0f */
    "QWERTYUIOP{}" "\0\0" "AS"                    /* 10-1f */
    "DFGHJKL:\"~" "\0" "|ZXCV"                     /* 20-2f */
    "BNM<>?\0*\0 " "\0\0\0\0\0\0"                  /* 30-3f */
    "\0\0\0\0\0\0\0" "789-456+1"                   /* 40-4f */
    "230.";                                       /* 50-53 */

void (__interrupt __far *oldtimerhandler)(void);
extern short hudpulse;
extern short sintable[2048];
extern unsigned char keys[17], keystatus[256];
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
extern const char lookup_filename[];
extern char lookup_missing[];

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
extern const char userdefs_filename[];

/* GAME uses the packed, 176-byte player record. */
#pragma pack(1)
struct player_view {
    short sprite_index;
    long viewmode, mapzoom;
    long posx, posy, posz, horiz;
    long previous_x, previous_y, previous_z, poszv;
    long oposx, oposy, oposz;
    unsigned long score;
    long bobcounter;
    short ang, oang, previous_ang, cursectnum, ocursectnum, lookang;
    short screen_size, ammo, batteryammo, remote_ammo, rocket_ammo, health;
    short newowner, holoduke, jumping_counter, airleft, last_weapon;
    short quote_time[13], quote_index, weapon_frame, curr_weapon;
    short w80, w82, parallax_sector;
    char *music;
    unsigned char gm, b8b, skill, level, episode, on_ground;
    unsigned char b90, b91, b92, b93, b94, b95, b96, b97, b98, b99, b9a, b9b;
    unsigned char b9c, b9d, b9e, b9f, ba0, ba1, ba2, ba3, ba4, ba5, ba6;
    unsigned char palette, ba8, ba9, baa, bab, bac, bad, bae, baf;
};
#pragma pack()

typedef char player_size_check[sizeof(struct player_view) == 176 ? 1 : -1];

extern struct player_view ps[17];
extern char demo_filename[], demo_missing[];

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

extern short EGS(short sect, long x, long y, long z, unsigned char cstat,
    short tile, signed char shade, unsigned char xrepeat, unsigned char yrepeat,
    short ang, short xvel, short zvel, short owner, short statnum);
extern long krand(void);
extern long ksqrt(long square);
extern long getangle(long x, long y);
extern long getincangle_shift(short angle, short target, short shift);
extern long setsprite(short i, long x, long y, long z);
extern int deletesprite(short i);
extern int changespritestat(short i, short stat);
extern short insertsprite(short sect, short stat);
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
extern void drawrooms(long x, long y, long z, short ang, long horizon, short sect);
extern void animatesprites(short ang);
extern void drawmasks(void);
extern void rotatesprite(long x, long y, long zoom, short ang, short tile);
extern void vscrn(short p);
extern void setgamemode(void);
extern void checkhitswitch(short w);
extern void loadtile(short tile);
extern void nextpage(void);
extern void FX_StopAllSounds(void);
extern short loadboard(char *name, long *x, long *y, long *z, short *ang, short *sect);
extern void precache(void);
extern void cacheit(void);
extern void allignwarpelevators(void);
extern void moveloop(void);
extern void playmusic(char *name);
extern void displaystatus(short p);
extern void dobonus(void);
extern void call_level_load_hook(char *name);
#pragma aux call_level_load_hook "level_load_hook_"
extern short disable_armor;
extern unsigned char level_tick_state;
extern signed char pending_svel[17], pending_vel[17], pending_avel[17];
extern long pending_bits[17];
extern char quote_text[13][32];
struct game_sound_voice {
    long x, y, z;
    short sprite;
    long handle;
    unsigned char active;
};
extern struct game_sound_voice sound_voices[39];

/* Inline Build helpers from the historical PRAGMAS.H. */
extern long ksgn(long value);
#pragma aux ksgn = "add ebx,ebx" "sbb eax,eax" "cmp eax,ebx" "adc al,0" \
    parm nomemory [ebx] modify exact [eax ebx];
extern long klabs(long value);
#pragma aux klabs = "test eax,eax" "jns skipnegate" "neg eax" "skipnegate:" \
    parm nomemory [eax];
extern void copybuf(void *source, void *dest, long count);
#pragma aux copybuf = "rep movsd" parm [esi] [edi] [ecx] modify exact [ecx esi edi];

#define FTA_INLINE(quote) \
    if (userdefs.quotes == 1 && ps[screenpeek].quote_time[quote] == 0) { \
        ps[screenpeek].quote_time[quote] = (short)(strlen(quote_text[quote]) << 2); \
        ps[screenpeek].quote_index = (quote); \
    }

struct animwall_entry {
    short wallnum;
    long tag;
};
extern struct animwall_entry animwall[256];

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
    ((keystatus[keys[binding]] | keystatus[scan] | keystatus[(scan) + 128]) != 0)

void update_timer_input(void)
{
    long forward = forward_velocity;
    long turn = turn_velocity;
    long strafe = strafe_velocity;

    if (!keystatus[keys[5]]) {
        if (held(2, 0x4b)) {
            turn -= 16;
            if (turn < -128)
                turn = -128;
        }
        if (held(3, 0x4d)) {
            turn += 16;
            if (turn > 127)
                turn = 127;
        }
    } else {
        if (held(2, 0x4b)) {
            strafe += 8;
            if (strafe > 127)
                strafe = 127;
        }
        if (held(3, 0x4d)) {
            strafe -= 8;
            if (strafe < -128)
                strafe = -128;
        }
    }
    if (held(0, 0x48)) {
        forward += 8;
        if (forward > 127)
            forward = 127;
    }
    if (held(1, 0x50)) {
        forward -= 8;
        if (forward < -128)
            forward = -128;
    }

    if (turn < 0) {
        turn += 10;
        if (turn > 0)
            turn = 0;
    } else if (turn > 0) {
        turn -= 10;
        if (turn < 0)
            turn = 0;
    }
    if (strafe < 0) {
        strafe += 2;
        if (strafe > 0)
            strafe = 0;
    } else if (strafe > 0) {
        strafe -= 2;
        if (strafe < 0)
            strafe = 0;
    }
    if (forward < 0) {
        forward += 2;
        if (forward > 0)
            forward = 0;
    } else if (forward > 0) {
        forward -= 2;
        if (forward < 0)
            forward = 0;
    }
    strafe_velocity = strafe;
    turn_velocity = turn;
    forward_velocity = forward;
}

#undef held

void level_load_hook(void)
{
}

long next_text_shade(void)
{
    short phase = hudpulse + 128;
    long shade = sintable[phase & 2047] >> 10;

    hudpulse = phase;
    return shade + 16;
}

void draw_weapon_tile(short x, short y, short tile, unsigned char shade,
                      unsigned char flags)
{
    unsigned char palette;

    if (ps[screenpeek].viewmode == 3) {
        palette = sector[ps[screenpeek].cursectnum].floorpal;
        if (palette)
            overwritesprite(x, y, tile, shade, flags, palette);
        else
            overwritesprite(x, y, tile, shade, flags, ps[screenpeek].palette);
    }
}

long gametext(long x, long y, unsigned char *text, unsigned char shade)
{
    if (ps[screenpeek].viewmode == 2)
        return 0;

/* Large glyph rendering shared with the decimal printer. */
{
    long left = x;
    short tile = -1;
    short descender;

    while (*text) {
        descender = 2;
        if (*text == '\n') {
            x = (short)left;
            y += 16;
        } else if (*text == ' ') {
            x += 5;
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
                overwritesprite(x, y + descender, tile, shade, 0, 0);
                x += tilesizx[tile] + 1;
            }
        }
        text++;
    }
}
    return x;
}

long minitext(long x, long y, unsigned char *text, unsigned char shade)
{
    if (ps[screenpeek].viewmode == 2)
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

void numtext_relative(long x, long y, long value, unsigned char shade)
{
    unsigned char *text;

    ltoa(value, tempbuf, 10);
    text = (unsigned char *)tempbuf;
    y--;
    if (ps[screenpeek].viewmode != 2) {
/* Large glyph rendering shared with the decimal printer. */
{
    long left = x;
    short tile = -1;
    short descender;

    while (*text) {
        descender = 2;
        if (*text == '\n') {
            x = (short)left;
            y += 16;
        } else if (*text == ' ') {
            x += 5;
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
                overwritesprite(x, y + descender, tile, shade, 0, 0);
                x += tilesizx[tile] + 1;
            }
        }
        text++;
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
        if (ps[screenpeek].viewmode == 2) {
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
        if (ps[screenpeek].viewmode == 3) {
            palette = sector[ps[screenpeek].cursectnum].floorpal;
            if (palette)
                overwritesprite((short)column, top, 1022, drawshade, 0, palette);
            else
                overwritesprite((short)column, top, 1022, drawshade, 0, ps[screenpeek].palette);
        }
    } else {
        if (ps[screenpeek].viewmode == 2) {
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
        if (ps[screenpeek].viewmode == 3) {
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

void numtext(long x, long y, long value, unsigned char shade)
{
    unsigned char *text;
    long row;

    ltoa(value, tempbuf, 10);
    text = (unsigned char *)tempbuf;
    row = y;
    if (ps[screenpeek].viewmode != 2) {
#define TEXT_ROW row
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
                shade = 96 + (*reflection & 31) - (depth >> 1);
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
    if (ps[screenpeek].viewmode == 3) {
        palette = sector[ps[screenpeek].cursectnum].floorpal;
        if (palette)
            overwritesprite(draw_x, draw_y, tile, shade, 1, palette);
        else
            overwritesprite(draw_x, draw_y, tile, shade, 1, ps[screenpeek].palette);
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

void read_demo(unsigned char slot)
{
    int file;

    demo_filename[7] = '0' + slot;
    file = open(demo_filename, O_RDWR | O_BINARY, S_IREAD);
    if (file != -1) {
        read(file, &ps[0].episode, sizeof(ps[0].episode));
        read(file, &ps[0].level, sizeof(ps[0].level));
        read(file, &ps[0].skill, sizeof(ps[0].skill));
        newgame(ps[0].episode, ps[0].level, ps[0].skill);
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
    filename[7] = '0' + (ps[0].level != 0 || ps[0].episode != 0);

    file = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_TEXT | O_BINARY, 0xb0);
    if (file != -1) {
        write(file, &ps[0].episode, sizeof(ps[0].episode));
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

/* The dispatch follows LameDuke's status-3 actor and sector conventions.
 * Raynorpat's GPLv2 reconstruction supplied the semantic starting point. */
void moveeffectors(void)
{
    long scratch, speed;
    short i, nexti;

    for (i = headspritestat[3]; i >= 0; i = nexti) {
        long *t = hittype[i], k, d;
        short sect = sprite[i].sectnum, j, p;
        long lotag = sprite[i].lotag;

        nexti = nextspritestat[i];
        switch (lotag) {
        case 0: {
            long dir = 0;

            for (j = headspritestat[3]; j >= 0; j = nextspritestat[j])
                if (sprite[i].hitag == sprite[j].hitag && (sprite[j].lotag == 1 || sprite[j].lotag == 5))
                    break;
            if (j >= 0) {
                speed = sector[sect].extra >> 3;
                scratch = dir = sprite[j].ang <= 1024 ? 1 : -1;
                if (t[3] == 0)
                    t[3] = klabs(sprite[i].x - sprite[j].x) + klabs(sprite[i].y - sprite[j].y);
                sprite[i].xvel = (short)t[3];
                sprite[i].x = sprite[j].x;
                sprite[i].y = sprite[j].y;
                sprite[i].ang += speed * dir;
                t[2] += speed * dir;
            }
            movesector(i);
            break;
        }
        case 2:
            if (t[4] > 0 && t[0] == 0) {
                if (sprite[i].hitag <= t[4]) {
                    t[0] = 1;
                    sprite[i].xvel = 16;
                } else
                    t[4]++;
            }
            if (t[0] <= 0)
                break;
            if (++t[0] > 24) {
                t[0] = -1;
                t[4] = -1;
                break;
            }
            sprite[i].xvel++;
            for (p = connecthead; p >= 0; p = connectpoint2[p])
                if (ps[p].cursectnum == sect && ps[p].on_ground) {
                    ps[p].posx += (sprite[i].xvel * sintable[(sprite[i].ang + 512) & 2047]) >> 14;
                    ps[p].posy += (sprite[i].xvel * sintable[sprite[i].ang & 2047]) >> 14;
                }
            for (j = headspritesect[sect]; j >= 0; j = nextspritesect[j])
                if (sprite[j].picnum != 175) {
                    sprite[j].x += (sprite[i].xvel * sintable[(sprite[i].ang + 512) & 2047]) >> 14;
                    sprite[j].y += (sintable[sprite[i].ang & 2047] * sprite[i].xvel) >> 14;
                    setsprite(j, sprite[j].x, sprite[j].y, sprite[j].z);
                }
            movesector(i);
            break;
        case 3:
            if (t[4] == 0)
                break;
            k = (randomstash / (sprite[i].hitag + 1)) & 1;
            if (k) {
                sector[sect].ceilingpal = (unsigned short)sprite[i].owner >> 8;
                sector[sect].floorpal = (unsigned char)sprite[i].owner;
            } else
                sector[sect].ceilingpal = sector[sect].floorpal = 0;
            t[0] = k ? sprite[i].shade : t[3];
            sector[sect].ceilingshade = sector[sect].floorshade = (signed char)t[0];
            for (k = sector[sect].wallptr; k < sector[sect].wallptr + sector[sect].wallnum; k++)
                if (!(wall[k].hitag & 1))
                    wall[k].shade = (signed char)t[0];
            break;
        case 4:
            k = (randomstash / (sprite[i].hitag + 1)) & 1;
            if (k) {
                t[1] = sprite[i].shade;
                t[0] = sprite[i].shade;
            } else {
                t[1] = t[2];
                t[0] = t[3];
            }
            if (k) {
                sector[sect].ceilingpal = (unsigned short)sprite[i].owner >> 8;
                sector[sect].floorpal = (unsigned char)sprite[i].owner;
            } else
                sector[sect].ceilingpal = sector[sect].floorpal = 0;
            sector[sect].floorshade = sector[sect].ceilingshade = (signed char)t[1];
            for (k = sector[sect].wallptr; k < sector[sect].wallptr + sector[sect].wallnum; k++)
                if (!(wall[k].hitag & 1))
                    wall[k].shade = (signed char)t[0];
            break;
        case 5: {
            /* The boss ship: hunts locators (stat 7, lotag t4) nearest the player. */
            long dist;

            p = findplayer(i, &dist);
            if (sprite[i].owner == -1) {
                t[4] = 0;
                scratch = 0x7FFFFFFF;
                for (;;) {
                    sprite[i].owner = findlocator((short)t[4], -1);
                    if (sprite[i].owner == -1)
                        break;
                    d = klabs(sprite[ps[p].sprite_index].x - sprite[sprite[i].owner].x) + klabs(sprite[ps[p].sprite_index].y - sprite[sprite[i].owner].y);
                    if (d < scratch) {
                        speed = sprite[i].owner;
                        scratch = d;
                    }
                    t[4]++;
                }
                sprite[i].owner = (short)speed;
                sprite[i].zvel = (short)(16 * ksgn(sprite[speed].z - sprite[i].z));
            }
            if (klabs(sprite[sprite[i].owner].x - sprite[i].x) + klabs(sprite[sprite[i].owner].y - sprite[i].y) < 1024) {
                short saved_angle = sprite[i].ang;

                sprite[i].ang = getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y);
                sprite[i].ang = saved_angle;
                sprite[i].owner = -1;
                break;
            }
            sprite[i].xvel = 256;
            speed = getincangle_shift(sprite[i].ang, getangle(sprite[sprite[i].owner].x - sprite[i].x, sprite[sprite[i].owner].y - sprite[i].y), 3);
            sprite[i].ang += speed;
            if ((krand() >> 8) < 223) {
                t[2] += getincangle_shift(t[2] + 512, getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y), 2);
                sector[sect].ceilingshade = 0;
            } else {
                t[2] += speed;
                sector[sect].ceilingshade = 127;
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
        }
        case 6:
            if (t[4] <= 0)
                sprite[i].xvel = 256;
            else {
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
            }
            for (j = headspritestat[3]; j >= 0; j = nextspritestat[j])
                if (sprite[j].lotag == 14 && sprite[j].hitag == sprite[i].hitag && hittype[j][0] == t[0]) {
                    sprite[j].xvel = sprite[i].xvel;
                    hittype[j][4] = t[4];
                }
            /* fall through */
        case 14:
        {
            long dang;

            if (sprite[i].owner == -1)
                sprite[i].owner = findlocator((short)t[3], (short)t[0]);
            d = klabs(sprite[sprite[i].owner].x - sprite[i].x) + klabs(sprite[sprite[i].owner].y - sprite[i].y);
            if (d < 1024) {
                if (lotag == 6 && (sprite[sprite[i].owner].hitag & 1))
                    t[4] = 256;
                t[3]++;
                sprite[i].owner = findlocator((short)t[3], (short)t[0]);
                if (sprite[i].owner == -1) {
                    t[3] = 0;
                    sprite[i].owner = findlocator(0, (short)t[0]);
                }
            }
            if (sprite[i].xvel == 0)
                break;
            dang = getincangle_shift(sprite[i].ang, getangle(sprite[sprite[i].owner].x - sprite[i].x, sprite[sprite[i].owner].y - sprite[i].y), 3);
            t[2] += dang;
            sprite[i].ang += dang;
            if (sprite[i].xvel == 256 && sound_voices[32].active == 0) {
                d = 1;
                if (!(sector[sect].floorstat & 1) && !(sector[sect].ceilingstat & 1))
                    spritesound(32, i);
            }
            if (sprite[i].xvel <= 64 && !(sector[sect].floorstat & 1) && !(sector[sect].ceilingstat & 1))
                stopsound(32);
            for (p = connecthead; p >= 0; p = connectpoint2[p]) {
                struct player_view *pl = &ps[p];

                if (sector[pl->cursectnum].hitag != sector[sect].hitag || sector[pl->cursectnum].hitag == -1)
                    continue;
                rotatepoint(sprite[i].x, sprite[i].y, pl->posx, pl->posy, (short)dang, &pl->posx, &pl->posy);
                if (d)
                    pl->posz -= 1024;
                pl->posx += (sprite[i].xvel * sintable[(sprite[i].ang + 512) & 2047]) >> 14;
                pl->posy += (sprite[i].xvel * sintable[sprite[i].ang & 2047]) >> 14;
                pl->ang += dang;
                setsprite(pl->sprite_index, pl->posx, pl->posy, pl->posz + 9216);
            }
            for (j = headspritesect[sect]; j >= 0; j = nextspritesect[j]) {
                short pic = sprite[j].picnum;

                if (pic == 175 || pic == 34 || pic == 283)
                    continue;
                rotatepoint(sprite[i].x, sprite[i].y, sprite[j].x, sprite[j].y, (short)dang, &sprite[j].x, &sprite[j].y);
                sprite[j].x += (sprite[i].xvel * sintable[(sprite[i].ang + 512) & 2047]) >> 14;
                sprite[j].y += (sprite[i].xvel * sintable[sprite[i].ang & 2047]) >> 14;
                sprite[j].ang += dang;
                setsprite(j, sprite[j].x, sprite[j].y, sprite[j].z);
            }
            movesector(i);
        }
            break;
        case 8:
        case 9: {
            long dir;

            if (getanimationgoal(&sector[sect].ceilingz) < 0)
                break;
            dir = (sector[sect].lotag & 0x8000) ? -t[3] : t[3];
            if (lotag == 9)
                dir = -dir;
            for (j = headspritestat[3]; j >= 0; j = nextspritestat[j]) {
                short s2 = sprite[j].sectnum;
                long minshade = sprite[j].shade, *u = hittype[j];

                if (sprite[j].lotag != lotag || sprite[j].hitag != sprite[i].hitag)
                    continue;
                for (scratch = sector[s2].wallptr; scratch < sector[s2].wallptr + sector[s2].wallnum; scratch++) {
                    if (wall[scratch].hitag & 1)
                        continue;
                    wall[scratch].shade += dir;
                    if (wall[scratch].shade < minshade)
                        wall[scratch].shade = (signed char)minshade;
                    else if (wall[scratch].shade > u[2])
                        wall[scratch].shade = (signed char)u[2];
                }
                sector[s2].floorshade += dir;
                sector[s2].ceilingshade += dir;
                if (sector[s2].floorshade < minshade)
                    sector[s2].floorshade = (signed char)minshade;
                else if (sector[s2].floorshade > u[0])
                    sector[s2].floorshade = (signed char)u[0];
                if (sector[s2].ceilingshade < minshade)
                    sector[s2].ceilingshade = (signed char)minshade;
                else if (sector[s2].ceilingshade > u[1])
                    sector[s2].ceilingshade = (signed char)u[0]; /* exe: clamps to t0, not t1 */
            }
            break;
        }
        case 10:
            if ((sector[sect].lotag & 0x7FFF) == 23) {
                if ((sector[sect].lotag & 0x8000) && sprite[i].hitag < ++t[0]) {
                    operatesectors(sect, i);
                    t[0] = 0;
                }
            } else if (sector[sect].floorz > sector[sect].ceilingz) {
                if (t[1] == 0 && sprite[i].hitag < ++t[0]) {
                    operatesectors(sect, i);
                    t[1] = 1;
                }
            } else {
                t[0] = 0;
                t[1] = 0;
            }
            break;
        case 11:
            /* A swinging door: t3 is its direction, t4 how far it has turned. */
            if (t[4] != 0) {
                short step = (short)(t[3] * (sprite[i].yvel >> 3));
                long w, end = sector[sect].wallptr + sector[sect].wallnum;

                t[2] += step;
                t[4] += step;
                movesector(i);
                for (w = sector[sect].wallptr; w < end; w++)
                    for (p = connecthead; p >= 0; p = connectpoint2[p])
                        if (clipinsidebox(ps[p].posx, ps[p].posy, (short)w, 128) == 1) {
                            t[2] -= step;
                            t[4] -= step;
                            movesector(i);
                        }
                if (t[4] <= -511 || t[4] >= 512) {
                    t[4] = 0;
                    t[2] &= ~0xFF;
                    movesector(i);
                }
            }
            break;
        case 12:
            if (t[0] == 1) {
                for (k = sector[sect].wallptr; k < sector[sect].wallptr + sector[sect].wallnum; k++)
                    wall[k].shade = (signed char)t[1];
                sector[sect].floorshade = (signed char)t[1];
                sector[sect].ceilingshade = (signed char)t[2];
            } else if (t[0] != 2)
                break;
            else if (sector[sect].floorshade > sprite[i].shade) {
                sector[sect].floorshade -= 2;
                sector[sect].ceilingshade -= 2;
                for (k = sector[sect].wallptr; k < sector[sect].wallptr + sector[sect].wallnum; k++)
                    wall[k].shade -= 2;
                break;
            }
            t[0] = 0;
            break;
        case 13:
            /* A blast ceiling: dropped force fields of its hitag set it off; at
               t2 4 it explodes. */
            if (t[2] != 0) {
                if (sprite[i].ang == 512) {
                    if (sprite[i].owner != 0)
                        sector[sect].ceilingz += (t[0] - sector[sect].ceilingz) >> 1;
                    else
                        sector[sect].floorz += (t[1] - sector[sect].floorz) >> 1;
                } else {
                    sector[sect].ceilingz += (t[0] - sector[sect].ceilingz) >> 1;
                    sector[sect].floorz += (t[1] - sector[sect].floorz) >> 1;
                }
                if (t[3] == 1) {
                    t[3] = 2;
                    sector[sect].ceilingstat ^= 1;
                    if (sprite[i].ang == 512) {
                        for (k = sector[sect].wallptr; k < sector[sect].wallptr + sector[sect].wallnum; k++)
                            wall[k].shade = sprite[i].shade;
                        sector[sect].floorshade = sprite[i].shade;
                        if (ps[0].parallax_sector >= 0) {
                            sector[sect].ceilingpicnum = sector[ps[0].parallax_sector].ceilingpicnum;
                            sector[sect].ceilingshade = sector[ps[0].parallax_sector].ceilingshade;
                        }
                    }
                }
                if (++t[2] > 7) {
                    if (t[3] != 2)
                        sector[sect].ceilingshade = 127;
                    if (sprite[i].ang != 512)
                        sector[sect].floorshade = 127;
                    goto delete_effector;
                }
            } else {
                for (j = headspritestat[6]; j >= 0; j = nextspritestat[j])
                    if (sprite[j].picnum == 57 && sprite[j].shade == -32 && sprite[j].hitag == sprite[i].hitag)
                        break;
                if (j >= 0)
                    for (j = headspritestat[3]; j >= 0; j = nextspritestat[j])
                        if (sprite[j].lotag == 13 && sprite[j].hitag == sprite[i].hitag)
                            hittype[j][2] = 1;
            }
            if (t[2] == 4 && sprite[i].ang != 512) {
                nuke(sprite[i].x, sprite[i].y, sector[sect].floorz - 12288 - ((krand() & 0x20) << 6), i);
                for (k = 0; k < 30; k += 3) {
                    long x = sprite[i].x, y = sprite[i].y;
                    long z = sector[sect].floorz - (krand() & 0x1000);
                    short pic = krand() % 11 + 235, a = krand() & 2047;
                    short xv = (krand() & 0x80) + 64, zv = -(krand() & 511) - 512;
                    short debris = insertsprite(sect, 5);
                    long *actor;

                    sprite[debris].x = x;
                    sprite[debris].y = y;
                    sprite[debris].z = z;
                    sprite[debris].picnum = pic;
                    sprite[debris].xrepeat = 24;
                    sprite[debris].yrepeat = 24;
                    sprite[debris].ang = a;
                    sprite[debris].xvel = xv;
                    sprite[debris].zvel = zv;
                    sprite[debris].owner = i;
                    sprite[debris].yvel = 0;
                    sprite[debris].cstat = 0;
                    hittype[debris][3] = 0;
                    sprite[debris].shade = -8;
                    hittype[debris][2] = 0;
                    actor = actorscrptr[pic];
                    hittype[debris][0] = 0;
                    if (actor != NULL) {
                        sprite[debris].extra = (short)actor[0];
                        hittype[debris][4] = actor[1];
                        hittype[debris][1] = actor[2];
                        if (pic != 57)
                            sprite[debris].hitag = (short)actor[3];
                    } else {
                        hittype[debris][4] = 0;
                        hittype[debris][1] = 0;
                    }
                }
            }
            break;
        case 15:
            if (t[4] == 0)
                break;
            sprite[i].xvel = 16;
            if (t[4] == 1) {
                if ((sprite[i].yvel >> 3) < t[3]) {
                    t[4] = 0;
                    break;
                }
                t[3]++;
            }
            if (t[4] == 2 && --t[3] < 1)
                t[4] = 0;
            movesector(i);
            break;
        case 16:
            t[2] += 32;
            if (sector[sect].floorz < sector[sect].ceilingz)
                sprite[i].shade = 0;
            else if (sector[sect].ceilingz < t[3]) {
                if (nextspritesect[headspritesect[sect]] == -1) {
                    goto delete_effector;
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
            /* A lift: t0 is its direction; riders move to the next floor's lift. */
            speed = 4 * sprite[i].yvel * t[0];
            sector[sect].ceilingz += speed;
            sector[sect].floorz += speed;
            for (j = headspritesect[sect]; j >= 0; j = nextspritesect[j])
                if (sprite[j].statnum < 3)
                    sprite[j].z += speed;
            if (t[0] == 0)
                break;
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
            for (p = connecthead; p >= 0; p = connectpoint2[p]) {
                if (ps[p].cursectnum != sect)
                    continue;
                for (j = headspritestat[3]; j >= 0; j = nextspritestat[j])
                    if (sprite[j].lotag == 17 && sector[sect].hitag - t[0] == sector[sprite[j].sectnum].hitag
                        && sprite[j].hitag == sprite[i].hitag) {
                        ps[p].posx += sprite[j].x - sprite[i].x;
                        ps[p].cursectnum = sprite[j].sectnum;
                        ps[p].posy += sprite[j].y - sprite[i].y;
                        setsprite(ps[p].sprite_index, ps[p].posx, ps[p].posy, ps[p].posz + 9216);
                    }
            }
            break;
        case 18: {
            /* A pull toward the effector; the player loop's empty body leaves p -1,
               so the pull lands on the record before ps. */
            unsigned char stats[3] = { 1, 4, 5 };
            long r = 16 * sprite[i].yvel;
            struct player_view *pl;
            short player;

            if (t[0] == 0)
                break;
            for (k = 0; k < 3; k++)
                for (j = headspritestat[stats[k]]; j >= 0; j = nextspritestat[j]) {
                    scratch = klabs(sprite[i].x - sprite[j].x) + klabs(sprite[i].y - sprite[j].y);
                    if (scratch >= r)
                        continue;
                    sprite[j].x += ksgn(sprite[i].x - sprite[j].x) << 7;
                    sprite[j].y += ksgn(sprite[i].y - sprite[j].y) << 7;
                    sprite[j].z += ksgn(sprite[i].z - sprite[j].z) << 12;
                    sprite[j].xvel = 0;
                    sprite[j].ang = (short)getangle(sprite[i].x - sprite[j].x, sprite[i].y - sprite[j].y);
                }
            /* The original skips the body of the connection walk and uses
             * the terminating index for this access. */
            for (player = connecthead; player >= 0; player = connectpoint2[player])
                ;
            pl = &ps[player];
            if (klabs(sprite[pl->sprite_index].x - sprite[i].x) + klabs(sprite[pl->sprite_index].y - sprite[i].y) < r) {
                pl->posx += ksgn(sprite[i].x - pl->posx) << 8;
                pl->posy += ksgn(sprite[i].y - pl->posy) << 8;
                clipmove(&pl->posx, &pl->posy, &pl->posz, &pl->cursectnum, 0, 0, 192, 1024, 1024, 0);
            }
            break;
        }
        case 19:
            if (t[0] == 0) {
                if (effectinsect(i) >= 0) {
                    FTA_INLINE(8)
                    for (scratch = headspritestat[3]; scratch >= 0; scratch = nextspritestat[scratch])
                        if ((sprite[scratch].lotag & 0x7FFF) == 19 && sprite[scratch].hitag == sprite[i].hitag
                            && hittype[scratch][0] == 0)
                            hittype[scratch][0] = 1;
                }
            } else if (sector[sect].ceilingz < sector[sect].floorz)
                sector[sect].ceilingz += sprite[i].yvel;
            else {
                sector[sect].ceilingz = sector[sect].floorz;
                goto delete_effector;
            }
            break;
        case 20: {
            /* A sliding sector: t1 and t2 are its two moving wall points. */
            long dx, dy;

            if (t[0] == 0)
                break;
            sprite[i].xvel = t[0] == 1 ? 128 : -128;
            dx = (sprite[i].xvel * sintable[(sprite[i].ang + 512) & 2047]) >> 14;
            dy = (sprite[i].xvel * sintable[sprite[i].ang & 2047]) >> 14;
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
            dragpoint((short)t[1], wall[t[1]].x + dx, wall[t[1]].y + dy);
            dragpoint((short)t[2], wall[t[2]].x + dx, wall[t[2]].y + dy);
            for (p = connecthead; p >= 0; p = connectpoint2[p])
                if (ps[p].cursectnum == sect && ps[p].on_ground) {
                    ps[p].posx += dx;
                    ps[p].posy += dy;
                }
            sector[sect].floorxpanning -= dx >> 3;
            sector[sect].floorypanning -= dy >> 3;
            sector[sect].ceilingxpanning -= dx >> 3;
            sector[sect].ceilingypanning -= dy >> 3;
            break;
        }
        case 21: {
            long *z;

            if (t[0] == 0)
                break;
            z = sprite[i].ang == 1536 ? &sector[sect].ceilingz : &sector[sect].floorz;
            scratch = (long)z;
            if (t[0] == 1) {
                sprite[i].xvel = (short)(16 * sprite[i].yvel * ksgn(sprite[i].z - *z));
                t[0]++;
            }
            if (sector[sect].extra != 0) {
                sector[sect].extra--;
                break;
            }
            *z += sprite[i].xvel;
            if (klabs(*z - sprite[i].z) < 1024) {
                *z = sprite[i].z;
                goto delete_effector;
            }
            break;
        }
        case 22:
            if (t[1] == 0)
                break;
            if (getanimationgoal(&sector[t[0]].ceilingz) >= 0)
                sector[sect].ceilingz += script_tics * sprite[i].yvel;
            else
                t[1] = 0;
            break;
        case 23:
            /* A flooding sector: once an effect lands in it, the sectors listed
               at msx[t1..] rise to the highest floor among them. */
            if (sprite[i].shade == 0) {
                if (effectinsect(i) >= 0)
                    sprite[i].shade = 1;
                break;
            }
            if (sprite[i].shade != 1)
                break;
            /* The initial sector comes from the shared scratch slot. */
            for (k = sector[scratch].wallptr; k < sector[scratch].wallptr + sector[scratch].wallnum; k++)
                if (sector[wall[k].nextsector].hitag == sector[scratch].hitag)
                    wall[k].picnum = 49;
            d = (long)0x80000000;
            for (k = 0; k < t[0]; k++) {
                scratch = msx[k + t[1]];
                if (d < sector[scratch].floorz && (sector[scratch].lotag & 1)) {
                    d = sector[scratch].floorz;
                    t[2] = scratch;
                }
            }
            for (k = 0; k < t[0]; k++) {
                scratch = msx[k + t[1]];
                if (sector[scratch].floorpicnum != 49)
                    sector[scratch].floorpicnum = 49;
                if ((sector[scratch].lotag & 0x7FFF) == 2)
                    sector[scratch].ceilingz = sector[t[2]].floorz;
                else {
                    sector[scratch].lotag |= 1;
                    if (sector[scratch].floorz >= sector[t[2]].floorz)
                        sector[scratch].floorz = sector[t[2]].floorz = sector[scratch].floorz - sprite[i].yvel;
                    if (sector[scratch].floorz < sector[scratch].ceilingz)
                        sector[scratch].floorz = sector[scratch].ceilingz;
                }
            }
            if (sector[sect].floorz < sprite[i].z)
                goto delete_effector;
        case 24: {
            /* A conveyor. */
            long dx = (sintable[(sprite[i].ang + 512) & 2047] * sprite[i].yvel) >> 18;
            long dy = (sintable[sprite[i].ang & 2047] * sprite[i].yvel) >> 18;

            scratch = dy;
            for (j = headspritesect[sect]; j >= 0; j = nextspritesect[j]) {
                short st = sprite[j].statnum;

                if (st == 9 || st == 0 || st == 3 || st == 10 || (sprite[j].picnum >= 1175 && sprite[j].picnum <= 1178))
                    continue;
                sprite[j].x += dx;
                sprite[j].y += dy;
                setsprite(j, sprite[j].x, sprite[j].y, sprite[j].z);
                if (sect != sprite[j].sectnum) {
                    sprite[j].x += 4 * dx;
                    sprite[j].y += 4 * dy;
                }
            }
            for (p = connecthead; p >= 0; p = connectpoint2[p])
                if (ps[p].cursectnum == sect && ps[p].on_ground) {
                    ps[p].posx += dx;
                    ps[p].posy += dy;
                    clipmove(&ps[p].posx, &ps[p].posy, &ps[p].posz, &ps[p].cursectnum, 0, 0, 192, 1024, 1024, 0);
                    setsprite(ps[p].sprite_index, ps[p].posx, ps[p].posy, ps[p].posz);
                }
            sector[sect].floorxpanning += dy >> 2;
            sector[sect].floorypanning += dx >> 2;
            break;
        }
        case 25:
            if (sprite[i].ang != 0)
                t[2] += 64;
            if (effectinsect(i) >= 0 && ++t[3] == 4) {
                goto delete_effector;
            }
            if (sector[sect].floorz <= sector[sect].ceilingz)
                sprite[i].shade = 0;
            else if (sector[sect].ceilingz <= t[4])
                sprite[i].shade = 1;
            if (sprite[i].shade) {
                sector[sect].ceilingz += 16 * sprite[i].yvel;
                if (sector[sect].ceilingz > sector[sect].floorz)
                    sector[sect].ceilingz = sector[sect].floorz;
            } else {
                sector[sect].ceilingz -= 16 * sprite[i].yvel;
                if (sector[sect].ceilingz < t[4])
                    sector[sect].ceilingz = t[4];
            }
            movesector(i);
            break;
        case 26: {
            /* A shaking or rolling sector that returns home (t3,t4) every 8 tics. */
            long dx = (32 * sintable[(sprite[i].ang + 512) & 2047]) >> 14, dy = (32 * sintable[sprite[i].ang & 2047]) >> 14;

            scratch = dx;
            sprite[i].xvel = 32;
            sprite[i].shade++;
            if (sprite[i].shade <= 7)
                sector[sect].floorz += sprite[i].zvel;
            else {
                sprite[i].x = t[3];
                sprite[i].y = t[4];
                sector[sect].floorz -= sprite[i].shade * sprite[i].zvel - sprite[i].zvel;
                sprite[i].shade = 0;
            }
            for (j = headspritesect[sect]; j >= 0; j = nextspritesect[j])
                if (sprite[j].statnum != 3 && sprite[j].statnum != 10) {
                    sprite[j].x += dx;
                    sprite[j].y += dy;
                    setsprite(j, sprite[j].x, sprite[j].y, sprite[j].z + sprite[i].zvel);
                }
            for (p = connecthead; p >= 0; p = connectpoint2[p])
                if (ps[p].cursectnum == sect && ps[p].on_ground) {
                    ps[p].posx += dx;
                    ps[p].posy += dy;
                    ps[p].posz += sprite[i].zvel;
                    setsprite(ps[p].sprite_index, ps[p].posx, ps[p].posy, ps[p].posz + 9216);
                }
            for (k = sector[sect].wallptr; k < sector[sect].wallptr + sector[sect].wallnum; k++)
                if (wall[k].lotag == 0)
                    wall[k].xpanning += 4;
            movesector(i);
            break;
        }
        case 27: {
            /* A demo camera: while a demo plays it takes the view when it can see
               the player from beyond its hitag. */
            long dist;

            p = findplayer(i, &dist);
            if (ps[p].health <= 0)
                break;
            if (t[0] < 0) {
                t[0]++;
                camerasprite = i;
                break;
            }
            if (demomode != 2 || ps[p].newowner != -1)
                break;
            if (cansee(sprite[i].x, sprite[i].y, sprite[i].z, sprite[i].sectnum, ps[p].posx, ps[p].posy, ps[p].posz, ps[p].cursectnum)) {
                if ((unsigned)sprite[i].hitag <= (unsigned)dist) {
                    if (t[0] == 999) {
                        t[0] = i == camerasprite ? 0 : -20;
                        camerasprite = i;
                    }
                } else {
                    t[0] = 999;
                    camerasprite = i;
                    sprite[i].ang = (short)getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y);
                    sprite[i].yvel = (short)((sprite[i].z - ps[p].posz) / 257 + 100);
                }
            } else if (t[0] == 999) {
                t[0] = i == camerasprite ? 0 : -20;
                camerasprite = i;
            }
            break;
        }
        case 128: {
            /* Breaking glass on wall t2: shards, then the wall clears after t1 tics. */
            short w = (short)t[2], nw;

            wall[w].cstat &= 0xDF;
            wall[w].cstat |= 0x10;
            nw = wall[w].nextwall;
            wall[nw].cstat &= 0xDF;
            wall[nw].cstat |= 0x10;
            wall[w].overpicnum++;
            wall[wall[w].nextwall].overpicnum++;
            if (t[0] == 0)
                for (k = 0; k < 15; k++) {
                    long z = sprite[i].z - (krand() & 0xFFF);
                    short zv, a;

                    if (z < sector[sprite[i].sectnum].ceilingz)
                        z = sprite[i].z;
                    zv = (short)(1024 - (krand() & 2047));
                    a = (short)(sprite[i].ang - 256 + (krand() & 511));
                    EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, z, 0, (short)((k & 3) + 970), -32, 18, 18, a, 64, zv, sprite[i].owner, 5);
                }
            if (t[0] < t[1]) {
                t[0]++;
                break;
            }
            wall[w].cstat &= 0xAE;
            wall[wall[w].nextwall].cstat &= 0xAE;
            goto delete_effector;
        }
        case 129: {
            unsigned char frames[12] = { 1, 0, 2, 0, 1, 0, 2, 0, 1, 0, 2, 0 };

            wall[t[2]].overpicnum = (short)(frames[t[0]] + 790);
            wall[wall[t[2]].nextwall].overpicnum = (short)(frames[t[0] + 2] + 790);
            if (++t[0] > 9)
                goto delete_effector;
        }
        }
        continue;

    delete_effector:
        deletesprite(i);
    }
}

/* A negative source selects an existing map sprite by index in pn. */
short spawn(long j, short pn)
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
    case 5:
        sprite[i].xrepeat = sprite[i].yrepeat = 0;
        changespritestat(i, 11);
        break;
    case 23: case 31:
        sprite[i].xrepeat = sprite[i].yrepeat = 0;
        if (sprite[i].picnum == 23)
            sector[sect].lotag ^= 0x4000;
        changespritestat(i, 8);
        break;
    case 34:
        changespritestat(i, 7);
        sprite[i].xrepeat = sprite[i].yrepeat = 0;
        break;
    case 30: case 313: case 314: case 1123:
        sprite[i].cstat |= 1;
        /* fall through */
    case 1122:
        sprite[i].shade = -12;
        show2dsprite[i >> 3] |= 1 << (i & 7);
        changespritestat(i, 1);
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
    case 265: case 266: case 267: case 268: case 269:
        sprite[i].picnum = 265;
        /* fall through */
    case 327:
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].shade = -16;
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
    case 57:
        sprite[i].owner = i;
        sprite[i].shade = -16;
        if (sprite[i].xrepeat <= 8)
            sprite[i].xrepeat = sprite[i].yrepeat = 0;
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].cstat |= 1;
        changespritestat(i, 6);
        break;
    case 175: {
        sprite[i].yvel = sector[sect].extra;
        sprite[i].xrepeat = 0;
        sprite[i].yrepeat = 0;
        switch (sprite[i].lotag) {
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
        case 24:
            sprite[i].yvel *= 2;
            sector[sect].floorstat |= 0x40;
            break;
        case 27:
            if (demomode == 1) {
                sprite[i].xrepeat = 64;
                sprite[i].yrepeat = 64;
            }
            break;
        }
        changespritestat(i, sprite[i].lotag == 7 ? 9 : 3);
        break;
    }
    case 215:
        sprite[i].xrepeat = sprite[i].yrepeat = 10;
        sprite[i].shade = -16;
        changespritestat(i, 5);
        break;
    case 283:
        sprite[i].xrepeat = sprite[i].yrepeat = 0;
        sprite[i].shade = -32;
        changespritestat(i, 10);
        break;
    case 323:
        sprite[i].z = sector[sect].floorz - (sprite[i].yrepeat << 7);
        break;
    case 408: case 490: case 579: case 1536: case 1680: case 1820:
        ps[0].bac++;
        ps[0].bae++;
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
    case 606: case 627: case 905: case 1200:
        sprite[i].owner = i;
        sprite[i].shade = -16;
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].yrepeat = sprite[i].xrepeat = 38;
        sprite[i].cstat |= 1;
        changespritestat(i, 6);
        break;
    case 686: case 1360: case 1370:
        if (sprite[i].picnum == 1360 && j >= 0 && sprite[j].statnum == 10)
            sprite[i].z += 4096;
        sprite[i].xrepeat = sprite[i].yrepeat = sprite[i].picnum == 1360 ? 8 : 28;
        sprite[i].cstat = (unsigned char)(krand() & 0xC);
        sprite[i].shade = -127;
        sprite[i].ang = sprite[j].ang;
        changespritestat(i, 5);
        break;
    case 930:
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].xrepeat = sprite[i].yrepeat = 32;
        hittype[i][3] = sector[sect].floorz;
        sprite[i].z = hittype[i][3] + 3072;
        sprite[i].cstat |= 1;
        ps[0].bae++;
        changespritestat(i, 2);
        break;
    case 931:
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].xrepeat = sprite[i].yrepeat = 32;
        hittype[i][3] = sector[sect].ceilingz;
        sprite[i].z = hittype[i][3] + 10752;
        sprite[i].cstat |= 9;
        ps[0].bae++;
        changespritestat(i, 2);
        break;
    case 940:
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].owner = i;
        sprite[i].xrepeat = sprite[i].yrepeat = 24;
        sprite[i].cstat |= 1;
        ps[0].bae++;
        changespritestat(i, 4);
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
    case 1280:
        sprite[i].xrepeat = sprite[i].yrepeat = 64;
        sprite[i].shade = -127;
        sprite[i].cstat |= 1;
        changespritestat(i, 2);
        break;
    case 1359:
        sprite[i].xrepeat = sprite[i].yrepeat = 32;
        sprite[i].cstat |= 0x22;
        sprite[i].ang = (short)(krand() & 2047);
        sprite[i].shade = 0;
        sprite[i].z = sector[sect].floorz;
        changespritestat(i, 5);
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
    case 1408:
        sprite[i].owner = -1;
        changespritestat(i, 1);
        break;
    }
    return i;
}

/* The window is rendered into the framebuffer, copied to the tile, then
 * drawn back at 1:1 before a second copy establishes the tile's orientation. */
/* Offsets advance by eight rows per iteration in each buffer. */
#define COPY_CAMERA_WINDOW(tile, row, dest) \
    for (; row < 320 * 95; row += 320 * 8, dest += 96 * 8) { \
        copybuf((void *)(frameplace + row), (void *)(waloff[tile] + dest), 24); \
        copybuf((void *)(frameplace + row + 320), (void *)(waloff[tile] + dest + 96), 24); \
        copybuf((void *)(frameplace + row + 640), (void *)(waloff[tile] + dest + 192), 24); \
        copybuf((void *)(frameplace + row + 960), (void *)(waloff[tile] + dest + 288), 24); \
        copybuf((void *)(frameplace + row + 1280), (void *)(waloff[tile] + dest + 384), 24); \
        copybuf((void *)(frameplace + row + 1600), (void *)(waloff[tile] + dest + 480), 24); \
        copybuf((void *)(frameplace + row + 1920), (void *)(waloff[tile] + dest + 576), 24); \
        copybuf((void *)(frameplace + row + 2240), (void *)(waloff[tile] + dest + 672), 24); \
    }

void render_camera_tile(long x, long y, long z, short ang, short sect, short tile)
{
    short row, dest, row2, dest2;

    setview(0, 0, 95, 95);
    drawrooms(x, y, z, ang, 100, sect);
    animatesprites(ang);
    drawmasks();
    row = dest = 0;
    COPY_CAMERA_WINDOW(tile, row, dest)
    row2 = dest2 = 0;
    rotatesprite(47L << 16, 47L << 16, 1L << 16, 0, tile);
    COPY_CAMERA_WINDOW(tile, row2, dest2)
    vscrn(screenpeek);
}

void render_sprite_camera(short i)
{
    short row, dest, row2, dest2, ang, sect, tile;
    long x, y, z;

    x = sprite[i].x;
    y = sprite[i].y;
    sect = sprite[i].sectnum;
    z = sprite[i].z;
    tile = sprite[i].picnum;
    ang = sprite[i].ang;
    setview(0, 0, 95, 95);
    drawrooms(x, y, z, ang, 100, sect);
    animatesprites(ang);
    drawmasks();
    row = dest = 0;
    COPY_CAMERA_WINDOW(tile, row, dest)
    rotatesprite(47L << 16, 47L << 16, 1L << 16, 0, tile);
    row2 = dest2 = 0;
    COPY_CAMERA_WINDOW(tile, row2, dest2)
    vscrn(screenpeek);
}

void render_mirror(short w)
{
    long x, y, z, player_x;
    short row, dest, row2, dest2, ang, sect, tile;

    y = wall[w].y;
    player_x = ps[screenpeek].posx;
    x = (wall[w].x + wall[wall[w].point2].x) / 2;
    y = (y + wall[wall[w].point2].y) / 2;
    z = ps[screenpeek].posz;
    ang = getangle(player_x - x, ps[screenpeek].posy - y);
    tile = 570;
    sect = wall[w].nextsector;
    setview(0, 0, 95, 95);
    drawrooms(x, y, z, ang, 100, sect);
    animatesprites(ang);
    drawmasks();
    row = dest = 0;
    COPY_CAMERA_WINDOW(570, row, dest)
    rotatesprite(47L << 16, 47L << 16, 1L << 16, 0, tile);
    row2 = dest2 = 0;
    COPY_CAMERA_WINDOW(tile, row2, dest2)
    vscrn(screenpeek);
}

#undef COPY_CAMERA_WINDOW

void resetplayer(short p)
{
    short look;

    ps[p].horiz = 100;
    ps[p].bobcounter = 0;
    ps[p].posx = ps[p].oposx;
    ps[p].posy = ps[p].oposy;
    ps[p].posz = ps[p].oposz;
    ps[p].ang = ps[p].oang;
    ps[p].cursectnum = ps[p].ocursectnum;
    ps[p].ba4 = 0;
    ps[p].on_ground = 0;
    look = 256 - (krand() & 512);
    ps[p].poszv = ps[p].jumping_counter = 0;
    ps[p].b8b = 0;
    ps[p].lookang = look;
    ps[p].w80 = -1;
    ps[p].holoduke = -1;
    ps[p].ba8 = 0;
    ps[p].newowner = -1;
    if (actorscrptr[283] != NULL)
        ps[p].health = (short)*actorscrptr[283];
    else
        ps[p].health = 100;
    ps[p].w82 = -1;
    ps[p].b90 = 0;
    ps[p].weapon_frame = 5;
    ps[p].ba1 = 6;
    ps[p].ba3 = 1;
    ps[p].mapzoom = 768;
}

void reset_player_motion(short p)
{
    short look;

    ps[p].ba2 = 0;
    ps[p].baa = 0;
    ps[p].ba9 = 0;
    ps[p].bad = 0;
    ps[p].bac = 0;
    ps[p].baf = 0;
    ps[p].bae = 0;
    ps[p].ba5 = 0;
    if (ps[p].viewmode == 2)
        setgamemode();
    camerasprite = -1;
    ps[p].viewmode = 3;
    mspos = 0;
    ps[p].parallax_sector = -1;
    ps[p].horiz = 100;
    viewscreen = -1;
    ps[p].oposx = ps[p].posx;
    ps[p].on_ground = 0;
    ps[p].oposy = ps[p].posy;
    ps[p].oposz = ps[p].posz;
    forward_velocity = 0;
    ps[p].oang = ps[p].ang;
    strafe_velocity = 0;
    ps[p].ocursectnum = ps[p].cursectnum;
    screenpeek = myconnectindex;
    ps[p].ba4 = 0;
    visibility = userdefs.visibility;
    parallaxtype = 0;
    ps[p].posx = ps[p].oposx;
    turn_velocity = 0;
    animatecnt = 0;
    ps[p].posy = ps[p].oposy;
    gamepaused = 0;
    ps[p].posz = ps[p].oposz;
    numanimwalls = 0;
    ps[p].ang = ps[p].oang;
    randomseed = 17;
    ps[p].cursectnum = ps[p].ocursectnum;
    ps[p].bobcounter = 0;
    look = 256 - (krand() & 512);
    ps[p].newowner = -1;
    ps[p].jumping_counter = 0;
    ps[p].w80 = -1;
    ps[p].holoduke = -1;
    ps[p].lookang = look;
    ps[p].ba8 = 0;
    ps[p].b8b = 0;
    ps[p].poszv = 0;
    if (actorscrptr[283] != NULL)
        ps[p].health = (short)*actorscrptr[283];
    else
        ps[p].health = 100;
    ps[p].w82 = -1;
    ps[p].b90 = 0;
    ps[p].weapon_frame = 5;
    ps[p].ba1 = 6;
    ps[p].ba3 = 1;
    ps[p].mapzoom = 768;
}

void prelevel(void)
{
    short p = 0, i, k, sect, nexti, map_sprite;
    long j;
    long *actor;

    ps[0].ba2 = 0;
    ps[0].baa = 0;
    ps[0].ba9 = 0;
    ps[0].bad = 0;
    ps[0].bac = 0;
    ps[0].baf = 0;
    ps[0].ba5 = 0;
    ps[0].bae = 0;
    if (ps[p].viewmode == 2)
        setgamemode();
    ps[p].viewmode = 3;
    ps[p].oposx = ps[p].posx;
    forward_velocity = 0;
    strafe_velocity = 0;
    ps[p].oposy = ps[p].posy;
    turn_velocity = 0;
    camerasprite = -1;
    ps[p].oposz = ps[p].posz;
    viewscreen = -1;
    ps[p].oang = ps[p].ang;
    gamepaused = 0;
    ps[p].ocursectnum = ps[p].cursectnum;
    numanimwalls = 0;
    ps[p].parallax_sector = -1;
    screenpeek = myconnectindex;
    parallaxtype = 0;
    visibility = userdefs.visibility;
    animatecnt = 0;
    mspos = 0;
    randomseed = 17;
    resetplayer(p);

    for (i = 0; i < 13; i++)
        ps[0].quote_time[i] = 0;
    for (i = 0; i < 512; i++)
        show2dwall[i] = 255;
    for (i = 0; i < 512; i++)
        show2dsprite[i] = 255 * disable_armor;

    for (i = 0; i < numsectors; i++) {
        sector[i].extra = 256;
        if (ps[0].parallax_sector == -1 && (sector[i].ceilingstat & 1))
            ps[0].parallax_sector = i;
        if ((sector[i].lotag & 32767) == 32767)
            ps[0].ba9++;
        else if (sector[i].lotag == 2)
            sector[i].ceilingpal = sector[i].floorpal = 1;
    }
    for (i = headspritestat[0]; i >= 0; i = nexti) {
        nexti = nextspritestat[i];
        if (sprite[i].picnum == 310) {
            sector[sprite[i].sectnum].extra = sprite[i].lotag;
            deletesprite(i);
        }
    }

    /* Initialize the map sprites in index order. */
    for (map_sprite = 0; map_sprite < 4096; map_sprite++) {
        short i = map_sprite, stat = -1;

        if (sprite[i].statnum >= 1024)
            continue;
        j = -1;
        k = sprite[i].picnum;
        hittype[i][3] = 0;
        hittype[i][2] = 0;
        hittype[i][0] = 0;
        if (k >= 265 && k <= 269)
            k = 265;
        actor = actorscrptr[k];
        if (actor == NULL) {
            hittype[i][4] = 0;
            hittype[i][1] = 0;
        } else {
            sprite[i].extra = (short)actor[0];
            hittype[i][4] = actor[1];
            hittype[i][1] = actor[2];
            if (k != 57)
                sprite[i].hitag = (short)actor[3];
        }
        sect = sprite[i].sectnum;
        switch (sprite[i].picnum) {
        case 5:
            sprite[i].xrepeat = sprite[i].yrepeat = 0;
            stat = 11;
            break;
        case 23: case 31:
            sprite[i].xrepeat = sprite[i].yrepeat = 0;
            if (sprite[i].picnum == 23)
                sector[sect].lotag ^= 0x4000;
            stat = 8;
            break;
        case 34:
            changespritestat(i, 7);
            sprite[i].xrepeat = sprite[i].yrepeat = 0;
            break;
        case 30: case 313: case 314: case 1123:
            sprite[i].cstat |= 1;
            /* fall through */
        case 1122:
            sprite[i].shade = -12;
            show2dsprite[i >> 3] |= 1 << (i & 7);
            stat = 1;
            break;
        case 305:
            hittype[i][0] = sprite[i].xrepeat;
            hittype[i][1] = sprite[i].yrepeat;
            /* fall through */
        case 32: case 51: case 216:
            sprite[i].xrepeat = sprite[i].yrepeat = 0;
            sprite[i].yvel = 0;
            stat = 1;
            break;
        case 265: case 266: case 267: case 268: case 269:
            sprite[i].picnum = 265;
            /* fall through */
        case 327:
            show2dsprite[i >> 3] |= 1 << (i & 7);
            sprite[i].shade = -16;
            stat = 1;
            break;
        case 38: case 60: case 70: case 80: case 159: case 263: case 315: case 316: case 381: case 620:
        case 677: case 720: case 721: case 722: case 723: case 724: case 726: case 727: case 728:
        case 1119: case 1120: case 1121: case 1125:
            sprite[i].shade = -17;
            show2dsprite[i >> 3] |= 1 << (i & 7);
            sprite[i].cstat |= 1;
            stat = 2;
            break;
        case 57:
            sprite[i].owner = i;
            sprite[i].shade = -16;
            if (sprite[i].xrepeat <= 8)
                sprite[i].xrepeat = sprite[i].yrepeat = 0;
            show2dsprite[i >> 3] |= 1 << (i & 7);
            sprite[i].cstat |= 1;
            stat = 6;
            break;
        case 175: {
            short k, wall_end;

            sprite[i].yvel = sector[sect].extra;
            sprite[i].xrepeat = 0;
            sprite[i].yrepeat = 0;
            switch (sprite[i].lotag) {
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
                hittype[i][3] = 8; /* the exe overwrites the value it just computed */
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
            case 24:
                sprite[i].yvel *= 2;
                sector[sect].floorstat |= 0x40;
                break;
            case 27:
                if (demomode == 1) {
                    sprite[i].xrepeat = 64;
                    sprite[i].yrepeat = 64;
                }
                break;
            }
            stat = sprite[i].lotag == 7 ? 9 : 3;
            break;
        }
        case 215:
            sprite[i].xrepeat = sprite[i].yrepeat = 10;
            sprite[i].shade = -16;
            stat = 5;
            break;
        case 283:
            sprite[i].xrepeat = sprite[i].yrepeat = 0;
            sprite[i].shade = -32;
            stat = 10;
            break;
        case 323:
            sprite[i].z = sector[sect].floorz - (sprite[i].yrepeat << 7);
            break;
        case 408: case 490: case 579: case 1536: case 1680: case 1820:
            ps[0].bac++;
            ps[0].bae++;
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
        case 606: case 627: case 905: case 1200:
            sprite[i].owner = i;
            sprite[i].shade = -16;
            show2dsprite[i >> 3] |= 1 << (i & 7);
            sprite[i].yrepeat = sprite[i].xrepeat = 38;
            sprite[i].cstat |= 1;
            stat = 6;
            break;
        case 686: case 1360: case 1370:
            if (sprite[i].picnum == 1360 && j >= 0 && sprite[j].statnum == 10)
                sprite[i].z += 4096;
            sprite[i].xrepeat = sprite[i].yrepeat = sprite[i].picnum == 1360 ? 8 : 28;
            sprite[i].cstat = (unsigned char)(krand() & 0xC);
            sprite[i].shade = -127;
            sprite[i].ang = sprite[j].ang;
            stat = 5;
            break;
        case 930:
            show2dsprite[i >> 3] |= 1 << (i & 7);
            sprite[i].xrepeat = sprite[i].yrepeat = 32;
            hittype[i][3] = sector[sect].floorz;
            sprite[i].z = hittype[i][3] + 3072;
            sprite[i].cstat |= 1;
            ps[0].bae++;
            stat = 2;
            break;
        case 931:
            show2dsprite[i >> 3] |= 1 << (i & 7);
            sprite[i].xrepeat = sprite[i].yrepeat = 32;
            hittype[i][3] = sector[sect].ceilingz;
            sprite[i].z = hittype[i][3] + 10752;
            sprite[i].cstat |= 9;
            ps[0].bae++;
            stat = 2;
            break;
        case 940:
            show2dsprite[i >> 3] |= 1 << (i & 7);
            sprite[i].owner = i;
            sprite[i].xrepeat = sprite[i].yrepeat = 24;
            sprite[i].cstat |= 1;
            ps[0].bae++;
            stat = 4;
            break;
        case 960:
            sprite[i].xrepeat = 16;
            sprite[i].yrepeat = 0;
            hittype[i][1] = krand() & 0x1F;
            sprite[i].z = sector[sect].ceilingz + 2048;
            stat = 2;
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
            stat = 2;
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
                stat = 5;
            }
            break;
        case 1175: {
            short k;

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
            stat = 6;
            break;
        }
        case 1190:
            sprite[i].xrepeat = sprite[i].yrepeat = 20;
            sprite[i].shade = -16;
            stat = 5;
            break;
        case 1280:
            sprite[i].xrepeat = sprite[i].yrepeat = 64;
            sprite[i].shade = -127;
            sprite[i].cstat |= 1;
            stat = 2;
            break;
        case 1359:
            sprite[i].xrepeat = sprite[i].yrepeat = 32;
            sprite[i].cstat |= 0x22;
            sprite[i].ang = (short)(krand() & 2047);
            sprite[i].shade = 0;
            sprite[i].z = sector[sect].floorz;
            stat = 5;
            break;
        case 1404:
            if (j >= 0) {
                sprite[i].owner = j;
                sprite[i].xrepeat = (unsigned char)(tilesizx[sprite[j].picnum] / 3);
                sprite[i].yrepeat = (unsigned char)(tilesizy[sprite[j].picnum] / 3);
                sprite[i].shade = -127;
            }
            stat = 5;
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
                stat = 4;
            }
            break;
        case 1408:
            sprite[i].owner = -1;
            stat = 1;
            break;
        }
        if (stat != -1)
            changespritestat(i, stat);
    }

    for (i = 0; i < numwalls; i++) {
        if (numanimwalls >= 256)
            gameexit("Maxed out on anim walls\n");
        if (wall[i].overpicnum >= 1184 && wall[i].overpicnum <= 1186) {
            if (wall[i].overpicnum == 1184 && waloff[1184] == -1)
                for (k = 0; k < 3; k++)
                    loadtile(1184 + k);
            if (wall[i].shade < 32)
                wall[i].cstat |= 1;
            else
                wall[i].cstat = 0;
            wall[i].shade = 0;
            if (wall[i].lotag != 0)
                wall[wall[i].nextwall].lotag = wall[i].lotag;
            animwall[numanimwalls++].wallnum = i;
            continue;
        }
        switch (wall[i].picnum) {
        case 406:
            wall[i].picnum = 405;
            checkhitswitch(wall[i].lotag);
            /* fall through */
        case 395: case 396: case 405:
            animwall[numanimwalls].tag = 0;
            animwall[numanimwalls++].wallnum = i;
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
        case 819: case 823: case 860: case 864:
            if (waloff[wall[i].picnum] == -1)
                for (k = 0; k < 4; k++)
                    loadtile(wall[i].picnum + k);
            /* fall through */
        case 570:
            animwall[numanimwalls++].wallnum = i;
            break;
        }
    }
}

void newgame(unsigned char episode, unsigned char level, unsigned char skill)
{
    ps[0].level = level;
    ps[0].episode = episode;
    ps[0].skill = skill;
    ps[0].b93 = 0;
    ps[0].b97 = 0;
    ps[0].b94 = 0;
    ps[0].b98 = 0;
    ps[0].b95 = 0;
    ps[0].b99 = 0;
    ps[0].b96 = 0;
    ps[0].b9a = 0;
    ps[0].b92 = 0;
    ps[0].b9c = 0;
    ps[0].b9e = 0;
    ps[0].b9f = 0;
    ps[0].ba0 = 0;
    ps[0].screen_size = -4;
    ps[0].b9d = 1;
    ps[0].score = 0;
    if (disable_armor) {
        ps[0].rocket_ammo = 99;
        ps[0].remote_ammo = 696;
        ps[0].ammo = 144;
        ps[0].batteryammo = 120;
    } else {
        ps[0].rocket_ammo = 5;
        ps[0].remote_ammo = 2;
        ps[0].ammo = 24;
        ps[0].batteryammo = 9;
    }
    ps[0].last_weapon = -1;
    ps[0].curr_weapon = 191;
    reccnt = 0;
}

void enterlevel(unsigned char mode)
{
    char *map_name = "L1.MAP";
    short i, p, load_result;
    unsigned char palette;

    if (disable_armor == 0 && (mode & 4)) {
        dobonus();
    } else {
        overwritesprite(0, 0, 482, 0, 0, 0);
        minitext(170, 50, (unsigned char *)"Entering...", 0);
        minitext(174, 62, (unsigned char *)"Episode", 0);
        minitext(174, 74, (unsigned char *)"Level", 0);
        numtext(232, 62, ps[0].episode + 1, 0);
        numtext(232, 74, ps[0].level + 1, 0);
        nextpage();
    }
    map_name[0] = 'L' + ps[0].episode;
    map_name[1] = '1' + ps[0].level;
    FX_StopAllSounds();
    call_level_load_hook(map_name);
    load_result = loadboard(map_name, &ps[0].posx, &ps[0].posy, &ps[0].posz,
                            &ps[0].ang, &ps[0].cursectnum);
    call_level_load_hook(map_name);
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
        pending_svel[i] = player_svel[i] = 0;
        pending_vel[i] = player_vel[i] = 0;
        pending_avel[i] = player_avel[i] = 0;
        pending_bits[i] = player_bits[i] = 0;
    }
    moveloop();
    if (option[0] == 7)
        setview(0, 0, 639, 479);
    else
        vscrn(0);

    palette = 0;
    i = headspritestat[10];
    for (p = connecthead; p >= 0; p = connectpoint2[p]) {
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].cstat = 1;
        sprite[i].shade = -16;
        sprite[i].owner = i;
        sprite[i].yvel = p;
        ps[p].palette = palette;
        if (palette != 0)
            palette++;
        else
            palette = 3;
        sprite[i].xrepeat = sprite[i].yrepeat = 42;
        ps[p].sprite_index = i;
        if (numplayers > 1) {
            ps[p].oposx = ps[p].posx = sprite[i].x;
            ps[p].oposy = ps[p].posy = sprite[i].y;
            ps[p].oposz = ps[p].posz = sprite[i].z;
            ps[p].oang = ps[p].ang = sprite[i].ang;
            ps[p].ocursectnum = ps[p].cursectnum = sprite[i].sectnum;
        }
        updatesector(sprite[i].x, sprite[i].y, &ps[p].cursectnum);
        i = nextspritestat[i];
    }
    {
        long local_player = myconnectindex;
        char *music = music_files[ps[local_player].episode][0];

        music += 13 * ps[local_player].level;
        ps[local_player].music = music;
        playmusic(music);
    }
    displaystatus(myconnectindex);
    lockclock = totalclock = 0;
    level_tick_state = script_tics = 0;
    ps[myconnectindex].gm = mode;
}

unsigned char check_file_id(void)
{
    int file;
    long expected, actual;

    file = open("FILE_ID.DIZ", O_RDWR | O_BINARY, S_IREAD);
    if (file == -1)
        return 1;

    read(file, tempbuf, 450);
    actual = 0;
    for (expected = 0; expected < 400; expected++, actual++) {
        if (file_id_record[expected] == '*') {
            expected++;
            actual += 2;
        }
        if (tempbuf[actual] != file_id_record[expected]) {
            close(file);
            return 1;
        }
    }
    close(file);
    bootleg_cache_pop();
    return 0;
}

/* Keep the local shade update as a call in this routine. */
extern long score_text_shade(void);
#pragma aux score_text_shade "next_text_shade_"

short newscore(void)
{
    short i, j;

    score_text_shade();
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

unsigned char getscorename(short row)
{
    return strget(50, row * 11 + 50, next_text_shade(), 31) != 0;
}
