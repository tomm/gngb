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

#include <config.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <SDL.h>

#include "emu.h"
#include "menu.h"
#include "message.h"
#include "vram.h"
#include "video_std.h"
#include "rom.h"
#include "sound.h"
#include "message.h"
#include "menu_image.h"
#include "save.h"

#define MAX_ITEM 6
#define MENUX 7
#define MENUY 7

#define SHADOW_IN 0
#define SHADOW_OUT 1
#define SHADOW_ETCHED_IN 2
#define SHADOW_ETCHED_OUT 3
#define SHADOW_NONE 4

int stop_all=0;

Uint8 radio_group[256];
SDL_Color buttonpal[]={{255,255,255},{214,214,214},{150,150,150},{0,0,0},{195,195,195}};

SDL_Surface *radio_on,*radio_off,*toggle_on,*toggle_off,*arrow_up,*arrow_down;

/*
  note on action function:
  if it return 1, the current menu is closed
  if it's 2, all menu are closed
  otherwise, it stays open
*/

int action_save_state(MENU_ITEM *self)
{
  int i=(int)self->user_data;
  save_state(NULL,i);
  return 2;
}

int action_load_state(MENU_ITEM *self)
{
  int i=(int)self->user_data;
  load_state(NULL,i);
  return 2;
}

void menu_draw_state(MENU_ITEM *self,int menu_pos)
{
  int i=(int)self->user_data;
  SDL_Surface *bmp;
  SDL_Rect b;
  
  b.x=94;
  b.y=78;

  b.w=55;
  b.h=50;
  bmp=get_surface_of_save_state(i);
  if(bmp) {
    SDL_FillRect(back,&b,0);
    b.x++;b.y++;
    SDL_BlitSurface(bmp,NULL,back,&b);
  } 
}

int action_loop_menu(MENU_ITEM *self)
{
  MENU *m=(MENU*)self->user_data;
  loop_menu(m);
  return 0;
}

int toggle_fullscreen(MENU_ITEM *self)
{
  switch_fullscreen();
  return 2;
}

int toggle_filter(MENU_ITEM *self)
{
  if (conf.gb_type&COLOR_GAMEBOY) {
    conf.color_filter^=1;
    GenFilter();
    update_all_pal();
  }
  return 2;
}

/* draw a preview of a pal */
void menu_draw_pal(MENU_ITEM *self,int menu_pos)
{
  SDL_Rect b;
  int i;
  int pal=(int)self->user_data;

  b.x=100;
  b.y=MENUY+menu_pos*hl+3;
  b.w=22;
  b.h=7;
  SDL_FillRect(back,&b,COL32_TO_16(0x000000));
  
  b.w=5;
  b.h=5;
  b.y+=1;
  for(i=0;i<4;i++) {
    b.x=101+i*5;
    
    SDL_FillRect(back,&b,COL32_TO_16(conf.pal[pal][3-i]));
  }

}

int action_setpal(MENU_ITEM *self)
{
  int p=(int)self->user_data;
  gb_set_pal(p);
  return 2;
}

int action_reset(MENU_ITEM *self)
{
  emu_reset(); 
  return 2;
}

int toggle_autofskip(MENU_ITEM *self)
{
  conf.autoframeskip^=1;
  return 2;
}

int toggle_sound(MENU_ITEM *self)
{
  conf.sound^=1;
  if (conf.sound)
    gbsound_init();
  else
    close_sound();

  return 2;
}

int toggle_sleepidle(MENU_ITEM *self)
{
  conf.sleep_idle^=1;
  return 2;
}

int toggle_fps(MENU_ITEM *self)
{
  conf.show_fps^=1;
  if (!conf.show_fps)
    unset_info();
  else
    set_info("fps:..");
  return 2;
}

int action_set_filter(MENU_ITEM *self)
{
  int p=(int)self->user_data;
  set_filter(p);
  return 2;
}

MENU save_state_menu={
  " Save state          ",
  NULL,
  0,0,0,0
};

MENU load_state_menu={
  " Load state          ",
  NULL,
  0,0,0,0
};

MENU filter_menu={
  " Filter               ",
  NULL,
  0,0,0,0
};

MENU video_menu={
  " Video               ",
  NULL,
  0,0,0,0
};

MENU fskip_menu={
  " Frame control       ",
  NULL,
  0,0,0,0
};

