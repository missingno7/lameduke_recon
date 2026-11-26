#ifndef GAME2_GAME_H
#define GAME2_GAME_H

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
void render_camera_tile(long x, long y, long z, short ang, short sect, short tile);
void render_sprite_camera(short sprite_index);
void render_mirror(short wall_index);
void resetplayer(short player);
void reset_player_motion(short player);
void prelevel(void);
void newgame(unsigned char episode, unsigned char level, unsigned char skill);
void enterlevel(unsigned char mode);
short newscore(void);
unsigned char getscorename(short row);

#endif
