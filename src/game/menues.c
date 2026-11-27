/* MENUES: the prototype's menu pages (descriptor setup, big-font text, slider, key/mouse probe, page dispatcher), the PAUSE sign, the view window,
   and Ken Silverman's lava/boiling-slime effect (initlava/movelava, Ken-Build GAME.C; BUILD.TXT 9/29/94) placed in this object.
   Menu code is BINARY_DERIVED; Duke3D 1.5 MENUES.C (1996) is its descendant (probe, bar, menutext, menus). */

#include <stdlib.h>
#include "menues.h"

extern struct menu menu_main, menu_pause, menu_episode, menu_skill;
extern struct menu menu_load, menu_save, menu_instructions, menu_options;
extern struct menu menu_ordering, menu_scores;
extern long current_menu;
extern unsigned long syncbits[];
extern short sound_volume, music_volume, mouse_sensitivity;
extern void FX_SetVolume(long volume);
extern void MUSIC_SetVolume(long volume);
extern short menu_x, menu_y, menu_mouse, menu_drag, transition_zoom;
extern short menu_zoom;

long lavanumdrops, lavanumframes;
unsigned char lavabakpic[130 * 130];
long lavadropx[16], lavadropy[16];

/* FILE_ID.DIZ notice compared by check_file_id (2:0291c, 500-byte initialised record). Owner by
   link position (after SOUNDS _DATA, before GAMEDEF _DATA): HYPOTHESIS. */
char diz[500] =
    "ллллВБА ALERT THE SYSOP IMMEDIATELY! АБВлллл*ллллллллллллллл"
    "ллллллллллллллллллллллллллллл*THIS FILE IS NOT SHAREWARE -- "
    "it is ILLEGAL*and forbidden to upload this copyrighted*softw"
    "are to bulletin boards. If you see this*message on a BBS, pl"
    "ease request that the*Sysop remove this software IMMEDIATELY"
    "!!!*лВБА OR CONTACT APOGEE: (214) 278-5655 АБВл*IT IS ILLEGA"
    "L TO GET THIS FILE FROM A BBS!*ллллллллллллллллллллллллллллл"
    "ллллллллллллллл";

extern struct user_defs ud;         /* the 21-byte USERDEFS.TMP block defined by GLOBAL */

/* dataown: one object, one name. Code uses the fields of GLOBAL's settings block ud directly; the unused
   externs sound_volume, music_volume, mouse_sensitivity and transition_zoom above only keep this unit's
   symbol slots (docs/BSS_LAYOUT.md). */

#define SAVE_SLOTS "empty\nempty\nempty\nempty\nempty\nempty\nempty\nempty\nBack"

/* Set up the static menu descriptors (title tile, text lines, position, input flags). */
void initmenus(void)
{
    menu_main.x = 100;
    menu_main.y = 46;
    menu_main.tile = 1129;
    menu_main.lines = 8;
    menu_main.flags = 7;
    menu_main.text = "Begin a New Game\nRestore a Game\nInstructions\nGame Options\n"
        "Ordering Info\nDemo\nHigh Scores\nQuit";

    menu_pause.x = 100;
    menu_pause.y = 40;
    menu_pause.tile = 1129;
    menu_pause.lines = 6;
    menu_pause.text = "Return to Game\nHow to Play Me\nSave a Game\nRestore a Game\n"
        "Game Options\nQuit to Main Menu";
    menu_pause.flags = 7;

    menu_episode.x = 100;
    menu_episode.y = 40;
    menu_episode.tile = 616;
    menu_episode.lines = 5;
    menu_episode.text = "Mr. Caliber\nMission Cockroach\nSuck Hole\nHard Landing\nBack";
    menu_episode.flags = 7;

    menu_skill.x = 100;
    menu_skill.y = 40;
    menu_skill.text = "Easy\nMedium\nHard\nHarder\nBack";
    menu_skill.tile = 617;
    menu_skill.lines = 5;
    menu_skill.flags = 7;

    menu_load.x = 100;
    menu_load.y = 40;
    menu_load.tile = 613;
    menu_load.lines = 9;
    menu_load.text = SAVE_SLOTS;
    menu_load.flags = 7;

    menu_save.x = 100;
    menu_save.y = 40;
    menu_save.tile = 1137;
    menu_save.lines = 9;
    menu_save.text = SAVE_SLOTS;
    menu_save.flags = 7;

    menu_instructions.x = 100;
    menu_instructions.y = 40;
    menu_instructions.tile = 615;
    menu_instructions.lines = 1;
    menu_instructions.flags = 4;
    menu_instructions.text = "Back";

    menu_options.x = 80;
    menu_options.y = 40;
    menu_options.flags = 7;
    menu_options.tile = 1128;
    menu_options.lines = 5;
    menu_options.text = "Sound Volume\nMusic Volume\nMouse Sensitivity\nJoystick\nBack";

    menu_ordering.x = 100;
    menu_ordering.y = 40;
    menu_ordering.tile = 1127;
    menu_ordering.text = "Back";
    menu_ordering.lines = 1;
    menu_ordering.flags = 4;

    menu_scores.x = 100;
    menu_scores.y = 40;
    menu_scores.tile = 1138;
    menu_scores.text = "\0";
    menu_scores.flags = 7;
    menu_scores.lines = 0;
}



