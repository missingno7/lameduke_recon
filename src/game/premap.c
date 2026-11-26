/* PREMAP (file name HYPOTHESIS): 1:11380-14fb0, CONST 2:00648-006a8, _BSS 2:0ac60 (mspos). */
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <sys/stat.h>

#include "build_v5.h"
#include "game.h"

extern void bootleg_cache_pop(void);
extern char file_id_record[];
extern long animatecnt;
extern char music_files[4][10][13];
extern void setview(long left, long top, long right, long bottom);
extern void rotatesprite(long x, long y, long zoom, short ang, short tile);
extern void checkhitswitch(short w);
extern void precache(void);
extern void allignwarpelevators(void);
extern void playmusic(char *name);
extern void call_level_load_hook(char *name);
#pragma aux call_level_load_hook "level_load_hook_"
extern unsigned char level_tick_state;

long mspos;

/* The window is rendered into the framebuffer, copied to the tile, then
 * drawn back at 1:1 before a second copy establishes the tile's orientation. */
/* Offsets advance by eight rows per iteration in each buffer. */
#define COPY_CAMERA_WINDOW(tile, row, dest)                                                  \
    for (; row < 320 * 95; row += 320 * 8, dest += 96 * 8) {                                 \
        copybuf((void *)(frameplace + row), (void *)(waloff[tile] + dest), 24);              \
        copybuf((void *)(frameplace + row + 320), (void *)(waloff[tile] + dest + 96), 24);   \
        copybuf((void *)(frameplace + row + 640), (void *)(waloff[tile] + dest + 192), 24);  \
        copybuf((void *)(frameplace + row + 960), (void *)(waloff[tile] + dest + 288), 24);  \
        copybuf((void *)(frameplace + row + 1280), (void *)(waloff[tile] + dest + 384), 24); \
        copybuf((void *)(frameplace + row + 1600), (void *)(waloff[tile] + dest + 480), 24); \
        copybuf((void *)(frameplace + row + 1920), (void *)(waloff[tile] + dest + 576), 24); \
        copybuf((void *)(frameplace + row + 2240), (void *)(waloff[tile] + dest + 672), 24); \
    }

void render_camera_tile(long x, long y, long z, short ang, short sect, short tile)
{
    short row, dest, row2, dest2;

    setview(0, 0, 95, 95);
    drawrooms(x, y, z, ang, 100, sect);
    animatesprites(ang);
    drawmasks();
    row = dest = 0;
    COPY_CAMERA_WINDOW(tile, row, dest)
    row2 = dest2 = 0;
    rotatesprite(47L << 16, 47L << 16, 1L << 16, 0, tile);
    COPY_CAMERA_WINDOW(tile, row2, dest2)
    vscrn(screenpeek);
}

void render_sprite_camera(short i)
{
    short row, dest, row2, dest2, ang, sect, tile;
    long x, y, z;

    x = sprite[i].x;
    y = sprite[i].y;
    sect = sprite[i].sectnum;
    z = sprite[i].z;
    tile = sprite[i].picnum;
    ang = sprite[i].ang;
    setview(0, 0, 95, 95);
    drawrooms(x, y, z, ang, 100, sect);
    animatesprites(ang);
    drawmasks();
    row = dest = 0;
    COPY_CAMERA_WINDOW(tile, row, dest)
    rotatesprite(47L << 16, 47L << 16, 1L << 16, 0, tile);
    row2 = dest2 = 0;
    COPY_CAMERA_WINDOW(tile, row2, dest2)
    vscrn(screenpeek);
}

void render_mirror(short w)
{
    long x, y, z, x1, y1, x2, y2, player_x;
    short row, dest, row2, dest2, ang, sect, tile;

    x1 = wall[w].x;
    y1 = wall[w].y;
    x2 = wall[wall[w].point2].x;
    y2 = wall[wall[w].point2].y;
    player_x = ps[screenpeek].posx;
    x = (x1 + x2) / 2;
    y = (y1 + y2) / 2;
    z = ps[screenpeek].posz;
    ang = getangle(player_x - x, ps[screenpeek].posy - y);
    tile = 570;
    sect = wall[w].nextsector;
    setview(0, 0, 95, 95);
    drawrooms(x, y, z, ang, 100, sect);
    animatesprites(ang);
    drawmasks();
    row = dest = 0;
    COPY_CAMERA_WINDOW(tile, row, dest)
    rotatesprite(47L << 16, 47L << 16, 1L << 16, 0, tile);
    row2 = dest2 = 0;
    COPY_CAMERA_WINDOW(tile, row2, dest2)
    vscrn(screenpeek);
}

