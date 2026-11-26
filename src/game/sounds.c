#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "sounds.h"

/* Explicit trailing NULs preserve the original literal alignment. */
char *sound_names[NUM_SOUNDS] = {
    "elev1.voc", "shooting.voc", "running.voc", "expl1.voc", "xoptor.voc",
    "glass.voc", "door1.voc", "knuckle.voc", "jump.voc", "land.voc",
    "scuba.voc", "waterfal.voc", "getweapn.voc", "bodyblop.voc\0\0\0", "bodybloa.voc",
    "bodyblor.voc", "onBoard.voc", "dripping.voc", "jetPakOn.voc", "jetPakI.voc",
    "fta_hm.voc", "fta_hf.voc", "fta_r.voc", "liteflck.voc", "biglite.voc",
    "switch.voc", "reactor.voc", "jetpakof.voc", "runningw.voc", "squish1.voc",
    "rebound.voc", "gun1.voc", "subway.voc", "clipin.voc", "clipout.voc",
    "elev3.voc", "elev2.voc", "door2.voc", "rpg.voc"
};

char *sound_device_names[] = {
    "None", "SoundBlaster\0\0\0", "Pro AudioSpectrum", "Adlib", "General Midi",
    "WaveBlaster", "Gravis UltraSound", "Disney Sound Source", "PC speaker\0"
};

int sound_device_map[] = { -1, 0, 1, 3, 4, 7, 9, 10, 12 };

/* Shared storage in the game data region. */
extern struct sound_file sound_file_table[NUM_SOUNDS];
extern unsigned char music_buffer[40000];
extern char bootleg_cache[300000];
extern struct sound_voice sound_voices[NUM_SOUNDS];

/* BootLeg CacheMaster bookkeeping owned by this module. */
long cache_sizes[128], cache_depth, cache_remaining;
char *cache_cursor;
#pragma pack(1)
struct user_settings {          /* the 21-byte USERDEFS.TMP block defined by GLOBAL */
    unsigned char visibility, quotes, brightness, sound, music;
    short debug, sound_volume, music_volume, paused, camera, mouse_sensitivity;
    unsigned char reserved[4];
};
#pragma pack()
extern struct user_settings ud;

/* dataown: one object, one name. The spellings below are the GLOBAL definitions or fields of
   its settings block; the extern declarations above keep this unit's symbol slots. */
extern char option[];
#define MusicToggle option[2]

char *bootleg_cache_alloc(long size)
{
    if (cache_depth > 127)
        gameexit("Clog Error.");

    cache_sizes[cache_depth] = size;
    cache_depth++;
    cache_cursor += size;
    cache_remaining -= size;
    if (cache_remaining <= 0)
        gameexit("Out of memory!\0");

    return cache_cursor - size;
}

void bootleg_cache_pop(void)
{
    if (cache_depth > 0) {
        cache_depth--;
        cache_cursor -= cache_sizes[cache_depth];
        cache_remaining += cache_sizes[cache_depth];
    }
}

void bootleg_cache_reset(void)
{
    while (cache_depth > 0) {
        cache_depth--;
        cache_cursor -= cache_sizes[cache_depth];
        cache_remaining += cache_sizes[cache_depth];
    }
}

void init_bootleg_cache(void)
{
    cache_cursor = bootleg_cache;
    cache_depth = 0;
    cache_remaining = sizeof(bootleg_cache);
}

int USRHOOKS_GetMem(char **ptr, unsigned long size)
{
    char *memory = bootleg_cache_alloc(size);
    if (!memory)
        gameexit("Out a memory.\0\0");
    *ptr = memory;
    return 0;
}

int USRHOOKS_FreeMem(char *ptr)
{
    if (cache_depth > 0) {
        cache_depth--;
        cache_cursor -= cache_sizes[cache_depth];
        cache_remaining += cache_sizes[cache_depth];
    }
    return 0;
}

void loadsounds(void)
{
    long i, length;
    int fd;
    char *data;

    for (i = 0; i < NUM_SOUNDS; i++) {
        fd = open(sound_names[i], O_RDWR | O_BINARY, S_IREAD);
        if (fd != -1) {
            length = filelength(fd);
            data = bootleg_cache_alloc(length);
            read(fd, data, length);
            close(fd);
            sound_file_table[i].data = data;
        }
    }
}

