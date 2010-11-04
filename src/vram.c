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
#include "message.h"
#include "emu.h"
#include "sgb.h"
#include <SDL/SDL.h>
#ifdef SDL_GL
#include <GL/gl.h>
#endif

#define RB_SIZE 5 

#ifdef SDL_YUV
INT8 rb_tab[2][RB_SIZE]={{0,1,2,2,1},
			 {0,2,1,2,1}};
UINT8 rb_shift=0;
#else
INT8 rb_tab[2][RB_SIZE]={{0,-2,2,-2,2},
			 {0,-1,-1,1,1}};
UINT8 rb_shift=0;
#endif

static struct mask_shift {
  unsigned char mask;
  unsigned char shift;
} tab_ms[8]={
  { 0x80,7 },
  { 0x40,6 },
  { 0x20,5 },
  { 0x10,4 },
  { 0x08,3 },
  { 0x04,2 },
  { 0x02,1 },
  { 0x01,0 }};

#define GET_GB_PIXEL(bit) (((((*tp)&tab_ms[(bit)].mask)>>tab_ms[(bit)].shift))|((((*(tp+1))&tab_ms[(bit)].mask)>>tab_ms[(bit)].shift)<<1))

SDL_Surface *gb_screen=NULL;
static SDL_Surface *back=NULL;

#ifdef SDL_YUV
SDL_Overlay *overlay;
SDL_Rect ov_rect;
UINT16 rgb2y[65536];
UINT8  rgb2u[65536];
UINT8  rgb2v[65536];

struct yuv{
  UINT16 y;
  UINT8  u;
  UINT8  v;
}rgb2yuv[65536];

#endif

UINT32 video_flag;

SDL_Rect dstR;
SDL_Rect scrR;

UINT16 grey[4];
UINT8 pal_bck[4]={0,3,3,3};
UINT8 pal_obj[2][4]={{3,3,3,3},{3,3,3,3}};

UINT16 pal_col_bck_gb[8][4];
UINT16 pal_col_obj_gb[8][4];
UINT16 pal_col_bck[8][4];
UINT16 pal_col_obj[8][4];
UINT16 Filter[32768];

UINT8 back_col[170][170];

UINT8 nb_spr;

UINT8 (*draw_screen)(void);
void (*blit_screen)(void);

UINT8 draw_screen_col(void);
UINT8 draw_screen_wb(void);
UINT8 draw_screen_sgb(void);

void blit_screen_default(void);
void blit_screen_sgb(void);

#ifdef SDL_YUV
UINT8 draw_screen_wb_yuv(void);
UINT8 draw_screen_col_yuv(void);
void blit_screen_yuv(void);
#else
#define draw_screen_wb_yuv NULL
#define draw_screen_col_yuv NULL
#define blit_screen_yuv NULL
#endif  

#ifdef SDL_GL
UINT8 draw_screen_wb_gl(void);
UINT8 draw_screen_col_gl(void);
UINT8 draw_screen_sgb_gl(void);
void blit_screen_gl(void);
#else 
#define draw_screen_wb_gl NULL
#define draw_screen_col_gl NULL
#define draw_screen_sgb_gl NULL
#define blit_screen_gl NULL
#endif

UINT8 (*draw_fun_matrix[3][3])(void)={
  {draw_screen_wb,draw_screen_wb_yuv,draw_screen_wb_gl},
  {draw_screen_col,draw_screen_col_yuv,draw_screen_col_gl},
  {draw_screen_sgb,NULL,draw_screen_sgb_gl}
};

void (*blit_fun_matrix[3][3])(void)={
  {blit_screen_default,blit_screen_yuv,blit_screen_gl},
  {blit_screen_default,blit_screen_yuv,blit_screen_gl},
  {blit_screen_sgb,NULL,blit_screen_gl}
};

#ifdef SDL_GL
void update_gldisp(void)
{
  static GLfloat tu=SCREEN_X/256.0,tv=SCREEN_Y/256.0;
  
  glClearColor( 0.0, 0.0, 0.0, 1.0 );
  glClear( GL_COLOR_BUFFER_BIT);
  glEnable(GL_TEXTURE_2D);

  if (rb_on) {
    rb_shift++;
    if (rb_shift>=RB_SIZE) {
      rb_on=0;
      rb_shift=0;
    }
  }
  
  if (conf.gb_type&SUPER_GAMEBOY) {
    glBindTexture(GL_TEXTURE_2D,sgb_tex.id);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0,0.0);
    glVertex2i(0,0);
    glTexCoord2f(1.0,0.0);
    glVertex2i(SGB_WIDTH,0);
    glTexCoord2f(1.0,SGB_HEIGHT/256.0);
    glVertex2i(SGB_WIDTH,SGB_HEIGHT);
    glTexCoord2f(0.0,SGB_HEIGHT/256.0);
    glVertex2i(0,SGB_HEIGHT);
    glEnd();
  }
  
  glBindTexture(GL_TEXTURE_2D,gl_tex.id);
  if (!sgb_mask)
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,SCREEN_X,SCREEN_Y,
		    GL_RGB,GL_UNSIGNED_SHORT_5_6_5,gl_tex.bmp);
  else glTexSubImage2D(GL_TEXTURE_2D,0,0,0,SCREEN_X,SCREEN_Y,
		       GL_RGB,GL_UNSIGNED_SHORT_5_6_5,NULL);

  glBegin(GL_QUADS);
  
  glTexCoord2f(rb_tab[0][rb_shift]/256.0,rb_tab[1][rb_shift]/256.0);
  glVertex2i(scxoff,scyoff);
  
  glTexCoord2f(tu+rb_tab[0][rb_shift]/256.0,rb_tab[1][rb_shift]/256.0);
  glVertex2i(SCREEN_X+scxoff,scyoff);

  glTexCoord2f(tu+rb_tab[0][rb_shift]/256.0,tv+rb_tab[1][rb_shift]/256.0); 
  glVertex2i(SCREEN_X+scxoff,SCREEN_Y+scyoff);
  
  glTexCoord2f(rb_tab[0][rb_shift]/256.0,tv+rb_tab[1][rb_shift]/256.0);
  glVertex2i(scxoff,SCREEN_Y+scyoff);
  
  glEnd();  
  
  update_message();
  glDisable(GL_TEXTURE_2D);
  SDL_GL_SwapBuffers();
}

