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

#ifdef DEBUG
#include "gngb_debuger/log.h"
#include "gngb_debuger/debuger.h"
#endif

GB_CPU *gbcpu=0;

//extern INT32 gdma_cycle;

static UINT8 t8;
static INT8 st8;
static UINT16 t16;
static INT16 st16;
static UINT32 t32;

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

inline UINT16 get_word(void)
{
  UINT16 v=((UINT16)(mem_read(PC))|(UINT16)((mem_read(PC+1)<<8)));
  PC+=2;
  return v;
}

inline UINT8 get_byte(void)
{
  return mem_read(PC++);
}

inline void push_r(REG *r)
{
  mem_write(--SP,(r)->b.h);
  mem_write(--SP,(r)->b.l);
}

inline void pop_r(REG *r)
{
  (r)->b.l=mem_read(SP++);
  (r)->b.h=mem_read(SP++);
} 

#define GET_BYTE get_byte()
#define GET_WORD get_word()

#define SET_FLAG(f) ((gbcpu->af.b.l)|=(f))
#define UNSET_FLAG(f) ((gbcpu->af.b.l)&=(f))
#define IS_SET(f) ((gbcpu->af.b.l&(f)))

#define SUB_CYCLE(a) return (a)

#define PUSH_R(r) (push_r(&r))
#define POP_R(r) (pop_r(&r))

#define EI (gbcpu->ei_flag=1)
#define DI (gbcpu->int_flag=0)

inline UINT8 unknown(void);
inline UINT8 nop(void);
inline UINT8 ld_bc_nn(void);
inline UINT8 ld_mem_bc_a(void);
inline UINT8 inc_bc(void);
inline UINT8 inc_b(void);
inline UINT8 dec_b(void);
inline UINT8 ld_b_n(void);
inline UINT8 rlca(void);
inline UINT8 ld_mem_nn_sp(void);
inline UINT8 add_hl_bc(void);
inline UINT8 ld_a_mem_bc(void);
inline UINT8 dec_bc(void);
inline UINT8 inc_c(void);
inline UINT8 dec_c(void);
inline UINT8 ld_c_n(void);
inline UINT8 rrca(void);
inline UINT8 stop(void);
inline UINT8 ld_de_nn(void);
inline UINT8 ld_mem_de_a(void);
inline UINT8 inc_de(void);
inline UINT8 inc_d(void);
inline UINT8 dec_d(void);
inline UINT8 ld_d_n(void);
inline UINT8 rla(void);
inline UINT8 jr_disp(void);
inline UINT8 add_hl_de(void);
inline UINT8 ld_a_mem_de(void);
inline UINT8 dec_de(void);
inline UINT8 inc_e(void);
inline UINT8 dec_e(void);
inline UINT8 ld_e_n(void);
inline UINT8 rra(void);
inline UINT8 jr_nz_disp(void);
inline UINT8 ld_hl_nn(void);
inline UINT8 ldi_mem_hl_a(void);
inline UINT8 inc_hl(void);
inline UINT8 inc_h(void);
inline UINT8 dec_h(void);
inline UINT8 ld_h_n(void);
inline UINT8 daa(void);
inline UINT8 jr_z_disp(void);
inline UINT8 add_hl_hl(void);
inline UINT8 ldi_a_mem_hl(void);
inline UINT8 dec_hl(void);
inline UINT8 inc_l(void);
inline UINT8 dec_l(void);
inline UINT8 ld_l_n(void);
inline UINT8 cpl(void);
inline UINT8 jr_nc_disp(void);
inline UINT8 ld_sp_nn(void);
inline UINT8 ldd_mem_hl_a(void);
inline UINT8 inc_sp(void);
inline UINT8 inc_mem_hl(void);
inline UINT8 dec_mem_hl(void);
inline UINT8 ld_mem_hl_n(void);
inline UINT8 scf(void);
inline UINT8 jr_c_disp(void);
inline UINT8 add_hl_sp(void);
inline UINT8 ldd_a_mem_hl(void);
inline UINT8 dec_sp(void);
inline UINT8 inc_a(void);
inline UINT8 dec_a(void);
inline UINT8 ld_a_n(void);
inline UINT8 ccf(void);
inline UINT8 ld_b_b(void);
inline UINT8 ld_b_c(void);
inline UINT8 ld_b_d(void);
inline UINT8 ld_b_e(void);
inline UINT8 ld_b_h(void);
inline UINT8 ld_b_l(void);
inline UINT8 ld_b_mem_hl(void);
inline UINT8 ld_b_a(void);
inline UINT8 ld_c_b(void);
inline UINT8 ld_c_c(void);
inline UINT8 ld_c_d(void);
inline UINT8 ld_c_e(void);
inline UINT8 ld_c_h(void);
inline UINT8 ld_c_l(void);
inline UINT8 ld_c_mem_hl(void);
inline UINT8 ld_c_a(void);
inline UINT8 ld_d_b(void);
inline UINT8 ld_d_c(void);
inline UINT8 ld_d_d(void);
inline UINT8 ld_d_e(void);
inline UINT8 ld_d_h(void);
inline UINT8 ld_d_l(void);
inline UINT8 ld_d_mem_hl(void);
inline UINT8 ld_d_a(void);
inline UINT8 ld_e_b(void);
inline UINT8 ld_e_c(void);
inline UINT8 ld_e_d(void);
inline UINT8 ld_e_e(void);
inline UINT8 ld_e_h(void);
inline UINT8 ld_e_l(void);
inline UINT8 ld_e_mem_hl(void);
inline UINT8 ld_e_a(void);
inline UINT8 ld_h_b(void);
inline UINT8 ld_h_c(void);
inline UINT8 ld_h_d(void);
inline UINT8 ld_h_e(void);
inline UINT8 ld_h_h(void);
inline UINT8 ld_h_l(void);
inline UINT8 ld_h_mem_hl(void);
inline UINT8 ld_h_a(void);
inline UINT8 ld_l_b(void);
inline UINT8 ld_l_c(void);
inline UINT8 ld_l_d(void);
inline UINT8 ld_l_e(void);
inline UINT8 ld_l_h(void);
inline UINT8 ld_l_l(void);
inline UINT8 ld_l_mem_hl(void);
inline UINT8 ld_l_a(void);
inline UINT8 ld_mem_hl_b(void);
inline UINT8 ld_mem_hl_c(void);
inline UINT8 ld_mem_hl_d(void);
inline UINT8 ld_mem_hl_e(void);
inline UINT8 ld_mem_hl_h(void);
inline UINT8 ld_mem_hl_l(void);
inline UINT8 halt(void);
inline UINT8 ld_mem_hl_a(void);
inline UINT8 ld_a_b(void);
inline UINT8 ld_a_c(void);
inline UINT8 ld_a_d(void);
inline UINT8 ld_a_e(void);
inline UINT8 ld_a_h(void);
inline UINT8 ld_a_l(void);
inline UINT8 ld_a_mem_hl(void);
inline UINT8 ld_a_a(void);
inline UINT8 add_a_b(void);
inline UINT8 add_a_c(void);
inline UINT8 add_a_d(void);
inline UINT8 add_a_e(void);
inline UINT8 add_a_h(void);
inline UINT8 add_a_l(void);
inline UINT8 add_a_mem_hl(void);
inline UINT8 add_a_a(void);
inline UINT8 adc_a_b(void);
inline UINT8 adc_a_c(void);
inline UINT8 adc_a_d(void);
inline UINT8 adc_a_e(void);
inline UINT8 adc_a_h(void);
inline UINT8 adc_a_l(void);
inline UINT8 adc_a_mem_hl(void);
inline UINT8 adc_a_a(void);
inline UINT8 sub_b(void);
inline UINT8 sub_c(void);
inline UINT8 sub_d(void);
inline UINT8 sub_e(void);
inline UINT8 sub_h(void);
inline UINT8 sub_l(void);
inline UINT8 sub_mem_hl(void);
inline UINT8 sub_a(void);
inline UINT8 sbc_a_b(void);
inline UINT8 sbc_a_c(void);
inline UINT8 sbc_a_d(void);
inline UINT8 sbc_a_e(void);
inline UINT8 sbc_a_h(void);
inline UINT8 sbc_a_l(void);
inline UINT8 sbc_a_mem_hl(void);
inline UINT8 sbc_a_a(void);
inline UINT8 and_b(void);
inline UINT8 and_c(void);
inline UINT8 and_d(void);
inline UINT8 and_e(void);
inline UINT8 and_h(void);
inline UINT8 and_l(void);
inline UINT8 and_mem_hl(void);
inline UINT8 and_a(void);
inline UINT8 xor_b(void);
inline UINT8 xor_c(void);
inline UINT8 xor_d(void);
inline UINT8 xor_e(void);
inline UINT8 xor_h(void);
inline UINT8 xor_l(void);
inline UINT8 xor_mem_hl(void);
inline UINT8 xor_a(void);
inline UINT8 or_b(void);
inline UINT8 or_c(void);
inline UINT8 or_d(void);
inline UINT8 or_e(void);
inline UINT8 or_h(void);
inline UINT8 or_l(void);
inline UINT8 or_mem_hl(void);
inline UINT8 or_a(void);
inline UINT8 cp_b(void);
inline UINT8 cp_c(void);
inline UINT8 cp_d(void);
inline UINT8 cp_e(void);
inline UINT8 cp_h(void);
inline UINT8 cp_l(void);
inline UINT8 cp_mem_hl(void);
inline UINT8 cp_a(void);
inline UINT8 ret_nz(void);
inline UINT8 pop_bc(void);
inline UINT8 jp_nz_nn(void);
inline UINT8 jp_nn(void);
inline UINT8 call_nz_nn(void);
inline UINT8 push_bc(void);
inline UINT8 add_a_n(void);
inline UINT8 rst_00h(void);
inline UINT8 ret_z(void);
inline UINT8 ret(void);
inline UINT8 jp_z_nn(void);
inline UINT8 cb_inst(void);
inline UINT8 call_z_nn(void);
inline UINT8 call_nn(void);
inline UINT8 adc_a_n(void);
inline UINT8 rst_8h(void);
inline UINT8 ret_nc(void);
inline UINT8 pop_de(void);
inline UINT8 jp_nc_nn(void);
inline UINT8 call_nc_nn(void);
inline UINT8 push_de(void);
inline UINT8 sub_n(void);
inline UINT8 rst_10h(void);
inline UINT8 ret_c(void);
inline UINT8 reti(void);
inline UINT8 jp_c_nn(void);
inline UINT8 call_c_nn(void);
inline UINT8 sbc_a_n(void);
inline UINT8 rst_18h(void);
inline UINT8 ld_mem_ff00_n_a(void);
inline UINT8 pop_hl(void);
inline UINT8 ld_mem_ff00_c_a(void);
inline UINT8 push_hl(void);
inline UINT8 and_n(void);
inline UINT8 rst_20h(void);
inline UINT8 add_sp_dd(void);
inline UINT8 jp_mem_hl(void);
inline UINT8 ld_mem_nn_a(void);
inline UINT8 xor_n(void);
inline UINT8 rst_28h(void);
inline UINT8 ld_a_mem_ff00_n(void);
inline UINT8 pop_af(void);
inline UINT8 ld_a_mem_c(void);
inline UINT8 di(void);
inline UINT8 push_af(void);
inline UINT8 or_n(void);
inline UINT8 rst_30h(void);
inline UINT8 ld_hl_sp_dd(void);
inline UINT8 ld_sp_hl(void);
inline UINT8 ld_a_mem_nn(void);
inline UINT8 ei(void);
inline UINT8 cp_n(void);
inline UINT8 rst_38h(void);

/* CB instruction */

