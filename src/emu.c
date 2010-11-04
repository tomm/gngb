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

#include <stdlib.h>

#include <config.h>

#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "getopt.h"
#endif

#include <string.h>
#include <SDL.h>
#include <SDL_keysym.h>
#include "emu.h"
#include "message.h"
#include "rom.h"
#include "vram.h"
#include "serial.h"
#include "memory.h"
#include "cpu.h"
#include "interrupt.h"
#include "menu.h"
#include "frame_skip.h"
#include "sound.h"
#include "sgb.h"
#include "video_std.h"
#include "save.h"

#define ABS(a) (((a)>=0)?(a):(-(a)))

/* Default mapping for joystick and keyboard */
Uint16 kmap[8]={SDLK_UP,SDLK_DOWN,SDLK_LEFT,SDLK_RIGHT,
		SDLK_x,SDLK_w,SDLK_RETURN,SDLK_RSHIFT};
Uint8 jmap[8]={1,1,0,0,3,2,0,1};

Sint16 *joy_axis;
Uint8 *joy_but;

/* Configuration File */

#define UINTEGER8  1
#define UINTEGER16 2
#define UINTEGER32 3
#define STRING     4

struct {
  char *name;
  int type;
  void *var;
  int size;
} config_var[]={{"fullscreen",UINTEGER32,&conf.fs,1},
		{"sound",UINTEGER32,&conf.sound,1},
		{"show_fps",UINTEGER32,&conf.show_fps,1},
		{"autoframeskip",UINTEGER32,&conf.autoframeskip,1},
		{"throttle",UINTEGER32,&conf.throttle,1},
		{"sleep_idle",UINTEGER32,&conf.sleep_idle,1},
		{"rumble",UINTEGER32,&conf.rumble_on,1},
		{"gb_type",UINTEGER32,&conf.gb_type,1},
		{"delay_int",UINTEGER32,&conf.delay_int,1},
		{"glmode",UINTEGER32,&conf.gl,1},
		{"yuv",UINTEGER32,&conf.yuv,1},
		{"yuv_type",UINTEGER32,&conf.yuv_type,1},
		{"resolution_w",UINTEGER32,&conf.res_w,1},
		{"resolution_h",UINTEGER32,&conf.res_h,1},
		{"sample_rate",UINTEGER32,&conf.sample_rate,1},
		{"joy_dev",UINTEGER32,&conf.joy_no,1},
		{"map_joy",UINTEGER8,jmap,8},
		{"map_key",UINTEGER16,kmap,8},
		{"pal_1",UINTEGER32,conf.pal[0],4},
		{"pal_2",UINTEGER32,conf.pal[1],4},
		{"pal_3",UINTEGER32,conf.pal[2],4},
		{"pal_4",UINTEGER32,conf.pal[3],4},
		{"pal_5",UINTEGER32,conf.pal[4],4},
		{"gdma_cycle",UINTEGER32,&conf.gdma_cycle,1},
		{"const_cycle",UINTEGER32,&conf.const_cycle,1},
		{"color_filter",UINTEGER32,&conf.color_filter,1},
		{"filter",UINTEGER32,&conf.filter,1},
		{NULL,0,NULL,0}};

