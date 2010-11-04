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


#include <SDL/SDL.h>
#include "interrupt.h"
#include "frame_skip.h"
#include "memory.h"
#include "vram.h"
#include "cpu.h"

#ifdef DEBUG
#include "log.h"
#endif

UINT16 tb_vram_cycle[2][11]={{170,184,196,208,220,234,244,260,270,284,296},
                             {342,370,394,420,445,470,492,518,543,568,593}}; 

UINT16 tb_hblank_cycle[2][11]={{204,190,178,166,154,140,130,114,104,90,78},
                               {405,380,356,330,305,280,258,232,207,182,157}};

UINT32 nb_cycle=0;
UINT32 last_cycle_do=0;

UINT8 lcdc_mode=OAM_PER;
UINT16 lcdc_mode_clk[4];

UINT8 cur_lcdc_mode;

UINT16 mode0cycle;
UINT16 mode1cycle;
UINT16 mode2cycle=82;
UINT16 mode3cycle;

UINT16 timer_clk_inc=0;   // nb_cycle when inc timer
INT16 lcdc_cycle=0;

UINT32 get_nb_cycle(void)
{
  UINT32 t=nb_cycle;
  SDL_LockAudio();
  nb_cycle=0;
  SDL_UnlockAudio();
  return t;
}

void go2double_speed(void)
{
  if (gbcpu->mode==DOUBLE_SPEED) return;
  mode0cycle=tb_hblank_cycle[1][0];
  mode1cycle=912;
  mode2cycle=162;
  mode3cycle=tb_vram_cycle[1][0];
  gbcpu->mode=DOUBLE_SPEED;
  vblank_cycle=70224*2;
}

void go2simple_speed(void)
{
  if (gbcpu->mode==SIMPLE_SPEED) return;
  mode0cycle=tb_hblank_cycle[0][0];
  mode1cycle=456;
  mode2cycle=82;
  mode3cycle=tb_vram_cycle[0][0];
  gbcpu->mode=SIMPLE_SPEED;
  vblank_cycle=70224;
}

inline UINT8 make_interrupt(UINT8 n) {
  if ((INT_ENABLE&n) && (gbcpu->int_flag)) {
#ifdef DEBUG
    put_log("make int %d CURLINE %d CMP_LINE %d INT_ENABLE %02x \n",n,CURLINE,CMP_LINE,INT_ENABLE);
#endif
    INT_FLAG&=(~n);
    gbcpu->int_flag=0;
    push_stack_word(gbcpu->pc.w);
    switch(n) {
    case VBLANK_INT:gbcpu->pc.w=0x0040;break;
    case LCDC_INT:gbcpu->pc.w=0x0048;break;
    case TIMEOWFL_INT:gbcpu->pc.w=0x0050;break;
    }   
    return 1;
  }
  return 0;
}

inline UINT16 lcdc_update(void)  // LCDC is on
{
  UINT16 ret=0;
  static UINT8 inc_line;
  UINT8 skip_this_frame;
  // static UINT8 skip_next_frame;
  
  if (inc_line) CURLINE++;
  
  if (CURLINE==0x90) {
    lcdc_mode=VBLANK_PER;
    /*if (LCDCSTAT&0x10) INT_FLAG|=LCDC_INT;
      else*/ INT_FLAG|=VBLANK_INT;
  }
  
 
  
  if (CURLINE==0x9a) {
    CURLINE=0;
    if (LCDCCONT&0x80) {
     if (conf.autofs) {
       skip_this_frame=skip_next_frame;
       //      skip_next_frame=barath_skip_next_frame(0);
       if (!skip_this_frame) blit_screen();
     }
     else blit_screen();
    }
    lcdc_mode=OAM_PER;
    // printf("%d \n",get_nb_cycle());
  }
  LCDCSTAT&=0xf8;
  if (CMP_LINE==CURLINE) LCDCSTAT|=0x04;
  switch(lcdc_mode) {
  case HBLANK_PER:       // HBLANK
    if (LCDCSTAT&0x08 && LCDCCONT&0x80) INT_FLAG|=LCDC_INT;
    if (LCDCCONT&0x80) {
      if (conf.autofs) {
         if (!skip_next_frame) draw_screen();
      }
      else draw_screen();
    }
    ret=tb_hblank_cycle[gbcpu->mode][nb_spr];
    lcdc_mode=OAM_PER;
    inc_line=1;
    if (dma_info.type==HDMA) do_hdma();
    break;
  case VBLANK_PER:       // VBLANK
    LCDCSTAT|=0x01;
    ret=mode1cycle;
    inc_line=1;
    break;
  case OAM_PER:       // OAM 
    LCDCSTAT|=0x02;
    inc_line=0;
    if (LCDCSTAT&0x20 && LCDCCONT&0x80) INT_FLAG|=LCDC_INT;
    if (LCDCSTAT&0x40 && LCDCSTAT&0x04 && LCDCCONT&0x80) INT_FLAG|=LCDC_INT;
    ret=mode2cycle;
    lcdc_mode=VRAM_PER;
    nb_spr=get_nb_spr();
    break;
  case VRAM_PER:       // VRAM
    LCDCSTAT|=0x03;
    ret=tb_vram_cycle[gbcpu->mode][nb_spr];
    lcdc_mode=HBLANK_PER;
    break;
  }
  return ret;
}

inline void timer_update(void)
{
  INT16 t=TIME_COUNTER+1;  //timer_inc;
  if (t>0xff) {
    TIME_COUNTER=TIME_MOD;
    INT_FLAG|=TIMEOWFL_INT;
  } else TIME_COUNTER=((UINT16)(t))&0xff;
}

inline void halt_update(void) // gbcpu->state=HALT_STATE
{ 
  if (INT_FLAG&INT_ENABLE) {
    gbcpu->state=0;
    gbcpu->pc.w++;
#ifdef DEBUG
    put_log("stop halt\n");
#endif
  }
}


		 
  
