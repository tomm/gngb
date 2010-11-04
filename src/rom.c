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

#define FILENAME_LEN 1024

typedef enum {
  CPU_SECTION,
  LCD_SECTION,
  PAL_SECTION,
  TIMER_SECTION,
  DMA_SECTION,
  RT_SECTION,
}SECTION_TYPE;

// Save & Load function

int save_load_cpu_reg(FILE *stream,char op);
int save_load_cpu_flag(FILE *stream,char op);
int save_load_lcd_cycles(FILE *stream,char op);
int save_load_lcd_info(FILE *stream,char op);
int save_load_pal(FILE *stream,char op);
int save_load_timer(FILE *stream,char op);
int save_load_dma(FILE *stream,char op);
int save_load_rt_reg(FILE *stream,char op);
int save_load_rt_info(FILE *stream,char op);

struct sub_section  {
  Sint8 id;
  Uint16 size;
  int (*save_load_function)(FILE *stream,char op);
}; 

struct sub_section cpu_sub_section[]={
  {0,6*2,save_load_cpu_reg},
  {1,4,save_load_cpu_flag},
  {-1,-1,NULL}};

struct sub_section lcd_sub_section[]={
  {0,3*2+4+1,save_load_lcd_cycles},
  {1,3,save_load_lcd_info},
  {-1,-1,NULL}};

struct sub_section pal_sub_section[]={
  {0,4*2*8*4+2*4+4,save_load_pal},
  {-1,-1,NULL}};

struct sub_section timer_sub_section[]={
  {0,4+2,save_load_timer},
  {-1,-1,NULL}};

struct sub_section dma_sub_section[]={
  {0,1+3*2+4,save_load_dma},
  {-1,-1,NULL}};

struct sub_section rt_sub_section[]={
  {0,5,save_load_rt_reg},
  {1,4,save_load_rt_info},
  {-1,-1,NULL}};

struct section {
  Sint8 id;
  struct sub_section *ss;
} tab_section[]={
  {CPU_SECTION,cpu_sub_section},
  {LCD_SECTION,lcd_sub_section},
  {PAL_SECTION,pal_sub_section},
  {TIMER_SECTION,timer_sub_section},
  {DMA_SECTION,dma_sub_section},
  {RT_SECTION,rt_sub_section},
  {-1,NULL}};    

Sint16 rom_type=UNKNOW_TYPE;
Uint8 rom_gb_type=UNKNOW;
static SDL_Surface *savestate_bmp=NULL;


char *get_name_without_ext(char *name) {
  char *a;
  int lg=strlen(name);
  int l,r,i;

  l=lg;
#ifndef GNGB_WIN32
  while(l>0 && name[l-1]!='/') l--;
#else
  while(l>0 && (name[l-1]!='\\')) l--;
#endif

  r=lg;
  while(r>l && name[r-1]!='.') r--;

  a=(char *)malloc(sizeof(char)*(5+(r-l)));
  i=0;
  while(l<r) 
    a[i++]=name[l++];
  
  a[i-1]=0;
  return a;
}

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

void get_filename_ext(char *f,char *ext) {
  /*  char *a=getenv("HOME");*/
  char buf[8];
  char *a=getenv("HOME");
  if(a==NULL) {
    buf[0] = '.';
    buf[1] = 0;
    a = buf;
  }

  f[0]=0;
  strcat(f,a);
  strcat(f,"/.gngb/");
  check_dir(f);
  strcat(f,rom_name);
  strcat(f,ext);
}

void get_ext_nb(char *r,int n) {
  sprintf(r,".%03d",n);
}

void get_bmp_ext_nb(char *r,int n) {
  sprintf(r,"%03d.bmp",n);
}


int load_ram(void) {
  FILE *stream;
  int i;
  char filename[FILENAME_LEN];

  get_filename_ext(filename,".sv");
  
  if (!(stream=fopen(filename,"rb"))) {
    fprintf(stderr,"Error while trying to read file %s \n",filename);
    return 1;
  }
  
  for(i=0;i<nb_ram_page;i++)
    fread(ram_page[i],sizeof(Uint8),0x2000,stream);

  fclose(stream);
  return 0;
}

