#include "emu.h"
#include "fileio.h"
#include "rom.h"
#include "memory.h"
#include "vram.h"
#include "interrupt.h"
#include "cpu.h"

#define FILENAME_LEN 1024
static SDL_Surface *savestate_bmp=NULL;

void get_filename_ext(char *f,char *ext) {
  /*  char *a=getenv("HOME");*/
  char buf[8];
  char *a=getenv("HOME");
  if(a==NULL) {
    buf[0] = '.';
    buf[1] = 0;
    a = buf;
  }

  f[0]=0;
  strcat(f,a);
  strcat(f,"/.gngb/");
  check_dir(f);
  strcat(f,rom_name);
  strcat(f,ext);
}

char *get_name_without_ext(char *name) {
  char *a;
  int lg=strlen(name);
  int l,r,i;

  l=lg;
#ifndef GNGB_WIN32
  while(l>0 && name[l-1]!='/') l--;
#else
  while(l>0 && (name[l-1]!='\\')) l--;
#endif

  r=l;
  while(r<=lg && name[r-1]!='.') r++;

  a=(char *)malloc(sizeof(char)*(5+(r-l)));
  i=0;
  while(l<r) 
    a[i++]=name[l++];
  
  a[i-1]=0;
  return a;
}


/* Ram Load/Save */

int load_ram(void) {
  GNGB_FILE * stream;
  int i;
  char filename[FILENAME_LEN];

  get_filename_ext(filename,".sv");
  
  if (!(stream=gngb_file_open(filename,"rb",UNKNOW_FILE_TYPE))) {
    fprintf(stderr,"Error while trying to read file %s \n",filename);
    return 1;
  }
  
  for(i=0;i<nb_ram_page;i++)
    gngb_file_read(ram_page[i],sizeof(Uint8),0x2000,stream);

  gngb_file_close(stream);
  return 0;
}

int save_ram(void) {
  GNGB_FILE * stream;
  int i;
  char filename[FILENAME_LEN];
 
  get_filename_ext(filename,".sv");
 
  if (!(stream=gngb_file_open(filename,"wb",GZIP_FILE_TYPE))) {
    fprintf(stderr,"Error while trying to write file %s \n",filename);
    return 1;
  }

  for(i=0;i<nb_ram_page;i++)
    gngb_file_write(ram_page[i],sizeof(Uint8),0x2000,stream);

  gngb_file_close(stream);
  return 0;
}

/* Timer Load/Save */

int save_rom_timer(void) {
  char filename[FILENAME_LEN];
  FILE *stream;
  int i;
  
  get_filename_ext(filename,".rt");

  if (!(stream=fopen(filename,"wt"))) {
    fprintf(stderr,"Error while trying to write file %s \n",filename);
    return 1;
  }
  
  for(i=0;i<5;i++) 
    fprintf(stream,"%02x ",rom_timer->reg[i]);

  fprintf(stream,"%ld",(long)time(NULL));

  fclose(stream);
  return 0;
}

int load_rom_timer(void) {
  char filename[FILENAME_LEN];
  FILE *stream;
  int i;
  long dt;

  get_filename_ext(filename,".rt");

  if (!(stream=fopen(filename,"rt"))) {
    fprintf(stderr,"Error while trying to read file %s \n",filename);
    return 1;
  }

  /* FIXME : must adjust the time still last time */
  for(i=0;i<5;i++) {
    int t;
    fscanf(stream,"%02x",&t);
    rom_timer->reg[i]=(Uint8)t;
  }
  fscanf(stream,"%ld",&dt);

  {
    int h,m,s,d,dd;
    
    dt=time(NULL)-dt;

    s=(dt%60);
    dt/=60;
    
    rom_timer->reg[0]+=s;
    if (rom_timer->reg[0]>=60) {
      rom_timer->reg[0]-=60;
      rom_timer->reg[1]++;
    }
 
    m=(dt%60);
    dt/=60;
    
    rom_timer->reg[1]+=m;
    if (rom_timer->reg[1]>=60) {
      rom_timer->reg[1]-=60;
      rom_timer->reg[2]++;
    }
  
    h=(dt%24);
    dt/=24;

    rom_timer->reg[2]+=h;
    if (rom_timer->reg[2]>=24) {
      rom_timer->reg[2]-=24;
      rom_timer->reg[3]++;
      if (!rom_timer->reg[3])
	dt++;
    }
 
    d=dt;
    dd=((rom_timer->reg[5]&0x01)<<9)|rom_timer->reg[4];
    
    dd+=dt;
    if (dd>=365) {
      dd=0;
      rom_timer->reg[5]|=0x80;        // set carry
    }

    rom_timer->reg[4]=dd&0xff;
    rom_timer->reg[5]=(rom_timer->reg[5]&0xfe)|((dd&0x100)>>9);

  }

  fclose(stream);
  return 0;
}


