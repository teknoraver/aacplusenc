/*
  Envelope estimation
*/
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <math.h>

#include "sbr.h"
#include "env_est.h"
#include "tran_det.h"
#include "qmf_enc.h"
#include "fram_gen.h"
#include "bit_sbr.h"
#include "cmondata.h"
#include "sbr_ram.h"
#include "ps_enc.h"

#include "counters.h" /* the 3GPP instrumenting tool */

#ifndef min
#define min(a,b) ( a < b ? a:b)
#endif

#ifndef max
#define max(a,b) ( a > b ? a:b)
#endif



#define QUANT_ERROR_THRES 200


/***************************************************************************/
/*!

  \brief  Quantisation of the panorama value

  \return the quantized pan value

****************************************************************************/
static int
mapPanorama (int nrgVal,
             int ampRes,
             int *quantError
             )
{
  int i;
  int min_val, val;
  int panTable[2][10] = {{0,2,4,6,8,12,16,20,24},
                         { 0, 2, 4, 8, 12 }};
  int maxIndex[2] = {9,5};

  int panIndex;
  int sign;

  COUNT_sub_start("mapPanorama");

  BRANCH(1); MOVE(1);
  sign = nrgVal > 0 ? 1 : -1;

  MULT(1);
  nrgVal = sign * nrgVal;

  MOVE(2);
  min_val = INT_MAX;
  panIndex = 0;

  PTR_INIT(2); /* pointer for maxIndex[ampRes],
                              panTable[ampRes][i]
               */
  LOOP(1);
  for (i = 0; i < maxIndex[ampRes]; i++) {

    ADD(1); MISC(1);
    val = abs (nrgVal - panTable[ampRes][i]);

    ADD(1); BRANCH(1);
    if (val < min_val) {

      MOVE(2);
      min_val = val;
      panIndex = i;
    }
  }

  MOVE(1);
  *quantError=min_val;

  INDIRECT(2); MULT(1); ADD(1); /* counting post-operations */

  COUNT_sub_end();

  return panTable[ampRes][maxIndex[ampRes]-1] + sign * panTable[ampRes][panIndex];
}


/***************************************************************************/
/*!

  \brief  Quantisation of the noise floor levels

  \return void

****************************************************************************/
static void
sbrNoiseFloorLevelsQuantisation (int   *iNoiseLevels,
                                 float *NoiseLevels,
                                 int   coupling
                                 )
{
  int i;
  int dummy;

  COUNT_sub_start("sbrNoiseFloorLevelsQuantisation");

  PTR_INIT(2); /* pointer for NoiseLevels[i],
                              iNoiseLevels[i]
               */
  LOOP(1);
  for (i = 0; i < MAX_NUM_NOISE_VALUES; i++) {
    int tmp;

    ADD(2); BRANCH(1);
    tmp = NoiseLevels[i] > (float)30.0f ? 30: (int) (NoiseLevels[i] + (float)0.5);

    BRANCH(1);
    if (coupling) {

      ADD(1); BRANCH(1); MOVE(1);
      tmp = tmp < -30 ? -30  : tmp;

      FUNC(3);
      tmp = mapPanorama (tmp,1,&dummy);
    }

    MOVE(1);
    iNoiseLevels[i] = tmp;

  }

  COUNT_sub_end();
}

/***************************************************************************/
/*!

  \brief  Calculation of noise floor for coupling

  \return void

****************************************************************************/
static void
coupleNoiseFloor(float *noise_level_left,
                 float *noise_level_right
                 )
{
  int i;

  COUNT_sub_start("coupleNoiseFloor");

  PTR_INIT(2); /* pointer for noise_level_left[i]
                              noise_level_right[i]
               */
  LOOP(1);
  for (i = 0; i < MAX_NUM_NOISE_VALUES; i++) {
    float temp1, temp2;

    MULT(2); ADD(2); TRANS(2);
    temp1 = (float) pow (2.0f, (- noise_level_right[i] + NOISE_FLOOR_OFFSET));
    temp2 = (float) pow (2.0f, (- noise_level_left[i]  + NOISE_FLOOR_OFFSET));

    ADD(2); DIV(1); TRANS(1); MULT(1); STORE(1);
    noise_level_left[i] =  (float)(NOISE_FLOOR_OFFSET - log((temp1 + temp2) / 2) * ILOG2);

    DIV(2); TRANS(1); MULT(1); STORE(1);
    noise_level_right[i] = (float) (log(temp2 / temp1) * ILOG2);
  }

  COUNT_sub_end();
}


/***************************************************************************/
/*!

  \brief  calculates the SBR envelope values

  \return void

****************************************************************************/
static void
calculateSbrEnvelope (float **YBufferLeft,
                      float **YBufferRight,
                      const SBR_FRAME_INFO *frame_info,
                      int *sfb_nrgLeft,
                      int *sfb_nrgRight,
                      HANDLE_SBR_CONFIG_DATA h_con,
                      HANDLE_ENV_CHANNEL h_sbr,
                      SBR_STEREO_MODE stereoMode,
                      int* maxQuantError)

