#include "actors.h"

/* Tile names from the prototype DEFS.CON. */
#define RADIUSEXPLOSION 46
#define MASTERSWITCH 51
#define SEENINE 57
#define FLAIR 174
#define SHOTSPARK1 191
#define LASERSITE 214
#define WATERBUBBLE 215
#define BULLET 389
#define BLOOD 488
#define RPG 563
#define REACTOR 620
#define HEAVYHBOMB 721
#define ARMPIECE 870
#define HEADPIECE 874
#define LEGPIECE 878
#define BOUNCEMINE 940
#define WATERDRIP 960
#define GREENSLIME 961
#define OOZ 1122
#define DOORSHOCK 1123
#define SHELL 1165
#define CRANE 1175
#define WATERSPLASH 1190
#define CANWITHSOMETHING 1200
#define OTHERSHOTSPARK 1210
#define CLUB 1230
#define SHARK 1280
#define EXPLOSION 1370
#define STICKSPARK 1404
#define BLUESPARK 1406
#define CAM1 1408

extern long msx[1024], msy[1024];
#pragma pack(1)
struct user_settings {          /* the 21-byte USERDEFS.TMP block defined by GLOBAL */
    unsigned char visibility, quotes, brightness, sound, music;
    short debug, sound_volume, music_volume, paused, camera, mouse_sensitivity;
    unsigned char reserved[4];
};
#pragma pack()
extern struct user_settings userdefs;

/* dataown: one object, one name. The spellings below are the GLOBAL definitions or fields of
   its settings block; the extern declarations above keep this unit's symbol slots. */
#define disable_armor userdefs.debug
#define music_volume userdefs.music_volume

void addweapon(short p, unsigned short weapon, unsigned char force)
{
    switch (weapon) {
    case CLUB:
        if (ps[p].gotweapon[0] == 0 || force) {
            ps[p].weapon_pos = 5;
            ps[p].last_weapon = ps[p].curr_weapon;
            ps[p].curr_weapon = CLUB;
        }
        ps[p].gotweapon[0] = 1;
        break;
    case SHOTSPARK1:
        if (ps[p].gotweapon[1] == 0 || force) {
            ps[p].weapon_pos = 5;
            ps[p].last_weapon = ps[p].curr_weapon;
            ps[p].curr_weapon = SHOTSPARK1;
        }
        ps[p].gotweapon[1] = 1;
        break;
    case BULLET:
        if (ps[p].gotweapon[2] == 0 || force) {
            ps[p].weapon_pos = 5;
            ps[p].last_weapon = ps[p].curr_weapon;
            ps[p].curr_weapon = BULLET;
        }
        ps[p].gotweapon[2] = 1;
        break;
    case HEAVYHBOMB:
        if (ps[p].gotweapon[3] == 0 || force) {
            ps[p].weapon_pos = 5;
            ps[p].last_weapon = ps[p].curr_weapon;
            ps[p].curr_weapon = HEAVYHBOMB;
        }
        ps[p].gotweapon[3] = 1;
        break;
    case RPG:
        if (ps[p].gotweapon[4] == 0 || force) {
            ps[p].weapon_pos = 5;
            ps[p].last_weapon = ps[p].curr_weapon;
            ps[p].curr_weapon = RPG;
        }
        ps[p].gotweapon[4] = 1;
        break;
    }
    ps[p].kickback_pic = 0;
}

void hitradius(short i, long radius, short damage)
{
    unsigned char statlist[3] = { 1, 6, 10 };
    short k, j, hit;
    long dx, dy, dy_squared;

    for (k = 0; k < 3; k++)
        for (j = headspritestat[statlist[k]]; j >= 0; j = nextspritestat[j]) {
            dy = sprite[i].y - sprite[j].y;
            dy_squared = dy * dy;
            dx = sprite[i].x - sprite[j].x;
            if (dx * dx + dy_squared < radius * radius) {
                sprite[i].ang = getangle(sprite[j].x - sprite[i].x, sprite[j].y - sprite[i].y);
                if (sprite[j].extra >= 0) {
                    hit = EGS(sprite[j].sectnum, sprite[j].x, sprite[j].y, sprite[j].z,
                        0, RADIUSEXPLOSION, -16, 0, 0, sprite[i].ang, 32, 0, sprite[i].owner, 4);
                    sprite[hit].extra = damage;
                }
            }
        }
}

void spawn_gib(short i, short tile, short ang, short speed, long zvel)
{
    EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z - (36 << 8),
        0, tile, -16, 24, 24, ang, speed, zvel, i, 5);
}

#define MOVE_ALONG(i, dz, clip) \
    movesprite((i), (sprite[(i)].xvel * sintable[(sprite[(i)].ang + 512) & 2047]) >> 14, \
        (sintable[(short)(sprite[(i)].ang & 2047)] * sprite[(i)].xvel) >> 14, \
        (dz), 128, 1024, 1024, (clip), synctics)

short ssp(short i, short cliptype)
{
    return MOVE_ALONG(i, sprite[i].zvel, cliptype) == 0;
}

void nuke(long x, long y, long z, short i)
{
    EGS(sprite[i].sectnum, x, y, z + (20 << 8), 2, FLAIR, -127, 96, 48, 0, 0, 0, i, 5);
    spawn(i, EXPLOSION);
}

void spawn_blood(short i, short at, short ang, short speed)
{
    EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[at].z,
        0, BLOOD, -16, 24, 13, ang + (krand() & 127),
        speed + (krand() & 31), -(krand() & 511), i, 5);
}

void movesector(short i)
{
    long ry, rx;
    short sn, w, end;
    long j;

    sprite[i].x += (sprite[i].xvel * sintable[(sprite[i].ang + 512) & 2047]) >> 14;
    sn = sprite[i].sectnum;
    sprite[i].y += (sprite[i].xvel * sintable[(short)(sprite[i].ang & 2047)]) >> 14;
    j = hittype[i][1];
    w = sector[sn].wallptr;
    end = sector[sn].wallnum + w;
    for (; w < end;) {
        long wx = msx[j];
        rotatepoint(0, 0, wx, msy[j++], hittype[i][2] & 2047, &rx, &ry);
        dragpoint(w++, rx + sprite[i].x, ry + sprite[i].y);
    }
    setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
}

void wakeactors(void)
{
    short i, nexti, p;
    long dist, z;

    for (i = headspritestat[2]; i >= 0; i = nexti) {
        nexti = nextspritestat[i];
        p = findplayer(i, &dist);
        if (cansee(sprite[i].x, sprite[i].y, sprite[i].z + (z = sprite[i].yrepeat * -128L),
                sprite[i].sectnum, ps[p].posx, ps[p].posy, ps[p].posz - (16 << 8),
                ps[p].cursectnum)) {
            switch ((unsigned short)sprite[i].picnum) {
            case 408:
                spritesound(21, i);
                FTA(0);
                break;
            case 490: case 1680:
                spritesound(20, i);
                FTA(0);
                break;
            case 579: case 1536:
                spritesound(22, i);
                FTA(3);
                break;
            case 38: case 1125:
                FTA(1);
                break;
            case REACTOR:
                FTA(2);
                break;
            case 60:
                FTA(6);
                break;
            }
            changespritestat(i, 1);
        }
    }
}

