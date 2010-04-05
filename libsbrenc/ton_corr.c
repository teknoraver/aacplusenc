/*
  General tonality correction detector module.
*/
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "sbr.h"
#include "sbr_ram.h"
#include "ton_corr.h"
#include "invf_est.h"
#include "sbr_misc.h"
#include "sbr_def.h"

#include "counters.h" /* the 3GPP instrumenting tool */


/*!
  Auto correlation coefficients.
*/
typedef struct {
  float  r00r;
  float  r11r;
  float  r01r;
  float  r01i;
  float  r02r;
  float  r02i;
  float  r12r;
  float  r12i;
  float  r22r;
  float  det;
} ACORR_COEFS;



#define LPC_ORDER 2



/**************************************************************************/
/*!
  \brief     Calculates the second order auto correlation.

  Written by Per Ekstrand CTS AB.

  \return    none.

*/
/**************************************************************************/
static void
calcAutoCorrSecondOrder (ACORR_COEFS *ac,
                         float **realBuf,
                         float **imagBuf,
                         int bd
                               )
{
  int   j, jminus1, jminus2;
  float rel = 1.0f / (1.0f + RELAXATION);

  COUNT_sub_start("calcAutoCorrSecondOrder");

  DIV(1); /* counting previous operations */

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(ACORR_COEFS));
  memset(ac, 0, sizeof(ACORR_COEFS));

  PTR_INIT(2); /* pointer for realBuf[][],
                              imagBuf[][]
               */
  LOOP(1);
  for ( j = 0; j < 14 - 1; j++ ) {

    jminus1 = j - 1;
    jminus2 = jminus1 - 1;

    MAC(2);
    ac->r00r += realBuf[j][bd] * realBuf[j][bd] +
                imagBuf[j][bd] * imagBuf[j][bd];


    MAC(2);
    ac->r11r += realBuf[jminus1][bd] * realBuf[jminus1][bd] +
                imagBuf[jminus1][bd] * imagBuf[jminus1][bd];

    MAC(2);
    ac->r01r += realBuf[j][bd] * realBuf[jminus1][bd] +
                imagBuf[j][bd] * imagBuf[jminus1][bd];

    MULT(1); ADD(1); MAC(1);
    ac->r01i += imagBuf[j][bd] * realBuf[jminus1][bd] -
                realBuf[j][bd] * imagBuf[jminus1][bd];

    MAC(2);
    ac->r02r += realBuf[j][bd] * realBuf[jminus2][bd] +
                imagBuf[j][bd] * imagBuf[jminus2][bd];

    MULT(1); ADD(1); MAC(1);
    ac->r02i += imagBuf[j][bd] * realBuf[jminus2][bd] -
                realBuf[j][bd] * imagBuf[jminus2][bd];

  }

  MOVE(1); MAC(2);
  ac->r22r = ac->r11r + realBuf[-2][bd] * realBuf[-2][bd] +
                        imagBuf[-2][bd] * imagBuf[-2][bd];

  MOVE(1); MAC(2);
  ac->r12r = ac->r01r + realBuf[-1][bd] * realBuf[-2][bd] +
                        imagBuf[-1][bd] * imagBuf[-2][bd];

  MULT(1); ADD(1); MAC(1);
  ac->r12i = ac->r01i + imagBuf[-1][bd] * realBuf[-2][bd] -
                        realBuf[-1][bd] * imagBuf[-2][bd];

  jminus1 = j - 1;
  jminus2 = jminus1 - 1;

  MAC(2);
  ac->r00r += realBuf[j][bd] * realBuf[j][bd] +
              imagBuf[j][bd] * imagBuf[j][bd];

  MAC(2);
  ac->r11r += realBuf[jminus1][bd] * realBuf[jminus1][bd] +
              imagBuf[jminus1][bd] * imagBuf[jminus1][bd];

  MAC(2);
  ac->r01r += realBuf[j][bd] * realBuf[jminus1][bd] +
              imagBuf[j][bd] * imagBuf[jminus1][bd];

  MULT(1); ADD(1); MAC(1);
  ac->r01i += imagBuf[j][bd] * realBuf[jminus1][bd] -
              realBuf[j][bd] * imagBuf[jminus1][bd];

  MAC(2);
  ac->r02r += realBuf[j][bd] * realBuf[jminus2][bd] +
              imagBuf[j][bd] * imagBuf[jminus2][bd];

  MAC(2);
  ac->r02i += imagBuf[j][bd] * realBuf[jminus2][bd] -
              realBuf[j][bd] * imagBuf[jminus2][bd];


  MULT(3); MAC(1); ADD(1);
  ac->det = ac->r11r * ac->r22r - rel * (ac->r12r * ac->r12r + ac->r12i * ac->r12i);

  INDIRECT(10); MOVE(10); /* move all register variables to the structure ac->... */

  COUNT_sub_end();
}


