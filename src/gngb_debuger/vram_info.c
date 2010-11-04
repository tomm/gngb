#include "debuger.h"
//#include "vram_info.h"
#include "../vram.h"
#include "../memory.h"
#include "../emu.h"
#//include "../message.h"
#include <panel.h>
//#include "win.h"

#define NONE -1
#define MODE_BACK 1
#define MODE_TILE 2
#define MODE_PAL 3

static int mode=NONE;

// tile info

int nb_tile=0;
UINT8 xflip=0,yflip=0;

MY_WIN *vram_info_win=NULL;

void vram_update(MY_WIN *w);

void init_vram_info(void) {
  WINDOW *w;

  mode=NONE;

  vram_info_win=new_mywin();
  w=newwin(20,20,0,0);
  vram_info_win->p=new_panel(w);
    
  vram_info_win->update=vram_update;
  vram_info_win->name=strdup("VRAM");
  draw_border_win(vram_info_win);

  hide_panel(vram_info_win->p);

  add2win_list(vram_info_win);
}

void vram_update(MY_WIN *w) {
  WINDOW *win=panel_window(w->p);

  switch(mode) {
  case MODE_TILE:
    mvwprintw(win,1,1,"NbTile: %d  ",nb_tile);
    break;
  case MODE_BACK:
    mvwprintw(win,1,1,"NbTile: %d  ",nb_tile);
    mvwprintw(win,2,1,"XFlip:  %d  ",xflip);
    mvwprintw(win,3,1,"YFlip:  %d  ",yflip);
    break;

  }
  wrefresh(win);
}

/* INFO BUF */

void clear_buf(void) {
  SDL_FillRect(gb_screen,NULL,0);
}

void draw_pixel(SDL_Surface *s,int x,int y,UINT16 c) {
  *(((UINT16 *)(s->pixels)+y*s->w+x))=c;
}

void draw_rect_fill(SDL_Surface *s,int x,int y,int w,int l,UINT16 c) {
  int i,j;
  for(j=y;j<y+l;j++)
    for(i=x;i<x+w;i++)
      *(((UINT16 *)(s->pixels)+j*s->w+i))=c;
}

void draw_hline(SDL_Surface *s,int y,UINT16 c) {
  int i;
  for(i=0;i<s->w;i++)
    *(((UINT16 *)(s->pixels)+y*s->w+i))=c;
}

static struct mask_shift {
  unsigned char mask;
  unsigned char shift;
} tab_ms[8]={
  { 0x80,7 },
  { 0x40,6 },
  { 0x20,5 },
  { 0x10,4 },
  { 0x08,3 },
  { 0x04,2 },
  { 0x02,1 },
  { 0x01,0 }};