/* State Load/Save */

typedef enum {
  CPU_SECTION,
  LCD_SECTION,
  PAL_SECTION,
  TIMER_SECTION,
  DMA_SECTION,
  RT_SECTION,
  PAD_MOVIE_SECTION,
  MAX_SECTION
}SECTION_TYPE;

// Save & Load function

int save_load_cpu_reg(GNGB_FILE * stream,char op);
int save_load_cpu_flag(GNGB_FILE * stream,char op);
int save_load_lcd_cycles(GNGB_FILE * stream,char op);
int save_load_lcd_info(GNGB_FILE * stream,char op);
int save_load_pal(GNGB_FILE * stream,char op);
int save_load_timer(GNGB_FILE * stream,char op);
int save_load_dma(GNGB_FILE * stream,char op);
int save_load_rt_reg(GNGB_FILE * stream,char op);
int save_load_rt_info(GNGB_FILE * stream,char op);
int save_load_pad_movie(GNGB_FILE * stream,char op);

int get_pad_movie_section_size(void);

struct sub_section  {
  Sint8 id;
  Sint16 size;
  int (*get_size)(void);
  int (*save_load_function)(GNGB_FILE * stream,char op);
}; 

struct sub_section cpu_sub_section[]={
  {0,6*2,NULL,save_load_cpu_reg},
  {1,4,NULL,save_load_cpu_flag},
  {-1,-1,NULL,NULL}};

struct sub_section lcd_sub_section[]={
  {0,3*2+4+1,NULL,save_load_lcd_cycles},
  {1,3,NULL,save_load_lcd_info},
  {-1,-1,NULL,NULL}};

struct sub_section pal_sub_section[]={
  {0,4*2*8*4+2*4+4,NULL,save_load_pal},
  {-1,-1,NULL,NULL}};

struct sub_section timer_sub_section[]={
  {0,4+2,NULL,save_load_timer},
  {-1,-1,NULL,NULL}};

struct sub_section dma_sub_section[]={
  {0,1+3*2+4,NULL,save_load_dma},
  {-1,-1,NULL,NULL}};

struct sub_section rt_sub_section[]={
  {0,5,NULL,save_load_rt_reg},
  {1,4,NULL,save_load_rt_info},
  {-1,-1,NULL,NULL}};

struct sub_section pad_movie_section[]={
  {0,-1,get_pad_movie_section_size,save_load_pad_movie},
  {-1,-1,NULL,NULL}};

struct section {
  Sint8 id;
  struct sub_section *ss;
} tab_section[]={
  {CPU_SECTION,cpu_sub_section},
  {LCD_SECTION,lcd_sub_section},
  {PAL_SECTION,pal_sub_section},
  {TIMER_SECTION,timer_sub_section},
  {DMA_SECTION,dma_sub_section},
  {RT_SECTION,rt_sub_section},
  {PAD_MOVIE_SECTION,pad_movie_section},
  {-1,NULL}};    

int save_load_cpu_reg(GNGB_FILE * stream,char op) {
  if (!op) { // write
    gngb_file_write(&gbcpu->af.w,sizeof(Uint16),1,stream);
    gngb_file_write(&gbcpu->bc.w,sizeof(Uint16),1,stream);
    gngb_file_write(&gbcpu->de.w,sizeof(Uint16),1,stream);
    gngb_file_write(&gbcpu->hl.w,sizeof(Uint16),1,stream);
    gngb_file_write(&gbcpu->sp.w,sizeof(Uint16),1,stream);
    gngb_file_write(&gbcpu->pc.w,sizeof(Uint16),1,stream);
  } else { // read
    gngb_file_read(&gbcpu->af.w,sizeof(Uint16),1,stream);
    gngb_file_read(&gbcpu->bc.w,sizeof(Uint16),1,stream);
    gngb_file_read(&gbcpu->de.w,sizeof(Uint16),1,stream);
    gngb_file_read(&gbcpu->hl.w,sizeof(Uint16),1,stream);
    gngb_file_read(&gbcpu->sp.w,sizeof(Uint16),1,stream);
    gngb_file_read(&gbcpu->pc.w,sizeof(Uint16),1,stream);
  }
  return 0;
}

