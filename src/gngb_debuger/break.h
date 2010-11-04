#ifndef BREAK_H
#define BREAK_H

#include "../global.h"

void add_break_point(UINT16 add);
void del_break_point(UINT16 add);
char is_break_point(UINT16 add);
char test_all_break();

#endif 
