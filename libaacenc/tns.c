/*
  Temporal Noise Shaping
*/
#include "assert.h"
#include "psy_const.h"
#include "tns.h"
#include "tns_param.h"
#include "psy_configuration.h"
#include "tns_func.h"
#include "minmax.h"
#include "aac_rom.h"
#include <stdio.h>
#include <stdlib.h>

#include "counters.h" /* the 3GPP instrumenting tool */

#define RATIO_MULT        0.25f

#define TNS_MODIFY_BEGIN           2600 /* Hz */
#define RATIO_PATCH_LOWER_BORDER   380  /* Hz */
#define TNS_PARCOR_THRESH          0.1f


static void CalcGaussWindow(float *win, const int winSize, const int samplingRate,
                            const int blockType, const float timeResolution );

static void CalcWeightedSpectrum(const float spectrum[],
                                 float weightedSpectrum[],
                                 float* sfbEnergy,
                                 const int* sfbOffset, int lpcStartLine,
                                 int lpcStopLine, int lpcStartBand,int lpcStopBand);

static void AutoCorrelation(const float input[], float corr[],
                            int samples, int corrCoeff);

static float AutoToParcor(const float input[],
                          float reflCoeff[], int numOfCoeff, float workBuffer[]);


static float CalcTnsFilter(const float* signal, const float window[], int numOfLines,
                           int tnsOrder, float parcor[]);

static void Parcor2Index(const float parcor[], int index[], int order,
                         int bitsPerCoeff);

static void Index2Parcor(const int index[], float parcor[], int order,
                         int bitsPerCoeff);



static void AnalysisFilterLattice(const float signal[], int numOfLines,
                                  const float parCoeff[], int order,
                                  float output[]);


/*****************************************************************************

    functionname: FreqToBandWithRounding
    description:  Returns index of nearest band border
    returns:
    input:        frequency, sampling frequency, total number of bands,
                  table of band borders
    output:
    globals:

*****************************************************************************/
static int FreqToBandWithRounding(int freq, int fs, int numOfBands,
                                  const int *bandStartOffset)
{
  int lineNumber, band;

  COUNT_sub_start("FreqToBandWithRounding");

  /*  assert(freq >= 0);  */

  INDIRECT(1); MULT(1); DIV(2); ADD(1);
  lineNumber = (freq*bandStartOffset[numOfBands]*4/fs+1)/2;

  /* freq > fs/2 */
  ADD(1); BRANCH(1);
  if (lineNumber >= bandStartOffset[numOfBands])
  {
    COUNT_sub_end();
    return numOfBands;
  }

  /* find band the line number lies in */
  PTR_INIT(1); /* bandStartOffset[] */
  LOOP(1);
  for (band=0; band<numOfBands; band++) {
    ADD(1); BRANCH(1);
    if (bandStartOffset[band+1]>lineNumber) break;
  }

  /* round to nearest band border */
  ADD(3); BRANCH(1);
  if (lineNumber - bandStartOffset[band] >
      bandStartOffset[band+1] - lineNumber )
    {
      ADD(1);
      band++;
    }

  COUNT_sub_end();

  return band;
};


