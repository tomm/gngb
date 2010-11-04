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
#include "vram.h"
#include "message.h"
#include "tiny_font.h"
#include "emu.h"
#include "interrupt.h"
#include "sgb.h"
#include "video_std.h"
#include "menu.h"

static Uint32 std_flag;
SDL_Surface *back=NULL;


Sint8 rb_tab[2][RB_SIZE]={{0,-2,2,-2,2},
			 {0,-1,-1,1,1}};

VIDEO_MODE video_std;

SDL_Rect clip_rct;
Uint8 win_line=0;

/*
static __inline__ Uint16 color_blend(Uint16 A,Uint16 B,Uint16 C,Uint16 D)
{
  return BLEND2x16_50(BLEND2x16_50(BLEND16_50(D,0,0xf7de),BLEND2x16_50(B,C,0xf7de),0xf7de),A,0xf7de);
  //  return BLEND16_50(BLEND16_50(A,D,0xf7de),BLEND16_50(B,C,0xf7de),0xf7de);
}
*/

//#define FILTER_S2X (random&1?(A==D?A:(B==C?B:A)):A)
#define FILTER_S2X (A==D?A:(B==C?B:A))


//#define FILTER_SMTH color_blend(A,B,C,D)
#define FILTER_SMTH (BLEND2x16_50(BLEND2x16_50(BLEND16_50(D,0,0xf7de),BLEND2x16_50(B,C,0xf7de),0xf7de),A,0xf7de));

int (*filtered_blit)(SDL_Surface *src,SDL_Rect *srcrect,SDL_Surface *dst,SDL_Rect *dstrect);


int blit_std_with_scanline(SDL_Surface *src,SDL_Rect *srcrect,SDL_Surface *dst,SDL_Rect *dstrect)
{
  int i,j;
  int w=dst->w>>1;
  int h=dst->h>>1;
  Uint16* bufs=(Uint16*)src->pixels;
  Uint16* bufd=(Uint16*)dst->pixels;

  /* rumble support */
  static Uint8 rumble=0;
  static Uint8 rb_time=0;
  if (rb_on) {
    rumble=2-rumble;
    bufs+=rumble;
    rb_time++;
    if (rb_time>8) 
      rb_time=rb_on=0;
  }

  for(j=0;j<h;j++) {
    for(i=0;i<w;i++) {
      bufd[i<<1]=bufd[(i<<1)+1]=bufs[i];
    }
    bufd+=dst->pitch;
    bufs+=(src->pitch>>1);
  }
  return 0;
}

int blit_std_with_scanline50(SDL_Surface *src,SDL_Rect *srcrect,SDL_Surface *dst,SDL_Rect *dstrect)
{
  int i,j;
  int w=dst->w>>1;
  int h=dst->h>>1;
  Uint16* bufs=(Uint16*)src->pixels+(src->pitch>>1);
  Uint16* nbufs=(Uint16*)src->pixels;
  Uint16* bufd=(Uint16*)dst->pixels;
  Uint16* nbufd=(Uint16*)dst->pixels+(dst->pitch>>1);
  
  static Uint8 rumble=0;
  static Uint8 rb_time=0;
  if (rb_on) {
    rumble=2-rumble;
    bufs+=rumble;
    rb_time++;
    if (rb_time>8) 
      rb_time=rb_on=0;
  }


  for(j=0;j<h;j++) {
    for(i=0;i<w;i++) {
      bufd[i<<1]=bufd[(i<<1)+1]=bufs[i];
      nbufd[i<<1]=nbufd[(i<<1)+1]=BLEND16_50(bufs[i],COL32_TO_16(0X303030),0xf7de);
    }
    bufd+=dst->pitch;
    nbufd+=dst->pitch;
    bufs+=(src->pitch>>1);
    nbufs+=(src->pitch>>1);
  }
  return 0;
}

