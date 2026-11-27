/* PLAYER: per-tick input sampling (getinput), player physics and weapon state machine (processinput), first-person weapon drawing (displayweapon, animateknuckles) and weapon firing (shoot).
   Descends from Ken Silverman's Sep-94 Build demo GAME.C getinput/processinput (sync arrays; NOTE our svel/vel names are swapped vs Ken's vel/svel) and is the direct ancestor of Duke Nukem 3D 1.5 PLAYER.C (1996):
   same sync-bit layout (jump 0, crouch 1, fire 2, run 5, look 6/7, weapons 8+, look up/down 13/14, center 18, pause 21, Esc 31) and same player_struct field order (fta/ftq ... somethingonplayer, on_crane, one_parallax_sectnum; toggle_key_flag, knuckle_incs ... hard_landing, max_secret_rooms).
   BINARY_DERIVED; semantics from DEFS.CON/GAME.CON, raynorpat rebuild, Ken K94s GAME.C and Duke 1.5 PLAYER.C. */

#include "player.h"

/* The tile IDs used by the prototype and its DEFS.CON. */
#define SHOTSPARK1 191
#define LASERSITE 214
#define APLAYER 283
#define BULLET 389
#define RPG 563
#define HEAVYHBOMB 721
#define HANDREMOTE 1140
#define OTHERSHOTSPARK 1210
#define CLUB 1230
extern struct user_defs ud;         /* the 21-byte USERDEFS.TMP block defined by GLOBAL */

/* dataown: one object, one name. Code uses the fields of GLOBAL's settings block ud directly; the unused
   externs user_visibility, mouse_sensitivity, sound_volume and disable_armor in player.h only keep this
   unit's symbol slots (docs/BSS_LAYOUT.md). */

/* Kick player snum's view (horiz +-4, look_ang = +-2*damage) away from the side sprite i hit from. */
void kickview(short snum, short i)
{
    short angle;

    if (sprite[ps[snum].i].extra / 20 >= ps[snum].health) {
        angle = getincangle(sprite[i].ang,
            getangle(sprite[i].x - ps[snum].posx, sprite[i].y - ps[snum].posy), 0);
        if (angle < -512 || angle >= 512)
            ps[snum].horiz -= 4;
        else
            ps[snum].horiz += 4;
        if (angle < 0)
            ps[snum].look_ang = -(sprite[i].extra << 1);
        else if (angle >= 0)
            ps[snum].look_ang = sprite[i].extra << 1;
    }
}

/* COMPILER_STEERED: expand the gun's angle adjustment at each use. */
#define TRACER_AIM_ANGLE(base, pos) ((base) + ((6 - (pos)) << 4))

/* Pistol laser sight: hitscan along the gun's aim and place a one-frame LASERSITE dot at the hit point. */
void spawn_pistol_tracer(short snum)
{
    /* COMPILER_STEERED: local order for the hitscan argument homes. */
    unsigned char weapon_pos;
    long sx;
    short hitspr;
    short hitwall;
    long sy;
    short i;
    long hity;
    short hitsect;
    long sz;
    long sa;
    long hitz;
    long hitx;

    sx = ps[snum].posx;
    sy = ps[snum].posy;
    weapon_pos = ps[snum].weapon_pos & 127;
    sz = ps[snum].posz - (weapon_pos << 10) + 5376;
    sa = ps[snum].ang - 16;
    hitscan(sx + (sintable[(TRACER_AIM_ANGLE(sa, weapon_pos) + 512) & 2047] >> 8),
        sy + (sintable[TRACER_AIM_ANGLE(sa, weapon_pos) & 2047] >> 8),
        sz, ps[snum].cursectnum,
        sintable[(TRACER_AIM_ANGLE(sa, weapon_pos) + (syncangvel[snum] >> 3) + 512) & 2047],
        sintable[(TRACER_AIM_ANGLE(sa, weapon_pos) + (syncangvel[snum] >> 3)) & 2047],
        (100 - ps[snum].horiz) << 11, &hitsect, &hitwall, &hitspr,
        &hitx, &hity, &hitz);

    sa = TRACER_AIM_ANGLE(sa, weapon_pos) + 1024;
    i = EGS(hitsect, hitx + (sintable[(sa + 512) & 2047] >> 8),
        hity + (sintable[sa & 2047] >> 8),
        hitz + 1024 + (sintable[ps[snum].bobcounter & 2047] >> 8),
        0, LASERSITE, -16, 12, 12, sa, 0, 0, ps[snum].i, 5);
    /* COMPILER_STEERED: reuse the finished aim local for the sprite size. */
    sa = (ksqrt((ps[screenpeek].posx - sprite[i].x) * (ps[screenpeek].posx - sprite[i].x)
        + (ps[screenpeek].posy - sprite[i].y) * (ps[screenpeek].posy - sprite[i].y)) >> 8) + 1;
    if (sa > 32)
        sa = 32;
    sprite[i].xrepeat = sprite[i].yrepeat = sa;
    if (sector[sprite[i].sectnum].floorz - 768 < sprite[i].z)
        sprite[i].z -= 768;
}

#undef TRACER_AIM_ANGLE

