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
#include <SDL/SDL.h>
#include "memory.h"
#include "cpu.h"
#include "rom.h"
#include "vram.h"
#include "interrupt.h"
#include "frame_skip.h"
#include "sound.h"

#ifdef LINUX_JOYSTICK
#include "joystick.h"
#endif

#ifdef USE_LOG
#include "log.h"
#endif

INT32 joy_axis[2]={0};
UINT32 joy_but=0;

UINT8 key[256];
UINT8 gb_pad_code[8]={98,104,100,102,53,52,36,62};

#ifdef LINUX_JOYSTICK
JOY_CONTROL *my_joy=0;
#endif

UINT8 rom_mask;
UINT16 nb_rom_page;
UINT16 nb_ram_page;
UINT16 nb_vram_page;
UINT16 nb_wram_page;
UINT16 active_rom_page=0;
UINT16 active_ram_page=0;
UINT16 active_vram_page=0;
UINT16 active_wram_page=0;
UINT8 **rom_page=NULL;
UINT8 **ram_page=NULL;
UINT8 **vram_page=NULL;
UINT8 **wram_page=NULL;
UINT8 oam_space[0xa0];  
UINT8 himem[0x160];     

UINT8 ram_enable=0;

UINT8 mbc1_mem_mode=MBC1_16_8_MEM_MODE;
UINT8 mbc1_line=0;
UINT8 mbc5_lower=0;
UINT8 mbc5_upper=0;

UINT8 ram_mask;

void (*select_rom_page)(UINT16 adr,UINT8 v);
void (*select_ram_page)(UINT16 adr,UINT8 v);


UINT8 IOMem[256]=
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

void select_default(UINT16 adr,UINT8 v)
{
  // do nothing
}

void mbc1_select_page(UINT16 adr,UINT8 v)
{
  UINT8 bank=v/*&0x1f*/;
  // printf("%d \n",v);
  if (bank<1) bank=1;
  active_rom_page=bank;
#ifdef USE_LOG
  put_log2("mbc1 select page %d\n",active_rom_page);
#endif
}

void mbc2_select_page(UINT16 adr,UINT8 v)
{
  UINT8 bank;
  if (adr==0x2100) {
    bank=v&0x1f;
    if (bank<1) bank=1;
    active_rom_page=bank;
  }
#ifdef USE_LOG
  put_log2("mbc2 select page %d\n",active_rom_page);
#endif
}

void mbc3_select_page(UINT16 adr,UINT8 v)
{
  UINT8 bank=v&0x7f;
  if (bank<1) bank=1;
  active_rom_page=bank;
#ifdef USE_LOG
  put_log2("mbc3 select page %d\n",active_rom_page);
#endif
}

void mbc5_select_page(UINT16 adr,UINT8 v)
{
  UINT16 bank;
  if (adr>=0x2000 && adr<0x3000)
    mbc5_lower=v&rom_mask;
  if (adr>=0x3000 && adr<0x4000 && nb_rom_page>=256) {
    mbc5_upper=v&0x01;
  }
  bank=mbc5_lower+((mbc5_upper)?256:0);
  active_rom_page=bank&rom_mask;
#ifdef USE_LOG
  put_log2("mbc5 select page %d\n",active_rom_page);
#endif
}
     

void init_gb_memory(void)
{
  memset(key,0,256);
  memset(oam_space,0,0xa0);
  memcpy(&himem[0x60],IOMem,0xff);

  NR52=0xF1;

  dma_info.type=NO_DMA;

  if (rom_type&MBC1) select_rom_page=mbc1_select_page;
  else if (rom_type&MBC2) select_rom_page=mbc2_select_page;
  else if (rom_type&MBC3) select_rom_page=mbc3_select_page;
  else if (rom_type&MBC5) select_rom_page=mbc5_select_page;
  else select_rom_page=select_default;  
}

void push_stack_word(UINT16 v)
{
  UINT8 h=((v&0xff00)>>8);
  UINT8 l=(v&0x00ff);    
  mem_write(--gbcpu->sp.w,h);
  mem_write(--gbcpu->sp.w,l);
}