int save_load_cpu_flag(GNGB_FILE * stream,char op) {
  if (!op) { // write
    gngb_file_write(&gbcpu->int_flag,sizeof(Uint8),1,stream);
    gngb_file_write(&gbcpu->ei_flag,sizeof(Uint8),1,stream);
    gngb_file_write(&gbcpu->state,sizeof(Uint8),1,stream);
    gngb_file_write(&gbcpu->mode,sizeof(Uint8),1,stream);
  } else { // read
    gngb_file_read(&gbcpu->int_flag,sizeof(Uint8),1,stream);
    gngb_file_read(&gbcpu->ei_flag,sizeof(Uint8),1,stream);
    gngb_file_read(&gbcpu->state,sizeof(Uint8),1,stream);
    gngb_file_read(&gbcpu->mode,sizeof(Uint8),1,stream);
  }
  return 0;
}

int save_load_lcd_cycles(GNGB_FILE * stream,char op) {
  if (!op) { // write 
    gngb_file_write(&gblcdc->cycle,sizeof(Sint16),1,stream);
    gngb_file_write(&gblcdc->mode1cycle,sizeof(Uint16),1,stream);
    gngb_file_write(&gblcdc->mode2cycle,sizeof(Uint16),1,stream);
    gngb_file_write(&gblcdc->vblank_cycle,sizeof(Uint32),1,stream);
    gngb_file_write(&gblcdc->timing,sizeof(Uint8),1,stream);
  } else { // read 
    gngb_file_read(&gblcdc->cycle,sizeof(Sint16),1,stream);
    gngb_file_read(&gblcdc->mode1cycle,sizeof(Uint16),1,stream);
    gngb_file_read(&gblcdc->mode2cycle,sizeof(Uint16),1,stream);
    gngb_file_read(&gblcdc->vblank_cycle,sizeof(Uint32),1,stream);
    gngb_file_read(&gblcdc->timing,sizeof(Uint8),1,stream);
  }
  return 0;
}

int save_load_lcd_info(GNGB_FILE * stream,char op) {
  if (!op) { // write 
    gngb_file_write(&gblcdc->mode,sizeof(Uint8),1,stream);
    gngb_file_write(&gblcdc->nb_spr,sizeof(Uint8),1,stream);
    gngb_file_write(&gblcdc->inc_line,sizeof(Uint8),1,stream);
  } else { // read
    gngb_file_read(&gblcdc->mode,sizeof(Uint8),1,stream);
    gngb_file_read(&gblcdc->nb_spr,sizeof(Uint8),1,stream);
    gngb_file_read(&gblcdc->inc_line,sizeof(Uint8),1,stream);
  }
  return 0;
}

int save_load_pal(GNGB_FILE * stream,char op) {
  if (!op) { // write 
    gngb_file_write(pal_col_bck_gb,sizeof(Uint16),8*4,stream);
    gngb_file_write(pal_col_obj_gb,sizeof(Uint16),8*4,stream);
    gngb_file_write(pal_col_bck,sizeof(Uint16),8*4,stream);
    gngb_file_write(pal_col_obj,sizeof(Uint16),8*4,stream);
    gngb_file_write(pal_bck,sizeof(Uint8),4,stream);
    gngb_file_write(pal_obj,sizeof(Uint8),2*4,stream);
  } else { // read
    gngb_file_read(pal_col_bck_gb,sizeof(Uint16),8*4,stream);
    gngb_file_read(pal_col_obj_gb,sizeof(Uint16),8*4,stream);
    gngb_file_read(pal_col_bck,sizeof(Uint16),8*4,stream);
    gngb_file_read(pal_col_obj,sizeof(Uint16),8*4,stream);
    gngb_file_read(pal_bck,sizeof(Uint8),4,stream);
    gngb_file_read(pal_obj,sizeof(Uint8),2*4,stream);
  }
  return 0;
}

int save_load_timer(GNGB_FILE * stream,char op) {
  if (!op) { // write 
    gngb_file_write(&gbtimer->clk_inc,sizeof(Uint16),1,stream);
    gngb_file_write(&gbtimer->cycle,sizeof(Uint32),1,stream);
  } else { // read
    gngb_file_read(&gbtimer->clk_inc,sizeof(Uint16),1,stream);
    gngb_file_read(&gbtimer->cycle,sizeof(Uint32),1,stream);
  }
  return 0;
}

