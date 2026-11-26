#ifndef GAME2_BUILD_V5_H
#define GAME2_BUILD_V5_H

/* Shared Build map-v5 ABI: the record layouts are in records.h (after src/engine/BUILD.H). */
#include "records.h"

typedef char wall_size_check[sizeof(struct walltype) == 35 ? 1 : -1];

typedef char sprite_size_check[sizeof(struct spritetype) == 38 ? 1 : -1];
typedef char sector_size_check[sizeof(struct sectortype) == 37 ? 1 : -1];

extern struct spritetype sprite[4096];
extern struct sectortype sector[1024];
extern short headspritestat[], nextspritestat[4096];

extern struct walltype wall[8192];
extern short headspritesect[], nextspritesect[4096];

#endif
