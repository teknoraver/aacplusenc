/*
  Missing harmonics detection
*/


#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "FloatFR.h"
#include "sbr.h"
#include "sbr_def.h"
#include "mh_det.h"
#include "sbr_ram.h"

#include "counters.h" /* the 3GPP instrumenting tool */

#define DELTA_TIME          9
#define MAX_COMP            2
#define TONALITY_QUOTA      0.1f
#define DIFF_QUOTA          0.75f
#define THR_DIFF           25.0f
#define THR_DIFF_GUIDE      1.26f
#define THR_TONE           15.0f
#define I_THR_TONE         (1.0f/15.0f)
#define THR_TONE_GUIDE      1.26f
#define THR_SFM_SBR         0.3f
#define THR_SFM_ORIG        0.1f
#define DECAY_GUIDE_ORIG    0.3f
#define DECAY_GUIDE_DIFF    0.5f



/**************************************************************************/
/*!
  \brief     Calculates the difference.

  \return    none.

*/
/**************************************************************************/
static void diff(float* pTonalityOrig,          /*!< */
                 float* pDiffMapped2Scfb,
                 const unsigned char* pFreqBandTable,
                 int nScfb,
                 char * indexVector)
{

  int i, ll, lu, k;
  float maxValOrig, maxValSbr;

  COUNT_sub_start("diff");

  PTR_INIT(2); /* pointers for pFreqBandTable[],
                               pDiffMapped2Scfb[]
               */
  LOOP(1);
  for(i=0; i < nScfb; i++){

    MOVE(2);
    ll = pFreqBandTable[i];
    lu = pFreqBandTable[i+1];

    MOVE(2);
    maxValOrig = 0;
    maxValSbr = 0;

    PTR_INIT(2); /* pointers for pTonalityOrig[],
                                 indexVector[]
                 */
    LOOP(1);
    for(k=ll;k<lu;k++){

      ADD(1); BRANCH(1);
      if(pTonalityOrig[k] > maxValOrig)
      {
        MOVE(1);
        maxValOrig = pTonalityOrig[k];
      }

      INDIRECT(1); ADD(1); BRANCH(1);
      if(pTonalityOrig[indexVector[k]] > maxValSbr)
      {
        MOVE(1);
        maxValSbr = pTonalityOrig[indexVector[k]];
      }
    }

    ADD(1); BRANCH(1);
    if(maxValSbr >= 1)
    {
      DIV(1); STORE(1);
      pDiffMapped2Scfb[i] = maxValOrig/maxValSbr;
    }
    else
    {
      MOVE(1);
      pDiffMapped2Scfb[i] = maxValOrig;
    }
  }

  COUNT_sub_end();
}


/**************************************************************************/
/*!
  \brief     Calculates a flatness measure.
  \return    none.

*/
/**************************************************************************/
static void calculateFlatnessMeasure(float* pQuotaBuffer,
                                     char* indexVector,
                                     float* pSfmOrigVec,
                                     float* pSfmSbrVec,
                                     const unsigned char* pFreqBandTable,
                                     int nSfb)
{
  int i,j;

  COUNT_sub_start("calculateFlatnessMeasure");

  PTR_INIT(4); /* pointers for pFreqBandTable[],
                               pSfmOrigVec[],
                               pSfmSbrVec[],
                               pSfmOrigVec[]
               */
  LOOP(1);
  for(i=0;i<nSfb;i++)
  {
    int ll = pFreqBandTable[i];
    int lu = pFreqBandTable[i+1];

    MOVE(2); /* counting previous operations */

    MOVE(2);
    pSfmOrigVec[i] = 1;
    pSfmSbrVec[i]  = 1;

    ADD(2); BRANCH(1);
    if(lu -ll > 1){
      float amOrig,amTransp,gmOrig,gmTransp,sfmOrig,sfmTransp;

      MOVE(2);
      amOrig = amTransp = 0;
      gmOrig = gmTransp = 1;

      PTR_INIT(2); /* pointers for pQuotaBuffer[],
                                   indexVector[]
                    */
      LOOP(1);
      for(j= ll;j<lu;j++){

        MOVE(1);
        sfmOrig   = pQuotaBuffer[j];

        INDIRECT(1); MOVE(1);
        sfmTransp = pQuotaBuffer[indexVector[j]];

        ADD(2); MULT(2);
        amOrig   += sfmOrig;
        gmOrig   *= sfmOrig;
        amTransp += sfmTransp;
        gmTransp *= sfmTransp;
      }

      ADD(1); /* lu-ll */ DIV(2);
      amOrig   /= lu-ll;
      amTransp /= lu-ll;

      DIV(1); /* 1.0f/(lu-ll) */ TRANS(2);
      gmOrig   = (float) pow(gmOrig, 1.0f/(lu-ll));
      gmTransp = (float) pow(gmTransp,1.0f/(lu-ll));

      BRANCH(1);
      if(amOrig)
      {
        DIV(1); STORE(1);
        pSfmOrigVec[i] = gmOrig/amOrig;
      }

      BRANCH(1);
      if(amTransp)
      {
        DIV(1); STORE(1);
        pSfmSbrVec[i] = gmTransp/amTransp;
      }
    }
  }

  COUNT_sub_end();
}