#undef COPY_CAMERA_WINDOW

void resetplayer(short p)
{
    short look;

    ps[p].horiz = 100;
    ps[p].bobcounter = 0;
    ps[p].posx = ps[p].oposx;
    ps[p].posy = ps[p].oposy;
    ps[p].posz = ps[p].oposz;
    ps[p].ang = ps[p].oang;
    ps[p].cursectnum = ps[p].ocursectnum;
    ps[p].knee_flash = 0;
    ps[p].on_ground = 0;
    look = 256 - (krand() & 512);
    ps[p].poszv = ps[p].jumping_counter = 0;
    ps[p].weapon_clock = 0;
    ps[p].lookang = look;
    ps[p].state80 = -1;
    ps[p].flash_sprite = -1;
    ps[p].hard_landing = 0;
    ps[p].newowner = -1;
    if (actorscrptr[283] != NULL)
        ps[p].health = (short)*actorscrptr[283];
    else
        ps[p].health = 100;
    ps[p].state82 = -1;
    ps[p].remote_active = 0;
    ps[p].kickback_pic = 5;
    ps[p].weapon_pos = 6;
    ps[p].raise_frame = 1;
    ps[p].mapzoom = 768;
}

void reset_player_motion(short p)
{
    short look;

    ps[p].statea2 = 0;
    ps[p].secret_rooms = 0;
    ps[p].max_secret_rooms = 0;
    ps[p].actors_squashed = 0;
    ps[p].max_squashed = 0;
    ps[p].actors_killed = 0;
    ps[p].max_actors_killed = 0;
    ps[p].footstep = 0;
    if (ps[p].display_mode == 2)
        setgamemode();
    camerasprite = -1;
    ps[p].display_mode = 3;
    mspos = 0;
    ps[p].parallax_sector = -1;
    ps[p].horiz = 100;
    viewscreen = -1;
    ps[p].oposx = ps[p].posx;
    ps[p].on_ground = 0;
    ps[p].oposy = ps[p].posy;
    ps[p].oposz = ps[p].posz;
    forward_velocity = 0;
    ps[p].oang = ps[p].ang;
    strafe_velocity = 0;
    ps[p].ocursectnum = ps[p].cursectnum;
    screenpeek = myconnectindex;
    ps[p].knee_flash = 0;
    visibility = userdefs.visibility;
    parallaxtype = 0;
    ps[p].posx = ps[p].oposx;
    turn_velocity = 0;
    animatecnt = 0;
    ps[p].posy = ps[p].oposy;
    gamepaused = 0;
    ps[p].posz = ps[p].oposz;
    numanimwalls = 0;
    ps[p].ang = ps[p].oang;
    randomseed = 17;
    ps[p].cursectnum = ps[p].ocursectnum;
    ps[p].bobcounter = 0;
    look = 256 - (krand() & 512);
    ps[p].newowner = -1;
    ps[p].jumping_counter = 0;
    ps[p].state80 = -1;
    ps[p].flash_sprite = -1;
    ps[p].lookang = look;
    ps[p].hard_landing = 0;
    ps[p].weapon_clock = 0;
    ps[p].poszv = 0;
    if (actorscrptr[283] != NULL)
        ps[p].health = (short)*actorscrptr[283];
    else
        ps[p].health = 100;
    ps[p].state82 = -1;
    ps[p].remote_active = 0;
    ps[p].kickback_pic = 5;
    ps[p].weapon_pos = 6;
    ps[p].raise_frame = 1;
    ps[p].mapzoom = 768;
}

