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


#ifndef _SOUND_H
#define _SOUND_H

#include "global.h"

UINT32 sample_rate;
UINT8  bit_per_sample;
UINT16 sample_per_update;

typedef struct SoundM1
{
  /* NR10 */
  UINT8 swp_time:3;  // -OOO----
  UINT8 swp_dir:1;   // ----O---
  UINT8 swp_shift:3; // -----OOO

  /* NR11 */
  UINT8 duty:2;      // OO------
  UINT8 len:6;       // --OOOOOO -> Write only

  /* NR12 */
  UINT8 env_start:4; // OOOO----
  UINT8 env_dir:1;   // ----O---
  UINT8 env_swp:3;   // -----OOO

  /* NR13 */
  UINT8 freq_lo;     // OOOOOOOO
  
  /* NR14 */
  UINT8 initial:1;   // O------- -> Write only
  UINT8 mode:1;      // -O------
  UINT8 freq_hi:3;   // -----OOO -> Write only

  UINT8 env_changed:1;
  UINT16 freq:11;    // freq_lo+(freq_hi<<8)
  float sample;     // Periode en samples
  float sample_dut; // Wave Duty en samples
  UINT16 sample_len; // durée en samples

  float sample_sweep_time;
  float env_per_step;
  float sample_env_per_step;
  float cp;
}SoundM1;
SoundM1 snd_m1;

typedef struct SoundM2
{
  /* NR21 */
  UINT8 duty:2;      // OO------
  UINT8 len:6;       // --OOOOOO -> Write only

  /* NR22 */
  UINT8 env_start:4; // OOOO----
  UINT8 env_dir:1;   // ----O---
  UINT8 env_swp:3;   // -----OOO

  /* NR23 */
  UINT8 freq_lo;     // OOOOOOOO
  
  /* NR24 */
  UINT8 initial:1;   // O------- -> Write only
  UINT8 mode:1;      // -O------
  UINT8 freq_hi:3;   // -----OOO -> Write only

  UINT8 env_changed:1;
  UINT16 freq:11;    // freq_lo+(freq_hi<<8)
  float sample;     // Periode en samples
  float sample_dut; // Wave Duty en samples
  UINT16 sample_len; // durée en samples

  float env_per_step;
  float sample_env_per_step;
  float cp;
}SoundM2;
SoundM2 snd_m2;

typedef struct SoundM3
{
  /* NR30 */
  UINT8 is_on:1;     // O-------
  
  /* NR31 */
  UINT8 len;         // OOOOOOOO 

  /* NR32 */
  UINT8 level:2;     // -OO-----

  /* NR33 */
  UINT8 freq_lo;     // OOOOOOOO
  
  /* NR34 */
  UINT8 initial:1;   // O------- -> Write only
  UINT8 mode:1;      // -O------
  UINT8 freq_hi:3;   // -----OOO -> Write only

  UINT8 wave[16];    // Wave Pattern RAM

  UINT16 freq:11;    // freq_lo+(freq_hi<<8)
  float sample;     // Periode en samples
  UINT16 sample_len; // durée en samples
  float cp;
}SoundM3;
SoundM3 snd_m3;

typedef struct SoundM4
{
  /* NR41 */
  UINT8 len:6;       // --OOOOOO

  /* NR42 */
  UINT8 env_start:4; // OOOO----
  UINT8 env_dir:1;   // ----O---
  UINT8 env_swp:3;   // -----OOO

  /* NR43 */
  UINT8 poly;

  /* NR14 */
  UINT8 initial:1;   // O------- -> Write only
  UINT8 mode:1;      // -O------
 

  UINT8 env_changed:1;
  UINT8 poly_changed:1;
  double sample;     // Periode en samples
  float freq;
  UINT16 sample_len; // durée en samples

  float env_per_step;
  float sample_env_per_step;
}SoundM4;
SoundM4 snd_m4;

/* Control general */
typedef struct SoundG
{
  /* NR50 */
  UINT8 SO1_OutputLevel : 3; // -----OOO
  UINT8 Vin_SO1         : 1; // ----O---
  UINT8 SO2_OutputLevel : 3; // -OOO----
  UINT8 Vin_SO2         : 1; // O-------

  /* NR51 */
  UINT8 Sound1_To_SO1   : 1;
  UINT8 Sound2_To_SO1   : 1;
  UINT8 Sound3_To_SO1   : 1;
  UINT8 Sound4_To_SO1   : 1;
  UINT8 Sound1_To_SO2   : 1;
  UINT8 Sound2_To_SO2   : 1;
  UINT8 Sound3_To_SO2   : 1;
  UINT8 Sound4_To_SO2   : 1;

  /* NR52 */
  UINT8 Sound_On_Off    : 1;
  UINT8 Sound1_On_Off   : 1;
  UINT8 Sound2_On_Off   : 1;
  UINT8 Sound3_On_Off   : 1;
  UINT8 Sound4_On_Off   : 1;
}SoundG;
SoundG snd_g;


#define LEFT  1
#define RIGHT 2

void update_gb_sound(UINT32 len);
int init_sound(void);
void close_sound(void);
void write_sound_reg(UINT16 add,UINT8 val);
UINT8 read_sound_reg(UINT16 add);



#endif







