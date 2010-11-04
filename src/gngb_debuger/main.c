#include <gtk/gtk.h>
#include <SDL/SDL.h>
#include <unistd.h>
#include "debuger.h"
#include "debuger_wid.h"
#include "../emu.h"
#include "../rom.h"
#include "../cpu.h"
#include "../sound.h"
#include "../memory.h"
#include "../vram.h"
#include "../interrupt.h"
#include "../sgb.h"

extern SDL_Joystick *joy;

int main(int argc,char *argv[]) {
  GtkWidget *window;
  GtkWidget *menu_bar;
  GtkWidget *vbox;
  GtkWidget *cpu_frame;
  GtkWidget *mem_frame;
  GtkWidget *io_frame;
  GtkWidget *code_frame;
  GtkWidget *table;
  GtkWidget *msg_win;

  gtk_init(&argc,&argv);

  // Init Gngb
   setup_default_conf();
  open_conf();  
  check_option(argc,argv);
  if(optind >= argc)
    print_help();
  
  if (open_rom(argv[optind])) {
    fprintf(stderr,"Error while trying to read file %s \n",argv[optind]);
    exit(1);
  }

  gbmemory_init();
  gblcdc_init();
  gbtimer_init();
  gbcpu_init();
  init_vram((conf.fs?SDL_FULLSCREEN:0)|(conf.gl?SDL_OPENGL:0));
  
  if (conf.gb_type&SUPER_GAMEBOY) sgb_init();

  if(SDL_NumJoysticks()>0){
    joy=SDL_JoystickOpen(conf.joy_no);
    if(joy) {
      printf("Name: %s\n", SDL_JoystickName(conf.joy_no));
      printf("Number of Axes: %d\n", SDL_JoystickNumAxes(joy));
      printf("Number of Buttons: %d\n", SDL_JoystickNumButtons(joy));
      printf("Number of Balls: %d\n", SDL_JoystickNumBalls(joy));
    }
  };

  if (conf.sound && gbsound_init()) conf.sound=0;  


  // Init Debuger
  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  acc_grp_win=gtk_accel_group_new();
  vbox=gtk_vbox_new(FALSE,0);
  
  gtk_signal_connect(GTK_OBJECT(window),"delete_event",
		     GTK_SIGNAL_FUNC(gtk_main_quit),NULL);

  table=gtk_table_new(3,4,FALSE);
  
  mem_frame=init_mem_info();
  gtk_table_attach_defaults(GTK_TABLE(table),mem_frame,
			    0,2,2,4);
  gtk_widget_show(mem_frame); 

  code_frame=init_code_info();
  gtk_table_attach_defaults(GTK_TABLE(table),code_frame,
			    0,2,0,2);
  gtk_widget_show(code_frame); 

  cpu_frame=init_cpu_info();
  gtk_table_attach_defaults(GTK_TABLE(table),cpu_frame,
			    2,3,0,1);
  gtk_widget_show(cpu_frame); 

  io_frame=init_io_info();
  gtk_table_attach_defaults(GTK_TABLE(table),io_frame,
			    2,3,1,4);
  gtk_widget_show(io_frame); 

  msg_win=init_msg_win();

  menu_bar=init_menu(window);
  gtk_box_pack_start(GTK_BOX(vbox),menu_bar,FALSE,FALSE,1);
  gtk_widget_show(menu_bar);

  gtk_box_pack_start(GTK_BOX(vbox),table,FALSE,FALSE,1);
  gtk_widget_show(table);

  gtk_container_add(GTK_CONTAINER(window),vbox);
  gtk_widget_show(vbox);
  gtk_widget_show(window);

  update_cpu_info();
  update_mem_info();
  update_io_info();
  update_code_info();
  
  gtk_window_add_accel_group(GTK_WINDOW(window),acc_grp_win);

  gtk_main();  
  return 0;
}


