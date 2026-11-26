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

long msx[1024], msy[1024];

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
    long dx, dy;

    for (k = 0; k < 3; k++)
        for (j = headspritestat[statlist[k]]; j >= 0; j = nextspritestat[j]) {
            dy = sprite[i].y - sprite[j].y;
            dx = sprite[i].x - sprite[j].x;
            if (dx * dx + dy * dy < radius * radius) {
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
        (dz), 128, 1024, 1024, (clip), script_tics)

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

    sprite[i].x += (sintable[(sprite[i].ang + 512) & 2047] * sprite[i].xvel) >> 14;
    sprite[i].y += (sprite[i].xvel * sintable[(short)(sprite[i].ang & 2047)]) >> 14;
    sn = sprite[i].sectnum;
    j = hittype[i][1];
    w = sector[sn].wallptr;
    end = w + sector[sn].wallnum;
    for (; w < end; w++) {
        long wx = msx[j], wy = msy[j];
        j++;
        rotatepoint(0, 0, wx, wy, hittype[i][2] & 2047, &rx, &ry);
        dragpoint(w, sprite[i].x + rx, sprite[i].y + ry);
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
        z = sprite[i].yrepeat << 7;
        if (cansee(sprite[i].x, sprite[i].y, sprite[i].z - z,
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

    if (sector[sn].floorz - sector[sn].ceilingz < (28 << 8)
        && (sector[sn].lotag & 0x8000) == 0 && (sector[sn].lotag & 0x7fff) != 0) {
        if ((short)(sector[sn].lotag & 0x7fff) != 23)
            sector[sn].ceilingz -= 4 << 8;
        sprite[i].z = sector[sprite[i].sectnum].floorz;
        switch ((unsigned short)sprite[i].picnum) {
        case 408: case 490: case 631: case 1536: case 1680: case 1820:
            ps[p].kills++;
            sprite[i].xvel = 0;
            sprite[i].cstat &= ~1;
            FTA(10);
            break;
        case 579:
            sprite[i].xvel = 0;
            ps[p].kills++;
            FTA(10);
            break;
        }
        return i;
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
    long z;

    EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z - (36 << 8),
        0, ARMPIECE, -16, 24, 24, (short)(sprite[i].ang + 1536), 64, -623, i, 5);
    EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z - (36 << 8),
        0, ARMPIECE, -16, 24, 24, (short)(sprite[i].ang - 512), 48, -447, i, 5);
    z = sprite[i].z += 8 << 8;
    EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, z - (36 << 8),
        0, LEGPIECE, -16, 24, 24, (short)(sprite[i].ang + 512), 32, -508, i, 5);
    EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z - (36 << 8),
        0, LEGPIECE, -16, 24, 24, (short)(sprite[i].ang - 512), 24, -768, i, 5);
    EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, (sprite[i].z -= 8 << 8) - (36 << 8),
        0, HEADPIECE, -16, 24, 24, (short)(sprite[i].ang - 1024), 32, -768, i, 5);
}

/* The actor and standable dispatchers contain expanded copies of this scan. */
#define HIT_BY_WEAPON(i, hit) do { \
    for ((hit) = headspritestat[4]; (hit) >= 0; (hit) = nextspritestat[(hit)]) \
        if (i != (hit) && i != sprite[(hit)].owner \
            && klabs(sprite[i].x - sprite[(hit)].x) + klabs(sprite[i].y - sprite[(hit)].y) < 768 \
            && klabs((sprite[i].z - sprite[(hit)].z) >> 8) <= 48) { \
            if (sprite[i].statnum != 3) { \
                sprite[i].extra -= sprite[(hit)].extra; \
                sprite[i].xvel = sprite[(hit)].xvel; \
                sprite[i].ang = sprite[(hit)].ang; \
                if ((sprite[i].statnum == 1 || sprite[i].statnum == 10) \
                    && sprite[(hit)].picnum == BULLET) \
                    spawn(i, STICKSPARK); \
            } \
            break; \
        } \
} while (0)

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
    if (disable_armor == 0 && ps[(p)].armor) \
        ps[(p)].armor -= 4; \
    else if (ps[(p)].health > 0) \
        ps[(p)].health -= (damage); \
    else \
        ps[(p)].health = 0; \
} while (0)

#define THROW_BLOOD(i, at, ang, speed) \
    EGS(sprite[(i)].sectnum, sprite[(i)].x, sprite[(i)].y, sprite[(at)].z, \
        0, BLOOD, -16, 24, 13, (ang) + (krand() & 127), \
        (speed) + (krand() & 31), -(krand() & 511), (i), 5)