void init_gl(void)
{
  glViewport(0,0,conf.gl_w,conf.gl_h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (conf.gb_type&SUPER_GAMEBOY)
    glOrtho(0.0,(GLfloat)SGB_WIDTH,(GLfloat)SGB_HEIGHT,0.0,-1.0,1.0);
  else 
    glOrtho(0.0,(GLfloat)SCREEN_X,(GLfloat)SCREEN_Y,0.0,-1.0,1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gl_tex.bmp=(UINT8 *)malloc(256*256*2);
  glEnable(GL_TEXTURE_2D);
  
  glGenTextures(1,&gl_tex.id);
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
	
	/*ng=g;
	  nb=b;
	  nr=r;*/
	
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

void gb_set_pal(int i)
{
#ifdef SDL_YUV
  if (conf.yuv) {
    grey[3]=0;
    grey[2]=64;
    grey[1]=128;
    grey[0]=192;
    memset(overlay->pixels[1],128,overlay->pitches[1]*(overlay->h/2));
    memset(overlay->pixels[2],128,overlay->pitches[2]*(overlay->h/2));
  } else {
#endif
    grey[0]=COL32_TO_16(conf.pal[i][0]); //0xc618; // ffe6ce
    grey[1]=COL32_TO_16(conf.pal[i][1]); //0x8410; // bfad9a
    grey[2]=COL32_TO_16(conf.pal[i][2]); //0x4208; // 7f7367
    grey[3]=COL32_TO_16(conf.pal[i][3]); //0x0000; // 3f3933
#ifdef SDL_YUV
  }  
#endif
}

void init_pallete(void)
{
  gb_set_pal(0);
  GenFilter();
}

#ifdef SDL_YUV
void init_rgb2yuv_table(void)
{
  UINT32 i;
  UINT8 y,u,v,r,g,b;
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
  }
}
#endif


void free_buffer(void)
{ 
  
}  

/* Reinit the screen after a resize event (For YUV)*/
void reinit_vram(void)
{
#ifdef SDL_YUV
  if (conf.yuv) {
    gb_screen=SDL_SetVideoMode(conf.yuv_w,conf.yuv_h,BIT_PER_PIXEL,video_flag);
    ov_rect.x=0;
    ov_rect.y=0;
    ov_rect.w=conf.yuv_w;
    ov_rect.h=conf.yuv_h;
  }
#endif
}

void init_vram(UINT32 flag)
{
  UINT8 type,mode;

  if (SDL_Init(SDL_INIT_JOYSTICK|SDL_INIT_VIDEO|((conf.sound)?SDL_INIT_AUDIO:0))<0) {
    printf("Couldn't initialize SDL: %s\n",SDL_GetError());
    exit(1);
  }
  atexit(SDL_Quit);   

#ifdef SDL_YUV
  if (conf.yuv) {
    flag|=SDL_RESIZABLE;
    if (conf.gb_type&COLOR_GAMEBOY) {
      conf.yuv_w*=2;
      conf.yuv_h*=2;
      gb_screen=SDL_SetVideoMode(conf.yuv_w,conf.yuv_h,BIT_PER_PIXEL,SDL_HWSURFACE|flag);
      overlay=SDL_CreateYUVOverlay(SCREEN_X*2,SCREEN_Y*2,SDL_YV12_OVERLAY,gb_screen); // FIXME : SGB support
    } else {
      gb_screen=SDL_SetVideoMode(conf.yuv_w,conf.yuv_h,BIT_PER_PIXEL,SDL_HWSURFACE|flag);
      overlay=SDL_CreateYUVOverlay(SCREEN_X,SCREEN_Y,SDL_YV12_OVERLAY,gb_screen); // FIXME : SGB support
    }
    ov_rect.x=0;
    ov_rect.y=0;
    ov_rect.w=conf.yuv_w;
    ov_rect.h=conf.yuv_h;
    init_rgb2yuv_table();
    memset(overlay->pixels[0],conf.yuv_interline_int,overlay->pitches[0]*overlay->h);
  } else
#endif

#ifdef SDL_GL
  if (conf.gl) 
    gb_screen=SDL_SetVideoMode(conf.gl_w,conf.gl_h,BIT_PER_PIXEL,SDL_HWSURFACE|flag);
  else 
#endif
    
    /*#ifndef DEBUG*/
    
    if (conf.gb_type&SUPER_GAMEBOY)
      gb_screen=SDL_SetVideoMode(SGB_WIDTH,SGB_HEIGHT,
				 BIT_PER_PIXEL,SDL_HWSURFACE|flag);
    else 
      gb_screen=SDL_SetVideoMode(SCREEN_X,SCREEN_Y,
				 BIT_PER_PIXEL,SDL_HWSURFACE|flag); 
  /*#else 
    gb_screen=SDL_SetVideoMode(DEBUG_SCR_X,DEBUG_SCR_Y,
    BIT_PER_PIXEL,SDL_HWSURFACE|flag);
    #endif
  */
  
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

  if (conf.gb_type&SUPER_GAMEBOY) {
    scxoff=48;
    scyoff=40;
  } else {
    scxoff=0;
    scyoff=0;
  }

  init_message();

  SDL_WM_SetCaption("Gngb",NULL);
  SDL_ShowCursor(0);

#ifdef SDL_GL
  if (conf.gl)
    init_gl();
#endif

  init_buffer();
  init_pallete();
  /*#ifdef SDL_GL
    if (conf.gl) {
    blit_screen=blit_screen_gl;
    if (conf.gb_type&COLOR_GAMEBOY) draw_screen=draw_screen_col_gl;
    else if (conf.gb_type&SUPER_GAMEBOY && rom_gb_type&SUPER_GAMEBOY) draw_screen=draw_screen_sgb_gl;
    else draw_screen=draw_screen_wb_gl;
    } else {
    #endif
    if (conf.yuv) {
    blit_screen=blit_screen_yuv;
    if (conf.gb_type&COLOR_GAMEBOY) draw_screen=draw_screen_col_yuv;
    else if (conf.gb_type&SUPER_GAMEBOY && rom_gb_type&SUPER_GAMEBOY) {
    draw_screen=draw_screen_sgb;
    blit_screen=blit_screen_sgb;
    } else draw_screen=draw_screen_wb_yuv;
    } else {
    blit_screen=blit_screen_default;
    if (conf.gb_type&COLOR_GAMEBOY) draw_screen=draw_screen_col;	
    else if (conf.gb_type&SUPER_GAMEBOY && rom_gb_type&SUPER_GAMEBOY) {
    draw_screen=draw_screen_sgb;
    blit_screen=blit_screen_sgb;
    } else draw_screen=draw_screen_wb;
    }
    #ifdef SDL_GL
    }
    #endif*/

  if (conf.yuv) mode=1;
  else if (conf.gl) mode=2;
  else mode=0;

  if (conf.gb_type&COLOR_GAMEBOY) type=1;
  else if (conf.gb_type&SUPER_GAMEBOY && rom_gb_type&SUPER_GAMEBOY) type=2;
  else type=0;

  draw_screen=draw_fun_matrix[type][mode];
  blit_screen=blit_fun_matrix[type][mode];

  video_flag=flag|SDL_HWSURFACE;
}

void switch_fullscreen(void) {
  SDL_WM_ToggleFullScreen(gb_screen);
  conf.fs^=1;
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

  if (!(LCDCCONT&0x02)) return 0;
  
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
    
    if ((CURLINE>=y) && (yoff<sizey) && (/*(x-8)>=0 && */((x-8)<160))) {
      
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
      if (att&0x10) gb_spr[nb_spr].pal=1;
      else gb_spr[nb_spr].pal=0;
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

/* Standart GB function */

inline void draw_spr(UINT16 *buf,GB_SPRITE *sp)
{
  UINT8 *tp;
  UINT8 nx;
  UINT8 /*bit0,bit1,*/wbit,c;

  tp=&vram_page[0][sp->no_tile<<4];
  if (!sp->yflip) tp+=((sp->yoff)<<1);
  else tp+=(sp->sizey-1-sp->yoff)<<1;
  
  for(nx=sp->xoff;nx<8;nx++) {
    if (!sp->xflip) wbit=nx;
    else wbit=7-nx;	
    /*bit0=((*tp)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      bit1=((*(tp+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      c=(bit1<<1)|bit0;*/
    c=GET_GB_PIXEL(wbit);
    if (c) {
      if (!(sp->priority)) buf[sp->x+nx]=grey[pal_obj[sp->pal][c]];
      else if (!back_col[sp->x+nx][CURLINE]) buf[sp->x+nx]=grey[pal_obj[sp->pal][c]];
    }
  }
}

inline void draw_obj(UINT16 *buf)
{
  INT8 i;
  for(i=nb_spr-1;i>=0;i--) 
    draw_spr(buf,&gb_spr[(UINT8)i]);
}

inline void draw_back(UINT16 *buf)
{
  UINT8 *tb,*tp;
  int y,x,i;
  int sx,sy;
  UINT8 /*bit0,bit1,*/c;
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
    /*bit0=((*tp)&tab_ms[sx].mask)>>tab_ms[sx].shift;
      bit1=((*(tp+1))&tab_ms[sx].mask)>>tab_ms[sx].shift;
      c=(bit1<<1)|bit0;*/
    c=GET_GB_PIXEL(sx);
    /* FIXME VIC VIPER */
    buf[x]=grey[gblcdc->vram_pal_line[x][c]];
    back_col[x][CURLINE]=c;
  }
  i++;
  for(;x<160;x+=8,i++) {
    no_tile=tb[i&0x1f];
    if (!(LCDCCONT&0x10)) no_tile=256+(signed char)no_tile;
    tp=&vram_page[0][no_tile<<4];
    tp+=(sy&0x07)<<1;
    for(sx=0;sx<8 && x+sx<160;sx++) {
      /*bit0=((*tp)&tab_ms[sx].mask)>>tab_ms[sx].shift;
	bit1=((*(tp+1))&tab_ms[sx].mask)>>tab_ms[sx].shift;
	c=(bit1<<1)|bit0;*/
      c=GET_GB_PIXEL(sx);
      /* FIXME VIC VIPER */
      buf[x+sx]=grey[gblcdc->vram_pal_line[x+sx][c]];
      back_col[x+sx][CURLINE]=c;
    }
  }
}

inline void draw_win(UINT16 *buf)
{
  UINT8 *tb,*tp;
  INT16 y,x,i,sx=0;
  INT16 no_tile;
  UINT8 /*bit0,bit1,*/c;
 
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
      for(sx=0;sx<8 && (x+sx)<160;sx++) {
	/*bit0=((*tp)&tab_ms[sx].mask)>>tab_ms[sx].shift;
	  bit1=((*(tp+1))&tab_ms[sx].mask)>>tab_ms[sx].shift;
	  c=(bit1<<1)|bit0;*/
	c=GET_GB_PIXEL(sx);
	/* FIXME VIC VIPER */
	buf[x+sx]=grey[gblcdc->vram_pal_line[x+sx][c]];
	back_col[x+sx][CURLINE]=c;
      }
    }
  }  
}

/* CGB drawing functions */

inline void draw_spr_col(UINT16 *buf,GB_SPRITE *sp)
{
  UINT8 *tp;
  UINT8 nx;
  UINT8 /*bit0,bit1,*/wbit,c;

  tp=&vram_page[sp->page][sp->no_tile<<4];
  if (!sp->yflip) tp+=((sp->yoff)<<1);
  else tp+=(sp->sizey-1-sp->yoff)<<1;
  
  for(nx=sp->xoff;nx<8;nx++) {
    if (!sp->xflip) wbit=nx;
    else wbit=7-nx;	
    /*bit0=((*tp)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      bit1=((*(tp+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      c=(bit1<<1)|bit0;*/
    c=GET_GB_PIXEL(wbit);

    /* TODO: OPTIMISATION */
    /*if (back_col[sp->x+nx][CURLINE]&0x80) {  
      if (c) {
	if (!(back_col[sp->x+nx][CURLINE]&0x0f)) 
	  buf[sp->x+nx]=pal_col_obj[sp->pal_col][c];
      }
    } else {
      if (!sp->priority) {
	if (c) buf[sp->x+nx]=pal_col_obj[sp->pal_col][c];
      } else {
	if (!(back_col[sp->x+nx][CURLINE]&0x0f)) buf[sp->x+nx]=pal_col_obj[sp->pal_col][c];
      }      
      }*/

    if (LCDCCONT&0x01) {
      if (c) {
	if (back_col[sp->x+nx][CURLINE]&0x80) {
	  if (!(back_col[sp->x+nx][CURLINE]&0x0f)) 
	    buf[sp->x+nx]=pal_col_obj[sp->pal_col][c];
 	} else if (!sp->priority) buf[sp->x+nx]=pal_col_obj[sp->pal_col][c];
	else if (!(back_col[sp->x+nx][CURLINE]&0x0f)) buf[sp->x+nx]=pal_col_obj[sp->pal_col][c];
      } 
    } else if (c) buf[sp->x+nx]=pal_col_obj[sp->pal_col][c];
  }
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
  UINT8 /*bit0,bit1,*/c,p,att,xflip,yflip;
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
    /*bit0=((*tp)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      bit1=((*(tp+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      c=(bit1<<1)|bit0;*/
    c=GET_GB_PIXEL(wbit);
    buf[x]=pal_col_bck[p][c];
    back_col[x][CURLINE]=(att&0x80)+c;
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
      /*bit0=((*tp)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	bit1=((*(tp+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	c=(bit1<<1)|bit0;*/
      c=GET_GB_PIXEL(wbit);
      buf[x+sx]=pal_col_bck[p][c];
      back_col[x+sx][CURLINE]=(att&0x80)+c;
    }
  }
}

