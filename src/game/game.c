#include <dos.h>
#include <fcntl.h>
#include <i86.h>
#include <io.h>
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
extern int strget(long x, long y, long shade, long limit);
extern void newgame(unsigned char episode, unsigned char level, unsigned char skill);

void (__interrupt __far *oldtimerhandler)(void);
extern short hudpulse;
extern short sintable[2048];
extern unsigned char keys[17], keystatus[256];
extern long turn_velocity, strafe_velocity, forward_velocity;
extern char tempbuf[];
extern long frameplace;

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

/* The demo I/O uses three fields from player zero's shared 176-byte record. */
struct player_demo_view {
    unsigned char before_demo_fields[0x8c];
    unsigned char skill, level, episode;
    unsigned char after_demo_fields[0x21];
};

typedef char player_size_check[sizeof(struct player_demo_view) == 176 ? 1 : -1];

extern struct player_demo_view ps[17];
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

unsigned char getscorename(short row)
{
    return strget(50, row * 11 + 50, next_text_shade(), 31) != 0;
}