short ifsquished(short i, short p)
{
    short sn = sprite[i].sectnum;
    short result;

    if (sector[sn].floorz - sector[sn].ceilingz < (28 << 8)
        && (sector[sn].lotag & 0x8000) == 0 && (sector[sn].lotag & 0x7fff) != 0) {
        if ((short)(sector[sn].lotag & 0x7fff) != 23)
            sector[sn].ceilingz -= 4 << 8;
        result = i;
        sprite[i].z = sector[sprite[i].sectnum].floorz;
        switch ((unsigned short)sprite[i].picnum) {
        case 408: case 490: case 631: case 1536: case 1680: case 1820:
            ps[p].kills++;
            sprite[result].xvel = 0;
            sprite[result].cstat &= ~1;
            FTA(10);
            return result;
        case 579:
            sprite[i].xvel = 0;
            ps[p].kills++;
            FTA(10);
            return i;
        }
        return result;
    }
    return -1;
}

short effectinsect(short i)
{
    short j, sn = sprite[i].sectnum;

    for (j = headspritestat[5]; j >= 0; j = nextspritestat[j])
        if (sn == sprite[j].sectnum && sprite[j].picnum != LASERSITE)
            return j;
    return -1;
}

short ifhitbyweapon(short i)
{
    short j;

    for (j = headspritestat[4]; j >= 0; j = nextspritestat[j])
        if (i != j && i != sprite[j].owner
            && klabs(sprite[i].x - sprite[j].x) + klabs(sprite[i].y - sprite[j].y) < 768
            && klabs((sprite[i].z - sprite[j].z) >> 8) <= 48) {
            if (sprite[i].statnum != 3) {
                sprite[i].extra -= sprite[j].extra;
                sprite[i].xvel = sprite[j].xvel;
                sprite[i].ang = sprite[j].ang;
                if ((sprite[i].statnum == 1 || sprite[i].statnum == 10)
                    && sprite[j].picnum == BULLET)
                    spawn(i, STICKSPARK);
            }
            return j;
        }
    return -1;
}

void guts(short i)
{
    spawn_gib(i, ARMPIECE, sprite[i].ang + 1536, 64, -623);
    spawn_gib(i, ARMPIECE, sprite[i].ang - 512, 48, -447);
    sprite[i].z += 8 << 8;
    spawn_gib(i, LEGPIECE, sprite[i].ang + 512, 32, -508);
    spawn_gib(i, LEGPIECE, sprite[i].ang - 512, 24, -768);
    sprite[i].z -= 8 << 8;
    spawn_gib(i, HEADPIECE, sprite[i].ang - 1024, 32, -768);
}

#define FIND_IN_SECTOR(sn, tile, j) do { \
    for ((j) = headspritesect[(sn)]; (j) >= 0; (j) = nextspritesect[(j)]) \
        if (sprite[(j)].picnum == (tile)) \
            break; \
} while (0)

#define FALL(s) do { \
    if ((s)->z < sector[(s)->sectnum].floorz) { \
        if ((s)->zvel < 2048) \
            (s)->zvel += 256; \
        (s)->z += (s)->zvel; \
    } else { \
        (s)->zvel = 0; \
        (s)->z = sector[(s)->sectnum].floorz; \
    } \
} while (0)

#define HURT(p, damage) do { \
    if (disable_armor == 0 && ps[(p)].armor > 0) \
        ps[(p)].armor -= 4; \
    else if (ps[(p)].health > 0) \
        ps[(p)].health -= (damage); \
    else \
        ps[(p)].health = 0; \
} while (0)