/**************************************************************************/
/*!
  \brief Calculates the tonal to noise ration

  \return none.

*/
/**************************************************************************/
void
CalculateTonalityQuotas(HANDLE_SBR_TON_CORR_EST hTonCorr,
                        float **sourceBufferReal,
                        float **sourceBufferImag,
                        int usb
                        )
{
  int    i, k, r, timeIndex;
  float  alphar[2], alphai[2], r01r, r02r, r11r, r12r, r22r, r01i, r02i,
          r12i, det, r00r;

  ACORR_COEFS ac;


  int     startIndexMatrix  = hTonCorr->startIndexMatrix;
  int     totNoEst          = hTonCorr->numberOfEstimates;
  int     noEstPerFrame     = hTonCorr->numberOfEstimatesPerFrame;
  int     move              = hTonCorr->move;
  int     noQmfChannels     = hTonCorr->noQmfChannels;
  float*  nrgVector         = hTonCorr->nrgVector;
  float** quotaMatrix       = hTonCorr->quotaMatrix;

  COUNT_sub_start("CalculateTonalityQuotas");

  /*
   * Buffering of the quotaMatrix and the quotaMatrixTransp.
   *********************************************************/
  for(i =  0 ; i < move; i++){


    FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(noQmfChannels);
    memcpy(quotaMatrix[i],
           quotaMatrix[i + noEstPerFrame],
           noQmfChannels * sizeof(float));
  }


  FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(move);
  memmove(nrgVector,nrgVector+noEstPerFrame,move*sizeof(float));

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(totNoEst - startIndexMatrix);
  memset(nrgVector+startIndexMatrix,0,(totNoEst-startIndexMatrix)*sizeof(float));


  LOOP(1);
  for (r = 0; r < usb; r++) {

    INDIRECT(1); MOVE(2);
    k = 2;
    timeIndex = startIndexMatrix;

    PTR_INIT(2); /* pointer for quotaMatrix[][]
                                nrgVector[]
                 */
    ADD(1); LOOP(1);
    while(k <= 32 - 14) {

      MOVE(9);
      r01r = r02r = r11r = r12r = r22r = r00r = 0;
      r01i = r02i = r12i = 0;

      FUNC(5);
      calcAutoCorrSecondOrder (&ac, &sourceBufferReal[k], &sourceBufferImag[k], r);

      r00r = (float) ac.r00r;
      r11r = (float) ac.r11r;
      r12r = (float) ac.r12r;
      r12i = (float) ac.r12i;
      r22r = (float) ac.r22r;
      r01r = (float) ac.r01r;
      r01i = (float) ac.r01i;
      r02r = (float) ac.r02r;
      r02i = (float) ac.r02i;
      det  = (float) ac.det;



      BRANCH(1);
      if (ac.det == 0 ) {

        MOVE(2);
        alphar[1] = alphai[1] = 0;
      } else {

        MULT(3); ADD(2); DIV(1);
        alphar[1] = ( r01r * r12r - r01i * r12i - r02r * r11r ) / det;

        MULT(2); ADD(1); MAC(1); DIV(1);
        alphai[1] = ( r01i * r12r + r01r * r12i - r02i * r11r ) / det;
      }

      BRANCH(1);
      if ( ac.r11r == 0 ) {

        MOVE(2);
        alphar[0] =  alphai[0] = 0;
      } else {

        MULT(2); ADD(1); MAC(1); DIV(1);
        alphar[0] = - ( r01r + alphar[1] * r12r + alphai[1] * r12i ) / r11r;

        MULT(3); ADD(2); DIV(1);
        alphai[0] = - ( r01i + alphai[1] * r12r - alphar[1] * r12i ) / r11r;
      }


      BRANCH(1);
      if(r00r){
        float tmp;

        MULT(2); MAC(3); DIV(1);
        tmp =  - ( alphar[0]*r01r + alphai[0]*r01i + alphar[1]*r02r + alphai[1]*r02i) / (r00r);

        ADD(1); DIV(1);
        quotaMatrix[timeIndex][r] = tmp / ((float)1.0 - tmp +(float)RELAXATION);
      }
      else {

        MOVE(1);
        quotaMatrix[timeIndex][r] = 0;
      }

      ADD(1);
      nrgVector[timeIndex] += r00r;

      k += 16;
      timeIndex++;
    }
  }

  COUNT_sub_end();
}



