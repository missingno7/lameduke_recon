#include <stdlib.h>
#include "sector.h"
#include "operate_helpers.h"

short animatesect[MAXANIMATES];
long *animateptr[MAXANIMATES];
long animategoal[MAXANIMATES], animatevel[MAXANIMATES], animatecnt;
animwalltype animwall[MAXANIMWALLS];
short numanimwalls;
extern short neartagsprite, neartagsector, neartagwall;
extern long neartaghitdist;

short checkcursectnums(short sn)
{
    short i;

    for (i = connecthead; i >= 0; i = connectpoint2[i])
        if (ps[i].cursectnum == sn)
            return i;
    return -1;
}

short findplayer(short sn, long *d)
{
    short i, closest_player;
    long distance, closest;

    closest = 0x7fffffff;
    for (i = connecthead; i >= 0; i = connectpoint2[i]) {
        distance = klabs(ps[i].posx - sprite[sn].x) +
            klabs(ps[i].posy - sprite[sn].y);
        if (distance < closest) {
            closest_player = i;
            closest = distance;
        }
    }
    *d = closest;
    return closest_player;
}

void doanimations(long tics)
{
    long i, j, a;

    for (i = animatecnt - 1; i >= 0; i--) {
        a = *animateptr[i];
        if (a < animategoal[i]) {
            a += animatevel[i] * tics;
            for (j = connecthead; j >= 0; j = connectpoint2[j])
                if (ps[j].cursectnum == animatesect[i] && ps[j].on_ground)
                    ps[j].posz += animatevel[i] * tics;
            if (a > animategoal[i]) {
                stopsound(SOUND_LIFT_MOVING);
                xyzsound(SOUND_LIFT_STOP, -1,
                    wall[sector[animatesect[i]].wallptr].x,
                    wall[sector[animatesect[i]].wallptr].y,
                    sector[animatesect[i]].floorz);
                a = animategoal[i];
            }
        }
        if (a > animategoal[i]) {
            a -= animatevel[i] * tics;
            for (j = connecthead; j >= 0; j = connectpoint2[j])
                if (ps[j].cursectnum == animatesect[i] && ps[j].on_ground)
                    ps[j].posz -= animatevel[i] * tics;
            if (a < animategoal[i]) {
                stopsound(SOUND_LIFT_MOVING);
                xyzsound(SOUND_LIFT_STOP, -1,
                    wall[sector[animatesect[i]].wallptr].x,
                    wall[sector[animatesect[i]].wallptr].y,
                    sector[animatesect[i]].floorz);
                a = animategoal[i];
            }
        }
        *animateptr[i] = a;
        if (a == animategoal[i]) {
            animatecnt--;
            animateptr[i] = animateptr[animatecnt];
            animategoal[i] = animategoal[animatecnt];
            animatevel[i] = animatevel[animatecnt];
            animatesect[i] = animatesect[animatecnt];
        }
    }
}

short getanimationgoal(long *position)
{
    long i, index = -1;

    for (i = 0; i < animatecnt; i++)
        if (position == animateptr[i]) {
            index = i;
            break;
        }
    return index;
}

int setanimation(short sn, long *position, long goal, long velocity)
{
    long i, index;

    if (animatecnt >= MAXANIMATES - 1)
        return -1;

    index = animatecnt;
    for (i = 0; i < animatecnt; i++)
        if (position == animateptr[i]) {
            index = i;
            break;
        }
    animatesect[index] = sn;
    animategoal[index] = goal;
    animateptr[index] = position;
    animatevel[index] = velocity;
    if (index == animatecnt)
        animatecnt++;
    return animatecnt - 1;
}

void animatecamsprite(short p)
{
    short i, camera;

    if (viewscreen != -1) {
        i = viewscreen;
        if (hittype[i][0] >= 11) {
            hittype[i][0] = 0;
            if (ps[p].camera >= 0)
                sprite[i].owner = ps[p].camera;
            if (sprite[i].owner >= 0) {
                camera = sprite[i].owner;
                if (klabs(sprite[ps[p].i].x - sprite[i].x) +
                    klabs(sprite[ps[p].i].y - sprite[i].y) < 2048)
                    render_camera_tile(sprite[camera].x, sprite[camera].y,
                        sprite[camera].z, sprite[camera].ang, sprite[camera].sectnum, 1408);
            }
        } else
            hittype[i][0]++;
    }
}