void moveactors(void)
{
    short i, nexti, j, hit;

    for (i = headspritestat[1]; i >= 0; i = nexti) {
        spritetype *s = &sprite[i];
        long *t = hittype[i], dist, d;
        short p, sect;

        nexti = nextspritestat[i];
        p = findplayer(i, &dist);
        sect = s->sectnum;
        if (s->picnum == CAM1) {
            if (dist < 2048)
                viewscreen = i;
            continue;
        }
        if (s->picnum == MASTERSWITCH) {
            /* Fire an armed master switch when its countdown expires. */
            if (s->yvel != 1 || --s->hitag > 0)
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
                    s->shade = 1;
            }
            deletesprite(i);
            continue;
        }
        if (s->picnum == SHARK) {
            unsigned char frames[4] = { 0, 1, 0, 3 };
            playertype *pl = &ps[p];
            short turn;

            if (MOVE_ALONG(i, s->zvel, 0))
                s->ang += 1024;
            t[1]++;
            HIT_BY_WEAPON(i, turn);
            if (turn >= 0) {
                if (s->extra < 0) {
                    ps[screenpeek].score += 5000;
                    spawn(i, EXPLOSION);
                    deletesprite(i);
                    continue;
                }
                t[2] = -1;
                s->ang += 1024;
            }
            switch (t[2]) {
            case -1:
            case 0:
                if (t[2] == -1)
                    t[2] = 0;
                else if ((krand() >> 8) >= 251)
                    t[2] = 1;
                /* Preserve the hit index when the shark steers toward the player. */
                if ((krand() >> 8) < 247) {
                    turn = (short)(sintable[(16 * t[1]) & 2047] >> 8);
                    s->ang += turn;
                } else
                    s->ang += getincangle_shift(s->ang, getangle(pl->posx - s->x, pl->posy - s->y), 2);
                if (turn >= -32 && turn <= 32) {
                    t[0] = frames[t[1] & 3];
                    if (s->xvel < 112)
                        s->xvel += 8;
                } else {
                    t[0] = turn >= 0 ? 2 : 4;
                    if (s->xvel > 64)
                        s->xvel -= 8;
                }
                break;
            case 1:
                if (pl->posz + 768 < s->z)
                    s->z -= 1024;
                else if (pl->posz - 768 > s->z)
                    s->z += 1024;
                s->ang += getincangle_shift(s->ang, getangle(pl->posx - s->x, pl->posy - s->y), 3);
                if (s->xvel >= 144)
                    t[0] = 0;
                else {
                    t[0] = frames[t[1] & 3];
                    s->xvel += 16;
                }
                if (dist < 768) {
                    t[2] = (krand() >> 8) < 247 ? 0 : 2;
                    s->ang += 1024;
                }
                break;
            case 2:
                s->x = (sintable[(pl->ang + 512) & 2047] >> 6) + pl->posx;
                s->y = pl->posy + (sintable[pl->ang & 2047] >> 6);
                s->z = pl->posz + 3072;
                t[0] = frames[t[1] & 3];
                pl->ang += sintable[((unsigned short)t[1] << 8) & 2047] >> 12;
                s->ang = pl->ang + 1024;
                if ((krand() >> 8) >= 223)
                    t[2] = 0;
                break;
            }
            continue;
        }
        if (s->picnum == OOZ) {
            if (++t[0] > 1024 || dist > 0x2000) {
                deletesprite(i);
                continue;
            }
            d = klabs(sector[sect].ceilingz - sector[sect].floorz);
            s->yrepeat = (unsigned char)(d >> 9);
            s->xrepeat = (unsigned char)(25 - ((short)(d >> 9) >> 1));
            s->z = sector[sect].floorz;
            continue;
        }
        if (s->picnum >= 961 && s->picnum <= 968) {
            short bite[8] = { 5, 5, 6, 6, 7, 7, 6, 5 };
            playertype *pl = &ps[p];
            short j, xv;

            t[1] += 32;
            if (t[0] == -4) {
                setsprite(i, s->x, s->y, s->z);
                s->cstat &= ~8;
                s->ang = pl->ang;
                if (player_bits[p] & 4) {
                    pl->kickback_pic = 0;
                    s->xvel = 128;
                    s->zvel = (short)(16 * (100 - pl->horiz) - 512);
                    t[0] = 0;
                    pl->face_parasite = -1;
                }
                s->z = pl->posz - t[2] + 2048;
                s->z += 16 * (100 - pl->horiz);
                if (t[2] > 512)
                    t[2] -= 128;
                if (t[2] < 348)
                    t[2] += 128;
                if (t[3] <= 0) {
                    s->picnum = 966;
                    if ((krand() >> 8) >= 223)
                        t[3] = 1;
                } else {
                    s->picnum = bite[t[3]] + 961;
                    if (t[3] == 4) {
                        HURT(p, 5);
                        spritesound(13, pl->i);
                    }
                    t[3] = t[3] >= 7 ? 0 : t[3] + 1;
                }
                s->xrepeat = (unsigned char)((sintable[t[1] & 2047] >> 13) + 20);
                s->yrepeat = (unsigned char)((sintable[t[1] & 2047] >> 13) + 15);
                s->x = pl->posx + (sintable[(pl->ang + 512) & 2047] >> 7);
                s->y = (sintable[pl->ang & 2047] >> 7) + pl->posy;
                continue;
            }
            if (s->xvel < 64 && dist < 512 && pl->face_parasite == -1) {
                pl->face_parasite = i;
                t[2] = (t[0] == 3 || t[0] == 2) ? 3072 : -3328;
                t[0] = -4;
            } else if (t[0] == -3) {
                if (s->xrepeat > 4)
                    s->xrepeat -= 4;
                if (s->yrepeat > 4)
                    s->yrepeat -= 4;
                else
                    deletesprite(i);
                continue;
            }
            HIT_BY_WEAPON(i, j);
            if (j >= 0) {
                pl->score += 1000;
                if (sprite[j].picnum == BULLET) {
                    t[0] = -3;
                    continue;
                }
                if ((krand() >> 8) >= 251 && klabs(sector[sect].floorz - sector[sect].ceilingz) < 49152
                    && (t[0] == 0 || t[0] == 2))
                    t[0]++;
            }
            if (t[0] == -1) {
                FALL(s);
                s->picnum = 965;
                s->cstat &= 0xF7;
                if (s->yrepeat > 62) {
                    spawn_gib(i, ARMPIECE, s->ang + 1536, 64, -623);
                    spawn_gib(i, ARMPIECE, s->ang - 512, 48, -447);
                    s->z += 8 << 8;
                    spawn_gib(i, LEGPIECE, s->ang + 512, 32, -508);
                    spawn_gib(i, LEGPIECE, s->ang - 512, 24, -768);
                    s->z -= 8 << 8;
                    spawn_gib(i, HEADPIECE, s->ang - 1024, 32, -768);
                }
                if (s->xrepeat > 32)
                    s->xrepeat -= krand() & 7;
                if (s->yrepeat <= 16) {
                    s->xrepeat = 40;
                    s->yrepeat = 16;
                    s->owner = -1;
                    t[0] = 0;
                } else
                    s->yrepeat -= krand() & 7;
                continue;
            }
            if (t[0] == -2) {
                spritetype *o;

                FALL(s);
                o = &sprite[s->owner];
                o->xvel = 0;
                s->z = o->z;
                s->x = (sintable[(o->ang + 512) & 2047] >> 11) + o->x;
                s->y = (sintable[o->ang & 2047] >> 11) + o->y;
                s->picnum = (short)((randomstash & 1) + 963);
                if (s->yrepeat < 64)
                    s->yrepeat += 2;
                else if (s->xrepeat < 32)
                    s->xrepeat += 4;
                else {
                    t[0] = -1;
                    if (klabs(s->x - o->x) + klabs(s->y - o->y) < 768)
                        deletesprite(s->owner);
                }
                continue;
            }
            if ((krand() >> 8) >= 223)
                for (j = headspritesect[sect]; j >= 0; j = nextspritesect[j]) {
                    short pic = sprite[j].picnum;

                    if (pic != 408 && pic != 490 && pic != 1536 && pic != 1680 && pic != 1820)
                        continue;
                    if (klabs(s->x - sprite[j].x) + klabs(s->y - sprite[j].y) < 768 && klabs(s->z - sprite[j].z) < 0x2000) {
                        s->owner = j;
                        t[0] = -2;
                        t[1] = 0;
                        continue;
                    }
                }
            if (t[0] == 0 || t[0] == 2) {
                s->picnum = GREENSLIME;
                if (t[0] == 2) {
                    s->zvel = 0;
                    s->cstat &= ~8;
                    if (sector[sect].ceilingz + 6144 < s->z) {
                        s->z += 2048;
                        t[0]++;
                        continue;
                    }
                } else {
                    s->cstat |= 8;
                    FALL(s);
                }
                MOVE_ALONG(i, s->zvel, 0);
                xv = s->xvel;
                if (xv > 96) {
                    s->xvel = xv - 2;
                    continue;
                }
                s->xvel = (short)(64 - (sintable[(t[1] + 128) & 2047] >> 9));
                s->ang += getincangle_shift(s->ang, getangle(pl->posx - s->x, pl->posy - s->y), 3);
                s->xrepeat = (unsigned char)((sintable[(t[1] + 128) & 2047] >> 11) + 36);
                s->yrepeat = (unsigned char)((sintable[t[1] & 2047] >> 13) + 16);
                if ((krand() >> 8) >= 251 && klabs(sector[sect].floorz - sector[sect].ceilingz) < 49152) {
                    s->zvel = 0;
                    t[0]++;
                }
            }
            if (t[0] == 1) {
                s->picnum = GREENSLIME;
                if (s->yrepeat < 40)
                    s->yrepeat += 8;
                if (s->xrepeat > 8)
                    s->xrepeat -= 4;
                if (s->zvel > -3072)
                    s->zvel -= 348;
                s->z += s->zvel;
                if (sector[sect].ceilingz + 4096 > s->z) {
                    s->z = sector[sect].ceilingz + 4096;
                    s->xvel = 0;
                    t[0]++;
                }
            }
            if (t[0] == 3) {
                s->picnum = 962;
                if (s->zvel < 3072)
                    s->zvel += 348;
                s->z += s->zvel;
                if (sector[sect].floorz - 0x2000 >= s->z) {
                    if (s->yrepeat < 36)
                        s->yrepeat += 8;
                    if (s->xrepeat > 8)
                        s->xrepeat -= 4;
                } else {
                    s->yrepeat -= 4;
                    s->xrepeat += 2;
                }
                if (sector[sect].floorz - 2048 < s->z) {
                    s->z = sector[sect].floorz - 2048;
                    t[0] = 0;
                    s->xvel = 0;
                }
            }
        } else if (s->picnum == WATERDRIP) {
            /* A drip: grows on the ceiling, falls, splashes. */
            if (dist > 0x2000)
                continue;
            if (t[0] == 0) {
                if (t[1] > 0)
                    t[1]--;
                else {
                    s->yrepeat++;
                    s->xrepeat -= 2;
                    if (s->yrepeat > 8) {
                        t[0] = 1;
                        s->xrepeat = 8;
                    }
                }
            }
            if (t[0] == 1) {
                FALL(s);
                if (sector[sect].floorz - 2048 < s->z) {
                    t[0] = 2;
                    spritesound(17, i);
                }
            }
            if (t[0] == 2) {
                s->xrepeat += 8;
                s->yrepeat -= 2;
                if (s->yrepeat < 2) {
                    s->z = sector[sect].ceilingz + 512;
                    t[0] = 0;
                    t[1] = krand() & 31;
                    s->xrepeat = 16;
                    s->yrepeat = 0;
                }
            }
        } else if (s->picnum == 930 || s->picnum == 931)
            {
                playertype *pl = &ps[p];

                HIT_BY_WEAPON(i, hit);
                if (hit >= 0) {
                    t[4] = 0;
                    if (s->extra < 0) {
                        pl->score += 1000;
                        spawn(i, EXPLOSION);
                        /* the exe counts this loop in its distance variable */
                        for (dist = 0; dist < 30; dist += 3) {
                            short zv = (short)(-(krand() & 511) - 512), xv = (short)((krand() & 128) + 64), a = (short)(krand() & 2047);

                            EGS(sect, s->x, s->y, s->z, 0, (short)(krand() % 11 + 235), -8, 32, 32, a, xv, zv, i, 5);
                        }
                        pl->turret_kills++;
                        t[0] = 2;
                        s->xrepeat = s->yrepeat = 0;
                    }
                }
                if (t[0] == 0) {
                    t[4] = 0;
                    if (s->xrepeat == 0) {
                        deletesprite(i);
                        continue;
                    }
                    if (dist < 4096) {
                        if (klabs(s->z - pl->posz) >= 2048) {
                            short step = (short)(pl->posz - s->z > 0 ? 2048 : -2048);

                            s->z += step;
                            if (s->picnum == 930)
                                sector[sect].floorz += step;
                            else
                                sector[sect].ceilingz += step;
                        } else {
                            t[1] = 0;
                            t[0]++;
                        }
                    }
                }
                if (t[0] == 1) {
                    s->ang += 2 * getincangle_shift(s->ang, getangle(pl->posx - s->x, pl->posy - s->y), 1);
                    s->ang &= (short)0xFF80;
                    if (++t[1] >= 8) {
                        s->shade = 4;
                        t[1] = 0;
                        if (dist > 1024) {
                            t[0] = 4;
                            goto wait;
                        }
                    } else {
                        t[4] ^= 1;
                        if (t[4]) {
                            s->z -= 0x2000;
                            shoot(i, BULLET, 0);
                            s->shade = -16;
                            s->z += 0x2000;
                        }
                    }
                }
                if (t[0] == 2) {
                    t[4] = 0;
                    if (s->picnum == 930) {
                        s->z += 2048;
                        sector[sect].floorz += 2048;
                        if (sector[sect].floorz > t[3]) {
                            t[0] = 0;
                            continue;
                        }
                    } else {
                        s->z -= 2048;
                        sector[sect].ceilingz -= 2048;
                        if (sector[sect].ceilingz < t[3]) {
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
                s->ang = (short)getangle(pl->posx - s->x, pl->posy - s->y);
            } else if (s->picnum == HEAVYHBOMB) {
            /* shortcut: the exe reads hittype[-1] for an ownerless bomb; ownerless ones use player 0 */
            long own = hittype[s->owner][0];
            short a;

            if (sector[sect].floorz - sector[sect].ceilingz < (28 << 8)
                && !(sector[sect].lotag & 0x8000) && (sector[sect].lotag & 0x7fff)) {
                if ((sector[sect].lotag & 0x7fff) != 23)
                    sector[sect].ceilingz -= 4 << 8;
                s->z = sector[s->sectnum].floorz;
                switch ((unsigned short)s->picnum) {
                case 408: case 490: case 631: case 1536: case 1680: case 1820:
                    s->xvel = 0;
                    s->cstat &= ~1;
                    ps[p].kills++;
                    FTA(10);
                    break;
                case 579:
                    s->xvel = 0;
                    ps[p].kills++;
                    FTA(10);
                    break;
                }
                if (s->extra < 0)
                    ps[own].remote_active = 0;
            }
            FALL(s);
            if (s->xvel <= 0)
                s->xvel = 0;
            else {
                s->xvel -= 4;
                if (s->xvel & 8)
                    s->cstat ^= 4;
            }
            MOVE_ALONG(i, s->zvel, 0);
            if (ps[own].remote_active || ps[own].i != s->owner) {
                if (dist > 512 && dist < 768) {
                    ps[p].remote_ammo++;
                    addweapon(p, 721, 1);
                    deletesprite(i);
                }
                continue;
            }
            s->yvel++;
            if (s->xrepeat || s->yrepeat) {
                s->xrepeat = s->yrepeat = 0;
                hitradius(i, 3092, s->extra);
            }
            if (s->yvel & 1)
                for (a = 0; a < 2047; a += 256)
                    EGS(s->sectnum, s->x, s->y, s->z - (s->yvel << 10), 0, 1370, -127, 48, 48, (short)a, 0, 0, i, 5);
            if (s->yvel > 20)
                deletesprite(i);
        } else if (s->picnum == DOORSHOCK) {
            if (dist <= 0x2000) {
                d = klabs(sector[sect].ceilingz - sector[sect].floorz);
                s->xrepeat = 16;
                s->yrepeat = (unsigned char)((d >> 9) + 8);
                s->z = sector[sect].floorz;
            }
        } else if (s->picnum == 32) {
            long range = (s->lotag + (s->hitag << 16)) >> 13;

            if (range != 0 && s->hitag != 0) {
                if (dist >= range || klabs(s->z - ps[p].posz) >= 12288)
                    t[0] = 0;
                else if (t[0] == 0) {
                    t[0] = 1;
                    operatemasterswitches(s->lotag);
                    operateactivators(s->lotag);
                }
            } else {
                short occupant = checkcursectnums(sect);

                if (occupant < 0 || !ps[occupant].on_ground)
                    t[0] = 0;
                else if (t[0] == 0) {
                    t[0] = 1;
                    operatemasterswitches(s->lotag);
                    operateactivators(s->lotag);
                }
            }
        } else if (s->picnum >= 305 && s->picnum <= 308)
            {

                if (dist > 0x2000)
                    continue;
                if (t[3] == 0)
                    t[3] = sector[sect].floorshade;
                for (;;) {
                    if (t[2]) {
                        t[2]--;
                        sector[sect].floorshade = sector[sect].ceilingshade = 20;
                        continue;
                    }
                    if ((s->yrepeat | s->xrepeat) == 0) {
                        s->xrepeat = (unsigned char)t[0];
                        s->yrepeat = (unsigned char)t[1];
                        break;
                    }
                    if (krand() & 8)
                        break;
                    t[0] = s->xrepeat;
                    t[1] = s->yrepeat;
                    t[2] = randomstash & 4;
                    s->xrepeat = s->yrepeat = 0;
                }
                s->xrepeat = (unsigned char)((randomstash & 7) + 8);
                s->picnum++;
                if (randomstash & 1)
                    s->cstat ^= 2;
                if (dist < 512)
                    spritesound(24, i);
                if (s->picnum == 309)
                    s->picnum = 305;
                sector[sect].floorshade = sector[sect].ceilingshade = (s->picnum & 1) ? 0 : 20;
            } else if (s->picnum >= 620 && s->picnum <= 625) {
            short k;
            long n;

            s->picnum = (short)(((krand() >> 8) < 63 ? 623 : 620) + (short)t[2]);
            if (++t[2] == 3)
                t[2] = 0;
            if (dist >= 4096) {
                t[3] = 0;
                stopsound(26);
            } else {
                t[0] += 128;
                draw_weapon_tile(246, 18, 611, (sintable[t[0] & 2047] >> 11) + 8, 1);
                if (t[3] == 0) {
                    spritesound(26, i);
                    t[3] = 1;
                }
            }
            if (t[1]) {
                if (++t[1] == 24) {
                    ps[screenpeek].score += 100000;
                    stopsound(26);
                    deletesprite(i);
                    continue;
                }
                s->ang += 300;
                s->z -= 6144;
                shoot(i, 0, 1);
                s->z += 6144;
                n = t[1];
                if (n == 2)
                    for (k = headspritestat[3]; k >= 0; k = nextspritestat[k])
                        if (sprite[k].lotag == 3)
                            hittype[k][4] = 1;
                if (n == 2 || n == 4 || n == 7 || n == 10 || n == 15) {
                    EGS(s->sectnum, s->x, s->y, s->z - (krand() & 0x3FFF) + 5120, 2, FLAIR, -127, 96, 48, 0, 0, 0, i, 5);
                    spawn(i, EXPLOSION);
                    for (k = 0; k < 16; k += 2) {
                        short zv = (short)-(krand() & 1023);

                        EGS(sect, s->x, s->y, s->z + (k << 9), 0, 235, -8, 32, 32, s->ang, (krand() & 255) + 128, zv, i, 5);
                    }
                }
            }
            if (t[1] == 0) {
                HIT_BY_WEAPON(i, hit);
                if (hit >= 0 && s->extra < 0)
                    t[1] = 1;
            }
        } else if (s->picnum == 265) {
            s->owner += 8;
            if (s->owner < 128)
                s->ang += 8;
            else if (s->owner < 384)
                s->ang -= 8;
            else if (s->owner < 512)
                s->ang += 8;
            else {
                s->owner = 8;
                s->ang += 16;
            }
        } else if (s->picnum == 263)
            {
                playertype *pl = &ps[p];

                if (dist > 4096)
                    continue;
                HIT_BY_WEAPON(i, hit);
                if (hit >= 0 && s->extra < 0) {
                    s->picnum = 273;
                    t[0] = 0;
                    s->cstat &= ~1;
                    continue;
                }
                switch (t[0]) {
                case 0:
                    if (dist < 4096)
                        t[0]++;
                    break;
                case 1:
                    t[0]++;
                    s->ang = (short)getangle(pl->posx - s->x, pl->posy - s->y);
                    if (dist != 0)
                        s->zvel = (short)(((pl->posz + 2048 - s->z) << 8) / dist);
                    s->xvel = 16;
                    break;
                case 2:
                    if (s->zvel > -1024)
                        s->zvel -= 64;
                    s->z += s->zvel;
                    if (s->xvel < 256)
                        s->xvel += 16;
                    if (MOVE_ALONG(i, s->zvel, 0))
                        t[0]++;
                    if (dist < 768) {
                        s->ang += 1024 - (randomstash & 2047);
                        HURT(p, 1);
                    }
                    break;
                case 3:
                    if (MOVE_ALONG(i, s->zvel, 0)) {
                        s->ang += 512;
                        t[0] = 1;
                    }
                    break;
                }
            } else if (s->picnum == 273) {
            if (dist > 4096)
                continue;
            FALL(s);
            if (++t[0] > 1024)
                deletesprite(i);
            else if (MOVE_ALONG(i, s->zvel, 0)) {
                s->z = sector[sect].floorz;
                if (sector[sect].lotag == 1)
                    deletesprite(i);
            }
        } else
            execute(i, p, dist);
    }
}

void moveweapons(void)
{
    short i, nexti, p;
    long dist;

    for (i = headspritestat[4]; i >= 0; i = nexti) {
        spritetype *s = &sprite[i];

        nexti = nextspritestat[i];
        p = findplayer(i, &dist);
        if (s->picnum == BLUESPARK) {
            execute(i, p, (short)dist);
            continue;
        }
        if (s->picnum == RADIUSEXPLOSION) {
            if (s->shade == -15) {
                shoot(i, CLUB, 1);
                if ((short)dist < 512 && s->owner != ps[p].i) {
                    kickview(p, i);
                    HURT(p, s->extra);
                }
            } else
                s->shade = -15;
            execute(i, p, (short)dist);
            continue;
        }
        if (s->picnum == BOUNCEMINE) {
            execute(i, p, (short)dist);
            continue;
        }
        if (s->picnum == RPG) {
            if (s->extra >= 0) {
                if (s->owner != ps[p].i && (short)dist < 768
                    && klabs((ps[p].posz - s->z) >> 8) <= 48) {
                    spritesound(13, ps[p].i);
                    kickview(p, i);
                    HURT(p, s->extra);
                    s->extra = -1;
                }
                if (MOVE_ALONG(i, s->zvel, 1)) {
                    shoot(i, RPG, 1);
                    s->extra = -1;
                }
            }
            execute(i, p, (short)dist);
            continue;
        }
        if (s->picnum == BULLET) {
            if (s->extra >= 0) {
                if ((short)dist < 768 && s->owner != ps[p].i
                    && klabs((ps[p].posz - s->z) >> 8) <= 48) {
                    HURT(p, s->extra);
                    kickview(p, i);
                    spritesound(13, ps[p].i);
                    s->extra = -1;
                    continue;
                }
                if (MOVE_ALONG(i, s->zvel, 1)) {
                    shoot(i, BULLET, 1);
                    s->extra = -1;
                }
            }
            execute(i, p, (short)dist);
        }
        if (s->picnum == SHOTSPARK1) {
            if ((short)dist < 512)
                kickview(p, i);
            if (s->shade == -15)
                shoot(i, 191, 1);
            else
                s->shade = -15;
            execute(i, p, (short)dist);
        } else if (s->picnum == OTHERSHOTSPARK) {
            if ((short)dist < 512)
                kickview(p, i);
            if (s->shade == -15) {
                shoot(i, OTHERSHOTSPARK, 1);
                if (s->yvel >= 0)
                    spritesound(30, i);
            } else
                s->shade = -15;
            execute(i, p, (short)dist);
        }
    }
}

void moveeffects(void)
{
    short i, nexti;

    for (i = headspritestat[5]; i >= 0; i = nexti) {
        spritetype *s = &sprite[i];
        long *t = hittype[i], dist;
        short sect, p;

        nexti = nextspritestat[i];
        p = findplayer(i, &dist);
        sect = s->sectnum;
        switch (s->picnum) {
        case WATERBUBBLE:
            s->ang = krand() & 2047;
            s->xvel = (short)(16 - (krand() & 31));
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
        if (s->picnum == SHELL) {
            if (MOVE_ALONG(i, s->zvel, 0) == 0) {
                if (s->zvel < 512)
                    s->zvel += 64;
                if (s->xvel > 0) {
                    s->xvel -= 4;
                    if (s->xvel < 0)
                        s->xvel = 0;
                } else if (s->xvel < 0) {
                    s->xvel += 4;
                    if (s->xvel > 0)
                        s->xvel = 0;
                }
            } else if (t[1]) {
                deletesprite(i);
            } else {
                s->xvel = (short)-s->xvel;
                s->zvel = (short)-s->zvel;
                t[1] = 1;
            }
            continue;
        }
        if (s->picnum == WATERSPLASH) {
            if ((sector[sect].lotag & 0x7FFF) != 1 || (t[0] += 128) > 2047) {
                deletesprite(i);
                continue;
            }
            s->cstat ^= 4;
            s->xrepeat = s->yrepeat = (unsigned char)((sintable[t[0]] >> 11) + 8);
        }
        if (s->picnum >= 970 && s->picnum <= 973) {
            FALL(s);
            MOVE_ALONG(i, s->zvel, 0);
            if (klabs(s->z - sector[sect].floorz) < 2048) {
                s->zvel = -1024;
                s->xrepeat >>= 1;
                s->yrepeat >>= 1;
                t[0]++;
            }
            if (t[0] > 2) {
                deletesprite(i);
                continue;
            }
        }
        if (s->picnum >= 1201 && s->picnum <= 1205) {
            FALL(s);
            if (s->xvel <= 0) {
                s->xvel = 0;
                if ((krand() >> 8) >= 127)
                    s->cstat ^= 0x80;
            } else {
                if (s->zvel > 512) {
                    if (s->picnum == 1204)
                        s->picnum = 1202;
                    if (s->picnum == 1205)
                        s->picnum = 1203;
                }
                if (s->picnum == 1202 || s->picnum == 1203)
                    s->cstat ^= 4;
                s->xvel -= 6;
                if ((krand() >> 8) >= 191)
                    spawn(i, 686);
            }
            if (MOVE_ALONG(i, s->zvel, 0) && sector[sect].floorz - 1024 < s->z) {
                s->zvel = (short)-s->zvel;
                if (++t[0] == 3) {
                    s->z = sector[sect].floorz - 512;
                    changespritestat(i, 0);
                    continue;
                }
            }
        }
        if (s->picnum >= 870 && s->picnum <= 873) {
            if (t[2]) {
                if (t[4] < 256)
                    t[4]++;
                else if (dist > 4096)
                    deletesprite(i);
                continue;
            }
            t[0]++;
            if (t[0] & 1)
                s->picnum++;
            if (s->picnum == HEADPIECE)
                s->picnum = ARMPIECE;
            s->xvel = s->xvel <= 0 ? 0 : (short)(s->xvel - 1);
            s->zvel = (short)(s->zvel + 192);
            if (MOVE_ALONG(i, s->zvel, 0)) {
                t[1]++;
                s->zvel = (short)-s->zvel;
            }
            if ((krand() >> 8) >= 239) {
                long speed = krand() & 32;

                THROW_BLOOD(i, i, (short)t[0], speed);
            }
            if (s->z + 2048 > sector[sect].floorz && t[1] > 1) {
                s->picnum = 871;
                s->z = sector[sect].floorz - 512;
                t[2] = 1;
                s->cstat |= krand() & 4;
            }
        }
        if (s->picnum >= 874 && s->picnum <= 877) {
            if (t[2]) {
                if (t[4] < 256)
                    t[4]++;
                else if (dist > 4096)
                    deletesprite(i);
                continue;
            }
            t[0]++;
            if (t[0] & 1)
                s->picnum++;
            if (s->picnum == LEGPIECE)
                s->picnum = HEADPIECE;
            s->xvel = s->xvel <= 0 ? 0 : (short)(s->xvel - 1);
            s->zvel = (short)(s->zvel + 128);
            if (MOVE_ALONG(i, s->zvel, 0)) {
                t[1]++;
                s->zvel = (short)-s->zvel;
            }
            if ((krand() >> 8) >= 239) {
                long speed = krand() & 32;

                THROW_BLOOD(i, i, (short)t[0] << 7, speed);
            }
            if (s->z + 2048 > sector[sect].floorz && t[1] > 1) {
                s->picnum = 875;
                s->z = sector[sect].floorz - 512;
                t[2] = 1;
                s->cstat |= krand() & 4;
            }
        }
        if (s->picnum >= 878 && s->picnum <= 881) {
            if (t[2]) {
                if (t[4] < 256)
                    t[4]++;
                else if (dist > 4096)
                    deletesprite(i);
                continue;
            }
            t[0]++;
            if (t[0] & 1)
                s->picnum++;
            if (s->picnum == 882)
                s->picnum = LEGPIECE;
            s->xvel = s->xvel <= 0 ? 0 : (short)(s->xvel - 1);
            s->zvel = (short)(s->zvel + 192);
            if (MOVE_ALONG(i, s->zvel, 0)) {
                t[1]++;
                s->zvel = (short)-s->zvel;
            }
            if ((krand() >> 8) >= 239) {
                long speed = krand() & 32;

                THROW_BLOOD(i, i, (short)t[0] << 7, speed);
            }
            if (s->z + 4096 > sector[sect].floorz && t[1] > 1) {
                s->picnum = 879;
                s->z = sector[sect].floorz - 512;
                t[2] = 1;
                s->cstat |= krand() & 4;
            }
        }
        if (s->picnum == FLAIR) {
            long a;

            if (t[0] == 0)
                t[1] = (krand() & 3) + 1;
            t[0]++;
            s->xrepeat = s->xrepeat == 96 ? 191 : (unsigned char)(s->xrepeat - 48);
            s->yrepeat -= 3;
            if (t[0] == t[1])
                for (a = 0; a < 2048; a += 768) {
                    s->ang = (short)((unsigned char)randomstash + a);
                    spawn(i, EXPLOSION);
                }
            if (t[0] > 3)
                deletesprite(i);
            continue;
        }
        if (s->picnum >= 235 && s->picnum <= 246) {
            short old = s->picnum;

            s->picnum = old == 238 ? 235 : old == 242 ? 239 : old == 246 ? 243 : (short)(old + 1);
            if (klabs(sector[sect].ceilingz - sector[sect].floorz) < 7680)
                s->xvel = 64;
            if (MOVE_ALONG(i, s->zvel, 1) == 0 && s->z < sector[sect].floorz) {
                if ((s->zvel >> 2) < 1024)
                    s->zvel += 128;
                s->z += s->zvel;
                if (s->xvel > 16)
                    s->xvel -= 4;
            } else if (++t[0] != 2) {
                s->zvel = (short)-s->zvel;
            } else {
                deletesprite(i);
            }
        }
    }
}

void moveplayers(void)
{
    short i, nexti;
    short frameang;
    unsigned char tics;

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
                sprite[i].ang = ps[sprite[i].yvel].ang;
                tics = script_tics;
                sprite[i].yvel++;
                frameang = (short)(sprite[i].ang & 2047);
                sprite[i].xvel = 128;
                movesprite(i, (sprite[i].xvel * sintable[(sprite[i].ang + 512) & 2047]) >> 14,
                    (sintable[frameang] * sprite[i].xvel) >> 14,
                    sprite[i].zvel, 128, 1024, 1024, 0, tics);
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
                    playmusic(song);
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
        spritetype *s = &sprite[i];
        long d;
        short sect = s->sectnum, p;

        nexti = nextspritestat[i];
        if (s->picnum == SEENINE) {
            /* A force field post; one that dies sets every post of its hitag to
               -32, and each of those then bursts. */
            if (s->shade == -32) {
                spawn(i, EXPLOSION);
                s->z -= 0x2000;
                shoot(i, OTHERSHOTSPARK, 1);
                s->z += 0x2000;
                spawn(i, EXPLOSION);
                spawn(i, EXPLOSION);
                deletesprite(i);
            } else {
                HIT_BY_WEAPON(i, j);
                if ((j >= 0 && s->extra < 0) || s->shade == -31) {
                    for (j = headspritestat[6]; j >= 0; j = nextspritestat[j])
                        if (s->hitag == sprite[j].hitag && sprite[j].picnum == SEENINE)
                            sprite[j].shade = -32;
                }
            }
        } else if (s->picnum >= CRANE && s->picnum < CRANE + 4) {
            long *t = hittype[i];

            s->xrepeat = s->yrepeat = 64;
            if (t[0] == 0) {
                if (checkcursectnums((short)t[1]) < 0) {
                    FIND_IN_SECTOR((short)t[1], 627, j);
                    if (j >= 0) {
                        s->ang = (short)getangle(msx[t[4] + 1] - s->x, msy[t[4] + 1] - s->y);
                        setsprite(j, msx[t[4] + 1], msy[t[4] + 1], sprite[j].z);
                        t[0]++;
                        continue;
                    }
                } else {
                    s->ang = (short)getangle(msx[t[4] + 1] - s->x, msy[t[4] + 1] - s->y);
                    t[0]++;
                }
            } else if (t[0] == 1) {
                if (s->xvel < 184) {
                    s->picnum = 1176;
                    s->xvel += 8;
                }
                MOVE_ALONG(i, s->zvel, 0);
                if (sect == t[1])
                    t[0]++;
            } else if (t[0] == 2 || t[0] == 7) {
                s->z += 1536;
                if (t[0] == 2) {
                    if (sector[sect].floorz - s->z < 12288 && s->picnum > CRANE)
                        s->picnum--;
                    if (sector[sect].floorz - s->z < 5120)
                        t[0]++;
                }
                if (t[0] == 7 && sector[sect].floorz - s->z < 5120) {
                    if (s->picnum > CRANE)
                        s->picnum--;
                    else {
                        if (s->owner == -2) {
                            p = findplayer(i, &d);
                            if (ps[p].crane == i)
                                ps[p].crane = -1;
                        }
                        t[0]++;
                        s->owner = -1;
                    }
                }
            } else if (t[0] == 3) {
                if (++s->picnum == 1177) {
                    p = checkcursectnums((short)t[1]);
                    if (p >= 0 && ps[p].on_ground) {
                        s->owner = -2;
                        ps[p].ang = (short)getangle(s->x - ps[p].posx, s->y - ps[p].posy);
                        ps[p].crane = i;
                    } else {
                        FIND_IN_SECTOR((short)t[1], 627, j);
                        if (j >= 0)
                            s->owner = j;
                    }
                    t[2] = 0;
                    t[0]++;
                    continue;
                }
            } else if (t[0] == 4) {
                if (++t[2] > 10)
                    t[0]++;
            } else if (t[0] == 5 || t[0] == 8) {
                if (t[0] == 8 && s->picnum < 1177 && sector[sect].floorz - s->z > 0x2000)
                    s->picnum++;
                if (s->z >= msx[t[4] + 2])
                    s->z -= 1536;
                else {
                    t[0]++;
                    s->xvel = 0;
                }
            } else if (t[0] == 6) {
                if (s->xvel < 192)
                    s->xvel += 8;
                s->ang = (short)getangle(msx[t[4]] - s->x, msy[t[4]] - s->y);
                MOVE_ALONG(i, s->zvel, 0);
                if ((s->y - msy[t[4]]) * (s->y - msy[t[4]]) + (s->x - msx[t[4]]) * (s->x - msx[t[4]]) < 0x4000)
                    t[0]++;
            } else if (t[0] == 9) {
                t[0] = 0;
            }
            setsprite((short)msy[t[4] + 2], s->x, s->y, s->z - 8704);
            if (s->owner == -1)
                continue;
            p = findplayer(i, &d);
            HIT_BY_WEAPON(i, j);
            if (j >= 0) {
                if (s->owner == -2 && ps[p].crane == i)
                    ps[p].crane = -1;
                s->owner = -1;
                s->picnum = CRANE;
            }
            if (s->owner == -2) {
                playertype *pl = &ps[p];

                if (pl->crane == i && p == screenpeek)
                    s->xrepeat = s->yrepeat = 0;
                pl->posx = s->x - (sintable[(pl->ang + 512) & 2047] >> 6);
                pl->posy = s->y - (sintable[pl->ang & 2047] >> 6);
                pl->posz = s->z - 7168;
                clipmove(&pl->posx, &pl->posy, &pl->posz, &pl->cursectnum, 0, 0, 192, 1024, 1024, 0);
                setsprite(pl->i, pl->posx, pl->posy, pl->posz);
            } else if (s->owner >= 0) {
                setsprite(s->owner, s->x, s->y, s->z);
            }
        } else if (s->picnum == CANWITHSOMETHING) {
            FALL(s);
            HIT_BY_WEAPON(i, j);
            if (j >= 0) {
                if (s->lotag)
                    EGS(sect, s->x, s->y, s->z - 4096, 1, s->lotag, -16, 32, 32, 0, 0, -1536, sprite[j].owner, 1);
                EGS(sect, s->x, s->y, s->z - 4096, 1, 1201, s->shade, 32, 32, sprite[j].ang, 128, -1024,
                    sprite[j].owner, 5);
                EGS(sect, s->x, s->y, s->z, 1, 1204, s->shade, 28, 32, sprite[j].ang - 512, 128, -1024,
                    sprite[j].owner, 5);
                EGS(sect, s->x, s->y, s->z, 1, 1205, s->shade, 28, 32, sprite[j].ang + 512, 128, -1024,
                    sprite[j].owner, 5);
                deletesprite(i);
            }
        } else {
            p = findplayer(i, &d);
            if (s->picnum == 627 || s->picnum == 905 || s->picnum == 974)
                execute(i, p, d);
        }
    }
}
