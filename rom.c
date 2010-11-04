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

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include "rom.h"
#include "memory.h"
#include "cpu.h"
#include "interrupt.h"
#include "vram.h"
#include "sound.h"

typedef enum {
  CPU_SECTION,
  LCDC_SECTION,
  PAL_SECTION,
  TIMER_SECTION,
  DMA_SECTION
}SECTION_TYPE;

INT16 rom_type;
UINT8 gameboy_type;

char *get_save_name(char *name) {
  char *a;
  int lg=strlen(name);
  int l,r,i;

  l=lg;
  while(l>0 && name[l-1]!='/') l--;
  r=lg;
  while(r>l && name[r-1]!='.') r--;

  a=(char *)malloc(sizeof(char)*(5+(r-l)));
  i=0;
  while(l<r) 
    a[i++]=name[l++];
  
  a[i++]='s';
  a[i++]='v';
  a[i++]=0;
  return a;
}

int check_dir(char *dir_name) {
  DIR *d;

  if (!(d=opendir(dir_name)) && (errno==ENOENT)) {
    mkdir(dir_name,0755);
    return 0;
  }
  return 1;
}

int load_ram(void) {
  FILE *stream;
  int i;
  char dir[100];
  char *a=getenv("HOME");

  dir[0]=0;
  strcat(dir,a);
  strcat(dir,"/.gngb/");
  check_dir(dir);

  strcat(dir,get_save_name(rom_name));
  stream=fopen(dir,"rb");
  if (!stream) {
    printf("Error while trying to read file %s \n",dir);
    return 1;
  }

  for(i=0;i<nb_ram_page;i++)
    fread(ram_page[i],sizeof(UINT8),0x2000,stream);

  fclose(stream);
  return 0;
}

int save_ram(void) {
  FILE *stream;
  int i;
  char dir[100];
  char *a=getenv("HOME");
  
  dir[0]=0;
  strcat(dir,a);
  strcat(dir,"/.gngb/");
  check_dir(dir);

  strcat(dir,get_save_name(rom_name));
  stream=fopen(dir,"wb");
  if (!stream) {
    printf("Error while trying to write file %s \n",dir);
    return 1;
  }

  for(i=0;i<nb_ram_page;i++)
    fwrite(ram_page[i],sizeof(UINT8),0x2000,stream);

  fclose(stream);
  return 0;
}

int open_rom(char *filename)
{
  UINT8 gb_memory[32768];
  FILE *stream;
  int i;

  if (rom_page) free_mem_page(rom_page,nb_rom_page);
  if (ram_page) free_mem_page(ram_page,nb_ram_page);
  if (vram_page) free_mem_page(vram_page,nb_vram_page);
  if (wram_page) free_mem_page(wram_page,nb_wram_page);
  
  rom_page=ram_page=vram_page=wram_page=NULL;

  if ((stream=fopen(filename,"rb"))) {
    printf("Open file %s\n",filename);
    strcpy(rom_name,filename);
    fread(gb_memory,sizeof(char),32768,stream);
    printf("Name    : %.15s\n",gb_memory+0x134);

    if (gb_memory[0x0143]==0x80) {
      gameboy_type=COLOR_GAMEBOY;
      printf("Color GameBoy\n");
      printf("Normal GameBoy Supported\n");
    } else if (gb_memory[0x0143]==0xc0) {
      gameboy_type=COLOR_GAMEBOY_ONLY;
      printf("Color GameBoy\n");
      printf("Normal GameBoy Not Supported !! \n");
    } else {
      gameboy_type=NORMAL_GAMEBOY;
      printf("Normal GameBoy\n");
    }

    if (gb_memory[0x0146]) {
      gameboy_type|=SUPER_GAMEBOY;
      printf("SGB\n");
    }
    
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
	fread(rom_page[i],sizeof(char),0x4000,stream);
    }

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

    if (rom_type&BATTERY) load_ram();

    /* VRAM & INTERNAL RAM */

    if (gameboy_type&COLOR_GAMEBOY) {
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
    fclose(stream);

    if ((rom_type&ROM_ONLY ||
	 rom_type&MBC1 ||
	 rom_type&MBC2 ||
	 rom_type&MBC3 ||
	 rom_type&MBC5) && rom_type!=UNKNOW_TYPE)
      return 0;
    else return 1;
  }
  return 1;
}


