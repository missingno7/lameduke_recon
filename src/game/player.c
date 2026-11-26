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
#define user_visibility userdefs.visibility
#define mouse_sensitivity userdefs.mouse_sensitivity
#define sound_volume userdefs.sound_volume
#define disable_armor userdefs.debug

void kickview(short p, short i)
{
    short angle;

    if (sprite[ps[p].i].extra / 20 >= ps[p].health) {
        angle = getincangle_shift(sprite[i].ang,
            getangle(sprite[i].x - ps[p].posx, sprite[i].y - ps[p].posy), 0);
        if (angle < -512 || angle >= 512)
            ps[p].horiz -= 4;
        else
            ps[p].horiz += 4;
        if (angle < 0)
            ps[p].lookang = -(sprite[i].extra << 1);
        else if (angle >= 0)
            ps[p].lookang = sprite[i].extra << 1;
    }
}

void spawn_pistol_tracer(short p)
{
    short hitsprite, hitwall, hitsect, i;
    short ang;
    long hitz, hity, hitx, size;
    long weapon_pos;

    weapon_pos = ps[p].weapon_pos & 127;
    ang = ps[p].ang - 16 + ((6 - weapon_pos) << 4);
    hitscan(ps[p].posx + (sintable[(ang + 512) & 2047] >> 8),
        ps[p].posy + (sintable[ang & 2047] >> 8),
        ps[p].posz - (weapon_pos << 10) + 5376, ps[p].cursectnum,
        sintable[(ang + (player_avel[p] >> 3) + 512) & 2047],
        sintable[(ang + (player_avel[p] >> 3)) & 2047],
        (100 - ps[p].horiz) << 11, &hitsect, &hitwall, &hitsprite,
        &hitx, &hity, &hitz);

    ang += 1024;
    i = EGS(hitsect, hitx + (sintable[(ang + 512) & 2047] >> 8),
        hity + (sintable[ang & 2047] >> 8),
        hitz + 1024 + (sintable[ps[p].bobcounter & 2047] >> 8),
        0, LASERSITE, -16, 12, 12, ang, 0, 0, ps[p].i, 5);
    size = (ksqrt((ps[screenpeek].posx - sprite[i].x) * (ps[screenpeek].posx - sprite[i].x)
        + (ps[screenpeek].posy - sprite[i].y) * (ps[screenpeek].posy - sprite[i].y)) >> 8) + 1;
    if (size > 32)
        size = 32;
    sprite[i].xrepeat = sprite[i].yrepeat = size;
    if (sector[sprite[i].sectnum].floorz - 768 < sprite[i].z)
        sprite[i].z -= 768;
}

