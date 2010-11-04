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
#include "vram.h"
#include "memory.h"
#include "rom.h"
#include "interrupt.h"
#include <SDL/SDL.h>
#ifdef SDL_GL
#include <GL/gl.h>
#endif

#define SCREEN_X 160
#define SCREEN_Y 144
#define BIT_PER_PIXEL 16

INT8 rb_tab[]={0,0,-2,2,-2,2,0,0,0,0,0,0,0};
UINT8 rb_shift=0;

struct mask_shift tab_ms[8]={
  { 0x80,7 },
  { 0x40,6 },
  { 0x20,5 },
  { 0x10,4 },
  { 0x08,3 },
  { 0x04,2 },
  { 0x02,1 },
  { 0x01,0 }};

static SDL_Surface *gb_screen=NULL,*back=NULL;

SDL_Rect dstR;
SDL_Rect scrR;

UINT16 grey[4];
UINT16 pal_bck[4];
UINT16 pal_obj0[4];
UINT16 pal_obj1[4];

UINT16 pal_col_bck_gb[8][4];
UINT16 pal_col_obj_gb[8][4];
UINT16 pal_col_bck[8][4];
UINT16 pal_col_obj[8][4];
UINT16 Filter[32768];

UINT8 back_col[170][170];

UINT8 nb_spr;

UINT8 (*draw_screen)(void);

UINT8 draw_screen_col(void);
UINT8 draw_screen_wb(void);
#ifdef SDL_GL
void update_gldisp(void/*UINT16 *buf*/)
{
  static GLfloat tu=SCREEN_X/256.0,tv=SCREEN_Y/256.0;
  
  // memcpy(gl_tex.bmp,buf,SCREEN_X*SCREEN_Y*2);

  glClearColor( 0.0, 0.0, 0.0, 1.0 );
  glClear( GL_COLOR_BUFFER_BIT);
  glEnable(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D,gl_tex.id);
  glTexSubImage2D(GL_TEXTURE_2D,0,0,0,SCREEN_X,SCREEN_Y,
                  GL_RGB,GL_UNSIGNED_SHORT_5_6_5,gl_tex.bmp);

  glBegin(GL_QUADS);
  glTexCoord2f(0.0,tv);
  glVertex2i(0,0);
 
  
  glTexCoord2f(tu,tv);
  glVertex2i(SCREEN_X,0);

  glTexCoord2f(tu,0.0); 
  glVertex2i(SCREEN_X,SCREEN_Y);
  
  
  glTexCoord2f(0.0,0.0);
  glVertex2i(0,SCREEN_Y);




  

  glEnd();  
  glDisable(GL_TEXTURE_2D);
  SDL_GL_SwapBuffers( );

}

void init_gl(void)
{
  glViewport(0,0,conf.gl_w,conf.gl_h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0,(GLfloat)SCREEN_X,0.0,(GLfloat)SCREEN_Y,-1.0,1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gl_tex.bmp=(UINT8 *)malloc(256*256*2);
  glEnable(GL_TEXTURE_2D);
  
  glGenTextures(1,&gl_tex.id);
  gl_tex.id=glGenLists(1);
  glBindTexture(GL_TEXTURE_2D,gl_tex.id);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB5,256,256,0,
	       GL_RGB,GL_UNSIGNED_BYTE,NULL);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  
  
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
  
}
#endif


/* take from VGBC (not VGB) */

int GetValue(int min,int max,int v)
{
  return min+(float)(max-min)*(2.0*(v/31.0)-(v/31.0)*(v/31.0));
}

