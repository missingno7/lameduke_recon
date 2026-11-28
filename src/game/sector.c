/* SECTOR: the animation list (Ken's 1994 engine doanimations/getanimationgoal/setanimation, moved into the game), doors and lifts by
   sector lotag, activators, master switches, force fields, the per-player interface keys, switches, shot walls/ceilings and the use key.
   Function order and names match Duke3D 1.5 SECTOR.C (1996) almost one for one; the sliding star door (lotag 9) is Ken's Sep-94
   operatesector code. Sector lotag bit 0x8000 = open/moving state, 0x4000 = locked. */

#include <stdlib.h>
#include "sector.h"

extern void qsetmode640350(void);

extern short animatesect[MAXANIMATES];
extern long *animateptr[MAXANIMATES];
extern long animategoal[MAXANIMATES], animatevel[MAXANIMATES], animatecnt;
extern animwalltype animwall[MAXANIMWALLS];
extern short numanimwalls;
/* Interim host. The only _BSS of the object that starts after GAME's (2:0ac60); its users are spawn,
   resetplayer and prelevel. See OWNERSHIP.md for the second GAME-range object. */
/* tempwallptr: PREMAP _BSS (gamesplit) */
extern short neartagsprite, neartagsector, neartagwall;
extern long neartaghitdist;

extern struct user_defs ud;         /* the 21-byte USERDEFS.TMP block defined by GLOBAL */

/* dataown: one object, one name. Code uses ud's fields and current_menu directly; the unused externs
   gamepaused, disable_armor and menupage in sector.h only keep this unit's symbol slots (docs/BSS_LAYOUT.md). */
extern long current_menu;

/* First player standing in sector sect, or -1. */
short checkcursectnums(short sect)
{
    short i;

    for (i = connecthead; i >= 0; i = connectpoint2[i])
        if (ps[i].cursectnum == sect)
            return i;
    return -1;
}

/* Nearest player to sprite i (Manhattan x+y); distance in *d. */
short findplayer(short sn, long *d)
{
    short j, closest_player;
    long distance, closest;

    closest = 0x7fffffff;
    for (j = connecthead; j >= 0; j = connectpoint2[j]) {
        distance = klabs(ps[j].posx - sprite[sn].x) +
            klabs(ps[j].posy - sprite[sn].y);
        if (distance < closest) {
            closest_player = j;
            closest = distance;
        }
    }
    *d = closest;
    return closest_player;
}

/* Advance the animation list by numtics (Ken's 1994 engine routine plus riders and lift sounds). */
void doanimations(long numtics)
{
    long i, p, animval;

    for (i = animatecnt - 1; i >= 0; i--) {
        animval = *animateptr[i];
        if (animval < animategoal[i]) {
            animval += animatevel[i] * numtics;
            for (p = connecthead; p >= 0; p = connectpoint2[p])
                if (ps[p].cursectnum == animatesect[i] && ps[p].on_ground)
                    ps[p].posz += animatevel[i] * numtics;
            if (animval > animategoal[i]) {
                stopsound(SOUND_LIFT_MOVING);
                xyzsound(SOUND_LIFT_STOP, -1,
                    wall[sector[animatesect[i]].wallptr].x,
                    wall[sector[animatesect[i]].wallptr].y,
                    sector[animatesect[i]].floorz);
                animval = animategoal[i];
            }
        }
        if (animval > animategoal[i]) {
            animval -= animatevel[i] * numtics;
            for (p = connecthead; p >= 0; p = connectpoint2[p])
                if (ps[p].cursectnum == animatesect[i] && ps[p].on_ground)
                    ps[p].posz -= animatevel[i] * numtics;
            if (animval < animategoal[i]) {
                stopsound(SOUND_LIFT_MOVING);
                xyzsound(SOUND_LIFT_STOP, -1,
                    wall[sector[animatesect[i]].wallptr].x,
                    wall[sector[animatesect[i]].wallptr].y,
                    sector[animatesect[i]].floorz);
                animval = animategoal[i];
            }
        }
        *animateptr[i] = animval;
        if (animval == animategoal[i]) {
            animatecnt--;
            animateptr[i] = animateptr[animatecnt];
            animategoal[i] = animategoal[animatecnt];
            animatevel[i] = animatevel[animatecnt];
            animatesect[i] = animatesect[animatecnt];
        }
    }
}

/* Slot index animating animptr, or -1 if it is not moving. */
short getanimationgoal(long *animptr)
{
    long i, j = -1;

    for (i = 0; i < animatecnt; i++)
        if (animptr == animateptr[i]) {
            j = i;
            break;
        }
    return j;
}

/* Start (or retarget) moving *animptr toward thegoal at thevel per tic for sector animsect. */
int setanimation(short animsect, long *animptr, long thegoal, long thevel)
{
    long i, j;

    if (animatecnt >= MAXANIMATES - 1)
        return -1;

    j = animatecnt;
    for (i = 0; i < animatecnt; i++)
        if (animptr == animateptr[i]) {
            j = i;
            break;
        }
    animatesect[j] = animsect;
    animateptr[j] = animptr;
    animategoal[j] = thegoal;
    animatevel[j] = thevel;
    if (j == animatecnt)
        animatecnt++;
    return animatecnt - 1;
}