UINT8 **alloc_mem_page(UINT16 nb_page,UINT32 size)
{
  UINT8 **page;
  int i;
  page=(UINT8 **)malloc(sizeof(UINT8 *)*nb_page);
  for(i=0;i<nb_page;i++) {
    page[i]=(UINT8 *)malloc(sizeof(UINT8)*size);
    memset(page[i],0,size);
  }
  return page;
}

void free_mem_page(UINT8 **page,UINT16 nb_page)
{
  int i;
  for(i=0;i<nb_page;i++) {
    free(page[i]);
    page[i]=NULL;
  }
  free(page);
}

inline void do_hdma(void) {
  int i;
  
  for(i=0;i<16;i++)
    mem_write(dma_info.dest++,mem_read(dma_info.src++));

  HDMA_CTRL1=(dma_info.src&0xff00)>>8;
  HDMA_CTRL2=(dma_info.src&0xf0);
  HDMA_CTRL3=(dma_info.dest&0xff00)>>8;
  HDMA_CTRL4=(dma_info.dest&0xf0);

  HDMA_CTRL5--;
  if (HDMA_CTRL5==0xff) dma_info.type=NO_DMA;
}

inline void do_gdma(void) {
  int i;
  for(i=0;i<dma_info.lg;i++)
    mem_write(dma_info.dest++,mem_read(dma_info.src++));
  HDMA_CTRL1=(dma_info.src&0xff00)>>8;
  HDMA_CTRL2=(dma_info.src&0xf0);
  HDMA_CTRL3=(dma_info.dest&0xff00)>>8;
  HDMA_CTRL4=(dma_info.dest&0xf0);
  HDMA_CTRL5=0xff;
  dma_info.type=NO_DMA;
}


inline void hdma_request(UINT8 v)
{
  if (LCDCCONT&0x80) {
    dma_info.v=v;
    dma_info.src=(HDMA_CTRL1<<8)|(HDMA_CTRL2&0xf0);
    dma_info.dest=(HDMA_CTRL3<<8)|(HDMA_CTRL4&0xf0)|0x8000;
    dma_info.lg=((v&0x7f)+1)<<4;
    
    HDMA_CTRL5=v&0x7f;
    dma_info.type=HDMA;
  } else dma_info.type=NO_DMA;
}

inline void gdma_request(UINT8 v)
{
  dma_info.v=v;
  dma_info.src=(HDMA_CTRL1<<8)|(HDMA_CTRL2&0xf0);
  dma_info.dest=(HDMA_CTRL3<<8)|(HDMA_CTRL4&0xf0)|0x8000;
  dma_info.lg=((v&0x7f)+1)<<4;
  
  if (!(LCDCCONT&0x80)  || (CURLINE>=0x90)) {
    //printf("gdma\n");
    dma_info.type=GDMA;
    do_gdma();
  } else {
    dma_info.type=NO_DMA;
    HDMA_CTRL5=0xff;
  }
  
  /* if (!(LCDCCONT&0x80)) do_gdma();
  else if ((LCDCSTAT&0x03)==0x01) do_gdma();
  else dma_info.type=GDMA_STAND;*/
}


inline void do_dma(UINT8 v)
{
  UINT16 a=v<<8;
  UINT8 bank;
 
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
  }
}

inline UINT8 mem_read_ff(UINT16 adr)
{
  // if (adr==0xff04) printf("%04x %02x\n",gbcpu->pc.w,DIVID);

  if (adr==0xff00) {
     update_gb_pad();
    if (GB_PAD&0x10) GB_PAD=((~(gb_pad&0x0f))&0xdf);
    else if (GB_PAD&0x20) GB_PAD=((~(gb_pad>>4))&0xef);
    return GB_PAD;
  }
  
  if (adr==0xff4d && gameboy_type&COLOR_GAMEBOY) {
    if (gbcpu->mode==DOUBLE_SPEED) return CPU_SPEED|0x80;
    else return 0x00;
  }

  if (adr==0xff69 && gameboy_type&COLOR_GAMEBOY) {
    if (BGPAL_SPE&0x01) 
      return pal_col_bck_gb[(BGPAL_SPE&0x38)>>3][(BGPAL_SPE&0x06)>>1]>>8;
    else return pal_col_bck_gb[(BGPAL_SPE&0x38)>>3][(BGPAL_SPE&0x06)>>1]&0xff;
  }

  if (adr==0xff6b && gameboy_type&COLOR_GAMEBOY) {
    if (OBJPAL_SPE&0x01) 
      return pal_col_obj_gb[(OBJPAL_SPE&0x38)>>3][(OBJPAL_SPE&0x06)>>1]>>8;
    else return pal_col_obj_gb[(OBJPAL_SPE&0x38)>>3][(OBJPAL_SPE&0x06)>>1]&0xff;
  }

  if (adr>=0xff10 && adr<=0xff3f && conf.sound) 
    return read_sound_reg(adr);

  return himem[adr-0xfea0];
}

