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
#include "gngb_debuger/debuger.h"
#endif

#define DELAY_CYCLE 12

UINT16 tb_vram_cycle[2][11]={{169,182,195,208,221,234,246,259,272,285,298},
                             {338,364,390,416,442,468,492,518,544,570,596}};

UINT16 tb_hblank_cycle[2][11]={{207,194,181,168,155,142,130,117,104,91,78},
			       {414,388,362,336,310,284,260,234,208,182,156}};

static const UINT16 lcd_cycle_tab[2][4]={{204,456,80,172},              /* GB */
					 {204*2,456*2,80*2,172*2}};	/* CGB */

UINT32 nb_cycle=0;

UINT8 vram_init_pal=0;

UINT16 gblcdc_update1(void);
UINT16 gblcdc_update2(void);

UINT8 ISTAT=0;

UINT16 (*gblcdc_update)(void)=gblcdc_update1;

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
  add_int_msg("set int %d CURLINE %02x CMPLINE %02x INT_ENABLE %02x\n",
	    n,CURLINE,CMP_LINE,INT_ENABLE);
#endif

  /*if (((old&n)^(INT_FLAG&n)) && (INT_FLAG&INT_ENABLE)) {
    if (gbcpu->state==HALT_STATE) {
#ifdef DEBUG
      add_msg("stop halt 0\n");
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
    add_int_msg("make int %d CURLINE %02x CMP_LINE %02x INT_ENABLE %02x\n",n,CURLINE,CMP_LINE,INT_ENABLE,LCDCSTAT);
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
  //gblcdc->cycle=lcd_cycle_tab[gblcdc->timing][2];
  LCDCSTAT=(LCDCSTAT&0xf8)|0x02;
  gblcdc->nb_spr=get_nb_spr();
  gblcdc->inc_line=0;
  gblcdc->mode=VRAM_PER;
  vram_init_pal=1;
  //if (CURLINE==CMP_LINE && CURLINE!=0) LCDCSTAT|=0x04;
  if (CHECK_LYC_LY) LCDCSTAT|=0x04;
  if ((LCDCSTAT&0x20) ||
      (LCDCSTAT&0x40 && LCDCSTAT&0x04)) set_interrupt(LCDC_INT);  
  clear_screen();
  //frame_skip(1);
}

/*int check_lcdstat_int(void) {
  if (CHECK_LYC_LY) LCDCSTAT|=0x04;
  else  LCDCSTAT&=~0x04;
  if ((LCDCSTAT&0x40 && LCDCSTAT&0x04) ||
  (LCDCSTAT&0x08 && (LCDCSTAT&0x03)==0x00) ||
  (LCDCSTAT&0x10 && (LCDCSTAT&0x03)==0x01) ||
  (LCDCSTAT&0x20 && (LCDCSTAT&0x03)==0x02)) return 1;
  return 0;
  }
*/

UINT16 gblcdc_update1(void)  // LCDC is on
{
  UINT16 ret=0;
  UINT8 skip_this_frame;
  int x;

  if (!(LCDCCONT&0x80)) return 0;
    
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
      //skip_next_frame=barath_skip_next_frame(conf.show_fps);
      //skip_next_frame=frame_skip(0);
      if (!skip_this_frame) blit_screen();
      skip_next_frame=frame_skip(0);
      //skip_next_frame=barath_skip_next_frame(1);
      //skip_next_frame=barath_skip_next_frame(conf.show_fps);
    } else blit_screen();
    if (!conf.delay_int) gblcdc->mode=OAM_PER;
    else gblcdc->mode=BEGIN_OAM_PER;
  }
  
  if (CURLINE==0x99 && gblcdc->mode==LINE_99) { 
    CURLINE=0x00;
    gblcdc->mode=END_VBLANK_PER;
  } else if (CURLINE==0x99) {   
    // CURLINE=0;
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
    //gblcdc->mode=END_VBLANK_PER;
    gblcdc->mode=LINE_99;
  }
 
  LCDCSTAT&=0xf8;
  /* FIXME */
  if (CHECK_LYC_LY)  LCDCSTAT|=0x04;
  //if (CMP_LINE==CURLINE) LCDCSTAT|=0x04;
  /*if (CMP_LINE==CURLINE && CMP_LINE!=0x00)  LCDCSTAT|=0x04;
    if (CURLINE==0x99 && CMP_LINE==0x00) LCDCSTAT|=0x04;*/
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
  case LINE_99:
    if ((LCDCSTAT&0x40 && LCDCSTAT&0x04))