/**************************************************************************/
/*!
  \brief     Calculates the input to the detection.
  \return    none.

*/
/**************************************************************************/
static void calculateDetectorInput(float** pQuotaBuffer,
                                   char* indexVector,
                                   float** tonalityDiff,
                                   float** pSfmOrig,
                                   float** pSfmSbr,
                                   const unsigned char* freqBandTable,
                                   int nSfb,
                                   int noEstPerFrame,
                                   int move,
                                   int noQmfBands)
{
  int est;

  COUNT_sub_start("calculateDetectorInput");


   LOOP(1);
   for(est =  0 ; est < move; est++){

    FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(noQmfBands);
    memcpy(tonalityDiff[est],
           tonalityDiff[est + noEstPerFrame],
           noQmfBands * sizeof(float));

    FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(noQmfBands);
    memcpy(pSfmOrig[est],
           pSfmOrig[est + noEstPerFrame],
           noQmfBands * sizeof(float));

    FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(noQmfBands);
    memcpy(pSfmSbr[est],
           pSfmSbr[est + noEstPerFrame],
           noQmfBands * sizeof(float));

  }


  PTR_INIT(4); /* pointers for pQuotaBuffer[],
                               tonalityDiff[],
                               pSfmOrig[],
                               pSfmSbr[]
               */
  LOOP(1);
  for(est = 0; est < noEstPerFrame; est++){

    FUNC(5);
    diff(pQuotaBuffer[est+move],
         tonalityDiff[est+move],
         freqBandTable,
         nSfb,
         indexVector);

    FUNC(6);
    calculateFlatnessMeasure(pQuotaBuffer[est+ move],
                             indexVector,
                             pSfmOrig[est + move],
                             pSfmSbr[est + move],
                             freqBandTable,
                             nSfb);
  }

  COUNT_sub_end();
}


/**************************************************************************/
/*!
  \brief     Checks if it is allowed to detect a tone.

  \return    newDetectionAllowed flag.

*/
/**************************************************************************/
static int isDetectionOfNewToneAllowed(const SBR_FRAME_INFO *pFrameInfo,
                                       int prevTransientFrame,
                                       int prevTransientPos,
                                       int prevTransientFlag,
                                       int transientPosOffset,
                                       int transientFlag,
                                       int transientPos,
                                       HANDLE_SBR_MISSING_HARMONICS_DETECTOR h_sbrMissingHarmonicsDetector)
{
  int transientFrame, newDetectionAllowed;

  COUNT_sub_start("isDetectionOfNewToneAllowed");

  MOVE(1);
  transientFrame = 0;

  BRANCH(1);
  if(transientFlag){

    INDIRECT(1); ADD(2); BRANCH(1);
    if(transientPos + transientPosOffset < pFrameInfo->borders[pFrameInfo->nEnvelopes])
    {
      MOVE(1);
      transientFrame = 1;
    }
  }
  else{
    LOGIC(1); BRANCH(1);
    if(prevTransientFlag && !prevTransientFrame){
      MOVE(1);
      transientFrame = 1;
    }
  }


  MOVE(1);
  newDetectionAllowed = 0;

  BRANCH(1);
  if(transientFrame){
    MOVE(1);
    newDetectionAllowed = 1;
  }
  else {
    INDIRECT(2); ADD(4); MISC(1); LOGIC(1); BRANCH(1);
    if(prevTransientFrame &&
       abs(pFrameInfo->borders[0] - (prevTransientPos + transientPosOffset -
                                     h_sbrMissingHarmonicsDetector->timeSlots)) < DELTA_TIME)
    {
      MOVE(1);
      newDetectionAllowed = 1;
    }
  }

  INDIRECT(3); MOVE(3);
  h_sbrMissingHarmonicsDetector->previousTransientFlag  = transientFlag;
  h_sbrMissingHarmonicsDetector->previousTransientFrame = transientFrame;
  h_sbrMissingHarmonicsDetector->previousTransientPos   = transientPos;

  COUNT_sub_end();

  return (newDetectionAllowed);
}




