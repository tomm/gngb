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
                

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include "memory.h"
#include "emu.h"
#include "cpu.h"
#include "rom.h"
#include "vram.h"
#include "interrupt.h"
#include "frame_skip.h"
#include "sound.h"
#include "serial.h"
#include "message.h"
#include "sgb.h"

#ifdef DEBUG
#include "gngb_debuger/debuger.h"
#endif

static Uint8 gb_pad;

Uint8 rom_mask;
Uint16 nb_rom_page;
Uint16 nb_ram_page;
Uint16 nb_vram_page;
Uint16 nb_wram_page;
Uint16 active_rom_page=0;
Uint16 active_ram_page=0;
Uint16 active_vram_page=0;
Uint16 active_wram_page=0;
Uint8 **rom_page=NULL;
Uint8 **ram_page=NULL;
Uint8 **vram_page=NULL;
Uint8 **wram_page=NULL;
Uint8 oam_space[0xa0];  
Uint8 himem[0x160];     

Uint8 ram_enable=0;

Uint8 mbc1_mem_mode=MBC1_16_8_MEM_MODE;
Uint8 mbc1_line=0;
Uint8 mbc5_lower=0;
Uint8 mbc5_upper=0;
Uint8 mbc7_lower=0;
Uint8 mbc7_upper=0;

Uint16 sensor_x=0x7ff,sensor_y=0x7ff;

Uint8 ram_mask;

MEM_READ_ENTRY mem_read_tab[0x10];
MEM_WRITE_ENTRY mem_write_tab[0x10];

void (*select_rom_page)(Uint16 adr,Uint8 v);
void (*select_ram_page)(Uint16 adr,Uint8 v);

