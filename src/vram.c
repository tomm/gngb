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
#include "video_std.h"
#include "menu.h"
#include <SDL.h>

SDL_Surface *gb_screen=NULL;


Uint16 grey[4];
Uint8 pal_bck[4]={0,3,3,3};
Uint8 pal_obj[2][4]={{3,3,3,3},{3,3,3,3}};

Uint16 pal_col_bck_gb[8][4];
Uint16 pal_col_obj_gb[8][4];
Uint16 pal_col_bck[8][4];
Uint16 pal_col_obj[8][4];
Uint16 Filter[32768];

Uint8 back_col[170][170];

Uint8 nb_spr;

struct mask_shift tab_ms[8]={
  { 0x80,7 },
  { 0x40,6 },
  { 0x20,5 },
  { 0x10,4 },
  { 0x08,3 },
  { 0x04,2 },
  { 0x02,1 },
  { 0x01,0 }};

/* Video Mode */

extern VIDEO_MODE video_yuy2;
extern VIDEO_MODE video_yv12;
extern VIDEO_MODE video_std;
extern VIDEO_MODE video_gl;

VIDEO_MODE *cur_mode;
SDL_Surface *back_save;
static SDL_Surface *bmp=NULL;

void (*draw_screen)(void);

/* take from VGBC (not VGB) */

int GetValue(int min,int max,int v)
{
  return min+(float)(max-min)*(2.0*(v/31.0)-(v/31.0)*(v/31.0));
}

void save_gb_screen(void)
{
  SDL_BlitSurface(back,NULL,back_save,NULL);
}

SDL_Surface *get_mini_screenshot(void) {
  int i,j;
  static int div=3;

  Uint16 *bufd;
  Uint16 *bufs1;
  Uint16 *bufs2;
  Uint16 *bufs3;
  Uint16 c1,c2,c3;

  if (!bmp)
    bmp=SDL_CreateRGBSurface(SDL_SWSURFACE,SCREEN_X/div,SCREEN_Y/div,BIT_PER_PIXEL,
			     0xf800,0x7e0,0x1f,0x00);
  bufd=(Uint16*)bmp->pixels;

  if (!back_save)
    return NULL;
  
  bufs1=(Uint16*)back_save->pixels;
  bufs2=(Uint16*)back_save->pixels+(back_save->pitch>>1);
  bufs3=(Uint16*)back_save->pixels+back_save->pitch;
  

  for (j=0;j<SCREEN_Y/div;j++) {
    for (i=0;i<SCREEN_X/div;i++) {
      c1=BLEND16_50(BLEND16_50(bufs1[i*div],bufs1[(i*div)+1],0xf7de),bufs1[(i*div)+2],0xf7de);
      c2=BLEND16_50(BLEND16_50(bufs2[i*div],bufs2[(i*div)+1],0xf7de),bufs2[(i*div)+2],0xf7de);
      c3=BLEND16_50(BLEND16_50(bufs3[i*div],bufs3[(i*div)+1],0xf7de),bufs3[(i*div)+2],0xf7de);
      bufd[i]=BLEND16_50(BLEND16_50(c1,c2,0xf7de),c3,0xf7de);
    }
    bufd+=(bmp->pitch>>1);
    bufs1+=(back_save->pitch>>1)*div;
    bufs2+=(back_save->pitch>>1)*div;
    bufs3+=(back_save->pitch>>1)*div;
  }
  return bmp;
}

void GenFilter(void)
{
  Uint16 r,g,b;
  Uint16 nr,ng,nb;

  for (r=0;r<32;r++) {
    for (g=0;g<32;g++) {
      for (b=0;b<32;b++) {
	if (conf.color_filter) {
	nr=GetValue(GetValue(4,14,g),GetValue(24,29,g),r)-4;
	ng=GetValue(GetValue(4+GetValue(0,5,r),14+GetValue(0,3,r),b),
		    GetValue(24+GetValue(0,3,r),29+GetValue(0,1,r),b),g)-4;
	nb=GetValue(GetValue(4+GetValue(0,5,r),14+GetValue(0,3,r),g),
	GetValue(24+GetValue(0,3,r),29+GetValue(0,1,r),g),b)-4;
	} else {
	  ng=g;
	  nb=b;
	  nr=r;
	}
	Filter[(b<<10)|(g<<5)|r]=(nr<<11)|(ng<<6)|nb;
      }
    }
  }
}

