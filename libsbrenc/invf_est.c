/*
  Sbr QMF inverse filtering detector
*/
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "invf_est.h"
#include "sbr_misc.h"
#include "sbr_def.h"

#include "counters.h" /* the 3GPP instrumenting tool */



#define MAX_NUM_REGIONS 10



#ifndef min
#define min(a,b) ( a < b ? a:b)
#endif

#ifndef max
#define max(a,b) ( a > b ? a:b)
#endif






static float quantStepsSbr[4]  = {1, 10, 14, 19};
static float quantStepsOrig[4] = {0,  3,  7, 10};
static float nrgBorders[4] =     {25.0f, 30.0f, 35.0f, 40.0f};

static DETECTOR_PARAMETERS detectorParamsAAC = {
    quantStepsSbr,
    quantStepsOrig,
    nrgBorders,
    4,
    4,
    4,
    {
      {INVF_MID_LEVEL,   INVF_LOW_LEVEL,  INVF_OFF,        INVF_OFF, INVF_OFF},
      {INVF_MID_LEVEL,   INVF_LOW_LEVEL,  INVF_OFF,        INVF_OFF, INVF_OFF},
      {INVF_HIGH_LEVEL,  INVF_MID_LEVEL,  INVF_LOW_LEVEL,  INVF_OFF, INVF_OFF},
      {INVF_HIGH_LEVEL,  INVF_HIGH_LEVEL, INVF_MID_LEVEL,  INVF_OFF, INVF_OFF},
      {INVF_HIGH_LEVEL,  INVF_HIGH_LEVEL, INVF_MID_LEVEL,  INVF_OFF, INVF_OFF}
    },
    {
      {INVF_LOW_LEVEL,   INVF_LOW_LEVEL,  INVF_LOW_LEVEL,  INVF_OFF, INVF_OFF},
      {INVF_LOW_LEVEL,   INVF_LOW_LEVEL,  INVF_LOW_LEVEL,  INVF_OFF, INVF_OFF},
      {INVF_HIGH_LEVEL,  INVF_MID_LEVEL,  INVF_MID_LEVEL,  INVF_OFF, INVF_OFF},
      {INVF_HIGH_LEVEL,  INVF_HIGH_LEVEL, INVF_MID_LEVEL,  INVF_OFF, INVF_OFF},
      {INVF_HIGH_LEVEL,  INVF_HIGH_LEVEL, INVF_MID_LEVEL,  INVF_OFF, INVF_OFF}
    },
    {-4, -3, -2, -1, 0}
};

static const float hysteresis = 1.0f;


static DETECTOR_PARAMETERS detectorParamsAACSpeech = {
    quantStepsSbr,
    quantStepsOrig,
    nrgBorders,
    4,
    4,
    4,
    {
      {INVF_MID_LEVEL,   INVF_MID_LEVEL,  INVF_LOW_LEVEL,  INVF_OFF, INVF_OFF},
      {INVF_MID_LEVEL,   INVF_MID_LEVEL,  INVF_LOW_LEVEL,  INVF_OFF, INVF_OFF},
      {INVF_HIGH_LEVEL,  INVF_MID_LEVEL,  INVF_MID_LEVEL,  INVF_OFF, INVF_OFF},
      {INVF_HIGH_LEVEL,  INVF_HIGH_LEVEL, INVF_MID_LEVEL,  INVF_OFF, INVF_OFF},
      {INVF_HIGH_LEVEL,  INVF_HIGH_LEVEL, INVF_MID_LEVEL,  INVF_OFF, INVF_OFF}
    },
    {
      {INVF_MID_LEVEL,   INVF_MID_LEVEL,  INVF_LOW_LEVEL,  INVF_OFF, INVF_OFF},
      {INVF_MID_LEVEL,   INVF_MID_LEVEL,  INVF_LOW_LEVEL,  INVF_OFF, INVF_OFF},
      {INVF_HIGH_LEVEL,  INVF_MID_LEVEL,  INVF_MID_LEVEL,  INVF_OFF, INVF_OFF},
      {INVF_HIGH_LEVEL,  INVF_HIGH_LEVEL, INVF_MID_LEVEL,  INVF_OFF, INVF_OFF},
      {INVF_HIGH_LEVEL,  INVF_HIGH_LEVEL, INVF_MID_LEVEL,  INVF_OFF, INVF_OFF}
    },
    {-4, -3, -2, -1, 0}
};