enum {
  OPT_SAMPLE_RATE=256,
  OPT_YUV_TYPE,
  OPT_FILTER
};
int option_index = 0;
static struct option long_options[] =
{
  {"help",0,NULL,'h'},
  {"autoframeskip", 0, NULL, 'a'},
  {"no-autoframeskip", 0, &conf.autoframeskip, 0},
  {"sleep_idle", 0, &conf.sleep_idle, 1},
  {"no-sleep_idle", 0, &conf.sleep_idle, 0},
  {"color_filter",0,&conf.color_filter,1},
  {"no-color_filter",0,&conf.color_filter,0},
  {"rumble",0,NULL,'r'},
  {"no-rumble",0,&conf.rumble_on,1},
  {"res",1,NULL,'R'},
  {"yuv",0,NULL,'Y'},
  {"filter",1,NULL,OPT_FILTER},
  {"no-yuv",0,&conf.yuv,0},
  {"yuv_type",1,NULL,OPT_YUV_TYPE},
  {"opengl",0,NULL,'o'},
  {"no-opengl",0,&conf.gl,0},
  {"fullscreen", 0, NULL, 'f'},
  {"no-fullscreen", 0, &conf.fs, 0},
  {"fps", 0, &conf.show_fps, 1},
  {"no-fps", 0, &conf.show_fps, 0},
  {"sound", 0, NULL, 's'},
  {"no-sound", 0, &conf.sound, 0},
  {"sample_rate",1, NULL, OPT_SAMPLE_RATE},
  {"color_gb",0,NULL,'C'},
  {"normal_gb",0,NULL,'G'},
  {"super_gb",0,NULL,'S'},
  {"auto_gb",0,&conf.gb_type,0},
  {"constant_cycle",0,NULL,'y'},
  {"no-constant_cycle",0,&conf.const_cycle,0},
  {"gdma_cycle",0,NULL,'g'},
  {"no-gdma_cycle",0,&conf.gdma_cycle,0},
  {"joy_dev",1,0,'j'},
  {"no-joy",0,&conf.use_joy,0},
  {"version",0,NULL,'v'},
  {0, 0, 0, 0}
};

void print_help(void) {
  printf("Usage: gngb [OPTION]... FILE\n");
  printf("Emulate the GameBoy rom pointed by FILE\n\n");
  printf("  -h, --help                 print this help and exit\n");
  printf("  -a, --autoframeskip        turn on autoframeskip\n");
  printf("      --sleep_idle           sleep when idle\n");
  printf("      --color_filter         turn on the color filter\n");
  printf("  -r, --rumble               turn on the rumble simulation\n");
  printf("      --filter=X             Set the filter to apply (only for standard mode)\n");
  printf("                              0 = none\n");
  printf("                              1 = scanline\n");
  printf("                              2 = scanline 50%%\n");
  printf("                              3 = smooth\n");
  printf("                              4 = pseudo cell shading\n");
  printf("  -R, --res=WxH              set the resolution to WxH (for YUV and GL mode)\n");
  printf("  -Y, --yuv                  turn YUV mode on\n");
  printf("      --yuv_type             set the type of the YUV overlay\n");
  printf("                              0 = YV12\n");
  printf("                              1 = YUY2\n");
  printf("  -o, --opengl               turn OpenGL mode on (if conpiled in)\n");
  printf("  -f, --fullscreen           run gngb in fullscreen\n");
  printf("      --fps                  show frame/sec\n");
  printf("  -s, --sound                turn on sound\n");
  printf("      --sample_rate=RATE     set the sample rate to RATE\n");
  printf("  -C, --color_gb             force to color gameboy mode\n");
  printf("  -G, --normal_gb            force to normal gameboy mode\n");
  printf("  -S, --super_gb             force to super gameboy mode (experimental)\n");
  printf("      --auto_gb              turn on automatique detection\n");
  printf("  -j, --joy_dev=N            use the Nth joystick\n");
  printf("  -g, --gdma_cycle           cpu stop during gdma transfer (experimental)\n");
  printf("  -v, --version              printf gngb number version\n");
  printf("\n");
  printf("Most options can be disabled whith --no-OPTION (Ex: --no-sound turn sound off)\n\n");
  exit(0);
}