/**************************************************************************/
/*!
  \brief     Cleans up after a transient.
  \return    none.

*/
/**************************************************************************/
static void transientCleanUp(float** quotaBuffer,
                             float** pDiffVecScfb,
                             int nSfb,
                             unsigned char ** detectionVectors,
                             const unsigned char* pFreqBandTable,
                             GUIDE_VECTORS guideVectors,
                             int newDetectionAllowed,
                             int start,
                             int stop)
{
  int i,j,li, ui,est;

  unsigned char pHarmVec[MAX_FREQ_COEFFS];

  COUNT_sub_start("transientCleanUp");

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(MAX_FREQ_COEFFS);
  memset(pHarmVec,0,MAX_FREQ_COEFFS*sizeof(unsigned char));

  PTR_INIT(1); /* pHarmVec[] */
  LOOP(1);
  for(est = start; est < stop; est++){

    PTR_INIT(1); /* detectionVectors[][] */
    LOOP(1);
    for(i=0;i<nSfb-1;i++){

      LOGIC(1); STORE(1);
      pHarmVec[i] = pHarmVec[i] || detectionVectors[est][i];
    }
  }


  PTR_INIT(5); /* pFreqBandTable[]
                  pHarmVec[]
                  guideVectors.guideVectorDetected[]
                  guideVectors.guideVectorOrig[]
                  guideVectors.guideVectorDiff[]
                */
  LOOP(1);
  for(i=0;i<nSfb-1;i++)
  {
    MOVE(2);
    li = pFreqBandTable[i];
    ui = pFreqBandTable[i+1];


    LOGIC(1); BRANCH(1);
    if(pHarmVec[i] && pHarmVec[i+1]){
      float maxVal1, maxVal2;
      int maxPos1, maxPos2;

      MOVE(2);
      li = pFreqBandTable[i];
      ui = pFreqBandTable[i+1];


      MOVE(1);
      maxPos1 = li;

      INDIRECT(1); MOVE(1);
      maxVal1 = quotaBuffer[start][li];

      PTR_INIT(1); /* quotaBuffer[][] */
      LOOP(1);
      for(j = li; j<ui; j++){

        ADD(1); BRANCH(1);
        if(quotaBuffer[start][j] > maxVal1){

          MOVE(2);
          maxVal1 = quotaBuffer[start][j];
          maxPos1 = j;
        }
      }

      LOOP(1);
      for(est = start + 1; est < stop; est++){

        PTR_INIT(1); /* quotaBuffer[][] */
        LOOP(1);
        for(j = li; j<ui; j++){

          ADD(1); BRANCH(1);
          if(quotaBuffer[est][j] > maxVal1){

            MOVE(2);
            maxVal1 = quotaBuffer[est][j];
            maxPos1 = j;
          }
        }
      }


      MOVE(2);
      li = pFreqBandTable[i+1];
      ui = pFreqBandTable[i+2];


      MOVE(1);
      maxPos2 = li;

      INDIRECT(1); MOVE(1);
      maxVal2 = quotaBuffer[start][li];

      PTR_INIT(1); /* quotaBuffer[][] */
      LOOP(1);
      for(j = li; j<ui; j++){

        ADD(1); BRANCH(1);
        if((float)quotaBuffer[start][j] > maxVal2){

          MOVE(2);
          maxVal2 = quotaBuffer[start][j];
          maxPos2 = j;
        }
      }

      LOOP(1);
      for(est = start + 1; est < stop; est++){

        PTR_INIT(1); /* quotaBuffer[][] */
        LOOP(1);
        for(j = li; j<ui; j++){

          ADD(1); BRANCH(1);
          if((float)quotaBuffer[est][j] > maxVal2){

            MOVE(2);
            maxVal2 = quotaBuffer[est][j];
            maxPos2 = j;
          }
        }
      }


      ADD(2); BRANCH(1);
      if(maxPos2-maxPos1 < 2){

        ADD(1); BRANCH(1);
        if(maxVal1 > maxVal2){

          MOVE(3);
          guideVectors.guideVectorDetected[i+1] = 0;
          guideVectors.guideVectorOrig[i+1] = 0;
          guideVectors.guideVectorDiff[i+1] = 0;

          PTR_INIT(1); /* detectionVectors[][] */
          LOOP(1);
          for(est = start; est<stop; est++){
            MOVE(1);
            detectionVectors[est][i+1] = 0;
          }
        }
        else{

          MOVE(3);
          guideVectors.guideVectorDetected[i] = 0;
          guideVectors.guideVectorOrig[i] = 0;
          guideVectors.guideVectorDiff[i] = 0;

          PTR_INIT(1); /* detectionVectors[][] */
          LOOP(1);
          for(est = start; est<stop; est++){
            MOVE(1);
            detectionVectors[est][i] = 0;
          }
        }
      }
    }
  }
  COUNT_sub_end();
}

