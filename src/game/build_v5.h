#ifndef GAME2_BUILD_V5_H
#define GAME2_BUILD_V5_H

/* Shared Build map-v5 ABI. See src/engine/BUILD.H. */
#pragma pack(1)
struct spritetype {
    long x, y, z;
    char cstat;
    signed char shade;
    unsigned char xrepeat, yrepeat;
    short picnum, ang, xvel, yvel, zvel, owner;
    short sectnum, statnum, lotag, hitag, extra;
};

struct sectortype {
    short wallptr, wallnum;
    short ceilingpicnum, floorpicnum, ceilingheinum, floorheinum;
    long ceilingz, floorz;
    signed char ceilingshade, floorshade;
    unsigned char ceilingxpanning, floorxpanning, ceilingypanning, floorypanning;
    unsigned char ceilingstat, floorstat, ceilingpal, floorpal, visibility;
    short lotag, hitag, extra;
};
struct walltype {
    long x, y;
    short point2, picnum, overpicnum;
    signed char shade;
    short cstat;
    unsigned char xrepeat, yrepeat, xpanning, ypanning;
    short nextsector, nextwall, nextsector2, nextwall2;
    short lotag, hitag, extra;
};

typedef char wall_size_check[sizeof(struct walltype) == 35 ? 1 : -1];
#pragma pack()

typedef char sprite_size_check[sizeof(struct spritetype) == 38 ? 1 : -1];
typedef char sector_size_check[sizeof(struct sectortype) == 37 ? 1 : -1];

extern struct spritetype sprite[4096];
extern struct sectortype sector[1024];
extern short headspritestat[], nextspritestat[4096];

extern struct walltype wall[8192];
extern short headspritesect[], nextspritesect[4096];

#endif