long lavaradcnt[32], lavaradx[32][128], lavarady[32][128];

/* Draw and edit one options slider (32 notches, value 0..255) at x,y. */
void draw_menu_slider(short x, short y, short *value, short shade)
{
    short i;

        if (menu_y > y - 2 && menu_y < y + 14)
        {
            if (keystatus[0x4b])
            {
                menu_x -= 8;
                if (menu_x < x)
                    *value = 0;
                else
                    *value = -32 + menu_x;
            }
            if (keystatus[0x4d])
            {
                menu_x += 8;
                if (menu_x > x + 256)
                    *value = 255;
                else
                    *value = -32 + menu_x;
            }
            if (menu_drag)
            {
                if (menu_x > x + 256)
                    *value = 255;
                else if (menu_x < x)
                    *value = 0;
                else
                    *value = -32 + menu_x;
                shade = 0;
            }
        }
        for (i = 0; i < 32; i++)
            if (i <= (*value >> 3))
                overwritesprite(x + i * 8, y, 678, (short)shade, 0, 0);
            else
                overwritesprite(x + i * 8, y + 4, 679, (short)shade, 0, 0);
}


/* Draw menu item text (big font) with the pointer line pulsing; returns the x after the last glyph. */
long menutext(long x, long y, unsigned char *t, long s,
    short cursor)
{
    /* long s and this local order: COMPILER_STEERED (inlined copies in menus; menutext itself is
       STRICT with either short or long s). */
    unsigned char shade;
    short start_x;
    short base_shade;
    short ac;

    if (ps[0].dimensionmode == 2) return 0;
    start_x = x;
    base_shade = (unsigned char)s;
        while (*t) {
            if (*t == '\n') {
                x = start_x;
                y += 16;
                ++t;
                continue;
            }
            if (klabs(y - cursor + 8) < 8) {
                shade = next_text_shade();
                if (menu_drag) shade -= 4;
            }
            else shade = base_shade;
            if (*t == ' ') {
                x += 5;
                ++t;
                continue;
            }
            if (*t >= '0' && *t <= '9') ac = *t + 101;
            if (*t >= 'a' && *t <= 'z') ac = *t + 26;
            if (*t >= 'A' && *t <= 'Z') ac = *t + 32;
            overwritesprite(x, y, ac, (unsigned char)shade, 0,
                sector[ps[screenpeek].cursectnum].floorpal);
            x += tilesizx[ac] + 1;
            ++t;
        }
    return x;
}

#define PRESSED_UP    (keystatus[0x48] | keystatus[0xc8])
#define PRESSED_DOWN  (keystatus[0x50] | keystatus[0xd0])
#define MOVED_CURSOR  (keystatus[0x48] | keystatus[0x50] |                      \
                       keystatus[0xc8] | keystatus[0xd0])

/* The descriptor flags use the original bitwise OR tests. */

long lavadropsizlookup[16];
unsigned char lavainc[128];

/* Read menu keys/mouse for page m: -1 quit (Q), -2 back (Esc), n>0 line picked, 0 nothing. */
long probe(struct menu m)
{
        {
            if (m.flags | 4)
            {
                if (keystatus[0x10] || menu_mouse == 2)
                {
                    keystatus[0x10] = 0;
                    return -1;
                }
                if (keystatus[0x01] || menu_mouse == 2)
                {
                    keystatus[0x01] = 0;
                    return -2;
                }
            }
            if (m.flags | 2)
            {
                if (keystatus[0x1c] || keystatus[0x39] || menu_mouse == 1)
                {
                    keystatus[0x1c] = 0;
                    keystatus[0x39] = 0;
                    return ((long)menu_y - m.y) / 16 + 1;
                }
            }
            if (m.flags | 1)
            {
                if (PRESSED_UP) menu_y -= 16;
                if (PRESSED_DOWN) menu_y += 16;
                if (MOVED_CURSOR)
                {
                    keystatus[0x48] = 0;
                    keystatus[0x50] = 0;
                    keystatus[0xc8] = 0;
                    keystatus[0xd0] = 0;
                    menu_x = m.x;
                    if (menu_y < m.y + 8) menu_y = m.y + (m.lines << 4) - 8;
                    if (menu_y > m.y + (m.lines << 4) - 8) menu_y = m.y + 8;
                }
            }
            return 0;
        }
}