/*************************************************************************/
/*!
  \brief     Do detection.
  \return    none.

*/
/**************************************************************************/
static void detection(float* quotaBuffer,
                      char* indexVector,
                      float* pDiffVecScfb,
                      int nSfb,
                      unsigned char* pHarmVec,
                      const unsigned char* pFreqBandTable,
                      float* sfmOrig,
                      float* sfmSbr,
                      GUIDE_VECTORS guideVectors,
                      GUIDE_VECTORS newGuideVectors,
                      int newDetectionAllowed)
{

  int i,j,ll, lu;
  float thresTemp,thresOrig;

  COUNT_sub_start("detection");

  PTR_INIT(5); /* pointers for guideVectors.guideVectorDiff[],
                               guideVectors.guideVectorOrig[],
                               pDiffVecScfb[],
                               pHarmVec[],
                               newGuideVectors.guideVectorDiff[]
               */
  LOOP(1);
  for(i=0;i<nSfb;i++)
  {
    BRANCH(1);
    if (guideVectors.guideVectorDiff[i]) {

      MULT(1); ADD(1); BRANCH(1); MOVE(1);
      thresTemp = max(DECAY_GUIDE_DIFF*guideVectors.guideVectorDiff[i],THR_DIFF_GUIDE);
    } else {

      MOVE(1);
      thresTemp = THR_DIFF;
    }

    ADD(1); BRANCH(1); MOVE(1);
    thresTemp = min(thresTemp, THR_DIFF);

    ADD(1); BRANCH(1);
    if(pDiffVecScfb[i] > thresTemp){

      MOVE(1);
      pHarmVec[i] = 1;

      MOVE(1);
      newGuideVectors.guideVectorDiff[i] = pDiffVecScfb[i];
    }
    else{

      BRANCH(1);
      if(guideVectors.guideVectorDiff[i]){

        MOVE(1);
        guideVectors.guideVectorOrig[i] = THR_TONE_GUIDE ;
      }
    }
  }


  PTR_INIT(4); /* pointers for guideVectors.guideVectorOrig[],
                               pHarmVec[],
                               newGuideVectors.guideVectorDiff[],
                               pFreqBandTable[]
               */
  LOOP(1);
  for(i=0;i<nSfb;i++){

    MOVE(2);
    ll = pFreqBandTable[i];
    lu = pFreqBandTable[i+1];

    MULT(1); ADD(1); BRANCH(1); MOVE(1);
    thresOrig   = max(guideVectors.guideVectorOrig[i]*DECAY_GUIDE_ORIG,THR_TONE_GUIDE );

    ADD(1); BRANCH(1); MOVE(1);
    thresOrig   = min(thresOrig,THR_TONE);

    BRANCH(1);
    if(guideVectors.guideVectorOrig[i]){

      PTR_INIT(1); /* pointers for quotaBuffer[] */
      LOOP(1);
      for(j= ll;j<lu;j++){

        ADD(1); BRANCH(1);
        if(quotaBuffer[j] > thresOrig){

          MOVE(2);
          pHarmVec[i] = 1;
          newGuideVectors.guideVectorOrig[i] = quotaBuffer[j];
        }
      }
    }
  }


  MOVE(1);
  thresOrig   = THR_TONE;

  PTR_INIT(6); /* pointers for newGuideVectors.guideVectorDiff[],
                               pHarmVec[],
                               pFreqBandTable[],
                               pDiffVecScfb[],
                               sfmSbr[],
                               sfmOrig[]
               */
  LOOP(1);
  for(i=0;i<nSfb;i++){

    MOVE(2);
    ll = pFreqBandTable[i];
    lu = pFreqBandTable[i+1];

    ADD(2); BRANCH(1);
    if(lu -ll > 1){

      PTR_INIT(1); /* pointers for quotaBuffer[] */
      LOOP(1);
      for(j= ll;j<lu;j++){

        ADD(3); LOGIC(2); BRANCH(1);
        if(quotaBuffer[j] > thresOrig && (sfmSbr[i] > THR_SFM_SBR && sfmOrig[i] < THR_SFM_ORIG)){

          MOVE(2);
          pHarmVec[i] = 1;
          newGuideVectors.guideVectorOrig[i] = quotaBuffer[j];
        }
      }
    }
    else{
      ADD(2); BRANCH(1);
      if(i < nSfb -1){

        MOVE(1);
        ll = pFreqBandTable[i];

        PTR_INIT(1); /* pointers for quotaBuffer[] */

        BRANCH(1);
        if(i>0){

          ADD(3); LOGIC(2); BRANCH(1);
          if(quotaBuffer[ll] > THR_TONE &&
             (pDiffVecScfb[+1] < I_THR_TONE ||
              pDiffVecScfb[i-1] < I_THR_TONE)
             ){

              MOVE(2);
              pHarmVec[i] = 1;
              newGuideVectors.guideVectorOrig[i] = quotaBuffer[ll];
          }
        }
        else{
          ADD(2); LOGIC(1); BRANCH(1);
          if(quotaBuffer[ll] > THR_TONE &&
             pDiffVecScfb[i+1] < I_THR_TONE){

              MOVE(2);
              pHarmVec[i] = 1;
              newGuideVectors.guideVectorOrig[i] = quotaBuffer[ll];
          }
        }
      }
    }
  }

  COUNT_sub_end();
}