/* SAVE/LOAD State ( Experimental ) */

char *get_snap_name(char *name,int n) {
  char *a;
  int lg=strlen(name);
  int l,r,i;

  l=lg;
  while(l>0 && name[l-1]!='/') l--;
  r=lg;
  while(r>l && name[r-1]!='.') r--;

  a=(char *)malloc(sizeof(char)*(6+(r-l)));
  i=0;
  while(l<r) 
    a[i++]=name[l++];
  
  a[i++]='0';a[i++]='0';
  switch(n) {
  case 0:a[i++]='0';break;
  case 1:a[i++]='1';break;
  case 2:a[i++]='2';break;
  case 3:a[i++]='3';break;
  case 4:a[i++]='4';break;
  case 5:a[i++]='5';break;
  case 6:a[i++]='6';break;
  case 7:a[i++]='7';break;
  case 8:a[i++]='8';break;
  }

  a[i++]=0;
  return a;
}

/* For keep a compatibility beetwen different version we save the 
   state of the gameboy by section.
   In this version we have 3 section:
        CPU_SECTION
	LCDC_SECTION
	PAL_SECTION 
*/   

void write_cpu_info(FILE *stream) {
  /* CPU INFO is
     - REGISTER: 6 * UINT16         (id=1 af,bc,de,hl,sp,pc)
     - IME,ie_flag,STATE,MODE: 3 * UINT8    (id=2) 
     size = 6*2+4+1+1
  */
  
  UINT8 t=CPU_SECTION;
  UINT16 size=18;

  // SECTION + size

  fwrite(&t,sizeof(UINT8),1,stream);
  fwrite(&size,sizeof(UINT16),1,stream);

  // REGISTER

  t=1;
  fwrite(&t,sizeof(UINT8),1,stream);
  fwrite(&gbcpu->af.w,sizeof(UINT16),1,stream);
  fwrite(&gbcpu->bc.w,sizeof(UINT16),1,stream);
  fwrite(&gbcpu->de.w,sizeof(UINT16),1,stream);
  fwrite(&gbcpu->hl.w,sizeof(UINT16),1,stream);
  fwrite(&gbcpu->sp.w,sizeof(UINT16),1,stream);
  fwrite(&gbcpu->pc.w,sizeof(UINT16),1,stream);

  // IME,ie_flag,STATE,MODE
  
  t=2;
  fwrite(&t,sizeof(UINT8),1,stream);
  fwrite(&gbcpu->int_flag,sizeof(UINT8),1,stream);
  fwrite(&gbcpu->ei_flag,sizeof(UINT8),1,stream);
  fwrite(&gbcpu->state,sizeof(UINT8),1,stream);
  fwrite(&gbcpu->mode,sizeof(UINT8),1,stream);
  
}

int read_cpu_info(FILE *stream,int size) {
  /* CPU INFO is defined by id
     - id=1 => REGISTER: 6 * UINT16  (af,bc,de,hl,sp,pc)
     - id=2 => IME,ie_flag,STATE,MODE: 4 * UINT8         
  */
  
  long end=ftell(stream)+size;
  UINT8 t;
  
  printf("read cpu info\n");

  while(ftell(stream)!=end) {
    fread(&t,sizeof(UINT8),1,stream);
    switch(t) {
    case 1:
      fread(&gbcpu->af.w,sizeof(UINT16),1,stream);
      fread(&gbcpu->bc.w,sizeof(UINT16),1,stream);
      fread(&gbcpu->de.w,sizeof(UINT16),1,stream);
      fread(&gbcpu->hl.w,sizeof(UINT16),1,stream);
      fread(&gbcpu->sp.w,sizeof(UINT16),1,stream);
      fread(&gbcpu->pc.w,sizeof(UINT16),1,stream);
      break;
    case 2:
      fread(&gbcpu->int_flag,sizeof(UINT8),1,stream);
      fread(&gbcpu->ei_flag,sizeof(UINT8),1,stream);
      fread(&gbcpu->state,sizeof(UINT8),1,stream);
      fread(&gbcpu->mode,sizeof(UINT8),1,stream);
      break;
    default:return -1;
    }
  }
  return 0;
}

