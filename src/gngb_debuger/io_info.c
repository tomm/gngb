#include <gtk/gtk.h>
#include <string.h>
#include "../global.h"
#include "../memory.h"
#include "../interrupt.h"

typedef struct {
  char *name;
  Uint16 add;
  GtkWidget *entry;
}io_reg_info;

void pack_io_reg_info(GtkWidget *vbox,io_reg_info regs[]) {
  GtkWidget *hbox,*label;
  int i;

  for(i=0;regs[i].name;i++) {
    hbox=gtk_hbox_new(FALSE,1);
    label=gtk_label_new(regs[i].name);
    gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);
    gtk_widget_show(label);
    regs[i].entry=gtk_entry_new_with_max_length(2);
    gtk_box_pack_end(GTK_BOX(hbox),regs[i].entry,TRUE,TRUE,1);
    gtk_widget_show(regs[i].entry);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,1);
    gtk_widget_show(hbox);
  }

}

void update_io_regs(io_reg_info regs[]) {
  char v[5];
  int i;
  for(i=0;regs[i].name;i++) {
    if (regs[i].entry) {
      sprintf(v,"%02x",mem_read(regs[i].add));
      gtk_entry_set_text(GTK_ENTRY(regs[i].entry),v);
    }       
  }
}

// Lcd Info

static io_reg_info lcd_regs[]={
  {"SCRX",0xff43,NULL},
  {"SCRY",0xff42,NULL},
  {"WINX",0xff4b,NULL},
  {"WINY",0xff4a,NULL},
  {"LY",0xff44,NULL},
  {"LYC",0xff45,NULL},
  {"LCDCONT",0xff40,NULL},
  {"LCDSTAT",0xff41,NULL},
  {NULL,0,NULL}
};

static GtkWidget *lcd_cycle_label,*lcd_per_label;

GtkWidget *init_lcd_info(void) {
  GtkWidget *frame,*vbox;
  
  frame=gtk_frame_new("Lcd");
  vbox=gtk_vbox_new(FALSE,0);
  pack_io_reg_info(vbox,lcd_regs);

  lcd_per_label=gtk_label_new("VRAM");
  gtk_box_pack_start(GTK_BOX(vbox),lcd_per_label,FALSE,FALSE,1);
  gtk_widget_show(lcd_per_label);

  lcd_cycle_label=gtk_label_new("000");
  gtk_box_pack_start(GTK_BOX(vbox),lcd_cycle_label,FALSE,FALSE,1);
  gtk_widget_show(lcd_cycle_label);

  gtk_container_add(GTK_CONTAINER(frame),vbox);
  gtk_widget_show(vbox);

  return frame;
}

void update_lcd_info(void) {
  char v[10]={0};
  update_io_regs(lcd_regs);

  
  switch(LCDCSTAT&0x03) {
  case 0x00:strcat(v,"HBLANK");break;
  case 0x01:strcat(v,"VBLANK");break;
  case 0x02:strcat(v,"OAM");break;
  case 0x03:strcat(v,"VRAM");break;
  }
  gtk_label_set_text(GTK_LABEL(lcd_per_label),v);

  v[0]=0;
  sprintf(v,"%d",gblcdc->cycle);
  gtk_label_set_text(GTK_LABEL(lcd_cycle_label),v);
}

// Timer Info

static io_reg_info timer_regs[]={
  {"DIV",0xFF04,NULL},
  {"TIMA",0xFF05,NULL},
  {"TMA",0xFF06,NULL},
  {"TAC",0xFF07,NULL},
  {NULL,0,NULL}
};

static GtkWidget *timer_frq_label;

GtkWidget *init_timer_info(void) {
  GtkWidget *frame,*vbox;
  
  frame=gtk_frame_new("Timer");
  vbox=gtk_vbox_new(FALSE,0);
  pack_io_reg_info(vbox,timer_regs);

  timer_frq_label=gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(vbox),timer_frq_label,FALSE,FALSE,1);
  gtk_widget_show(timer_frq_label);

  gtk_container_add(GTK_CONTAINER(frame),vbox);
  gtk_widget_show(vbox);

  return frame;
}

void update_timer_info(void) {
  char t[15]={0};
  update_io_regs(timer_regs);
  
  if (TIME_CONTROL&0x04) {
    switch(TIME_CONTROL&0x03) {
    case 0x00:strcat(t,"4.096 KHz");break;
    case 0x01:strcat(t,"262.144 KHz");break;
    case 0x02:strcat(t,"65.536 KHz");break;
    case 0x03:strcat(t,"16.384 KHz");break;
    }
  } else strcat(t,"No Timer");
  gtk_label_set_text(GTK_LABEL(timer_frq_label),t);
}

/*struct {
  char *name;
  Uint16 add;
  GtkWidget *entry;
}io_reg[]={
  {"separator",0x0000,NULL},
  {"P1",0xff00,NULL},
  {"IE",0xffff,NULL},
  {"IF",0xffff,NULL},
  {"separator",0x0000,NULL},
  {"SCRX",0xff43,NULL},
  {"SCRY",0xff42,NULL},
  {"WINX",0xff4b,NULL},
  {"WINY",0xff4a,NULL},
  {"LY",0xff44,NULL},
  {"LYC",0xff45,NULL},
  {"LCDCONT",0xff40,NULL},
  {"LCDSTAT",0xff41,NULL},
  {"separator",0x0000,NULL},
  {NULL,0x0000,NULL}
  };*/

struct {
  GtkWidget *(*init_wid)(void);
  void (*update_wid)(void);
}io_regs[]={
  {init_lcd_info,update_lcd_info},
  {init_timer_info,update_timer_info},
  {NULL,NULL}
};


GtkWidget *dbg_io_win_create(void) {
  GtkWidget *win;
  GtkWidget *scroll_win;
  GtkWidget *vbox;
  GtkWidget *w;
  int i;

  win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win),"Io");
  gtk_signal_connect_object(GTK_OBJECT(win),"delete_event",
			    GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(win));
  gtk_widget_show(win);

  scroll_win=gtk_scrolled_window_new(NULL,NULL);
  vbox=gtk_vbox_new(FALSE,2);

  for(i=0;io_regs[i].init_wid;i++) {
    w=io_regs[i].init_wid();
    gtk_box_pack_start(GTK_BOX(vbox),w,FALSE,FALSE,4);
    gtk_widget_show(w);

  }
     
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll_win),vbox);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(win),scroll_win);
  gtk_widget_show(scroll_win);
  return win;
}

void update_io_info(void) {
  int i;
  for(i=0;io_regs[i].update_wid;i++) io_regs[i].update_wid();  
}
