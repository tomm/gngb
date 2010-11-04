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

#ifndef _VIDEO_STD_H_
#define _VIDEO_STD_H_

#include "vram.h"

#define RB_SIZE 5 


extern Sint8 rb_tab[2][RB_SIZE];
extern SDL_Surface *back;

Uint8 rb_shift;
SDL_Rect dstR;
SDL_Rect scrR;

void draw_screen_sgb_std(void);
void draw_screen_wb_std(void);
void draw_screen_col_std(void);
void clear_screen_std(void);

__inline__ void draw_back_sgb_std(Uint16 *buf);
__inline__ void draw_win_sgb_std(Uint16 *buf);
__inline__ void draw_spr_sgb_std(Uint16 *buf,GB_SPRITE *sp);
__inline__ void draw_obj_sgb_std(Uint16 *buf);

__inline__ void draw_back_col_std(Uint16 *buf);
__inline__ void draw_win_col_std(Uint16 *buf);
__inline__ void draw_spr_col_std(Uint16 *buf,GB_SPRITE *sp);
__inline__ void draw_obj_col_std(Uint16 *buf);

__inline__ void draw_back_std(Uint16 *buf);
__inline__ void draw_win_std(Uint16 *buf);
__inline__ void draw_spr_std(Uint16 *buf,GB_SPRITE *sp);
__inline__ void draw_obj_std(Uint16 *buf);

#endif