int blit_std_with_filter_s2x(SDL_Surface *src,SDL_Rect *srcrect,SDL_Surface *dst,SDL_Rect *dstrect){
  Uint16* bufs=(Uint16*)src->pixels;
  Uint16* pbufs=(Uint16*)src->pixels;
  Uint16* nbufs=(Uint16*)src->pixels+(src->pitch>>1);
  Uint16* bufd=(Uint16*)dst->pixels;
  Uint16* nbufd=(Uint16*)dst->pixels+(dst->pitch>>1);

  int w=dst->w>>1;
  int h=dst->h>>1;
  int i,j;
  Uint16 A,B,C,D;
  Uint16 col;
  Uint16 t;
  
  
  static Uint8 rumble=0;
  static Uint8 rb_time=0;




  if (rb_on) {
    rumble=2-rumble;
    bufs+=rumble;
    nbufs+=rumble;
    pbufs+=rumble;
    rb_time++;
    if (rb_time>8) 
      rb_time=rb_on=0;
  }


  for(i=0;i<w;i++) {
    A=bufs[i];
    bufd[i<<1]=A;
    bufd[(i<<1)+1]=A;
    if (i==0) {
      nbufd[i<<1]=A;
    } else {
      C=bufs[i-1];
      D=nbufs[i-1];
      B=nbufs[i];
      nbufd[i<<1]=FILTER_S2X;
    }
    if (i==w-1) {
      nbufd[(i<<1)+1]=A;
    } else {
      C=bufs[i+1];
      D=nbufs[i+1];
      B=nbufs[i];
      nbufd[(i<<1)+1]=FILTER_S2X;
    }
  }
  bufs+=(src->pitch>>1);
  nbufs+=(src->pitch>>1);
  bufd+=(dst->pitch>>1);
  nbufd+=(dst->pitch>>1);

  for(j=1;j<h-1;j++) {
    for(i=0;i<w;i++) {
      A=bufs[i];
      t=0;
      if (i==0) {
	bufd[i<<1]=A;
      } else {
	C=bufs[i-1];
	D=pbufs[i-1];
	B=pbufs[i];
	col=FILTER_S2X;
	t+=(col==A);
	bufd[i<<1]=col;
      }
      if (i==w-1) {
	bufd[(i<<1)+1]=A;
      } else {
	C=bufs[i+1];
	D=pbufs[i+1];
	B=pbufs[i];
	col=FILTER_S2X;
	t+=(col==A);
	bufd[(i<<1)+1]=col;
      }

      if (i==0) {
	nbufd[i<<1]=A;
      } else {
	C=bufs[i-1];
	D=nbufs[i-1];
	B=nbufs[i];
	col=FILTER_S2X;
	t+=(col==A);
	nbufd[i<<1]=col;
      }
      if (i==w-1 || t==0) {
	nbufd[(i<<1)+1]=A;
      } else {
	C=bufs[i+1];
	D=nbufs[i+1];
	B=nbufs[i];
	nbufd[(i<<1)+1]=FILTER_S2X;
      }
    }
    bufs+=(src->pitch>>1);
    nbufs+=(src->pitch>>1);
    pbufs+=(src->pitch>>1);
    bufd+=dst->pitch;
    nbufd+=dst->pitch;
  }

  for(i=0;i<w;i++) {
    A=bufs[i];
    
    if (i==0) {
      bufd[i<<1]=A;
    } else {
      C=bufs[i-1];
      D=pbufs[i-1];
      B=pbufs[i];
      bufd[i<<1]=FILTER_S2X;
    }
    if (i==w-1) {
      bufd[(i<<1)+1]=A;
    } else {
      C=bufs[i+1];
      D=pbufs[i+1];
      B=pbufs[i];
      bufd[(i<<1)+1]=FILTER_S2X;
    }
    nbufd[i<<1]=A;
    nbufd[(i<<1)+1]=A;
  }
  return 0;
}
int blit_std_with_filter_smooth(SDL_Surface *src,SDL_Rect *srcrect,SDL_Surface *dst,SDL_Rect *dstrect){
  Uint16* bufs=(Uint16*)src->pixels;
  Uint16* pbufs=(Uint16*)src->pixels;
  Uint16* nbufs=(Uint16*)src->pixels+(src->pitch>>1);
  Uint16* bufd=(Uint16*)dst->pixels;
  Uint16* nbufd=(Uint16*)dst->pixels+(dst->pitch>>1);

  int w=dst->w>>1;
  int h=dst->h>>1;
  int i,j;
  Uint16 A,B,C,D;
  static Uint8 rumble=0;
  static Uint8 rb_time=0;
  if (rb_on) {
    rumble=2-rumble;
    bufs+=rumble;
    nbufs+=rumble;
    pbufs+=rumble;
    rb_time++;
    if (rb_time>8) 
      rb_time=rb_on=0;
  }

  
  for(i=0;i<w;i++) {
    A=bufs[i];
    bufd[i<<1]=A;
    bufd[(i<<1)+1]=A;
    if (i==0) {
      nbufd[i<<1]=A;
    } else {
      C=bufs[i-1];
      D=nbufs[i-1];
      B=nbufs[i];
      nbufd[i<<1]=FILTER_SMTH;
    }
    if (i==w-1) {
      nbufd[i<<1]=A;
    } else {
      C=bufs[i+1];
      D=nbufs[i+1];
      B=nbufs[i];
      nbufd[(i<<1)+1]=FILTER_SMTH;
    }
  }
  bufs+=(src->pitch>>1);
  nbufs+=(src->pitch>>1);
  bufd+=(dst->pitch>>1);
  nbufd+=(dst->pitch>>1);

  for(j=1;j<h-1;j++) {
    for(i=0;i<w;i++) {
      A=bufs[i];

      if (i==0) {
	bufd[i<<1]=A;
      } else {
	C=bufs[i-1];
	D=pbufs[i-1];
	B=pbufs[i];
	bufd[i<<1]=FILTER_SMTH;
      }
      if (i==w-1) {
	bufd[(i<<1)+1]=A;
      } else {
	C=bufs[i+1];
	D=pbufs[i+1];
	B=pbufs[i];
	bufd[(i<<1)+1]=FILTER_SMTH;
      }

      if (i==0) {
	nbufd[i<<1]=A;
      } else {
	C=bufs[i-1];
	D=nbufs[i-1];
	B=nbufs[i];
	nbufd[i<<1]=FILTER_SMTH;
      }
      if (i==w-1) {
	nbufd[(i<<1)+1]=A;
      } else {
	C=bufs[i+1];
	D=nbufs[i+1];
	B=nbufs[i];
	nbufd[(i<<1)+1]=FILTER_SMTH;
      }
    }
    bufs+=(src->pitch>>1);
    nbufs+=(src->pitch>>1);
    pbufs+=(src->pitch>>1);
    bufd+=dst->pitch;
    nbufd+=dst->pitch;
  }

  for(i=0;i<w;i++) {
    A=bufs[i];
    
    if (i==0) {
      bufd[i<<1]=A;
    } else {
      C=bufs[i-1];
      D=pbufs[i-1];
      B=pbufs[i];
      bufd[i<<1]=FILTER_SMTH;
    }
    if (i==w-1) {
      bufd[(i<<1)+1]=A;
    } else {
      C=bufs[i+1];
      D=pbufs[i+1];
      B=pbufs[i];
      bufd[(i<<1)+1]=FILTER_SMTH;
    }
    nbufd[i<<1]=A;
    nbufd[(i<<1)+1]=A;
    
  }
  return 0;
}
int blit_std_with_mblur(SDL_Surface *src,SDL_Rect *srcrect,SDL_Surface *dst,SDL_Rect *dstrect){
  SDL_SetAlpha(src,SDL_SRCALPHA,255);
  if (current_menu==NULL) {
    SDL_BlitSurface(back_save,srcrect,dst,dstrect);
    SDL_BlitSurface(src,NULL,back_save,NULL);
    SDL_SetAlpha(src,SDL_SRCALPHA,128);
  }
  SDL_BlitSurface(src,srcrect,dst,dstrect);
  return 0;
}

