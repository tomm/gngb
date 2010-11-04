#ifndef CPU_INFO_H
#define CPU_INFO_H

#ifdef GTK_DEBUGER

#include <gtk/gtk.h>
#include <glade/glade.h>

typedef struct {
  GtkWidget *reg_af_entry;
  GtkWidget *reg_bc_entry;
  GtkWidget *reg_de_entry;
  GtkWidget *reg_hl_entry;
  GtkWidget *reg_pc_entry;
  GtkWidget *reg_sp_entry;
  GtkWidget *flag_label;
  GtkWidget *ime_label;
  GtkWidget *mode_label;
  GtkWidget *state_label;
}CPU_INFO;

CPU_INFO cpu_info;

void init_cpu_info(GladeXML *gxml);

#endif

void show_cpu_info(void);

#endif