/* Fire weapon tile atwith from sprite i (player or actor); with impact != 0, move projectile i onto what it hits. */
void shoot(short i, short atwith, unsigned char impact)
{
    /* COMPILER_STEERED: declaring p here gives the two inlined
       findplayer results separate stack homes in processinput. */
    short hitspr;
    short hitwall;
    long sx;
    long sy;
    long sz;
    long j;
    short p;
    long distance;
    short sect;
    if (impact) {
        hitscan(sprite[i].x, sprite[i].y, sprite[i].z, sprite[i].sectnum,
            sintable[(sprite[i].ang + 512) & 2047], sintable[sprite[i].ang & 2047], 0,
            &sect, &hitwall, &j, &sx, &sy, &sz);
        if (ksqrt((sprite[i].x - sx) * (sprite[i].x - sx)
            + (sprite[i].y - sy) * (sprite[i].y - sy)) > 512)
            return;
        if (sector[sprite[i].sectnum].ceilingz + 2560 <= sprite[i].z
            || checkhitceiling(sprite[i].sectnum) == 0) {
            setsprite(i, sx, sy, sz);
            if (hitwall >= 0) {
                checkhitswitch(hitwall);
                checkhitwall(hitwall, sx, sy, sz, atwith);
            }
        }
        return;
    }

    /* COMPILER_STEERED: scope and order for the firing locals. */
    {
        long zvel;
        short sa;

        if (sprite[i].picnum == APLAYER) {
            sx = ps[sprite[i].yvel].posx + (syncangvel[sprite[i].yvel] >> 4);
            sy = ps[sprite[i].yvel].posy + (syncangvel[sprite[i].yvel] >> 4);
            sz = ps[sprite[i].yvel].posz + 3584;
            sa = ps[sprite[i].yvel].ang - 8;
            if (atwith == RPG)
                sa += ps[sprite[i].yvel].look_ang;
            sect = ps[sprite[i].yvel].cursectnum;
            ps[sprite[i].yvel].fire_flag = 1;
            switch (atwith) {
            case SHOTSPARK1:
                zvel = (100 - ps[sprite[i].yvel].horiz) * 31;
                break;
            case RPG:
                zvel = (100 - ps[sprite[i].yvel].horiz) * 15;
                sz -= 1536;
                break;
            default:
                zvel = (100 - ps[sprite[i].yvel].horiz) * 22;
                break;
            }
            {
                /* COMPILER_STEERED: cache the owner for the condition. */
                long owner = sprite[i].yvel;
                if (ps[owner].holoduke_on)
                    sprite[ps[sprite[i].yvel].holoduke_on].shade = 0;
            }
        } else {
            sx = sprite[i].x;
            sy = sprite[i].y;
            sz = sprite[i].z - 8192;
            sa = sprite[i].ang;
            p = findplayer(i, &distance);
            zvel = ((ps[p].posz - sprite[i].z) << 7)
                / (klabs(sprite[ps[p].i].x - sprite[i].x) + klabs(sprite[ps[p].i].y - sprite[i].y));
            sect = sprite[i].sectnum;
        }

        switch (atwith & 0xffff) {
        case CLUB: {
            long old_sz;

            sx += sintable[(sa + 544) & 2047] >> 7;
            sy += sintable[(sa + 32) & 2047] >> 7;
            /* EGS receives the height before this decrement. */
            old_sz = sz;
            sz -= 2048;
            j = EGS(sect, sx, sy, old_sz, 0, 46, -16, 0, 0, sa, 32, 0, i, 4);
            if (actorscrptr[CLUB])
                sprite[i].extra = *actorscrptr[CLUB];
            break;
        }
        case SHOTSPARK1:
            sa -= 8;
            hitscan(sx, sy, sz, sect, sintable[(sa + 512) & 2047],
                sintable[sa & 2047], zvel << 6, &sect, &hitwall, &hitspr, &sx, &sy, &sz);
            spritesound(31, i);
            if (sector[sect].ceilingz + 1024 > sz)
                sz = sector[sect].ceilingz + 2048;
            if (sprite[i].picnum == APLAYER) {
                sprite[(short)EGS(sect, sx, sy, sz - 2816, 0, OTHERSHOTSPARK, -16, 10, 10, sa, 64, zvel, i, 4)].yvel = hitwall;
            } else
                EGS(sect, sx, sy, sz - 2304, 0, SHOTSPARK1, -16, 16, 16, sa, 64, zvel, i, 4);
            break;
        case BULLET:
            spritesound(1, i);
            spritesound(24, i);
            EGS(sect, sx + (sintable[(sa + 512) & 2047] >> 6),
                sy + (sintable[sa & 2047] >> 6), sz - 2048,
                1, BULLET, -127, 12, 12, sa, 0, zvel, i, 4);
            break;
        case RPG:
            spritesound(38, i);
            EGS(sect, sx + (sintable[(sa + 640) & 2047] >> 5),
                sy + (sintable[(sa + 128) & 2047] >> 5), sz - 2304,
                65, RPG, -16, 16, 16, sa, 0, zvel, i, 4);
            break;
        }
    }
}

/* Draw the knuckle-crack animation (CRACKKNUCKLES) for player snum; return 1 if it was drawn. */
unsigned char animateknuckles(short gs, short snum)
{
    short knuckle_frames[11] = { 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0 };
    short looking_arc;

    if (!ps[snum].knuckle_incs || ps[snum].health <= 0)
        return 0;

    looking_arc = klabs(ps[snum].look_ang) / 9;
    myos((syncangvel[snum] >> 4) + 160 - (ps[snum].look_ang >> 1),
        looking_arc + 180 - (ps[snum].horiz >> 4), knuckle_frames[ps[snum].knuckle_incs - 1] + 681, gs, 3);
    if (ps[snum].knuckle_incs == 5)
        spritesound(7, ps[snum].i);
    return 1;
}
signed char clubframes[6][20] = {
    { 0, 0, 1, 2, 3, 4, 4, 3, 2, -1 },
    { 0, 0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, 1, 0, -1 },
    { 0, 0, 1, 2, 3, 4, 3, 2, -1 },
    { 0, 0, 2, 3, 4, 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, -1 },
    { 0, 0, 2, 4, 6, 8, 7, 5, 4, 3, 1, -1 },
    { 0, 0, 1, 2, 3, 4, 3, 1, 0, -1 }
};