int save_load_dma(GNGB_FILE * stream,char op) {
  if (!op) { // write 
    gngb_file_write(&dma_info.type,sizeof(Uint8),1,stream);
    gngb_file_write(&dma_info.src,sizeof(Uint16),1,stream);
    gngb_file_write(&dma_info.dest,sizeof(Uint16),1,stream);
    gngb_file_write(&dma_info.lg,sizeof(Uint16),1,stream);
    gngb_file_write(&dma_info.gdma_cycle,sizeof(Uint32),1,stream);
  } else { // read
    gngb_file_read(&dma_info.type,sizeof(Uint8),1,stream);
    gngb_file_read(&dma_info.src,sizeof(Uint16),1,stream);
    gngb_file_read(&dma_info.dest,sizeof(Uint16),1,stream);
    gngb_file_read(&dma_info.lg,sizeof(Uint16),1,stream);
    gngb_file_read(&dma_info.gdma_cycle,sizeof(Uint32),1,stream);
  }
  return 0;
}

int save_load_rt_reg(GNGB_FILE * stream,char op) {
  int t;
  if (!op) { // write 
    for(t=0;t<5;t++)
      gngb_file_write(&rom_timer->reg[t],sizeof(Uint8),1,stream);
  } else { // read
    for(t=0;t<5;t++)
      gngb_file_read(&rom_timer->reg[t],sizeof(Uint8),1,stream);
  }
  return 0;
}

int save_load_rt_info(GNGB_FILE * stream,char op) {
  if (!op) { // write 
    gngb_file_write(&rom_timer->cycle,sizeof(Uint16),1,stream);
    gngb_file_write(&rom_timer->reg_sel,sizeof(Uint8),1,stream);
    gngb_file_write(&rom_timer->latch,sizeof(Uint8),1,stream);
  } else { // read
    gngb_file_read(&rom_timer->cycle,sizeof(Uint16),1,stream);
    gngb_file_read(&rom_timer->reg_sel,sizeof(Uint8),1,stream);
    gngb_file_read(&rom_timer->latch,sizeof(Uint8),1,stream);
  }
  return 0;
}

int save_load_pad_movie(GNGB_FILE * stream,char op) {
  if (!op) {			/* Write */
    PAD_SAVE *p=gngb_movie.first_pad;
    gngb_file_write(&key_cycle,sizeof(Uint32),1,gngb_movie.stream);
    gngb_file_write(&gngb_movie.len,sizeof(Uint32),1,gngb_movie.stream);
    while(p) {
      gngb_file_write(&p->pad,sizeof(Uint8),1,gngb_movie.stream);
      p=p->next;
    }
  } else {			/* Read */
    int i;
    Uint32 len;
    Uint8 pad;
    gngb_movie.len=0;
    gngb_file_read(&key_cycle,sizeof(Uint32),1,gngb_movie.stream);
    gngb_file_read(&len,sizeof(Uint32),1,gngb_movie.stream);
    for(i=0;i<len;i++) {
      gngb_file_read(&pad,sizeof(Uint8),1,gngb_movie.stream);
      movie_add_pad(pad);
    }
  }
  return 0;
}

int get_pad_movie_section_size(void) {
  return 8+gngb_movie.len;
}

int load_section(GNGB_FILE * stream,struct section *s,Uint16 size) {
  //  long end=gngb_file_tell(stream)+size;
  Uint8 t;
  int m;
  
  /* m=how many sub section? */
  for(m=0;s->ss[m].id!=-1;m++);
  //while(gngb_file_tell(stream)<end) {
  while((Sint16)size>0) {
    gngb_file_read(&t,sizeof(Uint8),1,stream);
    if (t<m) s->ss[t].save_load_function(stream,1);
    else break;
    printf("Size %d\n",size);
    size-=(s->ss[t].size+1);
  }
  printf("Size %d\n",size);
  //if (gngb_file_tell(stream)!=end) return -1;
  //  if ((Sint16)size<0) return -1;
  return 0;
}