void animatewalls(void)
{
    short i, w, phase, mirror;

    mirror = 0;
    if ((gotpic[570 >> 3] & 4) > 0)
        mirror = 1;
    if ((gotpic[49 >> 3] & 2) > 0 && waloff[49] != -1)
        update_ripple_tile(waloff[49]);

    for (i = 0; i < numanimwalls; i++) {
        w = animwall[i].wallnum;
        switch (wall[w].picnum) {
        case 819: case 820: case 821: case 822:
            if (wall[w].picnum == 822)
                wall[w].picnum = 819;
            else
                wall[w].picnum++;
            break;
        case 823: case 824: case 825: case 826:
            if (wall[w].picnum == 826)
                wall[w].picnum = 823;
            else
                wall[w].picnum++;
            break;
        case 860: case 861: case 862: case 863:
            if (wall[w].picnum == 859)
                wall[w].picnum = 860;
            else
                wall[w].picnum++;
            break;
        case 864: case 865: case 866: case 867:
            if (wall[w].picnum == 867)
                wall[w].picnum = 864;
            else
                wall[w].picnum++;
            break;
        case 570:
            if (mirror) {
                render_mirror(w);
                mirror = 0;
            }
            break;
        case 764: case 765: case 766: case 767:
            if ((rand() & 255) < 16) {
                animwall[i].tag = wall[w].picnum;
                wall[w].picnum = 761;
            }
            break;
        case 761: case 762: case 763:
            wall[w].picnum = (short)animwall[i].tag;
            break;
        default:
            switch (wall[w].overpicnum) {
            case FORCEFIELD: case FORCEFIELD + 1: case FORCEFIELD + 2:
                phase = (short)animwall[i].tag;
                if (wall[w].cstat & 254) {
                    wall[w].xpanning += sintable[(phase + 512) & 2047] >> 12;
                    wall[w].ypanning += sintable[phase & 2047] >> 12;
                    animwall[i].tag += 128;
                    if (++wall[w].overpicnum > FORCEFIELD + 2)
                        wall[w].overpicnum = FORCEFIELD;
                    if (rand() & 4)
                        wall[w].cstat ^= 128;
                }
                break;
            }
            break;
        }
    }
    gotpic[49 >> 3] &= ~2;
    gotpic[570 >> 3] &= 0xeb;
}

void activatewarpelevators(short i, short direction)
{
    short j;

    for (j = headspritestat[3]; j >= 0; j = nextspritestat[j])
        if (sprite[j].lotag == 17 && sprite[j].hitag == sprite[i].hitag)
            if (klabs(sector[sprite[i].sectnum].floorz - hittype[i][2]) > sprite[j].yvel ||
                sector[sprite[j].sectnum].hitag == sector[sprite[i].sectnum].hitag - direction)
                break;

    if (j == -1)
        direction = 0;
    else if (direction == 0)
        spritesound(SOUND_LIFT_STOP, i);
    else
        spritesound(SOUND_LIFT_START, i);

    for (j = headspritestat[3]; j >= 0; j = nextspritestat[j])
        if (sprite[j].lotag == 17 && sprite[j].hitag == sprite[i].hitag)
            hittype[j][1] = hittype[j][0] = direction;
}

void operatesectors(short sn, short source)
{
#define SECTOR_INLINE_HELPERS
#include "operate.inc"
#undef SECTOR_INLINE_HELPERS
}

void operateactivators(short lotag)
{
    short i, sn, source;

    for (i = headspritestat[8]; i >= 0; i = nextspritestat[i]) {
        if (sprite[i].lotag != lotag)
            continue;

        sn = sprite[i].sectnum;
        if (sprite[i].picnum == 23) {
            sector[sn].lotag ^= 0x4000;
        } else {
            source = i;
#include "operate.inc"
        }
    }
}