inline void draw_win_col(UINT16 *buf)
{
  UINT8 *tb,*tp,*att_tb; // ,*tiles;
  UINT16 y,x,i,sx;
  INT16 no_tile;
  UINT8 /*bit0,bit1,*/c,p,att,xflip,yflip;
  
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
	/*bit0=((*tp)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	  bit1=((*(tp+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	  c=(bit1<<1)|bit0;*/
	c=GET_GB_PIXEL(wbit);
	buf[x+sx]=pal_col_bck[p][c];
	if (!(att&0x80)) back_col[x+sx][CURLINE]=c;
	else back_col[x+sx][CURLINE]=0x80+c;	    
      }
    }
  }  
}


/* SGB drawing Functions  To Optimize */

inline void draw_back_sgb(UINT16 *buf)
{
  UINT8 *tb,*tp;
  int y,x,i;
  int sx,sy;
  UINT8 /*bit0,bit1,*/c;
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
    /*bit0=((*tp)&tab_ms[sx].mask)>>tab_ms[sx].shift;
      bit1=((*(tp+1))&tab_ms[sx].mask)>>tab_ms[sx].shift;
      c=(bit1<<1)|bit0;*/
     c=GET_GB_PIXEL(sx);
    //buf[x]=sgb_pal[sgb_pal_map[x/8][CURLINE/8]][pal_bck[c]];
    /* FIXME VIC VIPER */
    buf[x]=sgb_pal[sgb_pal_map[x/8][CURLINE/8]][gblcdc->vram_pal_line[x+sx][c]];
    back_col[x][CURLINE]=c;
  }
  i++;
  for(;x<160;x+=8,i++) {
    no_tile=tb[i&0x1f];
    if (!(LCDCCONT&0x10)) no_tile=256+(signed char)no_tile;
    tp=&vram_page[0][no_tile<<4];
    tp+=(sy&0x07)<<1;
    for(sx=0;sx<8 && x+sx<160;sx++) {
      /*bit0=((*tp)&tab_ms[sx].mask)>>tab_ms[sx].shift;
	bit1=((*(tp+1))&tab_ms[sx].mask)>>tab_ms[sx].shift;
	c=(bit1<<1)|bit0;*/
      c=GET_GB_PIXEL(sx);
      //buf[x+sx]=sgb_pal[sgb_pal_map[x/8][CURLINE/8]][pal_bck[c]];
      //buf[x+sx]=sgb_pal[sgb_pal_map[x/8][CURLINE/8]][c];
      /* FIXME VIC VIPER */
      buf[x+sx]=sgb_pal[sgb_pal_map[x/8][CURLINE/8]][gblcdc->vram_pal_line[x+sx][c]];
      back_col[x+sx][CURLINE]=c;
    }
  }

}

