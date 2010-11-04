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

#ifndef SERIAL_H
#define SERIAL_H

#include "global.h"

struct {
  Sint16 cycle_todo;
  Uint16 p;
  Uint8 b;
  Uint8 byte_wait;
  Uint8 check;
  Uint8 wait;
  Uint8 ready2read;
}gbserial;

Sint16 serial_cycle_todo;
Sint8 gblisten;

void gbserial_init(int server_side,char *servername);
void gbserial_close(void);
void gbserial_send(Uint8 b);
Sint8 gbserial_receive(void);
char gbserial_check(void);
Uint8 gbserial_wait_data(void);

#endif


