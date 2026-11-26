#include <dos.h>
#include <fcntl.h>
#include <i86.h>
#include <io.h>
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

/* Fields used here from the shared 176-byte player record. */
#pragma pack(1)
struct player_view {
    short sprite_index;
    long viewmode;
    unsigned char before_score[0x30];
    unsigned long score;
    unsigned char before_sector[0x0a];
    short cursectnum;
    unsigned char before_demo_fields[0x46];
    unsigned char skill, level, episode;
    unsigned char before_palette[0x18];
    unsigned char palette;
    unsigned char tail[8];
};
#pragma pack()

typedef char player_size_check[sizeof(struct player_view) == 176 ? 1 : -1];

extern struct player_view ps[17];
extern char demo_filename[], demo_missing[];

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
