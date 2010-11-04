#include <SDL/SDL.h>
#include <curses.h>
#include <readline/readline.h>
#include <panel.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../global.h"
#include "../emu.h"
#include "../memory.h"
#include "../cpu.h"
#include "../rom.h"
#include "../vram.h"
#include "../interrupt.h"
#include "../serial.h"
#include "../frame_skip.h"
#include "../emu.h"
#include "../sgb.h"
#include "../sound.h"
#include "debuger.h"
#include "log.h"

extern SDL_Joystick *joy;

static int last_cols,last_lines;

/* Geometrie du debugger
	
                      C	    C
		      O	    O
		      L	    L
       	       	      1	    2
      -----------------------------
      |	       	      |	    |	  |
      |	    CODE      |	STK | CPU |
      |	       	      |	    | 	  |
LGN1  |---------------------------|
      |	       	      |	      	  |
      |	    MEM	      |	   LCD 	  |
      |		      |	 	  |
      -----------------------------
 */

void update_all_win(void);
void refresh_all_win(void);

int COL1,COL2;
int LGN1;

/* DEBUG FUNCTION */

void next_inst(void) {
  stop_at_next();
  update_gb();
  update_all_win();
  refresh_all_win();
}

void animate(void) {
  int c;
  nodelay(panel_window(cur_win->p),TRUE);
  do {
    stop_at_next();
    update_gb();
    update_all_win();
    refresh_all_win();
    c=wgetch(panel_window(cur_win->p));
  }while(c!=27);
}

void run(void) {
  conf.gb_done=0;
  update_gb();
  if (conf.fs) switch_fullscreen();
  update_all_win();
  refresh_all_win();
}

/* WINDOW FUNTION */

void update_all_win(void) {
  MY_WIN *d=cur_win,*f=cur_win;
  if (d) {
    do {
      if (!panel_hidden(d->p) && d->update) d->update(d);
      d=d->next;
    } while(d!=f);
  }
  cur_win->update(cur_win);
}

void refresh_all_win(void) {
  wrefresh(panel_window(cur_win->p));
  update_panels();
  doupdate();
}

void resize_all_win(void) {
  
  COL1=COLS-26;
  COL2=COLS-15;
  LGN1=LINES/2;

  mem_info_win->resize(mem_info_win,LINES-LGN1,COL1);
  move_panel(mem_info_win->p,LGN1,0);

  code_info_win->resize(code_info_win,LGN1,COL1);
  move_panel(code_info_win->p,0,0);

  io_info_win->resize(io_info_win,LINES-LGN1,COLS-COL1);
  move_panel(io_info_win->p,LGN1,COL1);
  
  cpu_info_win->resize(cpu_info_win,LGN1,COLS-COL2);
  move_panel(cpu_info_win->p,0,COL2);
  
  update_all_win();
  refresh_all_win();
}

void next_win(void) {
  /*wattrset(panel_window(cur_win->p),COLOR_PAIR(COLOR_WHITE));
    draw_border_win(cur_win);
    wattroff(panel_window(cur_win->p),COLOR_PAIR(COLOR_RED));
    wrefresh(panel_window(cur_win->p));*/
  unactive_win(cur_win);

  cur_win=cur_win->next;
  while(panel_hidden(cur_win->p)) cur_win=cur_win->next;
  active_win(cur_win);
  /* wattrset(panel_window(cur_win->p),COLOR_PAIR(COLOR_RED));
     draw_border_win(cur_win);
     wattroff(panel_window(cur_win->p),COLOR_PAIR(COLOR_RED));
     wrefresh(panel_window(cur_win->p));
     top_panel(cur_win->p);*/
  update_all_win();
  refresh_all_win();
}

int main(int argc,char **argv) {
  int c;  int i=0;
  MEVENT mevent;

  /* GBInit */
  setup_default_conf();
  open_conf();
  check_option(argc,argv);
  if(optind >= argc)
    print_help();

  if (open_rom(argv[optind])) {
    fprintf(stderr,"Error while trying to read file %s \n",argv[optind]);
    exit(1);
  }

  gbmemory_init();
  gblcdc_init();
  gbtimer_init();
  gbcpu_init();
  init_vram((conf.fs?SDL_FULLSCREEN:0)|(conf.gl?SDL_OPENGL:0));
  SDL_ShowCursor(1);
  
  if (conf.gb_type&SUPER_GAMEBOY) sgb_init();

  if(SDL_NumJoysticks()>0){
    joy=SDL_JoystickOpen(conf.joy_no);
    if(joy) {
      printf("Name: %s\n", SDL_JoystickName(conf.joy_no));
      printf("Number of Axes: %d\n", SDL_JoystickNumAxes(joy));
      printf("Number of Buttons: %d\n", SDL_JoystickNumButtons(joy));
      printf("Number of Balls: %d\n", SDL_JoystickNumBalls(joy));
    }
  }

  open_log();

  /*if (conf.sound) {
    gbsound_init();
    update_sound_reg();
    }*/

  /* ncurses init*/
  
  initscr();

  if (has_colors())    {
        start_color();         /*
         * Simple color assignment, often all we need.
         */
        init_pair(COLOR_BLACK, COLOR_BLACK, COLOR_BLACK);
        init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
        init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
        init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
        init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
        init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
  }
  
 
  nonl();         /* tell curses not to do NL->CR/NL on output */
  cbreak();       /* take input chars one at a time, no wait for \n */
  noecho();       /* don't echo input */
  mousemask(ALL_MOUSE_EVENTS|REPORT_MOUSE_POSITION,NULL);
  
  last_cols=COLS;
  last_lines=LINES;

  COL1=COLS-26;
  COL2=COLS-15;
  LGN1=LINES/2;
    
  init_code_info();
  init_cpu_info();
  init_mem_info();
  init_io_info();
  init_vram_info();
  init_msg_info();

  next_win();
  update_all_win();
  refresh_all_win();

  do {
    c=wgetch(panel_window(cur_win->p));
    
    if (c==ERR) continue;
    if (c==KEY_MOUSE) {
      getmouse(&mevent);
      if (cur_win->mouse_event) cur_win->mouse_event(cur_win,&mevent);
    } 
    
    if (cur_win->key_pressed) 
      if (cur_win->key_pressed(cur_win,c)) continue;
    
    if (c==KEY_RESIZE) resize_all_win();    
    else if (c==KEY_F(7)) next_inst();
    else if (c==KEY_F(9)) run();
    else if (c=='a') animate();
    // vram
    else if (c=='v') show_buffer();
    else if (c=='c') show_curline();
    else if (c=='b') {vram_info_show("BACK");refresh_all_win();}
    else if (c=='t') {vram_info_show("TILES");refresh_all_win();}
    else if (c=='p') {vram_info_show("PAL");refresh_all_win();}

    else if (c=='m') active_msg^=1;      
    else if (c=='M') {
      if (panel_hidden(msg_info_win->p)) show_panel(msg_info_win->p);
      else hide_panel(msg_info_win->p);
      if (cur_win==msg_info_win) next_win();
      refresh_all_win();
    }
    else if (c=='s') add_msg("truc bidule chouette %d",i++);
    
    else if (c=='\t') next_win();
    else if (cur_win->key_pressed) cur_win->key_pressed(cur_win,c);
    //refresh_all_win();
  }while(c!='q'); 

  close_log();

  endwin();
  return 0;
}