/*****************************************************************************

    functionname: InitTnsConfiguration
    description:  fill TNS_CONFIG structure with sensible content
    returns:
    input:        bitrate, samplerate, number of channels,
                  TNS Config struct (modified),
                  psy config struct,
                  tns active flag
    output:

*****************************************************************************/
int InitTnsConfiguration(   int bitRate,
                            long sampleRate,
                            int channels,
                            TNS_CONFIG *tC,
                            PSY_CONFIGURATION_LONG pc,
                            int active) 
{
  COUNT_sub_start("InitTnsConfiguration");

  INDIRECT(3); MOVE(3);
  tC->maxOrder     = TNS_MAX_ORDER;
  tC->tnsStartFreq = 1275;
  tC->coefRes      = 4;
  
  INDIRECT(1); PTR_INIT(1); DIV(1); FUNC(4); BRANCH(1); LOGIC(1);
  if ( GetTnsParam(&tC->confTab, bitRate/channels, channels, LONG_WINDOW) )
    return 1;


  INDIRECT(3); ADD(1); FUNC(5);
  CalcGaussWindow(tC->acfWindow, 
                  tC->maxOrder+1,
                  sampleRate, 
                  LONG_WINDOW,
                  tC->confTab.tnsTimeResolution);

  INDIRECT(1); PTR_INIT(1); FUNC(3);
  GetTnsMaxBands(sampleRate, LONG_WINDOW, &tC->tnsMaxSfb);

  INDIRECT(2); MOVE(1);
  tC->tnsActive = 1;

  BRANCH(1);
  if (active==0)  {
    MOVE(1);
    tC->tnsActive=0;
  }

  /*now calc band and line borders */

  INDIRECT(3); ADD(1); BRANCH(1); MOVE(1);
  tC->tnsStopBand=min(pc.sfbCnt, tC->tnsMaxSfb);

  INDIRECT(2); MOVE(1);
  tC->tnsStopLine = pc.sfbOffset[tC->tnsStopBand];

  INDIRECT(4); FUNC(4); STORE(1);
  tC->tnsStartBand=FreqToBandWithRounding(tC->tnsStartFreq, sampleRate,
                                          pc.sfbCnt, pc.sfbOffset);

  INDIRECT(3); FUNC(4); STORE(1);
  tC->tnsModifyBeginCb = FreqToBandWithRounding(TNS_MODIFY_BEGIN,
                                                sampleRate,
                                                pc.sfbCnt,
                                                pc.sfbOffset);

  INDIRECT(3); FUNC(4); STORE(1);
  tC->tnsRatioPatchLowestCb = FreqToBandWithRounding(RATIO_PATCH_LOWER_BORDER,
                                                     sampleRate,
                                                     pc.sfbCnt,
                                                     pc.sfbOffset);


  INDIRECT(2); MOVE(1);
  tC->tnsStartLine = pc.sfbOffset[tC->tnsStartBand];

  INDIRECT(3); FUNC(4); STORE(1);
  tC->lpcStopBand=FreqToBandWithRounding(tC->confTab.lpcStopFreq, sampleRate,
                                         pc.sfbCnt, pc.sfbOffset);

  INDIRECT(3); ADD(1); BRANCH(1); MOVE(1);
  tC->lpcStopBand=min(tC->lpcStopBand, pc.sfbActive);

  INDIRECT(2); MOVE(1);
  tC->lpcStopLine = pc.sfbOffset[tC->lpcStopBand];

  INDIRECT(4); FUNC(4); STORE(1);
  tC->lpcStartBand=FreqToBandWithRounding(tC->confTab.lpcStartFreq, sampleRate,
                                           pc.sfbCnt, pc.sfbOffset);

  INDIRECT(2); MOVE(1);
  tC->lpcStartLine = pc.sfbOffset[tC->lpcStartBand];

  INDIRECT(2); MOVE(1);
  tC->threshold =tC->confTab.threshOn;
  
  COUNT_sub_end();

  return 0;
}

/*****************************************************************************

    functionname: InitTnsConfigurationShort
    description:  fill TNS_CONFIG structure with sensible content
    returns:
    input:        bitrate, samplerate, number of channels,
                  TNS Config struct (modified),
                  psy config struct,
                  tns active flag
    output:

*****************************************************************************/
int InitTnsConfigurationShort(   int bitRate,
                                 long sampleRate,
                                 int channels,
                                 TNS_CONFIG *tC,
                                 PSY_CONFIGURATION_SHORT pc,
                                 int active) 
{
  COUNT_sub_start("InitTnsConfigurationShort");

  INDIRECT(3); MOVE(3);
  tC->maxOrder     = TNS_MAX_ORDER_SHORT;
  tC->tnsStartFreq = 2750;
  tC->coefRes      = 3;
  
  INDIRECT(1); PTR_INIT(1); DIV(1); FUNC(4); BRANCH(1); LOGIC(1);
  if ( GetTnsParam(&tC->confTab, bitRate/channels, channels, SHORT_WINDOW) )
    return 1;


  INDIRECT(3); ADD(1); FUNC(5);
  CalcGaussWindow(tC->acfWindow, 
                  tC->maxOrder+1,
                  sampleRate, 
                  SHORT_WINDOW,
                  tC->confTab.tnsTimeResolution);

  INDIRECT(1); PTR_INIT(1); FUNC(3);
  GetTnsMaxBands(sampleRate, SHORT_WINDOW, &tC->tnsMaxSfb);

  INDIRECT(2); MOVE(1);
  tC->tnsActive = 1;

  BRANCH(1);
  if (active==0)  {
    MOVE(1);
    tC->tnsActive=0;
  }

  /*now calc band and line borders */

  INDIRECT(3); ADD(1); BRANCH(1); MOVE(1);
  tC->tnsStopBand=min(pc.sfbCnt, tC->tnsMaxSfb);

  INDIRECT(2); MOVE(1);
  tC->tnsStopLine = pc.sfbOffset[tC->tnsStopBand];

  INDIRECT(4); FUNC(4); STORE(1);
  tC->tnsStartBand=FreqToBandWithRounding(tC->tnsStartFreq, sampleRate,
                                          pc.sfbCnt, pc.sfbOffset);

  INDIRECT(3); FUNC(4); STORE(1);
  tC->tnsModifyBeginCb = FreqToBandWithRounding(TNS_MODIFY_BEGIN,
                                                sampleRate,
                                                pc.sfbCnt,
                                                pc.sfbOffset);

  INDIRECT(3); FUNC(4); STORE(1);
  tC->tnsRatioPatchLowestCb = FreqToBandWithRounding(RATIO_PATCH_LOWER_BORDER,
                                                     sampleRate,
                                                     pc.sfbCnt,
                                                     pc.sfbOffset);


  INDIRECT(2); MOVE(1);
  tC->tnsStartLine = pc.sfbOffset[tC->tnsStartBand];

  INDIRECT(3); FUNC(4); STORE(1);
  tC->lpcStopBand=FreqToBandWithRounding(tC->confTab.lpcStopFreq, sampleRate,
                                         pc.sfbCnt, pc.sfbOffset);

  INDIRECT(3); ADD(1); BRANCH(1); MOVE(1);
  tC->lpcStopBand=min(tC->lpcStopBand, pc.sfbActive);

  INDIRECT(2); MOVE(1);
  tC->lpcStopLine = pc.sfbOffset[tC->lpcStopBand];

  INDIRECT(4); FUNC(4); STORE(1);
  tC->lpcStartBand=FreqToBandWithRounding(tC->confTab.lpcStartFreq, sampleRate,
                                          pc.sfbCnt, pc.sfbOffset);

  INDIRECT(2); MOVE(1);
  tC->lpcStartLine = pc.sfbOffset[tC->lpcStartBand];

  INDIRECT(2); MOVE(1);
  tC->threshold =tC->confTab.threshOn;
  
  COUNT_sub_end();

  return 0;
};


