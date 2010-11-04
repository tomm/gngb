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

#define NO_INT 0
#define VBLANK_INT 0x01
#define LCDC_INT 0x02
#define TIMEOWFL_INT 0x04
#define SERIAL_INT 0x08

extern UINT32 nb_cycle;

#define GBLCDC_ADD_CYCLE(n) { \
      gblcdc->cycle-=a; \
      if (gblcdc->cycle<=0) gblcdc->cycle+=(gblcdc_update()); }
/*if ((LCDCSTAT&0x03)==0x02) { \
	gblcdc->oam_last_p=gblcdc->oam_pixel; \
        gblcdc->oam_pixel+=((n)*gblcdc->oam_factor); \
	} else if ((LCDCSTAT&0x03)==0x03) gblcdc->vram_pixel+=((n)*gblcdc->vram_factor); \
	}*/
       

typedef struct {
  UINT8 mode;
  UINT8 nb_spr;
  UINT8 inc_line;
  
  INT16 cycle;
  UINT16 mode1cycle;
  UINT16 mode2cycle;
  UINT16 mode3cycle;
  UINT32 vblank_cycle;
  UINT8 timing;

  double vram_factor;
  UINT8 *vram_pal_line[160];
}GBLCDC;

GBLCDC *gblcdc;

UINT8 vram_pal_line_temp[160][4];
extern UINT8 vram_init_pal;

#define gb_set_pal_bck(v) { \
      int i; \
      BGPAL=v;\
      if ((LCDCSTAT&0x03)==0x03) { \
          UINT8 x=(double)((gblcdc->mode3cycle-gblcdc->cycle))*gblcdc->vram_factor;\
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
  UINT16 clk_inc;
  UINT32 cycle;
}GBTIMER;

GBTIMER *gbtimer;

UINT8 skip_next_frame;

void gblcdc_init(void);
void gblcdc_reset(void);

void gbtimer_init(void);
void gbtimer_reset(void);

void go2double_speed(void);
void go2simple_speed(void);
UINT32 get_nb_cycle(void);

void set_interrupt(UINT8 n);
void unset_interrupt(UINT8 n);
UINT8 make_interrupt(UINT8 n);
UINT8 request_interrupt(UINT8 n);

void gblcdc_set_on(void);
void gblcdc_addcycle(INT32 c);
UINT16 gblcdc_update(void);
void gbtimer_update(void);
void halt_update(void); 

#endif




