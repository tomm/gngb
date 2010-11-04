/*  gngb, a game boy color emulator
 *  Copyright (C) 2001 Peponas Thomas & Peponas Mathieu
 * 
 *  This program is free software; you can redistribute it and/or modify  
 *  it under the terms of the GNU General Public License as published by   
 *  the Free Software Foundation; either version 2 of the License, or    
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */


#include "global.h"
#include "cpu.h"
#include "memory.h"
#include "rom.h"
#include "vram.h"
#include "interrupt.h"

#ifdef LINUX_JOYSTICK
#include "joystick.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_PC 20

typedef struct {  
  UINT8 op;
  char id[20];
  void (*aff_op)(int op,UINT16 *pc,char *ret);
}OP_ID;

UINT8 quit_debug=0;
UINT8 force_normal_gb=0;
OP_ID tab_op[512];

void aff_op0(int op,UINT16 *pc,char *ret);
void aff_op1(int op,UINT16 *pc,char *ret);
void aff_op2(int op,UINT16 *pc,char *ret); 
void aff_op3(int op,UINT16 *pc,char *ret);

// function

void quit_gb_debug(void);
void aff_register(void);
void list_code(void);
void exec_until(void);
void dump_memory(void);
void redraw_buf(void);
void run(void);
void next_inst(void);
void set_reg(void);
void show_info(void);
void select_page(void);

void show_pal(void);
void show_tiles(int n);
void show_back();

typedef struct {
  char *id;
  void (*handler)(void);
}COMMANDE;

COMMANDE tab_com[]={
  {"q",quit_gb_debug},
  {"r",aff_register},
  {"l",list_code},
  {"u",exec_until},
  {"d",dump_memory},
  {"v",redraw_buf},
  {"e",run},
  {"n",next_inst},
  {"s",set_reg},
  {"i",show_info},
  {"b",select_page},
  {NULL,NULL}
};

char *buf_com=NULL;
char *arg=NULL;
int nb_arg=0;

#ifdef LINUX_JOYSTICK
JOY_CONTROL *my_joy;
#endif

UINT32 last_pc[MAX_PC]={0};

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

/******************* function utile *****************************/

/* WARNING arguments separate by only one space or tabulation */
int get_nb_com_args(char *a)
{
  int i;
  int nb=0;
  
  if (a[0]==0) return 0;

  i=0;
  while(a[i]!=0) {
    if ((a[i]==' ') ||  (a[i]=='\t')) nb++;
    i++;
  }
  return nb+1;
}

char *get_arg(char *com)
{
  char *ret=(char *)malloc(strlen(com));
  int i=0;
  int j=0;

  while((com[i]==' ') || (com[i]=='\t')) i++;  // supprime les espace devant
  while((com[i]!=' ') && (com[i]!='\t') && (com[i]!=0)) i++;  // supprime la comande
  
  ret[0]='\0';
  while(com[i]!='\0') {
    if ((com[i]!=' ') && (com[i]!='\t')) {
      ret[j++]=com[i];
      if ((com[i+1]==' ') || (com[i+1]=='\t')) {
	ret[j++]=' ';
      }
    }
    i++;
  }
  ret[j]='\0';
  return ret;
}  

/******************** function callback ***************************/

void quit_gb_debug(void)
{
  quit_debug=1;
}

void run(void)
{
  int i;
  while(!conf.gb_done) {    
    for(i=0;i<MAX_PC-1;i++)
      last_pc[i]=last_pc[i+1];
    last_pc[MAX_PC-1]=gbcpu->pc.w;
    update_gb_one();
  }
  conf.gb_done=0;
}    

