/*
  frequency scale
*/

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "freq_sca.h"
#include "sbr_misc.h"

#include "counters.h" /* the 3GPP instrumenting tool */

static int getStartFreq(int fs, int start_freq);
static int getStopFreq(int fs, int stop_freq);

static int  numberOfBands(int b_p_o, int start, int stop, float warp_factor);
static void CalcBands(int * diff, int start , int stop , int num_bands);
static int  modifyBands(int max_band, int * diff, int length);
static void cumSum(int start_value, int* diff, int length, unsigned char  *start_adress);



/*******************************************************************************
 Functionname:  getSbrStartFreqRAW
 *******************************************************************************
 Description:

 Arguments:

 Return:
 *******************************************************************************/

int
getSbrStartFreqRAW (int startFreq, int QMFbands, int fs)
{
  int result;

  COUNT_sub_start("getSbrStartFreqRAW");

  ADD(1); LOGIC(1); BRANCH(1);
  if ( startFreq < 0 || startFreq > 15)
  {
    COUNT_sub_end();
    return -1;
  }

  FUNC(2);
  result = getStartFreq(fs, startFreq);

  MULT(1); DIV(1); ADD(1); SHIFT(1);
  result =   (result*fs/QMFbands+1)>>1;

  COUNT_sub_end();

  return (result);

} /* End getSbrStartFreqRAW */


/*******************************************************************************
 Functionname:  getSbrStopFreq
 *******************************************************************************
 Description:

 Arguments:

 Return:
 *******************************************************************************/
int getSbrStopFreqRAW  (int stopFreq, int QMFbands, int fs)
{
  int result;

  COUNT_sub_start("getSbrStopFreqRAW");

  ADD(1); LOGIC(1); BRANCH(1);
  if ( stopFreq < 0 || stopFreq > 13)
  {
    COUNT_sub_end();
    return -1;
  }


  FUNC(2);
  result = getStopFreq( fs, stopFreq);

  MULT(1); DIV(1); ADD(1); SHIFT(1);
  result =   (result*fs/QMFbands+1)>>1;

  COUNT_sub_end();

  return (result);
} /* End getSbrStopFreq */


/*******************************************************************************
 Functionname:  getStartFreq
 *******************************************************************************
 Description:

 Arguments:

 Return:
 *******************************************************************************/
static int
getStartFreq(int fs, int start_freq)
{
  int k0_min;

  COUNT_sub_start("getStartFreq");

  BRANCH(2); MOVE(1);
  switch(fs){
  case 16000: k0_min = 24;
    break;
  case 22050: k0_min = 17;
    break;
  case 24000: k0_min = 16;
    break;
  case 32000: k0_min = 16;
    break;
  case 44100: k0_min = 12;
    break;
  case 48000: k0_min = 11;
    break;
  default:
    k0_min=11; /* illegal fs */

  }

  BRANCH(2); INDIRECT(1); ADD(1); /* counting post-operations */
  COUNT_sub_end();

  switch (fs) {

  case 16000:
    {
      int v_offset[]= {-8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7};
      return (k0_min + v_offset[start_freq]);
    }
    break;

  case 22050:
    {
      int v_offset[]= {-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13};
      return (k0_min + v_offset[start_freq]);
    }
    break;

  case 24000:
    {
      int v_offset[]= {-5, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 16};
      return (k0_min + v_offset[start_freq]);
    }
    break;

  case 32000:
    {
      int v_offset[]= {-6, -4, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 16};
      return (k0_min + v_offset[start_freq]);
    }
    break;

  case 44100:
  case 48000:
    {
      int v_offset[]= {-4, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 16, 20};
      return (k0_min + v_offset[start_freq]);
    }
    break;

  default:
    {
      int v_offset[]= {0, 1, 2, 3, 4, 5, 6, 7, 9, 11, 13, 16, 20, 24, 28, 33};
      return (k0_min + v_offset[start_freq]);
    }

  }

}/* End getStartFreq */


/*******************************************************************************
 Functionname:  getStopFreq
 *******************************************************************************
 Description:

 Arguments:

 Return:
 *******************************************************************************/
