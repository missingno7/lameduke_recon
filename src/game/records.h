/* RECORDS.H: the record layouts shared by every game unit and GLOBAL (one definition each).

   Only struct tags live here: tags, member names and macros take no Watcom 9.5b symbol slot
   (docs/BSS_LAYOUT.md rule 1), so including this header anywhere leaves every unit's _BSS layout alone.
   Declarations that do take slots (typedefs, externs, prototypes) stay in the unit headers, in their
   historical positions. The original header name is unknown; Duke Nukem 3D 1.5 keeps the player and
   user records in DUKE3D.H (struct player_struct, struct user_defs). */

#ifndef RECORDS_H
#define RECORDS_H

#pragma pack(1)

/* Build map v5 records (map version 5 as loaded by loadboard). Field names and types follow
   Ken Silverman's BUILD.H of October 1994 (src/engine/BUILD.H), including nextsector1/nextwall1. */

struct sectortype {                     /* 37 bytes */
    unsigned short wallptr, wallnum;    /* first wall, number of walls */
    short ceilingpicnum, floorpicnum;
    short ceilingheinum, floorheinum;   /* slopes (0 = flat) */
    long ceilingz, floorz;              /* z grows downward */
    signed char ceilingshade, floorshade;
    unsigned char ceilingxpanning, floorxpanning;
    unsigned char ceilingypanning, floorypanning;
    unsigned char ceilingstat, floorstat;
    unsigned char ceilingpal, floorpal;
    unsigned char visibility;
    short lotag, hitag;
    short extra;                        /* game: door/lift speed (default 256) */
};

struct walltype {                       /* 35 bytes */
    long x, y;
    short point2;                       /* next wall of the loop */
    short picnum, overpicnum;
    signed char shade;
    short cstat;
    unsigned char xrepeat, yrepeat, xpanning, ypanning;
    short nextsector1, nextwall1;       /* first neighbour (Ken's v5 spelling) */
    short nextsector2, nextwall2;       /* second neighbour */
    short lotag, hitag;
    short extra;
};

struct spritetype {                     /* 38 bytes */
    long x, y, z;
    unsigned char cstat;
    signed char shade;
    unsigned char xrepeat, yrepeat;
    short picnum, ang, xvel, yvel, zvel, owner;
    short sectnum, statnum;
    short lotag, hitag;
    short extra;                        /* game: strength */
};

/* The player record, ps[17] (2:88830, 176 bytes). Names follow Duke Nukem 3D 1.5's struct player_struct
   and Ken Silverman's Build GAME.C arrays where the use is the same (docs/RENAMES.md Priority 3);
   start* are HYPOTHESIS names for the respawn point (Duke 1.5 keeps it in po[]). Meanings are
   binary-derived (config/semantics.json "structs"). */