inline void draw_win_sgb(UINT16 *buf)
{
  UINT8 *tb,*tp;
  INT16 y,x,i,sx;
  INT16 no_tile;
  UINT8 /*bit0,bit1,*/c;
 
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
      for(sx=0;sx<8 && x+sx<160;sx++) {
	/*bit0=((*tp)&tab_ms[sx].mask)>>tab_ms[sx].shift;
	  bit1=((*(tp+1))&tab_ms[sx].mask)>>tab_ms[sx].shift;
	  c=(bit1<<1)|bit0;*/
	c=GET_GB_PIXEL(sx);
	//buf[x+sx]=pal_bck[c];
	//buf[x+sx]=sgb_pal[sgb_pal_map[x/8][CURLINE/8]][pal_bck[c]];
	buf[x+sx]=sgb_pal[sgb_pal_map[x/8][CURLINE/8]][gblcdc->vram_pal_line[x+sx][c]];
	back_col[x+sx][CURLINE]=c;
      }
    }
  }  
}

inline void draw_spr_sgb(UINT16 *buf,GB_SPRITE *sp)
{
  UINT8 *tp;
  UINT8 nx;
  UINT8 /*bit0,bit1,*/wbit,c;

  tp=&vram_page[0][sp->no_tile<<4];
  if (!sp->yflip) 
    tp+=((sp->yoff)<<1);
  else tp+=(sp->sizey-1-sp->yoff)<<1;
  
  for(nx=sp->xoff;nx<8;nx++) {
    if (!sp->xflip) wbit=nx;
    else wbit=7-nx;	
    /*bit0=((*tp)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      bit1=((*(tp+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      c=(bit1<<1)|bit0;*/
    c=GET_GB_PIXEL(wbit);
    if (c) {
      /* FIXME */
      //   printf("%d %d %d %d %d %d \n",sp->x,sp->y,CURLINE,sgb_pal_map[(sp->x+nx)/8][CURLINE/8],sp->pal,c);
      if (!(sp->priority)) buf[sp->x+nx]=sgb_pal[sgb_pal_map[(sp->x+nx)/8][CURLINE/8]][pal_obj[sp->pal][c]];
      else if (!back_col[sp->x+nx][CURLINE]) buf[sp->x+nx]=sgb_pal[sgb_pal_map[(sp->x+nx)/8][CURLINE/8]][pal_obj[sp->pal][c]];       
      
      /*if (!(sp->priority)) buf[sp->x+nx]=sgb_pal[0][pal_obj[sp->pal][c]];
	else if (!back_col[sp->x+nx][CURLINE]) buf[sp->x+nx]=sgb_pal[0][pal_obj[sp->pal][c]];*/
      /*if (!(sp->priority)) buf[sp->x+nx]=sgb_pal[sp->pal_col][c];
	else if (!back_col[sp->x+nx][CURLINE]) buf[sp->x+nx]=sgb_pal[sp->pal_col][c];*/
    }
  }
}

