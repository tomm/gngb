#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../cpu.h"
#include "../memory.h"
#include "log.h"

FILE *log_file;

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
    fprintf(log_file,"PC:%04x ROM_PAGE:%d halt:%d IME:%d ",gbcpu->pc.w,active_rom_page,gbcpu->state,gbcpu->int_flag);
    vfprintf(log_file,format,pvar);
  }
  va_end(pvar);
}