void GenFilter(void)
{
  UINT8 r,g,b;
  UINT8 nr,ng,nb;

  for (r=0;r<32;r++) {
    for (g=0;g<32;g++) {
      for (b=0;b<32;b++) {
	nr=GetValue(GetValue(4,14,g),GetValue(24,29,g),r)-4;
	ng=GetValue(GetValue(4+GetValue(0,5,r),14+GetValue(0,3,r),b),
		    GetValue(24+GetValue(0,3,r),29+GetValue(0,1,r),b),g)-4;
	nb=GetValue(GetValue(4+GetValue(0,5,r),14+GetValue(0,3,r),g),
                    GetValue(24+GetValue(0,3,r),29+GetValue(0,1,r),g),b)-4;
	Filter[(b<<10)|(g<<5)|r]=(nr<<11)|(ng<<6)|nb;
      }
    }
  }
}

void init_buffer(void)
{
  /*back=create_bitmap_ex(16,256,256);
    clear(back);*/
}

#define COL32_TO_16(col) ((((col&0xff0000)>>19)<<11)|(((col&0xFF00)>>10)<<5)|((col&0xFF)>>3))

void init_pallete(void)
{
  pal_bck[0]=grey[0]=COL32_TO_16(0xB8A68D); //0xc618; // ffe6ce
  pal_bck[1]=grey[1]=COL32_TO_16(0x917D5E); //0x8410; // bfad9a
  pal_bck[2]=grey[2]=COL32_TO_16(0x635030); //0x4208; // 7f7367
  pal_bck[3]=grey[3]=COL32_TO_16(0x211A10); //0x0000; // 3f3933

  pal_obj0[0]=grey[0];
  pal_obj0[1]=grey[1];
  pal_obj0[2]=grey[2];
  pal_obj0[3]=grey[3];

  pal_obj1[0]=grey[0];
  pal_obj1[1]=grey[1];
  pal_obj1[2]=grey[2];
  pal_obj1[3]=grey[3];

  GenFilter();
}

void free_buffer(void)
{ 
  
}  

void init_vram(UINT32 flag)
{
  if (SDL_Init(SDL_INIT_JOYSTICK|SDL_INIT_VIDEO|((conf.sound)?SDL_INIT_AUDIO:0))<0) {
    printf("Couldn't initialize SDL: %s\n",SDL_GetError());
    exit(1);
  }
  atexit(SDL_Quit);   

#ifdef SDL_GL
  if (conf.gl) 
    gb_screen=SDL_SetVideoMode(conf.gl_w,conf.gl_h,BIT_PER_PIXEL,SDL_HWSURFACE|flag);
  else 
#endif
    gb_screen=SDL_SetVideoMode(SCREEN_X,SCREEN_Y,BIT_PER_PIXEL,SDL_HWSURFACE|flag);
  back=SDL_CreateRGBSurface(SDL_HWSURFACE,SCREEN_X,SCREEN_Y+1,BIT_PER_PIXEL,
			    0xf800,0x7e0,0x1f,0x00);
  if (gb_screen==NULL) {
    printf("Couldn't set %dx%dx%d video mode: %s\n",
	   SCREEN_X,SCREEN_Y,BIT_PER_PIXEL,SDL_GetError());
    exit(1);
  }
  if (back==NULL) {
    printf("Couldn't allocate %dx%dx%d SDL_Surface: %s\n",
	   SCREEN_X,SCREEN_Y,BIT_PER_PIXEL,SDL_GetError());
    exit(1);
  }

  SDL_WM_SetCaption("Gngb",NULL);
  SDL_ShowCursor(0);

#ifdef SDL_GL
  init_gl();
#endif

  init_buffer();
  init_pallete();
#ifdef SDL_GL
  if (conf.gl) {
    if (gameboy_type&COLOR_GAMEBOY) draw_screen=draw_screen_gl_col;
    else draw_screen=draw_screen_gl_wb;
  } else {
#endif
    if (gameboy_type&COLOR_GAMEBOY) draw_screen=draw_screen_col;
    else draw_screen=draw_screen_wb;
#ifdef SDL_GL
  }
#endif
}

void switch_fullscreen(void) {
  SDL_WM_ToggleFullScreen(gb_screen);
}

void close_vram(void)
{
  //free_buffer();
}

