#include "debuger.h"
#include "../cpu.h"

#define BUF_SIZE 500
#define MSG_LEN 50

MY_WIN *msg_info_win=NULL;
int active_msg=0;
static int cur_line=1;
static WINDOW *pad=NULL;

char **msg_buf=NULL;
static int beg_msg=0;
static int cur_msg=0;
static int nb_line;


int msg_win_key_pressed(MY_WIN *w,int key_code);
void msg_win_update(MY_WIN *w);

void init_msg_info(void) {
  WINDOW *w;
  int i;
  
  msg_info_win=new_mywin();
  w=newwin(10,50,10,10);
  msg_info_win->p=new_panel(w);

  msg_info_win->name=strdup("MSG");
  draw_border_win(msg_info_win);

  msg_buf=(char **)malloc(sizeof(char *)*BUF_SIZE);
  for(i=0;i<BUF_SIZE;i++) {
    msg_buf[i]=(char *)malloc(sizeof(char)*MSG_LEN);
    msg_buf[i][0]=0;
  }
  

  nb_line=8;

  // callback

  msg_info_win->key_pressed=msg_win_key_pressed;
  msg_info_win->update=msg_win_update;  

  keypad(w,TRUE);
  
  add2win_list(msg_info_win);
}

void add_msg(const char *format,...) {
  WINDOW *w=panel_window(msg_info_win->p);
  va_list pvar;
  va_start(pvar,format);
  
  msg_buf[cur_msg][0]=0;

  sprintf(msg_buf[cur_msg],"%04x ",gbcpu->pc.w);

  vsprintf(&msg_buf[cur_msg][5],format,pvar);
  cur_msg=(cur_msg+1)%BUF_SIZE;
 
  msg_info_win->update(msg_info_win); 

  va_end(pvar);
}

void msg_win_update(MY_WIN *w) {
  int n=beg_msg;
  int i;
  WINDOW *win=panel_window(w->p);

  for(i=1;i<=nb_line;i++,n=(n+1)%BUF_SIZE) {
    if ((n+1)%BUF_SIZE==cur_msg)
      wattron(win,COLOR_PAIR(COLOR_RED));
    else  wattroff(win,COLOR_PAIR(COLOR_RED));
    mvwhline(win,i,1,' ',48);
    mvwaddstr(win,i,1,msg_buf[n]);
  }
  //wrefresh(win);
}

int msg_win_key_pressed(MY_WIN *w,int key_code) {
  switch(key_code) {
  case KEY_DOWN:
    beg_msg=(beg_msg+1)%BUF_SIZE;
    break;
  case KEY_UP:
    beg_msg--;
    if (beg_msg<0) beg_msg=BUF_SIZE;
    break;    
  default:return FALSE;
  }
  w->update(w); 
  return TRUE;
}