void moveactors(void)
{
    short i, nexti, j, hit;

    for (i = headspritestat[1]; i >= 0; i = nexti) {
        long *t, dist, d;
        short p, sect;

        nexti = nextspritestat[i];
        p = findplayer(i, &dist);
        sect = sprite[i].sectnum;
        t = hittype[i];
        if (sprite[i].picnum == CAM1) {
            if (dist < 2048)
                viewscreen = i;
            continue;
        }
        if (sprite[i].picnum == MASTERSWITCH) {
            /* Fire an armed master switch when its countdown expires. */
            if (sprite[i].yvel != 1 || --sprite[i].hitag > 0)
                continue;
            operatesectors(sect, i);
            for (j = headspritesect[sect]; j >= 0; j = nextspritesect[j]) {
                if (sprite[j].statnum == 6 && sprite[j].picnum == SEENINE)
                    sprite[j].shade = -31;
                if (sprite[j].statnum != 3)
                    continue;
                if (sprite[j].lotag == 2 || sprite[j].lotag == 21)
                    hittype[j][0] = 1;
                else if (sprite[j].lotag == 3)
                    hittype[j][4] = 1;
                else if (sprite[j].lotag == 23)
                    sprite[i].shade = 1;
            }
            deletesprite(i);
            continue;
        }
        if (sprite[i].picnum == SHARK) {
            unsigned char frames[4] = { 0, 1, 0, 3 };
            playertype *pl = &ps[p];
            short turn;

            if (MOVE_ALONG(i, sprite[i].zvel, 0))
                sprite[i].ang += 1024;
            hittype[i][1]++;
            turn = ifhitbyweapon(i);
            if (turn >= 0) {
                if (sprite[i].extra < 0) {
                    ps[screenpeek].score += 5000;
                    spawn(i, EXPLOSION);
                    deletesprite(i);
                    continue;
                }
                hittype[i][2] = -1;
                sprite[i].ang += 1024;
            }
            switch (hittype[i][2]) {
            case 0:
                if ((krand() >> 8) >= 251) {
                    hittype[i][2] = 1;
                    goto shark_steer;
                }
                /* fall through */
            case -1:
                if (hittype[i][2] == -1)
                    hittype[i][2]++;
shark_steer:
                /* Preserve the hit index when the shark steers toward the player. */
                if ((krand() >> 8) < 247) {
                    turn = (signed char)(sintable[(16 * hittype[i][1]) & 2047] >> 8);
                    sprite[i].ang += turn;
                } else
                    sprite[i].ang += getincangle_shift(sprite[i].ang, getangle(pl->posx - sprite[i].x, pl->posy - sprite[i].y), 2);
                if (turn >= -32 && turn <= 32) {
                    t[0] = frames[hittype[i][1] & 3];
                    if (sprite[i].xvel < 112)
                        sprite[i].xvel += 8;
                } else {
                    t[0] = turn >= 0 ? 2 : 4;
                    if (sprite[i].xvel > 64)
                        sprite[i].xvel -= 8;
                }
                break;
            case 1:
                if (pl->posz + 768 < sprite[i].z)
                    sprite[i].z -= 1024;
                else if (pl->posz - 768 > sprite[i].z)
                    sprite[i].z += 1024;
                sprite[i].ang += getincangle_shift(sprite[i].ang, getangle(pl->posx - sprite[i].x, pl->posy - sprite[i].y), 3);
                if (sprite[i].xvel >= 144)
                    t[0] = 0;
                else {
                    t[0] = frames[hittype[i][1] & 3];
                    sprite[i].xvel += 16;
                }
                if (dist < 768) {
                    hittype[i][2] = (krand() >> 8) < 247 ? 0 : 2;
                    sprite[i].ang += 1024;
                }
                break;
            case 2:
                sprite[i].x = (sintable[(pl->ang + 512) & 2047] >> 6) + pl->posx;
                sprite[i].y = pl->posy + (sintable[pl->ang & 2047] >> 6);
                sprite[i].z = pl->posz + 3072;
                t[0] = frames[hittype[i][1] & 3];
                pl->ang += sintable[((unsigned short)hittype[i][1] << 8) & 2047] >> 12;
                sprite[i].ang = pl->ang + 1024;
                if ((krand() >> 8) >= 223)
                    hittype[i][2] = 0;
                break;
            }
            continue;
        }
        if (sprite[i].picnum == OOZ) {
            if (++t[0] > 1024 || dist > 0x2000) {
                deletesprite(i);
                continue;
            }
            d = klabs(sector[sect].ceilingz - sector[sect].floorz);
            sprite[i].yrepeat = (unsigned char)(d >> 9);
            sprite[i].xrepeat = (unsigned char)(25 - ((short)(d >> 9) >> 1));
            sprite[i].z = sector[sect].floorz;
            continue;
        }
        if (sprite[i].picnum >= 961 && sprite[i].picnum <= 968) {
            short bite[8] = { 5, 5, 6, 6, 7, 7, 6, 5 };
            playertype *pl = &ps[p];
            short j, xv;

            hittype[i][1] += 32;
            if (hittype[i][0] == -4) {
                setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
                sprite[i].cstat &= ~8;
                sprite[i].ang = pl->ang;
                if (syncbits[p] & 4) {
                    pl->kickback_pic = 0;
                    sprite[i].xvel = 128;
                    sprite[i].zvel = (short)(16 * (100 - pl->horiz) - 512);
                    hittype[i][0] = 0;
                    pl->face_parasite = -1;
                }
                sprite[i].z = pl->posz - hittype[i][2] + 2048;
                sprite[i].z += 16 * (100 - pl->horiz);
                if (hittype[i][2] > 512)
                    hittype[i][2] -= 128;
                if (hittype[i][2] < 348)
                    hittype[i][2] += 128;
                if (hittype[i][3] <= 0) {
                    sprite[i].picnum = 966;
                    if ((krand() >> 8) >= 223)
                        hittype[i][3] = 1;
                } else {
                    sprite[i].picnum = bite[hittype[i][3]] + 961;
                    if (hittype[i][3] == 4) {
                        HURT(p, 5);
                        spritesound(13, pl->i);
                    }
                    hittype[i][3] = hittype[i][3] >= 7 ? 0 : hittype[i][3] + 1;
                }
                sprite[i].xrepeat = (unsigned char)((sintable[hittype[i][1] & 2047] >> 13) + 20);
                sprite[i].yrepeat = (unsigned char)((sintable[hittype[i][1] & 2047] >> 13) + 15);
                sprite[i].x = pl->posx + (sintable[(pl->ang + 512) & 2047] >> 7);
                sprite[i].y = (sintable[pl->ang & 2047] >> 7) + pl->posy;
                continue;
            }
            if (sprite[i].xvel < 64 && dist < 512 && pl->face_parasite == -1) {
                pl->face_parasite = i;
                hittype[i][2] = (hittype[i][0] == 3 || hittype[i][0] == 2) ? 3072 : -3328;
                hittype[i][0] = -4;
            } else if (hittype[i][0] == -3) {
                if (sprite[i].xrepeat > 4)
                    sprite[i].xrepeat -= 4;
                if (sprite[i].yrepeat > 4)
                    sprite[i].yrepeat -= 4;
                else
                    deletesprite(i);
                continue;
            }
            j = ifhitbyweapon(i);
            if (j >= 0) {
                pl->score += 1000;
                if (sprite[j].picnum == BULLET) {
                    hittype[i][0] = -3;
                    continue;
                }
                if ((krand() >> 8) >= 251 && klabs(sector[sect].floorz - sector[sect].ceilingz) < 49152
                    && (hittype[i][0] == 0 || hittype[i][0] == 2))
                    hittype[i][0]++;
            }
            if (hittype[i][0] == -1) {
                FALL(&sprite[i]);
                sprite[i].picnum = 965;
                sprite[i].cstat &= 0xF7;
                if (sprite[i].yrepeat > 62)
                    guts(i);
                if (sprite[i].xrepeat > 32)
                    sprite[i].xrepeat -= krand() & 7;
                if (sprite[i].yrepeat <= 16) {
                    sprite[i].xrepeat = 40;
                    sprite[i].yrepeat = 16;
                    sprite[i].owner = -1;
                    hittype[i][0] = 0;
                } else
                    sprite[i].yrepeat -= krand() & 7;
                continue;
            }
            if (hittype[i][0] == -2) {
                spritetype *o;

                FALL(&sprite[i]);
                o = &sprite[sprite[i].owner];
                o->xvel = 0;
                sprite[i].z = o->z;
                sprite[i].x = (sintable[(o->ang + 512) & 2047] >> 11) + o->x;
                sprite[i].y = (sintable[o->ang & 2047] >> 11) + o->y;
                sprite[i].picnum = (short)((randomstash & 1) + 963);
                if (sprite[i].yrepeat < 64)
                    sprite[i].yrepeat += 2;
                else if (sprite[i].xrepeat < 32)
                    sprite[i].xrepeat += 4;
                else {
                    hittype[i][0] = -1;
                    if (klabs(sprite[i].x - o->x) + klabs(sprite[i].y - o->y) < 768)
                        deletesprite(sprite[i].owner);
                }
                continue;
            }
            if ((krand() >> 8) >= 223)
                for (j = headspritesect[sect]; j >= 0; j = nextspritesect[j]) {
                    short pic = sprite[j].picnum;

                    if (pic != 408 && pic != 490 && pic != 1536 && pic != 1680 && pic != 1820)
                        continue;
                    if (klabs(sprite[i].x - sprite[j].x) + klabs(sprite[i].y - sprite[j].y) < 768 && klabs(sprite[i].z - sprite[j].z) < 0x2000) {
                        sprite[i].owner = j;
                        hittype[i][0] = -2;
                        hittype[i][1] = 0;
                        continue;
                    }
                }
            if (hittype[i][0] == 0 || hittype[i][0] == 2) {
                sprite[i].picnum = GREENSLIME;
                if (hittype[i][0] == 2) {
                    sprite[i].zvel = 0;
                    sprite[i].cstat &= ~8;
                    if (sector[sect].ceilingz + 6144 < sprite[i].z) {
                        sprite[i].z += 2048;
                        hittype[i][0]++;
                        continue;
                    }
                } else {
                    sprite[i].cstat |= 8;
                    FALL(&sprite[i]);
                }
                MOVE_ALONG(i, sprite[i].zvel, 0);
                xv = sprite[i].xvel;
                if (xv > 96) {
                    sprite[i].xvel = xv - 2;
                    continue;
                }
                sprite[i].xvel = (short)(64 - (sintable[(hittype[i][1] + 128) & 2047] >> 9));
                sprite[i].ang += getincangle_shift(sprite[i].ang, getangle(pl->posx - sprite[i].x, pl->posy - sprite[i].y), 3);
                sprite[i].xrepeat = (unsigned char)((sintable[(hittype[i][1] + 128) & 2047] >> 11) + 36);
                sprite[i].yrepeat = (unsigned char)((sintable[hittype[i][1] & 2047] >> 13) + 16);
                if ((krand() >> 8) >= 251 && klabs(sector[sect].floorz - sector[sect].ceilingz) < 49152) {
                    sprite[i].zvel = 0;
                    hittype[i][0]++;
                }
            }
            if (hittype[i][0] == 1) {
                sprite[i].picnum = GREENSLIME;
                if (sprite[i].yrepeat < 40)
                    sprite[i].yrepeat += 8;
                if (sprite[i].xrepeat > 8)
                    sprite[i].xrepeat -= 4;
                if (sprite[i].zvel > -3072)
                    sprite[i].zvel -= 348;
                sprite[i].z += sprite[i].zvel;
                if (sector[sect].ceilingz + 4096 > sprite[i].z) {
                    sprite[i].z = sector[sect].ceilingz + 4096;
                    sprite[i].xvel = 0;
                    hittype[i][0]++;
                }
            }
            if (hittype[i][0] == 3) {
                sprite[i].picnum = 962;
                if (sprite[i].zvel < 3072)
                    sprite[i].zvel += 348;
                sprite[i].z += sprite[i].zvel;
                if (sector[sect].floorz - 0x2000 >= sprite[i].z) {
                    if (sprite[i].yrepeat < 36)
                        sprite[i].yrepeat += 8;
                    if (sprite[i].xrepeat > 8)
                        sprite[i].xrepeat -= 4;
                } else {
                    sprite[i].yrepeat -= 4;
                    sprite[i].xrepeat += 2;
                }
                if (sector[sect].floorz - 2048 < sprite[i].z) {
                    sprite[i].z = sector[sect].floorz - 2048;
                    hittype[i][0] = 0;
                    sprite[i].xvel = 0;
                }
            }
        } else if (sprite[i].picnum == WATERDRIP) {
            /* A drip: grows on the ceiling, falls, splashes. */
            if (dist > 0x2000)
                continue;
            if (hittype[i][0] == 0) {
                if (hittype[i][1] > 0)
                    hittype[i][1]--;
                else {
                    sprite[i].yrepeat++;
                    sprite[i].xrepeat -= 2;
                    if (sprite[i].yrepeat > 8) {
                        hittype[i][0] = 1;
                        sprite[i].xrepeat = 8;
                    }
                }
            }
            if (hittype[i][0] == 1) {
                FALL(&sprite[i]);
                if (sector[sect].floorz - 2048 < sprite[i].z) {
                    hittype[i][0] = 2;
                    spritesound(17, i);
                }
            }
            if (hittype[i][0] == 2) {
                sprite[i].xrepeat += 8;
                sprite[i].yrepeat -= 2;
                if (sprite[i].yrepeat < 2) {
                    sprite[i].z = sector[sect].ceilingz + 512;
                    hittype[i][0] = 0;
                    hittype[i][1] = krand() & 31;
                    sprite[i].xrepeat = 16;
                    sprite[i].yrepeat = 0;
                }
            }
        } else if (sprite[i].picnum == 930 || sprite[i].picnum == 931)
            {
                playertype *pl = &ps[p];

                hit = ifhitbyweapon(i);
                if (hit >= 0) {
                    hittype[i][4] = 0;
                    if (sprite[i].extra < 0) {
                        pl->score += 1000;
                        spawn(i, EXPLOSION);
                        /* the exe counts this loop in its distance variable */
                        for (dist = 0; dist < 30; dist += 3) {
                            short zv = (short)(-(krand() & 511) - 512), xv = (short)((krand() & 128) + 64), a = (short)(krand() & 2047);

                            EGS(sect, sprite[i].x, sprite[i].y, sprite[i].z, 0, (short)(krand() % 11 + 235), -8, 32, 32, a, xv, zv, i, 5);
                        }
                        pl->turret_kills++;
                        t[0] = 2;
                        sprite[i].xrepeat = sprite[i].yrepeat = 0;
                    }
                }
                if (t[0] == 0) {
                    hittype[i][4] = 0;
                    if (sprite[i].xrepeat == 0) {
                        deletesprite(i);
                        continue;
                    }
                    if (dist < 4096) {
                        if (klabs(sprite[i].z - pl->posz) >= 2048) {
                            short step = (short)(pl->posz - sprite[i].z > 0 ? 2048 : -2048);

                            sprite[i].z += step;
                            if (sprite[i].picnum == 930)
                                sector[sect].floorz += step;
                            else
                                sector[sect].ceilingz += step;
                        } else {
                            hittype[i][1] = 0;
                            t[0]++;
                        }
                    }
                }
                if (t[0] == 1) {
                    sprite[i].ang += 2 * getincangle_shift(sprite[i].ang, getangle(pl->posx - sprite[i].x, pl->posy - sprite[i].y), 1);
                    sprite[i].ang &= (short)0xFF80;
                    if (++hittype[i][1] >= 8) {
                        sprite[i].shade = 4;
                        hittype[i][1] = 0;
                        if (dist > 1024) {
                            t[0] = 4;
                            goto wait;
                        }
                    } else {
                        hittype[i][4] ^= 1;
                        if (hittype[i][4]) {
                            sprite[i].z -= 0x2000;
                            shoot(i, BULLET, 0);
                            sprite[i].shade = -16;
                            sprite[i].z += 0x2000;
                        }
                    }
                }
                if (t[0] == 2) {
                    hittype[i][4] = 0;
                    if (sprite[i].picnum == 930) {
                        sprite[i].z += 2048;
                        sector[sect].floorz += 2048;
                        if (sector[sect].floorz > hittype[i][3]) {
                            t[0] = 0;
                            continue;
                        }
                    } else {
                        sprite[i].z -= 2048;
                        sector[sect].ceilingz -= 2048;
                        if (sector[sect].ceilingz < hittype[i][3]) {
                            t[0] = 0;
                            continue;
                        }
                    }
                }
                if (t[0] <= 3)
                    continue;
            wait:
                t[0]++;
                if (dist > 4096)
                    t[0] = 2;
                else if (t[0] > 10)
                    t[0] = 1;
                sprite[i].ang = (short)getangle(pl->posx - sprite[i].x, pl->posy - sprite[i].y);
            } else if (sprite[i].picnum == HEAVYHBOMB) {
            /* shortcut: the exe reads hittype[-1] for an ownerless bomb; ownerless ones use player 0 */
            long own = hittype[sprite[i].owner][0];
            short a;

            if (sector[sect].floorz - sector[sect].ceilingz < (28 << 8)
                && !(sector[sect].lotag & 0x8000) && (sector[sect].lotag & 0x7fff)) {
                if ((sector[sect].lotag & 0x7fff) != 23)
                    sector[sect].ceilingz -= 4 << 8;
                sprite[i].z = sector[sprite[i].sectnum].floorz;
                switch ((unsigned short)sprite[i].picnum) {
                case 408: case 490: case 631: case 1536: case 1680: case 1820:
                    sprite[i].xvel = 0;
                    sprite[i].cstat &= ~1;
                    ps[p].kills++;
                    FTA(10);
                    break;
                case 579:
                    sprite[i].xvel = 0;
                    ps[p].kills++;
                    FTA(10);
                    break;
                }
                if (sprite[i].extra < 0)
                    ps[own].remote_active = 0;
            }
            FALL(&sprite[i]);
            if (sprite[i].xvel <= 0)
                sprite[i].xvel = 0;
            else {
                sprite[i].xvel -= 4;
                if (sprite[i].xvel & 8)
                    sprite[i].cstat ^= 4;
            }
            MOVE_ALONG(i, sprite[i].zvel, 0);
            if (ps[own].remote_active || ps[own].i != sprite[i].owner) {
                if (dist > 512 && dist < 768) {
                    ps[p].remote_ammo++, addweapon(p, 721, 1);
                    deletesprite(i);
                }
                continue;
            }
            sprite[i].yvel++;
            if (sprite[i].xrepeat || sprite[i].yrepeat) {
                sprite[i].xrepeat = sprite[i].yrepeat = 0;
                hitradius(i, 3092, sprite[i].extra);
            }
            if (sprite[i].yvel & 1)
                for (a = 0; a < 2047; a += 256)
                    EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z - (sprite[i].yvel << 10), 0, 1370, -127, 48, 48, (short)a, 0, 0, i, 5);
            if (sprite[i].yvel > 20)
                deletesprite(i);
        } else if (sprite[i].picnum == DOORSHOCK) {
            if (dist <= 0x2000) {
                d = klabs(sector[sect].ceilingz - sector[sect].floorz);
                sprite[i].xrepeat = 16;
                sprite[i].yrepeat = (unsigned char)((d >> 9) + 8);
                sprite[i].z = sector[sect].floorz;
            }
        } else if (sprite[i].picnum == 32) {
            long range = (sprite[i].lotag + (sprite[i].hitag << 16)) >> 13;

            if (range != 0 && sprite[i].hitag != 0) {
                if (dist >= range || klabs(sprite[i].z - ps[p].posz) >= 12288)
                    t[0] = 0;
                else if (t[0] == 0) {
                    t[0] = 1;
                    operatemasterswitches(sprite[i].lotag);
                    operateactivators(sprite[i].lotag);
                }
            } else {
                short occupant = checkcursectnums(sect);

                if (occupant < 0 || !ps[occupant].on_ground)
                    t[0] = 0;
                else if (t[0] == 0) {
                    t[0] = 1;
                    operatemasterswitches(sprite[i].lotag);
                    operateactivators(sprite[i].lotag);
                }
            }
        } else if (sprite[i].picnum >= 305 && sprite[i].picnum <= 308)
            {

                if (dist > 0x2000)
                    continue;
                if (hittype[i][3] == 0)
                    hittype[i][3] = sector[sect].floorshade;
                for (;;) {
                    if (hittype[i][2]) {
                        hittype[i][2]--;
                        sector[sect].floorshade = sector[sect].ceilingshade = 20;
                        continue;
                    }
                    if ((sprite[i].yrepeat | sprite[i].xrepeat) == 0) {
                        sprite[i].xrepeat = (unsigned char)t[0];
                        sprite[i].yrepeat = (unsigned char)hittype[i][1];
                        break;
                    }
                    if (krand() & 8)
                        break;
                    t[0] = sprite[i].xrepeat;
                    hittype[i][1] = sprite[i].yrepeat;
                    hittype[i][2] = randomstash & 4;
                    sprite[i].xrepeat = sprite[i].yrepeat = 0;
                }
                sprite[i].xrepeat = (unsigned char)((randomstash & 7) + 8);
                sprite[i].picnum++;
                if (randomstash & 1)
                    sprite[i].cstat ^= 2;
                if (dist < 512)
                    spritesound(24, i);
                if (sprite[i].picnum == 309)
                    sprite[i].picnum = 305;
                sector[sect].floorshade = sector[sect].ceilingshade = (sprite[i].picnum & 1) ? 0 : 20;
            } else if (sprite[i].picnum >= 620 && sprite[i].picnum <= 625) {
            short k;
            long n;

            sprite[i].picnum = (short)(((krand() >> 8) < 63 ? 623 : 620) + (short)hittype[i][2]);
            if (++hittype[i][2] == 3)
                hittype[i][2] = 0;
            if (dist >= 4096) {
                hittype[i][3] = 0;
                stopsound(26);
            } else {
                t[0] += 128;
                draw_weapon_tile(246, 18, 611, (sintable[t[0] & 2047] >> 11) + 8, 1);
                if (hittype[i][3] == 0) {
                    spritesound(26, i);
                    hittype[i][3] = 1;
                }
            }
            if (hittype[i][1]) {
                if (++hittype[i][1] == 24) {
                    ps[screenpeek].score += 100000;
                    stopsound(26);
                    deletesprite(i);
                    continue;
                }
                sprite[i].ang += 300;
                sprite[i].z -= 6144;
                shoot(i, 0, 1);
                sprite[i].z += 6144;
                n = hittype[i][1];
                if (n == 2)
                    for (k = headspritestat[3]; k >= 0; k = nextspritestat[k])
                        if (sprite[k].lotag == 3)
                            hittype[k][4] = 1;
                if (n == 2 || n == 4 || n == 7 || n == 10 || n == 15) {
                    EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z - (krand() & 0x3FFF) + 5120, 2, FLAIR, -127, 96, 48, 0, 0, 0, i, 5);
                    spawn(i, EXPLOSION);
                    for (k = 0; k < 16; k += 2) {
                        short zv = (short)-(krand() & 1023);

                        EGS(sect, sprite[i].x, sprite[i].y, sprite[i].z + (k << 9), 0, 235, -8, 32, 32, sprite[i].ang, (krand() & 255) + 128, zv, i, 5);
                    }
                }
            }
            if (hittype[i][1] == 0) {
                hit = ifhitbyweapon(i);
                if (hit >= 0 && sprite[i].extra < 0)
                    hittype[i][1] = 1;
            }
        } else if (sprite[i].picnum == 265) {
            sprite[i].owner += 8;
            if (sprite[i].owner < 128)
                sprite[i].ang += 8;
            else if (sprite[i].owner < 384)
                sprite[i].ang -= 8;
            else if (sprite[i].owner < 512)
                sprite[i].ang += 8;
            else {
                sprite[i].owner = 8;
                sprite[i].ang += 16;
            }
        } else if (sprite[i].picnum == 263)
            {
                playertype *pl = &ps[p];

                if (dist > 4096)
                    continue;
                hit = ifhitbyweapon(i);
                if (hit >= 0 && sprite[i].extra < 0) {
                    sprite[i].picnum = 273;
                    t[0] = 0;
                    sprite[i].cstat &= ~1;
                    continue;
                }
                switch (t[0]) {
                case 0:
                    if (dist < 4096)
                        t[0]++;
                    break;
                case 1:
                    t[0]++;
                    sprite[i].ang = (short)getangle(pl->posx - sprite[i].x, pl->posy - sprite[i].y);
                    if (dist != 0)
                        sprite[i].zvel = (short)(((pl->posz + 2048 - sprite[i].z) << 8) / dist);
                    sprite[i].xvel = 16;
                    break;
                case 2:
                    if (sprite[i].zvel > -1024)
                        sprite[i].zvel -= 64;
                    sprite[i].z += sprite[i].zvel;
                    if (sprite[i].xvel < 256)
                        sprite[i].xvel += 16;
                    if (MOVE_ALONG(i, sprite[i].zvel, 0))
                        t[0]++;
                    if (dist < 768) {
                        sprite[i].ang += 1024 - (randomstash & 2047);
                        HURT(p, 1);
                    }
                    break;
                case 3:
                    if (MOVE_ALONG(i, sprite[i].zvel, 0)) {
                        sprite[i].ang += 512;
                        t[0] = 1;
                    }
                    break;
                }
            } else if (sprite[i].picnum == 273) {
            if (dist > 4096)
                continue;
            FALL(&sprite[i]);
            if (++t[0] > 1024)
                deletesprite(i);
            else if (MOVE_ALONG(i, sprite[i].zvel, 0)) {
                sprite[i].z = sector[sect].floorz;
                if (sector[sect].lotag == 1)
                    deletesprite(i);
            }
        } else
            execute(i, p, dist);
    }
}

