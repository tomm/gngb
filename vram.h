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
#include <SDL/SDL.h>

extern SDL_Surface *screen,*back;

extern UINT16 grey[4];
extern UINT16 pal_bck[4];
extern UINT16 pal_obj0[4];
extern UINT16 pal_obj1[4];

typedef struct {
  UINT16 gb_tp;      // 0BBBBBGGGGGRRRRR
  UINT16 alleg_tp;   // BBBBBGGGGG1RRRRR avec Filtre
}TYPE_COLOR;

extern TYPE_COLOR pal_col_bck[8][4];
extern TYPE_COLOR pal_col_obj[8][4];
extern UINT16 Filter[32768];

typedef struct {
  INT16 x,y;
  UINT8 xoff,yoff;
  UINT8 sizey;
  UINT8 xflip,yflip;
  UINT8 page,pal_col;
  UINT16 no_tile,*pal;
  UINT8 priority;
}GB_SPRITE;

GB_SPRITE gb_spr[11];
extern UINT8 nb_spr;

extern UINT8 (*draw_screen)(void);

void init_vram(UINT32 flag);
void close_vram(void);
inline void blit_screen(void);
void clear_screen(void);

inline UINT8 get_nb_spr(void);

inline void draw_tile(SDL_Surface *buf,int x,int y,int n);

#endif


