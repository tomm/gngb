#include <gtk/gtk.h>
#include "op.h"
#include "debuger.h"
#include "break.h"
#include "../global.h"
#include "../memory.h"
#include "../cpu.h"

#define NB_LINE 12

Sint8 opcode_size[0xffff]={-1};

struct {
  GtkWidget *clist;
  GtkObject *vadj;
  int curent_line;
  Uint16 curent_add;
  Uint16 begin_add;
  Uint16 last_add;
  Uint16 line_add[NB_LINE];
}code_info;

GdkColor bp_col={1,0xffff,0,0};
GdkColor pc_col={1,0,0xffff,0};
GdkColor pc_bp_col={1,0xffff,0xffff,0};
GdkColor normal_col={1,0xffff,0xffff,0xffff};  

static Uint16 get_last_add(Uint16 begin_add) {
  int i;
  for(i=0;i<NB_LINE;i++) 
    begin_add+=(get_nb_byte(mem_read(begin_add)));
  return begin_add;		
}

static void code_info_set_begin_add(Uint16 add,char update_adj) {
  int i;
  Uint8 op;
  char text[50];
 

  code_info.begin_add=add;
  gtk_clist_freeze(GTK_CLIST(code_info.clist));
  for(i=0;i<NB_LINE;i++) {
    code_info.line_add[i]=add;

    if (is_break_point(add)) {
      if (gbcpu->pc.w==add)
	gtk_clist_set_background(GTK_CLIST(code_info.clist),i,&pc_bp_col);
      else gtk_clist_set_background(GTK_CLIST(code_info.clist),i,&bp_col);
    } else {
      if (gbcpu->pc.w==add)
	gtk_clist_set_background(GTK_CLIST(code_info.clist),i,&pc_col);
      else gtk_clist_set_background(GTK_CLIST(code_info.clist),i,&normal_col);
    }
    
    sprintf(text,"0x%04x",add);
    gtk_clist_set_text(GTK_CLIST(code_info.clist),i,0,text);
    get_mem_id(add,text);
    gtk_clist_set_text(GTK_CLIST(code_info.clist),i,1,text);
    op=mem_read(add);
    add+=(aff_op(op,add,text));
    gtk_clist_set_text(GTK_CLIST(code_info.clist),i,2,text);
  }  
  code_info.last_add=add;
  gtk_clist_thaw(GTK_CLIST(code_info.clist));
  if (update_adj) gtk_adjustment_set_value(GTK_ADJUSTMENT(code_info.vadj),
					   code_info.begin_add);
}

static void code_info_add2begin_add(Sint16 off,char update_adj) {
  int i;
  Uint16 add=code_info.begin_add;
  if (off>0) {
    for(i=0;i<off && add<0xffff;i++) 
      add+=get_nb_byte(mem_read(add));
  } else {
    if (((Sint32)add+off)<=0) add=0;
    else add+=off;
  }
  code_info_set_begin_add(add,update_adj);
}

static char code_info_check_add(Uint32 add) {
  int i;
  for(i=0;i<NB_LINE;i++) {
    add+=get_nb_byte(mem_read(add));
    if (add>0xffff) return FALSE;
  }  
  return TRUE;
}
 
static void cb_vadj_value_changed(GtkAdjustment *adj,gpointer data) {
  Uint16 add=(Uint16)adj->value;
  if (code_info_check_add(add))
    code_info_set_begin_add(add,FALSE);
}