{
  int i, j, k, l, count, m = 0;
  int no_of_bands, start_pos, stop_pos, li, ui;
  FREQ_RES freq_res;

  int ca = 2 - h_sbr->encEnvData.init_sbr_amp_res;
  int quantError;
  int nEnvelopes = frame_info->nEnvelopes;
  int short_env = frame_info->shortEnv - 1;
  int timeStep = h_sbr->sbrExtractEnvelope.time_step;
  int missingHarmonic = 0;

  COUNT_sub_start("calculateSbrEnvelope");

  INDIRECT(4); MOVE(4); ADD(2); /* counting previous operations */

  ADD(1); BRANCH(1);
  if (stereoMode == SBR_COUPLING) {

    MOVE(1);
    *maxQuantError = 0;
  }

  PTR_INIT(5); /* pointers for frame_info->borders[i]
                               frame_info->freqRes[i]
                               h_sbr->encEnvData.addHarmonic[i]
                               sfb_nrgLeft[m]
                               sfb_nrgRight[m]
               */
  LOOP(1);
  for (i = 0; i < nEnvelopes; i++) {

    MULT(2);
    start_pos = timeStep * frame_info->borders[i];
    stop_pos = timeStep * frame_info->borders[i + 1];

    MOVE(1);
    freq_res = frame_info->freqRes[i];

    INDIRECT(1); MOVE(1);
    no_of_bands = h_con->nSfb[freq_res];

    ADD(1); BRANCH(1);
    if (i == short_env) {
      ADD(1);
      stop_pos = stop_pos - timeStep;
    }


    PTR_INIT(2); /* pointers for h_con->freqBandTable[freq_res][j],
                                 h_sbr->encEnvData.addHarmonic[j]
                 */
    LOOP(1);
    for (j = 0; j < no_of_bands; j++) {

      float nrgRight = 0;
      float nrgLeft = 0;
      float temp;

      MOVE(2); /* counting previous operations */

      MOVE(2);
      li = h_con->freqBandTable[freq_res][j];
      ui = h_con->freqBandTable[freq_res][j + 1];

      ADD(1); BRANCH(1);
      if(freq_res == FREQ_RES_HIGH){

        ADD(2); LOGIC(1); BRANCH(1);
        if(j == 0 && ui-li > 1){
          ADD(1);
          li++;
        }
      }
      else{
        ADD(2); LOGIC(1); BRANCH(1);
        if(j == 0 && ui-li > 2){
          ADD(1);
          li++;
        }
      }


      MOVE(1);
      missingHarmonic = 0;

      INDIRECT(1); BRANCH(1);
      if(h_sbr->encEnvData.addHarmonicFlag){

        ADD(1); BRANCH(1);
        if(freq_res == FREQ_RES_HIGH){

          BRANCH(1);
          if(h_sbr->encEnvData.addHarmonic[j]){

            MOVE(1);
            missingHarmonic = 1;
          }
        }
        else{
          int i;
          int startBandHigh = 0;
          int stopBandHigh = 0;

          MOVE(2); /* counting previous operations */

          PTR_INIT(1); /* pointer for h_con->freqBandTable[FREQ_RES_HIGH][startBandHigh] */
          LOOP(1);
          while(h_con->freqBandTable[FREQ_RES_HIGH][startBandHigh] < h_con->freqBandTable[FREQ_RES_LOW][j]) {
            ADD(1); /* while() condition */

            ADD(1);
            startBandHigh++;
          }

          PTR_INIT(1); /* pointer for h_con->freqBandTable[FREQ_RES_HIGH][stopBandHigh] */
          LOOP(1);
          while(h_con->freqBandTable[FREQ_RES_HIGH][stopBandHigh] < h_con->freqBandTable[FREQ_RES_LOW][j + 1]) {
            ADD(1); /* while() condition */

            ADD(1);
            stopBandHigh++;
          }

          PTR_INIT(1); /* pointer for h_sbr->encEnvData.addHarmonic[] */
          LOOP(1);
          for(i = startBandHigh; i<stopBandHigh; i++){

            BRANCH(1);
            if(h_sbr->encEnvData.addHarmonic[i]){

              MOVE(1);
              missingHarmonic = 1;
            }
          }
        }

      }


      BRANCH(1);
      if(missingHarmonic){

        float tmpNrg = 0;

        MOVE(1); /* counting previous operation */

        ADD(1);
        count = (stop_pos - start_pos);

        PTR_INIT(1); /* pointer for YBufferLeft[][] */
        LOOP(1);
        for (l = start_pos; l < stop_pos; l++) {

          ADD(1);
          nrgLeft += YBufferLeft[l/2][li];
        }

        LOOP(1);
        for (k = li+1; k < ui; k++){

          MOVE(1);
          tmpNrg = 0;

          PTR_INIT(1); /* pointer for YBufferLeft[][] */
          LOOP(1);
          for (l = start_pos; l < stop_pos; l++) {

            ADD(1);
            tmpNrg += YBufferLeft[l/2][k];
          }

          ADD(1); BRANCH(1);
          if(tmpNrg > nrgLeft){

            MOVE(1);
            nrgLeft = tmpNrg;
          }
        }

        ADD(2); BRANCH(1);
        if(ui-li > 2){

          MULT(1);
          nrgLeft = nrgLeft*0.398107267f;

        }
        else{

          ADD(2); BRANCH(1);
          if(ui-li > 1){

            MULT(1);
            nrgLeft = nrgLeft*0.5f;
          }
        }

        ADD(1); BRANCH(1);
        if (stereoMode == SBR_COUPLING) {

          PTR_INIT(1); /* pointer for YBufferRight[][] */
          LOOP(1);
          for (l = start_pos; l < stop_pos; l++) {

            ADD(1);
            nrgRight += YBufferRight[l/2][li];
          }


          LOOP(1);
          for (k = li+1; k < ui; k++){

            MOVE(1);
            tmpNrg = 0;

            PTR_INIT(1); /* pointer for YBufferRight[][] */
            LOOP(1);
            for (l = start_pos; l < stop_pos; l++) {

              ADD(1);
              tmpNrg += YBufferRight[l/2][k];
            }

            ADD(1); BRANCH(1);
            if(tmpNrg > nrgRight){

              MOVE(1);
              nrgRight = tmpNrg;
            }
          }


          ADD(2); BRANCH(1);
          if(ui-li > 2){

            MULT(1);
            nrgRight = nrgRight*0.398107267f;

          }
          else{

            ADD(2); BRANCH(1);
            if(ui-li > 1){

              MULT(1);
              nrgRight = nrgRight*0.5f;
            }
          }

        }


        ADD(1); BRANCH(1);
        if (stereoMode == SBR_COUPLING) {

          MOVE(1);
          temp = nrgLeft;

          ADD(1); MULT(1);
          nrgLeft = (nrgRight + nrgLeft) * (float)0.5f;

          ADD(2); DIV(1);
          nrgRight = (temp + 1) / (nrgRight + 1);
        }


    } /* end missingHarmonic */
    else{

        ADD(2); MULT(1);
        count = (stop_pos - start_pos) * (ui - li);


        LOOP(1);
        for (k = li; k < ui; k++) {

          PTR_INIT(1); /* pointer for YBufferLeft[][] */
          LOOP(1);
          for (l = start_pos; l < stop_pos; l++) {

            ADD(1);
            nrgLeft += YBufferLeft[l/2][k];
          }
        }

        ADD(1); BRANCH(1);
        if (stereoMode == SBR_COUPLING) {

          LOOP(1);
          for (k = li; k < ui; k++) {

            PTR_INIT(1); /* pointer for YBufferRight[][] */
            LOOP(1);
            for (l = start_pos; l < stop_pos; l++) {

              ADD(1);
              nrgRight += YBufferRight[l/2][k];
            }
          }
        }

       ADD(1); BRANCH(1);
       if (stereoMode == SBR_COUPLING) {

        MOVE(1);
        temp = nrgLeft;

        ADD(1); MULT(1);
        nrgLeft = (nrgRight + nrgLeft) * 0.5f;

        ADD(2); DIV(1);
        nrgRight = (temp + 1) / (nrgRight + 1);
      }

    }


      INDIRECT(1); MULT(2); DIV(1); ADD(1); TRANS(1);
      nrgLeft = (float) (log (nrgLeft / (count * 64) + EPS) * ILOG2);

      BRANCH(1); MOVE(1);
      nrgLeft = max(nrgLeft,0);

      MULT(1); ADD(1); STORE(1);
      sfb_nrgLeft[m] = (int) (ca * nrgLeft + 0.5);

      ADD(1); BRANCH(1);
      if (stereoMode == SBR_COUPLING) {

        TRANS(1); MULT(1);
        nrgRight = (float) (log (nrgRight) * ILOG2);

        INDIRECT(1); MULT(1); PTR_INIT(1); FUNC(3); STORE(1);
        sfb_nrgRight[m] = mapPanorama ((int)((float)ca * nrgRight),h_sbr->encEnvData.init_sbr_amp_res,&quantError);

        ADD(1); BRANCH(1);
        if(quantError > *maxQuantError) {
          MOVE(1);
          *maxQuantError = quantError;
        }
      }
      m++;

    } /* j */


    INDIRECT(1); BRANCH(1);
    if (h_con->useParametricCoding) {

      ADD(1);
      m-=no_of_bands;

      PTR_INIT(3); /* pointers for h_sbr->sbrExtractEnvelope.envelopeCompensation[j]
                                   sfb_nrgLeft[m]
                                   sfb_nrgRight[m]
                   */
      LOOP(1);
      for (j = 0; j < no_of_bands; j++) {

        ADD(1); LOGIC(1); BRANCH(1);
        if (freq_res==FREQ_RES_HIGH && h_sbr->sbrExtractEnvelope.envelopeCompensation[j]){

          MISC(1); MULT(1); ADD(1); STORE(1);
          sfb_nrgLeft[m] -= (int)(ca * abs(h_sbr->sbrExtractEnvelope.envelopeCompensation[j]));
        }

        BRANCH(1);
        if(sfb_nrgLeft[m] < 0) {

          MOVE(1);
          sfb_nrgLeft[m] = 0;
        }
        m++;
      }
    }
  } /* i*/

  COUNT_sub_end();
}



