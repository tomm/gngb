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


#include <SDL/SDL.h>
#include "sound.h"
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"

#define HZ(x) ((double)(131072.0)/(double)(2048-x))
#define HZ_M3(x) ((double)(4194304.0)/(64.0*(double)(2048-x)))
//#define LOG_SOUND

INT8 *playbuf;
FILE *fsound;

//INT8 vol_table[]={0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120};
/*
INT8 vol_table[]={0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30};
INT8 snd3_tbl_100[]={-30,-28,-24,-20,-16,-12,-8,-4,4,8,12,16,20,24,28,30};
INT8 snd3_tbl_50[]={-16,-12,-8,-4,4,8,12,16};
INT8 snd3_tbl_25[]={-8,-4,4,8};
*/
/*
INT8 vol_table[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
INT8 snd3_tbl_100[]={-8,-7,-6,-5,-4,-3,-2,-1,1,2,3,4,5,6,7,8};
INT8 snd3_tbl_50[]={-4,-3,-2,-1,1,2,3,4};
INT8 snd3_tbl_25[]={-2,-1,1,2};
*/
INT8 vol_table[]={0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120};
INT8 snd3_tbl_100[]={-120,-104,-88,-72,-56,-40,-24,-8,8,24,40,56,72,88,104,120};
INT8 snd3_tbl_50[]={-56,-40,-24,-8,8,24,40,56};
INT8 snd3_tbl_25[]={-24,-8,8,24};

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
  UINT32 snd_len =(UINT32)((float)get_nb_cycle()*(sample_rate/59.73)
			   /((gbcpu->mode== DOUBLE_SPEED)?140448.0:70224.0));

  if (snd_len) 
    update_gb_sound(snd_len<<1);

  if (add>=0xFF30 && add<=0xFF3F) { // Wave Pattern RAM
    if (!snd_m3.is_on) 
      snd_m3.wave[add-0xFF30]=val;
    return;
  }

  switch(add) {
    /*--- SOUND MODE 1 ---*/
  case 0xFF10 : // NR10
    snd_m1.swp_time=(val&0x70)>>4;
    snd_m1.swp_dir=(val&0x8)>>3;
    snd_m1.swp_shift=val&0x7;
    snd_m1.sample_sweep_time=(float)sample_rate*snd_m1.swp_time/128.0;
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
    snd_m1.sample=sample_rate*freq_table[snd_m1.freq];
    snd_m1.sample_dut=(float)snd_m1.sample/wave_duty[snd_m1.duty];
    break;
  case 0xFF14 : // NR14
    snd_m1.initial=(val&0x80)>>7;
    snd_m1.mode=(val&0x40)>>6;
    snd_m1.freq_hi=val&0x7;
    snd_m1.freq=((UINT16)(snd_m1.freq_lo))|(((UINT16)(snd_m1.freq_hi))<<8); // update freq
    snd_m1.sample=(float)sample_rate*freq_table[snd_m1.freq];
    snd_m1.sample_dut=(float)snd_m1.sample/wave_duty[snd_m1.duty];
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
    snd_m2.sample=sample_rate*freq_table[snd_m2.freq];
    snd_m2.sample_dut=(float)snd_m2.sample/wave_duty[snd_m2.duty];
    break;
  case 0xFF19 : // NR24
    snd_m2.initial=(val&0x80)>>7;
    snd_m2.mode=(val&0x40)>>6;
    snd_m2.freq_hi=val&0x7;
    snd_m2.freq=((UINT16)(snd_m2.freq_lo))|(((UINT16)(snd_m2.freq_hi))<<8); // update freq
    snd_m2.sample=(float)sample_rate*freq_table[snd_m2.freq];
    snd_m2.sample_dut=(float)snd_m2.sample/wave_duty[snd_m2.duty];
    break;

    /*--- Sound Mode 3 ---*/
  case 0xFF1A : // NR30
    snd_m3.is_on=(val&0x80)>>7;
    break;
  case 0xFF1B : // NR31
    snd_m3.len=val;
    snd_m3.sample_len=(float)sample_rate*(256.0-snd_m3.len)/2.0;
    break;
  case 0xFF1C : // NR32
    snd_m3.level=(val&0x60)>>5;
    // printf("--level%d %x\n", snd_m3.level,val);
    break;
  case 0xFF1D : // NR33
    snd_m3.freq_lo=val;
    snd_m3.freq=((UINT16)(snd_m3.freq_lo))|(((UINT16)(snd_m3.freq_hi))<<8);
    snd_m3.sample=(float)sample_rate*freq_table_m3[snd_m3.freq];// *64.0;
    break;
  case 0xFF1E : // NR34
    snd_m3.initial=(val&0x80)>>7;
    snd_m3.mode=(val&0x40)>>6;
    snd_m3.freq_hi=val&0x7;
    snd_m3.freq=((UINT16)(snd_m3.freq_lo))|(((UINT16)(snd_m3.freq_hi))<<8);
    snd_m3.sample=(float)sample_rate*freq_table_m3[snd_m3.freq];// *64.0;
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
    //    printf("poly %x %g\n",val,snd_m4.sample);
    break;
  case 0xFF23 : // NR44
    snd_m4.initial=(val&0x80)>>7;
    snd_m4.mode=(val&0x40)>>6;
    // printf("env mode %x:%d %d\n",val,snd_m4.initial,snd_m4.mode);
    break;

    
    /*--- GENERAL CONTROL ---*/
  case 0xFF24 : // NR50
    snd_g.SO1_OutputLevel=val&0x7;
    snd_g.SO2_OutputLevel=(val&0x70)>>4;
    snd_g.Vin_SO1=(val&0x8)>>3;
    snd_g.Vin_SO2=(val&0x80)>>7;
    // printf("outup %d %d \n",snd_g.SO1_OutputLevel,snd_g.SO2_OutputLevel);
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
  long double a=0.015896,b=7.7961e-6;
  long double y;
  //  printf("a:%Lg,b:%Lg\n",a,b);
  for(i=0;i<2048;i++) {
    /*
    y=(a-b*(double)i);
    // printf("%d:%Lg\n",i,y);
    if (y<20000 && y>0)
      freq_table[i]=y;
    else
      freq_table[i]=20000.0;
    */
    freq_table[i]=1.0/HZ(i);
    freq_table_m3[i]=1.0/HZ_M3(i);
    // printf("%d %g\n",i,freq_table[i]);
  }

  for(i=0;i<256;i++) {
    if ((i&0x7)==0)
      y=(long double)2.0;
    else
      y=(long double)1.0/(float)(i&0x7);

    freq_table_m4[i]=1.0/(long double)((long double)4194304.0*(long double)(1.0/8.0)*y*(long double)(1.0/(long double)(2<<(i>>4))));
    // printf("%x %Lg %Lg %Lg\n",i,freq_table_m4[i],(long double)524288.0*y,y);
  }
}


