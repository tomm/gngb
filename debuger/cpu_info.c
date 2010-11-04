#include <stdio.h>
#include "cpu_info.h"
#include "op.h"
#include "../cpu.h"
#include "../memory.h"

void show_cpu_info(void) {
  char t[30];
  UINT16 pc=gbcpu->pc.w;
  aff_op(mem_read(pc),pc,t);
  printf("[AF: %04x]    [HL: %04x]    [BC: %04x]",
	 gbcpu->af.w,
	 gbcpu->hl.w,
	 gbcpu->bc.w);
  if (gbcpu->int_flag) printf("   EI \n");
  else printf("   DI \n");
  printf("[DE: %04x]    [SP: %04x]    [PC: %04x]",
	 gbcpu->de.w,
	 gbcpu->sp.w,
	 gbcpu->pc.w);
  printf("    [");
  if (gbcpu->af.w&0x80) printf("Z"); else printf("z");
  if (gbcpu->af.w&0x40) printf("N"); else printf("n");
  if (gbcpu->af.w&0x20) printf("H"); else printf("h");
  if (gbcpu->af.w&0x10) printf("C....]\n"); else printf("c....]\n");    
  printf("[(SP): %02x%02x]   [(PC): %02x - %s ] \n",
	 mem_read(gbcpu->sp.w+1),mem_read(gbcpu->sp.w),
	 mem_read(pc),t);
  printf("state : ");
  if (gbcpu->state==HALT_STATE) printf("halt\n");
  else printf("normal\n");
}