void prelevel(void)
{
    short p = 0, i, k, sect, nexti, map_sprite;
    long j;
    long *actor;

    ps[0].statea2 = 0;
    ps[0].secret_rooms = 0;
    ps[0].max_secret_rooms = 0;
    ps[0].actors_squashed = 0;
    ps[0].max_squashed = 0;
    ps[0].actors_killed = 0;
    ps[0].footstep = 0;
    ps[0].max_actors_killed = 0;
    if (ps[p].display_mode == 2)
        setgamemode();
    ps[p].display_mode = 3;
    ps[p].oposx = ps[p].posx;
    forward_velocity = 0;
    strafe_velocity = 0;
    ps[p].oposy = ps[p].posy;
    turn_velocity = 0;
    camerasprite = -1;
    ps[p].oposz = ps[p].posz;
    viewscreen = -1;
    ps[p].oang = ps[p].ang;
    gamepaused = 0;
    ps[p].ocursectnum = ps[p].cursectnum;
    numanimwalls = 0;
    ps[p].parallax_sector = -1;
    screenpeek = myconnectindex;
    parallaxtype = 0;
    visibility = userdefs.visibility;
    animatecnt = 0;
    mspos = 0;
    randomseed = 17;
    resetplayer(p);

    for (i = 0; i < 13; i++)
        ps[0].quote_time[i] = 0;
    for (i = 0; i < 512; i++)
        show2dwall[i] = 255;
    for (i = 0; i < 512; i++)
        show2dsprite[i] = 255 * disable_armor;

    for (i = 0; i < numsectors; i++) {
        sector[i].extra = 256;
        if (ps[0].parallax_sector == -1 && (sector[i].ceilingstat & 1))
            ps[0].parallax_sector = i;
        if ((sector[i].lotag & 32767) == 32767)
            ps[0].max_secret_rooms++;
        else if (sector[i].lotag == 2)
            sector[i].ceilingpal = sector[i].floorpal = 1;
    }
    for (i = headspritestat[0]; i >= 0; i = nexti) {
        nexti = nextspritestat[i];
        if (sprite[i].picnum == 310) {
            sector[sprite[i].sectnum].extra = sprite[i].lotag;
            deletesprite(i);
        }
    }

    /* Initialize the map sprites in index order. */
    for (map_sprite = 0; map_sprite < 4096; map_sprite++) {
        short i = map_sprite;

        if (sprite[i].statnum >= 1024)
            continue;
        j = -1;
        k = sprite[i].picnum;
        hittype[i][3] = 0;
        hittype[i][2] = 0;
        hittype[i][0] = 0;
        if (k >= 265 && k <= 269)
            k = 265;
        actor = actorscrptr[k];
        if (actor == NULL) {
            hittype[i][4] = 0;
            hittype[i][1] = 0;
        } else {
            sprite[i].extra = (short)actor[0];
            hittype[i][4] = actor[1];
            hittype[i][1] = actor[2];
            if (k != 57)
                sprite[i].hitag = (short)actor[3];
        }
        sect = sprite[i].sectnum;
        switch (sprite[i].picnum) {
        case 1408:
            sprite[i].owner = -1;
            changespritestat(i, 1);
            break;
        case 1165:
            if (j >= 0) {
                short a;

                if (sprite[j].picnum == 283) {
                    short p = sprite[j].yvel;

                    a = (short)(ps[p].ang - (krand() & 0x3F));
                    hittype[i][0] = krand() & 1;
                    sprite[i].x = sprite[j].x + (sintable[(a + 512) & 2047] >> 6);
                    sprite[i].y = (sintable[a & 2047] >> 6) + sprite[j].y;
                    sprite[i].z = sprite[j].z - 32 * ps[p].horiz + 4608;
                    sprite[i].zvel = (short)-(krand() & 0xFF);
                } else {
                    a = sprite[i].ang;
                    sprite[i].x = sprite[j].x + (sintable[(a + 512) & 2047] >> 6);
                    sprite[i].y = (sintable[a & 2047] >> 6) + sprite[j].y;
                    sprite[i].z = sprite[j].z - 9216;
                }
                sprite[i].shade = -8;
                sprite[i].ang = (short)(a - 512);
                sprite[i].zvel = -192;
                sprite[i].xvel = 48;
                sprite[i].xrepeat = sprite[i].yrepeat = 5;
                changespritestat(i, 5);
            }
            break;
        case 1406:
            if (j >= 0) {
                sprite[i].xrepeat = sprite[i].yrepeat = 16;
                sprite[i].shade = -15;
                sprite[i].cstat |= rand() & 0xE;
                sprite[i].ang = sprite[j].ang;
                sprite[i].xvel = sprite[j].xvel;
                sprite[i].zvel = sprite[j].zvel;
                sprite[i].owner = j;
                sprite[i].z -= 0x2000;
                changespritestat(i, 4);
            }
            break;
        case 5:
            sprite[i].xrepeat = sprite[i].yrepeat = 0;
            changespritestat(i, 11);
            break;
        case 1404:
            if (j >= 0) {
                sprite[i].owner = j;
                sprite[i].xrepeat = (unsigned char)(tilesizx[sprite[j].picnum] / 3);
                sprite[i].yrepeat = (unsigned char)(tilesizy[sprite[j].picnum] / 3);
                sprite[i].shade = -127;
            }
            changespritestat(i, 5);
            break;
        case 1359:
            sprite[i].xrepeat = sprite[i].yrepeat = 32;
            sprite[i].cstat |= 0x22;
            sprite[i].ang = (short)(krand() & 2047);
            sprite[i].shade = 0;
            sprite[i].z = sector[sect].floorz;
            changespritestat(i, 5);
            break;
        case 1360:
            if (j >= 0 && sprite[j].statnum == 10)
                sprite[i].z += 4096;
            /* fall through */
        case 686: case 1370:
            if (sprite[i].picnum == 1360)
                sprite[i].xrepeat = sprite[i].yrepeat = 8;
            else
                sprite[i].xrepeat = sprite[i].yrepeat = 28;
            sprite[i].cstat = (unsigned char)(krand() & 0xC);
            sprite[i].shade = -127;
            sprite[i].ang = sprite[j].ang;
            changespritestat(i, 5);
            break;
        case 283:
            sprite[i].xrepeat = sprite[i].yrepeat = 0;
            sprite[i].shade = -32;
            changespritestat(i, 10);
            break;
        case 215:
            sprite[i].xrepeat = sprite[i].yrepeat = 10;
            sprite[i].shade = -16;
            changespritestat(i, 5);
            break;
        case 1280:
            sprite[i].xrepeat = sprite[i].yrepeat = 64;
            sprite[i].shade = -127;
            sprite[i].cstat |= 1;
            changespritestat(i, 2);
            break;
        case 1175: {
            short k;

            show2dsprite[i >> 3] |= 1 << (i & 7);
            sprite[i].cstat |= 0x40;
            sprite[i].picnum += 2;
            sprite[i].z = sector[sect].ceilingz + 12288;
            hittype[i][4] = mspos;
            msx[mspos] = sprite[i].x;
            msy[mspos] = sprite[i].y;
            msx[mspos + 2] = sprite[i].z;
            for (k = headspritestat[0]; k >= 0; k = nextspritestat[k]) {
                if (sprite[k].picnum == 1174 && sprite[k].hitag == sprite[i].hitag) {
                    hittype[i][1] = sprite[k].sectnum;
                    sprite[k].xrepeat = 48;
                    sprite[k].yrepeat = 128;
                    msx[mspos + 1] = sprite[k].x;
                    msy[mspos + 1] = sprite[k].y;
                    sprite[k].x = sprite[i].x;
                    sprite[k].y = sprite[i].y;
                    sprite[k].z = sprite[i].z;
                    sprite[k].shade = sprite[i].shade;
                    msy[mspos + 2] = k;
                    setsprite(k, sprite[k].x, sprite[k].y, sprite[k].z);
                    break;
                }
            }
            sprite[i].owner = -1;
            mspos += 3;
            changespritestat(i, 6);
            break;
        }
        case 1190:
            sprite[i].xrepeat = sprite[i].yrepeat = 20;
            sprite[i].shade = -16;
            changespritestat(i, 5);
            break;
        case 960:
            sprite[i].xrepeat = 16;
            sprite[i].yrepeat = 0;
            hittype[i][1] = krand() & 0x1F;
            sprite[i].z = sector[sect].ceilingz + 2048;
            changespritestat(i, 2);
            break;
        case 962:
            sprite[i].z = sector[sect].ceilingz + 6144;
            hittype[i][0] = 3;
            /* fall through */
        case 961:
            show2dsprite[i >> 3] |= 1 << (i & 7);
            sprite[i].shade = -16;
            sprite[i].xrepeat = 32;
            sprite[i].yrepeat = 16;
            sprite[i].cstat |= 1;
            changespritestat(i, 2);
            break;
        case 305:
            hittype[i][0] = sprite[i].xrepeat;
            hittype[i][1] = sprite[i].yrepeat;
            /* fall through */
        case 32: case 51: case 216:
            sprite[i].xrepeat = sprite[i].yrepeat = 0;
            sprite[i].yvel = 0;
            changespritestat(i, 1);
            break;
        case 408: case 490: case 579: case 1536: case 1680: case 1820:
            ps[0].max_squashed++;
            ps[0].max_actors_killed++;
            /* fall through */
        case 631:
            sprite[i].shade = -24;
            sprite[i].yrepeat = 43;
            show2dsprite[i >> 3] |= 1 << (i & 7);
            sprite[i].xrepeat = 52;
            if (sprite[i].picnum == 408)
                sprite[i].xrepeat -= 9;
            changespritestat(i, 2);
            sprite[i].cstat |= 1;
            break;
        case 34:
            changespritestat(i, 7);
            sprite[i].xrepeat = sprite[i].yrepeat = 0;
            break;
        case 23: case 31:
            sprite[i].xrepeat = sprite[i].yrepeat = 0;
            if (sprite[i].picnum == 23)
                sector[sect].lotag ^= 0x4000;
            changespritestat(i, 8);
            break;
        case 30: case 313: case 314: case 1123:
            sprite[i].cstat |= 1;
            /* fall through */
        case 1122:
            sprite[i].shade = -12;
            show2dsprite[i >> 3] |= 1 << (i & 7);
            changespritestat(i, 1);
            break;
        case 38: case 60: case 70: case 80: case 159: case 263: case 315: case 316: case 381: case 620:
        case 677: case 720: case 721: case 722: case 723: case 724: case 726: case 727: case 728:
        case 1119: case 1120: case 1121: case 1125:
            sprite[i].shade = -17;
            show2dsprite[i >> 3] |= 1 << (i & 7);
            sprite[i].cstat |= 1;
            changespritestat(i, 2);
            break;
        case 931:
            show2dsprite[i >> 3] |= 1 << (i & 7);
            sprite[i].xrepeat = sprite[i].yrepeat = 32;
            hittype[i][3] = sector[sect].ceilingz;
            sprite[i].z = hittype[i][3] + 10752;
            sprite[i].cstat |= 9;
            ps[0].max_actors_killed++;
            changespritestat(i, 2);
            break;
        case 930:
            show2dsprite[i >> 3] |= 1 << (i & 7);
            sprite[i].xrepeat = sprite[i].yrepeat = 32;
            hittype[i][3] = sector[sect].floorz;
            sprite[i].z = hittype[i][3] + 3072;
            sprite[i].cstat |= 1;
            ps[0].max_actors_killed++;
            changespritestat(i, 2);
            break;
        case 940:
            show2dsprite[i >> 3] |= 1 << (i & 7);
            sprite[i].owner = i;
            sprite[i].xrepeat = sprite[i].yrepeat = 24;
            sprite[i].cstat |= 1;
            ps[0].max_actors_killed++;
            changespritestat(i, 4);
            break;
        case 323:
            sprite[i].z = sector[sect].floorz - (sprite[i].yrepeat << 7);
            break;
        case 265: case 266: case 267: case 268: case 269:
            sprite[i].picnum = 265;
            /* fall through */
        case 327:
            show2dsprite[i >> 3] |= 1 << (i & 7);
            sprite[i].shade = -16;
            changespritestat(i, 1);
            break;
        case 175: {
            short k, wall_end;

            sprite[i].yvel = sector[sect].extra;
            sprite[i].xrepeat = 0;
            sprite[i].yrepeat = 0;
            switch (sprite[i].lotag) {
            case 27:
                if (demomode == 1) {
                    sprite[i].xrepeat = 64;
                    sprite[i].yrepeat = 64;
                }
                break;
            case 12:
                hittype[i][1] = sector[sect].floorshade;
                hittype[i][2] = sector[sect].ceilingshade;
                break;
            case 13:
                hittype[i][0] = sector[sect].ceilingz;
                hittype[i][1] = sector[sect].floorz;
                sprite[i].owner = klabs(hittype[i][0] - sprite[i].z) < klabs(hittype[i][1] - sprite[i].z);
                if (sprite[i].ang == 512) {
                    if (sprite[i].owner)
                        sector[sect].ceilingz = sprite[i].z;
                    else
                        sector[sect].floorz = sprite[i].z;
                } else {
                    sector[sect].floorz = sprite[i].z;
                    sector[sect].ceilingz = sprite[i].z;
                }
                if (!(sector[sect].ceilingstat & 1))
                    break;
                sector[sect].ceilingstat ^= 1;
                hittype[i][3] = 1;
                if (sprite[i].owner == 0 && sprite[i].ang == 512) {
                    sector[sect].ceilingstat ^= 1;
                    hittype[i][3] = 0;
                }
                sector[sect].ceilingshade = sector[sect].floorshade;
                if (sprite[i].ang == 512) {
                    wall_end = sector[sect].wallptr + sector[sect].wallnum;
                    for (k = sector[sect].wallptr; k < wall_end; k++) {
                        short ns = wall[k].nextsector;

                        if (ns >= 0 && !(sector[ns].ceilingstat & 1)) {
                            sector[sect].ceilingpicnum = sector[ns].ceilingpicnum;
                            sector[sect].ceilingshade = sector[ns].ceilingshade;
                            break;
                        }
                    }
                }
                break;
            case 17:
                hittype[i][2] = sector[sect].floorz;
                hittype[i][3] = sector[nextsectorneighborz(sect, sector[sect].floorz, -1, -1)].ceilingz;
                hittype[i][4] = sector[nextsectorneighborz(sect, sector[sect].ceilingz, 1, 1)].floorz;
                break;
            case 24:
                sprite[i].yvel *= 2;
                sector[sect].floorstat |= 0x40;
                break;
            case 23:
                sprite[i].xrepeat = 64;
                sprite[i].yrepeat = 64;
                sprite[i].shade = 0;
                hittype[i][1] = mspos;
                for (k = 0; k < numsectors; k++) {
                    if (sector[k].hitag == sprite[i].hitag) {
                        msx[mspos++] = k;
                        hittype[i][0]++;
                    }
                }
                break;
            case 20: {
                long best = 0x7FFFFFFF, w0 = -1;

                wall_end = sector[sect].wallptr + sector[sect].wallnum;
                for (k = sector[sect].wallptr; k < wall_end; k++) {
                    long d = ksqrt((sprite[i].y - wall[k].y) * (sprite[i].y - wall[k].y) + (sprite[i].x - wall[k].x) * (sprite[i].x - wall[k].x));

                    if (d < best) {
                        best = d;
                        w0 = k;
                    }
                }
                hittype[i][1] = w0;
                best = 0x7FFFFFFF;
                wall_end = sector[sect].wallptr + sector[sect].wallnum;
                for (k = sector[sect].wallptr; k < wall_end; k++) {
                    long d = ksqrt((sprite[i].y - wall[k].y) * (sprite[i].y - wall[k].y) + (sprite[i].x - wall[k].x) * (sprite[i].x - wall[k].x));

                    if (d < best && k != hittype[i][1]) {
                        best = d;
                        w0 = k;
                    }
                }
                hittype[i][2] = w0;
                break;
            }
            case 3:
                hittype[i][3] = sector[sect].floorshade;
                sector[sect].floorshade = sprite[i].shade;
                sector[sect].ceilingshade = sprite[i].shade;
                sprite[i].owner = (short)(sector[sect].ceilingpal << 8);
                sprite[i].owner |= sector[sect].floorpal;
                wall_end = sector[sect].wallptr + sector[sect].wallnum;
                for (k = sector[sect].wallptr; k < wall_end; k++)
                    if (!(wall[k].hitag & 1))
                        wall[k].shade = sprite[i].shade;
                break;
            case 4:
                hittype[i][2] = sector[sect].floorshade;
                sprite[i].owner = (short)(sector[sect].ceilingpal << 8);
                sprite[i].owner |= sector[sect].floorpal;
                wall_end = sector[sect].wallptr + sector[sect].wallnum;
                for (k = sector[sect].wallptr; k < wall_end; k++)
                    if (wall[k].shade > hittype[i][3])
                        hittype[i][3] = wall[k].shade;
                break;
            case 9:
                if ((sector[sect].lotag & 0x7FFF) && klabs(sector[sect].ceilingz - sprite[i].z) > 1024)
                    sector[sect].lotag |= 32768U;
                /* fall through */
            case 8:
                hittype[i][0] = sector[sect].floorshade;
                hittype[i][1] = sector[sect].ceilingshade;
                wall_end = sector[sect].wallptr + sector[sect].wallnum;
                for (k = sector[sect].wallptr; k < wall_end; k++)
                    if (wall[k].shade > hittype[i][2])
                        hittype[i][2] = wall[k].shade;
                hittype[i][3] = (sprite[i].yvel >> 8) + 2;
                if (hittype[i][3] < 8)
                    hittype[i][3] = 1;
                hittype[i][3] = 8; /* the exe overwrites the value it just computed */
                break;
            case 11:
                hittype[i][3] = sprite[i].ang <= 1024 ? -2 : 2;
                /* fall through */
            case 0: case 2: case 5: case 6: case 14: case 15: case 16: case 25: case 26: {
                short s;

                hittype[i][1] = mspos;
                wall_end = sector[sect].wallptr + sector[sect].wallnum;
                for (k = sector[sect].wallptr; k < wall_end; k++) {
                    msx[mspos] = wall[k].x - sprite[i].x;
                    msy[mspos] = wall[k].y - sprite[i].y;
                    mspos++;
                }
                if (sprite[i].lotag == 6 || sprite[i].lotag == 14 || sprite[i].lotag == 5) {
                    if (sector[sprite[i].sectnum].hitag != -1) {
                        sector[sect].floorstat |= 0x40;
                        sector[sect].ceilingstat |= 0x40;
                    }
                    /* The exe passes the wall loop's end index as the first sector hint. */
                    s = k;
                    for (k = 0; k < 10; k++) {
                        updatesector(k * (sintable[(sprite[i].ang + 512) & 2047] >> 5) + sprite[i].x,
                            sprite[i].y + k * (sintable[sprite[i].ang & 2047] >> 5), &s);
                        if (sector[s].hitag == 0)
                            break;
                        if (sector[s].hitag != -1) {
                            sector[s].ceilingstat |= 0x40;
                            sector[s].floorstat |= 0x40;
                        }
                    }
                    sprite[i].owner = -1;
                    hittype[i][0] = s;
                    hittype[i][3] = sprite[i].hitag;
                }
                if (sprite[i].lotag == 16)
                    hittype[i][3] = sector[sect].ceilingz;
                if (sprite[i].lotag == 25) {
                    hittype[i][4] = sector[sect].ceilingz;
                    sector[sect].ceilingz = sprite[i].z;
                }
                if (sprite[i].lotag == 26) {
                    hittype[i][3] = sprite[i].x;
                    hittype[i][4] = sprite[i].y;
                    sprite[i].zvel = sprite[i].shade == sector[sect].floorshade ? -256 : 256;
                    sector[sect].floorstat |= 0x40;
                    sprite[i].shade = 0;
                }
                break;
            }
            }
            changespritestat(i, sprite[i].lotag == 7 ? 9 : 3);
            break;
        }
        case 57:
            sprite[i].owner = i;
            sprite[i].shade = -16;
            if (sprite[i].xrepeat <= 8)
                sprite[i].xrepeat = sprite[i].yrepeat = 0;
            show2dsprite[i >> 3] |= 1 << (i & 7);
            sprite[i].cstat |= 1;
            changespritestat(i, 6);
            break;
        case 606: case 627: case 905: case 1200:
            sprite[i].owner = i;
            sprite[i].shade = -16;
            show2dsprite[i >> 3] |= 1 << (i & 7);
            sprite[i].yrepeat = sprite[i].xrepeat = 38;
            sprite[i].cstat |= 1;
            changespritestat(i, 6);
            break;
        }
    }

    for (i = 0; i < numwalls; i++) {
        if (numanimwalls >= 256)
            gameexit("Maxed out on anim walls\n");
        if (wall[i].overpicnum >= 1184 && wall[i].overpicnum <= 1186) {
            if (wall[i].overpicnum == 1184 && waloff[1184] == -1)
                for (k = 0; k < 3; k++)
                    loadtile(1184 + k);
            if (wall[i].shade < 32)
                wall[i].cstat |= 1;
            else
                wall[i].cstat = 0;
            wall[i].shade = 0;
            if (wall[i].lotag != 0)
                wall[wall[i].nextwall].lotag = wall[i].lotag;
            animwall[numanimwalls++].wallnum = i;
            continue;
        }
        switch (wall[i].picnum) {
        case 819: case 823: case 860: case 864:
            if (waloff[wall[i].picnum] == -1)
                for (k = 0; k < 4; k++)
                    loadtile(wall[i].picnum + k);
            /* fall through */
        case 570:
            animwall[numanimwalls++].wallnum = i;
            break;
        case 761:
            if (waloff[761] == -1)
                for (k = 761; k < 765; k++)
                    loadtile(k);
            /* fall through */
        case 764: case 765: case 766: case 767:
            animwall[numanimwalls].wallnum = i;
            animwall[numanimwalls].tag = wall[i].picnum;
            numanimwalls++;
            break;
        case 406:
            wall[i].picnum = 405;
            checkhitswitch(wall[i].lotag);
            animwall[numanimwalls].tag = 0;
            animwall[numanimwalls++].wallnum = i;
            break;
        case 395: case 396: case 405:
            animwall[numanimwalls].tag = 0;
            animwall[numanimwalls].wallnum = i;
            numanimwalls++;
            break;
        }
    }
}