void moveweapons(void)
{
    /* dist is a short: the original compares and passes the low word of the
       findplayer distance. */
    short p, i, dist, nexti;

    for (i = headspritestat[4]; i >= 0; i = nexti) {
        nexti = nextspritestat[i];
        p = findplayer(i, (long *)&dist);
        if (sprite[i].picnum == BLUESPARK) {
            execute(i, p, dist);
            continue;
        }
        if (sprite[i].picnum == RADIUSEXPLOSION) {
            if (sprite[i].shade == -15) {
                shoot(i, CLUB, 1);
                if (dist < 512 && sprite[i].owner != ps[p].i) {
                    kickview(p, i);
                    HURT(p, sprite[i].extra);
                }
            } else
                sprite[i].shade = -15;
            execute(i, p, dist);
            continue;
        }
        if (sprite[i].picnum == BOUNCEMINE) {
            execute(i, p, dist);
            continue;
        }
        if (sprite[i].picnum == RPG) {
            if (sprite[i].extra >= 0) {
                if (sprite[i].owner != ps[p].i && dist < 768
                    && klabs((ps[p].posz - sprite[i].z) >> 8) <= 48) {
                    spritesound(13, ps[p].i);
                    kickview(p, i);
                    HURT(p, sprite[i].extra);
                    sprite[i].extra = -1;
                }
                if (MOVE_ALONG(i, sprite[i].zvel, 1)) {
                    shoot(i, RPG, 1);
                    sprite[i].extra = -1;
                }
            }
            execute(i, p, dist);
            continue;
        }
        if (sprite[i].picnum == BULLET) {
            if (sprite[i].extra >= 0) {
                if (dist < 768 && sprite[i].owner != ps[p].i
                    && klabs((ps[p].posz - sprite[i].z) >> 8) <= 48) {
                    HURT(p, sprite[i].extra);
                    kickview(p, i);
                    spritesound(13, ps[p].i);
                    sprite[i].extra = -1;
                    continue;
                }
                if (MOVE_ALONG(i, sprite[i].zvel, 1)) {
                    shoot(i, BULLET, 1);
                    sprite[i].extra = -1;
                }
            }
            execute(i, p, dist);
        }
        if (sprite[i].picnum == SHOTSPARK1) {
            if (dist < 512)
                kickview(p, i);
            if (sprite[i].shade == -15)
                shoot(i, 191, 1);
            else
                sprite[i].shade = -15;
            execute(i, p, dist);
        } else if (sprite[i].picnum == OTHERSHOTSPARK) {
            if (dist < 512)
                kickview(p, i);
            if (sprite[i].shade == -15) {
                shoot(i, OTHERSHOTSPARK, 1);
                if (sprite[i].yvel >= 0)
                    spritesound(30, i);
            } else
                sprite[i].shade = -15;
            execute(i, p, dist);
        }
    }
}