void set_filter(int filter) {
  conf.filter=filter;
  conf.fs=0;
  if (conf.gb_type&SUPER_GAMEBOY)
    gb_screen=SDL_SetVideoMode(SGB_WIDTH,SGB_HEIGHT,
			       BIT_PER_PIXEL,std_flag);
  else {
    if (conf.filter) {
      if (conf.filter==5) {
	gb_screen=SDL_SetVideoMode(SCREEN_X,SCREEN_Y,
				   BIT_PER_PIXEL,std_flag);
      }
      else
	gb_screen=SDL_SetVideoMode(SCREEN_X*2,SCREEN_Y*2,
				   BIT_PER_PIXEL,std_flag);
      SDL_FillRect(gb_screen,NULL,0);
    }
    else
      gb_screen=SDL_SetVideoMode(SCREEN_X,SCREEN_Y,
				 BIT_PER_PIXEL,std_flag); 
  }

  switch (conf.filter) {
  case 1:  filtered_blit=blit_std_with_scanline;break;
  case 2:  filtered_blit=blit_std_with_scanline50;break;
  case 3:  filtered_blit=blit_std_with_filter_smooth;break;
  case 4:  filtered_blit=blit_std_with_filter_s2x;break;
  case 5:  filtered_blit=blit_std_with_mblur;break;
  default: filtered_blit=SDL_BlitSurface;break;
  }
}