/*****************************************************************************
    functionname: TnsDetect
    description:  do decision, if TNS shall be used or not
    returns:
    input:        tns data structure (modified),
                  tns config structure,
                  pointer to scratch space,
                  scalefactor size and table,
                  spectrum,
                  subblock num, blocktype,
                  sfb-wise energy.

*****************************************************************************/
int TnsDetect(TNS_DATA* tnsData,
              TNS_CONFIG tC,
              float* pScratchTns,
              const int sfbOffset[],
              float* spectrum,
              int subBlockNumber,
              int blockType,
              float * sfbEnergy)
{
  float predictionGain;
  float* pWeightedSpectrum = pScratchTns + subBlockNumber*FRAME_LEN_SHORT;

  COUNT_sub_start("TnsDetect");

  MULT(1); PTR_INIT(1); /* counting previous operation */

  
  BRANCH(1);
  if (tC.tnsActive) {

    FUNC(8);
    CalcWeightedSpectrum(spectrum,
                         pWeightedSpectrum,
                         sfbEnergy,
                         sfbOffset,
                         tC.lpcStartLine,
                         tC.lpcStopLine,
                         tC.lpcStartBand,
                         tC.lpcStopBand);

    ADD(1); BRANCH(1);
    if (blockType!=SHORT_WINDOW) {

        INDIRECT(1); PTR_INIT(1); ADD(1); FUNC(5);
        predictionGain = CalcTnsFilter( &pWeightedSpectrum[tC.lpcStartLine],
                                        tC.acfWindow,
                                        tC.lpcStopLine-tC.lpcStartLine,
                                        tC.maxOrder,
                                        tnsData->dataRaw.Long.subBlockInfo.parcor);

        INDIRECT(1); ADD(1); BRANCH(1); MOVE(1);
        tnsData->dataRaw.Long.subBlockInfo.tnsActive =
            (predictionGain > tC.threshold)?1:0;

        INDIRECT(1); MOVE(1);
        tnsData->dataRaw.Long.subBlockInfo.predictionGain = predictionGain;
    }
    else{

        PTR_INIT(1); ADD(1); INDIRECT(1); BRANCH(1);
        predictionGain = CalcTnsFilter( &pWeightedSpectrum[tC.lpcStartLine],
                                        tC.acfWindow,
                                        tC.lpcStopLine-tC.lpcStartLine,
                                        tC.maxOrder,
                                        tnsData->dataRaw.Short.subBlockInfo[subBlockNumber].parcor);

        INDIRECT(1); ADD(1); BRANCH(1); MOVE(1);
        tnsData->dataRaw.Short.subBlockInfo[subBlockNumber].tnsActive =
            (predictionGain > tC.threshold)?1:0;

        INDIRECT(1); MOVE(1);
        tnsData->dataRaw.Short.subBlockInfo[subBlockNumber].predictionGain = predictionGain;
    }

  }
  else{

    ADD(1); BRANCH(1);
    if (blockType!=SHORT_WINDOW){

        INDIRECT(1); MOVE(2);
        tnsData->dataRaw.Long.subBlockInfo.tnsActive = 0;
        tnsData->dataRaw.Long.subBlockInfo.predictionGain = 0.0f;
    }
    else {

        INDIRECT(1); MOVE(2);
        tnsData->dataRaw.Short.subBlockInfo[subBlockNumber].tnsActive = 0;
        tnsData->dataRaw.Short.subBlockInfo[subBlockNumber].predictionGain = 0.0f;
    }
  }

  COUNT_sub_end();

  return 0;
}


