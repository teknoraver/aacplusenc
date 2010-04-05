/*
  Psychoaccoustic configuration
*/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "psy_configuration.h"
#include "minmax.h"
#include "adj_thr.h"
#include "aac_rom.h"

#include "counters.h" /* the 3GPP instrumenting tool */



typedef struct{
  long  sampleRate;
  const unsigned char *paramLong;
  const unsigned char *paramShort;
}SFB_INFO_TAB;

static const float ABS_LEV = 20.0f;
static const float ABS_LOW = 16887.8f*NORM_PCM_ENERGY; /* maximum peak sine - 96 db*/
static const float BARC_THR_QUIET[] = {15.0f, 10.0f,  7.0f,  2.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 3.0f,  5.0f, 10.0f, 20.0f, 30.0f};


static SFB_INFO_TAB sfbInfoTab[] ={
  {11025, sfb_11025_long_1024, sfb_11025_short_128},
  {12000, sfb_12000_long_1024, sfb_12000_short_128},
  {16000, sfb_16000_long_1024, sfb_16000_short_128},
  {22050, sfb_22050_long_1024, sfb_22050_short_128},
  {24000, sfb_24000_long_1024, sfb_24000_short_128}
};




#define max_barc   24.0f /* maximum barc-value */
#define c_ratio     0.001258925f /* pow(10.0f, -(29.0f/10.0f)) */

#define maskLow                 (30.0f*0.1f) /* in 0.1dB/bark */
#define maskHigh                (15.0f*0.1f) /* in 0.1*dB/bark */
#define maskLowSprEnLong        (30.0f*0.1f) /* in 0.1dB/bark */
#define maskHighSprEnLong       (20.0f*0.1f) /* in 0.1dB/bark */
#define maskHighSprEnLongLowBr  (15.0f*0.1f) /* in 0.1dB/bark */
#define maskLowSprEnShort       (20.0f*0.1f) /* in 0.1dB/bark */
#define maskHighSprEnShort      (15.0f*0.1f) /* in 0.1dB/bark */


static int initSfbTable(long sampleRate,int blockType,int *sfbOffset,int *sfbCnt)
{
  const unsigned char *sfbParam = 0;
  unsigned int i;
  int  specStartOffset,specLines = 0;

  COUNT_sub_start("initSfbTable");

  MOVE(1); /* counting previous operation */

  /*
    select table
  */
  PTR_INIT(1); /* sfbInfoTab[] */
  LOOP(1);
  for(i = 0; i < sizeof(sfbInfoTab)/sizeof(SFB_INFO_TAB); i++){

    ADD(1); BRANCH(1);
    if(sfbInfoTab[i].sampleRate == sampleRate){

      BRANCH(2);
      switch(blockType){
      case LONG_WINDOW:
      case START_WINDOW:
      case STOP_WINDOW:
        MOVE(2);
        sfbParam = sfbInfoTab[i].paramLong;
        specLines = FRAME_LEN_LONG;
        break;

      case SHORT_WINDOW:
        MOVE(2);
        sfbParam = sfbInfoTab[i].paramShort;
        specLines = FRAME_LEN_SHORT;
        break;
      }
      break;
    }
  }

  BRANCH(1);
  if(sfbParam==0)
  {
    COUNT_sub_end();
    return(1);
  }

  /*
    calc sfb offsets
  */
  MOVE(2);
  *sfbCnt=0;
  specStartOffset = 0;
  
  LOOP(1);
  do{
    INDIRECT(1); MOVE(1);
    sfbOffset[*sfbCnt] = specStartOffset;

    INDIRECT(1); ADD(1);
    specStartOffset += sfbParam[*sfbCnt];

    ADD(1); STORE(1);
    (*sfbCnt)++;
  } while(specStartOffset< specLines);
  
  assert(specStartOffset == specLines);

  INDIRECT(1); MOVE(1);
  sfbOffset[*sfbCnt] = specStartOffset;

  COUNT_sub_end();

  return 0;
}


