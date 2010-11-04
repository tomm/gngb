#ifndef DEBUGER_WID_H
#define DBUEGER_WID_H

#include <gtk/gtk.h>

GtkAccelGroup *acc_grp_win;

// Mem Widget 

extern GtkWidget *dbg_mem_win;
GtkWidget *dbg_mem_win_create(void);
void update_mem_info(void);

// Cpu Widget 

extern GtkWidget *dbg_cpu_win;
GtkWidget *dbg_cpu_win_create(void);
void update_cpu_info(void);

// io Widget

extern GtkWidget *dbg_io_win;
GtkWidget *dbg_io_win_create(void);
void update_io_info(void);

// Code Widget 

extern GtkWidget *dbg_code_win;
GtkWidget *dbg_code_win_create(void);
void update_code_info(void);

void code_info_set_bp(void);

// Msg Widget
extern GtkWidget *dbg_msg_win;
GtkWidget *dbg_msg_win_create(void);

// Vram Widget
extern GtkWidget *dbg_vram_win;
GtkWidget *dbg_vram_win_create(void);

// Interrupt Widget
extern GtkWidget *dbg_int_win;
GtkWidget *dbg_int_win_create(void);

// Menu

GtkWidget *init_dbg_menu(GtkWidget *window);

#endif
