//#include "mem_info.h"
#include "debuger.h"
#include "../memory.h"
#include <form.h>

extern int COL1,LGN1;

static int cur_add=0;
static int add_max;

MY_WIN *mem_info_win=NULL;

void mem_win_update(MY_WIN *w);
int mem_win_key_pressed(MY_WIN *w,int key_code);
void mem_info_resize(MY_WIN *w,int l,int c);

void init_mem_info(void) {
  WINDOW *w;
  
  mem_info_win=new_mywin();
  w=newwin(LINES-LGN1,COL1,LGN1,0);
  mem_info_win->p=new_panel(w);
 
  add_max=0x10000-(LINES-LGN1-2)*0x10; 

  mem_info_win->name=strdup("MEM");
  draw_border_win(mem_info_win);

  // function
  
  mem_info_win->key_pressed=mem_win_key_pressed;
  mem_info_win->update=mem_win_update;
  mem_info_win->resize=mem_info_resize; 
  
  keypad(w,TRUE);
  
  add2win_list(mem_info_win);
}

int mem_win_key_pressed(MY_WIN *w,int key_code) {
  int c;
  
  switch(key_code) {
  case KEY_HOME:cur_add=0x0;break;
  case KEY_END:cur_add=add_max;break;
  case KEY_DOWN:cur_add+=0x10;break;
  case KEY_UP:cur_add-=0x10;break;
  case KEY_NPAGE:cur_add+=0xa0;break;
  case KEY_PPAGE:cur_add-=0xa0;break;
  case 'G':go_at(10,10);break;
  default:return FALSE;
  }
  
  if (cur_add<0) cur_add=0;
  if (cur_add>add_max) cur_add=add_max;

  mem_info_win->update(mem_info_win);
  return TRUE; 
}

void mem_win_update(MY_WIN *w) {
  int i,j;
  int add=cur_add;
  char t[10];
  int l;
   
  for(j=1;j<LINES-LGN1-1;j++) {
    mvwhline(panel_window(mem_info_win->p),j,1,' ',COL1-2);
    get_mem_id(add,t);
    mvwprintw(panel_window(mem_info_win->p),j,1,"%s",t);
    mvwprintw(panel_window(mem_info_win->p),j,l=(strlen(t)+2),"%04x",add);
    l+=5;
    for(i=0;i<16;i++)
      mvwprintw(panel_window(mem_info_win->p),j,l+3*i,"%02x",mem_read(add++));
  }
}

void mem_info_resize(MY_WIN *w,int l,int c) {
  wresize(panel_window(w->p),l,c);
  wclear(panel_window(w->p));
  draw_border_win(w);
  add_max=0x10000-(l-2)*0x10; 
}