/**************************************************************************/
/*!
  \brief     Do detection for every estimate
  \return    none.

*/
/**************************************************************************/
static void detectionWithPrediction(float** quotaBuffer,
                                    char* indexVector,
                                    float** pDiffVecScfb,
                                    int nSfb,
                                    const unsigned char* pFreqBandTable,
                                    float** sfmOrig,
                                    float** sfmSbr,
                                    unsigned char ** detectionVectors,
                                    unsigned char* prevFrameSfbHarm,
                                    GUIDE_VECTORS* guideVectors,
                                    int noEstPerFrame,
                                    int totNoEst,
                                    int newDetectionAllowed,
                                    unsigned char* pAddHarmonicsScaleFactorBands)
{
  int est = 0,i;
  int start;

  COUNT_sub_start("detectionWithPrediction");

  MOVE(1); /* counting previous operation */

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(nSfb);
  memset(pAddHarmonicsScaleFactorBands,0,nSfb*sizeof(unsigned char));

  BRANCH(1);
  if(newDetectionAllowed){

    ADD(1); BRANCH(1);
    if(totNoEst > 1){

      MOVE(1);
      start = noEstPerFrame;

      FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(nSfb);
      memcpy(guideVectors[noEstPerFrame].guideVectorDiff,guideVectors[0].guideVectorDiff,nSfb*sizeof(float));

      FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(nSfb);
      memcpy(guideVectors[noEstPerFrame].guideVectorOrig,guideVectors[0].guideVectorOrig,nSfb*sizeof(float));

      FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(nSfb);
      memset(guideVectors[noEstPerFrame-1].guideVectorDetected,0,nSfb*sizeof(unsigned char));
    }
    else{

      MOVE(1);
      start = 0;
    }
  }
  else{

    MOVE(1);
    start = 0;
  }


  PTR_INIT(6); /* guideVectors[],
                  detectionVectors[]
                  quotaBuffer[]
                  pDiffVecScfb[]
                  sfmOrig[]
                  sfmSbr[]
               */
  LOOP(1);
  for(est = start; est < totNoEst; est++){

    BRANCH(1);
    if(est > 0){
      FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(nSfb);
      memcpy(guideVectors[est].guideVectorDetected,detectionVectors[est-1],nSfb*sizeof(unsigned char));
    }

    FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(nSfb);
    memset(detectionVectors[est], 0, nSfb*sizeof(unsigned char));

    ADD(2); BRANCH(1);
    if(est < totNoEst-1){
      FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(nSfb);
      memset(guideVectors[est+1].guideVectorDiff,0,nSfb*sizeof(float));

      FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(nSfb);
      memset(guideVectors[est+1].guideVectorOrig,0,nSfb*sizeof(float));

      FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(nSfb);
      memset(guideVectors[est+1].guideVectorDetected,0,nSfb*sizeof(unsigned char));

      FUNC(11);
      detection(quotaBuffer[est],
                indexVector,
                pDiffVecScfb[est],
                nSfb,
                detectionVectors[est],
                pFreqBandTable,
                sfmOrig[est],
                sfmSbr[est],
                guideVectors[est],
                guideVectors[est+1],
                newDetectionAllowed);
    }
    else{
      FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(nSfb);
      memset(guideVectors[est].guideVectorDiff,0,nSfb*sizeof(float));

      FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(nSfb);
      memset(guideVectors[est].guideVectorOrig,0,nSfb*sizeof(float));

      FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(nSfb);
      memset(guideVectors[est].guideVectorDetected,0,nSfb*sizeof(unsigned char));

      FUNC(11);
      detection(quotaBuffer[est],
                indexVector,
                pDiffVecScfb[est],
                nSfb,
                detectionVectors[est],
                pFreqBandTable,
                sfmOrig[est],
                sfmSbr[est],
                guideVectors[est],
                guideVectors[est],
                newDetectionAllowed);
    }


  }



  BRANCH(1);
  if(newDetectionAllowed){

    ADD(1); BRANCH(1);
    if(totNoEst > 1){

      INDIRECT(1); FUNC(9);
      transientCleanUp(quotaBuffer,
                       pDiffVecScfb,
                       nSfb,
                       detectionVectors,
                       pFreqBandTable,
                       guideVectors[noEstPerFrame],
                       newDetectionAllowed,
                       start,
                       totNoEst);
    }
    else{

       FUNC(9);
       transientCleanUp(quotaBuffer,
                       pDiffVecScfb,
                       nSfb,
                       detectionVectors,
                       pFreqBandTable,
                       guideVectors[0],
                       newDetectionAllowed,
                       start,
                       totNoEst);
    }
  }



  PTR_INIT(1); /* pAddHarmonicsScaleFactorBands[] */
  LOOP(1);
  for(i = 0; i< nSfb; i++){

    PTR_INIT(1); /* detectionVectors[][] */
    LOOP(1);
    for(est = start; est < totNoEst; est++){

      LOGIC(1); STORE(1);
      pAddHarmonicsScaleFactorBands[i] = pAddHarmonicsScaleFactorBands[i] || detectionVectors[est][i];
    }
  }


  BRANCH(1);
  if(!newDetectionAllowed){
    PTR_INIT(2); /* pAddHarmonicsScaleFactorBands[]
                    prevFrameSfbHarm[]
                 */
    LOOP(1);
    for(i=0;i<nSfb;i++){

      ADD(1); BRANCH(1);
      if(pAddHarmonicsScaleFactorBands[i] - prevFrameSfbHarm[i] > 0)
      {

        MOVE(1);
        pAddHarmonicsScaleFactorBands[i] = 0;
      }
    }
  }

  COUNT_sub_end();
}