/* Draw player snum's first-person weapon (after the knuckle-crack animation). */
void displayweapon(short snum)
{
    /* COMPILER_STEERED: local order for the knee table and draw argument allocation. */
    short pistol_x;
    long draw_x;
    long xvel;
    short looking_arc;
    long gs;
    unsigned short cw;
    unsigned char weapon_pos;
    long tile;
    long weapon_xoffset;
    short look;
    long draw_y;
    unsigned char gun_pos;

    if (ps[snum].health <= 0)
        return;
    look = ps[snum].look_ang;
    visibility = ud.visibility;
    looking_arc = klabs(look) / 9;
    xvel = 0;
    if (syncbits[snum] & SB_RUN)
        xvel = syncangvel[snum] >> 3;
    gs = sector[ps[snum].cursectnum].floorshade;
    if (gs < 0)
        gs = 0;
    else if (gs > 24)
        gs = 24;
    if (animateknuckles(gs, snum))
        return;
    weapon_pos = ps[snum].weapon_pos & 127;
    if (weapon_pos < 2)
        return;
    gun_pos = (weapon_pos << 4) - 32;
    gun_pos -= sintable[ps[snum].bobcounter & 2047] >> 12;
    weapon_xoffset = (xvel + 12) + ((6 - weapon_pos) << 4);
    cw = ps[snum].last_weapon >= 0 ? ps[snum].last_weapon : ps[snum].current_weapon;

    switch (cw) {
    case CLUB: {
        switch ((unsigned char)clubframes[ps[snum].knee_sequence][ps[snum].kickback_pic]) {
        case 0:
        case 1: break;
        case 2: weapon_xoffset -= 10; break;
        case 3: weapon_xoffset -= 20; break;
        case 4: weapon_xoffset -= 30; break;
        case 5: weapon_xoffset -= 40; break;
        case 6: weapon_xoffset -= 40; break;
        case 7: weapon_xoffset -= 60; break;
        case 8: weapon_xoffset -= 80; break;
        case 9: weapon_xoffset -= 100; break;
        case 10: weapon_xoffset -= 130; break;
        }

        if (ps[snum].kickback_pic) {
            myos((weapon_xoffset + 180) + (syncangvel[snum] >> 4) - (ps[snum].look_ang >> 1),
                looking_arc + 280 - gun_pos - (ps[snum].horiz >> 4),
                CLUB + clubframes[ps[snum].knee_sequence][ps[snum].kickback_pic], gs, 3);
            if (clubframes[ps[snum].knee_sequence][ps[snum].kickback_pic] == 2) {
                if (ps[snum].ammo_bat_amount && (sector[ps[snum].cursectnum].lotag & 32767) != 2) {
                    draw_x = (weapon_xoffset + 229) + (syncangvel[snum] >> 4) - (ps[snum].look_ang >> 1);
                    draw_y = looking_arc + 207 - gun_pos - (ps[snum].horiz >> 4);
                    tile = 1240 + ps[snum].knee_flash % 3;
                    goto draw_overlay;
                }
                else
                    myos((weapon_xoffset + 229) + (syncangvel[snum] >> 4) - (ps[snum].look_ang >> 1),
                        looking_arc + 207 - gun_pos - (ps[snum].horiz >> 4), 1239, gs, 3);
            }
        } else
            myos((weapon_xoffset + 180) + (syncangvel[snum] >> 4) - (ps[snum].look_ang >> 1),
                looking_arc + 280 - gun_pos - (ps[snum].horiz >> 4), 1231, gs, 3);
        break;
    }
    case RPG:
        if (ps[snum].kickback_pic > 11) {
            unsigned char recoil[9] = { 40, 30, 22, 16, 12, 16, 22, 30, 30 };

            if (ps[snum].kickback_pic <= 13)
                gs = ps[snum].kickback_pic - 12;
            myos((weapon_xoffset + 270) + (syncangvel[snum] >> 5) - recoil[ps[snum].kickback_pic - 11],
                (looking_arc << 1) + 186 - gun_pos - (ps[snum].horiz >> 4), 1252, gs, 3);
        } else {
            weapon_xoffset += sintable[(1024 - (ps[snum].kickback_pic << 7)) & 2047] >> 9;
            gun_pos += sintable[(1536 - (ps[snum].kickback_pic << 7)) & 2047] >> 10;
            if (ps[snum].kickback_pic < 4)
                myos((weapon_xoffset + 150) + (syncangvel[snum] >> 5),
                    (looking_arc << 1) + 224 - gun_pos - (ps[snum].horiz >> 4), 1250, gs, 3);
            else if (ps[snum].kickback_pic < 7)
                myos((weapon_xoffset + 184) + (syncangvel[snum] >> 5),
                    (looking_arc << 1) + 180 - gun_pos - (ps[snum].horiz >> 4), 1251, gs, 3);
            else if (ps[snum].kickback_pic < 12)
                myos((weapon_xoffset + 260) + (syncangvel[snum] >> 5),
                    (looking_arc << 1) + 196 - gun_pos - (ps[snum].horiz >> 4), 1252, gs, 3);
        }
        break;
    case BULLET: {
        /* Watcom emits this initializer in the code prefix even when the
           body selects the same tiles directly. */
        short frames[5] = { 882, 883, 884, 885, 886 };

        switch (ps[snum].kickback_pic) {
        case 0:
            myos(weapon_xoffset + 160 + (syncangvel[snum] >> 4) - (ps[snum].look_ang >> 1),
                looking_arc + 240 - gun_pos - (ps[snum].horiz >> 4), 882, gs, 3);
            break;
        case 4:
            gs = 0;
            break;
        case 5:
            visibility++;
            break;
        }
        if (ps[snum].kickback_pic != 6 && ps[snum].kickback_pic != 0)
            myos(weapon_xoffset + 160 + (syncangvel[snum] >> 4) - (ps[snum].look_ang >> 1),
                looking_arc + 240 - gun_pos - (ps[snum].horiz >> 4), 881 + ps[snum].kickback_pic, gs, 3);
        break;
    }
    case SHOTSPARK1: {
        /* Frames 27 and above retry this test in D3D.EXE. */
pistol_frame:
        if (ps[snum].kickback_pic < 5) {
            short pistol_frames[5] = { 0, 1, 2, 0, 0 };

            pistol_x = 183 + (weapon_xoffset - xvel);
            if (ps[snum].kickback_pic == 1)
                gs = 0;
            if (ps[snum].kickback_pic == 2) {
                gs = 4;
                pistol_x -= 3;
            }
            myos((syncangvel[snum] >> 3) + (short)pistol_x - (ps[snum].look_ang >> 1),
                looking_arc + 240 - gun_pos - (ps[snum].horiz >> 4), 1150 + pistol_frames[ps[snum].kickback_pic], gs, 3);
        } else if (ps[snum].kickback_pic < 10)
            myos((syncangvel[snum] >> 3) + 190 - (ps[snum].look_ang >> 1),
                looking_arc + 230 - gun_pos - (ps[snum].horiz >> 4), 1160, gs, 3);
        else if (ps[snum].kickback_pic < 15) {
            if (ps[snum].kickback_pic == 10)
                spritesound(34, ps[snum].i);
            if (ps[snum].kickback_pic == 13)
                spritesound(33, ps[snum].i);
            myos((syncangvel[snum] >> 3) + 240 - 8 * ps[snum].kickback_pic - (ps[snum].look_ang >> 1),
                looking_arc + 130 - gun_pos - (ps[snum].horiz >> 4) + 16 * ps[snum].kickback_pic, 1162, gs, 3);
            myos((syncangvel[snum] >> 3) + 220 - (ps[snum].look_ang >> 1),
                looking_arc + 220 - gun_pos - (ps[snum].horiz >> 4), 1161, gs, 3);
        } else if (ps[snum].kickback_pic < 20) {
            myos((syncangvel[snum] >> 3) + 120 + 2 * ps[snum].kickback_pic - (ps[snum].look_ang >> 1),
                looking_arc + 430 - gun_pos - (ps[snum].horiz >> 4) - 8 * ps[snum].kickback_pic, 1163, gs, 3);
            myos((syncangvel[snum] >> 3) + 220 - (ps[snum].look_ang >> 1),
                looking_arc + 220 - gun_pos - (ps[snum].horiz >> 4), 1161, gs, 3);
        } else if (ps[snum].kickback_pic < 23) {
            myos((syncangvel[snum] >> 3) + 180 - (ps[snum].look_ang >> 1),
                looking_arc + 235 - gun_pos - (ps[snum].horiz >> 4), 1164, gs, 3);
            myos((syncangvel[snum] >> 3) + 220 - (ps[snum].look_ang >> 1),
                looking_arc + 210 - gun_pos - (ps[snum].horiz >> 4), 1161, gs, 3);
        } else if (ps[snum].kickback_pic < 25) {
            myos((syncangvel[snum] >> 3) + 160 - (ps[snum].look_ang >> 1),
                looking_arc + 245 - gun_pos - (ps[snum].horiz >> 4), 1164, gs, 3);
            myos((syncangvel[snum] >> 3) + 220 - (ps[snum].look_ang >> 1),
                looking_arc + 220 - gun_pos - (ps[snum].horiz >> 4), 1161, gs, 3);
        } else if (ps[snum].kickback_pic < 27)
            myos((syncangvel[snum] >> 3) + 190 - (ps[snum].look_ang >> 1),
                looking_arc + 235 - gun_pos - (ps[snum].horiz >> 4), 1160, gs, 3);
        else
            goto pistol_frame;
        break;
    }
    case HEAVYHBOMB: {
        if (ps[snum].kickback_pic) {
            unsigned char frames[21] = {
                0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
            };

            if (ps[snum].kickback_pic < 7)
                gun_pos -= 10 * ps[snum].kickback_pic;
            else if (ps[snum].kickback_pic < 12)
                gun_pos += 20 * (ps[snum].kickback_pic - 10);
            else if (ps[snum].kickback_pic < 20)
                gun_pos -= 9 * (ps[snum].kickback_pic - 14);
            myos(weapon_xoffset + 190 + (syncangvel[snum] >> 4) - (ps[snum].look_ang >> 1),
                looking_arc + 250 - gun_pos - (ps[snum].horiz >> 4), 1143 + frames[ps[snum].kickback_pic], gs, 3);
        } else
            myos(weapon_xoffset + 190 + (syncangvel[snum] >> 4) - (ps[snum].look_ang >> 1),
                looking_arc + 260 - gun_pos - (ps[snum].horiz >> 4), 1143, gs, 3);
        break;
    }
    case HANDREMOTE: {
        signed char frames[11] = { 0, 1, 1, 2, 1, 1, 0, 0, 0, 0, 0 };

        weapon_xoffset = -32;
        if (ps[snum].kickback_pic > 4)
            gun_pos -= 12 * (ps[snum].kickback_pic - 4);
        if (ps[snum].kickback_pic != 0) {
            tile = HANDREMOTE + frames[ps[snum].kickback_pic];
            draw_x = (weapon_xoffset + 150) + (syncangvel[snum] >> 4) - (ps[snum].look_ang >> 1);
            draw_y = looking_arc + 258 - gun_pos - (ps[snum].horiz >> 4);
            goto draw_overlay;
        } else {
            draw_x = (weapon_xoffset + 150) + (syncangvel[snum] >> 4) - (ps[snum].look_ang >> 1);
            draw_y = looking_arc + 258 - gun_pos - (ps[snum].horiz >> 4);
            tile = HANDREMOTE;
            goto draw_overlay;
        }
    }
    }
    return;

draw_overlay:
    myos(draw_x, draw_y, tile, gs, 3);
}