typedef const float FIR_FILTER[5];

static FIR_FILTER fir_0 = { 1.0f };
static FIR_FILTER fir_1 = { 0.3333333f, 0.6666666f };
static FIR_FILTER fir_2 = { 0.125f, 0.375f, 0.5f };
static FIR_FILTER fir_3 = { 0.0585786f, 0.2f, 0.3414214f, 0.4f };
static FIR_FILTER fir_4 = { 0.0318305f, 0.1151638f, 0.2181695f, 0.3015028f, 0.3333333f };

static FIR_FILTER *fir_table[5] = {
  &fir_0,
  &fir_1,
  &fir_2,
  &fir_3,
  &fir_4
};



/**************************************************************************/
/*!
  \brief     Calculates the values used for the detector.

  \return    none

*/
/**************************************************************************/
static void
calculateDetectorValues(float **quotaMatrixOrig,
                        char * indexVector,
                        float* nrgVector,
                        DETECTOR_VALUES *detectorValues,
                        int startChannel,
                        int stopChannel,
                        int startIndex,
                        int stopIndex
                        )
{
  int i,j;

  float origQuota, sbrQuota;

  const float* filter = *fir_table[INVF_SMOOTHING_LENGTH];

  float quotaVecOrig[64], quotaVecSbr[64];

  COUNT_sub_start("calculateDetectorValues");

  PTR_INIT(1); /* counting previous operation */

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(64);
  memset(quotaVecOrig,0,64*sizeof(float));

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(64);
  memset(quotaVecSbr ,0,64*sizeof(float));


  INDIRECT(1); MOVE(1);
  detectorValues->avgNrg = 0;

  PTR_INIT(3); /* quotaVecOrig[],
                  indexVector[],
                  quotaVecSbr[]
               */
  LOOP(1);
  for(j = startIndex ; j < stopIndex ; j++){

    PTR_INIT(1); /* quotaMatrixOrig[][] */
    LOOP(1);
    for(i = startChannel; i < stopChannel; i++) {

      ADD(1); STORE(1);
      quotaVecOrig[i] += (quotaMatrixOrig[j][i]);

      ADD(1); BRANCH(1);
      if(indexVector[i] != -1)
      {
        INDIRECT(1); ADD(1); STORE(1);
        quotaVecSbr[i] += (quotaMatrixOrig[j][indexVector[i]]);
      }
    }
    INDIRECT(1); ADD(1); STORE(1);
    detectorValues->avgNrg += nrgVector[j];
  }

  INDIRECT(1); ADD(1); DIV(1); STORE(1);
  detectorValues->avgNrg /= (stopIndex-startIndex);


  PTR_INIT(2); /* quotaVecOrig[]
                  quotaVecSbr[]
               */
  LOOP(1);
  for(i = startChannel; i < stopChannel; i++) {

    DIV(2); STORE(2);
    quotaVecOrig[i] /= (stopIndex-startIndex);
    quotaVecSbr[i]  /= (stopIndex-startIndex);
  }

  MOVE(2);
  origQuota = 0.0f;
  sbrQuota  = 0.0f;

  PTR_INIT(2); /* quotaVecOrig[],
                  quotaVecSbr[]
               */
  LOOP(1);
  for(i = startChannel; i < stopChannel; i++) {

    ADD(2);
    origQuota += quotaVecOrig[i];
    sbrQuota  += quotaVecSbr[i];
  }

  ADD(1); DIV(2);
  origQuota /= (stopChannel - startChannel);
  sbrQuota  /= (stopChannel - startChannel);


  FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(INVF_SMOOTHING_LENGTH);
  memmove(detectorValues->origQuotaMean, detectorValues->origQuotaMean + 1, INVF_SMOOTHING_LENGTH*sizeof(float));

  FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(INVF_SMOOTHING_LENGTH);
  memmove(detectorValues->sbrQuotaMean, detectorValues->sbrQuotaMean + 1, INVF_SMOOTHING_LENGTH*sizeof(float));


  INDIRECT(2); MOVE(2);
  detectorValues->origQuotaMean[INVF_SMOOTHING_LENGTH]          = origQuota;
  detectorValues->sbrQuotaMean[INVF_SMOOTHING_LENGTH]           = sbrQuota;



  INDIRECT(2); MOVE(2);
  detectorValues->origQuotaMeanFilt = 0;
  detectorValues->sbrQuotaMeanFilt = 0;


  PTR_INIT(3); /* detectorValues->origQuotaMean[]
                  detectorValues->sbrQuotaMean[]
                  filter[]
               */
  for(i=0;i<INVF_SMOOTHING_LENGTH+1;i++) {
    MAC(2);
    detectorValues->origQuotaMeanFilt += detectorValues->origQuotaMean[i]*filter[i];
    detectorValues->sbrQuotaMeanFilt  += detectorValues->sbrQuotaMean[i]*filter[i];
  }
  INDIRECT(2); STORE(2); /* store previous calculations */

  COUNT_sub_end();
}