/***************************************************************************/
/*!

  \brief  calculates the noise floor and the envelope values.

****************************************************************************/
void
extractSbrEnvelope (float *timeInPtr,
                    float *pCoreBuffer,
                    unsigned int timeInStride,
                    HANDLE_SBR_CONFIG_DATA h_con,
                    HANDLE_SBR_HEADER_DATA sbrHeaderData,
                    HANDLE_SBR_BITSTREAM_DATA sbrBitstreamData,
                    HANDLE_ENV_CHANNEL h_envChan[MAX_CHANNELS],
                    struct PS_ENC *hPsEnc,
                    HANDLE_SBR_QMF_FILTER_BANK hSynthesisQmfBank,
                    HANDLE_COMMON_DATA hCmonData)
{
  int ch, i, j, c;
  int nEnvelopes[MAX_CHANNELS];
  int transient_info[MAX_CHANNELS][2];

  const SBR_FRAME_INFO *frame_info[MAX_CHANNELS];

  int nChannels = h_con->nChannels;
  int nInChannels = (hPsEnc) ? 2:nChannels;

  SBR_STEREO_MODE stereoMode = h_con->stereoMode;

  FREQ_RES res[MAX_NUM_NOISE_VALUES];
  int v_tuning[6] = { 0, 2, 4, 0, 0, 0 };

  int   sfb_nrg    [MAX_CHANNELS][MAX_NUM_ENVELOPE_VALUES];

  float noiseFloor[MAX_CHANNELS][MAX_NUM_NOISE_VALUES];
  int   noise_level[MAX_CHANNELS][MAX_NUM_NOISE_VALUES];

#if (MAX_CHANNELS>1)
  int   sfb_nrg_coupling[MAX_CHANNELS][MAX_NUM_ENVELOPE_VALUES];
  int   noise_level_coupling[MAX_CHANNELS][MAX_NUM_NOISE_VALUES];
  int   maxQuantError;
#endif

  COUNT_sub_start("extractSbrEnvelope");

  BRANCH(1); MOVE(1); INDIRECT(2); MOVE(3);

  PTR_INIT(1); /* pointer for res[i] */
  LOOP(1);
  for(i=0; i<MAX_NUM_NOISE_VALUES; i++) {

    MOVE(1);
    res[i] = FREQ_RES_HIGH;
  }

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(noiseFloor));
  memset(noiseFloor,0,sizeof(noiseFloor));

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(transient_info));
  memset(transient_info,0,sizeof(transient_info));


  PTR_INIT(3); /* pointers for h_envChan[ch]->sbrExtractEnvelope,
                               h_envChan[ch]->sbrExtractEnvelope,
                               h_envChan[ch]->sbrQmf
               */
  LOOP(1);
  for(ch = 0; ch < nInChannels;ch++){

    BRANCH(1);
    if (hPsEnc) {
      BRANCH(1); PTR_INIT(1); FUNC(5);
      sbrAnalysisFiltering  (timeInPtr ? timeInPtr+ch:NULL,
                             MAX_CHANNELS,
                             h_envChan[ch]->sbrExtractEnvelope.rBuffer,
                             h_envChan[ch]->sbrExtractEnvelope.iBuffer,
                             &h_envChan[ch]->sbrQmf);
    }
    else
    {
      BRANCH(1); PTR_INIT(1); FUNC(5);
      sbrAnalysisFiltering  (timeInPtr ? timeInPtr+ch:NULL,
                             timeInStride,
                             h_envChan[ch]->sbrExtractEnvelope.rBuffer,
                             h_envChan[ch]->sbrExtractEnvelope.iBuffer,
                             &h_envChan[ch]->sbrQmf);
    }
  } /* ch */

#ifndef MONO_ONLY

  BRANCH(1); LOGIC(1);
  if (hPsEnc && hSynthesisQmfBank) {

    INDIRECT(2); FUNC(5);
    EncodePsFrame(hPsEnc,
                  h_envChan[0]->sbrExtractEnvelope.iBuffer,
                  h_envChan[0]->sbrExtractEnvelope.rBuffer,
                  h_envChan[1]->sbrExtractEnvelope.iBuffer,
                  h_envChan[1]->sbrExtractEnvelope.rBuffer);

    FUNC(4);
    SynthesisQmfFiltering (h_envChan[0]->sbrExtractEnvelope.rBuffer,
                           h_envChan[0]->sbrExtractEnvelope.iBuffer,
                           pCoreBuffer,
                           (HANDLE_SBR_QMF_FILTER_BANK)hSynthesisQmfBank);

    PTR_INIT(1); /* pointer for pCoreBuffer[] */
    INDIRECT(1); MULT(1); LOOP(1);
    for (i = 0; i < 1024; i++) {

      MULT(1); STORE(1);
      pCoreBuffer[i] = -pCoreBuffer[i];
    }
  }

#endif /* #ifndef MONO_ONLY */

  PTR_INIT(8); /* pointers for h_envChan[ch]->sbrExtractEnvelope,
                               h_envChan[ch]->sbrQmf,
                               h_envChan[ch]->TonCorr,
                               h_envChan[ch]->SbrEnvFrame,
                               h_envChan[ch]->sbrTransientDetector,
                               transient_info[ch],
                               h_con->freqBandTable[][],
                               h_con->nSfb[]
               */
  LOOP(1);
  for(ch = 0; ch < nChannels;ch++) {

    PTR_INIT(1); FUNC(5);
    getEnergyFromCplxQmfData (h_envChan[ch]->sbrExtractEnvelope.YBuffer + h_envChan[ch]->sbrExtractEnvelope.YBufferWriteOffset,
                              h_envChan[ch]->sbrExtractEnvelope.rBuffer,
                              h_envChan[ch]->sbrExtractEnvelope.iBuffer);




    INDIRECT(1); FUNC(4);
    CalculateTonalityQuotas(&h_envChan[ch]->TonCorr,
                            h_envChan[ch]->sbrExtractEnvelope.rBuffer,
                            h_envChan[ch]->sbrExtractEnvelope.iBuffer,
                            h_con->freqBandTable[HI][h_con->nSfb[HI]]);

    FUNC(4);
    transientDetect (h_envChan[ch]->sbrExtractEnvelope.YBuffer,
                     &h_envChan[ch]->sbrTransientDetector,
                     transient_info[ch],
                     h_envChan[ch]->sbrExtractEnvelope.time_step
                     );


    INDIRECT(2); FUNC(7);
    frameSplitter(h_envChan[ch]->sbrExtractEnvelope.YBuffer,
                  &h_envChan[ch]->sbrTransientDetector,
                  h_con->freqBandTable[1],
                  h_con->nSfb[1],
                  h_envChan[ch]->sbrExtractEnvelope.time_step,
                  h_envChan[ch]->sbrExtractEnvelope.no_cols,
                  transient_info[ch]);
  } /* ch */


#if (MAX_CHANNELS>1)
  ADD(1); BRANCH(1);
  if (stereoMode == SBR_COUPLING) {

    LOGIC(1); BRANCH(1);
    if (transient_info[0][1] && transient_info[1][1]) {

      ADD(1); BRANCH(1); MOVE(1);
      transient_info[0][0] =
        min (transient_info[1][0], transient_info[0][0]);

      MOVE(1);
      transient_info[1][0] = transient_info[0][0];
    }
    else {

      LOGIC(1); BRANCH(1);
      if (transient_info[0][1] && !transient_info[1][1]) {

        MOVE(1);
        transient_info[1][0] = transient_info[0][0];
      }
      else {

        LOGIC(1); BRANCH(1);
        if (!transient_info[0][1] && transient_info[1][1]) {

          MOVE(1);
          transient_info[0][0] = transient_info[1][0];
        }
        else {

          ADD(1); BRANCH(1); MOVE(1);
          transient_info[0][0] =
            max (transient_info[1][0], transient_info[0][0]);

          MOVE(1);
          transient_info[1][0] = transient_info[0][0];
        }
      }
    }
  }
#endif


  INDIRECT(1); FUNC(4); STORE(1);
  frame_info[0] = frameInfoGenerator (&h_envChan[0]->SbrEnvFrame,
                                      h_envChan[0]->sbrExtractEnvelope.pre_transient_info,
                                      transient_info[0],
                                      v_tuning);

  INDIRECT(1); PTR_INIT(1);
  h_envChan[0]->encEnvData.hSbrBSGrid = &h_envChan[0]->SbrEnvFrame.SbrGrid;