/* Draw a menu page (spinning title, items) and return its probe result. */
long draw_and_probe(struct menu m, unsigned char shade)
{
        if (menu_zoom < 8192 - 348) menu_zoom += 348; else menu_zoom = 8192;
        rotatesprite(160L << 16, ((m).y - tilesizy[(m).tile] / 2L - 8) << 16,
            ((long)menu_zoom << 3) - 4096, menu_zoom, (m).tile);
    menutext(m.x, m.y, m.text, shade, menu_y);
    return probe(m);
}

long lavadropsiz[16];

/* Run the current menu page for one frame (pages 0,1,2,10,20,30,40,50,60,100).
   Each page ends with its own update_menu_cursor(614) call: the oracle keeps identical choice bodies of
   different pages apart (no cross-jumping), which one shared call after the switch does not reproduce. */
void menus(void)
{
    long choice;

    if (current_menu == 0 && (ps[myconnectindex].gm & MODE_GAME))
    {
        current_menu = 100;
        goto init_cursor;
    }
    if (current_menu < 128)
    {
init_cursor:
        current_menu |= 128;
        menu_mouse = 0;
        menu_x = 160;
        menu_y = 100;
        menu_zoom = 0;
    }

    switch (current_menu & 127)
    {
    case 0:
        choice = draw_and_probe(menu_main, 4);
        switch (choice)
        {
        case -2: case -1: case 8:
            ps[myconnectindex].gm = MODE_END;
            break;
        case 1: current_menu = 1; break;
        case 2: current_menu = 10; break;
        case 3: current_menu = 20; break;
        case 4: current_menu = 30; break;
        case 5: current_menu = 40; break;
        case 6: ps[myconnectindex].gm = MODE_DEMO | MODE_GAME; break;
        case 7: current_menu = 60; break;
        }
        update_menu_cursor(614);
        break;

    case 1:
        choice = draw_and_probe(menu_episode, 4);
        switch (choice)
        {
        case -2: case -1: case 5: current_menu = 0; break;
        case 1: ps[0].volume = 0; current_menu = 2; break;
        case 2: ps[0].volume = 1; current_menu = 2; break;
        case 3: ps[0].volume = 2; current_menu = 2; break;
        case 4: ps[0].volume = 3; current_menu = 2; break;
        }
        update_menu_cursor(614);
        break;

    case 2:
        choice = draw_and_probe(menu_skill, 4);
        switch (choice)
        {
        case -2: case -1: case 5: current_menu = 1; break;
        case 1:
            ps[0].skill = 0;
            newgame(ps[0].volume, 0, 0);
            enterlevel(MODE_GAME);
            break;
        case 2:
            ps[0].skill = 1;
            newgame(ps[0].volume, 0, 1);
            enterlevel(MODE_GAME);
            break;
        case 3:
            ps[0].skill = 2;
            newgame(ps[0].volume, 0, 2);
            enterlevel(MODE_GAME);
            break;
        case 4:
            ps[0].skill = 3;
            newgame(ps[0].volume, 0, 3);
            enterlevel(MODE_GAME);
            break;
        }
        update_menu_cursor(614);
        break;

    case 10:
        choice = draw_and_probe(menu_load, 4);
        switch (choice)
        {
        case -2: case -1: case 9: current_menu = 0; break;
        case 1: loadplayer(0); break;
        case 2: loadplayer(1); break;
        case 3: loadplayer(2); break;
        case 4: loadplayer(3); break;
        case 5: loadplayer(4); break;
        case 6: loadplayer(5); break;
        case 7: loadplayer(6); break;
        case 8: loadplayer(7); break;
        }
        update_menu_cursor(614);
        break;

    case 20:
        if (syncbits[0] & SB_JUMP) gametext(10, 100, "Jump", 0);
        if (syncbits[0] & SB_CROUCH) gametext(10, 115, "Duck", 0);
        if (syncbits[0] & SB_FIRE) gametext(10, 130, "Shoot", 0);
        if (syncbits[0] & SB_UNUSED3) gametext(10, 145, "Open\0\0\0", 0);
        choice = draw_and_probe(menu_instructions, 4);
        switch (choice)
        {
        case -2: case -1: case 1: current_menu = 0; break;
        }
        update_menu_cursor(614);
        break;

    case 30:
        draw_menu_slider(32, menu_options.y, &ud.sound_volume, 16);
        FX_SetVolume(ud.sound_volume);
        draw_menu_slider(32, (short)(menu_options.y + 16), &ud.music_volume, 16);
        MUSIC_SetVolume(ud.music_volume);
        draw_menu_slider(32, (short)(menu_options.y + 32), &ud.mouse_sensitivity, 16);
        choice = draw_and_probe(menu_options, 4);
        switch (choice)
        {
        case -2: case -1: case 5: current_menu = 0; break;
        }
        update_menu_cursor(614);
        break;

    case 40:
        choice = draw_and_probe(menu_ordering, 4);
        switch (choice)
        {
        case -2: case -1: case 1: current_menu = 0; break;
        }
        update_menu_cursor(614);
        break;

    case 50:
        choice = draw_and_probe(menu_save, 4);
        switch (choice)
        {
        case -2: case -1: case 9: current_menu = 0; break;
        case 1: saveplayer(0); current_menu = 0; break;
        case 2: saveplayer(1); current_menu = 0; break;
        case 3: saveplayer(2); current_menu = 0; break;
        case 4: saveplayer(3); current_menu = 0; break;
        case 5: saveplayer(4); current_menu = 0; break;
        case 6: saveplayer(5); current_menu = 0; break;
        case 7: saveplayer(6); current_menu = 0; break;
        case 8: saveplayer(7); current_menu = 0; break;
        }
        update_menu_cursor(614);
        break;

    case 60:
        choice = draw_and_probe(menu_scores, 4);
        if ((unsigned long)choice >= (unsigned long)-2)
            current_menu = 0;
        else
            displayhighscores();
        return;

    case 100:
        choice = draw_and_probe(menu_pause, 4);
        switch (choice)
        {
        case -2: case 1:
            ps[myconnectindex].gm = MODE_END;
            break;
        case -1:
            ps[myconnectindex].gm = MODE_GAME;
            break;
        case 2: current_menu = 20; break;
        case 3: current_menu = 50; break;
        case 4: current_menu = 10; break;
        case 5: current_menu = 30; break;
        case 6: ps[myconnectindex].gm = MODE_DEMO; break;
        }
        update_menu_cursor(614);
        break;

    default:
        return;
    }
}

