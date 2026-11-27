/* ACTORS: per-tick sprite updates by status list (1 actors, 2 sleeping actors, 4 projectiles, 5 effects, 6 standables, 10 players, 11 music/sound fx) plus small spawn helpers (guts, blood, gibs, nuke, hitradius).
   Status 1/2 keep Ken Silverman's Sep-94 GAME.C meanings ('active monster' / 'monster waiting for you'); the rest is 3D Realms code and the direct ancestor of Duke Nukem 3D 1.5 ACTORS.C (1996):
   ms, movefta, ifsquished, ifhitsectors, ifhitbyweapon, ssp, guts, hitradius, moveweapons, moveexplosions (statnum 5), movestandables, moveplayers, movefx keep their roles there.
   Hard-coded tiles are DEFS.CON names; everything else is handed to the CON interpreter execute(). BINARY_DERIVED. */

#include "actors.h"

/* Ken's PRAGMAS.H sign helper, also used by PREMAP. */
extern long ksgn(long value);
#pragma aux ksgn = "add ebx,0x80000000" "sbb eax,eax" "add ebx,0x7fffffff" "adc eax,0" \
    parm nomemory [ebx] modify exact [eax ebx];

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
extern struct user_defs ud;         /* the 21-byte USERDEFS.TMP block defined by GLOBAL */

/* dataown: one object, one name. Code uses the fields of GLOBAL's settings block ud directly; the unused
   externs disable_armor and music_volume in actors.h only keep this unit's symbol slots (docs/BSS_LAYOUT.md). */

/* Give player p the weapon tile and switch to it if new or force is set (Duke 1.5 addweapon role). */
void addweapon(short p, unsigned short weapon, unsigned char force)
{
    switch (weapon) {
    case CLUB:
        if (ps[p].gotweapon[0] == 0 || force) {
            ps[p].weapon_pos = 5;
            ps[p].last_weapon = ps[p].current_weapon;
            ps[p].current_weapon = CLUB;
        }
        ps[p].gotweapon[0] = 1;
        break;
    case SHOTSPARK1:
        if (ps[p].gotweapon[1] == 0 || force) {
            ps[p].weapon_pos = 5;
            ps[p].last_weapon = ps[p].current_weapon;
            ps[p].current_weapon = SHOTSPARK1;
        }
        ps[p].gotweapon[1] = 1;
        break;
    case BULLET:
        if (ps[p].gotweapon[2] == 0 || force) {
            ps[p].weapon_pos = 5;
            ps[p].last_weapon = ps[p].current_weapon;
            ps[p].current_weapon = BULLET;
        }
        ps[p].gotweapon[2] = 1;
        break;
    case HEAVYHBOMB:
        if (ps[p].gotweapon[3] == 0 || force) {
            ps[p].weapon_pos = 5;
            ps[p].last_weapon = ps[p].current_weapon;
            ps[p].current_weapon = HEAVYHBOMB;
        }
        ps[p].gotweapon[3] = 1;
        break;
    case RPG:
        if (ps[p].gotweapon[4] == 0 || force) {
            ps[p].weapon_pos = 5;
            ps[p].last_weapon = ps[p].current_weapon;
            ps[p].current_weapon = RPG;
        }
        ps[p].gotweapon[4] = 1;
        break;
    }
    ps[p].kickback_pic = 0;
}

/* Damage everything in status 1/6/10 within r of sprite i by spawning RADIUSEXPLOSION damage sprites. */
void hitradius(short i, long r, short hp)
{
    unsigned char statlist[3] = { 1, 6, 10 };
    short k, j, hit;
    long dx, dy, dy_squared;

    for (k = 0; k < 3; k++)
        for (j = headspritestat[statlist[k]]; j >= 0; j = nextspritestat[j]) {
            dy = sprite[i].y - sprite[j].y;
            dy_squared = dy * dy;
            dx = sprite[i].x - sprite[j].x;
            if (dx * dx + dy_squared < r * r) {
                sprite[i].ang = getangle(sprite[j].x - sprite[i].x, sprite[j].y - sprite[i].y);
                if (sprite[j].extra >= 0) {
                    hit = EGS(sprite[j].sectnum, sprite[j].x, sprite[j].y, sprite[j].z,
                        0, RADIUSEXPLOSION, -16, 0, 0, sprite[i].ang, 32, 0, sprite[i].owner, 4);
                    sprite[hit].extra = hp;
                }
            }
        }
}

/* Spawn one moving gib (status 5) of tile gtype from sprite i. */
void spawn_gib(short i, short gtype, short ang, short speed, long zvel)
{
    EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z - (36 << 8),
        0, gtype, -16, 24, 24, ang, speed, zvel, i, 5);
}

#define MOVE_ALONG(i, dz, clip) \
    movesprite((i), (sprite[(i)].xvel * sintable[(sprite[(i)].ang + 512) & 2047]) >> 14, \
        (sintable[(short)(sprite[(i)].ang & 2047)] * sprite[(i)].xvel) >> 14, \
        (dz), 128, 1024, 1024, (clip), synctics)

/* Move sprite i by its velocity; 1 if the move was not blocked. */
short ssp(short i, short cliptype)
{
    return MOVE_ALONG(i, sprite[i].zvel, cliptype) == 0;
}

/* CON 'nuke': FLAIR flash plus EXPLOSION at (x,y,z) from sprite i. */
void nuke(long x, long y, long z, short i)
{
    EGS(sprite[i].sectnum, x, y, z + (20 << 8), 2, FLAIR, -127, 96, 48, 0, 0, 0, i, 5);
    spawn(i, EXPLOSION);
}

/* Spawn one BLOOD drop from sprite i. */
void spawn_blood(short i, short at, short ang, short speed)
{
    EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[at].z,
        0, BLOOD, -16, 24, 13, ang + (krand() & 127),
        speed + (krand() & 31), -(krand() & 511), i, 5);
}

