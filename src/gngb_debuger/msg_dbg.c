#include <gtk/gtk.h>
#include "../memory.h"
#include "../cpu.h"

char active_msg=0;

static GtkWidget *msg_text;

void cb_toggle(GtkWidget *widget,gpointer data) {
  if (GTK_TOGGLE_BUTTON(widget)->active)
    *((char *)data)=1;
  else *((char *)data)=0;
}

void cb_refresh_clicked(GtkWidget *widget,gpointer data) {
  gtk_text_thaw(GTK_TEXT(msg_text));
  gtk_text_freeze(GTK_TEXT(msg_text));
}

GtkWidget *init_msg_win(void) {
  GtkWidget *window;
  GtkWidget *toggle;
  GtkWidget *vbox,*hbox;
  GtkWidget *but;
  GtkWidget *vscrollbar;
  
  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect_object(GTK_OBJECT(window),"delete_event",
			    GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(window));
  gtk_widget_show(window);

  vbox=gtk_vbox_new(FALSE,0);
  
  // Msg Text

  hbox=gtk_hbox_new(FALSE,0);

  msg_text=gtk_text_new(NULL,NULL);
  gtk_box_pack_start(GTK_BOX(hbox),msg_text,TRUE,TRUE,1);
  gtk_widget_show(msg_text);
  gtk_text_freeze(GTK_TEXT(msg_text));

  vscrollbar=gtk_vscrollbar_new(GTK_TEXT(msg_text)->vadj);
  gtk_box_pack_start(GTK_BOX(hbox), vscrollbar,FALSE,FALSE,1);
  gtk_widget_show(vscrollbar);
  
  gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,1);
  gtk_widget_show(hbox);
  
  // Button

  toggle=gtk_toggle_button_new_with_label("Active");
  gtk_box_pack_start(GTK_BOX(vbox),toggle,FALSE,FALSE,1);
  gtk_signal_connect(GTK_OBJECT(toggle),"toggled",
		     GTK_SIGNAL_FUNC(cb_toggle),(gpointer)(&active_msg));
  gtk_widget_show(toggle);

  but=gtk_button_new_with_label("Refresh");
  gtk_box_pack_start(GTK_BOX(vbox),but,FALSE,FALSE,1);
  gtk_signal_connect(GTK_OBJECT(but),"clicked",
		     GTK_SIGNAL_FUNC(cb_refresh_clicked),NULL);
  gtk_widget_show(but);

  gtk_container_add(GTK_CONTAINER(window),vbox);
  gtk_widget_show(vbox);
  return window;
}

void add_msg(const char *format,...) {
  gchar *str;
  va_list pvar;
  va_start(pvar,format);
 
  if (!active_msg) return;
  //gtk_text_freeze(GTK_TEXT(msg_text));
  
  str=g_strdup_printf("PC:%04x LY:%02x LYC:%02x halt:%d IME:%d ",gbcpu->pc.w,CURLINE,CMP_LINE,gbcpu->state,gbcpu->int_flag);
  gtk_text_insert(GTK_TEXT(msg_text),NULL,NULL,NULL,str,-1);
  g_free(str);
  switch(LCDCSTAT&0x03) {
  case 0x00:str=g_strdup_printf("HBLANK ");break;
  case 0x01:str=g_strdup_printf("VBLANK ");break;
  case 0x02:str=g_strdup_printf("   OAM ");break;
  case 0x03:str=g_strdup_printf("  VRAM ");break;
  }
  gtk_text_insert(GTK_TEXT(msg_text),NULL,NULL,NULL,str,-1);
  g_free(str);
  
  str=g_strdup_vprintf(format,pvar);
  gtk_text_insert(GTK_TEXT(msg_text),NULL,NULL,NULL,str,-1);
  g_free(str);
  
  va_end(pvar); 
  //gtk_text_thaw(GTK_TEXT(msg_text));
}