/* Draw the spinning PAUSE sign (tile 1139) while the game is paused. */
void draw_pause_sign(void)
{
    if (ud.gamepaused < 8192 - 348)
        ud.gamepaused += 348;
    else
        ud.gamepaused = 8192;

    rotatesprite(160L << 16, (100L - tilesizy[1139] / 2L - 8) << 16,
        ((long)ud.gamepaused << 3) - 4096, ud.gamepaused, 1139);
}

/* Apply player snum's screen size to the view window (setview with a screen_size margin). */
void vscrn(short snum)
{
    long margin = ps[snum].screen_size;

    if (margin < 0) margin = 0;
    if (snum == screenpeek)
        setview(margin, margin, 319 - margin, 199 - margin);
}

/* Ken Silverman's lava/water effect (Ken-Build GAME.C initlava/movelava; BUILD.TXT 9/29/94). LameDuke uses
   at most 16 drops and the palette base 64 where Ken's June-1995 GAME.C has 32 drops and 192. */
#define LAVASIZ 128
#define LAVALOGSIZ 7
#define LAVAMAXDROPS 16

/* Ken's lava/slime effect: precompute drop rings and the per-row wave ramp (K95 GAME.C initlava). */
void initlava(void)
{
    long x, y, z, r;

    for (x = -16; x <= 16; x++)
        for (y = -16; y <= 16; y++)
        {
            r = ksqrt(x * x + y * y);
            lavaradx[r][lavaradcnt[r]] = x;
            lavarady[r][lavaradcnt[r]] = y;
            lavaradcnt[r]++;
        }

    for (z = 0; z < 16; z++)
        lavadropsizlookup[z] = 8 / (ksqrt(z) + 1);

    for (z = 0; z < LAVASIZ; z++)
        lavainc[z] = klabs((((z ^ 17) >> 4) & 7) - 4) + 12;

    lavanumdrops = 0;
    lavanumframes = 0;
}

