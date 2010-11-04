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
#include "emu.h"
#include "rom.h"

#ifdef DEBUG
#include "gngb_debuger/log.h"
#include "gngb_debuger/debuger.h"
#endif

#define DELAY_CYCLE 24

UINT16 tb_vram_cycle[2][11]={{169,182,195,208,221,234,246,259,272,285,298},
                             {338,364,390,416,442,468,492,518,544,570,596}};

UINT16 tb_hblank_cycle[2][11]={{207,194,181,168,155,142,130,117,104,91,78},
			       {414,388,362,336,310,284,260,234,208,182,156}};

UINT32 nb_cycle=0;

UINT8 vram_init_pal=0;

UINT32 get_nb_cycle(void)
{
  UINT32 t=nb_cycle;
  SDL_LockAudio();
  nb_cycle=0;
  SDL_UnlockAudio();
  return t;
}

inline void set_interrupt(UINT8 n) {
  static UINT8 old;

  old=INT_FLAG;
  INT_FLAG|=n;
   
#ifdef DEBUG
  if (active_log)
    put_log("set int %d CURLINE %02x CMPLINE %02x INT_ENABLE %02x\n",
	    n,CURLINE,CMP_LINE,INT_ENABLE);
#endif

  /*if (((old&n)^(INT_FLAG&n)) && (INT_FLAG&INT_ENABLE)) {
    if (gbcpu->state==HALT_STATE) {
#ifdef DEBUG
      if (active_log)
	put_log("stop halt 0\n");
#endif
      gbcpu->state=0;
      gbcpu->pc.w++;
    } 
    }*/
}

inline void unset_interrupt(UINT8 n) {
  INT_FLAG&=(~n);
}

