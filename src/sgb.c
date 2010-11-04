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

#include "memory.h"
#include "sgb.h"
#include "rom.h"
#include "vram.h"
#include "emu.h"
#include <SDL/SDL.h>

#ifdef DEBUG
#include "gngb_debuger/log.h"
#endif

#define SGB_CMD_END() {sgb.cmd=0xff;sgb.nb_pack=-1;}
#define SGB_COLOR(c) ((((c)&0x7C00)>>10)|(((c)&0x3E0)<<1)|(((c)&0x1F)<<11))

struct mask_shift {
  unsigned char mask;
  unsigned char shift;
};

static struct mask_shift tab_ms[8]={
  { 0x80,7 },
  { 0x40,6 },
  { 0x20,5 },
  { 0x10,4 },
  { 0x08,3 },
  { 0x04,2 },
  { 0x02,1 },
  { 0x01,0 }};

UINT8 sgb_tiles[256*32];
UINT8 sgb_map[32*32];
UINT8 sgb_att[32*32];

SDL_Surface *sgb_buf=NULL;

extern UINT16 Filter[32768];

UINT16 sgb_border_pal[64];
UINT16 sgb_scpal[512][4];   // 512 pallete of 4 color
UINT8 sgb_ATF[45][90];

static UINT8 sgb_flag=0;

#ifdef SDL_GL
void sgb_init_gl(void) {
  sgb_tex.bmp=(UINT8 *)malloc(256*256*2);
  glGenTextures(1,&sgb_tex.id);
  glBindTexture(GL_TEXTURE_2D,sgb_tex.id);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB5,256,256,0,
	       GL_RGB,GL_UNSIGNED_BYTE,NULL);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);  
}
#endif

void sgb_init(void) {
  int i;
  sgb_buf=SDL_CreateRGBSurface(SDL_HWSURFACE,SGB_WIDTH,SGB_HEIGHT,16,
			       0xf800,0x7e0,0x1f,0x00);
  if (sgb_buf==NULL) {
    fprintf(stderr,"Couldn't set %dx%dx%d video mode: %s\n",
	    SGB_WIDTH,SGB_HEIGHT,16,SDL_GetError());
    exit(1);
  }

  memset(sgb_pal_map,0,20*18);

  for(i=0;i<4;i++) {
    sgb_pal[i][0]=grey[0];
    sgb_pal[i][1]=grey[1];
    sgb_pal[i][2]=grey[2];
    sgb_pal[i][3]=grey[3];
  }

  sgb_mask=0;
  sgb.player=0;

  SGB_CMD_END();

#ifdef SDL_GL
  if (conf.gl) 
    sgb_init_gl();
#endif
}

/* SGB Border */

inline void sgb_tiles_pat_transfer(void) {
  //char type=sgb.pack[1]>>2;
  UINT8 range=sgb.pack[1]&0x03;
  UINT8 *src,*dst;

  //  printf("Pack %02x\n",sgb.pack[1]);
  src=&vram_page[0][0x800];
  dst=(!range)?(&sgb_tiles[0]):(&sgb_tiles[0x80*32]);
  memcpy(dst,src,0x80*32);
  
  SGB_CMD_END();
}

inline void sgb_tiles_map_transfer(void) {
  int i;

  for(i=0;i<32*32;i++) {
    /* FIXME: dkl2 et conker => 0x1000 */
    if (sgb_flag) {
      sgb_map[i]=vram_page[0][0x1000+i*2];
      sgb_att[i]=vram_page[0][0x1000+i*2+1];
    } else {
      sgb_map[i]=vram_page[0][0x800+i*2];
      sgb_att[i]=vram_page[0][0x800+i*2+1];
    }
  }
  
  /* FIXME: dkl2 et conker => 0x800 */
  if (sgb_flag) 
    for(i=0;i<64;i++) 
      sgb_border_pal[i]=SGB_COLOR((vram_page[0][0x800+i*2+1]<<8)|vram_page[0][0x800+i*2]);
  else 
    for(i=0;i<64;i++) 
      sgb_border_pal[i]=SGB_COLOR((vram_page[0][0x1000+i*2+1]<<8)|vram_page[0][0x1000+i*2]);
  
  SGB_CMD_END();
}