int save_section(GNGB_FILE * stream,struct section *s) {
  Uint16 size=0;
  int i;

  for(i=0;s->ss[i].id!=-1;i++,size++) {
    if (s->ss[i].size!=-1)    size+=s->ss[i].size;
    else size+=s->ss[i].get_size();
  }

  gngb_file_write(&s->id,sizeof(Sint8),1,stream);
  gngb_file_write(&size,sizeof(Uint16),1,stream);

  for(i=0;s->ss[i].id!=-1;i++/*,size++*/) {
    gngb_file_write(&s->ss[i].id,sizeof(Uint8),1,stream);
    s->ss[i].save_load_function(stream,0);
  }  
  return 0;
}

SDL_Surface* get_surface_of_save_state(int n)
{
  char filename[FILENAME_LEN];
  char t[8];

  if (savestate_bmp!=NULL)
    SDL_FreeSurface(savestate_bmp);

  get_bmp_ext_nb(t,n);
  get_filename_ext(filename,t);

  savestate_bmp=SDL_LoadBMP(filename);
  return savestate_bmp;
}

void save_state_file(GNGB_FILE * stream) {
  int i;
  
  /* We write all the bank and the active page */

  for(i=0;i<nb_ram_page;i++)
    gngb_file_write(ram_page[i],sizeof(Uint8),0x2000,stream);

  for(i=0;i<nb_vram_page;i++)
    gngb_file_write(vram_page[i],sizeof(Uint8),0x2000,stream);

  for(i=0;i<nb_wram_page;i++)
    gngb_file_write(wram_page[i],sizeof(Uint8),0x1000,stream);

  gngb_file_write(oam_space,sizeof(Uint8),0xa0,stream);
  gngb_file_write(himem,sizeof(Uint8),0x160,stream);

  gngb_file_write(&active_rom_page,sizeof(Uint16),1,stream);
  gngb_file_write(&active_ram_page,sizeof(Uint16),1,stream);
  gngb_file_write(&active_vram_page,sizeof(Uint16),1,stream);
  gngb_file_write(&active_wram_page,sizeof(Uint16),1,stream);

  /* now we write a couple of (section id,size) */

  for(i=0;tab_section[i].id!=-1;i++)
    if (tab_section[i].id!=RT_SECTION && tab_section[i].id!=PAD_MOVIE_SECTION) save_section(stream,&tab_section[i]);

  if (rom_type&TIMER) save_section(stream,&tab_section[RT_SECTION]);
}

int save_state(char *name,int n) {
  GNGB_FILE * stream;
  int i;
  char filename[FILENAME_LEN];
  char t[8];


  if (!name) {
    get_ext_nb(t,n);
    get_filename_ext(filename,t);
    if (!(stream=gngb_file_open(filename,"wb",NORMAL_FILE_TYPE))) {
      fprintf(stderr,"Error while trying to write file %s \n",filename);
      return 1;
    }
  } else {
    if (!(stream=gngb_file_open(name,"wb",NORMAL_FILE_TYPE))) {
      fprintf(stderr,"Error while trying to write file %s \n",name);
      return 1;
    }
  }

  save_state_file(stream);    
  gngb_file_close(stream);
  
  filename[0]=0;
  get_bmp_ext_nb(t,n);
  get_filename_ext(filename,t);
  SDL_SaveBMP(get_mini_screenshot(),filename);

  return 0;
}

int load_state_file(GNGB_FILE * stream) {
  int i;
  Uint8 section_id;
  Uint16 size;
  long end,last,begin;

  //begin=gngb_file_tell(stream);
  //end=gngb_file_size_until_end(stream);
  /*gngb_file_seek(stream,0,SEEK_END);
  end=gngb_file_tell(stream);
  gngb_file_seek(stream,0,SEEK_SET);*/

  // If the load crash we must restore current state
  //  save_state(-1);

  // we read all the bank and the active page 
  
  for(i=0;i<nb_ram_page;i++)
    gngb_file_read(ram_page[i],sizeof(Uint8),0x2000,stream);
  
  for(i=0;i<nb_vram_page;i++)
    gngb_file_read(vram_page[i],sizeof(Uint8),0x2000,stream);
  
  for(i=0;i<nb_wram_page;i++)
    gngb_file_read(wram_page[i],sizeof(Uint8),0x1000,stream);

  gngb_file_read(oam_space,sizeof(Uint8),0xa0,stream);
  gngb_file_read(himem,sizeof(Uint8),0x160,stream);

  gngb_file_read(&active_rom_page,sizeof(Uint16),1,stream);
  gngb_file_read(&active_ram_page,sizeof(Uint16),1,stream);
  gngb_file_read(&active_vram_page,sizeof(Uint16),1,stream);
  gngb_file_read(&active_wram_page,sizeof(Uint16),1,stream);

  // now we read a couple of (section id,size) 
  //while(gngb_file_tell(stream)<end) {
    while(!gngb_file_eof(stream)) {
    //last=gngb_file_tell(stream);
    //printf("Last %d\n",last);
    gngb_file_read(&section_id,sizeof(Uint8),1,stream);
    gngb_file_read(&size,sizeof(Uint16),1,stream);
    printf("Read Section %d size %d\n",section_id,size);
    if (((int)section_id>=MAX_SECTION) || (load_section(stream,&tab_section[section_id],size)<0)) {
      printf("Unknow Section %d\n",section_id);
      //      gngb_file_seek(stream,last,SEEK_SET);
      return 1;
    }
  }
  printf("Fin\n");
  key_cycle=0;
  return 0;
}