int save_ram(void) {
  FILE *stream;
  int i;
  char filename[FILENAME_LEN];

 
  get_filename_ext(filename,".sv");
 
  if (!(stream=fopen(filename,"wb"))) {
    fprintf(stderr,"Error while trying to write file %s \n",filename);
    return 1;
  }

  for(i=0;i<nb_ram_page;i++)
    fwrite(ram_page[i],sizeof(Uint8),0x2000,stream);

  fclose(stream);
  return 0;
}

int save_rom_timer(void) {
  char filename[FILENAME_LEN];
  FILE *stream;
  int i;
  
  get_filename_ext(filename,".rt");

  if (!(stream=fopen(filename,"wt"))) {
    fprintf(stderr,"Error while trying to write file %s \n",filename);
    return 1;
  }
  
  for(i=0;i<5;i++) 
    fprintf(stream,"%02x ",rom_timer->reg[i]);

  fprintf(stream,"%ld",(long)time(NULL));

  fclose(stream);
  return 0;
}

int load_rom_timer(void) {
  char filename[FILENAME_LEN];
  FILE *stream;
  int i;
  long dt;

  get_filename_ext(filename,".rt");

  if (!(stream=fopen(filename,"rt"))) {
    fprintf(stderr,"Error while trying to read file %s \n",filename);
    return 1;
  }

  /* FIXME : must adjust the time still last time */
  for(i=0;i<5;i++) {
    int t;
    fscanf(stream,"%02x",&t);
    rom_timer->reg[i]=(Uint8)t;
  }
  fscanf(stream,"%ld",&dt);

  {
    int h,m,s,d,dd;
    
    dt=time(NULL)-dt;

    s=(dt%60);
    dt/=60;
    
    rom_timer->reg[0]+=s;
    if (rom_timer->reg[0]>=60) {
      rom_timer->reg[0]-=60;
      rom_timer->reg[1]++;
    }
 
    m=(dt%60);
    dt/=60;
    
    rom_timer->reg[1]+=m;
    if (rom_timer->reg[1]>=60) {
      rom_timer->reg[1]-=60;
      rom_timer->reg[2]++;
    }
  
    h=(dt%24);
    dt/=24;

    rom_timer->reg[2]+=h;
    if (rom_timer->reg[2]>=24) {
      rom_timer->reg[2]-=24;
      rom_timer->reg[3]++;
      if (!rom_timer->reg[3])
	dt++;
    }
 
    d=dt;
    dd=((rom_timer->reg[5]&0x01)<<9)|rom_timer->reg[4];
    
    dd+=dt;
    if (dd>=365) {
      dd=0;
      rom_timer->reg[5]|=0x80;        // set carry
    }

    rom_timer->reg[4]=dd&0xff;
    rom_timer->reg[5]=(rom_timer->reg[5]&0xfe)|((dd&0x100)>>9);

  }

  fclose(stream);
  return 0;
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
  FILE *stream;
  int i;

  if (rom_page) free_mem_page(rom_page,nb_rom_page);
  if (ram_page) free_mem_page(ram_page,nb_ram_page);
  if (vram_page) free_mem_page(vram_page,nb_vram_page);
  if (wram_page) free_mem_page(wram_page,nb_wram_page);
  
  rom_page=ram_page=vram_page=wram_page=NULL;

  if ((stream=fopen(filename,"rb"))) {
    printf("Open file %s\n",filename);
    rom_name=get_name_without_ext(filename);
    fread(gb_memory,sizeof(char),32768,stream);
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
	fread(rom_page[i],sizeof(char),0x4000,stream);
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
    fclose(stream);

    if (rom_type!=UNKNOW_TYPE) return 0;
    else return 1;
  }
  return 1;
}


/* SAVE/LOAD State ( Experimental ) */
#if 0
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
  
  if (n==-1) {
    a[i++]='t';
    a[i++]='m';
    a[i++]='p';
    a[i++]=0;
    return a;
  }

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
#endif

/************ NEW SAVE AND LOAD STATE ******************/

