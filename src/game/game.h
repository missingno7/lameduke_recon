#ifndef GAME2_GAME_H
#define GAME2_GAME_H

void inittimer(void);
void uninittimer(void);
void update_timer_input(void);
void level_load_hook(void);
long next_text_shade(void);
short findlocator(short tag, short sector);
void loadlookup(void);
void draw_water_distortion(void);
void record(short player);
void read_userdefs(void);
void write_userdefs(void);
void read_demo(unsigned char slot);
void write_demo(void);
unsigned char check_file_id(void);
unsigned char getscorename(short row);

#endif
