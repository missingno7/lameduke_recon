/* SOUNDS: the 'BootLeg CacheMaster' LIFO arena (also the Apogee Sound System USRHOOKS memory hooks), VOC loading, MIDI playback
   and positional effects through Jim Dose's ASS (FX_PlayVOC3D/FX_Pan3D). Fixed 39-name VOC table, at most 3 copies of a sound.
   Duke3D 1.5 SOUNDS.C (1996) keeps xyzsound/spritesound/stopsound/pan3dsound/TestCallBack/playmusic/USRHOOKS_*; cache names are invented. */

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
extern struct user_defs ud;         /* the 21-byte USERDEFS.TMP block defined by GLOBAL */

/* dataown: one object, one name. camerasprite and SoundToggle (sounds.h) are bound names of ud's fields
   (2:a57b5, 2:a57ab); MusicToggle there is unused and only keeps this unit's symbol slots. playmusic tests
   the music card chosen in SETUP.DAT (option[2], Duke 1.5's MusicDevice), not ud.MusicToggle. */
extern char option[];
#define MusicDevice option[2]

/* BootLeg CacheMaster: allocate size bytes from the LIFO arena (exits on overflow). */
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

/* Free the newest BootLeg cache block. */
void bootleg_cache_pop(void)
{
    if (cache_depth > 0) {
        cache_depth--;
        cache_cursor -= cache_sizes[cache_depth];
        cache_remaining += cache_sizes[cache_depth];
    }
}

/* Free all BootLeg cache blocks (unused). */
void bootleg_cache_reset(void)
{
    while (cache_depth > 0) {
        cache_depth--;
        cache_cursor -= cache_sizes[cache_depth];
        cache_remaining += cache_sizes[cache_depth];
    }
}

/* Initialise the BootLeg CacheMaster arena (300000 bytes). */
void init_bootleg_cache(void)
{
    cache_cursor = bootleg_cache;
    cache_depth = 0;
    cache_remaining = sizeof(bootleg_cache);
}

/* Apogee Sound System hook: allocate size bytes into *ptr from the BootLeg cache. */
int USRHOOKS_GetMem(char **ptr, unsigned long size)
{
    char *memory = bootleg_cache_alloc(size);
    if (!memory)
        gameexit("Out a memory.\0\0");
    *ptr = memory;
    return 0;
}

/* Apogee Sound System hook: free (pops the newest BootLeg cache block). */
int USRHOOKS_FreeMem(char *ptr)
{
    if (cache_depth > 0) {
        cache_depth--;
        cache_cursor -= cache_sizes[cache_depth];
        cache_remaining += cache_sizes[cache_depth];
    }
    return 0;
}

/* Load every VOC of sound_names into the BootLeg cache (startup, after FX_SetupCard). */
void loadsounds(void)
{
    long i, l;
    int fp;
    char *data;

    for (i = 0; i < NUM_SOUNDS; i++) {
        fp = open(sound_names[i], O_RDWR | O_BINARY, S_IREAD);
        if (fp != -1) {
            l = filelength(fp);
            data = bootleg_cache_alloc(l);
            read(fp, data, l);
            close(fp);
            sound_file_table[i].ptr = data;
        }
    }
}

/* Load and loop a MIDI song through MUSIC_PlaySong. */
void playmusic(char *fn)
{
    FILE *fp;
    long l;

    if (MusicDevice == 0)
        return;

    fp = fopen(fn, "rb\0");
    fseek(fp, 0, SEEK_END);
    l = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fread(music_buffer, 1, l, fp);
    fclose(fp);
    MUSIC_PlaySong(music_buffer, 1);
}

/* Play sound num at map position x,y (owner sprite i, -1 = player 0). */
void xyzsound(short num, short i, long x, long y, long z)
{
    long cx, cy, sndist, voice;
    short ca, sndang, priority;

    if (SoundToggle == 0 || sound_voices[num].num > 2)
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

    sndang = 2048 + ca - getangle(cx - x, cy - y);
    sndang &= 2047;
    sndist = klabs(cx - x) + klabs(cy - y);
    priority = 0;
    if (num == 32)
        priority = 255;
    else if (num == 17)
        sndist <<= 5;
    if ((sndist >> 6) > 255)
        return;

    voice = FX_PlayVOC3D(sound_file_table[num].ptr, 0, sndang >> 4,
                         sndist >> 6, priority, num);
    if (voice > 0) {
        /* The prototype also stores Z in the Y field. */
        sound_voices[num].y = y;
        sound_voices[num].y = z;
        sound_voices[num].x = x;
        if (i == -1)
            sound_voices[num].i = ps[0].i;
        else
            sound_voices[num].i = i;
        sound_voices[num].voice = voice;
        sound_voices[num].num++;
    }
}

/* Play sound num at sprite i (inline copy of xyzsound). */
/* The indices use the signed low words of the register arguments. */
void spritesound(int number, int source)
{
    long x, y, z, cx, cy, sndist, scaled, voice;
    short ca, sndang, priority;
    short i;
    short num = number;

    i = source;
    x = sprite[i].x;
    y = sprite[i].y;
    z = sprite[i].z;

    if (SoundToggle == 0 || sound_voices[num].num > 2)
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

    sndang = 2048 + ca - getangle(cx - x, cy - y);
    sndang &= 2047;
    sndist = klabs(cx - x) + klabs(cy - y);
    priority = 0;
    if (num == 32)
        priority = 255;
    else if (num == 17)
        sndist <<= 5;
    scaled = sndist >> 6;
    if (scaled > 255)
        return;

    voice = FX_PlayVOC3D(sound_file_table[num].ptr, 0, sndang >> 4,
                         scaled, priority, num);
    if (voice > 0) {
        /* The prototype also stores Z in the Y field. */
        sound_voices[num].y = y;
        sound_voices[num].y = z;
        sound_voices[num].x = x;
        sound_voices[num].i = i == -1 ? ps[0].i : i;
        sound_voices[num].voice = voice;
        sound_voices[num].num++;
    }
}

/* Stop sound num (its last voice). */
void stopsound(short num)
{
    FX_StopSound(sound_voices[num].voice);
    if (sound_voices[(unsigned long)num].num > 0)
        sound_voices[(unsigned long)num].num--;
}

/* Re-pan all playing sounds to their owners (call once per frame). */
void pan3dsound(void)
{
    long cx, cy, dx, dy, sndist;
    short ca, sndang;
    long j;

    if (camerasprite == -1) {
        cx = ps[screenpeek].posx;
        cy = ps[screenpeek].posy;
        ca = ps[screenpeek].ang + ps[screenpeek].look_ang;
    } else {
        cx = sprite[camerasprite].x;
        cy = sprite[camerasprite].y;
        ca = sprite[camerasprite].ang;
    }

    for (j = 0; j < NUM_SOUNDS; j++) {
        if (sound_voices[j].num) {
            dx = sprite[sound_voices[j].i].x;
            dy = sprite[sound_voices[j].i].y;
            sndang = 2048 + ca - getangle(cx - dx, cy - dy);
            sndang &= 2047;
            sndist = klabs(cx - dx) + klabs(cy - dy);
            FX_Pan3D(sound_voices[j].voice, sndang >> 4, sndist >> 6);
        }
    }
}

/* FX callback: a voice of sound num finished. */
void TestCallBack(long num)
{
    if (sound_voices[num].num > 0)
        sound_voices[num].num--;
}