int save_load_cpu_reg(FILE *stream,char op) {
  if (!op) { // write
    fwrite(&gbcpu->af.w,sizeof(Uint16),1,stream);
    fwrite(&gbcpu->bc.w,sizeof(Uint16),1,stream);
    fwrite(&gbcpu->de.w,sizeof(Uint16),1,stream);
    fwrite(&gbcpu->hl.w,sizeof(Uint16),1,stream);
    fwrite(&gbcpu->sp.w,sizeof(Uint16),1,stream);
    fwrite(&gbcpu->pc.w,sizeof(Uint16),1,stream);
  } else { // read
    //printf("read Cpu1\n");
    fread(&gbcpu->af.w,sizeof(Uint16),1,stream);
    fread(&gbcpu->bc.w,sizeof(Uint16),1,stream);
    fread(&gbcpu->de.w,sizeof(Uint16),1,stream);
    fread(&gbcpu->hl.w,sizeof(Uint16),1,stream);
    fread(&gbcpu->sp.w,sizeof(Uint16),1,stream);
    fread(&gbcpu->pc.w,sizeof(Uint16),1,stream);
  }
  return 0;
}

int save_load_cpu_flag(FILE *stream,char op) {
  if (!op) { // write
    fwrite(&gbcpu->int_flag,sizeof(Uint8),1,stream);
    fwrite(&gbcpu->ei_flag,sizeof(Uint8),1,stream);
    fwrite(&gbcpu->state,sizeof(Uint8),1,stream);
    fwrite(&gbcpu->mode,sizeof(Uint8),1,stream);
  } else { // read
    //printf("read Cpu2\n");
    fread(&gbcpu->int_flag,sizeof(Uint8),1,stream);
    fread(&gbcpu->ei_flag,sizeof(Uint8),1,stream);
    fread(&gbcpu->state,sizeof(Uint8),1,stream);
    fread(&gbcpu->mode,sizeof(Uint8),1,stream);
  }
  return 0;
}

int save_load_lcd_cycles(FILE *stream,char op) {
  if (!op) { // write 
    fwrite(&gblcdc->cycle,sizeof(Sint16),1,stream);
    fwrite(&gblcdc->mode1cycle,sizeof(Uint16),1,stream);
    fwrite(&gblcdc->mode2cycle,sizeof(Uint16),1,stream);
    fwrite(&gblcdc->vblank_cycle,sizeof(Uint32),1,stream);
    fwrite(&gblcdc->timing,sizeof(Uint8),1,stream);
  } else { // read 
    //printf("read lcd1\n");
    fread(&gblcdc->cycle,sizeof(Sint16),1,stream);
    fread(&gblcdc->mode1cycle,sizeof(Uint16),1,stream);
    fread(&gblcdc->mode2cycle,sizeof(Uint16),1,stream);
    fread(&gblcdc->vblank_cycle,sizeof(Uint32),1,stream);
    fread(&gblcdc->timing,sizeof(Uint8),1,stream);
  }
  return 0;
}

int save_load_lcd_info(FILE *stream,char op) {
  if (!op) { // write 
    fwrite(&gblcdc->mode,sizeof(Uint8),1,stream);
    fwrite(&gblcdc->nb_spr,sizeof(Uint8),1,stream);
    fwrite(&gblcdc->inc_line,sizeof(Uint8),1,stream);
  } else { // read
    //printf("read lcd2\n");
    fread(&gblcdc->mode,sizeof(Uint8),1,stream);
    fread(&gblcdc->nb_spr,sizeof(Uint8),1,stream);
    fread(&gblcdc->inc_line,sizeof(Uint8),1,stream);
  }
  return 0;
}

int save_load_pal(FILE *stream,char op) {
  if (!op) { // write 
    fwrite(pal_col_bck_gb,sizeof(Uint16),8*4,stream);
    fwrite(pal_col_obj_gb,sizeof(Uint16),8*4,stream);
    fwrite(pal_col_bck,sizeof(Uint16),8*4,stream);
    fwrite(pal_col_obj,sizeof(Uint16),8*4,stream);
    fwrite(pal_bck,sizeof(Uint8),4,stream);
    fwrite(pal_obj,sizeof(Uint8),2*4,stream);
  } else { // read
    //printf("read Pal\n");
    fread(pal_col_bck_gb,sizeof(Uint16),8*4,stream);
    fread(pal_col_obj_gb,sizeof(Uint16),8*4,stream);
    fread(pal_col_bck,sizeof(Uint16),8*4,stream);
    fread(pal_col_obj,sizeof(Uint16),8*4,stream);
    fread(pal_bck,sizeof(Uint8),4,stream);
    fread(pal_obj,sizeof(Uint8),2*4,stream);
  }
  return 0;
}

