#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "cpu.h"
#include "memory.h"

FILE *log_file;
FILE *log_file2;
void open_log(void) {
  log_file=NULL;
  log_file2=NULL;
  /*log_file=fopen("debuger.log","wt");  
    log_file2=fopen("debuger.log2","wt"); */
  log_file=log_file2=stderr;
  log_file2=NULL;
}

void close_log(void) {
  if (log_file) fclose(log_file);
  if (log_file2) fclose(log_file2);
}

void put_log(const char *format,...) {
  va_list pvar;
  va_start(pvar,format);
  if (log_file) {
    fprintf(log_file,"PC:%04x ROM_PAGE:%d ",gbcpu->pc.w,active_rom_page);
    vfprintf(log_file,format,pvar);
  }
  va_end(pvar);
}

void put_log2(const char *format,...) {
  va_list pvar;
  va_start(pvar,format);
  if (log_file2) {
    fprintf(log_file2,"PC:%04x ",gbcpu->pc.w);
    vfprintf(log_file2,format,pvar);
  }
  va_end(pvar);
}
