#ifndef DEBUGER_WID_H
#define DBUEGER_WID_H

#include <gtk/gtk.h>

GtkAccelGroup *acc_grp_win;

// Mem Widget 

GtkWidget *init_mem_info(void);
void update_mem_info(void);

// Cpu Widget 

GtkWidget *init_cpu_info(void);
void update_cpu_info(void);

// io Widget

GtkWidget *init_io_info(void);
void update_io_info(void);

// Code Widget 

GtkWidget *init_code_info(void);
void update_code_info(void);

void code_info_set_bp(void);

// Msg Widget

GtkWidget *init_msg_win(void);

// Menu

GtkWidget *init_menu(GtkWidget *window);

#endif