void operatemasterswitches(short lotag)
{
    short i;

    for (i = headspritestat[1]; i >= 0; i = nextspritestat[i])
        if (sprite[i].picnum == MASTERSWITCH && sprite[i].lotag == lotag && sprite[i].yvel == 0)
            sprite[i].yvel = 1;
}

void operateforcefields(short lotag)
{
    short i, w;
    long sn;
    unsigned char pal;

    for (i = 0; i < numanimwalls; i++) {
        w = animwall[i].wallnum;
        if (lotag == wall[w].lotag || lotag == -1) {
            switch (wall[w].overpicnum) {
            case FORCEFIELD: case FORCEFIELD + 1: case FORCEFIELD + 2:
                animwall[i].tag = 0;
                if (wall[w].cstat) {
                    wall[w].cstat = 0;
                    sn = wall[w].nextsector;
                    pal = 0;
                } else {
                    wall[w].cstat = 85;
                    sn = wall[w].nextsector;
                    pal = 2;
                }
                sector[sn].ceilingpal = sector[sn].floorpal = pal;
                break;
            }
        }
    }
}

void playerkeys(short p)
{
    short sn, source;

    if ((player_bits[p] & 0x10000) && ps[p].jump_pressed == 0)
        ps[p].jump_pressed = 1;
    if (ps[p].viewmode == 2) {
        if ((player_bits[p] & 0x80000) && ps[p].mapzoom < 4096)
            ps[p].mapzoom += ps[p].mapzoom >> 3;
        if ((player_bits[p] & 0x100000) && ps[p].mapzoom > 48)
            ps[p].mapzoom -= ps[p].mapzoom >> 3;
    } else {
        if (player_bits[p] & 0x80000) {
            if (ps[p].screen_size > -4)
                ps[p].screen_size -= 4;
            if (option[0] == 7)
                setview(0, 0, 639, 479);
            else
                vscrn(p);
        }
        if (player_bits[p] & 0x100000) {
            if (ps[p].screen_size < 64)
                ps[p].screen_size += 4;
            if (option[0] == 7)
                setview(0, 0, 639, 479);
            else
                vscrn(p);
        }
    }
    if (player_bits[p] < 255) {
        ps[p].keys_pressed = 0;
        return;
    }
    if (ps[p].keys_pressed)
        return;
    ps[p].keys_pressed = 1;
    if (player_bits[p] & 0x200000)
        gamepaused = !gamepaused;
    if (gamepaused)
        return;
    if ((player_bits[p] & 0x80000000UL) && !(ps[p].game_mode & 2) && ps[p].camera == -1) {
        keystatus[1] = 0;
        menupage = 0;
        ps[p].game_mode |= 1;
        if (ps[p].viewmode == 2) {
            ps[p].viewmode = 3;
            if (p == screenpeek)
                setgamemode();
        }
    }
    if (player_bits[p] & 0x400000) {
        if (ps[p].viewmode == 3) {
            ps[p].viewmode = 2;
            if (p == screenpeek)
                qsetmode640480();
        } else {
            ps[p].viewmode = 3;
            setgamemode();
            vscrn(p);
            displayrooms(p);
        }
    }
    if (player_bits[p] & 0x20000)
        ps[p].toggle_view ^= 1;
    if ((ps[p].weapon_select & 127) == 6) {
        if (player_bits[p] & 0x100)
            addweapon(p, 1230, 1);
        if ((player_bits[p] & 0x200) && (ps[p].gotweapon[0] || disable_armor))
            addweapon(p, 191, 1);
        if ((player_bits[p] & 0x400) && (ps[p].gotweapon[1] || disable_armor))
            addweapon(p, 389, 1);
        if ((player_bits[p] & 0x800) && (ps[p].gotweapon[2] || disable_armor))
            addweapon(p, 721, 1);
        if ((player_bits[p] & 0x1000) && (ps[p].gotweapon[3] || disable_armor))
            addweapon(p, 563, 1);
    }
    if ((player_bits[p] & 0x1000000) && (ps[p].steroids || disable_armor)) {
        if (disable_armor)
            ps[p].steroids = 1;
        ps[p].steroids_on = ps[p].steroids_on == 1 ? 0 : 128;
    }
    if ((player_bits[p] & 0x2000000) && (ps[p].shield || disable_armor)) {
        if (disable_armor)
            ps[p].shield = 1;
        ps[p].nightvision_charge = ps[p].nightvision_charge == 0 ? 32 : 0;
    }
    if ((player_bits[p] & 0x4000000) && (ps[p].jetpack || disable_armor)) {
        if (disable_armor)
            ps[p].jetpack = 1;
        ps[p].jetpack_on ^= 1;
    }
    if ((player_bits[p] & 0x8000000) && (ps[p].holoduke_charge || disable_armor)) {
        if (disable_armor)
            ps[p].holoduke_charge = 1;
        if (ps[p].holoduke)
            ps[p].holoduke = 0;
        else {
            ps[p].holoduke = EGS(ps[p].cursectnum, ps[p].posx, ps[p].posy, ps[p].posz,
                0, 283, -16, 48, 48, ps[p].ang, 0, 0, -1, 10);
            hittype[ps[p].holoduke][0] = p;
        }
    }
    if ((player_bits[p] & 0x10000000) && (ps[p].scuba || disable_armor)) {
        if (disable_armor)
            ps[p].scuba = 1;
        ps[p].scuba_on = !ps[p].scuba_on;
        spritesound(ps[p].scuba_on ? 18 : 27, ps[p].i);
    }
    if ((player_bits[p] & 0x40000000) && ps[p].weapon_locked == -1)
        ps[p].weapon_select = (ps[p].weapon_select & 128) ? 5 : 129;
    if (!disable_armor) {
        if (player_bits[p] & 0x1000000) {
            menupage = 20;
            ps[p].game_mode |= 1;
        }
        if (player_bits[p] & 0x2000000) {
            menupage = 50;
            ps[p].game_mode |= 1;
        }
        if (player_bits[p] & 0x4000000) {
            menupage = 10;
            ps[p].game_mode |= 1;
        }
    } else if (player_bits[p] & 0x800000) {
        short i, w;

        FTA(11);
        for (sn = 0; sn < numsectors; sn++) {
            switch ((short)(sector[sn].lotag & 0xfff)) {
            case 15: case 16: case 17: case 18: case 19:
            case 20: case 21: case 22: case 23:
            case 25: case 26: case 27: case 28: case 29:
                if (sector[sn].lotag & 0xbfff)
                    sector[sn].lotag &= ~0x4000;
                source = ps[p].i;
#include "operate.inc"
                break;
            }
        }
        for (i = 0; i < numanimwalls; i++) {
            w = animwall[i].wallnum;
            switch (wall[w].overpicnum) {
            case FORCEFIELD: case FORCEFIELD + 1: case FORCEFIELD + 2:
                animwall[i].tag = 0;
                if (wall[w].cstat) {
                    wall[w].cstat = 0;
                    sector[wall[w].nextsector].ceilingpal = 0;
                    sector[wall[w].nextsector].floorpal = 0;
                } else {
                    wall[w].cstat = 85;
                    sector[wall[w].nextsector].ceilingpal = sector[wall[w].nextsector].floorpal = 2;
                }
                break;
            }
        }
    }
}