void newgame(unsigned char episode, unsigned char level, unsigned char skill)
{
    ps[0].level = level;
    ps[0].volume = episode;
    ps[0].skill = skill;
    ps[0].scuba_on = 0;
    ps[0].inventory[0] = 0;
    ps[0].jetpack_on = 0;
    ps[0].inventory[1] = 0;
    ps[0].state95 = 0;
    ps[0].inventory[2] = 0;
    ps[0].armor = 0;
    ps[0].inventory[3] = 0;
    ps[0].state92 = 0;
    ps[0].state9c[0] = 0;
    ps[0].state9c[2] = 0;
    ps[0].state9c[3] = 0;
    ps[0].state9c[4] = 0;
    ps[0].screen_size = -4;
    ps[0].state9c[1] = 1;
    ps[0].score = 0;
    if (disable_armor) {
        ps[0].rocket_ammo = 99;
        ps[0].remote_ammo = 696;
        ps[0].ammo = 144;
        ps[0].batteryammo = 120;
    } else {
        ps[0].rocket_ammo = 5;
        ps[0].remote_ammo = 2;
        ps[0].ammo = 24;
        ps[0].batteryammo = 9;
    }
    ps[0].last_weapon = -1;
    ps[0].curr_weapon = 191;
    reccnt = 0;
}

