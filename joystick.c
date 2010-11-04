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
#include <linux/joystick.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include "joystick.h"

JOY_CONTROL * install_joy(char *joy_dev) 
{
  JOY_CONTROL *t=0;
  t=(JOY_CONTROL *)malloc(sizeof(JOY_CONTROL));
  memset(t,0,sizeof(JOY_CONTROL));

  if ((t->joy_fd = open (joy_dev, O_RDONLY)) < 0) {
    printf("joystick (%s): device not detected",joy_dev);
    return 0;
  }
  return t;
}

void read_joy(JOY_CONTROL *j)
{
  int status, x, y;
  size_t expected;
  struct JS_DATA_TYPE js;

  if (!j) 
    return;

  if (j->joy_fd == -1)
    return;

  status = read(j->joy_fd, &js, expected = JS_RETURN);
  if (status == expected)
  {
    x = js.x;
    y = js.y;
    if (x < 0) x = 0;
    else if (x > 255) x = 255;
    if (y < 0) y = 0;
    else if (y > 255) y = 255;
    
    j->x = x - 128; 
    j->y = y - 128;
    j->but = js.buttons;
  } else  printf("Unknown status from joystick: %d (ignored) \n",status);
}

void remove_joy(JOY_CONTROL *j)
{
  close(j->joy_fd);
  free(j);
}