inline UINT8 rlc_b(void);
inline UINT8 rlc_c(void);
inline UINT8 rlc_d(void);
inline UINT8 rlc_e(void);
inline UINT8 rlc_h(void);
inline UINT8 rlc_l(void);
inline UINT8 rlc_mem_hl(void);
inline UINT8 rlc_a(void);
inline UINT8 rrc_b(void);
inline UINT8 rrc_c(void);
inline UINT8 rrc_d(void);
inline UINT8 rrc_e(void);
inline UINT8 rrc_h(void);
inline UINT8 rrc_l(void);
inline UINT8 rrc_mem_hl(void);
inline UINT8 rrc_a(void);
inline UINT8 rl_b(void);
inline UINT8 rl_c(void);
inline UINT8 rl_d(void);
inline UINT8 rl_e(void);
inline UINT8 rl_h(void);
inline UINT8 rl_l(void);
inline UINT8 rl_mem_hl(void);
inline UINT8 rl_a(void);
inline UINT8 rr_b(void);
inline UINT8 rr_c(void);
inline UINT8 rr_d(void);
inline UINT8 rr_e(void);
inline UINT8 rr_h(void);
inline UINT8 rr_l(void);
inline UINT8 rr_mem_hl(void);
inline UINT8 rr_a(void);
inline UINT8 sla_b(void);
inline UINT8 sla_c(void);
inline UINT8 sla_d(void);
inline UINT8 sla_e(void);
inline UINT8 sla_h(void);
inline UINT8 sla_l(void);
inline UINT8 sla_mem_hl(void);
inline UINT8 sla_a(void);
inline UINT8 sra_b(void);
inline UINT8 sra_c(void);
inline UINT8 sra_d(void);
inline UINT8 sra_e(void);
inline UINT8 sra_h(void);
inline UINT8 sra_l(void);
inline UINT8 sra_mem_hl(void);
inline UINT8 sra_a(void);
inline UINT8 swap_b(void);
inline UINT8 swap_c(void);
inline UINT8 swap_d(void);
inline UINT8 swap_e(void);
inline UINT8 swap_h(void);
inline UINT8 swap_l(void);
inline UINT8 swap_mem_hl(void);
inline UINT8 swap_a(void);
inline UINT8 srl_b(void);
inline UINT8 srl_c(void);
inline UINT8 srl_d(void);
inline UINT8 srl_e(void);
inline UINT8 srl_h(void);
inline UINT8 srl_l(void);
inline UINT8 srl_mem_hl(void);
inline UINT8 srl_a(void);
inline UINT8 bit_0_b(void);
inline UINT8 bit_0_c(void);
inline UINT8 bit_0_d(void);
inline UINT8 bit_0_e(void);
inline UINT8 bit_0_h(void);
inline UINT8 bit_0_l(void);
inline UINT8 bit_0_mem_hl(void);
inline UINT8 bit_0_a(void);
inline UINT8 bit_1_b(void);
inline UINT8 bit_1_c(void);
inline UINT8 bit_1_d(void);
inline UINT8 bit_1_e(void);
inline UINT8 bit_1_h(void);
inline UINT8 bit_1_l(void);
inline UINT8 bit_1_mem_hl(void);
inline UINT8 bit_1_a(void);
inline UINT8 bit_2_b(void);
inline UINT8 bit_2_c(void);
inline UINT8 bit_2_d(void);
inline UINT8 bit_2_e(void);
inline UINT8 bit_2_h(void);
inline UINT8 bit_2_l(void);
inline UINT8 bit_2_mem_hl(void);
inline UINT8 bit_2_a(void);
inline UINT8 bit_3_b(void);
inline UINT8 bit_3_c(void);
inline UINT8 bit_3_d(void);
inline UINT8 bit_3_e(void);
inline UINT8 bit_3_h(void);
inline UINT8 bit_3_l(void);
inline UINT8 bit_3_mem_hl(void);
inline UINT8 bit_3_a(void);
inline UINT8 bit_4_b(void);
inline UINT8 bit_4_c(void);
inline UINT8 bit_4_d(void);
inline UINT8 bit_4_e(void);
inline UINT8 bit_4_h(void);
inline UINT8 bit_4_l(void);
inline UINT8 bit_4_mem_hl(void);
inline UINT8 bit_4_a(void);
inline UINT8 bit_5_b(void);
inline UINT8 bit_5_c(void);
inline UINT8 bit_5_d(void);
inline UINT8 bit_5_e(void);
inline UINT8 bit_5_h(void);
inline UINT8 bit_5_l(void);
inline UINT8 bit_5_mem_hl(void);
inline UINT8 bit_5_a(void);
inline UINT8 bit_6_b(void);
inline UINT8 bit_6_c(void);
inline UINT8 bit_6_d(void);
inline UINT8 bit_6_e(void);
inline UINT8 bit_6_h(void);
inline UINT8 bit_6_l(void);
inline UINT8 bit_6_mem_hl(void);
inline UINT8 bit_6_a(void);
inline UINT8 bit_7_b(void);
inline UINT8 bit_7_c(void);
inline UINT8 bit_7_d(void);
inline UINT8 bit_7_e(void);
inline UINT8 bit_7_h(void);
inline UINT8 bit_7_l(void);
inline UINT8 bit_7_mem_hl(void);
inline UINT8 bit_7_a(void);
inline UINT8 res_0_b(void);
inline UINT8 res_0_c(void);
inline UINT8 res_0_d(void);
inline UINT8 res_0_e(void);
inline UINT8 res_0_h(void);
inline UINT8 res_0_l(void);
inline UINT8 res_0_mem_hl(void);
inline UINT8 res_0_a(void);
inline UINT8 res_1_b(void);
inline UINT8 res_1_c(void);
inline UINT8 res_1_d(void);
inline UINT8 res_1_e(void);
inline UINT8 res_1_h(void);
inline UINT8 res_1_l(void);
inline UINT8 res_1_mem_hl(void);
inline UINT8 res_1_a(void);
inline UINT8 res_2_b(void);
inline UINT8 res_2_c(void);
inline UINT8 res_2_d(void);
inline UINT8 res_2_e(void);
inline UINT8 res_2_h(void);
inline UINT8 res_2_l(void);
inline UINT8 res_2_mem_hl(void);
inline UINT8 res_2_a(void);
inline UINT8 res_3_b(void);
inline UINT8 res_3_c(void);
inline UINT8 res_3_d(void);
inline UINT8 res_3_e(void);
inline UINT8 res_3_h(void);
inline UINT8 res_3_l(void);
inline UINT8 res_3_mem_hl(void);
inline UINT8 res_3_a(void);
inline UINT8 res_4_b(void);
inline UINT8 res_4_c(void);
inline UINT8 res_4_d(void);
inline UINT8 res_4_e(void);
inline UINT8 res_4_h(void);
inline UINT8 res_4_l(void);
inline UINT8 res_4_mem_hl(void);
inline UINT8 res_4_a(void);
inline UINT8 res_5_b(void);
inline UINT8 res_5_c(void);
inline UINT8 res_5_d(void);
inline UINT8 res_5_e(void);
inline UINT8 res_5_h(void);
inline UINT8 res_5_l(void);
inline UINT8 res_5_mem_hl(void);
inline UINT8 res_5_a(void);
inline UINT8 res_6_b(void);
inline UINT8 res_6_c(void);
inline UINT8 res_6_d(void);
inline UINT8 res_6_e(void);
inline UINT8 res_6_h(void);
inline UINT8 res_6_l(void);
inline UINT8 res_6_mem_hl(void);
inline UINT8 res_6_a(void);
inline UINT8 res_7_b(void);
inline UINT8 res_7_c(void);
inline UINT8 res_7_d(void);
inline UINT8 res_7_e(void);
inline UINT8 res_7_h(void);
inline UINT8 res_7_l(void);
inline UINT8 res_7_mem_hl(void);
inline UINT8 res_7_a(void);
inline UINT8 set_0_b(void);
inline UINT8 set_0_c(void);
inline UINT8 set_0_d(void);
inline UINT8 set_0_e(void);
inline UINT8 set_0_h(void);
inline UINT8 set_0_l(void);
inline UINT8 set_0_mem_hl(void);
inline UINT8 set_0_a(void);
inline UINT8 set_1_b(void);
inline UINT8 set_1_c(void);
inline UINT8 set_1_d(void);
inline UINT8 set_1_e(void);
inline UINT8 set_1_h(void);
inline UINT8 set_1_l(void);
inline UINT8 set_1_mem_hl(void);
inline UINT8 set_1_a(void);
inline UINT8 set_2_b(void);
inline UINT8 set_2_c(void);
inline UINT8 set_2_d(void);
inline UINT8 set_2_e(void);
inline UINT8 set_2_h(void);
inline UINT8 set_2_l(void);
inline UINT8 set_2_mem_hl(void);
inline UINT8 set_2_a(void);
inline UINT8 set_3_b(void);
inline UINT8 set_3_c(void);
inline UINT8 set_3_d(void);
inline UINT8 set_3_e(void);
inline UINT8 set_3_h(void);
inline UINT8 set_3_l(void);
inline UINT8 set_3_mem_hl(void);
inline UINT8 set_3_a(void);
inline UINT8 set_4_b(void);
inline UINT8 set_4_c(void);
inline UINT8 set_4_d(void);
inline UINT8 set_4_e(void);
inline UINT8 set_4_h(void);
inline UINT8 set_4_l(void);
inline UINT8 set_4_mem_hl(void);
inline UINT8 set_4_a(void);
inline UINT8 set_5_b(void);
inline UINT8 set_5_c(void);
inline UINT8 set_5_d(void);
inline UINT8 set_5_e(void);
inline UINT8 set_5_h(void);
inline UINT8 set_5_l(void);
inline UINT8 set_5_mem_hl(void);
inline UINT8 set_5_a(void);
inline UINT8 set_6_b(void);
inline UINT8 set_6_c(void);
inline UINT8 set_6_d(void);
inline UINT8 set_6_e(void);
inline UINT8 set_6_h(void);
inline UINT8 set_6_l(void);
inline UINT8 set_6_mem_hl(void);
inline UINT8 set_6_a(void);
inline UINT8 set_7_b(void);
inline UINT8 set_7_c(void);
inline UINT8 set_7_d(void);
inline UINT8 set_7_e(void);
inline UINT8 set_7_h(void);
inline UINT8 set_7_l(void);
inline UINT8 set_7_mem_hl(void);
inline UINT8 set_7_a(void);

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
    
  gbcpu->bc.w=0x0013;
  gbcpu->hl.w=0x014d;
  gbcpu->de.w=0x00d8;
  gbcpu->sp.w=0xFFFE;
  gbcpu->pc.w=0x0100;
  gbcpu->mode=0;
  gbcpu->state=0;
  gbcpu->int_flag=0;
  gbcpu->ei_flag=1;
  gbcpu->mode=SIMPLE_SPEED;
}

// GAMEBOY OPERANDE 

inline UINT8 unknown(void){
#ifdef DEBUG
  if (active_msg)
    add_msg("unknow opcode");
#endif
  return 0;
}

inline UINT8 nop(void){
  SUB_CYCLE(4);
}

inline UINT8 ld_bc_nn(void){
  BC=GET_WORD;
  SUB_CYCLE(12);
}

inline UINT8 ld_mem_bc_a(void){
  mem_write(BC,A);
  SUB_CYCLE(8);
}

inline UINT8 inc_bc(void){
  BC++;
  SUB_CYCLE(8);
}