/* Refresh the viewscreen tile (CAM1) from its camera every 12 frames when the player is near. */
void animatecamsprite(short snum)
{
    short i, camera;

    if (camsprite != -1) {
        i = camsprite;
        if (hittype[i][0] >= 11) {
            hittype[i][0] = 0;
            if (ps[snum].newowner >= 0)
                sprite[i].owner = ps[snum].newowner;
            if (sprite[i].owner >= 0) {
                camera = sprite[i].owner;
                if (klabs(sprite[ps[snum].i].x - sprite[i].x) +
                    klabs(sprite[ps[snum].i].y - sprite[i].y) < 2048)
                    render_camera_tile(sprite[camera].x, sprite[camera].y,
                        sprite[camera].z, sprite[camera].ang, sprite[camera].sectnum, 1408);
            }
        } else
            hittype[i][0]++;
    }
}

/* Animate the walls collected by prelevel and the gotpic-driven lava and mirror tiles. */
void animatewalls(void)
{
    short p, i, t, mirror;

    mirror = 0;
    if ((gotpic[570 >> 3] & 4) > 0)
        mirror = 1;
    if ((gotpic[49 >> 3] & 2) > 0 && waloff[49] != -1)
        movelava(waloff[49]);

    for (p = 0; p < numanimwalls; p++) {
        i = animwall[p].wallnum;
        switch (wall[i].picnum) {
        case 819: case 820: case 821: case 822:
            if (wall[i].picnum == 822)
                wall[i].picnum = 819;
            else
                wall[i].picnum++;
            break;
        case 823: case 824: case 825: case 826:
            if (wall[i].picnum == 826)
                wall[i].picnum = 823;
            else
                wall[i].picnum++;
            break;
        case 860: case 861: case 862: case 863:
            if (wall[i].picnum == 859)
                wall[i].picnum = 860;
            else
                wall[i].picnum++;
            break;
        case 864: case 865: case 866: case 867:
            if (wall[i].picnum == 867)
                wall[i].picnum = 864;
            else
                wall[i].picnum++;
            break;
        case 570:
            if (mirror) {
                render_mirror(i);
                mirror = 0;
            }
            break;
        case 764: case 765: case 766: case 767:
            if ((rand() & 255) < 16) {
                animwall[p].tag = wall[i].picnum;
                wall[i].picnum = 761;
            }
            break;
        case 761: case 762: case 763:
            wall[i].picnum = (short)animwall[p].tag;
            break;
        default:
            switch (wall[i].overpicnum) {
            case FORCEFIELD: case FORCEFIELD + 1: case FORCEFIELD + 2:
                t = (short)animwall[p].tag;
                if (wall[i].cstat & 254) {
                    wall[i].xpanning += sintable[(t + 512) & 2047] >> 12;
                    wall[i].ypanning += sintable[t & 2047] >> 12;
                    animwall[p].tag += 128;
                    if (++wall[i].overpicnum > FORCEFIELD + 2)
                        wall[i].overpicnum = FORCEFIELD;
                    if (rand() & 4)
                        wall[i].cstat ^= 128;
                }
                break;
            }
            break;
        }
    }
    gotpic[49 >> 3] &= ~2;
    gotpic[570 >> 3] &= 0xeb;
}

/* Set every warp elevator (SE 17) of s's group moving in direction d, or stop them. */
void activatewarpelevators(short s, short d)
{
    short i;

    i = headspritestat[STAT_EFFECTOR];
    while (i >= 0) {
        if (sprite[i].lotag == 17 && sprite[i].hitag == sprite[s].hitag)
            if (klabs(sector[sprite[s].sectnum].floorz - hittype[s][2]) > sprite[i].yvel ||
                sector[sprite[i].sectnum].hitag == sector[sprite[s].sectnum].hitag - d)
                break;
        i = nextspritestat[i];
    }

    if (i == -1)
        d = 0;
    else if (d == 0)
        spritesound(SOUND_LIFT_STOP, s);
    else
        spritesound(SOUND_LIFT_START, s);

    i = headspritestat[STAT_EFFECTOR];
    while (i >= 0) {
        if (sprite[i].lotag == 17 && sprite[i].hitag == sprite[s].hitag) {
            hittype[i][0] = d;
            hittype[i][1] = d;
        }
        i = nextspritestat[i];
    }
}