#if (MAX_CHANNELS>1)

  BRANCH(2);
  switch (stereoMode) {
  case SBR_LEFT_RIGHT:
  case SBR_SWITCH_LRC:

    INDIRECT(1); FUNC(4); STORE(1);
    frame_info[1] = frameInfoGenerator (&h_envChan[1]->SbrEnvFrame,
                                        h_envChan[1]->sbrExtractEnvelope.pre_transient_info,
                                        transient_info[1],
                                        v_tuning);

    INDIRECT(1); PTR_INIT(1);
    h_envChan[1]->encEnvData.hSbrBSGrid = &h_envChan[1]->SbrEnvFrame.SbrGrid;

    INDIRECT(2); ADD(1); BRANCH(1);
    if (frame_info[0]->nEnvelopes != frame_info[1]->nEnvelopes) {

      MOVE(1);
      stereoMode = SBR_LEFT_RIGHT;

    } else {

      PTR_INIT(2); /* pointer for frame_info[0]->borders[i],
                                  frame_info[1]->borders[i]
                   */
      INDIRECT(1); LOOP(1);
      for (i = 0; i < frame_info[0]->nEnvelopes + 1; i++) {

        ADD(1); BRANCH(1);
        if (frame_info[0]->borders[i] != frame_info[1]->borders[i]) {

          MOVE(1);
          stereoMode = SBR_LEFT_RIGHT;
          break;
        }
      }

      PTR_INIT(2); /* pointer for frame_info[0]->freqRes[i],
                                  frame_info[1]->freqRes[i]
                   */
      INDIRECT(1); LOOP(1);
      for (i = 0; i < frame_info[0]->nEnvelopes; i++) {

        ADD(1); BRANCH(1);
        if (frame_info[0]->freqRes[i] != frame_info[1]->freqRes[i]) {

          MOVE(1);
          stereoMode = SBR_LEFT_RIGHT;
          break;
        }
      }

      INDIRECT(2); ADD(1); BRANCH(1);
      if (frame_info[0]->shortEnv != frame_info[1]->shortEnv) {

        MOVE(1);
        stereoMode = SBR_LEFT_RIGHT;
      }
    }
    break;
  case SBR_COUPLING:

    MOVE(1);
    frame_info[1] = frame_info[0];

    INDIRECT(2); PTR_INIT(1);
    h_envChan[1]->encEnvData.hSbrBSGrid = &h_envChan[0]->SbrEnvFrame.SbrGrid;
    break;
  case SBR_MONO:
    /* nothing to do */
    break;
  default:
    assert (0);
  }

