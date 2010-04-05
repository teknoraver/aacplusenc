/*
  Noise floor estimation
*/
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sbr.h"
#include "nf_est.h"

#include "counters.h" /* the 3GPP instrumenting tool */

static const float smoothFilter[4]  = {0.05857864376269f, 0.2f, 0.34142135623731f, 0.4f};

#ifndef min
#define min(a,b) ( a < b ? a:b)
#endif

#ifndef max
#define max(a,b) ( a > b ? a:b)
#endif


/**************************************************************************/
/*!
  \brief     The function applies smoothing to the noise levels.
  \return    none

*/
/**************************************************************************/
static void
smoothingOfNoiseLevels (float *NoiseLevels,
                        int nEnvelopes,
                        int noNoiseBands,
                        float prevNoiseLevels[NF_SMOOTHING_LENGTH][MAX_NUM_NOISE_VALUES],
                        const float* smoothFilter)
{
  int i,band,env;

  COUNT_sub_start("smoothingOfNoiseLevels");

  LOOP(1);
  for(env = 0; env < nEnvelopes; env++){

    LOOP(1);
    for (i = 1; i < NF_SMOOTHING_LENGTH; i++){
      FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(noNoiseBands);
      memcpy(prevNoiseLevels[i - 1],prevNoiseLevels[i],noNoiseBands*sizeof(float));
    }

    FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(noNoiseBands);
    memcpy(prevNoiseLevels[NF_SMOOTHING_LENGTH - 1],NoiseLevels+env*noNoiseBands,noNoiseBands*sizeof(float));

    MULT(1); PTR_INIT(1); /* pointer for NoiseLevels[] */
    LOOP(1);
    for (band = 0; band < noNoiseBands; band++){

      MOVE(1);
      NoiseLevels[band+ env*noNoiseBands] = 0;

      PTR_INIT(2); /* pointer for smoothFilter[],
                                  prevNoiseLevels[][]
                   */
      LOOP(1);
      for (i = 0; i < NF_SMOOTHING_LENGTH; i++){

        MAC(1);
        NoiseLevels[band+ env*noNoiseBands] += smoothFilter[i]*prevNoiseLevels[i][band];
      }
      STORE(1); /* NoiseLevels[band+ env*noNoiseBands] */
    }
  }

  COUNT_sub_end();
}




