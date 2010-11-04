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
#include <SDL.h>
#include "interrupt.h"
#include "frame_skip.h"
#include "memory.h"
#include "vram.h"
#include "cpu.h"
#include "emu.h"
#include "rom.h"

#ifdef DEBUG
#include "gngb_debuger/debuger.h"
#endif

#define DELAY_CYCLE 72

static const Uint16 lcd_cycle_tab[2][5]={{204,456,80,172,80},              /* GB */
					 {204*2,456*2,80*2,172*2,80*2}}; /* CGB */

Uint32 nb_cycle=0;
Uint8 vram_init_pal=0;

Uint32 get_nb_cycle(void)
{
  Uint32 t=nb_cycle;
  SDL_LockAudio();
  nb_cycle=0;
  SDL_UnlockAudio();
  return t;
}


#ifdef DEBUG

__inline__ void set_interrupt(Uint8 n) {
  INT_FLAG|=n;
  add_int_msg("set int %d CURLINE %02x CMPLINE %02x INT_ENABLE %02x\n",
	    n,CURLINE,CMP_LINE,INT_ENABLE);
}

__inline__ void unset_interrupt(Uint8 n) {
  INT_FLAG&=(~n);
  add_int_msg("unset int %d CURLINE %02x CMPLINE %02x INT_ENABLE %02x\n",
	      n,CURLINE,CMP_LINE,INT_ENABLE);
}

#endif