MENU main_menu={
  " Main                ",
  NULL,
  0,0,0,0
};

MENU *menu_list[]={
  &main_menu,
  &load_state_menu,
  &save_state_menu,
  &video_menu,
  &fskip_menu,
  NULL
};

MENU_ITEM *new_menu_item(char *name,Uint8 type)
{
  MENU_ITEM *t;
  t=(MENU_ITEM*)malloc(sizeof(MENU_ITEM));
  memset(t,0,sizeof(MENU_ITEM));
  t->name=strdup(name);
  t->type=type;
  t->next=NULL;
  return t;
}

void menu_push_back_item(MENU *m,MENU_ITEM *i)
{
  MENU_ITEM *t=m->item;
  m->size++;
  m->end=(m->size-1<MAX_ITEM?m->size-1:MAX_ITEM);
  if (t==NULL) {m->item=i;return;}
  
  while(t->next!=NULL)
    t=t->next;
  t->next=i;
}

void draw_v_line(SDL_Surface *s,int x,int y,int l,Uint32 color)
{
  SDL_Rect r;
  r.x=x;
  r.y=y;
  r.w=1;
  r.h=l;
  SDL_FillRect(s,&r,color);
}

void draw_h_line(SDL_Surface *s,int x,int y,int l,Uint32 color)
{
  SDL_Rect r;
  r.x=x;
  r.y=y;
  r.w=l;
  r.h=1;
  SDL_FillRect(s,&r,color);
}


void draw_border(SDL_Surface *s,int type,SDL_Rect *r)
{
  SDL_FillRect(s,r,COL32_TO_16(0xd6d6d6));
  switch(type) {
  case SHADOW_OUT:
    draw_h_line(s,r->x,r->y,r->w,COL32_TO_16(0xFFFFFF));
    draw_h_line(s,r->x+1,r->y+1,r->w-1,COL32_TO_16(0xd6d6d6));
    draw_h_line(s,r->x,r->y+r->h,r->w+1,COL32_TO_16(0x000000));
    draw_h_line(s,r->x+2,r->y+r->h-1,r->w-2,COL32_TO_16(0x969696));
    
    draw_v_line(s,r->x,r->y,r->h,COL32_TO_16(0xFFFFFF));
    draw_v_line(s,r->x+1,r->y+1,r->h-1,COL32_TO_16(0xd6d6d6));
    draw_v_line(s,r->x+r->w,r->y,r->h,COL32_TO_16(0x000000));
    draw_v_line(s,r->x+r->w-1,r->y+1,r->h-1,COL32_TO_16(0x969696));
    break;
  case SHADOW_ETCHED_IN:
    draw_h_line(s,r->x,r->y,r->w,COL32_TO_16(0x969696));
    draw_h_line(s,r->x+1,r->y+1,r->w-1,COL32_TO_16(0xFFFFFF));
    draw_h_line(s,r->x,r->y+r->h-1,r->w-1,COL32_TO_16(0x969696));
    draw_h_line(s,r->x,r->y+r->h,r->w,COL32_TO_16(0xFFFFFF));
    
    draw_v_line(s,r->x,r->y,r->h-1,COL32_TO_16(0x969696));
    draw_v_line(s,r->x+1,r->y+1,r->h-2,COL32_TO_16(0xFFFFFF));
    draw_v_line(s,r->x+r->w-1,r->y+2,r->h-2,COL32_TO_16(0x969696));
    draw_v_line(s,r->x+r->w,r->y+1,r->h,COL32_TO_16(0xFFFFFF));
    break;
  default:
    break;
  }
}

