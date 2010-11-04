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


#ifndef _ROM_H
#define _ROM_H

#include "global.h"
#include <stdlib.h>

/* rom type definition */
#define UNKNOW_TYPE    0x00
#define ROM_ONLY       0x01
#define MBC1           0x02
#define MBC2           0x04
#define MBC3           0x08
#define MBC5           0x10
#define RAM            0x20
#define BATTERY        0x40
#define TIMER          0x80
#define RUMBLE         0x100
#define SRAM           0x200
#define HUC1           0x400

char *rom_name;
extern Sint16 rom_type;

//  rom_gb_suport

extern Uint8 rom_gb_type;

typedef struct {
  Uint16 cycle;
  Uint8 reg_sel;
  Uint8 latch;
  Uint8 reg[5];
  Uint8 regl[5];   // register locked
}ROM_TIMER; // MBC3

ROM_TIMER *rom_timer;

int open_rom(char *filename);

#endif