static int
getStopFreq(int fs, int stop_freq)
{
  int result,i;
  int *v_stop_freq = NULL;
  int k1_min;
  int v_dstop[13];

  int v_stop_freq_32[14] =  {32,34,36,38,40,42,44,46,49,52,55,58,61,64};
  int v_stop_freq_44[14] =  {23,25,27,29,32,34,37,40,43,47,51,55,59,64};
  int v_stop_freq_48[14] =  {21,23,25,27,30,32,35,38,42,45,49,54,59,64};

  COUNT_sub_start("getStopFreq");

  PTR_INIT(4); /* counting previous operations */

  BRANCH(2);
  switch(fs){
  case 32000:
    MOVE(1);
    k1_min = 32;

    PTR_INIT(1);
    v_stop_freq =v_stop_freq_32;
    break;

  case 44100:
    MOVE(1);
    k1_min = 23;

    PTR_INIT(1);
    v_stop_freq =v_stop_freq_44;
    break;

  case 48000:
    MOVE(1);
    k1_min = 21;

    PTR_INIT(1);
    v_stop_freq =v_stop_freq_48;
    break;

  default:
    MOVE(1);
    k1_min = 21; /* illegal fs  */
  }



  PTR_INIT(2); /* v_dstop[]
                  v_stop_freq[]
               */
  LOOP(1);
  for(i = 0; i <= 12; i++) {
    ADD(1); STORE(1);
    v_dstop[i] = v_stop_freq[i+1] - v_stop_freq[i];
  }

  FUNC(2);
  Shellsort_int(v_dstop, 13);

  MOVE(1);
  result = k1_min;

  PTR_INIT(1); /* v_dstop[] */
  LOOP(1);
  for(i = 0; i < stop_freq; i++) {
    ADD(1);
    result = result + v_dstop[i];
  }

  COUNT_sub_end();

  return result;

}/* End getStopFreq */


/*******************************************************************************
 Functionname:  FindStartAndStopBand
 *******************************************************************************
 Description:

 Arguments:

 Return:
 *******************************************************************************/
int
FindStartAndStopBand(const int samplingFreq,
                     const int noChannels,
                     const int startFreq,
                     const int stopFreq,
                     const SR_MODE sampleRateMode,
                     int *k0,
                     int *k2)
{

  COUNT_sub_start("FindStartAndStopBand");


  FUNC(2); STORE(1);
  *k0 = getStartFreq(samplingFreq, startFreq);


  ADD(2); MULT(3); LOGIC(1); BRANCH(1);
  if( ( sampleRateMode == 1 ) &&
      ( samplingFreq*noChannels  <
        2**k0 * samplingFreq) ) {
    COUNT_sub_end();

    return (1);

  }


  ADD(1); BRANCH(1);
  if ( stopFreq < 14 ) {
    FUNC(2); STORE(1);
    *k2 = getStopFreq(samplingFreq, stopFreq);
  } else {
    ADD(1); BRANCH(1);
    if( stopFreq == 14 ) {
    MULT(1); STORE(1);
    *k2 = 2 * *k0;
  } else {
    MULT(1); STORE(1);
    *k2 = 3 * *k0;
  }
  }


  ADD(1); BRANCH(1);
  if (*k2 > noChannels) {
    MOVE(1);
    *k2 = noChannels;
  }


  ADD(3); DIV(1); BRANCH(1);
  if (*k2 - *k0 > noChannels / 2 - 4) {

    COUNT_sub_end();
    return (1);
  }

  ADD(2); BRANCH(1);
  if (*k2 > noChannels - 2) {

    COUNT_sub_end();
    return (1);
  }


  ADD(2); BRANCH(1);
  if ((*k2 - *k0) > MAX_FREQ_COEFFS)
  {
    COUNT_sub_end();
    return (1);
  }

  ADD(1); BRANCH(1);
  if ((*k2 - *k0) < 0)
  {
    COUNT_sub_end();
    return (1);
  }

  COUNT_sub_end();

  return(0);
}

/*******************************************************************************
 Functionname:  UpdateFreqScale
 *******************************************************************************
 Description:

 Arguments:

 Return:
 *******************************************************************************/
int
UpdateFreqScale(unsigned char  *v_k_master, int *h_num_bands,
                const int k0, const int k2,
                const int freqScale,
                const int alterScale)

