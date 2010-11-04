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
#include <signal.h>

extern SDL_Joystick *sdl_joy;

GtkWidget *dbg_vram_win=NULL;
GtkWidget *dbg_msg_win=NULL;
GtkWidget *dbg_int_win=NULL;
GtkWidget *dbg_mem_win=NULL;
GtkWidget *dbg_cpu_win=NULL;
GtkWidget *dbg_io_win=NULL;
GtkWidget *dbg_code_win=NULL;

void my_exit(void) {
  printf("PC: %04x\n",gbcpu->pc.w);
  printf("AF: %04x\n",gbcpu->af.w);
  printf("BC: %04x\n",gbcpu->bc.w);
  printf("DE: %04x\n",gbcpu->de.w);
  printf("HL: %04x\n",gbcpu->hl.w);
}

void recieve_sigint(int signum) {
  printf("PC: %04x\n",gbcpu->pc.w);
  printf("AF: %04x\n",gbcpu->af.w);
  printf("BC: %04x\n",gbcpu->bc.w);
  printf("DE: %04x\n",gbcpu->de.w);
  printf("HL: %04x\n",gbcpu->hl.w);
  if (!conf.gb_done) conf.gb_done=1;
}


int main(int argc,char *argv[]) {
  GtkWidget *window;
  GtkWidget *menu_bar;
  GtkWidget *vbox;
  
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

  conf.fs=0;
  emu_init();

  /*init_vram((conf.fs?SDL_FULLSCREEN:0)|(conf.gl?SDL_OPENGL:0));	
  gbmemory_init();
  gblcdc_init();
  gbtimer_init();
  gbcpu_init();
  
  if (conf.gb_type&SUPER_GAMEBOY) sgb_init();

  if(SDL_NumJoysticks()>0){
    sdl_joy=SDL_JoystickOpen(conf.joy_no);
    if(sdl_joy) {
      printf("Name: %s\n", SDL_JoystickName(conf.joy_no));
      printf("Number of Axes: %d\n", SDL_JoystickNumAxes(sdl_joy));
      printf("Number of Buttons: %d\n", SDL_JoystickNumButtons(sdl_joy));
      printf("Number of Balls: %d\n", SDL_JoystickNumBalls(sdl_joy));
    }
    };

    if (conf.sound && gbsound_init()) conf.sound=0;  */

  signal(SIGINT,recieve_sigint);
  atexit(my_exit);

  // Init Debuger
  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  acc_grp_win=gtk_accel_group_new();
  vbox=gtk_vbox_new(FALSE,0);
  
  gtk_signal_connect(GTK_OBJECT(window),"delete_event",
		     GTK_SIGNAL_FUNC(gtk_main_quit),NULL);

  //table=gtk_table_new(3,4,FALSE);
  
  
  dbg_msg_win=dbg_msg_win_create();
  dbg_vram_win=dbg_vram_win_create();
  dbg_int_win=dbg_int_win_create();
  dbg_mem_win=dbg_mem_win_create();
  dbg_cpu_win=dbg_cpu_win_create();
  dbg_io_win=dbg_io_win_create();
  dbg_code_win=dbg_code_win_create();

  menu_bar=init_dbg_menu(window);
  /*  gtk_box_pack_start(GTK_BOX(vbox),menu_bar,FALSE,FALSE,1);
      gtk_widget_show(menu_bar);

  gtk_box_pack_start(GTK_BOX(vbox),table,FALSE,FALSE,1);
  gtk_widget_show(table);*/

  gtk_container_add(GTK_CONTAINER(window),menu_bar);
  gtk_widget_show(menu_bar);
  gtk_widget_show(window);

  update_cpu_info();
  update_mem_info();
  update_io_info();
  update_code_info();
  
  gtk_window_add_accel_group(GTK_WINDOW(window),acc_grp_win);

  gtk_main();  
  return 0;
}


