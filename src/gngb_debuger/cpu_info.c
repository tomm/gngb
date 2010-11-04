#include <gtk/gtk.h>
#include "../cpu.h"

struct {
  GtkWidget *flag_label;
  GtkWidget *state_label;
  GtkWidget *ei_di_label;
  GtkWidget *reg_entry[6];
}cpu_info;

GtkWidget *init_cpu_info(void) {
  GtkWidget *frame;
  GtkWidget *vbox,*hbox;
  GtkWidget *label;
  char *reg_label[6]={"A","BC","DE","HL","SP","PC","Flag"};
  int i;

  frame=gtk_frame_new("CPU");
  
  /*hbox=gtk_hbox_new(FALSE,0);
  vbox=gtk_vbox_new(FALSE,0);
  
  for(i=0;i<7;i++) {
    label=gtk_label_new(reg_label[i]);
    gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,2);
    gtk_widget_show(label);
  }

  gtk_box_pack_start(GTK_BOX(hbox),vbox,FALSE,FALSE,2);    
  gtk_widget_show(vbox);

  vbox=gtk_vbox_new(FALSE,0);
  
  cpu_info.flag_label=gtk_label_new("xxxx");
  gtk_box_pack_start(GTK_BOX(vbox),cpu_info.flag_label,TRUE,TRUE,2);
  gtk_widget_show(cpu_info.flag_label);

  for(i=0;i<6;i++) {
    cpu_info.reg_entry[i]=gtk_entry_new_with_max_length(5);
    gtk_box_pack_start(GTK_BOX(vbox),cpu_info.reg_entry[i],TRUE,TRUE,2);
    gtk_widget_show(cpu_info.reg_entry[i]);
  }
  
  gtk_box_pack_start(GTK_BOX(hbox),vbox,FALSE,FALSE,2);    
  gtk_widget_show(vbox);  

  gtk_container_add(GTK_CONTAINER(frame),hbox);  
  gtk_widget_show(hbox);*/

  vbox=gtk_vbox_new(FALSE,0);
  hbox=gtk_hbox_new(FALSE,0);
    
  label=gtk_label_new("A ");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);
  gtk_widget_show(label);

  cpu_info.reg_entry[0]=gtk_entry_new_with_max_length(5);
  gtk_box_pack_start(GTK_BOX(hbox),cpu_info.reg_entry[0],TRUE,TRUE,2);
  gtk_widget_show(cpu_info.reg_entry[0]);
  
  label=gtk_label_new("Flag");
  gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);
  gtk_widget_show(label);

  cpu_info.flag_label=gtk_label_new("xxxx");
  gtk_box_pack_start(GTK_BOX(hbox),cpu_info.flag_label,TRUE,TRUE,2);
  gtk_widget_show(cpu_info.flag_label);

  gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,2);    
  gtk_widget_show(hbox);
  
  for(i=1;i<6;i++) {
    hbox=gtk_hbox_new(FALSE,0);
    
    label=gtk_label_new(reg_label[i]);
    gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,2);
    gtk_widget_show(label);
    
    cpu_info.reg_entry[i]=gtk_entry_new_with_max_length(5);
    gtk_box_pack_start(GTK_BOX(hbox),cpu_info.reg_entry[i],TRUE,TRUE,2);
    gtk_widget_show(cpu_info.reg_entry[i]);
    
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,2);    
    gtk_widget_show(hbox);
  }

  hbox=gtk_hbox_new(FALSE,0);
  
  cpu_info.state_label=gtk_label_new("Run");
  gtk_box_pack_start(GTK_BOX(hbox),cpu_info.state_label,FALSE,FALSE,2);
  gtk_widget_show(cpu_info.state_label);
  
  cpu_info.ei_di_label=gtk_label_new("EI");
  gtk_box_pack_end(GTK_BOX(hbox),cpu_info.ei_di_label,FALSE,FALSE,2);
  gtk_widget_show(cpu_info.ei_di_label);
  
  gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,2);    
  gtk_widget_show(hbox);
  
  gtk_container_add(GTK_CONTAINER(frame),vbox);  
  gtk_widget_show(vbox);
  return frame;
}

void update_cpu_info(void) {
  char text[10];

  // State
  text[0]=0;
  if (gbcpu->state&HALT_STATE) strcat(text,"halt");
  else strcat(text,"run");
  gtk_label_set_text(GTK_LABEL(cpu_info.state_label),text);
	
  // EI/DI
  text[0]=0;
  if (!gbcpu->int_flag) strcat(text,"DI");
  else strcat(text,"EI");
	
  //Flag
  text[0]=0;
  if (IS_SET(FLAG_Z)) text[0]='Z'; else text[0]='z';
  if (IS_SET(FLAG_N)) text[1]='N'; else text[1]='n';
  if (IS_SET(FLAG_H)) text[2]='H'; else text[2]='h';
  if (IS_SET(FLAG_C)) text[3]='C'; else text[3]='c';
  text[4]=0;
  gtk_label_set_text(GTK_LABEL(cpu_info.flag_label),text);
  
  sprintf(text,"%02x",gbcpu->af.b.h);
  gtk_entry_set_text(GTK_ENTRY(cpu_info.reg_entry[0]),text);

  sprintf(text,"%04x",gbcpu->bc.w);
  gtk_entry_set_text(GTK_ENTRY(cpu_info.reg_entry[1]),text);
  sprintf(text,"%04x",gbcpu->de.w);
  gtk_entry_set_text(GTK_ENTRY(cpu_info.reg_entry[2]),text);
  sprintf(text,"%04x",gbcpu->hl.w);
  gtk_entry_set_text(GTK_ENTRY(cpu_info.reg_entry[3]),text);
  sprintf(text,"%04x",gbcpu->sp.w);
  gtk_entry_set_text(GTK_ENTRY(cpu_info.reg_entry[4]),text);
  sprintf(text,"%04x",gbcpu->pc.w);
  gtk_entry_set_text(GTK_ENTRY(cpu_info.reg_entry[5]),text);
}