void shoot(short i, short tile, unsigned char move_to_hit)
{
    long zvel;
    long y;
    short sect;
    long z;
    long x;
    long distance;
    short p;
    short angle;
    short hitsprite;
    short j;
    short hitwall;

    if (move_to_hit) {
        hitscan(sprite[i].x, sprite[i].y, sprite[i].z, sprite[i].sectnum,
            sintable[(sprite[i].ang + 512) & 2047], sintable[sprite[i].ang & 2047], 0,
            &sect, &hitwall, &hitsprite, &x, &y, &z);
        if (ksqrt((sprite[i].x - x) * (sprite[i].x - x)
            + (sprite[i].y - y) * (sprite[i].y - y)) > 512)
            return;
        if (sector[sprite[i].sectnum].ceilingz + 2560 <= sprite[i].z
            || checkhitceiling(sprite[i].sectnum) == 0) {
            setsprite(i, x, y, z);
            if (hitwall >= 0) {
                checkhitswitch(hitwall);
                checkhitwall(hitwall, x, y, z, tile);
            }
        }
        return;
    }

    if (sprite[i].picnum == APLAYER) {
        p = sprite[i].yvel;
        x = ps[p].posx + (player_avel[p] >> 4);
        y = ps[p].posy + (player_avel[p] >> 4);
        z = ps[p].posz + 3584;
        angle = ps[p].ang - 8;
        if (tile == RPG)
            angle += ps[p].lookang;
        p = sprite[i].yvel;
        sect = ps[p].cursectnum;
        ps[p].muzzle_flash = 1;
        switch (tile) {
        case SHOTSPARK1:
            zvel = (100 - ps[p].horiz) * 31;
            break;
        case RPG:
            zvel = (100 - ps[p].horiz) * 15;
            z -= 1536;
            break;
        default:
            zvel = (100 - ps[sprite[i].yvel].horiz) * 22;
            break;
        }
        if (ps[sprite[i].yvel].flash_sprite)
            sprite[ps[sprite[i].yvel].flash_sprite].shade = 0;
    } else {
        x = sprite[i].x;
        y = sprite[i].y;
        z = sprite[i].z - 8192;
        angle = sprite[i].ang;
        p = findplayer(i, &distance);
        j = ps[p].i;
        zvel = ((ps[p].posz - sprite[i].z) << 7)
            / (klabs(sprite[j].x - sprite[i].x) + klabs(sprite[j].y - sprite[i].y));
        sect = sprite[i].sectnum;
    }

    switch ((long)(unsigned short)tile) {
    case CLUB:
        x += sintable[(angle + 544) & 2047] >> 7;
        y += sintable[(angle + 32) & 2047] >> 7;
        z -= 2048;
        EGS(sect, x, y, z, 0, 46, -16, 0, 0, angle, 32, 0, i, 4);
        if (actorscrptr[CLUB])
            sprite[i].extra = *actorscrptr[CLUB];
        break;
    case SHOTSPARK1:
        angle -= 8;
        hitscan(x, y, z, sect, sintable[(angle + 512) & 2047],
            sintable[angle & 2047], zvel << 6, &sect, &hitwall, &hitsprite, &x, &y, &z);
        spritesound(31, i);
        if (sector[sect].ceilingz + 1024 > z)
            z = sector[sect].ceilingz + 2048;
        if (sprite[i].picnum == APLAYER) {
            j = EGS(sect, x, y, z - 2816, 0, OTHERSHOTSPARK, -16, 10, 10, angle, 64, zvel, i, 4);
            sprite[j].yvel = hitwall;
        } else
            EGS(sect, x, y, z - 2304, 0, SHOTSPARK1, -16, 16, 16, angle, 64, zvel, i, 4);
        break;
    case BULLET:
        spritesound(1, i);
        spritesound(24, i);
        EGS(sect, x + (sintable[(angle + 512) & 2047] >> 6),
            y + (sintable[angle & 2047] >> 6), z - 2048,
            1, BULLET, -127, 12, 12, angle, 0, zvel, i, 4);
        break;
    case RPG:
        spritesound(38, i);
        EGS(sect, x + (sintable[(angle + 640) & 2047] >> 5),
            y + (sintable[(angle + 128) & 2047] >> 5), z - 2304,
            65, RPG, -16, 16, 16, angle, 0, zvel, i, 4);
        break;
    }
}

unsigned char displayraise(short shade, short p)
{
    short frames[11] = { 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0 };
    short bob, tile;

    if (!ps[p].raise_frame || ps[p].health <= 0)
        return 0;

    tile = frames[ps[p].raise_frame - 1];
    bob = klabs(ps[p].lookang) / 9;
    draw_weapon_tile((player_avel[p] >> 4) + 160 - (ps[p].lookang >> 1),
        bob + 180 - (ps[p].horiz >> 4), tile + 681, shade, 3);
    if (ps[p].raise_frame == 5)
        spritesound(7, ps[p].i);
    return 1;
}
signed char knee_frames[6][20] = {
    { 0, 0, 1, 2, 3, 4, 4, 3, 2, -1 },
    { 0, 0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, 1, 0, -1 },
    { 0, 0, 1, 2, 3, 4, 3, 2, -1 },
    { 0, 0, 2, 3, 4, 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, -1 },
    { 0, 0, 2, 4, 6, 8, 7, 5, 4, 3, 1, -1 },
    { 0, 0, 1, 2, 3, 4, 3, 1, 0, -1 }
};

