#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../cpu.h"
#include "../memory.h"
#include "log.h"

FILE *log_file;

UINT16 nb_nop=0;

void open_log(void) {
  active_log=0;
  log_file=fopen("debuger.log","wt");  
}

void close_log(void) {
  if (log_file) fclose(log_file);
}

void put_log(const char *format,...) {
  va_list pvar;
  va_start(pvar,format);
  if (log_file) {
    fprintf(log_file,"PC:%04x LY:%02x LYC:%02x halt:%d IME:%d ",gbcpu->pc.w,CURLINE,CMP_LINE,gbcpu->state,gbcpu->int_flag);
    switch(LCDCSTAT&0x03) {
    case 0x00:fprintf(log_file,"HBLANK ");break;
    case 0x01:fprintf(log_file,"VBLANK ");break;
    case 0x02:fprintf(log_file,"   OAM ");break;
    case 0x03:fprintf(log_file,"  VRAM ");break;
    }
    vfprintf(log_file,format,pvar);
  }
  va_end(pvar);
}

void put_log_message(const char *mes) {
  fprintf(log_file,mes);
  fprintf(log_file,"\n");
}




