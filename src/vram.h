/*  gngb, a game boy color emulator
 *  Copyright (C) 2001 Peponas Thomas & Peponas Mathieu
 * 
 *  This program is free software; you can redistribute it and/or modify  
 *  it under the terms of the GNU General Public License as published by   
 *  the Free Software Foundation; either version 2 of the License, or    
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */


#ifndef _VRAM_H
#define _VRAM_H

#include "global.h"
#include <SDL.h>

#define SCREEN_X 160
#define SCREEN_Y 144
#define BIT_PER_PIXEL 16

#define GET_GB_PIXEL(bit) (((((*tp)&tab_ms[(bit)].mask)>>tab_ms[(bit)].shift))|((((*(tp+1))&tab_ms[(bit)].mask)>>tab_ms[(bit)].shift)<<1))
#define COL32_TO_16(col) ((((col&0xff0000)>>19)<<11)|(((col&0xFF00)>>10)<<5)|((col&0xFF)>>3))

/* blend a single 16 bit pixel at 50% */
#define BLEND16_50(d, s, mask)                                          \
        ((((s & mask) + (d & mask)) >> 1) + (s & d & (~mask & 0xffff)))

/* blend two 16 bit pixels at 50% */
#define BLEND2x16_50(d, s, mask)                                             \
        (((s & (mask | mask << 16)) >> 1) + ((d & (mask | mask << 16)) >> 1) \
         + (s & d & (~(mask | mask << 16))))


typedef struct {
  void (*init)(Uint32 flag);
  void (*reinit)(void);
  //  void (*draw_col)(void);
  //  void (*draw_wb)(void);
  //  void (*draw_sgb)(void);
  void (*blit)(void);
  void (*blit_sgb_mask)(void);
  //  void (*set_pal)(int i);
  //  void (*draw_mes)(int x,int y,char *mes);
  //  void (*clear_scr)(void);
}VIDEO_MODE;

extern Uint8 back_col[170][170];

struct mask_shift {
  unsigned char mask;
  unsigned char shift;
};

extern struct mask_shift tab_ms[8];
extern Uint32 video_flag;
int scxoff,scyoff;		/* shift of the screen */

extern Uint16 grey[4];
extern Uint8 pal_bck[4];
extern Uint8 pal_obj[2][4];

extern Uint16 pal_col_bck_gb[8][4]; // 0BBBBBGGGGGRRRRR
extern Uint16 pal_col_obj_gb[8][4];
extern Uint16 pal_col_bck[8][4];    // RRRRRGGGGG1BBBBB avec filtre
extern Uint16 pal_col_obj[8][4];
extern Uint16 Filter[32768];

extern SDL_Surface *gb_screen;
extern SDL_Surface *back_save;
extern VIDEO_MODE *cur_mode;

typedef struct {
  Sint16 x,y;
  Uint8 xoff,yoff;
  Uint8 sizey;
  Uint8 xflip,yflip;
  Uint8 page,pal_col;
  Uint16 no_tile;
  Uint8 pal;
  Uint8 priority;
}GB_SPRITE;

GB_SPRITE gb_spr[40];
extern Uint8 nb_spr;

Uint8 rb_on;

extern void (*draw_screen)(void);

SDL_Surface *get_mini_screenshot(void);
void save_gb_screen(void);
void init_vram(Uint32 flag);
void switch_fullscreen(void);
void blit_screen(void);
void clear_screen(void);

void gb_set_pal(int i);

__inline__ Uint8 get_nb_spr(void);

void reinit_vram(void);

void GenFilter(void);
void update_all_pal(void);
#endif