INT8 update_snd_m1(void)
{
  static float cp=0;
  static int env=0;
  static int sp=0;
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
    cp=0;
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
    cp=0;
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
      if (snd_m1.swp_dir==1) {
	snd_m1.freq= snd_m1.freq - (float)snd_m1.freq/(float)(1<<(snd_m1.swp_shift));
	snd_m1.sample=sample_rate*freq_table[snd_m1.freq];
	snd_m1.sample_dut=(float)snd_m1.sample/wave_duty[snd_m1.duty];
      } else {
	snd_m1.freq= snd_m1.freq + (float)snd_m1.freq/(float)(1<<(snd_m1.swp_shift));
	snd_m1.sample=sample_rate*freq_table[snd_m1.freq];
	snd_m1.sample_dut=(float)snd_m1.sample/wave_duty[snd_m1.duty];
      }
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

  if (cp+1.0>snd_m1.sample) {
    sp=1;//1-sp;
    // printf("- %f\n",cp);
    cp=(cp+1.0)-snd_m1.sample;
  } else if (cp+1.0>snd_m1.sample_dut && sp==1) {
    sp=0;//1-sp;
    // printf("%f\n",cp);
     cp+=1.0;// cp-snd_m1.sample_dut;
  } else
    cp+=1.0;

  if (sp)
    return -vol_table[env];
  else
    return vol_table[env];


}