inline UINT8 mem_read(UINT16 adr)
{
  UINT8 bank;

  if (adr>=0xfe00 && adr<0xfea0) return oam_space[adr-0xfe00];
  if (adr>=0xfea0 && adr<0xff00) return himem[adr-0xfea0];
  if (adr>=0xff00) return mem_read_ff(adr);
  if (adr>=0xe000 && adr<0xfe00) adr-=0x2000;  // echo mem
  
  bank=(adr&0xf000)>>12;
  switch(bank) {
  case 0x00:
  case 0x01:
  case 0x02:
  case 0x03:return rom_page[0][adr];break;
  case 0x04:
  case 0x05:
  case 0x06:
  case 0x07:return rom_page[active_rom_page][adr-0x4000];break;
  case 0x08:
  case 0x09:return vram_page[active_vram_page][adr-0x8000];break;
  case 0x0a:
  case 0x0b:return ram_page[active_ram_page][adr-0xa000];break;
  case 0x0c:return wram_page[0][adr-0xc000];break;
  case 0x0d:return wram_page[active_wram_page][adr-0xd000];break;
  }
  return 0xFF;
}

inline void write2lcdccont(UINT8 v)
{
  //  static UINT8 temp;
  
  //printf("write to lcdccont %0.2x \n",v);
  //  printf("%02x\n",v);
  if ((LCDCCONT&0x80) && (!(v&0x80))) {  // LCDC go to off
    lcdc_mode=HBLANK_PER;
    LCDCSTAT=(LCDCSTAT&0xfc);
    CURLINE=0;
    dma_info.type=NO_DMA;
    HDMA_CTRL5=0xff;
    //clear_screen();
  }
  
  if ((!(LCDCCONT&0x80)) && (v&0x80)) {
    /*if (dma_info.type==HDMA_STAND) {
      dma_info.type=HDMA;
      do_hdma();
      }*/
    if (conf.autofs) barath_skip_next_frame(0);
  }
   
  LCDCCONT=v;
}