/* Operate sector sn according to its lotag (doors, lifts, elevators); ii is the initiator. */
void operatesectors(short sn, short ii)
{
{
    /* COMPILER_STEERED: declaration order chosen by search (r3sector perm.py); it only moves register ties. */
    short a;
    long j, middle, daz;
    short i, p;

    switch ((short)(sector[sn].lotag & 32767)) {
    case 26:
        if (getanimationgoal(&sector[sn].floorz) == -1) {
            sector[sn].lotag &= 0xff00;
            sector[sn].lotag |= 22;
            operatesectors(sn, ii);
            sector[sn].lotag &= 0xff00;
            sector[sn].lotag |= 9;
            operatesectors(sn, ii);
            sector[sn].lotag &= 0xff00;
            sector[sn].lotag |= 26;
        }
        break;
    case 9: {
        /* The sliding-door geometry follows Ken's September 1994 routine. */
        /* COMPILER_STEERED: initialize both center coordinates here for the inlined door scans. */
        long cx = 0, cy = 0, last, first, sp, dx, dy, displacement;
        short w, prev;
        long door[2], door_index;

        first = sector[sn].wallptr;
        last = first + sector[sn].wallnum - 1;
        sp = sector[sn].extra >> 4;
        for (w = first; w <= last; w++) {
            cx += wall[w].x;
            cy += wall[w].y;
        }
        cx /= last - first + 1;
        cy /= last - first + 1;
        door[0] = -1;
        door[1] = -1;
        for (w = first; w <= last; w++)
            if (wall[w].x == cx || wall[w].y == cy) {
                if (door[0] == -1)
                    door[0] = w;
                else
                    door[1] = w;
            }

        for (door_index = 0; door_index < 2; door_index++) {
            if (wall[door[door_index]].x == cx && wall[door[door_index]].y == cy) {
                prev = door[door_index] - 1;
                if (prev < first)
                    prev = last;
                dx = ((wall[prev].x + wall[wall[door[door_index]].point2].x) >> 1) - wall[door[door_index]].x;
                dy = ((wall[prev].y + wall[wall[door[door_index]].point2].y) >> 1) - wall[door[door_index]].y;
                if (dx != 0) {
                    displacement = wall[wall[wall[door[door_index]].point2].point2].x;
                    displacement -= wall[wall[door[door_index]].point2].x;
                    setanimation(sn, &wall[door[door_index]].x, wall[door[door_index]].x + displacement, sp);
                    setanimation(sn, &wall[prev].x, wall[prev].x + displacement, sp);
                    setanimation(sn, &wall[wall[door[door_index]].point2].x, wall[wall[door[door_index]].point2].x + displacement, sp);
                    spritesound(6, ii);
                } else if (dy != 0) {
                    displacement = wall[wall[wall[door[door_index]].point2].point2].y;
                    displacement -= wall[wall[door[door_index]].point2].y;
                    setanimation(sn, &wall[door[door_index]].y, wall[door[door_index]].y + displacement, sp);
                    setanimation(sn, &wall[prev].y, wall[prev].y + displacement, sp);
                    setanimation(sn, &wall[wall[door[door_index]].point2].y, wall[wall[door[door_index]].point2].y + displacement, sp);
                    spritesound(6, ii);
                }
            } else {
                prev = door[door_index] - 1;
                if (prev < first)
                    prev = last;
                dx = ((wall[prev].x + wall[wall[door[door_index]].point2].x) >> 1) - wall[door[door_index]].x;
                dy = ((wall[prev].y + wall[wall[door[door_index]].point2].y) >> 1) - wall[door[door_index]].y;
                if (dx != 0) {
                    setanimation(sn, &wall[door[door_index]].x, cx, sp);
                    setanimation(sn, &wall[prev].x, cx + dx, sp);
                    setanimation(sn, &wall[wall[door[door_index]].point2].x, cx + dx, sp);
                    spritesound(6, ii);
                } else if (dy != 0) {
                    setanimation(sn, &wall[door[door_index]].y, cy, sp);
                    setanimation(sn, &wall[prev].y, cy + dy, sp);
                    setanimation(sn, &wall[wall[door[door_index]].point2].y, cy + dy, sp);
                    spritesound(6, ii);
                }
            }
        }
        break;
    }
    case 15:
        a = headspritesect[sn];
        while (a >= 0) {
            if (sprite[a].picnum == SECTOREFFECTOR && sprite[a].lotag == 17)
                break;
            a = nextspritesect[a];
        }
        p = checkcursectnums(sn);
        if (p >= 0) {
            if (syncbits[p] & SB_JUMP)
                activatewarpelevators(a, -1);
            if (syncbits[p] & SB_CROUCH)
                activatewarpelevators(a, 1);
        } else {
            if (sector[sn].floorz > sprite[a].z)
                activatewarpelevators(a, -1);
            else
                activatewarpelevators(a, 1);
        }
        break;
    case 16: {
        long *floor;

lift_down:
        a = nextsectorneighborz(sn, sector[sn].floorz, 1, 1);
        if (a == -1) {
            sector[sn].lotag ^= 1;
            goto lift_up;
        }
        floor = &sector[sn].floorz;
        setanimation(sn, floor, sector[a].floorz, sector[sn].extra);
        spritesound(SOUND_LIFT_START, ii);
        break;
    }
    case 17:
lift_up:
        a = nextsectorneighborz(sn, sector[sn].floorz, 1, -1);
        if (a == -1) {
            sector[sn].lotag ^= 1;
            goto lift_down;
        }
        setanimation(sn, &sector[sn].floorz, sector[a].floorz, sector[sn].extra);
        spritesound(SOUND_LIFT_START, ii);
        break;
    case 18:
elevator_down:
        a = nextsectorneighborz(sn, sector[sn].floorz, 1, 1);
        if (a == -1) {
            sector[sn].lotag ^= 1;
            goto elevator_up;
        }
        daz = sector[a].floorz;
        setanimation(sn, &sector[sn].floorz, daz, 256);
        j = sector[sn].ceilingz - sector[sn].floorz;
        p = sector[sn].extra;
        setanimation(sn, &sector[sn].ceilingz, daz + j, p);
        spritesound(SOUND_LIFT_START, ii);
        break;
    case 19:
elevator_up:
        a = nextsectorneighborz(sn, sector[sn].floorz, 1, -1);
        if (a == -1) {
            sector[sn].lotag ^= 1;
            goto elevator_down;
        }
        daz = sector[a].floorz;
        middle = sector[sn].extra;
        setanimation(sn, &sector[sn].floorz, daz, middle);
        setanimation(sn, &sector[sn].ceilingz, daz + (sector[sn].ceilingz - sector[sn].floorz), middle);
        spritesound(SOUND_LIFT_START, ii);
        break;
    case 29:
        if (sector[sn].lotag & 0x8000)
            daz = sector[nextsectorneighborz(sn, sector[sn].ceilingz, 1, 1)].floorz;
        else
            daz = sector[nextsectorneighborz(sn, sector[sn].ceilingz, -1, -1)].ceilingz;
        a = sector[sn].extra;
        p = headspritestat[STAT_EFFECTOR];
        while (p >= 0) {
            if (sprite[p].lotag == 22 && sprite[p].hitag == sector[sn].hitag) {
                if (sector[sn].lotag & 0x8000)
                    sprite[p].yvel = a;
                else
                    sprite[p].yvel = -a;
                hittype[p][0] = sn;
                hittype[p][1] = 1;
            }
            p = nextspritestat[p];
        }
        sector[sn].lotag ^= 0x8000;
        setanimation(sn, &sector[sn].ceilingz, daz, a);
        spritesound(6, ii);
        break;
    case 20:
        if (sector[sn].lotag & 0x8000) {
            i = headspritestat[STAT_EFFECTOR];
            while (i >= 0) {
                if (sprite[i].sectnum == sn && sprite[i].lotag == 9) {
                    daz = sprite[i].z;
                    break;
                }
                i = nextspritestat[i];
            }
            if (i == -1)
                daz = sector[sn].floorz;
        } else
            daz = sector[nextsectorneighborz(sn, sector[sn].ceilingz, -1, -1)].ceilingz;
        sector[sn].lotag ^= 0x8000;
        setanimation(sn, &sector[sn].ceilingz, daz, sector[sn].extra);
        spritesound(6, ii);
        break;
    case 21:
        a = getanimationgoal(&sector[sn].floorz);
        if (a >= 0) {
            if (animategoal[sn] == sector[sn].ceilingz)
                animategoal[a] = sector[nextsectorneighborz(sn, sector[sn].ceilingz, 1, 1)].floorz;
            else
                animategoal[a] = sector[sn].ceilingz;
            daz = animategoal[a];
        } else {
            if (sector[sn].ceilingz == sector[sn].floorz)
                daz = sector[nextsectorneighborz(sn, sector[sn].ceilingz, 1, 1)].floorz;
            else
                daz = sector[sn].ceilingz;
            sector[sn].lotag ^= 0x8000;
            if (setanimation(sn, &sector[sn].floorz, daz, sector[sn].extra) >= 0)
                spritesound(6, ii);
        }
        break;
    case 22:
        middle = (sector[sn].ceilingz + sector[sn].floorz) >> 1;
        a = getanimationgoal(&sector[sn].ceilingz);
        if (a >= 0) {
            if (middle == animategoal[sn])
                animategoal[a] = sector[nextsectorneighborz(sn, sector[sn].floorz, -1, -1)].ceilingz;
            else
                animategoal[a] = middle;
        } else {
            if (middle == sector[sn].ceilingz)
                daz = sector[nextsectorneighborz(sn, sector[sn].floorz, -1, -1)].ceilingz;
            else
                daz = middle;
            {
                long *plane = &sector[sn].ceilingz;

                if (setanimation(sn, plane, daz, sector[sn].extra) >= 0) {
                    sector[sn].lotag ^= 0x8000;
                    spritesound(6, ii);
                }
            }
        }
        a = getanimationgoal(&sector[sn].floorz);
        if (a >= 0) {
            if (middle == animategoal[sn])
                animategoal[a] = sector[nextsectorneighborz(sn, sector[sn].ceilingz, 1, 1)].floorz;
            else
                animategoal[a] = middle;
            daz = animategoal[a];
        } else {
            if (middle == sector[sn].floorz)
                daz = sector[nextsectorneighborz(sn, sector[sn].ceilingz, 1, 1)].floorz;
            else
                daz = middle;
            if (setanimation(sn, &sector[sn].floorz, daz, sector[sn].extra) >= 0)
                spritesound(6, ii);
        }
        break;
    case 23:
        j = -1;
        i = headspritestat[STAT_EFFECTOR];
        while (i >= 0) {
            if (sprite[i].lotag == 11 && sprite[i].sectnum == sn && hittype[i][4] == 0) {
                j = i;
                sector[sn].lotag ^= 0x8000;
                hittype[i][4] = 1;
                hittype[i][3] = -hittype[i][3];
                break;
            }
            i = nextspritestat[i];
        }
        if (i >= 0)
            spritesound(6, ii);
        if (j < 0)
            break;
        i = headspritestat[STAT_EFFECTOR];
        while (i >= 0) {
            if (i != j && sprite[i].lotag == 11 && sprite[i].hitag == sprite[j].hitag && hittype[i][4] == 0) {
                long *data = hittype[i];

                sector[sprite[i].sectnum].lotag ^= 0x8000;
                data[4] = 1;
                data[3] = -data[3];
            }
            i = nextspritestat[i];
        }
        break;
    case 25:
        j = headspritestat[STAT_EFFECTOR];
        while (j >= 0) {
            if (sprite[j].lotag == 15 && sprite[j].sectnum == sn) {
                spritesound(6, ii);
                break;
            }
            j = nextspritestat[j];
        }
        i = headspritestat[STAT_EFFECTOR];
        while (i >= 0) {
            if (sprite[i].hitag == sprite[j].hitag) {
                if (sprite[i].lotag == 18)
                    hittype[i][0] ^= 1;
                if (sprite[i].lotag == 15) {
                    sector[sprite[i].sectnum].lotag ^= 0x8000;
                    sprite[i].ang += 1024;
                    if (sector[sprite[i].sectnum].lotag & 0x8000)
                        hittype[i][4] = 1;
                    else
                        hittype[i][4] = 2;
                }
            }
            i = nextspritestat[i];
        }
        break;
    case 27:
        j = headspritestat[STAT_EFFECTOR];
        while (j >= 0) {
            if ((sprite[j].lotag & 255) == 20 && sprite[j].sectnum == sn) {
                sector[sprite[j].sectnum].lotag ^= 0x8000;
                hittype[j][0] = (sector[sprite[j].sectnum].lotag & 0x8000) ? 1 : 2;
                spritesound(6, ii);
                break;
            }
            j = nextspritestat[j];
        }
        break;
    case 28:
        j = headspritesect[sn];
        while (j >= 0) {
            if (sprite[j].statnum == STAT_EFFECTOR && (sprite[j].lotag & 32767) == 21)
                break;
            j = nextspritesect[j];
        }
        daz = sprite[j].hitag;
        j = headspritestat[STAT_EFFECTOR];
        while (j >= 0) {
            if ((sprite[j].lotag & 32767) == 21 && hittype[j][0] == 0 && sprite[j].hitag == daz)
                hittype[j][0] = 1;
            j = nextspritestat[j];
        }
        break;
    }
}
}

