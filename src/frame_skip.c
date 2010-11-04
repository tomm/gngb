/* The code of autoframeskip is taken from the emulator XMame 
   I just modify it for portability  and customization
*/
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <SDL/SDL.h>
#include "frame_skip.h"
#include "message.h"
#include "emu.h"

#define FRAMESKIP_LEVELS 22

#ifndef CLOCKS_PER_SEC        /* Thanks to Benjamin */
#define CLOCKS_PER_SEC 1000000L
#endif

//typedef clock_t uclock_t;
typedef unsigned long uclock_t;
#define  UCLOCKS_PER_SEC CLOCKS_PER_SEC
// #define uclock clock


#define TICKS_PER_SEC 1000000L
#define CPU_FPS 59.7

/*long get_ticks(void)
{
  return SDL_GetTicks()*1000;
}*/


uclock_t uclock(void)
{
  static uclock_t init_sec = 0;
  struct timeval tv;
  
  gettimeofday(&tv, 0);
  if (init_sec == 0) init_sec = tv.tv_sec;
  
  return (tv.tv_sec - init_sec) * 1000000 + tv.tv_usec;
}

static int modframe = 0;

//int throttle=1;
//int autoframeskip=1;
int frameskip;
//int sleep_idle=0;
int max_autoframeskip=FRAMESKIP_LEVELS;//22;

static int barath_skip_this_frame(void)
{
  return (modframe >= FRAMESKIP_LEVELS);
}

int barath_skip_next_frame(int showfps)
{
  static uclock_t curr = 0;
  static uclock_t prev = 0;
  static uclock_t avg_uclocks = 0;
  static int frames_skipped = 0;
  static int sysload = 0;
  static float framerate = 1;
  static float speed = 1;
  static float lag_rate = -30;

  int skip_this_frame = barath_skip_this_frame();
  int scratch_time = get_ticks();
#ifdef barath_debug
  int debug_value;
  static float slow_speed = 1;
  int uclocks_per_frame = slow_speed * TICKS_PER_SEC / CPU_FPS;
#else
  int uclocks_per_frame = TICKS_PER_SEC / CPU_FPS;
#endif
  /* project target display time of this frame */
  uclock_t target = prev + (frames_skipped + 1) * uclocks_per_frame;

  /* if lagging by more than 2 frames don't try to make up for it */
  while (scratch_time - target > uclocks_per_frame * 2) {
    target += uclocks_per_frame;
    lag_rate++;
  }
  lag_rate *= 0.99;

  {
    static float framerateavg = 0;
    framerateavg = (framerateavg * 5 + 1 - skip_this_frame) / 6.0;
    framerate = (framerate * 5 + framerateavg) / 6.0;
  }
  if (conf.throttle) {
    int leading = ((sysload > 33) && conf.sleep_idle) ? 0 : uclocks_per_frame;
    int sparetime = target - scratch_time;

    /* test for load-induced lags and set sysload */
    if (conf.autoframeskip && conf.sleep_idle) {
      /* if lag is excessive and framerate is low then we have a system hiccup */
      if ((sysload < 100) && (lag_rate > 3) && (frameskip < max_autoframeskip)) {
	sysload++;
	lag_rate = 3;
      }
      /* after ~2000 frames of no lag start lowering sysload */
      else if (sysload && (fabs(lag_rate) < .00001)) {
	sysload--;
	lag_rate = .000011;	/* wait ~10 frames */
      }
    }
    if (conf.autoframeskip) {
      /* this is an attempt at proportionate feedback to smooth things out */
      int feedback = ((sparetime - uclocks_per_frame / 2) / (uclocks_per_frame / 3));

      frameskip = ((1.0 - framerate) * (FRAMESKIP_LEVELS - 1)) - feedback;
      //printf("fs %d %d\n",frameskip,frames_skipped);
#ifdef barath_debug
      debug_value = feedback;
#endif
      if (frameskip > max_autoframeskip)
	frameskip = max_autoframeskip;
      else if (frameskip < 0)
	frameskip = 0;
    }
    if (sparetime > 0) {
      /* if we're more than 2 frames ahead we need to resynch */
      if (sparetime > uclocks_per_frame * 2)
	target = scratch_time;
      else {
	/* idle until we hit frame ETA or leading */
	//profiler_mark(PROFILER_IDLE);
	while (target - get_ticks() > leading)
	  if (conf.sleep_idle)
	    //usleep(100);
	    SDL_Delay(1);
	//profiler_mark(PROFILER_END);
      }
    }
    /* if we are behind we should force a skip: */
    else if (conf.autoframeskip && (frameskip < max_autoframeskip)
	     && (frames_skipped < 1)) {
      modframe = FRAMESKIP_LEVELS * 2 - frameskip;
      //printf("%d \n",modframe);
    }

  }				/* if (throttle) */
  if (skip_this_frame && (frames_skipped < FRAMESKIP_LEVELS))
    frames_skipped++;
  else {
    /* update frame timer */
    prev = target;

    /* calculate average running speed for display purposes */
    scratch_time = curr;
    curr = get_ticks();
    avg_uclocks = (avg_uclocks * 5 + curr - scratch_time) / (6 + frames_skipped);
    speed = (speed * 5 + (float) uclocks_per_frame / avg_uclocks) / 6.0;
    /* double-forward average  */
    //#if 0
    if (showfps) {
      static int showme = 20;
      if (showme++ > 20) {
	int fps= CPU_FPS * framerate * speed + .5;
	//sprintf(conf.fps,"%2d",fps);
	//printf("%2d\n",fps);
	set_message("fps:%d",fps);
	showme=0;
      }
    }
    //#endif
    frames_skipped = 0;
  }


  /* give a little grace in case something else sets it off */
  if (conf.sleep_idle && conf.autoframeskip && (sysload > 33)) {
    //profiler_mark(PROFILER_IDLE);
    //usleep(100);
    SDL_Delay(1);
    //profiler_mark(PROFILER_END);
  }
  /* advance frameskip counter */
  if (modframe >= FRAMESKIP_LEVELS)
    modframe -= FRAMESKIP_LEVELS;
  modframe += frameskip;

  return barath_skip_this_frame();
}

