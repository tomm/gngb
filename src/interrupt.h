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

#ifndef _INTERUPT_H
#define _INTERUPT_H

#include "global.h"

#define HBLANK_PER 0
#define VBLANK_PER 1
#define OAM_PER 2
#define VRAM_PER 3
#define END_VBLANK_PER 4
#define LINE_90_BEGIN 5
#define LINE_90_END 6
#define BEGIN_OAM_PER 7
#define LINE_99 8

#define NO_INT 0
#define VBLANK_INT 0x01
#define LCDC_INT 0x02
#define TIMEOWFL_INT 0x04
#define SERIAL_INT 0x08

extern Uint32 nb_cycle;

#define GBLCDC_ADD_CYCLE(n) { \
      gblcdc->cycle-=(n); \
      if (gblcdc->cycle<=0) gblcdc->cycle+=(gblcdc_update()); }
/*if ((LCDCSTAT&0x03)==0x02) { \
	gblcdc->oam_last_p=gblcdc->oam_pixel; \
        gblcdc->oam_pixel+=((n)*gblcdc->oam_factor); \
	} else if ((LCDCSTAT&0x03)==0x03) gblcdc->vram_pixel+=((n)*gblcdc->vram_factor); \
	}*/
       

typedef struct {
  Uint8 mode;
  Uint8 nb_spr;
  Uint8 inc_line;
  
  Sint16 cycle;
  Uint16 mode1cycle;
  Uint16 mode2cycle;
  Uint16 mode3cycle;
  Uint32 vblank_cycle;
  Uint8 timing;

  Uint8 win_curline;		/* Gameboy Window Current Line */

  double vram_factor;
  Uint8 *vram_pal_line[160];
}GBLCDC;

GBLCDC *gblcdc;

Uint8 vram_pal_line_temp[160][4];
extern Uint8 vram_init_pal;

#define gb_set_pal_bck(v) { \
      int i; \
      BGPAL=v;\
      if ((LCDCSTAT&0x03)==0x03) { \
          Uint8 x=(double)((gblcdc->mode3cycle-gblcdc->cycle))*gblcdc->vram_factor;\
          vram_pal_line_temp[x][0]=BGPAL&3; \
	  vram_pal_line_temp[x][1]=(BGPAL>>2)&3; \
	  vram_pal_line_temp[x][2]=(BGPAL>>4)&3; \
	  vram_pal_line_temp[x][3]=(BGPAL>>6)&3; \
          for(i=x;i<160;i++) \
              gblcdc->vram_pal_line[i]=vram_pal_line_temp[x]; \
          vram_init_pal=1; \
      } else { \
           pal_bck[0]=BGPAL&3;\
           pal_bck[1]=(BGPAL>>2)&3;\
           pal_bck[2]=(BGPAL>>4)&3;\
           pal_bck[3]=(BGPAL>>6)&3;\
      } \
   }

typedef struct {
  Uint16 clk_inc;
  Sint32 cycle;
}GBTIMER;

GBTIMER *gbtimer;

void gblcdc_init(void);
void gblcdc_reset(void);

void gbtimer_init(void);
void gbtimer_reset(void);

void go2double_speed(void);
void go2simple_speed(void);
Uint32 get_nb_cycle(void);

#ifdef DEBUG
void set_interrupt(Uint8 n);
void unset_interrupt(Uint8 n);
#else 
#define set_interrupt(n) ((INT_FLAG|=(n)))
#define unset_interrupt(n) ((INT_FLAG&=(~(n))))
#endif

Uint8 make_interrupt(Uint8 n);

void gblcdc_set_on(void);
void gblcdc_addcycle(Sint32 c);
Uint16 gblcdc_update(void);
void gbtimer_update(void);
void halt_update(void); 

#endif