/* Fire every activator on channel low: unlock/lock or operate its sector. */
void operateactivators(short low)
{
    short i;

    i = headspritestat[STAT_ACTIVATOR];
    while (i >= 0) {
        if (sprite[i].lotag == low) {
        if (sprite[i].picnum == 23) {
            sector[sprite[i].sectnum].lotag ^= 0x4000;
        } else {
            operatesectors(sprite[i].sectnum, i);
        }
        }
        i = nextspritestat[i];
    }
}

/* Arm the master switches on channel low. */
void operatemasterswitches(short low)
{
    short i;

    /* COMPILER_STEERED: use a signed byte offset in the inlined list walks. */
    for (i = headspritestat[STAT_ACTOR]; i >= 0;
         i = *(short *)((char *)&nextspritestat + ((long)i + i)))
        if (sprite[i].picnum == MASTERSWITCH && sprite[i].lotag == low && sprite[i].yvel == 0)
            sprite[i].yvel = 1;
}

/* Switch the force fields on channel low (or all) on/off. */
void operateforcefields(short low)
{
    short p, i;

    for (p = 0; p < numanimwalls; p++) {
        i = animwall[p].wallnum;
        if (low == wall[i].lotag || low == -1) {
            switch (wall[i].overpicnum) {
            case FORCEFIELD: case FORCEFIELD + 1: case FORCEFIELD + 2:
                animwall[p].tag = 0;
                if (wall[i].cstat) {
                    wall[i].cstat = 0;
                    sector[wall[i].nextsector1].floorpal = 0;
                    sector[wall[i].nextsector1].ceilingpal = 0;
                } else {
                    wall[i].cstat = 85;
                    sector[wall[i].nextsector1].floorpal = 2;
                    sector[wall[i].nextsector1].ceilingpal = 2;
                }
                break;
            }
        }
    }
}

