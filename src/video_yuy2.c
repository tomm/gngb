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
#include "global.h"
#include "memory.h"
#include "video_yuv.h"
#include "vram.h"
#include "video_std.h"
#include "sgb.h"
#include "message.h"
#include "emu.h"
#include "interrupt.h"

void init_video_yuy2(Uint32 flag)
{
  yuv_flag=SDL_HWSURFACE|flag|SDL_RESIZABLE; /* YUV mode are resizable */
  
   /* we need this because YUV may not support down scale */
  if (conf.gb_type&SUPER_GAMEBOY) {
    if (conf.res_w<SGB_WIDTH*2)
      conf.res_w=SGB_WIDTH*2;
    if (conf.res_h<SGB_HEIGHT*2)
      conf.res_h=SGB_HEIGHT*2;
  }

  gb_screen=SDL_SetVideoMode(conf.res_w,conf.res_h,BIT_PER_PIXEL, yuv_flag);
  if (conf.gb_type&SUPER_GAMEBOY) {
    overlay=SDL_CreateYUVOverlay(SGB_WIDTH*2,SGB_HEIGHT,SDL_YUY2_OVERLAY,gb_screen);
  } else {
    overlay=SDL_CreateYUVOverlay(SCREEN_X*2,SCREEN_Y,SDL_YUY2_OVERLAY,gb_screen);
  }

  /* Error check */
  if (gb_screen==NULL) {
    printf("Couldn't set %dx%dx%d video mode: %s\n",
	   SCREEN_X,SCREEN_Y,BIT_PER_PIXEL,SDL_GetError());
    exit(1);
  }
  if (overlay==NULL) {
    printf("Couldn't allocate overlay surface: %s\n", SDL_GetError());
    exit(1);
  }

  ov_rect.x=0;
  ov_rect.y=0;
  ov_rect.w=conf.res_w;
  ov_rect.h=conf.res_h;
  init_rgb2yuv_table();
  // init_default_palette();
  init_message_yuv();
}

void blit_screen_yuy2()
{
  int i,j;
  Uint32 *buf=(Uint32*)overlay->pixels[0];
  Uint16 *t=back->pixels;
  
  static Uint8 rumble=0;
  static Uint8 rb_time=0;
  //  printf("%d\n",back->pitch>>1);
  
  if (conf.gb_type&SUPER_GAMEBOY) {
    buf+=(overlay->pitches[0]>>2)*40+48;
  }
  
  if (rb_on) {
    rumble=2-rumble;
    t+=rumble;
    rb_time++;
    if (rb_time>8) 
      rb_time=rb_on=0;
  }
  if (conf.gb_type&SUPER_GAMEBOY) {

    for(j=0;j<SCREEN_Y;j++){
      for(i=0;i<SCREEN_X;i++) {
	buf[i]=rgb2yuv[t[i]].yuy2;
	
      }
      buf+=overlay->pitches[0]>>2;
      t+=back->pitch>>1;
    }
	
  } else {
    for(i=0;i<SCREEN_X*SCREEN_Y;i++)
      buf[i]=rgb2yuv[t[i]].yuy2;
  }

  SDL_DisplayYUVOverlay(overlay,&ov_rect);
  if ((!(LCDCCONT&0x20) || !(LCDCCONT&0x01)) && (conf.gb_type&NORMAL_GAMEBOY)) 
    clear_screen();

}

void blit_sgb_mask_yuy2(void)
{
  int i;
  Uint32 *buf=(Uint32*)overlay->pixels[0];
  Uint16 *t=sgb_buf->pixels;
  
  for(i=0;i<SGB_WIDTH*SGB_HEIGHT;i++)
    buf[i]=rgb2yuv[t[i]].yuy2;
}


VIDEO_MODE video_yuy2={
  init_video_yuy2,
  reinit_video_yuv,
  //  draw_screen_col_yuy2,
  //  draw_screen_wb_yuy2,
  //  NULL,
  blit_screen_yuy2,
  blit_sgb_mask_yuy2
  //  NULL,
  //  draw_message_yuy2,
  //  NULL
};