void moveeffects(void)
{
    short i, nexti;

    for (i = headspritestat[5]; i >= 0; i = nexti) {
        long *t = hittype[i], dist;
        short sect, p;

        nexti = nextspritestat[i];
        p = findplayer(i, &dist);
        sect = sprite[i].sectnum;
        switch (sprite[i].picnum) {
        case WATERBUBBLE:
            sprite[i].ang = krand() & 2047;
            sprite[i].xvel = (short)(16 - (krand() & 31));
            /* fall through */
        case LASERSITE:
        case BLOOD:
        case 686:
        case 1359:
        case 1360:
        case EXPLOSION:
        case STICKSPARK:
            execute(i, p, dist);
            continue;
        }
        if (sprite[i].picnum == SHELL) {
            if (ssp(i, 0)) {
                if (sprite[i].zvel < 512)
                    sprite[i].zvel += 64;
                if (sprite[i].xvel > 0) {
                    sprite[i].xvel -= 4;
                    if (sprite[i].xvel < 0)
                        sprite[i].xvel = 0;
                } else if (sprite[i].xvel < 0) {
                    sprite[i].xvel += 4;
                    if (sprite[i].xvel > 0)
                        sprite[i].xvel = 0;
                }
            } else if (t[1]) {
                deletesprite(i);
            } else {
                sprite[i].xvel = (short)-sprite[i].xvel;
                sprite[i].zvel = (short)-sprite[i].zvel;
                t[1] = 1;
            }
            continue;
        }
        if (sprite[i].picnum == WATERSPLASH) {
            if ((sector[sect].lotag & 0x7FFF) != 1 || (t[0] += 128) > 2047) {
                deletesprite(i);
                continue;
            }
            sprite[i].cstat ^= 4;
            sprite[i].xrepeat = sprite[i].yrepeat = (unsigned char)((sintable[t[0]] >> 11) + 8);
        }
        if (sprite[i].picnum >= 970 && sprite[i].picnum <= 973) {
            FALL(&sprite[i]);
            ssp(i, 0);
            if (klabs(sprite[i].z - sector[sect].floorz) < 2048) {
                sprite[i].zvel = -1024;
                sprite[i].xrepeat >>= 1;
                sprite[i].yrepeat >>= 1;
                t[0]++;
            }
            if (t[0] > 2) {
                deletesprite(i);
                continue;
            }
        }
        if (sprite[i].picnum >= 1201 && sprite[i].picnum <= 1205) {
            FALL(&sprite[i]);
            if (sprite[i].xvel <= 0) {
                sprite[i].xvel = 0;
                if ((krand() >> 8) >= 127)
                    sprite[i].cstat ^= 0x80;
            } else {
                if (sprite[i].zvel > 512) {
                    if (sprite[i].picnum == 1204)
                        sprite[i].picnum = 1202;
                    if (sprite[i].picnum == 1205)
                        sprite[i].picnum = 1203;
                }
                if (sprite[i].picnum == 1202 || sprite[i].picnum == 1203)
                    sprite[i].cstat ^= 4;
                sprite[i].xvel -= 6;
                if ((krand() >> 8) >= 191)
                    spawn(i, 686);
            }
            if (MOVE_ALONG(i, sprite[i].zvel, 0) && sector[sect].floorz - 1024 < sprite[i].z) {
                sprite[i].zvel = (short)-sprite[i].zvel;
                if (++t[0] == 3) {
                    sprite[i].z = sector[sect].floorz - 512;
                    changespritestat(i, 0);
                    continue;
                }
            }
        }
        if (sprite[i].picnum >= 870 && sprite[i].picnum <= 873) {
            if (hittype[i][2]) {
                if (hittype[i][4] < 256)
                    hittype[i][4]++;
                else if (dist > 4096)
                    deletesprite(i);
                continue;
            }
            t[0]++;
            if (t[0] & 1)
                sprite[i].picnum++;
            if (sprite[i].picnum == HEADPIECE)
                sprite[i].picnum = ARMPIECE;
            sprite[i].xvel = sprite[i].xvel > 0 ? (short)(sprite[i].xvel - 1) : 0;
            sprite[i].zvel = (short)(sprite[i].zvel + 192);
            if (MOVE_ALONG(i, sprite[i].zvel, 0)) {
                t[1]++;
                sprite[i].zvel = (short)-sprite[i].zvel;
            }
            if ((krand() >> 8) >= 239) {
                long speed = krand() & 32;

                spawn_blood(i, i, (short)t[0], speed);
            }
            if (sprite[i].z + 2048 > sector[sect].floorz && t[1] > 1) {
                sprite[i].picnum = 871;
                sprite[i].z = sector[sect].floorz - 512;
                hittype[i][2] = 1;
                sprite[i].cstat |= krand() & 4;
            }
        }
        if (sprite[i].picnum >= 874 && sprite[i].picnum <= 877) {
            if (hittype[i][2]) {
                if (hittype[i][4] < 256)
                    hittype[i][4]++;
                else if (dist > 4096)
                    deletesprite(i);
                continue;
            }
            t[0]++;
            if (t[0] & 1)
                sprite[i].picnum++;
            if (sprite[i].picnum == LEGPIECE)
                sprite[i].picnum = HEADPIECE;
            sprite[i].xvel = sprite[i].xvel > 0 ? (short)(sprite[i].xvel - 1) : 0;
            sprite[i].zvel = (short)(sprite[i].zvel + 128);
            if (MOVE_ALONG(i, sprite[i].zvel, 0)) {
                t[1]++;
                sprite[i].zvel = (short)-sprite[i].zvel;
            }
            if ((krand() >> 8) >= 239) {
                long speed = krand() & 32;

                spawn_blood(i, i, (short)t[0] << 7, speed);
            }
            if (sprite[i].z + 2048 > sector[sect].floorz && t[1] > 1) {
                sprite[i].picnum = 875;
                sprite[i].z = sector[sect].floorz - 512;
                hittype[i][2] = 1;
                sprite[i].cstat |= krand() & 4;
            }
        }
        if (sprite[i].picnum >= 878 && sprite[i].picnum <= 881) {
            if (hittype[i][2]) {
                if (hittype[i][4] < 256)
                    hittype[i][4]++;
                else if (dist > 4096)
                    deletesprite(i);
                continue;
            }
            t[0]++;
            if (t[0] & 1)
                sprite[i].picnum++;
            if (sprite[i].picnum == 882)
                sprite[i].picnum = LEGPIECE;
            sprite[i].xvel = sprite[i].xvel > 0 ? (short)(sprite[i].xvel - 1) : 0;
            sprite[i].zvel = (short)(sprite[i].zvel + 192);
            if (MOVE_ALONG(i, sprite[i].zvel, 0)) {
                t[1]++;
                sprite[i].zvel = (short)-sprite[i].zvel;
            }
            if ((krand() >> 8) >= 239) {
                long speed = krand() & 32;

                spawn_blood(i, i, (short)t[0] << 7, speed);
            }
            if (sprite[i].z + 4096 > sector[sect].floorz && t[1] > 1) {
                sprite[i].picnum = 879;
                sprite[i].z = sector[sect].floorz - 512;
                hittype[i][2] = 1;
                sprite[i].cstat |= krand() & 4;
            }
        }
        if (sprite[i].picnum == FLAIR) {
            long a;

            if (t[0] == 0)
                t[1] = (krand() & 3) + 1;
            t[0]++;
            sprite[i].xrepeat = sprite[i].xrepeat == 96 ? 191 : (unsigned char)(sprite[i].xrepeat - 48);
            sprite[i].yrepeat -= 3;
            if (t[0] == t[1])
                for (a = 0; a < 2048; a += 768) {
                    sprite[i].ang = (short)((unsigned char)randomstash + a);
                    spawn(i, EXPLOSION);
                }
            if (t[0] > 3)
                deletesprite(i);
            continue;
        }
        if (sprite[i].picnum >= 235 && sprite[i].picnum <= 246) {
            short old = sprite[i].picnum;

            sprite[i].picnum = old == 238 ? 235 : old == 242 ? 239 : old == 246 ? 243 : (short)(old + 1);
            if (klabs(sector[sect].ceilingz - sector[sect].floorz) < 7680)
                sprite[i].xvel = 64;
            if (ssp(i, 1) && sprite[i].z < sector[sect].floorz) {
                if ((sprite[i].zvel >> 2) < 1024)
                    sprite[i].zvel += 128;
                sprite[i].z += sprite[i].zvel;
                if (sprite[i].xvel > 16)
                    sprite[i].xvel -= 4;
            } else if (++t[0] != 2) {
                sprite[i].zvel = (short)-sprite[i].zvel;
            } else {
                deletesprite(i);
            }
        }
    }
}

