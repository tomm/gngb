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
#include <SDL/SDL.h>
#include "global.h"
#include "tiny_font.c"
#include "message.h"
#include "vram.h"
#include "emu.h"
#ifdef SDL_GL
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#ifdef SDL_GL

#define FONTBUF_GL_W 128
#define FONTBUF_GL_H 128

GLubyte *gl_buf;
int fontgl_texid;

#endif

SDL_Surface *fontbuf=NULL;

static int wl,hl,xm,ym;
static int tempo_mes;

#define BUF_ALPHA 240

char mes_buf[50];

#ifdef SDL_GL
inline void draw_char_gl(int x,int y,unsigned char c) {
  int indice=c-32;
  static float u1,v1,u2,v2;
  static int x1,x2,y1,y2;


  x1=((indice*8)%FONTBUF_GL_W);
  y1=((indice*8)/FONTBUF_GL_W)*hl;
  x2=x1+wl;
  y2=y1+hl;

  u1=(float)x1/FONTBUF_GL_W;
  u2=(float)x2/FONTBUF_GL_W;
  v1=(float)y1/FONTBUF_GL_H;
  v2=(float)y2/FONTBUF_GL_H;

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D,fontgl_texid);
  
  glBegin(GL_QUADS);
  
  glTexCoord2f(u1,v1);
  glVertex2i(x,y);
  glTexCoord2f(u2,v1);
  glVertex2i(x+wl,y);
  glTexCoord2f(u2,v2);
  glVertex2i(x+wl,y+hl);
  glTexCoord2f(u1,v2);
  glVertex2i(x,y+hl);
 
  glEnd();
  
}
#endif

inline void draw_char(SDL_Surface *dest,int x,int y,unsigned char c) {
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

#ifdef SDL_GL

/*void blitl(int x1,int y1,int x2,int y2) {
  int x,y;
  for(y=0;y<hl;y++) {
  for(x=0;x<wl;x++) {
  gl_buf[((y1+y)*FONTBUF_GL_W+(x1+x))*3]=tiny_font.pixel_data[((x+x2)+(y+y2)*tiny_font.width)*3];
  gl_buf[((y1+y)*FONTBUF_GL_W+(x1+x))*3+1]=tiny_font.pixel_data[((x+x2)+(y+y2)*tiny_font.width)*3+1];
  gl_buf[((y1+y)*FONTBUF_GL_W+(x1+x))*3+2]=tiny_font.pixel_data[((x+x2)+(y+y2)*tiny_font.width)*3+2];
  }
  }
  }*/

void init_message_gl(void) {
  int x1,y1,x,y;
  int i;
  
  gl_buf=(GLubyte *)malloc(sizeof(GLubyte)*FONTBUF_GL_W*FONTBUF_GL_H*3);
  memset(gl_buf,0,sizeof(GLubyte)*FONTBUF_GL_W*FONTBUF_GL_H*3);
  
  x1=0;y1=0;
  for(i=0;i<tiny_font.width;i+=wl) {

    for(y=0;y<hl;y++) {
      for(x=0;x<wl;x++) {
	gl_buf[((y1+y)*FONTBUF_GL_W+(x1+x))*3]=tiny_font.pixel_data[((x+i)+(y)*tiny_font.width)*3];
	gl_buf[((y1+y)*FONTBUF_GL_W+(x1+x))*3+1]=tiny_font.pixel_data[((x+i)+(y)*tiny_font.width)*3+1];
	gl_buf[((y1+y)*FONTBUF_GL_W+(x1+x))*3+2]=tiny_font.pixel_data[((x+i)+(y)*tiny_font.width)*3+2];
      }
    }
    
    x1+=8;
    if (x1+wl>FONTBUF_GL_W) {
      y1+=hl;
      x1=0;
    }
  }

  glGenTextures(1,&fontgl_texid);
  glBindTexture(GL_TEXTURE_2D,fontgl_texid);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,
	       FONTBUF_GL_W,FONTBUF_GL_H,0,
	       GL_RGB,GL_UNSIGNED_BYTE,(GLvoid *)gl_buf);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
  
  free(gl_buf);
}
#endif

void init_message(void) {
  wl=tiny_font.width/(128-32);
  hl=tiny_font.height;
  fontbuf=SDL_CreateRGBSurfaceFrom((void*)tiny_font.pixel_data,
				   tiny_font.width,
				   tiny_font.height,
				   24,tiny_font.width*3,0xFF0000,0xFF00,0xFF,0);
  SDL_SetAlpha(fontbuf,SDL_SRCALPHA,BUF_ALPHA);
#ifdef SDL_GL
  if (conf.gl)
    init_message_gl();
#endif
}

#ifdef SDL_GL
void draw_message_gl(int x,int y,char *mes) {
  int i;
  if (!fontbuf) init_message();
  for(i=0;i<strlen(mes);i++)
    draw_char_gl(x+i*wl,y,mes[i]);
}
#endif

void draw_message(SDL_Surface *dest,int x,int y,char *mes) {
  int i;
  if (!fontbuf) init_message();
  for(i=0;i<strlen(mes);i++)
    draw_char(dest,x+i*wl,y,mes[i]);
}

void set_message(const char *format,...) {
  va_list pvar;
  va_start(pvar,format);

  tempo_mes=5*60; // 5 sec
  xm=0;
  ym=144-hl;

  mes_buf[0]=0;
  vsprintf(mes_buf,format,pvar);
  SDL_SetAlpha(fontbuf,SDL_SRCALPHA,BUF_ALPHA);
  va_end(pvar);
}

void update_message(void) {
  if (tempo_mes) {
    if (tempo_mes<BUF_ALPHA) {
      SDL_SetAlpha(fontbuf,SDL_SRCALPHA,tempo_mes);
    }
    tempo_mes--;
#ifdef SDL_GL
    if (conf.gl) 
      draw_message_gl(scxoff+xm,scyoff+ym,mes_buf);
    else
#endif
      draw_message(gb_screen,scxoff+xm,scyoff+ym,mes_buf);
  } 
}
