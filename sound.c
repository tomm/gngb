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


/* TODO : Rewrite the length code for evry channel */

#include <stdlib.h>
#include <SDL/SDL.h>
#include "sound.h"
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"

#define HZ(x) ((double)(131072.0)/(double)(2048-x))
#define HZ_M3(x) ((double)(4194304.0)/(64.0*(double)(2048-x)))
// #define LOG_SOUND

INT8 *playbuf;
FILE *fsound;
/*
INT8 vol_table[]={0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60};
INT8 snd3_tbl_100[]={-60,-52,-45,-37,-30,-22,-15,-7,7,15,22,30,37,45,52,60};
INT8 snd3_tbl_50[]={-30,-22,-15,-7,7,15,22,30};
INT8 snd3_tbl_25[]={-15,-7,7,15};
*/
INT8 vol_table[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
INT8 snd3_tbl_100[]={-16,-15,-12,-10,-8,-6,-4,-2,2,4,6,8,10,12,14,16};
INT8 snd3_tbl_50[]={-10,-8,-4,-2,2,4,8,10};
INT8 snd3_tbl_25[]={-4,-2,2,4};

UINT16 wduty[4][8] = 
{
  {0,0,-1,0,0,0,0,0 },
  {0,-1,-1,0,0,0,0,0 },
  {0,-1,-1,-1,-1,0,0,0 },
  {0,0,0,-1,-1,-1,-1,-1 }
};


double freq_table[2048];
double freq_table_m3[2048];
long double freq_table_m4[256];

UINT16 lastpos=0,curpos=0;
UINT32 buf_size;

UINT8 m1_len,m1_slen;
float wave_duty[]={8,4,2,1.3333};

/* retourne l'entier de l bit, debutant au bit d de v */
// #define BITS(v,d,l) ((v>>(d-l+1))&(l*l-1))


void write_sound_reg(UINT16 add,UINT8 val)
{
  // UINT32 snd_len =(UINT32)((float)get_nb_cycle()*(sample_rate/59.73)
  //			   /((gbcpu->mode== DOUBLE_SPEED)?140448.0:70224.0));
  UINT32 snd_len = (UINT32)((float)get_nb_cycle()*sample_rate/(float)((gbcpu->mode== DOUBLE_SPEED)?8388608.0:4194304.0));
  static float samp;
  // printf("%d\n",snd_len);
  if (snd_len) 
    update_gb_sound(snd_len<<1);

  if (add>=0xFF30 && add<=0xFF3F) { // Wave Pattern RAM
    if (!snd_m3.is_on) {
      snd_m3.wave[add-0xFF30]=val;
      // printf("WaveRam Write %x\n",add-0xFF30);
    }
    return;
  }

  switch(add) {
    /*--- GENERAL CONTROL ---*/
  case 0xFF24 : // NR50
    snd_g.SO1_OutputLevel=val&0x7;
    snd_g.SO2_OutputLevel=(val&0x70)>>4;
    snd_g.Vin_SO1=(val&0x8)>>3;
    snd_g.Vin_SO2=(val&0x80)>>7;
    // printf("outup %d %d \n",snd_g.SO1_OutputLevel,snd_g.SO2_OutputLevel);
    return;
    break;
  case 0xFF25 : // NR51
    snd_g.Sound4_To_SO2=(val&0x80)>>7;
    snd_g.Sound3_To_SO2=(val&0x40)>>6;
    snd_g.Sound2_To_SO2=(val&0x20)>>5;
    snd_g.Sound1_To_SO2=(val&0x10)>>4;
    snd_g.Sound4_To_SO1=(val&0x8)>>3;
    snd_g.Sound3_To_SO1=(val&0x4)>>2;
    snd_g.Sound2_To_SO1=(val&0x2)>>1;
    snd_g.Sound1_To_SO1=(val&0x1);
    break;
  case 0xFF26 : // NR52
    snd_g.Sound_On_Off=(val&0x80)>>7;
    /* le reste est read only (Sound Status bit) */
    break;


    /*--- SOUND MODE 1 ---*/
  case 0xFF10 : // NR10
    snd_m1.swp_time=(val&0x70)>>4;
    snd_m1.swp_dir=(val&0x8)>>3;
    snd_m1.swp_shift=val&0x7;
    snd_m1.sample_sweep_time=(float)sample_rate*snd_m1.swp_time/128.0;
    //    printf("sweep %d %d %d\n",snd_m1.swp_time,snd_m1.swp_dir,snd_m1.swp_shift);
    break;
  case 0xFF11 : // NR11
    snd_m1.duty=(val&0xC0)>>6;
    snd_m1.len=val&0x3F;
    snd_m1.sample_dut=(float)snd_m1.sample/wave_duty[snd_m1.duty];
    snd_m1.sample_len=(64.0-(float)snd_m1.len)*((float)sample_rate/256.0);
    break;
  case 0xFF12 : // NR12
    snd_m1.env_start=(val&0xF0)>>4;
    snd_m1.env_dir=(val&0x8)>>3;
    snd_m1.env_swp=(val&0x7);
    snd_m1.env_changed=1;
    snd_m1.sample_env_per_step=(float)snd_m1.env_swp*sample_rate/64.0;
    break;
  case 0xFF13 : // NR13
    snd_m1.freq_lo=val;
    snd_m1.freq=((UINT16)(snd_m1.freq_lo))|(((UINT16)(snd_m1.freq_hi))<<8); // update freq
    //samp=snd_m1.sample;
    snd_m1.sample=freq_table[snd_m1.freq];
    snd_m1.sample_dut=snd_m1.sample/wave_duty[snd_m1.duty];
    //snd_m1.cp=snd_m1.cp*snd_m1.sample/samp;
    break;
  case 0xFF14 : // NR14
    snd_m1.initial=(val&0x80)>>7;
    snd_m1.mode=(val&0x40)>>6;
    snd_m1.freq_hi=val&0x7;
    snd_m1.freq=((UINT16)(snd_m1.freq_lo))|(((UINT16)(snd_m1.freq_hi))<<8); // update freq
    //samp=snd_m1.sample;
    snd_m1.sample=freq_table[snd_m1.freq];
    snd_m1.sample_dut=(float)snd_m1.sample/wave_duty[snd_m1.duty];
    //snd_m1.cp=snd_m1.cp*snd_m1.sample/samp;
    break;

    /*--- SOUND MODE 2 ---*/
  case 0xFF16 : // NR21
    snd_m2.duty=(val&0xC0)>>6;
    snd_m2.len=val&0x3F;
    snd_m2.sample_dut=(float)snd_m2.sample/wave_duty[snd_m2.duty];
    snd_m2.sample_len=(64.0-(float)snd_m2.len)*((float)sample_rate/256.0);
    break;
  case 0xFF17 : // NR22
    snd_m2.env_start=(val&0xF0)>>4;
    snd_m2.env_dir=(val&0x8)>>3;
    snd_m2.env_swp=(val&0x7);
    snd_m2.env_changed=1;
    snd_m2.sample_env_per_step=(float)snd_m2.env_swp*sample_rate/64.0;
    break;
  case 0xFF18 : // NR23
    snd_m2.freq_lo=val;
    snd_m2.freq=((UINT16)(snd_m2.freq_lo))|(((UINT16)(snd_m2.freq_hi))<<8); // update freq
    //samp=snd_m2.sample;
    snd_m2.sample=freq_table[snd_m2.freq];
    //snd_m2.cp=snd_m2.cp*snd_m2.sample/samp;
    snd_m2.sample_dut=(float)snd_m2.sample/wave_duty[snd_m2.duty];
    break;
  case 0xFF19 : // NR24
    snd_m2.initial=(val&0x80)>>7;
    snd_m2.mode=(val&0x40)>>6;
    snd_m2.freq_hi=val&0x7;
    snd_m2.freq=((UINT16)(snd_m2.freq_lo))|(((UINT16)(snd_m2.freq_hi))<<8); // update freq
    //samp=snd_m2.sample;
    snd_m2.sample=freq_table[snd_m2.freq];
    snd_m2.sample_dut=(float)snd_m2.sample/wave_duty[snd_m2.duty];
    //snd_m2.cp=snd_m2.cp*snd_m2.sample/samp;
    break;

    /*--- Sound Mode 3 ---*/
  case 0xFF1A : // NR30
    snd_m3.is_on=(val&0x80)>>7;
    //    printf("snd3 on:%d\n",snd_m3.is_on);
    break;
  case 0xFF1B : // NR31
    snd_m3.len=val;
    // snd_m3.sample_len=(float)sample_rate*(256.0-snd_m3.len)/2.0;
    snd_m3.sample_len=(float)sample_rate*(256.0-snd_m3.len)*(1.0/256.0);
    // printf("SNDM3 LEN WRITE %d \n",snd_m3.len);
    break;
  case 0xFF1C : // NR32
    snd_m3.level=(val&0x60)>>5;
    //     printf("--level%d %x\n", snd_m3.level,val);
    break;
  case 0xFF1D : // NR33
    snd_m3.freq_lo=val;
    snd_m3.freq=((UINT16)(snd_m3.freq_lo))|(((UINT16)(snd_m3.freq_hi))<<8);
        samp=snd_m3.sample;
    snd_m3.sample=freq_table_m3[snd_m3.freq]/32.0;// *64.0;
    //    printf("sample %f\n",snd_m3.sample);
        snd_m3.cp=snd_m3.cp* snd_m3.sample/samp;
    break;
  case 0xFF1E : // NR34
    snd_m3.initial=(val&0x80)>>7;
    snd_m3.mode=(val&0x40)>>6;
    snd_m3.freq_hi=val&0x7;
    snd_m3.freq=((UINT16)(snd_m3.freq_lo))|(((UINT16)(snd_m3.freq_hi))<<8);
        samp=snd_m3.sample;
    snd_m3.sample=freq_table_m3[snd_m3.freq]/32.0;// *64.0;
    //    printf("sample %f\n",snd_m3.sample);
        snd_m3.cp=snd_m3.cp* snd_m3.sample/samp;
    break;

    /*--- SOUND MODE 4 ---*/
  case 0xFF20 : // NR41
    snd_m4.len=val&0x3F;
    snd_m4.sample_len=(64.0-(float)snd_m4.len)*((float)sample_rate/256.0);
    break;
  case 0xFF21 : // NR42
    snd_m4.env_start=(val&0xF0)>>4;
    snd_m4.env_dir=(val&0x8)>>3;
    snd_m4.env_swp=(val&0x7);
    snd_m4.env_changed=1;
    snd_m4.sample_env_per_step=(float)snd_m4.env_swp*sample_rate/64.0;
    break;
  case 0xFF22 : // NR43
    snd_m4.poly=val;
    snd_m4.sample=(double)sample_rate*freq_table_m4[val];
    //    printf("poly %x:%x %x %x %g\n",val,(val&0xF0)>>4,(val&0x8)>>3,val&0x7,snd_m4.sample);
    snd_m4.poly_changed=1;
    break;
  case 0xFF23 : // NR44
    snd_m4.initial=(val&0x80)>>7;
    snd_m4.mode=(val&0x40)>>6;
    // printf("env mode %x:%d %d\n",val,snd_m4.initial,snd_m4.mode);
    break;

    

    
  }
  

}
   
UINT8 read_sound_reg(UINT16 add)
{
  if (add>=0xFF30 && add<=0xFF3F) // Wave Pattern RAM
    return snd_m3.wave[add-0xFF30];

  switch(add) {
    
    /*--- SOUND MODE 1 ---*/
  case 0xFF10 : // NR10
    return (snd_m1.swp_time<<4)|(snd_m1.swp_dir<<3)|snd_m1.swp_shift;
  case 0xFF11 : // NR11
    return (snd_m1.duty<<6);
  case 0xFF12 : // NR12
    return (snd_m1.env_start<<4)|(snd_m1.env_dir<<3)|snd_m1.env_swp;
  case 0xFF13 : // NR13
    return snd_m1.freq_lo;
  case 0xFF14 : // NR14
    return snd_m1.mode<<6;

    /*--- SOUND MODE 1 ---*/
  case 0xFF16 : // NR21
    return (snd_m2.duty<<6);
  case 0xFF17 : // NR22
    return (snd_m2.env_start<<4)|(snd_m2.env_dir<<3)|snd_m2.env_swp;
  case 0xFF18 : // NR23
    return snd_m2.freq_lo;
  case 0xFF19 : // NR24
    return snd_m2.mode<<6;

    /*--- SOUND MODE 3 ---*/
  case 0xFF1A : // NR30
    // printf("read snd3 on\n");
    return snd_m3.is_on<<7;
  case 0xFF1B : // NR31
    return snd_m3.len;
  case 0xFF1C : // NR32
    return snd_m3.level<<5;
  case 0xFF1D : // NR33
    return snd_m3.freq_lo;
  case 0xFF1E : // NR34
    return snd_m3.mode<<6;

    /*--- SOUND MODE 4 ---*/
  case 0xFF20 : // NR41
    return snd_m4.len;
  case 0xFF21 : // NR42
    //    printf("sndm4 read env\n");
    return (snd_m4.env_start<<4)|(snd_m4.env_dir<<3)|snd_m4.env_swp;
  case 0xFF22 : // NR43
    return snd_m4.poly;
  case 0xFF23 : // NR44
    return snd_m4.mode<<6;

    /*--- GENERAL CONTROL ---*/
  case 0xFF24 : // NR50
    return (snd_g.Vin_SO2<<7)|(snd_g.SO2_OutputLevel<<4)|(snd_g.Vin_SO1<<3)|snd_g.SO2_OutputLevel;
  case 0xFF25 : // NR51
    return (snd_g.Sound4_To_SO2<<7)|
      (snd_g.Sound3_To_SO2<<6)|
      (snd_g.Sound2_To_SO2<<5)|
      (snd_g.Sound1_To_SO2<<4)|
      (snd_g.Sound4_To_SO1<<3)|
      (snd_g.Sound3_To_SO1<<2)|
      (snd_g.Sound2_To_SO1<<1)|
      snd_g.Sound1_To_SO1;
  case 0xFF26 : // NR52
    return (snd_g.Sound_On_Off<<7)|
      (snd_g.Sound4_On_Off<<3)|
      (snd_g.Sound3_On_Off<<3)|
      (snd_g.Sound2_On_Off<<3)|
      snd_g.Sound1_On_Off;
  }
  return 0;
}
      
  

void init_freq_table(void)
{
  int i;
  long double y;

  for(i=0;i<2048;i++) {
    freq_table[i]=sample_rate/HZ(i);
    freq_table_m3[i]=sample_rate/HZ_M3(i);
  }

  for(i=0;i<256;i++) {
    if ((i&0x7)==0)
      y=(long double)2.0;
    else
      y=(long double)1.0/(float)(i&0x7);
    freq_table_m4[i]=1.0/(4194304.0*(1.0/8.0)*y*(1.0/(long double)(2<<(i>>4))));
  }
}


inline INT8 update_snd_m1(void)
{
  static float cp=0;
  static float samp;
  static int env=0;
  static char sp=0;
  static int lp=0; 
  static int ep=0;
  static int fp=0;
  static int cur_swp_shift;
  static int cur_env_step;

  if (snd_m1.initial) {
    snd_m1.initial=0;
    env=snd_m1.env_start;
    cur_env_step=snd_m1.env_swp;
    cur_swp_shift=snd_m1.swp_shift;
    snd_m1.cp=0;
    //    sp=0;
    lp=0;
    ep=0;
    fp=0;
  }

  if (snd_m1.env_changed && snd_m1.mode==0) {
    snd_m1.env_changed=0;
    env=snd_m1.env_start;
    cur_env_step=snd_m1.env_swp;
    cur_swp_shift=snd_m1.swp_shift;
    snd_m1.cp=0;
    //    sp=0;
    ep=0;
    fp=0;
  }


  if (snd_m1.mode==1) { // on doit gerer la durée
    // printf("Mode 1\n");
    lp++;
    if (lp>snd_m1.sample_len) {
      snd_g.Sound1_On_Off=0;return 0;
    }
  }

  if (snd_m1.swp_time!=0) { // FreqSweep operation
    fp++;
    if (fp>snd_m1.sample_sweep_time) {
      fp=0;
      //samp=snd_m1.sample;
      if (snd_m1.swp_dir==1) {
	snd_m1.freq -= snd_m1.freq>>snd_m1.swp_shift;
	snd_m1.sample=freq_table[snd_m1.freq];
	snd_m1.sample_dut=snd_m1.sample/wave_duty[snd_m1.duty];
      } else {
	snd_m1.freq += snd_m1.freq>>snd_m1.swp_shift;
	snd_m1.sample=freq_table[snd_m1.freq];
	snd_m1.sample_dut=snd_m1.sample/wave_duty[snd_m1.duty];
      }
      //snd_m1.cp=snd_m1.cp*snd_m1.sample/samp;
      if (snd_m1.freq==0)
	snd_m1.freq=0;

      if (snd_m1.freq>=2020) {
	snd_g.Sound1_On_Off=0;env=0;return 0;
      }
    }

  }

  if (cur_env_step!=0 ) {
    if ((env>0 && snd_m1.env_dir==0) ||
	(env<15 && snd_m1.env_dir==1)) {
      ep++;
      if ((float)ep>snd_m1.sample_env_per_step) {
	ep=0;
	// cur_env_step--;
	if (snd_m1.env_dir==0) 
	  env-=1;//snd_m1.env_per_step;
	else
	  env+=1;//snd_m1.env_per_step;
      
	if (env==0 && snd_m1.env_dir==0) {
	  snd_g.Sound1_On_Off=0;env=0;return 0;
	}
      }
    }
  }
  if (env==0) {snd_g.Sound1_On_Off=0;return 0;} // Volume à zero
  snd_g.Sound1_On_Off=1;

  /* clipping */
  if (env>15) env=15;
  if (env<0) env=0;
  /*  
  sp=wduty[snd_m1.duty][((int)(snd_m1.cp/(snd_m1.sample/8.0)))&0x7];
  snd_m1.cp+=1;
  return vol_table[env]&sp;
  */

   
  if (snd_m1.cp+1.0>snd_m1.sample) {
    sp=1;//1-sp;
    // printf("- %f\n",snd_m1.cp);
    snd_m1.cp=(snd_m1.cp+1.0)-snd_m1.sample;
  } else if (snd_m1.cp+1.0>snd_m1.sample_dut && sp==1) {
    sp=0;//1-sp;
    // printf("%f\n",snd_m1.cp);
     snd_m1.cp+=1.0;// snd_m1.cp-snd_m1.sample_dut;
  } else
    snd_m1.cp+=1.0;
  
  if (sp)
    return -vol_table[env];
  else
    return vol_table[env];
  

}

inline INT8 update_snd_m2(void)
{
  //  static float cp=0;
  static int env=0;
  static int sp=0;
  static int lp=0; 
  static int ep=0;
  static int cur_env_step;

  if (snd_m2.initial) {
    snd_m2.initial=0;
    env=snd_m2.env_start;
    cur_env_step=snd_m2.env_swp;
    snd_m2.cp=0;
    //    sp=0;
    lp=0;
    ep=0;
  }

  if (snd_m2.env_changed && snd_m2.mode==0) {
    snd_m2.env_changed=0;
    env=snd_m2.env_start;
    cur_env_step=snd_m2.env_swp;
    snd_m2.cp=0;
    //    sp=0;
    ep=0;
  }


  if (snd_m2.mode==1) { // on doit gerer la durée
    // printf("Mode 1\n");
    lp++;
    if (lp>snd_m2.sample_len) {
      snd_g.Sound2_On_Off=0;return 0;
    }
  }

  if (cur_env_step!=0) {
    if ((env>0 && snd_m2.env_dir==0) ||
	(env<15 && snd_m2.env_dir==1)) {

      ep++;
      if ((float)ep>snd_m2.sample_env_per_step) {
	ep=0;
	// cur_env_step--;
	if (snd_m2.env_dir==0) 
	  env-=1;//snd_m2.env_per_step;
	else
	  env+=1;//snd_m2.env_per_step;

	if (env==0 && snd_m2.env_dir==0) {
	  snd_g.Sound2_On_Off=0;env=0;return 0;
	}
      }
    }
  }
  if (env==0) {snd_g.Sound2_On_Off=0;return 0;} // Volume à zero
  snd_g.Sound2_On_Off=1;

  /* clipping */
  if (env>15) env=15;
  if (env<0) env=0;
  /*
  sp=wduty[snd_m2.duty][((int)(snd_m2.cp/(snd_m2.sample/8.0)))&0x7];
  snd_m2.cp+=1;
  return vol_table[env]&sp;
  */
  
  if (snd_m2.cp+1.0>snd_m2.sample) {
    sp=1;//1-sp;
    snd_m2.cp=(snd_m2.cp+1.0)-snd_m2.sample;
  } else if (snd_m2.cp+1.0>snd_m2.sample_dut && sp==1) {
    sp=0;//1-sp;
    snd_m2.cp+=1.0;
  } else
    snd_m2.cp+=1.0;

  if (sp)
    return -vol_table[env];
  else
    return vol_table[env];


}

inline INT8 update_snd_m3(void)
{
  // static float cp=0;
  static int lp=0;
  static int sp=0;
  static int val=0;
  float j;

  if (snd_m3.sample==0)
    return 0;

  if (!snd_m3.is_on) {
    snd_g.Sound3_On_Off=0;
    sp=0;
    lp=0;
    snd_m3.cp=0;
    return 0;
  }

  if (snd_m3.initial) {
    snd_m3.initial=0;
    snd_m3.cp=0;
    sp=0;
    lp=0;
    //    return 50;
  }

  if (snd_m3.mode==1) { // on doit gerer la durée
    // printf("Mode 1\n");
    lp++;
    if (lp>snd_m3.sample_len) {
      //     snd_m3.is_on=0;
      snd_g.Sound3_On_Off=0;
      sp=0;
      // lp=0;
      snd_m3.cp=0;
      return 0;
      
    }
  }


  snd_g.Sound3_On_Off=1;
  /*
  if (snd_m3.cp+1.0>(float)(snd_m3.sample)) {
    //   printf("snd_m3.cp:%d\n",snd_m3.cp);
    snd_m3.cp=(snd_m3.cp+1.0)-(snd_m3.sample);

    for (j=0;j<1.0;j+=snd_m3.sample) {
      sp++;
      if (sp>=32)
	sp=0;
      if (sp&0x1)
	val=snd_m3.wave[sp>>1]&0xF;
      else
	val=(snd_m3.wave[sp>>1]&0xF0)>>4;
    }
    // printf("val %x:%x,",val,snd_m3.wave[sp>>1]);
  } else
    snd_m3.cp+=1.0;
  */
  
  sp=(int)(snd_m3.cp/snd_m3.sample)&0x1F;
  snd_m3.cp+=1.0;
  if (sp&0x1)
    val=snd_m3.wave[sp>>1]&0xF;
  else
    val=(snd_m3.wave[sp>>1]&0xF0)>>4;
  


  /*  val -=8;
  if (snd_m3.level) val <<= (3 - snd_m3.level);
  else val=0;
  return val;
  */
  switch(snd_m3.level) {
  case 0: // mute
    return 0;
    break;
  case 1:
    return snd3_tbl_100[val];
  case 2:
    return snd3_tbl_50[val>>1];
    break;
  case 3:
    return snd3_tbl_25[val>>2];
    break;
  }

  return 0;
}

inline INT8 update_snd_m4(void)
{
  static float cp=0;
  static int env=0;
  static int sp=0;
  static int lp=0; 
  static int ep=0;
  static int cur_env_step;
  static UINT32 poly_counter;
  int vol;
  static int rol=0;
  float j;

  /* prohibited code */
  if ((snd_m4.poly&0x80)>>4==0xE || (snd_m4.poly&0x80)>>4==0xF)
    return 0;
  if (snd_m4.sample==0)
    return 0;

  if (snd_m4.initial) {
    snd_m4.initial=0;
    env=snd_m4.env_start;
    cur_env_step=snd_m4.env_swp;
    cp=0;
    //    sp=0;
    lp=0;
    ep=0;
  }

  if (snd_m4.env_changed && snd_m4.mode==0) {
    snd_m4.env_changed=0;
    env=snd_m4.env_start;
    cur_env_step=snd_m4.env_swp;
    cp=0;
    //    sp=0;
    ep=0;
    
  }

  if (snd_m4.poly_changed) {
    if (snd_m4.poly&0x8)
      poly_counter=0x40;
    else
      poly_counter=0x4000;
    snd_m4.poly_changed=0;
    sp=0;
  } 


  if (snd_m4.mode==1) { // on doit gerer la durée
 
    lp++;
    if (lp>snd_m4.sample_len) {
      // env=0;
      snd_g.Sound4_On_Off=0;return 0;
    }
   
  }

  if (cur_env_step!=0) {
 
    if ((env>0 && snd_m4.env_dir==0) ||
	(env<15 && snd_m4.env_dir==1)) {
      //      printf("%d,",env);
      ep++;
      if ((float)ep>snd_m4.sample_env_per_step) {
	ep=0;
	// cur_env_step--;
	
	if (snd_m4.env_dir==0) 
	  env-=1;//snd_m4.env_per_step;
	else
	  env+=1;//snd_m4.env_per_step;

	if (env==0 && snd_m4.env_dir==0) {
	  snd_g.Sound4_On_Off=0;env=0;return 0;
	}
      }
    }
  }
  if (env==0) {snd_g.Sound4_On_Off=0;return 0;} // Volume à zero
  snd_g.Sound4_On_Off=1;

  /* clipping */
  if (env>15) env=15;
  if (env<0) env=0;

  vol=vol_table[env];
  
  if (snd_m4.sample<0.8) {
    vol=vol*(snd_m4.sample+0.2);
  }
  
  if (cp+1.0>snd_m4.sample) {
   if (snd_m4.sample<1.0) {
      //printf("%f %f %f\n",cp,floor(1.0/snd_m4.sample)*snd_m4.sample,snd_m4.sample); 
      cp=(cp+1.0)-((int)(1.0/snd_m4.sample)*snd_m4.sample);
    }
    else
      cp=(cp+1.0)-snd_m4.sample;
 
   //for (j=0;j<1.0;j+=snd_m4.sample) {
   j=0;   do{

   
      sp =(poly_counter&1);
   
      if (snd_m4.poly&0x08)
	poly_counter ^= (((poly_counter&0x1)^((poly_counter&0x2)>>1))<<7);
      else
	poly_counter ^= (((poly_counter&0x1)^((poly_counter&0x2)>>1))<<15);
      
      poly_counter>>=1;
   }while(++j<1.0/snd_m4.sample);
   // printf("%f %f\n",j,1.0/snd_m4.sample);
    //cp=(cp)-snd_m4.sample;
    //   printf("%f %f %f\n",cp,1.0/snd_m4.sample,snd_m4.sample); 
 
  } else
    cp+=1.0;


  //  return rol;

  if (!sp)
    return -vol;
  else
    return vol;


}



void update_gb_sound(UINT32 snd_len)
{
  int i;
  INT8 *pl=playbuf+(lastpos<<1);
  INT16 p=0,l=0,r=0;

   SDL_LockAudio();
  

  if ((snd_len+(lastpos<<1))>=buf_size) {
    //printf("ho:%d %d\n",snd_len,(INT16)(lastpos<<1));
    snd_len=buf_size-(lastpos<<1); // on borne.
  }

  //printf("update gb sound len:%d %d\n",lastpos<<1,snd_len>>1);




// return;
  if (!(NR52&0x80)) {  // ALL SOUND ON/OFF
    SDL_UnlockAudio();
    return;
  }
  
  //if (!(NR50&0x80)) return; // SO2 ON/OFF
  //if (!(NR50&0x08)) return; // SO1 ON/OFF
  //  printf("SO2:%d\n",(NR50&0x70)>>4);
  //  printf("SO1:%d\n",(NR50&0x7));
  
  //printf("Output %d:%d\n",NR51&0x10,NR51&0x01);
  //printf("yo\n");
    
  //printf("curpos start:%d len:%d\n",curpos,len);

  for(i=0;i<(snd_len>>1) && pl<playbuf+buf_size;i++) {

    l=r=0;
    
    p=update_snd_m1();
    if (snd_g.Sound1_To_SO2) l=p;
    if (snd_g.Sound1_To_SO1) r=p;

          
    p=update_snd_m2();
    if (snd_g.Sound2_To_SO2) l+=p;
    if (snd_g.Sound2_To_SO1) r+=p;
    
    
    p=update_snd_m3();
  
    
    if (snd_g.Sound3_To_SO2) l+=p;
    if (snd_g.Sound3_To_SO1) r+=p;
    
    p=update_snd_m4();
    
    if (snd_g.Sound4_To_SO2) l+=p;
    if (snd_g.Sound4_To_SO1) r+=p;
    
 
    /*
    l=l/(float)(8-snd_g.SO2_OutputLevel);
    r=r/(float)(8-snd_g.SO1_OutputLevel);
    */

    
    l*=snd_g.SO2_OutputLevel;
    r*=snd_g.SO1_OutputLevel;
    
    

    
    l=l>>3;
    r=r>>3;
    

    if (l<-127) l=-127;
    if (l>127) l=127;
    if (r<-127) r=-127;
    if (r>127) r=127;

    // printf("laspos %d\n",i);

    /* p+=update_sound_m2();
    if (p>127) p=127;
    else if (p<-127) p=-127;
    */	
    *pl++=(INT8)l;
    *pl++=(INT8)r;
    curpos++;
  }
  if (curpos==buf_size) curpos=0;
  lastpos=curpos;

  //  printf("curpos end:%d\n",curpos);
  //  *(pl-1)=100;

  SDL_UnlockAudio();
}

/* fonction callback */
void update_stream(void *userdata,UINT8 *stream,int snd_len)
{

  /* update tous les buffer dans playbuf*/

  if (snd_len-(lastpos<<1)>0)
     update_gb_sound(snd_len-(lastpos<<1));
  get_nb_cycle();

#ifdef LOG_SOUND
  fwrite(playbuf,snd_len,1,fsound);
#endif


  memcpy(stream,playbuf,snd_len);
  memset(playbuf,0,snd_len);
  
 
  lastpos=curpos=0;

 
}


int init_sound(void)
{
  SDL_AudioSpec desired;
#ifdef LOG_SOUND
  fsound=fopen("./sound.raw","wb");
#endif

  sample_rate=44100;

  sample_per_update=1024;


  desired.freq=sample_rate;
  desired.samples=sample_per_update;
  desired.format=AUDIO_S8;
  desired.channels = 2;
  desired.callback=update_stream;
  desired.userdata = NULL;

  SDL_OpenAudio(&desired,NULL);
  playbuf=(INT8*)malloc(desired.size);
  buf_size=desired.size;
  memset(playbuf,0,desired.size);
  printf("Allocating %d\n",desired.size);
  init_freq_table();

  SDL_PauseAudio(0);
  SDL_Delay(1000); // Pour pemetre au thread audio de ce lancer
  return 0;
}

void close_sound(void)
{
  SDL_CloseAudio();
#ifdef LOG_SOUND
  fclose(fsound);
#endif
}