void checkhitswitch(long w)
{
#define SWITCH_EFFECTORS_INLINED
#include "switch.inc"
#undef SWITCH_EFFECTORS_INLINED
}

void checkhitwall(long w, long x, long y, long z, short weapon)
{
    short sn, i, j, k, end, next;
    long shade;
    signed char floor_shade, ceiling_shade;

    switch (wall[w].overpicnum) {
    case 179:
    case 331:
    case 573:
        wall[w].overpicnum = 573;
        updatesector(x, y, &sn);
        i = EGS(sn, x, y, z, 0, SECTOREFFECTOR, 0, 0, 0, ps[0].ang, 0, 0, ps[screenpeek].i, 3);
        sprite[i].lotag = 128;
        hittype[i][1] = 5;
        hittype[i][2] = w;
        spritesound(5, i);
        return;
    case 790:
        if (weapon == 563)
            wall[wall[w].nextwall].overpicnum = wall[w].overpicnum = 573;
        else {
            updatesector(x, y, &sn);
            i = EGS(sn, x, y, z, 0, SECTOREFFECTOR, 0, 0, 0, 0, 0, 0, ps[screenpeek].i, 3);
            sprite[i].lotag = 129;
            hittype[i][2] = w;
        }
        return;
    }
    switch (wall[w].picnum) {
    case 819: case 820: case 821: case 822:
        wall[w].picnum = 1183;
        xyzsound(23, -1, wall[w].x, wall[w].y, sector[wall[w].nextsector].floorz);
        break;
    case 823: case 824: case 825: case 826:
        wall[w].picnum = 1182;
        xyzsound(23, -1, wall[w].x, wall[w].y, sector[wall[w].nextsector].floorz);
        break;
    case 860: case 861: case 862: case 863:
        wall[w].picnum = 1180;
        xyzsound(23, -1, wall[w].x, wall[w].y, sector[wall[w].nextsector].floorz);
        break;
    case 864: case 865: case 866: case 867:
        wall[w].picnum = 1181;
        xyzsound(23, -1, wall[w].x, wall[w].y, sector[wall[w].nextsector].floorz);
        break;
    case 35: case 36: case 26: case 212:
        if (wall[w].lotag == 0)
            break;
        if (wall[w].picnum == 35) wall[w].picnum = 664;
        if (wall[w].picnum == 36) wall[w].picnum = 665;
        if (wall[w].picnum == 26) wall[w].picnum = 670;
        if (wall[w].picnum == 212) wall[w].picnum = 671;

        sn = wall[w].nextsector;
        j = sector[sn].wallptr;
        end = j + sector[sn].wallnum;
        shade = 0;
        for (; j < end; j++)
            if (wall[j].shade > shade)
                shade = wall[j].shade;
        floor_shade = sector[sn].floorshade;
        ceiling_shade = sector[sn].ceilingshade;
        for (i = headspritestat[3]; i >= 0; i = nextspritestat[i])
            if (wall[w].lotag == sprite[i].hitag && sprite[i].lotag == 3) {
                hittype[i][3] = shade;
                hittype[i][4] = 1;
            }
        for (i = 0; i < numsectors; i++)
            if (i != sn && wall[w].lotag == sector[i].hitag) {
                sector[i].floorpal = 0;
                sector[i].ceilingpal = 0;
                for (j = headspritesect[i]; j >= 0; j = next) {
                    next = nextspritesect[j];
                    if (sprite[j].picnum == SECTOREFFECTOR && (sprite[j].lotag == 8 || sprite[j].lotag == 9))
                        deletesprite(j);
                }
                sector[i].floorshade = floor_shade;
                sector[i].ceilingshade = ceiling_shade;
                k = sector[i].wallptr;
                end = k + sector[i].wallnum;
                for (; k < end; k++) {
                    next = wall[k].nextwall;
                    wall[k].shade = (signed char)shade;
                    if (next >= 0)
                        wall[next].shade = (signed char)shade;
                }
            }
        break;
    }
}