inline void mem_write_ff(UINT16 adr,UINT8 v) {
  UINT16 a;
  UINT8 c,p;


  if (gameboy_type&COLOR_GAMEBOY) {
    if (adr==0xff4d) {
      if (v&0x80 || v&0x01) go2double_speed();
      CPU_SPEED=v;
      return;
    }

    if (adr==0xff4f) {
      active_vram_page=v&0x01;
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
	BGPAL_DATA=pal_col_bck_gb[(BGPAL_SPE&0x38)>>3][(BGPAL_SPE&0x06)>>1]>>8;
      else BGPAL_DATA=pal_col_bck_gb[(BGPAL_SPE&0x38)>>3][(BGPAL_SPE&0x06)>>1]&0xff;
      return;
    }
  
    if (adr==0xff69) {
      c=(BGPAL_SPE&0x06)>>1;
      p=(BGPAL_SPE&0x38)>>3;
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
	OBJPAL_DATA=pal_col_obj_gb[(OBJPAL_SPE&0x38)>>3][(OBJPAL_SPE&0x06)>>1]>>8;
      else OBJPAL_DATA=pal_col_obj_gb[(OBJPAL_SPE&0x38)>>3][(OBJPAL_SPE&0x06)>>1]&0xff;
      return;
    }

    if (adr==0xff6b) {
      c=(OBJPAL_SPE&0x06)>>1;
      p=(OBJPAL_SPE&0x38)>>3;
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
      active_wram_page=v&0x07;
      if (!active_wram_page) active_wram_page=1;
      WRAM_BANK=active_wram_page;
      return;
    }
  } // end COLOR_GAMEBOY

  // Update sound if necessary
  /*
 if (adr==0xFF24) {
    UINT32 snd_len =(UINT32)((float)get_nb_cycle()*(sample_rate/59.73)
			   /((gbcpu->mode== DOUBLE_SPEED)?140448.0:70224.0));
    
    if (snd_len) 
      update_gb_sound(snd_len<<1);
  
    snd_g.SO1_OutputLevel=v&0x7;
    snd_g.SO2_OutputLevel=(v&0x70)>>4;
    snd_g.Vin_SO1=(v&0x8)>>3;
    snd_g.Vin_SO2=(v&0x80)>>7;
    return;
  }
  */
  if (adr>=0xff10 && adr<=0xff3f && conf.sound) {
    write_sound_reg(adr,v);
    return;
    // printf("Sample rate:%f\n",sample_rate/59.73);
    /*
    snd_len = (UINT32) ((float)get_nb_cycle()*(sample_rate/59.73)/((gbcpu->mode== DOUBLE_SPEED)?140448.0:70224.0));
   
    if (snd_len) 
    
      update_gb_sound(snd_len<<1);
    */
    
  }

  switch(adr) {
  case 0xff00:
    if (v==0x30) GB_PAD=0xff;
    else GB_PAD=v;
    break;
  case 0xff0f:
#ifdef USE_LOG
    put_log("write to INT_FLAG %02x\n",v);
#endif
    INT_FLAG=v&0x1f;break;
  case 0xffff:INT_ENABLE=v&0x1f;break;
  case 0xff04:DIVID=0;break;
  case 0xff07:
    if (v&4) {
      UINT8 a;
      if (gbcpu->mode==SIMPLE_SPEED) a=0;
      else a=1;
      switch(v&3) {
      case 0: timer_clk_inc=1024<<a;break;
      case 1: timer_clk_inc=16<<a;break;
      case 2: timer_clk_inc=64<<a;break;
      case 3: timer_clk_inc=256<<a;break;
      }
    } else timer_clk_inc=0;
    TIME_CONTROL=v;
    break;
  case 0xff40:
    write2lcdccont(v);
    break;
  case 0xff41:LCDCSTAT=(LCDCSTAT&0x07)|(v&0xf8);break;
  case 0xff44:CURLINE=0;break;
  case 0xff45:CMP_LINE=v;break;
  case 0xff46:      // DMA
    do_dma(v);
    break;
  case 0xff47:
    BGPAL=v;
    pal_bck[0]=grey[BGPAL&3];
    pal_bck[1]=grey[(BGPAL>>2)&3];
    pal_bck[2]=grey[(BGPAL>>4)&3];
    pal_bck[3]=grey[(BGPAL>>6)&3];
    break;
  case 0xff48:
    OBJ0PAL=v;
    pal_obj0[0]=grey[OBJ0PAL&3];
    pal_obj0[1]=grey[(OBJ0PAL>>2)&3];
    pal_obj0[2]=grey[(OBJ0PAL>>4)&3];
    pal_obj0[3]=grey[(OBJ0PAL>>6)&3];
    break;
  case 0xff49:
    OBJ1PAL=v;
    pal_obj1[0]=grey[OBJ1PAL&3];
    pal_obj1[1]=grey[(OBJ1PAL>>2)&3];
    pal_obj1[2]=grey[(OBJ1PAL>>4)&3];
    pal_obj1[3]=grey[(OBJ1PAL>>6)&3];
    break;
  case 0xff4d:CPU_SPEED=0x80;break;
  default:himem[adr-0xfea0]=v;break;
  }
}


