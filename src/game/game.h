#ifndef GAME2_GAME_H
#define GAME2_GAME_H

void inittimer(void);
void uninittimer(void);
void keytimerstuff(void);
void level_load_hook(void);
void __interrupt __far timerhandler(void);
void gameexit(char *message);
void getpackets(void);
void waitforeverybody(void);
void sync(void);
long next_text_shade(void);
void operatefta(short p);
void FTA(short q);
void myos(short x, short y, short tile, unsigned char shade,
                      unsigned char flags);
long gametext(long x, long y, unsigned char *text, unsigned char shade);
long minitext(long x, long y, unsigned char *text, unsigned char shade);
void gamenumber(long x, long y, long value, unsigned char shade);
int strget(long x, long y, long shade, long limit);
short LocateTheLocator(short tag, short sector);
short EGS(short sect, long x, long y, long z, unsigned char cstat, short picnum,
          signed char shade, unsigned char xrepeat, unsigned char yrepeat,
          short ang, short xvel, short zvel, short owner, signed char stat);
void cacheit(void);
void genspriteremaps(void);
void numtext(long x, long y, long value, unsigned char shade);
void numtext_fixed(long x, long y, long value, unsigned char shade);
void scratchmarks(long x, long y, long units, unsigned char shade);
void animatesprites(short ang);
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
void saveplayer(char slot);      /* the slot arrives in AL; MENUES declares long */
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
short newscore(void);
unsigned char getscorename(short row);

#endif
