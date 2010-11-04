#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "../emu.h"
#include "debuger.h"
#include "debuger_wid.h"

static void cb_open(GtkWidget *widget,gpointer data) {

}

static void cb_quit(GtkWidget *widget,gpointer data) {
  gtk_main_quit();
}

static void cb_run(GtkWidget *widget,gpointer data) {
  db_run();
}

static void cb_step(GtkWidget *widget,gpointer data) {
  db_step();
}

static void cb_set_bp(GtkWidget *widget,gpointer data) {
  db_set_bp();
}

static void cb_stop(GtkWidget *widget,gpointer data) {
  conf.gb_done=1;
}


/* Return A Menu Bar */
GtkWidget *init_menu(GtkWidget *window) {
  GtkWidget *menu;
  GtkWidget *menu_item;
  GtkWidget *menu_bar;
  
  menu_bar=gtk_menu_bar_new();

  // File Menu
  menu=gtk_menu_new();
  // Open
  menu_item=gtk_menu_item_new_with_label("Open");
  gtk_menu_append(GTK_MENU(menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     GTK_SIGNAL_FUNC(cb_open),NULL);
  // Quit
  menu_item=gtk_menu_item_new_with_label("Quit");
  gtk_menu_append(GTK_MENU(menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     GTK_SIGNAL_FUNC(cb_quit),NULL);

  menu_item=gtk_menu_item_new_with_label("File");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),menu);
  gtk_menu_bar_append(GTK_MENU_BAR(menu_bar),menu_item);
  gtk_widget_show(menu_item);

  // Emu Menu
  menu=gtk_menu_new();
  // Run
  menu_item=gtk_menu_item_new_with_label("Run");
  gtk_menu_append(GTK_MENU(menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     GTK_SIGNAL_FUNC(cb_run),NULL);
  gtk_widget_add_accelerator(menu_item,"activate",acc_grp_win,GDK_F9,0,GTK_ACCEL_VISIBLE);
  // Set Bp
  menu_item=gtk_menu_item_new_with_label("Set Break");
  gtk_menu_append(GTK_MENU(menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     GTK_SIGNAL_FUNC(cb_set_bp),NULL);
  gtk_widget_add_accelerator(menu_item,"activate",acc_grp_win,GDK_F2,0,GTK_ACCEL_VISIBLE);
   // Step
  menu_item=gtk_menu_item_new_with_label("Step");
  gtk_menu_append(GTK_MENU(menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     GTK_SIGNAL_FUNC(cb_step),NULL);
  gtk_widget_add_accelerator(menu_item,"activate",acc_grp_win,GDK_F7,0,GTK_ACCEL_VISIBLE);
  // Stop
  menu_item=gtk_menu_item_new_with_label("Stop");
  gtk_menu_append(GTK_MENU(menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     GTK_SIGNAL_FUNC(cb_stop),NULL);

  menu_item=gtk_menu_item_new_with_label("Emu");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),menu);
  gtk_menu_bar_append(GTK_MENU_BAR(menu_bar),menu_item);
  gtk_widget_show(menu_item);
   

  return menu_bar;
}