void write_lcdc_info(FILE *stream) {
  /* LCDC INFO is defined by id
     - id=1 => MODE: 1 * UINT8
     - id=2 => CYCLE_TODO: 1 * UINT16
     - id=3 => VBLANK_CYCLE*MODE1CYCLE*MODE2CYCLE: UINT32, 2*UINT16
     size = 1+2+4+2*2+3
  */

  UINT8 t=LCDC_SECTION;
  UINT16 size=14;

  // SECTION + size

  fwrite(&t,sizeof(UINT8),1,stream);
  fwrite(&size,sizeof(UINT16),1,stream);
     
  // MODE

  t=1;
  fwrite(&t,sizeof(UINT8),1,stream);
  fwrite(&gblcdc->mode,sizeof(UINT8),1,stream);

  // CYCLE_TODO

  t=2;
  fwrite(&t,sizeof(UINT8),1,stream);
  fwrite(&gblcdc->cycle,sizeof(UINT16),1,stream);

  // VBLANK_CYCLE*MODE1CYCLE*MODE2CYCLE:

  t=3;
  fwrite(&t,sizeof(UINT8),1,stream);
  fwrite(&gblcdc->vblank_cycle,sizeof(UINT32),1,stream);
  fwrite(&gblcdc->mode1cycle,sizeof(UINT16),1,stream);
  fwrite(&gblcdc->mode2cycle,sizeof(UINT16),1,stream);
  
}

int read_lcdc_info(FILE *stream,int size) {
  /* LCDC INFO is defined by id
     - id=1 => MODE: 1 * UINT8
     - id=2 => CYCLE_TODO: 1 * UINT16
     - id=3 => VBLANK_CYCLE*MODE1CYCLE*MODE2CYCLE: UINT32, 2*UINT16
  */

  long end=ftell(stream)+size;
  UINT8 t;
  
  printf("read lcdc info\n");

  while(ftell(stream)!=end) {
    fread(&t,sizeof(UINT8),1,stream);
    switch(t) {
    case 1:
      fread(&gblcdc->mode,sizeof(UINT8),1,stream);
      break;
    case 2:
      fread(&gblcdc->cycle,sizeof(UINT16),1,stream);
      break;
    case 3:
      fread(&gblcdc->vblank_cycle,sizeof(UINT32),1,stream);
      fread(&gblcdc->mode1cycle,sizeof(UINT16),1,stream);
      fread(&gblcdc->mode2cycle,sizeof(UINT16),1,stream);
      break;
    default:return -1;
    }
  }
  return 0; 
}

void write_pal_info(FILE *stream) {
  /* PAL INFO is simply defined by id
     - id=1 => all palette
     size = 512+96
   */
  
  UINT8 t=PAL_SECTION;
  UINT16 size=512+96+1;

  // SECTION + size
  
  fwrite(&t,sizeof(UINT8),1,stream);
  fwrite(&size,sizeof(UINT16),1,stream);

  // WRITE ALL PALETTE

  t=1;
  fwrite(&t,sizeof(UINT8),1,stream);

  fwrite(pal_col_bck_gb,sizeof(UINT32),8*4,stream);
  fwrite(pal_col_obj_gb,sizeof(UINT32),8*4,stream);
  fwrite(pal_col_bck,sizeof(UINT32),8*4,stream);
  fwrite(pal_col_obj,sizeof(UINT32),8*4,stream);
  fwrite(pal_bck,sizeof(UINT32),8,stream);
  fwrite(pal_obj0,sizeof(UINT32),8,stream);
  fwrite(pal_obj1,sizeof(UINT32),8,stream);

}

