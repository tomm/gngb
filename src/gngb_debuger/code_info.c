//#include "code_info.h"
#include "debuger.h"
#include "../memory.h"
#include "../cpu.h"
#include "op.h"
//#include "break.h"

extern int COL1,LGN1;

static UINT16 cur_add=0x100;
static UINT16 beg_add=0x100;
static UINT16 last_add=0x100;
static UINT16 max_add=0xfff0;
static INT8 cur_add_pos=0;
static int nb_line;
char *op_size;

MY_WIN *code_info_win=NULL;

void code_win_update(MY_WIN *w);
int code_win_key_pressed(MY_WIN *w,int key_code);
int code_win_mouse_event(MY_WIN *w,MEVENT *e);
void code_win_resize(MY_WIN *w,int l,int c);

void init_code_info(void) {
  WINDOW *w;
  
  code_info_win=new_mywin();
  w=newwin(LGN1,COL1,0,0);

  code_info_win->p=new_panel(w);
  
  code_info_win->name=strdup("CODE");
  draw_border_win(code_info_win); 
 
  // function
  code_info_win->key_pressed=code_win_key_pressed;
  code_info_win->mouse_event=code_win_mouse_event;
  code_info_win->update=code_win_update;
  code_info_win->resize=code_win_resize;
  

  keypad(w,TRUE);
  
  //  wrefresh(code_info_win->w);
  add2win_list(code_info_win);

  nb_line=LGN1-2;
  op_size=(char *)malloc(LGN1-2);
}

int code_win_key_pressed(MY_WIN *w,int key_code) {
  int i;
  
  switch(key_code) {
  case KEY_HOME:beg_add=cur_add=0x100;cur_add_pos=0;break;
  case KEY_END:beg_add=cur_add=max_add;cur_add_pos=0;break;
  case KEY_DOWN:
    cur_add+=get_nb_byte(mem_read(cur_add));
    if (cur_add>last_add) beg_add+=get_nb_byte(mem_read(beg_add));
    else cur_add_pos++;
    break;
  case KEY_UP:
    cur_add_pos--;
    if (cur_add_pos>0)
      cur_add-=op_size[cur_add_pos];
    else {
      cur_add_pos=0;
      cur_add--;
      beg_add=cur_add;
    }
    break;
  case KEY_NPAGE:
    beg_add=last_add;
    cur_add=beg_add;
    for(i=0;i<cur_add_pos;i++)
      cur_add+=get_nb_byte(mem_read(cur_add));
    break;
  case KEY_PPAGE:
    beg_add-=nb_line;
    cur_add=beg_add;
    for(i=0;i<cur_add_pos;i++)
      cur_add+=get_nb_byte(mem_read(cur_add));
    break;
  case KEY_F(2):
    if (is_break_point(cur_add)) 
      del_break_point(cur_add);
    else add_break_point(cur_add);
    break;
  default:return FALSE;
  }
  
  /*if (cur_add<0) cur_add=0;
    if (cur_add>max_add) cur_add=max_add;
    if (cur_add<beg_add) beg_add=cur_add;
    if (cur_add>=last_add) beg_add+=get_nb_byte(mem_read(beg_add));*/

  code_info_win->update(code_info_win);
  return TRUE;
}

int code_win_mouse_event(MY_WIN *w,MEVENT *e) {
  return FALSE;
}

void code_win_update(MY_WIN *w) {
  static UINT16 old_pc=0x100;
  int i;
  UINT16 add;
  UINT16 l;
  UINT8 id;
  char s[100];
  char t[10];
  int att_on=0;
  int att_off=0;

  WINDOW *win=panel_window(w->p);

  if (old_pc!=gbcpu->pc.w && (gbcpu->pc.w<beg_add || gbcpu->pc.w>last_add)) {
    beg_add=cur_add=old_pc=gbcpu->pc.w;
    cur_add_pos=0;
  }    
  
  add=beg_add;
  for(i=1;i<LGN1-1;i++) {
    /*    if (add==cur_add) 
      wattron(win,A_REVERSE|COLOR_PAIR(COLOR_WHITE));
    else {
      wattroff(win,A_REVERSE|COLOR_PAIR(COLOR_GREEN));
      if (add==gbcpu->pc.w) 
	wattron(win,A_REVERSE|COLOR_PAIR(COLOR_GREEN));
      else {
	wattroff(win,A_REVERSE|COLOR_PAIR(COLOR_GREEN));
	if (is_break_point(add)) 
	  wattron(win,A_REVERSE|COLOR_PAIR(COLOR_RED));
	else wattroff(win,A_REVERSE|COLOR_PAIR(COLOR_RED));
      }  
      }*/
    att_on=0;
    att_off=0;
    if (add==cur_add) att_on|=A_REVERSE|COLOR_PAIR(COLOR_BLUE);
    else att_off|=A_REVERSE|COLOR_PAIR(COLOR_WHITE);
    if (add==gbcpu->pc.w) att_on|=A_REVERSE|COLOR_PAIR(COLOR_GREEN);
    else att_off|=A_REVERSE|COLOR_PAIR(COLOR_GREEN);
    if (is_break_point(add)) att_on|=A_REVERSE|COLOR_PAIR(COLOR_RED);
    else att_off|=A_REVERSE|COLOR_PAIR(COLOR_RED);

    wattroff(win,att_off);
    wattron(win,att_on);

    mvwhline(win,i,1,' ',COL1-2);
    get_mem_id(add,t);
    l=add;
    id=mem_read(add);
    add+=aff_op(id,add,s);
    mvwprintw(win,i,1,"%s %04x %02x %s",t,l,id,s);
    add++;
    op_size[i-1]=add-l;
  }
  last_add=add-1;
  //wrefresh(win);
}

void code_win_resize(MY_WIN *w,int l,int c) {
  wresize(panel_window(w->p),l,c);
  wclear(panel_window(w->p));
  draw_border_win(w);
  nb_line=l-2;
  op_size=(char *)realloc(op_size,l-2);
}