void setup_default_conf(void) {
  int i;

  conf.autoframeskip=0;          
  conf.throttle=1;
  conf.sleep_idle=0;

  conf.sound=0;
  conf.serial_on=0;
  conf.use_joy=1;
  conf.joy_no=0;
  conf.fs=0;
  conf.gb_done=0;
  conf.rumble_on=0;
  conf.delay_int=0;
  conf.show_fps=0;
  conf.gb_type=0;
  /* deprecated: now allways constant cycle */
  conf.const_cycle=0;
  conf.gdma_cycle=0;
  conf.color_filter=1;
  conf.sample_rate=44100;
  conf.gl=0;
  conf.filter=0;

  conf.yuv=0;  
  conf.yuv_type=0;
  conf.yuv_interline_int=0x2020; // intensité d'une interligne en yuv color ((int&0xff)<<8)|(int&0xff) (unused now)
  conf.res_w=160*2;
  conf.res_h=144*2;
  conf.video_flag=0;

  conf.show_keycode=0;
  
  for (i=0;i<5;i++) {
    conf.pal[i][0]=0xB8A68D; //0xc618; // ffe6ce
    conf.pal[i][1]=0x917D5E; //0x8410; // bfad9a
    conf.pal[i][2]=0x635030; //0x4208; // 7f7367
    conf.pal[i][3]=0x211A10; //0x0000; // 3f3933
  }

  /* Movie */
  conf.save_movie=0;
  conf.play_movie=0;

}

/* Function for read configuration File */

int discard_line(char *buf)
{
  if (buf[0]=='#')
    return 1;
  if (buf[0]=='\n')
    return 1;
  return 0;
}
void read_tab(void *tab,int type,char *val,int size)
{
  int i=0;
  char *v;

  v=strtok(val,",");
  while(v!=NULL && i<size) {
    if (type==UINTEGER8)
      ((Uint8 *)tab)[i]=strtol(v,NULL,0);
    else if (type==UINTEGER16)
      ((Uint16 *)tab)[i]=strtol(v,NULL,0);
    else
      ((Uint32 *)tab)[i]=strtol(v,NULL,0);
    v=strtok(NULL,",");
    i++;
  }
}

void open_conf() {
  char *filename;
  char buf[512];
  char name[32];
  char val[64];
  int i=0;
  FILE *f;
  int len;
  char *s;

  s = getenv("HOME");
  if(s==NULL) {
    buf[0] = '.';
    buf[1] = 0;
    s = buf;
  }
  len=strlen("gngbrc")+strlen(s)+strlen("/.gngb/")+1;

  filename=(char *)malloc(len);
  sprintf(filename,"%s/.gngb/gngbrc",s);
  if ((f=fopen(filename,"rb"))==0) return;
  
  while(!feof(f)) {
    i=0;
    fgets(buf,510,f);

    if (discard_line(buf))
      continue;
    
    sscanf(buf,"%s %s\n",name,val);
    while(config_var[i].name!=NULL) {
      if (strcmp(name,config_var[i].name)==0) {
	if (config_var[i].size==1)
	  if (config_var[i].type==UINTEGER8)
	    *(Uint8 *) config_var[i].var=strtol(val,NULL,0);
	  else if (config_var[i].type==UINTEGER16)
	    *(Uint16 *) config_var[i].var=strtol(val,NULL,0);
	  else if (config_var[i].type==UINTEGER32)
	    *(Uint32 *) config_var[i].var=strtol(val,NULL,0);
	  else {
	    *((char **) config_var[i].var)=(char *)malloc(strlen(val)+1);
	    strncpy(*((char **) config_var[i].var),val,strlen(val)+1);
	  }
	else
	  read_tab(config_var[i].var,config_var[i].type,val,config_var[i].size);
	break;
      }
      i++;
    }
  }
  fclose(f);
}