/* Move sector effector i and drag its walls (msx/msy from T2, angle T3). */
void ms(short i)
{
    long ty, tx;
    short sn, w, endwall;
    long j;

    sprite[i].x += (sprite[i].xvel * sintable[(sprite[i].ang + 512) & 2047]) >> 14;
    sn = sprite[i].sectnum;
    sprite[i].y += (sprite[i].xvel * sintable[(short)(sprite[i].ang & 2047)]) >> 14;
    j = hittype[i][1];
    w = sector[sn].wallptr;
    endwall = sector[sn].wallnum + w;
    for (; w < endwall;) {
        long wx = msx[j];
        rotatepoint(0, 0, wx, msy[j++], hittype[i][2] & 2047, &tx, &ty);
        dragpoint(w++, tx + sprite[i].x, ty + sprite[i].y);
    }
    setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
}

/* Wake status-2 actors that can see a player (Duke 1.5 movefta; check_fta_sounds inline). */
void movefta(void)
{
    short i, nexti, p;

    for (i = headspritestat[STAT_ZOMBIEACTOR]; i >= 0; i = nexti) {
        long dist;

        nexti = nextspritestat[i];
        p = findplayer(i, &dist);
        if (cansee(sprite[i].x, sprite[i].y, sprite[i].z - (sprite[i].yrepeat << 7),
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
            changespritestat(i, STAT_ACTOR);
        }
    }
}

/* Crush test for sprite i in a closing sector; returns i if squished, else -1. */
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
            ps[p].squashable++;
            sprite[result].xvel = 0;
            sprite[result].cstat &= ~1;
            FTA(10);
            return result;
        case 579:
            sprite[i].xvel = 0;
            ps[p].squashable++;
            FTA(10);
            return i;
        }
        return result;
    }
    return -1;
}

/* First status-5 effect sprite in sprite i's sector, or -1 (Duke 1.5 ifhitsectors). */
short ifhitsectors(short i)
{
    short j, sn = sprite[i].sectnum;

    for (j = headspritestat[STAT_MISC]; j >= 0; j = nextspritestat[j])
        if (sn == sprite[j].sectnum && sprite[j].picnum != LASERSITE)
            return j;
    return -1;
}

/* Projectile (status 4) touching sprite i: apply its damage and return it, else -1. */
short ifhitbyweapon(short i)
{
    short j;

    for (j = headspritestat[STAT_PROJECTILE]; j >= 0; j = nextspritestat[j])
        if (i != j && i != sprite[j].owner
            && klabs(sprite[i].x - sprite[j].x) + klabs(sprite[i].y - sprite[j].y) < 768
            && klabs((sprite[i].z - sprite[j].z) >> 8) <= 48) {
            if (sprite[i].statnum != STAT_EFFECTOR) {
                sprite[i].extra -= sprite[j].extra;
                sprite[i].xvel = sprite[j].xvel;
                sprite[i].ang = sprite[j].ang;
                if ((sprite[i].statnum == STAT_ACTOR || sprite[i].statnum == STAT_PLAYER)
                    && sprite[j].picnum == BULLET)
                    spawn(i, STICKSPARK);
            }
            return j;
        }
    return -1;
}

/* CON 'guts': throw arm, leg and head pieces from sprite i. */
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
    if (ud.debug == 0 && ps[(p)].shield_amount > 0) \
        ps[(p)].shield_amount -= 4; \
    else if (ps[(p)].health > 0) \
        ps[(p)].health -= (damage); \
    else \
        ps[(p)].health = 0; \
} while (0)

