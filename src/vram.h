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
#ifdef SDL_GL
#include <GL/gl.h>

typedef struct GLTEX {
  GLubyte *bmp;
  GLuint id;
}GLTEX;

GLTEX gl_tex;

#endif

#ifdef DEBUG
#define DEBUG_SCR_X 300
#define DEBUG_SCR_Y 300
#endif

#define SCREEN_X 160
#define SCREEN_Y 144
#define BIT_PER_PIXEL 16


int scxoff,scyoff;		/* shift of the screen */

extern UINT16 grey[4];
extern UINT8 pal_bck[4];
extern UINT8 pal_obj[2][4];

extern UINT16 pal_col_bck_gb[8][4]; // 0BBBBBGGGGGRRRRR
extern UINT16 pal_col_obj_gb[8][4];
extern UINT16 pal_col_bck[8][4];    // RRRRRGGGGG1BBBBB avec filtre
extern UINT16 pal_col_obj[8][4];
extern UINT16 Filter[32768];

extern SDL_Surface *gb_screen;

typedef struct {
  INT16 x,y;
  UINT8 xoff,yoff;
  UINT8 sizey;
  UINT8 xflip,yflip;
  UINT8 page,pal_col;
  UINT16 no_tile;
  UINT8 pal;
  UINT8 priority;
}GB_SPRITE;

GB_SPRITE gb_spr[11];
extern UINT8 nb_spr;

UINT8 rb_on;

extern UINT8 (*draw_screen)(void);
extern void (*blit_screen)(void);

void init_vram(UINT32 flag);
void close_vram(void);
void switch_fullscreen(void);
//inline void blit_screen(void);
void clear_screen(void);

void gb_set_pal(int i);

inline UINT8 get_nb_spr(void);

void reinit_vram(void);

#endif