/*****************************************************************************

    functionname: atan_approx
    description:  Calculates atan , val >=0
    returns:      approx of atan(val), error is less then 0.5 %
    input:
    output:

*****************************************************************************/
static float atan_approx(float val)
{
  COUNT_sub_start("atan_approx");

  ADD(1); BRANCH(1);
  if(val < (float)1.0)
  {
    DIV(1); MULT(2); ADD(1); /* counting post-operations */
    COUNT_sub_end();
    return(val/((float)1.0f+(float)0.280872f*val*val));
  }
  else
  {
    DIV(1); MULT(1); ADD(2); /* counting post-operations */
    COUNT_sub_end();
    return((float)1.57079633f-val/((float)0.280872f +val*val));
  }

}


/*****************************************************************************

    functionname: BarcLineValue
    description:  Calculates barc value for one frequency line
    returns:      barc value of line
    input:        number of lines in transform, index of line to check, Fs
    output:

*****************************************************************************/
static float BarcLineValue(int noOfLines, int fftLine, long samplingFreq) {

  float center_freq, temp, bvalFFTLine;

  COUNT_sub_start("BarcLineValue");

  /*
    center frequency of fft line
  */
  MULT(2); DIV(1);
  center_freq = (float) fftLine * ((float)samplingFreq * (float)0.5f)/(float)noOfLines;

  MULT(1); FUNC(1);
  temp = (float) atan_approx((float)1.3333333e-4f * center_freq);

  MULT(4); ADD(1); FUNC(1);
  bvalFFTLine = (float)13.3f * atan_approx((float)0.00076f * center_freq) + (float)3.5f * temp * temp;

  COUNT_sub_end();

  return(bvalFFTLine);

}



static void initThrQuiet(int   numPb,
                         int   *pbOffset,
                         float *pbBarcVal,
                         float *pbThresholdQuiet) {
  int i;
  float barcThrQuiet;

  COUNT_sub_start("initThrQuiet");

  PTR_INIT(1); /* pbBarcVal[]
                  pbThresholdQuiet[]
                  pbOffset[]
               */
  LOOP(1);
  for(i=0; i<numPb; i++) {

    /* because one pb equals a sfb in psych it's better to use
       the minimum of the bark-wise threshold in quiet */
    int bv1, bv2;

    BRANCH(1);
    if (i>0)
    {
       ADD(1); SHIFT(1);
       bv1 = (int)(pbBarcVal[i] + pbBarcVal[i-1])>>1;
    }
    else
    {
       SHIFT(1);
       bv1 = (int)(pbBarcVal[i])>>1;
    }

    ADD(2); BRANCH(1);
    if (i<numPb-1)
    {
       ADD(1); SHIFT(1);
       bv2 = (int)(pbBarcVal[i] + pbBarcVal[i+1])>>1;
    }
    else
    {
       MOVE(1);
       bv2 = (int)(pbBarcVal[i]);
    }

    ADD(2); BRANCH(2); MOVE(2);
    bv1=min(bv1,(int)max_barc);
    bv2=min(bv2,(int)max_barc);


    INDIRECT(2); ADD(1); BRANCH(1); MOVE(1);
    barcThrQuiet = min(BARC_THR_QUIET[bv1], BARC_THR_QUIET[bv2]);

    ADD(2); MULT(3); TRANS(1); STORE(1);
    pbThresholdQuiet[i] = (float) pow(10.0f,(barcThrQuiet - ABS_LEV) * (float)0.1f)*ABS_LOW*(float)(pbOffset[i+1] - pbOffset[i]);

  }

  COUNT_sub_end();
}



