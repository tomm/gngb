#include "debuger.h"
//#include "cpu_info.h"
#include "../cpu.h"
//#include "win.h"

extern int LGN1,COL2;

void cpu_win_update(MY_WIN *w);
void cpu_win_resize(MY_WIN *w,int l,int c);

MY_WIN *cpu_info_win=NULL;

void init_cpu_info(void) {
  WINDOW *w;

  cpu_info_win=new_mywin();
  w=newwin(LGN1,COLS-COL2,0,COL2);
  cpu_info_win->p=new_panel(w);

  cpu_info_win->name=strdup("CPU");
  draw_border_win(cpu_info_win);
  
  // Function callback
  cpu_info_win->update=cpu_win_update;
  cpu_info_win->resize=cpu_win_resize;

  keypad(w,TRUE);
  
  //wrefresh(cpu_info_win->w);
  add2win_list(cpu_info_win);
}

void cpu_win_update(MY_WIN *w) {
  char s[40];
  WINDOW *win=panel_window(w->p);

  mvwprintw(win,1,1,"AF    %04x",gbcpu->af.w);
  mvwprintw(win,2,1,"BC    %04x",gbcpu->bc.w);
  mvwprintw(win,3,1,"HL    %04x",gbcpu->hl.w);
  mvwprintw(win,4,1,"DE    %04x",gbcpu->de.w);
  mvwprintw(win,5,1,"Flag: %c%c%c%c",
	    ((gbcpu->af.b.l)&FLAG_Z)?'Z':'z',
	    ((gbcpu->af.b.l)&FLAG_N)?'N':'n', 
	    ((gbcpu->af.b.l)&FLAG_H)?'H':'h',
	    ((gbcpu->af.b.l)&FLAG_C)?'C':'c');
  mvwprintw(win,6,1,"PC    %04x",gbcpu->pc.w);
  mvwprintw(win,7,1,"SP    %04x",gbcpu->sp.w);
  if (gbcpu->state==HALT_STATE) 
    mvwaddstr(win,8,1,"HALT");
  else mvwaddstr(win,8,1,"RUN");
  if (gbcpu->int_flag)
    mvwaddstr(win,8,7,"EI");
  else mvwaddstr(win,8,7,"DI");
  //wrefresh(win);
}

void cpu_win_resize(MY_WIN *w,int l,int c) {
  wresize(panel_window(w->p),l,c);
}
