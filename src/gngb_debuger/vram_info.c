#include <gtk/gtk.h>
#include <SDL/SDL.h>
#include "../global.h"
#include "../vram.h"
#include "../memory.h"
#include "../emu.h"

char active_back=1;
char active_obj=1;
char active_win=1;

static GdkImage *buffer=NULL;
static GdkVisual *visual=NULL;

/* DrawFunction */

#define GET_GB_PIXEL(bit) (((((*tp)&tab_ms[(bit)].mask)>>tab_ms[(bit)].shift))|((((*(tp+1))&tab_ms[(bit)].mask)>>tab_ms[(bit)].shift)<<1))


static void draw_tile(GdkImage *buffer,int dstx,int dsty,int tile,int tile_data,int bank,int xflip,int yflip,Uint16 *pal) {
  Uint8 *tp;
  Sint16 y,x;
  Uint8 c;
  
  if (!tile_data) tile=256+(signed char)tile;
  tp=&vram_page[bank][(tile<<4)];
  
  if (yflip) {
    tp+=(7*2);
    for(y=0;y<8;y++,tp-=2) {
      for(x=0;x<8;x++) {
	int wbit;
	if (!xflip) wbit=x;
	else wbit=7-x;     
	c=GET_GB_PIXEL(wbit);
	gdk_image_put_pixel(buffer,dstx+x,dsty+y,pal[c]);
      }
    }
  } else {
    for(y=0;y<8;y++,tp+=2) {
      for(x=0;x<8;x++) {
	int wbit;
	if (!xflip) wbit=x;
	else wbit=7-x;     
	c=GET_GB_PIXEL(wbit);
	gdk_image_put_pixel(buffer,dstx+x,dsty+y,pal[c]);
      }
    }
  }
}

static void draw_background(GdkImage *buffer,int tile_map,int tile_data) {
  Uint8 *tb,*att_tb;
  int n=0;  
  int x,y;

  switch(tile_map) {
  case 0:
    tb=(LCDCCONT&0x08)?(&vram_page[0][0x1c00]):(&vram_page[0][0x1800]);
    if (conf.gb_type&COLOR_GAMEBOY) att_tb=(LCDCCONT&0x08)?(&vram_page[1][0x1c00]):(&vram_page[1][0x1800]);
    break;
  case 1:
    tb=&vram_page[0][0x1800];
    if (conf.gb_type&COLOR_GAMEBOY) att_tb=&vram_page[1][0x1800];
    break;
  case 2:
    tb=&vram_page[0][0x1c00];
    if (conf.gb_type&COLOR_GAMEBOY) att_tb=&vram_page[1][0x1c00];
    break;
  default:printf("Heu ya un pb\n");exit(1);
  }

  if (conf.gb_type&COLOR_GAMEBOY) {
    Uint8 att;
    for(y=0;y<256;y+=8)
      for(x=0;x<256;x+=8) {
	att=att_tb[n];
	draw_tile(buffer,x,y,tb[n],tile_data,(att&0x08)>>3,(att&0x20),(att&0x40),pal_col_bck[att&0x07]);
	n++;
      }
  } else {
    for(y=0;y<256;y+=8)
      for(x=0;x<256;x+=8) {
	draw_tile(buffer,x,y,tb[n],tile_data,0,0,0,grey);
	n++;
      }
  }
}


/****************** Widget Init ********************/

void clear_buffer(void) {
  int x,y;
  for(y=0;y<256;y++)
    for(x=0;x<256;x++)
      gdk_image_put_pixel(buffer,x,y,0);
}

/* BackGround Frame */

struct {
  GtkWidget *darea;
  char tile_map;
  char tile_data;
}bg_info;

