#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <SDL/SDL.h>
#include <SDL/SDL_keysym.h>
#include "emu.h"
#include "message.h"
#include "rom.h"
#include "vram.h"
#include "serial.h"
#include "memory.h"
#include "cpu.h"
#include "interrupt.h"


/* Mapping for joystick and keyboard */
UINT16 kmap[8]={SDLK_UP,SDLK_DOWN,SDLK_LEFT,SDLK_RIGHT,
		SDLK_x,SDLK_w,SDLK_RETURN,SDLK_RSHIFT};
UINT8 jmap[8]={1,1,0,0,3,2,0,1};

// Configuration File

#define UINTEGER8  1
#define UINTEGER16 2
#define UINTEGER32 3
#define STRING     4

struct {
  char *name;
  int type;
  void *var;
  int size;
} config_var[]={{"fullscreen",UINTEGER8,&conf.fs,1},
		{"sound",UINTEGER8,&conf.sound,1},
		{"show_fps",UINTEGER8,&conf.show_fps,1},
		{"autoframeskip",UINTEGER8,&conf.autoframeskip,1},
		{"throttle",UINTEGER8,&conf.throttle,1},
		{"sleep_idle",UINTEGER8,&conf.sleep_idle,1},
		{"rumble",UINTEGER8,&conf.rumble_on,1},
		{"gb_type",UINTEGER8,&conf.gb_type,1},
		{"delay_int",UINTEGER8,&conf.delay_int,1},
		{"glmode",UINTEGER8,&conf.gl,1},		
		{"gl_size_w",UINTEGER16,&conf.gl_w,1},
		{"gl_size_h",UINTEGER16,&conf.gl_h,1},
		{"no_joy",UINTEGER8,&conf.joy_no,1},
		{"nb_joy",UINTEGER8,&conf.joy_no,1},
		{"map_joy",UINTEGER8,jmap,8},
		{"map_key",UINTEGER16,kmap,8},
		{"pal_1",UINTEGER32,conf.pal[0],4},
		{"pal_2",UINTEGER32,conf.pal[1],4},
		{"pal_3",UINTEGER32,conf.pal[2],4},
		{"pal_4",UINTEGER32,conf.pal[3],4},
		{"pal_5",UINTEGER32,conf.pal[4],4},
		{"gdma_cycle",UINTEGER8,&conf.gdma_cycle,1},
		{"const_cycle",UINTEGER8,&conf.const_cycle,1},
		{NULL,0,NULL,0}};


void print_help(void) {
  printf("gngb [option] game \n");
  printf("option:\n");
  printf("-h           : show this help\n");
  printf("-a           : auto frame skip\n");
  printf("-G           : force normal gameboy\n");
  printf("-S           : force super gameboy\n");
  printf("-C           : force color gameboy\n");
  printf("-f           : fullscreen\n");
  printf("-j joy_num   : use joy_num as joystick\n");
  printf("-s           : sound on\n");
  printf("-r           : rumble on\n");
  printf("-o           : opengl renderer\n");
  printf("-O WxH       : same as o, but with WxH resolution (instead of double size)\n");
  printf("-y           : use constant cycle\n");
  printf("-g           : try to emulate gdma cycle (experimental)\n");
  exit(0);
}

void setup_default_conf(void) {
  int i;

  conf.autoframeskip=0;          
  conf.throttle=1;
  conf.sleep_idle=0;

  conf.sound=0;
  conf.serial_on=0;
  conf.joy_no=0;
  conf.fs=0;
  conf.gb_done=0;
  conf.rumble_on=0;
  conf.delay_int=0;
  conf.show_fps=0;
  conf.gb_type=0;
  conf.const_cycle=0;
  conf.gdma_cycle=0;

  conf.gl=0;
  conf.gl_w=160*2;
  conf.gl_h=144*2;

  conf.yuv=0;  // ATTENTION!!!
  conf.yuv_interline_int=0x2020; // intensité d'une interligne en yuv color ((int&0xff)<<8)|(int&0xff)
  conf.yuv_w=160;
  conf.yuv_h=144;

  conf.show_keycode=0;

  for (i=0;i<5;i++) {
    conf.pal[i][0]=0xB8A68D; //0xc618; // ffe6ce
    conf.pal[i][1]=0x917D5E; //0x8410; // bfad9a
    conf.pal[i][2]=0x635030; //0x4208; // 7f7367
    conf.pal[i][3]=0x211A10; //0x0000; // 3f3933
  }
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
      /*((UINT8 *)tab)[i]=atoi(v);*/
      ((UINT8 *)tab)[i]=strtol(v,NULL,0);
    else if (type==UINTEGER16)
      /*((UINT16 *)tab)[i]=atoi(v);*/
      ((UINT16 *)tab)[i]=strtol(v,NULL,0);
    else
      ((UINT32 *)tab)[i]=strtol(v,NULL,0);
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
  int len=strlen("gngbrc")+strlen(getenv("HOME"))+strlen("/.gngb/")+1;

  filename=(char *)malloc(len);
  sprintf(filename,"%s/.gngb/gngbrc",getenv("HOME"));
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
	    /*	    *(UINT8 *) config_var[i].var=atoi(val);*/
	    *(UINT8 *) config_var[i].var=strtol(val,NULL,0);
	  else if (config_var[i].type==UINTEGER16)
	    /*	    *(UINT16 *) config_var[i].var=atoi(val);*/
	    *(UINT16 *) config_var[i].var=strtol(val,NULL,0);
	  else if (config_var[i].type==UINTEGER32)
	    *(UINT32 *) config_var[i].var=strtol(val,NULL,0);
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
  char c;
  while((c=getopt(argc,argv,"yrGSChasfj:O:oYdglc:"))!=EOF) {
    switch(c) {
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
#ifdef SDL_GL
    case 'O':
      sscanf(optarg,"%hdx%hd",&conf.gl_w,&conf.gl_h);
      conf.gl=1;break;
    case 'o' :
      conf.gl=1;break;
#else
    case 'o':
    case 'O':printf("Opengl mode not conpiled in\n");break; 
#endif
#ifdef SDL_YUV
    case 'Y':
      conf.yuv=1;
      break;
#else 
    case 'Y':printf("YUV mode not conpiled in\n");break;   
#endif

      /* For DEBUG */
    case 'd':conf.delay_int=1;break;
      /* Link */
    case 'l':conf.serial_on=1;gbserial_init(1,NULL);break;
    case 'c':conf.serial_on=1;gbserial_init(0,optarg);break;
    }
  }
  if (conf.yuv) conf.gl=0;
}

