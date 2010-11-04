#include "debuger.h"
//#include "io_info.h"
#include "../interrupt.h"
#include "../memory.h"
//#include "win.h"

extern int LGN1,COL1;

int cur_reg=0;
int nb_line;
int nb_reg;

void show_separator(WINDOW *w,int i);
void show_separator_color(WINDOW *w,int i);
void show_cycle(WINDOW *w,int i);
void show_gdma_cycle(WINDOW *w,int i);
void show_lcdcstat(WINDOW *w,int i);
void show_nbspr(WINDOW *w,int i);

struct {
  char type;
  union {
    struct {
      char *str;
      //UINT8 *add_val;
      UINT16 add;
    } str_val;
    void (*show_str)(WINDOW *w,int i);
  } str;
} io_reg[] = {
  {0,{show_cycle}},
  {0,{show_separator}},
  {1,{{"P1         %02x",0xff00}}},
  {1,{{"INT_ENABLE %02x",0xffff}}},
  {1,{{"INT_FLAG   %02x",0xff0f}}},
  {0,{show_separator}},
  {1,{{"SCRX       %02x",0xff43}}},
  {1,{{"SCRY       %02x",0xff42}}},
  {1,{{"WINX       %02x",0xff4b}}},
  {1,{{"WINY       %02x",0xff4a}}},
  {1,{{"CURLINE    %02x",0xff44}}},
  {1,{{"CMPLINE    %02x",0xff45}}},
  {1,{{"LCDCCONT   %02x",0xff40}}},
  {1,{{"LCDCSTAT   %02x",0xff41}}},
  {0,{show_lcdcstat}},
  {0,{show_nbspr}},
  {0,{show_separator}},
  {1,{{"BGPAL      %02x",0xff47}}},
  {1,{{"OBJ0PAL    %02x",0xff48}}},
  {1,{{"OBJ1PAL    %02x",0xff49}}},
  {0,{show_separator}},
  {1,{{"DIV        %02x",0xff04}}},
  {1,{{"TCOUNTER   %02x",0xff05}}},
  {1,{{"TMOD       %02x",0xff06}}},
  {1,{{"TCONTROL   %02x",0xff07}}},
  {0,{show_separator_color}},
  {1,{{"VRAM_BANK  %02x",0xff4f}}},
  {1,{{"WRAM_BANK  %02x",0xff70}}},
  {0,{show_separator}},
  {1,{{"HDMA1      %02x",0xff51}}},
  {1,{{"HDMA2      %02x",0xff52}}},
  {1,{{"HDMA3      %02x",0xff53}}},
  {1,{{"HDMA4      %02x",0xff54}}},
  {1,{{"HDMA5      %02x",0xff55}}},
  {0,{show_gdma_cycle}},
  {0,{show_separator}},
  {1,{{"BGPALSPE   %02x",0xff68}}},
  {1,{{"BGPALDATA  %02x",0xff69}}},
  {1,{{"OBJPALSPE  %02x",0xff6a}}},
  {1,{{"OBJPALDATA %02x",0xff6b}}},
  {0,{show_separator}},
  {1,{{"NR10       %02x",0xff10}}},
  {1,{{"NR11       %02x",0xff11}}},
  {1,{{"NR12       %02x",0xff12}}},
  {1,{{"NR13       %02x",0xff13}}},
  {1,{{"NR14       %02x",0xff14}}},
  {0,{show_separator}},
  {1,{{"NR21       %02x",0xff16}}},
  {1,{{"NR22       %02x",0xff17}}},
  {1,{{"NR23       %02x",0xff18}}},
  {1,{{"NR24       %02x",0xff19}}},
  {0,{show_separator}},
  {1,{{"NR30       %02x",0xff1a}}},
  {1,{{"NR31       %02x",0xff1b}}},
  {1,{{"NR32       %02x",0xff1c}}},
  {1,{{"NR33       %02x",0xff1d}}},
  {1,{{"NR34       %02x",0xff1e}}},
  {0,{show_separator}},
  {1,{{"NR41       %02x",0xff20}}},
  {1,{{"NR42       %02x",0xff21}}},
  {1,{{"NR43       %02x",0xff22}}},
  {1,{{"NR44       %02x",0xff23}}},
  {0,{show_separator}},
  {1,{{"NR50       %02x",0xff24}}},
  {1,{{"NR51       %02x",0xff25}}},
  {1,{{"NR52       %02x",0xff26}}},  
  {-1,NULL}};