inline void draw_obj_sgb(UINT16 *buf)
{
  INT8 i;
  for(i=nb_spr-1;i>=0;i--) 
    draw_spr_sgb(buf,&gb_spr[(UINT8)i]);
}


#ifdef SDL_YUV
/* Standart GB function (YUV support)*/

inline void draw_spr_yuv(UINT8 *buf,GB_SPRITE *sp)
{
  UINT8 *tp;
  UINT8 nx;
  UINT8 /*bit0,bit1,*/wbit,c;

  tp=&vram_page[0][sp->no_tile<<4];
  if (!sp->yflip) tp+=((sp->yoff)<<1);
  else tp+=(sp->sizey-1-sp->yoff)<<1;
  
  for(nx=sp->xoff;nx<8;nx++) {
    if (!sp->xflip) wbit=nx;
    else wbit=7-nx;	
    /*bit0=((*tp)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      bit1=((*(tp+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      c=(bit1<<1)|bit0;*/
    c=GET_GB_PIXEL(wbit);
    if (c) {
      if (!(sp->priority)) buf[sp->x+nx]=grey[pal_obj[sp->pal][c]];
      else if (!back_col[sp->x+nx][CURLINE]) buf[sp->x+nx]=grey[pal_obj[sp->pal][c]];
    }
  }
}

inline void draw_obj_yuv(UINT8 *buf)
{
  INT8 i;
  for(i=nb_spr-1;i>=0;i--) {
    draw_spr_yuv(buf,&gb_spr[(UINT8)i]);
  }
}

inline void draw_back_yuv(UINT8 *buf)
{
  UINT8 *tb,*tp;
  int y,x,i;
  int sx,sy;
  UINT8 /*bit0,bit1,*/c;
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
    /*bit0=((*tp)&tab_ms[sx].mask)>>tab_ms[sx].shift;
      bit1=((*(tp+1))&tab_ms[sx].mask)>>tab_ms[sx].shift;
      c=(bit1<<1)|bit0;*/
    c=GET_GB_PIXEL(sx);
    /* FIXME VIC VIPER */
    buf[x]=grey[gblcdc->vram_pal_line[x][c]];
    back_col[x][CURLINE]=c;
  }
  i++;
  for(;x<160;x+=8,i++) {
    no_tile=tb[i&0x1f];
    if (!(LCDCCONT&0x10)) no_tile=256+(signed char)no_tile;
    tp=&vram_page[0][no_tile<<4];
    tp+=(sy&0x07)<<1;
    for(sx=0;sx<8 && x+sx<160;sx++) {
      /*bit0=((*tp)&tab_ms[sx].mask)>>tab_ms[sx].shift;
	bit1=((*(tp+1))&tab_ms[sx].mask)>>tab_ms[sx].shift;
	c=(bit1<<1)|bit0;*/
      c=GET_GB_PIXEL(sx);
      /* FIXME VIC VIPER */
      buf[x+sx]=grey[gblcdc->vram_pal_line[x+sx][c]];
      back_col[x+sx][CURLINE]=c;
    }
  }
}

inline void draw_win_yuv(UINT8 *buf)
{
  UINT8 *tb,*tp;
  INT16 y,x,i,sx=0;
  INT16 no_tile;
  UINT8 /*bit0,bit1,*/c;
 
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
      for(sx=0;sx<8 && (x+sx)<160;sx++) {
	/*bit0=((*tp)&tab_ms[sx].mask)>>tab_ms[sx].shift;
	  bit1=((*(tp+1))&tab_ms[sx].mask)>>tab_ms[sx].shift;
	  c=(bit1<<1)|bit0;*/
	c=GET_GB_PIXEL(sx);
	/* FIXME VIC VIPER */
	buf[x+sx]=grey[gblcdc->vram_pal_line[x+sx][c]];
	back_col[x+sx][CURLINE]=c;
      }
    }
  }  
}

