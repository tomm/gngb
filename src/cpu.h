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


#ifndef _CPU_H
#define _CPU_H

#include "global.h"

#define FLAG_Z 0x80
#define FLAG_N 0x40
#define FLAG_H 0x20
#define FLAG_C 0x10
#define FLAG_NZ 0x7F
#define FLAG_NN 0xBF
#define FLAG_NH 0xDF
#define FLAG_NC 0xEF

typedef union {
	UINT16 w;
	struct {
		UINT8 l,h;
	}b;
}REG;

#define SIMPLE_SPEED 0
#define DOUBLE_SPEED 1

#define HALT_STATE 1
#define STOP_STATE 2

typedef struct {
  REG af;
  REG bc;
  REG de;
  REG hl;
  REG sp;
  REG pc;
  UINT8 int_flag;        // IME 
  UINT8 ei_flag;    
  UINT8 mode;
  UINT8 state;
}GB_CPU;

extern GB_CPU *gbcpu;

void gbcpu_init(void);  
void gbcpu_reset(void);
inline UINT8 gbcpu_exec_one(void);
inline void update_gb(void);

#endif

	