/**************************************************************************/
/*!
  \brief     Returns the region in which the input value belongs.

  \return    region.

*/
/**************************************************************************/
static int
findRegion(float currVal,
           const float* borders,
           const int numBorders,
           int prevRegion
           )
{
  int i;

  COUNT_sub_start("findRegion");

  ADD(1); BRANCH(1);
  if(currVal < borders[0])
  {
    COUNT_sub_end();
    return 0;
  }

  PTR_INIT(1); /* borders[] */
  LOOP(1);
  for(i = 1; i < numBorders; i++){

    ADD(2); LOGIC(1); BRANCH(1);
    if( currVal >= borders[i-1] && currVal < borders[i])
    {
      COUNT_sub_end();
      return i;
    }
  }

  ADD(1); BRANCH(1);
  if(currVal > borders[numBorders-1])
  {
    COUNT_sub_end();
    return numBorders;
  }

  COUNT_sub_end();
  return 0;
}





/**************************************************************************/
/*!
  \brief     Makes a decision based on the quota vector.

  \return     decision on which invf mode to use

*/
/**************************************************************************/
static INVF_MODE
decisionAlgorithm(const DETECTOR_PARAMETERS* detectorParams,
                  DETECTOR_VALUES detectorValues,
                  int transientFlag,
                  INVF_MODE prevInvfMode,
                  int* prevRegionSbr,
                  int* prevRegionOrig
                  )
{
  int invFiltLevel, regionSbr, regionOrig, regionNrg;

  const float *quantStepsSbr  = detectorParams->quantStepsSbr;
  const float *quantStepsOrig = detectorParams->quantStepsOrig;
  const float *nrgBorders     = detectorParams->nrgBorders;
  const int numRegionsSbr     = detectorParams->numRegionsSbr;
  const int numRegionsOrig    = detectorParams->numRegionsOrig;
  const int numRegionsNrg     = detectorParams->numRegionsNrg;

  float quantStepsSbrTmp[MAX_NUM_REGIONS];
  float quantStepsOrigTmp[MAX_NUM_REGIONS];

  float origQuotaMeanFilt;
  float sbrQuotaMeanFilt;
  float nrg;

  COUNT_sub_start("decisionAlgorithm");

  INDIRECT(6); MOVE(3); PTR_INIT(3); /* counting previous operations */

  ADD(3); MULT(3); TRANS(3);
  origQuotaMeanFilt = (float) (ILOG2*3.0*log(detectorValues.origQuotaMeanFilt+EPS));
  sbrQuotaMeanFilt  = (float) (ILOG2*3.0*log(detectorValues.sbrQuotaMeanFilt+EPS));
  nrg               = (float) (ILOG2*1.5*log(detectorValues.avgNrg+EPS));




  FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(numRegionsSbr);
  memcpy(quantStepsSbrTmp,quantStepsSbr,numRegionsSbr*sizeof(float));

  FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(numRegionsOrig);
  memcpy(quantStepsOrigTmp,quantStepsOrig,numRegionsOrig*sizeof(float));


  PTR_INIT(2); /* quantStepsSbrTmp[*prevRegionSbr]
                  quantStepsOrigTmp[*prevRegionOrig]
               */

  ADD(1); BRANCH(1);
  if(*prevRegionSbr < numRegionsSbr)
  {
    ADD(1); STORE(1);
    quantStepsSbrTmp[*prevRegionSbr] = quantStepsSbr[*prevRegionSbr] + hysteresis;
  }

  BRANCH(1);
  if(*prevRegionSbr > 0)
  {
    ADD(1); STORE(1);
    quantStepsSbrTmp[*prevRegionSbr - 1] = quantStepsSbr[*prevRegionSbr - 1] - hysteresis;
  }


  if(*prevRegionOrig < numRegionsOrig)
  {
    ADD(1); STORE(1);
    quantStepsOrigTmp[*prevRegionOrig] = quantStepsOrig[*prevRegionOrig] + hysteresis;
  }

  BRANCH(1);
  if(*prevRegionOrig > 0)
  {
    ADD(1); STORE(1);
    quantStepsOrigTmp[*prevRegionOrig - 1] = quantStepsOrig[*prevRegionOrig - 1] - hysteresis;
  }

  FUNC(4);
  regionSbr  = findRegion(sbrQuotaMeanFilt, quantStepsSbrTmp, numRegionsSbr, *prevRegionSbr);

  FUNC(4);
  regionOrig = findRegion(origQuotaMeanFilt, quantStepsOrigTmp, numRegionsOrig, *prevRegionOrig);

  FUNC(4);
  regionNrg  = findRegion(nrg,nrgBorders,numRegionsNrg, 0);


  MOVE(2);
  *prevRegionSbr = regionSbr;
  *prevRegionOrig = regionOrig;

  ADD(1); BRANCH(1);
  if(transientFlag == 1){

    INDIRECT(1); MOVE(1);
    invFiltLevel = detectorParams->regionSpaceTransient[regionSbr][regionOrig];
  }
  else{
    INDIRECT(1); MOVE(1);
    invFiltLevel = detectorParams->regionSpace[regionSbr][regionOrig];
  }

  INDIRECT(1); ADD(2); BRANCH(1); MOVE(1);
  invFiltLevel = max(invFiltLevel + detectorParams->EnergyCompFactor[regionNrg],0);

  COUNT_sub_end();

  return (INVF_MODE) (invFiltLevel);
}




