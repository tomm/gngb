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

#include <config.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <errno.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_ZLIB_H
#include <zlib.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "rom.h"
#include "memory.h"
#include "emu.h"
#include "cpu.h"
#include "interrupt.h"
#include "vram.h"
#include "sound.h"
#include "emu.h"
#include "fileio.h"
#include "save.h"

#define FILENAME_LEN 1024

Sint16 rom_type=UNKNOW_TYPE;
Uint8 rom_gb_type=UNKNOW;

int check_dir(char *dir_name) {
#ifdef GNGB_WIN32
  BOOL res;
  
  res = CreateDirectory(dir_name,NULL);
  if(res == 0) {
    return 0;
  } else {
    return 1;
  }
#else
  DIR *d;

  if (!(d=opendir(dir_name)) && (errno==ENOENT)) {
    mkdir(dir_name,0755);
    return 0;
  }
  return 1;
#endif
}

void get_ext_nb(char *r,int n) {
  sprintf(r,".%03d",n);
}

void get_bmp_ext_nb(char *r,int n) {
  sprintf(r,"%03d.bmp",n);
}

void set_gameboy_type(void) {
  if (rom_gb_type&COLOR_GAMEBOY)  // prefer always CGB
    conf.gb_type=COLOR_GAMEBOY;
  else {
    if (rom_gb_type&SUPER_GAMEBOY && !conf.yuv) // prefer SGB if not CGB
      conf.gb_type=SUPER_GAMEBOY;
    else 
      conf.gb_type=NORMAL_GAMEBOY;
  }
}

void check_gameboy_type(void) {
  if (conf.gb_type&COLOR_GAMEBOY && 
      rom_gb_type&NORMAL_GAMEBOY &&
      !(rom_gb_type&COLOR_GAMEBOY)) {
    printf("WARNING:Normal gb game on Color gb dont work for the moment\n");
    printf("Force to normal gb\n");
    conf.gb_type&=(~COLOR_GAMEBOY);
  }
}

