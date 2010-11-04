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
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <SDL.h>
#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "getopt.h"
#endif

#include "global.h"
#include "emu.h"
#include "memory.h"
#include "cpu.h"
#include "rom.h"
#include "vram.h"
#include "interrupt.h"
#include "serial.h"
#include "frame_skip.h"
#include "emu.h"
#include "sgb.h"
//#include "optargs.h"
#include "sound.h"
#include "save.h"


extern SDL_Joystick *sdl_joy;

void exit_gngb(void)
{
  if (rom_page) free_mem_page(rom_page,nb_rom_page);
  if (ram_page) free_mem_page(ram_page,nb_ram_page);
  if (vram_page) free_mem_page(vram_page,nb_vram_page);
  if (wram_page) free_mem_page(wram_page,nb_wram_page);

  if (conf.sound) close_sound();
  if (conf.serial_on) gbserial_close();
}

int main(int argc,char *argv[])
{
  setup_default_conf();
  open_conf();  
  check_option(argc,argv);
  if(optind >= argc)
    print_help();
  
  if (open_rom(argv[optind])) {
    fprintf(stderr,"Error while trying to read file %s \n",argv[optind]);
    exit(1);
  }

  emu_init();
  cpu_run();
    
  if (rom_type&BATTERY) {
    save_ram();
    if (rom_type&TIMER) save_rom_timer();
  }
  exit_gngb();
  return 0;
}