/* CGB drawing functions (YUV Support) */

inline void draw_spr_col_yuv(UINT16 *buf,UINT8 *buf_u, UINT8 *buf_v,GB_SPRITE *sp)
{
  UINT8 *tp;
  UINT8 nx;
  UINT8 /*bit0,bit1,*/wbit,c;

  tp=&vram_page[sp->page][sp->no_tile<<4];
  if (!sp->yflip) tp+=((sp->yoff)<<1);
  else tp+=(sp->sizey-1-sp->yoff)<<1;
  
  for(nx=sp->xoff;nx<8;nx++) {
    if (!sp->xflip) wbit=nx;
    else wbit=7-nx;	
    /*bit0=((*tp)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      bit1=((*(tp+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      c=(bit1<<1)|bit0;*/
    c=GET_GB_PIXEL(wbit);

    /* TODO: OPTIMISATION */
    /*if (back_col[sp->x+nx][CURLINE]&0x80) {  // bg priority
      if (c) {
      if (!(back_col[sp->x+nx][CURLINE]&0x0f)) {
      buf[sp->x+nx]=rgb2y[pal_col_obj[sp->pal_col][c]];
      buf_u[sp->x+nx]=rgb2u[pal_col_obj[sp->pal_col][c]];
      buf_v[sp->x+nx]=rgb2v[pal_col_obj[sp->pal_col][c]];
      }
      }
      } else {
      if (!sp->priority) {
      if (c) {
      buf[sp->x+nx]=rgb2y[pal_col_obj[sp->pal_col][c]];
      buf_u[sp->x+nx]=rgb2u[pal_col_obj[sp->pal_col][c]];
      buf_v[sp->x+nx]=rgb2v[pal_col_obj[sp->pal_col][c]];
      }
      } else {
      if (!(back_col[sp->x+nx][CURLINE]&0x0f)) {
      buf[sp->x+nx]=rgb2y[pal_col_obj[sp->pal_col][c]];
      buf_u[sp->x+nx]=rgb2u[pal_col_obj[sp->pal_col][c]];
      buf_v[sp->x+nx]=rgb2v[pal_col_obj[sp->pal_col][c]];
      }
      }      
      }*/

    if (LCDCCONT&0x01) {
      if (c) {
	if (back_col[sp->x+nx][CURLINE]&0x80) {
	  if (!(back_col[sp->x+nx][CURLINE]&0x0f)) {
	    //buf[sp->x+nx]=pal_col_obj[sp->pal_col][c];
	    buf[sp->x+nx]=rgb2yuv[pal_col_obj[sp->pal_col][c]].y;
	    buf_u[sp->x+nx]=rgb2yuv[pal_col_obj[sp->pal_col][c]].u;
	    buf_v[sp->x+nx]=rgb2yuv[pal_col_obj[sp->pal_col][c]].v;
	  }
 	} else if (!sp->priority) {
	  //buf[sp->x+nx]=pal_col_obj[sp->pal_col][c];
	  buf[sp->x+nx]=rgb2yuv[pal_col_obj[sp->pal_col][c]].y;
	  buf_u[sp->x+nx]=rgb2yuv[pal_col_obj[sp->pal_col][c]].u;
	  buf_v[sp->x+nx]=rgb2yuv[pal_col_obj[sp->pal_col][c]].v;
	}
	else if (!(back_col[sp->x+nx][CURLINE]&0x0f)) {
	  //buf[sp->x+nx]=pal_col_obj[sp->pal_col][c];
	  buf[sp->x+nx]=rgb2yuv[pal_col_obj[sp->pal_col][c]].y;
	  buf_u[sp->x+nx]=rgb2yuv[pal_col_obj[sp->pal_col][c]].u;
	  buf_v[sp->x+nx]=rgb2yuv[pal_col_obj[sp->pal_col][c]].v;
	}
      }
    } else if (c) {
      //buf[sp->x+nx]=pal_col_obj[sp->pal_col][c];
      buf[sp->x+nx]=rgb2yuv[pal_col_obj[sp->pal_col][c]].y;
      buf_u[sp->x+nx]=rgb2yuv[pal_col_obj[sp->pal_col][c]].u;
      buf_v[sp->x+nx]=rgb2yuv[pal_col_obj[sp->pal_col][c]].v;
    }

  }
}


inline void draw_obj_col_yuv(UINT16 *buf,UINT8 *buf_u, UINT8 *buf_v)
{
  INT8 i;
  for(i=nb_spr-1;i>=0;i--) 
    draw_spr_col_yuv(buf,buf_u,buf_v,&gb_spr[(UINT8)i]);
}

inline void draw_back_col_yuv(UINT16 *buf,UINT8 *buf_u, UINT8 *buf_v)
{
  UINT8 *tb,*tp,*att_tb;
  UINT16 y,x,i;
  UINT16 sx,sy;
  UINT8 /*bit0,bit1,*/c,p,att,xflip,yflip;
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
  for(sx=SCRX&0x07;sx<8 && sx<160;sx++,x++) {
    int wbit;
    if (!xflip) wbit=sx;
    else wbit=7-sx;
    /*bit0=((*tp)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      bit1=((*(tp+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      c=(bit1<<1)|bit0;*/
    c=GET_GB_PIXEL(wbit);
    buf[x]=rgb2yuv[pal_col_bck[p][c]].y;
    buf_u[x]=rgb2yuv[pal_col_bck[p][c]].u;
    buf_v[x]=rgb2yuv[pal_col_bck[p][c]].v;
    back_col[x][CURLINE]=(att&0x80)+c;
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

    for(sx=0;sx<8 && (x+sx)<160;sx++) {
      int wbit;
      if (!xflip) wbit=sx;
      else wbit=7-sx;
      /*bit0=((*tp)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	bit1=((*(tp+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	c=(bit1<<1)|bit0;*/
      c=GET_GB_PIXEL(wbit);
      buf[x+sx]=rgb2yuv[pal_col_bck[p][c]].y;
      buf_u[x+sx]=rgb2yuv[pal_col_bck[p][c]].u;
      buf_v[x+sx]=rgb2yuv[pal_col_bck[p][c]].v;
      back_col[x+sx][CURLINE]=(att&0x80)+c;
    }
  }
}

