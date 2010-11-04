/*  gngb, a game boy color emulator
 *  Copyright (C) 2001 Peponas Thomas & Peponas Mathieu
 * 
 *  This program is free software; you can redistribute it and/or modify  
 *  it under the terms of the GNU General Public License as published by   
 *  the Free Software Foundation; either version 2 of the License, or    
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */


#include <stdio.h>
#include <stdlib.h>
#include "rom.h"
#include "cpu.h"
#include "memory.h"
#include "interrupt.h"
#include "serial.h"
#include "emu.h"

GB_CPU *gbcpu=0;

//extern Sint32 gdma_cycle;

static Uint8 t8;
static Sint8 st8;
static Uint16 t16;
static Sint16 st16;
static Uint32 t32;
static Uint8 v;
static Uint8 a;

// registre

#define REG_AF (gbcpu->af)
#define REG_BC (gbcpu->bc)
#define REG_DE (gbcpu->de)
#define REG_HL (gbcpu->hl)
#define REG_SP (gbcpu->sp)
#define REG_PC (gbcpu->pc)
#define AF (gbcpu->af.w)
#define BC (gbcpu->bc.w)
#define DE (gbcpu->de.w)
#define HL (gbcpu->hl.w)
#define PC (gbcpu->pc.w)
#define SP (gbcpu->sp.w)
#define A (gbcpu->af.b.h)
#define F (gbcpu->af.b.l)
#define B (gbcpu->bc.b.h)
#define C (gbcpu->bc.b.l)
#define D (gbcpu->de.b.h)
#define E (gbcpu->de.b.l)
#define H (gbcpu->hl.b.h)
#define L (gbcpu->hl.b.l)

__inline__ Uint16 get_word(void)
{
  //Uint16 v=((Uint16)(mem_read(PC)));
  Uint16 v1,v2;
  mem_read_fast(PC,v1);
  PC++;
  //v|=(Uint16)((mem_read(PC)<<8));
  mem_read_fast(PC,v2);
  PC++;
  return (v1|(v2<<8));
  //return v;
}

__inline__ Uint8 get_byte(void)
{
  //Uint8 t=mem_read(PC);
  Uint8 t;
  mem_read_fast(PC,t);
  PC++;
  return t;
}

__inline__ void push_r(REG *r)
{
  //mem_write(--SP,(r)->b.h);
  //mem_write(--SP,(r)->b.l);
  SP--;
  mem_write_fast(SP,(r)->b.h);
  SP--;
  mem_write_fast(SP,(r)->b.l);
}

__inline__ void pop_r(REG *r)
{
  //(r)->b.l=mem_read(SP);
  mem_read_fast(SP,(r)->b.l);
  SP++;
  //(r)->b.h=mem_read(SP);
  mem_read_fast(SP,(r)->b.h);
  SP++;
} 

#define GET_BYTE get_byte()
#define GET_WORD get_word()


#define SUB_CYCLE(c) return (c)
//define SUB_CYCLE(c) a=c;break;

#define PUSH_R(r) (push_r(&r))
#define POP_R(r) (pop_r(&r))

#define EI (gbcpu->ei_flag=1)
#define DI (gbcpu->int_flag=0)

#define CP_A_R(v) {t16=A-(v); ((t16&0x0100)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); ((t16&0xff)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); ((((v&0x0f)>((A)&0x0f)))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH)); SET_FLAG(FLAG_N); } 

#define RLC_R(r) {t8=((r)&0x80)>>7; (r)=((r)<<1)|t8; ((t8)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH);}

#define RRC_R(r) {t8=((r)&0x01)<<7; (r)=((r)>>1)|t8; ((t8)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH);}

#define RL_R(r) {t8=(IS_SET(FLAG_C)?(1):(0)); (((r)&0x80)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); (r)=((r)<<1)|t8; ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH); }

#define SLA_R(r) {(((r)&0x80)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); (r)<<=1; ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH);}

#define SRA_R(r) {(((r)&0x01)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); (r)=((r)&0x80)|((r)>>1); ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH);}

#define SWAP_R(r) {(r)=(((r)&0xf0)>>4)|(((r)&0x0f)<<4); ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH&FLAG_NC);}

#define SRL_R(r) {(((r)&0x01)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); (r)>>=1; ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH);}

#define BIT_N_R(n,r) {(((r)&(n))?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); SET_FLAG(FLAG_H); UNSET_FLAG(FLAG_NN);}




void gbcpu_init(void)
{
  gbcpu=(GB_CPU *)malloc(sizeof(GB_CPU));
  gbcpu_reset();
}

void gbcpu_reset(void) {
  //if (conf.normal_gb || conf.gb_type==NORMAL_GAMEBOY) {
  if (conf.gb_type&COLOR_GAMEBOY) {
    /* FIXME : 0xff => SGB2 */
    /*if (conf.gb_type&SUPER_GAMEBOY) 
      gbcpu->af.w=0xffb0;
      else*/ gbcpu->af.w=0x11b0;
  }
  else if (conf.gb_type&NORMAL_GAMEBOY || conf.gb_type&SUPER_GAMEBOY) 
    gbcpu->af.w=0x01B0;
    
  gbcpu->bc.w=(Uint16)0x0013;
  gbcpu->hl.w=(Uint16)0x014d;
  gbcpu->de.w=(Uint16)0x00d8;
  gbcpu->sp.w=(Uint16)0xFFFE;
  gbcpu->pc.w=(Uint16)0x0100;
  gbcpu->mode=0;
  gbcpu->state=0;
  gbcpu->int_flag=0;
  gbcpu->ei_flag=1;
  gbcpu->mode=SIMPLE_SPEED;
}

// GAMEBOY OPERANDE 

__inline__ Uint8 unknown(void){
  printf("unknow opcode");
  return 0;
}