void displayweapon(short p)
{
    long shade, x, xvel;
    long draw_x, draw_y, tile;
    short bob;
    unsigned char y, weapon_pos;
    unsigned short weapon;

    if (ps[p].health <= 0)
        return;
    visibility = user_visibility;
    bob = klabs(ps[p].lookang) / 9;
    xvel = (player_bits[p] & 32) ? player_avel[p] >> 3 : 0;
    shade = sector[ps[p].cursectnum].floorshade;
    if (shade < 0)
        shade = 0;
    else if (shade > 24)
        shade = 24;
    if (displayraise(shade, p))
        return;
    weapon_pos = ps[p].weapon_pos & 127;
    if (weapon_pos < 2)
        return;
    y = (weapon_pos << 4) - 32;
    y -= sintable[ps[p].bobcounter & 2047] >> 12;
    x = xvel + 12 + ((6 - weapon_pos) << 4);
    weapon = ps[p].last_weapon >= 0 ? ps[p].last_weapon : ps[p].curr_weapon;

    switch (weapon) {
    case CLUB: {
        unsigned char frame = knee_frames[ps[p].knee_sequence][ps[p].kickback_pic];

        switch (frame) {
        case 0:
        case 1: break;
        case 2: x -= 10; break;
        case 3: x -= 20; break;
        case 4: x -= 30; break;
        case 5:
        case 6: x -= 40; break;
        case 7: x -= 60; break;
        case 8: x -= 80; break;
        case 9: x -= 100; break;
        case 10: x -= 130; break;
        }

        if (ps[p].kickback_pic) {
            draw_weapon_tile((player_avel[p] >> 4) + x + 180 - (ps[p].lookang >> 1),
                bob + 280 - y - (ps[p].horiz >> 4),
                CLUB + knee_frames[ps[p].knee_sequence][ps[p].kickback_pic], shade, 3);
            if (knee_frames[ps[p].knee_sequence][ps[p].kickback_pic] == 2) {
                if (ps[p].batteryammo && (sector[ps[p].cursectnum].lotag & 32767) != 2) {
                    draw_x = (player_avel[p] >> 4) + x + 229 - (ps[p].lookang >> 1);
                    draw_y = bob + 207 - y - (ps[p].horiz >> 4);
                    tile = 1240 + ps[p].knee_flash % 3;
                    goto draw_overlay;
                }
                else
                    draw_weapon_tile((player_avel[p] >> 4) + x + 229 - (ps[p].lookang >> 1),
                        bob + 207 - y - (ps[p].horiz >> 4), 1239, shade, 3);
            }
        } else
            draw_weapon_tile((player_avel[p] >> 4) + x + 180 - (ps[p].lookang >> 1),
                bob + 280 - y - (ps[p].horiz >> 4), 1231, shade, 3);
        break;
    }
    case RPG:
        if (ps[p].kickback_pic > 11) {
            unsigned char recoil[9] = { 40, 30, 22, 16, 12, 16, 22, 30, 30 };

            if (ps[p].kickback_pic <= 13)
                shade = ps[p].kickback_pic - 12;
            draw_weapon_tile((player_avel[p] >> 5) + x - recoil[ps[p].kickback_pic - 11] + 270,
                (bob << 1) + 186 - y - (ps[p].horiz >> 4), 1252, shade, 3);
        } else {
            x += sintable[(1024 - (ps[p].kickback_pic << 7)) & 2047] >> 9;
            y += sintable[(1536 - (ps[p].kickback_pic << 7)) & 2047] >> 10;
            if (ps[p].kickback_pic < 4)
                draw_weapon_tile((player_avel[p] >> 5) + x + 150,
                    (bob << 1) + 224 - y - (ps[p].horiz >> 4), 1250, shade, 3);
            else if (ps[p].kickback_pic < 7)
                draw_weapon_tile((player_avel[p] >> 5) + x + 184,
                    (bob << 1) + 180 - y - (ps[p].horiz >> 4), 1251, shade, 3);
            else
                draw_weapon_tile((player_avel[p] >> 5) + x + 260,
                    (bob << 1) + 196 - y - (ps[p].horiz >> 4), 1252, shade, 3);
        }
        break;
    case BULLET: {
        /* Watcom emits this initializer in the code prefix even when the
           body selects the same tiles directly. */
        short frames[5] = { 882, 883, 884, 885, 886 };

        switch (ps[p].kickback_pic) {
        case 0:
            draw_weapon_tile((player_avel[p] >> 4) + x + 160 - (ps[p].lookang >> 1),
                bob + 240 - y - (ps[p].horiz >> 4), 882, shade, 3);
            break;
        case 4:
            shade = 0;
            break;
        case 5:
            visibility++;
            break;
        }
        if (ps[p].kickback_pic != 6 && ps[p].kickback_pic != 0)
            draw_weapon_tile((player_avel[p] >> 4) + x + 160 - (ps[p].lookang >> 1),
                bob + 240 - y - (ps[p].horiz >> 4), 881 + ps[p].kickback_pic, shade, 3);
        break;
    }
    case SHOTSPARK1: {
        struct playerstate *pl = &ps[p];
        if (ps[p].kickback_pic < 5) {
            short pistol[5] = { 0, 1, 2, 0, 0 };

            x += 183 - xvel;
            if (ps[p].kickback_pic == 1)
                shade = 0;
            if (ps[p].kickback_pic == 2) {
                shade = 4;
                x -= 3;
            }
            draw_weapon_tile((player_avel[p] >> 3) + (short)x - (ps[p].lookang >> 1),
                bob + 240 - y - (ps[p].horiz >> 4), 1150 + pistol[ps[p].kickback_pic], shade, 3);
        } else if (ps[p].kickback_pic < 10)
            draw_weapon_tile((player_avel[p] >> 3) + 190 - (ps[p].lookang >> 1),
                bob + 230 - y - (ps[p].horiz >> 4), 1160, shade, 3);
        else if (pl->kickback_pic < 15) {
            if (pl->kickback_pic == 10)
                spritesound(34, pl->i);
            if (pl->kickback_pic == 13)
                spritesound(33, pl->i);
            draw_weapon_tile((player_avel[p] >> 3) + 240 - 8 * pl->kickback_pic - (pl->lookang >> 1),
                bob + 130 + 16 * pl->kickback_pic - y - (pl->horiz >> 4), 1162, shade, 3);
            draw_weapon_tile((player_avel[p] >> 3) + 220 - (pl->lookang >> 1),
                bob + 220 - y - (pl->horiz >> 4), 1161, shade, 3);
        } else if (pl->kickback_pic < 20) {
            draw_weapon_tile((player_avel[p] >> 3) + 120 + 2 * pl->kickback_pic - (pl->lookang >> 1),
                bob + 430 - y - (pl->horiz >> 4) - 8 * pl->kickback_pic, 1163, shade, 3);
            draw_weapon_tile((player_avel[p] >> 3) + 220 - (pl->lookang >> 1),
                bob + 220 - y - (pl->horiz >> 4), 1161, shade, 3);
        } else if (pl->kickback_pic < 23) {
            draw_weapon_tile((player_avel[p] >> 3) + 180 - (pl->lookang >> 1),
                bob + 235 - y - (pl->horiz >> 4), 1164, shade, 3);
            draw_weapon_tile((player_avel[p] >> 3) + 220 - (pl->lookang >> 1),
                bob + 210 - y - (pl->horiz >> 4), 1161, shade, 3);
        } else if (pl->kickback_pic < 25) {
            draw_weapon_tile((player_avel[p] >> 3) + 160 - (pl->lookang >> 1),
                bob + 245 - y - (pl->horiz >> 4), 1164, shade, 3);
            draw_weapon_tile((player_avel[p] >> 3) + 220 - (pl->lookang >> 1),
                bob + 220 - y - (pl->horiz >> 4), 1161, shade, 3);
        } else if (pl->kickback_pic < 27)
            draw_weapon_tile((player_avel[p] >> 3) + 190 - (pl->lookang >> 1),
                bob + 235 - y - (pl->horiz >> 4), 1160, shade, 3);
        break;
    }
    case HEAVYHBOMB:
        if (ps[p].kickback_pic) {
            unsigned char frames[21] = {
                0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
            };

            if (ps[p].kickback_pic < 7)
                y -= 10 * ps[p].kickback_pic;
            else if (ps[p].kickback_pic < 12)
                y += 20 * (ps[p].kickback_pic - 10);
            else if (ps[p].kickback_pic < 20)
                y -= 9 * (ps[p].kickback_pic - 14);
            draw_weapon_tile((player_avel[p] >> 4) + x + 190 - (ps[p].lookang >> 1),
                bob + 250 - y - (ps[p].horiz >> 4), 1143 + frames[ps[p].kickback_pic], shade, 3);
        } else
            draw_weapon_tile((player_avel[p] >> 4) + x + 190 - (ps[p].lookang >> 1),
                bob + 260 - y - (ps[p].horiz >> 4), 1143, shade, 3);
        break;
    case HANDREMOTE: {
        unsigned char frames[11] = { 0, 1, 1, 2, 1, 1, 0, 0, 0, 0, 0 };

        x = -32;
        if (ps[p].kickback_pic > 4)
            y -= 12 * (ps[p].kickback_pic - 4);
        if (ps[p].kickback_pic != 0)
            tile = HANDREMOTE + frames[ps[p].kickback_pic];
        else
            tile = HANDREMOTE;
        draw_x = (player_avel[p] >> 4) + x + 150 - (ps[p].lookang >> 1);
        draw_y = bob + 258 - y - (ps[p].horiz >> 4);
        goto draw_overlay;
    }
    }
    return;

draw_overlay:
    draw_weapon_tile(draw_x, draw_y, tile, shade, 3);
}