/* Handle player snum's interface and inventory keys (Duke3D 1.5 cheatkeys), including the /d door cheat. */
void cheatkeys(short snum)
{
    short i;

    if ((syncbits[snum] & SB_KNUCKLES) && ps[snum].knuckle_incs == 0)
        ps[snum].knuckle_incs = 1;
    if (ps[snum].dimensionmode == 2) {
        if ((syncbits[snum] & SB_ENLARGE_VIEW) && ps[snum].zoom < 4096)
            ps[snum].zoom += ps[snum].zoom >> 3;
        if ((syncbits[snum] & SB_SHRINK_VIEW) && ps[snum].zoom > 48)
            ps[snum].zoom -= ps[snum].zoom >> 3;
    } else {
        if (syncbits[snum] & SB_ENLARGE_VIEW) {
            if (ps[snum].screen_size > -4)
                ps[snum].screen_size -= 4;
            if (option[0] == 7)
                setview(0, 0, 639, 479);
            else
                vscrn(snum);
        }
        if (syncbits[snum] & SB_SHRINK_VIEW) {
            if (ps[snum].screen_size < 64)
                ps[snum].screen_size += 4;
            if (option[0] == 7)
                setview(0, 0, 639, 479);
            else
                vscrn(snum);
        }
    }
    if (syncbits[snum] < 255) {
        ps[snum].interface_toggle_flag = 0;
        return;
    }
    if (ps[snum].interface_toggle_flag)
        return;
    ps[snum].interface_toggle_flag = 1;
    if (syncbits[snum] & SB_PAUSE)
        ud.gamepaused = !ud.gamepaused;
    if (ud.gamepaused)
        return;
    if ((syncbits[snum] & SB_ESCAPE) && !(ps[snum].gm & MODE_DEMO) && ps[snum].newowner == -1) {
        unsigned char escape_flags;

        keystatus[1] = 0;
        /* COMPILER_STEERED: read gm before changing current_menu. */
        escape_flags = ps[snum].gm;
        current_menu = 0;
        ps[snum].gm = escape_flags | MODE_MENU;
        if (ps[snum].dimensionmode == 2) {
            ps[snum].dimensionmode = 3;
            if (snum == screenpeek)
                setgamemode();
        }
    }
    if (syncbits[snum] & SB_MAP) {
        if (ps[snum].dimensionmode == 3) {
            ps[snum].dimensionmode = 2;
            if (screenpeek == snum)
                qsetmode640350();
        } else {
            ps[snum].dimensionmode = 3;
            setgamemode();
            vscrn(snum);
            displayrooms(snum);
        }
    }
    if (syncbits[snum] & SB_INVENTORY)
        ps[snum].show_inventory ^= 1;
    if ((ps[snum].weapon_pos & 127) == 6) {
        if (syncbits[snum] & SB_WEAPON_1)
            addweapon(snum, 1230, 1);
        if ((syncbits[snum] & SB_WEAPON_2) && (ps[snum].gotweapon[1] || ud.debug))
            addweapon(snum, 191, 1);
        if ((syncbits[snum] & SB_WEAPON_3) && (ps[snum].gotweapon[2] || ud.debug))
            addweapon(snum, 389, 1);
        if ((syncbits[snum] & SB_WEAPON_4) && (ps[snum].gotweapon[3] || ud.debug))
            addweapon(snum, 721, 1);
        if ((syncbits[snum] & SB_WEAPON_5) && (ps[snum].gotweapon[4] || ud.debug))
            addweapon(snum, 563, 1);
    }
    if ((syncbits[snum] & SB_STEROIDS) && (ps[snum].inventory[2] || ud.debug)) {
        if (ud.debug)
            ps[snum].inventory[2] = 1;
        ps[snum].steroids_on ^= 1;
        if (ps[snum].steroids_on)
            ps[snum].steroids_on = 128;
    }
    if ((syncbits[snum] & SB_SHIELD) && (ps[snum].inventory[3] || ud.debug)) {
        if (ud.debug)
            ps[snum].inventory[3] = 1;
        ps[snum].shield_amount = !ps[snum].shield_amount;
        if (ps[snum].shield_amount)
            ps[snum].shield_amount = 32;
    }
    if ((syncbits[snum] & SB_SCUBA) && (ps[snum].inventory[0] || ud.debug)) {
        if (ud.debug)
            ps[snum].inventory[0] = 1;
        ps[snum].scuba_on ^= 1;
    }
    if ((syncbits[snum] & SB_HOLODUKE) && (ps[snum].inventory[4] || ud.debug)) {
        if (ud.debug)
            ps[snum].inventory[4] = 1;
        if (ps[snum].holoduke_on == 0) {
            i = EGS(ps[snum].cursectnum, ps[snum].posx, ps[snum].posy, ps[snum].posz,
                0, 283, -16, 48, 48, ps[snum].ang, 0, 0, -1, 10);
            ps[snum].holoduke_on = i;
            hittype[i][0] = snum;
        } else
            ps[snum].holoduke_on = 0;
    }
    if ((syncbits[snum] & SB_JETPACK) && (ps[snum].inventory[1] || ud.debug)) {
        if (ud.debug)
            ps[snum].inventory[1] = 1;
        ps[snum].jetpack_on = !ps[snum].jetpack_on;
        if (ps[snum].jetpack_on)
            spritesound(18, ps[snum].i);
        else
            spritesound(27, ps[snum].i);
    }
    if ((syncbits[snum] & SB_HOLSTER) && ps[snum].somethingonplayer == -1)
    {
        if (ps[snum].weapon_pos & 128)
            ps[snum].weapon_pos = 5;
        else
            ps[snum].weapon_pos = 129;
    }
    if (!ud.debug) {
        if (syncbits[snum] & SB_STEROIDS) {
            unsigned char gm = ps[snum].gm;
            current_menu = 20;
            ps[snum].gm = gm | MODE_MENU;
        }
        if (syncbits[snum] & SB_SHIELD) {
            unsigned char gm = ps[snum].gm;
            current_menu = 50;
            ps[snum].gm = gm | MODE_MENU;
        }
        if (syncbits[snum] & SB_SCUBA) {
            unsigned char scuba_flags = ps[snum].gm;
            current_menu = 10;
            ps[snum].gm = scuba_flags | MODE_MENU;
        }
    } else if (syncbits[snum] & SB_DEBUG_DOORS) {
        FTA(11);
        for (i = 0; i < numsectors; i++) {
            switch ((short)(sector[i].lotag & 0xfff)) {
            case 15: case 16: case 17: case 18: case 19:
            case 20: case 21: case 22: case 23:
            case 25: case 26: case 27: case 28: case 29:
                if (sector[i].lotag & 0xbfff)
                    sector[i].lotag &= ~0x4000;
                operatesectors(i, ps[snum].i);
                break;
            }
        }
        operateforcefields(-1);
    }
}