/*****************************************************************************
    functionname: TnsSync
    description:

*****************************************************************************/
void TnsSync(TNS_DATA *tnsDataDest,
             const TNS_DATA *tnsDataSrc,
             const TNS_CONFIG tC,
             const int subBlockNumber,
             const int blockType)
{
   TNS_SUBBLOCK_INFO *sbInfoDest;
   const TNS_SUBBLOCK_INFO *sbInfoSrc;
   int i;

   COUNT_sub_start("TnsSync");

   ADD(1); BRANCH(1);
   if (blockType!=SHORT_WINDOW) {

      INDIRECT(2); PTR_INIT(2);
      sbInfoDest = &tnsDataDest->dataRaw.Long.subBlockInfo;
      sbInfoSrc  = &tnsDataSrc->dataRaw.Long.subBlockInfo;
   }
   else {

      INDIRECT(2); PTR_INIT(2);
      sbInfoDest = &tnsDataDest->dataRaw.Short.subBlockInfo[subBlockNumber];
      sbInfoSrc  = &tnsDataSrc->dataRaw.Short.subBlockInfo[subBlockNumber];
   }

   INDIRECT(2); ADD(2); MISC(1); MULT(1); BRANCH(1);
   if (fabs(sbInfoDest->predictionGain - sbInfoSrc->predictionGain) <
       ((float)0.03f * sbInfoDest->predictionGain)) {

      INDIRECT(2); MOVE(1);
      sbInfoDest->tnsActive = sbInfoSrc->tnsActive;

      PTR_INIT(2); /* sbInfoDest->parcor[]
                      sbInfoSrc->parcor[]
                   */
      LOOP(1);
      for ( i=0; i< tC.maxOrder; i++) {

        MOVE(1);
        sbInfoDest->parcor[i] = sbInfoSrc->parcor[i];
      }
   }

   COUNT_sub_end();
}