/* Update active actors (status 1): hard-coded tiles here, everything else through CON execute(). */
void moveactors(void)
{
    /* COMPILER_STEERED: the player index scope affects the case offsets. */
    short i, j, nexti, p, sect;
    long l, dist;

    for (i = headspritestat[STAT_ACTOR]; i >= 0; i = nexti) {
        long *t;

        nexti = nextspritestat[i];
        p = findplayer(i, &dist);
        sect = sprite[i].sectnum;
        t = hittype[i];
        if (sprite[i].picnum == CAM1) {
            if (dist < 2048)
                camsprite = i;
            continue;
        }
        if (sprite[i].picnum == MASTERSWITCH) {
            /* Fire an armed master switch when its countdown expires. */
            if (sprite[i].yvel != 1 || --sprite[i].hitag > 0)
                continue;
            operatesectors(sect, i);
            for (j = headspritesect[sect]; j >= 0; j = nextspritesect[j]) {
                short pic;

                if (sprite[j].statnum == STAT_EFFECTOR)
                    switch (sprite[j].lotag) {
                    case 2:
                    case 21:
                        hittype[j][0] = 1;
                        break;
                    case 3:
                        hittype[j][4] = 1;
                        break;
                    case 23:
                        sprite[i].shade = 1;
                        break;
                    }
                else if (sprite[j].statnum == STAT_STANDABLE) {
                    pic = sprite[j].picnum;
                    if (pic == SEENINE)
                        sprite[j].shade = -31;
                }
            }
            deletesprite(i);
            continue;
        }
        if (sprite[i].picnum == SHARK) {
            unsigned char frames[4] = { 0, 1, 0, 3 };

            if (MOVE_ALONG(i, sprite[i].zvel, 0))
                sprite[i].ang += 1024;
            t[1]++;
            j = ifhitbyweapon(i);
            if (j >= 0) {
                if (sprite[i].extra < 0) {
                    ps[screenpeek].score = ps[screenpeek].score + 5000;
                    spawn(i, EXPLOSION);
                    deletesprite(i);
                    continue;
                }
                t[2] = -1;
                sprite[i].ang += 1024;
            }
            switch (t[2]) {
            case 0:
                if ((krand() >> 8) >= 251) {
                    t[2] = 1;
                    goto shark_steer;
                }
                /* fall through */
            case -1:
                if (t[2] == -1)
                    t[2]++;
shark_steer:
                /* j keeps the hit index when the shark steers toward the player. */
                if ((krand() >> 8) >= 247)
                    sprite[i].ang += getincangle(sprite[i].ang, getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y), 2);
                else {
                    j = sintable[(t[1] << 4) & 2047] >> 8;
                    sprite[i].ang += j;
                }
                if (j > 32 || j < -32) {
                    /* COMPILER_STEERED: ternary changes the case layout. */
                    t[0] = j < 0 ? 4 : 2;
                    if (sprite[i].xvel > 64)
                        sprite[i].xvel -= 8;
                } else {
                    t[0] = frames[t[1] & 3];
                    if (sprite[i].xvel < 112)
                        sprite[i].xvel += 8;
                }
                break;
            case 1:
                if (ps[p].posz + 768 < sprite[i].z)
                    sprite[i].z -= 1024;
                else if (ps[p].posz - 768 > sprite[i].z)
                    sprite[i].z += 1024;
                sprite[i].ang = sprite[i].ang + getincangle(sprite[i].ang, getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y), 3);
                if (sprite[i].xvel < 144) {
                    t[0] = frames[t[1] & 3];
                    sprite[i].xvel += 16;
                } else
                    t[0] = 0;
                if (dist < 768) {
                    if ((krand() >> 8) >= 247)
                        t[2] = 2;
                    else
                        t[2] = 0;
                    sprite[i].ang += 1024;
                }
                break;
            case 2:
                sprite[i].x = ps[p].posx + (sintable[(ps[p].ang + 512) & 2047] >> 6);
                sprite[i].y = (sintable[ps[p].ang & 2047] >> 6) + ps[p].posy;
                sprite[i].z = ps[p].posz + 3072;
                t[0] = frames[t[1] & 3];
                ps[p].ang += sintable[(t[1] << 8) & 2047] >> 12;
                sprite[i].ang = ps[p].ang + 1024;
                if ((krand() >> 8) >= 223)
                    t[2] = 0;
                break;
            }
            continue;
        }
        if (sprite[i].picnum == OOZ) {
            if (++t[0] > 1024) {
                deletesprite(i);
                continue;
            }
            if (dist > 0x2000) {
                deletesprite(i);
                continue;
            }
            j = klabs(sector[sect].ceilingz - sector[sect].floorz) >> 9;
            sprite[i].yrepeat = j;
            sprite[i].xrepeat = 25 - (j >> 1);
            sprite[i].z = sector[sect].floorz;
            continue;
        }
        if (sprite[i].picnum >= 961 && sprite[i].picnum <= 968) {
            t[1] += 128;
            if (t[0] == -4) {
                setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
                sprite[i].cstat &= ~8;
                sprite[i].ang = ps[p].ang;
                if (syncbits[p] & SB_FIRE) {
                    ps[p].kickback_pic = 0;
                    sprite[i].xvel = 128;
                    sprite[i].zvel = ((100 - ps[p].horiz) << 4) - 512;
                    t[0] = 0;
                    ps[p].somethingonplayer = -1;
                }
                sprite[i].z = ps[p].posz - t[2] + 2048;
                sprite[i].z += (100 - ps[p].horiz) << 4;
                if (t[2] > 512)
                    t[2] -= 128;
                if (t[2] < 348)
                    t[2] += 128;
                if (t[3] > 0) {
                    short frames[8] = { 5, 5, 6, 6, 7, 7, 6, 5 };

                    sprite[i].picnum = 961 + frames[t[3]];
                    if (t[3] == 4) {
                        HURT(p, 5);
                        spritesound(13, ps[p].i);
                    }
                    if (t[3] < 7)
                        t[3]++;
                    else
                        t[3] = 0;
                } else {
                    sprite[i].picnum = 966;
                    if ((krand() >> 8) >= 223)
                        t[3] = 1;
                }
                sprite[i].xrepeat = (sintable[t[1] & 2047] >> 13) + 20;
                sprite[i].yrepeat = (sintable[t[1] & 2047] >> 13) + 15;
                sprite[i].x = ps[p].posx + (sintable[(ps[p].ang + 512) & 2047] >> 7);
                sprite[i].y = ps[p].posy + (sintable[ps[p].ang & 2047] >> 7);
                continue;
            }
            if (sprite[i].xvel < 64 && dist < 512 && ps[p].somethingonplayer == -1) {
                ps[p].somethingonplayer = i;
                if (t[0] == 3 || t[0] == 2)
                    t[2] = 3072;
                else
                    t[2] = -3328;
                t[0] = -4;
            } else if (t[0] == -3) {
                if (sprite[i].xrepeat > 4)
                    sprite[i].xrepeat -= 4;
                if (sprite[i].yrepeat <= 4)
                    deletesprite(i);
                else
                    sprite[i].yrepeat -= 4;
                continue;
            }
            j = ifhitbyweapon(i);
            if (j >= 0) {
                ps[p].score += 1000;
                if (sprite[j].picnum == BULLET) {
                    t[0] = -3;
                    continue;
                }
                if ((krand() >> 8) >= 251 && klabs(sector[sect].floorz - sector[sect].ceilingz) < 49152
                    && (t[0] == 0 || t[0] == 2))
                    t[0]++;
            }
            if (t[0] == -1) {
                FALL(&sprite[i]);
                sprite[i].picnum = 965;
                sprite[i].cstat &= ~8;
                if (sprite[i].yrepeat > 62)
                    guts(i);
                if (sprite[i].xrepeat > 32)
                    sprite[i].xrepeat -= krand() & 7;
                if (sprite[i].yrepeat > 16)
                    sprite[i].yrepeat -= krand() & 7;
                else {
                    sprite[i].xrepeat = 40;
                    sprite[i].yrepeat = 16;
                    sprite[i].owner = -1;
                    t[0] = 0;
                }
                continue;
            }
            if (t[0] == -2) {
                FALL(&sprite[i]);
                sprite[sprite[i].owner].xvel = 0;
                l = sprite[sprite[i].owner].ang;
                sprite[i].z = sprite[sprite[i].owner].z;
                sprite[i].x = sprite[sprite[i].owner].x + (sintable[(l + 512) & 2047] >> 11);
                sprite[i].y = sprite[sprite[i].owner].y + (sintable[l & 2047] >> 11);
                sprite[i].picnum = 963 + (randomstash & 1);
                if (sprite[i].yrepeat < 64)
                    sprite[i].yrepeat += 2;
                else if (sprite[i].xrepeat < 32)
                    sprite[i].xrepeat += 4;
                else {
                    t[0] = -1;
                    dist = klabs(sprite[i].x - sprite[sprite[i].owner].x) + klabs(sprite[i].y - sprite[sprite[i].owner].y);
                    if (dist < 768)
                        deletesprite(sprite[i].owner);
                }
                continue;
            }
            /* Check randomly for an actor to swallow. */
            if ((krand() >> 8) >= 223)
                for (j = headspritesect[sect]; j >= 0; j = nextspritesect[j])
                    switch (sprite[j].picnum) {
                    case 408: case 490: case 1536: case 1680: case 1820:
                        if (klabs(sprite[i].x - sprite[j].x) + klabs(sprite[i].y - sprite[j].y) < 768
                            && klabs(sprite[i].z - sprite[j].z) < 8192) {
                            sprite[i].owner = j;
                            t[0] = -2;
                            t[1] = 0;
                            goto BOLT;
                        }
                    }
            /* Moving on the ground or ceiling. */
            if (t[0] == 0 || t[0] == 2) {
                sprite[i].picnum = GREENSLIME;
                if (t[0] == 2) {
                    sprite[i].zvel = 0;
                    sprite[i].cstat &= ~8;
                    if (sector[sect].ceilingz + 6144 < sprite[i].z) {
                        sprite[i].z += 2048;
                        t[0]++;
                        continue;
                    }
                } else {
                    sprite[i].cstat |= 8;
                    FALL(&sprite[i]);
                }
                MOVE_ALONG(i, sprite[i].zvel, 0);
                if (sprite[i].xvel > 96) {
                    sprite[i].xvel -= 2;
                    continue;
                }
                if (sprite[i].xvel < 32)
                    sprite[i].xvel += 4;
                sprite[i].xvel = 64 - (sintable[(t[1] + 512) & 2047] >> 9);
                sprite[i].ang += getincangle(sprite[i].ang, getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y), 3);
                sprite[i].xrepeat = (sintable[(t[1] + 512) & 2047] >> 11) + 36;
                sprite[i].yrepeat = (sintable[t[1] & 2047] >> 13) + 16;
                if ((krand() >> 8) >= 251 && klabs(sector[sect].floorz - sector[sect].ceilingz) < 49152) {
                    sprite[i].zvel = 0;
                    t[0]++;
                }
            }
            if (t[0] == 1) {
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
                    t[0]++;
                }
            }
            if (t[0] == 3) {
                sprite[i].picnum = 962;
                if (sprite[i].zvel < 3072)
                    sprite[i].zvel += 348;
                sprite[i].z += sprite[i].zvel;
                if (sector[sect].floorz - 0x2000 < sprite[i].z) {
                    sprite[i].yrepeat -= 4;
                    sprite[i].xrepeat += 2;
                } else {
                    if (sprite[i].yrepeat < 36)
                        sprite[i].yrepeat += 8;
                    if (sprite[i].xrepeat > 8)
                        sprite[i].xrepeat -= 4;
                }
                if (sector[sect].floorz - 2048 < sprite[i].z) {
                    sprite[i].z = sector[sect].floorz - 2048;
                    t[0] = 0;
                    sprite[i].xvel = 0;
                }
            }
        } else if (sprite[i].picnum == WATERDRIP) {
            /* A drip: grows on the ceiling, falls, splashes. */
            if (dist > 0x2000)
                continue;
            if (t[0] == 0) {
                if (t[1] > 0)
                    t[1]--;
                else {
                    sprite[i].yrepeat++;
                    sprite[i].xrepeat -= 2;
                    if (sprite[i].yrepeat > 8) {
                        t[0] = 1;
                        sprite[i].xrepeat = 8;
                    }
                }
            }
            if (t[0] == 1) {
                FALL(&sprite[i]);
                if (sector[sect].floorz - 2048 < sprite[i].z) {
                    t[0] = 2;
                    spritesound(17, i);
                }
            }
            if (t[0] == 2) {
                sprite[i].xrepeat += 8;
                sprite[i].yrepeat -= 2;
                if (sprite[i].yrepeat < 2) {
                    sprite[i].z = sector[sect].ceilingz + 512;
                    t[0] = 0;
                    t[1] = krand() & 31;
                    sprite[i].xrepeat = 16;
                    sprite[i].yrepeat = 0;
                }
            }
        } else if (sprite[i].picnum >= 930 && sprite[i].picnum <= 931) {
            /* Floor (930) and ceiling (931) turrets. */
            j = ifhitbyweapon(i);
            if (j >= 0) {
                t[4] = 0;
                if (sprite[i].extra < 0) {
                    ps[p].score += 1000;
                    spawn(i, EXPLOSION);
                    for (dist = 0; dist < 30; dist += 3)
                        EGS(sect, sprite[i].x, sprite[i].y, sprite[i].z, 0, krand() % 11 + 235, -8, 32, 32,
                            krand() & 2047, (krand() & 128) + 64, -(krand() & 511) - 512, i, 5);
                    ps[p].actors_killed++;
                    t[0] = 2;
                    sprite[i].xrepeat = sprite[i].yrepeat = 0;
                }
            }
            if (t[0] == 0) {
                t[4] = 0;
                if (sprite[i].xrepeat == 0) {
                    deletesprite(i);
                    continue;
                }
                if (dist < 4096) {
                    if (klabs(sprite[i].z - ps[p].posz) < 2048) {
                        t[1] = 0;
                        t[0]++;
                    } else {
                        j = ksgn(ps[p].posz - sprite[i].z) << 11;
                        sprite[i].z += j;
                        if (sprite[i].picnum == 930)
                            sector[sect].floorz += j;
                        else
                            sector[sect].ceilingz += j;
                    }
                }
            }
            if (t[0] == 1) {
                sprite[i].ang += getincangle(sprite[i].ang, getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y), 1) << 1;
                sprite[i].ang &= 0xff80;
                if (++t[1] < 8) {
                    t[4] ^= 1;
                    if (t[4]) {
                        sprite[i].z -= 0x2000;
                        shoot(i, BULLET, 0);
                        sprite[i].shade = -16;
                        sprite[i].z += 0x2000;
                    }
                } else {
                    sprite[i].shade = 4;
                    t[1] = 0;
                    if (dist > 1024) {
                        t[0] = 4;
                        goto wait;
                    }
                }
            }
            if (t[0] == 2) {
                t[4] = 0;
                if (sprite[i].picnum == 930) {
                    sprite[i].z += 2048;
                    sector[sect].floorz += 2048;
                    if (sector[sect].floorz > t[3]) {
                        t[0] = 0;
                        continue;
                    }
                } else {
                    sprite[i].z -= 2048;
                    sector[sect].ceilingz -= 2048;
                    if (sector[sect].ceilingz < t[3]) {
                        t[0] = 0;
                        continue;
                    }
                }
            }
            if (t[0] > 3) {
wait:
                t[0]++;
                if (dist > 4096)
                    t[0] = 2;
                else if (t[0] > 10)
                    t[0] = 1;
                sprite[i].ang = getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y);
            }
        } else if (sprite[i].picnum == HEAVYHBOMB) {
            /* l: the owner's player (hittype[-1] for an ownerless bomb). */
            l = hittype[sprite[i].owner][0];
            j = ifsquished(i, p);
            if (j >= 0 && sprite[i].extra < 0)
                ps[l].hbomb_on = 0;
            FALL(&sprite[i]);
            if (sprite[i].xvel > 0) {
                sprite[i].xvel -= 4;
                if (sprite[i].xvel & 8)
                    sprite[i].cstat ^= 4;
            } else
                sprite[i].xvel = 0;
            MOVE_ALONG(i, sprite[i].zvel, 0);
            if (ps[l].hbomb_on == 0 && ps[l].i == sprite[i].owner) {
                sprite[i].yvel++;
                if (sprite[i].xrepeat || sprite[i].yrepeat) {
                    sprite[i].xrepeat = sprite[i].yrepeat = 0;
                    hitradius(i, 3092, sprite[i].extra);
                }
                if (sprite[i].yvel & 1)
                    for (j = 0; j < 2047; j += 256)
                        EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z - (sprite[i].yvel << 10), 0,
                            EXPLOSION, -127, 48, 48, j, 0, 0, i, 5);
                if (sprite[i].yvel > 20)
                    deletesprite(i);
            } else if (dist > 512 && dist < 768) {
                ps[p].ammo_h_amount++, addweapon(p, HEAVYHBOMB, 1);
                deletesprite(i);
            }
        } else if (sprite[i].picnum == DOORSHOCK) {
            if (dist > 0x2000)
                continue;
            j = klabs(sector[sect].ceilingz - sector[sect].floorz) >> 9;
            sprite[i].xrepeat = 16;
            sprite[i].yrepeat = j + 8;
            sprite[i].z = sector[sect].floorz;
        } else if (sprite[i].picnum == 32) {
            /* Touch plate: range (lotag + hitag << 16) >> 13, or a player standing in its sector. */
            l = (sprite[i].lotag + (sprite[i].hitag << 16)) >> 13;
            if (l != 0 && sprite[i].hitag != 0) {
                if (dist < l && klabs(sprite[i].z - ps[p].posz) < 12288) {
                    if (t[0] == 0) {
                        t[0] = 1;
                        operatemasterswitches(sprite[i].lotag);
                        operateactivators(sprite[i].lotag);
                    }
                } else
                    t[0] = 0;
            } else {
                j = checkcursectnums(sect);
                if (j >= 0 && ps[j].on_ground) {
                    if (t[0] == 0) {
                        t[0] = 1;
                        operatemasterswitches(sprite[i].lotag);
                        operateactivators(sprite[i].lotag);
                    }
                } else
                    t[0] = 0;
            }
        } else if (sprite[i].picnum >= 305 && sprite[i].picnum <= 308) {
            /* Flickering light. */
            if (dist > 0x2000)
                continue;
            if (t[3] == 0)
                t[3] = sector[sect].floorshade;
            for (;;) {
                if (t[2]) {
                    t[2]--;
                    sector[sect].floorshade = 20;
                    sector[sect].ceilingshade = 20;
                    goto BOLT;
                }
                if ((sprite[i].xrepeat | sprite[i].yrepeat) == 0) {
                    sprite[i].xrepeat = t[0];
                    sprite[i].yrepeat = t[1];
                    break;
                }
                if (krand() & 8)
                    break;
                t[0] = sprite[i].xrepeat;
                t[1] = sprite[i].yrepeat;
                t[2] = randomstash & 4;
                sprite[i].xrepeat = sprite[i].yrepeat = 0;
            }
            l = randomstash & 7;
            sprite[i].xrepeat = l + 8;
            sprite[i].picnum++;
            if (l & 1)
                sprite[i].cstat ^= 2;
            if (dist < 512)
                spritesound(24, i);
            if (sprite[i].picnum == 309)
                sprite[i].picnum = 305;
            if (sprite[i].picnum & 1) {
                sector[sect].floorshade = 0;
                sector[sect].ceilingshade = 0;
            } else {
                sector[sect].floorshade = 20;
                sector[sect].ceilingshade = 20;
            }
        } else if (sprite[i].picnum >= REACTOR && sprite[i].picnum <= 625) {
            if ((krand() >> 8) >= 63)
                sprite[i].picnum = 620 + t[2];
            else
                sprite[i].picnum = 623 + t[2];
            if (++t[2] == 3)
                t[2] = 0;
            if (dist < 4096) {
                t[0] += 128;
                myos(246, 18, 611, (sintable[t[0] & 2047] >> 11) + 8, 1);
                if (t[3] == 0) {
                    spritesound(26, i);
                    t[3] = 1;
                }
            } else {
                t[3] = 0;
                stopsound(26);
            }
            if (t[1]) {
                if (++t[1] == 24) {
                    ps[screenpeek].score += 100000;
                    stopsound(26);
                    deletesprite(i);
                    continue;
                }
                sprite[i].z -= 6144;
                sprite[i].ang += 300;
                shoot(i, 0, 1);
                sprite[i].z += 6144;
                switch (t[1]) {
                case 2:
                    for (j = headspritestat[STAT_EFFECTOR]; j >= 0; j = nextspritestat[j])
                        if (sprite[j].lotag == 3)
                            hittype[j][4] = 1;
                case 4:
                case 7:
                case 10:
                case 15:
                    EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z - (krand() & 0x3FFF) + 5120, 2, FLAIR,
                        -127, 96, 48, 0, 0, 0, i, 5);
                    spawn(i, EXPLOSION);
                    for (j = 0; j < 16; j += 2)
                        EGS(sect, sprite[i].x, sprite[i].y, sprite[i].z + (j << 9), 0, 235, -8, 32, 32,
                            sprite[i].ang, (krand() & 255) + 128, -(krand() & 1023), i, 5);
                }
            }
            if (t[1] == 0) {
                j = ifhitbyweapon(i);
                if (j >= 0 && sprite[i].extra < 0)
                    t[1] = 1;
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
        } else if (sprite[i].picnum == 273) {
            if (dist > 4096)
                continue;
            FALL(&sprite[i]);
            if (++t[0] > 1024) {
                deletesprite(i);
                continue;
            }
            if (MOVE_ALONG(i, sprite[i].zvel, 0)) {
                sprite[i].z = sector[sect].floorz;
                if (sector[sect].lotag == 1)
                    deletesprite(i);
            }
        } else if (sprite[i].picnum == 263) {
            if (dist > 4096)
                continue;
            j = ifhitbyweapon(i);
            if (j >= 0 && sprite[i].extra < 0) {
                sprite[i].picnum = 273;
                t[0] = 0;
                sprite[i].cstat &= ~1;
                continue;
            }
            /* COMPILER_STEERED: copy the state before the switch. */
            l = t[0];
            switch (l) {
            case 0:
                if (dist < 4096)
                    t[0]++;
                break;
            case 1:
                t[0]++;
                sprite[i].ang = getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y);
                if (dist != 0)
                    sprite[i].zvel = ((ps[p].posz + (2048 - sprite[i].z)) << 8) / dist;
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
                    j = 1024 - (randomstash & 2047);
                    sprite[i].ang += j;
                    if (ud.debug == 0 && ps[p].shield_amount > 0) {
                        ps[p].shield_amount -= 4;
                        break;
                    }
                    if (ps[p].health > 0)
                        ps[p].health--;
                    else
                        ps[p].health = 0;
                }
                break;
            case 3:
                if (MOVE_ALONG(i, sprite[i].zvel, 0)) {
                    sprite[i].ang += 512;
                    t[0] = 1;
                }
                break;
            }
        } else
            execute(i, p, dist);
