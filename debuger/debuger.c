#include <gtk/gtk.h>
#include <glib.h>
#include <glade/glade.h>
#include "debuger.h"
#include "../cpu.h"
#include "../vram.h"
#include "../memory.h"
#include "../rom.h"
#include "../interrupt.h"

#define REG_AF_ENTRY (app.cpu_info.reg_AF_entry)
#define REG_BC_ENTRY (app.cpu_info.reg_BC_entry)
#define REG_DE_ENTRY (app.cpu_info.reg_DE_entry)
#define REG_HL_ENTRY (app.cpu_info.reg_HL_entry)
#define REG_PC_ENTRY (app.cpu_info.reg_PC_entry)
#define REG_SP_ENTRY (app.cpu_info.reg_SP_entry)

typedef enum {
  DREG_AF,
  DREG_BC,
  DREG_DE,
  DREG_HL,
  DREG_PC,
  DREG_SP,
  DREG_F
}DREG_ID;  

typedef struct {
  int id;
  char *name;
}DREG_TAB;

DREG_TAB reg_tab[]={
  {DREG_AF,"AF"},
  {DREG_BC,"BC"},
  {DREG_DE,"DE"},
  {DREG_HL,"HL"},
  {DREG_PC,"PC"},
  {DREG_SP,"SP"},
  {DREG_F,"F"},
  {-1,NULL}};

typedef struct {  
  UINT8 op;
  char id[20];
  void (*aff_op)(int op,UINT16 *pc,char *ret);
}OP_ID;

OP_ID tab_op[512];

void aff_op0(int op,UINT16 *pc,char *ret);
void aff_op1(int op,UINT16 *pc,char *ret);
void aff_op2(int op,UINT16 *pc,char *ret); 
void aff_op3(int op,UINT16 *pc,char *ret);

/******************* aff op ************************************/

void aff_op0(int op,UINT16 *pc,char *ret)  // 0 operande
{
  strcpy(ret,tab_op[op].id);
}
  
void aff_op1(int op,UINT16 *pc,char *ret)  // 1 operande 1 byte
{
  int i=0;
  char a[4];
  strcpy(ret,tab_op[op].id);
  while(ret[i]!='n') i++;
  sprintf(a,"%02x",mem_read(*pc+1));
  memcpy(ret+i,a,2);
  (*pc)+=1;
}

void aff_op2(int op,UINT16 *pc,char *ret)   // 1 operande 2 byte
{
  int i=0;
  char a[4];
  strcpy(ret,tab_op[op].id);
  while(ret[i]!='n') i++;
  sprintf(a,"%02x",mem_read(*pc+2));
  memcpy(ret+i,a,2);
  sprintf(a,"%02x",mem_read(*pc+1));
  memcpy(ret+i+2,a,2);
  (*pc)+=2;
}

void aff_op3(int op,UINT16 *pc,char *ret)  // operande BC
{
  int i=mem_read(*pc+1);
  strcpy(ret,tab_op[i+256].id);
  (*pc)+=1;
}

/**************** Divers ***************************/

int get_reg(char *name) {
  int i=0;
  
  while(reg_tab[i].id>=0) {
    if (!strcmp(reg_tab[i].name,name)) return reg_tab[i].id;
    i++;
  }
  return -1;
}

UINT16 pc_inc_dec(UINT16 pc,INT16 d) {
  char p[30];

  pc+=d;
  return pc;
}

UINT16 get_hex1(char a) {
  switch(a) {
  case '0':return 0x00;
  case '1':return 0x01;
  case '2':return 0x02;
  case '3':return 0x03;
  case '4':return 0x04;
  case '5':return 0x05;
  case '6':return 0x06;
  case '7':return 0x07;
  case '8':return 0x08;
  case '9':return 0x09; 
  case 'a':
  case 'A':return 0x0a;
  case 'b':
  case 'B':return 0x0b;
  case 'c':
  case 'C':return 0x0c;
  case 'd':
  case 'D':return 0x0d;
  case 'e':
  case 'E':return 0x0e;
  case 'f':
  case 'F':return 0x0f;
  default:return 0x00;
  }
}