void blit_screen_default_std(void) {
 
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
  
  filtered_blit(back,&scrR,gb_screen,&dstR);

  SDL_Flip(gb_screen);
  /* FIXME: clear_screen */
  if ((!(LCDCCONT&0x20) || !(LCDCCONT&0x01)) && (conf.gb_type&NORMAL_GAMEBOY)) clear_screen();
}

void blit_screen_sgb_std(void) {
  
  if (rb_on) {
    SDL_SetClipRect(gb_screen,&clip_rct);

      rb_shift++;
      if (rb_shift>=RB_SIZE) {
	rb_on=0;
	rb_shift=0;
	SDL_SetClipRect(gb_screen,NULL);
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


  SDL_Flip(gb_screen);
  /* FIXME: clear_screen */
  if ((!(LCDCCONT&0x20) || !(LCDCCONT&0x01)) && (conf.gb_type&NORMAL_GAMEBOY))
    clear_screen();
}

void init_video_std(Uint32 flag) {
  std_flag=flag|SDL_HWSURFACE;

  if (conf.gb_type&SUPER_GAMEBOY)
    gb_screen=SDL_SetVideoMode(SGB_WIDTH,SGB_HEIGHT,
			       BIT_PER_PIXEL,std_flag);
  else {
    if (conf.filter) {
      if (conf.filter==5) {
	gb_screen=SDL_SetVideoMode(SCREEN_X,SCREEN_Y,
				   BIT_PER_PIXEL,std_flag);
      }
      else
	gb_screen=SDL_SetVideoMode(SCREEN_X*2,SCREEN_Y*2,
				   BIT_PER_PIXEL,std_flag);
      SDL_FillRect(gb_screen,NULL,0);
    }
    else
      gb_screen=SDL_SetVideoMode(SCREEN_X,SCREEN_Y,
				 BIT_PER_PIXEL,std_flag); 
  }

  switch (conf.filter) {
  case 1:  filtered_blit=blit_std_with_scanline;break;
  case 2:  filtered_blit=blit_std_with_scanline50;break;
  case 3:  filtered_blit=blit_std_with_filter_smooth;break;
  case 4:  filtered_blit=blit_std_with_filter_s2x;break;
  case 5:  filtered_blit=blit_std_with_mblur;break;
  default: filtered_blit=SDL_BlitSurface;break;
  }

    if (gb_screen==NULL) {
      printf("Couldn't set %dx%dx%d video mode: %s\n",
	     SCREEN_X,SCREEN_Y,BIT_PER_PIXEL,SDL_GetError());
      exit(1);
    }
   
    
    if (conf.gb_type&SUPER_GAMEBOY) {
      scxoff=48;
      scyoff=40;
      video_std.blit=blit_screen_sgb_std;
    } else {
      scxoff=0;
      scyoff=0;
      video_std.blit=blit_screen_default_std;
    }
    clip_rct.x=scxoff;
    clip_rct.y=scyoff;
    clip_rct.w=SCREEN_X;
    clip_rct.h=SCREEN_Y;

}

void blit_sgb_mask_std(void)
{
  SDL_BlitSurface(sgb_buf,NULL,gb_screen,NULL);
}

__inline__ void draw_spr_std(Uint16 *buf,GB_SPRITE *sp)
{
  Uint8 *tp;
  Uint8 nx;
  Uint8 wbit,c;

  tp=&vram_page[0][sp->no_tile<<4];
  if (!sp->yflip) tp+=((sp->yoff)<<1);
  else tp+=(sp->sizey-1-sp->yoff)<<1;
  
  for(nx=sp->xoff;nx<8;nx++) {
    if (!sp->xflip) wbit=nx;
    else wbit=7-nx;	
    c=GET_GB_PIXEL(wbit);
    if (c) {
      if (!(sp->priority)) buf[sp->x+nx]=grey[pal_obj[sp->pal][c]];
      else if (!back_col[sp->x+nx][CURLINE]) buf[sp->x+nx]=grey[pal_obj[sp->pal][c]];
    }
  }
}

__inline__ void draw_obj_std(Uint16 *buf)
{
  Sint8 i;
  for(i=nb_spr-1;i>=0;i--) 
    draw_spr_std(buf,&gb_spr[(Uint8)i]);
}

__inline__ void draw_back_std(Uint16 *buf)
{
  Uint8 *tb,*tp;
  int y,x,i;
  int sx,sy;
  Uint8 c;
  Sint16 no_tile;
 
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
    c=GET_GB_PIXEL(sx);
    /* FIXME: VIC VIPER Laser */
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
      c=GET_GB_PIXEL(sx);
      /* FIXME: VIC VIPER Laser */
      buf[x+sx]=grey[gblcdc->vram_pal_line[x+sx][c]];
      back_col[x+sx][CURLINE]=c;
    }
  }
}