void getinput(short p)
{
    short elapsed;

    if (p < 16) {
        elapsed = (short)(totalclock - lockclock);
        if (elapsed > 255)
            elapsed = 255;
        script_tics = elapsed;
    }
    if (ps[myconnectindex].gm & 1) {
        svel = 0;
        player_bits[myconnectindex] = 0;
        player_avel[p] = player_vel[p] = player_svel[p] = 0;
        avel = vel = 0;
        return;
    }
    if (moustat == 1) {
        getmousevalues(&menu_x, &menu_y, &menu_drag);
        avel += (menu_x * (mouse_sensitivity + 129)) >> 7;
        svel -= (menu_y * (mouse_sensitivity + 129)) >> 7;
        if (avel < -128)
            avel = -128;
        else if (avel > 127)
            avel = 127;
        if (svel < -128)
            svel = -128;
        else if (svel > 127)
            svel = 127;
    }
    player_svel[p] = svel;
    player_vel[p] = vel;
    player_avel[p] = avel;

    player_bits[p] = keystatus[0x1e];
    player_bits[p] += (unsigned long)keystatus[0x2c] << 1;
    player_bits[p] += (unsigned long)keystatus[0x1d] << 2;
    player_bits[p] += (unsigned long)keystatus[0x39] << 4;
    player_bits[p] += (unsigned long)(keystatus[0x2a] | keystatus[0x36]) << 5;
    player_bits[p] += (unsigned long)keystatus[0x52] << 6;
    player_bits[p] += (unsigned long)keystatus[0x53] << 7;
    player_bits[p] += (unsigned long)keystatus[0x02] << 8;
    player_bits[p] += (unsigned long)keystatus[0x03] << 9;
    player_bits[p] += (unsigned long)keystatus[0x04] << 10;
    player_bits[p] += (unsigned long)keystatus[0x05] << 11;
    player_bits[p] += (unsigned long)keystatus[0x06] << 12;
    player_bits[p] += (unsigned long)(keystatus[0xc9] | keystatus[0x47]) << 13;
    player_bits[p] += (unsigned long)(keystatus[0xd1] | keystatus[0x4f]) << 14;
    player_bits[p] += (unsigned long)keystatus[0x07] << 15;
    player_bits[p] += (unsigned long)keystatus[0x2e] << 16;
    player_bits[p] += (unsigned long)keystatus[0x17] << 17;
    player_bits[p] += (unsigned long)keystatus[0x4c] << 18;
    /* This command samples keypad plus twice in the original. */
    player_bits[p] += (unsigned long)(keystatus[0x4e] | keystatus[0x4e]) << 19;
    player_bits[p] += (unsigned long)(keystatus[0x4a] | keystatus[0x0c]) << 20;
    player_bits[p] += (unsigned long)keystatus[0x19] << 21;
    player_bits[p] += (unsigned long)keystatus[0x0f] << 22;
    player_bits[p] += (unsigned long)keystatus[0x16] << 23;
    player_bits[p] += (unsigned long)keystatus[0x3b] << 24;
    player_bits[p] += (unsigned long)keystatus[0x3c] << 25;
    player_bits[p] += (unsigned long)keystatus[0x3d] << 26;
    player_bits[p] += (unsigned long)keystatus[0x3e] << 27;
    player_bits[p] += (unsigned long)keystatus[0x3f] << 28;
    player_bits[p] += (unsigned long)keystatus[0x40] << 29;
    player_bits[p] += (unsigned long)(keystatus[0x1c] | keystatus[0x9c]) << 30;
    player_bits[p] += (unsigned long)keystatus[0x01] << 31;
}