static void cb_clist_scroll_horizontal(GtkCList *clist,GtkScrollType scroll_type,gfloat position,gpointer data) {
  Sint32 add;
  switch(scroll_type) {
  case GTK_SCROLL_NONE:return;
  case GTK_SCROLL_STEP_BACKWARD:
    if (code_info.curent_line==0) {
      /*      add=code_info.begin_add-1;
	      if (add>=0) code_info_set_begin_add(add,TRUE);*/
      add=code_info.begin_add-((opcode_size[code_info.begin_add]&0x0c)>>2);
      if (add>=0) code_info_set_begin_add(add,TRUE);
    } else code_info.curent_line--;
    break;
  case GTK_SCROLL_STEP_FORWARD:
    if (code_info.curent_line==(NB_LINE-1)) {
      add=code_info.begin_add+get_nb_byte(mem_read(code_info.begin_add));
      if (add<0xffff) code_info_set_begin_add(add,TRUE);
    } else code_info.curent_line++;
    break;
  case GTK_SCROLL_PAGE_BACKWARD:
    add=code_info.begin_add-NB_LINE;
    /*while(add>0 && code_info.begin_add!=get_last_add(add)) add--;*/
    if (add>=0) code_info_set_begin_add(add,TRUE);
    break;
  case GTK_SCROLL_PAGE_FORWARD:
    add=code_info.last_add;
    if (add<0xffff) code_info_set_begin_add(add,TRUE);
    break;
  case GTK_SCROLL_JUMP:printf("code scroll jump");
  }
}

static void cb_clist_select_row(GtkCList *clist,gint row,gint column,GdkEvent *event,gpointer data) {

  if (event->type==GDK_2BUTTON_PRESS) {
    if (!is_break_point(code_info.line_add[row])) 
      add_break_point(code_info.line_add[row]);
    else del_break_point(code_info.line_add[row]);
    code_info.curent_line=row;
    gtk_clist_unselect_row(GTK_CLIST(code_info.clist),row,column);
    code_info_set_begin_add(code_info.begin_add,FALSE);
  }
}

GtkWidget *dbg_code_win_create(void) {
  GtkWidget *win;
  GtkWidget *vscroll_bar;
  GtkWidget *hbox;
  int i;
  char *text[]={"ADD     ","TYPE   ","OP"};
		
  code_info.begin_add=0x100;
  code_info.curent_line=NB_LINE-1;

  win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win),"Code");
  gtk_signal_connect_object(GTK_OBJECT(win),"delete_event",
			    GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(win));
  gtk_widget_show(win);

  hbox=gtk_hbox_new(FALSE,1);
 
  code_info.vadj=gtk_adjustment_new(0,0,0x10000-NB_LINE,1,NB_LINE-2,NB_LINE);
  gtk_signal_connect(GTK_OBJECT(code_info.vadj),"value_changed",
		     GTK_SIGNAL_FUNC(cb_vadj_value_changed),NULL);
  
  code_info.clist=gtk_clist_new_with_titles(3,text);
  gtk_clist_column_titles_passive(GTK_CLIST(code_info.clist));
  gtk_signal_connect(GTK_OBJECT(code_info.clist),"scroll_vertical",
		     GTK_SIGNAL_FUNC(cb_clist_scroll_horizontal),NULL);
  gtk_signal_connect(GTK_OBJECT(code_info.clist),"select_row",
		     GTK_SIGNAL_FUNC(cb_clist_select_row),NULL);
  
  gtk_box_pack_start(GTK_BOX(hbox),code_info.clist,TRUE,TRUE,2);    
  for(i=0;i<NB_LINE;i++)
    gtk_clist_prepend(GTK_CLIST(code_info.clist),text);
  code_info_set_begin_add(0x100,TRUE);
  gtk_widget_show(code_info.clist);

  vscroll_bar=gtk_vscrollbar_new(GTK_ADJUSTMENT(code_info.vadj));
  gtk_box_pack_start(GTK_BOX(hbox),vscroll_bar,FALSE,FALSE,2);    
  gtk_widget_show(vscroll_bar); 

  gtk_container_add(GTK_CONTAINER(win),hbox);
  gtk_widget_show(hbox);

  return win;
}


void update_code_info(void) {
  Uint32 add=0;
  Uint8 t=0;

  if (gbcpu->pc.w<code_info.begin_add || gbcpu->pc.w>=code_info.last_add)
    code_info_set_begin_add(gbcpu->pc.w,TRUE);
  else code_info_set_begin_add(code_info.begin_add,FALSE);

  while(add<0xffff) {
    opcode_size[add]=t<<2;
    opcode_size[add]|=(t=get_nb_byte(mem_read(add)));
    add+=t;    
  }

}

void code_info_set_bp(void) {
  gtk_clist_select_row(GTK_CLIST(code_info.clist),code_info.curent_line,0);
}


