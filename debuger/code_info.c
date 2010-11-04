#include "code_info.h"
#include "../memory.h"
#include "op.h"

void show_code_info(UINT16 pc,UINT16 nb_line) {
  char t[30];
  int last,id,i;

  for(i=0;i<nb_line;i++) {
    t[0]=0;
    last=pc;
    pc+=aff_op(id=mem_read(pc),pc,t);
    printf("address %x val %02x : %s \n",last,id,t);
    pc++;
  }
}