inline UINT8 get_nb_spr(void)
{
  UINT8 *sp=oam_space;
  INT16 no_tile,x,y,att;
  UINT8 sizey;
  UINT8 i,yoff,xoff;
  
  sizey=(LCDCCONT&0x04)?16:8;	
  
  nb_spr=0;
  for(i=0;i<40;i++) {
    y=*sp;
    x=*(sp+1);
    no_tile=*(sp+2);
    att=*(sp+3);
    sp+=4;

    y-=16;
    yoff=CURLINE-y;
    
    if ((CURLINE>=y) && (yoff<sizey)) {
      
      if (x<8) xoff=8-x;	
      else xoff = 0;
      
      gb_spr[nb_spr].sizey=sizey;
      gb_spr[nb_spr].x=x-8;
      gb_spr[nb_spr].y=y;
      gb_spr[nb_spr].xoff=xoff;
      gb_spr[nb_spr].yoff=yoff;
      gb_spr[nb_spr].xflip=(att&0x20)>>5;
      gb_spr[nb_spr].yflip=(att&0x40)>>6;
      gb_spr[nb_spr].pal_col=(att&0x07);
      if (att&0x10) gb_spr[nb_spr].pal=pal_obj1;
      else gb_spr[nb_spr].pal=pal_obj0;
      gb_spr[nb_spr].page=(att&0x08)>>3;
      gb_spr[nb_spr].priority=(att&0x80);
      if (sizey==16) gb_spr[nb_spr].no_tile=no_tile&0xfe;
      else  gb_spr[nb_spr].no_tile=no_tile;

      nb_spr++;
      if (nb_spr>10) {
	nb_spr=10;
	return nb_spr;
      }
    }
  }
  return nb_spr;    
}

