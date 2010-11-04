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

#include <config.h>
#include <SDL.h>
#include "cpu.h"
#include "emu.h"
#include "vram.h"
#include "video_yuv.h"
#include "memory.h"
#include "message.h"

void init_message_yuv(void) {

}

void init_rgb2yuv_table(void)
{
  Uint32 i;
  Uint8 y,u,v,r,g,b;
  for(i=0;i<=65535;i++) {
    r=((i&0xF800)>>11)<<3;
    g=((i&0x7E0)>>5)<<2;
    b=(i&0x1F)<<3;
    y = (0.257 * r) + (0.504 * g) + (0.098 * b) + 16;
    u = (0.439 * r) - (0.368 * g) - (0.071 * b) + 128;
    v =-(0.148 * r) - (0.291 * g) + (0.439 * b) + 128;
    rgb2yuv[i].y=(y<<8)|y;
    rgb2yuv[i].u=u;
    rgb2yuv[i].v=v;
#ifndef WORDS_BIGENDIAN
    rgb2yuv[i].yuy2=(u<<24)|(y<<16)|(v<<8)|y;
#else
    rgb2yuv[i].yuy2=(y<<24)|(v<<16)|(y<<8)|u;
#endif
  }
}


void reinit_video_yuv(void){
  gb_screen=SDL_SetVideoMode(conf.res_w,conf.res_h,BIT_PER_PIXEL,yuv_flag);
  ov_rect.x=0;
  ov_rect.y=0;
  ov_rect.w=conf.res_w;
  ov_rect.h=conf.res_h;
}