void aff_register(void)
{
  char aff[30];
  UINT16 pc=gbcpu->pc.w;;
  tab_op[mem_read(pc)].aff_op(mem_read(pc),&pc,aff);    
  printf("[AF: %04x]    [HL: %04x]    [BC: %04x]",
	 gbcpu->af.w,
	 gbcpu->hl.w,
	 gbcpu->bc.w);
  if (gbcpu->int_flag) printf("   EI \n");
  else printf("   DI \n");
  printf("[DE: %04x]    [SP: %04x]    [PC: %04x]",
	 gbcpu->de.w,
	 gbcpu->sp.w,
	 gbcpu->pc.w);
  printf("    [");
  if (gbcpu->af.w&0x80) printf("Z"); else printf("z");
  if (gbcpu->af.w&0x40) printf("N"); else printf("n");
  if (gbcpu->af.w&0x20) printf("H"); else printf("h");
  if (gbcpu->af.w&0x10) printf("C....]\n"); else printf("c....]\n");    
  printf("[(SP): %02x%02x]   [(PC): %02x - %s ] \n",
	 mem_read(gbcpu->sp.w+1),mem_read(gbcpu->sp.w),
	 mem_read(pc),aff);
  printf("state : ");
  if (gbcpu->state==HALT_STATE) printf("halt\n");
  else printf("normal\n");
}

void list_code(void)
{
  UINT16 line,last;
  int i,id;
  char op[30];
 
  if (nb_arg==1)
    sscanf(arg,"%x",&line);
  else line=gbcpu->pc.w;
  
  for(i=0;i<10;i++) {
    op[0]=0;
    last=line;
    id=mem_read(line);
    tab_op[id].aff_op(id,&line,op);
    printf("address %x val %02x : %s \n",last,id,op);
    line++;
  }
}     

void exec_until(void)
{
  UINT16 add,i;
    
  if (nb_arg==1) 
    sscanf(arg,"%x",&add);
  else return;

  if (add==gbcpu->pc.w) {
    for(i=0;i<MAX_PC-1;i++)
      last_pc[i]=last_pc[i+1];
    last_pc[MAX_PC-1]=gbcpu->pc.w;
    update_gb_one();
  }
  
  while(!conf.gb_done && add!=gbcpu->pc.w) {
    for(i=0;i<MAX_PC-1;i++)
      last_pc[i]=last_pc[i+1];
    last_pc[MAX_PC-1]=gbcpu->pc.w;
    update_gb_one();
    if (gbcpu->sp.w<=0x1000 && gbcpu->sp.w>=0x100)  conf.gb_done=1;
  }  
  conf.gb_done=0;
}

void dump_memory(void)
{
  int i,j;
  UINT16 add;
  
  if (nb_arg!=1) return;
  sscanf(arg,"%x",&add);

  for(i=add,j=0;i<=add+0xff;i++) {
    if (j==0) printf("%04x  ",i);
    printf("%02x ",mem_read(i));
    if (j==15) {
      printf("\n");
      j=0;
    } else j++;    
  }  
}

/*void draw_tile(void) {
  UINT8 curline=CURLINE;
  UINT8 lcdccont=LCDCCONT;
  UINT8 scrx=SCRX,scry=SCRY;
  UINT8 **t=vram_page;
  UINT8 **vram;
  int i,v;

  if (nb_arg!=1) return;
  sscanf(arg,"%d",&v);
  
  vram=alloc_mem_page(nb_vram_page,0x2000);

  for(i=0;i<nb_vram_page;i++) 
    memcpy(vram[i],vram_page[i],0x2000);
    
  for(i=0x1800;i<0x2000;i++) {
    vram[0][i]=i-0x1800;
    if (nb_vram_page>1) vram[1][i]=i-0x1800;
  }

  switch(v) {
  case 0:LCDCCONT=0x81;break;
  case 1:LCDCCONT=0x91;break;
  }
  
  SCRX=0;SCRY=0;
  vram_page=vram;
  for(CURLINE=0;CURLINE<0x90;CURLINE++)
    draw_screen();

  blit_screen();

  LCDCCONT=lcdccont;
  CURLINE=curline;
  vram_page=t;  
  SCRX=scrx;SCRY=scry;
  free_mem_page(vram,nb_vram_page);
  }*/

void redraw_buf(void)
{
  int curline=CURLINE;
  int lcdccont=LCDCCONT;
  int v;

  if (nb_arg!=1) return;
  sscanf(arg,"%d",&v);
  switch(v) {
  case 0:LCDCCONT=0x81;break;
  case 1:LCDCCONT=0x89;break;
  case 2:LCDCCONT=0x91;break;
  case 3:LCDCCONT=0x99;break;
  }
  
  for(CURLINE=0;CURLINE<0x90;CURLINE++)
    draw_screen();
  blit_screen();
  LCDCCONT=lcdccont;
  CURLINE=curline;  
}