__inline__ Uint8 gbcpu_exec_one(void)
{
	static Uint8 opcode;
	if (gbcpu->ei_flag==1) {
		gbcpu->int_flag=1;  
		gbcpu->ei_flag=0;
	}
	mem_read_fast(PC,opcode);
	PC++;
	switch(opcode) {

  case 0x0:
	  SUB_CYCLE(4);
  case 0x1:
	  BC=GET_WORD;
	  SUB_CYCLE(12);
  case 0x2:
	  mem_write_fast(BC,A);
	  SUB_CYCLE(8);
  case 0x3:
	  BC++;
	  SUB_CYCLE(8);
  case 0x4:
	  ((B^0x0f)? UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
	  B++;
	  ((B) ? UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x5:
	  ((B&0x0f)?UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
	  B--;
	  ((B)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x6:
	  B=GET_BYTE;
	  SUB_CYCLE(8);
  case 0x7:
	  ((t8=(A&0x80))?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  UNSET_FLAG(FLAG_NN&FLAG_NZ&FLAG_NH);
	  A=((A<<1)|(t8>>7));
	  SUB_CYCLE(4);
  case 0x8:
	  t16=GET_WORD;
	  mem_write_fast(t16,REG_SP.b.l);
	  mem_write_fast(t16+1,REG_SP.b.h);
	  SUB_CYCLE(20);
  case 0x9:
	  t32=HL+BC;
	  ((t32&0x00010000)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((HL&0x0f)+(BC&0x0f))>0x0f)?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  HL=t32&0xffff;
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(8);
  case 0xa:
	  mem_read_fast(BC,A);
	  SUB_CYCLE(8);
	case 0xb:
	  BC--;
	  SUB_CYCLE(8);
  case 0xc:
	  ((C^0x0f)? UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
	  C++;
	  ((C) ? UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0xd:
	  ((C&0x0f)?UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
	  C--;
	  ((C)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0xe:
	  C=GET_BYTE;
	  SUB_CYCLE(8);
  case 0xf:
	  ((t8=(A&0x01))?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  UNSET_FLAG(FLAG_NN&FLAG_NZ&FLAG_NH);
	  A=((A>>1)|(t8<<7));
	  SUB_CYCLE(4);
	case 0x10:
	  /* FIXME: Stop=>PC++ fix konami2 collection */
	  PC++;
	  //printf("Stop Instruction at %04x",PC);
	  SUB_CYCLE(4);
  case 0x11:
	  DE=GET_WORD;
	  SUB_CYCLE(12);
  case 0x12:
	  mem_write_fast(DE,A);
	  SUB_CYCLE(8);
  case 0x13:
	  DE++;
	  SUB_CYCLE(8);
  case 0x14:
	  ((D^0x0f)? UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
	  D++;
	  ((D) ? UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x15:
	  ((D&0x0f)?UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
	  D--;
	  ((D)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x16:
	  D=GET_BYTE;
	  SUB_CYCLE(8);
  case 0x17:
	  t8=((IS_SET(FLAG_C))?1:0);
	  ((A&0x80)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  UNSET_FLAG(FLAG_NN&FLAG_NZ&FLAG_NH);
	  A=(A<<1)|t8;
	  SUB_CYCLE(4);
  case 0x18:
	  PC=((Sint8)GET_BYTE)+PC;
	  SUB_CYCLE(12);
  case 0x19:
	  t32=HL+DE;
	  ((t32&0x00010000)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((HL&0x0f)+(DE&0x0f))>0x0f)?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  HL=t32&0xffff;
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(8);
  case 0x1a:
	  mem_read_fast(DE,A);
	  SUB_CYCLE(8);
  case 0x1b:
	  DE--;
	  SUB_CYCLE(8);
  case 0x1c:
	  ((E^0x0f)? UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
	  E++;
	  ((E) ? UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x1d:
	  ((E&0x0f)?UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
	  E--;
	  ((E)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x1e:
	  E=GET_BYTE;
	  SUB_CYCLE(8);
  case 0x1f:
	  t8=((IS_SET(FLAG_C))?1:0);
	  ((A&0x01)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  UNSET_FLAG(FLAG_NN&FLAG_NZ&FLAG_NH);
	  A=(A>>1)|(t8<<7);
	  SUB_CYCLE(4);
	case 0x20:
	  if (IS_SET(FLAG_Z)) {
		  PC++;
		  SUB_CYCLE(8);
	  } else {
		  PC=((Sint8)GET_BYTE)+PC;
		  SUB_CYCLE(12);
	  }
  case 0x21:
	  HL=GET_WORD;
	  SUB_CYCLE(12);
  case 0x22:
	  mem_write_fast(HL,A);
	  HL++;
	  SUB_CYCLE(8);
  case 0x23:
	  HL++;
	  SUB_CYCLE(8);
  case 0x24:
	  ((H^0x0f)? UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
	  H++;
	  ((H) ? UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x25:
	  ((H&0x0f)?UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
	  H--;
	  ((H)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x26:
	  H=GET_BYTE;
	  SUB_CYCLE(8);
  case 0x27:
	  t16=A;
	  if ((t16&0x0f) > 0x09) {
		  if (IS_SET(FLAG_N)) t16-=6;
		  else t16+=06;
	  }
	  if ((t16&0xf0)>0x90) {
		  if (IS_SET(FLAG_N)) t16-=0x60;
		  else t16+=0x60;
		  SET_FLAG(FLAG_C);
	  } else UNSET_FLAG(FLAG_NC);
	  A=t16&0xff;
	  UNSET_FLAG(FLAG_NH);
	  SUB_CYCLE(4);
  case 0x28:
	  if (IS_SET(FLAG_Z)) {
		  PC=((Sint8)GET_BYTE)+PC;
		  SUB_CYCLE(12);
	  } else {
		  PC++;
		  SUB_CYCLE(8);
	  }
  case 0x29:
	  t32=HL+HL;
	  ((t32&0x00010000)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((HL&0x0f)+(HL&0x0f))>0x0f)?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  HL=t32&0xffff;
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(8);
  case 0x2a:
	  mem_read_fast(HL,A);
	  HL++;
	  SUB_CYCLE(8);
  case 0x2b:
	  HL--;
	  SUB_CYCLE(8);
  case 0x2c:
	  ((L^0x0f)? UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
	  L++;
	  ((L) ? UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x2d:
	  ((L&0x0f)?UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
	  L--;
	  ((L)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x2e:
	  L=GET_BYTE;
	  SUB_CYCLE(8);
  case 0x2f:
	  A=(~A);
	  SET_FLAG(FLAG_N|FLAG_H);
	  SUB_CYCLE(4);
	case 0x30:
	  if (IS_SET(FLAG_C)) {
		  PC++;
		  SUB_CYCLE(8);
	  } else {
		  PC=((Sint8)GET_BYTE)+PC;
		  SUB_CYCLE(12);
	  }
  case 0x31:
	  SP=GET_WORD;
	  SUB_CYCLE(12);
  case 0x32:
	  mem_write_fast(HL,A);
	  HL--;
	  SUB_CYCLE(8);
  case 0x33:
	  SP++;
	  SUB_CYCLE(8);
  case 0x34:
	  mem_read_fast(HL,t8);
	  ((t8^0x0f)? UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
	  t8++;
	  ((t8) ? UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  mem_write_fast(HL,t8);
	  SUB_CYCLE(12);
  case 0x35:
	  mem_read_fast(HL,t8);
	  ((t8&0x0f)?UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
	  t8--;
	  ((t8)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  mem_write_fast(HL,t8);
	  SUB_CYCLE(12);
  case 0x36:
	  t8=GET_BYTE;
	  mem_write_fast(HL,t8);
	  SUB_CYCLE(12);
  case 0x37:
	  SET_FLAG(FLAG_C);
	  UNSET_FLAG(FLAG_NN&FLAG_NH);
	  SUB_CYCLE(4);
  case 0x38:
	  if (IS_SET(FLAG_C)) {
		  PC=((Sint8)GET_BYTE)+PC;
		  SUB_CYCLE(12);
	  } else {
		  PC++;
		  SUB_CYCLE(8);
  	  }
  case 0x39:
	  t32=HL+SP;
	  ((t32&0x00010000)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((HL&0x0f)+(SP&0x0f))>0x0f)?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  HL=t32&0xffff;
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(8);
  case 0x3a:
	  mem_read_fast(HL,A);
	  HL--;
	  SUB_CYCLE(8);
  case 0x3b:
	  SP--;
	  SUB_CYCLE(8);
  case 0x3c:
	  ((A^0x0f)? UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
	  A++;
	  ((A) ? UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x3d:
	  ((A&0x0f)?UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
	  A--;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x3e:
	  A=GET_BYTE;
	  SUB_CYCLE(8);
  case 0x3f:
	  F^=FLAG_C;
	  UNSET_FLAG(FLAG_NN&FLAG_NH);
	  SUB_CYCLE(4);
	case 0x40:
	  SUB_CYCLE(4);
  case 0x41:
	  B=C;
	  SUB_CYCLE(4);
  case 0x42:
	  B=D;
	  SUB_CYCLE(4);
  case 0x43:
	  B=E;
	  SUB_CYCLE(4);
  case 0x44:
	  B=H;
	  SUB_CYCLE(4);
  case 0x45:
	  B=L;
	  SUB_CYCLE(4);
  case 0x46:
	  mem_read_fast(HL,B);
	  SUB_CYCLE(8);
  case 0x47:
	  B=A;
	  SUB_CYCLE(4);
  case 0x48:
	  C=B;
	  SUB_CYCLE(4);
  case 0x49:
	  SUB_CYCLE(4);
  case 0x4a:
	  C=D;
	  SUB_CYCLE(4);
  case 0x4b:
	  C=E;
	  SUB_CYCLE(4);
  case 0x4c:
	  C=H;
	  SUB_CYCLE(4);
  case 0x4d:
	  C=L;
	  SUB_CYCLE(4);
  case 0x4e:
	  mem_read_fast(HL,C);
	  SUB_CYCLE(8);
  case 0x4f:
	  C=A;
	  SUB_CYCLE(4);
   case 0x50:
	  D=B;
	  SUB_CYCLE(4);
  case 0x51:
	  D=C;
	  SUB_CYCLE(4);
  case 0x52:
	  SUB_CYCLE(4);
  case 0x53:
	  D=E;
	  SUB_CYCLE(4);
  case 0x54:
	  D=H;
	  SUB_CYCLE(4);
  case 0x55:
	  D=L;
	  SUB_CYCLE(4);
  case 0x56:
	  mem_read_fast(HL,D);
	  SUB_CYCLE(8);
  case 0x57:
	  D=A;
	  SUB_CYCLE(4);
  case 0x58:
	  E=B;
	  SUB_CYCLE(4);
  case 0x59:
	  E=C;
	  SUB_CYCLE(4);
  case 0x5a:
	  E=D;
	  SUB_CYCLE(4);
  case 0x5b:
	  SUB_CYCLE(4);
  case 0x5c:
	  E=H;
	  SUB_CYCLE(4);
  case 0x5d:
	  E=L;
	  SUB_CYCLE(4);
  case 0x5e:
	  mem_read_fast(HL,E);
	  SUB_CYCLE(8);
  case 0x5f:
	  E=A;
	  SUB_CYCLE(4);
 case 0x60:
	  H=B;
	  SUB_CYCLE(4);
  case 0x61:
	  H=C;
	  SUB_CYCLE(4);
  case 0x62:
	  H=D;
	  SUB_CYCLE(4);
  case 0x63:
	  H=E;
	  SUB_CYCLE(4);
  case 0x64:
	  SUB_CYCLE(4);
  case 0x65:
	  H=L;
	  SUB_CYCLE(4);
  case 0x66:
	  mem_read_fast(HL,H);
	  SUB_CYCLE(8);
  case 0x67:
	  H=A;
	  SUB_CYCLE(4);
  case 0x68:
	  L=B;
	  SUB_CYCLE(4);
  case 0x69:
	  L=C;
	  SUB_CYCLE(4);
  case 0x6a:
	  L=D;
	  SUB_CYCLE(4);
  case 0x6b:
	  L=E;
	  SUB_CYCLE(4);
  case 0x6c:
	  L=H;
	  SUB_CYCLE(4);
  case 0x6d:
	  SUB_CYCLE(4);
  case 0x6e:
	  mem_read_fast(HL,L);
	  SUB_CYCLE(8);
  case 0x6f:
	  L=A;
	  SUB_CYCLE(4);
 case 0x70:
	  mem_write_fast(HL,B);
	  SUB_CYCLE(8);
  case 0x71:
	  mem_write_fast(HL,C);
	  SUB_CYCLE(8);
  case 0x72:
	  mem_write_fast(HL,D);
	  SUB_CYCLE(8);
  case 0x73:
	  mem_write_fast(HL,E);
	  SUB_CYCLE(8);
  case 0x74:
	  mem_write_fast(HL,H);
	  SUB_CYCLE(8);
  case 0x75:
	  mem_write_fast(HL,L);
	  SUB_CYCLE(8);
  case 0x76:
	  if (gbcpu->int_flag) {
		  gbcpu->state=HALT_STATE;
		  gbcpu->pc.w--;
	  } else {
		  //printf("WARNING Halt with DIn");
	  }
	  SUB_CYCLE(4);
  case 0x77:
	  mem_write_fast(HL,A);
	  SUB_CYCLE(8);
  case 0x78:
	  A=B;
	  SUB_CYCLE(4);
  case 0x79:
	  A=C;
	  SUB_CYCLE(4);
  case 0x7a:
	  A=D;
	  SUB_CYCLE(4);
  case 0x7b:
	  A=E;
	  SUB_CYCLE(4);
  case 0x7c:
	  A=H;
	  SUB_CYCLE(4);
  case 0x7d:
	  A=L;
	  SUB_CYCLE(4);
  case 0x7e:
	  mem_read_fast(HL,A);
	  SUB_CYCLE(8);
  case 0x7f:
	  SUB_CYCLE(4);
 case 0x80:
	  st16=A+B;
	  ((st16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(B&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=st16&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x81:
	  st16=A+C;
	  ((st16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(C&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=st16&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x82:
	  st16=A+D;
	  ((st16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(D&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=st16&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x83:
	  st16=A+E;
	  ((st16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(E&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=st16&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x84:
	  st16=A+H;
	  ((st16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(H&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=st16&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x85:
	  st16=A+L;
	  ((st16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(L&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=st16&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x86:
	  mem_read_fast(HL,t8);
	  st16=A+t8;
	  ((st16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(t8&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=st16&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(8);
  case 0x87:
	  st16=A+A;
	  ((st16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(A&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=st16&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x88:
	  t16=B+(IS_SET(FLAG_C)?(1):(0));
	  t32=A+t16;
	  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=t32&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x89:
	  t16=C+(IS_SET(FLAG_C)?(1):(0));
	  t32=A+t16;
	  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=t32&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x8a:
	  t16=D+(IS_SET(FLAG_C)?(1):(0));
	  t32=A+t16;
	  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=t32&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x8b:
	  t16=E+(IS_SET(FLAG_C)?(1):(0));
	  t32=A+t16;
	  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=t32&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x8c:
	  t16=H+(IS_SET(FLAG_C)?(1):(0));
	  t32=A+t16;
	  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=t32&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x8d:
	  t16=L+(IS_SET(FLAG_C)?(1):(0));
	  t32=A+t16;
	  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=t32&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
  case 0x8e:
	  mem_read_fast(HL,t16);
	  t16+=(IS_SET(FLAG_C)?(1):(0));
	  t32=A+t16;
	  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=t32&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(8);
  case 0x8f:
	  t16=A+(IS_SET(FLAG_C)?(1):(0));
	  t32=A+t16;
	  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=t32&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(4);
 case 0x90:
	  ((B>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((B&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=B;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x91:
	  ((C>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((C&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=C;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x92:
	  ((D>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((D&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=D;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x93:
	  ((E>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((E&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=E;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x94:
	  ((H>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((H&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=H;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x95:
	  ((L>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((L&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=L;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x96:
	  mem_read_fast(HL,t8);
	  ((t8>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((t8&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=t8;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(8);
  case 0x97:
	  ((A>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=A;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x98:
	  t16=B+((IS_SET(FLAG_C))?(1):(0));
	  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=t16;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x99:
	  t16=C+((IS_SET(FLAG_C))?(1):(0));
	  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=t16;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x9a:
	  t16=D+((IS_SET(FLAG_C))?(1):(0));
	  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=t16;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x9b:
	  t16=E+((IS_SET(FLAG_C))?(1):(0));
	  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=t16;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x9c:
	  t16=H+((IS_SET(FLAG_C))?(1):(0));
	  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=t16;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x9d:
	  t16=L+((IS_SET(FLAG_C))?(1):(0));
	  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=t16;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
  case 0x9e:
	  mem_read_fast(HL,t16);
	  t16+=((IS_SET(FLAG_C))?(1):(0));
	  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=t16;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(8);
  case 0x9f:
	  t16=A+((IS_SET(FLAG_C))?(1):(0));
	  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=t16;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(4);
	case 0xa0:
	  A&=B;
	  if (A) {
		  UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_H);
	  } else {
		  UNSET_FLAG(FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_Z|FLAG_H);
	  }
	  SUB_CYCLE(4);
  case 0xa1:
	  A&=C;
	  if (A) {
		  UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_H);
	  } else {
		  UNSET_FLAG(FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_Z|FLAG_H);
	  }
	  SUB_CYCLE(4);
  case 0xa2:
	  A&=D;
	  if (A) {
		  UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_H);
	  } else {
		  UNSET_FLAG(FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_Z|FLAG_H);
	  }
	  SUB_CYCLE(4);
  case 0xa3:
	  A&=E;
	  if (A) {
		  UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_H);
	  } else {
		  UNSET_FLAG(FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_Z|FLAG_H);
	  }
	  SUB_CYCLE(4);
  case 0xa4:
	  A&=H;
	  if (A) {
		  UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_H);
	  } else {
		  UNSET_FLAG(FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_Z|FLAG_H);
	  }
	  SUB_CYCLE(4);
  case 0xa5:
	  A&=L;
	  if (A) {
		  UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_H);
	  } else {
		  UNSET_FLAG(FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_Z|FLAG_H);
	  }
	  SUB_CYCLE(4);
  case 0xa6:
	  mem_read_fast(HL,t8);
	  A&=t8;
	  if (A) {
		  UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_H);
	  } else {
		  UNSET_FLAG(FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_Z|FLAG_H);
	  }
	  SUB_CYCLE(8);
  case 0xa7:
	  A&=A;
	  if (A) {
		  UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_H);
	  } else {
		  UNSET_FLAG(FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_Z|FLAG_H);
	  }
	  SUB_CYCLE(4);
  case 0xa8:
	  A^=B;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(4);
  case 0xa9:
	  A^=C;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(4);
  case 0xaa:
	  A^=D;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(4);
  case 0xab:
	  A^=E;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(4);
  case 0xac:
	  A^=H;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(4);
  case 0xad:
	  A^=L;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(4);
  case 0xae:
	  mem_read_fast(HL,t8);
	  A^=t8;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(8);
  case 0xaf:
	  A^=A;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(4);
	case 0xb0:
	  A|=B;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(4);
  case 0xb1:
	  A|=C;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(4);
  case 0xb2:
	  A|=D;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(4);
  case 0xb3:
	  A|=E;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(4);
  case 0xb4:
	  A|=H;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(4);
  case 0xb5:
	  A|=L;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(4);
  case 0xb6:
	  mem_read_fast(HL,t8);
	  A|=t8;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(8);
  case 0xb7:
	  A|=A;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(4);
  

#define CP_A_R(v) {t16=A-(v); ((t16&0x0100)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); ((t16&0xff)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); ((((v&0x0f)>((A)&0x0f)))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH)); SET_FLAG(FLAG_N); }

  case 0xb8:
	  CP_A_R(B);
	  SUB_CYCLE(4);
  case 0xb9:
	  CP_A_R(C);
	  SUB_CYCLE(4);
  case 0xba:
	  CP_A_R(D);
	  SUB_CYCLE(4);

  case 0xbb:
	  CP_A_R(E);
	  SUB_CYCLE(4);
  case 0xbc:
	  CP_A_R(H);
	  SUB_CYCLE(4);
  case 0xbd:
	  CP_A_R(L);
	  SUB_CYCLE(4);
  case 0xbe:
	  mem_read_fast(HL,t8);
	  CP_A_R(t8);
	  SUB_CYCLE(8);
  case 0xbf:
	  CP_A_R(A);
	  SUB_CYCLE(4);
	case 0xc0:
	  if (IS_SET(FLAG_Z)) SUB_CYCLE(8);
	  else {
		  POP_R(REG_PC);
		  SUB_CYCLE(20);
	  }
  case 0xc1:
	  POP_R(REG_BC);
	  SUB_CYCLE(12);
  case 0xc2:
	  if (IS_SET(FLAG_Z)) {
		  PC+=2;
		  SUB_CYCLE(12);
	  }
	  else {
		  PC=GET_WORD;
		  SUB_CYCLE(16);
	  }
  case 0xc3:
	  PC=GET_WORD;
	  SUB_CYCLE(16);
  case 0xc4:
	  if (IS_SET(FLAG_Z)) {
		  PC+=2;
		  SUB_CYCLE(12);
	  } else {
		  Uint16 v=GET_WORD;
		  PUSH_R(REG_PC);
		  PC=v;
		  SUB_CYCLE(24);
	  }
  case 0xc5:
	  PUSH_R(REG_BC);
	  SUB_CYCLE(16);
  case 0xc6:
	  t8=GET_BYTE;
	  t16=A+t8;
	  ((t16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(t8&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=t16&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(8);
  case 0xc7:
	  PUSH_R(REG_PC);
	  PC=0x0000;
	  SUB_CYCLE(16);
  case 0xc8:
	  if (IS_SET(FLAG_Z)) {
		  POP_R(REG_PC);
		  SUB_CYCLE(20);
	  }else SUB_CYCLE(8);
  case 0xc9:
	  POP_R(REG_PC);
	  SUB_CYCLE(16);
  case 0xca:
	  if (IS_SET(FLAG_Z)) {
		  PC=GET_WORD;
		  SUB_CYCLE(16);
	  } else {
		  PC+=2;
		  SUB_CYCLE(12);
	  }
	  
  case 0xcb: 
	  switch(GET_BYTE) {
#define RLC_R(r) {t8=((r)&0x80)>>7; (r)=((r)<<1)|t8; ((t8)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH);}


case 0x0:
RLC_R(B);
SUB_CYCLE(8);


case 0x1:
RLC_R(C);
SUB_CYCLE(8);


case 0x2:
RLC_R(D);
SUB_CYCLE(8);


case 0x3:
RLC_R(E);
SUB_CYCLE(8);


case 0x4:
RLC_R(H);
SUB_CYCLE(8);


case 0x5:
RLC_R(L);
SUB_CYCLE(8);


case 0x6:
mem_read_fast(HL,a);
RLC_R(a);
mem_write_fast(HL,a);
SUB_CYCLE(16);


case 0x7:
RLC_R(A);
SUB_CYCLE(8);


#define RRC_R(r) {t8=((r)&0x01)<<7; (r)=((r)>>1)|t8; ((t8)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH);}


case 0x8:
RRC_R(B);
SUB_CYCLE(8);


case 0x9:
RRC_R(C);
SUB_CYCLE(8);


case 0xa:
RRC_R(D);
SUB_CYCLE(8);


case 0xb:
RRC_R(E);
SUB_CYCLE(8);


case 0xc:
RRC_R(H);
SUB_CYCLE(8);


case 0xd:
RRC_R(L);
SUB_CYCLE(8);


case 0xe:
mem_read_fast(HL,a);
RRC_R(a);
mem_write_fast(HL,a);
SUB_CYCLE(16);


case 0xf:
RRC_R(A);
SUB_CYCLE(8);


#define RL_R(r) {t8=(IS_SET(FLAG_C)?(1):(0)); (((r)&0x80)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); (r)=((r)<<1)|t8; ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH); }

case 0x10:
RL_R(B);
SUB_CYCLE(8);


case 0x11:
RL_R(C);
SUB_CYCLE(8);


case 0x12:
RL_R(D);
SUB_CYCLE(8);


case 0x13:
RL_R(E);
SUB_CYCLE(8);


case 0x14:
RL_R(H);
SUB_CYCLE(8);


case 0x15:
RL_R(L);
SUB_CYCLE(8);


case 0x16:
mem_read_fast(HL,a);
RL_R(a);
mem_write_fast(HL,a);
SUB_CYCLE(16);


case 0x17:
RL_R(A);
SUB_CYCLE(8);


#define RR_R(r) {t8=(IS_SET(FLAG_C)?(0x80):(0x00)); (((r)&0x01)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); (r)=((r)>>1)|t8; ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH);}

case 0x18:
RR_R(B);
SUB_CYCLE(8);


case 0x19:
RR_R(C);
SUB_CYCLE(8);


case 0x1a:
RR_R(D);
SUB_CYCLE(8);


case 0x1b:
RR_R(E);
SUB_CYCLE(8);


case 0x1c:
RR_R(H);
SUB_CYCLE(8);


case 0x1d:
RR_R(L);
SUB_CYCLE(8);


case 0x1e:
mem_read_fast(HL,a);
RR_R(a);
mem_write_fast(HL,a);
SUB_CYCLE(16);


case 0x1f:
RR_R(A);
SUB_CYCLE(8);


#define SLA_R(r) {(((r)&0x80)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); (r)<<=1; ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH);}

case 0x20:
SLA_R(B);
SUB_CYCLE(8);


case 0x21:
SLA_R(C);
SUB_CYCLE(8);


case 0x22:
SLA_R(D);
SUB_CYCLE(8);


case 0x23:
SLA_R(E);
SUB_CYCLE(8);


case 0x24:
SLA_R(H);
SUB_CYCLE(8);


case 0x25:
SLA_R(L);
SUB_CYCLE(8);


case 0x26:
mem_read_fast(HL,a);
SLA_R(a);
mem_write_fast(HL,a);
SUB_CYCLE(16);


case 0x27:
SLA_R(A);
SUB_CYCLE(8);


#define SRA_R(r) {(((r)&0x01)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); (r)=((r)&0x80)|((r)>>1); ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH);}

case 0x28:
SRA_R(B);
SUB_CYCLE(8);


case 0x29:
SRA_R(C);
SUB_CYCLE(8);


case 0x2a:
SRA_R(D);
SUB_CYCLE(8);


case 0x2b:
SRA_R(E);
SUB_CYCLE(8);


case 0x2c:
SRA_R(H);
SUB_CYCLE(8);


case 0x2d:
SRA_R(L);
SUB_CYCLE(8);


case 0x2e:
mem_read_fast(HL,a);
SRA_R(a);
mem_write_fast(HL,a);
SUB_CYCLE(16);


case 0x2f:
SRA_R(A);
SUB_CYCLE(8);


#define SWAP_R(r) {(r)=(((r)&0xf0)>>4)|(((r)&0x0f)<<4); ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH&FLAG_NC);}

case 0x30:
SWAP_R(B);
SUB_CYCLE(8);


case 0x31:
SWAP_R(C);
SUB_CYCLE(8);


case 0x32:
SWAP_R(D);
SUB_CYCLE(8);


case 0x33:
SWAP_R(E);
SUB_CYCLE(8);


case 0x34:
SWAP_R(H);
SUB_CYCLE(8);


case 0x35:
SWAP_R(L);
SUB_CYCLE(8);


case 0x36:
mem_read_fast(HL,a);
SWAP_R(a);
mem_write_fast(HL,a);
SUB_CYCLE(16);


case 0x37:
SWAP_R(A);
SUB_CYCLE(8);


#define SRL_R(r) {(((r)&0x01)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); (r)>>=1; ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH);}

case 0x38:
SRL_R(B);
SUB_CYCLE(8);


case 0x39:
SRL_R(C);
SUB_CYCLE(8);


case 0x3a:
SRL_R(D);
SUB_CYCLE(8);


case 0x3b:
SRL_R(E);
SUB_CYCLE(8);


case 0x3c:
SRL_R(H);
SUB_CYCLE(8);


case 0x3d:
SRL_R(L);
SUB_CYCLE(8);


case 0x3e:
mem_read_fast(HL,a);
SRL_R(a);
mem_write_fast(HL,a);
SUB_CYCLE(16);


case 0x3f:
SRL_R(A);
SUB_CYCLE(8);


#define BIT_N_R(n,r) {(((r)&(n))?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); SET_FLAG(FLAG_H); UNSET_FLAG(FLAG_NN);}


case 0x40:
BIT_N_R(0x01,B);
SUB_CYCLE(8);


case 0x41:
BIT_N_R(0x01,C);
SUB_CYCLE(8);


case 0x42:
BIT_N_R(0x01,D);
SUB_CYCLE(8);


case 0x43:
BIT_N_R(0x01,E);
SUB_CYCLE(8);


case 0x44:
BIT_N_R(0x01,H);
SUB_CYCLE(8);


case 0x45:
BIT_N_R(0x01,L);
SUB_CYCLE(8);


case 0x46:
mem_read_fast(HL,a);
BIT_N_R(0x01,a);
SUB_CYCLE(12);


case 0x47:
BIT_N_R(0x01,A);
SUB_CYCLE(8);


case 0x48:
BIT_N_R(0x02,B);
SUB_CYCLE(8);


case 0x49:
BIT_N_R(0x02,C);
SUB_CYCLE(8);


case 0x4a:
BIT_N_R(0x02,D);
SUB_CYCLE(8);


case 0x4b:
BIT_N_R(0x02,E);
SUB_CYCLE(8);


case 0x4c:
BIT_N_R(0x02,H);
SUB_CYCLE(8);


case 0x4d:
BIT_N_R(0x02,L);
SUB_CYCLE(8);


case 0x4e:
mem_read_fast(HL,a);
BIT_N_R(0x02,a);
SUB_CYCLE(12);


case 0x4f:
BIT_N_R(0x02,A);
SUB_CYCLE(8);


case 0x50:
BIT_N_R(0x04,B);
SUB_CYCLE(8);


case 0x51:
BIT_N_R(0x04,C);
SUB_CYCLE(8);


case 0x52:
BIT_N_R(0x04,D);
SUB_CYCLE(8);


case 0x53:
BIT_N_R(0x04,E);
SUB_CYCLE(8);


case 0x54:
BIT_N_R(0x04,H);
SUB_CYCLE(8);


case 0x55:
BIT_N_R(0x04,L);
SUB_CYCLE(8);


case 0x56:
mem_read_fast(HL,a);
BIT_N_R(0x04,a);
SUB_CYCLE(12);


case 0x57:
BIT_N_R(0x04,A);
SUB_CYCLE(8);


case 0x58:
BIT_N_R(0x08,B);
SUB_CYCLE(8);


case 0x59:
BIT_N_R(0x08,C);
SUB_CYCLE(8);


case 0x5a:
BIT_N_R(0x08,D);
SUB_CYCLE(8);


case 0x5b:
BIT_N_R(0x08,E);
SUB_CYCLE(8);


case 0x5c:
BIT_N_R(0x08,H);
SUB_CYCLE(8);


case 0x5d:
BIT_N_R(0x08,L);
SUB_CYCLE(8);


case 0x5e:
mem_read_fast(HL,a);
BIT_N_R(0x08,a);
SUB_CYCLE(12);


case 0x5f:
BIT_N_R(0x08,A);
SUB_CYCLE(8);


case 0x60:
BIT_N_R(0x10,B);
SUB_CYCLE(8);


case 0x61:
BIT_N_R(0x10,C);
SUB_CYCLE(8);


case 0x62:
BIT_N_R(0x10,D);
SUB_CYCLE(8);


case 0x63:
BIT_N_R(0x10,E);
SUB_CYCLE(8);


case 0x64:
BIT_N_R(0x10,H);
SUB_CYCLE(8);


case 0x65:
BIT_N_R(0x10,L);
SUB_CYCLE(8);


case 0x66:
mem_read_fast(HL,a);
BIT_N_R(0x10,a);
SUB_CYCLE(12);


case 0x67:
BIT_N_R(0x10,A);
SUB_CYCLE(8);


case 0x68:
BIT_N_R(0x20,B);
SUB_CYCLE(8);


case 0x69:
BIT_N_R(0x20,C);
SUB_CYCLE(8);


case 0x6a:
BIT_N_R(0x20,D);
SUB_CYCLE(8);


case 0x6b:
BIT_N_R(0x20,E);
SUB_CYCLE(8);


case 0x6c:
BIT_N_R(0x20,H);
SUB_CYCLE(8);


case 0x6d:
BIT_N_R(0x20,L);
SUB_CYCLE(8);


case 0x6e:
mem_read_fast(HL,a);
BIT_N_R(0x20,a);
SUB_CYCLE(12);


case 0x6f:
BIT_N_R(0x20,A);
SUB_CYCLE(8);


case 0x70:
BIT_N_R(0x40,B);
SUB_CYCLE(8);


case 0x71:
BIT_N_R(0x40,C);
SUB_CYCLE(8);


case 0x72:
BIT_N_R(0x40,D);
SUB_CYCLE(8);


case 0x73:
BIT_N_R(0x40,E);
SUB_CYCLE(8);


case 0x74:
BIT_N_R(0x40,H);
SUB_CYCLE(8);


case 0x75:
BIT_N_R(0x40,L);
SUB_CYCLE(8);


case 0x76:
mem_read_fast(HL,a);
BIT_N_R(0x40,a);
SUB_CYCLE(12);


case 0x77:
BIT_N_R(0x40,A);
SUB_CYCLE(8);


case 0x78:
BIT_N_R(0x80,B);
SUB_CYCLE(8);


case 0x79:
BIT_N_R(0x80,C);
SUB_CYCLE(8);


case 0x7a:
BIT_N_R(0x80,D);
SUB_CYCLE(8);


case 0x7b:
BIT_N_R(0x80,E);
SUB_CYCLE(8);


case 0x7c:
BIT_N_R(0x80,H);
SUB_CYCLE(8);


case 0x7d:
BIT_N_R(0x80,L);
SUB_CYCLE(8);


case 0x7e:
mem_read_fast(HL,a);
BIT_N_R(0x80,a);
SUB_CYCLE(12);


case 0x7f:
BIT_N_R(0x80,A);
SUB_CYCLE(8);


case 0x80:
B&=0xfe;
SUB_CYCLE(8);


case 0x81:
C&=0xfe;
SUB_CYCLE(8);


case 0x82:
D&=0xfe;
SUB_CYCLE(8);


case 0x83:
E&=0xfe;
SUB_CYCLE(8);


case 0x84:
H&=0xfe;
SUB_CYCLE(8);


case 0x85:
L&=0xfe;
SUB_CYCLE(8);


case 0x86:
mem_read_fast(HL,v);
v&=0xfe;
mem_write_fast(HL,v);
SUB_CYCLE(16);


case 0x87:
A&=0xfe;
SUB_CYCLE(8);


case 0x88:
B&=0xfd;
SUB_CYCLE(8);


case 0x89:
C&=0xfd;
SUB_CYCLE(8);


case 0x8a:
D&=0xfd;
SUB_CYCLE(8);


case 0x8b:
E&=0xfd;
SUB_CYCLE(8);


case 0x8c:
H&=0xfd;
SUB_CYCLE(8);


case 0x8d:
L&=0xfd;
SUB_CYCLE(8);


case 0x8e:
mem_read_fast(HL,v);
v&=0xfd;
mem_write_fast(HL,v);
SUB_CYCLE(16);


case 0x8f:
A&=0xfd;
SUB_CYCLE(8);


case 0x90:
B&=0xfb;
SUB_CYCLE(8);


case 0x91:
C&=0xfb;
SUB_CYCLE(8);


case 0x92:
D&=0xfb;
SUB_CYCLE(8);


case 0x93:
E&=0xfb;
SUB_CYCLE(8);


case 0x94:
H&=0xfb;
SUB_CYCLE(8);


case 0x95:
L&=0xfb;
SUB_CYCLE(8);


case 0x96:
mem_read_fast(HL,v);
v&=0xfb;
mem_write_fast(HL,v);
SUB_CYCLE(16);


case 0x97:
A&=0xfb;
SUB_CYCLE(8);


case 0x98:
B&=0xf7;
SUB_CYCLE(8);


case 0x99:
C&=0xf7;
SUB_CYCLE(8);


case 0x9a:
D&=0xf7;
SUB_CYCLE(8);


case 0x9b:
E&=0xf7;
SUB_CYCLE(8);


case 0x9c:
H&=0xf7;
SUB_CYCLE(8);


case 0x9d:
L&=0xf7;
SUB_CYCLE(8);


case 0x9e:
mem_read_fast(HL,v);
v&=0xf7;
mem_write_fast(HL,v);
SUB_CYCLE(16);


case 0x9f:
A&=0xf7;
SUB_CYCLE(8);


case 0xa0:
B&=0xef;
SUB_CYCLE(8);


case 0xa1:
C&=0xef;
SUB_CYCLE(8);


case 0xa2:
D&=0xef;
SUB_CYCLE(8);


case 0xa3:
E&=0xef;
SUB_CYCLE(8);


case 0xa4:
H&=0xef;
SUB_CYCLE(8);


case 0xa5:
L&=0xef;
SUB_CYCLE(8);


case 0xa6:
mem_read_fast(HL,v);
v&=0xef;
mem_write_fast(HL,v);
SUB_CYCLE(16);


case 0xa7:
A&=0xef;
SUB_CYCLE(8);


case 0xa8:
B&=0xdf;
SUB_CYCLE(8);


case 0xa9:
C&=0xdf;
SUB_CYCLE(8);


case 0xaa:
D&=0xdf;
SUB_CYCLE(8);


case 0xab:
E&=0xdf;
SUB_CYCLE(8);


case 0xac:
H&=0xdf;
SUB_CYCLE(8);


case 0xad:
L&=0xdf;
SUB_CYCLE(8);


case 0xae:
mem_read_fast(HL,v);
v&=0xdf;
mem_write_fast(HL,v);
SUB_CYCLE(16);


case 0xaf:
A&=0xdf;
SUB_CYCLE(8);


case 0xb0:
B&=0xbf;
SUB_CYCLE(8);


case 0xb1:
C&=0xbf;
SUB_CYCLE(8);


case 0xb2:
D&=0xbf;
SUB_CYCLE(8);


case 0xb3:
E&=0xbf;
SUB_CYCLE(8);


case 0xb4:
H&=0xbf;
SUB_CYCLE(8);


case 0xb5:
L&=0xbf;
SUB_CYCLE(8);


case 0xb6:
mem_read_fast(HL,v);
v&=0xbf;
mem_write_fast(HL,v);
SUB_CYCLE(16);


case 0xb7:
A&=0xbf;
SUB_CYCLE(8);


case 0xb8:
B&=0x7f;
SUB_CYCLE(8);


case 0xb9:
C&=0x7f;
SUB_CYCLE(8);


case 0xba:
D&=0x7f;
SUB_CYCLE(8);


case 0xbb:
E&=0x7f;
SUB_CYCLE(8);


case 0xbc:
H&=0x7f;
SUB_CYCLE(8);


case 0xbd:
L&=0x7f;
SUB_CYCLE(8);


case 0xbe:
mem_read_fast(HL,v);
v&=0x7f;
mem_write_fast(HL,v);
SUB_CYCLE(16);


case 0xbf:
A&=0x7f;
SUB_CYCLE(8);


case 0xc0:
B|=0x01;
SUB_CYCLE(8);


case 0xc1:
C|=0x01;
SUB_CYCLE(8);


case 0xc2:
D|=0x01;
SUB_CYCLE(8);


case 0xc3:
E|=0x01;
SUB_CYCLE(8);


case 0xc4:
H|=0x01;
SUB_CYCLE(8);


case 0xc5:
L|=0x01;
SUB_CYCLE(8);


case 0xc6:
mem_read_fast(HL,v);
v|=0x01;
mem_write_fast(HL,v);
SUB_CYCLE(16);


case 0xc7:
A|=0x01;
SUB_CYCLE(8);


case 0xc8:
B|=0x02;
SUB_CYCLE(8);


case 0xc9:
C|=0x02;
SUB_CYCLE(8);


case 0xca:
D|=0x02;
SUB_CYCLE(8);


case 0xcb:
E|=0x02;
SUB_CYCLE(8);


case 0xcc:
H|=0x02;
SUB_CYCLE(8);


case 0xcd:
L|=0x02;
SUB_CYCLE(8);


case 0xce:
mem_read_fast(HL,v);
v|=0x02;
mem_write_fast(HL,v);
SUB_CYCLE(16);


case 0xcf:
A|=0x02;
SUB_CYCLE(8);


case 0xd0:
B|=0x04;
SUB_CYCLE(8);


case 0xd1:
C|=0x04;
SUB_CYCLE(8);


case 0xd2:
D|=0x04;
SUB_CYCLE(8);


case 0xd3:
E|=0x04;
SUB_CYCLE(8);


case 0xd4:
H|=0x04;
SUB_CYCLE(8);


case 0xd5:
L|=0x04;
SUB_CYCLE(8);


case 0xd6:
mem_read_fast(HL,v);
v|=0x04;
mem_write_fast(HL,v);
SUB_CYCLE(16);


case 0xd7:
A|=0x04;
SUB_CYCLE(8);


case 0xd8:
B|=0x08;
SUB_CYCLE(8);


case 0xd9:
C|=0x08;
SUB_CYCLE(8);


case 0xda:
D|=0x08;
SUB_CYCLE(8);


case 0xdb:
E|=0x08;
SUB_CYCLE(8);


case 0xdc:
H|=0x08;
SUB_CYCLE(8);


case 0xdd:
L|=0x08;
SUB_CYCLE(8);


case 0xde:
mem_read_fast(HL,v);
v|=0x08;
mem_write_fast(HL,v);
SUB_CYCLE(16);


case 0xdf:
A|=0x08;
SUB_CYCLE(8);


case 0xe0:
B|=0x10;
SUB_CYCLE(8);


case 0xe1:
C|=0x10;
SUB_CYCLE(8);


case 0xe2:
D|=0x10;
SUB_CYCLE(8);


case 0xe3:
E|=0x10;
SUB_CYCLE(8);


case 0xe4:
H|=0x10;
SUB_CYCLE(8);


case 0xe5:
L|=0x10;
SUB_CYCLE(8);


case 0xe6:
mem_read_fast(HL,v);
v|=0x10;
mem_write_fast(HL,v);
SUB_CYCLE(16);


case 0xe7:
A|=0x10;
SUB_CYCLE(8);


case 0xe8:
B|=0x20;
SUB_CYCLE(8);


case 0xe9:
C|=0x20;
SUB_CYCLE(8);


case 0xea:
D|=0x20;
SUB_CYCLE(8);


case 0xeb:
E|=0x20;
SUB_CYCLE(8);


case 0xec:
H|=0x20;
SUB_CYCLE(8);


case 0xed:
L|=0x20;
SUB_CYCLE(8);


case 0xee:
mem_read_fast(HL,v);
v|=0x20;
mem_write_fast(HL,v);
SUB_CYCLE(16);


case 0xef:
A|=0x20;
SUB_CYCLE(8);


case 0xf0:
B|=0x40;
SUB_CYCLE(8);


case 0xf1:
C|=0x40;
SUB_CYCLE(8);


case 0xf2:
D|=0x40;
SUB_CYCLE(8);


case 0xf3:
E|=0x40;
SUB_CYCLE(8);


case 0xf4:
H|=0x40;
SUB_CYCLE(8);


case 0xf5:
L|=0x40;
SUB_CYCLE(8);


case 0xf6:
mem_read_fast(HL,v);
v|=0x40;
mem_write_fast(HL,v);
SUB_CYCLE(16);


case 0xf7:
A|=0x40;
SUB_CYCLE(8);


case 0xf8:
B|=0x80;
SUB_CYCLE(8);


case 0xf9:
C|=0x80;
SUB_CYCLE(8);


case 0xfa:
D|=0x80;
SUB_CYCLE(8);


case 0xfb:
E|=0x80;
SUB_CYCLE(8);


case 0xfc:
H|=0x80;
SUB_CYCLE(8);


case 0xfd:
L|=0x80;
SUB_CYCLE(8);


case 0xfe:
mem_read_fast(HL,v);
v|=0x80;
mem_write_fast(HL,v);
SUB_CYCLE(16);


  case 0xff:
	  A|=0x80;
	  SUB_CYCLE(8);

	  }
	case 0xcc: 
	  if (IS_SET(FLAG_Z)) {
		  t16=GET_WORD;
		  PUSH_R(REG_PC);
		  PC=t16;
		  SUB_CYCLE(24);
	  } else {
		  PC+=2;
		  SUB_CYCLE(12);
	  }
  case 0xcd:
	  t16=GET_WORD;
	  PUSH_R(REG_PC);
	  PC=t16;
	  SUB_CYCLE(24);
  case 0xce: 
	  t16=GET_BYTE+(IS_SET(FLAG_C)?(1):(0));
	  t32=A+t16;
	  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  A=t32&0xff;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  UNSET_FLAG(FLAG_NN);
	  SUB_CYCLE(8);
  case 0xcf: 
	  PUSH_R(REG_PC);
	  PC=0x0008;
	  SUB_CYCLE(16);

  case 0xd0:
	  if (IS_SET(FLAG_C)) SUB_CYCLE(8);
	  else {
		  POP_R(REG_PC);
		  SUB_CYCLE(20);
	  }
  case 0xd1:
	  POP_R(REG_DE);
	  SUB_CYCLE(12);
  case 0xd2:
	  if (IS_SET(FLAG_C)) {
		  PC+=2;
		  SUB_CYCLE(12);
	  } else {
		  PC=GET_WORD;
		  SUB_CYCLE(16);
	  }
  case 0xd3: return unknown();
  case 0xd4:
	  if (IS_SET(FLAG_C)) {
		  PC+=2;
		  SUB_CYCLE(12);
	  } else {
		  t16=GET_WORD;
		  PUSH_R(REG_PC);
		  PC=t16;
		  SUB_CYCLE(24);
	  }
  case 0xd5:
	  PUSH_R(REG_DE);
	  SUB_CYCLE(16);
  case 0xd6:
	  t8=GET_BYTE;
	  ((t8>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((t8&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=t8;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(8);
  case 0xd7:
	  PUSH_R(REG_PC);
	  PC=0x0010;
	  SUB_CYCLE(16);
  case 0xd8:
	  if (IS_SET(FLAG_C)) {
		  POP_R(REG_PC);
		  SUB_CYCLE(20);
	  }else SUB_CYCLE(8);
  case 0xd9:
	  POP_R(REG_PC);
	  gbcpu->int_flag=1;
	  SUB_CYCLE(16);
  case 0xda:
	  if (IS_SET(FLAG_C)) {
		  PC=GET_WORD;
		  SUB_CYCLE(16);
	  }else {
		  PC+=2;
		  SUB_CYCLE(12);
	  }
  case 0xdb: return unknown();
  case 0xdc:
	  if (IS_SET(FLAG_C)) {
		  t16=GET_WORD;
		  PUSH_R(REG_PC);
		  PC=t16;
		  SUB_CYCLE(24);
	  } else {
		  PC+=2;
		  SUB_CYCLE(12);
	  }
  case 0xdd: return unknown();
  case 0xde:
	  t16=GET_BYTE+((IS_SET(FLAG_C))?(1):(0));
	  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  A-=t16;
	  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
	  SET_FLAG(FLAG_N);
	  SUB_CYCLE(8);
  case 0xdf:
	  PUSH_R(REG_PC);
	  PC=0x0018;
	  SUB_CYCLE(16);
	case 0xe0: 
	  mem_write_ff(0xff00+GET_BYTE,A);
	  SUB_CYCLE(12);
  case 0xe1: 
	  POP_R(REG_HL);
	  SUB_CYCLE(12);
  case 0xe2: 
	  mem_write_ff(0xff00+C,A);
	  SUB_CYCLE(8);
  case 0xe3: return unknown();
  case 0xe4: return unknown();
  case 0xe5: 
	  PUSH_R(REG_HL);
	  SUB_CYCLE(16);
  case 0xe6: 
	  A&=GET_BYTE;
	  if (A) {
		  UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_H);
	  } else {
		  UNSET_FLAG(FLAG_NN&FLAG_NC);
		  SET_FLAG(FLAG_Z|FLAG_H);
	  }
	  SUB_CYCLE(8);
  case 0xe7: 
	  PUSH_R(REG_PC);
	  PC=0x0020;
	  SUB_CYCLE(16);
  case 0xe8: 
	  st8=GET_BYTE;
	  t32=SP+st8;
	  ((t32&0x00010000)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  ((((SP&0x0f)+(st8&0x0f))&0x10)?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
	  SP=t32&0xffff;
	  UNSET_FLAG(FLAG_NN&FLAG_NZ);
	  SUB_CYCLE(16);
  case 0xe9: 
	  PC=HL;
	  SUB_CYCLE(4);
  case 0xea: 
	  t16=GET_WORD;
	  mem_write_fast(t16,A);
	  SUB_CYCLE(16);
  case 0xeb: return unknown();
  case 0xec: return unknown();
  case 0xed: return unknown();
  case 0xee: 
	  A^=GET_BYTE;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(8);
  case 0xef: 
	  PUSH_R(REG_PC);
	  PC=0x0028;
	  SUB_CYCLE(16);
  case 0xf0: 
	  A=mem_read_ff(0xff00+GET_BYTE);
	  SUB_CYCLE(12);
  case 0xf1:
	  POP_R(REG_AF);
	  SUB_CYCLE(12);
  case 0xf2: 
	  A=mem_read_ff(0xff00+C);
	  SUB_CYCLE(8);
  case 0xf3: 
	  DI;
	  SUB_CYCLE(4);
  case 0xf4: return unknown();
  case 0xf5: 
	  PUSH_R(REG_AF);
	  SUB_CYCLE(16);
  case 0xf6: 
	  A|=GET_BYTE;
	  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
	  if (A) UNSET_FLAG(FLAG_NZ);
	  else SET_FLAG(FLAG_Z);
	  SUB_CYCLE(8);
  case 0xf7: 
	  PUSH_R(REG_PC);
	  PC=0x0030;
	  SUB_CYCLE(16);
  case 0xf8: 
	  st8=GET_BYTE;
	  t32=SP+st8;
	  ((t32&0x00010000)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
	  (((SP&0x0f)+(st8&0x0f))>0x0f) ?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
	  HL=t32&0xffff;
	  UNSET_FLAG(FLAG_NN&FLAG_NZ);
	  SUB_CYCLE(12);
  case 0xf9: 
	  SP=HL;
	  SUB_CYCLE(8);
  case 0xfa: 
	  t16=GET_WORD;
	  mem_read_fast(t16,A);
	  SUB_CYCLE(16);
  case 0xfb: 
	  EI;
	  SUB_CYCLE(4);
  case 0xfc: return unknown();
  case 0xfd: return unknown();
  case 0xfe: 
	  t8=GET_BYTE;
	  CP_A_R(t8);
	  SUB_CYCLE(8);
  case 0xff: 
	  PUSH_R(REG_PC);
	  PC=0x0038;
	  SUB_CYCLE(16);
  }
  return 0;
}

__inline__ void rom_timer_inc(void) {
  rom_timer->reg[0]=(rom_timer->reg[0]+1)%60;
  if (!rom_timer->reg[0]) {
    rom_timer->reg[1]=(rom_timer->reg[1]+1)%60;
    if (!rom_timer->reg[1]) {
      rom_timer->reg[2]=(rom_timer->reg[2]+1)%24;
      if (!rom_timer->reg[2]) {
	rom_timer->reg[3]++;
	if (rom_timer->reg[3]) {
	  if (rom_timer->reg[4]&0x01)
	    rom_timer->reg[4]|=0x80; // set carry
	  else rom_timer->reg[4]|=0x01; // set dayh bit 
	}
      }
    }
  }
}

__inline__ void cpu_run(void) {
  static Uint32 divid_cycle;
  int v=0;
  Uint8 a;

  do {
    
    v=0;

    /*FIXME: GDMA and interrupt */
    if (dma_info.type!=GDMA) {
	    if (INT_FLAG&VBLANK_INT)  {v=make_interrupt(VBLANK_INT);}
	    if ((INT_FLAG&LCDC_INT) && (!v)) {v=make_interrupt(LCDC_INT);}
	    if ((INT_FLAG&TIMEOWFL_INT) && (!v)) {v=make_interrupt(TIMEOWFL_INT); }
	    if ((INT_FLAG&SERIAL_INT) && (!v)) {v=make_interrupt(SERIAL_INT); }
    }

    //if (v) a+=24;
    
    if (dma_info.type==GDMA) {
      /* FIXME */
      dma_info.gdma_cycle-=4;
      if (dma_info.gdma_cycle<=0) {
	dma_info.type=NO_DMA;
	HDMA_CTRL5=0xff;
      }
      a=4;
    } else {
      /*gblcdc_addcycle(gdma_cycle);
	}*/
	    if (gbcpu->state!=HALT_STATE) {
		    a=gbcpu_exec_one();
	    }
	    else a=4;
	    
    }
    
    //if (v) a+=24;

    //if (gbcpu->state==HALT_STATE) halt_update();
 
    nb_cycle+=a;
    
    divid_cycle+=a;
    if (divid_cycle>256) {
      DIVID++;
      divid_cycle=0;
    }
    
    if (LCDCCONT&0x80) GBLCDC_ADD_CYCLE(a)
    else {
      key_cycle+=a;
      if (key_cycle>=gblcdc->vblank_cycle) {
	update_key();
	key_cycle=0;
      }
    }
       
    if (TIME_CONTROL&0x04) {
      gbtimer->cycle+=a;
      if (gbtimer->cycle>=gbtimer->clk_inc) {
	gbtimer_update();
	gbtimer->cycle-=gbtimer->clk_inc;
      }
    }

    /* FIXME: MBC3 timing */
    if (rom_type&TIMER && rom_timer->reg[4]&0x40) {
      rom_timer->cycle+=a;
      if (rom_timer->cycle>111) {
	rom_timer_inc();
	rom_timer->cycle=0;
      }
    }

    /* FIXME: serial is experimentale */
    if (conf.serial_on) {
      // if (gbserial_check()) {
      if (gbserial.ready2read) {
	printf("Receive data\n");
	
	  if (SC&0x80) {	/* Transfert is on the way */

	    if (SC&0x01) {	/* Server */
		    SB=gbserial_read();
		    set_interrupt(SERIAL_INT);
		    printf("Server read %02x make int\n",SB);
		    SC&=0x7f;
		    serial_cycle_todo=0;
	    } else {		/* Client */
		    printf("Client write %02x make int\n",SB);
		    gbserial_write(SB);
		    SB=gbserial_read();
		    printf("Client read %02x make int\n",SB);
		    /* TODO: Make interrupt in n cycle */
	      SC&=0x7f;
	      set_interrupt(SERIAL_INT);
	      serial_cycle_todo=0;
	    }

	  } else {
	    gbserial_read();
	    gbserial.ready2read=0;
	  }
	  
	  gbserial.wait=0;
      } 
	
	
      if (serial_cycle_todo>0) {
	serial_cycle_todo-=a;
	if (serial_cycle_todo<=0) {
	  serial_cycle_todo=0;
	  if (SC&0x80) {
	    SB=0xFF;
	    set_interrupt(SERIAL_INT);
	  }
	}
      }
    } /* End of serial Update */



  }
  while(!conf.gb_done);
}

#undef REG_AF
#undef REG_BC
#undef REG_DE
#undef REG_HL
#undef REG_PC
#undef AF
#undef BC
#undef DE
#undef HL
#undef PC
#undef SP
#undef A
#undef F
#undef B
#undef C
#undef D
#undef E
#undef H
#undef L