Uint8 win_curline=0;

__inline__ void draw_win_std(Uint16 *buf)
{
  Uint8 *tb,*tp;
  Sint16 y,x,i,sx=0;
  Sint16 no_tile;
  Uint8 c;
   
  if (LCDCCONT&0x40) tb=&vram_page[0][0x1c00];
  else tb=&vram_page[0][0x1800];
  
  if (WINX>=166) return;
  
  if (CURLINE>=WINY) {
    
    /*tb+=(((CURLINE-WINY)>>3)<<5);
      y=((CURLINE-WINY)&0x07)<<1;*/
    tb+=(((gblcdc->win_curline/*-WINY*/)>>3)<<5);
    y=((gblcdc->win_curline/*-WINY*/)&0x07)<<1;
    x=(((WINX-7)<0)?0:(WINX-7));
    for(i=0;x<160;x+=8,i++) {
      no_tile=tb[i];
      if (!(LCDCCONT&0x10)) no_tile=256+(signed char)no_tile;
      tp=&vram_page[0][no_tile<<4];
      tp+=y;
      for(sx=0;sx<8 && (x+sx)<160;sx++) {
	c=GET_GB_PIXEL(sx);
	/* FIXME: VIC VIPER Laser */
	buf[x+sx]=grey[gblcdc->vram_pal_line[x+sx][c]];
	back_col[x+sx][CURLINE]=c;
      }
    }
    gblcdc->win_curline++;
  }  
}

/* CGB drawing functions */