static void bg_darea_expose_event(GtkWidget *widget,GdkEvent  *event,gpointer user_data) {
  GtkDrawingArea *darea;
  GdkDrawable *drawable;
  GdkGC *gc;

  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;
  gc=widget->style->black_gc;
  
  if (buffer) {
    draw_background(buffer,bg_info.tile_map,bg_info.tile_data);
    gdk_draw_image(drawable,gc,buffer,0,0,0,0,256,256);
    gdk_draw_rectangle(drawable,gc,FALSE,SCRX,SCRY,160,144);
  }
}

static void cb_set_bg_tile_map(GtkWidget *widget,gpointer data) {
  bg_info.tile_map=(unsigned char)GPOINTER_TO_INT(data);
  gtk_widget_draw(bg_info.darea,NULL);
}

static void cb_set_bg_tile_data(GtkWidget *widget,gpointer data) {
  bg_info.tile_data=(unsigned char)GPOINTER_TO_INT(data);
  gtk_widget_draw(bg_info.darea,NULL);
}

static GtkWidget *init_bg_frame(void) {
  GtkWidget *frame;
  GtkWidget *darea;
  GtkWidget *vbox,*hbox;
  GtkWidget *option_menu;
  GtkWidget *menu;
  GtkWidget *menu_item;

  frame=gtk_frame_new(NULL);

  vbox=gtk_vbox_new(FALSE,1);

  // Drawing Area

  darea=gtk_drawing_area_new();
  gtk_widget_set_usize (darea, 256, 256);
  gtk_signal_connect (GTK_OBJECT (darea),
		      "expose_event",
		      GTK_SIGNAL_FUNC (bg_darea_expose_event),
		      NULL);
  gtk_box_pack_start(GTK_BOX(vbox),darea,FALSE,FALSE,1);
  gtk_widget_show(darea); 

  // Option Menu 

  hbox=gtk_hbox_new(FALSE,1);

  // Tile Map
  
  option_menu=gtk_option_menu_new();
  menu=gtk_menu_new();
  menu_item=gtk_menu_item_new_with_label("Auto");
  gtk_menu_append(GTK_MENU(menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     GTK_SIGNAL_FUNC(cb_set_bg_tile_map),GINT_TO_POINTER(0));
  menu_item=gtk_menu_item_new_with_label("0x9800");
  gtk_menu_append(GTK_MENU(menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     GTK_SIGNAL_FUNC(cb_set_bg_tile_map),GINT_TO_POINTER(1));
  menu_item=gtk_menu_item_new_with_label("0x9C00");
  gtk_menu_append(GTK_MENU(menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     GTK_SIGNAL_FUNC(cb_set_bg_tile_map),GINT_TO_POINTER(2));
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu),menu);
  gtk_widget_show(option_menu);

  gtk_box_pack_start(GTK_BOX(hbox),option_menu,FALSE,FALSE,1);
  gtk_widget_show(option_menu); 

  // Tile Data

  option_menu=gtk_option_menu_new();
  menu=gtk_menu_new();
  menu_item=gtk_menu_item_new_with_label("0x8800");
  gtk_menu_append(GTK_MENU(menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     GTK_SIGNAL_FUNC(cb_set_bg_tile_data),GINT_TO_POINTER(0));
  menu_item=gtk_menu_item_new_with_label("0x8000");
  gtk_menu_append(GTK_MENU(menu),menu_item);
  gtk_widget_show(menu_item);
  gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		     GTK_SIGNAL_FUNC(cb_set_bg_tile_data),GINT_TO_POINTER(1));
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu),menu);
  gtk_widget_show(option_menu);

  gtk_box_pack_start(GTK_BOX(hbox),option_menu,FALSE,FALSE,1);
  gtk_widget_show(option_menu); 
  
  
  gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,1);
  gtk_widget_show(hbox); 

  gtk_container_add(GTK_CONTAINER(frame),vbox);
  gtk_widget_show(vbox); 

  bg_info.darea=darea;
  bg_info.tile_map=0;
  bg_info.tile_data=0;

  return frame;
}

