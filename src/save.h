#ifndef SAVE_H
#define SAVE_H

int save_ram(void);
int load_ram(void);
int save_rom_timer(void);
int load_rom_timer(void);
SDL_Surface* get_surface_of_save_state(int n);
int save_state(char *name,int n);
int load_state(char *name,int n);

void begin_save_movie(void);
void end_save_movie(void);
void play_movie(void);
void movie_add_pad(Uint8 pad);
Uint8 movie_get_next_pad(void);

#endif