__inline__ void draw_spr_col_std(Uint16 *buf,GB_SPRITE *sp)
{
  Uint8 *tp;
  Uint8 nx;
  Uint8 wbit,c;

  tp=&vram_page[sp->page][sp->no_tile<<4];
  if (!sp->yflip) tp+=((sp->yoff)<<1);
  else tp+=(sp->sizey-1-sp->yoff)<<1;
  
  for(nx=sp->xoff;nx<8;nx++) {
    if (!sp->xflip) wbit=nx;
    else wbit=7-nx;	
    c=GET_GB_PIXEL(wbit);

    /* TODO: OPTIMISATION */
    /* if (back_col[sp->x+nx][CURLINE]&0x80) {  
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


__inline__ void draw_obj_col_std(Uint16 *buf)
{
  Sint8 i;
  for(i=nb_spr-1;i>=0;i--) 
    draw_spr_col_std(buf,&gb_spr[(Uint8)i]);
}

__inline__ void draw_back_col_std(Uint16 *buf)
{
  Uint8 *tb,*tp,*att_tb;
  Uint16 y,x,i;
  Uint16 sx,sy;
  Uint8 c,p,att,xflip,yflip;
  Sint16 no_tile;
 
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
      c=GET_GB_PIXEL(wbit);
      buf[x+sx]=pal_col_bck[p][c];
      back_col[x+sx][CURLINE]=(att&0x80)+c;
    }
  }
}

__inline__ void draw_win_col_std(Uint16 *buf)
{
  Uint8 *tb,*tp,*att_tb; 
  Uint16 y,x,i,sx;
  Sint16 no_tile;
  Uint8 c,p,att,xflip,yflip;
  
  if (LCDCCONT&0x40) {
    tb=&vram_page[0][0x1c00];
    att_tb=&vram_page[1][0x1c00];
  } else {
    tb=&vram_page[0][0x1800];
    att_tb=&vram_page[1][0x1800];
  }

  if (WINX>=166) return;
  
  if (CURLINE>=WINY) {
    
    /*tb+=(((CURLINE-WINY)>>3)<<5);
      att_tb+=(((CURLINE-WINY)>>3)<<5);
      y=((CURLINE-WINY)&0x07);*/

    tb+=(((gblcdc->win_curline)>>3)<<5);
    att_tb+=(((gblcdc->win_curline)>>3)<<5);
    y=((gblcdc->win_curline)&0x07);

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
	c=GET_GB_PIXEL(wbit);
	buf[x+sx]=pal_col_bck[p][c];
	if (!(att&0x80)) back_col[x+sx][CURLINE]=c;
	else back_col[x+sx][CURLINE]=0x80+c;	    
      }
    }
    gblcdc->win_curline++;
  }  
}


/* SGB drawing Functions  To Optimize */

__inline__ void draw_back_sgb_std(Uint16 *buf)
{
  Uint8 *tb,*tp;
  int y,x,i;
  int sx,sy;
  Uint8 c;
  Sint16 no_tile;

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
      c=GET_GB_PIXEL(sx);
      //buf[x+sx]=sgb_pal[sgb_pal_map[x/8][CURLINE/8]][pal_bck[c]];
      //buf[x+sx]=sgb_pal[sgb_pal_map[x/8][CURLINE/8]][c];
      /* FIXME VIC VIPER */
      buf[x+sx]=sgb_pal[sgb_pal_map[x/8][CURLINE/8]][gblcdc->vram_pal_line[x+sx][c]];
      back_col[x+sx][CURLINE]=c;
    }
  }

}

__inline__ void draw_win_sgb_std(Uint16 *buf)
{
  Uint8 *tb,*tp;
  Sint16 y,x,i,sx;
  Sint16 no_tile;
  Uint8 c;
 
  if (LCDCCONT&0x40) tb=&vram_page[0][0x1c00];
  else tb=&vram_page[0][0x1800];
  
  if (WINX>=166) return;
  
  if (CURLINE>=WINY) {
    
    /*tb+=(((CURLINE-WINY)>>3)<<5);
      y=((CURLINE-WINY)&0x07)<<1;*/
    tb+=(((gblcdc->win_curline/*-WINY*/)>>3)<<5);
    y=((gblcdc->win_curline/*-WINY*/)&0x07)<<1;
    x=(((WINX-7)<0)?0:(WINX-7));
    for(i=0;x<160;x+=8,i++) {
      no_tile=tb[i];
      if (!(LCDCCONT&0x10)) no_tile=256+(signed char)no_tile;
      tp=&vram_page[0][no_tile<<4];
      tp+=y;
      for(sx=0;sx<8 && x+sx<160;sx++) {
	c=GET_GB_PIXEL(sx);
	//buf[x+sx]=pal_bck[c];
	//buf[x+sx]=sgb_pal[sgb_pal_map[x/8][CURLINE/8]][pal_bck[c]];
	buf[x+sx]=sgb_pal[sgb_pal_map[x/8][CURLINE/8]][gblcdc->vram_pal_line[x+sx][c]];
	back_col[x+sx][CURLINE]=c;
      }
    }
    win_curline++;
  }  
}