/**************************************************************************/
/*!
  \brief     Calculates a compensation vector.
  \return    none.

*/
/**************************************************************************/
static void calculateCompVector(unsigned char* pAddHarmonicsScaleFactorBands, /*!<  */
                                float** tonality,
                                char* envelopeCompensation,
                                int nSfb,
                                const unsigned char* freqBandTable,
                                float** diff,
                                int totNoEst,
                                char *prevEnvelopeCompensation,
                                int newDetectionAllowed)
{
  int i,j,l,ll,lu,maxPosF,maxPosT;
  float maxVal;
  int compValue;

  COUNT_sub_start("calculateCompVector");



  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(nSfb);
  memset(envelopeCompensation,0,nSfb*sizeof(char));

  PTR_INIT(3); /* pAddHarmonicsScaleFactorBands[]
                  freqBandTable[]
                  envelopeCompensation[]
               */
  LOOP(1);
  for(i=0 ; i < nSfb; i++){

    BRANCH(1);
    if(pAddHarmonicsScaleFactorBands[i]){ /* A missing sine was detected*/

      MOVE(2);
      ll = freqBandTable[i];
      lu = freqBandTable[i+1];

      MOVE(3);
      maxPosF = 0;
      maxPosT = 0;
      maxVal = 0;

      LOOP(1);
      for(j=0;j<totNoEst;j++){

        PTR_INIT(1); /* tonality[][] */
        LOOP(1);
        for(l=ll; l<lu; l++){

          ADD(1); BRANCH(1);
          if(tonality[j][l] > maxVal)
          {
            MOVE(3);
            maxVal = tonality[j][l];
            maxPosF = l;
            maxPosT = j;
          }
        }
      }


      ADD(1); LOGIC(1); BRANCH(1);
      if(maxPosF == ll && i){

        INDIRECT(1); ADD(2); TRANS(1); MULT(1); MISC(1);
        compValue = (int) (fabs(ILOG2*log(diff[maxPosT][i - 1]+EPS)) + 0.5f);

        ADD(1); BRANCH(1);
        if (compValue > MAX_COMP)
        {
          MOVE(1);
          compValue = MAX_COMP;
        }

        BRANCH(1);
        if(!pAddHarmonicsScaleFactorBands[i-1]) {

          INDIRECT(1); MULT(1); ADD(1); BRANCH(1);
          if(tonality[maxPosT][maxPosF -1] > TONALITY_QUOTA*tonality[maxPosT][maxPosF]){

            MULT(1); STORE(1);
            envelopeCompensation[i-1] = -1*compValue;
          }
        }
      }

      ADD(4); BRANCH(1);
      if(maxPosF == lu-1 && i+1 < nSfb){

        INDIRECT(1); ADD(2); TRANS(1); MULT(1); MISC(1);
        compValue = (int) (fabs(ILOG2*log(diff[maxPosT][i + 1]+EPS)) + 0.5f);

        ADD(1); BRANCH(1);
        if (compValue > MAX_COMP)
        {
          MOVE(1);
          compValue = MAX_COMP;
        }

        BRANCH(1);
        if(!pAddHarmonicsScaleFactorBands[i+1]) {

          INDIRECT(1); MULT(1); ADD(1); BRANCH(1);
          if(tonality[maxPosT][maxPosF+1] > TONALITY_QUOTA*tonality[maxPosT][maxPosF]){

            MOVE(1);
            envelopeCompensation[i+1] = compValue;
          }
        }
      }

      ADD(2); LOGIC(1); BRANCH(1);
      if(i && i < nSfb - 1){

        INDIRECT(1); ADD(2); TRANS(1); MULT(1); MISC(1);
        compValue = (int) (fabs(ILOG2*log(diff[maxPosT][i -1]+EPS)) + 0.5f);

        ADD(1); BRANCH(1);
        if (compValue > MAX_COMP)
        {
          MOVE(1);
          compValue = MAX_COMP;
        }

        DIV(1); MULT(1); ADD(1); BRANCH(1);
        if((float)1.0f/diff[maxPosT][i-1] > DIFF_QUOTA*diff[maxPosT][i]){

          MULT(1); STORE(1);
          envelopeCompensation[i-1] = -1*compValue;
        }

        INDIRECT(1); ADD(2); TRANS(1); MULT(1); MISC(1);
        compValue = (int) (fabs(ILOG2*log(diff[maxPosT][i + 1]+EPS)) + 0.5f);

        ADD(1); BRANCH(1);
        if (compValue > MAX_COMP)
        {
          MOVE(1);
          compValue = MAX_COMP;
        }

        DIV(1); MULT(1); ADD(1); BRANCH(1);
        if((float)1.0f/diff[maxPosT][i+1] > DIFF_QUOTA*diff[maxPosT][i]){

          MOVE(1);
          envelopeCompensation[i+1] = compValue;
        }
      }
    }
  }



  BRANCH(1);
  if(!newDetectionAllowed){

    PTR_INIT(2); /* envelopeCompensation[]
                    prevEnvelopeCompensation[]
                 */
    LOOP(1);
    for(i=0;i<nSfb;i++){

      LOGIC(1); BRANCH(1);
      if(envelopeCompensation[i] != 0 && prevEnvelopeCompensation[i] == 0)
      {
        MOVE(1);
        envelopeCompensation[i] = 0;
      }
    }
  }

  COUNT_sub_end();
}