/* Sample local input into the sync slot snum (Ken getinput; bit layout survives into Duke 1.5). */
void getinput(short snum)
{
    short i;

    if (snum < 16) {
        i = (short)(totalclock - lockclock);
        if (i > 255)
            i = 255;
        synctics = i;
    }
    if (ps[myconnectindex].gm & MODE_MENU) {
        vel = 0;
        syncbits[myconnectindex] = 0;
        syncvel[snum] = 0;
        syncsvel[snum] = 0;
        syncangvel[snum] = 0;
        angvel = svel = 0;
        return;
    }
    if (moustat == 1) {
        getmousevalues(&menu_x, &menu_y, &menu_drag);
        angvel += (menu_x * (ud.mouse_sensitivity + 129)) >> 7;
        vel -= (menu_y * (ud.mouse_sensitivity + 129)) >> 7;
        if (angvel < -128)
            angvel = -128;
        else if (angvel > 127)
            angvel = 127;
        if (vel < -128)
            vel = -128;
        else if (vel > 127)
            vel = 127;
    }
    syncvel[snum] = vel;
    syncsvel[snum] = svel;
    syncangvel[snum] = angvel;

    syncbits[snum] = keystatus[0x1e];
    syncbits[snum] += (unsigned long)keystatus[0x2c] << 1;
    syncbits[snum] += (unsigned long)keystatus[0x1d] << 2;
    syncbits[snum] += (unsigned long)keystatus[0x39] << 4;
    syncbits[snum] += (unsigned long)(keystatus[0x2a] | keystatus[0x36]) << 5;
    syncbits[snum] += (unsigned long)keystatus[0x52] << 6;
    syncbits[snum] += (unsigned long)keystatus[0x53] << 7;
    syncbits[snum] += (unsigned long)keystatus[0x02] << 8;
    syncbits[snum] += (unsigned long)keystatus[0x03] << 9;
    syncbits[snum] += (unsigned long)keystatus[0x04] << 10;
    syncbits[snum] += (unsigned long)keystatus[0x05] << 11;
    syncbits[snum] += (unsigned long)keystatus[0x06] << 12;
    syncbits[snum] += (unsigned long)(keystatus[0xc9] | keystatus[0x47]) << 13;
    syncbits[snum] += (unsigned long)(keystatus[0xd1] | keystatus[0x4f]) << 14;
    syncbits[snum] += (unsigned long)keystatus[0x07] << 15;
    syncbits[snum] += (unsigned long)keystatus[0x2e] << 16;
    syncbits[snum] += (unsigned long)keystatus[0x17] << 17;
    syncbits[snum] += (unsigned long)keystatus[0x4c] << 18;
    /* This command samples keypad plus twice in the original. */
    syncbits[snum] += (unsigned long)(keystatus[0x4e] | keystatus[0x4e]) << 19;
    syncbits[snum] += (unsigned long)(keystatus[0x4a] | keystatus[0x0c]) << 20;
    syncbits[snum] += (unsigned long)keystatus[0x19] << 21;
    syncbits[snum] += (unsigned long)keystatus[0x0f] << 22;
    syncbits[snum] += (unsigned long)keystatus[0x16] << 23;
    syncbits[snum] += (unsigned long)keystatus[0x3b] << 24;
    syncbits[snum] += (unsigned long)keystatus[0x3c] << 25;
    syncbits[snum] += (unsigned long)keystatus[0x3d] << 26;
    syncbits[snum] += (unsigned long)keystatus[0x3e] << 27;
    syncbits[snum] += (unsigned long)keystatus[0x3f] << 28;
    syncbits[snum] += (unsigned long)keystatus[0x40] << 29;
    syncbits[snum] += (unsigned long)(keystatus[0x1c] | keystatus[0x9c]) << 30;
    syncbits[snum] += (unsigned long)keystatus[0x01] << 31;
}