void display_menu(MENU *m)
{
  SDL_Rect b;
  int i;
  MENU_ITEM *item=m->item;


  b.x=MENUX-2;
  b.y=MENUY-2;
  b.w=wl*21+3;
  b.h=hl*10+3;
  draw_border(back,SHADOW_OUT,&b);

  b.x=MENUX;
  b.y=MENUY+hl*2-hl/2;
  b.w=wl*21-2;
  b.h=hl*8;
  draw_border(back,SHADOW_ETCHED_IN,&b);
  
 
  if (!item) return;

  for(i=0;i<m->begin;i++)
    item=item->next;

  draw_message(MENUX,MENUY,m->title);
 
  for(i=m->begin;i<=m->end;i++) {
 
    if (i==m->id) {
      b.x=MENUX+3;
      b.y=MENUY+(i-m->begin+2)*hl;
      b.w=wl*21-8;
      b.h=hl;
      SDL_FillRect(back,&b,COL32_TO_16(0xeaeaea));
      if (item->draw_info && item->draw_type==DRAW_WHEN_ACTIVE)
	item->draw_info(item,i-m->begin+2);
    }
    if (item->type==TOGGLE) {
      b.x=135;
      b.y=MENUY+(i-m->begin+2)*hl+2;
      SDL_BlitSurface((item->state?toggle_on:toggle_off),NULL,back,&b);
    }
    if (item->type==RADIO) {
      b.x=135;
      b.y=MENUY+(i-m->begin+2)*hl+2;
      SDL_BlitSurface((item->radio==radio_group[item->group]?radio_on:radio_off),NULL,back,&b);
    }
    draw_message(MENUX+wl,MENUY+(i-m->begin+2)*hl,item->name);
    if (item->draw_info && item->draw_type==DRAW_ALWAYS)
      item->draw_info(item,i-m->begin+2);
    item=item->next;
  }
  if (m->end!=m->size-1) {
    b.x=75;
    b.y=MENUY+(MAX_ITEM+1)*hl+hl/2+24;
    b.w=b.h=9;
    SDL_BlitSurface(arrow_down,NULL,back,&b);
  }
  if (m->begin!=0) {
    b.x=75;
    b.y=22;
    SDL_BlitSurface(arrow_up,NULL,back,&b);
  }


}

void init_menu(void){
  int i=0;
  MENU_ITEM *item;
  char tmpbuf[255];

  /* init all the icon */

  arrow_down=img2surface(img_arrow_down);
  arrow_up=img2surface(img_arrow_up);
  radio_on=img2surface(img_radio_on);
  radio_off=img2surface(img_radio_off);
  toggle_on=img2surface(img_toggle_on);
  toggle_off=img2surface(img_toggle_off);
  
  SDL_SetColors(arrow_down,buttonpal,0,5);
  SDL_SetColors(arrow_up,buttonpal,0,5);
  SDL_SetColors(radio_off,buttonpal,0,5);
  SDL_SetColors(radio_on,buttonpal,0,5);
  SDL_SetColors(toggle_off,buttonpal,0,5);
  SDL_SetColors(toggle_on,buttonpal,0,5);
  SDL_SetColorKey(radio_off,SDL_SRCCOLORKEY|SDL_RLEACCEL,5);
  SDL_SetColorKey(radio_on,SDL_SRCCOLORKEY|SDL_RLEACCEL,5);

  /* Main Menu creation */

  /* load state */
  item=new_menu_item("Load state ...",ACTION);
  item->user_data=(void*)&load_state_menu;
  item->func=action_loop_menu;
  menu_push_back_item(&main_menu,item);
  
  /*save state */
  item=new_menu_item("Save state ...",ACTION);
  item->user_data=(void*)&save_state_menu;
  item->func=action_loop_menu;
  menu_push_back_item(&main_menu,item);

  /*video */
  item=new_menu_item("Video      ...",ACTION);
  item->user_data=(void*)&video_menu;
  item->func=action_loop_menu;
  menu_push_back_item(&main_menu,item);

  /*Framskip&co */
  item=new_menu_item("Framskip   ...",ACTION);
  item->user_data=(void*)&fskip_menu;
  item->func=action_loop_menu;
  menu_push_back_item(&main_menu,item);

  item=new_menu_item("Sound",TOGGLE);
  item->state=conf.sound;
  item->func=toggle_sound;
  menu_push_back_item(&main_menu,item);

  item=new_menu_item("Reset",ACTION);
  item->func=action_reset;
  menu_push_back_item(&main_menu,item);

  /* Load state */
  for (i=0;i<8;i++) {
    snprintf(tmpbuf,254,"Slot %d",i+1);
    item=new_menu_item(tmpbuf,ACTION);
    item->func=action_load_state;
    item->draw_info=menu_draw_state;
    item->draw_type=DRAW_WHEN_ACTIVE;
    item->user_data=(void*)i;
    menu_push_back_item(&load_state_menu,item);
  }
  
  /* Save state */
  for (i=0;i<8;i++) {
    snprintf(tmpbuf,254,"Slot %d",i+1);
    item=new_menu_item(tmpbuf,ACTION);
    item->func=action_save_state;
    item->draw_info=menu_draw_state;
    item->draw_type=DRAW_WHEN_ACTIVE;
    item->user_data=(void*)i;
    menu_push_back_item(&save_state_menu,item);
  }

  /* Filter Menu */
  item=new_menu_item("None",RADIO);
  item->group=1;
  item->radio=0;
  item->user_data=(void*)0;
  item->func=action_set_filter;
  item->draw_type=DRAW_ALWAYS;
  menu_push_back_item(&filter_menu,item);
  for (i=1;i<=5;i++) {
    snprintf(tmpbuf,254,"Filter %d",i);
    item=new_menu_item(tmpbuf,RADIO);
    item->group=1;
    item->radio=i;
    item->user_data=(void*)i;
    item->func=action_set_filter;
    item->draw_type=DRAW_ALWAYS;
    menu_push_back_item(&filter_menu,item);
  }

  /* video menu */
  item=new_menu_item("Fullscreen",TOGGLE);
  item->state=conf.fs;
  item->func=toggle_fullscreen;
  menu_push_back_item(&video_menu,item);  

  if (conf.gb_type&COLOR_GAMEBOY) {
    item=new_menu_item("Color filter",TOGGLE);
    item->state=conf.color_filter;
    item->func=toggle_filter;
    menu_push_back_item(&video_menu,item);
  } else if (conf.gb_type&NORMAL_GAMEBOY) {
    for (i=0;i<5;i++) {
      snprintf(tmpbuf,254,"Palette %d",i+1);
      item=new_menu_item(tmpbuf,RADIO);
      item->group=0;
      item->radio=i;
      item->user_data=(void*)i;
      item->func=action_setpal;
      item->draw_type=DRAW_ALWAYS;
      item->draw_info=menu_draw_pal;
      menu_push_back_item(&video_menu,item);
    }
  }

  item=new_menu_item("Filter          ...",ACTION);
  item->user_data=(void*)&filter_menu;
  item->func=action_loop_menu;
  menu_push_back_item(&video_menu,item);

  /* frameskip menu */
  item=new_menu_item("Auto frameskip",TOGGLE);
  item->state=conf.autoframeskip;
  item->func=toggle_autofskip;
  menu_push_back_item(&fskip_menu,item);

  item=new_menu_item("Sleep idle",TOGGLE);
  item->state=conf.sleep_idle;
  item->func=toggle_sleepidle;
  menu_push_back_item(&fskip_menu,item);
  
  item=new_menu_item("Show FPS",TOGGLE);
  item->state=conf.show_fps;
  item->func=toggle_fps;
  menu_push_back_item(&fskip_menu,item);

}