/**************************************************************************/
/*!
  \brief     Detects missing harmonics in the QMF
  \return    none.

*/
/**************************************************************************/
void
SbrMissingHarmonicsDetectorQmf(HANDLE_SBR_MISSING_HARMONICS_DETECTOR h_sbrMHDet,
                               float ** pQuotaBuffer,
                               char* indexVector,
                               const SBR_FRAME_INFO *pFrameInfo,
                               const int* pTranInfo,
                               int* pAddHarmonicsFlag,
                               unsigned char* pAddHarmonicsScaleFactorBands,
                               const unsigned char* freqBandTable,
                               int nSfb,
                               char* envelopeCompensation)
{

  int i;
  int transientFlag = pTranInfo[1];
  int transientPos  = pTranInfo[0];
  int newDetectionAllowed;


  unsigned char ** detectionVectors  = h_sbrMHDet->detectionVectors;
  int move                = h_sbrMHDet->move;
  int noEstPerFrame       = h_sbrMHDet->noEstPerFrame;
  int totNoEst            = h_sbrMHDet->totNoEst;
  float** sfmSbr          = h_sbrMHDet->sfmSbr;
  float** sfmOrig         = h_sbrMHDet->sfmOrig;
  float** tonalityDiff    = h_sbrMHDet->tonalityDiff;
  int prevTransientFlag   = h_sbrMHDet->previousTransientFlag;
  int prevTransientFrame  = h_sbrMHDet->previousTransientFrame;
  int transientPosOffset  = h_sbrMHDet->transientPosOffset;
  int prevTransientPos    = h_sbrMHDet->previousTransientPos;
  GUIDE_VECTORS* guideVectors = h_sbrMHDet->guideVectors;

  int noQmfBands =  freqBandTable[nSfb] - freqBandTable[0];

  COUNT_sub_start("SbrMissingHarmonicsDetectorQmf");

  INDIRECT(12); PTR_INIT(5); MOVE(8); /* counting previous operation */

  FUNC(8);
  newDetectionAllowed = isDetectionOfNewToneAllowed(pFrameInfo,
                                                    prevTransientFrame,
                                                    prevTransientPos,
                                                    prevTransientFlag,
                                                    transientPosOffset,
                                                    transientFlag,
                                                    transientPos,
                                                    h_sbrMHDet);


  FUNC(10);
  calculateDetectorInput(pQuotaBuffer,
                         indexVector,
                         tonalityDiff,
                         sfmOrig,
                         sfmSbr,
                         freqBandTable,
                         nSfb,
                         noEstPerFrame,
                         move,
                         noQmfBands);


  INDIRECT(1); FUNC(14);
  detectionWithPrediction(pQuotaBuffer,
                          indexVector,
                          tonalityDiff,
                          nSfb,
                          freqBandTable,
                          sfmOrig,
                          sfmSbr,
                          detectionVectors,
                          h_sbrMHDet->guideScfb,
                          guideVectors,
                          noEstPerFrame,
                          totNoEst,
                          newDetectionAllowed,
                          pAddHarmonicsScaleFactorBands);



  INDIRECT(1); FUNC(9);
  calculateCompVector(pAddHarmonicsScaleFactorBands,
                      pQuotaBuffer,
                      envelopeCompensation,
                      nSfb,
                      freqBandTable,
                      tonalityDiff,
                      totNoEst,
                      h_sbrMHDet->prevEnvelopeCompensation,
                      newDetectionAllowed);



  MOVE(1);
  *pAddHarmonicsFlag = 0;

  PTR_INIT(1); /* pAddHarmonicsScaleFactorBands[i] */
  LOOP(1);
  for(i=0;i<nSfb;i++){

    BRANCH(1);
    if(pAddHarmonicsScaleFactorBands[i]){

      MOVE(1);
      *pAddHarmonicsFlag = 1;
      break;
    }
  }


  FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(nSfb);
  memcpy(h_sbrMHDet->prevEnvelopeCompensation, envelopeCompensation, nSfb*sizeof(char));

  FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(nSfb);
  memcpy(h_sbrMHDet->guideScfb, pAddHarmonicsScaleFactorBands, nSfb*sizeof(unsigned char));

  FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(nSfb);
  memcpy(guideVectors[0].guideVectorDetected,pAddHarmonicsScaleFactorBands,nSfb*sizeof(unsigned char));

  ADD(1); BRANCH(1);
  if(totNoEst > noEstPerFrame){
    FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(nSfb);
    memcpy(guideVectors[0].guideVectorDiff,guideVectors[noEstPerFrame].guideVectorDiff,nSfb*sizeof(float));

    FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(nSfb);
    memcpy(guideVectors[0].guideVectorOrig,guideVectors[noEstPerFrame].guideVectorOrig,nSfb*sizeof(float));
  }
  else {
    FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(nSfb);
    memcpy(guideVectors[0].guideVectorDiff,guideVectors[noEstPerFrame-1].guideVectorDiff,nSfb*sizeof(float));

    FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(nSfb);
    memcpy(guideVectors[0].guideVectorOrig,guideVectors[noEstPerFrame-1].guideVectorOrig,nSfb*sizeof(float));
  }

  PTR_INIT(3); /* guideVectors[0].guideVectorDiff[]
                  guideVectors[0].guideVectorOrig[]
                  pAddHarmonicsScaleFactorBands[]
               */
  LOOP(1);
  for(i=0;i<nSfb;i++){

    LOGIC(2); BRANCH(1);
    if((guideVectors[0].guideVectorDiff[i] ||
        guideVectors[0].guideVectorOrig[i]) &&
      !pAddHarmonicsScaleFactorBands[i]){

      MOVE(2);
      guideVectors[0].guideVectorDiff[i] = 0;
      guideVectors[0].guideVectorOrig[i] = 0;
    }
  }

  COUNT_sub_end();
}