int open_rom(char *filename)
{
  Uint8 gb_memory[32768];
  GNGB_FILE * stream;
  int i;

  if (rom_page) free_mem_page(rom_page,nb_rom_page);
  if (ram_page) free_mem_page(ram_page,nb_ram_page);
  if (vram_page) free_mem_page(vram_page,nb_vram_page);
  if (wram_page) free_mem_page(wram_page,nb_wram_page);
  
  rom_page=ram_page=vram_page=wram_page=NULL;
  
  if ((stream=gngb_file_open(filename,"rb",UNKNOW_FILE_TYPE))) {
    if (stream->type==ZIP_ARCH_FILE_TYPE) {
      if (zip_file_open_next_rom(stream->stream)<0) return -1;
    }

    printf("Open file %s\n",filename);
    rom_name=get_name_without_ext(filename);
    printf("Rom Name %s\n",rom_name);
    gngb_file_read(gb_memory,sizeof(char),32768,stream);
    printf("Name    : %.15s\n",gb_memory+0x134);

    if (gb_memory[0x0143]==0x80) {
      rom_gb_type=COLOR_GAMEBOY|NORMAL_GAMEBOY;
      printf("Color GameBoy\n");
      printf("Normal GameBoy Supported\n");
    } else if (gb_memory[0x0143]==0xc0) {
      rom_gb_type=COLOR_GAMEBOY_ONLY;
      printf("Color GameBoy\n");
      printf("Normal GameBoy Not Supported !! \n");
    } else {
      rom_gb_type=NORMAL_GAMEBOY;
      printf("Normal GameBoy\n");
    }

    if (gb_memory[0x0146]) {
      rom_gb_type|=SUPER_GAMEBOY;
      printf("SGB Supported %02x\n",gb_memory[0x0146]);
    }
    
    //if (conf.normal_gb) gameboy_type=NORMAL_GAMEBOY;

    if (conf.gb_type==UNKNOW) set_gameboy_type();
    else check_gameboy_type();

    printf("configuration %02x : ",gb_memory[0x147]);
    switch(gb_memory[0x147]) {
    case 0x00: printf("ROM ONLY\n"); rom_type=ROM_ONLY;break;
    case 0x01: printf("MBC1\n"); rom_type=MBC1;break;
    case 0x02: printf("MBC1+RAM\n"); rom_type=MBC1|RAM;break;
    case 0x03: printf("MBC1+RAM+BATTERY\n"); rom_type=MBC1|RAM|BATTERY;break;
    case 0x05: printf("MBC2\n"); rom_type=MBC2;break;
    case 0x06: printf("MBC2+BATTERY\n"); rom_type=MBC2|BATTERY;break;
    case 0x08: printf("ROM+RAM\n"); rom_type=ROM_ONLY|RAM;break;
    case 0x09: printf("ROM+RAM+BATTERY\n"); rom_type=ROM_ONLY|RAM|BATTERY;break;
    case 0x0b:
    case 0x0c:
    case 0x0d: printf("unknown\n"); rom_type=UNKNOW_TYPE;break;
    case 0x0f: printf("MBC3+TIMER+BATTERY\n");rom_type=MBC3|TIMER|BATTERY;break;
    case 0x10: printf("MBC3+TIMER+RAM+BATTERY\n");rom_type=MBC3|TIMER|BATTERY|RAM;break;
    case 0x11: printf("MBC3\n");rom_type=MBC3;break;
    case 0x12: printf("MBC3+RAM\n"); rom_type=MBC3|RAM;break;
    case 0x13: printf("MBC3+RAM+BATTERY\n"); rom_type=MBC3|RAM|BATTERY;break;
    case 0x19: printf("MBC5\n"); rom_type=MBC5;break;
    case 0x1a: printf("MBC5+RAM\n"); rom_type=MBC5|RAM;break;
    case 0x1b: printf("MBC5+RAM+BATTERY\n"); rom_type=MBC5|RAM|BATTERY;break;
    case 0x1c: printf("MBC5+RUMBLE\n"); rom_type=MBC5|RUMBLE;break;
    case 0x1d: printf("MBC5+RUMBLE+SRAM\n"); rom_type=MBC5|RUMBLE|SRAM;break;
    case 0x1e: printf("MBC5+RUMBLE+SRAM+BATTERY\n"); rom_type=MBC5|RUMBLE|SRAM|BATTERY;break;
    case 0x22: printf("MBC7 (not suported)\n"); rom_type=UNKNOW_TYPE;break;
    case 0xFF: printf(" Hudson HuC-1\n"); /* FIXME: HUC1=MBC1 */rom_type=MBC1;break;
    default: printf("unknown %02x \n",gb_memory[0x147]); rom_type=UNKNOW_TYPE;break;
    }
    printf("ROM size %d : %u * 16 kbyte \n",gb_memory[0x148],1<<gb_memory[0x148]<<1);
      
    /* ROM */
       
    rom_mask=(1<<gb_memory[0x148]<<1)-1;
    rom_page=alloc_mem_page(nb_rom_page=(1<<gb_memory[0x148]<<1),0x4000);
    memcpy(rom_page[0],gb_memory,0x4000);
    memcpy(rom_page[1],gb_memory+0x4000,0x4000);
    active_rom_page=1;

    if (rom_type&MBC1 || rom_type&MBC2 || rom_type&MBC3 || rom_type&MBC5) {
      for(i=2;i<nb_rom_page;i++) 
	gngb_file_read(rom_page[i],sizeof(char),0x4000,stream);
    }

    if (rom_type&TIMER) {
      rom_timer=(ROM_TIMER *)malloc(sizeof(ROM_TIMER));
      rom_timer->cycle=0;
      rom_timer->reg_sel=0;
      rom_timer->latch=0;
      memset(rom_timer->reg,0,sizeof(Uint8)*5);
    } else rom_timer=NULL;
    

    /* RAM */

    if ((rom_type&RAM) || (rom_type&SRAM)) {
      switch(gb_memory[0x149]) {
      case 0:printf("RAM size %02x : 0 ?????\n",gb_memory[0x149]);nb_ram_page=1;ram_mask=0x00;break;
      case 1:printf("RAM size %02x : 2 kbyte\n",gb_memory[0x149]);nb_ram_page=1;ram_mask=0x00;break;
      case 2:printf("RAM size %02x : 8 kbyte\n",gb_memory[0x149]);nb_ram_page=1;ram_mask=0x00;break;
      case 3:printf("RAM size %02x : 32 kbyte\n",gb_memory[0x149]);nb_ram_page=4;ram_mask=0x03;break;
      case 4:printf("RAM size %02x : 128 kbyte\n",gb_memory[0x149]);nb_ram_page=16;ram_mask=0x0f;break;
      }
          
    } else {
      printf("NO RAM\n");
      nb_ram_page=1;
      ram_mask=0x00;
    }
    
    ram_page=alloc_mem_page(nb_ram_page,0x2000);
    active_ram_page=0;	

    if (rom_type&BATTERY) {
      load_ram();
      if (rom_type&TIMER) load_rom_timer();
    }

    /* VRAM & INTERNAL RAM */

    if (conf.gb_type&COLOR_GAMEBOY) {
      nb_vram_page=2;
      nb_wram_page=8;
    } else {
      nb_vram_page=1;
      nb_wram_page=2;
    }

    vram_page=alloc_mem_page(nb_vram_page,0x2000);
    active_vram_page=0;
    wram_page=alloc_mem_page(nb_wram_page,0x1000);
    active_wram_page=1;

    switch(gb_memory[0x014a])  {
    case 0:printf("Country : Japanese \n");break;
    case 1:printf("Country : Non-Japanese \n");break;
    }
    
    gngb_file_close(stream);

    if (rom_type!=UNKNOW_TYPE) return 0;
    else return 1;
  }
  return 1;
}