/* Use or shoot wall switch w: flip all switches on its channel and fire the channel. */
void checkhitswitch(long w)
{
{
    /* COMPILER_STEERED: keep the wall displacement live across the toggle switch. */
    short i;
    long displacement;
    short lotag;
    short j;

    switch (wall[w].picnum) {
    case 395: case 396: case 405: case 406:
        break;
    default:
        return;
    }
    lotag = wall[w].lotag;
    for (i = 0; i < numanimwalls; i++) {
        j = animwall[i].wallnum;
        /* Build v5 stores each wall in 35 bytes. */
        displacement = (long)j * 8;
        displacement += j;
        displacement *= 4;
        displacement -= j;
        if (lotag == ((walltype *)((char *)&wall + displacement))->lotag) {
            switch (((walltype *)((char *)&wall + displacement))->picnum) {
            case 395: case 405:
                ((walltype *)((char *)&wall + displacement))->picnum++;
                break;
            case 396: case 406:
                ((walltype *)((char *)&wall + displacement))->picnum--;
                break;
            }
        }
    }

    switch (wall[w].picnum) {
    case 395:
        FTA(4);
        xyzsound(25, -1, wall[w].x, wall[w].y, 0);
        operateactivators(lotag);
        operatemasterswitches(lotag);
        break;
    case 396:
        FTA(4);
        xyzsound(25, -1, wall[w].x, wall[w].y, 0);
        operateactivators(lotag);
        operatemasterswitches(lotag);
        break;
    case 405:
        i = headspritestat[STAT_EFFECTOR];
        while (i >= 0) {
            if (sprite[i].lotag == 12 && sprite[i].hitag == wall[w].lotag)
                hittype[i][0] = 2;
            i = nextspritestat[i];
        }
        xyzsound(25, -1, wall[w].x, wall[w].y, 0);
        operateactivators(lotag);
        operateforcefields(lotag);
        break;
    case 406:
        i = headspritestat[STAT_EFFECTOR];
        while (i >= 0) {
            if (sprite[i].lotag == 12 && sprite[i].hitag == wall[w].lotag)
                hittype[i][0] = 1;
            i = nextspritestat[i];
        }
        xyzsound(25, -1, wall[w].x, wall[w].y, 0);
        operateactivators(lotag);
        operateforcefields(lotag);
        break;
    }
}
}

