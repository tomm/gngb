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
#include <SDL/SDL.h>

/*typedef struct {
  UINT8 autofs; 
  UINT8 fs;     
  UINT8 sound;
  UINT8 normal_gb;
  UINT8 rumble_on;
  UINT8 serial_on;
  UINT8 gb_done;
  UINT8 joy_no;
  UINT8 gl;
  int gl_w,gl_h;
}GNGB_CONF;

GNGB_CONF conf;*/

/* mbc1 mem mode type */

#define MBC1_16_8_MEM_MODE 0
#define MBC1_4_32_MEM_MODE 1

extern UINT8 rom_mask;
extern UINT16 nb_rom_page;
extern UINT16 nb_ram_page;
extern UINT16 nb_vram_page;
extern UINT16 nb_wram_page;
extern UINT16 active_rom_page;
extern UINT16 active_ram_page;
extern UINT16 active_vram_page;
extern UINT16 active_wram_page;

extern UINT8 **rom_page;       // 0000 - 4000 bank #0 4000 - 8000 bank #n  
extern UINT8 **ram_page;       // a000 - c000  
extern UINT8 **vram_page;      // 8000 - a000
extern UINT8 **wram_page;      // c000 - fe00 
extern UINT8 oam_space[0xa0];  // fe00 - fea0
extern UINT8 himem[0x160];     // fea0 - ffff

extern UINT8 mbc1_mem_mode;
extern UINT8 mbc1_line;

extern UINT8 mbc5_lower;
extern UINT8 mbc5_upper;

extern UINT8 ram_mask;

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

UINT8 **alloc_mem_page(UINT16 nb_page,UINT32 size);
void free_mem_page(UINT8 **page,UINT16 nb_page);

void gbmemory_init(void);
void gbmemory_reset(void);

inline UINT8 mem_read(UINT16 adr);
inline void mem_write(UINT16 adr,UINT8 v);
inline UINT8 mem_read_ff(UINT16 adr);
inline void mem_write_ff(UINT16 adr,UINT8 v);

void push_stack_word(UINT16 v);

inline void do_gdma(void);
inline void do_hdma(void);

#define NO_DMA 0
#define SPRITE_DMA 1
#define HDMA 2
#define GDMA 3
#define HDMA_STAND 4
#define GDMA_STAND 5

typedef struct {
  UINT8 type;
  UINT16 src,dest;
  UINT16 lg;
}DMA_INFO;

extern DMA_INFO dma_info;

//UINT8 gb_pad;

#endif




