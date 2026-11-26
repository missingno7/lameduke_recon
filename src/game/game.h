#ifndef GAME2_GAME_H
#define GAME2_GAME_H

void inittimer(void);
void uninittimer(void);
void update_timer_input(void);
void level_load_hook(void);
long next_text_shade(void);
long gametext(long x, long y, unsigned char *text, unsigned char shade);
long minitext(long x, long y, unsigned char *text, unsigned char shade);
void numtext_relative(long x, long y, long value, unsigned char shade);
short findlocator(short tag, short sector);
void loadlookup(void);
void numtext(long x, long y, long value, unsigned char shade);
void numtext_fixed(long x, long y, long value, unsigned char shade);
void draw_meter(long x, long y, long units, unsigned char shade);
void draw_water_distortion(void);
void record(short player);
void read_userdefs(void);
void write_userdefs(void);
void read_demo(unsigned char slot);
void write_demo(void);
unsigned char check_file_id(void);
unsigned char getscorename(short row);

#endif
