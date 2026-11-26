/* GLOBAL: the code-less object that defines the game's shared data.

   Evidence (build/workers/dataown/OWNERSHIP.md): one _BSS contribution spans 2:0ac70-a57c3 in a single
   Watcom 9.5b size-bucket sequence (8-multiples, 4-multiples, 130, 2, odd, 1) that interleaves storage used by
   GAME, SECTOR, PLAYER, ACTORS, SOUNDS, MENUES and GAMEDEF; its initialised _DATA (2:023dc-02838) follows
   PLAYER's and precedes SOUNDS'. No code-bearing unit can own both. Duke Nukem 3D 1.5 keeps the same kind of
   storage in its code-less GLOBAL.C. The file name is a HYPOTHESIS.

   Struct tags do not take symbol slots. The declaration order below is COMPILER_STEERED: it places each
   hash-ascending run of the oracle order on its own 22-slot symbol page, and real game prototypes fill the
   pages (build/workers/dataown/gen_global.py). Names marked unref_ have no reference in the executable. */

#pragma pack(1)
struct player_view {            /* 176 bytes; field names in game.c */
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

struct user_settings {          /* the 21-byte USERDEFS.TMP block */
    unsigned char visibility, quotes, brightness, sound, music;
    short debug, sound_volume, music_volume, paused, camera, mouse_sensitivity;
    unsigned char reserved[4];
};

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

struct menu {
    short x, y, lines;
    short unused;
    short tile;
    unsigned char *text;
    unsigned char flags;
};

struct animwall_entry {
    short wallnum;
    long tag;
};

struct fx_device {
    long MaxVoices, MaxSampleBits, MaxChannels;
};
#pragma pack()

void inittimer(void);
void uninittimer(void);
void keytimerstuff(void);
void level_load_hook(void);

/* symbol page 1 */
long msx[1024];
long msy[1024];
char tempbuf[1024];
struct animwall_entry animwall[256];
void gameexit(char *message);
void getpackets(void);
void waitforeverybody(void);
void sync(void);
long next_text_shade(void);
void operatefta(short p);
void FTA(short q);
long gametext(long x, long y, unsigned char *text, unsigned char shade);
long minitext(long x, long y, unsigned char *text, unsigned char shade);
void gamenumber(long x, long y, long value, unsigned char shade);
int strget(long x, long y, long shade, long limit);
short LocateTheLocator(short tag, short sector);
void cacheit(void);
void genspriteremaps(void);
void numtext(long x, long y, long value, unsigned char shade);
void numtext_fixed(long x, long y, long value, unsigned char shade);
void scratchmarks(long x, long y, long units, unsigned char shade);
void animatesprites(short ang);

/* symbol page 2 */
short animatesect[512];
long *animateptr[512];
long animategoal[512];
struct user_settings ud;
void draw_water_distortion(void);
void displayrooms(short p);
void nonsharedkeys(void);
void comlinehelp(char **argv);
void checkcommandline(int argc, char **argv);
void wait_for_players(void);
void displaystatus(short snum);
void playback(void);
void record(short player);
void domovethings(void);
void dobonus(void);
void cameratext(void);
void update_menu_cursor(short tile);
void displayhighscores(void);
void lotsofglass(short i, short count);
void movetransports(void);
void read_userdefs(void);
void write_userdefs(void);

/* symbol page 3 */
char bootleg_cache[300000];
struct player_view ps[17];
long hittype[4096][5];
unsigned char typemessage[80];
char unref_9d430[16384];
short numanimwalls;
struct menu menu_main;
void loadplayer(char slot);
void opendemoread(unsigned char slot);
void closedemowrite(void);
void moveeffectors(void);
void spawn(long source, short tile_or_index);
void render_camera_tile(long x, long y, long z, short ang, short sect, short tile);
void render_sprite_camera(short sprite_index);
void render_mirror(short wall_index);
void resetplayer(short player);
void resetprestat(short player);
void prelevel(void);
void newgame(unsigned char episode, unsigned char level, unsigned char skill);
void enterlevel(unsigned char mode);
unsigned char check_file_id(void);
short newscore(void);

/* symbol page 4 */
struct sound_file sound_file_table[39];
unsigned char music_buffer[40000];
long vel;
long svel;
long angvel;
long lockclock;
short randomstash;
short neartagsprite;
short neartagsector;
short neartagwall;
struct menu menu_episode;
unsigned char getscorename(short row);
extern void *bootleg_cache_alloc(long size);
extern void bootleg_cache_pop(void);
extern short getangle(long x, long y);
extern short ssp(short i, long cliptype);
extern long krand(void);
extern long ifhitbyweapon(short i);
extern long ifsquished(short i, short p);
extern void shoot(short i, short tile);
extern void spritesound(short sound, short i);
extern void nuke(long x, long y, long z, short i);

/* symbol page 5 */
signed char recsyncvel[16384];
long neartaghitdist;
short unref_a539e;
struct menu menu_skill;
struct menu menu_instructions;
struct menu menu_options;
struct menu unref_a5778;
extern void addweapon(short p, short tile, short amount);
extern void hitradius(short i, long radius, long damage);
extern void guts(short i);
extern int deletesprite(short i);
extern long klabs(long value);
extern void fill_dwords(void *dest, unsigned long value, unsigned long count);
short getincangle(short a, short na, short shift);
char ispecial(char c);
char isaltok(char c);
void getlabel(void);
long keyword(void);
long transword(void);
void transnum(void);
char parsecommand(void);
void passone(void);

/* symbol page 6 */
long recsyncbits[16384];
unsigned char recsynctics[16384];
long *scriptptr;
long reccnt;
long animatecnt;
long syncbits[17];
long osyncbits[17];
long recstat;
struct fx_device fx_device;
long current_menu;
short menu_drag;
short menu_x;
short menu_y;
struct menu menu_pause;
void loadefs(void);
short furthestangle(short i, short angs);
void move_actor(short i, short p, long x);
char parse(short i, short p, long x);
void execute(short i, short p, long x);
extern void stopsound(short sound);
extern void xyzsound(short sound, short i, long x, long y, long z);
extern void update_ripple_tile(long tile_address);

/* symbol page 7 */
signed char recsyncsvel[16384];
signed char recsyncangvel[16384];
long animatevel[512];
long script[6144];
char playerreadyflag[16];
unsigned char *msgtext;
short menu_mouse;
short menu_previous_buttons;
struct menu menu_load;
unsigned char typemessageleng;
extern short nextsectorneighborz(short sectnum, long z, short topbottom, short direction);
extern void updatesector(long x, long y, short *sectnum);
extern void setview(long x1, long y1, long x2, long y2);
extern void setgamemode(void);
extern void qsetmode640480(void);
extern void vscrn(short p);
short checkcursectnums(short sn);
short findplayer(short sn, long *d);
void doanimations(long tics);
short getanimationgoal(long *position);
int setanimation(short sn, long *position, long goal, long velocity);
void animatecamsprite(short p);

/* symbol page 8 */
long *actorscrptr[4096];
char output_path[80];
long *parsing_actor;
long *labelcode;
short screenpeek;
short msgtime;
short hudpulse;
short menu_zoom;
struct menu menu_ordering;
struct menu menu_save;
unsigned char level_tick_state;
unsigned char synctics;
void animatewalls(void);
void activatewarpelevators(short i, short direction);
void operatesectors(short sn, short source);
void operateactivators(short lotag);
void operatemasterswitches(short lotag);
void operateforcefields(short lotag);
void cheatkeys(short p);
void checkhitswitch(long w);
void checkhitwall(long w, long x, long y, long z, short weapon);
char checkhitceiling(short sn);

/* symbol page 9 */
char music_fn[4][10][13];
long *insptr;
char *textptr;
long labelcnt;
char *label;
char env_music_fn[10][13];
short camsprite;
signed char syncvel[17];
signed char osyncvel[17];
signed char syncsvel[17];
signed char osyncsvel[17];
signed char syncangvel[17];
signed char osyncangvel[17];
struct menu menu_scores;
struct sound_voice sound_voices[39];
unsigned char warning;
unsigned char error;
char killit_flag;
void checksectors(short p);
void allignwarpelevators(void);
extern long ksqrt(long square);
extern long FX_SetVolume(long volume);


/* Initialised shared data, 2:023dc-02838, in definition order. The score block equals the two trailing
   parts of the supplied USERDEFS.TMP. */
char score_names[12][33] = {
    "It only looks painful!",
    "It's fun to kill things.",
    "I like blood and guts.",
    "",
    "Call Toll Free.",
    "1-800-GAME123; that's a",
    "one-eight-hundred GAME 123.",
    "",
    " \"To be the best,",
    "  order the rest.\"",
    "",
    ""
};

long high_scores[12] = { 8675309, 131072, 40961, 666, 69, 4 };

char fta_quotes[13][32] = {
    "She's tougher than she looks!",
    "Go get the gun.",
    "The reactor must be destroyed!",
    "Enemy ALERT!!!",
    "The door is unlocked...",
    "The door is locked...",
    "Cola increases health.",
    "It's CRASHINGING!!!!!",
    "Shutting airlock bay doors.",
    "You found a secret room..!",
    "Pintched by the doors!",
    "ALL DOORS UNLOCKED!!!",
    "Falling to your bloody death."
};

/* US PC set-1 printable keys, with zero entries for non-text keys. */
unsigned char scantoasc[128] =
    "\0\0" "1234567890-=" "\0\0"                 /* 00-0f */
    "qwertyuiop[]" "\0\0" "as"                    /* 10-1f */
    "dfghjkl;'\x60" "\0" "\\zxcv"                  /* 20-2f */
    "bnm,./\0*\0 " "\0\0\0\0\0\0"                  /* 30-3f */
    "\0\0\0\0\0\0\0" "789-456+1"                   /* 40-4f */
    "230.";                                       /* 50-53 */

unsigned char scantoascwithshift[128] =
    "\0\0" "!@#$%^&*()_+" "\0\0"                 /* 00-0f */
    "QWERTYUIOP{}" "\0\0" "AS"                    /* 10-1f */
    "DFGHJKL:\"~" "\0" "|ZXCV"                     /* 20-2f */
    "BNM<>?\0*\0 " "\0\0\0\0\0\0"                  /* 30-3f */
    "\0\0\0\0\0\0\0" "789-456+1"                   /* 40-4f */
    "230.";                                       /* 50-53 */