void moveplayers(void)
{
    short i, nexti;

    for (i = headspritestat[10]; i >= 0; i = nexti) {
        nexti = nextspritestat[i];
        if (sprite[i].owner >= 0) {
            if (ps[sprite[i].yvel].newowner >= 0) {
                parallaxtype = 2;
                sprite[i].x = ps[sprite[i].yvel].oldx;
                sprite[i].y = ps[sprite[i].yvel].oldy;
                sprite[i].z = ps[sprite[i].yvel].oldz + (36 << 8);
                sprite[i].ang = ps[sprite[i].yvel].oldang;
            } else {
                execute(i, sprite[i].yvel, 0);
                ps[sprite[i].yvel].muzzle_flash = 0;
                sprite[i].x = ps[sprite[i].yvel].posx;
                sprite[i].y = ps[sprite[i].yvel].posy;
                sprite[i].z = ps[sprite[i].yvel].posz + (36 << 8);
                sprite[i].ang = ps[sprite[i].yvel].ang;
            }
            setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
        } else if (ps[sprite[i].yvel].holoduke == 0) {
            deletesprite(i);
        } else {
            if (sprite[i].z < sector[sprite[i].sectnum].floorz) {
                if (sprite[i].zvel < 2048)
                    sprite[i].zvel += 256;
                sprite[i].z += sprite[i].zvel;
            } else {
                sprite[i].zvel = 0;
                sprite[i].z = sector[sprite[i].sectnum].floorz;
            }
            if ((krand() >> 8) >= 239) {
                sprite[i].shade = 0;
                sprite[i].cstat |= 2;
            } else {
                sprite[i].shade = 4;
                sprite[i].cstat &= 0xfd;
            }
            if (sprite[i].yvel < 8) {
                sprite[i].xvel = 128;
                sprite[i].ang = ps[sprite[i].yvel].ang;
                sprite[i].yvel++;
                movesprite(i, (sprite[i].xvel * sintable[(sprite[i].ang + 512) & 2047]) >> 14,
                    (sintable[(short)(sprite[i].ang & 2047)] * sprite[i].xvel) >> 14,
                    sprite[i].zvel, 128, 1024, 1024, 0, synctics);
            } else {
                sprite[i].ang = 2047 - ps[sprite[i].yvel].ang;
                setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
            }
        }
    }
}

