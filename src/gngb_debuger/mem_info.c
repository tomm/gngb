#include <gtk/gtk.h>
#include "debuger.h"
#include "../global.h"
#include "../memory.h"

struct {
  GtkWidget *clist;
  GtkObject *vadj;
  Uint16 begin_add;
}mem_info;

void mem_info_set_begin_add(Uint16 add) {
  int i,j;
  char text[10];
  mem_info.begin_add=add;
  
  gtk_clist_freeze(GTK_CLIST(mem_info.clist));
  for(i=0;i<10;i++) {
    sprintf(text,"0x%04x",add);
    gtk_clist_set_text(GTK_CLIST(mem_info.clist),i,0,text);
    get_mem_id(add,text);
    gtk_clist_set_text(GTK_CLIST(mem_info.clist),i,1,text);
    for(j=0;j<16;j++,add++) {
      sprintf(text,"%02x",mem_read(add));
      gtk_clist_set_text(GTK_CLIST(mem_info.clist),i,j+2,text);
    }
  }  
  gtk_clist_thaw(GTK_CLIST(mem_info.clist));
}


static void cb_vadj_value_changed(GtkAdjustment *adj,gpointer data) {
  Uint16 add;
  add=((Uint16)adj->value)*16;
  mem_info_set_begin_add(add);
}

static void cb_clist_scroll_horizontal(GtkCList *clist,GtkScrollType scroll_type,gfloat position) {
  float value=GTK_ADJUSTMENT(mem_info.vadj)->value;
  switch(scroll_type) {
  case GTK_SCROLL_NONE:return;
  case GTK_SCROLL_STEP_BACKWARD:value-=GTK_ADJUSTMENT(mem_info.vadj)->step_increment;break;
  case GTK_SCROLL_STEP_FORWARD:value+=GTK_ADJUSTMENT(mem_info.vadj)->step_increment;break;
  case GTK_SCROLL_PAGE_BACKWARD:value-=GTK_ADJUSTMENT(mem_info.vadj)->page_increment;break;
  case GTK_SCROLL_PAGE_FORWARD:value+=GTK_ADJUSTMENT(mem_info.vadj)->page_increment;break;
  case GTK_SCROLL_JUMP:printf("scroll jump");
  }
  if (value>(GTK_ADJUSTMENT(mem_info.vadj)->upper-GTK_ADJUSTMENT(mem_info.vadj)->page_size))
    value=GTK_ADJUSTMENT(mem_info.vadj)->upper-GTK_ADJUSTMENT(mem_info.vadj)->page_size;
  if (value<GTK_ADJUSTMENT(mem_info.vadj)->lower)
    value=GTK_ADJUSTMENT(mem_info.vadj)->lower;
  gtk_adjustment_set_value(GTK_ADJUSTMENT(mem_info.vadj),value);
  gtk_adjustment_value_changed(GTK_ADJUSTMENT(mem_info.vadj));
}

GtkWidget *dbg_mem_win_create(void) {
  GtkWidget *win;
  GtkWidget *vscroll_bar;
  GtkWidget *hbox;
  int i;
  char *text[]={"ADD     ","TYPE   ",
		"00","01","02","03","04","05","06","07",
		"08","09","0A","0B","0C","0D","0E","0F"};

  win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win),"Memory");
  gtk_signal_connect_object(GTK_OBJECT(win),"delete_event",
			    GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(win));
  gtk_widget_show(win);

  hbox=gtk_hbox_new(FALSE,1);
 
  mem_info.vadj=gtk_adjustment_new(0,0,0x10000/0x10,1,8,10);
  gtk_signal_connect(GTK_OBJECT(mem_info.vadj),"value_changed",
		     GTK_SIGNAL_FUNC(cb_vadj_value_changed),NULL);
  
  mem_info.clist=gtk_clist_new_with_titles(18,text);
  gtk_clist_column_titles_passive(GTK_CLIST(mem_info.clist));
  gtk_signal_connect(GTK_OBJECT(mem_info.clist),"scroll_vertical",
		     GTK_SIGNAL_FUNC(cb_clist_scroll_horizontal),NULL);
  for(i=0;i<10;i++)
    gtk_clist_prepend(GTK_CLIST(mem_info.clist),text);
  mem_info_set_begin_add(0x0000);
 
  gtk_box_pack_start(GTK_BOX(hbox),mem_info.clist,FALSE,FALSE,2);    
  gtk_widget_show(mem_info.clist);

  vscroll_bar=gtk_vscrollbar_new(GTK_ADJUSTMENT(mem_info.vadj));
  gtk_box_pack_start(GTK_BOX(hbox),vscroll_bar,FALSE,FALSE,2);    
  gtk_widget_show(vscroll_bar); 

  gtk_container_add(GTK_CONTAINER(win),hbox);
  gtk_widget_show(hbox);

  return win;
}

void update_mem_info(void) {
  int i,j;
  char text[5];
  Uint16 add=mem_info.begin_add;

  gtk_clist_freeze(GTK_CLIST(mem_info.clist));
  for(i=0;i<10;i++) {
    for(j=0;j<16;j++,add++) {
      sprintf(text,"%02x",mem_read(add));
      gtk_clist_set_text(GTK_CLIST(mem_info.clist),i,j+2,text);
    }
  }  
  gtk_clist_thaw(GTK_CLIST(mem_info.clist));
}
