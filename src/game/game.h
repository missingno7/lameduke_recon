#ifndef GAME2_GAME_H
#define GAME2_GAME_H

/* Declarations shared by GAME (1:00010-11380) and PREMAP (1:11380-14fb0). */

extern void overwritesprite(long x, long y, long tile, long shade,
                            long orientation, long palette);
extern short sintable[2048];
extern char tempbuf[];
extern long frameplace;
extern short screenpeek;
extern short myconnectindex;
extern short tilesizx[4096];
extern long reccnt;
extern signed char player_avel[], player_vel[], player_svel[];
extern long player_bits[];
extern unsigned char script_tics;
#pragma pack(1)
struct user_settings {
    unsigned char visibility, quotes, brightness, sound, music;
    short debug, sound_volume, music_volume, paused, camera, mouse_sensitivity;
    unsigned char reserved[4];
};
#pragma pack()

typedef char user_settings_size_check[sizeof(struct user_settings) == 21 ? 1 : -1];

extern struct user_settings userdefs;
extern char score_names[12][33];
extern long high_scores[12], demomode;

/* The packed, 176-byte player record. Field names follow src/game/player.h; GAME's own
   names replace PLAYER's placeholders where GAME shows the meaning (mapzoom, quote_time and
   quote_index, parallax_sector, music, palette and the end-of-level tallies). */
#pragma pack(1)
struct player_view {
    short i;
    long display_mode, mapzoom;
    long posx, posy, posz, horiz;
    long previous_x, previous_y, previous_z, poszv;
    long oposx, oposy, oposz;
    unsigned long score;
    long bobcounter;
    short ang, oang, previous_ang, cursectnum, ocursectnum, lookang;
    short screen_size, ammo, batteryammo, remote_ammo, rocket_ammo, health;
    short newowner, flash_sprite, jumping_counter, airleft, last_weapon;
    short quote_time[13];
    short quote_index, kickback_pic, curr_weapon, state80, state82, parallax_sector;
    char *music;
    unsigned char gm, weapon_clock, skill, level, volume;
    unsigned char on_ground, remote_active, knee_sequence, state92;
    unsigned char scuba_on, jetpack_on, state95, armor;
    unsigned char inventory[5], state9c[5];
    unsigned char weapon_pos, statea2, raise_frame, knee_flash, footstep;
    unsigned char statea6, palette, hard_landing, max_secret_rooms, secret_rooms;
    unsigned char muzzle_flash, max_squashed, actors_squashed, max_actors_killed, actors_killed;
};
#pragma pack()

typedef char player_size_check[sizeof(struct player_view) == 176 ? 1 : -1];

extern struct player_view ps[17];
extern long hittype[4096][5], msx[1024], msy[1024], mspos;
extern long *actorscrptr[4096];
extern short connecthead, connectpoint2[], randomstash;
extern short camerasprite, viewscreen, gamepaused, numplayers, numsectors, numwalls;
extern unsigned char show2dsprite[512], show2dwall[1024];
extern unsigned char parallaxtype, visibility, option[];
extern long waloff[4096], totalclock, lockclock, randomseed;
extern short numanimwalls, tilesizy[4096];
extern long krand(void);
extern long ksqrt(long square);
extern long getangle(long x, long y);
extern long setsprite(short i, long x, long y, long z);
extern int deletesprite(short i);
extern int changespritestat(short i, short stat);
extern void updatesector(long x, long y, short *sect);
extern short nextsectorneighborz(short sect, long z, short plane, short direction);
extern void drawrooms(long x, long y, long z, long ang, long horizon, long sect);
extern void drawmasks(void);
extern void vscrn(short p);
extern void setgamemode(void);
extern void loadtile(long tile);
extern void nextpage(void);
extern void FX_StopAllSounds(void);
extern int loadboard(char *name, long *x, long *y, long *z, short *ang, short *sect);
extern signed char pending_svel[17], pending_vel[17], pending_avel[17];
extern long pending_bits[17];

