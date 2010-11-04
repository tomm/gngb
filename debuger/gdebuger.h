#ifndef DEBUGER_H
#define DEBUGER_H

#include <gtk/gtk.h>
#include <glade/glade.h>
#include "../global.h"

typedef struct {
  GtkWidget *reg_AF_entry;
  GtkWidget *reg_BC_entry;
  GtkWidget *reg_DE_entry;
  GtkWidget *reg_HL_entry;
  GtkWidget *reg_PC_entry;
  GtkWidget *reg_SP_entry;
  GtkWidget *FLAG_label;
  GtkWidget *IME_label;
  GtkWidget *MODE_label;
  GtkWidget *STATE_label;
}CPU_INFO;

typedef struct {
  GtkWidget *clist;
  GtkAdjustment *adj;
  GtkWidget *vscroolbar;
  int begin;
  int last;
}CODE_INFO;

typedef struct {
  GtkWidget *curline_label;
  GtkWidget *cmpline_label;
  GtkWidget *lcdccont_label;
  GtkWidget *lcdcstat_label;
  GtkWidget *lcdccycle_label;
}LCDC_INFO;

typedef struct {
  GtkWidget *ienable_label;
  GtkWidget *iflag_label;
}INT_INFO;

typedef struct {
  GtkWidget *tiles_bk0_preview;  
  GtkWidget *tiles_bk1_preview;
  GtkWidget *back_preview;
  UINT16 *pal0,*pal1;
  int page;
}BUF_INFO;

typedef struct {
  GtkWidget *rom_0_clist;
  GtkWidget *rom_n_clist;
  GtkWidget *ram_clist;
  GtkWidget *vram_clist;
  GtkWidget *wram_clist;
  GtkWidget *oam_clist;
  GtkWidget *himem_clist;
  
  GtkWidget *nb_rom_page_label;
  GtkWidget *nb_vram_page_label;
  GtkWidget *nb_ram_page_label;
  GtkWidget *nb_wram_page_label;

  GtkWidget *active_rom_entry;
  GtkWidget *active_ram_entry;
  GtkWidget *active_vram_entry;
  GtkWidget *active_wram_entry;
  
  int page;
}MEM_INFO;

typedef struct {
  GladeXML *xml_main;
  GtkWidget *file_selector;
  CPU_INFO cpu_info;  
  CODE_INFO code_info;
  LCDC_INFO lcdc_info;
  INT_INFO int_info;
  BUF_INFO buf_info;
  MEM_INFO mem_info;
  GSList *break_list;
}DEBUGER;

DEBUGER app;

void init_app(void);
void quit_app(void);
void update_reg(void);
void clear_memory(void);
void update_cpu_info(void);
void update_code_info(UINT16 pc);
void update_lcdc_info(void);
void update_int_info(void);
void update_buf_info(void);
void update_mem_info(void);
void update_all(void);
void run(void);
void next(void);
void set_breakpoint(UINT16 p);
void clear_breakpoint(UINT16 p);
int is_breakpoint(UINT16 p);

/**************** Gtk Callback *********************/

void on_open_activate(GtkMenuItem *menuitem,gpointer user_data);
void on_quit_activate(GtkMenuItem *menuitem,gpointer user_data);
void on_run_activate(GtkMenuItem *menuitem,gpointer user_data);
void on_next_activate(GtkMenuItem *menuitem,gpointer user_data);
void on_cpu_entry_activate(GtkEditable *editable,gpointer user_data);
void on_file_selector_ok_but_clicked(GtkButton *button,gpointer user_data);
void on_code_adj_changed(GtkAdjustment *adj,gpointer data);
void on_code_clist_select_row(GtkCList *clist,gint row,gint column,GdkEvent *event,gpointer user_data);
void on_code_clist_unselect_row(GtkCList *clist,gint row,gint column,GdkEvent *event,gpointer user_data);
gboolean on_tile_bk0_preview_expose_event(GtkWidget *widget,GdkEventExpose  *event,gpointer user_data);
void on_bk0_conbo_entry_changed(GtkEditable *editable,gpointer user_data);
void on_bk1_conbo_entry_changed(GtkEditable *editable,gpointer user_data);
void on_buf_notebook_switch_page(GtkNotebook *notebook,GtkNotebookPage *page,gint page_num,gpointer user_data);
void on_mem_notebook_switch_page(GtkNotebook *notebook,GtkNotebookPage *page,gint page_num,gpointer user_data);
#endif