static void initSpreading(int numPb,
                          float *pbBarcValue,
                          float *pbMaskLoFactor,
                          float *pbMaskHiFactor,
                          float *pbMaskLoFactorSprEn,
                          float *pbMaskHiFactorSprEn,
                          const long bitrate,
                          const int blockType)
{
  int i;
  float maskLowSprEn, maskHighSprEn;

  COUNT_sub_start("initSpreading");

  ADD(1); BRANCH(1);
  if (blockType != SHORT_WINDOW) {

    MOVE(1);
    maskLowSprEn = maskLowSprEnLong;

    ADD(1); BRANCH(1); MOVE(1);
    maskHighSprEn = (bitrate>22000)?maskHighSprEnLong:maskHighSprEnLongLowBr;
  }
  else {

    MOVE(2);
    maskLowSprEn = maskLowSprEnShort;
    maskHighSprEn = maskHighSprEnShort;
  }

  PTR_INIT(7); /* pbBarcValue[i]
                  pbMaskHiFactor[i]
                  pbMaskLoFactor[i-1]
                  pbMaskHiFactorSprEn[i]
                  pbMaskLoFactorSprEn[i-1]
                  pbMaskLoFactor[numPb-1]
                  pbMaskLoFactorSprEn[numPb-1]
               */
  LOOP(1);
  for(i=0; i<numPb; i++) {

    BRANCH(1);
    if (i > 0) {
       float dbVal;

       ADD(1); MULT(1);
       dbVal = maskHigh * (pbBarcValue[i]-pbBarcValue[i-1]);

       MULT(1); TRANS(1); STORE(1);
       pbMaskHiFactor[i] = (float) pow(10.0f, -dbVal);

       ADD(1); MULT(1);
       dbVal = maskLow * (pbBarcValue[i]-pbBarcValue[i-1]);

       MULT(1); TRANS(1); STORE(1);
       pbMaskLoFactor[i-1] = (float) pow(10.0f, -dbVal);

       ADD(1); MULT(1);
       dbVal = maskHighSprEn * (pbBarcValue[i]-pbBarcValue[i-1]);

       MULT(1); TRANS(1); STORE(1);
       pbMaskHiFactorSprEn[i] = (float) pow(10.0f, -dbVal);

       ADD(1); MULT(1);
       dbVal = maskLowSprEn * (pbBarcValue[i]-pbBarcValue[i-1]);

       MULT(1); TRANS(1); STORE(1);
       pbMaskLoFactorSprEn[i-1] = (float) pow(10.0f, -dbVal);
    }
    else {
       MOVE(2);
       pbMaskHiFactor[i] = 0.0f;
       pbMaskLoFactor[numPb-1] = 0.0f;

       MOVE(2);
       pbMaskHiFactorSprEn[i] = 0.0f;
       pbMaskLoFactorSprEn[numPb-1] = 0.0f;
    }

  }

  COUNT_sub_end();
}



static void initBarcValues(int numPb,
                           int *pbOffset,
                           int numLines,
                           long samplingFrequency,
                           float *pbBval)
{
  int   i;
  float pbBval0,pbBval1;

  COUNT_sub_start("initBarcValues");

  MOVE(1);
  pbBval0=0.0;

  PTR_INIT(1); /* pbOffset[] */
  LOOP(1);
  for(i=0; i<numPb; i++){
    FUNC(3);
    pbBval1   = BarcLineValue(numLines, pbOffset[i+1], samplingFrequency);

    ADD(1); MULT(1); STORE(1);
    pbBval[i] = (pbBval0+pbBval1)*(float)0.5f;

    MOVE(1);
    pbBval0=pbBval1;
  }

  COUNT_sub_end();
}



