#ifndef GAME2_GAME_H
#define GAME2_GAME_H

void inittimer(void);
void uninittimer(void);
void update_timer_input(void);
void level_load_hook(void);
long next_text_shade(void);
void draw_weapon_tile(short x, short y, short tile, unsigned char shade,
                      unsigned char flags);
long gametext(long x, long y, unsigned char *text, unsigned char shade);
long minitext(long x, long y, unsigned char *text, unsigned char shade);
void numtext_relative(long x, long y, long value, unsigned char shade);
int strget(long x, long y, long shade, long limit);
short findlocator(short tag, short sector);
void loadlookup(void);
void numtext(long x, long y, long value, unsigned char shade);
void numtext_fixed(long x, long y, long value, unsigned char shade);
void draw_meter(long x, long y, long units, unsigned char shade);
void draw_water_distortion(void);
void record(short player);
void update_menu_cursor(short tile);
void read_userdefs(void);
void write_userdefs(void);
void read_demo(unsigned char slot);
void write_demo(void);
unsigned char check_file_id(void);
short newscore(void);
unsigned char getscorename(short row);

void moveeffectors(void);
short spawn(long source, short tile_or_index);
void render_camera_tile(long x, long y, long z, short ang, short sect, short tile);
void render_sprite_camera(short sprite_index);
void render_mirror(short wall_index);
void resetplayer(short player);
void reset_player_motion(short player);
void prelevel(void);
void newgame(unsigned char episode, unsigned char level, unsigned char skill);
void enterlevel(unsigned char mode);

#endif