/**************************************************************************/
/*!
  \brief Extracts the parameters required in the decoder.

  \return none.

*/
/**************************************************************************/
void
TonCorrParamExtr(HANDLE_SBR_TON_CORR_EST hTonCorr,
                 INVF_MODE* infVec,
                 float* noiseLevels,
                 int* missingHarmonicFlag,
                 unsigned char * missingHarmonicsIndex,
                 char * envelopeCompensation,
                 const SBR_FRAME_INFO *frameInfo,
                 int* transientInfo,
                 unsigned char* freqBandTable,
                 int nSfb,
                 XPOS_MODE xposType
                 )
{
  int band;
  int transientFlag = transientInfo[1] ;
  int transientPos  = transientInfo[0];
  int transientFrame, transientFrameInvfEst;
  INVF_MODE* infVecPtr;

  COUNT_sub_start("TonCorrParamExtr");

  MOVE(2); /* counting previous operations */

  MOVE(1);
  transientFrame = 0;

  INDIRECT(1); BRANCH(1);
  if(hTonCorr->transientNextFrame){

    INDIRECT(1); MOVE(2);
    transientFrame = 1;
    hTonCorr->transientNextFrame = 0;

    BRANCH(1);
    if(transientFlag){

      INDIRECT(2); ADD(2); BRANCH(1);
      if(transientPos + hTonCorr->transientPosOffset >= frameInfo->borders[frameInfo->nEnvelopes]){

        INDIRECT(1); MOVE(1);
        hTonCorr->transientNextFrame = 1;
      }
    }
  }
  else{

    BRANCH(1);
    if(transientFlag){

      INDIRECT(2); ADD(2); BRANCH(1);
      if(transientPos + hTonCorr->transientPosOffset < frameInfo->borders[frameInfo->nEnvelopes]){

        INDIRECT(1); MOVE(2);
        transientFrame = 1;
        hTonCorr->transientNextFrame = 0;
      }
      else{

        INDIRECT(1); MOVE(1);
        hTonCorr->transientNextFrame = 1;
      }
    }
  }


  INDIRECT(1); MOVE(1);
  transientFrameInvfEst = hTonCorr->transientNextFrame;


  INDIRECT(1); BRANCH(1);
  if (hTonCorr->switchInverseFilt)
  {
    INDIRECT(7); PTR_INIT(1); ADD(1); FUNC(8);
    qmfInverseFilteringDetector (&hTonCorr->sbrInvFilt,
                                 hTonCorr->quotaMatrix,
                                 hTonCorr->nrgVector,
                                 hTonCorr->indexVector,
                                 hTonCorr->frameStartIndexInvfEst,
                                 hTonCorr->numberOfEstimatesPerFrame + hTonCorr->frameStartIndexInvfEst,
                                 transientFrameInvfEst,
                                 infVec);
  }


  ADD(1); BRANCH(1);
  if (xposType == XPOS_LC ){

    INDIRECT(3); PTR_INIT(1); FUNC(10);
    SbrMissingHarmonicsDetectorQmf(&hTonCorr->sbrMissingHarmonicsDetector,
                                   hTonCorr->quotaMatrix,
                                   hTonCorr->indexVector,
                                   frameInfo,
                                   transientInfo,
                                   missingHarmonicFlag,
                                   missingHarmonicsIndex,
                                   freqBandTable,
                                   nSfb,
                                   envelopeCompensation);
  }
  else{

    MOVE(1);
    *missingHarmonicFlag = 0;

    FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(nSfb);
    memset(missingHarmonicsIndex,0,nSfb*sizeof(int));
  }


  INDIRECT(1); MOVE(1);
  infVecPtr = hTonCorr->sbrInvFilt.prevInvfMode;

  INDIRECT(6); PTR_INIT(1); FUNC(11);
  sbrNoiseFloorEstimateQmf (&hTonCorr->sbrNoiseFloorEstimate,
                            frameInfo,
                            noiseLevels,
                            hTonCorr->quotaMatrix,
                            hTonCorr->indexVector,
                            *missingHarmonicFlag,
                            hTonCorr->frameStartIndex,
                            hTonCorr->numberOfEstimatesPerFrame,
                            hTonCorr->numberOfEstimates,
                            transientFrame,
                            infVecPtr);


  PTR_INIT(2); /* hTonCorr->sbrInvFilt.prevInvfMode[]
                  infVec[]
               */
  INDIRECT(1); LOOP(1);
  for(band = 0 ; band < hTonCorr->sbrInvFilt.noDetectorBands; band++){

    MOVE(1);
    hTonCorr->sbrInvFilt.prevInvfMode[band] = infVec[band];
  }

  COUNT_sub_end();
}