char checkhitceiling(short sn)
{
    short i, j, end, next, shade, best, ceiling;
    signed char floor_shade, ceiling_shade;

    if (sector[sn].hitag == 0)
        goto no_hit;
    switch (sector[sn].ceilingpicnum) {
    case 35: case 36: case 26: case 212:
        if (sector[sn].ceilingpicnum == 35) sector[sn].ceilingpicnum = 664;
        if (sector[sn].ceilingpicnum == 36) sector[sn].ceilingpicnum = 665;
        if (sector[sn].ceilingpicnum == 26) sector[sn].ceilingpicnum = 670;
        if (sector[sn].ceilingpicnum == 212) sector[sn].ceilingpicnum = 671;
        break;
    default:
        goto no_hit;
    }

    ceiling = sector[sn].ceilingshade;
    for (i = headspritestat[3]; i >= 0; i = nextspritestat[i])
        if (sprite[i].hitag == sector[sn].hitag && sprite[i].lotag == 3)
            hittype[i][4] = 1;
    /* The original assigns best only after finding a darker linked ceiling. */
    for (i = 0; i < numsectors; i++)
        if (i != sn && sector[i].hitag == sector[sn].hitag && ceiling < sector[i].ceilingshade) {
            ceiling = sector[i].ceilingshade;
            best = i;
        }

    floor_shade = sector[best].floorshade;
    ceiling_shade = sector[best].ceilingshade;
    j = sector[best].wallptr;
    end = j + sector[best].wallnum;
    shade = 0;
    for (; j < end; j++)
        if (shade < wall[j].shade)
            shade = wall[j].shade;

    for (i = 0; i < numsectors; i++)
        if (i != best && sector[i].hitag == sector[sn].hitag) {
            sector[i].floorshade = floor_shade;
            sector[i].floorpal = 0;
            sector[i].ceilingshade = ceiling_shade;
            j = sector[i].wallptr;
            end = j + sector[i].wallnum;
            sector[i].ceilingpal = 0;
            for (; j < end; j++) {
                next = wall[j].nextwall;
                wall[j].shade = (signed char)shade;
                if (next >= 0)
                    wall[next].shade = (signed char)shade;
            }
        }
    return 1;

no_hit:
    return 0;
}

