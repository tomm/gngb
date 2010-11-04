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

char *get_snap_name(char *name) {
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
  
  a[i++]='s';
  a[i++]='n';
  a[i++]='a';
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

int save_snap(void) {
  FILE *stream;
  int i;
  char dir[100];
  char *a=getenv("HOME");
  
  dir[0]=0;
  strcat(dir,a);
  strcat(dir,"/.gngb/");
  check_dir(dir);

  strcat(dir,get_snap_name(rom_name));
  stream=fopen(dir,"wb");
  if (!stream) {
    printf("Error while trying to write file %s \n",dir);
    return 1;
  }

  for(i=0;i<nb_ram_page;i++)
    fwrite(ram_page[i],sizeof(UINT8),0x2000,stream);

  for(i=0;i<nb_vram_page;i++)
    fwrite(vram_page[i],sizeof(UINT8),0x2000,stream);

  for(i=0;i<nb_wram_page;i++)
    fwrite(wram_page[i],sizeof(UINT8),0x1000,stream);

  fwrite(oam_space,sizeof(UINT8),0xa0,stream);
  fwrite(himem,sizeof(UINT8),0x160,stream);

  fwrite(&active_rom_page,sizeof(UINT8),1,stream);
  fwrite(&active_ram_page,sizeof(UINT8),1,stream);
  fwrite(&active_vram_page,sizeof(UINT8),1,stream);
  fwrite(&active_wram_page,sizeof(UINT8),1,stream);
  fwrite(gbcpu,sizeof(GB_CPU),1,stream);

  fclose(stream);
  return 0;
}

int load_snap(void) {
  FILE *stream;
  int i;
  char dir[100];
  char *a=getenv("HOME");
  
  dir[0]=0;
  strcat(dir,a);
  strcat(dir,"/.gngb/");
  check_dir(dir);

  strcat(dir,get_snap_name(rom_name));
  stream=fopen(dir,"rb");
  if (!stream) {
    printf("Error while trying to read file %s \n",dir);
    return 1;
  }

  for(i=0;i<nb_ram_page;i++)
    fread(ram_page[i],sizeof(UINT8),0x2000,stream);

  for(i=0;i<nb_vram_page;i++)
    fread(vram_page[i],sizeof(UINT8),0x2000,stream);

  for(i=0;i<nb_wram_page;i++)
    fread(wram_page[i],sizeof(UINT8),0x1000,stream);

  fread(oam_space,sizeof(UINT8),0xa0,stream);
  fread(himem,sizeof(UINT8),0x160,stream);

  fread(&active_rom_page,sizeof(UINT8),1,stream);
  fread(&active_ram_page,sizeof(UINT8),1,stream);
  fread(&active_vram_page,sizeof(UINT8),1,stream);
  fread(&active_wram_page,sizeof(UINT8),1,stream);
  fread(gbcpu,sizeof(GB_CPU),1,stream);

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
    
    printf("configuration : ");
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
    printf("ROM size : %u * 16 kbyte \n", 1 << gb_memory[0x148] << 1 );
      
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
      printf("ram id %02x\n",gb_memory[0x149]);
      switch(gb_memory[0x149]) {
      case 0:printf("RAM size : 0 ?????\n");nb_ram_page=1;ram_mask=0x00;break;
      case 1:printf("RAM size : 2 kbyte\n");nb_ram_page=1;ram_mask=0x00;break;
      case 2:printf("RAM size : 8 kbyte\n");nb_ram_page=1;ram_mask=0x00;break;
      case 3:printf("RAM size : 32 kbyte\n");nb_ram_page=4;ram_mask=0x03;break;
      case 4:printf("RAM size : 128 kbyte\n");nb_ram_page=16;ram_mask=0x0f;break;
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