void check_option(int argc,char *argv[]) 
{
  int c=-1;
  while((c=getopt_long(argc,argv,"yrGSChvasfj:oYR:dglc:FO",long_options, &option_index))!=EOF) {
    switch(c) {
    case OPT_SAMPLE_RATE:conf.sample_rate=atoi(optarg);break;
    case OPT_YUV_TYPE:conf.yuv_type=atoi(optarg);break;
    case OPT_FILTER:
      conf.filter=atoi(optarg);
      conf.res_w=SCREEN_X*2;
      conf.res_h=SCREEN_Y*2;
      break;
    case 'G':conf.gb_type=NORMAL_GAMEBOY;printf("Force to normal GB\n");break;
    case 'S':conf.gb_type|=SUPER_GAMEBOY;printf("Force to super GB\n");break;
    case 'C':conf.gb_type|=COLOR_GAMEBOY;printf("Force to color GB\n");break;
    case 'a':conf.autoframeskip=1;break;
    case 's':conf.sound=1;break;
    case 'j':conf.joy_no=atoi(optarg);break;
    case 'f':conf.fs=1;break;
    case 'r':conf.rumble_on=1;break;  
    case 'y':conf.const_cycle=1;break;
    case 'g':conf.gdma_cycle=1;break;
    case 'h':print_help();break;
    case 'v':printf("%s\n",VERSION);exit(0);break;
#ifdef SDL_GL
    case 'o' :
      conf.gl=1;break;
#else
    case 'o':printf("Opengl mode not conpiled in\n");break; 
#endif
#ifdef SDL_YUV
    case 'Y':
      conf.yuv=1;
      break;
#else 
    case 'Y':printf("YUV mode not conpiled in\n");break;   
#endif
    case 'R': if (conf.yuv || conf.gl)
      sscanf(optarg,"%dx%d",&conf.res_w,&conf.res_h);
    break;
    case 'd':conf.delay_int=1;break;
      /* Link */
    case 'l':conf.serial_on=1;gbserial_init(1,NULL);break;
    case 'c':conf.serial_on=1;gbserial_init(0,optarg);break;
    }
  }
  if (conf.yuv) conf.gl=0;
}

__inline__ void update_key(void) {
  static char restore_fps;
  static Uint8 pause;
  SDL_Event event;
  
  while(SDL_PollEvent(&event) || pause) {
    switch (event.type) {
#if defined(SDL_YUV) || defined(SDL_GL)
    case SDL_VIDEORESIZE:
      if (conf.yuv || conf.gl) {
        conf.res_w=event.resize.w;
        conf.res_h=event.resize.h;
        reinit_vram();
      }
      break;
#endif
    case SDL_QUIT:
      conf.gb_done=1;
      break;

    case SDL_JOYAXISMOTION:
      joy_axis[event.jaxis.axis]=event.jaxis.value;
      if (conf.show_keycode) set_message("%d",event.jaxis.axis);
      break;
    case SDL_JOYBUTTONDOWN:
      joy_but[event.jbutton.button]=1;
      if (conf.show_keycode) set_message("%d",event.jbutton.button);
      break;
    case SDL_JOYBUTTONUP:
      joy_but[event.jbutton.button]=0;
      if (conf.show_keycode) set_message("%d",event.jbutton.button);
      break;
      
    case SDL_KEYUP:
      key[event.key.keysym.sym]=0;
      //printf("key released %d\n",event.key.keysym.sym);
      break;
    case SDL_KEYDOWN:
      key[event.key.keysym.sym]=1;
      //printf("key pressed %d\n",event.key.keysym.sym);
      if (conf.show_keycode) set_info("%d",event.key.keysym.sym);
      switch(event.key.keysym.sym) {
      case SDLK_ESCAPE: conf.gb_done=1;break;
      case SDLK_TAB:
	if (conf.sound) SDL_PauseAudio(1);
	/* save the screen */
	save_gb_screen();

	loop_menu(&main_menu);
	if (conf.sound) SDL_PauseAudio(0);
	reset_frame_skip();
	restore_message_pal();

	break;
      case SDLK_F9:switch_fullscreen();break;  // F9: swicth fullscreen
      case SDLK_F10:conf.show_fps^=1;
	if (!conf.show_fps)
	  unset_info();
	else
	  set_info("fps:..");
	break;     // F10: show fps
      case SDLK_F11:emu_reset();set_message("Reset");break; // F11: Reset 
      case SDLK_F12:
	if (event.key.keysym.mod==1) {
	  SDL_SaveBMP(gb_screen,"./gngb.bmp");
	} else {
	  conf.show_keycode^=1;    // F12: show keycode 
	  if (conf.show_keycode) {
	    set_message("Show keysym code : ON");
	    restore_fps=conf.show_fps;
	    conf.show_fps=0;
	  } else {
	    set_message("Show keysym code : OFF");
	    unset_info();
	    if (restore_fps) conf.show_fps=1;
	  }
	}
	break;
      case SDLK_KP1:
	if (conf.gb_type&NORMAL_GAMEBOY) {
	  set_message("Set Pal 1");
	  gb_set_pal(0);
	}
	break;
      case SDLK_KP2:
	if (conf.gb_type&NORMAL_GAMEBOY) {
	  set_message("Set Pal 2");
	  gb_set_pal(1);
	}
	break;
      case SDLK_KP3:
	if (conf.gb_type&NORMAL_GAMEBOY) {
	  set_message("Set Pal 3");
	  gb_set_pal(2);
	}
	break;
      case SDLK_KP4:
	if (conf.gb_type&NORMAL_GAMEBOY) {
	  set_message("Set Pal 4");
	  gb_set_pal(3);
	}
	break;
       case SDLK_KP5:
	 if (conf.gb_type&NORMAL_GAMEBOY) {
	   set_message("Set Pal 5");
	   gb_set_pal(4);
	 }
	break;      
      case SDLK_KP6:
	if (conf.gb_type&COLOR_GAMEBOY) {
	  conf.color_filter^=1;
	  GenFilter();
	  update_all_pal();
	}
	break;
	
      case SDLK_F8:
	rb_on=1;
	break;   
      case SDLK_F7:
	pause=1;
	break;	
      case SDLK_F6:
	pause=0;
	break;	
      case SDLK_F4:
	if (conf.save_movie) end_save_movie();
	else begin_save_movie();
	break;
      case SDLK_F5:
	play_movie();
	break;
      default:
	break;
      }
      break;
    default:
      break;
    }
  }
}