/**************************************************************************/
/*!
  \brief     Creates an instance of the missing harmonics detector.
  \return    errorCode, noError if OK.

*/
/**************************************************************************/
int
CreateSbrMissingHarmonicsDetector (int chan,
                                   HANDLE_SBR_MISSING_HARMONICS_DETECTOR hSbrMHDet,
                                   int sampleFreq,
                                   unsigned char* freqBandTable,
                                   int nSfb,
                                   int qmfNoChannels,
                                   int totNoEst,
                                   int move,
                                   int noEstPerFrame)
{
  int i;
  float* ptr;

  HANDLE_SBR_MISSING_HARMONICS_DETECTOR hs =hSbrMHDet;

  COUNT_sub_start("CreateSbrMissingHarmonicsDetector");

  PTR_INIT(1); /* counting previous operation */

  assert(totNoEst == NO_OF_ESTIMATES);

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(SBR_MISSING_HARMONICS_DETECTOR));
  memset(hs,0,sizeof( SBR_MISSING_HARMONICS_DETECTOR));


  INDIRECT(2); MOVE(2);
  hs->transientPosOffset =  4;
  hs->timeSlots          = 16;


  INDIRECT(3); MOVE(3);
  hs->qmfNoChannels = qmfNoChannels;
  hs->sampleFreq = sampleFreq;
  hs->nSfb = nSfb;

  INDIRECT(3); MOVE(3);
  hs->totNoEst = totNoEst;
  hs->move = move;
  hs->noEstPerFrame = noEstPerFrame;

  MULT(1); INDIRECT(1); PTR_INIT(1);
  ptr = &sbr_toncorrBuff[chan*5*NO_OF_ESTIMATES*MAX_FREQ_COEFFS];

  PTR_INIT(5); /* hs->tonalityDiff[i]
                  hs->sfmOrig[i]
                  hs->sfmSbr[i]
                  hs->guideVectors[i]
                  hs->detectionVectors[i]
               */
  LOOP(1);
  for(i=0; i < totNoEst; i++) {

    PTR_INIT(1); ADD(1);
    hs->tonalityDiff[i] = ptr; ptr += MAX_FREQ_COEFFS;

    FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(MAX_FREQ_COEFFS);
    memset(hs->tonalityDiff[i],0,sizeof(float)*MAX_FREQ_COEFFS);

    PTR_INIT(1); ADD(1);
    hs->sfmOrig[i] = ptr; ptr += MAX_FREQ_COEFFS;

    FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(MAX_FREQ_COEFFS);
    memset(hs->sfmOrig[i],0,sizeof(float)*MAX_FREQ_COEFFS);

    PTR_INIT(1); ADD(1);
    hs->sfmSbr[i]  = ptr; ptr += MAX_FREQ_COEFFS;

    FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(MAX_FREQ_COEFFS);
    memset(hs->sfmSbr[i],0,sizeof(float)*MAX_FREQ_COEFFS);

    PTR_INIT(1); ADD(1);
    hs->guideVectors[i].guideVectorDiff = ptr; ptr += MAX_FREQ_COEFFS;

    FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(MAX_FREQ_COEFFS);
    memset(hs->guideVectors[i].guideVectorDiff,0,sizeof(float)*MAX_FREQ_COEFFS);

    PTR_INIT(1); ADD(1);
    hs->guideVectors[i].guideVectorOrig = ptr; ptr += MAX_FREQ_COEFFS;

    FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(MAX_FREQ_COEFFS);
    memset(hs->guideVectors[i].guideVectorOrig,0,sizeof(float)*MAX_FREQ_COEFFS);

    MULT(2); ADD(1); PTR_INIT(1); ADD(1);
    hs->detectionVectors[i] = &(sbr_detectionVectors[chan*NO_OF_ESTIMATES*MAX_FREQ_COEFFS + i*MAX_FREQ_COEFFS]);

    FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(MAX_FREQ_COEFFS);
    memset(hs->detectionVectors[i],0,sizeof(unsigned char)*MAX_FREQ_COEFFS);

    MULT(2); ADD(1); PTR_INIT(1); ADD(1);
    hs->guideVectors[i].guideVectorDetected = &(sbr_guideVectorDetected[chan*NO_OF_ESTIMATES*MAX_FREQ_COEFFS + i*MAX_FREQ_COEFFS]);

    FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(MAX_FREQ_COEFFS);
    memset(hs->guideVectors[i].guideVectorDetected,0,sizeof(unsigned char)*MAX_FREQ_COEFFS);

  }

  INDIRECT(2); MULT(1); PTR_INIT(1);
  hs->prevEnvelopeCompensation = &(sbr_prevEnvelopeCompensation[chan*MAX_FREQ_COEFFS]);

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(MAX_FREQ_COEFFS);
  memset( hs->prevEnvelopeCompensation,0, sizeof(char)*MAX_FREQ_COEFFS);

  INDIRECT(2); MULT(1); PTR_INIT(1);
  hs->guideScfb = &(sbr_guideScfb[chan*MAX_FREQ_COEFFS]);

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(MAX_FREQ_COEFFS);
  memset( hs->guideScfb,0, sizeof(unsigned char)*MAX_FREQ_COEFFS);

  INDIRECT(3); MOVE(3);
  hs->previousTransientFlag = 0;
  hs->previousTransientFrame = 0;
  hs->previousTransientPos = 0;

  assert (ptr-&sbr_toncorrBuff[0] <= 5*MAX_CHANNELS*NO_OF_ESTIMATES*MAX_FREQ_COEFFS);

  COUNT_sub_end();

  return 0;
}

