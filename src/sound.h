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

Uint32 sample_rate;
Uint8  bit_per_sample;
Uint16 sample_per_update;

typedef struct SoundM1
{
  /* NR10 */
  Uint8 swp_time;  // -OOO----
  Uint8 swp_dir;   // ----O---
  Uint8 swp_shift; // -----OOO

  /* NR11 */
  Uint8 duty;      // OO------
  Uint8 len;       // --OOOOOO -> Write only

  /* NR12 */
  Uint8 env_start; // OOOO----
  Uint8 env_dir;   // ----O---
  Uint8 env_swp;   // -----OOO

  /* NR13 */
  Uint8 freq_lo;     // OOOOOOOO
  
  /* NR14 */
  Uint8 initial;   // O------- -> Write only
  Uint8 mode;      // -O------
  Uint8 freq_hi;   // -----OOO -> Write only

  Uint8 env_changed;
  Uint16 freq;    // freq_lo+(freq_hi<<8)
  float sample;     // Periode en samples
  float sample_dut; // Wave Duty en samples
  Uint16 sample_len; // durée en samples

  float sample_sweep_time;
  float env_per_step;
  float sample_env_per_step;
  float cp;
}SoundM1;
SoundM1 snd_m1;

typedef struct SoundM2
{
  /* NR21 */
  Uint8 duty;      // OO------
  Uint8 len;       // --OOOOOO -> Write only

  /* NR22 */
  Uint8 env_start; // OOOO----
  Uint8 env_dir;   // ----O---
  Uint8 env_swp;   // -----OOO

  /* NR23 */
  Uint8 freq_lo;     // OOOOOOOO
  
  /* NR24 */
  Uint8 initial;   // O------- -> Write only
  Uint8 mode;      // -O------
  Uint8 freq_hi;   // -----OOO -> Write only

  Uint8 env_changed;
  Uint16 freq;    // freq_lo+(freq_hi<<8)
  float sample;     // Periode en samples
  float sample_dut; // Wave Duty en samples
  Uint16 sample_len; // durée en samples

  float env_per_step;
  float sample_env_per_step;
  float cp;
}SoundM2;
SoundM2 snd_m2;

typedef struct SoundM3
{
  /* NR30 */
  Uint8 is_on;     // O-------
  
  /* NR31 */
  Uint8 len;         // OOOOOOOO 

  /* NR32 */
  Uint8 level;     // -OO-----

  /* NR33 */
  Uint8 freq_lo;     // OOOOOOOO
  
  /* NR34 */
  Uint8 initial;   // O------- -> Write only
  Uint8 mode;      // -O------
  Uint8 freq_hi;   // -----OOO -> Write only

  Uint8 wave[16];    // Wave Pattern RAM

  Uint16 freq;    // freq_lo+(freq_hi<<8)
  float sample;     // Periode en samples
  Uint16 sample_len; // durée en samples
  float cp;
}SoundM3;
SoundM3 snd_m3;

typedef struct SoundM4
{
  /* NR41 */
  Uint8 len;       // --OOOOOO

  /* NR42 */
  Uint8 env_start; // OOOO----
  Uint8 env_dir;   // ----O---
  Uint8 env_swp;   // -----OOO

  /* NR43 */
  Uint8 poly;

  /* NR14 */
  Uint8 initial;   // O------- -> Write only
  Uint8 mode;      // -O------
 

  Uint8 env_changed;
  Uint8 poly_changed;
  double sample;     // Periode en samples
  float freq;
  Uint16 sample_len; // durée en samples

  float env_per_step;
  float sample_env_per_step;
}SoundM4;
SoundM4 snd_m4;

/* Control general */
typedef struct SoundG
{
  /* NR50 */
  Uint8 SO1_OutputLevel ; // -----OOO
  Uint8 Vin_SO1         ; // ----O---
  Uint8 SO2_OutputLevel ; // -OOO----
  Uint8 Vin_SO2         ; // O-------

  /* NR51 */
  Uint8 Sound1_To_SO1   ;
  Uint8 Sound2_To_SO1   ;
  Uint8 Sound3_To_SO1   ;
  Uint8 Sound4_To_SO1   ;
  Uint8 Sound1_To_SO2   ;
  Uint8 Sound2_To_SO2   ;
  Uint8 Sound3_To_SO2   ;
  Uint8 Sound4_To_SO2   ;

  /* NR52 */
  Uint8 Sound_On_Off    ;
  Uint8 Sound1_On_Off   ;
  Uint8 Sound2_On_Off   ;
  Uint8 Sound3_On_Off   ;
  Uint8 Sound4_On_Off   ;
}SoundG;
SoundG snd_g;


#define LEFT  1
#define RIGHT 2

void update_gb_sound(float len);
int gbsound_init(void);
void close_sound(void);
void write_sound_reg(Uint16 add,Uint8 val);
Uint8 read_sound_reg(Uint16 add);
void update_sound_reg(void);


#endif