int load_state(char *name,int n) {
  GNGB_FILE * stream;
  int i;
  char filename[FILENAME_LEN];
  char t[5];
  Uint8 section_id;
  Uint16 size;
  long end,last,begin;

  if (!name) {
    get_ext_nb(t,n);
    get_filename_ext(filename,t);
    if (!(stream=gngb_file_open(filename,"rb",UNKNOW_FILE_TYPE))) {
      fprintf(stderr,"Error while trying to read file %s \n",filename);
      return -1;
    }
  } else {
    if (!(stream=gngb_file_open(name,"rb",UNKNOW_FILE_TYPE))) {
      fprintf(stderr,"Error while trying to read file %s \n",name);
      return -1;
    }
  }
    
  if (load_state_file(stream)) {
    gngb_file_close(stream);
    //  load_state(-1);
    fprintf(stderr,"Error while reading file %s \n",filename);
    return -1;
  }

  gngb_file_close(stream);
  if (conf.sound) 
    update_sound_reg();
  return 0;

}

/* Movie Loas/Save */

/* Movie */

GNGB_MOVIE gngb_movie={NULL,0,NULL};

void begin_save_movie(void) {
  conf.save_movie=1;
  get_filename_ext(gngb_movie.name,".mv");
  gngb_movie.stream=gngb_file_open(gngb_movie.name,"wb",NORMAL_FILE_TYPE);
  gngb_movie.len=0;
  gngb_movie.first_pad=gngb_movie.last_pad=NULL;
  save_state_file(gngb_movie.stream);  
}

void end_save_movie(void) {
  PAD_SAVE *p=gngb_movie.first_pad;
  conf.save_movie=0;
  
  save_section(gngb_movie.stream,&tab_section[PAD_MOVIE_SECTION]);
  gngb_file_close(gngb_movie.stream);
}

void play_movie(void) {
  int i,len;
  Uint8 pad;
  conf.play_movie=1;
  get_filename_ext(gngb_movie.name,".mv");
  gngb_movie.stream=gngb_file_open(gngb_movie.name,"rb",UNKNOW_FILE_TYPE);
  gngb_movie.first_pad=gngb_movie.last_pad=NULL;
  gngb_movie.len=0;
  load_state_file(gngb_movie.stream);
  gngb_movie.last_pad=gngb_movie.first_pad;
}

Uint8 movie_get_next_pad(void) {
  Uint8 pad;
  
  pad=gngb_movie.last_pad->pad;
  gngb_movie.last_pad=gngb_movie.last_pad->next;
  
  if (!gngb_movie.last_pad) conf.play_movie=0;
  return pad;
}

void movie_add_pad(Uint8 pad) {
  PAD_SAVE *p=(PAD_SAVE *)malloc(sizeof(PAD_SAVE));
 
  gngb_movie.len++;
  p->pad=pad;
  p->next=NULL;
  if (!gngb_movie.first_pad) {
    gngb_movie.first_pad=gngb_movie.last_pad=p;   
  } else {
    gngb_movie.last_pad->next=p;
    gngb_movie.last_pad=p;
  }
}


/* Config Load/Save */

static Uint8 read_int8(void *data);
static Uint8 write_int8(void *date);
static Uint8 read_int16(void *data);
static Uint8 write_int16(void *data);
static Uint8 read_int32(void *data);
static Uint8 write_int32(void *data);