void next_inst(void)
{
  update_gb();
  aff_register();
}

void set_reg(void)
{
  UINT16 v;
  char c;
  
  if (nb_arg==2) {
    sscanf(arg,"%c %x",&c,&v);
    c=arg[0];
    switch(c) {
    case 'A':gbcpu->af.b.h=v;break;
    case 'F':gbcpu->af.b.l=v;break;
    case 'B':gbcpu->bc.b.h=v;break;
    case 'C':gbcpu->bc.b.l=v;break;  
    case 'H':gbcpu->hl.b.h=v;break;
    case 'L':gbcpu->hl.b.l=v;break;  
    case 'D':gbcpu->de.b.h=v;break;
    case 'E':gbcpu->de.b.l=v;break;  
    case 'P':gbcpu->pc.w=v;break; 
    case 'S':gbcpu->sp.w=v;break; 
    }
  }
  printf("set %c to %02x \n",c,v);
  free(arg);
}

void show_info(void)
{
  int i;
  printf("active rom page %d on %d\n",active_rom_page,nb_rom_page-1);
  printf("active ram page %d on %d\n",active_ram_page,nb_ram_page-1);
  printf("active vram page %d on %d\n",active_vram_page,nb_vram_page-1);
  printf("active wram page %d on %d\n",active_wram_page,nb_wram_page-1);
  printf("LCDCCONT %02x LCDCSTAT %02x\n",LCDCCONT,LCDCSTAT);
  printf("CURLINE %02x CMP_LINE %02x\n",CURLINE,CMP_LINE);
  printf("INT_FLAG %02x INT_ENABLE %02x \n",INT_FLAG,INT_ENABLE);
  printf("last pc : ");
  for(i=0;i<MAX_PC;i++) printf("%02x \n",last_pc[i]);
}

void select_page(void)
{
  UINT16 v;
  char c;
  
  if (nb_arg==2) {
    sscanf(arg,"%c %d",&c,&v);
    c=arg[0];
    switch(c) {
    case 'O':active_rom_page=v;break;
    case 'A':active_ram_page=v;break;
    case 'V':active_vram_page=v;break;
    case 'W':active_wram_page=v;break;  
    }
  }
  free(arg);  
}
  
/***********************************************************/


/************************************************************/

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

void check_option(int argc,char *argv[]) 
{
  char c;
  conf.normal_gb=0;
  conf.autofs=0;
  conf.sound=0;
  conf.fs=0;
  while((c=getopt(argc,argv,"gas"))!=EOF) {
    switch(c) {
      case 'g':conf.normal_gb=1;break;
      case 'a':conf.autofs=1;break;
      case 's':conf.sound=1;break;
    }
  }
} 

int main(int argc,char *argv[])
{
  int i;
  char command[10]={0};
  int flag=1;

  check_option(argc,argv);
  if (optind>=argc) exit(1);
  if (open_rom(argv[optind])) exit(1);

  gbcpu_init();
  init_vram(0);
  init_gb_memory();

#ifdef LINUX_JOYSTICK 
  my_joy=install_joy(JOY_DEVICE0);
#endif

  init_tab_op();

  while(!quit_debug) {
    buf_com=readline("GNGB COM >");
    if ((buf_com) && (buf_com[0]!=0)) {
      add_history(buf_com);
      sscanf(buf_com,"%s",command);
      if (arg) free(arg);
      arg=get_arg(buf_com);
      nb_arg=get_nb_com_args(arg);      
      free(buf_com);
    }
    i=0;
    flag=1;
    while((tab_com[i].id!=NULL) && (flag)) {
      if (!(flag=strcmp(tab_com[i].id,command)))
	tab_com[i].handler();
      i++;
    }
  }

  if (rom_type&BATTERY) save_ram();
  if (rom_page) free_mem_page(rom_page,nb_rom_page);
  if (ram_page) free_mem_page(ram_page,nb_ram_page);
  if (vram_page) free_mem_page(vram_page,nb_vram_page);
  if (wram_page) free_mem_page(wram_page,nb_wram_page);

#ifdef LINUX_JOYSTICK
  remove_joy(my_joy);
#endif

  close_vram();
  exit(0);
}