static void initMinSnr(const long    bitrate,
                       const long    samplerate,
                       const int     numLines,
                       const int     *sfbOffset,
                       const float *pbBarcVal,
                       const int     sfbActive,
                       float       *sfbMinSnr)
{
   int sfb;
   float barcFactor;
   float barcWidth;
   float pePerWindow, pePart;
   float snr;
   float pbVal0,pbVal1;

   COUNT_sub_start("initMinSnr");

   /* relative number of active barks */
   DIV(2); ADD(1); BRANCH(1);
   barcFactor = (float)1.0/min(pbBarcVal[sfbActive-1]/max_barc,(float)1.0);

   DIV(1); MULT(1); FUNC(1);
   pePerWindow = bits2pe((float)bitrate / (float)samplerate * (float)numLines);

   MOVE(1);
   pbVal0=(float)0.0;

   PTR_INIT(3); /* pbBarcVal[]
                   sfbOffset[]
                   sfbMinSnr[]
                */
   LOOP(1);
   for (sfb = 0; sfb < sfbActive; sfb++) {

      MULT(1); ADD(1);
      pbVal1 = (float)2.0*pbBarcVal[sfb]-pbVal0;

      ADD(1);
      barcWidth=pbVal1-pbVal0;

      MOVE(1);
      pbVal0=pbVal1;

      /* allow at least 2.4% of pe for each active barc */
      MULT(2);
      pePart = pePerWindow * (float)0.024f * barcFactor;

      /* adapt to sfb bands */
      MULT(1);
      pePart *= barcWidth;

      /* pe -> snr calculation */
      ADD(1); DIV(1);
      pePart /= (float)(sfbOffset[sfb+1] - sfbOffset[sfb]);

      TRANS(1); ADD(1);
      snr = (float) pow(2.0f, pePart) - 1.5f;

      ADD(1); BRANCH(1); DIV(1);
      snr = 1.0f / max(snr, 1.0f);

      /* upper limit is -1 dB */
      ADD(1); BRANCH(1); MOVE(1);
      snr = min(snr, 0.8f);

      /* lower limit is -25 dB */
      ADD(1); BRANCH(1); MOVE(1);
      snr = max(snr, 0.003f);

      MOVE(1);
      sfbMinSnr[sfb] = snr;
   }

   COUNT_sub_end();
}


int InitPsyConfiguration(long  bitrate,
                         long  samplerate,
                         int   bandwidth,
                         PSY_CONFIGURATION_LONG *psyConf) 
{
  int sfb;
  float sfbBarcVal[MAX_SFB_LONG];

  COUNT_sub_start("InitPsyConfiguration");

  /*
    init sfb table
  */
  INDIRECT(2); PTR_INIT(1); FUNC(4);
  if(initSfbTable(samplerate,LONG_WINDOW,psyConf->sfbOffset,&(psyConf->sfbCnt)))
  {
    COUNT_sub_end();
    return(1);
  }

  /*
    calculate barc values for each pb
  */
  INDIRECT(4); FUNC(5);
  initBarcValues(psyConf->sfbCnt,
                 psyConf->sfbOffset,
                 psyConf->sfbOffset[psyConf->sfbCnt],
                 samplerate,
                 sfbBarcVal);


  /*
    init thresholds in quiet
  */
  INDIRECT(4); FUNC(4);
  initThrQuiet(psyConf->sfbCnt,
               psyConf->sfbOffset,
               sfbBarcVal,
               psyConf->sfbThresholdQuiet);


  /*
    calculate spreading function
  */
  INDIRECT(6); FUNC(8);
  initSpreading(psyConf->sfbCnt,
                sfbBarcVal,
                psyConf->sfbMaskLowFactor,
                psyConf->sfbMaskHighFactor,
                psyConf->sfbMaskLowFactorSprEn,
                psyConf->sfbMaskHighFactorSprEn,
                bitrate,
                LONG_WINDOW);


  /*
    init ratio
  */
  INDIRECT(1); MOVE(1);
  psyConf->ratio = c_ratio;

  INDIRECT(2); MOVE(2);
  psyConf->maxAllowedIncreaseFactor = 2.0f;
  psyConf->minRemainingThresholdFactor = 0.01f;

  INDIRECT(1); MOVE(1);
  psyConf->clipEnergy = 1.0e9f*NORM_PCM_ENERGY;

  INDIRECT(1); MULT(1); DIV(1); STORE(1);
  psyConf->lowpassLine = (int)((2*bandwidth*FRAME_LEN_LONG)/samplerate);
  
  PTR_INIT(1); /* psyConf->sfbOffset[] */
  INDIRECT(2); LOOP(1);
  for (sfb = 0; sfb < psyConf->sfbCnt; sfb++){
    ADD(1); BRANCH(1);
    if (psyConf->sfbOffset[sfb] >= psyConf->lowpassLine)
       break;
   }
  INDIRECT(1); MOVE(1);
  psyConf->sfbActive  = sfb;

  /*
    calculate minSnr
  */
  INDIRECT(5); FUNC(7);
  initMinSnr(bitrate,
             samplerate,
             psyConf->sfbOffset[psyConf->sfbCnt],
             psyConf->sfbOffset,
             sfbBarcVal,
             psyConf->sfbActive,
             psyConf->sfbMinSnr);

  COUNT_sub_end();

  return 0;
}


