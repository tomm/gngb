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

#ifndef _MENU_H_
#define _MENU_H_

#include <config.h>
#include <SDL.h>

typedef struct MENU_ITEM{
  char *name;
  int (*func)(struct MENU_ITEM *self);
  void (*draw_info)(struct MENU_ITEM *self,int menu_pos);
  Uint8 type;
  void *user_data;
  Uint8 state; /* on/off */
  Uint8 group; /* for radio_button */
  Uint8 radio; /* the radio_buuton place in the group */
  Uint8 draw_type;
  struct MENU_ITEM *next;
}MENU_ITEM;

typedef struct MENU{
  char *title;
  MENU_ITEM *item;
  int size,begin,end,id;
}MENU;

#define ACTION 0
#define TOGGLE 1
#define RADIO  2

#define DRAW_ALWAYS   0
#define DRAW_WHEN_ACTIVE 1

extern MENU main_menu;
MENU *current_menu;
void loop_menu(MENU *m);
void display_menu(MENU *m);

#endif
