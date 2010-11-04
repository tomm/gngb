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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif

#include <math.h>
#include <SDL.h>
#include "frame_skip.h"
#include "message.h"
#include "emu.h"


#ifndef uclock_t
#define uclock_t Uint32
#endif

#define TICKS_PER_SEC 1000000UL
#define CPU_FPS 59.7
#define MAX_FRAMESKIP 20


static char init_frame_skip=1;
char skip_next_frame=0;
#ifdef HAVE_GETTIMEOFDAY
static struct timeval init_tv = {0,0};
#endif

void reset_frame_skip(void) {
#ifdef HAVE_GETTIMEOFDAY
  init_tv.tv_usec = 0;
  init_tv.tv_sec = 0;
#endif
  skip_next_frame=0;
  init_frame_skip=1;
}

uclock_t get_ticks(void) {
#ifdef HAVE_GETTIMEOFDAY
  struct timeval tv;

  gettimeofday(&tv, 0);
  if (init_tv.tv_sec == 0) init_tv = tv;
  return (tv.tv_sec - init_tv.tv_sec) * TICKS_PER_SEC + tv.tv_usec - init_tv.tv_usec;
#else
  return SDL_GetTicks()*1000;
#endif
}

int frame_skip(int init) {
  static int f2skip;
  static uclock_t F=(uclock_t)((double)TICKS_PER_SEC/CPU_FPS);
  static uclock_t sec=0;
  static uclock_t rfd;
  static uclock_t target;
  static int nbFrame=0;
  static int skpFrm=0;

  if (init_frame_skip) {
    init_frame_skip=0;
    target=get_ticks();
    nbFrame=0;
    sec=0;
    return 0;
  }
 
  target+=F;
if (f2skip>0) {
    f2skip--;
    skpFrm++;
    return 1;
  } else 
    skpFrm=0;


  rfd=get_ticks();
  
  if (conf.autoframeskip) {
    if (rfd<target && f2skip==0 ) {
      while(get_ticks()<target) {
	if (conf.sleep_idle) 
#ifdef HAVE_USLEEP
	  usleep(10);
#else
	SDL_Delay(1);
#endif
      }
    } else {
      f2skip=(rfd-target)/(double)F;
      if (f2skip>MAX_FRAMESKIP)
	f2skip=MAX_FRAMESKIP;
    }
}
 
  nbFrame++;
  if (get_ticks()-sec>=TICKS_PER_SEC) {
	  if (conf.show_fps) {
		  set_info("fps:%d",nbFrame);
	  }
	  nbFrame=0;
	  sec=get_ticks();
  }
  return 0;
}