/**************************************************************************/
/*!
  \brief     Searches for the closest match

  \return   closest entry.

*/
/**************************************************************************/
static int
findClosestEntry(int goalSb,
                 unsigned char *v_k_master,
                 int numMaster,
                 int direction)
{
  int index;

  COUNT_sub_start("findClosestEntry");

  ADD(1); BRANCH(1);
  if( goalSb <= v_k_master[0] )
  {
    COUNT_sub_end();
    return v_k_master[0];
  }

  INDIRECT(1); ADD(1); BRANCH(1);
  if( goalSb >= v_k_master[numMaster] )
  {
    COUNT_sub_end();
    return v_k_master[numMaster];
  }

  BRANCH(1);
  if(direction) {

    MOVE(1);
    index = 0;

    PTR_INIT(1); /* v_k_master[index] */
    LOOP(1);
    while( v_k_master[index] < goalSb ) {

      ADD(1);
      index++;
    }
  } else {

    MOVE(1);
    index = numMaster;

    PTR_INIT(1); /* v_k_master[index] */
    LOOP(1);
    while( v_k_master[index] > goalSb ) {

      ADD(1);
      index--;
    }
  }

  COUNT_sub_end();

  return v_k_master[index];
}


/**************************************************************************/
/*!
  \brief     resets the patch

  \return   errorCode
*/
/**************************************************************************/
static int
resetPatch(HANDLE_SBR_TON_CORR_EST hTonCorr,
           int xposctrl,
           int highBandStartSb,
           int channelOffset,
           unsigned char *v_k_master,
           int numMaster,
           int fs,
           int noChannels)
{
  int patch,k,i;
  int targetStopBand;

  PATCH_PARAM  *patchParam = hTonCorr->patchParam;

  int sbGuard = hTonCorr->guard;
  int sourceStartBand;
  int patchDistance;
  int numBandsInPatch;

  int lsb = v_k_master[0];
  int usb = v_k_master[numMaster];
  int xoverOffset = highBandStartSb - v_k_master[0];

  int goalSb;

  COUNT_sub_start("resetPatch");

  INDIRECT(3); PTR_INIT(1); MOVE(3); ADD(1); /* counting previous operations */


  ADD(1); BRANCH(1);
  if (xposctrl == 1) {

    ADD(1);
    lsb += xoverOffset;

    MOVE(1);
    xoverOffset = 0;
  }

  MULT(2); DIV(1); ADD(1);
  goalSb = (int)( 2 * noChannels * 16000.0f / fs  + 0.5f );

  FUNC(4);
  goalSb = findClosestEntry(goalSb, v_k_master, numMaster, 1);

  INDIRECT(1); ADD(1);
  sourceStartBand = hTonCorr->shiftStartSb + xoverOffset;

  ADD(1);
  targetStopBand = lsb + xoverOffset;

  MOVE(1);
  patch = 0;

  PTR_INIT(1); /* patchParam[patch] */
  LOOP(1);
  while(targetStopBand < usb) {

    ADD(1); BRANCH(1);
    if (patch >= MAX_NUM_PATCHES)
    {
      COUNT_sub_end();
      return(1);
    }

    MOVE(1);
    patchParam[patch].guardStartBand = targetStopBand;

    ADD(1);
    targetStopBand += sbGuard;

    MOVE(1);
    patchParam[patch].targetStartBand = targetStopBand;

    ADD(1);
    numBandsInPatch = goalSb - targetStopBand;

    ADD(2); BRANCH(1);
    if ( numBandsInPatch >= lsb - sourceStartBand ) {
      ADD(1);
      patchDistance   = targetStopBand - sourceStartBand;

      LOGIC(1);
      patchDistance   = patchDistance & ~1;

      ADD(2);
      numBandsInPatch = lsb - (targetStopBand - patchDistance);

      ADD(2); FUNC(4);
      numBandsInPatch = findClosestEntry(targetStopBand + numBandsInPatch, v_k_master, numMaster, 0) -
                        targetStopBand;
    }


    ADD(2);
    patchDistance   = numBandsInPatch + targetStopBand - lsb;

    ADD(1); LOGIC(1);
    patchDistance   = (patchDistance + 1) & ~1;

    BRANCH(1);
    if (numBandsInPatch <= 0) {

      ADD(1);
      patch--;
    } else {

      ADD(1); STORE(1);
      patchParam[patch].sourceStartBand = targetStopBand - patchDistance;

      MOVE(2);
      patchParam[patch].targetBandOffs  = patchDistance;
      patchParam[patch].numBandsInPatch = numBandsInPatch;

      ADD(1); STORE(1);
      patchParam[patch].sourceStopBand  = patchParam[patch].sourceStartBand + numBandsInPatch;

      ADD(1);
      targetStopBand += patchParam[patch].numBandsInPatch;
    }

    INDIRECT(1); MOVE(1);
    sourceStartBand = hTonCorr->shiftStartSb;

    ADD(2); MISC(1); BRANCH(1);
    if( abs(targetStopBand - goalSb) < 3) {
      MOVE(1);
      goalSb = usb;
    }

    ADD(1);
    patch++;

  }

  ADD(1);
  patch--;


  ADD(1); LOGIC(1); BRANCH(1);
  if ( patchParam[patch].numBandsInPatch < 3 && patch > 0 ) {
    ADD(1);
    patch--;

    ADD(1);
    targetStopBand = patchParam[patch].targetStartBand + patchParam[patch].numBandsInPatch;
  }

  INDIRECT(1); ADD(1); STORE(1);
  hTonCorr->noOfPatches = patch + 1;

  PTR_INIT(1); /* hTonCorr->indexVector[] */
  INDIRECT(1); LOOP(1);
  for(k = 0; k < hTonCorr->patchParam[0].guardStartBand; k++)
  {
    MOVE(1);
    hTonCorr->indexVector[k] = k;
  }

  PTR_INIT(1); /* hTonCorr->patchParam[] */
  INDIRECT(1); LOOP(1);
  for(i = 0; i < hTonCorr->noOfPatches; i++)
  {
    int sourceStart    = hTonCorr->patchParam[i].sourceStartBand;
    int targetStart    = hTonCorr->patchParam[i].targetStartBand;
    int numberOfBands  = hTonCorr->patchParam[i].numBandsInPatch;
    int startGuardBand = hTonCorr->patchParam[i].guardStartBand;

    MOVE(4); /* counting previous operations */

    PTR_INIT(1); /* hTonCorr->indexVector[] */
    ADD(1); LOOP(1);
    for(k = 0; k < (targetStart- startGuardBand); k++)
    {
      MULT(1); STORE(1);
      hTonCorr->indexVector[startGuardBand+k] = -1;
    }

    PTR_INIT(1); /* hTonCorr->indexVector[] */
    LOOP(1);
    for(k = 0; k < numberOfBands; k++)
    {
      ADD(1); STORE(1);
      hTonCorr->indexVector[targetStart+k] = sourceStart+k;
    }
  }

  COUNT_sub_end();

  return (0);
}



