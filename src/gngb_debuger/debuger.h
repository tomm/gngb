#ifndef DEBUGER_H
#define DEBUGER_H

#include <panel.h>
#include "../global.h"

typedef struct _win {
  PANEL *p;
  char *name;
  void (*update)(struct _win *w);
  int (*key_pressed)(struct _win *w,int key_code);
  int (*mouse_event)(struct _win *w,MEVENT *event);
  void (*resize)(struct _win *w,int l,int c);
  struct _win *next;
}MY_WIN;

extern MY_WIN *cur_win;

extern MY_WIN *code_info_win;
extern MY_WIN *io_info_win;
extern MY_WIN *mem_info_win;
extern MY_WIN *vram_info_win;
extern MY_WIN *cpu_info_win;
extern MY_WIN *msg_info_win;

void init_cpu_info(void);
void init_vram_info(void);
void init_mem_info(void);
void init_io_info(void);
void init_code_info(void);
void init_msg_info(void);

/* My_Win function */

MY_WIN *new_mywin(void);
void add2win_list(MY_WIN *w);
void del2win_list(MY_WIN *w);
void draw_border_win(MY_WIN *w);
void active_win(MY_WIN *w);
void unactive_win(MY_WIN *w);

UINT16 go_at(int x,int y);

/* VRAM */

void vram_info_show(char *id);

/* BREAKPOINT */

typedef struct _break_point {
  UINT16 add;
  struct _break_point *next;
}BREAK_POINT;

void add_break_point(UINT16 add);
int is_break_point(UINT16 add);
void del_break_point(UINT16 add);

void stop_at_next(void);
int continue_run(void);

/* Message */

extern int active_msg;
void add_msg(const char *format,...);

void get_mem_id(UINT16 add,char *ret);

#endif