void update_all_pal(void) {
  Uint8 p,c;
  for(p=0;p<8;p++)
    for(c=0;c<4;c++) {
      pal_col_bck[p][c]=Filter[pal_col_bck_gb[p][c]&0x7FFF];
      pal_col_obj[p][c]=Filter[pal_col_obj_gb[p][c]&0x7FFF];
    }
}

void gb_set_pal(int i) {

    grey[0]=COL32_TO_16(conf.pal[i][0]); //0xc618; // ffe6ce
    grey[1]=COL32_TO_16(conf.pal[i][1]); //0x8410; // bfad9a
    grey[2]=COL32_TO_16(conf.pal[i][2]); //0x4208; // 7f7367
    grey[3]=COL32_TO_16(conf.pal[i][3]); //0x0000; // 3f3933

}

void init_pallete(void)
{
  gb_set_pal(0);
  GenFilter();
}

/* Reinit the screen after a resize event (For YUV & GL)*/
void reinit_vram(void)
{
  if (cur_mode->reinit) cur_mode->reinit();
}

void init_vram(Uint32 flag)
{
  char *mousedrv=getenv("SDL_NOMOUSE");
  
  if (SDL_Init(SDL_INIT_JOYSTICK|SDL_INIT_VIDEO)<0) {
    printf("Couldn't initialize SDL: %s\n",SDL_GetError());
    exit(1);
  }
  atexit(SDL_Quit);   

  /* Define the video mode to use */
  if (conf.yuv && conf.yuv_type==0)
    cur_mode=&video_yv12;
  else if (conf.yuv && conf.yuv_type==1)
    cur_mode=&video_yuy2;
#ifdef SDL_GL
  else if (conf.gl)
    cur_mode=&video_gl;
#endif
  else
    cur_mode=&video_std;

  SDL_WM_SetCaption("Gngb",NULL); 
  if (mousedrv==NULL) 
    SDL_ShowCursor(0);

  init_message();

  conf.video_flag=flag;
  cur_mode->init(flag);

  back=SDL_CreateRGBSurface(SDL_SWSURFACE,SCREEN_X,SCREEN_Y+1,BIT_PER_PIXEL,
			    0xf800,0x7e0,0x1f,0x00);
  back_save=SDL_CreateRGBSurface(SDL_SWSURFACE,SCREEN_X,SCREEN_Y,BIT_PER_PIXEL,
				 0xf800,0x7e0,0x1f,0x00);

  if (back==NULL) {
    printf("Couldn't allocate %dx%dx%d SDL_Surface: %s\n",
	   SCREEN_X,SCREEN_Y,BIT_PER_PIXEL,SDL_GetError());
    exit(1);
  }
  

  init_pallete();
  /*
    if (conf.gb_type&COLOR_GAMEBOY) draw_screen=cur_mode->draw_col;
    else if (conf.gb_type&SUPER_GAMEBOY) draw_screen=cur_mode->draw_sgb;
    else draw_screen=cur_mode->draw_wb;
  */
  if (conf.gb_type&COLOR_GAMEBOY) draw_screen=draw_screen_col_std;
  else if (conf.gb_type&SUPER_GAMEBOY) draw_screen=draw_screen_sgb_std;
  else draw_screen=draw_screen_wb_std;
  
  //blit_screen=cur_mode->blit;

}

void blit_screen(void)
{
  update_key();
  update_message();  
  cur_mode->blit();
}

void switch_fullscreen(void) {
  SDL_WM_ToggleFullScreen(gb_screen);
  conf.fs^=1;
}

__inline__ Uint8 get_nb_spr(void)
{
  Uint8 *sp=oam_space;
  Sint16 no_tile,x,y,att;
  Uint8 sizey;
  Uint8 i,yoff,xoff;

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
      /* Pas plus de 10 sinon la pause de eur_zld deconne */
      if (nb_spr>10) {
	nb_spr=10;
	return nb_spr;
      }
      
    }
  }
  return nb_spr;    
}

__inline__ void clear_screen(void)
{
  Uint8 i,j;
  //  if (cur_mode->clear_scr) cur_mode->clear_scr();
  if (current_menu!=NULL) return;
  clear_screen_std();
  for(i=0;i<170;i++)
    for(j=0;j<170;j++)
      back_col[i][j]=0x00;
}