/**************************************************************************/
/*!
  \brief  Creates an instance of the tonality correction parameter module.

  \return   errorCode
*/
/**************************************************************************/
int
CreateTonCorrParamExtr (int chan,
                        HANDLE_SBR_TON_CORR_EST hTonCorr,
                        int fs,
                        int usb,
                        int noQmfChannels,
                        int xposCtrl,
                        int highBandStartSb,
                        int channelOffset,
                        unsigned char *v_k_master,
                        int numMaster,
                        int ana_max_level,
                        unsigned char *freqBandTable[2],
                        int* nSfb,
                        int noiseBands,
                        int noiseFloorOffset,
                        unsigned int useSpeechConfig
                        )
{
  int i;

  COUNT_sub_start("CreateTonCorrParamExtr");

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(SBR_TON_CORR_EST));
  memset(hTonCorr,0,sizeof(SBR_TON_CORR_EST));

  INDIRECT(6); MOVE(6);
  hTonCorr->numberOfEstimates         = NO_OF_ESTIMATES;
  hTonCorr->numberOfEstimatesPerFrame = 2;
  hTonCorr->frameStartIndexInvfEst    = 0;
  hTonCorr->transientPosOffset        = 4;
  hTonCorr->move                      = 2;
  hTonCorr->startIndexMatrix          = 2;


  INDIRECT(3); MOVE(3);
  hTonCorr->frameStartIndex    = 0;
  hTonCorr->prevTransientFlag  = 0;
  hTonCorr->transientNextFrame = 0;

  INDIRECT(1); MOVE(1);
  hTonCorr->noQmfChannels = noQmfChannels;


  MULT(2); ADD(1); INDIRECT(1);
  PTR_INIT(2); /* hTonCorr->quotaMatrix[]
                  sbr_quotaMatrix[]
               */
  INDIRECT(1); LOOP(1);
  for(i=0;i<hTonCorr->numberOfEstimates;i++) {
    PTR_INIT(1);
    hTonCorr->quotaMatrix[i] = &(sbr_quotaMatrix[chan* NO_OF_ESTIMATES*QMF_CHANNELS + i*noQmfChannels]);

    FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(QMF_CHANNELS);
    memset(hTonCorr->quotaMatrix[i] ,0, sizeof(float)*QMF_CHANNELS);
  }

   /* Reset the patch.*/
  INDIRECT(2); MOVE(2);
  hTonCorr->guard = 0;
  hTonCorr->shiftStartSb = 1;

  FUNC(8); BRANCH(1);
  if(resetPatch(hTonCorr,
                xposCtrl,
                highBandStartSb,
                channelOffset,
                v_k_master,
                numMaster,
                fs,
                noQmfChannels))
  {
    COUNT_sub_end();
    return(1);
  }

  INDIRECT(1); PTR_INIT(1); FUNC(8);
  if(CreateSbrNoiseFloorEstimate (&hTonCorr->sbrNoiseFloorEstimate,
                                   ana_max_level,
                                   freqBandTable[LO],
                                   nSfb[LO],
                                   noiseBands,
                                   noiseFloorOffset,
                                   //timeSlots,
                                   useSpeechConfig))
  {
    COUNT_sub_end();
    return(1);
  }


  INDIRECT(4); PTR_INIT(1); FUNC(5);
  if(createInvFiltDetector(&hTonCorr->sbrInvFilt,
                            hTonCorr->sbrNoiseFloorEstimate.freqBandTableQmf,
                            hTonCorr->sbrNoiseFloorEstimate.noNoiseBands,
                            hTonCorr->numberOfEstimatesPerFrame,
                            useSpeechConfig))
  {
    COUNT_sub_end();
    return(1);
  }


  INDIRECT(4); PTR_INIT(1); FUNC(10);
  if(CreateSbrMissingHarmonicsDetector (chan,
                                        &hTonCorr->sbrMissingHarmonicsDetector,
                                        fs,
                                        freqBandTable[HI],
                                        nSfb[HI],
                                        noQmfChannels,
                                        hTonCorr->numberOfEstimates,
                                        hTonCorr->move,
                                        hTonCorr->numberOfEstimatesPerFrame))
  {
    COUNT_sub_end();
    return(1);
  }



  COUNT_sub_end();

  return (0);
}



/**************************************************************************/
/*!
  \brief  Deletes the tonality correction parameter module.

  \return   none
*/
/**************************************************************************/
void
DeleteTonCorrParamExtr (HANDLE_SBR_TON_CORR_EST hTonCorr)
{

  COUNT_sub_start("DeleteTonCorrParamExtr");

  BRANCH(1);
  if (hTonCorr) {

   INDIRECT(1); PTR_INIT(1); FUNC(1);
   deleteInvFiltDetector (&hTonCorr->sbrInvFilt);

  }

  COUNT_sub_end();
}