/* the following code is experimantale i dont use it */
uclock_t elapsed_clock(int init)
{
  static uclock_t init_sec = 0;
  struct timeval tv;

  if (init) init_sec=0;

  gettimeofday(&tv, 0);
  if (init_sec == 0) init_sec = tv.tv_sec;
  // printf(" %d\n",(tv.tv_sec - init_sec) * 1000000 + tv.tv_usec);
  return (tv.tv_sec - init_sec) * 1000000L + tv.tv_usec;
}

static char init_frame_skip=1;
char skip_next_frame=0;
static struct timeval init_tv = {0,0};

uclock_t get_ticks(void) {
  struct timeval tv;
  gettimeofday(&tv, 0);
  if (init_tv.tv_sec == 0) init_tv = tv;
  return (tv.tv_sec - init_tv.tv_sec) * 1000000UL + tv.tv_usec - init_tv.tv_usec;
}

void reset_frame_skip(void) {
  init_tv.tv_usec = 0;
  init_tv.tv_sec = 0;
  skip_next_frame=0;
  init_frame_skip=1;
}

int frame_skip(int init) {
  static int frame2skip;
  //static uclock_t F=(uclock_t)((double)1000000UL/CPU_FPS);
  static uclock_t t=0,lt=0,sec=0;
  static uclock_t f=(uclock_t)((double)1000000UL/CPU_FPS);
  static int nbFrame=0;
  uclock_t dt;
  
  if (init_frame_skip) {
    init_frame_skip=0;
    t=get_ticks();
    frame2skip=0;
    return 0;
  } else {
    lt=t;
    t=get_ticks();
  }

  if (frame2skip) {
    frame2skip--;
    return 1;
  }

  dt=t-lt;
 
  /*if (dt>F*12)
    dt=F*12;*/
  
  nbFrame++;
  if ((t-sec)>=1000000UL) {
    if (conf.show_fps) set_message("fps:%d",nbFrame);
    nbFrame=0;
    sec=get_ticks();
  }
  

  //printf("%d %d\n",dt,f);
  /*
  if (skpFrm>0) {
    skpFrm--;
    return 1;
    }*/
  
  
  if (dt<f) {
    while(dt<f) {
      // printf("wait\n");
      //SDL_Delay(1);
      //usleep(1);
    //SDL_Delay((f-dt)/1000);
      dt=(t=get_ticks())-lt;
      /*dt=elapsed_clock(0)-lt;
	t=elapsed_clock(0);*/
      //printf("%d %d\n",dt,i++);
    }
    
    //f=F-(dt-f);
    return 0;
  } else {
    //printf("skip\n");
    // printf("skip\n");
    //f=F-((dt-f)%F);
    // printf("%d %d\n",F,f);
    //skpFrm++;
    frame2skip=(dt/f);
    //printf("frame2skip %d:\n",frame2skip);
    return 1;
  }
}





