#ifndef _VIDEO_GL_H_
#define _VIDEO_GL_H_

#ifdef SDL_GL

#include <GL/gl.h>

typedef struct GLTEX {
  GLubyte *bmp;
  GLuint id;
}GLTEX;

GLTEX gl_tex;
GLTEX sgb_tex;

void sgb_init_gl(void);

#endif

#endif