/* Effects of a shot on wall dawallnum: glass, tech walls and breakable lights. */
void checkhitwall(long dawallnum, long x, long y, long z, short atwith)
{
    /* COMPILER_STEERED: declaration order selects EAX for the second EGS result. */
    short k, nextj, endwall;
    signed char ceiling_shade;
    short i, j;
    signed char floor_shade;
    short sn, startwall, darkestwall;

    switch (wall[dawallnum].overpicnum) {
    case 179:
    case 331:
    case 573:
        wall[dawallnum].overpicnum = 573;
        updatesector(x, y, &sn);
        i = EGS(sn, x, y, z, 0, SECTOREFFECTOR, 0, 0, 0, ps[0].ang, 0, 0, ps[screenpeek].i, 3);
        sprite[i].lotag = 128;
        hittype[i][1] = 5;
        hittype[i][2] = dawallnum;
        spritesound(5, i);
        return;
    case 790:
        if (atwith == 563) {
            wall[dawallnum].overpicnum = 573;
            wall[wall[dawallnum].nextwall1].overpicnum = 573;
        } else {
            updatesector(x, y, &sn);
            i = EGS(sn, x, y, z, 0, SECTOREFFECTOR, 0, 0, 0, 0, 0, 0, ps[screenpeek].i, 3);
            sprite[i].lotag = 129;
            hittype[i][2] = dawallnum;
        }
        return;
    }
    switch (wall[dawallnum].picnum) {
    case 819: case 820: case 821: case 822:
        wall[dawallnum].picnum = 1183;
        xyzsound(23, -1, wall[dawallnum].x, wall[dawallnum].y, sector[wall[dawallnum].nextsector1].floorz);
        break;
    case 823: case 824: case 825: case 826:
        wall[dawallnum].picnum = 1182;
        xyzsound(23, -1, wall[dawallnum].x, wall[dawallnum].y, sector[wall[dawallnum].nextsector1].floorz);
        break;
    case 860: case 861: case 862: case 863:
        wall[dawallnum].picnum = 1180;
        xyzsound(23, -1, wall[dawallnum].x, wall[dawallnum].y, sector[wall[dawallnum].nextsector1].floorz);
        break;
    case 864: case 865: case 866: case 867:
        wall[dawallnum].picnum = 1181;
        xyzsound(23, -1, wall[dawallnum].x, wall[dawallnum].y, sector[wall[dawallnum].nextsector1].floorz);
        break;
    case 35: case 36: case 26: case 212:
        if (wall[dawallnum].lotag == 0)
            break;
        if (wall[dawallnum].picnum == 35) wall[dawallnum].picnum = 664;
        if (wall[dawallnum].picnum == 36) wall[dawallnum].picnum = 665;
        if (wall[dawallnum].picnum == 26) wall[dawallnum].picnum = 670;
        if (wall[dawallnum].picnum == 212) wall[dawallnum].picnum = 671;

        sn = wall[dawallnum].nextsector1;
        startwall = sector[sn].wallptr;
        endwall = startwall + sector[sn].wallnum;
        darkestwall = 0;
        for (j = startwall; j < endwall; j++)
            if (wall[j].shade > darkestwall)
                darkestwall = wall[j].shade;
        floor_shade = sector[sn].floorshade;
        ceiling_shade = sector[sn].ceilingshade;
        i = headspritestat[STAT_EFFECTOR];
        while (i >= 0) {
            /* COMPILER_STEERED: a separate byte offset selects EAX for the list advance. */
            long link_offset;

            if (sprite[i].hitag == wall[dawallnum].lotag && sprite[i].lotag == 3) {
                hittype[i][3] = darkestwall;
                hittype[i][4] = 1;
            }
            link_offset = (long)i + i;
            i = *(short *)((char *)nextspritestat + link_offset);
        }
        for (i = 0; i < numsectors; i++)
            if (i != sn && sector[i].hitag == wall[dawallnum].lotag) {
                sector[i].floorpal = 0;
                sector[i].ceilingpal = 0;
                for (j = headspritesect[i]; j >= 0; j = nextj) {
                    nextj = nextspritesect[j];
                    if (sprite[j].picnum == SECTOREFFECTOR && (sprite[j].lotag == 8 || sprite[j].lotag == 9))
                        deletesprite(j);
                }
                sector[i].floorshade = floor_shade;
                sector[i].ceilingshade = ceiling_shade;
                startwall = sector[i].wallptr;
                endwall = startwall + sector[i].wallnum;
                for (k = startwall; k < endwall; k++) {
                    wall[k].shade = darkestwall;
                    if (wall[k].nextwall1 >= 0)
                        wall[wall[k].nextwall1].shade = darkestwall;
                }
            }
        break;
    }
}

