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

#ifndef _VIDEO_YUV_H_
#define _VIDEO_YUV_H_

#include "global.h"

SDL_Overlay *overlay;
SDL_Rect ov_rect;
Uint32 yuv_flag;

struct yuv{
  Uint16 y;
  Uint8  u;
  Uint8  v;
  Uint32 yuy2;
}rgb2yuv[65536];

void init_message_yuv(void);
void init_rgb2yuv_table(void);
void blit_screen_yuv(void);
void reinit_video_yuv(void);

#endif