BOLT:
        ;
    }
}

/* Update projectiles (status 4): flight, player hits and impacts, then CON. */
void moveweapons(void)
{
    /* The original compares and passes the low word of findplayer's distance. */
    short p, i, nexti;
    /* COMPILER_STEERED: keep the call result wide before narrowing the player index. */
    int player;

    for (i = headspritestat[STAT_PROJECTILE]; i >= 0; i = nexti) {
        long distance;

        nexti = nextspritestat[i];
        player = findplayer(i, &distance);
        p = player;
        if (sprite[i].picnum == BLUESPARK) {
            execute(i, p, (short)distance);
            continue;
        }
        if (sprite[i].picnum == RADIUSEXPLOSION) {
            if (sprite[i].shade == -15) {
                shoot(i, CLUB, 1);
                if ((short)distance < 512 && sprite[i].owner != ps[p].i) {
                    kickview(p, i);
                    HURT(p, sprite[i].extra);
                }
            } else
                sprite[i].shade = -15;
            execute(i, p, (short)distance);
            continue;
        }
        if (sprite[i].picnum == BOUNCEMINE) {
            execute(i, p, (short)distance);
            continue;
        }
        if (sprite[i].picnum == RPG) {
            if (sprite[i].extra >= 0) {
                if (sprite[i].owner != ps[p].i && (short)distance < 768
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
            execute(i, p, (short)distance);
            continue;
        }
        if (sprite[i].picnum == BULLET) {
            if (sprite[i].extra >= 0) {
                if ((short)distance < 768 && sprite[i].owner != ps[p].i
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
            execute(i, p, (short)distance);
        }
        if (sprite[i].picnum == SHOTSPARK1) {
            if ((short)distance < 512)
                kickview(p, i);
            if (sprite[i].shade == -15)
                shoot(i, 191, 1);
            else
                sprite[i].shade = -15;
            execute(i, p, (short)distance);
        } else if (sprite[i].picnum == OTHERSHOTSPARK) {
            if ((short)distance < 512)
                kickview(p, i);
            if (sprite[i].shade == -15) {
                shoot(i, OTHERSHOTSPARK, 1);
                if (sprite[i].yvel >= 0)
                    spritesound(30, i);
            } else
                sprite[i].shade = -15;
            execute(i, p, (short)distance);
        }
    }
}

/* Update effect sprites (status 5): debris, gibs, casings and splashes; other effects run CON. */
void moveexplosions(void)
{
    short i, nexti;

    for (i = headspritestat[STAT_MISC]; i >= 0; i = nexti) {
        long *t, x;
        short sect, p;

        nexti = nextspritestat[i];
        sect = sprite[i].sectnum;
        t = hittype[i];
        p = findplayer(i, &x);
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
            execute(i, p, x);
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
            } else if (hittype[i][1]) {
                deletesprite(i);
            } else {
                sprite[i].xvel = (short)-sprite[i].xvel;
                sprite[i].zvel = (short)-sprite[i].zvel;
                hittype[i][1] = 1;
            }
            continue;
        }
        if (sprite[i].picnum == WATERSPLASH) {
            if ((sector[sect].lotag & 0x7FFF) != 1) {
                deletesprite(i);
                continue;
            }
            t[0] += 128;
            if (t[0] > 2047) {
                deletesprite(i);
                continue;
            }
            sprite[i].cstat ^= 4;
            sprite[i].yrepeat = sprite[i].xrepeat = (unsigned char)((sintable[t[0]] >> 11) + 8);
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
            if (sprite[i].xvel > 0) {
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
            } else {
                sprite[i].xvel = 0;
                if ((krand() >> 8) >= 127)
                    sprite[i].cstat ^= 0x80;
            }
            /* COMPILER_STEERED: calling ssp changes the inlined move layout. */
            if (!ssp(i, 0) && sector[sect].floorz - 1024 < sprite[i].z) {
                sprite[i].zvel = (short)-sprite[i].zvel;
                if (++t[0] == 3) {
                    sprite[i].z = sector[sect].floorz - 512;
                    changespritestat(i, STAT_DEFAULT);
                    continue;
                }
            }
        }
        if (sprite[i].picnum >= 870 && sprite[i].picnum <= 873) {
            if (t[2]) {
                if (t[4] < 256)
                    t[4]++;
                else if (x > 4096)
                    deletesprite(i);
                continue;
            }
            t[0]++;
            if (t[0] & 1)
                sprite[i].picnum++;
            if (sprite[i].picnum == HEADPIECE)
                sprite[i].picnum = ARMPIECE;
            if (sprite[i].xvel > 0)
                sprite[i].xvel--;
            else
                sprite[i].xvel = 0;
            sprite[i].zvel += 192;
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
                t[2] = 1;
                sprite[i].cstat |= krand() & 4;
            }
        }
        if (sprite[i].picnum >= 874 && sprite[i].picnum <= 877) {
            if (t[2]) {
                if (t[4] < 256)
                    t[4]++;
                else if (x > 4096)
                    deletesprite(i);
                continue;
            }
            t[0]++;
            if (t[0] & 1)
                sprite[i].picnum++;
            if (sprite[i].picnum == LEGPIECE)
                sprite[i].picnum = HEADPIECE;
            if (sprite[i].xvel > 0)
                sprite[i].xvel--;
            else
                sprite[i].xvel = 0;
            sprite[i].zvel += 128;
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
                t[2] = 1;
                sprite[i].cstat |= krand() & 4;
            }
        }
        if (sprite[i].picnum >= 878 && sprite[i].picnum <= 881) {
            if (t[2]) {
                if (t[4] < 256)
                    t[4]++;
                else if (x > 4096)
                    deletesprite(i);
                continue;
            }
            t[0]++;
            if (t[0] & 1)
                sprite[i].picnum++;
            if (sprite[i].picnum == 882)
                sprite[i].picnum = LEGPIECE;
            if (sprite[i].xvel > 0)
                sprite[i].xvel--;
            else
                sprite[i].xvel = 0;
            sprite[i].zvel += 192;
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
                t[2] = 1;
                sprite[i].cstat |= krand() & 4;
            }
        }
        if (sprite[i].picnum == FLAIR) {
            if (t[0] == 0)
                t[1] = (krand() & 3) + 1;
            t[0]++;
            if (sprite[i].xrepeat == 96)
                sprite[i].xrepeat = 191;
            else
                sprite[i].xrepeat -= 48;
            sprite[i].yrepeat -= 3;
            if (t[0] == t[1])
                /* COMPILER_STEERED: reuse the distance slot for the emission loop. */
                for (x = 0; x < 2048; x += 768) {
                    sprite[i].ang = (short)((unsigned char)randomstash + x);
                    spawn(i, EXPLOSION);
                }
            if (t[0] > 3)
                deletesprite(i);
            continue;
        }
        if (sprite[i].picnum >= 235 && sprite[i].picnum <= 246) {
            sprite[i].picnum++;
            if (sprite[i].picnum == 239)
                sprite[i].picnum = 235;
            else if (sprite[i].picnum == 243)
                sprite[i].picnum = 239;
            else if (sprite[i].picnum == 247)
                sprite[i].picnum = 243;
            if (klabs(sector[sect].ceilingz - sector[sect].floorz) < 7680)
                sprite[i].xvel = 64;
            if (ssp(i, 1) && sprite[i].z < sector[sect].floorz) {
                if ((sprite[i].zvel >> 2) < 1024)
                    sprite[i].zvel += 128;
                sprite[i].z += sprite[i].zvel;
                if (sprite[i].xvel > 16)
                    sprite[i].xvel -= 4;
            } else if (++t[0] == 2) {
                deletesprite(i);
            } else {
                sprite[i].zvel = (short)-sprite[i].zvel;
            }
        }
    }
}