{

  int     b_p_o = 0;
  float   warp = 0;
  int     dk = 0;

  /* Internal variables */
  int     two_regions = 0;
  int     k1 = 0, i;
  int     num_bands0;
  int     num_bands1;
  int     diff_tot[MAX_OCTAVE + MAX_SECOND_REGION];
  int     *diff0 = diff_tot;
  int     *diff1 = diff_tot+MAX_OCTAVE;
  int     k2_achived;
  int     k2_diff;
  int     incr = 0;

  COUNT_sub_start("UpdateFreqScale");

  MOVE(7); ADD(1); /* counting previous operations */


  ADD(1); BRANCH(1);
  if(freqScale==1)
  {
    MOVE(1);
    b_p_o=12;
  }

  ADD(1); BRANCH(1);
  if(freqScale==2)
  {
    MOVE(1);
    b_p_o=10;
  }

  ADD(1); BRANCH(1);
  if(freqScale==3)
  {
    MOVE(1);
    b_p_o=8;
  }


  BRANCH(1);
  if(freqScale > 0)
    {
      BRANCH(1);
      if(alterScale==0)
      {
        MOVE(1);
        warp=1.0;
      }
      else
      {
        MOVE(1);
        warp=1.3f;
      }


      MULT(2); ADD(1); BRANCH(1);
      if(4*k2 >= 9*k0)
        {
          MOVE(1);
          two_regions=1;

          MULT(1);
          k1=2*k0;

          FUNC(4);
          num_bands0=numberOfBands(b_p_o, k0, k1, 1.0);

          FUNC(4);
          num_bands1=numberOfBands(b_p_o, k1, k2, warp);

          FUNC(4);
          CalcBands(diff0, k0, k1, num_bands0);

          FUNC(2);
          Shellsort_int( diff0, num_bands0);

          BRANCH(1);
          if (diff0[0] == 0)
          {
            COUNT_sub_end();

            return (1);

          }

          FUNC(4);
          cumSum(k0, diff0, num_bands0, v_k_master);

          FUNC(4);
          CalcBands(diff1, k1, k2, num_bands1);

          FUNC(2);
          Shellsort_int( diff1, num_bands1);

          INDIRECT(1); ADD(1); BRANCH(1);
          if(diff0[num_bands0-1] > diff1[0])
            {
              INDIRECT(1); FUNC(3); BRANCH(1);
              if(modifyBands(diff0[num_bands0-1],diff1, num_bands1))
              {
                COUNT_sub_end();
                return(1);
              }

            }


          INDIRECT(1); PTR_INIT(1); FUNC(4);
          cumSum(k1, diff1, num_bands1, &v_k_master[num_bands0]);
          ADD(1); STORE(1);
          *h_num_bands=num_bands0+num_bands1;

        }
      else
        {
          MOVE(2);
          two_regions=0;
          k1=k2;

          FUNC(4);
          num_bands0=numberOfBands(b_p_o, k0, k1, 1.0);

          FUNC(4);
          CalcBands(diff0, k0, k1, num_bands0);

          FUNC(2);
          Shellsort_int( diff0, num_bands0);

          BRANCH(1);
          if (diff0[0] == 0)
          {
            COUNT_sub_end();
            return (1);

          }

          FUNC(4);
          cumSum(k0, diff0, num_bands0, v_k_master);

          MOVE(1);
          *h_num_bands=num_bands0;

        }
    }
  else
    {
      BRANCH(1);
      if (alterScale==0) {

        MOVE(1);
        dk = 1;

        ADD(1); DIV(1); MULT(1);
        num_bands0 = 2 * ((k2 - k0)/2);
      } else {

        MOVE(1);
        dk = 2;

        ADD(2); DIV(2); MULT(1);
        num_bands0 = 2 * (((k2 - k0)/dk +1)/2);
      }

      MULT(1); ADD(1);
      k2_achived = k0 + num_bands0*dk;

      ADD(1);
      k2_diff = k2 - k2_achived;

      PTR_INIT(1);
      LOOP(1);
      for(i=0;i<num_bands0;i++)
      {
        MOVE(1);
        diff_tot[i] = dk;
      }



      BRANCH(1);
      if (k2_diff < 0) {

          MOVE(2);
          incr = 1;
          i = 0;
      }



      BRANCH(1);
      if (k2_diff > 0) {

          MOVE(1);
          incr = -1;

          ADD(1);
          i = num_bands0-1;
      }


      PTR_INIT(1); /* diff_tot[] */
      LOOP(1);
      while (k2_diff != 0) {

        ADD(1); STORE(1);
        diff_tot[i] = diff_tot[i] - incr;

        /* ADD(1): pointer increment */
        i = i + incr;

        ADD(1);
        k2_diff = k2_diff + incr;
      }

      FUNC(4);
      cumSum(k0, diff_tot, num_bands0, v_k_master);

      MOVE(1);
      *h_num_bands=num_bands0;

    }

  ADD(1); BRANCH(1);
  if (*h_num_bands < 1)
  {
    COUNT_sub_end();
    return(1);
  }

  COUNT_sub_end();

  return (0);
}/* End UpdateFreqScale */


static int
numberOfBands(int b_p_o, int start, int stop, float warp_factor)
{
  int result=0;

  COUNT_sub_start("numberOfBands");

  MOVE(1); /* counting previous operation */

  MULT(4); ADD(1); TRANS(2); DIV(2);
  result = 2* (int) ( b_p_o * log( (float) (stop)/start) / (2.0*log(2.0)*warp_factor) +0.5);

  COUNT_sub_end();

  return(result);
}


