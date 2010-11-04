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

/* rom type definition */
#define UNKNOW_TYPE    -1
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

char rom_name[256];
extern INT16 rom_type;


#define NORMAL_GAMEBOY 0x01
#define SUPER_GAMEBOY 0x02
#define COLOR_GAMEBOY 0x04
#define COLOR_GAMEBOY_ONLY 0x0c

extern UINT8 gameboy_type;

int open_rom(char *filename);
int save_ram(void);
int load_ram(void);
int save_snap(void);
int load_snap(void);

#endif