/*****************************************************************************
    functionname: TnsEncode
    description:

*****************************************************************************/
int TnsEncode(TNS_INFO* tnsInfo,
              TNS_DATA* tnsData,
              int numOfSfb,
              TNS_CONFIG tC,
              int lowPassLine,
              float* spectrum,
              int subBlockNumber,
              int blockType)
{
  int i;

  COUNT_sub_start("TnsEncode");

  ADD(1); BRANCH(1);
  if (blockType!=SHORT_WINDOW) {

    PTR_INIT(6); /* tnsInfo->numOfFilters[subBlockNumber]
                    tnsInfo->coef[subBlockNumber*TRANS_FAC]
                    tnsInfo->order[subBlockNumber]
                    tnsInfo->tnsActive[subBlockNumber]
                    tnsInfo->coefRes[subBlockNumber]
                    tnsInfo->length[subBlockNumber]
                 */

    INDIRECT(1); BRANCH(1);
    if (tnsData->dataRaw.Long.subBlockInfo.tnsActive == 0) {

      INDIRECT(1); MOVE(1);
      tnsInfo->tnsActive[subBlockNumber] = 0;

      COUNT_sub_end();
      return 0;
    }
    else {

      INDIRECT(2); FUNC(4);
      Parcor2Index(tnsData->dataRaw.Long.subBlockInfo.parcor,
                   &tnsInfo->coef[0],
                   tC.maxOrder,
                   tC.coefRes);

      INDIRECT(2); FUNC(4);
      Index2Parcor(&tnsInfo->coef[0],
                   tnsData->dataRaw.Long.subBlockInfo.parcor,
                   tC.maxOrder,
                   tC.coefRes);

      PTR_INIT(1); /* tnsData->dataRaw.Long.subBlockInfo.parcor[i] */
      LOOP(1);
      for (i=tC.maxOrder-1; i>=0; i--)  {

        ADD(2); LOGIC(1); BRANCH(1);
        if (tnsData->dataRaw.Long.subBlockInfo.parcor[i]>TNS_PARCOR_THRESH  ||
            tnsData->dataRaw.Long.subBlockInfo.parcor[i]<-TNS_PARCOR_THRESH)
          break;
      }
      INDIRECT(1); ADD(1); STORE(1);
      tnsInfo->order[subBlockNumber]=i+1;

      INDIRECT(1); MOVE(1);
      tnsInfo->tnsActive[subBlockNumber]=1;

      PTR_INIT(1); /* tnsInfo->tnsActive[] */
      LOOP(1);
      for (i=subBlockNumber+1; i<TRANS_FAC; i++) {

        MOVE(1);
        tnsInfo->tnsActive[i]=0;
      }

      INDIRECT(1); MOVE(1);
      tnsInfo->coefRes[subBlockNumber]=tC.coefRes;

      INDIRECT(1); ADD(1); STORE(1);
      tnsInfo->length[subBlockNumber]= numOfSfb-tC.tnsStartBand;


      INDIRECT(2); PTR_INIT(1); ADD(2); BRANCH(1); MOVE(1); FUNC(5);
      AnalysisFilterLattice(&(spectrum[tC.tnsStartLine]),
                            min(tC.tnsStopLine,lowPassLine)-tC.tnsStartLine,
                            tnsData->dataRaw.Long.subBlockInfo.parcor,
                            tnsInfo->order[subBlockNumber],
                            &(spectrum[tC.tnsStartLine]));

    }
  }     /* if (blockType!=SHORT_WINDOW) */
  else /*short block*/ {

    PTR_INIT(6); /* tnsData->dataRaw.Short.subBlockInfo[subBlockNumber]
                    tnsInfo->tnsActive[subBlockNumber]
                    tnsInfo->coef[subBlockNumber*TRANS_FAC]
                    tnsInfo->order[subBlockNumber]
                    tnsInfo->coefRes[subBlockNumber]
                    tnsInfo->length[subBlockNumber]
                 */

    INDIRECT(1); BRANCH(1);
    if (tnsData->dataRaw.Short.subBlockInfo[subBlockNumber].tnsActive == 0) {

      INDIRECT(1); MOVE(1);
      tnsInfo->tnsActive[subBlockNumber] = 0;

      COUNT_sub_end();
      return(0);
    }
    else {

      INDIRECT(2); FUNC(4);
      Parcor2Index(tnsData->dataRaw.Short.subBlockInfo[subBlockNumber].parcor,
                   &tnsInfo->coef[subBlockNumber*TNS_MAX_ORDER_SHORT],
                   tC.maxOrder,
                   tC.coefRes);

      INDIRECT(2); FUNC(4);
      Index2Parcor(&tnsInfo->coef[subBlockNumber*TNS_MAX_ORDER_SHORT],
                   tnsData->dataRaw.Short.subBlockInfo[subBlockNumber].parcor,
                   tC.maxOrder,
                   tC.coefRes);

      PTR_INIT(1); /* tnsData->dataRaw.Short.subBlockInfo[subBlockNumber].parcor[] */
      LOOP(1);
      for (i=tC.maxOrder-1; i>=0; i--)  {

        INDIRECT(1); ADD(2); LOGIC(1); BRANCH(1);
        if (tnsData->dataRaw.Short.subBlockInfo[subBlockNumber].parcor[i]>TNS_PARCOR_THRESH  ||
            tnsData->dataRaw.Short.subBlockInfo[subBlockNumber].parcor[i]<-TNS_PARCOR_THRESH)
          break;
      }

      INDIRECT(1); ADD(1); STORE(1);
      tnsInfo->order[subBlockNumber]=i+1;

      INDIRECT(1); MOVE(1);
      tnsInfo->tnsActive[subBlockNumber]=1;

      INDIRECT(1); MOVE(1);
      tnsInfo->coefRes[subBlockNumber]=tC.coefRes;

      INDIRECT(1); ADD(1); STORE(1);
      tnsInfo->length[subBlockNumber]= numOfSfb-tC.tnsStartBand;


      INDIRECT(2); PTR_INIT(1); ADD(1); FUNC(5);
      AnalysisFilterLattice(&(spectrum[tC.tnsStartLine]), tC.tnsStopLine-tC.tnsStartLine,
                 tnsData->dataRaw.Short.subBlockInfo[subBlockNumber].parcor,
                 tnsInfo->order[subBlockNumber],
                 &(spectrum[tC.tnsStartLine]));
      
    }
  }

  COUNT_sub_end();

  return 0;
}







/*****************************************************************************

    functionname: CalcGaussWindow
    edscription:  calculates Gauss window for acf windowing depending on desired
                  temporal resolution, transform size and sampling rate
    returns:      -
    input:        window size, fs, bitRate, no. of transform lines, time res.
    output:       window coefficients (right half)

*****************************************************************************/
static void CalcGaussWindow(float  *win,
                            const int winSize,
                            const int samplingRate,
                            const int blockType,
                            const float timeResolution )
{
  int     i;
  float gaussExp = 3.14159265358979323f * samplingRate * 0.001f * (float)timeResolution / (blockType != SHORT_WINDOW ? 1024.0f:128.0f);
   
  COUNT_sub_start("CalcGaussWindow");

    ADD(1); BRANCH(1); MOVE(1); /* .. != .. ? */ MULT(2); DIV(1);

    MULT(2);
    gaussExp = -0.5f * gaussExp * gaussExp;

    PTR_INIT(1); /* win[] */
    LOOP(1);
    for(i=0; i<winSize; i++) {

      ADD(1); MULT(2); TRANS(1); STORE(1);
      win[i] = (float) exp( gaussExp * (i+0.5) * (i+0.5) );
    }

  COUNT_sub_end();
}




