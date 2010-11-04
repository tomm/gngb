#ifdef SDL_GL

#include <SDL.h>
#include <stdlib.h>
#include "global.h"
#include "memory.h"
#include "vram.h"
#include "message.h"
#include "tiny_font.h"
#include "emu.h"
#include "interrupt.h"
#include "sgb.h"
#include "video_std.h"
#include "video_gl.h"
#include <GL/gl.h>
#include <GL/glu.h>

static Uint32 gl_flag;
#define FONTBUF_GL_W 128
#define FONTBUF_GL_H 128
GLubyte *gl_buf;
int fontgl_texid;

extern Uint16 sensor_x,sensor_y;

void sgb_init_gl(void) {
  sgb_tex.bmp=(Uint8 *)malloc(256*256*2);
  glGenTextures(1,&sgb_tex.id);
  glBindTexture(GL_TEXTURE_2D,sgb_tex.id);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB5,256,256,0,
	       GL_RGB,GL_UNSIGNED_BYTE,NULL);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);  
}


void reinit_video_gl(void)
{
  gb_screen=SDL_SetVideoMode(conf.res_w,conf.res_h,BIT_PER_PIXEL,gl_flag);
  glViewport(0,0,conf.res_w,conf.res_h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (conf.gb_type&SUPER_GAMEBOY)
    glOrtho(0.0,(GLfloat)SGB_WIDTH,(GLfloat)SGB_HEIGHT,0.0,-1.0,1.0);
  else 
    glOrtho(0.0,(GLfloat)SCREEN_X,(GLfloat)SCREEN_Y,0.0,-1.0,1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  return;
}

void init_video_gl(Uint32 flag)
{
  gl_flag=SDL_HWSURFACE|flag|SDL_RESIZABLE;
  gb_screen=SDL_SetVideoMode(conf.res_w,conf.res_h,BIT_PER_PIXEL,gl_flag);

  if (gb_screen==NULL) {
    printf("Couldn't set %dx%dx%d video mode: %s\n",
	   SCREEN_X,SCREEN_Y,BIT_PER_PIXEL,SDL_GetError());
    exit(1);
  }

  glViewport(0,0,conf.res_w,conf.res_h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (conf.gb_type&SUPER_GAMEBOY)
    glOrtho(0.0,(GLfloat)SGB_WIDTH,(GLfloat)SGB_HEIGHT,0.0,-1.0,1.0);
  else 
    glOrtho(0.0,(GLfloat)SCREEN_X,(GLfloat)SCREEN_Y,0.0,-200.0,200.0);
  
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  //  gl_tex.bmp=(Uint8 *)malloc(256*256*2);
  glEnable(GL_TEXTURE_2D);
  
  glGenTextures(1,&gl_tex.id);
  glBindTexture(GL_TEXTURE_2D,gl_tex.id);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB5,256,256,0,
	       GL_RGB,GL_UNSIGNED_BYTE,NULL);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);

  if (conf.gb_type&SUPER_GAMEBOY) {
    scxoff=48;
    scyoff=40;
    sgb_init_gl();
  } else {
    scxoff=0;
    scyoff=0;
  }
  //init_message_gl();
}

static void update_gldisp(void)
{
  static GLfloat tu=SCREEN_X/256.0,tv=SCREEN_Y/256.0;
 
  glClearColor( 0.0, 0.0, 0.0, 1.0 );
  glClear( GL_COLOR_BUFFER_BIT);
  glEnable(GL_TEXTURE_2D);

  if (rb_on) {
    rb_shift++;
    if (rb_shift>=RB_SIZE) {
      rb_on=0;
      rb_shift=0;
    }
  }
  
  if (conf.gb_type&SUPER_GAMEBOY) {
    glBindTexture(GL_TEXTURE_2D,sgb_tex.id);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0,0.0);
    glVertex2i(0,0);
    glTexCoord2f(1.0,0.0);
    glVertex2i(SGB_WIDTH,0);
    glTexCoord2f(1.0,SGB_HEIGHT/256.0);
    glVertex2i(SGB_WIDTH,SGB_HEIGHT);
    glTexCoord2f(0.0,SGB_HEIGHT/256.0);
    glVertex2i(0,SGB_HEIGHT);
    glEnd();
  }

  glBindTexture(GL_TEXTURE_2D,gl_tex.id);

  if (!sgb_mask)
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,SCREEN_X,SCREEN_Y,
		    GL_RGB,GL_UNSIGNED_SHORT_5_6_5,back->pixels);
  else glTexSubImage2D(GL_TEXTURE_2D,0,0,0,SCREEN_X,SCREEN_Y,
		       GL_RGB,GL_UNSIGNED_SHORT_5_6_5,NULL);

  glBegin(GL_QUADS);

  glTexCoord2f(rb_tab[0][rb_shift]/256.0,rb_tab[1][rb_shift]/256.0);
  glVertex2i(scxoff,scyoff);
  
  glTexCoord2f(tu+rb_tab[0][rb_shift]/256.0,rb_tab[1][rb_shift]/256.0);
  glVertex2i(SCREEN_X+scxoff,scyoff);
  
  glTexCoord2f(tu+rb_tab[0][rb_shift]/256.0,tv+rb_tab[1][rb_shift]/256.0); 
  glVertex2i(SCREEN_X+scxoff,SCREEN_Y+scyoff);
  
  glTexCoord2f(rb_tab[0][rb_shift]/256.0,tv+rb_tab[1][rb_shift]/256.0);
  glVertex2i(scxoff,SCREEN_Y+scyoff);
  
  glEnd();  

  glDisable(GL_TEXTURE_2D);
  SDL_GL_SwapBuffers();

  if ((!(LCDCCONT&0x20) || !(LCDCCONT&0x01)) && (conf.gb_type&NORMAL_GAMEBOY))
    clear_screen();
}

void blit_screen_gl(void) {
  update_gldisp();

  if ((!(LCDCCONT&0x20) || !(LCDCCONT&0x01)) && (conf.gb_type&NORMAL_GAMEBOY)) clear_screen();
}

void blit_sgb_mask_gl(void)
{
  glBindTexture(GL_TEXTURE_2D,sgb_tex.id);
  glTexSubImage2D(GL_TEXTURE_2D,0,0,0,SGB_WIDTH,SGB_HEIGHT,
		  GL_RGB,GL_UNSIGNED_SHORT_5_6_5,sgb_buf->pixels);
  
}

VIDEO_MODE video_gl={
  init_video_gl,
  reinit_video_gl,
  //  draw_screen_col_gl,
  //  draw_screen_wb_gl,
  //draw_screen_sgb_gl,
  blit_screen_gl,
  blit_sgb_mask_gl
  //NULL,
  //draw_message_gl,
  //NULL
};

#endif