#endif /* (MAX_CHANNELS>1) */


  PTR_INIT(15); /* pointers for h_envChan[ch]->sbrExtractEnvelope,
                                h_envChan[ch]->encEnvData,
                                h_envChan[ch]->encEnvData.hSbrBSGrid->frameClass,
                                h_envChan[ch]->sbrQmf,
                                h_envChan[ch]->sbrCodeEnvelope,
                                h_envChan[ch]->sbrCodeNoiseFloor,
                                h_envChan[ch]->sbrExtractEnvelope,
                                h_envChan[ch]->TonCorr,
                                h_con->freqBandTable[][],
                                h_con->nSfb[],
                                frame_info[ch]->nEnvelopes,
                                frame_info[ch]->freqRes[i],
                                nEnvelopes[ch],
                                noiseFloor[ch],
                                transient_info[ch]
                */
  LOOP(1);
  for(ch = 0; ch < nChannels;ch++){

    MOVE(3);
    h_envChan[ch]->sbrExtractEnvelope.pre_transient_info[0] = transient_info[ch][0];
    h_envChan[ch]->sbrExtractEnvelope.pre_transient_info[1] = transient_info[ch][1];
    h_envChan[ch]->encEnvData.noOfEnvelopes = nEnvelopes[ch] = frame_info[ch]->nEnvelopes;


    LOOP(1);
    for (i = 0; i < nEnvelopes[ch]; i++){

      INDIRECT(1); ADD(1); BRANCH(1); MOVE(1);
      h_envChan[ch]->encEnvData.noScfBands[i] =
      (frame_info[ch]->freqRes[i] == FREQ_RES_HIGH ? h_con->nSfb[FREQ_RES_HIGH] : h_con->nSfb[FREQ_RES_LOW]);
    }


   ADD(2); LOGIC(1); BRANCH(1);
   if( ( h_envChan[ch]->encEnvData.hSbrBSGrid->frameClass == FIXFIX ) &&
        ( nEnvelopes[ch] == 1 ) ) {

      ADD(1); BRANCH(1);
      if( h_envChan[ch]->encEnvData.init_sbr_amp_res != SBR_AMP_RES_1_5 ) {

        FUNC(4);
        InitSbrHuffmanTables (&h_envChan[ch]->encEnvData,
                              &h_envChan[ch]->sbrCodeEnvelope,
                              &h_envChan[ch]->sbrCodeNoiseFloor,
                              SBR_AMP_RES_1_5);
      }
    }
    else {

      INDIRECT(1); ADD(1); BRANCH(1);
      if(sbrHeaderData->sbr_amp_res != h_envChan[ch]->encEnvData.init_sbr_amp_res ) {

        FUNC(4);
        InitSbrHuffmanTables (&h_envChan[ch]->encEnvData,
                              &h_envChan[ch]->sbrCodeEnvelope,
                              &h_envChan[ch]->sbrCodeNoiseFloor,
                              sbrHeaderData->sbr_amp_res);
      }
    }


    INDIRECT(1); FUNC(11);
    TonCorrParamExtr(&h_envChan[ch]->TonCorr,
                     h_envChan[ch]->encEnvData.sbr_invf_mode_vec,
                     noiseFloor[ch],
                     &h_envChan[ch]->encEnvData.addHarmonicFlag,
                     h_envChan[ch]->encEnvData.addHarmonic,
                     h_envChan[ch]->sbrExtractEnvelope.envelopeCompensation,
                     frame_info[ch],
                     transient_info[ch],
                     h_con->freqBandTable[HI],
                     h_con->nSfb[HI],
                     h_envChan[ch]->encEnvData.sbr_xpos_mode);

    MOVE(2);
    h_envChan[ch]->encEnvData.sbr_invf_mode = h_envChan[ch]->encEnvData.sbr_invf_mode_vec[0];
    h_envChan[ch]->encEnvData.noOfnoisebands = h_envChan[ch]->TonCorr.sbrNoiseFloorEstimate.noNoiseBands;




  } /* ch */


  BRANCH(2);
  switch (stereoMode) {

  case SBR_MONO:
    INDIRECT(1); FUNC(8);
    calculateSbrEnvelope (h_envChan[0]->sbrExtractEnvelope.YBuffer, NULL, frame_info[0], sfb_nrg[0],
                          NULL,h_con,h_envChan[0], SBR_MONO,NULL);
    break;

#if (MAX_CHANNELS>1)

  case SBR_LEFT_RIGHT:

    INDIRECT(1); FUNC(8);
    calculateSbrEnvelope (h_envChan[0]->sbrExtractEnvelope.YBuffer, NULL, frame_info[0], sfb_nrg[0],
                          NULL, h_con, h_envChan[0], SBR_MONO,NULL);

    INDIRECT(1); FUNC(8);
    calculateSbrEnvelope (h_envChan[1]->sbrExtractEnvelope.YBuffer, NULL, frame_info[1], sfb_nrg[1],
                          NULL, h_con,h_envChan[1], SBR_MONO,NULL);
    break;

  case SBR_COUPLING:
    INDIRECT(2); FUNC(8);
    calculateSbrEnvelope (h_envChan[0]->sbrExtractEnvelope.YBuffer, h_envChan[1]->sbrExtractEnvelope.YBuffer, frame_info[0],
                          sfb_nrg[0], sfb_nrg[1], h_con,h_envChan[0], SBR_COUPLING,&maxQuantError);

    break;

  case SBR_SWITCH_LRC:

    INDIRECT(1); FUNC(8);
    calculateSbrEnvelope (h_envChan[0]->sbrExtractEnvelope.YBuffer, NULL, frame_info[0], sfb_nrg[0],
                          NULL, h_con,h_envChan[0], SBR_MONO,NULL);

    INDIRECT(1); FUNC(8);
    calculateSbrEnvelope (h_envChan[1]->sbrExtractEnvelope.YBuffer, NULL, frame_info[1], sfb_nrg[1],
                          NULL, h_con,h_envChan[1], SBR_MONO,NULL);

    INDIRECT(2); FUNC(8);
    calculateSbrEnvelope (h_envChan[0]->sbrExtractEnvelope.YBuffer, h_envChan[1]->sbrExtractEnvelope.YBuffer, frame_info[0],
                          sfb_nrg_coupling[0], sfb_nrg_coupling[1], h_con,h_envChan[0], SBR_COUPLING,&maxQuantError);
    break;

#endif /* (MAX_CHANNELS>1) */

  default:
    assert(0);
  }



  BRANCH(2);
  switch (stereoMode) {

  case SBR_MONO:

    FUNC(3);
    sbrNoiseFloorLevelsQuantisation (noise_level[0], noiseFloor[0], 0);

    INDIRECT(4); ADD(1); BRANCH(1); FUNC(8);
    codeEnvelope (noise_level[0], res,
                  &h_envChan[0]->sbrCodeNoiseFloor,
                  h_envChan[0]->encEnvData.domain_vec_noise, 0,
                  (frame_info[0]->nEnvelopes > 1 ? 2 : 1), 0,
                 sbrBitstreamData->HeaderActive);

    break;

#if (MAX_CHANNELS>1)

  case SBR_LEFT_RIGHT:

    FUNC(3);
    sbrNoiseFloorLevelsQuantisation (noise_level[0],noiseFloor[0], 0);

    INDIRECT(4); ADD(1); BRANCH(1); FUNC(8);
    codeEnvelope (noise_level[0], res,
                  &h_envChan[0]->sbrCodeNoiseFloor,
                  h_envChan[0]->encEnvData.domain_vec_noise, 0,
                  (frame_info[0]->nEnvelopes > 1 ? 2 : 1), 0,
                  sbrBitstreamData->HeaderActive);


    FUNC(3);
    sbrNoiseFloorLevelsQuantisation (noise_level[1],noiseFloor[1], 0);

    INDIRECT(4); ADD(1); BRANCH(1); FUNC(8);
    codeEnvelope (noise_level[1], res,
                  &h_envChan[1]->sbrCodeNoiseFloor,
                  h_envChan[1]->encEnvData.domain_vec_noise, 0,
                  (frame_info[1]->nEnvelopes > 1 ? 2 : 1), 0,
                  sbrBitstreamData->HeaderActive);

    break;

  case SBR_COUPLING:

    FUNC(2);
    coupleNoiseFloor(noiseFloor[0],noiseFloor[1]);

    FUNC(3);
    sbrNoiseFloorLevelsQuantisation (noise_level[0],noiseFloor[0], 0);

    INDIRECT(4); ADD(1); BRANCH(1); FUNC(8);
    codeEnvelope (noise_level[0], res,
                  &h_envChan[0]->sbrCodeNoiseFloor,
                  h_envChan[0]->encEnvData.domain_vec_noise, 1,
                  (frame_info[0]->nEnvelopes > 1 ? 2 : 1), 0,
                  sbrBitstreamData->HeaderActive);


    FUNC(3);
    sbrNoiseFloorLevelsQuantisation (noise_level[1],noiseFloor[1], 1);

    INDIRECT(4); ADD(1); BRANCH(1); FUNC(8);
    codeEnvelope (noise_level[1], res,
                  &h_envChan[1]->sbrCodeNoiseFloor,
                  h_envChan[1]->encEnvData.domain_vec_noise, 1,
                  (frame_info[1]->nEnvelopes > 1 ? 2 : 1), 1,
                  sbrBitstreamData->HeaderActive);



    break;

  case SBR_SWITCH_LRC:

    FUNC(3);
    sbrNoiseFloorLevelsQuantisation (noise_level[0],noiseFloor[0], 0);

    FUNC(3);
    sbrNoiseFloorLevelsQuantisation (noise_level[1],noiseFloor[1], 0);

    FUNC(2);
    coupleNoiseFloor(noiseFloor[0],noiseFloor[1]);

    FUNC(3);
    sbrNoiseFloorLevelsQuantisation (noise_level_coupling[0],noiseFloor[0], 0);

    FUNC(3);
    sbrNoiseFloorLevelsQuantisation (noise_level_coupling[1],noiseFloor[1], 1);
    break;


#endif /* (MAX_CHANNELS>1) */

  default:
    assert(0);

  }


  BRANCH(2);
  switch (stereoMode) {

  case SBR_MONO:

    INDIRECT(2); MOVE(2);
    sbrHeaderData->coupling = 0;
    h_envChan[0]->encEnvData.balance = 0;

    INDIRECT(6); FUNC(8);
    codeEnvelope (sfb_nrg[0], frame_info[0]->freqRes,
                  &h_envChan[0]->sbrCodeEnvelope,
                  h_envChan[0]->encEnvData.domain_vec,
                  sbrHeaderData->coupling,
                  frame_info[0]->nEnvelopes, 0,
                  sbrBitstreamData->HeaderActive);
    break;

#if (MAX_CHANNELS>1)

  case SBR_LEFT_RIGHT:

    INDIRECT(1); MOVE(1);
    sbrHeaderData->coupling = 0;

    INDIRECT(2); MOVE(2);
    h_envChan[0]->encEnvData.balance = 0;
    h_envChan[1]->encEnvData.balance = 0;


    INDIRECT(6); FUNC(8);
    codeEnvelope (sfb_nrg[0], frame_info[0]->freqRes,
                  &h_envChan[0]->sbrCodeEnvelope,
                  h_envChan[0]->encEnvData.domain_vec,
                  sbrHeaderData->coupling,
                  frame_info[0]->nEnvelopes, 0,
                  sbrBitstreamData->HeaderActive);

    INDIRECT(6); FUNC(8);
    codeEnvelope (sfb_nrg[1], frame_info[1]->freqRes,
                  &h_envChan[1]->sbrCodeEnvelope,
                  h_envChan[1]->encEnvData.domain_vec,
                  sbrHeaderData->coupling,
                  frame_info[1]->nEnvelopes, 0,
                  sbrBitstreamData->HeaderActive);
    break;

  case SBR_COUPLING:

    INDIRECT(3); MOVE(3);
    sbrHeaderData->coupling = 1;
    h_envChan[0]->encEnvData.balance = 0;
    h_envChan[1]->encEnvData.balance = 1;

    INDIRECT(6); FUNC(8);
    codeEnvelope (sfb_nrg[0], frame_info[0]->freqRes,
                  &h_envChan[0]->sbrCodeEnvelope,
                  h_envChan[0]->encEnvData.domain_vec,
                  sbrHeaderData->coupling,
                  frame_info[0]->nEnvelopes, 0,
                  sbrBitstreamData->HeaderActive);

    INDIRECT(6); FUNC(8);
    codeEnvelope (sfb_nrg[1], frame_info[1]->freqRes,
                  &h_envChan[1]->sbrCodeEnvelope,
                  h_envChan[1]->encEnvData.domain_vec,
                  sbrHeaderData->coupling,
                  frame_info[1]->nEnvelopes, 1,
                  sbrBitstreamData->HeaderActive);
    break;

  case SBR_SWITCH_LRC:
    {
      int payloadbitsLR;
      int payloadbitsCOUPLING;

      int sfbNrgPrevTemp[MAX_CHANNELS][MAX_FREQ_COEFFS];
      int noisePrevTemp[MAX_CHANNELS][MAX_NUM_NOISE_COEFFS];
      int upDateNrgTemp[MAX_CHANNELS];
      int upDateNoiseTemp[MAX_CHANNELS];
      int domainVecTemp[MAX_CHANNELS][MAX_ENVELOPES];
      int domainVecNoiseTemp[MAX_CHANNELS][MAX_ENVELOPES];

      int tempFlagRight = 0;
      int tempFlagLeft = 0;


      MOVE(2); /* counting previous operations */


      PTR_INIT(6); /* pointers for h_envChan[ch]->sbrCodeEnvelope,
                                   h_envChan[ch]->sbrCodeNoiseFloor,
                                   sfbNrgPrevTemp[ch],
                                   noisePrevTemp[ch],
                                   upDateNrgTemp[ch],
                                   upDateNoiseTemp[ch]
                   */
      LOOP(1);
      for(ch = 0; ch < nChannels;ch++){

        FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(MAX_FREQ_COEFFS);
        memcpy (sfbNrgPrevTemp[ch], h_envChan[ch]->sbrCodeEnvelope.sfb_nrg_prev,
              MAX_FREQ_COEFFS * sizeof (int));

        FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(MAX_NUM_NOISE_COEFFS);
        memcpy (noisePrevTemp[ch], h_envChan[ch]->sbrCodeNoiseFloor.sfb_nrg_prev,
              MAX_NUM_NOISE_COEFFS * sizeof (int));

        MOVE(2);
        upDateNrgTemp[ch] = h_envChan[ch]->sbrCodeEnvelope.upDate;
        upDateNoiseTemp[ch] = h_envChan[ch]->sbrCodeNoiseFloor.upDate;

        INDIRECT(1); BRANCH(1);
        if(sbrHeaderData->prev_coupling){

          MOVE(2);
          h_envChan[ch]->sbrCodeEnvelope.upDate = 0;
          h_envChan[ch]->sbrCodeNoiseFloor.upDate = 0;
        }
      } /* ch */


      INDIRECT(5); FUNC(8);
      codeEnvelope (sfb_nrg[0], frame_info[0]->freqRes,
                    &h_envChan[0]->sbrCodeEnvelope,
                    h_envChan[0]->encEnvData.domain_vec, 0,
                    frame_info[0]->nEnvelopes, 0,
                    sbrBitstreamData->HeaderActive);

      INDIRECT(5); FUNC(8);
      codeEnvelope (sfb_nrg[1], frame_info[1]->freqRes,
                    &h_envChan[1]->sbrCodeEnvelope,
                    h_envChan[1]->encEnvData.domain_vec, 0,
                    frame_info[1]->nEnvelopes, 0,
                    sbrBitstreamData->HeaderActive);

      c = 0;

      PTR_INIT(5); /* pointers for h_envChan[0]->encEnvData.noScfBands[i]
                                   h_envChan[0]->encEnvData.ienvelope[i][j]
                                   h_envChan[1]->encEnvData.ienvelope[i][j]
                                   sfb_nrg[0][c]
                                   sfb_nrg[1][c]
                   */
      LOOP(1);
      for (i = 0; i < nEnvelopes[0]; i++) {

        LOOP(1);
        for (j = 0; j < h_envChan[0]->encEnvData.noScfBands[i]; j++)
          {
            MOVE(2);
            h_envChan[0]->encEnvData.ienvelope[i][j] = sfb_nrg[0][c];
            h_envChan[1]->encEnvData.ienvelope[i][j] = sfb_nrg[1][c];

            c++;
          }
      }



      INDIRECT(4); ADD(1); BRANCH(1); FUNC(8);
      codeEnvelope (noise_level[0], res,
                    &h_envChan[0]->sbrCodeNoiseFloor,
                    h_envChan[0]->encEnvData.domain_vec_noise, 0,
                    (frame_info[0]->nEnvelopes > 1 ? 2 : 1), 0,
                    sbrBitstreamData->HeaderActive);


      PTR_INIT(2); /* pointers for h_envChan[0]->encEnvData.sbr_noise_levels[i],
                                   noise_level[0][i]
                   */
      LOOP(1);
      for (i = 0; i < MAX_NUM_NOISE_VALUES; i++) {

        MOVE(1);
        h_envChan[0]->encEnvData.sbr_noise_levels[i] = noise_level[0][i];
      }


      INDIRECT(4); ADD(1); BRANCH(1); FUNC(8);
      codeEnvelope (noise_level[1], res,
                    &h_envChan[1]->sbrCodeNoiseFloor,
                    h_envChan[1]->encEnvData.domain_vec_noise, 0,
                    (frame_info[1]->nEnvelopes > 1 ? 2 : 1), 0,
                    sbrBitstreamData->HeaderActive);

      PTR_INIT(2); /* pointers for h_envChan[1]->encEnvData.sbr_noise_levels[i],
                                   noise_level[1][i]
                   */
      LOOP(1);
      for (i = 0; i < MAX_NUM_NOISE_VALUES; i++) {

        MOVE(1);
        h_envChan[1]->encEnvData.sbr_noise_levels[i] = noise_level[1][i];
      }


      INDIRECT(3); MOVE(3);
      sbrHeaderData->coupling = 0;
      h_envChan[0]->encEnvData.balance = 0;
      h_envChan[1]->encEnvData.balance = 0;

      INDIRECT(2); FUNC(5);
      payloadbitsLR = CountSbrChannelPairElement (sbrHeaderData,
                                                  sbrBitstreamData,
                                                  &h_envChan[0]->encEnvData,
                                                  &h_envChan[1]->encEnvData,
                                                  hCmonData);


      PTR_INIT(8); /* pointers for h_envChan[ch]->sbrCodeEnvelope,
                                   h_envChan[ch]->sbrCodeNoiseFloor,
                                   h_envChan[ch]->encEnvData.domain_vec,
                                   h_envChan[ch]->encEnvData.domain_vec_noise,
                                   upDateNrgTemp[ch],
                                   upDateNoiseTemp[ch],
                                   domainVecTemp[ch],
                                   domainVecNoiseTemp[ch]
                   */
      LOOP(1);
      for(ch = 0; ch < nChannels;ch++){
        int   itmp;

        PTR_INIT(2); /* pointers for h_envChan[ch]->sbrCodeEnvelope.sfb_nrg_prev[i],
                                     sfbNrgPrevTemp[ch][i]
                     */
        LOOP(1);
        for(i=0;i<MAX_FREQ_COEFFS;i++){


          INDIRECT(1); MOVE(3);
          itmp =  h_envChan[ch]->sbrCodeEnvelope.sfb_nrg_prev[i];
          h_envChan[ch]->sbrCodeEnvelope.sfb_nrg_prev[i]=sfbNrgPrevTemp[ch][i];
          sfbNrgPrevTemp[ch][i]=itmp;
        }

        PTR_INIT(2); /* pointers for h_envChan[ch]->sbrCodeNoiseFloor.sfb_nrg_prev[i],
                                     noisePrevTemp[ch][i]
                     */
        LOOP(1);
        for(i=0;i<MAX_NUM_NOISE_COEFFS;i++){


          INDIRECT(1); MOVE(3);
          itmp =  h_envChan[ch]->sbrCodeNoiseFloor.sfb_nrg_prev[i];
          h_envChan[ch]->sbrCodeNoiseFloor.sfb_nrg_prev[i]=noisePrevTemp[ch][i];
          noisePrevTemp[ch][i]=itmp;
       }

        INDIRECT(1); MOVE(3);
        itmp  = h_envChan[ch]->sbrCodeEnvelope.upDate;
        h_envChan[ch]->sbrCodeEnvelope.upDate=upDateNrgTemp[ch];
        upDateNrgTemp[ch] = itmp;

        INDIRECT(1); MOVE(3);
        itmp =  h_envChan[ch]->sbrCodeNoiseFloor.upDate;
        h_envChan[ch]->sbrCodeNoiseFloor.upDate=upDateNoiseTemp[ch];
        upDateNoiseTemp[ch]=itmp;

        FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(MAX_ENVELOPES);
        memcpy(domainVecTemp[ch],h_envChan[ch]->encEnvData.domain_vec,sizeof(int)*MAX_ENVELOPES);

        FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(MAX_ENVELOPES);
        memcpy(domainVecNoiseTemp[ch],h_envChan[ch]->encEnvData.domain_vec_noise,sizeof(int)*MAX_ENVELOPES);


        INDIRECT(1); BRANCH(1);
        if(!sbrHeaderData->prev_coupling){

          MOVE(2);
          h_envChan[ch]->sbrCodeEnvelope.upDate = 0;
          h_envChan[ch]->sbrCodeNoiseFloor.upDate = 0;
        }
      } /* ch */



      INDIRECT(5); FUNC(8);
      codeEnvelope (sfb_nrg_coupling[0], frame_info[0]->freqRes,
                    &h_envChan[0]->sbrCodeEnvelope,
                    h_envChan[0]->encEnvData.domain_vec, 1,
                    frame_info[0]->nEnvelopes, 0,
                    sbrBitstreamData->HeaderActive);

      INDIRECT(5); FUNC(8);
      codeEnvelope (sfb_nrg_coupling[1], frame_info[1]->freqRes,
                    &h_envChan[1]->sbrCodeEnvelope,
                    h_envChan[1]->encEnvData.domain_vec, 1,
                    frame_info[1]->nEnvelopes, 1,
                    sbrBitstreamData->HeaderActive);


      c = 0;

      PTR_INIT(5); /* pointers for h_envChan[0]->encEnvData.noScfBands[i]
                                   h_envChan[0]->encEnvData.ienvelope[i][j]
                                   h_envChan[1]->encEnvData.ienvelope[i][j]
                                   sfb_nrg_coupling[0][c]
                                   sfb_nrg_coupling[1][c]
                   */
      LOOP(1);
      for (i = 0; i < nEnvelopes[0]; i++) {

        LOOP(1);
        for (j = 0; j < h_envChan[0]->encEnvData.noScfBands[i]; j++) {

          MOVE(2);
          h_envChan[0]->encEnvData.ienvelope[i][j] = sfb_nrg_coupling[0][c];
          h_envChan[1]->encEnvData.ienvelope[i][j] = sfb_nrg_coupling[1][c];

          c++;
        }
      }



      INDIRECT(4); ADD(1); BRANCH(1); FUNC(8);
      codeEnvelope (noise_level_coupling[0], res,
                    &h_envChan[0]->sbrCodeNoiseFloor,
                    h_envChan[0]->encEnvData.domain_vec_noise, 1,
                    (frame_info[0]->nEnvelopes > 1 ? 2 : 1), 0,
                     sbrBitstreamData->HeaderActive);

      PTR_INIT(2); /* pointers for h_envChan[0]->encEnvData.sbr_noise_levels[i],
                                   noise_level_coupling[0][i]
                   */
      LOOP(1);
      for (i = 0; i < MAX_NUM_NOISE_VALUES; i++) {

        MOVE(1);
        h_envChan[0]->encEnvData.sbr_noise_levels[i] = noise_level_coupling[0][i];
      }


      INDIRECT(4); ADD(1); BRANCH(1); FUNC(8);
      codeEnvelope (noise_level_coupling[1], res,
                    &h_envChan[1]->sbrCodeNoiseFloor,
                    h_envChan[1]->encEnvData.domain_vec_noise, 1,
                    (frame_info[1]->nEnvelopes > 1 ? 2 : 1), 1,
                    sbrBitstreamData->HeaderActive);

      PTR_INIT(2); /* pointers for h_envChan[1]->encEnvData.sbr_noise_levels[i],
                                   noise_level_coupling[1][i]
                   */
      LOOP(1);
      for (i = 0; i < MAX_NUM_NOISE_VALUES; i++) {

        MOVE(1);
        h_envChan[1]->encEnvData.sbr_noise_levels[i] = noise_level_coupling[1][i];
      }

      INDIRECT(1); MOVE(1);
      sbrHeaderData->coupling = 1;

      INDIRECT(2); MOVE(2);
      h_envChan[0]->encEnvData.balance  = 0;
      h_envChan[1]->encEnvData.balance  = 1;

      INDIRECT(2); MOVE(2);
      tempFlagLeft  = h_envChan[0]->encEnvData.addHarmonicFlag;
      tempFlagRight = h_envChan[1]->encEnvData.addHarmonicFlag;

      INDIRECT(2); PTR_INIT(2); FUNC(5);
      payloadbitsCOUPLING =
        CountSbrChannelPairElement (sbrHeaderData,
                                    sbrBitstreamData,
                                    &h_envChan[0]->encEnvData,
                                    &h_envChan[1]->encEnvData,
                                    hCmonData);

      INDIRECT(2); MOVE(2);
      h_envChan[0]->encEnvData.addHarmonicFlag = tempFlagLeft;
      h_envChan[1]->encEnvData.addHarmonicFlag = tempFlagRight;

      ADD(1); BRANCH(1);
      if (payloadbitsCOUPLING < payloadbitsLR) {

          LOOP(1);
          for(ch = 0; ch < nChannels;ch++){

            FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(MAX_NUM_ENVELOPE_VALUES);
            memcpy (sfb_nrg[ch], sfb_nrg_coupling[ch],
                  MAX_NUM_ENVELOPE_VALUES * sizeof (int));

            FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(MAX_NUM_NOISE_VALUES);
            memcpy (noise_level[ch], noise_level_coupling[ch],
                  MAX_NUM_NOISE_VALUES * sizeof (int));
          }

          INDIRECT(3); MOVE(3);
          sbrHeaderData->coupling = 1;
          h_envChan[0]->encEnvData.balance  = 0;
          h_envChan[1]->encEnvData.balance  = 1;
      }
      else{
          PTR_INIT(4); /* pointer for h_envChan[ch]->sbrCodeEnvelope.upDate,
                                      h_envChan[ch]->sbrCodeNoiseFloor.upDate,
                                      upDateNrgTemp[ch],
                                      upDateNoiseTemp[ch]
                       */
          LOOP(1);
          for(ch = 0; ch < nChannels;ch++){

            FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(MAX_FREQ_COEFFS);
            memcpy (h_envChan[ch]->sbrCodeEnvelope.sfb_nrg_prev,
                    sfbNrgPrevTemp[ch], MAX_FREQ_COEFFS * sizeof (int));

            MOVE(1);
            h_envChan[ch]->sbrCodeEnvelope.upDate = upDateNrgTemp[ch];

            FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(MAX_FREQ_COEFFS);
            memcpy (h_envChan[ch]->sbrCodeNoiseFloor.sfb_nrg_prev,
                    noisePrevTemp[ch], MAX_NUM_NOISE_COEFFS * sizeof (int));

            FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(MAX_ENVELOPES);
            memcpy(h_envChan[ch]->encEnvData.domain_vec,domainVecTemp[ch],sizeof(int)*MAX_ENVELOPES);

            FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(MAX_ENVELOPES);
            memcpy(h_envChan[ch]->encEnvData.domain_vec_noise,domainVecNoiseTemp[ch],sizeof(int)*MAX_ENVELOPES);

            MOVE(1);
            h_envChan[ch]->sbrCodeNoiseFloor.upDate = upDateNoiseTemp[ch];
          }

          INDIRECT(3); MOVE(3);
          sbrHeaderData->coupling = 0;
          h_envChan[0]->encEnvData.balance  = 0;
          h_envChan[1]->encEnvData.balance  = 0;
        }
    }
    break;

#endif /* (MAX_CHANNELS>1) */

  default:
    assert(0);

  } /* end switch(stereoMode) */

  ADD(1); BRANCH(1);
  if (nChannels == 1) {

    INDIRECT(1); ADD(1); BRANCH(1);
    if (h_envChan[0]->encEnvData.domain_vec[0] == TIME) {

      INDIRECT(1); ADD(1);
      h_envChan[0]->sbrCodeEnvelope.dF_edge_incr_fac++;
    }
    else {

      INDIRECT(1); MOVE(1);
      h_envChan[0]->sbrCodeEnvelope.dF_edge_incr_fac = 0;
    }
  }
  else {

    INDIRECT(2); ADD(2); LOGIC(1); BRANCH(1);
    if (h_envChan[0]->encEnvData.domain_vec[0] == TIME ||
        h_envChan[1]->encEnvData.domain_vec[0] == TIME) {

      INDIRECT(2); ADD(2);
      h_envChan[0]->sbrCodeEnvelope.dF_edge_incr_fac++;
      h_envChan[1]->sbrCodeEnvelope.dF_edge_incr_fac++;
    }
    else {

      INDIRECT(2); MOVE(2);
      h_envChan[0]->sbrCodeEnvelope.dF_edge_incr_fac = 0;
      h_envChan[1]->sbrCodeEnvelope.dF_edge_incr_fac = 0;
    }
  }


  PTR_INIT(5); /* pointers for h_envChan[ch]->encEnvData.noScfBands[i],
                               h_envChan[ch]->encEnvData.ienvelope[i][j],
                               h_envChan[ch]->encEnvData.sbr_noise_levels[i],
                               sfb_nrg[ch][c]
                               noise_level[ch][i]
               */
  LOOP(1);
  for(ch = 0; ch < nChannels;ch++){
    c = 0;

    LOOP(1);
    for (i = 0; i < nEnvelopes[ch]; i++) {

      LOOP(1);
      for (j = 0; j < h_envChan[ch]->encEnvData.noScfBands[i]; j++) {

        MOVE(1);
        h_envChan[ch]->encEnvData.ienvelope[i][j] = sfb_nrg[ch][c];

        c++;
      }
    }

    LOOP(1);
    for (i = 0; i < MAX_NUM_NOISE_VALUES; i++){

      MOVE(1);
      h_envChan[ch]->encEnvData.sbr_noise_levels[i] = noise_level[ch][i];

    }

  }/* ch */

  ADD(1); BRANCH(1);
  if (nChannels == 2) {

    INDIRECT(2); PTR_INIT(2); FUNC(5);
    WriteEnvChannelPairElement(sbrHeaderData,
                               sbrBitstreamData,
                               &h_envChan[0]->encEnvData,
                               &h_envChan[1]->encEnvData,
                               hCmonData);
  }
  else {

    INDIRECT(1); PTR_INIT(1); FUNC(5);
    WriteEnvSingleChannelElement(sbrHeaderData,
                                 sbrBitstreamData,
                                 &h_envChan[0]->encEnvData,
                                 hPsEnc,
                                 hCmonData);
  }


  PTR_INIT(3); /* pointers for h_envChan[ch]->sbrExtractEnvelope.YBufferWriteOffset,
                               h_envChan[ch]->sbrExtractEnvelope,
                               h_envChan[ch]->sbrExtractEnvelope.YBuffer[]
               */
  LOOP(1);
  for(ch = 0; ch < nChannels;ch++){

    LOOP(1);
    for (i = 0; i < h_envChan[ch]->sbrExtractEnvelope.YBufferWriteOffset; i++) {
      float *temp;

      temp = h_envChan[ch]->sbrExtractEnvelope.YBuffer[i];

      MOVE(2);
      h_envChan[ch]->sbrExtractEnvelope.YBuffer[i] = h_envChan[ch]->sbrExtractEnvelope.YBuffer[i + h_envChan[ch]->sbrExtractEnvelope.no_cols/2];
      h_envChan[ch]->sbrExtractEnvelope.YBuffer[i + h_envChan[ch]->sbrExtractEnvelope.no_cols/2] = temp;
    }
  }/* ch */

  INDIRECT(2); MOVE(1);
  sbrHeaderData->prev_coupling = sbrHeaderData->coupling;

  COUNT_sub_end();
}