Uint8 IOMem[256]=
{0xCF, 0x00, 0x7E, 0xFF, 0xAD, 0x00, 0x00, 0xF8, 0xFF, 0xFF,
 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x80, 0xBF, 0xF0, 0xFF,
 0xBF, 0xFF, 0x3F, 0x00, 0xFF, 0xBF, 0x7F, 0xFF, 0x9F, 0xFF,
 0xBF, 0xFF, 0xFF, 0x00, 0x00, 0xBF, 0x77, 0xF3, 0xF1, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0xFE,
 0x0E, 0x7F, 0x00, 0xFF, 0x58, 0xDF, 0x00, 0xEC, 0x00, 0xBF,
 0x0c, 0xED, 0x03, 0xF7, 0x91, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0xFC, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x7E, 0xFF, 0xFE,
 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0xC1, 0x20, 0x00, 0x00,
 0x00, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

DMA_INFO dma_info;

/* Ram Info: 
   Because some roms use Ram Space to communicate Ram can't always be in 
   direct access */

Uint8 ram_can_direct_access=1;
Uint8 (*mem_read_a000_bfff)(Uint16 adr)=NULL;
void (*mem_write_a000_bfff)(Uint16 adr,Uint8 v)=NULL;

/* Read Function */

Uint8 mbc7_read_a000_bfff(Uint16 adr);
Uint8 timer_read_a000_bfff(Uint16 adr);

Uint8 mem_read_a000_bfff_default(Uint16 adr);
Uint8 mem_read_f000_ffff_default(Uint16 adr);

/* Write Function */

void mem_write_0000_1fff_default(Uint16 adr,Uint8 v);
void mem_write_2000_3fff_default(Uint16 adr,Uint8 v);
void mem_write_4000_5fff_default(Uint16 adr,Uint8 v);
void mem_write_6000_7fff_default(Uint16 adr,Uint8 v);
void mem_write_a000_bfff_default(Uint16 adr,Uint8 v);
void mem_write_f000_ffff_default(Uint16 adr,Uint8 v);

void mbc1_write_4000_5fff(Uint16 adr,Uint8 v);
void mbc1_write_6000_7fff(Uint16 adr,Uint8 v);

void rumble_write_4000_5fff(Uint16 adr,Uint8 v);

void timer_write_4000_5fff(Uint16 adr,Uint8 v);
void timer_write_6000_7fff(Uint16 adr,Uint8 v);
void timer_write_a000_bfff(Uint16 adr,Uint8 v);

void mbc7_write_a000_bfff(Uint16 adr,Uint8 v);

/* Fast Mem Function */

__inline__ void set_mem_read_direct_access(Uint8 mem_area,Uint8 *buf) {
  mem_read_tab[mem_area].type=MEM_DIRECT_ACCESS;
  mem_read_tab[mem_area].b=buf;
}

__inline__ void set_mem_write_direct_access(Uint8 mem_area,Uint8 *buf) {
  mem_write_tab[mem_area].type=MEM_DIRECT_ACCESS;
  mem_write_tab[mem_area].b=buf;
}

__inline__ void set_mem_read_fun_access(Uint8 mem_area,Uint8 (*fun)(Uint16 adr)) {
  mem_read_tab[mem_area].type=MEM_FUN_ACCESS;
  mem_read_tab[mem_area].f=fun;
}

__inline__ void set_mem_write_fun_access(Uint8 mem_area,void (*fun)(Uint16 adr,Uint8 v)) {
  mem_write_tab[mem_area].type=MEM_FUN_ACCESS;
  mem_write_tab[mem_area].f=fun;
}

__inline__ void set_active_rom_page(Uint16 page) {
  Uint8 i;
  active_rom_page=page&rom_mask;
  for(i=0;i<4;i++) {
    set_mem_read_direct_access(i,&rom_page[0][i*0x1000]);
    set_mem_read_direct_access(i+4,&rom_page[active_rom_page][i*0x1000]);
  }
}

__inline__ void set_active_vram_page(Uint16 page) {
  Uint8 i;
  active_vram_page=page&0x01;
  for(i=0;i<2;i++) {
    set_mem_read_direct_access(i+8,&vram_page[active_vram_page][i*0x1000]);
    set_mem_write_direct_access(i+8,&vram_page[active_vram_page][i*0x1000]);
  }
}

__inline__ void set_active_ram_page(Uint16 page) {
  Uint8 i;
  active_ram_page=page&ram_mask;
  if (ram_can_direct_access) {
    for(i=0;i<2;i++) {
      set_mem_read_direct_access(i+0x0A,&ram_page[active_ram_page][i*0x1000]);
      set_mem_write_direct_access(i+0x0A,&ram_page[active_ram_page][i*0x1000]);
    }
  }
}

__inline__ void set_active_wram_page(Uint16 page) {
  int i;
  active_wram_page=page&0x07;
  if (!active_wram_page) active_wram_page=1;
  
  set_mem_read_direct_access(0x0C,&wram_page[0][0]);
  set_mem_read_direct_access(0x0D,&wram_page[active_wram_page][0]);
  set_mem_read_direct_access(0x0E,&wram_page[0][0]);
  
  set_mem_write_direct_access(0x0C,&wram_page[0][0]);
  set_mem_write_direct_access(0x0D,&wram_page[active_wram_page][0]);
  set_mem_write_direct_access(0x0E,&wram_page[0][0]);
}

__inline__ void disable_ram(void) {
  Uint8 i;
  for(i=0;i<2;i++) {
    set_mem_read_fun_access(i+0x0A,mem_read_a000_bfff);
    set_mem_write_fun_access(i+0x0A,mem_write_a000_bfff);
  }
  ram_enable=0;
}

__inline__ void enable_ram(void) {
  set_active_ram_page(active_ram_page);
  ram_enable=1;
}

void select_default(Uint16 adr,Uint8 v)
{
  // do nothing
}

void mbc1_select_page(Uint16 adr,Uint8 v)
{
  Uint8 bank=v&rom_mask;
  if (bank<1) bank=1;
  set_active_rom_page(bank);
}

void mbc2_select_page(Uint16 adr,Uint8 v)
{
  Uint8 bank;
  if (adr==0x2100) {
    bank=v&rom_mask;
    if (bank<1) bank=1;
    set_active_rom_page(bank);
  }
}

void mbc3_select_page(Uint16 adr,Uint8 v)
{
  Uint8 bank=v&rom_mask;
  if (bank<1) bank=1;
  set_active_rom_page(bank);
}

void mbc5_select_page(Uint16 adr,Uint8 v)
{
  Uint16 bank;
  if (adr>=0x2000 && adr<0x3000)
    mbc5_lower=v;
  if (adr>=0x3000 && adr<0x4000 && nb_rom_page>=256) {
    mbc5_upper=v&0x01;
  }
  bank=mbc5_lower+((mbc5_upper)?256:0);
  set_active_rom_page(bank);
}



     

void gbmemory_init(void)
{
  //gbmemory_reset();

  memset(key,0,256);
  memset(oam_space,0,0xa0);
  memcpy(&himem[0x60],IOMem,0xff);

  DIVID=0x00;
  TIME_MOD=0x00;
  TIME_CONTROL=0x00;
  TIME_COUNTER=0x00;
  
  NR10=0x80;
  NR11=0xbf;
  NR12=0xf3;
  NR14=0xbf;
  NR21=0x3f;
  NR21=0x00;
  NR24=0xbf;
  NR30=0x7f;
  NR31=0xff;
  NR32=0x9f;
  NR33=0xbf;
  NR41=0xff;
  NR42=0x00;
  NR43=0x00;
  NR44=0xbf;
  NR50=0x77;
  NR51=0xf3;
  /* FIXME: SGB value != */
  NR52=0xf1;

  CMP_LINE=0x00;
  INT_ENABLE=0x00;

  dma_info.type=NO_DMA;
   
  if (rom_type&MBC1) select_rom_page=mbc1_select_page;
  else if (rom_type&MBC2) select_rom_page=mbc2_select_page;
  else if (rom_type&MBC3) select_rom_page=mbc3_select_page;
  else if (rom_type&MBC5) select_rom_page=mbc5_select_page;
  else select_rom_page=select_default;  

  gbmemory_reset();
}

void gbmemory_reset(void) {
  int i;
  
  memset(key,0,256);
  memset(oam_space,0,0xa0);
  memcpy(&himem[0x60],IOMem,0xff);
  
  DIVID=0x00;
  TIME_MOD=0x00;
  TIME_CONTROL=0x00;
  TIME_COUNTER=0x00;
  
  NR10=0x80;
  NR11=0xbf;
  NR12=0xf3;
  NR14=0xbf;
  NR21=0x3f;
  NR21=0x00;
  NR24=0xbf;
  NR30=0x7f;
  NR31=0xff;
  NR32=0x9f;
  NR33=0xbf;
  NR41=0xff;
  NR42=0x00;
  NR43=0x00;
  NR44=0xbf;
  NR50=0x77;
  NR51=0xf3;
  /* FIXME: SGB value != */
  NR52=0xf1;

  LCDCCONT=0x91;
  CURLINE=0x00;
  CMP_LINE=0x00;

  INT_ENABLE=0x00;
  INT_FLAG=0x00;
  
  WRAM_BANK=0x00;
  VRAM_BANK=0x00;  
 
  dma_info.type=NO_DMA;

  active_rom_page=1;
  active_ram_page=0;
  active_vram_page=0;
  active_wram_page=1;
  
  ram_enable=0;

  /* FAST MEM SYSTEM */
  
  /* ROM */
  set_mem_write_fun_access(0x00,mem_write_0000_1fff_default);
  set_mem_write_fun_access(0x01,mem_write_0000_1fff_default);
  set_mem_write_fun_access(0x02,mem_write_2000_3fff_default);
  set_mem_write_fun_access(0x03,mem_write_2000_3fff_default);
  set_mem_write_fun_access(0x04,mem_write_4000_5fff_default);
  set_mem_write_fun_access(0x05,mem_write_4000_5fff_default);
  set_mem_write_fun_access(0x06,mem_write_6000_7fff_default);
  set_mem_write_fun_access(0x07,mem_write_6000_7fff_default);

  if (rom_type&MBC1) select_rom_page=mbc1_select_page;
  else if (rom_type&MBC2) select_rom_page=mbc2_select_page;
  else if (rom_type&MBC3) select_rom_page=mbc3_select_page;
  else if (rom_type&MBC5) select_rom_page=mbc5_select_page;
  else select_rom_page=select_default;  

  set_mem_write_fun_access(0x02,select_rom_page);
  set_mem_write_fun_access(0x03,select_rom_page);

  for(i=0;i<4;i++) {
    set_mem_read_direct_access(i,&rom_page[0][i*0x1000]);
    set_mem_read_direct_access(i+4,&rom_page[active_rom_page][i*0x1000]);
  }

  if (rom_type&MBC1) {
    set_mem_write_fun_access(0x04,mbc1_write_4000_5fff);
    set_mem_write_fun_access(0x05,mbc1_write_4000_5fff);
    set_mem_write_fun_access(0x06,mbc1_write_6000_7fff);
    set_mem_write_fun_access(0x07,mbc1_write_6000_7fff);
  }

  if (rom_type&RUMBLE) {
    set_mem_write_fun_access(0x04,rumble_write_4000_5fff);
    set_mem_write_fun_access(0x05,rumble_write_4000_5fff);
  }    

  if (rom_type&TIMER) {
    set_mem_write_fun_access(0x04,timer_write_4000_5fff);
    set_mem_write_fun_access(0x05,timer_write_4000_5fff);
    set_mem_write_fun_access(0x06,timer_write_6000_7fff);
    set_mem_write_fun_access(0x07,timer_write_6000_7fff);
  }

  /* VRAM */
  for(i=0;i<2;i++) {
    set_mem_read_direct_access(i+8,&vram_page[0][i*0x1000]);
    set_mem_write_direct_access(i+8,&vram_page[0][i*0x1000]);
  }
    
  /* RAM */
  if (rom_type&TIMER) {
    ram_can_direct_access=0;
    mem_write_a000_bfff=timer_write_a000_bfff;
    mem_read_a000_bfff=timer_read_a000_bfff;
     for(i=0;i<2;i++) {
       set_mem_read_fun_access(i+0x0A,mem_read_a000_bfff);
       set_mem_write_fun_access(i+0x0A,mem_write_a000_bfff);
     }
  } else {
    ram_can_direct_access=1;
    mem_read_a000_bfff=mem_read_a000_bfff_default;
    mem_write_a000_bfff=mem_write_a000_bfff_default;
    for(i=0;i<2;i++) {
      set_mem_read_direct_access(i+0x0A,&ram_page[0][i*0x1000]);
      set_mem_write_direct_access(i+0x0A,&ram_page[0][i*0x1000]);
    }
  }

  /* WRAM */
  set_mem_read_direct_access(0x0C,&wram_page[0][0]);
  set_mem_read_direct_access(0x0D,&wram_page[active_wram_page][0]);
  set_mem_read_direct_access(0x0E,&wram_page[0][0]);

  set_mem_write_direct_access(0x0C,&wram_page[0][0]);
  set_mem_write_direct_access(0x0D,&wram_page[active_wram_page][0]);
  set_mem_write_direct_access(0x0E,&wram_page[0][0]);

  /* OAM HIMEM */
  set_mem_read_fun_access(0x0f,mem_read_f000_ffff_default);
  set_mem_write_fun_access(0x0f,mem_write_f000_ffff_default);  
}

void push_stack_word(Uint16 v)
{
  Uint8 h=((v&0xff00)>>8);
  Uint8 l=(v&0x00ff);    
  
  --gbcpu->sp.w;
  mem_write_fast(gbcpu->sp.w,h);
  --gbcpu->sp.w;
  mem_write_fast(gbcpu->sp.w,l);
  /*mem_write(--gbcpu->sp.w,h);
    mem_write(--gbcpu->sp.w,l);*/
}

Uint8 **alloc_mem_page(Uint16 nb_page,Uint32 size)
{
  Uint8 **page;
  int i;
  page=(Uint8 **)malloc(sizeof(Uint8 *)*nb_page);
  for(i=0;i<nb_page;i++) {
    page[i]=(Uint8 *)malloc(sizeof(Uint8)*size);
    memset(page[i],0,size);
  }
  return page;
}

void free_mem_page(Uint8 **page,Uint16 nb_page)
{
  int i;
  for(i=0;i<nb_page;i++) {
    free(page[i]);
    page[i]=NULL;
  }
  free(page);
}

/* DMA FUNCTIONS */

__inline__ void do_hdma(void) {
  int i;
  Uint8 t;

  for(i=0;i<16;i++) {
    mem_read_fast(dma_info.src,t);
    dma_info.src++;
    mem_write_fast(dma_info.dest,t);    
    dma_info.dest++;
  }
  
  HDMA_CTRL1=(dma_info.src&0xff00)>>8;
  HDMA_CTRL2=(dma_info.src&0xf0);
  HDMA_CTRL3=(dma_info.dest&0xff00)>>8;
  HDMA_CTRL4=(dma_info.dest&0xf0);
  
  HDMA_CTRL5--;
  dma_info.lg-=16;
  if (HDMA_CTRL5==0xff) dma_info.type=NO_DMA;
}

__inline__ void do_gdma(void) {
  int i;
  Uint8 t;
  Uint8 bk;
  Uint8 *d=&vram_page[active_vram_page][dma_info.dest&0x1fff];
  
  for(i=0;i<dma_info.lg;i++) {
    mem_read_fast(dma_info.src,t);
    dma_info.src++;
    mem_write_fast(dma_info.dest,t);
    dma_info.dest++;
  }
  /*} else {
    for(i=0;i<dma_info.lg;i++) {
    t=mem_read(dma_info.src);
    dma_info.src++;
    mem_write(dma_info.dest++,t);   
    }
    }*/  

  HDMA_CTRL1=(dma_info.src&0xff00)>>8;
  HDMA_CTRL2=(dma_info.src&0xf0);
  HDMA_CTRL3=(dma_info.dest&0xff00)>>8;
  HDMA_CTRL4=(dma_info.dest&0xf0);
  if (!conf.gdma_cycle) {
    HDMA_CTRL5=0xff;
    dma_info.type=NO_DMA;
  } else dma_info.type=GDMA;
}


__inline__ void hdma_request(Uint8 v)
{
  /* FIXME : control are necesary ( i think ) */
  //if (LCDCCONT&0x80) {

  if (dma_info.type==HDMA) {
    int i;
    Uint8 t;
    for(i=0;i<dma_info.lg;i++) {
      t=mem_read(dma_info.src);
      dma_info.src++;
      mem_write(dma_info.dest++,t);
    }
  }

  dma_info.src=((HDMA_CTRL1<<4)|(HDMA_CTRL2>>4))<<4;
  dma_info.dest=((((HDMA_CTRL3&31)|0x80)<<4)|(HDMA_CTRL4>>4))<<4;
  dma_info.lg=((v&0x7f)+1)<<4;
  HDMA_CTRL5=v&0x7f;
  dma_info.type=HDMA;
  
  if ((LCDCSTAT&0x03)==0x00) do_hdma();
    /*} else  {
      dma_info.v=v;
      dma_info.src=(HDMA_CTRL1<<8)|(HDMA_CTRL2&0xf0);
      dma_info.dest=(HDMA_CTRL3<<8)|(HDMA_CTRL4&0xf0)|0x8000;
      dma_info.lg=((v&0x7f)+1)<<4;
      HDMA_CTRL5=0xff;
      dma_info.type=NO_DMA;
      do_gdma();
      }*/
}

__inline__ void gdma_request(Uint8 v)
{
  dma_info.src=((HDMA_CTRL1<<4)|(HDMA_CTRL2>>4))<<4;
  dma_info.dest=((((HDMA_CTRL3&31)|0x80)<<4)|(HDMA_CTRL4>>4))<<4;
  dma_info.lg=((v&0x7f)+1)<<4;
 
  /* FIXME: gdma: how many cycle take GDMA ? */
  dma_info.gdma_cycle=((((gbcpu->mode==DOUBLE_SPEED)?110:220)+((v&0x7f)*7.63))*0.000001)*
    (((gbcpu->mode==DOUBLE_SPEED)?4194304*2:4194304));

  /* TGB: */
  dma_info.gdma_cycle=456*2+((v&0x7f)+1)*32*(gbcpu->mode==DOUBLE_SPEED?2:1);

  

  /* FIXME: gdma: control are necesary ( i think ) */
  do_gdma();
}


__inline__ void do_dma(Uint8 v)
{
  Uint16 a=v<<8;
  Uint8 bank;
  int i;

  DMA=v;

  if (a>=0xfea0 && a<0xffff) 
    memcpy(oam_space,&himem[a-0xfea0],0xa0);
  
  if (a>=0xe000 && a<0xfe00) a-=0x2000;  // echo mem
  
  bank=(a&0xf000)>>12;
  switch(bank) {
  case 0x00:
  case 0x01:
  case 0x02:
  case 0x03:memcpy(oam_space,&rom_page[0][a],0xa0);return;  
  case 0x04:
  case 0x05:
  case 0x06:
  case 0x07:memcpy(oam_space,&rom_page[active_rom_page][a-0x4000],0xa0);return;
  case 0x08:
  case 0x09:memcpy(oam_space,&vram_page[active_vram_page][a-0x8000],0xa0);return;
  case 0x0a:
  case 0x0b:memcpy(oam_space,&ram_page[active_ram_page][a-0xa000],0xa0);return;
  case 0x0c:memcpy(oam_space,&wram_page[0][a-0xc000],0xa0);return;
  case 0x0d:memcpy(oam_space,&wram_page[active_wram_page][a-0xd000],0xa0);return; 
  default:
    for(i=0;i<0xa0;i++)
      oam_space[i]=mem_read(a+i);
    break;
  }
}

/* READ FUNCTIONS */

__inline__ Uint8 mem_read_ff(Uint16 adr)
{
  if (adr==0xff00) {
    Uint8 t=0xff;
    /*if (sgb.check) {
      sgb.check=0;
      return GB_PAD;
      }*/
    // if (GB_PAD==0x30) 
    //printf("read GB_PAD %02x\n",GB_PAD);
    /* FIXME */
    /* if (GB_PAD==0x03) {
      printf("determine type\n");
      return (gameboy_type==SUPER_GAMEBOY)?0x3f:0xff;
      }*/
    //printf("%d %02x\n",sgb.b_i,GB_PAD);
    /*if (GB_PAD==0x30) return 0xff;
      if (GB_PAD==0x10) GB_PAD=((~(gb_pad&0x0f))&0xdf);
      else if (GB_PAD==0x20) GB_PAD=((~(gb_pad>>4))&0xef);
      return GB_PAD;*/

    // if (GB_PAD==0xff) return ((conf.gb_type&SUPER_GAMEBOY)?0x3f:0xff);

    switch(GB_PAD&0x30) {
    case 0x00:
    case 0xff:t=0xff;break;      
    case 0x10:t=((~(gb_pad&0x0f)))|0x10; break;
    case 0x20:t=((~(gb_pad>>4)))|0x20; break;
    case 0x30:t=(((!sgb.player)?(~0x00):(~0x01)));break;
    }
    /*if (GB_PAD==0x10) GB_PAD=((~(gb_pad&0x0f)))|0x10;
      else if (GB_PAD==0x20) GB_PAD=((~(gb_pad>>4)))|0x20;
      return GB_PAD;*/
    //if (sgb.player&0x80) {
    if (conf.gb_type&SUPER_GAMEBOY) t/*GB_PAD*/&=0x3f;
      /*      sgb.player&=(~0x80);
	      }*/
    return  t;
    //GB_PAD=t;
    //return GB_PAD;
  }

  if (conf.gb_type&COLOR_GAMEBOY) {
    if (adr==0xff4d) {
      if (gbcpu->mode==DOUBLE_SPEED) return CPU_SPEED|0x80;
      else return 0x00;
    }
    
    if (adr==0xff69) {
      if (BGPAL_SPE&0x01)
	return pal_col_bck_gb[(BGPAL_SPE&0x38)>>3][(BGPAL_SPE&0x06)>>1]>>8;
    else return pal_col_bck_gb[(BGPAL_SPE&0x38)>>3][(BGPAL_SPE&0x06)>>1]&0xff;
    }
    
    if (adr==0xff6b) {
      if (OBJPAL_SPE&0x01) 
	return pal_col_obj_gb[(OBJPAL_SPE&0x38)>>3][(OBJPAL_SPE&0x06)>>1]>>8;
      else return pal_col_obj_gb[(OBJPAL_SPE&0x38)>>3][(OBJPAL_SPE&0x06)>>1]&0xff;
    }
  }

  if (adr>=0xff10 && adr<=0xff3f && conf.sound) return read_sound_reg(adr);
  
#ifdef DEBUG
  if (adr==0xff40) add_mem_msg("read LCDCCONT %02x\n",LCDCCONT);
  if (adr==0xff41) add_mem_msg("read LCDCSTAT %02x\n",LCDCSTAT);
  if (adr==0xff0f) add_mem_msg("read INT_FLAG %02x\n",INT_FLAG);
  if (adr==0xffff) add_mem_msg("read INT_ENABLE %02x\n",INT_ENABLE);
  if (adr==0xff45) add_mem_msg("read CMP_LINE %02x\n",CMP_LINE);
#endif

  return himem[adr-0xfea0];
}

Uint8 mem_read_a000_bfff_default(Uint16 adr) {
  if (!ram_enable) return 0xff;
  return ram_page[active_ram_page][adr-0xa000];
}

Uint8 mem_read_f000_ffff_default(Uint16 adr) {
  if (adr>=0xff00) return mem_read_ff(adr);
  if (adr>=0xfe00 && adr<0xfea0) return oam_space[adr-0xfe00];
  if (adr>=0xfea0 && adr<0xff00) return himem[adr-0xfea0];
  return 0xff;
}


Uint8 timer_read_a000_bfff(Uint16 adr) {
  /* FIXME: Timer and ram enable ???? */
  if (!ram_enable) return 0xFF;
  if (rom_timer->reg_sel&0x08) 
    return rom_timer->regl[rom_timer->reg_sel&0x07];
  return ram_page[active_ram_page][adr-0xa000];
}

Uint8 mem_read_default(Uint16 adr)
{
  Uint8 bank;
  
  if (adr>=0xfe00 && adr<0xfea0) return oam_space[adr-0xfe00];
  if (adr>=0xfea0 && adr<0xff00) return himem[adr-0xfea0];
  if (adr>=0xff00) return mem_read_ff(adr);
  if (adr>=0xe000 && adr<0xfe00) adr-=0x2000;  // echo mem
  
  bank=(adr&0xf000)>>12;
  switch(bank) {
  case 0x00:
  case 0x01:
  case 0x02:
  case 0x03:return rom_page[0][adr];
  case 0x04:
  case 0x05:
  case 0x06:
  case 0x07:return rom_page[active_rom_page][adr-0x4000];
  case 0x08:
  case 0x09:return vram_page[active_vram_page][adr-0x8000];
  case 0x0a:
  case 0x0b:
    if (!ram_enable) return 0xff;
    if (rom_type&TIMER && rom_timer->reg_sel&0x08) 
      return rom_timer->regl[rom_timer->reg_sel&0x07];
    return ram_page[active_ram_page][adr-0xa000];
  case 0x0c:return wram_page[0][adr-0xc000];
  case 0x0d:return wram_page[active_wram_page][adr-0xd000];
  }
  return 0xFF;
}

__inline__ void update_gb_pad(void) {
  gb_pad=0;
  
  if ((joy_but[jmap[PAD_START]]) || (key[kmap[PAD_START]])) gb_pad|=0x08; /* Start */
  if ((joy_but[jmap[PAD_SELECT]]) || (key[kmap[PAD_SELECT]])) gb_pad|=0x04; /* Select */
  if ((joy_but[jmap[PAD_A]]) || (key[kmap[PAD_A]])) gb_pad|=0x01; /* A */
  if ((joy_but[jmap[PAD_B]]) || (key[kmap[PAD_B]])) gb_pad|=0x02; /* B */
  
  if ((joy_axis[jmap[PAD_LEFT]]<-1000) || (key[kmap[PAD_LEFT]])) gb_pad|=0x20;
  if ((joy_axis[jmap[PAD_RIGHT]]>1000) || (key[kmap[PAD_RIGHT]])) gb_pad|=0x10;
  if ((joy_axis[jmap[PAD_UP]]<-1000) ||  (key[kmap[PAD_UP]])) gb_pad|=0x40;
  if ((joy_axis[jmap[PAD_DOWN]]>1000) || (key[kmap[PAD_DOWN]])) gb_pad|=0x80;
  
}

extern Uint8 win_line;

__inline__ void write2lcdccont(Uint8 v)
{
  if ((LCDCCONT&0x80) && (!(v&0x80))) {  // LCDC go to off
#ifdef DEBUG
    add_mem_msg("LCDC got to off \n");
#endif
    gblcdc->mode=HBLANK_PER;
    LCDCSTAT=(LCDCSTAT&0xfc);
    CURLINE=0;
    gblcdc->cycle=0;
    
    dma_info.type=NO_DMA;
    HDMA_CTRL5=0xff;
    clear_screen();
    reset_frame_skip();
  }
    
  if ((!(LCDCCONT&0x80)) && (v&0x80)) { // LCDC go to on
#ifdef DEBUG
    add_mem_msg("LCDC got to on \n");
#endif
    gblcdc_set_on();
  }
  LCDCCONT=v;
}

__inline__ void mem_write_ff(Uint16 adr,Uint8 v) {
  Uint16 a;
  Uint8 c,p;

  if (conf.gb_type&COLOR_GAMEBOY) {
    if (adr==0xff4d) {
      if (v&0x80 || v&0x01) {
	go2double_speed();
	/* FIXME: */
	get_nb_cycle();
      }
      CPU_SPEED=(v&0xfe);
      return;
    }

    if (adr==0xff4f) {
      set_active_vram_page(v);
      VRAM_BANK=active_vram_page;
      return;
    }

    if (adr==0xff55) {   // HDMA & GDMA
      if (v&0x80) hdma_request(v);
      else gdma_request(v);
      return;
    }

    if (adr==0xff68) {
      BGPAL_SPE=v&0xbf;
      if (BGPAL_SPE&0x01)
	BGPAL_DATA=pal_col_bck_gb[(BGPAL_SPE>>3)&0x07][(BGPAL_SPE>>1)&0x03]>>8;
      else BGPAL_DATA=pal_col_bck_gb[(BGPAL_SPE>>3)&0x07][(BGPAL_SPE>>1)&0x03]&0xff;
      return;
    }
  
    if (adr==0xff69) {
     
      c=(BGPAL_SPE>>1)&0x03;
      p=(BGPAL_SPE>>3)&0x07;
      if (BGPAL_SPE&0x01) 
	pal_col_bck_gb[p][c]=(pal_col_bck_gb[p][c]&0x00ff)|(v<<8);
      else pal_col_bck_gb[p][c]=(pal_col_bck_gb[p][c]&0xff00)|v;
     
      pal_col_bck[p][c]=Filter[pal_col_bck_gb[p][c]&0x7FFF];
      if (BGPAL_SPE&0x80) {
	a=BGPAL_SPE&0x3f;
	a++;
	BGPAL_SPE=(a&0x3f)|0x80;
      }
      BGPAL_DATA=v;
      return;
    }

    if (adr==0xff6a) {
      OBJPAL_SPE=v&0xbf;
      if (OBJPAL_SPE&0x01) 
	OBJPAL_DATA=pal_col_obj_gb[(OBJPAL_SPE>>3)&0x07][(OBJPAL_SPE>>1)&0x03]>>8;
      else OBJPAL_DATA=pal_col_obj_gb[(OBJPAL_SPE>>3)&0x07][(OBJPAL_SPE>>1)&0x03]&0xff;
      return;
    }

    if (adr==0xff6b) {
      c=(OBJPAL_SPE>>1)&0x03;
      p=(OBJPAL_SPE>>3)&0x07;
      if (OBJPAL_SPE&0x01) 
	pal_col_obj_gb[p][c]=(pal_col_obj_gb[p][c]&0x00ff)|(v<<8);
      else pal_col_obj_gb[p][c]=(pal_col_obj_gb[p][c]&0xff00)|v;
    
      pal_col_obj[p][c]=Filter[pal_col_obj_gb[p][c]&0x7FFF];
      if (OBJPAL_SPE&0x80) {
	a=OBJPAL_SPE&0x3f;
	a++;
	OBJPAL_SPE=(a&0x3f)|0x80;
      }
      OBJPAL_DATA=v;
      return;
    }

    if (adr==0xff70) {
      set_active_wram_page(v);
      WRAM_BANK=active_wram_page;
      return;
    }
  } // end COLOR_GAMEBOY

  // Update sound if necessary
  if (adr>=0xff10 && adr<=0xff3f && conf.sound) {
    write_sound_reg(adr,v);
    return;
  }

  switch(adr) {
  case 0xff00:
    if (sgb.on) { // sgb transfert
      switch(v) {
      case 0x00:sgb_init_transfer();return;
      case 0x10:sgb.b=1;break;
      case 0x20:sgb.b=0;break;
      case 0x30:
	if (sgb.b_i==-1) {sgb.b_i=0;return;}
	if (sgb.b_i==128 /*&& sgb.b==0*/) {
	  sgb_exec_cmd();
	  sgb.on=0;
	  return;
	}
	sgb.pack[sgb.b_i/8]=(sgb.pack[sgb.b_i/8]>>1)|(sgb.b<<(8-sgb.b%8));
	sgb.b_i++;
      }
    } else {
      if (v==0x00) {
	if (conf.gb_type&SUPER_GAMEBOY) sgb_init_transfer();
	return;
      }
      if (v==0x30 ) GB_PAD=0xff;
      else GB_PAD=v;
      update_gb_pad();
    }
    break;
    /* EXPERIMENTAL */
  case 0xff01:
    SB=v;
    break;
  case 0xff02:
    
    /*    SC=v&0x81;
	  if ((v&0x80)&&(v&0x01)) serial_cycle_todo=512;
	  break;*/

    if (!conf.serial_on) {
      if ((v&0x81)==0x81) {
	SB=0xff;
	SC=v&0x7f;
#ifdef DEBUG
	if (link_int_enable) set_interrupt(SERIAL_INT);
#else
	set_interrupt(SERIAL_INT);
#endif
      }
    } else {
      if ((v&0x81)==0x81) {
	send_byte(SB);
	serial_cycle_todo=4096;
      } //else serial_cycle_todo=500;
      SC=v;
    }

    /*    if (conf.serial_on) {
      if ((v&0x81)==0x81) {
	send_byte(SB);
	//serial_cycle_todo=4096*2;
      } 
    } 
    SB=0xff;
    SC=v&0x7f;*/
    break;
  case 0xff0f:
    unset_interrupt(((INT_FLAG)^(v&0x1f))^(v&0x1f));
    if (v&0x1f) set_interrupt(v&0x1f);
#ifdef DEBUG
    add_mem_msg("write %02x to INT_FLAG %02x\n",v,INT_FLAG);
#endif
    break;
  case 0xffff:
    INT_ENABLE=v&0x1f;
#ifdef DEBUG
    add_mem_msg("write %02x to INT_ENABLE %02x\n",v,INT_ENABLE);
#endif
    break;
  case 0xff04:DIVID=0;break;
  case 0xff05:
    TIME_COUNTER=v;
    /* FIXME */
    gbtimer->cycle=0;
#ifdef DEBUG
    add_mem_msg("write %02x to TIMER_COUNTER, %02x:TIMER_CONTROLE, %02x:TIME_MOD\n",v,TIME_CONTROL,TIME_MOD);
#endif
    break;
  case 0xff06:
    TIME_MOD=v;
#ifdef DEBUG
    add_mem_msg("write %02x to TIMER_MOD\n",v);
#endif
    break;
  case 0xff07:
    if (v&4) {
      switch(v&3) {
      case 0: gbtimer->clk_inc=1024;break;
      case 1: gbtimer->clk_inc=16;break;
      case 2: gbtimer->clk_inc=64;break;
      case 3: gbtimer->clk_inc=256;break;
      }
    } else gbtimer->clk_inc=0;
    gbtimer->cycle=gbtimer->clk_inc;
    /* FIXME */
    /*if (!(TIME_CONTROL&0x04))
      gbtimer->cycle+=gbtimer->clk_inc;
      else gbtimer->cycle=gbtimer->clk_inc;*/
    //    if ((v&0x04) && !(TIME_CONTROL&0x04)) gbtimer->cycle=0;
    TIME_CONTROL=v;
#ifdef DEBUG
    add_mem_msg("write %02x to TIMER_CONTROLER\n",v);
#endif
    break;
  case 0xff40:
    write2lcdccont(v);
#ifdef DEBUG
    add_mem_msg("write %02x to LCDCCONT %02x\n",v,LCDCCONT);
#endif   
    break;
  case 0xff41:
    /* Emulate Normal Gameboy Bug (fix Legend Of Zerd) */
    if (!(conf.gb_type&COLOR_GAMEBOY)) {
#ifdef DEBUG
      //if (!(LCDCSTAT&0x03) || (LCDCSTAT&0x03)==0x01) set_interrupt(LCDC_INT);
      if (!(LCDCSTAT&0x02)) 
	set_interrupt(LCDC_INT);
      add_int_msg("Emulate gmb lcdcstat write bug\n");
#else
      //if (!(LCDCSTAT&0x03) || (LCDCSTAT&0x03)==0x01) set_interrupt(LCDC_INT);
      if (!(LCDCSTAT&0x02)) 
	/* FIXME: Stat write bug */
	set_interrupt(LCDC_INT);
#endif
      //if ((v&0x20) && (!(LCDCSTAT&0x20)) && (LCDCSTAT&0x02)) set_interrupt(LCDC_INT);
      LCDCSTAT=(LCDCSTAT&0x07)|(v&0x78);
    } else LCDCSTAT=(LCDCSTAT&0x07)|(v&0x78);    
#ifdef DEBUG
    add_mem_msg("Write %02x to LCDCSTAT %02x\n",v,LCDCSTAT);
#endif  
    break;
  case 0xff44:
#ifdef DEBUG
    add_mem_msg("Write to CURLINE %02x \n",v);
#endif
    CURLINE=0;
    if (LCDCCONT&0x80) gblcdc_set_on();
    break;
  case 0xff45:CMP_LINE=v;
#ifdef DEBUG
    add_mem_msg("Write %02x to CMPLINE\n",v);
#endif   
    /* FIXME */
    if (CURLINE==CMP_LINE) LCDCSTAT|=0x04;
    else LCDCSTAT&=~0x04;
    /*#ifdef DEBUG
    if (lcd_lyc_int_enable && LCDCCONT&0x80 && LCDCSTAT&0x40 && LCDCSTAT&0x04 && (LCDCSTAT&0x02)==0x02) {
      set_interrupt(LCDC_INT);
      add_int_msg("Write To Cmp Line and set lyc int\n");
    }
    #else*/
    if (LCDCCONT&0x80 && LCDCSTAT&0x40 && LCDCSTAT&0x04 && (LCDCSTAT&0x02)==0x02) 
      set_interrupt(LCDC_INT);
    //#endif
    break;
  case 0xff46:      // DMA
    do_dma(v);
    break;
  case 0xff47:
#ifdef DEBUG
    add_mem_msg("set bck_pal %02x\n",v);
#endif
    gb_set_pal_bck(v);
    break;
  case 0xff48:
#ifdef DEBUG
    add_mem_msg("set obj_pal0 %02x\n",v);
#endif  
    OBJ0PAL=v;
    pal_obj[0][0]=OBJ0PAL&3;
    pal_obj[0][1]=(OBJ0PAL>>2)&3;
    pal_obj[0][2]=(OBJ0PAL>>4)&3;
    pal_obj[0][3]=(OBJ0PAL>>6)&3;
    break;
  case 0xff49:
#ifdef DEBUG
    add_mem_msg("set obj_pal1 %02x\n",v);
#endif
    OBJ1PAL=v;
    pal_obj[1][0]=OBJ1PAL&3;
    pal_obj[1][1]=(OBJ1PAL>>2)&3;
    pal_obj[1][2]=(OBJ1PAL>>4)&3;
    pal_obj[1][3]=(OBJ1PAL>>6)&3;
    break;
  case 0xff4d:CPU_SPEED=0x80;break;
  default:
    himem[adr-0xfea0]=v;
    break;
  }
}

/* Mbc1 */

void mbc1_write_4000_5fff(Uint16 adr,Uint8 v) {
  if (mbc1_mem_mode==MBC1_16_8_MEM_MODE) 
    mbc1_line=v&0x03;
  else set_active_ram_page(v);
}

void mbc1_write_6000_7fff(Uint16 adr,Uint8 v) {
 if (!v) mbc1_mem_mode=MBC1_16_8_MEM_MODE;
 else if (v==1) mbc1_mem_mode=MBC1_4_32_MEM_MODE;
}

/* Rumble */

void rumble_write_4000_5fff(Uint16 adr,Uint8 v) {
  /* FIXME: Rumble write v&0x08 we must active ram Page ????? */
  set_active_ram_page(v);
  if (conf.rumble_on && v&0x08) rb_on=1;
}

/* Timer */

__inline__ void rom_timer_lock(void) {
  memcpy(rom_timer->regl,rom_timer->reg,sizeof(Uint8)*5);
}

__inline__ void rom_timer_write(Uint8 v) {
  if (!(rom_timer->reg_sel&0x08)) return;
  switch(rom_timer->reg_sel&0x07) {
  case 0x00:rom_timer->reg[0]=(v%60);break;  // seconds
  case 0x01:rom_timer->reg[1]=(v%60);break;  // minutes
  case 0x02:rom_timer->reg[2]=(v%24);break;  // hours
  case 0x03:rom_timer->reg[3]=v;break;       // dayl
  case 0x04:rom_timer->reg[4]=v;break;       // dayh,start|stop,day carry
  }
}   

void timer_write_a000_bfff(Uint16 adr,Uint8 v) {
  /* FIXME: timer write and ram enable ????? */
  if (!ram_enable) return;
  if (rom_timer->reg_sel&0x08) {
    rom_timer_write(v);
    return;
  }
  ram_page[active_ram_page][adr-0xa000]=v;
}

void timer_write_4000_5fff(Uint16 adr,Uint8 v) {
  rom_timer->reg_sel=v&0x0f;
}

void timer_write_6000_7fff(Uint16 adr,Uint8 v) {
  if (!rom_timer->latch && v) rom_timer_lock();
  rom_timer->latch=v;
}


/* Write Function Default */

void mem_write_0000_1fff_default(Uint16 adr,Uint8 v) {
  if ((v&0x0f)==0x0a) enable_ram();
  else disable_ram();
}

void mem_write_2000_3fff_default(Uint16 adr,Uint8 v) {
  select_rom_page(adr,v);
}

void mem_write_4000_5fff_default(Uint16 adr,Uint8 v) {
  set_active_ram_page(v);
}

void mem_write_6000_7fff_default(Uint16 adr,Uint8 v) {
  //printf("WARNING: Write %02x at %04x\n",v,adr);
}

void mem_write_a000_bfff_default(Uint16 adr,Uint8 v) {
  if (!ram_enable) return;
  ram_page[active_ram_page][adr-0xa000]=v;
}

/*void mem_write_c000_dfff_default(Uint16 adr,Uint8 v) {
  Uint8 bk;
  bk=(adr&0xf000)>>12;
  switch(bk) {
  case 0xc:wram_page[0][adr-0xc000]=v;return;
  case 0xd:wram_page[active_wram_page][adr-0xd000]=v;return;
  }
}

void mem_write_e000_efff_default(Uint16 adr,Uint8 v) {
  wram_page[0][adr-0xe000]=v;
  }*/

void mem_write_f000_ffff_default(Uint16 adr,Uint8 v) {
  if (adr>=0xff00) {
    mem_write_ff(adr,v);
    return;
  }
  if (adr>=0xfe00 && adr<0xfea0) {
    oam_space[adr-0xfe00]=v;
    return;
  }
  if (adr>=0xfea0 && adr<0xff00) {
    himem[adr-0xfea0]=v;
    return;
  }
  wram_page[active_wram_page][adr-0xf000]=v;
}


/*void mem_write_0000_7fff(Uint16 adr,Uint8 v) {
  Uint8 bk;
  
  bk=(adr&0xf000)>>12;
  switch(bk) {
  case 0:
  case 1:
    if ((v&0x0f)==0x0a) enable_ram();
    else disable_ram();
    return;
  case 2:
  case 3:select_rom_page(adr,v);return;
  case 4:
  case 5:
    if (rom_type&MBC1) {
      if (mbc1_mem_mode==MBC1_16_8_MEM_MODE) 
	mbc1_line=v&0x03;
      else set_active_ram_page(v);
      return;
    }
    else set_active_ram_page(v);
    if (rom_type&RUMBLE && conf.rumble_on && v&0x08) rb_on=1;
    if (rom_type&TIMER) rom_timer->reg_sel=v&0x0f;
    return;
  case 6:
  case 7:
    if (rom_type&MBC1) {
      if (!v) mbc1_mem_mode=MBC1_16_8_MEM_MODE;
      else if (v==1) mbc1_mem_mode=MBC1_4_32_MEM_MODE;
      return;
    }
    if (rom_type&TIMER) {
      if (!rom_timer->latch && v) rom_timer_lock();
      rom_timer->latch=v;
    }
    return;
  default:
    printf("Warning: Write 0000 7fff bank %02x\n",bk);break;
  }  
  }*/

void mem_write_default(Uint16 adr,Uint8 v) 
{
  Uint8 bk;

  printf("PC: %04x: Write Default %02x at %04x\n",gbcpu->pc.w,v,adr);

  if (adr>=0xfe00 && adr<0xfea0) {
    oam_space[adr-0xfe00]=v;
    return;
  }
  
  if (adr>=0xfea0 && adr<0xff00) {
    himem[adr-0xfea0]=v;
    return;
  }
  
  if (adr>=0xff00) {
    mem_write_ff(adr,v);
    return;
  }

  if (adr>=0xe000 && adr<0xfe00) adr-=0x2000;  // echo mem
  
  bk=(adr&0xf000)>>12;
  switch(bk) {
  case 0:
  case 1:
    if ((v&0x0f)==0x0a) enable_ram();
    else disable_ram();
    return;
    //ram_enable=((v&0x0f)==0x0a)?(1):(0);return;
  case 2:
  case 3:select_rom_page(adr,v);return;
  case 4:
  case 5:
    if (rom_type&MBC1) {
      if (mbc1_mem_mode==MBC1_16_8_MEM_MODE) 
	mbc1_line=v&0x03;
      else set_active_ram_page(v);
      return;
    }
    else set_active_ram_page(v);
    
    if (rom_type&RUMBLE && conf.rumble_on && v&0x08) rb_on=1;

    if (rom_type&TIMER) {
      rom_timer->reg_sel=v&0x0f;
      //      printf("select %02x \n",rom_timer->reg_sel);
    }
    return;
  case 6:
  case 7:
    if (rom_type&MBC1) {
      if (!v) mbc1_mem_mode=MBC1_16_8_MEM_MODE;
      else if (v==1) mbc1_mem_mode=MBC1_4_32_MEM_MODE;
      return;
    }
    if (rom_type&TIMER) {
      if (!rom_timer->latch && v) rom_timer_lock();
      rom_timer->latch=v;
    }
    return;
  case 8:
  case 9:vram_page[active_vram_page][adr-0x8000]=v;return;
  case 0xa:
  case 0xb:
    if (!ram_enable) return;
    if (rom_type&TIMER && rom_timer->reg_sel&0x08) {
      rom_timer_write(v);
      return;
    }
    ram_page[active_ram_page][adr-0xa000]=v;
    return;
  case 0xc:wram_page[0][adr-0xc000]=v;return;
  case 0xd:wram_page[active_wram_page][adr-0xd000]=v;return;
  }
}