void enterlevel(unsigned char mode)
{
    char *map_name = "L1.MAP";
    short i, p, load_result;
    unsigned char palette;

    if (disable_armor == 0 && (mode & 4)) {
        dobonus();
    } else {
        overwritesprite(0, 0, 482, 0, 0, 0);
        minitext(170, 50, (unsigned char *)"Entering...", 0);
        minitext(174, 62, (unsigned char *)"Episode", 0);
        minitext(174, 74, (unsigned char *)"Level", 0);
        numtext(232, 62, ps[0].volume + 1, 0);
        numtext(232, 74, ps[0].level + 1, 0);
        nextpage();
    }
    map_name[0] = 'L' + ps[0].volume;
    map_name[1] = '1' + ps[0].level;
    FX_StopAllSounds();
    call_level_load_hook(map_name);
    load_result = loadboard(map_name, &ps[0].posx, &ps[0].posy, &ps[0].posz,
                            &ps[0].ang, &ps[0].cursectnum);
    call_level_load_hook(map_name);
    if (load_result == -1)
        gameexit("Board not found.");
    prelevel();
    EGS(ps[0].cursectnum, ps[0].posx, ps[0].posy, ps[0].posz, 0, 283, 0, 0, 0,
        ps[0].ang, 0, 0, 0, 10);
    precache();
    cacheit();
    allignwarpelevators();
    for (i = 0; i < 17; i++) {
        memcpy(&ps[i], &ps[0], sizeof(ps[0]));
        player_svel[i] = pending_svel[i] = 0;
        player_vel[i] = pending_vel[i] = 0;
        player_avel[i] = pending_avel[i] = 0;
        player_bits[i] = pending_bits[i] = 0;
    }
    moveloop();
    if (option[0] == 7)
        setview(0, 0, 639, 479);
    else
        vscrn(0);

    palette = 0;
    i = headspritestat[10];
    for (p = connecthead; p >= 0; p = connectpoint2[p]) {
        show2dsprite[i >> 3] |= 1 << (i & 7);
        sprite[i].cstat = 1;
        sprite[i].shade = -16;
        sprite[i].owner = i;
        sprite[i].yvel = p;
        ps[p].palette = palette;
        if (palette != 0)
            palette++;
        else
            palette = 3;
        sprite[i].xrepeat = sprite[i].yrepeat = 42;
        ps[p].i = i;
        if (numplayers > 1) {
            ps[p].oposx = ps[p].posx = sprite[i].x;
            ps[p].oposy = ps[p].posy = sprite[i].y;
            ps[p].oposz = ps[p].posz = sprite[i].z;
            ps[p].oang = ps[p].ang = sprite[i].ang;
            ps[p].ocursectnum = ps[p].cursectnum = sprite[i].sectnum;
        }
        updatesector(sprite[i].x, sprite[i].y, &ps[p].cursectnum);
        i = nextspritestat[i];
    }
    ps[myconnectindex].music = music_files[ps[myconnectindex].volume][0] + 13 * ps[myconnectindex].level;
    playmusic(ps[myconnectindex].music);
    displaystatus(myconnectindex);
    lockclock = totalclock = 0;
    level_tick_state = script_tics = 0;
    ps[myconnectindex].gm = mode;
}

unsigned char check_file_id(void)
{
    int file;
    long expected, actual;

    file = open("FILE_ID.DIZ", O_RDWR | O_BINARY, S_IREAD);
    if (file == -1)
        return 1;

    read(file, tempbuf, 450);
    actual = 0;
    for (expected = 0; expected < 400; expected++, actual++) {
        if (file_id_record[expected] == '*') {
            expected++;
            actual += 2;
        }
        if (tempbuf[actual] != file_id_record[expected]) {
            close(file);
            return 1;
        }
    }
    close(file);
    bootleg_cache_pop();
    return 0;
}


short newscore(void)
{
    short i, j;

    next_text_shade();
    for (i = 0; i < 12; i++) {
        if (ps[myconnectindex].score > (unsigned long)high_scores[i]) {
            for (j = 10; j > i; j--) {
                high_scores[j + 1] = high_scores[j];
                memcpy(score_names[j + 1], score_names[j], 31);
            }
            score_names[i][0] = 0;
            high_scores[i] = ps[myconnectindex].score;
            return i;
        }
    }
    return -1;
}

unsigned char getscorename(short row)
{
    return strget(50, row * 11 + 50, next_text_shade(), 31) != 0;
}
