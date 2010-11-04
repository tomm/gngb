/* The code of autoframeskip is taken from the emulator Mame */

#ifndef _FRAME_SKIP_H
#define _FRAME_SKIP_H

int barath_skip_next_frame(int showfps);

extern char skip_next_frame;

void reset_frame_skip(void);
int frame_skip(int init);


#endif