/* Ken's lava/slime effect: one animation step of the 128x128 tile dapic (K95 GAME.C movelava). */
void movelava(unsigned char *dapic)
{
    unsigned char dat, *ptr;
    long x, y, z, zx, dalavadropsiz, dadropsizlookup, offs, offs2;
    long dalavax, dalavay;

    z = 3;
    if (lavanumdrops + z >= LAVAMAXDROPS)
        z = LAVAMAXDROPS - lavanumdrops - 1;
    while (z >= 0)
    {
        lavadropx[lavanumdrops] = (rand() & (LAVASIZ - 1));
        lavadropy[lavanumdrops] = (rand() & (LAVASIZ - 1));
        lavadropsiz[lavanumdrops] = 1;
        lavanumdrops++;
        z--;
    }

    z = lavanumdrops - 1;
    while (z >= 0)
    {
        dadropsizlookup = lavadropsizlookup[lavadropsiz[z]] * (((z & 1) << 1) - 1);
        dalavadropsiz = lavadropsiz[z];
        dalavax = lavadropx[z]; dalavay = lavadropy[z];
        for (zx = lavaradcnt[lavadropsiz[z]] - 1; zx >= 0; zx--)
        {
            offs = (((lavaradx[dalavadropsiz][zx] + dalavax) & (LAVASIZ - 1)) << LAVALOGSIZ);
            offs += ((lavarady[dalavadropsiz][zx] + dalavay) & (LAVASIZ - 1));
            dapic[offs] += dadropsizlookup;
            if (dapic[offs] < 64) dapic[offs] = 64;
        }

        lavadropsiz[z]++;
        if (lavadropsiz[z] > 10)
        {
            lavanumdrops--;
            lavadropx[z] = lavadropx[lavanumdrops];
            lavadropy[z] = lavadropy[lavanumdrops];
            lavadropsiz[z] = lavadropsiz[lavanumdrops];
        }
        z--;
    }

        /* Back up dapic with 1 pixel extra on each boundary
           (to prevent anding for wrap-around) */
    offs = ((long)dapic);
    offs2 = (LAVASIZ + 2) + 1 + ((long)lavabakpic);
    for (x = 0; x < LAVASIZ; x++)
    {
        copybuf((void *)offs, (void *)offs2, LAVASIZ >> 2);
        offs += LAVASIZ;
        offs2 += LAVASIZ + 2;
    }
    for (y = 0; y < LAVASIZ; y++)
    {
        lavabakpic[y + 1] = dapic[y + ((LAVASIZ - 1) << LAVALOGSIZ)];
        lavabakpic[y + 1 + (LAVASIZ + 1) * (LAVASIZ + 2)] = dapic[y];
    }
    for (x = 0; x < LAVASIZ; x++)
    {
        lavabakpic[(x + 1) * (LAVASIZ + 2)] = dapic[(x << LAVALOGSIZ) + (LAVASIZ - 1)];
        lavabakpic[(x + 1) * (LAVASIZ + 2) + (LAVASIZ + 1)] = dapic[x << LAVALOGSIZ];
    }
    lavabakpic[0] = dapic[LAVASIZ * LAVASIZ - 1];
    lavabakpic[LAVASIZ + 1] = dapic[LAVASIZ * (LAVASIZ - 1)];
    lavabakpic[(LAVASIZ + 2) * (LAVASIZ + 1)] = dapic[LAVASIZ - 1];
    lavabakpic[(LAVASIZ + 2) * (LAVASIZ + 2) - 1] = dapic[0];

    for (z = (LAVASIZ + 2) * (LAVASIZ + 2) - 4; z >= 0; z -= 4)
    {
        lavabakpic[z + 0] &= 31;
        lavabakpic[z + 1] &= 31;
        lavabakpic[z + 2] &= 31;
        lavabakpic[z + 3] &= 31;
    }

    for (x = LAVASIZ - 1; x >= 0; x--)
    {
        offs = (x + 1) * (LAVASIZ + 2) + 1;
        ptr = (unsigned char *)((x << LAVALOGSIZ) + (long)dapic);

        zx = ((x + lavanumframes) & (LAVASIZ - 1));

        offs2 = LAVASIZ - 1;
        for (y = offs; y < offs + LAVASIZ; y++)
        {
            dat = lavainc[(offs2--) & zx];
            dat += lavabakpic[y - (LAVASIZ + 2) - 1];
            dat += lavabakpic[y - (LAVASIZ + 2)];
            dat += lavabakpic[y - (LAVASIZ + 2) + 1];
            dat += lavabakpic[y - 1];
            dat += lavabakpic[y + 1];
            dat += lavabakpic[y + (LAVASIZ + 2)];
            dat += lavabakpic[y + (LAVASIZ + 2) - 1];
            *ptr++ = (dat >> 3) + 64;
        }
    }

    lavanumframes++;
}

/* Reset the parallaxing sky (pskyoff[0]=0, pskybits=3). */
void resetparallax(void)
{
    pskyoff[0] = 0;
    pskybits = 3;
}
