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


#ifndef _JOYSTICK_H
#define _JOYSTICK_H

#define JOY_DEVICE0 "/dev/js0"
#define JOY_DEVICE1 "/dev/js1"
#define JOY_DEVICE2 "/dev/js2"
#define JOY_DEVICE3 "/dev/js3"

//static int joy_fd=-1;

typedef struct _joy_control {
  int joy_fd;
  int x,y;
  unsigned short but;
}JOY_CONTROL;


// function

JOY_CONTROL *install_joy(char *);
void read_joy(JOY_CONTROL *);
void remove_joy(JOY_CONTROL *);

#endif