__inline__ void draw_spr_sgb_std(Uint16 *buf,GB_SPRITE *sp)
{
  Uint8 *tp;
  Uint8 nx;
  Uint8 /*bit0,bit1,*/wbit,c;

  tp=&vram_page[0][sp->no_tile<<4];
  if (!sp->yflip) 
    tp+=((sp->yoff)<<1);
  else tp+=(sp->sizey-1-sp->yoff)<<1;
  
  for(nx=sp->xoff;nx<8;nx++) {
    if (!sp->xflip) wbit=nx;
    else wbit=7-nx;	
    c=GET_GB_PIXEL(wbit);
    if (c) {
      /* FIXME */
      if (!(sp->priority)) buf[sp->x+nx]=sgb_pal[sgb_pal_map[(sp->x+nx)/8][CURLINE/8]][pal_obj[sp->pal][c]];
      else if (!back_col[sp->x+nx][CURLINE]) buf[sp->x+nx]=sgb_pal[sgb_pal_map[(sp->x+nx)/8][CURLINE/8]][pal_obj[sp->pal][c]];       
      
      /*if (!(sp->priority)) buf[sp->x+nx]=sgb_pal[0][pal_obj[sp->pal][c]];
	else if (!back_col[sp->x+nx][CURLINE]) buf[sp->x+nx]=sgb_pal[0][pal_obj[sp->pal][c]];*/
      /*if (!(sp->priority)) buf[sp->x+nx]=sgb_pal[sp->pal_col][c];
	else if (!back_col[sp->x+nx][CURLINE]) buf[sp->x+nx]=sgb_pal[sp->pal_col][c];*/
    }
  }
}

__inline__ void draw_obj_sgb_std(Uint16 *buf)
{
  Sint8 i;
  for(i=nb_spr-1;i>=0;i--) 
    draw_spr_sgb_std(buf,&gb_spr[(Uint8)i]);
}

void draw_screen_wb_std(void)
{
  Uint16 *buf=(Uint16 *)back->pixels + CURLINE*(back->pitch>>1);
  if (SDL_MUSTLOCK(back) && SDL_LockSurface(back)<0) printf("can't lock surface\n");

  if (LCDCCONT&0x01) draw_back_std(buf);
  if (LCDCCONT&0x20) draw_win_std(buf);
  if (LCDCCONT&0x02) draw_obj_std(buf);

  if (SDL_MUSTLOCK(back)) SDL_UnlockSurface(back);
}

void draw_screen_col_std(void) 
{
  Uint16 *buf=(Uint16 *)back->pixels + CURLINE*(back->pitch>>1);
  if (SDL_MUSTLOCK(back) && SDL_LockSurface(back)<0) printf("can't lock surface\n");
  draw_back_col_std(buf);
  if (LCDCCONT&0x20) draw_win_col_std(buf);
  if (LCDCCONT&0x02) draw_obj_col_std(buf);

  if (SDL_MUSTLOCK(back)) SDL_UnlockSurface(back);
}

void draw_screen_sgb_std(void)
{
  Uint16 *buf=(Uint16 *)back->pixels + CURLINE*(back->pitch>>1);
  if (SDL_MUSTLOCK(back) && SDL_LockSurface(back)<0) printf("can't lock surface\n");

  if (LCDCCONT&0x01) draw_back_sgb_std(buf);
  if (LCDCCONT&0x20) draw_win_sgb_std(buf);
  /* FIXME */
  if (LCDCCONT&0x02) draw_obj_sgb_std(buf);

  if (SDL_MUSTLOCK(back)) SDL_UnlockSurface(back);
}

void clear_screen_std(void)
{
  if (back && gb_screen) {
    if (conf.gb_type&COLOR_GAMEBOY) {
      SDL_FillRect(back,NULL,pal_col_bck[0][0]);
      //SDL_FillRect(gb_screen,NULL,pal_col_bck[0][0]);
    } else {
      SDL_FillRect(back,NULL,grey[0]);
      //SDL_FillRect(gb_screen,NULL,grey[0]);
    }   
  }
  SDL_Flip(gb_screen);
}

VIDEO_MODE video_std={
  init_video_std,
  NULL,
  //  draw_screen_col_std,
  //  draw_screen_wb_std,
  //  draw_screen_sgb_std,
  NULL,
  blit_sgb_mask_std,
  //  NULL,
  //  draw_message_std,
  //  clear_screen_std
};



