#include "../interrupt.h"
#include "../memory.h"
#include "io_info.h"

void show_io_info(void) {
  printf("********* LCDC ***********\n");
  printf("LCDCCONT %02x\n",LCDCCONT);
  printf("LCDCSTAT %02x ",LCDCSTAT);
  switch(LCDCSTAT&0x03) {
  case 0:printf("HBLANK\n");break;
  case 1:printf("VBLANK\n");break; 
  case 2:printf("OAM\n");break;
  case 3:printf("VRAM\n");break;
  }			      
  printf("CURLINE %02x CMPLINE %02x\n",CURLINE,CMP_LINE);
  printf("cycle todo : %d\n",gblcdc->cycle);
  printf("******* INTERRUPT *********\n");
  printf("INT_ENABLE %02x INT_FLAG %02x\n",INT_ENABLE,INT_FLAG);
}