/* Air and scuba handling for player snum in underwater (lotag 2) sectors. */
void update_air(short snum)
{
    if ((sector[ps[snum].cursectnum].lotag & 32767) == 2) {
        if ((krand() >> 8) >= 239)
            spritesound(10, ps[snum].i);
        if (ps[snum].scuba_on == 0) {
            if (ps[snum].airleft > 0)
                ps[snum].airleft -= 32;
            else if (ud.debug == 0 && ps[snum].shield_amount > 0)
                ps[snum].shield_amount -= 4;
            else if (ps[snum].health > 0)
                ps[snum].health--;
            else
                ps[snum].health = 0;
            FX_SetVolume(ud.sound_volume);
        } else {
            FX_SetVolume(ud.sound_volume - 32);
            if (--ps[snum].airleft < 1) {
                ps[snum].scuba_on = 0;
                ps[snum].airleft = 1728;
            }
        }
    } else {
        FX_SetVolume(ud.sound_volume);
        if (ps[snum].scuba_on == 0)
            ps[snum].airleft = 1728;
    }
}

/* Run one tick of player snum from syncvel/syncsvel/syncangvel/syncbits: movement, physics, view and weapon. */
void processinput(short snum)
{
    /* COMPILER_STEERED: local order for Watcom's allocation and stack homes. */
    long fz;
    long yvect;
    long xvect;
    long doubvel;
    long cz;
    short psectlotag;
    long wpos;
    short i;
    long lz;
    long hz;
    long tics2;
    long eye;

    if (ps[snum].health <= 0) {
        if ((sector[ps[snum].cursectnum].lotag & 0x7FFF) == 2) {
            if (klabs(ps[snum].posz - sector[ps[snum].cursectnum].floorz) > 4608)
                ps[snum].posz += 348;
            updatesector(ps[snum].posx, ps[snum].posy, &ps[snum].cursectnum);
            clipmove(&ps[snum].posx, &ps[snum].posy, &ps[snum].posz, &ps[snum].cursectnum, 0, 0, 144, 1024, 1024, 0);
            setsprite(ps[snum].i, ps[snum].posx, ps[snum].posy, ps[snum].posz);
        }
        return;
    }
    if (ps[snum].newowner >= 0) {
        i = ps[snum].newowner;
        ps[snum].posx = sprite[i].x;
        ps[snum].posy = sprite[i].y;
        ps[snum].posz = sprite[i].z;
        ps[snum].ang = sprite[i].ang;
        return;
    }
    if (ps[snum].fta[ps[snum].ftq] > 1)
        ps[snum].fta[ps[snum].ftq]--;
    doubvel = synctics;
    if (syncbits[snum] & SB_LOOK_LEFT)
        ps[snum].look_ang -= 192;
    else if (syncbits[snum] & SB_LOOK_RIGHT)
        ps[snum].look_ang += 192;
    ps[snum].look_ang -= ps[snum].look_ang / 3;
    if (ps[snum].on_crane >= 0)
        goto domove;

    /* COMPILER_STEERED: compute the run step early to get a stack home for it. */
    tics2 = doubvel + doubvel;
    getzrange(ps[snum].posx, ps[snum].posy, ps[snum].posz, ps[snum].cursectnum, &cz, &hz, &fz, &lz, 128, 0);
    if (ps[snum].jetpack_on == 0) {
        int dx = ps[snum].oposx - ps[snum].posx, dy = ps[snum].oposy - ps[snum].posy;

        ps[snum].bobcounter += ksqrt(dx * dx + dy * dy);
    }
    ps[snum].oposx = ps[snum].posx;
    ps[snum].oposy = ps[snum].posy;
    ps[snum].oposz = ps[snum].posz;
    ps[snum].oang = ps[snum].ang;
    if (ps[snum].steroids_on)
        ps[snum].steroids_on--;

    update_air(snum);

    psectlotag = sector[ps[snum].cursectnum].lotag & 0x7FFF;
    if (psectlotag == 2) {
        if (syncvel[snum] < 0)
            syncvel[snum] >>= 2;
        ps[snum].jumping_counter = 0;
        if (syncbits[snum] & SB_JUMP) {
            if (ps[snum].poszv > -2048)
                ps[snum].poszv -= 348;
            else
                ps[snum].poszv = -2048;
        } else if (syncbits[snum] & SB_CROUCH) {
            if (ps[snum].poszv < 2048)
                ps[snum].poszv += 348;
            else
                ps[snum].poszv = 2048;
        } else {
            ps[snum].bobcounter += 32;
            ps[snum].poszv += sintable[(2 * ps[snum].bobcounter) & 2047] >> 8;
            ps[snum].poszv >>= 1;
        }
        ps[snum].posz += ps[snum].poszv;
        if (fz - 3840 < ps[snum].posz)
            ps[snum].posz += (fz - 3840 - ps[snum].posz) >> 1;
        if (cz + 1024 > ps[snum].posz) {
            ps[snum].posz = cz + 1024;
            ps[snum].poszv = 0;
        }
        goto doturn;
    }
    if (ps[snum].jetpack_on) {
        ps[snum].on_ground = 0;
        ps[snum].jumping_counter = 0;
        if (syncbits[snum] & SB_RUN)
            doubvel = tics2;
        if (ps[snum].jetpack_on < 11) {
            if (ps[snum].jetpack_on == 10)
                spritesound(19, ps[snum].i);
            ps[snum].posz -= ++ps[snum].jetpack_on << 7;
        }
        if (syncbits[snum] & SB_JUMP)
            ps[snum].posz -= 2048;
        if (syncbits[snum] & SB_CROUCH)
            ps[snum].posz += 3096;
        if (syncvel[snum] > 0)
            ps[snum].horiz -= 16;
        if (syncvel[snum] < 0)
            ps[snum].horiz += 16;
        if (fz - 4096 < ps[snum].posz)
            ps[snum].posz += (fz - 4096 - ps[snum].posz) >> 1;
        if (cz + 4096 > ps[snum].posz)
            ps[snum].posz += (cz + 4096 - ps[snum].posz) >> 1;
        goto doturn;
    }
    if (psectlotag == 2)
        goto doturn;
    stopsound(19);
    if ((sector[ps[snum].cursectnum].lotag & 0x7FFF) == 1) {
        short s;

        ps[snum].bobcounter += 128;
        s = sintable[(ps[snum].bobcounter >> 1) & 2047];
        eye = 34;
        ps[snum].posz += klabs(s) >> 6;
        if (ps[snum].on_ground == 0 && (syncbits[snum] & SB_RUN))
            doubvel = tics2;
    } else {
        eye = 40;
        if (!(syncbits[snum] & SB_CROUCH) && (syncbits[snum] & SB_RUN))
            doubvel = tics2;
    }
    if (fz - (eye << 8) > ps[snum].posz) {
        ps[snum].on_ground = 0;
        if (ps[snum].poszv < 6144)
            ps[snum].poszv += 320;
        ps[snum].bobcounter = 0;
    } else {
        if (ps[snum].on_ground == 0 && ps[snum].poszv > 3072) {
            spritesound(9, ps[snum].i);
            ps[snum].hard_landing = (unsigned char)(ps[snum].poszv >> 10);
        }
        ps[snum].on_ground = 1;
        if (eye == 40) {
            ps[snum].posz += (fz - (eye << 8) - ps[snum].posz) >> 1;
            if (ps[snum].poszv > 0)
                ps[snum].poszv -= 512;
            else
                ps[snum].poszv = 0;
        } else {
            ps[snum].posz += (fz - (eye << 8) - ps[snum].posz) >> 2;
            if (ps[snum].poszv > 0)
                ps[snum].poszv -= 64;
            else
                ps[snum].poszv = 0;
        }
        if ((syncbits[snum] & SB_JUMP) && ps[snum].jumping_counter == 0 && fz - cz > 17408) {
            ps[snum].jumping_counter = 348;
            spritesound(8, ps[snum].i);
        }
        if ((syncbits[snum] & SB_CROUCH) && ps[snum].on_ground) {
            if (eye == 40)
                ps[snum].posz += 3072;
            else
                ps[snum].posz += 2048;
        }
    }
    if (ps[snum].jumping_counter) {
        short jump;

        /* COMPILER_STEERED: the oracle reloads the counter after this store. */
        ps[snum].bobcounter = 1024;
        jump = ps[snum].jumping_counter;
        if (jump < 1024) {
            ps[snum].on_ground = 0;
            ps[snum].poszv -= sintable[jump] >> 5;
            ps[snum].jumping_counter += 92;
        } else {
            ps[snum].jumping_counter = 0;
            ps[snum].poszv = 0;
        }
    }
    if (cz + 4096 > ps[snum].posz) {
        ps[snum].posz = cz + 4096;
        ps[snum].poszv = 512;
        ps[snum].jumping_counter = 0;
    }
    ps[snum].posz += ps[snum].poszv;

doturn:
    if (syncangvel[snum]) {
        ps[snum].ang += doubvel * syncangvel[snum] / 24;
        ps[snum].ang = (ps[snum].ang + 2048) & 2047;
    }
    if (klabs(cz - fz) < 7168) {
        struct sectortype *sec = &sector[ps[snum].cursectnum];
        psectlotag = sec->lotag & 0x7FFF;
        if (psectlotag > 2 && psectlotag != 23) {
            sec->ceilingz -= 1024;
            if (ud.debug == 0 && ps[snum].shield_amount > 0)
                ps[snum].shield_amount -= 4;
            else if (ps[snum].health > 0)
                ps[snum].health -= 5;
            else
                ps[snum].health = 0;
        }
    }
    xvect = yvect = 0;
    if (syncsvel[snum] | syncvel[snum]) {
        if (sintable[ps[snum].bobcounter & 2047] >> 12 == 3) {
            if (ps[snum].walking_snd_toggle == 0 && ps[snum].on_ground) {
                switch (sector[ps[snum].cursectnum].lotag & 0x7FFF) {
                case 1:
                    spritesound(28, ps[snum].i);
                    ps[snum].walking_snd_toggle = 1;
                    break;
                case 2:
                    break;
                default:
                    spritesound(2, ps[snum].i);
                    ps[snum].walking_snd_toggle = 1;
                    break;
                }
            }
        } else
            ps[snum].walking_snd_toggle = 0;
        if (ps[snum].hard_landing)
            doubvel = 0;
        if (syncvel[snum]) {
            xvect += (doubvel * syncvel[snum] * sintable[(ps[snum].ang + 2560) & 2047]) >> 3;
            yvect += (doubvel * syncvel[snum] * sintable[(ps[snum].ang + 2048) & 2047]) >> 3;
        }
        if (syncsvel[snum]) {
            xvect += (doubvel * syncsvel[snum] * sintable[(ps[snum].ang + 2048) & 2047]) >> 3;
            yvect += (doubvel * syncsvel[snum] * sintable[(ps[snum].ang + 1536) & 2047]) >> 3;
        }
        if (syncbits[snum] & SB_CROUCH) {
            xvect >>= 1;
            yvect >>= 1;
        }
        ps[snum].horiz += sintable[(ps[snum].bobcounter + 1024) & 2047] >> 13;
        if (ps[snum].horiz > 50 && ps[snum].horiz < 150)
            ps[snum].horiz += 32 - ps[snum].horiz / 3;
    }

domove:
    updatesector(ps[snum].posx, ps[snum].posy, &ps[snum].cursectnum);
    clipmove(&ps[snum].posx, &ps[snum].posy, &ps[snum].posz, &ps[snum].cursectnum, xvect, yvect, 144, 1024, 1024, 0);
    setsprite(ps[snum].i, ps[snum].posx, ps[snum].posy, ps[snum].posz);
    if ((syncbits[snum] & SB_CENTER_VIEW) || ps[snum].jetpack_on || ps[snum].current_weapon == RPG || ps[snum].hard_landing) {
        ps[snum].horiz += 32 - ps[snum].horiz / 3;
        if (ps[snum].current_weapon == RPG || ps[snum].jetpack_on) {
            if (syncbits[snum] & SB_LOOK_UP)
                ps[snum].horiz += 64;
            else if (syncbits[snum] & SB_LOOK_DOWN)
                ps[snum].horiz -= 64;
        }
    } else if (syncbits[snum] & SB_LOOK_UP) {
        ps[snum].horiz += 48;
    } else if (syncbits[snum] & SB_LOOK_DOWN) {
        ps[snum].horiz -= 48;
    }
    if (ps[snum].hard_landing > 0) {
        ps[snum].hard_landing--;
        ps[snum].horiz -= 13 * ps[snum].hard_landing;
    }
    if (ps[snum].horiz > 199)
        ps[snum].horiz = 199;
    else if (ps[snum].horiz < 0)
        ps[snum].horiz = 0;

    if (ps[snum].knuckle_incs && lockclock - ps[snum].weapon_clock > 5) {
        ps[snum].weapon_clock = (unsigned char)lockclock;
        ps[snum].knuckle_incs++;
        if (ps[snum].knuckle_incs == 12 || (syncbits[snum] & SB_FIRE)) {
            ps[snum].knuckle_incs = 0;
            ps[snum].weapon_pos = 0x81;
        }
        return;
    }
    wpos = ps[snum].weapon_pos;
    if ((wpos & 0x7F) > 2 && ps[snum].current_weapon == SHOTSPARK1 && ps[snum].kickback_pic < 5) {
        spawn_pistol_tracer(snum);
    }
    if ((wpos & 0x7F) && (wpos & 0x7F) != 6) {
        if (wpos & 0x80)
            ps[snum].weapon_pos++;
        else
            ps[snum].weapon_pos--;
    } else if (ps[snum].last_weapon >= 0) {
        ps[snum].weapon_pos = 0x81;
        ps[snum].last_weapon = -1;
    }

    if ((syncbits[snum] & SB_FIRE) && ps[snum].kickback_pic == 0 && ps[snum].last_weapon == -1) {
        if ((ps[snum].weapon_pos & 0x7F) != 6)
            ps[snum].weapon_pos = 0x81;
        switch (ps[snum].current_weapon) {
        case SHOTSPARK1: {
            short ammo = ps[snum].ammo_amount;

            if (ammo > 0) {
                ps[snum].kickback_pic = 1;
                ps[snum].ammo_amount = ammo - 1;
            }
            break;
        }
        case BULLET:
            if (ps[snum].ammo_bat_amount > 0) {
                ps[snum].kickback_pic = 2;
                ps[snum].ammo_bat_amount--;
            }
            break;
        case RPG:
            ps[snum].kickback_pic = 1;
            break;
        case CLUB:
            if (ps[snum].current_weapon == CLUB) {
                if (ps[snum].ammo_bat_amount)
                    ps[snum].knee_sequence = 5;
                else
                    ps[snum].knee_sequence = krand() % 5;
            }
        case HEAVYHBOMB:
        case HANDREMOTE:
            ps[snum].kickback_pic = 1;
            break;
        }
        if (ps[snum].kickback_pic)
            ps[snum].weapon_clock = (unsigned char)lockclock;
        return;
    }
    if (ps[snum].kickback_pic == 0 || lockclock - ps[snum].weapon_clock <= 7)
        return;
    ps[snum].weapon_clock = (unsigned char)lockclock;
    switch (ps[snum].current_weapon) {
    case HEAVYHBOMB:
        if (++ps[snum].kickback_pic == 11) {
            if (ps[snum].ammo_h_amount > 0)
                ps[snum].ammo_h_amount--;
            EGS(ps[snum].cursectnum, (sintable[(ps[snum].ang + 512) & 2047] >> 4) + ps[snum].posx,
                (sintable[ps[snum].ang & 2047] >> 4) + ps[snum].posy, ps[snum].posz + 1024, 1, HEAVYHBOMB, -16, 16, 16,
                ps[snum].ang, 128, -348, ps[snum].i, 1);
            ps[snum].hbomb_on = 1;
        }
        if (ps[snum].kickback_pic > 19) {
            ps[snum].current_weapon = HANDREMOTE;
            ps[snum].kickback_pic = 0;
        }
        break;
    case HANDREMOTE:
        if (++ps[snum].kickback_pic == 2)
            ps[snum].hbomb_on = 0;
        if (ps[snum].kickback_pic == 10) {
            ps[snum].kickback_pic = 0;
            if (ps[snum].ammo_h_amount)
                ps[snum].current_weapon = HEAVYHBOMB;
            else
                addweapon(snum, SHOTSPARK1, 0);
        }
        break;
    case SHOTSPARK1:
        if (ps[snum].kickback_pic == 1) {
            shoot(ps[snum].i, SHOTSPARK1, 0);
        } else if (ps[snum].kickback_pic == 2)
            spawn(ps[snum].i, 1165);
        ps[snum].kickback_pic++;
        if (ps[snum].kickback_pic >= 5 && (ps[snum].ammo_amount % 12 != 0 || ps[snum].ammo_amount == 0))
            ps[snum].kickback_pic = 0;
        if (ps[snum].kickback_pic == 27)
            ps[snum].kickback_pic = 0;
        break;
    case BULLET:
        ps[snum].kickback_pic++;
        if (ps[snum].kickback_pic >= 6)
            ps[snum].kickback_pic = 0;
        if (ps[snum].kickback_pic == 3) {
            shoot(ps[snum].i, BULLET, 0);
        }
        break;
    case CLUB: {
        unsigned char frame;

        ps[snum].kickback_pic++;
        frame = clubframes[ps[snum].knee_sequence][ps[snum].kickback_pic];
        switch (frame) {
        case 255:
            ps[snum].kickback_pic = 0;
            break;
        case 3:
            if (ps[snum].kickback_pic == 4 && ps[snum].ammo_bat_amount && (syncbits[snum] & SB_FIRE)
                && (sector[ps[snum].cursectnum].lotag & 0x7FFF) != 2) {
                shoot(ps[snum].i, CLUB, 0);
                spritesound(24, ps[snum].i);
                ps[snum].ammo_bat_amount--;
                ps[snum].kickback_pic--;
                ps[snum].knee_flash++;
            }
            break;
        case 4:
            shoot(ps[snum].i, CLUB, 0);
            break;
        }
        break;
    }
    case RPG:
        if (ps[snum].ammo_r_amount > 0 && (syncbits[snum] & SB_FIRE) && ps[snum].kickback_pic == 10) {
            ps[snum].kickback_pic = 11;
            ps[snum].ammo_r_amount--;
            shoot(ps[snum].i, RPG, 0);
            ps[snum].horiz += 64;
        }
        if (ps[snum].kickback_pic > 10) {
            if (ps[snum].kickback_pic < 18)
                ps[snum].kickback_pic++;
            else
                ps[snum].kickback_pic = 10;
        } else if (ps[snum].kickback_pic != 0 && ps[snum].kickback_pic < 10)
            ps[snum].kickback_pic++;
        break;
    }
}