int read_pal_info(FILE *stream,int size) {
  /* PAL INFO is simply defined by id
     - id=1 => all palette
  */

  long end=ftell(stream)+size;
  UINT8 t;
  
  printf("read pal info\n");

  while(ftell(stream)!=end) {
    fread(&t,sizeof(UINT8),1,stream);
    switch(t) {
    case 1:
      fread(pal_col_bck_gb,sizeof(UINT32),8*4,stream);
      fread(pal_col_obj_gb,sizeof(UINT32),8*4,stream);
      fread(pal_col_bck,sizeof(UINT32),8*4,stream);
      fread(pal_col_obj,sizeof(UINT32),8*4,stream);
      fread(pal_bck,sizeof(UINT32),8,stream);
      fread(pal_obj0,sizeof(UINT32),8,stream);
      fread(pal_obj1,sizeof(UINT32),8,stream);
      break;
    default:return -1;
    }
  }
  return 0;
}

void write_timer_info(FILE *stream) {
  /* TIMER INFO is simply defined by id
     - id=1 => clk_inc 1*UINT16
     - id=2 => cycle 1*UINT32
     size = 8
   */

  UINT8 t=TIMER_SECTION;
  UINT16 size=8;

  // SECTION + size
  
  fwrite(&t,sizeof(UINT8),1,stream);
  fwrite(&size,sizeof(UINT16),1,stream);
  
  // clk_inc

  t=1;
  fwrite(&t,sizeof(UINT8),1,stream);
  fwrite(&gbtimer->clk_inc,sizeof(UINT16),1,stream);

  // cycle

  t=2;
  fwrite(&t,sizeof(UINT8),1,stream);
  fwrite(&gbtimer->cycle,sizeof(UINT32),1,stream);
}

int read_timer_info(FILE *stream,int size) {
  /* TIMER INFO is simply defined by id
     - id=1 => clk_inc 1*UINT16
     - id=2 => cycle 1*UINT32
  */

  long end=ftell(stream)+size;
  UINT8 t;
  
  printf("read timer info\n");

  while(ftell(stream)!=end) {
    fread(&t,sizeof(UINT8),1,stream);
    switch(t) {
    case 1:
      fread(&gbtimer->clk_inc,sizeof(UINT16),1,stream);
      break;
    case 2:
      fread(&gbtimer->cycle,sizeof(UINT32),1,stream);
      break;
    default:return -1;
    }
  }
 return 0;
}

void write_dma_info(FILE *stream) {
  /* DMA INFO is simply defined by id
     - id=1 => type  1*UINT8
     - id=2 => src,dest,lg 3*UINT16
     size = 9
   */

  UINT8 t=DMA_SECTION;
  UINT16 size=9;

  // SECTION + size
  
  fwrite(&t,sizeof(UINT8),1,stream);
  fwrite(&size,sizeof(UINT16),1,stream);

  // type

  t=1;
  fwrite(&t,sizeof(UINT8),1,stream);
  fwrite(&dma_info.type,sizeof(UINT8),1,stream);

  // src,dest,lg
  
  t=2;
  fwrite(&t,sizeof(UINT8),1,stream);
  fwrite(&dma_info.src,sizeof(UINT16),1,stream);
  fwrite(&dma_info.dest,sizeof(UINT16),1,stream);
  fwrite(&dma_info.lg,sizeof(UINT16),1,stream);

}

int read_dma_info(FILE *stream,int size) {
  /* DMA INFO is simply defined by id
     - id=1 => type  1*UINT8
     - id=2 => src,dest,lg 3*UINT16
     
  */

  long end=ftell(stream)+size;
  UINT8 t;
  
  printf("read dma info\n");

  while(ftell(stream)!=end) {
    fread(&t,sizeof(UINT8),1,stream);
    switch(t) {
    case 1:
      fread(&dma_info.type,sizeof(UINT8),1,stream);
      break;
    case 2:
      fread(&dma_info.src,sizeof(UINT16),1,stream);
      fread(&dma_info.dest,sizeof(UINT16),1,stream);
      fread(&dma_info.lg,sizeof(UINT16),1,stream);
      break;
    default:return -1;
    }
  }
  return 0;
}

