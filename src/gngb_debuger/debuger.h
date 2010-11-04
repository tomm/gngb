#ifndef DEBUGER_H
#define DEBUGER_H

#include "../global.h"

void get_mem_id(UINT16 add,char *ret);

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

#endif