inline void draw_spr_col(UINT16 *buf,GB_SPRITE *sp)
{
  UINT8 *tp;
  UINT8 nx;
  UINT8 bit0,bit1,wbit,c;

  tp=&vram_page[sp->page][sp->no_tile<<4];
  if (!sp->yflip) tp+=((sp->yoff)<<1);
  else tp+=(sp->sizey-1-sp->yoff)<<1;
  
  for(nx=sp->xoff;nx<8;nx++) {
    if (!sp->xflip) wbit=nx;
    else wbit=7-nx;	
    bit0=((*tp)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
    bit1=((*(tp+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
    c=(bit1<<1)|bit0;
    if (LCDCCONT&0x01) {
      if (c) {
	if (back_col[sp->x+nx][CURLINE]&0x80) {
	  if (!(back_col[sp->x+nx][CURLINE]&0x0f)) 
	    buf[sp->x+nx]=pal_col_obj[sp->pal_col][c];
	}
	else if (!sp->priority) buf[sp->x+nx]=pal_col_obj[sp->pal_col][c];
	else if (!(back_col[sp->x+nx][CURLINE]&0x0f)) buf[sp->x+nx]=pal_col_obj[sp->pal_col][c];
      }
    }
    else if (c) buf[sp->x+nx]=pal_col_obj[sp->pal_col][c];
  }
}

inline void draw_spr(UINT16 *buf,GB_SPRITE *sp)
{
  UINT8 *tp;
  UINT8 nx;
  UINT8 bit0,bit1,wbit,c;

  tp=&vram_page[0][sp->no_tile<<4];
  if (!sp->yflip) tp+=((sp->yoff)<<1);
  else tp+=(sp->sizey-1-sp->yoff)<<1;
  
  for(nx=sp->xoff;nx<8;nx++) {
    if (!sp->xflip) wbit=nx;
    else wbit=7-nx;	
    bit0=((*tp)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
    bit1=((*(tp+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
    c=(bit1<<1)|bit0;
    if (c) {
      if (!(sp->priority)) buf[sp->x+nx]=sp->pal[c];
      else if (!back_col[sp->x+nx][CURLINE]) buf[sp->x+nx]=sp->pal[c];
    }
  }
}

inline void draw_obj(UINT16 *buf)
{
  INT8 i;
  for(i=nb_spr-1;i>=0;i--) 
    draw_spr(buf,&gb_spr[(UINT8)i]);
}

inline void draw_obj_col(UINT16 *buf)
{
  INT8 i;
  for(i=nb_spr-1;i>=0;i--) 
    draw_spr_col(buf,&gb_spr[(UINT8)i]);
}

inline void draw_back_col(UINT16 *buf)
{
  UINT8 *tb,*tp,*att_tb;
  UINT16 y,x,i;
  UINT16 sx,sy;
  UINT8 bit0,bit1,c,p,att,xflip,yflip;
  INT16 no_tile;
 
  if (LCDCCONT&0x08) {// select Tile Map
    tb=&vram_page[0][0x1c00];
    att_tb=&vram_page[1][0x1c00];
  } else {
    tb=&vram_page[0][0x1800];
    att_tb=&vram_page[1][0x1800];
  }

  y=CURLINE;
  sy=SCRY+CURLINE;
  tb+=((sy>>3)<<5)&0x3ff;
  att_tb+=((sy>>3)<<5)&0x3ff;
  i=SCRX>>3;
  
  no_tile=tb[i&0x1f];
  att=att_tb[i&0x1f];
  
  yflip = (att & 0x40) ? 1 : 0;
  xflip = (att & 0x20) ? 1 : 0;
  p=att&0x07;
 
  if (!(LCDCCONT&0x10)) no_tile=256+(signed char)no_tile;
  tp=&vram_page[(att&0x08)>>3][(no_tile<<4)];
  
  if (!yflip) tp+=((sy&0x07)<<1);
  else tp+=((7-(sy&0x07))<<1);

  x=0;
  for(sx=SCRX&0x07;sx<8;sx++,x++) {
    int wbit;
    if (!xflip) wbit=sx;
    else wbit=7-sx;
    bit0=((*tp)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
    bit1=((*(tp+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
    c=(bit1<<1)|bit0;
    buf[x]=pal_col_bck[p][c];
    if (!(att&0x80)) back_col[x][CURLINE]=c;
    else back_col[x+sx][CURLINE]=0x80+c;
  }

  i++;
  for(;x<160;x+=8,i++) {
    no_tile=tb[i&0x1f];
    att=att_tb[i&0x1f];
    yflip = (att & 0x40) ? 1 : 0;
    xflip = (att & 0x20) ? 1 : 0;    
    p=att&0x07;

    if (!(LCDCCONT&0x10)) no_tile=256+(signed char)no_tile;
    
    tp=&vram_page[(att&0x08)>>3][(no_tile<<4)];
    if (!yflip) tp+=((sy&0x07)<<1);
    else tp+=((7-(sy&0x07))<<1);

    for(sx=0;sx<8;sx++) {
      int wbit;
      if (!xflip) wbit=sx;
      else wbit=7-sx;
      bit0=((*tp)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      bit1=((*(tp+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      c=(bit1<<1)|bit0;
      buf[x+sx]=pal_col_bck[p][c];
      if (!(att&0x80)) back_col[x+sx][CURLINE]=c;
      else back_col[x+sx][CURLINE]=0x80+c;
    }
  }

}

inline void draw_back(UINT16 *buf)
{
  UINT8 *tb,*tp;
  int y,x,i;
  int sx,sy;
  UINT8 bit0,bit1,c;
  INT16 no_tile;
 
  if (LCDCCONT&0x08) tb=&vram_page[0][0x1c00];
  else  tb=&vram_page[0][0x1800];

  y=CURLINE;
  sy=SCRY+CURLINE;
  tb+=((sy>>3)<<5)&0x3ff;
  i=SCRX>>3;

  no_tile=tb[i&0x1f];
  if (!(LCDCCONT&0x10)) no_tile=256+(signed char)no_tile;
  tp=&vram_page[0][no_tile<<4];
  tp+=(sy&0x07)<<1;
  
  x=0;
  for(sx=SCRX&0x07;sx<8;sx++,x++) {
    bit0=((*tp)&tab_ms[sx].mask)>>tab_ms[sx].shift;
    bit1=((*(tp+1))&tab_ms[sx].mask)>>tab_ms[sx].shift;
    c=(bit1<<1)|bit0;
    buf[x]=pal_bck[c];
    back_col[x][CURLINE]=c;
  }
  i++;
  for(;x<160;x+=8,i++) {
    no_tile=tb[i&0x1f];
    if (!(LCDCCONT&0x10)) no_tile=256+(signed char)no_tile;
    tp=&vram_page[0][no_tile<<4];
    tp+=(sy&0x07)<<1;
    for(sx=0;sx<8;sx++) {
      bit0=((*tp)&tab_ms[sx].mask)>>tab_ms[sx].shift;
      bit1=((*(tp+1))&tab_ms[sx].mask)>>tab_ms[sx].shift;
      c=(bit1<<1)|bit0;
      buf[x+sx]=pal_bck[c];
      back_col[x+sx][CURLINE]=c;
    }
  }

}

inline void draw_win_col(UINT16 *buf)
{
  UINT8 *tb,*tp,*att_tb; // ,*tiles;
  UINT16 y,x,i,sx;
  INT16 no_tile;
  UINT8 bit0,bit1,c,p,att,xflip,yflip;
  
  if (LCDCCONT&0x40) {
    tb=&vram_page[0][0x1c00];
    att_tb=&vram_page[1][0x1c00];
  } else {
    tb=&vram_page[0][0x1800];
    att_tb=&vram_page[1][0x1800];
  }

  if (WINX>=166) return;
  
  if (CURLINE>=WINY) {
    
    tb+=(((CURLINE-WINY)>>3)<<5);
    att_tb+=(((CURLINE-WINY)>>3)<<5);
    y=((CURLINE-WINY)&0x07);

    x=(((WINX-7)<0)?0:(WINX-7));
    for(i=0;x<160;x+=8,i++) {
      no_tile=tb[i];
      att=att_tb[i];
      yflip = (att & 0x40) ? 1 : 0;
      xflip = (att & 0x20) ? 1 : 0;  
      p=att&0x07;

      if (!(LCDCCONT&0x10)) no_tile=256+(signed char)no_tile;
      tp=&vram_page[(att&0x08)>>3][(no_tile<<4)]; 
      if (!yflip) tp+=y<<1;
      else tp+=((7-y)<<1);
      
      for(sx=0;sx<8;sx++) {
	int wbit;
	if (!xflip) wbit=sx;
	else wbit=7-sx;
	bit0=((*tp)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	bit1=((*(tp+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	c=(bit1<<1)|bit0;
	buf[x+sx]=pal_col_bck[p][c];
	if (!(att&0x80)) back_col[x+sx][CURLINE]=c;
	else back_col[x+sx][CURLINE]=0x80+c;	    
      }
    }
  }  
}

inline void draw_win(UINT16 *buf)
{
  UINT8 *tb,*tp;
  INT16 y,x,i,sx;
  INT16 no_tile;
  UINT8 bit0,bit1,c;
 
  if (LCDCCONT&0x40) tb=&vram_page[0][0x1c00];
  else tb=&vram_page[0][0x1800];
  
  if (WINX>=166) return;
  
  if (CURLINE>=WINY) {
    
    tb+=(((CURLINE-WINY)>>3)<<5);
    y=((CURLINE-WINY)&0x07)<<1;
    x=(((WINX-7)<0)?0:(WINX-7));
    for(i=0;x<160;x+=8,i++) {
      no_tile=tb[i];
      if (!(LCDCCONT&0x10)) no_tile=256+(signed char)no_tile;
      tp=&vram_page[0][no_tile<<4];
      tp+=y;
      for(sx=0;sx<8;sx++) {
	bit0=((*tp)&tab_ms[sx].mask)>>tab_ms[sx].shift;
	bit1=((*(tp+1))&tab_ms[sx].mask)>>tab_ms[sx].shift;
	c=(bit1<<1)|bit0;
	buf[x+sx]=pal_bck[c];
	back_col[x+sx][CURLINE]=c;
      }
    }
  }  
}

UINT8 draw_screen_wb(void)
{
  UINT16 *buf=(UINT16 *)back->pixels + CURLINE*SCREEN_X;
  if (SDL_MUSTLOCK(back) && SDL_LockSurface(back)<0) printf("can't lock surface\n");
  if (LCDCCONT&0x01) draw_back(buf);
  if (LCDCCONT&0x20) draw_win(buf);
  if (LCDCCONT&0x02) draw_obj(buf);
  if (SDL_MUSTLOCK(back)) SDL_UnlockSurface(back);
  return 0;
}

UINT8 draw_screen_col(void) 
{
  UINT16 *buf=(UINT16 *)back->pixels + CURLINE*SCREEN_X;
  if (SDL_MUSTLOCK(back) && SDL_LockSurface(back)<0) printf("can't lock surface\n");
  draw_back_col(buf);
  if (LCDCCONT&0x20) draw_win_col(buf);
  if (LCDCCONT&0x02) draw_obj_col(buf);
  if (SDL_MUSTLOCK(back)) SDL_UnlockSurface(back);
  return 0;
}

#ifdef SDL_GL
UINT8 draw_screen_gl_wb(void)
{
  UINT16 *buf=(UINT16 *)gl_tex.bmp + CURLINE*SCREEN_X;
  //  if (SDL_MUSTLOCK(back) && SDL_LockSurface(back)<0) printf("can't lock surface\n");
  if (LCDCCONT&0x01) draw_back(buf);
  if (LCDCCONT&0x20) draw_win(buf);
  if (LCDCCONT&0x02) draw_obj(buf);
  //  if (SDL_MUSTLOCK(back)) SDL_UnlockSurface(back);
  return 0;
}

UINT8 draw_screen_gl_col(void) 
{
  UINT16 *buf=(UINT16 *)gl_tex.bmp + CURLINE*SCREEN_X;
  //  if (SDL_MUSTLOCK(back) && SDL_LockSurface(back)<0) printf("can't lock surface\n");
  draw_back_col(buf);
  if (LCDCCONT&0x20) draw_win_col(buf);
  if (LCDCCONT&0x02) draw_obj_col(buf);
  //  if (SDL_MUSTLOCK(back)) SDL_UnlockSurface(back);
  return 0;
}
#endif
     
     
inline void clear_screen(void)
{
  UINT8 i,j;
 
  SDL_FillRect(back,NULL,pal_bck[0]);
  for(i=0;i<170;i++)
    for(j=0;j<170;j++)
      back_col[i][j]=0x00;
}

inline void blit_screen(void)
{
  dstR.x=rb_tab[rb_shift]; dstR.y=rb_tab[rb_shift+1]; dstR.w=SCREEN_X/*-rb_tab[rb_shift]*/; dstR.h=SCREEN_Y;
  scrR.x=0; scrR.y=0; scrR.w=SCREEN_X; scrR.h=SCREEN_Y;
 
  if (rb_on) {
    rb_shift++;
    if (rb_shift>10) {
      rb_on=0;
      rb_shift=0;
    }
  }

#ifdef SDL_GL
  if (conf.gl)
    update_gldisp();
  else {
#endif
    SDL_BlitSurface(back,&scrR,gb_screen,&dstR);
    SDL_Flip(gb_screen);
#ifdef SDL_GL
  }
#endif  
  if ((!(LCDCCONT&0x20) || !(LCDCCONT&0x01)) && (gameboy_type&NORMAL_GAMEBOY))
      clear_screen();
}