#ifdef DEBUG 
      {
	add_int_msg("set LCDC_INT END_VBLANK_PER CURLINE %02x\n",CURLINE);
	set_interrupt(LCDC_INT);
      }
#else
    set_interrupt(LCDC_INT);
#endif
    LCDCSTAT|=0x01;
    /* FIXME: Line 99 Timing */
    ret=30;
    gblcdc->inc_line=0;
    break;
  case END_VBLANK_PER:
    if ((LCDCSTAT&0x40 && LCDCSTAT&0x04))
#ifdef DEBUG 
      {
	add_int_msg("set LCDC_INT END_VBLANK_PER CURLINE %02x\n",CURLINE);
	set_interrupt(LCDC_INT);
      }
#else
    set_interrupt(LCDC_INT);
#endif
    LCDCSTAT|=0x01;
    /* FIXME */
    ret=gblcdc->mode1cycle-30;
    gblcdc->inc_line=0;
    break;
  case VBLANK_PER:       // VBLANK
    /* FIXME => water.gbc */
    if (/*(LCDCSTAT&0x10) || */
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
    /* FIXME Pinball Deluxe */
    if (LCDCSTAT&0x20) 
#ifdef DEBUG
     {
	add_int_msg("set LCDC_INT OAM_PER\n");
	set_interrupt(LCDC_INT);
      } 
#else
    set_interrupt(LCDC_INT);
#endif
    if (LCDCSTAT&0x40 && LCDCSTAT&0x04) 
#ifdef DEBUG 
      {
	add_int_msg("set LCDC_INT OAM_PER CURLINE %02x\n",CURLINE);
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

/* NEW GB LCDC UPDATE */

void set_lcd_int(char type) {
#ifdef DEBUG
  add_int_msg("Set lcd Int type%d\n",type);
#endif
  switch(type) {
  case LY_LYC_CO_SEL:break;
  case OAM_SEL:ISTAT&=0x3f;break;
  case VBLANK_SEL:ISTAT&=0x1f;break;
  case HBLANK_SEL:ISTAT&=0x0f;break;
  }
  set_interrupt(LCDC_INT);
}

static void set_lcd_stat(UINT8 v) {
  LCDCSTAT=(LCDCSTAT&0x78)|v;
  //if (CURLINE==CMP_LINE /*&& CURLINE!=0*/) LCDCSTAT|=0x04;
  if (CHECK_LYC_LY) LCDCSTAT|=0x04;
   
  if (LCDCCONT&0x80) {
    switch(v) {
    case 0:
      /*if (LCDCSTAT&0x08) set_interrupt(LCDC_INT);*/
      if (ISTAT&0x08) set_lcd_int(HBLANK_SEL);
      break;
    case 1:
      /*if ((LCDCSTAT&0x40 && LCDCSTAT&0x04)) set_interrupt(LCDC_INT);*/
      if ((ISTAT&0x40 && LCDCSTAT&0x04)) set_lcd_int(LY_LYC_CO_SEL);
      break;
    case 2:
      // if (CURLINE==CMP_LINE && ISTAT&0x40) set_lcd_int(LY_LYC_CO_SEL);
      /*if ((LCDCSTAT&0x40 && LCDCSTAT&0x04) ) set_interrupt(LCDC_INT);*/
      if (ISTAT&0x20) set_lcd_int(OAM_SEL);
      if ((ISTAT&0x40 && LCDCSTAT&0x04)) set_lcd_int(LY_LYC_CO_SEL);
      break;
    case 3:break;
    default:break;
    }
  }
}

/*void gblcdc_set_on(void) {
  CURLINE=0x00;
  gblcdc->cycle=lcd_cycle_tab[gblcdc->timing][2];
  gblcdc->nb_spr=get_nb_spr();
  vram_init_pal=1;
  set_lcd_stat(2);
  ISTAT=LCDCSTAT;
  clear_screen();
  }*/

UINT16 gblcdc_update2(void) {
  static char delay_vblank;
  
  UINT8 skip_this_frame=0;
  
  if (!(LCDCCONT&0x80)) {	/* LCD IS OFF */
    return 0;
    switch(LCDCSTAT&0x03) {
    case 0:						/* HBLANK */
      set_lcd_stat(2);
      return lcd_cycle_tab[gblcdc->timing][2];
      break;
    case 1:						/* VBLANK */
      set_lcd_stat(2);
      return lcd_cycle_tab[gblcdc->timing][2];
      break;
    case 2:						/* OAM */
      set_lcd_stat(3);
      return lcd_cycle_tab[gblcdc->timing][3];
      break;
    case 3:						/* VRAM */
      set_lcd_stat(0);
      return lcd_cycle_tab[gblcdc->timing][0];
      break;
    }    
  } else {			/* LCD IS ON */
    switch(LCDCSTAT&0x03) {
    
    case 0:
      CURLINE++;
      if (CURLINE==0x90) {
	if ((gbcpu->state&HALT_STATE)) {
	  set_interrupt(VBLANK_INT);
	  if (ISTAT&0x10) set_lcd_int(VBLANK_SEL);
	  set_lcd_stat(1);
	  return lcd_cycle_tab[gblcdc->timing][1];
	} else {
	  delay_vblank=1;
	  set_lcd_stat(1);
	  return DELAY_CYCLE;
	}
      }
      gblcdc->nb_spr=get_nb_spr();
      set_lcd_stat(2);
      return lcd_cycle_tab[gblcdc->timing][2];
      break;

    case 1:
      if (CURLINE==0x90 && delay_vblank) {
	/* FIX: Addams Family */
	delay_vblank=0;
	set_interrupt(VBLANK_INT);
	if (ISTAT&0x10) set_lcd_int(VBLANK_SEL);
	return lcd_cycle_tab[gblcdc->timing][1]-DELAY_CYCLE;
      }

      if (CURLINE==0x00) {
	set_lcd_stat(2);
	return lcd_cycle_tab[gblcdc->timing][2];
      }
      
      CURLINE++;
      if (CURLINE==0x99) {
	//CURLINE=0x00;
	if (conf.autoframeskip) {
	  skip_this_frame=skip_next_frame;
	  skip_next_frame=barath_skip_next_frame(conf.show_fps);
	  if (!skip_this_frame) blit_screen();
	} else blit_screen();
	set_lcd_stat(1);
	/* FIX : Sagaia */
	/*if ((CMP_LINE==0x00) && LCDCSTAT&0x40) {
	  LCDCSTAT|=0x04;
	  set_interrupt(LCDC_INT);
	  } */
	return /*lcd_cycle_tab[gblcdc->timing][1]*/60;
      }

      if (CURLINE==0x9a) {
	CURLINE=0x00;
	set_lcd_stat(1);
	/*if (CMP_LINE==0x00 && ISTAT&0x40) {
	  LCDCSTAT|=0x04;
	  set_lcd_int(LY_LYC_CO_SEL);
	  }*/
	return lcd_cycle_tab[gblcdc->timing][1]-60;
      }
      
      set_lcd_stat(1);
      return lcd_cycle_tab[gblcdc->timing][1];
      break;

    case 2:
      /* Emulate Vic Viper Laser */
      if (vram_init_pal) {
	int x;
	for(x=0;x<160;x++)
	  gblcdc->vram_pal_line[x]=pal_bck;
	pal_bck[0]=BGPAL&3;
	pal_bck[1]=(BGPAL>>2)&3;
	pal_bck[2]=(BGPAL>>4)&3;
	pal_bck[3]=(BGPAL>>6)&3;
	vram_init_pal=0;
      }
      
      set_lcd_stat(3);
      return lcd_cycle_tab[gblcdc->timing][3];
      break;
      
    case 3:
      if (!skip_this_frame) draw_screen();
      if (dma_info.type==HDMA) do_hdma();
      set_lcd_stat(0);
      return lcd_cycle_tab[gblcdc->timing][0];      
      break;
    }
  }
  return 0;
}

/* FIXME */
void gblcdc_addcycle(INT32 c) {
  UINT8 v=0;

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


// GB TIMER

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
    add_int_msg("stop halt 1\n");
#endif
  }
}