void movemusic(void)
{
    short i;
    long dist;
    char *song;

    for (i = headspritestat[11]; i >= 0; i = nextspritestat[i]) {
        if (sprite[i].lotag > 999) {
            dist = (klabs(sprite[ps[screenpeek].i].x - sprite[i].x)
                + klabs(sprite[ps[screenpeek].i].y - sprite[i].y));
            if ((dist >> 3) < sprite[i].hitag) {
                if (hittype[i][0] == 0) {
                    spritesound(sprite[i].lotag - 1000, i);
                    hittype[i][0] = 1;
                }
            } else if (hittype[i][0] != 0) {
                stopsound(sprite[i].lotag - 1000);
                hittype[i][0] = 0;
            }
        } else if (hittype[i][1] > 0) {
            hittype[i][1] -= 12;
            if (hittype[i][1] < 0) {
                if (++hittype[i][0] > 3)
                    hittype[i][0] = 0;
                hittype[i][1] = 0;
            }
            MUSIC_SetVolume(hittype[i][1]);
        } else {
            dist = (klabs(sprite[ps[screenpeek].i].x - sprite[i].x)
                + klabs(sprite[ps[screenpeek].i].y - sprite[i].y));
            switch (hittype[i][0]) {
            case 0: case 2:
                if (hittype[i][0] == 0)
                    song = music_files[ps[screenpeek].volume][ps[screenpeek].level];
                else
                    song = env_music[sprite[i].lotag];
                hittype[i][0]++;
                if (song != ps[screenpeek].music) {
                    ps[screenpeek].music = song;
                    playmusic(ps[screenpeek].music);
                    MUSIC_SetVolume(music_volume);
                }
                break;
            case 1:
                if ((dist >> 3) < sprite[i].hitag)
                    hittype[i][1] = music_volume;
                break;
            case 3:
                if ((dist >> 3) >= sprite[i].hitag)
                    hittype[i][1] = music_volume;
                break;
            }
        }
    }
}