void checksectors(short p)
{
    short i, sn, source;
    long w;

    switch (sector[ps[p].cursectnum].lotag) {
    case 32767:
        sector[ps[p].cursectnum].lotag = 0;
        FTA(9);
        ps[p].secrets++;
        return;
    case -1:
        ps[p].game_mode = 8;
        return;
    }
    if (ps[p].game_mode & 16)
        return;
    if (!(player_bits[p] & (0x80000000UL | 16))) {
        ps[p].use_pressed = 0;
        return;
    }
    if (ps[p].use_pressed)
        return;

    ps[p].use_pressed = 1;
    neartag(ps[p].posx, ps[p].posy, ps[p].posz + 5120, ps[p].cursectnum, ps[p].ang,
        &neartagsector, &neartagwall, &neartagsprite, &neartaghitdist, 1024);
    if (wall[neartagwall].picnum == 180 || ps[p].camera >= 0) {
        if (!(player_bits[p] & 0x80000000UL))
            for (i = headspritestat[1]; i >= 0; i = nextspritestat[i])
                if (sprite[i].picnum == 265 && sprite[i].yvel == 0) {
                    sprite[i].yvel = 1;
                    ps[p].camera = i;
                    return;
                }

        ps[p].camera = -1;
        ps[p].posx = ps[p].oldx;
        ps[p].posy = ps[p].oldy;
        ps[p].posz = ps[p].oldz;
        ps[p].ang = ps[p].oldang;
        for (i = headspritestat[1]; i >= 0; i = nextspritestat[i])
            if (sprite[i].picnum == 265)
                sprite[i].yvel = 0;
    }

    switch ((short)(sector[neartagsector].lotag & 32767)) {
    case 9:
    case 15: case 16: case 17: case 18: case 19: case 20: case 21: case 22: case 23:
    case 25: case 26: case 27: case 29:
        sn = neartagsector;
        source = ps[p].i;
#include "operate.inc"
        return;
    }
    switch ((short)(sector[ps[p].cursectnum].lotag & 32767)) {
    case 15: case 16: case 17: case 18: case 19:
    case 26: case 27: case 28:
        sn = ps[p].cursectnum;
        source = ps[p].i;
#define lift_up current_lift_up
#define elevator_up current_elevator_up
#include "operate.inc"
#undef elevator_up
#undef lift_up
        return;
    }
    w = neartagwall;
#include "switch.inc"
}

void allignwarpelevators(void)
{
    short i, j;

    for (i = headspritestat[3]; i >= 0; i = nextspritestat[i])
        if (sprite[i].lotag == 17 && sprite[i].shade > 16)
            for (j = headspritestat[3]; j >= 0; j = nextspritestat[j])
                if (sprite[j].lotag == 17 && i != j && sprite[i].hitag == sprite[j].hitag) {
                    sector[sprite[j].sectnum].floorz = sector[sprite[i].sectnum].floorz;
                    sector[sprite[j].sectnum].ceilingz = sector[sprite[i].sectnum].ceilingz;
                }
}