int save_load_timer(FILE *stream,char op) {
  if (!op) { // write 
    fwrite(&gbtimer->clk_inc,sizeof(Uint16),1,stream);
    fwrite(&gbtimer->cycle,sizeof(Uint32),1,stream);
  } else { // read
    //printf("read timer\n");
    fread(&gbtimer->clk_inc,sizeof(Uint16),1,stream);
    fread(&gbtimer->cycle,sizeof(Uint32),1,stream);
  }
  return 0;
}

int save_load_dma(FILE *stream,char op) {
  if (!op) { // write 
    fwrite(&dma_info.type,sizeof(Uint8),1,stream);
    fwrite(&dma_info.src,sizeof(Uint16),1,stream);
    fwrite(&dma_info.dest,sizeof(Uint16),1,stream);
    fwrite(&dma_info.lg,sizeof(Uint16),1,stream);
    fwrite(&dma_info.gdma_cycle,sizeof(Uint32),1,stream);
  } else { // read
    //printf("read DMA\n");
    fread(&dma_info.type,sizeof(Uint8),1,stream);
    fread(&dma_info.src,sizeof(Uint16),1,stream);
    fread(&dma_info.dest,sizeof(Uint16),1,stream);
    fread(&dma_info.lg,sizeof(Uint16),1,stream);
    fread(&dma_info.gdma_cycle,sizeof(Uint32),1,stream);
  }
  return 0;
}

int save_load_rt_reg(FILE *stream,char op) {
  int t;
  if (!op) { // write 
    for(t=0;t<5;t++)
      fwrite(&rom_timer->reg[t],sizeof(Uint8),1,stream);
  } else { // read
    for(t=0;t<5;t++)
      fread(&rom_timer->reg[t],sizeof(Uint8),1,stream);
  }
  return 0;
}

int save_load_rt_info(FILE *stream,char op) {
  if (!op) { // write 
    fwrite(&rom_timer->cycle,sizeof(Uint16),1,stream);
    fwrite(&rom_timer->reg_sel,sizeof(Uint8),1,stream);
    fwrite(&rom_timer->latch,sizeof(Uint8),1,stream);
  } else { // read
    fread(&rom_timer->cycle,sizeof(Uint16),1,stream);
    fread(&rom_timer->reg_sel,sizeof(Uint8),1,stream);
    fread(&rom_timer->latch,sizeof(Uint8),1,stream);
  }
  return 0;
}

int load_section(FILE *stream,struct section *s,Uint16 size) {
  long end=ftell(stream)+size;
  Uint8 t;
  int m;

  /* m=how many sub section? */
  for(m=0;s->ss[m].id!=-1;m++);
  while(ftell(stream)<end) {
    fread(&t,sizeof(Uint8),1,stream);
    if (t<m) s->ss[t].save_load_function(stream,1);
    else break;
  }
  if (ftell(stream)!=end) return -1;
  return 0;
}

int save_section(FILE *stream,struct section *s) {
  Uint16 size=0;
  int i;

  for(i=0;s->ss[i].id!=-1;i++,size++) {
    size+=s->ss[i].size;
  }
 
  fwrite(&s->id,sizeof(Sint8),1,stream);
  fwrite(&size,sizeof(Uint16),1,stream);

  for(i=0;s->ss[i].id!=-1;i++/*,size++*/) {
    fwrite(&s->ss[i].id,sizeof(Uint8),1,stream);
    s->ss[i].save_load_function(stream,0);
  }  
  return 0;
}