/**************************************************************************/
/*!
  \brief     Estiamtion of the inverse filtering level required
             in the decoder.

 \return    none.

*/
/**************************************************************************/
void
qmfInverseFilteringDetector (HANDLE_SBR_INV_FILT_EST hInvFilt,
                             float ** quotaMatrix,
                             float *nrgVector,
                             char* indexVector,
                             int startIndex,
                             int stopIndex,
                             int transientFlag,
                             INVF_MODE* infVec
                             )
{
  int band;


  COUNT_sub_start("qmfInverseFilteringDetector");

  PTR_INIT(6); /* hInvFilt->freqBandTableInvFilt[band]
                  hInvFilt->detectorValues[band]
                  hInvFilt->prevInvfMode[band]
                  hInvFilt->prevRegionSbr[band]
                  hInvFilt->prevRegionOrig[band]
                  infVec[band]
               */
  INDIRECT(1); LOOP(1);
  for(band = 0 ; band < hInvFilt->noDetectorBands; band++){
    int startChannel = hInvFilt->freqBandTableInvFilt[band];
    int stopChannel  = hInvFilt->freqBandTableInvFilt[band+1];

    MOVE(2); /* counting previous operations */

    INDIRECT(1); PTR_INIT(1); FUNC(9);
    calculateDetectorValues(quotaMatrix,
                            indexVector,
                            nrgVector,
                            &hInvFilt->detectorValues[band],
                            startChannel,
                            stopChannel,
                            startIndex,
                            stopIndex
                            );


    INDIRECT(1); PTR_INIT(2); FUNC(6); STORE(1);
    infVec[band]= decisionAlgorithm(hInvFilt->detectorParams,
                                    hInvFilt->detectorValues[band],
                                    transientFlag,
                                    hInvFilt->prevInvfMode[band],
                                    &hInvFilt->prevRegionSbr[band],
                                    &hInvFilt->prevRegionOrig[band]);

  }

  COUNT_sub_end();
}