inline void draw_win_col_yuv(UINT16 *buf,UINT8 *buf_u, UINT8 *buf_v)
{
  UINT8 *tb,*tp,*att_tb; // ,*tiles;
  UINT16 y,x,i,sx;
  INT16 no_tile;
  UINT8 /*bit0,bit1,*/c,p,att,xflip,yflip;
  
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
	/*bit0=((*tp)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	  bit1=((*(tp+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	  c=(bit1<<1)|bit0;*/
	c=GET_GB_PIXEL(wbit);
	buf[x+sx]=rgb2yuv[pal_col_bck[p][c]].y;
	buf_u[x+sx]=rgb2yuv[pal_col_bck[p][c]].u;
	buf_v[x+sx]=rgb2yuv[pal_col_bck[p][c]].v;
	back_col[x+sx][CURLINE]=((att&0x80)+c);
      }
    }
  }  
}

/* TODO SGB Function (YUV Support) */

#endif

/**********************************/

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

UINT8 draw_screen_sgb(void)
{
  UINT16 *buf=(UINT16 *)back->pixels + CURLINE*SCREEN_X;
  if (SDL_MUSTLOCK(back) && SDL_LockSurface(back)<0) printf("can't lock surface\n");
  if (LCDCCONT&0x01) draw_back_sgb(buf);
  if (LCDCCONT&0x20) draw_win_sgb(buf);
  /* FIXME */
  if (LCDCCONT&0x02) draw_obj_sgb(buf);
  if (SDL_MUSTLOCK(back)) SDL_UnlockSurface(back);
  return 0;
}

/* YUV SUPPORT */
#ifdef SDL_YUV
UINT8 draw_screen_wb_yuv(void)
{
  UINT8 *buf=(UINT8 *)overlay->pixels[0] + CURLINE*SCREEN_X;
  //  if (SDL_MUSTLOCK(back) && SDL_LockSurface(back)<0) printf("can't lock surface\n");
  if (LCDCCONT&0x01) draw_back_yuv(buf);
  if (LCDCCONT&0x20) draw_win_yuv(buf);
  if (LCDCCONT&0x02) draw_obj_yuv(buf);
  //if (SDL_MUSTLOCK(back)) SDL_UnlockSurface(back);
  return 0;
}

static inline void copy_yuv_line(UINT32 *src,UINT32 *dst) {
  int i;
  for (i=0;i<(overlay->pitches[0]>>2);i+=4) {
    dst[i]=src[i];
    dst[i+1]=src[i+1];
    dst[i+2]=src[i+2];
    dst[i+3]=src[i+3];
  }
}

UINT8 draw_screen_col_yuv(void) 
{
  UINT16 *buf=(UINT16 *)overlay->pixels[0] + CURLINE*overlay->pitches[0];
  UINT8  *buf_u=(UINT8 *)overlay->pixels[1] + CURLINE*overlay->pitches[1];
  UINT8  *buf_v=(UINT8 *)overlay->pixels[2] + CURLINE*overlay->pitches[2];
  //UINT16 yuv_interline_int=conf.yuv_interline_int;
  //if (SDL_MUSTLOCK(back) && SDL_LockSurface(back)<0) printf("can't lock surface\n");
  draw_back_col_yuv(buf,buf_u,buf_v);
  if (LCDCCONT&0x20) draw_win_col_yuv(buf,buf_u,buf_v);
  if (LCDCCONT&0x02) draw_obj_col_yuv(buf,buf_u,buf_v);
  //if (SDL_MUSTLOCK(back)) SDL_UnlockSurface(back);
#if 0
  buf+=overlay->pitches[0]/2;
  /* Erase some trash made by drawback col (Better solution?) */
  buf[0]=yuv_interline_int;buf[1]=yuv_interline_int;buf[2]=yuv_interline_int;buf[3]=yuv_interline_int;
  buf[4]=yuv_interline_int;buf[5]=yuv_interline_int;buf[6]=yuv_interline_int;buf[7]=yuv_interline_int;
#else 
  copy_yuv_line((UINT32 *)buf,(UINT32 *)(buf+(overlay->pitches[0]>>1)));
#endif
  return 0;
}

#endif


/* GL SUPPORT */

#ifdef SDL_GL

UINT8 draw_screen_wb_gl(void)
{
  UINT16 *buf=(UINT16 *)gl_tex.bmp + CURLINE*SCREEN_X;
  if (LCDCCONT&0x01) draw_back(buf);
  if (LCDCCONT&0x20) draw_win(buf);
  if (LCDCCONT&0x02) draw_obj(buf);
  return 0;
}

UINT8 draw_screen_col_gl(void) 
{
  UINT16 *buf=(UINT16 *)gl_tex.bmp + CURLINE*SCREEN_X;
  draw_back_col(buf);
  if (LCDCCONT&0x20) draw_win_col(buf);
  if (LCDCCONT&0x02) draw_obj_col(buf);
  return 0;
}

UINT8 draw_screen_sgb_gl(void)
{
  UINT16 *buf=(UINT16 *)gl_tex.bmp + CURLINE*SCREEN_X;
  if (LCDCCONT&0x01) draw_back_sgb(buf);
  if (LCDCCONT&0x20) draw_win_sgb(buf);
  /* FIXME */
  if (LCDCCONT&0x02) draw_obj_sgb(buf);
  return 0;
}

#endif
     
inline void clear_screen(void)
{
  UINT8 i,j;
  if (back)  SDL_FillRect(back,NULL,grey[0]);
  for(i=0;i<170;i++)
    for(j=0;j<170;j++)
      back_col[i][j]=0x00;
}

