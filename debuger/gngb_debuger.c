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


#include "../global.h"
#include "../cpu.h"
#include "../memory.h"
#include "../rom.h"
#include "../vram.h"
#include "../interrupt.h"
#include "../serial.h"

#include "cpu_info.h"
#include "io_info.h"
#include "mem_info.h"
#include "code_info.h"
#include "op.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

UINT8 quit_debug=0;

// function

void quit_gb_debug(void);
void next_inst(void);
void run(void);
void exec_until(void);
void dump_mem(void);
void list_code(void);
void set_log(void);
void unset_log(void);
void save_state2(void);
void load_state2(void);
void show_stack(void);

typedef struct {
  char *id;
  void (*handler)(void);
}COMMANDE;

UINT16 last;

COMMANDE tab_com[]={
  {"r",show_cpu_info},
  {"q",quit_gb_debug},
  {"n",next_inst},
  {"e",run},
  {"u",exec_until},
  {"d",dump_mem},
  {"l",list_code},
  {"i",show_io_info},
  {"al",set_log},
  {"dl",unset_log},
  {"save",save_state2},
  {"load",load_state2},
  {"ss",show_stack},
  {NULL,NULL}
};

char *arg_line;

/******************** Debuger function ***************************/

void quit_gb_debug(void)
{
  quit_debug=1;
}

void next_inst(void) {
  update_gb();
  show_cpu_info();
}

void run(void) {
  conf.gb_done=0;
  while(!conf.gb_done) 
    update_gb();
  show_cpu_info();
}

void exec_until(void) {
  UINT16 add;
  char *s=NULL;

  if ((s=strtok(arg_line," \t"))) 
    sscanf(s,"%x",&add);
  else {
    printf("Stop at add (hex): ");
    scanf("%x",&add);
  }

  if (add>0xffff) return;

  if (gbcpu->pc.w==add)    
    update_gb();
  
  conf.gb_done=0;
  while(!conf.gb_done && gbcpu->pc.w!=add) {
    last=gbcpu->pc.w;
    update_gb();
  }
  printf("%04x\n",last);
  
  show_cpu_info();
}

void dump_mem(void) {
  UINT16 add;
  char *s=NULL;

  if ((s=strtok(arg_line," \t"))) 
    sscanf(s,"%x",&add);
  else {
    printf("Dump at add (hex): ");
    scanf("%x",&add);
  }
  show_mem_info(add,16);
}

void list_code(void) {
  UINT16 add=gbcpu->pc.w;
  char *s=NULL;
  
  if ((s=strtok(arg_line," \t"))) 
    sscanf(s,"%x",&add);
  show_code_info(add,15);
}

void set_log(void) {
  active_log=1;
  printf("active log\n");
}

void unset_log(void) {
  active_log=0;
  printf("desactive log\n");
}

void save_state2(void) {
  int n;
  char *s=NULL;

  if ((s=strtok(arg_line," \t"))) 
    sscanf(s,"%d",&n);
  else {
    printf("Save state nb: ");
    scanf("%d",&n);
  }
  save_state(n);
}

void load_state2(void) {
  int n;
  char *s=NULL;

  if ((s=strtok(arg_line," \t"))) 
    sscanf(s,"%d",&n);
  else {
    printf("Save state nb: ");
    scanf("%d",&n);
  }
  load_state(n);
}

void show_stack(void) {
  UINT16 add=gbcpu->sp.w;
  int i;

  for(i=add;i<add+32 && i<0xffff;i+=2)
    printf("%04x:[%02x%02x]\n",i,mem_read(i+1),mem_read(i));	 

}

char *get_arg_line(char *c) {

  while((*c)!=0 && (*c)!=' ' && (*c)!='\t') c++;
  while((*c)!=0 && ((*c)==' ' || (*c)=='\t')) c++;

  return c;
}

void check_option(int argc,char *argv[]) 
{
  char c;
  conf.normal_gb=0;
  conf.autofs=0;
  conf.sound=0;
  conf.serial_on=0;
  conf.joy_no=0;
  conf.fs=0;
  conf.gb_done=0;
  conf.rumble_on=0;
  while((c=getopt(argc,argv,"c:lrghasfj:"))!=EOF) {
    switch(c) {
    case 'g':conf.normal_gb=1;break;
    case 'a':conf.autofs=1;break;
    case 's':conf.sound=1;break;
    case 'j':conf.joy_no=atoi(optarg);break;
    case 'r':conf.rumble_on=1;break;  
    case 'l':conf.serial_on=1;gbserial_init(1,NULL);break;
    case 'c':conf.serial_on=1;gbserial_init(0,optarg);break;
    }
  }
} 

int main(int argc,char *argv[])
{
  int i;
  char command[20]={0},line[20]={0};
  char *buf_com;

  check_option(argc,argv);
  if (optind>=argc) exit(1);
  if (open_rom(argv[optind])) exit(1);

  gbmemory_init();
  gblcdc_init();
  gbcpu_init();
  gbtimer_init();
  init_vram(0);

  open_log();

  while(!quit_debug && (buf_com=readline("GNGB COM >"))) {
    char flag=0;
    if ((buf_com) && (buf_com[0]!=0)) {
      add_history(buf_com);
      sscanf(buf_com,"%s",command);
      strcpy(line,buf_com);
      free(buf_com);
    }
    i=0;
    flag=1;
    arg_line=get_arg_line(line);
    while((tab_com[i].id!=NULL) && (flag)) {
      if (!(flag=strcmp(tab_com[i].id,command)))
	tab_com[i].handler();
      i++;
    }
  }

  if (rom_type&BATTERY) save_ram();
  if (rom_page) free_mem_page(rom_page,nb_rom_page);
  if (ram_page) free_mem_page(ram_page,nb_ram_page);
  if (vram_page) free_mem_page(vram_page,nb_vram_page);
  if (wram_page) free_mem_page(wram_page,nb_wram_page);

  close_vram();
  close_log();
  exit(0);
}