void loop_menu(MENU *m)
{
  SDL_Event event;
  static int init=1;
  int done=0,i;
  MENU_ITEM *item;
  stop_all=0;
  if (init) {
    init_menu();
    init=0;
  }
  
  current_menu=m;
  SDL_SetColorKey(fontbuf,SDL_SRCCOLORKEY|SDL_RLEACCEL,0);


  while(!done && !stop_all) {
    blit_screen();
    SDL_WaitEvent(&event);
    switch (event.type) {
#if defined(SDL_YUV) || defined(SDL_GL)
    case SDL_VIDEORESIZE:
      if (conf.yuv || conf.gl) {
        conf.res_w=event.resize.w;
        conf.res_h=event.resize.h;
        reinit_vram();
	blit_screen();
      }
      break;
#endif
    case SDL_KEYDOWN:
      switch(event.key.keysym.sym) {
      case SDLK_ESCAPE: 
	done=1;
      break;
      case SDLK_UP:
	if (m->id>0) m->id--;
	if (m->id<m->begin) {
	  m->begin--;
	  m->end--;
	}
	break;
      case SDLK_DOWN:
	if (m->id<m->size-1) m->id++;
	if (m->id>m->end) {
	  m->end++;
	  m->begin++;
	}
	break;
      case SDLK_RETURN:
	item=m->item;
	for(i=0;i<m->id;i++)
	  item=item->next;
	if (item->type==TOGGLE)
	  item->state^=1;
	if (item->type==RADIO)
	  radio_group[item->group]=item->radio;
	if (item->func) {
	  done=item->func(item);
	  if (done==2) stop_all=1;
	  current_menu=m; /* if func call loop_menu, current_menu is trashed */
	}
	break;
      default:
	break;
      }
    }
 
  }
  current_menu=NULL;
}