int InitPsyConfigurationShort(long  bitrate,
                              long  samplerate,
                              int   bandwidth,
                              PSY_CONFIGURATION_SHORT *psyConf) 
{
  int sfb;
  float sfbBarcVal[MAX_SFB_SHORT];

  COUNT_sub_start("InitPsyConfiguration");

  /*
    init sfb table
  */
  INDIRECT(2); PTR_INIT(1); FUNC(4);
  if(initSfbTable(samplerate,SHORT_WINDOW,psyConf->sfbOffset,&(psyConf->sfbCnt)))
  {
    COUNT_sub_end();
    return(1);
  }

  /*
    calculate barc values for each pb
  */
  INDIRECT(4); FUNC(5);
  initBarcValues(psyConf->sfbCnt,
                 psyConf->sfbOffset,
                 psyConf->sfbOffset[psyConf->sfbCnt],
                 samplerate,
                 sfbBarcVal);


  /*
    init thresholds in quiet
  */
  INDIRECT(4); FUNC(4);
  initThrQuiet(psyConf->sfbCnt,
               psyConf->sfbOffset,
               sfbBarcVal,
               psyConf->sfbThresholdQuiet);


  /*
    calculate spreading function
  */
  INDIRECT(6); FUNC(8);
  initSpreading(psyConf->sfbCnt,
                sfbBarcVal,
                psyConf->sfbMaskLowFactor,
                psyConf->sfbMaskHighFactor,
                psyConf->sfbMaskLowFactorSprEn,
                psyConf->sfbMaskHighFactorSprEn,
                bitrate,
                SHORT_WINDOW);


  /*
    init ratio
  */
  INDIRECT(1); MOVE(1);
  psyConf->ratio = c_ratio;

  INDIRECT(2); MOVE(2);
  psyConf->maxAllowedIncreaseFactor = 2.0f;
  psyConf->minRemainingThresholdFactor = 0.01f;

  INDIRECT(1); MOVE(1); DIV(1);
  psyConf->clipEnergy = 1.0e9f*NORM_PCM_ENERGY / (TRANS_FAC * TRANS_FAC);

  INDIRECT(1); MULT(1); DIV(1); STORE(1);
  psyConf->lowpassLine = (int)((2*bandwidth*FRAME_LEN_SHORT)/samplerate);
  
  PTR_INIT(1); /* psyConf->sfbOffset[] */
  INDIRECT(2); LOOP(1);
  for (sfb = 0; sfb < psyConf->sfbCnt; sfb++){
    ADD(1); BRANCH(1);
    if (psyConf->sfbOffset[sfb] >= psyConf->lowpassLine)
       break;
   }
  INDIRECT(1); MOVE(1);
  psyConf->sfbActive  = sfb;

  /*
    calculate minSnr
  */
  INDIRECT(5); FUNC(7);
  initMinSnr(bitrate,
             samplerate,
             psyConf->sfbOffset[psyConf->sfbCnt],
             psyConf->sfbOffset,
             sfbBarcVal,
             psyConf->sfbActive,
             psyConf->sfbMinSnr);

  COUNT_sub_end();

  return 0;
}