static void
CalcBands(int * diff, int start , int stop , int num_bands)
{
  int i;
  int previous;
  int current;

  COUNT_sub_start("CalcBands");

  MOVE(1);
  previous=start;

  PTR_INIT(1); /* diff[] */
  LOOP(1);
  for(i=1; i<= num_bands; i++)
    {
      DIV(2); TRANS(1); MULT(1); ADD(1);
      current=(int) ( (start * pow( (float)stop/start, (float)i/num_bands)) + 0.5f);

      ADD(1); STORE(1);
      diff[i-1]=current-previous;

      MOVE(1);
      previous=current;
    }

  COUNT_sub_end();

}/* End CalcBands */


static void
cumSum(int start_value, int* diff, int length,  unsigned char *start_adress)
{
  int i;

  COUNT_sub_start("cumSum");

  MOVE(1);
  start_adress[0]=start_value;

  PTR_INIT(2); /* start_adress[]
                  diff[]
               */
  LOOP(1);
  for(i=1;i<=length;i++)
  {
    ADD(1); STORE(1);
    start_adress[i]=start_adress[i-1]+diff[i-1];
  }

  COUNT_sub_end();

} /* End cumSum */


static int
modifyBands(int max_band_previous, int * diff, int length)
{
  int change=max_band_previous-diff[0];

  COUNT_sub_start("modifyBands");

  ADD(1); /* counting previous operation */


  INDIRECT(1); ADD(2); DIV(1); BRANCH(1);
  if ( change > (diff[length-1] - diff[0]) / 2 )
  {
    MOVE(1);
    change = (diff[length-1] - diff[0]) / 2;
  }

  ADD(1); STORE(1);
  diff[0] += change;

  ADD(1); STORE(1);
  diff[length-1] -= change;

  FUNC(2);
  Shellsort_int(diff, length);

  COUNT_sub_end();

  return(0);
}/* End modifyBands */


/*******************************************************************************
 Functionname:  UpdateHiRes
 *******************************************************************************
 Description:

 Arguments:

 Return:
 *******************************************************************************/
int
UpdateHiRes(unsigned char *h_hires, int *num_hires,unsigned char * v_k_master,
            int num_master , int *xover_band, SR_MODE drOrSr,
            int noQMFChannels)
{
  int i;
  int divider;
  int max1,max2;

  COUNT_sub_start("UpdateHiRes");


  ADD(1); BRANCH(1); MOVE(1);
  divider = (drOrSr == DUAL_RATE) ? 2 : 1;

  DIV(1); INDIRECT(1); ADD(2); LOGIC(1); BRANCH(1);
  if( (v_k_master[*xover_band] > (noQMFChannels/divider) ) ||
      ( *xover_band > num_master ) )  {



    MOVE(2);
    max1=0;
    max2=num_master;

    PTR_INIT(1); /* v_k_master[] */
    ADD(1); LOOP(1);
    while( (v_k_master[max1+1] < (noQMFChannels/divider)) &&
           ( (max1+1) < max2) )
      {
        ADD(2); LOGIC(1); /* while()-condition */

        ADD(1);
        max1++;
      }

    MOVE(1);
    *xover_band=max1;
  }

  ADD(1); STORE(1);
  *num_hires = num_master - *xover_band;

  PTR_INIT(2); /* h_hires[]
                  v_k_master[]
               */
  LOOP(1);
  for(i = *xover_band; i <= num_master; i++)
    {
      MOVE(1);
      h_hires[i - *xover_band] = v_k_master[i];
    }

  COUNT_sub_end();

  return (0);
}/* End UpdateHiRes */


/*******************************************************************************
 Functionname:  UpdateLoRes
 *******************************************************************************
 Description:

 Arguments:

 Return:
 *******************************************************************************/
void
UpdateLoRes(unsigned char * h_lores, int *num_lores, unsigned char * h_hires, int num_hires)
{
  int i;

  COUNT_sub_start("UpdateLoRes");

  LOGIC(1); BRANCH(1);
  if(num_hires%2 == 0)
    {
      DIV(1); STORE(1);
      *num_lores=num_hires/2;


      PTR_INIT(2); /* h_lores[]
                      h_hires[]
                   */
      LOOP(1);
      for(i=0;i<=*num_lores;i++)
      {
        MOVE(1);
        h_lores[i]=h_hires[i*2];
      }

    }
  else
    {
      ADD(1); DIV(1); STORE(1);
      *num_lores=(num_hires+1)/2;


      MOVE(1);
      h_lores[0]=h_hires[0];

      PTR_INIT(2); /* h_lores[]
                      h_hires[]
                   */
      LOOP(1);
      for(i=1;i<=*num_lores;i++)
        {
          MOVE(1);
          h_lores[i]=h_hires[i*2-1];
        }
    }

  COUNT_sub_end();

}/* End UpdateLoRes */