/**************************************************************************/
/*!
  \brief     Creates an instance of the inverse filtering level estimator.

  \return   errorCode

*/
/**************************************************************************/
int
createInvFiltDetector (HANDLE_SBR_INV_FILT_EST hInvFilt,
                       int* freqBandTableDetector,
                       int numDetectorBands,
                       int numberOfEstimatesPerFrame,
                       unsigned int useSpeechConfig
                       )
{
  int i;

  COUNT_sub_start("createInvFiltDetector");


  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(SBR_INV_FILT_EST));
  memset( hInvFilt,0,sizeof(SBR_INV_FILT_EST));

  BRANCH(1);
  if (useSpeechConfig) {
    INDIRECT(1); PTR_INIT(1);
    hInvFilt->detectorParams=&detectorParamsAACSpeech;
  }
  else {
    INDIRECT(1); PTR_INIT(1);
    hInvFilt->detectorParams=&detectorParamsAAC;
  }

  INDIRECT(1); MOVE(1);
  hInvFilt->noDetectorBandsMax = numDetectorBands;


  PTR_INIT(4); /* hInvFilt->detectorValues[]
                  hInvFilt->prevInvfMode[]
                  hInvFilt->prevRegionOrig[]
                  hInvFilt->prevRegionSbr[]
               */
  LOOP(1);
  for(i=0;i<hInvFilt->noDetectorBandsMax;i++){
    FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(DETECTOR_VALUES));
    memset(&hInvFilt->detectorValues[i],0,sizeof(DETECTOR_VALUES));

    MOVE(3);
    hInvFilt->prevInvfMode[i]   = INVF_OFF;
    hInvFilt->prevRegionOrig[i] = 0;
    hInvFilt->prevRegionSbr[i]  = 0;
  }


  INDIRECT(1); FUNC(3);
  resetInvFiltDetector(hInvFilt,
                       freqBandTableDetector,
                       hInvFilt->noDetectorBandsMax);


  COUNT_sub_end();

  return (0);
}


/**************************************************************************/
/*!
  \brief     resets sbr inverse filtering structure.

  \return   errorCode

*/
/**************************************************************************/
int
resetInvFiltDetector(HANDLE_SBR_INV_FILT_EST hInvFilt,
                     int* freqBandTableDetector,
                     int numDetectorBands)
{

  COUNT_sub_start("resetInvFiltDetector");

  INDIRECT(1); MOVE(1);

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(numDetectorBands+1);
  memcpy(hInvFilt->freqBandTableInvFilt,freqBandTableDetector,(numDetectorBands+1)*sizeof(int));

  INDIRECT(1); MOVE(1);
  hInvFilt->noDetectorBands = numDetectorBands;

  COUNT_sub_end();

  return (0);
}


/**************************************************************************/
/*!
  \brief    deletes sbr inverse filtering structure.

  \return   none

*/
/**************************************************************************/
void
deleteInvFiltDetector (HANDLE_SBR_INV_FILT_EST hs)
{
  COUNT_sub_start("deleteInvFiltDetector");

  /*
    nothing to do
  */

  COUNT_sub_end();
}
