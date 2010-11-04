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


#ifndef _MEMORY_H
#define _MEMORY_H

#include "global.h"
#include <SDL.h>

/* mbc1 mem mode type */

#define MBC1_16_8_MEM_MODE 0
#define MBC1_4_32_MEM_MODE 1

extern Uint8 rom_mask;
extern Uint16 nb_rom_page;
extern Uint16 nb_ram_page;
extern Uint16 nb_vram_page;
extern Uint16 nb_wram_page;
extern Uint16 active_rom_page;
extern Uint16 active_ram_page;
extern Uint16 active_vram_page;
extern Uint16 active_wram_page;

extern Uint8 **rom_page;       // 0000 - 4000 bank #0 4000 - 8000 bank #n  
extern Uint8 **ram_page;       // a000 - c000  
extern Uint8 **vram_page;      // 8000 - a000
extern Uint8 **wram_page;      // c000 - fe00 
extern Uint8 oam_space[0xa0];  // fe00 - fea0
extern Uint8 himem[0x160];     // fea0 - ffff

extern Uint8 mbc1_mem_mode;
extern Uint8 mbc1_line;

extern Uint8 mbc5_lower;
extern Uint8 mbc5_upper;

extern Uint8 ram_mask;

// REGISTER

#define GB_PAD (himem[0x60])
#define SB (himem[0x61])
#define SC (himem[0x62])
#define DIVID (himem[0x64])
#define TIME_COUNTER (himem[0x65])
#define TIME_MOD (himem[0x66])
#define TIME_CONTROL (himem[0x67])
#define INT_FLAG (himem[0x6f])
#define INT_ENABLE (himem[0x15f])
#define LCDCCONT (himem[0xa0])
#define LCDCSTAT (himem[0xa1])
#define CURLINE (himem[0xa4])
#define CMP_LINE (himem[0xa5])
#define SCRX (himem[0xa3])
#define SCRY (himem[0xa2])
#define WINX (himem[0xab])
#define WINY (himem[0xaa])
#define DMA (himem[0xa6])
#define BGPAL (himem[0xa7])
#define OBJ0PAL (himem[0xa8])
#define OBJ1PAL (himem[0xa9])

#define NR10 (himem[0x70])
#define NR11 (himem[0x71])
#define NR12 (himem[0x72])
#define NR13 (himem[0x73])
#define NR14 (himem[0x74])

#define NR21 (himem[0x76])
#define NR22 (himem[0x77])
#define NR23 (himem[0x78])
#define NR24 (himem[0x79])

#define NR30 (himem[0x7a])
#define NR31 (himem[0x7b])
#define NR32 (himem[0x7c])
#define NR33 (himem[0x7d])
#define NR34 (himem[0x7e])

#define NR41 (himem[0x80])
#define NR42 (himem[0x81])
#define NR43 (himem[0x82])
#define NR44 (himem[0x83])

#define NR50 (himem[0x84])
#define NR51 (himem[0x85])
#define NR52 (himem[0x86])

// COLOR GAMEBOY 

#define CPU_SPEED (himem[0xad])
#define VRAM_BANK (himem[0xaf])
#define HDMA_CTRL1 (himem[0xb1])
#define HDMA_CTRL2 (himem[0xb2])
#define HDMA_CTRL3 (himem[0xb3])
#define HDMA_CTRL4 (himem[0xb4])
#define HDMA_CTRL5 (himem[0xb5])
#define IR_PORT (himem[0xb6])
#define BGPAL_SPE (himem[0xc8])
#define BGPAL_DATA (himem[0xc9])
#define OBJPAL_SPE (himem[0xca])
#define OBJPAL_DATA (himem[0xcb])
#define WRAM_BANK (himem[0xd0])

Uint8 **alloc_mem_page(Uint16 nb_page,Uint32 size);
void free_mem_page(Uint8 **page,Uint16 nb_page);

void gbmemory_init(void);
void gbmemory_reset(void);

/* Mem Read/Write */

#define MEM_DIRECT_ACCESS 1
#define MEM_FUN_ACCESS 2

typedef struct {
  Uint32 type;
  Uint8 *b;
  Uint8 (*f)(Uint16 adr);
}MEM_READ_ENTRY;

extern MEM_READ_ENTRY mem_read_tab[0x10];

typedef struct {
  Uint32 type;
  Uint8 *b;
  void (*f)(Uint16 adr,Uint8 v);
}MEM_WRITE_ENTRY;

extern MEM_WRITE_ENTRY mem_write_tab[0x10];

Uint8 mem_read_default(Uint16 adr);
void mem_write_default(Uint16 adr,Uint8 v);
Uint8 mem_read_ff(Uint16 adr);
void mem_write_ff(Uint16 adr,Uint8 v);

#define mem_write mem_write_default
#define mem_read mem_read_default

/* To use this macro you don't have to use autoincrementation in argument */
#define mem_read_fast(a,v) {\
    if (mem_read_tab[((a)>>12)&0xff].type!=MEM_DIRECT_ACCESS) {\
       (v)=mem_read_tab[((a)>>12)&0xff].f((a));\
    } else {\
       (v)=mem_read_tab[((a)>>12)&0xff].b[(a)&0xfff];\
    }\
  }

#define mem_write_fast(a,v) {\
    if (mem_write_tab[((a)>>12)&0xff].type!=MEM_DIRECT_ACCESS) {\
       mem_write_tab[((a)>>12)&0xff].f((a),(v));\
    } else {\
       mem_write_tab[((a)>>12)&0xff].b[(a)&0xfff]=(v);\
    }\
  }

void push_stack_word(Uint16 v);

/* DMA Function */

void do_gdma(void);
void do_hdma(void);

#define NO_DMA 0
#define SPRITE_DMA 1
#define HDMA 2
#define GDMA 3

typedef struct {
  Sint32 gdma_cycle;
  Uint16 src,dest;
  Uint16 lg;
  Uint8 type;
}DMA_INFO;

extern DMA_INFO dma_info;

#endif




