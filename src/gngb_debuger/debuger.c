#include <string.h>
#include "debuger.h"
#include "debuger_wid.h"
#include "break.h"
#include "../emu.h"
#include "../global.h"
#include "../cpu.h"
#include "../vram.h"
#include "../memory.h"

void get_mem_id(Uint16 adr,char *ret) {
  Uint8 bank;

  ret[0]=0;
  if (adr>=0xfe00 && adr<0xfea0) {
    strcat(ret,"OAM");
    return;
  }

  if (adr>=0xfea0) {
    strcat(ret,"IO");
    return;
  }

  if (adr>=0xe000 && adr<0xfe00) adr-=0x2000;  // echo mem

  bank=(adr&0xf000)>>12;
  switch(bank) {
  case 0x00:
  case 0x01:
  case 0x02:
  case 0x03:
    strcat(ret,"ROM0");
    return;
    
  case 0x04:
  case 0x05:
  case 0x06:
  case 0x07:
    strcat(ret,"ROM");
    sprintf(&ret[3],"%d",active_rom_page);
    return;
    
  case 0x08:
  case 0x09:
    strcat(ret,"VRAM");
    sprintf(&ret[4],"%d",active_vram_page);
    return;
    
  case 0x0a:
  case 0x0b:
    strcat(ret,"RAM");
    sprintf(&ret[3],"%d",active_ram_page);
    return;

  case 0x0c:
    strcat(ret,"WRAM0");
    return;
    
  case 0x0d:
    strcat(ret,"WRAM");
    sprintf(&ret[4],"%d",active_wram_page);
    return;
  }
}

// Debug 

void update_all(void) {
  char t=active_msg;
  active_msg=0;
  update_cpu_info();
  update_mem_info();
  update_io_info();
  update_code_info();
  active_msg=t;
}

int continue_run(void) {
  return !test_all_break();
}

void db_run(void) {
  conf.gb_done=0;
  update_gb();
  if (conf.fs) switch_fullscreen();
  update_all();
}

void db_step(void) {
  conf.gb_done=1;
  update_gb();
  update_all();
}

void db_set_bp(void) {
  code_info_set_bp();
}

