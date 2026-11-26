#ifndef SOUNDS_H
#define SOUNDS_H

#define NUM_SOUNDS 39

#pragma pack(1)
struct sound_file {
    char *data;
    long length;
};

struct sound_voice {
    long x, y, z;
    short sprite;
    long handle;
    unsigned char active;
};

/* Portions of the map-v5 sprite and prototype player records used by SOUNDS. */
struct sound_sprite {
    long x, y, z;
    char fields_before_ang[6];
    short ang;
    char fields_after_ang[18];
};

struct sound_player {
    short sprite;
    char fields_before_pos[8];
    long posx, posy;
    char fields_before_ang[44];
    short ang;
    char fields_before_look_ang[8];
    short look_ang;
    char fields_after_look_ang[102];
};
#pragma pack()

extern struct sound_sprite sprite[];
extern struct sound_player ps[];
extern short screenpeek, camerasprite;
extern char SoundToggle, MusicToggle;

extern void gameexit(char *message);
extern int getangle(long x, long y);
extern int MUSIC_PlaySong(unsigned char *song, int loopflag);
extern int FX_PlayVOC3D(char *data, int pitch, int angle, int distance,
                       int priority, unsigned long callback);
extern int FX_StopSound(int voice);
extern int FX_Pan3D(int voice, int angle, int distance);

/* Build's original absolute-value intrinsic (Build PRAGMAS.H). */
extern long klabs(long value);
#pragma aux klabs = \
    "test eax, eax" \
    "jns skipnegate" \
    "neg eax" \
    "skipnegate:" \
    parm nomemory [eax]

#endif