inline void update_key(void) {
  SDL_Event event;
  while(SDL_PollEvent(&event)) {
    switch (event.type) {
#ifdef SDL_YUV
    case SDL_VIDEORESIZE:
      if (conf.yuv) {
        conf.yuv_w=event.resize.w;
        conf.yuv_h=event.resize.h;
        reinit_vram();
      }
      break;
#endif


    case SDL_JOYAXISMOTION:
      if (conf.show_keycode) 
	  set_message("%d",event.jaxis.axis);
      break;
    case SDL_JOYBUTTONDOWN:
      if (conf.show_keycode) 
	  set_message("%d",event.jbutton.button);
      break;
    case SDL_KEYUP:
      key[event.key.keysym.sym]=0;
      //printf("key released %d\n",event.key.keysym.sym);
      break;
    case SDL_KEYDOWN:
      key[event.key.keysym.sym]=1;
      //printf("key pressed %d\n",event.key.keysym.sym);
      if (conf.show_keycode) 
	set_message("%d",event.key.keysym.sym);
      switch(event.key.keysym.sym) {
      case SDLK_ESCAPE: conf.gb_done=1;break;
      case SDLK_F9:switch_fullscreen();break;  // F9: swicth fullscreen
      case SDLK_F10:conf.show_fps^=1;break;     // F10: show fps (work only with -a)
      case SDLK_F11:emu_reset();set_message("Reset");break; // F11: Reset 
      case SDLK_F12:conf.show_keycode^=1;    // F12: shwo keycode 
	if (conf.show_keycode) 
	  set_message("Show keysym code : ON");
	else 
	  set_message("Show keysym code : OFF");
	break;
      case SDLK_KP1:
	set_message("Set Pal 1");
	gb_set_pal(0);
	break;
      case SDLK_KP2:
	set_message("Set Pal 2");
	gb_set_pal(1);
	break;
      case SDLK_KP3:
	set_message("Set Pal 3");
	gb_set_pal(2);
	break;
      case SDLK_KP4:
	set_message("Set Pal 4");
	gb_set_pal(3);
	break;
       case SDLK_KP5:
	set_message("Set Pal 5");
	gb_set_pal(4);
	break;      
      case SDLK_KP6:
	rb_on=1;
	break;   

    	/* FIXME : Save State are experimental */
      case SDLK_F1: 
      case SDLK_F2:
      case SDLK_F3:
      case SDLK_F4:
      case SDLK_F5:
      case SDLK_F6:
      case SDLK_F7:
      case SDLK_F8:
	if (event.key.keysym.mod==1) {
	  if (!save_state(event.key.keysym.scancode-67)) 
	    set_message("Save state %d",event.key.keysym.scancode-67);
	  else set_message("Error Save state");
	} else {
 	  if (!load_state(event.key.keysym.scancode-67)) 
	    set_message("Load state %d",event.key.keysym.scancode-67);
	  else set_message("Error Load state");
	}
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

void emu_run(void) {
  
}

void emu_reset(void) {
  gblcdc_reset();
  gbtimer_reset();
  gbcpu_reset();
  gbmemory_reset();
  //  if (conf.sound) gbsound_reset();
}

void emu_pause(void) {

}

void emu_quit(void) {
  if (rom_type&BATTERY) {
    save_ram();
    if (rom_type&TIMER) save_rom_timer();
  }
}

