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

#ifdef DEBUG
#include "gngb_debuger/debuger.h"
#endif


void init_video_yv12(Uint32 flag)
{
  yuv_flag=SDL_HWSURFACE|flag|SDL_RESIZABLE; /* YUV mode are resizable */

  /* we need this because YUV may not support down scale */
  if (conf.gb_type&SUPER_GAMEBOY) {
    if (conf.res_w<SGB_WIDTH*2)
      conf.res_w=SGB_WIDTH*2;
    if (conf.res_h<SGB_HEIGHT*2)
      conf.res_h=SGB_HEIGHT*2;
  }

  gb_screen=SDL_SetVideoMode(conf.res_w,conf.res_h,BIT_PER_PIXEL,yuv_flag);
  if (conf.gb_type&SUPER_GAMEBOY) {
    overlay=SDL_CreateYUVOverlay(SGB_WIDTH*2,SGB_HEIGHT*2,SDL_YV12_OVERLAY,gb_screen);
  } else {
    overlay=SDL_CreateYUVOverlay(SCREEN_X*2,SCREEN_Y*2,SDL_YV12_OVERLAY,gb_screen);
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
  //  init_default_palette();
  init_message_yuv();
}

void blit_screen_yv12(void)
{
    int i,j;
  static Uint8 rumble=0;
  static Uint8 rb_time=0;
  
  Uint16 *bufy=(Uint16*)overlay->pixels[0];
  Uint8 *bufu=(Uint8*)overlay->pixels[1];
  Uint8 *bufv=(Uint8*)overlay->pixels[2];
  Uint16 *nbufy=(Uint16*)overlay->pixels[0]+(overlay->pitches[0]>>1);
  Uint16 *t=back->pixels;
  
  if (conf.gb_type&SUPER_GAMEBOY) {
    bufy+=overlay->pitches[0]*40+48;
    nbufy+=overlay->pitches[0]*40+48;
    bufu+=overlay->pitches[1]*40+48;
    bufv+=overlay->pitches[2]*40+48;
  }
  
  if (rb_on) {
    rumble=2-rumble;
    t+=rumble;
    rb_time++;
    if (rb_time>8) 
      rb_time=rb_on=0;
  }
  for(j=0;j<SCREEN_Y;j++){
    for(i=0;i<SCREEN_X;i++){
      nbufy[i]=bufy[i]=rgb2yuv[t[i]].y;
      bufu[i]=rgb2yuv[t[i]].u;
      bufv[i]=rgb2yuv[t[i]].v;
      
    }
    nbufy+=overlay->pitches[0];
    bufy+=overlay->pitches[0];
    bufu+=overlay->pitches[1];
    bufv+=overlay->pitches[2];
    t=&t[i];
  }

  SDL_DisplayYUVOverlay(overlay,&ov_rect);
  if ((!(LCDCCONT&0x20) || !(LCDCCONT&0x01)) && (conf.gb_type&NORMAL_GAMEBOY)) 
    clear_screen();

}

void blit_sgb_mask_yv12(void)
{
      int i,j;

  
  Uint16 *bufy=(Uint16*)overlay->pixels[0];
  Uint8 *bufu=(Uint8*)overlay->pixels[1];
  Uint8 *bufv=(Uint8*)overlay->pixels[2];
  Uint16 *nbufy=(Uint16*)overlay->pixels[0]+(overlay->pitches[0]>>1);
  Uint16 *t=sgb_buf->pixels;
  
  for(j=0;j<SGB_HEIGHT;j++){
    for(i=0;i<SGB_WIDTH;i++){
      nbufy[i]=bufy[i]=rgb2yuv[t[i]].y;
      bufu[i]=rgb2yuv[t[i]].u;
      bufv[i]=rgb2yuv[t[i]].v;
      
    }
    nbufy+=overlay->pitches[0];
    bufy+=overlay->pitches[0];
    bufu+=overlay->pitches[1];
    bufv+=overlay->pitches[2];
    t=&t[i];
  }

}

VIDEO_MODE video_yv12={
  init_video_yv12,
  reinit_video_yuv,
  //  draw_screen_col_yv12,
  //  draw_screen_wb_yv12,
  //  NULL,
  blit_screen_yv12,
  blit_sgb_mask_yv12
  //  set_pal_yv12,
  //draw_message_yv12,
  //NULL
};