SDL_Surface* get_surface_of_save_state(int n)
{
  char filename[FILENAME_LEN];
  char t[8];

  if (savestate_bmp!=NULL)
    SDL_FreeSurface(savestate_bmp);

  get_bmp_ext_nb(t,n);
  get_filename_ext(filename,t);

  savestate_bmp=SDL_LoadBMP(filename);
  return savestate_bmp;
}

int save_state(int n) {
  FILE *stream;
  int i;
  char filename[FILENAME_LEN];
  char t[8];


  get_ext_nb(t,n);
  get_filename_ext(filename,t);
  if (!(stream=fopen(filename,"wb"))) {
    fprintf(stderr,"Error while trying to write file %s \n",filename);
    return 1;
  }

  // We write all the bank and the active page 

  for(i=0;i<nb_ram_page;i++)
    fwrite(ram_page[i],sizeof(Uint8),0x2000,stream);

  for(i=0;i<nb_vram_page;i++)
    fwrite(vram_page[i],sizeof(Uint8),0x2000,stream);

  for(i=0;i<nb_wram_page;i++)
    fwrite(wram_page[i],sizeof(Uint8),0x1000,stream);

  fwrite(oam_space,sizeof(Uint8),0xa0,stream);
  fwrite(himem,sizeof(Uint8),0x160,stream);

  fwrite(&active_rom_page,sizeof(Uint16),1,stream);
  fwrite(&active_ram_page,sizeof(Uint16),1,stream);
  fwrite(&active_vram_page,sizeof(Uint16),1,stream);
  fwrite(&active_wram_page,sizeof(Uint16),1,stream);

  // now we write a couple of (section id,size) 

  for(i=0;tab_section[i].id!=-1;i++)
    if (tab_section[i].id!=RT_SECTION) save_section(stream,&tab_section[i]);

  //if (rom_type&TIMER) write_rt_info(stream);

  if (rom_type&TIMER) save_section(stream,&tab_section[RT_SECTION]);
    
  fclose(stream);
  
  filename[0]=0;
  get_bmp_ext_nb(t,n);
  get_filename_ext(filename,t);
  SDL_SaveBMP(get_mini_screenshot(),filename);

  return 0;
}

int load_state(int n) {
  FILE *stream;
  int i;
  char filename[FILENAME_LEN];
  char t[5];
  Uint8 section_id;
  Uint16 size;
  long end,last,begin;


  get_ext_nb(t,n);
  get_filename_ext(filename,t);

  if (!(stream=fopen(filename,"rb"))) {
    fprintf(stderr,"Error while trying to read file %s \n",filename);
    return -1;
  }
  begin=ftell(stream);
  fseek(stream,0,SEEK_END);
  end=ftell(stream);
  fseek(stream,0,SEEK_SET);

  // If the load crash we must restore current state
  //  save_state(-1);
 
  // we read all the bank and the active page 
  //printf("read page \n");

  for(i=0;i<nb_ram_page;i++)
    fread(ram_page[i],sizeof(Uint8),0x2000,stream);
  
  for(i=0;i<nb_vram_page;i++)
    fread(vram_page[i],sizeof(Uint8),0x2000,stream);
  
  for(i=0;i<nb_wram_page;i++)
    fread(wram_page[i],sizeof(Uint8),0x1000,stream);

  fread(oam_space,sizeof(Uint8),0xa0,stream);
  fread(himem,sizeof(Uint8),0x160,stream);

  fread(&active_rom_page,sizeof(Uint16),1,stream);
  fread(&active_ram_page,sizeof(Uint16),1,stream);
  fread(&active_vram_page,sizeof(Uint16),1,stream);
  fread(&active_wram_page,sizeof(Uint16),1,stream);

  // now we read a couple of (section id,size) 
  while(ftell(stream)<end) {
    fread(&section_id,sizeof(Uint8),1,stream);
    fread(&size,sizeof(Uint16),1,stream);
    last=ftell(stream);
    if ((load_section(stream,&tab_section[section_id],size)<0))  
      goto read_error;

  }
  
  fclose(stream);
  if (conf.sound) 
    update_sound_reg();
  return 0;

 read_error:  
  fclose(stream);
  //  load_state(-1);
  fprintf(stderr,"Error while reading file %s \n",filename);
  return -1;
}

