#include <form.h>
#include "debuger.h" 
#include "../cpu.h"
#include "../memory.h"

// My_Win

MY_WIN *win_list=NULL;
MY_WIN *cur_win=NULL;

MY_WIN *new_mywin(void) {
  MY_WIN *w=(MY_WIN *)malloc(sizeof(MY_WIN));
  w->p=NULL;
  w->update=NULL;
  w->key_pressed=NULL;
  w->mouse_event=NULL;
  w->resize=NULL;
  w->next=NULL;
}

void add2win_list(MY_WIN *w) {
  if (!win_list) {
    win_list=w;
    win_list->next=win_list;
    cur_win=win_list;
  } else {
    w->next=win_list->next;
    win_list->next=w;
  }
}

void del2win_list(MY_WIN *w) {


}

void draw_border_win(MY_WIN *w) {
  wborder(panel_window(w->p),0,0,0,0,0,0,0,0);
  if (w->name)
    mvwaddstr(panel_window(w->p),0,2,w->name);
}

void active_win(MY_WIN *w) {
  wattrset(panel_window(w->p),COLOR_PAIR(COLOR_RED));
  draw_border_win(w);
  wattroff(panel_window(w->p),COLOR_PAIR(COLOR_RED));
  wrefresh(panel_window(w->p));
  top_panel(w->p);
}

void unactive_win(MY_WIN *w) {
  wattrset(panel_window(w->p),COLOR_PAIR(COLOR_WHITE));
  draw_border_win(w);
  wattroff(panel_window(w->p),COLOR_PAIR(COLOR_RED));
  wrefresh(panel_window(w->p));
}

// go at

static FORM *form_go_at=NULL;
static PANEL *panel_go_at=NULL;

FIELD *tab_field[2];/*={
  new_field(1,10,1,1,0,0),
  NULL};*/

void init_go_at(void) {
  /*FIELD *tab_field[]={
    new_field(1,10,1,1,0,0),
    NULL};*/
  
  

  WINDOW *w=newwin(3,12,0,0),*ww;
  keypad(w,TRUE);
  wattron(w,COLOR_PAIR(COLOR_RED));
  wborder(w,0,0,0,0,0,0,0,0);
  mvwaddstr(w,0,2,"Go At");
  wattroff(w,COLOR_PAIR(COLOR_RED));

  tab_field[0]=new_field(1,10,1,1,0,1);
  tab_field[1]=NULL;

  add_msg("field %p",tab_field[0]);
  //set_field_type(tab_field[0],TYPE_INTEGER,4,0x00,0xffff);
  panel_go_at=new_panel(w);
  hide_panel(panel_go_at);

  form_go_at=new_form(tab_field);
  set_current_field(form_go_at,tab_field[0]);
  add_msg("field %p",tab_field[0]);
  set_form_win(form_go_at,w);

  ww=subwin(w,1,10,1,1);
  keypad(ww,TRUE);
  set_form_sub(form_go_at,ww);
}

UINT16 go_at(int x,int y) {
  int c;
  if (!form_go_at) init_go_at();
  show_panel(panel_go_at);
  add_msg("avant post field %p",tab_field[0]);
  add_msg("avant post field2 %p",current_field(form_go_at));
  post_form(form_go_at);
  set_current_field(form_go_at,tab_field[0]);
  add_msg("apres post field %p",tab_field[0]);
  add_msg("apres post field2 %p",current_field(form_go_at));
  add_msg("field index %d", field_index(tab_field[0]));
  wrefresh(form_win(form_go_at));
  wrefresh(form_sub(form_go_at));
  do {
    c=getch();
    //c=wgetch(form_sub(form_go_at));
    //c=form_driver(form_go_at,REQ_VALIDATION);
    if (c=='l') form_driver(form_go_at,REQ_PREV_CHAR);
    else if (c=='r') form_driver(form_go_at,REQ_NEXT_CHAR);
    else if (c==27) form_driver(form_go_at,REQ_VALIDATION);
    //else waddch(form_sub(form_go_at),c);
      wrefresh(form_sub(form_go_at));

  }while(c!=27);
  add_msg("field %p",current_field(form_go_at));
  add_msg("go at %s",field_buffer(current_field(form_go_at),0));
  unpost_form(form_go_at);
  hide_panel(panel_go_at);
  update_panels();
}

// BreakPoint

static int one_inst=0;
static BREAK_POINT *break_list=NULL;

void add_break_point(UINT16 add) {
  BREAK_POINT *b;

  b=(BREAK_POINT *)malloc(sizeof(BREAK_POINT));
  b->next=NULL;
  b->add=add;


  if (!break_list) {
    break_list=b;
  } else {
    b->next=break_list;
    break_list=b;
  }
}

int is_break_point(UINT16 add) {
  BREAK_POINT *b;
  for(b=break_list;b && b->add!=add;b=b->next);
  return (b!=NULL);
}

void del_break_point(UINT16 add) {
  BREAK_POINT *b,*p;
  for(b=break_list,p=NULL;b && b->add!=add;p=b,b=b->next);
  if (b) {
    if (p)
      p->next=b->next;
    else break_list=b->next;
    free(b);
  }  
}

void stop_at_next(void) {
  one_inst=1;
}

int continue_run(void) {
  if (is_break_point(gbcpu->pc.w)) return 0;
  if (one_inst) {
    one_inst=0;
    return 0;
  }
  return 1;
}

void get_mem_id(UINT16 adr,char *ret) {
  UINT8 bank;

  ret[0]=0;
  if (adr>=0xfe00 && adr<0xfea0) {
    strcat(ret,"OAM");
    return;
  }

  if (adr>=0xfea0) {
    strcat(ret,"IO");
    return;
  }

  if (adr>=0xe000 && adr<0xfe00) adr-=0x2000;  // echo mem

  bank=(adr&0xf000)>>12;
  switch(bank) {
  case 0x00:
  case 0x01:
  case 0x02:
  case 0x03:
    strcat(ret,"ROM0");
    return;
    
  case 0x04:
  case 0x05:
  case 0x06:
  case 0x07:
    strcat(ret,"ROM");
    sprintf(&ret[3],"%d",active_rom_page);
    return;
    
  case 0x08:
  case 0x09:
    strcat(ret,"VRAM");
    sprintf(&ret[4],"%d",active_vram_page);
    return;
    
  case 0x0a:
  case 0x0b:
    strcat(ret,"RAM");
    sprintf(&ret[3],"%d",active_ram_page);
    return;

  case 0x0c:
    strcat(ret,"WRAM0");
    return;
    
  case 0x0d:
    strcat(ret,"WRAM");
    sprintf(&ret[4],"%d",active_wram_page);
    return;
    
  }
}


