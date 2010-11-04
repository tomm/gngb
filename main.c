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
#include <unistd.h>
#include "global.h"
#include "memory.h"
#include "cpu.h"
#include "rom.h"
#include "vram.h"
#include "interrupt.h"

#ifdef LINUX_JOYSTICK
#include "joystick.h"
#endif

#include "sound.h"
#include <SDL/SDL.h>

UINT32 fullscr=0;

void exit_gngb(void)
{
  if (rom_page) free_mem_page(rom_page,nb_rom_page);
  if (ram_page) free_mem_page(ram_page,nb_ram_page);
  if (vram_page) free_mem_page(vram_page,nb_vram_page);
  if (wram_page) free_mem_page(wram_page,nb_wram_page);

#ifdef LINUX_JOYSTICK
  if (my_joy) remove_joy(my_joy);
#endif
  close_vram();
  if (conf.sound)
    close_sound();
  //allegro_exit();
  exit(0);
}
 
void print_help(void) {
  printf("gngb [option] game \n");
  printf("option:\n");
  printf("h : show this help\n");
  printf("a : auto frame skip\n");
  printf("g : force normal gameboy\n");
  printf("f : fullscreen\n");
  printf("s : sound on\n");
  exit_gngb();
}
  


void check_option(int argc,char *argv[]) 
{
  char c;
  conf.normal_gb=0;
  conf.autofs=0;
  conf.sound=0;
  conf.fs=0;
  conf.gb_done=0;
  while((c=getopt(argc,argv,"ghasf"))!=EOF) {
    switch(c) {
    case 'g':conf.normal_gb=1;break;
    case 'a':conf.autofs=1;break;
    case 's':conf.sound=1;break;
    case 'f':fullscr=SDL_FULLSCREEN;break;
    case 'h':print_help();break;
    }
  }
}

void remap_gb_pad(void) {
  char *text_str[]={"up","down","left","right","A","B","Start","Select"};
  int i=0;
  SDL_Event event;

  printf("%s\n",text_str[i]);
  while(i<7) {
    while(SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_KEYDOWN:
	printf("%d\n",event.key.keysym.scancode);
	gb_pad_code[i]=event.key.keysym.scancode;
	printf("%s\n",text_str[++i]);
	break;
      }
    }
  }
}   

int main(int argc,char *argv[])
{
  
  check_option(argc,argv);
  if(optind >= argc)
    print_help();
  
  if (open_rom(argv[optind])) {
    printf("Error while trying to read file %s \n",argv[optind]);
    exit_gngb();
  }

#ifdef LINUX_JOYSTICK 
  my_joy=install_joy(JOY_DEVICE0);
#endif
  gbcpu_init();
  init_gb_memory();
  init_vram(fullscr);
  if (conf.sound) init_sound();

  while(!conf.gb_done) {
    update_gb();
    //main_loop();

  }

  if (rom_type&BATTERY) save_ram();
  exit_gngb();
  exit(0);
}