inline void sgb_draw_one_tile(UINT16 *buf,int x,int y,UINT16 no_tile,UINT8 att) {
  int sx,sy;
  char bit0,bit1,bit2,bit3;
  int c;
  char xflip=att&0x40;
  UINT16 *b=&buf[x+y*SGB_WIDTH];

  /* FIXME: comme ca dkl2 et conker ca marche mais dragon quest nan :(*/
  if (sgb_flag) {
    if (no_tile>=128) no_tile=((64+no_tile)%128)+128;
    else  no_tile=(64+no_tile)%128;
  }
  
  no_tile=no_tile|((att&0x03)<<6);

  if (att&0x80) {    // yflip
    for(sy=7;sy>=0;sy--,b+=SGB_WIDTH) {
      for(sx=0;sx<8;sx++) {
	int wbit;
	if (!xflip) wbit=sx;
	else wbit=7-sx;
	bit0=(sgb_tiles[no_tile*32+sy*2]&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	bit1=(sgb_tiles[no_tile*32+1+sy*2]&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	bit2=(sgb_tiles[no_tile*32+16+sy*2]&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	bit3=(sgb_tiles[no_tile*32+16+1+sy*2]&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	c=(bit3<<3)|(bit2<<2)|(bit1<<1)|bit0;
	b[sx]=sgb_border_pal[c|((att&0x0c)<<2)];
      }
    }
  } else {
    for(sy=0;sy<8;sy++,b+=SGB_WIDTH) {
      for(sx=0;sx<8;sx++) {
	int wbit;
	if (!xflip) wbit=sx;
	else wbit=7-sx;
	bit0=(sgb_tiles[no_tile*32+sy*2]&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	bit1=(sgb_tiles[no_tile*32+1+sy*2]&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	bit2=(sgb_tiles[no_tile*32+16+sy*2]&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	bit3=(sgb_tiles[no_tile*32+16+1+sy*2]&tab_ms[wbit].mask)>>tab_ms[wbit].shift;
	c=(bit3<<3)|(bit2<<2)|(bit1<<1)|bit0;
	b[sx]=sgb_border_pal[c|((att&0x0c)<<2)];
      }
    }
  }
}

inline void sgb_draw_tiles(void) {
  int i,j;
  UINT16 *buf;

#ifdef SDL_GL
  if (conf.gl)
    buf=(UINT16 *)sgb_tex.bmp;
  else
#endif
    buf=(UINT16 *)sgb_buf->pixels;

  for(j=0;j<28;j++) 
    for(i=0;i<32;i++) 
      sgb_draw_one_tile(buf,i*8,j*8,sgb_map[i+j*32],sgb_att[i+j*32]);
    
#ifdef SDL_GL
  if (conf.gl) {
    glBindTexture(GL_TEXTURE_2D,sgb_tex.id);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,SGB_WIDTH,SGB_HEIGHT,
		    GL_RGB,GL_UNSIGNED_SHORT_5_6_5,sgb_tex.bmp);
  } else
#endif    
    SDL_BlitSurface(sgb_buf,NULL,gb_screen,NULL);
}

/* Area Designation Mode Functions */

inline void set_pal_Hline(int line,int pal) {
  int i;
  for(i=0;i<20;i++)
    sgb_pal_map[i][line]=pal;
}

inline void set_pal_Vline(int line,int pal) {
  int i;
  for(i=0;i<18;i++)
    sgb_pal_map[line][i]=pal; 
}

inline void set_pal_Hline_range(int l1,int l2,int pal) {
  int i,j;
  for(j=l1;j<=l2;j++)
    for(i=0;i<20;i++)
      sgb_pal_map[i][j]=pal;
}

inline void set_pal_Vline_range(int l1,int l2,int pal) {
  int i,j;
  for(i=l1;i<=l2;i++)
    for(j=0;j<18;j++)    
      sgb_pal_map[i][j]=pal;
}

inline void set_pal_inside_block(int x1,int y1,int x2,int y2,int pal) {
  int x,y;
  if (x1<0) x1=0;
  if (x2>=20) x2=20;
  if (y1<0) y1=0;
  if (y2>=18) y2=18;
  for(x=x1+1;x<x2;x++)
    for(y=y1+1;y<y2;y++)
      sgb_pal_map[x][y]=pal;
}

inline void set_pal_outside_block(int x1,int y1,int x2,int y2,int pal) {
  if (y1>0)
    set_pal_Hline_range(0,y1-1,pal);
  if (y2<17)
    set_pal_Hline_range(y2+1,17,pal);
  if (x1>0)
    set_pal_Vline_range(0,x1-1,pal);
  if (x2<19)
    set_pal_Vline_range(x2+1,19,pal);
}

inline void set_pal_on_block(int x1,int y1,int x2,int y2,int pal) {
  int x,y;
  if (x1<0) x1=0;
  if (x2>=20) x2=19;
  for(x=x1;x<=x2;x++) {
    if (y1>=0)
      sgb_pal_map[x][y1]=pal;
    if (y2<18)
      sgb_pal_map[x][y2]=pal;
  }

  if (y1<0) y1=0;
  if (y2>=18) y2=17;
  for(y=y1;y<=y2;y++) {
    if (x1>=0)
      sgb_pal_map[x1][y]=pal;
    if (x2<20)
      sgb_pal_map[x2][y]=pal;
  }
}

inline void sgb_block_mode(void) {
  static INT8 nb_dataset;
  static UINT8 dataset[6];
  static UINT8 ds_i; // dataset indice  
  UINT8 p_i;   // packet indice

  if (sgb.nb_pack==-1) {  // first call
    //printf("Block mode\n");
    sgb.nb_pack=sgb.pack[0]&0x07;
    nb_dataset=sgb.pack[1]&0x1f;
    //printf("nb dataset %02x\n",nb_dataset);
    p_i=2;
    ds_i=0;
  } else p_i=0;

  while(nb_dataset>0 && p_i<SGB_PACKSIZE) {
    dataset[ds_i++]=sgb.pack[p_i++];
    if (ds_i==6) { // on traite le dataset
      INT8 SH,SV,EH,EV;
      UINT8 px,py,pz;
      SH=dataset[2]&0x1f;
      SV=dataset[3]&0x1f;
      EH=dataset[4]&0x1f;
      EV=dataset[5]&0x1f;
      px=(dataset[1]>>4)&0x03;
      py=(dataset[1]>>2)&0x03;
      pz=(dataset[1]&0x03);
    
      nb_dataset--;
      
      //printf("type %d p %d %d %d line  %d %d %d %d\n",dataset[0]&0x07,px,py,pz,SH,SV,EH,EV);
      ds_i=0;
      switch(dataset[0]&0x07) {
      case 0x00:break;
      case 0x01:
	set_pal_inside_block(SH,SV,EH,EV,pz);
	set_pal_on_block(SH,SV,EH,EV,pz);
	break;
      case 0x02:
	set_pal_on_block(SH,SV,EH,EV,py);
	break;
      case 0x03:
	set_pal_inside_block(SH,SV,EH,EV,py);
	set_pal_on_block(SH,SV,EH,EV,py);
	break;
      case 0x04:
	set_pal_outside_block(SH,SV,EH,EV,px);
	break;	
      case 0x05:
	set_pal_inside_block(SH,SV,EH,EV,pz);
	set_pal_outside_block(SH,SV,EH,EV,px);
	break;
      case 0x06:
	set_pal_outside_block(SH,SV,EH,EV,px);
	break;
      case 0x07:
	set_pal_inside_block(SH,SV,EH,EV,pz);
	set_pal_on_block(SH,SV,EH,EV,py);
	set_pal_outside_block(SH,SV,EH,EV,px);	  
	break;
      }
    }
  }
  
  sgb.nb_pack--;
  if (sgb.nb_pack==0) SGB_CMD_END();
}

inline void sgb_line_mode(void) {
  int i;
  static INT16 nb_dataset;

  if (sgb.nb_pack==-1) {  // first call
    sgb.nb_pack=sgb.pack[0]&0x07;
    //printf("%d\n",sgb.nb_pack);
    //printf("Line mode\n");
    nb_dataset=sgb.pack[1];
    //printf("nb dataset %d\n",nb_dataset);
    i=2;
  } else i=0;

  while(nb_dataset>0 && i<SGB_PACKSIZE) {
    nb_dataset--;
    /*printf("mode %d\n",(sgb.pack[i]&0x40));
    printf("line %d\n",sgb.pack[i]&0x1f);
    printf("pal %d\n",(sgb.pack[i]&0x60)>>5);*/
    if ((sgb.pack[i]&0x40))  // mode vertical
      set_pal_Vline(sgb.pack[i]&0x1f,(sgb.pack[i]>>5)&0x03);
    //set_pal_Vline_range(0,sgb.pack[i]&0x1f,(sgb.pack[i]>>5)&0x03);
    else  // mode horizontal
      set_pal_Hline(sgb.pack[i]&0x1f,(sgb.pack[i]>>5)&0x03);
    //set_pal_Hline_range(0,sgb.pack[i]&0x1f,(sgb.pack[i]>>5)&0x03);

    i++;
  }

  sgb.nb_pack--;
  if (sgb.nb_pack==0) SGB_CMD_END();
}

inline void sgb_divide_mode(void) {
  int line=sgb.pack[2]&0x1f;
  /*printf("Divide mode\n");
    printf(((sgb.pack[1]&0x40)?"Vertical\n":"Horizontal\n"));
    printf("Line %d\n",sgb.pack[2]);
    printf("Color %d %d %d \n",(sgb.pack[1]&0x30)>>4,(sgb.pack[1]&0x0c)>>2,sgb.pack[1]&0x03);*/
  
  if (!(sgb.pack[1]&0x40)) { 
    set_pal_Vline(line,(sgb.pack[1]&0x30)>>4);
    set_pal_Vline_range(0,line-1,(sgb.pack[1]>>2)&0x03);
    set_pal_Vline_range(line+1,19,(sgb.pack[1]/*>>2*/)&0x03);
  } else { 
    set_pal_Hline(line,(sgb.pack[1]&0x30)>>4);
    set_pal_Hline_range(0,line-1,(sgb.pack[1]>>2)&0x03);
    set_pal_Hline_range(line+1,17,(sgb.pack[1]/*>>2*/)&0x03);
  }
  SGB_CMD_END();
}


inline void sgb_1chr_mode(void) {
  static UINT8 mode;
  static UINT16 nb_dataset;
  static UINT8 I,J;
  static int i;

  if (sgb.nb_pack==-1) {  // first call
    sgb.nb_pack=sgb.pack[0]&0x07;
    /*printf("1chr mode\n");
      printf("Nb packet %d\n",sgb.pack[0]&0x07);
      printf("X %d Y %d\n",sgb.pack[1]&0x1f,sgb.pack[2]&0x1f);
      printf("Nb dataset %d\n",sgb.pack[3]);
      printf("MSB %d\n",sgb.pack[4]);
      printf("Writing style %d\n",sgb.pack[5]);
      printf("Data ....\n");*/
    
    I=(sgb.pack[1]&0x1f); J=(sgb.pack[2]&0x1f);
    nb_dataset=((sgb.pack[4]&0x01)<<8)|sgb.pack[3];
    if (nb_dataset>360) nb_dataset=360;
    mode=sgb.pack[5]&0x01;
    i=6;
  } else i=0;
  
  while(i<SGB_PACKSIZE) {
    int t,p;
    for(t=3;t>=0;t--) {
      //p=(sgb.pack[i]&(0x03<<(t*2)))>>(t*2);
      p=(sgb.pack[i]>>(t*2))&0x03;
      sgb_pal_map[I][J]=p;
      
      if (!mode) {
	I=(I+1)%20;
	if (!I) J++;
      } else {
	J=(J+1)%18;
	if (!J) I++;
      }
    }
    i++;
  }
  
  sgb.nb_pack--;
  //printf("packet num %d\n",sgb.nb_pack);
  if (sgb.nb_pack==0) SGB_CMD_END();
}

void set_ATF_nf(int nf,int mode) {
  int i,j,n;
  UINT8 *t=sgb_ATF[nf];
  /*printf("Set data from ATF \n");
    printf("Num file %d\n",nf);
    printf("Mode %d\n",mode);*/
  
  for(j=0;j<18;j++) {
    i=0;
    for(n=0;n<5;n++,t++) {
      sgb_pal_map[i++][j]=(*t)>>6;
      sgb_pal_map[i++][j]=((*t)&0x30)>>4;
      sgb_pal_map[i++][j]=((*t)&0x0c)>>2;
      sgb_pal_map[i++][j]=((*t)&0x03);
    }
  }
  if (mode) sgb_mask=0;
}

void sgb_set_ATF(void) {
  int i;
  UINT8 *t=&vram_page[0][0x800];
  
  //printf("Set ATF \n");
  for(i=0;i<45;i++,t+=90)
    memcpy(sgb_ATF[i],t,90);  
  set_ATF_nf(0,0);
  
  SGB_CMD_END();
}

void sgb_set_data_ATF(void) {
  set_ATF_nf(sgb.pack[1]&0x3f,sgb.pack[1]&0x40);
  SGB_CMD_END();
}

/* Pallete Functions */

inline void sgb_set_scpal_data(void) {
  int i,j,n;
  UINT8 *t=&vram_page[0][0x800];
  
  for(i=0;i<512;i++) 
    for(j=0;j<4;j++,t+=2)
      sgb_scpal[i][j]=SGB_COLOR((*(t+1)<<8)|(*t));

  /* FIXME */
  /*n=0;
    for(i=16;i<32;i++)
    for(j=0;j<4;j++)
    sgb_border_pal[n++]=sgb_scpal[i][j];
    sgb_draw_tiles();*/

  SGB_CMD_END();
}

inline void sgb_set_pal_indirect(void) {
  
  memcpy(sgb_pal[0],sgb_scpal[((sgb.pack[2]&0x01)<<8)|sgb.pack[1]],2*4);
  memcpy(sgb_pal[1],sgb_scpal[((sgb.pack[4]&0x01)<<8)|sgb.pack[3]],2*4);
  memcpy(sgb_pal[2],sgb_scpal[((sgb.pack[6]&0x01)<<8)|sgb.pack[5]],2*4);
  memcpy(sgb_pal[3],sgb_scpal[((sgb.pack[8]&0x01)<<8)|sgb.pack[7]],2*4);

  if (sgb.pack[9])
    set_ATF_nf(sgb.pack[9]&0x3f,1);

  SGB_CMD_END();
}


inline void sgb_setpal(int p1,int p2) {
  int i;
  for(i=0;i<4;i++) 
    sgb_pal[i][0]=SGB_COLOR((sgb.pack[2]<<8)|sgb.pack[1]);
  
  sgb_pal[p1][1]=SGB_COLOR((sgb.pack[4]<<8)|sgb.pack[3]);
  sgb_pal[p1][2]=SGB_COLOR((sgb.pack[6]<<8)|sgb.pack[5]);
  sgb_pal[p1][3]=SGB_COLOR((sgb.pack[8]<<8)|sgb.pack[7]);
  
  sgb_pal[p2][1]=SGB_COLOR((sgb.pack[10]<<8)|sgb.pack[9]);
  sgb_pal[p2][2]=SGB_COLOR((sgb.pack[12]<<8)|sgb.pack[11]);
  sgb_pal[p2][3]=SGB_COLOR((sgb.pack[14]<<8)|sgb.pack[13]);
  
  SGB_CMD_END();
}

/* Misc Function */

inline void sgb_function(void) {
  switch(sgb.pack[1]) {
  case 0x01:/*sgb_flag=1;*/break; // Please if you know what do this function (email me)
  }
  SGB_CMD_END();
}

inline void sgb_window_mask(void) {

  /* je ne sais pas si c'est ca */
  /* apres quelque test: apparement nan :( */
  switch(sgb.pack[1]&0x03) {
  case 0x00:sgb_mask=0;break;
  case 0x01:  // i dint know what it do
  case 0x02:
    /*for(i=0;i<4;i++)
      memset(sgb_pal[i],0,sizeof(UINT16)*4);
      break;*/
  case 0x03:
    /*for(i=0;i<4;i++)
      memset(sgb_pal[i],0xffff,sizeof(UINT16)*4);
      break;*/
    sgb_mask=1;
    break;   
  }
  SGB_CMD_END();
}

void sgb_exec_cmd(void) {
  
  if (sgb.cmd==0xff) {
    int i;
    /*printf("sgb:%02x nb:%d pack: ",sgb.pack[0]>>3,sgb.pack[0]&0x07);
    for(i=0;i<SGB_PACKSIZE;i++)
      printf("%02x ",sgb.pack[i]);
      putchar('\n');*/
#ifdef DEBUG  
    if (active_log)
      put_log("sgb:%02x nb:%d pack:\n",sgb.pack[0]>>3,sgb.pack[0]&0x07);
#endif
    sgb.cmd=sgb.pack[0]>>3;
  }   

  switch(sgb.cmd) {
  case 0x11:
    //GB_PAD=(gameboy_type&SUPER_GAMEBOY)?0x30:0xff;
    //printf("check sgb %02x\n",GB_PAD);
    if (sgb.pack[1]==0x03) 
      GB_PAD=(conf.gb_type&SUPER_GAMEBOY)?0x30:0xff;
    else {
      sgb.player=sgb.pack[1]&0x01;
      //sgb.player|=0x80;
    }
    SGB_CMD_END();
    break;
    //GB_PAD=0xff;break;
    
    // SGB Function
    
  case 0x0e:sgb_function();break;
    
    // SGB Border

  case 0x13:sgb_tiles_pat_transfer();break;
  case 0x14:
    sgb_tiles_map_transfer();
    sgb_draw_tiles();
    break;

    // Area using pallete
    
  case 0x04:sgb_block_mode();break;
  case 0x05:sgb_line_mode();break;
  case 0x06:sgb_divide_mode();break;
  case 0x07:sgb_1chr_mode();break;
    
    // ATF

  case 0x15:sgb_set_ATF();break;
  case 0x16:sgb_set_data_ATF();break;
    
    // Palette direct

  case 0x00:sgb_setpal(0,1);break;
  case 0x01:sgb_setpal(2,3);break;
  case 0x02:sgb_setpal(0,3);break;
  case 0x03:sgb_setpal(1,2);break;

    // Palette indirect

  case 0x0a:sgb_set_pal_indirect();break;
  case 0x0b:sgb_set_scpal_data();break;

    // Misc

  case 0x17:sgb_window_mask();break;

  case 0x18:			// I dont know what do this function
  case 0x19:          
    SGB_CMD_END();
    break;

  default:
    SGB_CMD_END();break;
  }

}