void emu_init(void) {
  init_vram((conf.fs?SDL_FULLSCREEN:0)|(conf.gl?SDL_OPENGL:0));
  gbmemory_init();
  gblcdc_init();
  gbtimer_init();
  gbcpu_init();

  
  if (conf.gb_type&SUPER_GAMEBOY) sgb_init();

  if (conf.use_joy) {
    if(SDL_NumJoysticks()>0) {
      sdl_joy=SDL_JoystickOpen(conf.joy_no);
      if(sdl_joy) {
	printf("Name: %s\n", SDL_JoystickName(conf.joy_no));
	printf("Number of Axes: %d\n", SDL_JoystickNumAxes(sdl_joy));
	printf("Number of Buttons: %d\n", SDL_JoystickNumButtons(sdl_joy));
	printf("Number of Balls: %d\n", SDL_JoystickNumBalls(sdl_joy));
	joy_axis=(Sint16 *)malloc(sizeof(Sint16)*SDL_JoystickNumAxes(sdl_joy));
	joy_but=(Uint8 *)malloc(sizeof(Uint8)*SDL_JoystickNumButtons(sdl_joy));
	memset(joy_axis,0,sizeof(Sint16)*SDL_JoystickNumAxes(sdl_joy));
	memset(joy_but,0,sizeof(Uint8)*SDL_JoystickNumButtons(sdl_joy));
      } else {
	joy_axis=(Sint16 *)malloc(sizeof(Sint16)*2);
	joy_but=(Uint8 *)malloc(sizeof(Uint8)*4);
	memset(joy_axis,0,sizeof(Sint16)*2);
	memset(joy_but,0,sizeof(Uint8)*4);
	memset(jmap,0,8);
      }
    } else conf.use_joy=0;
  }

  if (conf.sound)
    if (gbsound_init()) conf.sound=0;   
}

void emu_run(void) {
  
}

void emu_reset(void) {
  gblcdc_reset();
  gbtimer_reset();
  gbcpu_reset();
  gbmemory_reset();
  //if (conf.sound) gbsound_reset();
}

void emu_pause(void) {
 }

void emu_quit(void) {
  if (rom_type&BATTERY) {
    save_ram();
    if (rom_type&TIMER) save_rom_timer();
  }
}

