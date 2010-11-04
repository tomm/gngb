#ifndef DEBUGER_H
#define DEBUGER_H

#include "../global.h"

void get_mem_id(Uint16 add,char *ret);

// Messages

extern char active_msg;
void add_msg(const char *format,...);

#define add_cpu_msg add_msg
#define add_int_msg add_msg
#define add_mem_msg add_msg
#define add_sgb_msg add_msg

// Debug 

void db_run(void);
void db_step(void);
void db_set_bp(void);
void continue_run(void);

// Vram

extern char active_back;
extern char active_obj;
extern char active_win;

// Interrupt

extern char vblank_int_enable;
extern char lcd_oam_int_enable;
extern char lcd_lyc_int_enable;
extern char lcd_hblank_int_enable;
extern char lcd_vblank_int_enable;
extern char lcd_vram_int_enable;
extern char timer_int_enable;
extern char link_int_enable;
extern char pad_int_eanble;


#endif