/* Shot ceiling light in sector sn: bust it and black out its hitag group; 1 if handled. */
char checkhitceiling(short sn)
{
    short best, darkestceiling, darkestwall, i, j, startwall, endwall;
    signed char floor_shade, ceiling_shade;

    if (sector[sn].hitag == 0)
        return 0;
    switch (sector[sn].ceilingpicnum) {
    case 35: case 36: case 26: case 212:
        if (sector[sn].ceilingpicnum == 35) sector[sn].ceilingpicnum = 664;
        if (sector[sn].ceilingpicnum == 36) sector[sn].ceilingpicnum = 665;
        if (sector[sn].ceilingpicnum == 26) sector[sn].ceilingpicnum = 670;
        if (sector[sn].ceilingpicnum == 212) sector[sn].ceilingpicnum = 671;

        darkestceiling = sector[sn].ceilingshade;
        for (i = headspritestat[STAT_EFFECTOR]; i >= 0; i = nextspritestat[i])
            if (sprite[i].hitag == sector[sn].hitag && sprite[i].lotag == 3)
                hittype[i][4] = 1;
        /* The original assigns best only after finding a darker linked ceiling. */
        for (i = 0; i < numsectors; i++)
            if (i != sn && sector[i].hitag == sector[sn].hitag && darkestceiling < sector[i].ceilingshade) {
                darkestceiling = sector[i].ceilingshade;
                best = i;
            }

        floor_shade = sector[best].floorshade;
        ceiling_shade = sector[best].ceilingshade;
        startwall = sector[best].wallptr;
        endwall = startwall + sector[best].wallnum;
        darkestwall = 0;
        for (j = startwall; j < endwall; j++)
            if (darkestwall < wall[j].shade)
                darkestwall = wall[j].shade;

        for (i = 0; i < numsectors; i++)
            if (i != best && sector[i].hitag == sector[sn].hitag) {
                /* COMPILER_STEERED: interleave wall-bound loads with the palette writes. */
                sector[i].floorshade = floor_shade;
                sector[i].ceilingshade = ceiling_shade;
                startwall = sector[i].wallptr;
                sector[i].floorpal = 0;
                endwall = startwall + sector[i].wallnum;
                sector[i].ceilingpal = 0;
                for (j = startwall; j < endwall; j++) {
                    wall[j].shade = darkestwall;
                    if (wall[j].nextwall1 >= 0)
                        wall[wall[j].nextwall1].shade = darkestwall;
                }
            }
        return 1;
    }
    return 0;
}

/* Sector specials and the use key for player snum. */
void checksectors(short snum)
{
    short w;
    long i = -1;

    switch (sector[ps[snum].cursectnum].lotag) {
    case 32767:
        sector[ps[snum].cursectnum].lotag = 0;
        FTA(9);
        ps[snum].secret_rooms++;
        return;
    case -1:
        ps[snum].gm = MODE_EOL;
        return;
    }
    if (ps[snum].gm & MODE_TYPE)
        return;
    if (!(syncbits[snum] & SB_OPEN) && !(syncbits[snum] & SB_ESCAPE)) {
        ps[snum].toggle_key_flag = 0;
        return;
    }
    if (ps[snum].toggle_key_flag)
        return;

    ps[snum].toggle_key_flag = 1;
    neartag(ps[snum].posx, ps[snum].posy, ps[snum].posz + 5120, ps[snum].cursectnum, ps[snum].ang,
        &neartagsector, &neartagwall, &neartagsprite, &neartaghitdist, 1024);
    if (wall[neartagwall].picnum == 180 || ps[snum].newowner >= 0) {
        if (syncbits[snum] & SB_ESCAPE)
            i = -1;
        else {
            i = headspritestat[STAT_ACTOR];
            while (i >= 0) {
                if (sprite[i].picnum == 265 && sprite[i].yvel == 0) {
                    sprite[i].yvel = 1;
                    ps[snum].newowner = i;
                    return;
                }
                i = nextspritestat[i];
            }
        }
        if (i == -1) {
            ps[snum].newowner = -1;
            ps[snum].posx = ps[snum].oposx;
            ps[snum].posy = ps[snum].oposy;
            ps[snum].posz = ps[snum].oposz;
            ps[snum].ang = ps[snum].oang;
            i = headspritestat[STAT_ACTOR];
            while (i >= 0) {
                if (sprite[i].picnum == 265)
                    sprite[i].yvel = 0;
                i = nextspritestat[i];
            }
        }
    }

    w = sector[neartagsector].lotag & 32767;
    switch (w) {
    case 9:
    case 15: case 16: case 17: case 18: case 19: case 20: case 21: case 22: case 23:
    case 25: case 26: case 27: case 29: {
        short sector_number = neartagsector;
        short owner = ps[snum].i;
        operatesectors(sector_number, owner);
        return;
    }
    }
    {
        short under_tag = sector[ps[snum].cursectnum].lotag & 32767;
        if ((unsigned short)under_tag < 15 || ((unsigned short)under_tag > 19 && ((unsigned short)under_tag < 26 || (unsigned short)under_tag > 28)))
            goto wall_switch;
        {
            short current_sector = ps[snum].cursectnum;
            short owner = ps[snum].i;
            operatesectors(current_sector, owner);
            return;
        }
    }
wall_switch: {
    short target = neartagwall;

    checkhitswitch(target);
}
}

/* Align all warp elevators of a group to the master (shade > 16) at level start. */
void allignwarpelevators(void)
{
    short i, j;

    for (i = headspritestat[STAT_EFFECTOR]; i >= 0; i = nextspritestat[i])
        if (sprite[i].lotag == 17 && sprite[i].shade > 16)
            for (j = headspritestat[STAT_EFFECTOR]; j >= 0; j = nextspritestat[j])
                if (sprite[j].lotag == 17 && i != j && sprite[i].hitag == sprite[j].hitag) {
                    sector[sprite[j].sectnum].floorz = sector[sprite[i].sectnum].floorz;
                    sector[sprite[j].sectnum].ceilingz = sector[sprite[i].sectnum].ceilingz;
                }
}
