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

#ifndef EMU_H
#define EMU_H

#include "global.h"
#include <SDL_keysym.h>

// GAMEBOY TYPE

#define UNKNOW 0x00
#define NORMAL_GAMEBOY 0x01
#define SUPER_GAMEBOY 0x02
#define COLOR_GAMEBOY 0x04
#define COLOR_GAMEBOY_ONLY 0x0c

typedef struct {
  int autoframeskip;           /* auto frameskip */
  int throttle;
  int sleep_idle;

  int fs;               /* fullscreen */  
  int sound;
  int color_filter;
  int rumble_on;
  int serial_on;
  int gb_done;
  int joy_no;
  int gl;
  int yuv;
  int yuv_type;
  int filter;
  int delay_int;
  int show_fps;
  int show_keycode;
  int res_w,res_h;
  int yuv_interline_int;
  int sample_rate;
  int gb_type;
  int const_cycle;
  int gdma_cycle;
  Sint32 pal[5][4];
}GNGB_CONF;

GNGB_CONF conf;

SDL_Joystick *sdl_joy;

Uint16 key[SDLK_LAST];
Sint16 *joy_axis;
Uint8 *joy_but;

#define PAD_UP 0
#define PAD_DOWN 1
#define PAD_LEFT 2
#define PAD_RIGHT 3
#define PAD_A 4
#define PAD_B 5
#define PAD_START 6
#define PAD_SELECT 7

extern Uint8 jmap[8];
extern Uint16 kmap[8];

void print_help(void);

void setup_default_conf(void);
void open_conf(void);
void check_option(int argc,char *argv[]);
void update_key(void);

void emu_init(void);
void emu_run(void);
void emu_reset(void);
void emu_pause(void);
void emu_quit(void);

#endif