void update_air(short p)
{
    if ((sector[ps[p].cursectnum].lotag & 32767) == 2) {
        if ((krand() >> 8) >= 239)
            spritesound(10, ps[p].i);
        if (ps[p].scuba_on == 0) {
            if (ps[p].airleft > 0)
                ps[p].airleft -= 32;
            else if (disable_armor == 0 && ps[p].armor > 0)
                ps[p].armor -= 4;
            else if (ps[p].health > 0)
                ps[p].health--;
            else
                ps[p].health = 0;
            FX_SetVolume(sound_volume);
        } else {
            FX_SetVolume(sound_volume - 32);
            if (--ps[p].airleft < 1) {
                ps[p].scuba_on = 0;
                ps[p].airleft = 1728;
            }
        }
    } else {
        FX_SetVolume(sound_volume);
        if (ps[p].scuba_on == 0)
            ps[p].airleft = 1728;
    }
}

void processinput(short snum)
{
    long tics, tics2, cz, ch, fz, fh, xvect, yvect;
    short lotag;
    long eye;
    unsigned char move_to_hit;
    short i, tile;
    long kick;

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
    i = ps[snum].newowner;
    if (i >= 0) {
        ps[snum].posx = sprite[i].x;
        ps[snum].posy = sprite[i].y;
        ps[snum].posz = sprite[i].z;
        ps[snum].ang = sprite[i].ang;
        return;
    }
    if (ps[snum].weapon_delay[ps[snum].weapon_slot] > 1)
        ps[snum].weapon_delay[ps[snum].weapon_slot]--;
    tics = script_tics;
    if (player_bits[snum] & 0x40)
        ps[snum].lookang -= 192;
    else if (player_bits[snum] & 0x80)
        ps[snum].lookang += 192;
    ps[snum].lookang -= ps[snum].lookang / 3;
    if (ps[snum].state82 >= 0)
        goto domove;

    getzrange(ps[snum].posx, ps[snum].posy, ps[snum].posz, ps[snum].cursectnum, &cz, &ch, &fz, &fh, 128, 0);
    if (ps[snum].jetpack_on == 0) {
        int dx = ps[snum].previous_x - ps[snum].posx, dy = ps[snum].previous_y - ps[snum].posy;

        ps[snum].bobcounter += ksqrt(dx * dx + dy * dy);
    }
    ps[snum].previous_x = ps[snum].posx;
    ps[snum].previous_y = ps[snum].posy;
    ps[snum].previous_z = ps[snum].posz;
    ps[snum].previous_ang = ps[snum].ang;
    if (ps[snum].state95)
        ps[snum].state95--;

    update_air(snum);

    lotag = sector[ps[snum].cursectnum].lotag & 0x7FFF;
    if (lotag == 2) {
        if (player_svel[snum] < 0)
            player_svel[snum] >>= 2;
        ps[snum].jumping_counter = 0;
        if (player_bits[snum] & 1) {
            if (ps[snum].poszv > -2048)
                ps[snum].poszv -= 348;
            else
                ps[snum].poszv = -2048;
        } else if (player_bits[snum] & 2) {
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
    tics2 = tics + tics;
    if (ps[snum].jetpack_on) {
        ps[snum].on_ground = 0;
        ps[snum].jumping_counter = 0;
        if (player_bits[snum] & 0x20)
            tics = tics2;
        if (ps[snum].jetpack_on < 11) {
            if (ps[snum].jetpack_on == 10)
                spritesound(19, ps[snum].i);
            ps[snum].posz -= ++ps[snum].jetpack_on << 7;
        }
        if (player_bits[snum] & 1)
            ps[snum].posz -= 2048;
        if (player_bits[snum] & 2)
            ps[snum].posz += 3096;
        if (player_svel[snum] > 0)
            ps[snum].horiz -= 16;
        if (player_svel[snum] < 0)
            ps[snum].horiz += 16;
        if (fz - 4096 < ps[snum].posz)
            ps[snum].posz += (fz - 4096 - ps[snum].posz) >> 1;
        if (cz + 4096 > ps[snum].posz)
            ps[snum].posz += (cz + 4096 - ps[snum].posz) >> 1;
        goto doturn;
    }
    if (lotag == 2)
        goto doturn;
    stopsound(19);
    if ((sector[ps[snum].cursectnum].lotag & 0x7FFF) == 1) {
        short s;

        ps[snum].bobcounter += 128;
        s = sintable[(ps[snum].bobcounter >> 1) & 2047];
        eye = 34;
        ps[snum].posz += klabs(s) >> 6;
        if (ps[snum].on_ground == 0 && (player_bits[snum] & 0x20))
            tics = tics2;
    } else {
        eye = 40;
        if (!(player_bits[snum] & 2) && (player_bits[snum] & 0x20))
            tics = tics2;
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
        if ((player_bits[snum] & 1) && ps[snum].jumping_counter == 0 && fz - cz > 17408) {
            ps[snum].jumping_counter = 348;
            spritesound(8, ps[snum].i);
        }
        if ((player_bits[snum] & 2) && ps[snum].on_ground) {
            if (eye == 40)
                ps[snum].posz += 3072;
            else
                ps[snum].posz += 2048;
        }
    }
    if (ps[snum].jumping_counter) {
        ps[snum].bobcounter = 1024;
        if (ps[snum].jumping_counter < 1024) {
            ps[snum].on_ground = 0;
            ps[snum].poszv -= sintable[ps[snum].jumping_counter] >> 5;
            ps[snum].jumping_counter += 92;
        } else {
            ps[snum].jumping_counter = 0;
            ps[snum].poszv = 0;
        }
    }
    if (cz + 4096 > ps[snum].posz) {
        ps[snum].posz = cz + 4096;
        ps[snum].jumping_counter = 0;
        ps[snum].poszv = 512;
    }
    ps[snum].posz += ps[snum].poszv;

doturn:
    if (player_avel[snum]) {
        ps[snum].ang += tics * player_avel[snum] / 24;
        ps[snum].ang = (ps[snum].ang + 2048) & 2047;
    }
    if (klabs(cz - fz) < 7168) {
        lotag = sector[ps[snum].cursectnum].lotag & 0x7FFF;
        if (lotag > 2 && lotag != 23) {
            sector[ps[snum].cursectnum].ceilingz -= 1024;
            if (disable_armor == 0 && ps[snum].armor > 0)
                ps[snum].armor -= 4;
            else if (ps[snum].health > 0)
                ps[snum].health -= 5;
            else
                ps[snum].health = 0;
        }
    }
    xvect = yvect = 0;
    if (player_vel[snum] | player_svel[snum]) {
        if (sintable[ps[snum].bobcounter & 2047] >> 12 == 3) {
            if (ps[snum].footstep == 0 && ps[snum].on_ground) {
                switch (sector[ps[snum].cursectnum].lotag & 0x7FFF) {
                case 1:
                    spritesound(28, ps[snum].i);
                    ps[snum].footstep = 1;
                    break;
                case 2:
                    break;
                default:
                    spritesound(2, ps[snum].i);
                    ps[snum].footstep = 1;
                    break;
                }
            }
        } else
            ps[snum].footstep = 0;
        if (ps[snum].hard_landing)
            tics = 0;
        if (player_svel[snum]) {
            xvect += (tics * player_svel[snum] * sintable[(ps[snum].ang + 2560) & 2047]) >> 3;
            yvect += (tics * player_svel[snum] * sintable[(ps[snum].ang + 2048) & 2047]) >> 3;
        }
        if (player_vel[snum]) {
            xvect += (tics * player_vel[snum] * sintable[(ps[snum].ang + 2048) & 2047]) >> 3;
            yvect += (tics * player_vel[snum] * sintable[(ps[snum].ang + 1536) & 2047]) >> 3;
        }
        if (player_bits[snum] & 2) {
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
    if ((player_bits[snum] & 0x40000) || ps[snum].jetpack_on || ps[snum].curr_weapon == RPG || ps[snum].hard_landing) {
        ps[snum].horiz += 32 - ps[snum].horiz / 3;
        if (ps[snum].curr_weapon == RPG || ps[snum].jetpack_on) {
            if (player_bits[snum] & 0x2000)
                ps[snum].horiz += 64;
            else if (player_bits[snum] & 0x4000)
                ps[snum].horiz -= 64;
        }
    } else if (player_bits[snum] & 0x2000) {
        ps[snum].horiz += 48;
    } else if (player_bits[snum] & 0x4000) {
        ps[snum].horiz -= 48;
    }
    if (ps[snum].hard_landing) {
        ps[snum].hard_landing--;
        ps[snum].horiz -= 13 * ps[snum].hard_landing;
    }
    if (ps[snum].horiz > 199)
        ps[snum].horiz = 199;
    else if (ps[snum].horiz < 0)
        ps[snum].horiz = 0;

    if (ps[snum].raise_frame && lockclock - ps[snum].weapon_clock > 5) {
        ps[snum].weapon_clock = (unsigned char)lockclock;
        ps[snum].raise_frame++;
        if (ps[snum].raise_frame == 12 || (player_bits[snum] & 4)) {
            ps[snum].raise_frame = 0;
            ps[snum].weapon_pos = 0x81;
        }
        return;
    }
    kick = ps[snum].weapon_pos;
    if ((kick & 0x7F) > 2 && ps[snum].curr_weapon == SHOTSPARK1 && ps[snum].kickback_pic < 5) {
        spawn_pistol_tracer(snum);
    }
    if ((kick & 0x7F) && (kick & 0x7F) != 6) {
        if (kick & 0x80)
            ps[snum].weapon_pos++;
        else
            ps[snum].weapon_pos--;
    } else if (ps[snum].last_weapon >= 0) {
        ps[snum].weapon_pos = 0x81;
        ps[snum].last_weapon = -1;
    }

    if ((player_bits[snum] & 4) && ps[snum].kickback_pic == 0 && ps[snum].last_weapon == -1) {
        if ((ps[snum].weapon_pos & 0x7F) != 6)
            ps[snum].weapon_pos = 0x81;
        switch (ps[snum].curr_weapon) {
        case SHOTSPARK1:
            if (ps[snum].ammo > 0) {
                ps[snum].kickback_pic = 1;
                ps[snum].ammo--;
            }
            break;
        case BULLET:
            if (ps[snum].batteryammo > 0) {
                ps[snum].kickback_pic = 2;
                ps[snum].batteryammo--;
            }
            break;
        case CLUB:
            ps[snum].knee_sequence = (unsigned char)(ps[snum].batteryammo ? 5 : krand() % 5);
            ps[snum].kickback_pic = 1;
            break;
        case RPG:
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
    switch (ps[snum].curr_weapon) {
    case HEAVYHBOMB:
        if (++ps[snum].kickback_pic == 11) {
            if (ps[snum].remote_ammo > 0)
                ps[snum].remote_ammo--;
            EGS(ps[snum].cursectnum, (sintable[(ps[snum].ang + 512) & 2047] >> 4) + ps[snum].posx,
                (sintable[ps[snum].ang & 2047] >> 4) + ps[snum].posy, ps[snum].posz + 1024, 1, HEAVYHBOMB, -16, 16, 16,
                ps[snum].ang, 128, -348, ps[snum].i, 1);
            ps[snum].remote_active = 1;
        }
        if (ps[snum].kickback_pic > 19) {
            ps[snum].curr_weapon = HANDREMOTE;
            ps[snum].kickback_pic = 0;
        }
        break;
    case HANDREMOTE:
        if (++ps[snum].kickback_pic == 2)
            ps[snum].remote_active = 0;
        if (ps[snum].kickback_pic == 10) {
            ps[snum].kickback_pic = 0;
            if (ps[snum].remote_ammo)
                ps[snum].curr_weapon = HEAVYHBOMB;
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
        if (ps[snum].kickback_pic >= 5 && (ps[snum].ammo % 12 != 0 || ps[snum].ammo == 0))
            ps[snum].kickback_pic = 0;
        if (ps[snum].kickback_pic == 27)
            ps[snum].kickback_pic = 0;
        break;
    case BULLET:
        if (++ps[snum].kickback_pic >= 6)
            ps[snum].kickback_pic = 0;
        if (ps[snum].kickback_pic == 3) {
            shoot(ps[snum].i, BULLET, 0);
        }
        break;
    case CLUB: {
        unsigned char frame;

        ps[snum].kickback_pic++;
        frame = knee_frames[ps[snum].knee_sequence][ps[snum].kickback_pic];
        if (frame < 4) {
            if (frame == 3 && ps[snum].kickback_pic == 4 && ps[snum].batteryammo && (player_bits[snum] & 4)
                && (sector[ps[snum].cursectnum].lotag & 0x7FFF) != 2) {
                shoot(ps[snum].i, CLUB, 0);
                spritesound(24, ps[snum].i);
                ps[snum].batteryammo--;
                ps[snum].kickback_pic--;
                ps[snum].knee_flash++;
            }
        } else if (frame == 4) {
            shoot(ps[snum].i, CLUB, 0);
        } else if (frame == 255) {
            ps[snum].kickback_pic = 0;
        }
        break;
    }
    case RPG:
        if (ps[snum].rocket_ammo > 0 && (player_bits[snum] & 4) && ps[snum].kickback_pic == 10) {
            ps[snum].kickback_pic = 11;
            ps[snum].rocket_ammo--;
            shoot(ps[snum].i, RPG, 0);
            ps[snum].horiz += 64;
        }
        if (ps[snum].kickback_pic <= 10) {
            if (ps[snum].kickback_pic != 0 && ps[snum].kickback_pic < 10)
                ps[snum].kickback_pic++;
        } else if (ps[snum].kickback_pic >= 18) {
            ps[snum].kickback_pic = 10;
        } else {
            ps[snum].kickback_pic++;
        }
        break;
    }
}