__inline__ Uint8 make_interrupt(Uint8 n) {

  if ((INT_ENABLE&n) && (gbcpu->int_flag)) {
    if (gbcpu->state==HALT_STATE) {
      gbcpu->state=0;
      gbcpu->pc.w++;
    } 
    
#ifdef DEBUG
    add_int_msg("make int %d CURLINE %02x CMP_LINE %02x INT_ENABLE %02x\n",n,CURLINE,CMP_LINE,INT_ENABLE);
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

// GB LCD

void gblcdc_init(void) {
  gblcdc=(GBLCDC *)malloc(sizeof(GBLCDC));
  gblcdc_reset();
}

void gblcdc_reset(void) {
  int i;
  
  gblcdc->vblank_cycle=70224;
  gblcdc->mode1cycle=456;
  gblcdc->mode2cycle=80;
  gblcdc->mode3cycle=170;
  gblcdc->cycle=gblcdc->mode2cycle;
  /* FIXME: SGB timing change */
  /*if (conf.gb_type==SUPER_GAMEBOY) 
    gblcdc->timing=0;
    else*/ gblcdc->timing=0;
  /* TRICK for emulate vic viper laser */
  gblcdc->vram_factor=1.0;

  for(i=0;i<160;i++)
    gblcdc->vram_pal_line[i]=pal_bck;

  gblcdc_set_on();  
}

void gblcdc_set_on(void) {
  gblcdc->win_curline=0;
  CURLINE=0x00;
  LCDCCONT|=0x80;
  gblcdc->inc_line=0;
  gblcdc->mode=END_VBLANK_PER;
  vram_init_pal=1;
  clear_screen();
  reset_frame_skip();
  gblcdc->cycle=gblcdc_update(); 
}

Uint16 gblcdc_update(void)  // LCDC is on
{
  Uint16 ret=0;
  Uint8 skip_this_frame;
  int x;

  if (!(LCDCCONT&0x80)) return 0;
  
  if (gblcdc->inc_line) {
    CURLINE++;
    LCDCSTAT&=0xf8;
    if ((CURLINE && CURLINE==CMP_LINE) || 
	((CURLINE==0x99 && CMP_LINE==0x00)))  LCDCSTAT|=0x04;
  }
  
  if (CURLINE==0x90 && (gblcdc->mode==OAM_PER || gblcdc->mode==BEGIN_OAM_PER)) {
    if (!conf.delay_int) {
      gblcdc->mode=VBLANK_PER;
#ifdef DEBUG
      if (vblank_int_enable) set_interrupt(VBLANK_INT);
#else
      set_interrupt(VBLANK_INT);
#endif
    } else gblcdc->mode=LINE_90_BEGIN;
  }

  if (CURLINE==0x00 && gblcdc->mode==END_VBLANK_PER) {
    LCDCSTAT&=0xf8;
    
    skip_this_frame=skip_next_frame;
    if (!skip_this_frame) blit_screen();
    skip_next_frame=frame_skip(0);
    if (1/*!conf.delay_int*/) gblcdc->mode=OAM_PER;
    else gblcdc->mode=BEGIN_OAM_PER;
    gblcdc->win_curline=0;
  }
  
  if (CURLINE==0x99 && gblcdc->mode==LINE_99) { 
    CURLINE=0x00;
    gblcdc->mode=END_VBLANK_PER;
  } else if (CURLINE==0x99) {   
    
    gblcdc->mode=LINE_99;
  }
  
  switch(gblcdc->mode) {
  case HBLANK_PER:       // HBLANK
#ifdef DEBUG
    if (lcd_hblank_int_enable && LCDCSTAT&0x08) set_interrupt(LCDC_INT);
#else
    if (LCDCSTAT&0x08) set_interrupt(LCDC_INT);
#endif
    if (!skip_next_frame) draw_screen();
    ret=lcd_cycle_tab[gblcdc->timing][0];
    if (1/*!conf.delay_int*/) gblcdc->mode=OAM_PER;
    else gblcdc->mode=BEGIN_OAM_PER;
    if (dma_info.type==HDMA) do_hdma();
    gblcdc->inc_line=1;
    LCDCSTAT&=0xfc;
    break;
  case LINE_90_BEGIN:
    gblcdc->inc_line=0;
    LCDCSTAT|=0x01;
    if (LCDCSTAT&0x40 && LCDCSTAT&0x04) set_interrupt(LCDC_INT);
    ret=DELAY_CYCLE;
    gblcdc->mode=LINE_90_END;
    break;
  case LINE_90_END:
    gblcdc->inc_line=1;
#ifdef DEBUG
    if (lcd_vblank_int_enable && LCDCSTAT&0x10) set_interrupt(LCDC_INT);
#else
    if (LCDCSTAT&0x10) set_interrupt(LCDC_INT);
#endif
    ret=lcd_cycle_tab[gblcdc->timing][1]-DELAY_CYCLE-8;
    LCDCSTAT|=0x01;
    gblcdc->mode=VBLANK_PER;
#ifdef DEBUG
    if (vblank_int_enable) set_interrupt(VBLANK_INT);
#else
    set_interrupt(VBLANK_INT);
#endif
    break;
  case LINE_99:
    LCDCSTAT|=0x01;
    /* FIXME: Line 99 Timing */
    ret=lcd_cycle_tab[gblcdc->timing][4];
    gblcdc->inc_line=0;
    break;
  case END_VBLANK_PER:
    LCDCSTAT|=0x01;
#ifdef DEBUG
    if (lcd_lyc_int_enable && LCDCSTAT&0x40 && LCDCSTAT&0x04) set_interrupt(LCDC_INT);
#else
    if (LCDCSTAT&0x40 && LCDCSTAT&0x04) set_interrupt(LCDC_INT);
#endif
    ret=lcd_cycle_tab[gblcdc->timing][1]-lcd_cycle_tab[gblcdc->timing][4];
    gblcdc->inc_line=0;
    break;
  case VBLANK_PER:       // VBLANK
    /* FIXME => water.gbc */
#ifdef DEBUG
    if (lcd_lyc_int_enable && LCDCSTAT&0x40 && LCDCSTAT&0x04) set_interrupt(LCDC_INT);
#else
    if (LCDCSTAT&0x40 && LCDCSTAT&0x04) set_interrupt(LCDC_INT);
#endif
    LCDCSTAT|=0x01;
    ret=lcd_cycle_tab[gblcdc->timing][1];
    gblcdc->inc_line=1;
    break;
  case BEGIN_OAM_PER:
    LCDCSTAT|=0x02;
    gblcdc->inc_line=0;
    gblcdc->mode=OAM_PER;
    ret=DELAY_CYCLE;
    ret=24;
    if (LCDCSTAT&0x40 && LCDCSTAT&0x04)  set_interrupt(LCDC_INT);
    break;
   case OAM_PER:       // OAM 
    LCDCSTAT|=0x02;
    gblcdc->inc_line=0;
    /* FIXME: Pinball Deluxe */
#ifdef DEBUG
    if (lcd_oam_int_enable && LCDCSTAT&0x20) set_interrupt(LCDC_INT);
#else
    if (LCDCSTAT&0x20) set_interrupt(LCDC_INT);
#endif

#ifdef DEBUG
    if (lcd_lyc_int_enable && LCDCSTAT&0x40 && LCDCSTAT&0x04) set_interrupt(LCDC_INT);
#else
    if (LCDCSTAT&0x40 && LCDCSTAT&0x04)  set_interrupt(LCDC_INT);
#endif

    ret=lcd_cycle_tab[gblcdc->timing][2];
    /*if (conf.delay_int) ret-=DELAY_CYCLE;*/
    gblcdc->mode=VRAM_PER;
    gblcdc->nb_spr=get_nb_spr();
    break;
  case VRAM_PER:       // VRAM
    LCDCSTAT|=0x03;
    ret=lcd_cycle_tab[gblcdc->timing][3];
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
void gblcdc_addcycle(Sint32 c) {
  Uint8 v=0;

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


/* GB TIMER */

void gbtimer_init(void) {
  gbtimer=(GBTIMER *)malloc(sizeof(GBTIMER));
  gbtimer_reset();
}

void gbtimer_reset(void) {
  gbtimer->clk_inc=0;
  gbtimer->cycle=0; 
}

void gbtimer_update(void)
{
  TIME_COUNTER++;
  if (TIME_COUNTER==0x00) {
    TIME_COUNTER=TIME_MOD;
#ifdef DEBUG
    if (timer_int_enable) set_interrupt(TIMEOWFL_INT); 
#else
    set_interrupt(TIMEOWFL_INT); 
#endif
  }
}

__inline__ void halt_update(void) // gbcpu->state=HALT_STATE
{ 
  if (INT_FLAG&INT_ENABLE) {
    gbcpu->state=0;
    gbcpu->pc.w++;
#ifdef DEBUG
    add_int_msg("stop halt 1\n");
#endif
  }
}