/* Inline Build helpers from the historical PRAGMAS.H. */
extern long ksgn(long value);
#pragma aux ksgn = "add ebx,0x80000000" "sbb eax,eax" "add ebx,0x7fffffff" "adc eax,0" \
    parm nomemory [ebx] modify exact [eax ebx];
extern long klabs(long value);
#pragma aux klabs = "test eax,eax" "jns skipnegate" "neg eax" "skipnegate:" \
    parm nomemory [eax];
extern void copybuf(void *source, void *dest, long count);
/* Ken's 1994 copybuf (BUIL0422) has no modify clause: the callers reload their loop state. */
#pragma aux copybuf = "rep movsd" parm [esi] [edi] [ecx];
struct animwall_entry {
    short wallnum;
    long tag;
};
extern struct animwall_entry animwall[256];

/* dataown: one object, one name. GLOBAL defines the timer ramps as avel/vel/svel and the 21-byte settings
   block as userdefs; the separate spellings used here are its fields. */
extern long avel, vel, svel;
#define turn_velocity avel
#define strafe_velocity vel
#define forward_velocity svel
#define disable_armor userdefs.debug
#define gamepaused userdefs.paused
#define camerasprite userdefs.camera
#define facing_sprite userdefs.camera

/* GAME, 1:00010-11380 */
void inittimer(void);
void uninittimer(void);
void update_timer_input(void);
void level_load_hook(void);
void __interrupt __far timerhandler(void);
void gameexit(char *message);
void domovethings_multiplayer(void);
void moveloop(void);
void network_move_loop(void);
long next_text_shade(void);
void displayquotes(short p);
void FTA(short q);
void draw_weapon_tile(short x, short y, short tile, unsigned char shade,
                      unsigned char flags);
long gametext(long x, long y, unsigned char *text, unsigned char shade);
long minitext(long x, long y, unsigned char *text, unsigned char shade);
void numtext_relative(long x, long y, long value, unsigned char shade);
int strget(long x, long y, long shade, long limit);
short findlocator(short tag, short sector);
short EGS(short sect, long x, long y, long z, unsigned char cstat, short picnum,
          signed char shade, unsigned char xrepeat, unsigned char yrepeat,
          short ang, short xvel, short zvel, short owner, signed char stat);
void cacheit(void);
void loadlookup(void);
void numtext(long x, long y, long value, unsigned char shade);
void numtext_fixed(long x, long y, long value, unsigned char shade);
void draw_meter(long x, long y, long units, unsigned char shade);
void animatesprites(short ang);
void draw_water_distortion(void);
void displayrooms(short p);
void nonsharedkeys(void);
void show_help(char **argv);
void checkcommandline(int argc, char **argv);
void wait_for_players(void);
void displaystatus(short snum);
void playback_demo(void);
void record(short player);
void domovethings(void);
void dobonus(void);
void draw_monitor_overlay(void);
void update_menu_cursor(short tile);
void displayhighscores(void);
void spawn_random_effects(short i, short count);
void move_status9(void);
void read_userdefs(void);
void write_userdefs(void);
void saveplayer(char slot);      /* the slot arrives in AL; MENUES declares long */
void loadplayer(char slot);
void read_demo(unsigned char slot);
void write_demo(void);
void moveeffectors(void);
void spawn(long source, short tile_or_index);

/* PREMAP, 1:11380-14fb0 (file name HYPOTHESIS) */
void render_camera_tile(long x, long y, long z, short ang, short sect, short tile);
void render_sprite_camera(short sprite_index);
void render_mirror(short wall_index);
void resetplayer(short player);
void reset_player_motion(short player);
void prelevel(void);
void newgame(unsigned char episode, unsigned char level, unsigned char skill);
void enterlevel(unsigned char mode);
unsigned char check_file_id(void);
short newscore(void);
unsigned char getscorename(short row);

#endif