/* Tile Frame */

#define TILE_BANK_W 160
#define TILE_BANK_H 240

struct {
  GtkWidget *darea[2];
  int pal[2];
}tiles_info;

static void tiles_darea_expose_event(GtkWidget *widget,GdkEvent  *event,gpointer user_data) {
  GtkDrawingArea *darea;
  GdkDrawable *drawable;
  GdkGC *gc;
  int i,x,y;
  int bank=GPOINTER_TO_INT(user_data);
  Uint16 *pal;

  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;
  gc=widget->style->black_gc;

  pal=grey;
  if (conf.gb_type&COLOR_GAMEBOY && tiles_info.pal[bank]) {
    int p=tiles_info.pal[bank]-1;
    if (p>8) pal=pal_col_obj[p-8];
    else pal=pal_col_bck[p];    
  }
  

  clear_buffer();
  if ((bank==1 && conf.gb_type&COLOR_GAMEBOY) ||
      (bank==0)) {
    for(i=0,x=1,y=1;i<384;i++) {
      draw_tile(buffer,x,y,i,0,bank,0,0,pal);
      x=(x+10)%TILE_BANK_W;
      if (x<=2) {x=1;y+=10;}
    }
  }
  gdk_draw_image(drawable,gc,buffer,0,0,0,0,TILE_BANK_W,TILE_BANK_H);
  
}

static void cb_set_tiles_pal(GtkWidget *widget,gpointer data) {
  int v=GPOINTER_TO_INT(data);
  int bank=(v&0x80)>>7;
  int pal=(v&0x7f);
  tiles_info.pal[bank]=pal;
  gtk_widget_draw(tiles_info.darea[bank],NULL);
}

static GtkWidget *init_tiles_frame(void) {
  int i,j;
  GtkWidget *frame;
  GtkWidget *darea;
  GtkWidget *vbox,*hbox;
  GtkWidget *option_menu;
  GtkWidget *menu;
  GtkWidget *menu_item;
  char *pal_name[]={"Bck",
		    "Col0","Col1","Col2","Col3","Col4","Col5","Col6","Col7",
		    "ObjCol0","ObjCol1","ObjCol2","ObjCol3","ObjCol4","ObjCol5","ObjCol6","ObjCol7"};
  int nelem=sizeof(pal_name)/sizeof(char *);

  frame=gtk_frame_new(NULL);
  hbox=gtk_hbox_new(FALSE,1);

  // Drawing Areas

  for(i=0;i<2;i++) {
    vbox=gtk_vbox_new(FALSE,1);
    darea=gtk_drawing_area_new();
    gtk_widget_set_usize (darea,TILE_BANK_W,TILE_BANK_H);
    gtk_signal_connect (GTK_OBJECT (darea),
			"expose_event",
			GTK_SIGNAL_FUNC (tiles_darea_expose_event),
			GINT_TO_POINTER(i));
    gtk_box_pack_start(GTK_BOX(vbox),darea,FALSE,FALSE,1);
    gtk_widget_show(darea);
    tiles_info.darea[i]=darea;
    
    gtk_box_pack_start(GTK_BOX(hbox),vbox,TRUE,TRUE,1);
    gtk_widget_show(vbox);

    // Tile Map
    
    option_menu=gtk_option_menu_new();
    menu=gtk_menu_new();
    for(j=0;j<nelem;j++) {
      menu_item=gtk_menu_item_new_with_label(pal_name[j]);
      gtk_menu_append(GTK_MENU(menu),menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
		       GTK_SIGNAL_FUNC(cb_set_tiles_pal),GINT_TO_POINTER(i<<7|j));
      gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu),menu);
      gtk_widget_show(option_menu);
    }
    gtk_box_pack_start(GTK_BOX(vbox),option_menu,FALSE,FALSE,1);
    gtk_widget_show(option_menu);
  }
  
  gtk_container_add(GTK_CONTAINER(frame),hbox);
  gtk_widget_show(hbox); 
  return frame; 
}