/***************************************************************************/
/*!

  \brief  creates an envelope extractor handle

  \return error status

****************************************************************************/
int
CreateExtractSbrEnvelope (int chan,
                          HANDLE_SBR_EXTRACT_ENVELOPE  hSbrCut,
                          int start_index)
{
  int i;
  int YBufferLength, rBufferLength;

  COUNT_sub_start("CreateExtractSbrEnvelope");

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(SBR_EXTRACT_ENVELOPE));
  memset(hSbrCut,0,sizeof(SBR_EXTRACT_ENVELOPE));

  INDIRECT(1); MOVE(1);
  hSbrCut->YBufferWriteOffset = 32;

  INDIRECT(1); ADD(1);
  YBufferLength = hSbrCut->YBufferWriteOffset + 32;

  MOVE(1);
  rBufferLength = 32;

  INDIRECT(2); MOVE(2);
  hSbrCut->pre_transient_info[0] = 0;
  hSbrCut->pre_transient_info[1] = 0;

  INDIRECT(3); MOVE(3);
  hSbrCut->no_cols = 32;
  hSbrCut->no_rows = 64;
  hSbrCut->start_index = start_index;

  INDIRECT(2); MOVE(2);
  hSbrCut->time_slots = 16;
  hSbrCut->time_step = 2;

  assert(rBufferLength  ==   QMF_TIME_SLOTS);
  assert(YBufferLength  == 2*QMF_TIME_SLOTS);


  DIV(1);
  YBufferLength /= 2;

  INDIRECT(1); DIV(1); STORE(1);
  hSbrCut->YBufferWriteOffset /= 2;

  PTR_INIT(1); /* hSbrCut->YBuffer[] */
  MULT(2); LOOP(1);
  for (i = 0; i < YBufferLength; i++) {
    INDIRECT(1); ADD(1); PTR_INIT(1);
    hSbrCut->YBuffer[i] = &sbr_envYBuffer[chan*YBufferLength*64 + i*QMF_CHANNELS];
  }

  PTR_INIT(2); /* hSbrCut->rBuffer[]
                  hSbrCut->iBuffer[]
               */
  MULT(1); LOOP(1);
  for (i = 0; i < rBufferLength; i++) {
    INDIRECT(1); ADD(1); PTR_INIT(1);
    hSbrCut->rBuffer[i] = &sbr_envRBuffer[chan*QMF_TIME_SLOTS*QMF_CHANNELS + i*QMF_CHANNELS];

    INDIRECT(1); ADD(1); PTR_INIT(1);
    hSbrCut->iBuffer[i] = &sbr_envIBuffer[chan*QMF_TIME_SLOTS*QMF_CHANNELS + i*QMF_CHANNELS];
  }


  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(MAX_FREQ_COEFFS);
  memset(hSbrCut->envelopeCompensation,0,sizeof(char)*MAX_FREQ_COEFFS);


  COUNT_sub_end();

  return 0;
}




/***************************************************************************/
/*!

  \brief  deinitializes an envelope extractor handle

  \return void

****************************************************************************/
void
deleteExtractSbrEnvelope (HANDLE_SBR_EXTRACT_ENVELOPE hSbrCut)
{

  COUNT_sub_start("deleteExtractSbrEnvelope");

  BRANCH(1);
  if (hSbrCut) {


  }

  COUNT_sub_end();
}