/**************************************************************************/
/*!
  \brief     Does the noise floor level estiamtion.
  \return    none

*/
/**************************************************************************/
static void
qmfBasedNoiseFloorDetection(float *noiseLevel,
                            float** quotaMatrixOrig,
                            char* indexVector,
                            int startIndex,
                            int stopIndex,
                            int startChannel,
                            int stopChannel,
                            float ana_max_level,
                            float noiseFloorOffset,
                            int missingHarmonicFlag,
                            float weightFac,
                            INVF_MODE diffThres,
                            INVF_MODE inverseFilteringLevel)
{
  int l,k;
  float meanOrig=0, meanSbr=0, diff;
  float tonalityOrig, tonalitySbr;

  COUNT_sub_start("qmfBasedNoiseFloorDetection");

  MOVE(2); /* counting previous operations */

  ADD(1); BRANCH(1);
  if(missingHarmonicFlag == 1){

    PTR_INIT(1); /* pointers for indexVector[l] */
    LOOP(1);
    for(l = startChannel; l < stopChannel;l++){

      MOVE(1);
      tonalityOrig = 0;

      PTR_INIT(1); /* pointers for quotaMatrixOrig[][] */
      LOOP(1);
      for(k = startIndex ; k < stopIndex; k++){

        ADD(1);
        tonalityOrig += quotaMatrixOrig[k][l];
      }

      ADD(1); DIV(1);
      tonalityOrig /= (stopIndex-startIndex);

      MOVE(1);
      tonalitySbr = 0;

      LOOP(1);
      for(k = startIndex ; k < stopIndex; k++){

        INDIRECT(1); ADD(1);
        tonalitySbr += quotaMatrixOrig[k][indexVector[l]];
      }

      /* ADD(1); already calculated */ DIV(1);
      tonalitySbr /= (stopIndex-startIndex);


      ADD(1); BRANCH(1);
      if(tonalityOrig > meanOrig)
      {
        MOVE(1);
        meanOrig = tonalityOrig;
      }

      ADD(1); BRANCH(1);
      if(tonalitySbr > meanSbr)
      {
        MOVE(1);
        meanSbr = tonalitySbr;
      }
    }
  }
  else{

    PTR_INIT(1); /* pointers for indexVector[l] */
    LOOP(1);
    for(l = startChannel; l < stopChannel;l++){

      MOVE(1);
      tonalityOrig = 0;

      PTR_INIT(1); /* pointers for quotaMatrixOrig[][] */
      LOOP(1);
      for(k = startIndex ; k < stopIndex; k++){

        ADD(1);
        tonalityOrig += quotaMatrixOrig[k][l];
      }

      ADD(1); DIV(1);
      tonalityOrig /= (stopIndex-startIndex);

      MOVE(1);
      tonalitySbr = 0;

      LOOP(1);
      for(k = startIndex ; k < stopIndex; k++){

        INDIRECT(1); ADD(1);
        tonalitySbr += quotaMatrixOrig[k][indexVector[l]];
      }

      /* ADD(1); already calculated */ DIV(1);
      tonalitySbr /= (stopIndex-startIndex);

      ADD(2);
      meanOrig += tonalityOrig;
      meanSbr += tonalitySbr;
    }

    ADD(1); DIV(2);
    meanOrig /= (stopChannel - startChannel);
    meanSbr /= (stopChannel - startChannel);
  }


  ADD(2); LOGIC(1); BRANCH(1);
  if(meanOrig < (float)0.000976562f && meanSbr < (float)0.000976562f){

    MOVE(2);
    meanOrig = 101.5936673f;
    meanSbr  = 101.5936673f;
  }

  ADD(1); BRANCH(1);
  if(meanOrig < (float)1.0f)
  {
    MOVE(1);
    meanOrig = 1.0f;
  }

  ADD(1); BRANCH(1);
  if(meanSbr < (float)1.0f)
  {
    MOVE(1);
    meanSbr  = 1.0f;
  }



  ADD(1); BRANCH(1);
  if(missingHarmonicFlag == 1){

    MOVE(1);
    diff = 1.0f;
  }
  else{

    MULT(1); DIV(1); ADD(1); BRANCH(1); MOVE(1);
    diff = max ((float)1.0f, weightFac*meanSbr/meanOrig);
  }

  ADD(3); LOGIC(2); BRANCH(1);
  if(inverseFilteringLevel == INVF_MID_LEVEL ||
     inverseFilteringLevel == INVF_LOW_LEVEL ||
     inverseFilteringLevel == INVF_OFF){

    MOVE(1);
    diff = 1.0f;
  }

  ADD(1); BRANCH(1);
  if (inverseFilteringLevel <= diffThres) {

    MOVE(1);
    diff = 1.0f;
  }


  DIV(1); /* value still in register */
  *noiseLevel =  diff/meanOrig;

  MULT(1); /* value still in register */
  *noiseLevel *= noiseFloorOffset;


  ADD(1); BRANCH(1); MOVE(1);
  *noiseLevel = min (*noiseLevel, (float)ana_max_level);

  COUNT_sub_end();
}









/**************************************************************************/
/*!
  \brief     Does the noise floor level estiamtion..
  \return    none

*/
/**************************************************************************/
void
sbrNoiseFloorEstimateQmf (HANDLE_SBR_NOISE_FLOOR_ESTIMATE h_sbrNoiseFloorEstimate,
                          const SBR_FRAME_INFO *frame_info,
                          float *noiseLevels,
                          float** quotaMatrixOrig,
                          char* indexVector,
                          int missingHarmonicsFlag,
                          int startIndex,
                          int numberOfEstiamtesPerFrame,
                          int totalNumberOfEstimates,
                          int transientFlag,
                          INVF_MODE* pInvFiltLevels
                          )

