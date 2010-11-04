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
#include <SDL/SDL.h>
#include "interrupt.h"
#include "frame_skip.h"
#include "memory.h"
#include "vram.h"
#include "cpu.h"

#ifdef DEBUG
#include "debuger/log.h"
#endif

UINT16 tb_vram_cycle[2][11]={{170,184,196,208,220,234,244,260,270,284,296},
                             {340,368,392,416,440,468,488,520,540,568,592}}; 

UINT16 tb_hblank_cycle[2][11]={{204,190,178,166,154,140,130,114,104,90,78},
                               {408,348,356,332,308,280,260,228,208,180,156}};

UINT32 nb_cycle=0;
UINT16 timer_clk_inc=0;   // nb_cycle when inc timer

UINT32 get_nb_cycle(void)
{
  UINT32 t=nb_cycle;
  SDL_LockAudio();
  nb_cycle=0;
  SDL_UnlockAudio();
  return t;
}

void gblcdc_init(void) {
  
  gblcdc=(GBLCDC *)malloc(sizeof(GBLCDC));
  
  gblcdc->vblank_cycle=70224;

  gblcdc->mode1cycle=456;
  gblcdc->mode2cycle=82;

  gblcdc->cycle_todo=gblcdc->mode2cycle;
  gblcdc->mode=OAM_PER;
}

void go2double_speed(void)
{
  if (gbcpu->mode==DOUBLE_SPEED) return;
  gblcdc->mode1cycle=912;
  gblcdc->mode2cycle=164;
  gblcdc->vblank_cycle=70224*2;

  gbcpu->mode=DOUBLE_SPEED;
}

void go2simple_speed(void)
{
  if (gbcpu->mode==SIMPLE_SPEED) return;
  gblcdc->mode1cycle=456;
  gblcdc->mode2cycle=82;
  gblcdc->vblank_cycle=70224;

  gbcpu->mode=SIMPLE_SPEED;
}

inline void set_interrupt(UINT8 n) {
  static UINT8 old;

  old=INT_FLAG;
  INT_FLAG|=n;

  if (((old&n)^(INT_FLAG&n)) && (INT_FLAG&INT_ENABLE)) {
    if (gbcpu->state==HALT_STATE) {
      gbcpu->state=0;
      gbcpu->pc.w++;
    }
  }
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
    case SERIAL_INT:gbcpu->pc.w=0x0058;break;
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
 
  if (inc_line) CURLINE++;
  
  if (CURLINE==0x90) {
    gblcdc->mode=VBLANK_PER;
    if (CMP_LINE==CURLINE) LCDCSTAT|=0x04;
    if ((LCDCCONT&0x80 && LCDCSTAT&0x10) || 
	(LCDCSTAT&0x40 && LCDCSTAT&0x04 && LCDCCONT&0x80)) set_interrupt(LCDC_INT);
    if (LCDCCONT&0x80) set_interrupt(VBLANK_INT); 
  }

  /* normally it is 0x9a (I think) but with 0x9b addamsfamily work */
  if (CURLINE==0x9a) {   
    CURLINE=0;
    if (LCDCCONT&0x80) {
     if (conf.autofs) {
       skip_this_frame=skip_next_frame;
       skip_next_frame=barath_skip_next_frame(0);
       //skip_next_frame=frame_skip(0);
       if (!skip_this_frame) blit_screen();
     }
     else blit_screen();
    }
    //    printf("%d \n",get_nb_cycle());
    gblcdc->mode=OAM_PER;
  }

  LCDCSTAT&=0xf8;
  if (CMP_LINE==CURLINE) LCDCSTAT|=0x04;

  switch(gblcdc->mode) {
  case HBLANK_PER:       // HBLANK
    if (LCDCSTAT&0x08 && LCDCCONT&0x80) set_interrupt(LCDC_INT);
    if (LCDCCONT&0x80) {
      if (conf.autofs) {
	if (!skip_next_frame) draw_screen();
      }
      else draw_screen();
    }
    ret=tb_hblank_cycle[gbcpu->mode][nb_spr];
    gblcdc->mode=OAM_PER;
    if (dma_info.type==HDMA) do_hdma();
    /*else*/ inc_line=1;
    break;
  case VBLANK_PER:       // VBLANK
    LCDCSTAT|=0x01;
    ret=gblcdc->mode1cycle;
    inc_line=1;
    //if (dma_info.type==GDMA_STAND) do_gdma();
    break;
  case OAM_PER:       // OAM 
    LCDCSTAT|=0x02;
    inc_line=0;
    if (LCDCSTAT&0x20 && LCDCCONT&0x80) set_interrupt(LCDC_INT);
    if (LCDCSTAT&0x40 && LCDCSTAT&0x04 && LCDCCONT&0x80) set_interrupt(LCDC_INT);  
    ret=gblcdc->mode2cycle;
    gblcdc->mode=VRAM_PER;
    nb_spr=get_nb_spr();   
    break;
  case VRAM_PER:       // VRAM
    LCDCSTAT|=0x03;
    ret=tb_vram_cycle[gbcpu->mode][nb_spr];
    gblcdc->mode=HBLANK_PER;
    break;
  }
  return ret;
}

inline void timer_update(void)
{
  INT16 t=TIME_COUNTER+1;  //timer_inc;
  if (t>0xff) {
    TIME_COUNTER=TIME_MOD;
    set_interrupt(TIMEOWFL_INT); 
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