UINT16 read_hex4(char *t) {
  UINT16 a=get_hex1(t[0])<<12;
  UINT16 b=get_hex1(t[1])<<8;
  UINT16 c=get_hex1(t[2])<<4;

  return a|b|c|get_hex1(t[3]);  
}

/**************** Draw *****************************/
 
void draw_tile(GtkPreview *preview,int x,int y,UINT8 *tp,UINT16 pal[4]) {
  int sx,sy,bit0,bit1,c;
  UINT8 buf[3*8];
  
  for(sy=y;sy<y+8;sy++,tp+=2) {
    for(sx=0;sx<8;sx++) {
      int wbit;
      wbit=sx;
      bit0=((*tp)&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      bit1=((*(tp+1))&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
      c=(bit1<<1)|bit0;
      buf[sx*3]=((pal[c]&0xf800)>>11)<<3;
      buf[sx*3+1]=((pal[c]&0x07c0)>>6)<<3;
      buf[sx*3+2]=((pal[c]&0x001f))<<3;
    }
    gtk_preview_draw_row(GTK_PREVIEW(preview),buf,x,sy,8);
  }  
}

void draw_tiles_bk(void) {
  int i,x,y;
  UINT8 *buf;
  
  if (nb_vram_page>0) {
    buf=vram_page[0];
    x=y=2;
    for(i=0;i<384;i++,buf+=16) {
      draw_tile(GTK_PREVIEW(app.buf_info.tiles_bk0_preview),x,y,buf,
		app.buf_info.pal0);
      x+=10;
      if (x>160) {
	x=2;
	y+=10;
      }
    }
  }
  
  if (nb_vram_page>1) {
    buf=vram_page[1];
    x=y=2;
    for(i=0;i<384;i++,buf+=16) {
      draw_tile(GTK_PREVIEW(app.buf_info.tiles_bk1_preview),x,y,buf,
		app.buf_info.pal1);
      x+=10;
      if (x>160) {
	x=2;
	y+=10;
      }
    }
  }
  gtk_widget_queue_draw(GTK_WIDGET(app.buf_info.tiles_bk0_preview));
  gtk_widget_queue_draw(GTK_WIDGET(app.buf_info.tiles_bk1_preview));
}

void draw_back_all(void) {
  int i,j,x,y;
  UINT8 *tb,*att_tb,*tp,p;
  INT16 no_tile,att;
  
  if (nb_vram_page>1) {
    if (LCDCCONT&0x08) {// select Tile Map
      tb=&vram_page[0][0x1c00];
      att_tb=&vram_page[1][0x1c00];
    } else {
      tb=&vram_page[0][0x1800];
      att_tb=&vram_page[1][0x1800];
    }
    x=y=0;
    for(j=0;j<32;j++) 
      for(i=0;i<32;i++) {
	no_tile=*tb++;
	att=*att_tb++;
	p=att&0x07;
	if (!(LCDCCONT&0x10)) no_tile=256+(signed char)no_tile;
	tp=&vram_page[(att&0x08)>>3][(no_tile<<4)];
	draw_tile(GTK_PREVIEW(app.buf_info.back_preview),i*8,j*8,tp,
		  pal_col_bck[p]);
      }
  }  
}

/*************** Memory ****************************/

void print_mem(GtkCList *clist,UINT8 *buf,UINT16 begin,INT16 size) {
  char *text[17];
  int i,n;

  gtk_clist_freeze(clist);
  if (buf!=wram_page[active_wram_page]) gtk_clist_clear(clist);
  n=0;
  while(size>0) {
    text[0]=g_strdup_printf("%04x",begin);
    for(i=0;i<16;i++)
      text[i+1]=g_strdup_printf("%02x",buf[n++]);
    gtk_clist_append(clist,text);
    size-=16;
    begin+=16;
  }
  gtk_clist_columns_autosize(clist);
  gtk_clist_thaw(clist);
}


/**************** Gtk Callback *********************/

void on_open_activate(GtkMenuItem *menuitem,gpointer user_data) {
  gtk_widget_show(app.file_selector);
}

void on_run_activate(GtkMenuItem *menuitem,gpointer user_data) {
  run();
}

void on_next_activate(GtkMenuItem *menuitem,gpointer user_data) {
  next();
}

void on_quit_activate(GtkMenuItem *menuitem,gpointer user_data) {
  quit_app();
}

void on_cpu_entry_activate(GtkEditable *editable,gpointer user_data) {
  switch(get_reg((char *)user_data)) {
  case DREG_AF:
    sscanf(gtk_entry_get_text(GTK_ENTRY(editable)),"%x",&gbcpu->af.w);
    printf("set AF to %x \n",gbcpu->af.w);
    break;
  case DREG_BC:
    sscanf(gtk_entry_get_text(GTK_ENTRY(editable)),"%x",&gbcpu->bc.w);
    printf("set BC to %x \n",gbcpu->bc.w);
    break; 
  case DREG_DE:
    sscanf(gtk_entry_get_text(GTK_ENTRY(editable)),"%x",&gbcpu->de.w);
    printf("set DE to %x \n",gbcpu->de.w);
    break; 
  case DREG_HL:
    sscanf(gtk_entry_get_text(GTK_ENTRY(editable)),"%x",&gbcpu->hl.w);
    printf("set HL to %x \n",gbcpu->hl.w);
    break; 
  case DREG_PC:
    sscanf(gtk_entry_get_text(GTK_ENTRY(editable)),"%x",&gbcpu->pc.w);
    printf("set PC to %x \n",gbcpu->pc.w);
    break; 
  case DREG_SP:
    sscanf(gtk_entry_get_text(GTK_ENTRY(editable)),"%x",&gbcpu->sp.w);
    printf("set SP to %x \n",gbcpu->sp.w);
    break;
  default:break;
  }
}

void on_file_selector_ok_but_clicked(GtkButton *button,gpointer user_data) {

  clear_memory();

  if (open_rom(gtk_file_selection_get_filename(GTK_FILE_SELECTION(app.file_selector)))) {
    return;
  }
  gbcpu_init();
  init_gb_memory(SDL_JoystickName(conf.joy_no));
  if (gameboy_type&COLOR_GAMEBOY) draw_screen=draw_screen_col;
  else draw_screen=draw_screen_wb;
  gtk_widget_hide(app.file_selector);
  update_all();
}

void on_code_adj_changed(GtkAdjustment *adj,gpointer data) {
  static UINT16 last;
  last=app.code_info.begin;  
  app.code_info.begin=pc_inc_dec(last,(UINT16)adj->value-last);
  update_code_info(app.code_info.begin);
}

void on_code_clist_select_row(GtkCList *clist,gint row,gint column,GdkEvent *event,gpointer user_data) {
  UINT16 pc;
  char *text;
  
  if (!gtk_clist_get_text(clist,row,0,&text)) return;
  pc=read_hex4(text);
  if (!is_breakpoint(pc)) {
    printf("set breakpoint at %04x\n",pc);
    set_breakpoint(pc);   
  }  else {
    printf("clear breakpoint at %04x\n",pc);
    clear_breakpoint(pc);    
  }
  update_code_info(app.code_info.begin);    
}

void on_code_clist_unselect_row(GtkCList *clist,gint row,gint column,GdkEvent *event,gpointer user_data) {
  UINT16 pc;
  char *text;
  
  if (!gtk_clist_get_text(clist,row,0,&text)) return;
  pc=read_hex4(text);
  if (is_breakpoint(pc)) {
    printf("clear breakpoint at %04x\n",pc);
    clear_breakpoint(pc);    
  }
  update_code_info(app.code_info.begin);  
}

void on_mem_notebook_switch_page(GtkNotebook *notebook,GtkNotebookPage *page,gint page_num,gpointer user_data) {
  app.mem_info.page=page_num;
  update_mem_info();
}

void on_active_page_entry_activate(GtkEditable *editable,gpointer user_data) {
  UINT16 a=0;
  char *txt;
  sscanf(gtk_entry_get_text(GTK_ENTRY(editable)),"%d",&a);
  if (!strcmp((char *)user_data,"ROM")) {
    if (a>nb_rom_page) a=nb_rom_page-1;
    active_rom_page=a;
    printf("set active rom page: %d \n",a); 
  }
  gtk_entry_set_text(GTK_ENTRY(editable),g_strdup_printf("%d",a));
  update_mem_info();
}

void on_buf_notebook_switch_page(GtkNotebook *notebook,GtkNotebookPage *page,gint page_num,gpointer user_data) {
  app.buf_info.page=page_num;
  update_buf_info();
}

void on_bk0_conbo_entry_changed(GtkEditable *editable,gpointer user_data) {
  /* Plus BOURRIN tu meurs */
  char *t=gtk_editable_get_chars(editable,0,-1);
  if (!strcmp(t,"bck")) app.buf_info.pal0=pal_bck;
  else if (!strcmp(t,"obj0")) app.buf_info.pal0=pal_obj0;
  else if (!strcmp(t,"obj1")) app.buf_info.pal0=pal_obj0;
  else if (!strcmp(t,"col_bck0")) app.buf_info.pal0=pal_col_bck[0];
  else if (!strcmp(t,"col_bck1")) app.buf_info.pal0=pal_col_bck[1];
  else if (!strcmp(t,"col_bck2")) app.buf_info.pal0=pal_col_bck[2];
  else if (!strcmp(t,"col_bck3")) app.buf_info.pal0=pal_col_bck[3];
  else if (!strcmp(t,"col_bck4")) app.buf_info.pal0=pal_col_bck[4];
  else if (!strcmp(t,"col_bck5")) app.buf_info.pal0=pal_col_bck[5];
  else if (!strcmp(t,"col_bck6")) app.buf_info.pal0=pal_col_bck[6];
  else if (!strcmp(t,"col_bck7")) app.buf_info.pal0=pal_col_bck[7];
  else if (!strcmp(t,"col_obj0")) app.buf_info.pal0=pal_col_obj[0];
  else if (!strcmp(t,"col_obj1")) app.buf_info.pal0=pal_col_obj[1];
  else if (!strcmp(t,"col_obj2")) app.buf_info.pal0=pal_col_obj[2];
  else if (!strcmp(t,"col_obj3")) app.buf_info.pal0=pal_col_obj[3];
  else if (!strcmp(t,"col_obj4")) app.buf_info.pal0=pal_col_obj[4];
  else if (!strcmp(t,"col_obj5")) app.buf_info.pal0=pal_col_obj[5];
  else if (!strcmp(t,"col_obj6")) app.buf_info.pal0=pal_col_obj[6];
  else if (!strcmp(t,"col_obj7")) app.buf_info.pal0=pal_col_obj[7];
  update_buf_info();
}

void on_bk1_conbo_entry_changed(GtkEditable *editable,gpointer user_data) {
  char *t=gtk_editable_get_chars(editable,0,-1);
  if (!strcmp(t,"bck")) app.buf_info.pal1=pal_bck;
  else if (!strcmp(t,"obj0")) app.buf_info.pal1=pal_obj0;
  else if (!strcmp(t,"obj1")) app.buf_info.pal1=pal_obj0;
  else if (!strcmp(t,"col_bck0")) app.buf_info.pal1=pal_col_bck[0];
  else if (!strcmp(t,"col_bck1")) app.buf_info.pal1=pal_col_bck[1];
  else if (!strcmp(t,"col_bck2")) app.buf_info.pal1=pal_col_bck[2];
  else if (!strcmp(t,"col_bck3")) app.buf_info.pal1=pal_col_bck[3];
  else if (!strcmp(t,"col_bck4")) app.buf_info.pal1=pal_col_bck[4];
  else if (!strcmp(t,"col_bck5")) app.buf_info.pal1=pal_col_bck[5];
  else if (!strcmp(t,"col_bck6")) app.buf_info.pal1=pal_col_bck[6];
  else if (!strcmp(t,"col_bck7")) app.buf_info.pal1=pal_col_bck[7];
  else if (!strcmp(t,"col_obj0")) app.buf_info.pal1=pal_col_obj[0];
  else if (!strcmp(t,"col_obj1")) app.buf_info.pal1=pal_col_obj[1];
  else if (!strcmp(t,"col_obj2")) app.buf_info.pal1=pal_col_obj[2];
  else if (!strcmp(t,"col_obj3")) app.buf_info.pal1=pal_col_obj[3];
  else if (!strcmp(t,"col_obj4")) app.buf_info.pal1=pal_col_obj[4];
  else if (!strcmp(t,"col_obj5")) app.buf_info.pal1=pal_col_obj[5];
  else if (!strcmp(t,"col_obj6")) app.buf_info.pal1=pal_col_obj[6];
  else if (!strcmp(t,"col_obj7")) app.buf_info.pal1=pal_col_obj[7];
  update_buf_info();
}


/**************** app **************************/

void init_app(void) {
  app.xml_main=glade_xml_new("./debuger.glade",NULL);  
  glade_xml_signal_autoconnect(app.xml_main);
  /* CPU_INFO */
  REG_AF_ENTRY=glade_xml_get_widget(app.xml_main,"cpu_AF_entry");
  REG_BC_ENTRY=glade_xml_get_widget(app.xml_main,"cpu_BC_entry");
  REG_DE_ENTRY=glade_xml_get_widget(app.xml_main,"cpu_DE_entry");
  REG_HL_ENTRY=glade_xml_get_widget(app.xml_main,"cpu_HL_entry");
  REG_PC_ENTRY=glade_xml_get_widget(app.xml_main,"cpu_PC_entry");
  REG_SP_ENTRY=glade_xml_get_widget(app.xml_main,"cpu_SP_entry");
  app.cpu_info.FLAG_label=glade_xml_get_widget(app.xml_main,"cpu_FLAG_label");
  app.cpu_info.IME_label=glade_xml_get_widget(app.xml_main,"cpu_IME_label");
  app.cpu_info.STATE_label=glade_xml_get_widget(app.xml_main,"cpu_STATE_label");
  app.cpu_info.MODE_label=glade_xml_get_widget(app.xml_main,"cpu_MODE_label");

  /* CODE_INFO */
  app.code_info.clist=glade_xml_get_widget(app.xml_main,"code_clist");
  app.code_info.vscroolbar=glade_xml_get_widget(app.xml_main,"code_vscroolbar");
  app.code_info.adj=gtk_range_get_adjustment(GTK_RANGE(app.code_info.vscroolbar));
  gtk_signal_connect(GTK_OBJECT(app.code_info.adj),"value_changed",
		     GTK_SIGNAL_FUNC(on_code_adj_changed),NULL);
  app.code_info.begin=0;
  app.code_info.last=0;

  /* LCDC INFO */

  app.lcdc_info.curline_label=glade_xml_get_widget(app.xml_main,
						   "lcdc_CURLINE_label");
  app.lcdc_info.cmpline_label=glade_xml_get_widget(app.xml_main,
						   "lcdc_CMPLINE_label");
  app.lcdc_info.lcdccont_label=glade_xml_get_widget(app.xml_main,
						    "lcdc_LCDCCONT_label");
  app.lcdc_info.lcdcstat_label=glade_xml_get_widget(app.xml_main,
						    "lcdc_LCDCSTAT_label");
  app.lcdc_info.lcdccycle_label=glade_xml_get_widget(app.xml_main,
						     "lcdc_cycle_label");
  /* INT INFO */

   app.int_info.ienable_label=glade_xml_get_widget(app.xml_main,
						   "int_IENABLE_label");
   app.int_info.iflag_label=glade_xml_get_widget(app.xml_main,
						 "int_IFLAG_label");
   
   /* BUF INFO */

   app.buf_info.tiles_bk0_preview=glade_xml_get_widget(app.xml_main,
						       "tile_bk0_preview");
   app.buf_info.tiles_bk1_preview=glade_xml_get_widget(app.xml_main,
						       "tile_bk1_preview");
   app.buf_info.back_preview=glade_xml_get_widget(app.xml_main,
						  "back_preview");
   app.buf_info.pal0=app.buf_info.pal1=pal_bck;

   /* MEM INFO */

   app.mem_info.page=0;
   app.mem_info.rom_0_clist=glade_xml_get_widget(app.xml_main,"rom_0_clist");
   app.mem_info.rom_n_clist=glade_xml_get_widget(app.xml_main,"rom_n_clist");
   app.mem_info.ram_clist=glade_xml_get_widget(app.xml_main,"ram_clist");
   app.mem_info.vram_clist=glade_xml_get_widget(app.xml_main,"vram_clist");
   app.mem_info.wram_clist=glade_xml_get_widget(app.xml_main,"wram_clist");
   app.mem_info.oam_clist=glade_xml_get_widget(app.xml_main,"oam_clist");
   app.mem_info.himem_clist=glade_xml_get_widget(app.xml_main,"himem_clist");
   
   app.mem_info.nb_rom_page_label=glade_xml_get_widget(app.xml_main,"nb_rom_page_label");
   app.mem_info.nb_ram_page_label=glade_xml_get_widget(app.xml_main,"nb_ram_page_label");
   app.mem_info.nb_vram_page_label=glade_xml_get_widget(app.xml_main,"nb_vram_page_label");
   app.mem_info.nb_wram_page_label=glade_xml_get_widget(app.xml_main,"nb_wram_page_label");
   app.mem_info.active_rom_entry=glade_xml_get_widget(app.xml_main,"active_rom_entry");
   app.mem_info.active_ram_entry=glade_xml_get_widget(app.xml_main,"active_ram_entry");
   app.mem_info.active_vram_entry=glade_xml_get_widget(app.xml_main,"active_vram_entry");
   app.mem_info.active_wram_entry=glade_xml_get_widget(app.xml_main,"active_wram_entry");
   
   app.file_selector=glade_xml_get_widget(app.xml_main,"file_selector");
   gtk_file_selection_set_filename(GTK_FILE_SELECTION(app.file_selector),
				   "/mnt/DOS_hdb5/emulator/gbgame/");  
  app.break_list=NULL;  
}

void quit_app(void) {
  
  /* GNGB */
  if (rom_page) free_mem_page(rom_page,nb_rom_page);
  if (ram_page) free_mem_page(ram_page,nb_ram_page);
  if (vram_page) free_mem_page(vram_page,nb_vram_page);
  if (wram_page) free_mem_page(wram_page,nb_wram_page);
  close_vram();
  if (conf.sound) close_sound();
  /* DEBUGER */
  exit(0);
}

/*************************** update *******************/

void update_all(void) {
  update_cpu_info();
  update_lcdc_info();
  update_int_info();
  update_buf_info();
  update_mem_info();
  if (gbcpu->pc.w<app.code_info.begin || 
      gbcpu->pc.w>app.code_info.last)
    update_code_info(gbcpu->pc.w);
  else update_code_info(app.code_info.begin);
}

void update_cpu_info(void) {
  char c[6];
  char str[5];
  sprintf(c,"%04x",gbcpu->af.w);
  gtk_entry_set_text(GTK_ENTRY(REG_AF_ENTRY),c);
  sprintf(c,"%04x",gbcpu->bc.w);
  gtk_entry_set_text(GTK_ENTRY(REG_BC_ENTRY),c);
  sprintf(c,"%04x",gbcpu->hl.w);
  gtk_entry_set_text(GTK_ENTRY(REG_HL_ENTRY),c);
  sprintf(c,"%04x",gbcpu->de.w);
  gtk_entry_set_text(GTK_ENTRY(REG_DE_ENTRY),c);
  sprintf(c,"%04x",gbcpu->pc.w);
  gtk_entry_set_text(GTK_ENTRY(REG_PC_ENTRY),c);
  sprintf(c,"%04x [%02x%02x]",gbcpu->sp.w,mem_read(gbcpu->sp.w+1),mem_read(gbcpu->sp.w));
  gtk_entry_set_text(GTK_ENTRY(REG_SP_ENTRY),c);  
  
  str[0]=0;
  if (gbcpu->af.b.l&0x80) strcat(str,"Z"); else strcat(str,"z");
  if (gbcpu->af.b.l&0x40) strcat(str,"N"); else strcat(str,"n");
  if (gbcpu->af.b.l&0x20) strcat(str,"H"); else strcat(str,"h");
  if (gbcpu->af.b.l&0x10) strcat(str,"C"); else strcat(str,"c");
  gtk_label_set_text(GTK_LABEL(app.cpu_info.FLAG_label),str);

  (gbcpu->int_flag)?gtk_label_set_text(GTK_LABEL(app.cpu_info.IME_label),"1"):gtk_label_set_text(GTK_LABEL(app.cpu_info.IME_label),"0");
  (gbcpu->state==HALT_STATE)?gtk_label_set_text(GTK_LABEL(app.cpu_info.STATE_label),"Halt"):gtk_label_set_text(GTK_LABEL(app.cpu_info.STATE_label),"Run");
  (gbcpu->mode==SIMPLE_SPEED)?gtk_label_set_text(GTK_LABEL(app.cpu_info.MODE_label),"Simple Speed"):gtk_label_set_text(GTK_LABEL(app.cpu_info.MODE_label),"Double Speed");
}

void update_code_info(UINT16 pc) {
  GtkWidget *c=app.code_info.clist;
  char add[6],code[30],comment[30];
  char *text[3];
  UINT32 line=pc,id,i=0,row,last; 
  GdkColor red,blue;

  app.code_info.begin=pc;

  red.red=0xffff;
  red.blue=red.green=0;

  blue.blue=0xffff;
  blue.green=blue.red=0;

  gtk_clist_freeze(GTK_CLIST(c));
  gtk_clist_clear(GTK_CLIST(c));

  text[0]=add;
  text[1]=code;
  text[2]=comment;

  sprintf(comment,"");
  
  while(i<15) {
    sprintf(add,"%04x",line);
    last=line;
    tab_op[id=mem_read(line)].aff_op(id,&line,code);
    row=gtk_clist_append(GTK_CLIST(c),text);
    if (is_breakpoint(last)) 
      gtk_clist_set_background(GTK_CLIST(c),row,&red);
    if (last==gbcpu->pc.w) 
      gtk_clist_set_background(GTK_CLIST(c),row,&blue);
    line++;
    i++;
  }
  app.code_info.last=line-1;
  gtk_clist_thaw(GTK_CLIST(c));
  gtk_adjustment_set_value(GTK_ADJUSTMENT(app.code_info.adj),
			   (float)app.code_info.begin);
}

void update_lcdc_info(void) {
  char str[6];
  sprintf(str,"%02x",CURLINE);
  gtk_label_set_text(GTK_LABEL(app.lcdc_info.curline_label),str);
  sprintf(str,"%02x",CMP_LINE);
  gtk_label_set_text(GTK_LABEL(app.lcdc_info.cmpline_label),str);
  sprintf(str,"%02x",LCDCCONT);
  gtk_label_set_text(GTK_LABEL(app.lcdc_info.lcdccont_label),str);
  sprintf(str,"%02x",LCDCSTAT);
  gtk_label_set_text(GTK_LABEL(app.lcdc_info.lcdcstat_label),str);
  sprintf(str,"%d",gblcdc->cycle_todo);
  gtk_label_set_text(GTK_LABEL(app.lcdc_info.lcdccycle_label),str);
}

void update_int_info(void) {
  char str[4];
  sprintf(str,"%02x",INT_ENABLE);
  gtk_label_set_text(GTK_LABEL(app.int_info.ienable_label),str);
  sprintf(str,"%02x",INT_FLAG);
  gtk_label_set_text(GTK_LABEL(app.int_info.iflag_label),str);
}

void update_buf_info(void) {
  switch(app.buf_info.page) {
  case 0:break;
  case 1:draw_tiles_bk();break;
  case 2:draw_back_all();break;
  }
}

void update_mem_info(void) {
  switch(app.mem_info.page) {
  case 0:
    if (nb_rom_page>0)
      print_mem(GTK_CLIST(app.mem_info.rom_0_clist),rom_page[0],0x0,0x4000);
    break;
  case 1:
    if (nb_rom_page>0)
      print_mem(GTK_CLIST(app.mem_info.rom_n_clist),rom_page[active_rom_page],0x4000,0x4000);
    gtk_label_set_text(GTK_LABEL(app.mem_info.nb_rom_page_label),g_strdup_printf("%d",nb_rom_page));
    gtk_entry_set_text(GTK_ENTRY(app.mem_info.active_rom_entry),g_strdup_printf("%d",active_rom_page));
    break;
  case 2:
    if (nb_vram_page>0)
      print_mem(GTK_CLIST(app.mem_info.vram_clist),vram_page[active_vram_page],0x8000,0x2000);
    gtk_label_set_text(GTK_LABEL(app.mem_info.nb_vram_page_label),g_strdup_printf("%d",nb_vram_page));
    gtk_entry_set_text(GTK_ENTRY(app.mem_info.active_vram_entry),g_strdup_printf("%d",active_vram_page));
    break;
  case 3:
    if (nb_ram_page>0)
      print_mem(GTK_CLIST(app.mem_info.ram_clist),ram_page[active_ram_page],0xa000,0x2000);
    gtk_label_set_text(GTK_LABEL(app.mem_info.nb_ram_page_label),g_strdup_printf("%d",nb_ram_page));
    gtk_entry_set_text(GTK_ENTRY(app.mem_info.active_ram_entry),g_strdup_printf("%d",active_ram_page));
    break;  
  case 4:
    if (nb_wram_page>0) {
      print_mem(GTK_CLIST(app.mem_info.wram_clist),wram_page[0],0xc000,0x1000);
      print_mem(GTK_CLIST(app.mem_info.wram_clist),wram_page[active_wram_page],0xd000,0x1000);
    }
    gtk_label_set_text(GTK_LABEL(app.mem_info.nb_wram_page_label),g_strdup_printf("%d",nb_wram_page));
    gtk_entry_set_text(GTK_ENTRY(app.mem_info.active_wram_entry),g_strdup_printf("%d",active_wram_page));
    break;
  case 5:
    print_mem(GTK_CLIST(app.mem_info.oam_clist),oam_space,0xfe00,0x100);
    break;
  case 6:
    print_mem(GTK_CLIST(app.mem_info.himem_clist),himem,0xff00,0xff);
    break;    
  }
}

/******************** debuger ************************/

void clear_memory(void) {
  if (rom_page) free_mem_page(rom_page,nb_rom_page);
  if (ram_page) free_mem_page(ram_page,nb_ram_page);
  if (vram_page) free_mem_page(vram_page,nb_vram_page);
  if (wram_page) free_mem_page(wram_page,nb_wram_page);
  rom_page=ram_page=vram_page=wram_page=NULL;
}

void run(void) {
  if (is_breakpoint(gbcpu->pc.w)) update_gb_one();
  while((!conf.gb_done) && (!is_breakpoint(gbcpu->pc.w))) {
    if (gbcpu->hl.w==0x2ff) {
      conf.gb_done=1;
    }
    update_gb_one();
  }
  conf.gb_done=0;
  update_all();
}

void next(void) {
  update_gb_one();
  update_all();
}

void set_breakpoint(UINT16 p) {
  app.break_list=g_slist_append(app.break_list,GINT_TO_POINTER((UINT32)p));
}

void clear_breakpoint(UINT16 p) {
  app.break_list=g_slist_remove(app.break_list,GINT_TO_POINTER((UINT32)p));
}

int is_breakpoint(UINT16 p) {
  if (g_slist_index(app.break_list,GINT_TO_POINTER((UINT32)p))>=0) 
    return 1;
  else return 0;
    
}

/*************************************************/

void init_tab_op(void)
{
  int i;
  FILE *stream;
  int aff_op=0;  
  
  for(i=0;i<512;i++) {
    tab_op[i].op=0;
    tab_op[i].id[0]=0;
    strcat(tab_op[i].id,"NOP");
  }    

  i=0;
  stream=fopen("op_id.txt","rt");
  for(i=0;i<512;i++) {
    fscanf(stream,"%x %s %d",&tab_op[i].op,tab_op[i].id,&aff_op);
    if (aff_op==1) tab_op[i].aff_op=aff_op1;
    else if (aff_op==2) tab_op[i].aff_op=aff_op2;
    else if (aff_op==3) tab_op[i].aff_op=aff_op3;
    else tab_op[i].aff_op=aff_op0;
  }
  fclose(stream);
}

int main(int argc,char **argv) {

  gtk_init(&argc,&argv);
  glade_init();

  open_log();

  init_tab_op();

  gblcdc_init();
  gbcpu_init();
  init_vram(0);

  if(SDL_NumJoysticks()>0){
    joy=SDL_JoystickOpen(conf.joy_no);
    if(joy) {
      printf("Name: %s\n", SDL_JoystickName(conf.joy_no));
      printf("Number of Axes: %d\n", SDL_JoystickNumAxes(joy));
      printf("Number of Buttons: %d\n", SDL_JoystickNumButtons(joy));
      printf("Number of Balls: %d\n", SDL_JoystickNumBalls(joy));
    }
  }
  
  init_gb_memory(SDL_JoystickName(conf.joy_no));

  init_app();
  gtk_main();

  close_log();
  quit_app();
  return 0;
}

