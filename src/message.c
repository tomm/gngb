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

#include <stdlib.h>
#include <stdarg.h>
#include <SDL.h>
#include <string.h>
#include "global.h"
#include "tiny_font.h"
#include "message.h"
#include "emu.h"
#include "menu.h"
#include "video_std.h"

SDL_Surface *fontbuf=NULL;

//int wl,hl,xm,ym;
static int tempo_mes;

#define BUF_ALPHA 240

char mes_buf[50];
char info_buf[50];
//extern SDL_Surface *gb_screen;
int mx_off,my_off;
int info=0;
SDL_Color fontpal[]={{214,214,214},{0,0,0}};


//void (*draw_message)(int x,int y,char *mes);


static __inline__ void draw_char(SDL_Surface *dest,int x,int y,unsigned char c) {
  static SDL_Rect font_rect,dest_rect;
  int indice=c-32;
  
  if (c<32 || c>127) return;
     
  font_rect.x=indice*wl;
  font_rect.y=0;
  font_rect.w=wl;
  font_rect.h=hl;
 
  dest_rect.x=x;
  dest_rect.y=y;
  dest_rect.w=wl;
  dest_rect.h=hl;

  SDL_BlitSurface(fontbuf,&font_rect,dest,&dest_rect);
}

void draw_message(int x,int y,char *mes) {
  int i;
      
  for(i=0;i<strlen(mes);i++)
    draw_char(back,x+i*wl,y,mes[i]);
}

void restore_message_pal(void)
{
  SDL_SetColors(fontbuf,fontpal,0,2);
  /* reset color keying */
  SDL_SetColorKey(fontbuf,0,0);
}


void init_message(void) {
  wl=tiny_font.width/(128-32);
  hl=tiny_font.height;

  fontbuf=img2surface(tiny_font);
  restore_message_pal();
  //SDL_SetColorKey(fontbuf,SDL_SRCCOLORKEY|SDL_RLEACCEL,1);
  /*
  fontbuf=SDL_CreateRGBSurfaceFrom((void*)tiny_font.pixel_data,
				   tiny_font.width,
				   tiny_font.height,
				   24,tiny_font.width*3,0xFF0000,0xFF00,0xFF,0);
  */
  //  SDL_SetAlpha(fontbuf,SDL_SRCALPHA,BUF_ALPHA);
  //draw_message=draw_message_default;

}
void set_message(const char *format,...) {
  va_list pvar;
  va_start(pvar,format);

  tempo_mes=5*60; // 5 sec
  xm=0;
 
  ym=144-hl;

  mes_buf[0]=0;
  vsprintf(mes_buf,format,pvar);
  //  SDL_SetAlpha(fontbuf,SDL_SRCALPHA,BUF_ALPHA);
  va_end(pvar);
}

void set_info(const char *format,...) {
  va_list pvar;
  va_start(pvar,format);
  info=1;
  
  info_buf[0]=0;
  vsprintf(info_buf,format,pvar);
  //  SDL_SetAlpha(fontbuf,SDL_SRCALPHA,BUF_ALPHA);
  va_end(pvar);
}

void unset_info(void) {
  info=0;
}

void update_message(void) {
 
  if (current_menu!=NULL) {
    display_menu(current_menu);
  } else {
    if (tempo_mes) {
      /*    if (tempo_mes<BUF_ALPHA) {
            SDL_SetAlpha(fontbuf,SDL_SRCALPHA,tempo_mes);
	    }*/
      tempo_mes--;
      draw_message(mx_off+xm,my_off+ym,mes_buf);
    } 
    if (info) {
      //SDL_SetAlpha(fontbuf,SDL_SRCALPHA,BUF_ALPHA);
      draw_message(mx_off,my_off,info_buf);
    }
  }
}