/*****************************************************************************

    functionname: CalcWeightedSpectrum
    description:  calculate weighted spectrum for LPC calculation
    returns:      -
    input:        input spectrum, ptr. to weighted spectrum, no. of lines,
                  sfb energies
    output:       weighted spectrum coefficients

*****************************************************************************/
static void CalcWeightedSpectrum(const float    spectrum[],
                                 float          weightedSpectrum[],
                                 float        *sfbEnergy,
                                 const int     *sfbOffset,
                                 int            lpcStartLine,
                                 int            lpcStopLine,
                                 int            lpcStartBand,
                                 int            lpcStopBand)
{
    int     i, sfb;
    float   tmp;
    float   tnsSfbMean[MAX_SFB];    /* length [lpcStopBand-lpcStartBand] should be sufficient here */

    COUNT_sub_start("CalcWeightedSpectrum");

    /* calc 1/sqrt(en) */
    PTR_INIT(2); /* tnsSfbMean[]
                    sfbEnergy[]
                 */
    LOOP(1);
    for( sfb = lpcStartBand; sfb < lpcStopBand; sfb++){

      ADD(1); TRANS(1); DIV(1); STORE(1);
      tnsSfbMean[sfb] = (float) ( 1.0 / sqrt(sfbEnergy[sfb] + 1e-30f) );
    }

    /* spread normalized values from sfbs to lines */
    MOVE(1);
    sfb = lpcStartBand;

    PTR_INIT(1); /* tnsSfbMean[] */
    MOVE(1);
    tmp = tnsSfbMean[sfb];

    PTR_INIT(2); /* sfbOffset[]
                    weightedSpectrum[]
                 */
    LOOP(1);
    for ( i=lpcStartLine; i<lpcStopLine; i++){

        ADD(1); BRANCH(1);
        if (sfbOffset[sfb+1]==i){

            ADD(1);
            sfb++;

            ADD(2); BRANCH(1);
            if (sfb+1 < lpcStopBand){

                MOVE(1);
                tmp = tnsSfbMean[sfb];
            }
        }

        MOVE(1);
        weightedSpectrum[i] = tmp;
    }

    /*filter down*/
    PTR_INIT(1); /* weightedSpectrum[] */
    ADD(1); LOOP(1);
    for (i=lpcStopLine-2; i>=lpcStartLine; i--){
      ADD(1); MULT(1); STORE(1);
      weightedSpectrum[i] = (weightedSpectrum[i] + weightedSpectrum[i+1]) * 0.5f;
    }

    /* filter up */
    PTR_INIT(1); /* weightedSpectrum[] */
    ADD(1); LOOP(1);
    for (i=lpcStartLine+1; i<lpcStopLine; i++){

      ADD(1); MULT(1); STORE(1);
      weightedSpectrum[i] = (weightedSpectrum[i] + weightedSpectrum[i-1]) * 0.5f;
    }

    /* weight and normalize */
    PTR_INIT(1); /* weightedSpectrum[] */
    LOOP(1);
    for (i=lpcStartLine; i<lpcStopLine; i++){

      MULT(1); STORE(1);
      weightedSpectrum[i] = weightedSpectrum[i] * spectrum[i];
    }

    COUNT_sub_end();
}



/*****************************************************************************

    functionname: CalcTnsFilter
    description:  LPC calculation for one TNS filter
    returns:      prediction gain
    input:        signal spectrum, acf window, no. of spectral lines,
                  max. TNS order, ptr. to reflection ocefficients
    output:       reflection coefficients

*****************************************************************************/
static float CalcTnsFilter(const float *signal,
                            const float window[],
                            int numOfLines,
                            int tnsOrder,
                            float parcor[])
{
    float   autoCorrelation[TNS_MAX_ORDER+1];
    float   parcorWorkBuffer[2*TNS_MAX_ORDER];
    float  predictionGain;
    int     i;

    COUNT_sub_start("CalcTnsFilter");

    assert(tnsOrder <= TNS_MAX_ORDER);

    ADD(1); FUNC(4);
    AutoCorrelation(signal, autoCorrelation, numOfLines, tnsOrder+1);

    BRANCH(1);
    if(window) {

        PTR_INIT(2); /* autoCorrelation[]
                        window[]
                     */
        LOOP(1);
        for(i=0; i<tnsOrder+1; i++) {

          MULT(1); STORE(1);
          autoCorrelation[i] = autoCorrelation[i] * window[i];
        }
    }

    FUNC(4);
    predictionGain = AutoToParcor(autoCorrelation, parcor, tnsOrder, parcorWorkBuffer);

    COUNT_sub_end();

    return(predictionGain);
}