int save_state(int n) {
  FILE *stream;
  int i;
  char dir[100];
  char *a=getenv("HOME");
  
  dir[0]=0;
  strcat(dir,a);
  strcat(dir,"/.gngb/");
  check_dir(dir);

  strcat(dir,get_snap_name(rom_name,n));
  stream=fopen(dir,"wb");
  if (!stream) {
    printf("Error while trying to write file %s \n",dir);
    return 1;
  }

  /* We write all the bank and the active page */

  for(i=0;i<nb_ram_page;i++)
    fwrite(ram_page[i],sizeof(UINT8),0x2000,stream);

  for(i=0;i<nb_vram_page;i++)
    fwrite(vram_page[i],sizeof(UINT8),0x2000,stream);

  for(i=0;i<nb_wram_page;i++)
    fwrite(wram_page[i],sizeof(UINT8),0x1000,stream);

  fwrite(oam_space,sizeof(UINT8),0xa0,stream);
  fwrite(himem,sizeof(UINT8),0x160,stream);

  fwrite(&active_rom_page,sizeof(UINT16),1,stream);
  fwrite(&active_ram_page,sizeof(UINT16),1,stream);
  fwrite(&active_vram_page,sizeof(UINT16),1,stream);
  fwrite(&active_wram_page,sizeof(UINT16),1,stream);

  /* now we write a couple of (section id,size) */
  
  write_cpu_info(stream);
  write_lcdc_info(stream);
  write_pal_info(stream);
  write_timer_info(stream);
  write_dma_info(stream);

  fclose(stream);
  return 0;
}

int load_state(int n) {
  FILE *stream;
  int i;
  char dir[100];
  char *a=getenv("HOME");
  UINT8 section_id;
  UINT16 size;
  long end;

  dir[0]=0;
  strcat(dir,a);
  strcat(dir,"/.gngb/");
  check_dir(dir);

  strcat(dir,get_snap_name(rom_name,n));
  stream=fopen(dir,"rb");
  if (!stream) {
    printf("Error while trying to read file %s \n",dir);
    return -1;
  }

  fseek(stream,0,SEEK_END);
  end=ftell(stream);
  fseek(stream,0,SEEK_SET);

  /* we read all the bank and the active page */

  printf("read page \n");

  for(i=0;i<nb_ram_page;i++)
    fread(ram_page[i],sizeof(UINT8),0x2000,stream);
  
  for(i=0;i<nb_vram_page;i++)
    fread(vram_page[i],sizeof(UINT8),0x2000,stream);
  
  for(i=0;i<nb_wram_page;i++)
    fread(wram_page[i],sizeof(UINT8),0x1000,stream);

  fread(oam_space,sizeof(UINT8),0xa0,stream);
  fread(himem,sizeof(UINT8),0x160,stream);

  fread(&active_rom_page,sizeof(UINT16),1,stream);
  fread(&active_ram_page,sizeof(UINT16),1,stream);
  fread(&active_vram_page,sizeof(UINT16),1,stream);
  fread(&active_wram_page,sizeof(UINT16),1,stream);

  /* now we read a couple of (section id,size) */

  while(ftell(stream)!=end) {

    fread(&section_id,sizeof(UINT8),1,stream);
    fread(&size,sizeof(UINT16),1,stream);
    //    printf("section %d size %d \n",section_id,size);
    switch(section_id) {
    case CPU_SECTION:
      if (read_cpu_info(stream,size)<0) goto read_error;
      break;
    case LCDC_SECTION:
      if (read_lcdc_info(stream,size)<0) goto read_error;
      break;
    case PAL_SECTION:
      if (read_pal_info(stream,size)<0) goto read_error;
      break;
    case TIMER_SECTION:
      if (read_timer_info(stream,size)<0) goto read_error;
      break;
    case DMA_SECTION:
      if (read_dma_info(stream,size)<0) goto read_error;
      break;
    }
  }
  
  fclose(stream);
  if (conf.sound) 
    update_sound_reg();
  return 0;

 read_error:  
  fclose(stream);
  printf("Error while reading file %s \n",dir);
  return -1;
}



