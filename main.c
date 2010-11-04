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
#include <unistd.h>
#include "global.h"
#include "memory.h"
#include "cpu.h"
#include "rom.h"
#include "vram.h"
#include "interrupt.h"

#include "sound.h"
#include <SDL/SDL.h>

UINT32 fullscr=0;

void exit_gngb(void)
{
  if (rom_page) free_mem_page(rom_page,nb_rom_page);
  if (ram_page) free_mem_page(ram_page,nb_ram_page);
  if (vram_page) free_mem_page(vram_page,nb_vram_page);
  if (wram_page) free_mem_page(wram_page,nb_wram_page);

  close_vram();
  if (conf.sound)
    close_sound();
  
  exit(0);
}
 
void print_help(void) {
  printf("gngb [option] game \n");
  printf("option:\n");
  printf("-h           : show this help\n");
  printf("-a           : auto frame skip\n");
  printf("-g           : force normal gameboy\n");
  printf("-f           : fullscreen\n");
  printf("-j joy_num   : use joy_num as joystick\n");
  printf("-s           : sound on\n");
  printf("-r           : rumble on\n");
  exit_gngb();
}
  


void check_option(int argc,char *argv[]) 
{
  char c;
  conf.normal_gb=0;
  conf.autofs=0;
  conf.sound=0;
  conf.joy_no=0;
  conf.fs=0;
  conf.gb_done=0;
  conf.rumble_on=0;
  while((c=getopt(argc,argv,"rghasfj:"))!=EOF) {
    switch(c) {
    case 'g':conf.normal_gb=1;break;
    case 'a':conf.autofs=1;break;
    case 's':conf.sound=1;break;
    case 'j':conf.joy_no=atoi(optarg);break;
    case 'f':conf.fs=1;fullscr|=SDL_FULLSCREEN;break;
    case 'r':conf.rumble_on=1;break;  
    case 'h':print_help();break;
    }
  }
}

int main(int argc,char *argv[])
{
  
  check_option(argc,argv);
  if(optind >= argc)
    print_help();
  
  if (open_rom(argv[optind])) {
    fprintf(stderr,"Error while trying to read file %s \n",argv[optind]);
    exit_gngb();
  }

  gbcpu_init();
  init_vram(fullscr);

  if(SDL_NumJoysticks()>0){
    joy=SDL_JoystickOpen(conf.joy_no);
    if(joy) {
      printf("Name: %s\n", SDL_JoystickName(conf.joy_no));
      printf("Number of Axes: %d\n", SDL_JoystickNumAxes(joy));
      printf("Number of Buttons: %d\n", SDL_JoystickNumButtons(joy));
      printf("Number of Balls: %d\n", SDL_JoystickNumBalls(joy));
    }
  }

  init_gb_memory(SDL_JoystickNumAxes(joy));
  if (conf.sound) init_sound();
  
  while(!conf.gb_done) {
    update_gb();
  }

  if (rom_type&BATTERY) save_ram();
  exit_gngb();
  exit(0);
}