/*****************************************************************************

    functionname: AutoCorrelation
    description:  calc. autocorrelation (acf)
    returns:      -
    input:        input values, no. of input values, no. of acf values
    output:       acf values

*****************************************************************************/
static void AutoCorrelation(const float input[],
                            float       corr[],
                            int         samples,
                            int         corrCoeff) {
    int         i, j;
    float       accu;
    int         scf;

    COUNT_sub_start("AutoCorrelation");
  
    /* 
      get next power of 2 of samples 
    */
    LOOP(1);
    for(scf=0;(1<<scf) < samples;scf++);
   
    MOVE(1);
    accu = 0.0;

    PTR_INIT(1); /* pointer for input[] */
    LOOP(1);
    for(j=0; j<samples; j++) {

      MAC(1);
      accu += input[j] * input[j];
    }

    MOVE(1);
    corr[0] = accu;

    ADD(1);
    samples--;

    PTR_INIT(1); /* pointer for corr[] */
    LOOP(1);
    for(i=1; i<corrCoeff; i++) {

        MOVE(1);
        accu = 0.0;

        PTR_INIT(2); /* pointer for input[], input[j+i] */
        LOOP(1);
        for(j=0; j<samples; j++) {

          MAC(1);
          accu += input[j] * input[j+i];
        }

        MOVE(1);
        corr[i] = accu;

        ADD(1);
        samples--;
    }

    COUNT_sub_end();
}    


/*****************************************************************************

    functionname: AutoToParcor
    description:  conversion autocorrelation to reflection coefficients
    returns:      prediction gain
    input:        <order+1> input values, no. of output values (=order),
                  ptr. to workbuffer (required size: 2*order)
    output:       <order> reflection coefficients

*****************************************************************************/
static float AutoToParcor(const float input[], float reflCoeff[], int numOfCoeff,
                          float workBuffer[]) {
  int i, j;
  float  *pWorkBuffer; /* temp pointer */
  float predictionGain = 1.0f;

  COUNT_sub_start("AutoToParcor");

  MOVE(1); /* counting previous operation */

  PTR_INIT(1); /* reflCoeff[] */
  LOOP(1);
  for(i=0;i<numOfCoeff;i++)
  {
    MOVE(1);
    reflCoeff[i] = 0;
  }

  BRANCH(1);
  if(input[0] == 0.0)
  {
    COUNT_sub_end();
    return(predictionGain);
  }

  PTR_INIT(3); /* workBuffer[]
                  workBuffer[i+numOfCoeff]
                  input[]
               */
  LOOP(1);
  for(i=0; i<numOfCoeff; i++) {

    MOVE(2);
    workBuffer[i] = input[i];
    workBuffer[i+numOfCoeff] = input[i+1];
  }

  PTR_INIT(2); /* workBuffer[i+numOfCoeff]
                  reflCoeff[i]
               */
  LOOP(1);
  for(i=0; i<numOfCoeff; i++) {
    float refc, tmp;

    MOVE(1);
    tmp = workBuffer[numOfCoeff + i];

    BRANCH(1);
    if(tmp < 0.0)
    {
      MULT(1);
      tmp = -tmp;
    }

    ADD(1); BRANCH(1);
    if(workBuffer[0] < tmp)
      break;

    BRANCH(1);
    if(workBuffer[0] == 0.0f) {

      MOVE(1);
      refc = 0.0f;
    } else {

      DIV(1);
      refc = tmp / workBuffer[0];
    }

    BRANCH(1);
    if(workBuffer[numOfCoeff + i] > 0.0)
    {
      MULT(1);
      refc = -refc;
    }

    MOVE(1);
    reflCoeff[i] = refc;

    PTR_INIT(1);
    pWorkBuffer = &(workBuffer[numOfCoeff]);

    PTR_INIT(1); /* workBuffer[j-i] */
    LOOP(1);
    for(j=i; j<numOfCoeff; j++) {
      float accu1, accu2;

      MAC(2); STORE(2);
      accu1 = pWorkBuffer[j]  + refc * workBuffer[j-i];
      accu2 = workBuffer[j-i] + refc * pWorkBuffer[j];
      pWorkBuffer[j] = accu1;
      workBuffer[j-i] = accu2;
    }
  }

  ADD(2); DIV(1);
  predictionGain = (input[0] + 1e-30f) / (workBuffer[0] + 1e-30f);

  COUNT_sub_end();

  return(predictionGain);
}



static int Search3(float parcor)
{
  int index=0;
  int i;

  COUNT_sub_start("Search3");

  MOVE(1); /* counting previous operation */

  PTR_INIT(1); /* tnsCoeff3Borders[] */
  LOOP(1);
  for(i=0;i<8;i++){

    ADD(1); BRANCH(1);
    if(parcor > tnsCoeff3Borders[i])
    {
      MOVE(1);
      index=i;
    }
  }

  ADD(1); /* counting post-operation */
  COUNT_sub_end();

  return(index-4);
}