inline UINT8 inc_b(void){
  ((B^0x0f)? UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
  B++;
  ((B) ? UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 dec_b(void){
  ((B&0x0f)?UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
  B--;
  ((B)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 ld_b_n(void){
  B=GET_BYTE;
  SUB_CYCLE(8);
}

inline UINT8 rlca(void){
  //  UINT8 v;
  ((t8=(A&0x80))?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  UNSET_FLAG(FLAG_NN&FLAG_NZ&FLAG_NH);
  A=((A<<1)|(t8>>7));
  SUB_CYCLE(4);
}

inline UINT8 ld_mem_nn_sp(void){
  //  UINT16 v=GET_WORD;
  t16=GET_WORD;
  mem_write(t16,REG_SP.b.l);
  mem_write(t16+1,REG_SP.b.h);  
  SUB_CYCLE(20);
}

inline UINT8 add_hl_bc(void){
  //  UINT32 r=HL+BC;
  UINT32 t32=HL+BC;
  ((t32&0x00010000)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((L&0x10)^(C&0x10))^(r&0x10))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((HL&0x0f)+(BC&0x0f))>0x0f)?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  HL=t32&0xffff;
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(8);
}

inline UINT8 ld_a_mem_bc(void){
  A=mem_read(BC);
  SUB_CYCLE(8);
}

inline UINT8 dec_bc(void){
  BC--;
  SUB_CYCLE(8);
}

inline UINT8 inc_c(void){
  ((C^0x0f)? UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
  C++;
  ((C) ? UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 dec_c(void){
  ((C&0x0f)?UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
  C--;
  ((C)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 ld_c_n(void){
  C=GET_BYTE;
  SUB_CYCLE(8);
}

inline UINT8 rrca(void){
  //  UINT8 v;
  ((t8=(A&0x01))?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  UNSET_FLAG(FLAG_NN&FLAG_NZ&FLAG_NH);
  A=((A>>1)|(t8<<7));
  SUB_CYCLE(4);
}

inline UINT8 stop(void){
  SUB_CYCLE(4);
}

inline UINT8 ld_de_nn(void){
  DE=GET_WORD;
  SUB_CYCLE(12);
}

inline UINT8 ld_mem_de_a(void){
  mem_write(DE,A);
  SUB_CYCLE(8);
}

inline UINT8 inc_de(void){
  DE++;
  SUB_CYCLE(8);
}

inline UINT8 inc_d(void){
  ((D^0x0f)? UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
  D++;
  ((D) ? UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 dec_d(void){
  ((D&0x0f)?UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
  D--;
  ((D)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 ld_d_n(void){
  D=GET_BYTE;
  SUB_CYCLE(8);
}

inline UINT8 rla(void){
  //UINT8 v=((IS_SET(FLAG_C))?1:0);
  t8=((IS_SET(FLAG_C))?1:0);
  ((A&0x80)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  UNSET_FLAG(FLAG_NN&FLAG_NZ&FLAG_NH);
  A=(A<<1)|t8;
  SUB_CYCLE(4);
}

inline UINT8 jr_disp(void){
  PC+=(INT8)GET_BYTE;
  SUB_CYCLE(12);
}

inline UINT8 add_hl_de(void){
  //UINT32 r=HL+DE;
  t32=HL+DE;
  ((t32&0x00010000)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((L&0x10)^(E&0x10))^(r&0x10))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((HL&0x0f)+(DE&0x0f))>0x0f)?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  HL=t32&0xffff;
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(8);
}

inline UINT8 ld_a_mem_de(void){
  A=mem_read(DE);
  SUB_CYCLE(8);
}

inline UINT8 dec_de(void){
  DE--;
  SUB_CYCLE(8);
}

inline UINT8 inc_e(void){
  ((E^0x0f)? UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
  E++;
  ((E) ? UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 dec_e(void){
  ((E&0x0f)?UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
  E--;
  ((E)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 ld_e_n(void){
  E=GET_BYTE;
  SUB_CYCLE(8);
}

inline UINT8 rra(void){
  //UINT8 v=((IS_SET(FLAG_C))?1:0);
  t8=((IS_SET(FLAG_C))?1:0);
  ((A&0x01)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  UNSET_FLAG(FLAG_NN&FLAG_NZ&FLAG_NH);
  A=(A>>1)|(t8<<7);
  SUB_CYCLE(4);
}

inline UINT8 jr_nz_disp(void){
  if (IS_SET(FLAG_Z)) {
    PC++;
    SUB_CYCLE(8);
  } else {
    PC+=(INT8)GET_BYTE;
    SUB_CYCLE(12);
  }
}

inline UINT8 ld_hl_nn(void){
  HL=GET_WORD;
  SUB_CYCLE(12);
}

inline UINT8 ldi_mem_hl_a(void){
  mem_write(HL++,A);
  SUB_CYCLE(8);
}

inline UINT8 inc_hl(void){
  HL++;
  SUB_CYCLE(8);
}

inline UINT8 inc_h(void){
  ((H^0x0f)? UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
  H++;
  ((H) ? UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 dec_h(void){
  ((H&0x0f)?UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
  H--;
  ((H)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 ld_h_n(void){
  H=GET_BYTE;
  SUB_CYCLE(8);
}

inline UINT8 daa(void){
  //UINT16  v=A;
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
}

inline UINT8 jr_z_disp(void){
  if (IS_SET(FLAG_Z)) {
    PC+=(INT8)GET_BYTE;
    SUB_CYCLE(12);
  } else {
    PC++;
    SUB_CYCLE(8);
  }
}

inline UINT8 add_hl_hl(void){
  //  UINT32 r=HL+HL;
  t32=HL+HL;
  ((t32&0x00010000)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((L&0x10)^(L&0x10))^(r&0x10))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((HL&0x0f)+(HL&0x0f))>0x0f)?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  HL=t32&0xffff;
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(8);
}

inline UINT8 ldi_a_mem_hl(void){
  A=mem_read(HL++);
  SUB_CYCLE(8);
}

inline UINT8 dec_hl(void){
  HL--;
  SUB_CYCLE(8);
}

inline UINT8 inc_l(void){
  ((L^0x0f)? UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
  L++;
  ((L) ? UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 dec_l(void){
  ((L&0x0f)?UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
  L--;
  ((L)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 ld_l_n(void){
  L=GET_BYTE;
  SUB_CYCLE(8);
}

inline UINT8 cpl(void){
  A=(~A);
  SET_FLAG(FLAG_N|FLAG_H);
  SUB_CYCLE(4);
}

inline UINT8 jr_nc_disp(void){
  if (IS_SET(FLAG_C)) {
    PC++;
    SUB_CYCLE(8);
  } else {
    PC+=(INT8)GET_BYTE;
    SUB_CYCLE(12);
  }
}

inline UINT8 ld_sp_nn(void){
  SP=GET_WORD;
  SUB_CYCLE(12);
}

inline UINT8 ldd_mem_hl_a(void){
  mem_write(HL--,A);
  SUB_CYCLE(8);
}

inline UINT8 inc_sp(void){
  SP++;
  SUB_CYCLE(8);
}

inline UINT8 inc_mem_hl(void){
  //UINT8 v=mem_read(HL);	
  t8=mem_read(HL);
  ((t8^0x0f)? UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
  t8++;
  ((t8) ? UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  mem_write(HL,t8);
  SUB_CYCLE(12);
}

inline UINT8 dec_mem_hl(void){
  //UINT8 v=mem_read(HL);
  t8=mem_read(HL);
  ((t8&0x0f)?UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
  t8--;
  ((t8)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  mem_write(HL,t8);
  SUB_CYCLE(12);
}

inline UINT8 ld_mem_hl_n(void){
  mem_write(HL,GET_BYTE);
  SUB_CYCLE(12);
}

inline UINT8 scf(void){
  SET_FLAG(FLAG_C);
  UNSET_FLAG(FLAG_NN&FLAG_NH);
  SUB_CYCLE(4);
}

inline UINT8 jr_c_disp(void){
  if (IS_SET(FLAG_C)) {
    PC+=(INT8)GET_BYTE;
    SUB_CYCLE(12);
  } else {
    PC++;
    SUB_CYCLE(8);
  }
}

inline UINT8 add_hl_sp(void){
  //UINT32 r=HL+SP;
  t32=HL+SP;
  ((t32&0x00010000)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((L&0x10)^(SP&0x10))^(r&0x10))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((HL&0x0f)+(SP&0x0f))>0x0f)?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  HL=t32&0xffff;
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(8);
}

inline UINT8 ldd_a_mem_hl(void){
  A=mem_read(HL--);
  SUB_CYCLE(8);
}

inline UINT8 dec_sp(void){
  SP--;
  SUB_CYCLE(8);
}

inline UINT8 inc_a(void){
  ((A^0x0f)? UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
  A++;
  ((A) ? UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 dec_a(void){
  ((A&0x0f)?UNSET_FLAG(FLAG_NH):SET_FLAG(FLAG_H));
  A--;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 ld_a_n(void){
  A=GET_BYTE;
  SUB_CYCLE(8);
}

inline UINT8 ccf(void){
  F^=FLAG_C;
  UNSET_FLAG(FLAG_NN&FLAG_NH);
  SUB_CYCLE(4);
}

/********************************/

inline UINT8 ld_b_b(void){
  //B=B;
  SUB_CYCLE(4);
}

inline UINT8 ld_b_c(void){
  B=C;
  SUB_CYCLE(4);
}

inline UINT8 ld_b_d(void){
  B=D;
  SUB_CYCLE(4);
}

inline UINT8 ld_b_e(void){
  B=E;
  SUB_CYCLE(4);
}

inline UINT8 ld_b_h(void){
  B=H;
  SUB_CYCLE(4);
}

inline UINT8 ld_b_l(void){
  B=L;
  SUB_CYCLE(4);
}

inline UINT8 ld_b_mem_hl(void){
  B=mem_read(HL);
  SUB_CYCLE(8);
}

inline UINT8 ld_b_a(void){
  B=A;
  SUB_CYCLE(4);
}

inline UINT8 ld_c_b(void){
  C=B;
  SUB_CYCLE(4);
}

inline UINT8 ld_c_c(void){
  //  C=C;
  SUB_CYCLE(4);
}

inline UINT8 ld_c_d(void){
  C=D;
  SUB_CYCLE(4);
}

inline UINT8 ld_c_e(void){
  C=E;
  SUB_CYCLE(4);
}

inline UINT8 ld_c_h(void){
  C=H;
  SUB_CYCLE(4);
}

inline UINT8 ld_c_l(void){
  C=L;
  SUB_CYCLE(4);
}

inline UINT8 ld_c_mem_hl(void){
  C=mem_read(HL);
  SUB_CYCLE(8);
}

inline UINT8 ld_c_a(void){
  C=A;
  SUB_CYCLE(4);
}

inline UINT8 ld_d_b(void){
  D=B;
  SUB_CYCLE(4);
}

inline UINT8 ld_d_c(void){
  D=C;
  SUB_CYCLE(4);
}

inline UINT8 ld_d_d(void){
  //D=D;
  SUB_CYCLE(4);
}

inline UINT8 ld_d_e(void){
  D=E;
  SUB_CYCLE(4);
}

inline UINT8 ld_d_h(void){
  D=H;
  SUB_CYCLE(4);
}

inline UINT8 ld_d_l(void){
  D=L;
  SUB_CYCLE(4);
}

inline UINT8 ld_d_mem_hl(void){
  D=mem_read(HL);
  SUB_CYCLE(8);
}

inline UINT8 ld_d_a(void){
  D=A;
  SUB_CYCLE(4);
}

inline UINT8 ld_e_b(void){
  E=B;
  SUB_CYCLE(4);
}

inline UINT8 ld_e_c(void){
  E=C;
  SUB_CYCLE(4);
}

inline UINT8 ld_e_d(void){
  E=D;
  SUB_CYCLE(4);
}

inline UINT8 ld_e_e(void){
  //	E=E;
  SUB_CYCLE(4);
}

inline UINT8 ld_e_h(void){
  E=H;
  SUB_CYCLE(4);
}

inline UINT8 ld_e_l(void){
  E=L;
  SUB_CYCLE(4);
}

inline UINT8 ld_e_mem_hl(void){
  E=mem_read(HL);
  SUB_CYCLE(8);
}

inline UINT8 ld_e_a(void){
  E=A;
  SUB_CYCLE(4);
}

inline UINT8 ld_h_b(void){
  H=B;
  SUB_CYCLE(4);
}

inline UINT8 ld_h_c(void){
  H=C;
  SUB_CYCLE(4);
}

inline UINT8 ld_h_d(void){
  H=D;
  SUB_CYCLE(4);
}

inline UINT8 ld_h_e(void){
  H=E;
  SUB_CYCLE(4);
}

inline UINT8 ld_h_h(void){
  //  H=H;
  SUB_CYCLE(4);
}

inline UINT8 ld_h_l(void){
  H=L;
  SUB_CYCLE(4);
}

inline UINT8 ld_h_mem_hl(void){
  H=mem_read(HL);
  SUB_CYCLE(8);
}

inline UINT8 ld_h_a(void){
  H=A;
  SUB_CYCLE(4);
}

inline UINT8 ld_l_b(void){
  L=B;
  SUB_CYCLE(4);
}

inline UINT8 ld_l_c(void){
  L=C;
  SUB_CYCLE(4);
}

inline UINT8 ld_l_d(void){
  L=D;
  SUB_CYCLE(4);
}

inline UINT8 ld_l_e(void){
  L=E;
  SUB_CYCLE(4);
}

inline UINT8 ld_l_h(void){
  L=H;
  SUB_CYCLE(4);
}

inline UINT8 ld_l_l(void){
  //  L=L;
  SUB_CYCLE(4);
}

inline UINT8 ld_l_mem_hl(void){
  L=mem_read(HL);
  SUB_CYCLE(8);
}

inline UINT8 ld_l_a(void){
  L=A;
  SUB_CYCLE(4);
}

inline UINT8 ld_mem_hl_b(void){
  mem_write(HL,B);
  SUB_CYCLE(8);
}

inline UINT8 ld_mem_hl_c(void){
  mem_write(HL,C);
  SUB_CYCLE(8);
}

inline UINT8 ld_mem_hl_d(void){
  mem_write(HL,D);
  SUB_CYCLE(8);
}

inline UINT8 ld_mem_hl_e(void){
  mem_write(HL,E);
  SUB_CYCLE(8);
}

inline UINT8 ld_mem_hl_h(void){
  mem_write(HL,H);
  SUB_CYCLE(8);
}

inline UINT8 ld_mem_hl_l(void){
  mem_write(HL,L);
  SUB_CYCLE(8);
}

inline UINT8 halt(void){
  if (gbcpu->int_flag) {
#ifdef DEBUG
    if (active_log)
      put_log("set halt \n");
#endif
    gbcpu->state=HALT_STATE;
    gbcpu->pc.w--;
  } /*else {
    UINT16 p=gbcpu->pc.w;
    printf("here\n");
    gbcpu->pc.w--;
    mem_write(gbcpu->pc.w,mem_read(p));
    gbcpu_exec_one();
    mem_write(p,0x76);
    }*/
  SUB_CYCLE(4);
}

inline UINT8 ld_mem_hl_a(void){
  mem_write(HL,A);
  SUB_CYCLE(8);
}

inline UINT8 ld_a_b(void){
  A=B;
  SUB_CYCLE(4);
}

inline UINT8 ld_a_c(void){
  A=C;
  SUB_CYCLE(4);
}

inline UINT8 ld_a_d(void){
  A=D;
  SUB_CYCLE(4);
}

inline UINT8 ld_a_e(void){
  A=E;
  SUB_CYCLE(4);
}

inline UINT8 ld_a_h(void){
  A=H;
  SUB_CYCLE(4);
}

inline UINT8 ld_a_l(void){
  A=L;
  SUB_CYCLE(4);
}

inline UINT8 ld_a_mem_hl(void){
  A=mem_read(HL);
  SUB_CYCLE(8);
}

inline UINT8 ld_a_a(void){
  //A=A;
  SUB_CYCLE(4);
}

/*****************************************/

inline UINT8 add_a_b(void){
  //INT16 r=A+B;
  st16=A+B;
  ((st16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //  ((((A&0x10)^(B&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(B&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=st16&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 add_a_c(void){
  //INT16 r=A+C;
  st16=A+C;
  ((st16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
//((((A&0x10)^(C&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(C&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=st16&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 add_a_d(void){
  //INT16 r=A+D;
  st16=A+D;
  ((st16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((A&0x10)^(D&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(D&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=st16&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 add_a_e(void){
  //INT16 r=A+E;
  st16=A+E;
  ((st16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((A&0x10)^(E&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(E&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=st16&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 add_a_h(void){
  //INT16 r=A+H;
  st16=A+H;
  ((st16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((A&0x10)^(H&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(H&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=st16&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 add_a_l(void){
  //INT16 r=A+L;
  st16=A+L;
  ((st16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((A&0x10)^(L&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(L&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=st16&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 add_a_mem_hl(void){
  //UINT8 v=mem_read(HL);
  //INT16 r=A+v;
  t8=mem_read(HL);
  st16=A+t8;
  ((st16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((A&0x10)^(v&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(t8&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=st16&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(8);
}

inline UINT8 add_a_a(void){
  //INT16 r=A+A;
  st16=A+A;
  ((st16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((A&0x10)^(A&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(A&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=st16&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 adc_a_b(void){
  //  UINT16 v=B+(IS_SET(FLAG_C)?(1):(0));
  //  UINT16 r=A+v;
  t16=B+(IS_SET(FLAG_C)?(1):(0));
  t32=A+t16;
  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((A&0x10)^(v&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=t32&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 adc_a_c(void){
  //UINT16 v=C+(IS_SET(FLAG_C)?(1):(0));
  //UINT16 r=A+v;
  t16=C+(IS_SET(FLAG_C)?(1):(0));
  t32=A+t16;
  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((A&0x10)^(v&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=t32&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 adc_a_d(void){
  //UINT16 v=D+(IS_SET(FLAG_C)?(1):(0));
  //UINT16 r=A+v;
  t16=D+(IS_SET(FLAG_C)?(1):(0));
  t32=A+t16;
  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((A&0x10)^(v&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=t32&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 adc_a_e(void){
  //UINT16 v=E+(IS_SET(FLAG_C)?(1):(0));
  //UINT16 r=A+v;
  t16=E+(IS_SET(FLAG_C)?(1):(0));
  t32=A+t16;
  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((A&0x10)^(v&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=t32&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 adc_a_h(void){
  //UINT16 v=H+(IS_SET(FLAG_C)?(1):(0));
  //UINT16 r=A+v;
  t16=H+(IS_SET(FLAG_C)?(1):(0));
  t32=A+t16;
  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((A&0x10)^(v&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=t32&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 adc_a_l(void){
  //UINT16 v=L+(IS_SET(FLAG_C)?(1):(0));
  //UINT16 r=A+v;
  t16=L+(IS_SET(FLAG_C)?(1):(0));
  t32=A+t16;
  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((A&0x10)^(v&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=t32&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 adc_a_mem_hl(void){
  //UINT16 v=mem_read(HL)+(IS_SET(FLAG_C)?(1):(0));
  //UINT16 r=A+v;
  t16=mem_read(HL)+(IS_SET(FLAG_C)?(1):(0));
  t32=A+t16;
  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((A&0x10)^(v&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=t32&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(8);
}

inline UINT8 adc_a_a(void){
  //UINT16 v=A+(IS_SET(FLAG_C)?(1):(0));
  //UINT16 r=A+v;
  t16=A+(IS_SET(FLAG_C)?(1):(0));
  t32=A+t16;
  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((A&0x10)^(v&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=t32&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(4);
}

inline UINT8 sub_b(void){
  ((B>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((B&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=B;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 sub_c(void){
  ((C>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((C&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=C;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 sub_d(void){
  ((D>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((D&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=D;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 sub_e(void){
  ((E>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((E&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=E;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 sub_h(void){
  ((H>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((H&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=H;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 sub_l(void){
  ((L>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((L&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=L;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 sub_mem_hl(void) {
  //UINT8 v=mem_read(HL);
  t8=mem_read(HL);
  ((t8>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((t8&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=t8;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(8);
}

inline UINT8 sub_a(void){
  ((A>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((A&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=A;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 sbc_a_b(void){
  //UINT16 v=B+((IS_SET(FLAG_C))?(1):(0));
  t16=B+((IS_SET(FLAG_C))?(1):(0));
  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=t16;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 sbc_a_c(void){
  //UINT16 v=C+((IS_SET(FLAG_C))?(1):(0));
  t16=C+((IS_SET(FLAG_C))?(1):(0));
  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=t16;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 sbc_a_d(void){
  //UINT16 v=D+((IS_SET(FLAG_C))?(1):(0));
  t16=D+((IS_SET(FLAG_C))?(1):(0));
  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=t16;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 sbc_a_e(void){
  //UINT16 v=E+((IS_SET(FLAG_C))?(1):(0));
  t16=E+((IS_SET(FLAG_C))?(1):(0));
  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=t16;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 sbc_a_h(void){
  //UINT16 v=H+((IS_SET(FLAG_C))?(1):(0));
  t16=H+((IS_SET(FLAG_C))?(1):(0));
  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=t16;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 sbc_a_l(void){
  //UINT16 v=L+((IS_SET(FLAG_C))?(1):(0));
  t16=L+((IS_SET(FLAG_C))?(1):(0));
  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=t16;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}

inline UINT8 sbc_a_mem_hl(void){
  //UINT16 v=mem_read(HL)+((IS_SET(FLAG_C))?(1):(0));
  t16=mem_read(HL)+((IS_SET(FLAG_C))?(1):(0));
  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=t16;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(8);
}

inline UINT8 sbc_a_a(void){
  //UINT16 v=A+((IS_SET(FLAG_C))?(1):(0));
  t16=A+((IS_SET(FLAG_C))?(1):(0));
  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=t16;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(4);
}    

inline UINT8 and_b(void){
  A&=B;
  if (A) {
    UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_H);
  } else {
    UNSET_FLAG(FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_Z|FLAG_H);
  }   		
  SUB_CYCLE(4);
}

inline UINT8 and_c(void){
  A&=C;
  if (A) {
    UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_H);
  } else {
    UNSET_FLAG(FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_Z|FLAG_H);
  }   		
  SUB_CYCLE(4);
}

inline UINT8 and_d(void){
  A&=D;
  if (A) {
    UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_H);
  } else {
    UNSET_FLAG(FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_Z|FLAG_H);
  }   		
  SUB_CYCLE(4);
}

inline UINT8 and_e(void){
  A&=E;
  if (A) {
    UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_H);
  } else {
    UNSET_FLAG(FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_Z|FLAG_H);
  }   		
  SUB_CYCLE(4);
}

inline UINT8 and_h(void){
  A&=H;
  if (A) {
    UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_H);
  } else {
    UNSET_FLAG(FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_Z|FLAG_H);
  }   		
  SUB_CYCLE(4);
}

inline UINT8 and_l(void){
  A&=L;
  if (A) {
    UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_H);
  } else {
    UNSET_FLAG(FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_Z|FLAG_H);
  }   		
  SUB_CYCLE(4);
}

inline UINT8 and_mem_hl(void){
  A&=mem_read(HL);
  if (A) {
    UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_H);
  } else {
    UNSET_FLAG(FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_Z|FLAG_H);
  }   		
  SUB_CYCLE(8);
}

inline UINT8 and_a(void){
  A&=A;
  if (A) {
    UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_H);
  } else {
    UNSET_FLAG(FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_Z|FLAG_H);
  }   		
  SUB_CYCLE(4);
}

inline UINT8 xor_b(void){
  A^=B;
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z);
  SUB_CYCLE(4);
}

inline UINT8 xor_c(void){
  A^=C;
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z);
  SUB_CYCLE(4);
}

inline UINT8 xor_d(void){
  A^=D;
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z);
  SUB_CYCLE(4);
}

inline UINT8 xor_e(void){
  A^=E;
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z);
  SUB_CYCLE(4);
}

inline UINT8 xor_h(void){
  A^=H;
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z);
  SUB_CYCLE(4);
}

inline UINT8 xor_l(void){
  A^=L;
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z);
  SUB_CYCLE(4);
}

inline UINT8 xor_mem_hl(void){
  A^=mem_read(HL);
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z);
  SUB_CYCLE(8);
}

inline UINT8 xor_a(void){
  A^=A;
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z);
  SUB_CYCLE(4);
}

inline UINT8 or_b(void){
  A|=B;
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z);
  SUB_CYCLE(4);
}

inline UINT8 or_c(void){
  A|=C;
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z);
  SUB_CYCLE(4);
}

inline UINT8 or_d(void){
  A|=D;
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z);
  SUB_CYCLE(4);
}

inline UINT8 or_e(void){
  A|=E;
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z);
  SUB_CYCLE(4);
}

inline UINT8 or_h(void){
  A|=H;
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z);
  SUB_CYCLE(4);
}

inline UINT8 or_l(void){
  A|=L;
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z);
  SUB_CYCLE(4);
}

inline UINT8 or_mem_hl(void){
  A|=mem_read(HL);
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z);
  SUB_CYCLE(8);
}

inline UINT8 or_a(void){
  A|=A;
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z); 
  SUB_CYCLE(4);
}

#define CP_A_R(v) {t16=A-(v); ((t16&0x0100)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); ((t16&0xff)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); ((((v&0x0f)>((A)&0x0f)))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH)); SET_FLAG(FLAG_N); } 

inline UINT8 cp_b(void){
  CP_A_R(B);
  SUB_CYCLE(4);
}   

inline UINT8 cp_c(void){
  CP_A_R(C);
  SUB_CYCLE(4);
}

inline UINT8 cp_d(void){
  CP_A_R(D);
  SUB_CYCLE(4);
}

inline UINT8 cp_e(void){
  CP_A_R(E);
  SUB_CYCLE(4);
}

inline UINT8 cp_h(void){
  CP_A_R(H);
  SUB_CYCLE(4);
}

inline UINT8 cp_l(void){
  CP_A_R(L);
  SUB_CYCLE(4);
}

inline UINT8 cp_mem_hl(void){
  //UINT8 a=mem_read(HL);
  t8=mem_read(HL);
  CP_A_R(t8);
  SUB_CYCLE(8);
}

inline UINT8 cp_a(void){
  CP_A_R(A);
  SUB_CYCLE(4);
}

/**************************************/

inline UINT8 ret_nz(void){
  if (IS_SET(FLAG_Z)) SUB_CYCLE(8);
  else {
    POP_R(REG_PC);
    SUB_CYCLE(20);
  }
}

inline UINT8 pop_bc(void){
  POP_R(REG_BC);
  SUB_CYCLE(12);
}

inline UINT8 jp_nz_nn(void){
  if (IS_SET(FLAG_Z)) {
    PC+=2;
    SUB_CYCLE(12);
  }
  else {
    PC=GET_WORD;
    SUB_CYCLE(16);
  }
}

inline UINT8 jp_nn(void){
  PC=GET_WORD;
  SUB_CYCLE(16);
}

inline UINT8 call_nz_nn(void){
  if (IS_SET(FLAG_Z)) {
    PC+=2;
    SUB_CYCLE(12);
  } else {
    UINT16 v=GET_WORD;
    PUSH_R(REG_PC);
    PC=v;
    SUB_CYCLE(24);
  }
}

inline UINT8 push_bc(void){
  PUSH_R(REG_BC);
  SUB_CYCLE(16);
}

inline UINT8 add_a_n(void){
  //UINT16 v=GET_BYTE;
  //UINT16 r=A+v;
  t8=GET_BYTE;
  t16=A+t8;
  ((t16&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((A&0x10)^(v&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(t8&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=t16&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(8);
}

inline UINT8 rst_00h(void){
  PUSH_R(REG_PC);
  PC=0x0000;
  SUB_CYCLE(16);
}

inline UINT8 ret_z(void){
  if (IS_SET(FLAG_Z)) {
    POP_R(REG_PC);
    SUB_CYCLE(20);
  }else SUB_CYCLE(8);
}

inline UINT8 ret(void){
  POP_R(REG_PC);
  SUB_CYCLE(16);
}

inline UINT8 jp_z_nn(void){
  if (IS_SET(FLAG_Z)) {
    PC=GET_WORD;
    SUB_CYCLE(16);
  } else {
    PC+=2;
    SUB_CYCLE(12);
  }
}

inline UINT8 call_z_nn(void){
  if (IS_SET(FLAG_Z)) {
    //UINT16 v=GET_WORD;
    t16=GET_WORD;
    PUSH_R(REG_PC);
    PC=t16;
    SUB_CYCLE(24);
  } else {
    PC+=2;
    SUB_CYCLE(12);
  }
}

inline UINT8 call_nn(void){
  //UINT16 v=GET_WORD;
  t16=GET_WORD;
  PUSH_R(REG_PC);
  PC=t16;
  SUB_CYCLE(24);
}

inline UINT8 adc_a_n(void){
  //UINT16 v=GET_BYTE+(IS_SET(FLAG_C)?(1):(0));
  //UINT16 r=A+v;
  t16=GET_BYTE+(IS_SET(FLAG_C)?(1):(0));
  t32=A+t16;
  ((t32&0x0100) ? SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  //((((A&0x10)^(v&0x10))^(r&0x10)) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((A&0X0f)+(t16&0x0f))>0x0f) ? SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  A=t32&0xff;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  UNSET_FLAG(FLAG_NN);
  SUB_CYCLE(8);
}

inline UINT8 rst_8h(void){
  PUSH_R(REG_PC);
  PC=0x0008;
  SUB_CYCLE(16);
}

inline UINT8 ret_nc(void){
  if (IS_SET(FLAG_C)) SUB_CYCLE(8);
  else {
    POP_R(REG_PC);
    SUB_CYCLE(20);
  }		
}

inline UINT8 pop_de(void){
  POP_R(REG_DE);
  SUB_CYCLE(12);
}

inline UINT8 jp_nc_nn(void){
  if (IS_SET(FLAG_C)) {
    PC+=2;
    SUB_CYCLE(12);
  } else {
    PC=GET_WORD;
    SUB_CYCLE(16);
  }
}

inline UINT8 call_nc_nn(void){
  if (IS_SET(FLAG_C)) {
    PC+=2;
    SUB_CYCLE(12);
  } else {
    //UINT16 v=GET_WORD;
    t16=GET_WORD;
    PUSH_R(REG_PC);
    PC=t16;
    SUB_CYCLE(24);
  }
}

inline UINT8 push_de(void){
  PUSH_R(REG_DE);
  SUB_CYCLE(16);
}

inline UINT8 sub_n(void){
  //UINT8 v=GET_BYTE;
  t8=GET_BYTE;
  ((t8>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((t8&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=t8;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(8);
}

inline UINT8 rst_10h(void){
  PUSH_R(REG_PC);
  PC=0x0010;
  SUB_CYCLE(16);
}

inline UINT8 ret_c(void){
  if (IS_SET(FLAG_C)) {
    POP_R(REG_PC);
    SUB_CYCLE(20);
  }else SUB_CYCLE(8);	
}

inline UINT8 reti(void){
#ifdef DEBUG
  if (active_log)
    put_log("RETI\n");
#endif 
  POP_R(REG_PC);
  //EI;
  gbcpu->int_flag=1;
  SUB_CYCLE(16);
}

inline UINT8 jp_c_nn(void){
  if (IS_SET(FLAG_C)) {
    PC=GET_WORD;
    SUB_CYCLE(16);
  }else {
    PC+=2;
    SUB_CYCLE(12);
  }
}

inline UINT8 call_c_nn(void){
  if (IS_SET(FLAG_C)) {
    //UINT16 v=GET_WORD;
    t16=GET_WORD;
    PUSH_R(REG_PC);
    PC=t16;
    SUB_CYCLE(24);
  } else {
    PC+=2;
    SUB_CYCLE(12);
  }
}

inline UINT8 sbc_a_n(void){
  //UINT16 v=GET_BYTE+((IS_SET(FLAG_C))?(1):(0));
  t16=GET_BYTE+((IS_SET(FLAG_C))?(1):(0));
  ((t16>A)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  (((t16&0x0f)>(A&0x0f))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  A-=t16;
  ((A)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z));
  SET_FLAG(FLAG_N);
  SUB_CYCLE(8);
}

inline UINT8 rst_18h(void){
  PUSH_R(REG_PC);
  PC=0x0018;
  SUB_CYCLE(16);
}

inline UINT8 ld_mem_ff00_n_a(void){
  mem_write_ff(0xff00+GET_BYTE,A);
  SUB_CYCLE(12);
}

inline UINT8 pop_hl(void){
  POP_R(REG_HL);
  SUB_CYCLE(12);
}

inline UINT8 ld_mem_ff00_c_a(void){
  mem_write_ff(0xff00+C,A);
  SUB_CYCLE(8);
}

inline UINT8 push_hl(void){
  PUSH_R(REG_HL);
  SUB_CYCLE(16);
}

inline UINT8 and_n(void){
  A&=GET_BYTE;
  if (A) {
    UNSET_FLAG(FLAG_NZ&FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_H);
  } else {
    UNSET_FLAG(FLAG_NN&FLAG_NC);
    SET_FLAG(FLAG_Z|FLAG_H);
  }   		
  SUB_CYCLE(8);
}

inline UINT8 rst_20h(void){
  PUSH_R(REG_PC);
  PC=0x0020;
  SUB_CYCLE(16);
}

inline UINT8 add_sp_dd(void){
  //INT8 v=GET_BYTE;
  //UINT32 r=SP+v;
  st8=GET_BYTE;
  t32=SP+st8;
  ((t32&0x00010000)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  ((((SP&0x0f)+(st8&0x0f))&0x10)?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  SP=t32&0xffff;
  UNSET_FLAG(FLAG_NN&FLAG_NZ);
  SUB_CYCLE(16);
}

inline UINT8 jp_mem_hl(void){
  PC=HL;
  SUB_CYCLE(4);
}

inline UINT8 ld_mem_nn_a(void){
  mem_write(GET_WORD,A);
  SUB_CYCLE(16);
}

inline UINT8 xor_n(void){
  A^=GET_BYTE;
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z);
  SUB_CYCLE(8);
}

inline UINT8 rst_28h(void){
  PUSH_R(REG_PC);
  PC=0x0028;
  SUB_CYCLE(16);
}

inline UINT8 ld_a_mem_ff00_n(void){
  A=mem_read_ff(0xff00+GET_BYTE);
  SUB_CYCLE(12);
}

inline UINT8 pop_af(void){
  POP_R(REG_AF);
  SUB_CYCLE(12);
}

inline UINT8 ld_a_mem_c(void){
  A=mem_read_ff(0xff00+C);
  SUB_CYCLE(8);
}

inline UINT8 di(void){
#ifdef DEBUG
  if (active_log) 
    put_log("DI\n");
#endif
  DI;
  SUB_CYCLE(4);
}

inline UINT8 push_af(void){
  PUSH_R(REG_AF);
  SUB_CYCLE(16);
}

inline UINT8 or_n(void){
  A|=GET_BYTE;
  UNSET_FLAG(FLAG_NN&FLAG_NC&FLAG_NH);
  if (A) UNSET_FLAG(FLAG_NZ);
  else SET_FLAG(FLAG_Z);
  SUB_CYCLE(8);
}

inline UINT8 rst_30h(void){
  PUSH_R(REG_PC);
  PC=0x0030;
  SUB_CYCLE(16);
}

inline UINT8 ld_hl_sp_dd(void){
  //INT8 v=GET_BYTE;
  //UINT32 r=SP+v;
  st8=GET_BYTE;
  t32=SP+st8;
  ((t32&0x00010000)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC));
  // ((((SP&0x10)^(v&0x10))^(r&0x10))?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH));
  (((SP&0x0f)+(st8&0x0f))>0x0f) ?SET_FLAG(FLAG_H):UNSET_FLAG(FLAG_NH);
  HL=t32&0xffff;
  UNSET_FLAG(FLAG_NN&FLAG_NZ);
  SUB_CYCLE(12);
}

inline UINT8 ld_sp_hl(void){
  SP=HL;
  SUB_CYCLE(8);
}

inline UINT8 ld_a_mem_nn(void){
  A=mem_read(GET_WORD);
  SUB_CYCLE(16);
}

inline UINT8 ei(void){
#ifdef DEBUG
  if (active_log)
    put_log("EI\n");
#endif
  EI;
  SUB_CYCLE(4);
}

inline UINT8 cp_n(void){
  //UINT8 a=GET_BYTE;
  t8=GET_BYTE;
  CP_A_R(t8);
  SUB_CYCLE(8);
}

inline UINT8 rst_38h(void){
  PUSH_R(REG_PC);
  PC=0x0038;
  SUB_CYCLE(16);
}

/****************************************/

// CB INSTRUCTION

#define RLC_R(r) {t8=((r)&0x80)>>7; (r)=((r)<<1)|t8; ((t8)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH);}


inline UINT8 rlc_b(void){
  RLC_R(B);
  SUB_CYCLE(8);
}

inline UINT8 rlc_c(void){
  RLC_R(C);
  SUB_CYCLE(8);
}

inline UINT8 rlc_d(void){
  RLC_R(D);
  SUB_CYCLE(8);
}

inline UINT8 rlc_e(void){
  RLC_R(E);
  SUB_CYCLE(8);
}

inline UINT8 rlc_h(void){
  RLC_R(H);
  SUB_CYCLE(8);
}

inline UINT8 rlc_l(void){
  RLC_R(L);
  SUB_CYCLE(8);
}

inline UINT8 rlc_mem_hl(void){
  UINT8 a=mem_read(HL); // laisse le
  RLC_R(a);
  mem_write(HL,a);
  SUB_CYCLE(16);
}

inline UINT8 rlc_a(void){
  RLC_R(A);
  SUB_CYCLE(8);
}

#define RRC_R(r) {t8=((r)&0x01)<<7; (r)=((r)>>1)|t8; ((t8)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH);}


inline UINT8 rrc_b(void){
  RRC_R(B);
  SUB_CYCLE(8);
}

inline UINT8 rrc_c(void){
  RRC_R(C);
  SUB_CYCLE(8);
}

inline UINT8 rrc_d(void){
  RRC_R(D);
  SUB_CYCLE(8);
}

inline UINT8 rrc_e(void){
  RRC_R(E);
  SUB_CYCLE(8);
}

inline UINT8 rrc_h(void){
  RRC_R(H);
  SUB_CYCLE(8);
}

inline UINT8 rrc_l(void){
  RRC_R(L);
  SUB_CYCLE(8);
}

inline UINT8 rrc_mem_hl(void){
  UINT8 a=mem_read(HL);
  RRC_R(a);
  mem_write(HL,a);
  SUB_CYCLE(16);
}

inline UINT8 rrc_a(void){
  RRC_R(A);
  SUB_CYCLE(8);
}

#define RL_R(r) {t8=(IS_SET(FLAG_C)?(1):(0)); (((r)&0x80)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); (r)=((r)<<1)|t8; ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH); }

inline UINT8 rl_b(void){
  RL_R(B);
  SUB_CYCLE(8);
}

inline UINT8 rl_c(void){
  RL_R(C);
  SUB_CYCLE(8);
}

inline UINT8 rl_d(void){
  RL_R(D);
  SUB_CYCLE(8);
}

inline UINT8 rl_e(void){
  RL_R(E);
  SUB_CYCLE(8);
}

inline UINT8 rl_h(void){
  RL_R(H);
  SUB_CYCLE(8);
}

inline UINT8 rl_l(void){
  RL_R(L);
  SUB_CYCLE(8);
}

inline UINT8 rl_mem_hl(void){
  UINT8 a=mem_read(HL);
  RL_R(a);
  mem_write(HL,a);
  SUB_CYCLE(16);
}

inline UINT8 rl_a(void){
  RL_R(A);
  SUB_CYCLE(8);
}

#define RR_R(r) {t8=(IS_SET(FLAG_C)?(0x80):(0x00)); (((r)&0x01)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); (r)=((r)>>1)|t8; ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH);}

inline UINT8 rr_b(void){
  RR_R(B);
  SUB_CYCLE(8);
}

inline UINT8 rr_c(void){
  RR_R(C);
  SUB_CYCLE(8);
}

inline UINT8 rr_d(void){
  RR_R(D);
  SUB_CYCLE(8);
}

inline UINT8 rr_e(void){
  RR_R(E);
  SUB_CYCLE(8);
}

inline UINT8 rr_h(void){
  RR_R(H);
  SUB_CYCLE(8);
}

inline UINT8 rr_l(void){
  RR_R(L);
  SUB_CYCLE(8);
}

inline UINT8 rr_mem_hl(void){
  UINT8 a=mem_read(HL);
  RR_R(a);
  mem_write(HL,a);
  SUB_CYCLE(16);
}

inline UINT8 rr_a(void){
  RR_R(A);
  SUB_CYCLE(8);
}

#define SLA_R(r) {(((r)&0x80)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); (r)<<=1; ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH);}

inline UINT8 sla_b(void){
  SLA_R(B);
  SUB_CYCLE(8);
}

inline UINT8 sla_c(void){
  SLA_R(C);
  SUB_CYCLE(8);
}

inline UINT8 sla_d(void){
  SLA_R(D);
  SUB_CYCLE(8);
}

inline UINT8 sla_e(void){
  SLA_R(E);
  SUB_CYCLE(8);
}

inline UINT8 sla_h(void){
  SLA_R(H);
  SUB_CYCLE(8);
}

inline UINT8 sla_l(void){
  SLA_R(L);
  SUB_CYCLE(8);
}

inline UINT8 sla_mem_hl(void){
  UINT8 a=mem_read(HL);
  SLA_R(a);
  mem_write(HL,a);
  SUB_CYCLE(16);
}

inline UINT8 sla_a(void){
  SLA_R(A);
  SUB_CYCLE(8);
}

#define SRA_R(r) {(((r)&0x01)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); (r)=((r)&0x80)|((r)>>1); ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH);}

inline UINT8 sra_b(void){
  SRA_R(B);
  SUB_CYCLE(8);
}

inline UINT8 sra_c(void){
  SRA_R(C);
  SUB_CYCLE(8);
}

inline UINT8 sra_d(void){
  SRA_R(D);
  SUB_CYCLE(8);
}

inline UINT8 sra_e(void){
  SRA_R(E);
  SUB_CYCLE(8);
}

inline UINT8 sra_h(void){
  SRA_R(H);
  SUB_CYCLE(8);
}

inline UINT8 sra_l(void){
  SRA_R(L);
  SUB_CYCLE(8);
}

inline UINT8 sra_mem_hl(void){
  UINT8 a=mem_read(HL);
  SRA_R(a);
  mem_write(HL,a);
  SUB_CYCLE(16);
}

inline UINT8 sra_a(void){
  SRA_R(A);
  SUB_CYCLE(8);
}

#define SWAP_R(r) {(r)=(((r)&0xf0)>>4)|(((r)&0x0f)<<4); ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH&FLAG_NC);}

inline UINT8 swap_b(void){
  SWAP_R(B);
  SUB_CYCLE(8);
}

inline UINT8 swap_c(void){
  SWAP_R(C);
  SUB_CYCLE(8);
}

inline UINT8 swap_d(void){
  SWAP_R(D);
  SUB_CYCLE(8);
}

inline UINT8 swap_e(void){
  SWAP_R(E);
  SUB_CYCLE(8);
}

inline UINT8 swap_h(void){
  SWAP_R(H);
  SUB_CYCLE(8);
}

inline UINT8 swap_l(void){
  SWAP_R(L);
  SUB_CYCLE(8);
}

inline UINT8 swap_mem_hl(void){
  UINT8 a=mem_read(HL);
  SWAP_R(a);
  mem_write(HL,a);
  SUB_CYCLE(16);
}

inline UINT8 swap_a(void){
  SWAP_R(A);
  SUB_CYCLE(8);
}

#define SRL_R(r) {(((r)&0x01)?SET_FLAG(FLAG_C):UNSET_FLAG(FLAG_NC)); (r)>>=1; ((r)?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); UNSET_FLAG(FLAG_NN&FLAG_NH);}
 
inline UINT8 srl_b(void){
  SRL_R(B);
  SUB_CYCLE(8);
}

inline UINT8 srl_c(void){
  SRL_R(C);
  SUB_CYCLE(8);
}

inline UINT8 srl_d(void){
  SRL_R(D);
  SUB_CYCLE(8);
}

inline UINT8 srl_e(void){
  SRL_R(E);
  SUB_CYCLE(8);
}

inline UINT8 srl_h(void){
  SRL_R(H);
  SUB_CYCLE(8);
}

inline UINT8 srl_l(void){
  SRL_R(L);
  SUB_CYCLE(8);
}

inline UINT8 srl_mem_hl(void){
  UINT8 a=mem_read(HL);
  SRL_R(a);
  mem_write(HL,a);
  SUB_CYCLE(16);
}

inline UINT8 srl_a(void){
  SRL_R(A);
  SUB_CYCLE(8);
}

#define BIT_N_R(n,r) {(((r)&(n))?UNSET_FLAG(FLAG_NZ):SET_FLAG(FLAG_Z)); SET_FLAG(FLAG_H); UNSET_FLAG(FLAG_NN);}
  

inline UINT8 bit_0_b(void){
  BIT_N_R(0x01,B);
  SUB_CYCLE(8);
}

inline UINT8 bit_0_c(void){
  BIT_N_R(0x01,C);
  SUB_CYCLE(8);
}

inline UINT8 bit_0_d(void){
  BIT_N_R(0x01,D);
  SUB_CYCLE(8);
}

inline UINT8 bit_0_e(void){
  BIT_N_R(0x01,E);
  SUB_CYCLE(8);
}

inline UINT8 bit_0_h(void){
  BIT_N_R(0x01,H);
  SUB_CYCLE(8);
}

inline UINT8 bit_0_l(void){
  BIT_N_R(0x01,L);
  SUB_CYCLE(8);
}

inline UINT8 bit_0_mem_hl(void){
  UINT8 a=mem_read(HL);
  BIT_N_R(0x01,a);
  SUB_CYCLE(12);
}

inline UINT8 bit_0_a(void){
  BIT_N_R(0x01,A);
  SUB_CYCLE(8);
}

inline UINT8 bit_1_b(void){
  BIT_N_R(0x02,B);
  SUB_CYCLE(8);
}

inline UINT8 bit_1_c(void){
  BIT_N_R(0x02,C);
  SUB_CYCLE(8);
}

inline UINT8 bit_1_d(void){
  BIT_N_R(0x02,D);
  SUB_CYCLE(8);
}

inline UINT8 bit_1_e(void){
  BIT_N_R(0x02,E);
  SUB_CYCLE(8);
}

inline UINT8 bit_1_h(void){
  BIT_N_R(0x02,H);
  SUB_CYCLE(8);
}

inline UINT8 bit_1_l(void){
  BIT_N_R(0x02,L);
  SUB_CYCLE(8);
}

inline UINT8 bit_1_mem_hl(void){
  UINT8 a=mem_read(HL);
  BIT_N_R(0x02,a);
  SUB_CYCLE(12);
}

inline UINT8 bit_1_a(void){
  BIT_N_R(0x02,A);
  SUB_CYCLE(8);
}

inline UINT8 bit_2_b(void){
  BIT_N_R(0x04,B);
  SUB_CYCLE(8);
}

inline UINT8 bit_2_c(void){
  BIT_N_R(0x04,C);
  SUB_CYCLE(8);
}

inline UINT8 bit_2_d(void){
  BIT_N_R(0x04,D);
  SUB_CYCLE(8);
}

inline UINT8 bit_2_e(void){
  BIT_N_R(0x04,E);
  SUB_CYCLE(8);
}

inline UINT8 bit_2_h(void){
  BIT_N_R(0x04,H);
  SUB_CYCLE(8);
}

inline UINT8 bit_2_l(void){
  BIT_N_R(0x04,L);
  SUB_CYCLE(8);
}

inline UINT8 bit_2_mem_hl(void){
  UINT8 a=mem_read(HL);
  BIT_N_R(0x04,a);
  SUB_CYCLE(12);
}

inline UINT8 bit_2_a(void){
  BIT_N_R(0x04,A);
  SUB_CYCLE(8);
}

inline UINT8 bit_3_b(void){
  BIT_N_R(0x08,B);
  SUB_CYCLE(8);
}

inline UINT8 bit_3_c(void){
  BIT_N_R(0x08,C);
  SUB_CYCLE(8);
}

inline UINT8 bit_3_d(void){
  BIT_N_R(0x08,D);
  SUB_CYCLE(8);
}

inline UINT8 bit_3_e(void){
  BIT_N_R(0x08,E);
  SUB_CYCLE(8);
}

inline UINT8 bit_3_h(void){
  BIT_N_R(0x08,H);
  SUB_CYCLE(8);
}

inline UINT8 bit_3_l(void){
  BIT_N_R(0x08,L);
  SUB_CYCLE(8);
}

inline UINT8 bit_3_mem_hl(void){
  UINT8 a=mem_read(HL);
  BIT_N_R(0x08,a);
  SUB_CYCLE(12);
}

inline UINT8 bit_3_a(void){
  BIT_N_R(0x08,A);
  SUB_CYCLE(8);
}

inline UINT8 bit_4_b(void){
  BIT_N_R(0x10,B);
  SUB_CYCLE(8);
}

inline UINT8 bit_4_c(void){
  BIT_N_R(0x10,C);
  SUB_CYCLE(8);
}

inline UINT8 bit_4_d(void){
  BIT_N_R(0x10,D);
  SUB_CYCLE(8);
}

inline UINT8 bit_4_e(void){
  BIT_N_R(0x10,E);
  SUB_CYCLE(8);
}

inline UINT8 bit_4_h(void){
  BIT_N_R(0x10,H);
  SUB_CYCLE(8);
}

inline UINT8 bit_4_l(void){
  BIT_N_R(0x10,L);
  SUB_CYCLE(8);
}

inline UINT8 bit_4_mem_hl(void){
  UINT8 a=mem_read(HL);
  BIT_N_R(0x10,a);
  SUB_CYCLE(12);
}

inline UINT8 bit_4_a(void){
  BIT_N_R(0x10,A);
  SUB_CYCLE(8);
}

inline UINT8 bit_5_b(void){
  BIT_N_R(0x20,B);
  SUB_CYCLE(8);
}

inline UINT8 bit_5_c(void){
  BIT_N_R(0x20,C);
  SUB_CYCLE(8);
}

inline UINT8 bit_5_d(void){
  BIT_N_R(0x20,D);
  SUB_CYCLE(8);
}

inline UINT8 bit_5_e(void){
  BIT_N_R(0x20,E);
  SUB_CYCLE(8);
}

inline UINT8 bit_5_h(void){
  BIT_N_R(0x20,H);
  SUB_CYCLE(8);
}

inline UINT8 bit_5_l(void){
  BIT_N_R(0x20,L);
  SUB_CYCLE(8);
}

inline UINT8 bit_5_mem_hl(void){
  UINT8 a=mem_read(HL);
  BIT_N_R(0x20,a);
  SUB_CYCLE(12);
}

inline UINT8 bit_5_a(void){
  BIT_N_R(0x20,A);
  SUB_CYCLE(8);
}

inline UINT8 bit_6_b(void){
  BIT_N_R(0x40,B);
  SUB_CYCLE(8);
}

inline UINT8 bit_6_c(void){
  BIT_N_R(0x40,C);
  SUB_CYCLE(8);
}

inline UINT8 bit_6_d(void){
  BIT_N_R(0x40,D);
  SUB_CYCLE(8);
}

inline UINT8 bit_6_e(void){
  BIT_N_R(0x40,E);
  SUB_CYCLE(8);
}

inline UINT8 bit_6_h(void){
  BIT_N_R(0x40,H);
  SUB_CYCLE(8);
}

inline UINT8 bit_6_l(void){
  BIT_N_R(0x40,L);
  SUB_CYCLE(8);
}

inline UINT8 bit_6_mem_hl(void){
  UINT8 a=mem_read(HL);
  BIT_N_R(0x40,a);
  SUB_CYCLE(12);
}

inline UINT8 bit_6_a(void){
  BIT_N_R(0x40,A);
  SUB_CYCLE(8);
}

inline UINT8 bit_7_b(void){
  BIT_N_R(0x80,B);
  SUB_CYCLE(8);
}

inline UINT8 bit_7_c(void){
  BIT_N_R(0x80,C);
  SUB_CYCLE(8);
}

inline UINT8 bit_7_d(void){
  BIT_N_R(0x80,D);
  SUB_CYCLE(8);
}

inline UINT8 bit_7_e(void){
  BIT_N_R(0x80,E);
  SUB_CYCLE(8);
}

inline UINT8 bit_7_h(void){
  BIT_N_R(0x80,H);
  SUB_CYCLE(8);
}

inline UINT8 bit_7_l(void){
  BIT_N_R(0x80,L);
  SUB_CYCLE(8);
}

inline UINT8 bit_7_mem_hl(void){
  UINT8 a=mem_read(HL);
  BIT_N_R(0x80,a);
  SUB_CYCLE(12);
}

inline UINT8 bit_7_a(void){
  BIT_N_R(0x80,A);
  SUB_CYCLE(8);
}

inline UINT8 res_0_b(void){
  B&=0xfe;
  SUB_CYCLE(8);
}

inline UINT8 res_0_c(void){
  C&=0xfe;
  SUB_CYCLE(8);
}

inline UINT8 res_0_d(void){
  D&=0xfe;
  SUB_CYCLE(8);
}

inline UINT8 res_0_e(void){
  E&=0xfe;
  SUB_CYCLE(8);
}

inline UINT8 res_0_h(void){
  H&=0xfe;
  SUB_CYCLE(8);
}

inline UINT8 res_0_l(void){
  L&=0xfe;
  SUB_CYCLE(8);
}

inline UINT8 res_0_mem_hl(void){
  UINT8 v=mem_read(HL);
  v&=0xfe;
  mem_write(HL,v);
  SUB_CYCLE(16);
}

inline UINT8 res_0_a(void){
  A&=0xfe;
  SUB_CYCLE(8);
}

inline UINT8 res_1_b(void){
  B&=0xfd;
  SUB_CYCLE(8);
}

inline UINT8 res_1_c(void){
  C&=0xfd;
  SUB_CYCLE(8);
}

inline UINT8 res_1_d(void){
  D&=0xfd;
  SUB_CYCLE(8);
}

inline UINT8 res_1_e(void){
  E&=0xfd;
  SUB_CYCLE(8);
}

inline UINT8 res_1_h(void){
  H&=0xfd;
  SUB_CYCLE(8);
}

inline UINT8 res_1_l(void){
  L&=0xfd;
  SUB_CYCLE(8);
}

inline UINT8 res_1_mem_hl(void){
  UINT8 v=mem_read(HL);
  v&=0xfd;
  mem_write(HL,v);
  SUB_CYCLE(16);
}

inline UINT8 res_1_a(void){
  A&=0xfd;
  SUB_CYCLE(8);
}

inline UINT8 res_2_b(void){
  B&=0xfb;
  SUB_CYCLE(8);
}

inline UINT8 res_2_c(void){
  C&=0xfb;
  SUB_CYCLE(8);
}

inline UINT8 res_2_d(void){
  D&=0xfb;
  SUB_CYCLE(8);
}

inline UINT8 res_2_e(void){
  E&=0xfb;
  SUB_CYCLE(8);
}

inline UINT8 res_2_h(void){
  H&=0xfb;
  SUB_CYCLE(8);
}

inline UINT8 res_2_l(void){
  L&=0xfb;
  SUB_CYCLE(8);
}

inline UINT8 res_2_mem_hl(void){
  UINT8 v=mem_read(HL);
  v&=0xfb;
  mem_write(HL,v);
  SUB_CYCLE(16);
}

inline UINT8 res_2_a(void){
  A&=0xfb;
  SUB_CYCLE(8);
}

inline UINT8 res_3_b(void){
  B&=0xf7;
  SUB_CYCLE(8);
}

inline UINT8 res_3_c(void){
  C&=0xf7;
  SUB_CYCLE(8);
}

inline UINT8 res_3_d(void){
  D&=0xf7;
  SUB_CYCLE(8);
}

inline UINT8 res_3_e(void){
  E&=0xf7;
  SUB_CYCLE(8);
}

inline UINT8 res_3_h(void){
  H&=0xf7;
  SUB_CYCLE(8);
}

inline UINT8 res_3_l(void){
  L&=0xf7;
  SUB_CYCLE(8);
}

inline UINT8 res_3_mem_hl(void){
  UINT8 v=mem_read(HL);
  v&=0xf7;
  mem_write(HL,v);
  SUB_CYCLE(16);
}

inline UINT8 res_3_a(void){
  A&=0xf7;
  SUB_CYCLE(8);
}

inline UINT8 res_4_b(void){
  B&=0xef;
  SUB_CYCLE(8);
}

inline UINT8 res_4_c(void){
  C&=0xef;
  SUB_CYCLE(8);
}

inline UINT8 res_4_d(void){
  D&=0xef;
  SUB_CYCLE(8);
}

inline UINT8 res_4_e(void){
  E&=0xef;
  SUB_CYCLE(8);
}

inline UINT8 res_4_h(void){
  H&=0xef;
  SUB_CYCLE(8);
}

inline UINT8 res_4_l(void){
  L&=0xef;
  SUB_CYCLE(8);
}

inline UINT8 res_4_mem_hl(void){
  UINT8 v=mem_read(HL);
  v&=0xef;
  mem_write(HL,v);
  SUB_CYCLE(16);
}

inline UINT8 res_4_a(void){
  A&=0xef;
  SUB_CYCLE(8);
}

inline UINT8 res_5_b(void){
  B&=0xdf;
  SUB_CYCLE(8);
}

inline UINT8 res_5_c(void){
  C&=0xdf;
  SUB_CYCLE(8);
}

inline UINT8 res_5_d(void){
  D&=0xdf;
  SUB_CYCLE(8);
}

inline UINT8 res_5_e(void){
  E&=0xdf;
  SUB_CYCLE(8);
}

inline UINT8 res_5_h(void){
  H&=0xdf;
  SUB_CYCLE(8);
}

inline UINT8 res_5_l(void){
  L&=0xdf;
  SUB_CYCLE(8);
}

inline UINT8 res_5_mem_hl(void){
  UINT8 v=mem_read(HL);
  v&=0xdf;
  mem_write(HL,v);
  SUB_CYCLE(16);
}

inline UINT8 res_5_a(void){
  A&=0xdf;
  SUB_CYCLE(8);
}

inline UINT8 res_6_b(void){
  B&=0xbf;
  SUB_CYCLE(8);
}

inline UINT8 res_6_c(void){
  C&=0xbf;
  SUB_CYCLE(8);
}

inline UINT8 res_6_d(void){
  D&=0xbf;
  SUB_CYCLE(8);
}

inline UINT8 res_6_e(void){
  E&=0xbf;
  SUB_CYCLE(8);
}

inline UINT8 res_6_h(void){
  H&=0xbf;
  SUB_CYCLE(8);
}

inline UINT8 res_6_l(void){
  L&=0xbf;
  SUB_CYCLE(8);
}

inline UINT8 res_6_mem_hl(void){
  UINT8 v=mem_read(HL);
  v&=0xbf;
  mem_write(HL,v);
  SUB_CYCLE(16);
}

inline UINT8 res_6_a(void){
  A&=0xbf;
  SUB_CYCLE(8);
}

inline UINT8 res_7_b(void){
  B&=0x7f;
  SUB_CYCLE(8);
}

inline UINT8 res_7_c(void){
  C&=0x7f;
  SUB_CYCLE(8);
}

inline UINT8 res_7_d(void){
  D&=0x7f;
  SUB_CYCLE(8);
}

inline UINT8 res_7_e(void){
  E&=0x7f;
  SUB_CYCLE(8);
}

inline UINT8 res_7_h(void){
  H&=0x7f;
  SUB_CYCLE(8);
}

inline UINT8 res_7_l(void){
  L&=0x7f;
  SUB_CYCLE(8);
}

inline UINT8 res_7_mem_hl(void){
  UINT8 v=mem_read(HL);
  v&=0x7f;
  mem_write(HL,v);
  SUB_CYCLE(16);
}

inline UINT8 res_7_a(void){
  A&=0x7f;
  SUB_CYCLE(8);
}

inline UINT8 set_0_b(void){
  B|=0x01;
  SUB_CYCLE(8);
}

inline UINT8 set_0_c(void){
  C|=0x01;
  SUB_CYCLE(8);
}

inline UINT8 set_0_d(void){
  D|=0x01;
  SUB_CYCLE(8);
}

inline UINT8 set_0_e(void){
  E|=0x01;
  SUB_CYCLE(8);
}

inline UINT8 set_0_h(void){
  H|=0x01;
  SUB_CYCLE(8);
}

inline UINT8 set_0_l(void){
  L|=0x01;
  SUB_CYCLE(8);
}

inline UINT8 set_0_mem_hl(void){
  UINT8 v=mem_read(HL);
  v|=0x01;
  mem_write(HL,v);
  SUB_CYCLE(16);
}

inline UINT8 set_0_a(void){
  A|=0x01;
  SUB_CYCLE(8);
}

inline UINT8 set_1_b(void){
  B|=0x02;
  SUB_CYCLE(8);
}

inline UINT8 set_1_c(void){
  C|=0x02;
  SUB_CYCLE(8);
}

inline UINT8 set_1_d(void){
  D|=0x02;
  SUB_CYCLE(8);
}

inline UINT8 set_1_e(void){
  E|=0x02;
  SUB_CYCLE(8);
}

inline UINT8 set_1_h(void){
  H|=0x02;
  SUB_CYCLE(8);
}

inline UINT8 set_1_l(void){
  L|=0x02;
  SUB_CYCLE(8);
}

inline UINT8 set_1_mem_hl(void){
  UINT8 v=mem_read(HL);
  v|=0x02;
  mem_write(HL,v);
  SUB_CYCLE(16);
}

inline UINT8 set_1_a(void){
  A|=0x02;
  SUB_CYCLE(8);
}

inline UINT8 set_2_b(void){
  B|=0x04;
  SUB_CYCLE(8);
}

inline UINT8 set_2_c(void){
  C|=0x04;
  SUB_CYCLE(8);
}

inline UINT8 set_2_d(void){
  D|=0x04;
  SUB_CYCLE(8);
}

inline UINT8 set_2_e(void){
  E|=0x04;
  SUB_CYCLE(8);
}

inline UINT8 set_2_h(void){
  H|=0x04;
  SUB_CYCLE(8);
}

inline UINT8 set_2_l(void){
  L|=0x04;
  SUB_CYCLE(8);
}

inline UINT8 set_2_mem_hl(void){
  UINT8 v=mem_read(HL);
  v|=0x04;
  mem_write(HL,v);
  SUB_CYCLE(16);
}

inline UINT8 set_2_a(void){
  A|=0x04;
  SUB_CYCLE(8);
}

inline UINT8 set_3_b(void){
  B|=0x08;
  SUB_CYCLE(8);
}

inline UINT8 set_3_c(void){
  C|=0x08;
  SUB_CYCLE(8);
}

inline UINT8 set_3_d(void){
  D|=0x08;
  SUB_CYCLE(8);
}

inline UINT8 set_3_e(void){
  E|=0x08;
  SUB_CYCLE(8);
}

inline UINT8 set_3_h(void){
  H|=0x08;
  SUB_CYCLE(8);
}

inline UINT8 set_3_l(void){
  L|=0x08;
  SUB_CYCLE(8);
}

inline UINT8 set_3_mem_hl(void){
  UINT8 v=mem_read(HL);
  v|=0x08;
  mem_write(HL,v);
  SUB_CYCLE(16);
}

inline UINT8 set_3_a(void){
  A|=0x08;
  SUB_CYCLE(8);
}

inline UINT8 set_4_b(void){
  B|=0x10;
  SUB_CYCLE(8);
}

inline UINT8 set_4_c(void){
  C|=0x10;
  SUB_CYCLE(8);
}

inline UINT8 set_4_d(void){
  D|=0x10;
  SUB_CYCLE(8);
}

inline UINT8 set_4_e(void){
  E|=0x10;
  SUB_CYCLE(8);
}

inline UINT8 set_4_h(void){
  H|=0x10;
  SUB_CYCLE(8);
}

inline UINT8 set_4_l(void){
  L|=0x10;
  SUB_CYCLE(8);
}

inline UINT8 set_4_mem_hl(void){
  UINT8 v=mem_read(HL);
  v|=0x10;
  mem_write(HL,v);
  SUB_CYCLE(16);
}

inline UINT8 set_4_a(void){
  A|=0x10;
  SUB_CYCLE(8);
}

inline UINT8 set_5_b(void){
  B|=0x20;
  SUB_CYCLE(8);
}

inline UINT8 set_5_c(void){
  C|=0x20;
  SUB_CYCLE(8);
}

inline UINT8 set_5_d(void){
  D|=0x20;
  SUB_CYCLE(8);
}

inline UINT8 set_5_e(void){
  E|=0x20;
  SUB_CYCLE(8);
}

inline UINT8 set_5_h(void){
  H|=0x20;
  SUB_CYCLE(8);
}

inline UINT8 set_5_l(void){
  L|=0x20;
  SUB_CYCLE(8);
}

inline UINT8 set_5_mem_hl(void){
  UINT8 v=mem_read(HL);
  v|=0x20;
  mem_write(HL,v);
  SUB_CYCLE(16);
}

inline UINT8 set_5_a(void){
  A|=0x20;
  SUB_CYCLE(8);
}

inline UINT8 set_6_b(void){
  B|=0x40;
  SUB_CYCLE(8);
}

inline UINT8 set_6_c(void){
  C|=0x40;
  SUB_CYCLE(8);
}

inline UINT8 set_6_d(void){
  D|=0x40;
  SUB_CYCLE(8);
}

inline UINT8 set_6_e(void){
  E|=0x40;
  SUB_CYCLE(8);
}

inline UINT8 set_6_h(void){
  H|=0x40;
  SUB_CYCLE(8);
}

inline UINT8 set_6_l(void){
  L|=0x40;
  SUB_CYCLE(8);
}

inline UINT8 set_6_mem_hl(void){
  UINT8 v=mem_read(HL);
  v|=0x40;
  mem_write(HL,v);
  SUB_CYCLE(16);
}

inline UINT8 set_6_a(void){
  A|=0x40;
  SUB_CYCLE(8);
}

inline UINT8 set_7_b(void){
  B|=0x80;
  SUB_CYCLE(8);
}

inline UINT8 set_7_c(void){
  C|=0x80;
  SUB_CYCLE(8);
}

inline UINT8 set_7_d(void){
  D|=0x80;
  SUB_CYCLE(8);
}

inline UINT8 set_7_e(void){
  E|=0x80;
  SUB_CYCLE(8);
}

inline UINT8 set_7_h(void){
  H|=0x80;
  SUB_CYCLE(8);
}

inline UINT8 set_7_l(void){
  L|=0x80;
  SUB_CYCLE(8);
}

inline UINT8 set_7_mem_hl(void){
  UINT8 v=mem_read(HL);
  v|=0x80;
  mem_write(HL,v);
  SUB_CYCLE(16);
}

inline UINT8 set_7_a(void){
  A|=0x80;
  SUB_CYCLE(8);
}

inline UINT8 cb_inst(void){
  switch(GET_BYTE) {
  case 0x00: return rlc_b();
  case 0x01: return rlc_c();
  case 0x02: return rlc_d();
  case 0x03: return rlc_e();
  case 0x04: return rlc_h();
  case 0x05: return rlc_l();
  case 0x06: return rlc_mem_hl();
  case 0x07: return rlc_a();
  case 0x08: return rrc_b();
  case 0x09: return rrc_c();
  case 0x0a: return rrc_d();
  case 0x0b: return rrc_e();
  case 0x0c: return rrc_h();
  case 0x0d: return rrc_l();
  case 0x0e: return rrc_mem_hl();
  case 0x0f: return rrc_a();
  case 0x10: return rl_b();
  case 0x11: return rl_c();
  case 0x12: return rl_d();
  case 0x13: return rl_e();
  case 0x14: return rl_h();
  case 0x15: return rl_l();
  case 0x16: return rl_mem_hl();
  case 0x17: return rl_a();
  case 0x18: return rr_b();
  case 0x19: return rr_c();
  case 0x1a: return rr_d();
  case 0x1b: return rr_e();
  case 0x1c: return rr_h();
  case 0x1d: return rr_l();
  case 0x1e: return rr_mem_hl();
  case 0x1f: return rr_a();
  case 0x20: return sla_b();
  case 0x21: return sla_c();
  case 0x22: return sla_d();
  case 0x23: return sla_e();
  case 0x24: return sla_h();
  case 0x25: return sla_l();
  case 0x26: return sla_mem_hl();
  case 0x27: return sla_a();
  case 0x28: return sra_b();
  case 0x29: return sra_c();
  case 0x2a: return sra_d();
  case 0x2b: return sra_e();
  case 0x2c: return sra_h();
  case 0x2d: return sra_l();
  case 0x2e: return sra_mem_hl();
  case 0x2f: return sra_a();
  case 0x30: return swap_b();
  case 0x31: return swap_c();
  case 0x32: return swap_d();
  case 0x33: return swap_e();
  case 0x34: return swap_h();
  case 0x35: return swap_l();
  case 0x36: return swap_mem_hl();
  case 0x37: return swap_a();
  case 0x38: return srl_b();
  case 0x39: return srl_c();
  case 0x3a: return srl_d();
  case 0x3b: return srl_e();
  case 0x3c: return srl_h();
  case 0x3d: return srl_l();
  case 0x3e: return srl_mem_hl();
  case 0x3f: return srl_a();
  case 0x40: return bit_0_b();
  case 0x41: return bit_0_c();
  case 0x42: return bit_0_d();
  case 0x43: return bit_0_e();
  case 0x44: return bit_0_h();
  case 0x45: return bit_0_l();
  case 0x46: return bit_0_mem_hl();
  case 0x47: return bit_0_a();
  case 0x48: return bit_1_b();
  case 0x49: return bit_1_c();
  case 0x4a: return bit_1_d();
  case 0x4b: return bit_1_e();
  case 0x4c: return bit_1_h();
  case 0x4d: return bit_1_l();
  case 0x4e: return bit_1_mem_hl();
  case 0x4f: return bit_1_a();
  case 0x50: return bit_2_b();
  case 0x51: return bit_2_c();
  case 0x52: return bit_2_d();
  case 0x53: return bit_2_e();
  case 0x54: return bit_2_h();
  case 0x55: return bit_2_l();
  case 0x56: return bit_2_mem_hl();
  case 0x57: return bit_2_a();
  case 0x58: return bit_3_b();
  case 0x59: return bit_3_c();
  case 0x5a: return bit_3_d();
  case 0x5b: return bit_3_e();
  case 0x5c: return bit_3_h();
  case 0x5d: return bit_3_l();
  case 0x5e: return bit_3_mem_hl();
  case 0x5f: return bit_3_a();
  case 0x60: return bit_4_b();
  case 0x61: return bit_4_c();
  case 0x62: return bit_4_d();
  case 0x63: return bit_4_e();
  case 0x64: return bit_4_h();
  case 0x65: return bit_4_l();
  case 0x66: return bit_4_mem_hl();
  case 0x67: return bit_4_a();
  case 0x68: return bit_5_b();
  case 0x69: return bit_5_c();
  case 0x6a: return bit_5_d();
  case 0x6b: return bit_5_e();
  case 0x6c: return bit_5_h();
  case 0x6d: return bit_5_l();
  case 0x6e: return bit_5_mem_hl();
  case 0x6f: return bit_5_a();
  case 0x70: return bit_6_b();
  case 0x71: return bit_6_c();
  case 0x72: return bit_6_d();
  case 0x73: return bit_6_e();
  case 0x74: return bit_6_h();
  case 0x75: return bit_6_l();
  case 0x76: return bit_6_mem_hl();
  case 0x77: return bit_6_a();
  case 0x78: return bit_7_b();
  case 0x79: return bit_7_c();
  case 0x7a: return bit_7_d();
  case 0x7b: return bit_7_e();
  case 0x7c: return bit_7_h();
  case 0x7d: return bit_7_l();
  case 0x7e: return bit_7_mem_hl();
  case 0x7f: return bit_7_a();
  case 0x80: return res_0_b();
  case 0x81: return res_0_c();
  case 0x82: return res_0_d();
  case 0x83: return res_0_e();
  case 0x84: return res_0_h();
  case 0x85: return res_0_l();
  case 0x86: return res_0_mem_hl();
  case 0x87: return res_0_a();
  case 0x88: return res_1_b();
  case 0x89: return res_1_c();
  case 0x8a: return res_1_d();
  case 0x8b: return res_1_e();
  case 0x8c: return res_1_h();
  case 0x8d: return res_1_l();
  case 0x8e: return res_1_mem_hl();
  case 0x8f: return res_1_a();
  case 0x90: return res_2_b();
  case 0x91: return res_2_c();
  case 0x92: return res_2_d();
  case 0x93: return res_2_e();
  case 0x94: return res_2_h();
  case 0x95: return res_2_l();
  case 0x96: return res_2_mem_hl();
  case 0x97: return res_2_a();
  case 0x98: return res_3_b();
  case 0x99: return res_3_c();
  case 0x9a: return res_3_d();
  case 0x9b: return res_3_e();
  case 0x9c: return res_3_h();
  case 0x9d: return res_3_l();
  case 0x9e: return res_3_mem_hl();
  case 0x9f: return res_3_a();
  case 0xa0: return res_4_b();
  case 0xa1: return res_4_c();
  case 0xa2: return res_4_d();
  case 0xa3: return res_4_e();
  case 0xa4: return res_4_h();
  case 0xa5: return res_4_l();
  case 0xa6: return res_4_mem_hl();
  case 0xa7: return res_4_a();
  case 0xa8: return res_5_b();
  case 0xa9: return res_5_c();
  case 0xaa: return res_5_d();
  case 0xab: return res_5_e();
  case 0xac: return res_5_h();
  case 0xad: return res_5_l();
  case 0xae: return res_5_mem_hl();
  case 0xaf: return res_5_a();
  case 0xb0: return res_6_b();
  case 0xb1: return res_6_c();
  case 0xb2: return res_6_d();
  case 0xb3: return res_6_e();
  case 0xb4: return res_6_h();
  case 0xb5: return res_6_l();
  case 0xb6: return res_6_mem_hl();
  case 0xb7: return res_6_a();
  case 0xb8: return res_7_b();
  case 0xb9: return res_7_c();
  case 0xba: return res_7_d();
  case 0xbb: return res_7_e();
  case 0xbc: return res_7_h();
  case 0xbd: return res_7_l();
  case 0xbe: return res_7_mem_hl();
  case 0xbf: return res_7_a();
  case 0xc0: return set_0_b();
  case 0xc1: return set_0_c();
  case 0xc2: return set_0_d();
  case 0xc3: return set_0_e();
  case 0xc4: return set_0_h();
  case 0xc5: return set_0_l();
  case 0xc6: return set_0_mem_hl();
  case 0xc7: return set_0_a();
  case 0xc8: return set_1_b();
  case 0xc9: return set_1_c();
  case 0xca: return set_1_d();
  case 0xcb: return set_1_e();
  case 0xcc: return set_1_h();
  case 0xcd: return set_1_l();
  case 0xce: return set_1_mem_hl();
  case 0xcf: return set_1_a();
  case 0xd0: return set_2_b();
  case 0xd1: return set_2_c();
  case 0xd2: return set_2_d();
  case 0xd3: return set_2_e();
  case 0xd4: return set_2_h();
  case 0xd5: return set_2_l();
  case 0xd6: return set_2_mem_hl();
  case 0xd7: return set_2_a();
  case 0xd8: return set_3_b();
  case 0xd9: return set_3_c();
  case 0xda: return set_3_d();
  case 0xdb: return set_3_e();
  case 0xdc: return set_3_h();
  case 0xdd: return set_3_l();
  case 0xde: return set_3_mem_hl();
  case 0xdf: return set_3_a();
  case 0xe0: return set_4_b();
  case 0xe1: return set_4_c();
  case 0xe2: return set_4_d();
  case 0xe3: return set_4_e();
  case 0xe4: return set_4_h();
  case 0xe5: return set_4_l();
  case 0xe6: return set_4_mem_hl();
  case 0xe7: return set_4_a();
  case 0xe8: return set_5_b();
  case 0xe9: return set_5_c();
  case 0xea: return set_5_d();
  case 0xeb: return set_5_e();
  case 0xec: return set_5_h();
  case 0xed: return set_5_l();
  case 0xee: return set_5_mem_hl();
  case 0xef: return set_5_a();
  case 0xf0: return set_6_b();
  case 0xf1: return set_6_c();
  case 0xf2: return set_6_d();
  case 0xf3: return set_6_e();
  case 0xf4: return set_6_h();
  case 0xf5: return set_6_l();
  case 0xf6: return set_6_mem_hl();
  case 0xf7: return set_6_a();
  case 0xf8: return set_7_b();
  case 0xf9: return set_7_c();
  case 0xfa: return set_7_d();
  case 0xfb: return set_7_e();
  case 0xfc: return set_7_h();
  case 0xfd: return set_7_l();
  case 0xfe: return set_7_mem_hl();
  case 0xff: return set_7_a();  
  }
  return 0;
}

inline UINT8 gbcpu_exec_one(void)
{
  /* FIXME */
  if (gbcpu->ei_flag==1) {
    gbcpu->int_flag=1;  
    gbcpu->ei_flag=0;
  }
  
  switch(mem_read(PC++)) {
  case 0x00: return nop();
  case 0x01: return ld_bc_nn();
  case 0x02: return ld_mem_bc_a();
  case 0x03: return inc_bc();
  case 0x04: return inc_b();
  case 0x05: return dec_b();
  case 0x06: return ld_b_n();
  case 0x07: return rlca();
  case 0x08: return ld_mem_nn_sp();
  case 0x09: return add_hl_bc();
  case 0x0a: return ld_a_mem_bc();
  case 0x0b: return dec_bc();
  case 0x0c: return inc_c();
  case 0x0d: return dec_c();
  case 0x0e: return ld_c_n();
  case 0x0f: return rrca();
  case 0x10: return stop();
  case 0x11: return ld_de_nn();
  case 0x12: return ld_mem_de_a();
  case 0x13: return inc_de();
  case 0x14: return inc_d();
  case 0x15: return dec_d();
  case 0x16: return ld_d_n();
  case 0x17: return rla();
  case 0x18: return jr_disp();
  case 0x19: return add_hl_de();
  case 0x1a: return ld_a_mem_de();
  case 0x1b: return dec_de();
  case 0x1c: return inc_e();
  case 0x1d: return dec_e();
  case 0x1e: return ld_e_n();
  case 0x1f: return rra();
  case 0x20: return jr_nz_disp();
  case 0x21: return ld_hl_nn();
  case 0x22: return ldi_mem_hl_a();
  case 0x23: return inc_hl();
  case 0x24: return inc_h();
  case 0x25: return dec_h();
  case 0x26: return ld_h_n();
  case 0x27: return daa();
  case 0x28: return jr_z_disp();
  case 0x29: return add_hl_hl();
  case 0x2a: return ldi_a_mem_hl();
  case 0x2b: return dec_hl();
  case 0x2c: return inc_l();
  case 0x2d: return dec_l();
  case 0x2e: return ld_l_n();
  case 0x2f: return cpl();
  case 0x30: return jr_nc_disp();
  case 0x31: return ld_sp_nn();
  case 0x32: return ldd_mem_hl_a();
  case 0x33: return inc_sp();
  case 0x34: return inc_mem_hl();
  case 0x35: return dec_mem_hl();
  case 0x36: return ld_mem_hl_n();
  case 0x37: return scf();
  case 0x38: return jr_c_disp();
  case 0x39: return add_hl_sp();
  case 0x3a: return ldd_a_mem_hl();
  case 0x3b: return dec_sp();
  case 0x3c: return inc_a();
  case 0x3d: return dec_a();
  case 0x3e: return ld_a_n();
  case 0x3f: return ccf();
  case 0x40: return ld_b_b();
  case 0x41: return ld_b_c();
  case 0x42: return ld_b_d();
  case 0x43: return ld_b_e();
  case 0x44: return ld_b_h();
  case 0x45: return ld_b_l();
  case 0x46: return ld_b_mem_hl();
  case 0x47: return ld_b_a();
  case 0x48: return ld_c_b();
  case 0x49: return ld_c_c();
  case 0x4a: return ld_c_d();
  case 0x4b: return ld_c_e();
  case 0x4c: return ld_c_h();
  case 0x4d: return ld_c_l();
  case 0x4e: return ld_c_mem_hl();
  case 0x4f: return ld_c_a();
  case 0x50: return ld_d_b();
  case 0x51: return ld_d_c();
  case 0x52: return ld_d_d();
  case 0x53: return ld_d_e();
  case 0x54: return ld_d_h();
  case 0x55: return ld_d_l();
  case 0x56: return ld_d_mem_hl();
  case 0x57: return ld_d_a();
  case 0x58: return ld_e_b();
  case 0x59: return ld_e_c();
  case 0x5a: return ld_e_d();
  case 0x5b: return ld_e_e();
  case 0x5c: return ld_e_h();
  case 0x5d: return ld_e_l();
  case 0x5e: return ld_e_mem_hl();
  case 0x5f: return ld_e_a();
  case 0x60: return ld_h_b();
  case 0x61: return ld_h_c();
  case 0x62: return ld_h_d();
  case 0x63: return ld_h_e();
  case 0x64: return ld_h_h();
  case 0x65: return ld_h_l();
  case 0x66: return ld_h_mem_hl();
  case 0x67: return ld_h_a();
  case 0x68: return ld_l_b();
  case 0x69: return ld_l_c();
  case 0x6a: return ld_l_d();
  case 0x6b: return ld_l_e();
  case 0x6c: return ld_l_h();
  case 0x6d: return ld_l_l();
  case 0x6e: return ld_l_mem_hl();
  case 0x6f: return ld_l_a();
  case 0x70: return ld_mem_hl_b();
  case 0x71: return ld_mem_hl_c();
  case 0x72: return ld_mem_hl_d();
  case 0x73: return ld_mem_hl_e();
  case 0x74: return ld_mem_hl_h();
  case 0x75: return ld_mem_hl_l();
  case 0x76: return halt();
  case 0x77: return ld_mem_hl_a();
  case 0x78: return ld_a_b();
  case 0x79: return ld_a_c();
  case 0x7a: return ld_a_d();
  case 0x7b: return ld_a_e();
  case 0x7c: return ld_a_h();
  case 0x7d: return ld_a_l();
  case 0x7e: return ld_a_mem_hl();
  case 0x7f: return ld_a_a();
  case 0x80: return add_a_b();
  case 0x81: return add_a_c();
  case 0x82: return add_a_d();
  case 0x83: return add_a_e();
  case 0x84: return add_a_h();
  case 0x85: return add_a_l();
  case 0x86: return add_a_mem_hl();
  case 0x87: return add_a_a();
  case 0x88: return adc_a_b();
  case 0x89: return adc_a_c();
  case 0x8a: return adc_a_d();
  case 0x8b: return adc_a_e();
  case 0x8c: return adc_a_h();
  case 0x8d: return adc_a_l();
  case 0x8e: return adc_a_mem_hl();
  case 0x8f: return adc_a_a();
  case 0x90: return sub_b();
  case 0x91: return sub_c();
  case 0x92: return sub_d();
  case 0x93: return sub_e();
  case 0x94: return sub_h();
  case 0x95: return sub_l();
  case 0x96: return sub_mem_hl();
  case 0x97: return sub_a();
  case 0x98: return sbc_a_b();
  case 0x99: return sbc_a_c();
  case 0x9a: return sbc_a_d();
  case 0x9b: return sbc_a_e();
  case 0x9c: return sbc_a_h();
  case 0x9d: return sbc_a_l();
  case 0x9e: return sbc_a_mem_hl();
  case 0x9f: return sbc_a_a();
  case 0xa0: return and_b();
  case 0xa1: return and_c();
  case 0xa2: return and_d();
  case 0xa3: return and_e();
  case 0xa4: return and_h();
  case 0xa5: return and_l();
  case 0xa6: return and_mem_hl();
  case 0xa7: return and_a();
  case 0xa8: return xor_b();
  case 0xa9: return xor_c();
  case 0xaa: return xor_d();
  case 0xab: return xor_e();
  case 0xac: return xor_h();
  case 0xad: return xor_l();
  case 0xae: return xor_mem_hl();
  case 0xaf: return xor_a();
  case 0xb0: return or_b();
  case 0xb1: return or_c();
  case 0xb2: return or_d();
  case 0xb3: return or_e();
  case 0xb4: return or_h();
  case 0xb5: return or_l();
  case 0xb6: return or_mem_hl();
  case 0xb7: return or_a();
  case 0xb8: return cp_b();
  case 0xb9: return cp_c();
  case 0xba: return cp_d();
  case 0xbb: return cp_e();
  case 0xbc: return cp_h();
  case 0xbd: return cp_l();
  case 0xbe: return cp_mem_hl();
  case 0xbf: return cp_a();
  case 0xc0: return ret_nz();
  case 0xc1: return pop_bc();
  case 0xc2: return jp_nz_nn();
  case 0xc3: return jp_nn();
  case 0xc4: return call_nz_nn();
  case 0xc5: return push_bc();
  case 0xc6: return add_a_n();
  case 0xc7: return rst_00h();
  case 0xc8: return ret_z();
  case 0xc9: return ret();
  case 0xca: return jp_z_nn();
  case 0xcb: return cb_inst();
  case 0xcc: return call_z_nn();
  case 0xcd: return call_nn();
  case 0xce: return adc_a_n();
  case 0xcf: return rst_8h();
  case 0xd0: return ret_nc();
  case 0xd1: return pop_de();
  case 0xd2: return jp_nc_nn();
  case 0xd3: return unknown();
  case 0xd4: return call_nc_nn();
  case 0xd5: return push_de();
  case 0xd6: return sub_n();
  case 0xd7: return rst_10h();
  case 0xd8: return ret_c();
  case 0xd9: return reti();
  case 0xda: return jp_c_nn();
  case 0xdb: return unknown();
  case 0xdc: return call_c_nn();
  case 0xdd: return unknown();
  case 0xde: return sbc_a_n();
  case 0xdf: return rst_18h();
  case 0xe0: return ld_mem_ff00_n_a();
  case 0xe1: return pop_hl();
  case 0xe2: return ld_mem_ff00_c_a();
  case 0xe3: return unknown();
  case 0xe4: return unknown();
  case 0xe5: return push_hl();
  case 0xe6: return and_n();
  case 0xe7: return rst_20h();
  case 0xe8: return add_sp_dd();
  case 0xe9: return jp_mem_hl();
  case 0xea: return ld_mem_nn_a();
  case 0xeb: return unknown();
  case 0xec: return unknown();
  case 0xed: return unknown();
  case 0xee: return xor_n();
  case 0xef: return rst_28h();
  case 0xf0: return ld_a_mem_ff00_n();
  case 0xf1: return pop_af();
  case 0xf2: return ld_a_mem_c();
  case 0xf3: return di();
  case 0xf4: return unknown();
  case 0xf5: return push_af();
  case 0xf6: return or_n();
  case 0xf7: return rst_30h();
  case 0xf8: return ld_hl_sp_dd();
  case 0xf9: return ld_sp_hl();
  case 0xfa: return ld_a_mem_nn();
  case 0xfb: return ei();
  case 0xfc: return unknown();
  case 0xfd: return unknown();
  case 0xfe: return cp_n();
  case 0xff: return rst_38h();
  }
  return 0;
}

inline void rom_timer_inc(void) {
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

inline void update_gb(void) {
  static UINT8 a;
  static UINT32 divid_cycle;
  static UINT32 key_cycle;
  //static INT16 serial_cycle;
  int v=0;

  //#ifndef DEBUG
  do {
    //#endif
    
    v=0;

    //if (gbcpu->state==HALT_STATE) halt_update();
    if (INT_FLAG&VBLANK_INT)  v=make_interrupt(VBLANK_INT);
    if (INT_FLAG&LCDC_INT && !v) v=make_interrupt(LCDC_INT);
    if (INT_FLAG&TIMEOWFL_INT && !v) v=make_interrupt(TIMEOWFL_INT); 
    if (INT_FLAG&SERIAL_INT && !v) v=make_interrupt(SERIAL_INT); 

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
      
      if (gbcpu->state!=HALT_STATE) 
	a=gbcpu_exec_one();
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
    
    if (LCDCCONT&0x80) 
      GBLCDC_ADD_CYCLE(a);
    
    if (TIME_CONTROL&0x04) {
      gbtimer->cycle+=a;
      if (gbtimer->cycle>=gbtimer->clk_inc) {
	gbtimer_update();
	gbtimer->cycle-=gbtimer->clk_inc;
      }
    }

    if (rom_type&TIMER && rom_timer->reg[4]&0x40) {
      rom_timer->cycle+=a;
      if (rom_timer->cycle>111) {
	rom_timer_inc();
	rom_timer->cycle=0;
      }
    }
    /* FIXME: serial is experimentale */

    /*if (conf.serial_on) {
      if (serial_cycle_todo<=0 && (SC&0x80)) {
	int n;
	if ((n=recept_byte())>=0) {
	  if (!(SC&0x01)) send_byte(SB);
	  SB=(UINT8)n;
	  SC&=0x7f;
	  set_interrupt(SERIAL_INT);
	}
      } else serial_cycle_todo-=a;
      }*/
    
    //if (gbcpu->state==HALT_STATE) halt_update();
    /*if (INT_FLAG&VBLANK_INT)  v=make_interrupt(VBLANK_INT);
      if (INT_FLAG&LCDC_INT && !v) v=make_interrupt(LCDC_INT);
      if (INT_FLAG&TIMEOWFL_INT && !v) v=make_interrupt(TIMEOWFL_INT); 
      if (INT_FLAG&SERIAL_INT && !v) v=make_interrupt(SERIAL_INT); */

    key_cycle+=a;
    if (key_cycle>=gblcdc->vblank_cycle) {
      update_key();
      key_cycle=0;
    }
#ifndef DEBUG
  } while(!conf.gb_done);
#else
  } while(!conf.gb_done && continue_run());
#endif

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