/* Update player sprites (status 10) and holoduke decoys. */
void moveplayers(void)
{
    short i, nexti;

    for (i = headspritestat[STAT_PLAYER]; i >= 0; i = nexti) {
        nexti = nextspritestat[i];
        if (sprite[i].owner >= 0) {
            if (ps[sprite[i].yvel].newowner >= 0) {
                parallaxtype = 2;
                sprite[i].x = ps[sprite[i].yvel].oposx;
                sprite[i].y = ps[sprite[i].yvel].oposy;
                sprite[i].z = ps[sprite[i].yvel].oposz + (36 << 8);
                sprite[i].ang = ps[sprite[i].yvel].oang;
            } else {
                execute(i, sprite[i].yvel, 0);
                ps[sprite[i].yvel].fire_flag = 0;
                sprite[i].x = ps[sprite[i].yvel].posx;
                sprite[i].y = ps[sprite[i].yvel].posy;
                sprite[i].z = ps[sprite[i].yvel].posz + (36 << 8);
                sprite[i].ang = ps[sprite[i].yvel].ang;
            }
            setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
        } else if (ps[sprite[i].yvel].holoduke_on == 0) {
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

/* Range-triggered sound effects and level/environment music switching (status 11). */
void movefx(void)
{
    short i;
    long dist;
    char *song;

    for (i = headspritestat[STAT_FX]; i >= 0; i = nextspritestat[i]) {
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
                    song = music_fn[ps[screenpeek].volume][ps[screenpeek].level];
                else
                    song = env_music_fn[sprite[i].lotag];
                hittype[i][0]++;
                if (song != ps[screenpeek].music) {
                    ps[screenpeek].music = song;
                    playmusic(ps[screenpeek].music);
                    MUSIC_SetVolume(ud.music_volume);
                }
                break;
            case 1:
                if ((dist >> 3) < sprite[i].hitag)
                    hittype[i][1] = ud.music_volume;
                break;
            case 3:
                if ((dist >> 3) >= sprite[i].hitag)
                    hittype[i][1] = ud.music_volume;
                break;
            }
        }
    }
}

/* Update standables (status 6): force posts, cranes, cans; script standables run CON. */
void movestandables(void)
{
    short i, nexti, j;

    for (i = headspritestat[STAT_STANDABLE]; i >= 0; i = nexti) {
        long d, *t;
        short sect, p;

        nexti = nextspritestat[i];
        sect = sprite[i].sectnum;
        t = hittype[i];
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
                    for (j = headspritestat[STAT_STANDABLE]; j >= 0; j = nextspritestat[j])
                        if (sprite[i].hitag == sprite[j].hitag && sprite[j].picnum == SEENINE)
                            sprite[j].shade = -32;
                }
            }
        } else if (sprite[i].picnum >= CRANE && sprite[i].picnum < CRANE + 4) {
            sprite[i].xrepeat = sprite[i].yrepeat = 64;
            if (t[0] == 0) {
                if (checkcursectnums((short)t[1]) < 0) {
                    FIND_IN_SECTOR((short)t[1], 627, j);
                    if (j >= 0) {
                        sprite[i].ang = (short)getangle(msx[t[4] + 1] - sprite[i].x, msy[t[4] + 1] - sprite[i].y);
                        setsprite(j, msx[t[4] + 1], msy[t[4] + 1], sprite[j].z);
                        t[0]++;
                        continue;
                    }
                } else {
                    sprite[i].ang = (short)getangle(msx[t[4] + 1] - sprite[i].x, msy[t[4] + 1] - sprite[i].y);
                    t[0]++;
                }
            } else if (t[0] == 1) {
                if (sprite[i].xvel < 184) {
                    sprite[i].picnum = 1176;
                    sprite[i].xvel += 8;
                }
                ssp(i, 0);
                if (sect == t[1])
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
                            if (ps[p].on_crane == i)
                                ps[p].on_crane = -1;
                        }
                        t[0]++;
                        sprite[i].owner = -1;
                    }
                }
            } else if (t[0] == 3) {
                if (++sprite[i].picnum == 1177) {
                    p = checkcursectnums((short)t[1]);
                    if (p >= 0 && ps[p].on_ground) {
                        sprite[i].owner = -2;
                        ps[p].ang = (short)getangle(sprite[i].x - ps[p].posx, sprite[i].y - ps[p].posy);
                        ps[p].on_crane = i;
                    } else {
                        FIND_IN_SECTOR((short)t[1], 627, j);
                        if (j >= 0)
                            sprite[i].owner = j;
                    }
                    t[2] = 0;
                    t[0]++;
                    continue;
                }
            } else if (t[0] == 4) {
                if (++t[2] > 10)
                    t[0]++;
            } else if (t[0] == 5 || t[0] == 8) {
                if (t[0] == 8 && sprite[i].picnum < 1177 && sector[sect].floorz - sprite[i].z > 0x2000)
                    sprite[i].picnum++;
                if (sprite[i].z >= msx[t[4] + 2])
                    sprite[i].z -= 1536;
                else {
                    t[0]++;
                    sprite[i].xvel = 0;
                }
            } else if (t[0] == 6) {
                if (sprite[i].xvel < 192)
                    sprite[i].xvel += 8;
                sprite[i].ang = (short)getangle(msx[t[4]] - sprite[i].x, msy[t[4]] - sprite[i].y);
                MOVE_ALONG(i, sprite[i].zvel, 0);
                if ((sprite[i].y - msy[t[4]]) * (sprite[i].y - msy[t[4]]) + (sprite[i].x - msx[t[4]]) * (sprite[i].x - msx[t[4]]) < 0x4000)
                    t[0]++;
            } else if (t[0] == 9) {
                t[0] = 0;
            }
            setsprite((short)msy[t[4] + 2], sprite[i].x, sprite[i].y, sprite[i].z - 8704);
            if (sprite[i].owner == -1)
                continue;
            p = findplayer(i, &d);
            j = ifhitbyweapon(i);
            if (j >= 0) {
                if (sprite[i].owner == -2 && ps[p].on_crane == i)
                    ps[p].on_crane = -1;
                sprite[i].owner = -1;
                sprite[i].picnum = CRANE;
            }
            if (sprite[i].owner == -2) {
                if (ps[p].on_crane == i && p == screenpeek)
                    sprite[i].xrepeat = sprite[i].yrepeat = 0;
                ps[p].posx = sprite[i].x - (sintable[(ps[p].ang + 512) & 2047] >> 6);
                ps[p].posy = sprite[i].y - (sintable[ps[p].ang & 2047] >> 6);
                ps[p].posz = sprite[i].z - 7168;
                clipmove(&ps[p].posx, &ps[p].posy, &ps[p].posz, &ps[p].cursectnum, 0, 0, 192, 1024, 1024, 0);
                setsprite(ps[p].i, ps[p].posx, ps[p].posy, ps[p].posz);
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