{

  int nNoiseEnvelopes, startPos[2], stopPos[2], env, band;

  int noNoiseBands      = h_sbrNoiseFloorEstimate->noNoiseBands;
  int *freqBandTable    = h_sbrNoiseFloorEstimate->freqBandTableQmf;

  COUNT_sub_start("sbrNoiseFloorEstimateQmf");

  INDIRECT(2); MOVE(1); PTR_INIT(1); /* counting previous operations */

  INDIRECT(1); MOVE(1);
  nNoiseEnvelopes = frame_info->nNoiseEnvelopes;

  ADD(1); BRANCH(1);
  if(nNoiseEnvelopes == 1){

    MOVE(1);
    startPos[0] = startIndex;

    ADD(1); STORE(1);
    stopPos[0]  = startIndex + 2;
  }
  else{

    MOVE(1);
    startPos[0] = startIndex;

    ADD(3); STORE(3);
    stopPos[0]  = startIndex + 1;
    startPos[1] = startIndex + 1;
    stopPos[1]  = startIndex + 2;
  }


  PTR_INIT(2); /* pointers for startPos[],
                               stopPos[]
               */
  LOOP(1);
  for(env = 0; env < nNoiseEnvelopes; env++){

    PTR_INIT(4); /* pointers for h_sbrNoiseFloorEstimate->noiseFloorOffset[],
                                 noiseLevels[],
                                 freqBandTable[],
                                 pInvFiltLevels[]
                 */
    LOOP(1);
    for(band = 0; band < noNoiseBands; band++){


      INDIRECT(4); PTR_INIT(1); MULT(1); FUNC(13);
      qmfBasedNoiseFloorDetection(&noiseLevels[band + env*noNoiseBands],
                                  quotaMatrixOrig,
                                  indexVector,
                                  startPos[env],
                                  stopPos[env],
                                  freqBandTable[band],
                                  freqBandTable[band+1],
                                  h_sbrNoiseFloorEstimate->ana_max_level,
                                  h_sbrNoiseFloorEstimate->noiseFloorOffset[band],
                                  missingHarmonicsFlag,
                                  h_sbrNoiseFloorEstimate->weightFac,
                                  h_sbrNoiseFloorEstimate->diffThres,
                                  pInvFiltLevels[band]);

    }

  }


  INDIRECT(3); FUNC(5);
  smoothingOfNoiseLevels(noiseLevels,
                         nNoiseEnvelopes,
                         h_sbrNoiseFloorEstimate->noNoiseBands,
                         h_sbrNoiseFloorEstimate->prevNoiseLevels,
                         h_sbrNoiseFloorEstimate->smoothFilter);




  LOOP(1);
  for(env = 0; env < nNoiseEnvelopes; env++){

    PTR_INIT(1); /* pointers for noiseLevels[] */
    LOOP(1);
    for(band = 0; band < noNoiseBands; band++){

      MULT(1); /* env*noNoiseBands */ TRANS(1); MULT(1); ADD(1); STORE(1);
      noiseLevels[band + env*noNoiseBands] = NOISE_FLOOR_OFFSET - (float) (ILOG2*log(noiseLevels[band + env*noNoiseBands]));
    }
  }

  COUNT_sub_end();
}



/**************************************************************************/
/*!
  \brief
  \return    errorCode,

*/
/**************************************************************************/
static int
downSampleLoRes(int *v_result,
                int num_result,
                const unsigned char *freqBandTableRef,
                int num_Ref)
{
  int step;
  int i,j;
  int org_length,result_length;
  int v_index[MAX_FREQ_COEFFS/2];

  COUNT_sub_start("downSampleLoRes");

  MOVE(2);
  org_length=num_Ref;
  result_length=num_result;

  MOVE(1);
  v_index[0]=0;	/* Always use left border */

  i=0;

  PTR_INIT(1); /* v_index[] */
  LOOP(1);
  while(org_length > 0)
    {
      i++;

      DIV(1);
      step=org_length/result_length;

      ADD(1);
      org_length=org_length - step;

      ADD(1);
      result_length--;

      ADD(1); STORE(1);
      v_index[i]=v_index[i-1]+step;
    }

  ADD(1); BRANCH(1);
  if(i != num_result )
  {
    COUNT_sub_end();
    return (1);/* error */
  }

  PTR_INIT(2); /* v_result[j]
                  v_index[j]
               */
  LOOP(1);
  for(j=0;j<=i;j++)
    {
      INDIRECT(1); MOVE(1);
      v_result[j]=freqBandTableRef[v_index[j]];
    }

  COUNT_sub_end();

  return (0);
}



