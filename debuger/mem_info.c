#include "mem_info.h"
#include "../memory.h"


void show_mem_info(UINT16 add,UINT8 nb_line) {
  int i;
  for(;nb_line;nb_line--) {
    printf("%04x  ",add);
    for(i=0;i<16;i++) 
      printf("%02x ",mem_read(add++));
    printf("\n");
  }  
}
