#ifndef SOUNDS_H
#define SOUNDS_H

#include "records.h"

#define NUM_SOUNDS 39

extern struct spritetype sprite[];
extern struct player_struct ps[];
extern short screenpeek;
extern struct user_defs ud;

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