void movestandables(void)
{
    short i, nexti, j;

    for (i = headspritestat[6]; i >= 0; i = nexti) {
        long d;
        short sect = sprite[i].sectnum, p;

        nexti = nextspritestat[i];
        if (sprite[i].picnum == SEENINE) {
            /* A force field post; one that dies sets every post of its hitag to
               -32, and each of those then bursts. */
            if (sprite[i].shade == -32) {
                spawn(i, EXPLOSION);
                sprite[i].z -= 0x2000;
                shoot(i, OTHERSHOTSPARK, 1);
                sprite[i].z += 0x2000;
                spawn(i, EXPLOSION);
                spawn(i, EXPLOSION);
                deletesprite(i);
            } else {
                j = ifhitbyweapon(i);
                if ((j >= 0 && sprite[i].extra < 0) || sprite[i].shade == -31) {
                    for (j = headspritestat[6]; j >= 0; j = nextspritestat[j])
                        if (sprite[i].hitag == sprite[j].hitag && sprite[j].picnum == SEENINE)
                            sprite[j].shade = -32;
                }
            }
        } else if (sprite[i].picnum >= CRANE && sprite[i].picnum < CRANE + 4) {
            long *t = hittype[i];

            sprite[i].xrepeat = sprite[i].yrepeat = 64;
            if (t[0] == 0) {
                if (checkcursectnums((short)hittype[i][1]) < 0) {
                    FIND_IN_SECTOR((short)hittype[i][1], 627, j);
                    if (j >= 0) {
                        sprite[i].ang = (short)getangle(msx[hittype[i][4] + 1] - sprite[i].x, msy[hittype[i][4] + 1] - sprite[i].y);
                        setsprite(j, msx[hittype[i][4] + 1], msy[hittype[i][4] + 1], sprite[j].z);
                        t[0]++;
                        continue;
                    }
                } else {
                    sprite[i].ang = (short)getangle(msx[hittype[i][4] + 1] - sprite[i].x, msy[hittype[i][4] + 1] - sprite[i].y);
                    t[0]++;
                }
            } else if (t[0] == 1) {
                if (sprite[i].xvel < 184) {
                    sprite[i].picnum = 1176;
                    sprite[i].xvel += 8;
                }
                ssp(i, 0);
                if (sect == hittype[i][1])
                    t[0]++;
            } else if (t[0] == 2 || t[0] == 7) {
                sprite[i].z += 1536;
                if (t[0] == 2) {
                    if (sector[sect].floorz - sprite[i].z < 12288 && sprite[i].picnum > CRANE)
                        sprite[i].picnum--;
                    if (sector[sect].floorz - sprite[i].z < 5120)
                        t[0]++;
                }
                if (t[0] == 7 && sector[sect].floorz - sprite[i].z < 5120) {
                    if (sprite[i].picnum > CRANE)
                        sprite[i].picnum--;
                    else {
                        if (sprite[i].owner == -2) {
                            p = findplayer(i, &d);
                            if (ps[p].crane == i)
                                ps[p].crane = -1;
                        }
                        t[0]++;
                        sprite[i].owner = -1;
                    }
                }
            } else if (t[0] == 3) {
                if (++sprite[i].picnum == 1177) {
                    p = checkcursectnums((short)hittype[i][1]);
                    if (p >= 0 && ps[p].on_ground) {
                        sprite[i].owner = -2;
                        ps[p].ang = (short)getangle(sprite[i].x - ps[p].posx, sprite[i].y - ps[p].posy);
                        ps[p].crane = i;
                    } else {
                        FIND_IN_SECTOR((short)hittype[i][1], 627, j);
                        if (j >= 0)
                            sprite[i].owner = j;
                    }
                    hittype[i][2] = 0;
                    t[0]++;
                    continue;
                }
            } else if (t[0] == 4) {
                if (++t[2] > 10)
                    t[0]++;
            } else if (t[0] == 5 || t[0] == 8) {
                if (t[0] == 8 && sprite[i].picnum < 1177 && sector[sect].floorz - sprite[i].z > 0x2000)
                    sprite[i].picnum++;
                if (sprite[i].z >= msx[hittype[i][4] + 2])
                    sprite[i].z -= 1536;
                else {
                    t[0]++;
                    sprite[i].xvel = 0;
                }
            } else if (t[0] == 6) {
                if (sprite[i].xvel < 192)
                    sprite[i].xvel += 8;
                sprite[i].ang = (short)getangle(msx[hittype[i][4]] - sprite[i].x, msy[hittype[i][4]] - sprite[i].y);
                MOVE_ALONG(i, sprite[i].zvel, 0);
                if ((sprite[i].y - msy[hittype[i][4]]) * (sprite[i].y - msy[hittype[i][4]]) + (sprite[i].x - msx[hittype[i][4]]) * (sprite[i].x - msx[hittype[i][4]]) < 0x4000)
                    t[0]++;
            } else if (t[0] == 9) {
                t[0] = 0;
            }
            setsprite((short)msy[hittype[i][4] + 2], sprite[i].x, sprite[i].y, sprite[i].z - 8704);
            if (sprite[i].owner == -1)
                continue;
            p = findplayer(i, &d);
            j = ifhitbyweapon(i);
            if (j >= 0) {
                if (sprite[i].owner == -2 && ps[p].crane == i)
                    ps[p].crane = -1;
                sprite[i].owner = -1;
                sprite[i].picnum = CRANE;
            }
            if (sprite[i].owner == -2) {
                playertype *pl = &ps[p];

                if (pl->crane == i && p == screenpeek)
                    sprite[i].xrepeat = sprite[i].yrepeat = 0;
                pl->posx = sprite[i].x - (sintable[(pl->ang + 512) & 2047] >> 6);
                pl->posy = sprite[i].y - (sintable[pl->ang & 2047] >> 6);
                pl->posz = sprite[i].z - 7168;
                clipmove(&pl->posx, &pl->posy, &pl->posz, &pl->cursectnum, 0, 0, 192, 1024, 1024, 0);
                setsprite(pl->i, pl->posx, pl->posy, pl->posz);
            } else if (sprite[i].owner >= 0) {
                setsprite(sprite[i].owner, sprite[i].x, sprite[i].y, sprite[i].z);
            }
        } else if (sprite[i].picnum == CANWITHSOMETHING) {
            FALL(&sprite[i]);
            j = ifhitbyweapon(i);
            if (j >= 0) {
                if (sprite[i].lotag)
                    EGS(sect, sprite[i].x, sprite[i].y, sprite[i].z - 4096, 1, sprite[i].lotag, -16, 32, 32, 0, 0, -1536, sprite[j].owner, 1);
                EGS(sect, sprite[i].x, sprite[i].y, sprite[i].z - 4096, 1, 1201, sprite[i].shade, 32, 32, sprite[j].ang, 128, -1024,
                    sprite[j].owner, 5);
                EGS(sect, sprite[i].x, sprite[i].y, sprite[i].z, 1, 1204, sprite[i].shade, 28, 32, sprite[j].ang - 512, 128, -1024,
                    sprite[j].owner, 5);
                EGS(sect, sprite[i].x, sprite[i].y, sprite[i].z, 1, 1205, sprite[i].shade, 28, 32, sprite[j].ang + 512, 128, -1024,
                    sprite[j].owner, 5);
                deletesprite(i);
            }
        } else {
            p = findplayer(i, &d);
            if (sprite[i].picnum == 627 || sprite[i].picnum == 905 || sprite[i].picnum == 974)
                execute(i, p, d);
        }
    }
}