/* Blit Function */

void blit_screen_default(void) {
 
  if (rb_on) {
      rb_shift++;
      if (rb_shift>=RB_SIZE) {
	rb_on=0;
	rb_shift=0;
      }
    }

  scrR.x=0; scrR.y=0; scrR.w=SCREEN_X; scrR.h=SCREEN_Y;
  
  dstR.x=rb_tab[0][rb_shift]+scxoff; 
  dstR.y=rb_tab[1][rb_shift]+scyoff; 
  dstR.w=SCREEN_X; 
  dstR.h=SCREEN_Y;
  
  SDL_BlitSurface(back,&scrR,gb_screen,&dstR);
  
  update_message();
  SDL_Flip(gb_screen);
    
  if ((!(LCDCCONT&0x20) || !(LCDCCONT&0x01)) && (conf.gb_type&NORMAL_GAMEBOY)) clear_screen();
}

void blit_screen_sgb(void) {
  
  if (rb_on) {
      rb_shift++;
      if (rb_shift>=RB_SIZE) {
	rb_on=0;
	rb_shift=0;
      }
    }

  scrR.x=0; scrR.y=0; scrR.w=SCREEN_X; scrR.h=SCREEN_Y;
  
  dstR.x=rb_tab[0][rb_shift]+scxoff; 
  dstR.y=rb_tab[1][rb_shift]+scyoff; 
  dstR.w=SCREEN_X; 
  dstR.h=SCREEN_Y;
  
  /* FIXME: sgb support and inverse the blit*/
  /*    if (conf.gb_type&SUPER_GAMEBOY) 
	SDL_BlitSurface(sgb_buf,NULL,gb_screen,NULL);*/
  if (sgb_mask) SDL_FillRect(back,NULL,pal_bck[0]);
  else SDL_BlitSurface(back,&scrR,gb_screen,&dstR);
  
  update_message();
  SDL_Flip(gb_screen);
    
  if ((!(LCDCCONT&0x20) || !(LCDCCONT&0x01)) && (conf.gb_type&NORMAL_GAMEBOY))
    clear_screen();
}

#ifdef SDL_YUV
void blit_screen_yuv(void) {

  if (rb_on) {
    rb_shift++;
    if (rb_shift>=RB_SIZE) {
	rb_on=0;
	rb_shift=0;
    }
  }
  
  /*scrR.x=0; scrR.y=0; scrR.w=SCREEN_X; scrR.h=SCREEN_Y;
    
    dstR.x=rb_tab[0][rb_shift]+scxoff; 
    dstR.y=rb_tab[1][rb_shift]+scyoff; 
    dstR.w=SCREEN_X; 
    dstR.h=SCREEN_Y;*/

  ov_rect.x=rb_tab[0][rb_shift];
  ov_rect.y=rb_tab[1][rb_shift];
  ov_rect.w=conf.yuv_w-rb_tab[0][rb_shift];
  ov_rect.h=conf.yuv_h-rb_tab[1][rb_shift];

  
    
    /* FIXME: sgb support and inverse the blit*/
    /*    if (conf.gb_type&SUPER_GAMEBOY) 
	  SDL_BlitSurface(sgb_buf,NULL,gb_screen,NULL);*/
  /*    if (sgb_mask) 
	SDL_FillRect(back,NULL,pal_bck[0]);*/
  //if (conf.yuv) 
  
  update_message();
  SDL_DisplayYUVOverlay(overlay,&ov_rect);
  if ((!(LCDCCONT&0x20) || !(LCDCCONT&0x01)) && (conf.gb_type&NORMAL_GAMEBOY)) clear_screen();
}
#endif

#ifdef SDL_GL
void blit_screen_gl(void) {
  update_gldisp();
  update_message();
  if ((!(LCDCCONT&0x20) || !(LCDCCONT&0x01)) && (conf.gb_type&NORMAL_GAMEBOY)) clear_screen();
}
#endif

/*  inline void blit_screen(void) */
/*  { */
/*  #ifdef SDL_GL */
/*    if (conf.gl) */
/*      update_gldisp(); */
/*    else { */
/*  #endif     */
/*      if (rb_on) { */
/*        rb_shift++; */
/*        if (rb_shift>=RB_SIZE) { */
/*  	rb_on=0; */
/*  	rb_shift=0; */
/*        } */
/*      } */

/*      scrR.x=0; scrR.y=0; scrR.w=SCREEN_X; scrR.h=SCREEN_Y; */
/*      //if (conf.gb_type&SUPER_GAMEBOY)  { */
/*      dstR.x=rb_tab[0][rb_shift]+scxoff;  */
/*      dstR.y=rb_tab[1][rb_shift]+scyoff;  */
/*      dstR.w=SCREEN_X;  */
/*      dstR.h=SCREEN_Y; */
/*} else { 
  dstR.x=rb_tab[0][rb_shift];  
  dstR.y=rb_tab[1][rb_shift];  
  dstR.w=SCREEN_X;  
  dstR.h=SCREEN_Y; 
  }*/ 

/* FIXME: sgb support and inverse the blit*/ 
/*    if (conf.gb_type&SUPER_GAMEBOY)  */
/*  	  SDL_BlitSurface(sgb_buf,NULL,gb_screen,NULL);*/ 
/*      if (sgb_mask)  */
/*        SDL_FillRect(back,NULL,pal_bck[0]); */
/*      if (conf.yuv)  */
/*        SDL_DisplayYUVOverlay(overlay,&ov_rect); */
/*      else */
/*        SDL_BlitSurface(back,&scrR,gb_screen,&dstR); */
    
/*      update_message(); */
/*      //SDL_Flip(gb_screen); */
/*  #ifdef SDL_GL */
/*    } */
/*  #endif   */
/*    if ((!(LCDCCONT&0x20) || !(LCDCCONT&0x01)) && (conf.gb_type&NORMAL_GAMEBOY)) */
/*      clear_screen(); */
/*  } */