inline UINT8 make_interrupt(UINT8 n) {
 
  if ((INT_ENABLE&n) && (gbcpu->int_flag)) {
    if (gbcpu->state==HALT_STATE) {
      gbcpu->state=0;
      gbcpu->pc.w++;
    } 

#ifdef DEBUG
    if (active_log)
      put_log("make int %d CURLINE %02x CMP_LINE %02x INT_ENABLE %02x\n",n,CURLINE,CMP_LINE,INT_ENABLE,LCDCSTAT);
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

UINT8 request_interrupt(UINT8 n) {
  if ((INT_ENABLE&n) && (gbcpu->int_flag)) 
    return 1;
  return 0;
}


void gblcdc_init(void) {
  gblcdc=(GBLCDC *)malloc(sizeof(GBLCDC));
  gblcdc_reset();
}

void gblcdc_reset(void) {
  int i;
  char t[20];
  gblcdc->vblank_cycle=70224;
  gblcdc->mode1cycle=456;
  gblcdc->mode2cycle=80;
  gblcdc->mode3cycle=170;
  gblcdc->cycle=gblcdc->mode2cycle;
  /* FIXME: SGB timing change */
  if (conf.gb_type==SUPER_GAMEBOY) 
    gblcdc->timing=0;
  else gblcdc->timing=0;
  /* TRICK for emulate vic viper laser */
  gblcdc->vram_factor=1.0;

  if (conf.const_cycle) 
    for(i=0;i<11;i++) {
      tb_hblank_cycle[0][i]=204;
      tb_hblank_cycle[1][i]=204*2;
      tb_vram_cycle[0][i]=172;
      tb_vram_cycle[1][i]=172*2;
    }
  
  for(i=0;i<160;i++)
    gblcdc->vram_pal_line[i]=pal_bck;

  gblcdc_set_on();  
}

void gblcdc_set_on(void) {
  CURLINE=0x00;
  gblcdc->cycle=gblcdc->mode2cycle;
  LCDCSTAT=(LCDCSTAT&0xf8)|0x02;
  gblcdc->nb_spr=get_nb_spr();
  gblcdc->inc_line=0;
  gblcdc->mode=VRAM_PER;
  vram_init_pal=1;
  if (CURLINE==CMP_LINE)
    LCDCSTAT|=0x04;
  if (LCDCSTAT&0x20) set_interrupt(LCDC_INT);
  if (LCDCSTAT&0x40 && LCDCSTAT&0x04) set_interrupt(LCDC_INT);  
  clear_screen();
}

void gbtimer_init(void) {
  gbtimer=(GBTIMER *)malloc(sizeof(GBTIMER));
  gbtimer_reset();
}

void gbtimer_reset(void) {
  gbtimer->clk_inc=0;
  gbtimer->cycle=0; 
}

void go2double_speed(void)
{
  if (gbcpu->mode==DOUBLE_SPEED) return;
  gblcdc->mode1cycle=912;
  gblcdc->mode2cycle=160;
  gblcdc->vblank_cycle=70224*2;
  gblcdc->timing=1;
  gbcpu->mode=DOUBLE_SPEED;
}

void go2simple_speed(void)
{
  if (gbcpu->mode==SIMPLE_SPEED) return;
  gblcdc->mode1cycle=456;
  gblcdc->mode2cycle=80;
  gblcdc->vblank_cycle=70224;
  gblcdc->timing=0;
  gbcpu->mode=SIMPLE_SPEED;
}

inline UINT16 gblcdc_update(void)  // LCDC is on
{
  UINT16 ret=0;
  UINT8 skip_this_frame;
  int x;
    
  if (gblcdc->inc_line) CURLINE++;
  
  if (CURLINE==0x90 && (gblcdc->mode==OAM_PER || gblcdc->mode==BEGIN_OAM_PER)) {
    if (!conf.delay_int) {
      gblcdc->mode=VBLANK_PER;
      set_interrupt(VBLANK_INT);
    } else gblcdc->mode=LINE_90_BEGIN;
  }

  if (CURLINE==0x00 && gblcdc->mode==END_VBLANK_PER) {
    if (conf.autoframeskip) {
      skip_this_frame=skip_next_frame;
      skip_next_frame=barath_skip_next_frame(conf.show_fps);
      if (!skip_this_frame) blit_screen();
    } else blit_screen();
    if (!conf.delay_int)
      gblcdc->mode=OAM_PER;
    else gblcdc->mode=BEGIN_OAM_PER;
  }
  
  if (CURLINE==0x99) {   
    CURLINE=0;
    /*if (conf.autoframeskip) {
      skip_this_frame=skip_next_frame;
      skip_next_frame=barath_skip_next_frame(conf.show_fps);
      if (!skip_this_frame) blit_screen();
      }
      else blit_screen();
      if (!conf.delay_int)
      gblcdc->mode=OAM_PER;
      else gblcdc->mode=BEGIN_OAM_PER;
#ifdef DEBUG
      if (active_log)
	put_log("nb_cycle: %d\n",get_nb_cycle());
	#endif*/
    gblcdc->mode=END_VBLANK_PER;
  }
 
  LCDCSTAT&=0xf8;
  /* FIXME */
  if (CMP_LINE==CURLINE) LCDCSTAT|=0x04;
  //if (CMP_LINE==CURLINE && CMP_LINE!=0x00)  LCDCSTAT|=0x04;
  /*if ((CMP_LINE!=0x00 && CMP_LINE==CURLINE) ||
    (CMP_LINE==0x00 && gblcdc->mode==END_VBLANK_PER)) LCDCSTAT|=0x04;*/
  
  switch(gblcdc->mode) {
  case HBLANK_PER:       // HBLANK
    if (LCDCSTAT&0x08) set_interrupt(LCDC_INT);
    if (conf.autoframeskip) {
      if (!skip_next_frame) draw_screen();
    } else draw_screen();
    ret=tb_hblank_cycle[gblcdc->timing][gblcdc->nb_spr];
    if (!conf.delay_int)
      gblcdc->mode=OAM_PER;
    else gblcdc->mode=BEGIN_OAM_PER;
    if (dma_info.type==HDMA) do_hdma();
    gblcdc->inc_line=1;
    break;
  case LINE_90_BEGIN:
    gblcdc->inc_line=0;
    ret=DELAY_CYCLE;
    LCDCSTAT|=0x01;
    if ((LCDCSTAT&0x10) || 
	(LCDCSTAT&0x40 && LCDCSTAT&0x04)) set_interrupt(LCDC_INT);
    gblcdc->mode=LINE_90_END;
    break;
  case LINE_90_END:
    gblcdc->inc_line=1;
    ret=gblcdc->mode1cycle-DELAY_CYCLE;
    LCDCSTAT|=0x01;
    gblcdc->mode=VBLANK_PER;
    /*if ((LCDCSTAT&0x10) || 
      (LCDCSTAT&0x40 && LCDCSTAT&0x04)) set_interrupt(LCDC_INT);*/
    set_interrupt(VBLANK_INT);
    break;
  case END_VBLANK_PER:
    if ((LCDCSTAT&0x10) || 
	(LCDCSTAT&0x40 && LCDCSTAT&0x04)) 
#ifdef DEBUG 
      {
	if (active_msg)
	  add_msg("set LCDC_INT END_VBLANK_PER CURLINE %02x",CURLINE);
	set_interrupt(LCDC_INT);
      }
#else
    set_interrupt(LCDC_INT);
#endif
    LCDCSTAT|=0x01;
    ret=gblcdc->mode1cycle;
    gblcdc->inc_line=0;
    break;
  case VBLANK_PER:       // VBLANK
    /* FIXME => water.gbc */
    if ((LCDCSTAT&0x10) || 
	(LCDCSTAT&0x40 && LCDCSTAT&0x04)) set_interrupt(LCDC_INT);
    LCDCSTAT|=0x01;
    ret=gblcdc->mode1cycle;
    gblcdc->inc_line=1;
    break;
  case BEGIN_OAM_PER:
    LCDCSTAT|=0x02;
    gblcdc->inc_line=0;
    gblcdc->mode=OAM_PER;
    ret=DELAY_CYCLE;
    break;
  case OAM_PER:       // OAM 
    LCDCSTAT|=0x02;
    gblcdc->inc_line=0;
    if (LCDCSTAT&0x20) set_interrupt(LCDC_INT);
    if (LCDCSTAT&0x40 && LCDCSTAT&0x04) 
#ifdef DEBUG 
      {
	if (active_msg)
	  add_msg("set LCDC_INT OAM_PER CURLINE %02x",CURLINE);
	set_interrupt(LCDC_INT);
      }
#else
    set_interrupt(LCDC_INT);
#endif
    ret=gblcdc->mode2cycle;
    if (conf.delay_int) ret-=DELAY_CYCLE;
    gblcdc->mode=VRAM_PER;
    gblcdc->nb_spr=get_nb_spr();
    break;
  case VRAM_PER:       // VRAM
    LCDCSTAT|=0x03;
    ret=tb_vram_cycle[gblcdc->timing][gblcdc->nb_spr];
    gblcdc->mode=HBLANK_PER;
    /* FIXME */
    gblcdc->mode3cycle=ret;
    gblcdc->vram_factor=160.0/(double)(gblcdc->mode3cycle);
    if (vram_init_pal) {
      for(x=0;x<160;x++)
	gblcdc->vram_pal_line[x]=pal_bck;
      pal_bck[0]=BGPAL&3;
      pal_bck[1]=(BGPAL>>2)&3;
      pal_bck[2]=(BGPAL>>4)&3;
      pal_bck[3]=(BGPAL>>6)&3;
      vram_init_pal=0;
    }
    break;
  }
  return ret;
}

/* FIXME */
void gblcdc_addcycle(INT32 c) {
  UINT8 v;

  if (!(LCDCCONT&0x80)) return;
  while(c>gblcdc->cycle) {
    if (INT_FLAG&VBLANK_INT)  v=make_interrupt(VBLANK_INT);
    if (INT_FLAG&LCDC_INT && !v) v=make_interrupt(LCDC_INT);
    if (INT_FLAG&TIMEOWFL_INT && !v) v=make_interrupt(TIMEOWFL_INT); 
    c-=gblcdc->cycle;
    gblcdc->cycle=gblcdc_update();
  }
  gblcdc->cycle-=c;
  dma_info.type=NO_DMA;
  HDMA_CTRL5=0xff;
}

inline void gbtimer_update(void)
{
  /*INT16 t=TIME_COUNTER+1;  //timer_inc;
  if (t>0xff) {
    TIME_COUNTER=TIME_MOD;
    set_interrupt(TIMEOWFL_INT); 
    } else TIME_COUNTER=((UINT16)(t))&0xff;*/
  TIME_COUNTER++;
  /* FIXME ==0xff or ==0x00 */
  if (TIME_COUNTER==0xff) {
    TIME_COUNTER=TIME_MOD;
    set_interrupt(TIMEOWFL_INT); 
  }
}

inline void halt_update(void) // gbcpu->state=HALT_STATE
{ 
  if (INT_FLAG&INT_ENABLE) {
    gbcpu->state=0;
    gbcpu->pc.w++;
#ifdef DEBUG
    if (active_log)
      put_log("stop halt 1\n");
#endif
  }
}