INT8 update_snd_m2(void)
{
  static float cp=0;
  static int env=0;
  static int sp=0;
  static int lp=0; 
  static int ep=0;
  static int cur_env_step;

  if (snd_m2.initial) {
    snd_m2.initial=0;
    env=snd_m2.env_start;
    cur_env_step=snd_m2.env_swp;
    cp=0;
    //    sp=0;
    lp=0;
    ep=0;
  }

  if (snd_m2.env_changed && snd_m2.mode==0) {
    snd_m2.env_changed=0;
    env=snd_m2.env_start;
    cur_env_step=snd_m2.env_swp;
    cp=0;
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

  if (cp+1.0>snd_m2.sample) {
    sp=1;//1-sp;
    cp=(cp+1.0)-snd_m2.sample;
  } else if (cp+1.0>snd_m2.sample_dut && sp==1) {
    sp=0;//1-sp;
    cp+=1.0;
  } else
    cp+=1.0;

  if (sp)
    return -vol_table[env];
  else
    return vol_table[env];


}

INT8 update_snd_m3(void)
{
  static float cp=0;
  static int lp=0;
  static int sp=0;
  static int val=0;

  if (snd_m3.initial) {
    snd_m3.initial=0;
    cp=0;
    sp=0;
    lp=0;
  }

  if (!snd_m3.is_on) {
    snd_g.Sound3_On_Off=0;
    sp=0;
    lp=0;
    cp=0;
    return 0;
  }

  if (snd_m3.mode==1) { // on doit gerer la durée
    // printf("Mode 1\n");
    lp++;
    if (lp>snd_m3.sample_len) {
      //     snd_m3.is_on=0;
      snd_g.Sound3_On_Off=0;
      sp=0;
      lp=0;
      cp=0;
      return 0;
      
    }
  }


  snd_g.Sound3_On_Off=1;

  if (cp+1.0>(float)(snd_m3.sample/33.0)) {
    //   printf("cp:%d\n",cp);
    cp=(cp+1.0)-(snd_m3.sample/33.0);
    sp++;
    if (sp>32)
      sp=0;
    if (sp&0x1)
      val=snd_m3.wave[sp>>1]&0xF;
    else
      val=(snd_m3.wave[sp>>1]&0xF0)>>4;
    // printf("val %x:%x,",val,snd_m3.wave[sp>>1]);
  } else
    cp+=1.0;

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

  //  printf("%d %d\n",val,sp);
/*  if (sp&0x1)
    return -vol_table[val];
    else 
    return vol_table[val];
*/

}

INT8 update_snd_m4(void)
{
  static float cp=0;
  static int env=0;
  static int sp=0;
  static int lp=0; 
  static int ep=0;
  static int cur_env_step;
  int vol;

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



  if (snd_m4.mode==1) { // on doit gerer la durée
    // printf("Mode 1\n");
    lp++;
    if (lp>snd_m4.sample_len) {
      // printf("STOP\n");
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

  if (((snd_m4.poly&0xF0)>>4)<2) 
    vol=vol/4.0;
  else if (((snd_m4.poly&0xF0)>>4)<4)
    vol=vol/3.0;
  else if (((snd_m4.poly&0xF0)>>4)<6) 
    vol=vol>>1;


  if (cp+1.0>snd_m4.sample) {
    sp=rand()&0x1;// 0 ou 1
    cp=(cp+1.0)-snd_m4.sample;

  } else
    cp+=1.0;
 

  if (sp)
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
  

  if ((snd_len+(lastpos<<1))>buf_size) {
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

  for(i=0;i<(snd_len>>1);i++) {

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

    l=l/(float)(8-snd_g.SO2_OutputLevel);
    r=r/(float)(8-snd_g.SO1_OutputLevel);

    l=l>>2;
    r=r>>2;
    
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
  lastpos=curpos;

  //  printf("curpos end:%d\n",curpos);
  //  *(pl-1)=100;

  SDL_UnlockAudio();
}

/* fonction callback */
void update_stream(void *userdata,UINT8 *stream,int snd_len)
{
  int i;
  UINT32 lg;
  /* update tous les buffer dans playbuf*/

 
  
  //  printf("---- %d-%d = %d\n",len,(lastpos<<1),len-(lastpos<<1));
  /*
  lg = (float)get_nb_cycle()*(sample_rate/59.73)/((gbcpu->mode== DOUBLE_SPEED)?140448.0:70224.0);
  //  printf("--- len:%d\n",lg);
  if (lg) 
    update_gb_sound(lg<<1);
  */
  if (snd_len-(lastpos<<1)>0)
     update_gb_sound(snd_len-(lastpos<<1));
  get_nb_cycle();
  // playbuf[(lastpos<<1)+(lg<<1)]=100;
  //    playbuf[0]=-100; // DEBUG!!!!
  //  playbuf[len-1]=100;


#ifdef LOG_SOUND
  fwrite(playbuf,snd_len,1,fsound);
#endif

  /* NO PROB HERE */
  memcpy(stream,playbuf,snd_len);
  memset(playbuf,0,snd_len);
  
 
  lastpos=curpos=0;
  // get_nb_cycle();
 
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
}

void close_sound(void)
{
  SDL_CloseAudio();
#ifdef LOG_SOUND
  fclose(fsound);
#endif
}