static int Search4(float parcor)
{
  int index=0;
  int i;

  COUNT_sub_start("Search4");

  MOVE(1); /* counting previous operation */

  PTR_INIT(1); /* tnsCoeff4Borders[] */
  LOOP(1);
  for(i=0;i<16;i++){

    ADD(1); BRANCH(1);
    if(parcor > tnsCoeff4Borders[i])
    {
      MOVE(1);
      index=i;
    }
  }

  ADD(1); /* counting post-operation */
  COUNT_sub_end();

  return(index-8);
}



/*****************************************************************************

    functionname: Parcor2Index

*****************************************************************************/
static void Parcor2Index(const float parcor[], int index[], int order,
                         int bitsPerCoeff) {
  int i;

  COUNT_sub_start("Parcor2Index");

  PTR_INIT(2); /* index[]
                  parcor[]
               */
  LOOP(1);
  for(i=0; i<order; i++) {

    ADD(1); BRANCH(1);
    if(bitsPerCoeff == 3)
    {
      FUNC(1); STORE(1);
      index[i] = Search3(parcor[i]);
    }
    else
    {
      FUNC(1); STORE(1);
      index[i] = Search4(parcor[i]);
    }
  }

  COUNT_sub_end();
}


/*****************************************************************************

    functionname: Index2Parcor
    description:  inverse quantization for reflection coefficients
    returns:      -
    input:        quantized values, ptr. to reflection coefficients,
                  no. of coefficients, resolution
    output:       reflection coefficients

*****************************************************************************/
static void Index2Parcor(const int index[], float parcor[], int order,
                         int bitsPerCoeff) {
  int i;

  COUNT_sub_start("Index2Parcor");

  PTR_INIT(2); /* parcor[]
                  index[]
               */
  LOOP(1);
  for(i=0; i<order; i++) {

    ADD(1); BRANCH(1); /* .. == .. ? */ INDIRECT(1); MOVE(1);
    parcor[i] = bitsPerCoeff == 4 ? tnsCoeff4[index[i]+8] : tnsCoeff3[index[i]+4];
  }

  COUNT_sub_end();
}



/*****************************************************************************

    functionname: FIRLattice

*****************************************************************************/
static float FIRLattice(int order, 
                        float x,
                        float *state_par,
                        const float *coef_par)
{
   int i;
   float accu, tmp, tmpSave;

   COUNT_sub_start("FIRLattice");

   MOVE(1);
   tmpSave = x;

   PTR_INIT(2); /* coef_par[]
                   state_par[]
                */
   LOOP(1);
   for(i=0; i<order-1; i++) {

     MULT(1);
     tmp      = coef_par[i] * x;

     ADD(1);
     tmp     += state_par[i];

     MULT(1);
     accu     = coef_par[i] * state_par[i];

     ADD(1);
     accu    += x;

     MOVE(3);
     x        = accu;
     state_par[i] = tmpSave;
     tmpSave  = tmp;
  }

  /* last stage: only need half operations */

  MULT(1);
  accu  = state_par[order-1] * coef_par[order-1];

  ADD(1);
  accu += x;

  MOVE(2);
  x     = accu;
  state_par[order-1] = tmpSave;

  COUNT_sub_end();

  return x;
}



/*****************************************************************************

    functionname: AnalysisFilterLattice
    description:  TNS analysis filter
    returns:      -
    input:        input signal values, no. of lines, PARC coefficients,
                  filter order, ptr. to output values, filtering direction
    output:       filtered signal values

*****************************************************************************/
static void AnalysisFilterLattice(const float signal[], int numOfLines,
                                  const float parCoeff[], int order,
                                  float output[])
{

  float state_par[TNS_MAX_ORDER] = {0.0f};
  int j;

  COUNT_sub_start("AnalysisFilterLattice");

  MOVE(TNS_MAX_ORDER); /* counting previous operation */

  PTR_INIT(2); /* output[]
                  signal[]
               */
  LOOP(1);
  for(j=0; j<numOfLines; j++) {

    FUNC(4); STORE(1);
    output[j] = FIRLattice(order,signal[j],state_par,parCoeff);
  }

  COUNT_sub_end();
}


/*****************************************************************************

    functionname: ApplyTnsMultTableToRatios
    description:  change thresholds according to tns
    returns:      -
    input:        modifier table,
                  TNS subblock info,
                  thresholds (modified),
                  number of bands

*****************************************************************************/
void ApplyTnsMultTableToRatios(int startCb,
                               int stopCb,
                               float *thresholds) 
{
  int i;

  COUNT_sub_start("ApplyTnsMultTableToRatios");

  PTR_INIT(1); /* thresholds[]
               */
  LOOP(1);
  for(i=startCb; i<stopCb; i++) {
    
    MULT(1); STORE(1);
    thresholds[i] = thresholds[i]*RATIO_MULT;
  }
  
  COUNT_sub_end();
}