/**************************************************************************/
/*!
  \brief    Creates an instance of the noise floor level estimation module.
  \return    errorCode

*/
/**************************************************************************/
int
CreateSbrNoiseFloorEstimate (HANDLE_SBR_NOISE_FLOOR_ESTIMATE  h_sbrNoiseFloorEstimate,
                             int ana_max_level,
                             const unsigned char *freqBandTable,
                             int nSfb,
                             int noiseBands,
                             int noiseFloorOffset,
                             unsigned int useSpeechConfig
                            )
{
  int i;


  COUNT_sub_start("CreateSbrNoiseFloorEstimate");

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(SBR_NOISE_FLOOR_ESTIMATE));
  memset(h_sbrNoiseFloorEstimate,0,sizeof(SBR_NOISE_FLOOR_ESTIMATE));



  INDIRECT(1); MOVE(1);
  h_sbrNoiseFloorEstimate->smoothFilter = smoothFilter;

  BRANCH(1);
  if (useSpeechConfig) {

    INDIRECT(2); MOVE(2);
    h_sbrNoiseFloorEstimate->weightFac = 1.0;
    h_sbrNoiseFloorEstimate->diffThres = INVF_LOW_LEVEL;
  }
  else {

    INDIRECT(2); MOVE(2);
    h_sbrNoiseFloorEstimate->weightFac = 0.25;
    h_sbrNoiseFloorEstimate->diffThres = INVF_MID_LEVEL;
  }

  INDIRECT(1); DIV(1); TRANS(1); STORE(1);
  h_sbrNoiseFloorEstimate->ana_max_level          = (float) pow(2,ana_max_level/3.0f);

  INDIRECT(1); MOVE(1);
  h_sbrNoiseFloorEstimate->noiseBands             = noiseBands;


  FUNC(3); BRANCH(1);
  if(resetSbrNoiseFloorEstimate(h_sbrNoiseFloorEstimate,freqBandTable,nSfb))
  {
    COUNT_sub_end();
    return(1);
  }


  PTR_INIT(1); /* h_sbrNoiseFloorEstimate->noiseFloorOffset[] */
  INDIRECT(1); LOOP(1);
  for(i=0;i<h_sbrNoiseFloorEstimate->noNoiseBands;i++)
  {
    DIV(1); TRANS(1); STORE(1);
    h_sbrNoiseFloorEstimate->noiseFloorOffset[i] = (float) pow(2,noiseFloorOffset/3.0f);
  }

  COUNT_sub_end();

  return 0;
}



/**************************************************************************/
/*!
  \brief     Resets the current instance of the noise floor estiamtion
          module.
  \return    errorCode

*/
/**************************************************************************/
int
resetSbrNoiseFloorEstimate (HANDLE_SBR_NOISE_FLOOR_ESTIMATE h_sbrNoiseFloorEstimate,
                            const unsigned char *freqBandTable,
                            int nSfb)
{
  int k2,kx;

  COUNT_sub_start("resetSbrNoiseFloorEstimate");

  INDIRECT(1); MOVE(2);
  k2=freqBandTable[nSfb];
  kx=freqBandTable[0];

  INDIRECT(1); BRANCH(1);
  if(h_sbrNoiseFloorEstimate->noiseBands == 0){
    INDIRECT(1); MOVE(1);
    h_sbrNoiseFloorEstimate->noNoiseBands = 1;
  }
  else{
    INDIRECT(1); DIV(1); MULT(1); TRANS(1); ADD(1); STORE(1);
    h_sbrNoiseFloorEstimate->noNoiseBands =(int) ( (h_sbrNoiseFloorEstimate->noiseBands * log( (float)k2/kx) * ILOG2)+0.5);

    BRANCH(1);
    if( h_sbrNoiseFloorEstimate->noNoiseBands==0)
    {
      MOVE(1);
      h_sbrNoiseFloorEstimate->noNoiseBands=1;
    }
  }

  INDIRECT(2); FUNC(4); /* counting post-operations */

  COUNT_sub_end();

  return(downSampleLoRes(h_sbrNoiseFloorEstimate->freqBandTableQmf,
                         h_sbrNoiseFloorEstimate->noNoiseBands,
                         freqBandTable,nSfb));
}





/**************************************************************************/
/*!
  \brief     Deletes the current instancce of the noise floor level
  estimation module.
  \return    none

*/
/**************************************************************************/
void
deleteSbrNoiseFloorEstimate (HANDLE_SBR_NOISE_FLOOR_ESTIMATE h_sbrNoiseFloorEstimate)
{

  COUNT_sub_start("deleteSbrNoiseFloorEstimate");

  BRANCH(1);
  if (h_sbrNoiseFloorEstimate) {
    /*
      nothing to do
    */
  }

  COUNT_sub_end();
}
