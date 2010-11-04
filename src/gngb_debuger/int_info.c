#include <gtk/gtk.h>

// Interrupt Info

char vblank_int_enable=1;
char lcd_oam_int_enable=1;
char lcd_lyc_int_enable=1;
char lcd_hblank_int_enable=1;
char lcd_vblank_int_enable=1;
char lcd_vram_int_enable=1;
char timer_int_enable=1;
char link_int_enable=1;
char pad_int_eanble=1;

/* CallBack */

static void dbg_int_win_toggle(GtkWidget *widget,gpointer data) {
  if (GTK_TOGGLE_BUTTON(widget)->active)
    *((char *)data)=1;
  else *((char *)data)=0;
}

GtkWidget *dbg_int_win_create(void) {
  GtkWidget *win;
  GtkWidget *vbox;
  GtkWidget *toggle;
  gpointer tabp[]={
    &vblank_int_enable,
    &lcd_oam_int_enable,
    &lcd_lyc_int_enable,
    &lcd_hblank_int_enable,
    &lcd_vblank_int_enable,
    &lcd_vram_int_enable,
    &timer_int_enable,
    &link_int_enable,
    &pad_int_eanble };
  gchar *but_label[]={
    "vblank",
    "lcd_oam",
    "lcd_lyc",
    "lcd_hblank",
    "lcd_vblank",
    "lcd_vram",
    "timer",
    "link",
    "pad"
  };
  int i;

  win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win),"interrupt");
  gtk_signal_connect_object(GTK_OBJECT(win),"delete_event",
			    GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(win));
  gtk_widget_show(win);
  
  vbox=gtk_vbox_new(FALSE,0);
  
  for(i=0;i<9;i++) {
    toggle=gtk_toggle_button_new_with_label(but_label[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle),TRUE);
    gtk_box_pack_start(GTK_BOX(vbox),toggle,FALSE,FALSE,1);
    gtk_signal_connect(GTK_OBJECT(toggle),"toggled",
		       GTK_SIGNAL_FUNC(dbg_int_win_toggle),(gpointer)tabp[i]);
    gtk_widget_show(toggle);
  }
    
  gtk_container_add(GTK_CONTAINER(win),vbox);
  gtk_widget_show(vbox);
  return win;
}
