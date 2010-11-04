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

#ifndef MESSAGE_H
#define MESSAGE_H

#include <SDL.h>

int wl,hl,xm,ym;
//extern void (*draw_message)(int x,int y,char *mes);
extern SDL_Surface *fontbuf;

#define RED_MASK(b) (b==4?0xFF000000:(b==3?0xFF0000:0))
#define GREEN_MASK(b) (b==4?0xFF0000:(b==3?0xFF00:0))
#define BLUE_MASK(b) (b==4?0xFF00:(b==3?0xFF:0))
#define ALPHA_MASK(b) (b==4?0xFF:0)

#define img2surface(__a__) SDL_CreateRGBSurfaceFrom((void*)__a__.pixel_data,\
						 __a__.width,\
						 __a__.height,\
						 __a__.bytes_per_pixel*8,\
						 __a__.width*__a__.bytes_per_pixel,\
						 RED_MASK(__a__.bytes_per_pixel),\
						 GREEN_MASK(__a__.bytes_per_pixel),\
						 BLUE_MASK(__a__.bytes_per_pixel),\
						 ALPHA_MASK(__a__.bytes_per_pixel))

void init_message(void);
void set_message(const char *format,...);
void set_info(const char *format,...);
void unset_info(void);
void update_message(void);
void draw_message(int x,int y,char *mes);
void restore_message_pal(void);

#endif