void draw_tile(SDL_Surface *s,UINT8 *tile,int x,int y,int xflip,int yflip,UINT16 *p) {
  int i,j;
  UINT8 *t;
  unsigned char c,bit0,bit1;
  for(j=0;j<8;j++) {
    if (!yflip) t=tile+(j<<1);
    else t=tile+((7-(j&0x07))<<1);  
    for(i=0;i<8;i++) {
      int wbit;
      if (!xflip) wbit=i;
      else wbit=7-i;
      bit0=((*t)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      bit1=((*(t+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      c=(bit1<<1)|bit0;
      draw_pixel(s,x+i,y+j,p[c]);
    }
  }
}

void show_pal(void) {
  int i,j;
  mode=MODE_PAL;
  for(j=0;j<8;j++)
    for(i=0;i<4;i++) {
      draw_rect_fill(gb_screen,i*20,j*20,16,16,pal_col_bck[j][i]);
      draw_rect_fill(gb_screen,i*20+100,j*20,16,16,pal_col_obj[j][i]);
    }
}

void show_tiles_wb(void) {
  int i,x=0,y=0;
  for(i=0;i<384;i++) {
    draw_tile(gb_screen,&vram_page[0][i<<4],x,y,0,0,grey);
    x=(x+10)%(DEBUG_SCR_X-10);
    y+=(!x)?10:0;
  }
}

void show_tiles_col(void) {
  int i,x=0,y=0;
  for(i=0;i<384;i++) {
    draw_tile(gb_screen,&vram_page[0][i<<4],x,y,0,0,grey);
    x=(x+10)%(DEBUG_SCR_X);
    y+=(!x)?10:0;
  }
  
  for(i=0;i<384;i++) {
    draw_tile(gb_screen,&vram_page[1][i<<4],x,y,0,0,grey);
    x=(x+10)%(DEBUG_SCR_X);
    y+=(!x)?10:0;
  }
}

void show_tiles(void) {
  mode=MODE_TILE;
  if (conf.gb_type&COLOR_GAMEBOY) show_tiles_col();
  else show_tiles_wb();
}

void show_buffer(void) {
  clear_buf();
  blit_screen();
}

void show_curline(void) {
  show_buffer();
  draw_hline(gb_screen,CURLINE,0xffff);
  SDL_Flip(gb_screen);
}

void show_back_col(void) {
  int i,j;
  UINT8 *tb,*att_tb,*tile;
  int no_tile;
  int att;

  if (LCDCCONT&0x08) {// select Tile Map
    tb=&vram_page[0][0x1c00];
    att_tb=&vram_page[1][0x1c00];
  } else {
    tb=&vram_page[0][0x1800];
    att_tb=&vram_page[1][0x1800];
  }

  for(j=0;j<32;j++)
    for(i=0;i<32;i++) {
      no_tile=*tb++;
      att=*att_tb++;
      if (!(LCDCCONT&0x10)) no_tile=256+(signed char)no_tile;
      tile=&vram_page[(att&0x08)>>3][(no_tile<<4)];
      draw_tile(gb_screen,tile,i*8,j*8,att&0x20,att&0x40,pal_col_bck[att&0x07]);
    }
}

void show_back_wb(void) {
  int i,j;
  UINT8 *tb,*tile;
  int no_tile;
  

  if (LCDCCONT&0x08)  tb=&vram_page[0][0x1c00];
  tb=&vram_page[0][0x1800];
  
  for(j=0;j<32;j++)
    for(i=0;i<32;i++) {
      no_tile=*tb++;
      if (!(LCDCCONT&0x10)) no_tile=256+(signed char)no_tile;
      tile=&vram_page[0][(no_tile<<4)];
      draw_tile(gb_screen,tile,i*8,j*8,0,0,grey);
    }
}

void show_back(void) {
  mode=MODE_BACK;
  if (conf.gb_type&COLOR_GAMEBOY) show_back_col();
  else show_back_wb();
}

void set_tile_info(int x,int y) {
  UINT8 *tb;
  UINT8 *att_tb;
  int t;

  if (LCDCCONT&0x08)  tb=&vram_page[0][0x1c00];
  tb=&vram_page[0][0x1800];

  if (conf.gb_type&COLOR_GAMEBOY) {
    if (LCDCCONT&0x08)  att_tb=&vram_page[1][0x1c00];
    att_tb=&vram_page[1][0x1800];
  }

  t=(y/8)*32+(x/8);
  nb_tile=tb[t];
  if (conf.gb_type&COLOR_GAMEBOY) {
    xflip=(att_tb[t]&0x20)?1:0;
    yflip=(att_tb[t]&0x40)?1:0;
  } else xflip=yflip=0;
  
}

void do_vram_info_event_loop(void) {
  SDL_Event event;
  char done=0;

  while(!done) {
    while(SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_MOUSEMOTION:
	if (mode==MODE_TILE) nb_tile=(event.motion.y/10)*(DEBUG_SCR_X/10)+(event.motion.x/10);
	if (mode==MODE_BACK) set_tile_info(event.motion.x,event.motion.y);
	vram_info_win->update(vram_info_win);
	break;
      
      case SDL_KEYDOWN:
	switch(event.key.keysym.sym) {
	case SDLK_ESCAPE: done=1;break;
	default:break;
	}
	break;
      }
    }
  }
}

void vram_info_show(char *id) {
  clear_buf();
  wclear(panel_window(vram_info_win->p));
  draw_border_win(vram_info_win);
  top_panel(vram_info_win->p);
  if (!strcmp(id,"TILES")) show_tiles();
  else if (!strcmp(id,"PAL")) show_pal();
  else if (!strcmp(id,"BACK")) show_back();
  SDL_Flip(gb_screen);
  do_vram_info_event_loop();
  clear_buf();
  blit_screen();
  hide_panel(vram_info_win->p);
}