struct player_struct {
    short i;                            /* 00 APLAYER sprite (statnum 10) */
    long dimensionmode;                 /* 02 2 = 2D map, 3 = 3D view (Ken) */
    long zoom;                          /* 06 2D map zoom, 48..4096, reset 768 (Ken) */
    long posx, posy, posz;              /* 0a eye position; player sprite z = posz + (36 << 8) */
    long horiz;                         /* 16 view pitch, 100 = level, 0..199 */
    long oposx, oposy, oposz;           /* 1a previous-tick position */
    long poszv;                         /* 26 vertical velocity (Ken's hvel) */
    long startposx, startposy, startposz; /* 2a respawn point (set at level start) */
    unsigned long score;                /* 36 */
    long bobcounter;                    /* 3a walk bob phase */
    short ang;                          /* 3e view angle 0..2047 */
    short startang;                     /* 40 respawn angle */
    short oang;                         /* 42 previous-tick angle */
    short cursectnum;                   /* 44 */
    short startsectnum;                 /* 46 respawn sector */
    short look_ang;                     /* 48 look left/right offset, decays by 1/3 per tick */
    short screen_size;                  /* 4a view border -4..64; >= 0 draws the status bar */
    short ammo;                         /* 4c pistol (SHOTSPARK1) */
    short batteryammo;                  /* 4e BULLET gun and club zaps */
    short remote_ammo;                  /* 50 hand bombs (HEAVYHBOMB) */
    short rocket_ammo;                  /* 52 RPG */
    short health;                       /* 54 */
    short newowner;                     /* 56 camera sprite being viewed, -1 = none */
    short holoduke_on;                  /* 58 holoduke decoy sprite, -1 reset, 0 = off */
    short jumping_counter;              /* 5a 348 at take-off, +92 per tick to 1024 */
    short airleft;                      /* 5c 1728 = full */
    short last_weapon;                  /* 5e weapon being lowered, -1 = none */
    short fta[13];                      /* 60 display timer per 'first time active' quote */
    short ftq;                          /* 7a quote on screen */
    short kickback_pic;                 /* 7c weapon animation frame, 0 = idle */
    short curr_weapon;                  /* 7e weapon as a TILE number (CLUB, SHOTSPARK1, ...) */
    short somethingonplayer;            /* 80 GREENSLIME on the player's face, -1 = none */
    short on_crane;                     /* 82 CRANE carrying the player, -1 = none */
    short one_parallax_sectnum;         /* 84 first sky sector (ps[0] only) */
    char *music;                        /* 86 song file name playing */
    unsigned char gm;                   /* 8a MODE_* bits */
    unsigned char weapon_clock;         /* 8b low byte of lockclock at the last weapon frame */
    unsigned char skill;                /* 8c */
    unsigned char level;                /* 8d level within the episode */
    unsigned char volume;               /* 8e episode */
    unsigned char on_ground;            /* 8f */
    unsigned char hbomb_on;             /* 90 thrown hand bomb armed */
    unsigned char knee_sequence;        /* 91 club swing row (krand()%5, 5 = zap) */
    unsigned char show_inventory;       /* 92 inventory bar visible (I) */
    unsigned char scuba_on;             /* 93 */
    unsigned char jetpack_on;           /* 94 1..11 lift-off ramp */
    unsigned char steroids_on;          /* 95 countdown from 128 */
    unsigned char shield_amount;        /* 96 0 or 32; hits cost 4 of it instead of health */
    unsigned char inventory[5];         /* 97 INV_SCUBA, INV_JETPACK, INV_STEROIDS, INV_SHIELD, INV_HOLODUKE */
    unsigned char gotweapon[5];         /* 9c CLUB, SHOTSPARK1, BULLET, HEAVYHBOMB, RPG */
    unsigned char weapon_pos;           /* a1 0..6 (6 = up), bit 7 = raising */
    unsigned char toggle_key_flag;      /* a2 open/use key latch */
    unsigned char knuckle_incs;         /* a3 knuckle-crack frame counter */
    unsigned char knee_flash;           /* a4 club zap counter */
    unsigned char walking_snd_toggle;   /* a5 footstep latch per bob cycle */
    unsigned char interface_toggle_flag; /* a6 one-shot key latch */
    unsigned char palookup;             /* a7 player palette */
    unsigned char hard_landing;         /* a8 landing countdown */
    unsigned char max_secret_rooms;     /* a9 */
    unsigned char secret_rooms;         /* aa */
    unsigned char shooting;             /* ab fired this tick (CON ifp pshooting) */
    unsigned char max_squashed;         /* ac bonus-screen tallies */
    unsigned char actors_squashed;      /* ad */
    unsigned char max_actors_killed;    /* ae */
    unsigned char actors_killed;        /* af */
};

/* ps[].gm bits (the values of Duke Nukem 3D 1.5 DUKE3D.H). */
#define MODE_MENU 1                     /* menu open */
#define MODE_DEMO 2                     /* back to the attract demos */
#define MODE_GAME 4                     /* game running */
#define MODE_EOL 8                      /* end of level */
#define MODE_TYPE 16                    /* typing a chat message */
#define MODE_END 128                    /* leave the game loop */

/* Sprite status lists (statnum), as used by the game units. */
#define STAT_DEFAULT 0                  /* inactive map sprites, settled debris */
#define STAT_ACTOR 1                    /* active actors (moveactors) */
#define STAT_ZOMBIEACTOR 2              /* sleeping actors waiting to see a player (movefta) */
#define STAT_EFFECTOR 3                 /* sector effectors (moveeffectors) */
#define STAT_PROJECTILE 4               /* projectiles and damage sprites (moveweapons) */
#define STAT_MISC 5                     /* explosions, smoke, blood, gibs, casings (moveexplosions) */
#define STAT_STANDABLE 6                /* standables (movestandables) */
#define STAT_LOCATOR 7                  /* LOCATORS for LocateTheLocator */
#define STAT_ACTIVATOR 8                /* ACTIVATOR / ACTIVATORLOCKED */
#define STAT_TRANSPORT 9                /* transporters (movetransports) */
#define STAT_PLAYER 10                  /* APLAYER sprites and holoduke decoys */
#define STAT_FX 11                      /* MUSICANDSFX (movefx) */