inline void mem_write(UINT16 adr,UINT8 v) 
{
  UINT8 bk;
  
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
  case 1:if (v==0x0a) ram_enable=1; else ram_enable=0;return;
  case 2:
  case 3:select_rom_page(adr,v);return;
  case 4:
  case 5:
    if (rom_type&MBC1) {
      if (mbc1_mem_mode==MBC1_16_8_MEM_MODE) 
	mbc1_line=v&0x03;
      else active_ram_page=(v)&ram_mask;
    }
    else active_ram_page=(v)&ram_mask;
#ifdef USE_LOG
    put_log2("select RAM page %d\n",active_ram_page);
#endif
    return;
  case 6:
  case 7:
    if (rom_type&MBC1) {
      if (!v) mbc1_mem_mode=MBC1_16_8_MEM_MODE;
      else if (v==1) mbc1_mem_mode=MBC1_4_32_MEM_MODE;
    }
    return;
  case 8:
  case 9:vram_page[active_vram_page][adr-0x8000]=v;return;
  case 0xa:
  case 0xb:if (ram_enable) ram_page[active_ram_page][adr-0xa000]=v;return;
  case 0xc:wram_page[0][adr-0xc000]=v;return;
  case 0xd:wram_page[active_wram_page][adr-0xd000]=v;return;
  }
}

void update_gb_pad(void) {
//  read_joy(my_joy);
#ifdef LINUX_JOYSTICK
  if ((my_joy->but&0x01) || (key[gb_pad_code[PAD_START]])) gb_pad|=0x08;   // Start
  if ((my_joy->but&0x02) || (key[gb_pad_code[PAD_SELECT]])) gb_pad|=0x04;  // Select
  if ((my_joy->but&0x08) || (key[gb_pad_code[PAD_A]])) gb_pad|=0x01;     // A
  if ((my_joy->but&0x04) || (key[gb_pad_code[PAD_B]])) gb_pad|=0x02; // B

  if ((my_joy->x<0) || (key[gb_pad_code[PAD_LEFT]])) gb_pad|=0x20;
  if ((my_joy->x>0) || (key[gb_pad_code[PAD_RIGHT]])) gb_pad|=0x10;
  if ((my_joy->y<0) || (key[gb_pad_code[PAD_UP]])) gb_pad|=0x40;
  if ((my_joy->y>0) || (key[gb_pad_code[PAD_DOWN]])) gb_pad|=0x80;
#else
  /*x=SDL_JoystickGetAxis(joy,0);
    y=SDL_JoystickGetAxis(joy,1);
    b=SDL_JoystickGetButton(joy,0);
    b|=SDL_JoystickGetButton(joy,1)<<1;
    b|=SDL_JoystickGetButton(joy,2)<<2;
    b|=SDL_JoystickGetButton(joy,3)<<3;*/
  gb_pad=0;
  if ((joy_but&0x01) || (key[gb_pad_code[PAD_START]])) gb_pad|=0x08;   // Start
  if ((joy_but&0x02) || (key[gb_pad_code[PAD_SELECT]])) gb_pad|=0x04;  // Select
  if ((joy_but&0x08) || (key[gb_pad_code[PAD_A]])) gb_pad|=0x01;     // A
  if ((joy_but&0x04) || (key[gb_pad_code[PAD_B]])) gb_pad|=0x02; // B

  if ((joy_axis[0]<0) || (key[gb_pad_code[PAD_LEFT]])) gb_pad|=0x20;
  if ((joy_axis[0]>0) || (key[gb_pad_code[PAD_RIGHT]])) gb_pad|=0x10;
  if ((joy_axis[1]<0) || (key[gb_pad_code[PAD_UP]])) gb_pad|=0x40;
  if ((joy_axis[1]>0) || (key[gb_pad_code[PAD_DOWN]])) gb_pad|=0x80;
#endif
}


inline void update_key(void) {
  SDL_Event event;

#ifdef LINUX_JOYSTICK
  read_joy(my_joy);
#endif

  while(SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_KEYUP:
      key[event.key.keysym.scancode]=0;
      break;
    case SDL_KEYDOWN:
      key[event.key.keysym.scancode]=1;
      switch(event.key.keysym.scancode) {
      case 9: conf.gb_done=1;break; // ESC
      case 67:if (!save_snap()) printf("save snap done\n");break;
      case 68:if (!load_snap()) printf("load snap done\n");break;
      }
      break;
    case SDL_JOYAXISMOTION:
      joy_axis[event.jaxis.axis]=event.jaxis.value;
      break;
    case SDL_JOYBUTTONDOWN:
      joy_but|=(1<<event.jbutton.button);
      break;
    case SDL_JOYBUTTONUP:
      joy_but&=(~(1<<event.jbutton.button));
      break;  
    case SDL_QUIT:
      conf.gb_done=1;
      break;
    default:
      break;
    }
  }
}