// CallBack

static void cb_toggle(GtkWidget *widget,gpointer data) {
  if (GTK_TOGGLE_BUTTON(widget)->active)
    *((char *)data)=1;
  else *((char *)data)=0;
}

static void vram_refresh(GtkWidget *widget,gpointer data) {
  Uint8 t=CURLINE;
  clear_screen();
  for(CURLINE=0;CURLINE<0x90;CURLINE++) {
    get_nb_spr();
    draw_screen();
  }
  CURLINE=t;
  blit_screen();
}

static void nb_append_page(GtkWidget *nb,GtkWidget *widget,char *text) {
  GtkWidget *label;

  label=gtk_label_new(text);
  gtk_notebook_append_page(GTK_NOTEBOOK(nb),widget,label);
  gtk_widget_show(label);
  gtk_widget_show(widget);
}


GtkWidget *dbg_vram_win_create(void) {
  GtkWidget *win;
  GtkWidget *notebook;
  GtkWidget *frame;
  GtkWidget *toggle;
  GtkWidget *button;
  GtkWidget *vbox,*hbox;
 
  win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win),"Vram");
  gtk_signal_connect_object(GTK_OBJECT(win),"delete_event",
			    GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(win));
  gtk_widget_show(win);

  hbox=gtk_hbox_new(FALSE,0);

  vbox=gtk_vbox_new(FALSE,0);

  toggle=gtk_toggle_button_new_with_label("Background");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle),TRUE);
  gtk_box_pack_start(GTK_BOX(vbox),toggle,FALSE,FALSE,1);
  gtk_signal_connect(GTK_OBJECT(toggle),"toggled",
		     GTK_SIGNAL_FUNC(cb_toggle),(gpointer)(&active_back));
  gtk_widget_show(toggle);
  
  toggle=gtk_toggle_button_new_with_label("Window");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle),TRUE);
  gtk_box_pack_start(GTK_BOX(vbox),toggle,FALSE,FALSE,1);
  gtk_signal_connect(GTK_OBJECT(toggle),"toggled",
		     GTK_SIGNAL_FUNC(cb_toggle),(gpointer)(&active_win));
  gtk_widget_show(toggle);

  toggle=gtk_toggle_button_new_with_label("Object");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle),TRUE);
  gtk_box_pack_start(GTK_BOX(vbox),toggle,FALSE,FALSE,1);
  gtk_signal_connect(GTK_OBJECT(toggle),"toggled",
		     GTK_SIGNAL_FUNC(cb_toggle),(gpointer)(&active_obj));
  gtk_widget_show(toggle);
  

  button=gtk_button_new_with_label("Refresh");
  gtk_box_pack_start(GTK_BOX(vbox),button,FALSE,FALSE,1);
  gtk_signal_connect(GTK_OBJECT(button),"clicked",
		     GTK_SIGNAL_FUNC(vram_refresh),NULL);
  gtk_widget_show(button);

  gtk_box_pack_start(GTK_BOX(hbox),vbox,FALSE,FALSE,1);
  gtk_widget_show(vbox);

  // NoteBook

  notebook=gtk_notebook_new();

  // BackGround
  
  frame=init_bg_frame();
  nb_append_page(notebook,frame,"Bg");

  // Tile

  frame=init_tiles_frame();
  nb_append_page(notebook,frame,"Tiles");
     
  gtk_box_pack_start(GTK_BOX(hbox),notebook,FALSE,FALSE,1);
  gtk_widget_show(notebook);

  gtk_container_add(GTK_CONTAINER(win),hbox);
  gtk_widget_show(hbox);
  
  // Visual & buffer

  visual=gdk_visual_get_best_with_depth(16);
  buffer=gdk_image_new(GDK_IMAGE_NORMAL,visual,256,256);
    

  return win;
}