/* Input sync bits (syncbits[], recsyncbits[]); names after Duke Nukem 3D 1.5's SK_ bits where the key matches. */
#define SB_JUMP 0x1                     /* A: jump / swim up / jetpack up */
#define SB_CROUCH 0x2                   /* Z: duck / swim down / jetpack down; halves movement */
#define SB_FIRE 0x4                     /* Ctrl: fire; also ends knuckle crack; slime shake-off */
#define SB_UNUSED3 0x8                  /* Never set by getinput; menues shows 'Open' for it */
#define SB_OPEN 0x10                    /* Space: use/open (doors, switches, cameras) */
#define SB_RUN 0x20                     /* Shift (either): run (doubvel doubled), weapon sway; CON prunning */
#define SB_LOOK_LEFT 0x40               /* Keypad Ins: look_ang -= 192 */
#define SB_LOOK_RIGHT 0x80              /* Keypad Del: look_ang += 192 */
#define SB_WEAPON_1 0x100               /* 1: CLUB */
#define SB_WEAPON_2 0x200               /* 2: pistol (SHOTSPARK1) if owned */
#define SB_WEAPON_3 0x400               /* 3: BULLET gun if owned */
#define SB_WEAPON_4 0x800               /* 4: HEAVYHBOMB if owned */
#define SB_WEAPON_5 0x1000              /* 5: RPG if owned */
#define SB_LOOK_UP 0x2000               /* PgUp / keypad 7: horiz +48 (+64 with RPG/jetpack) */
#define SB_LOOK_DOWN 0x4000             /* PgDn / keypad 1: horiz -48 */
#define SB_UNUSED15 0x8000              /* 6: sampled but never read */
#define SB_KNUCKLES 0x10000             /* C: start the knuckle crack (knuckle_incs = 1 if idle) */
#define SB_INVENTORY 0x20000            /* I: toggle the inventory bar */
#define SB_CENTER_VIEW 0x40000          /* Keypad 5: return horiz toward centre */
#define SB_ENLARGE_VIEW 0x80000         /* Keypad +: bigger view (screen_size -4) / zoom in on map */
#define SB_SHRINK_VIEW 0x100000         /* Keypad - / '-': smaller view / zoom out */
#define SB_PAUSE 0x200000               /* P: toggle pause */
#define SB_MAP 0x400000                 /* Tab: toggle 2D map (dimensionmode 2/3) */
#define SB_DEBUG_DOORS 0x800000         /* U: debug only, open every door/forcefield (quote 11) */
#define SB_STEROIDS 0x1000000           /* F1: toggle steroids; without debug also opens menu page 20 (help) */
#define SB_SHIELD 0x2000000             /* F2: toggle shield (32); without debug also menu page 50 (save) */
#define SB_SCUBA 0x4000000              /* F3: toggle scuba; without debug also menu page 10 (restore) */
#define SB_HOLODUKE 0x8000000           /* F4: spawn/remove holoduke decoy */
#define SB_JETPACK 0x10000000           /* F5: toggle jetpack (JETPAKON/JETPAKOF sounds) */
#define SB_UNUSED29 0x20000000          /* F6: sampled but never read */
#define SB_HOLSTER 0x40000000           /* Enter: lower/raise weapon (not while somethingonplayer) */
#define SB_ESCAPE 0x80000000UL          /* Esc: open menu (gm |= 1); also leaves a camera via checksectors */

/* The user settings block ud (2:a57a8, 21 bytes) = the head of USERDEFS.TMP (read_userdefs/write_userdefs).
   SoundToggle (2:a57ab) and camerasprite (2:a57b5) are also bound as names of their own. */
struct user_defs {
    unsigned char visibility;           /* depth cue, V cycles 16 down to 7; default 13 */
    unsigned char quotes;               /* help quotes on (/q); Duke 1.5 ud.fta_on */
    unsigned char brightness;           /* gamma 0..4 (F10) */
    unsigned char SoundToggle;          /* S key */
    unsigned char MusicToggle;          /* M key */
    short debug;                        /* /d debug mode */
    short sound_volume;                 /* 0..255 */
    short music_volume;                 /* 0..255 */
    short gamepaused;                   /* pause flag, and the PAUSE sign zoom (0..8192) */
    short camerasprite;                 /* camera sprite viewed, -1 = none; Duke 1.5 ud.camerasprite */
    short mouse_sensitivity;            /* 0..255, default 128 */
    unsigned char reserved[4];          /* saved and loaded, never read */
};

/* One menu page descriptor (menu_main .. menu_scores). */
struct menu {
    short x, y, lines;
    short unused;
    short tile;                         /* title tile */
    unsigned char *text;
    unsigned char flags;
};

/* SOUNDS: one loaded VOC file and one playing voice (Duke 1.5 SoundOwner analogue). */
struct sound_file {
    char *data;
    long length;
};

struct sound_voice {
    long x, y, z;
    short sprite;
    long handle;
    unsigned char active;
};

/* Animated wall entry (Duke 1.5 struct animwalltype). */
struct animwalltype {
    short wallnum;
    long tag;
};

/* The Apogee Sound System FX device record filled by FX_SetupCard (FX_MAN.H's fx_device). */
struct fx_device {
    long MaxVoices, MaxSampleBits, MaxChannels;
};

#pragma pack()

#endif