typedef struct {
  char *str;
  UINT8 *val_add;
}IO_REG_INFO;


MY_WIN *io_info_win=NULL;

void io_win_update(MY_WIN *w);
int io_win_key_pressed(MY_WIN *w,int key_code);
void io_win_resize(MY_WIN *w,int l,int c);



void init_io_info(void) {
  WINDOW *w;
  
  io_info_win=new_mywin();
  w=newwin(LINES-LGN1,COLS-COL1,LGN1,COL1);
  io_info_win->p=new_panel(w);

  io_info_win->name=strdup("IO");
  draw_border_win(io_info_win);

  
  
  // Function

  nb_line=LINES-LGN1-2;
  for(nb_reg=0;io_reg[nb_reg].type!=-1;nb_reg++);

  io_info_win->update=io_win_update;
  io_info_win->key_pressed=io_win_key_pressed;
  io_info_win->resize=io_win_resize;

  keypad(w,TRUE);
  
  //  wrefresh(io_info_win->w);
  add2win_list(io_info_win);
}

int io_win_key_pressed(MY_WIN *w,int key_code) {
  
  switch(key_code) {
  case KEY_DOWN:cur_reg++;break;
  case KEY_UP:cur_reg--;break;
  default:return FALSE;
  }
  
  if (cur_reg>(nb_reg-nb_line)) cur_reg=(nb_reg-nb_line);
  if (cur_reg<0) cur_reg=0;
  
  io_info_win->update(io_info_win);
  return TRUE;
}

// fonction show_str

void show_separator(WINDOW *w,int i) {
  mvwhline(w,i,1,'-',COLS-COL1-2);
}

void show_separator_color(WINDOW *w,int i) {
  mvwhline(w,i,1,'-',COLS-COL1-2);
  mvwprintw(w,i,2,"COLOR");
}

void show_cycle(WINDOW *w,int i) {
  mvwprintw(w,i,1,"Cycle %d ",nb_cycle);
}

void show_gdma_cycle(WINDOW *w,int i) {
  mvwprintw(w,i,1,"GdmaCycle %d ",dma_info.gdma_cycle);
}

void show_lcdcstat(WINDOW *w,int i) {
  switch(LCDCSTAT&0x03) {
  case 0x00:mvwprintw(w,i,1,"HBLANK",LCDCSTAT);break;
  case 0x01:mvwprintw(w,i,1,"VBLANK",LCDCSTAT);break;
  case 0x02:mvwprintw(w,i,1,"OAM   ",LCDCSTAT);break;
  case 0x03:mvwprintw(w,i,1,"VRAM  ",LCDCSTAT);break;
  }
  mvwprintw(w,i,8,"Next in %d ",gblcdc->cycle);
}

void show_nbspr(WINDOW *w,int i) {
  mvwprintw(w,i,1,"NbSpr   %d ",gblcdc->nb_spr);
}

void io_win_update(MY_WIN *w) {
  int i;
  int n=cur_reg;
  
  for(i=1;io_reg[n].type!=-1 && i<nb_line+1;i++,n++) {
    mvwhline(panel_window(io_info_win->p),i,1,' ',COLS-COL1-2);
    if (io_reg[n].type==0) io_reg[n].str.show_str(panel_window(io_info_win->p),i);
    else mvwprintw(panel_window(io_info_win->p),i,1,io_reg[n].str.str_val.str,mem_read(io_reg[n].str.str_val.add));
  }
  //  wrefresh(panel_window(io_info_win->p));
}

void io_win_resize(MY_WIN *w,int l,int c) {
  wresize(panel_window(w->p),l,c);
  wclear(panel_window(w->p));
  draw_border_win(w);
  nb_line=l-2;
}
