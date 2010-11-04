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
#define BEGIN_VBLANK_PER 4

#define NO_INT 0
#define VBLANK_INT 0x01
#define LCDC_INT 0x02
#define TIMEOWFL_INT 0x04
#define SERIAL_INT 0x08

extern UINT16 timer_clk_inc;
extern UINT32 nb_cycle;

typedef struct {
  INT16 cycle_todo;
  UINT8 mode;
  
  UINT32 vblank_cycle;

  UINT16 mode1cycle;
  UINT16 mode2cycle;

}GBLCDC;

GBLCDC *gblcdc;

UINT8 skip_next_frame;

void gblcdc_init(void);

void go2double_speed(void);
void go2simple_speed(void);
inline void main_loop(void);
UINT32 get_nb_cycle(void);

void set_interrupt(UINT8 n);
UINT8 make_interrupt(UINT8 n);
UINT16 lcdc_update(void);
UINT16 lcdc_update_off(void);
void timer_update(void);
void halt_update(void); 

#endif