void playmusic(char *filename)
{
    FILE *fp;
    long length;

    if (MusicToggle == 0)
        return;

    fp = fopen(filename, "rb\0");
    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fread(music_buffer, 1, length, fp);
    fclose(fp);
    MUSIC_PlaySong(music_buffer, 1);
}

void xyzsound(short num, short i, long x, long y, long z)
{
    long cx, cy, distance, voice;
    short ca, angle, priority;

    if (SoundToggle == 0 || sound_voices[num].active > 2)
        return;

    if (camerasprite == -1) {
        cx = ps[screenpeek].posx;
        cy = ps[screenpeek].posy;
        ca = ps[screenpeek].ang + ps[screenpeek].look_ang;
    } else {
        cx = sprite[camerasprite].x;
        cy = sprite[camerasprite].y;
        ca = sprite[camerasprite].ang;
    }

    angle = 2048 + ca - getangle(cx - x, cy - y);
    angle &= 2047;
    distance = klabs(cx - x) + klabs(cy - y);
    priority = 0;
    if (num == 32)
        priority = 255;
    else if (num == 17)
        distance <<= 5;
    if ((distance >> 6) > 255)
        return;

    voice = FX_PlayVOC3D(sound_file_table[num].data, 0, angle >> 4,
                         distance >> 6, priority, num);
    if (voice > 0) {
        /* The prototype also stores Z in the Y field. */
        sound_voices[num].y = y;
        sound_voices[num].y = z;
        sound_voices[num].x = x;
        if (i == -1)
            sound_voices[num].sprite = ps[0].sprite;
        else
            sound_voices[num].sprite = i;
        sound_voices[num].handle = voice;
        sound_voices[num].active++;
    }
}

/* The indices use the signed low words of the register arguments. */
void spritesound(int number, int source)
{
    long x, y, z, cx, cy, distance, scaled, voice;
    short ca, angle, priority;
    short i;
    short num = number;

    i = source;
    x = sprite[i].x;
    y = sprite[i].y;
    z = sprite[i].z;

    if (SoundToggle == 0 || sound_voices[num].active > 2)
        return;

    if (camerasprite == -1) {
        cx = ps[screenpeek].posx;
        cy = ps[screenpeek].posy;
        ca = ps[screenpeek].ang + ps[screenpeek].look_ang;
    } else {
        cx = sprite[camerasprite].x;
        cy = sprite[camerasprite].y;
        ca = sprite[camerasprite].ang;
    }

    angle = 2048 + ca - getangle(cx - x, cy - y);
    angle &= 2047;
    distance = klabs(cx - x) + klabs(cy - y);
    priority = 0;
    if (num == 32)
        priority = 255;
    else if (num == 17)
        distance <<= 5;
    scaled = distance >> 6;
    if (scaled > 255)
        return;

    voice = FX_PlayVOC3D(sound_file_table[num].data, 0, angle >> 4,
                         scaled, priority, num);
    if (voice > 0) {
        /* The prototype also stores Z in the Y field. */
        sound_voices[num].y = y;
        sound_voices[num].y = z;
        sound_voices[num].x = x;
        sound_voices[num].sprite = i == -1 ? ps[0].sprite : i;
        sound_voices[num].handle = voice;
        sound_voices[num].active++;
    }
}

void stopsound(short num)
{
    FX_StopSound(sound_voices[num].handle);
    if (sound_voices[(unsigned long)num].active > 0)
        sound_voices[(unsigned long)num].active--;
}

void pan3dsound(void)
{
    long cx, cy, dx, dy, distance;
    short ca, angle;
    long i;

    if (camerasprite == -1) {
        cx = ps[screenpeek].posx;
        cy = ps[screenpeek].posy;
        ca = ps[screenpeek].ang + ps[screenpeek].look_ang;
    } else {
        cx = sprite[camerasprite].x;
        cy = sprite[camerasprite].y;
        ca = sprite[camerasprite].ang;
    }

    for (i = 0; i < NUM_SOUNDS; i++) {
        if (sound_voices[i].active) {
            dx = sprite[sound_voices[i].sprite].x;
            dy = sprite[sound_voices[i].sprite].y;
            angle = 2048 + ca - getangle(cx - dx, cy - dy);
            angle &= 2047;
            distance = klabs(cx - dx) + klabs(cy - dy);
            FX_Pan3D(sound_voices[i].handle, angle >> 4, distance >> 6);
        }
    }
}

void TestCallBack(long number)
{
    if (sound_voices[number].active > 0)
        sound_voices[number].active--;
}



