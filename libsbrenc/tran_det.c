/*
  Transient detector
*/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "FloatFR.h"
#include "tran_det.h"
#include "sbr_def.h"
#include "fram_gen.h"
#include "sbr_ram.h"

#include "counters.h" /* the 3GPP instrumenting tool */



static const float ABS_THRES=1.28e5;
static const float STD_FAC=1.0f;


/*******************************************************************************
 Functionname:  spectralChange
 *******************************************************************************
 \author  Klaus Peichl
 \brief   Calculates a measure for the spectral change within the frame

 \return  calculated value
*******************************************************************************/
static float spectralChange(float Energies[16][MAX_FREQ_COEFFS],
                            float EnergyTotal,
                            int nSfb,
                            int start,
                            int border,
                            int stop)
{
  int i,j;
  int len1 = border-start;
  int len2 = stop-border;
  float lenRatio = (float)len1 / (float)len2;
  float delta;
  float delta_sum= 0.0f;
  float nrg1[MAX_FREQ_COEFFS];
  float nrg2[MAX_FREQ_COEFFS];


  float pos_weight = (0.5f - (float)len1 / (float)(len1+len2));

  COUNT_sub_start("spectralChange");

  ADD(4); DIV(2); MOVE(1); /* counting previous operations */

  MULT(2); ADD(1);
  pos_weight = 1.0f - 4.0f * pos_weight * pos_weight;


  PTR_INIT(2); /* pointer for nrg1[j],
                              nrg2[j]
               */
  LOOP(1);
  for ( j=0; j<nSfb; j++ ) {


    MULT(2); STORE(2);
    nrg1[j] = 1.0e6f * len1;
    nrg2[j] = 1.0e6f * len2;

    PTR_INIT(1); /* pointer for Energies[i][j] */
    LOOP(1);
    for (i=start; i<border; i++) {

      ADD(1); STORE(1);
      nrg1[j] += Energies[i][j];
    }

    PTR_INIT(1); /* pointer for Energies[i][j] */
    LOOP(1);
    for (i=border; i<stop; i++) {

      ADD(1); STORE(1);
      nrg2[j] += Energies[i][j];
    }
  }

  PTR_INIT(2); /* pointer for nrg1[j],
                              nrg2[j]
               */
  LOOP(1);
  for ( j=0; j<nSfb; j++ ) {

    DIV(1); MULT(1); TRANS(1); MISC(1);
    delta = (float) fabs( log( nrg2[j] / nrg1[j] * lenRatio) );

    ADD(2); DIV(1); TRANS(1); MULT(1);
    delta_sum += (float) (sqrt( (nrg1[j] + nrg2[j]) / EnergyTotal) * delta);
  }

  MULT(1); /* counting post-operations */

  COUNT_sub_end();

  return delta_sum * pos_weight;
}


/*******************************************************************************
 Functionname:  addLowbandEnergies
 *******************************************************************************
 \author  Klaus Peichl
 \brief   Calculates total lowband energy
 \return  total energy in the lowband
*******************************************************************************/
static float addLowbandEnergies(float **Energies,
                                unsigned char * freqBandTable,
                                int nSfb,
                                int slots)
{
  int i,k;
  float nrgTotal=1.0f;

  COUNT_sub_start("addLowbandEnergies");

  MOVE(1); /* counting previous operations */

  SHIFT(1); LOOP(1);
  for (k = 0; k < freqBandTable[0]; k++) {

    PTR_INIT(1); /* pointer for Energies[][] */
    LOOP(1);
    for (i=0; i<slots; i++) {

        ADD(1);
        nrgTotal += Energies[(i+slots/2)/2][k];
    }
  }

  COUNT_sub_end();

  return(nrgTotal);
}


/*******************************************************************************
 Functionname:  addHighbandEnergies
 *******************************************************************************
 \author  Klaus Peichl
 \brief   Add highband energies
 \return  total energy in the highband
*******************************************************************************/
static float addHighbandEnergies(float **Energies, /*!< input */
                                 float EnergiesM[16][MAX_FREQ_COEFFS], /*!< Combined output */
                                 unsigned char * freqBandTable,
                                 int nSfb,
                                 int sbrSlots,
                                 int timeStep)
{
  int i,j,k,slotIn,slotOut;
  int li,ui;
  float nrgTotal=1.0;

  COUNT_sub_start("addHighbandEnergies");

  MOVE(1); /* counting previous operations */

  LOOP(1);
  for (slotOut=0; slotOut<sbrSlots; slotOut++) {

    SHIFT(1);
    slotIn = 2*slotOut;

    PTR_INIT(2); /* pointers for Energies[][],
                                 freqBandTable[]
                 */
    LOOP(1);
    for ( j=0; j<nSfb; j++ ) {

      MOVE(1);
      EnergiesM[slotOut][j] = 0;

      MOVE(2);
      li = freqBandTable[j];
      ui = freqBandTable[j + 1];

      LOOP(1);
      for (k = li; k < ui; k++) {

        PTR_INIT(1); /* pointers for Energies[][] */
        LOOP(1);
        for (i=0; i<timeStep; i++) {

         ADD(1); STORE(1);
         EnergiesM[slotOut][j] += Energies[(slotIn+i)/2][k];
        }
      }
    }
  }

  LOOP(1);
  for (slotOut=0; slotOut<sbrSlots; slotOut++) {

    PTR_INIT(1); /* pointers for Energies[][] */
    LOOP(1);
    for (j=0; j<nSfb; j++) {

      ADD(1);
      nrgTotal += EnergiesM[slotOut][j];
    }
  }

  COUNT_sub_end();

  return(nrgTotal);
}


/*******************************************************************************
 Functionname:  frameSplitter
 *******************************************************************************
 \author  Klaus Peichl
 \brief   Decides if a frame shall be splitted into 2 envelopes
*******************************************************************************/
void
frameSplitter(float **Energies,
              HANDLE_SBR_TRANSIENT_DETECTOR h_sbrTransientDetector,
              unsigned char * freqBandTable,
              int nSfb,
              int timeStep,
              int no_cols,
              int *tran_vector)
{
  COUNT_sub_start("frameSplitter");

  BRANCH(1);
  if (tran_vector[1]==0) { /* no transient was detected */
    float delta;
    float EnergiesM[16][MAX_FREQ_COEFFS];
    float EnergyTotal, newLowbandEnergy;
    int border;
    int sbrSlots = no_cols / timeStep;

    assert( sbrSlots * timeStep == no_cols );

    DIV(1); /* counting previous operations */

    FUNC(4);
    newLowbandEnergy = addLowbandEnergies(Energies,
                                          freqBandTable,
                                          nSfb,
                                          no_cols);

    INDIRECT(1); ADD(1); MULT(1);
    EnergyTotal = 0.5f * (newLowbandEnergy + h_sbrTransientDetector->prevLowBandEnergy);

    INDIRECT(1); FUNC(6); STORE(1);
    h_sbrTransientDetector->totalHighBandEnergy = addHighbandEnergies( Energies,
                                                  EnergiesM,
                                                  freqBandTable,
                                                  nSfb,
                                                  sbrSlots,
                                                  timeStep);

    INDIRECT(1); ADD(1);
    EnergyTotal += h_sbrTransientDetector->totalHighBandEnergy;

    INDIRECT(1); MULT(1); DIV(1); STORE(1);
    h_sbrTransientDetector->totalHighBandEnergy /= (sbrSlots*nSfb);

    ADD(1); SHIFT(1);
    border = (sbrSlots+1) / 2;

    FUNC(6);
    delta = spectralChange( EnergiesM,
                            EnergyTotal,
                            nSfb,
                            0,
                            border,
                            sbrSlots);

    INDIRECT(1); ADD(1); BRANCH(1); MOVE(1);
    if (delta > h_sbrTransientDetector->split_thr)
      tran_vector[0] = 1; /* Set flag for splitting */
    else
      tran_vector[0] = 0;

    INDIRECT(1); MOVE(1);
    h_sbrTransientDetector->prevLowBandEnergy = newLowbandEnergy;
  }
  COUNT_sub_end();
}



static void
calculateThresholds (float **Energies,
                     int     noCols,
                     int     noRows,
                     float  *thresholds)
{
  int i, j;
  float mean_val, std_val, temp;
  float i_noCols  = (float)1.0f / (float)(noCols+noCols/2);
  float i_noCols1 = (float)1.0f / (float)(noCols+noCols/2 - 1);

  COUNT_sub_start("calculateThresholds");

  SHIFT(1); ADD(2); DIV(1); /* counting previous operations */

  PTR_INIT(2); /* pointer for Energies[],
                              thresholds[]
               */
  LOOP(1);
  for (i = 0; i < noRows; i++)
  {

    MOVE(2);
    mean_val = std_val = 0;

    LOOP(1);
    for (j = noCols/2; j < 2*noCols; j++) {

      ADD(1);
      mean_val += (float)Energies[j/2][i];
    }

    MULT(1);
    mean_val = mean_val * i_noCols;       /* average */

    LOOP(1);
    for (j = noCols/2; j < 2*noCols;j++) {

      ADD(1);
      temp = mean_val - (float)Energies[j/2][i];

      MAC(1);
      std_val += temp * temp;
    }

    MULT(1); TRANS(1);
    std_val = (float) sqrt (std_val * i_noCols1);

    ADD(1); BRANCH(1); STORE(1); /* max() */ MULT(1); MAC(1);
    thresholds[i] =
      max (ABS_THRES, (float)0.66f*thresholds[i] + (float)0.34f*STD_FAC * std_val);

  }

  COUNT_sub_end();

}


static void
extractTransientCandidates (float **Energies,
                            int     noCols,
                            int     start_band,
                            int     stop_band,
                            float  *thresholds,
                            float  *transients,
                            int     bufferLength)

{
  int i, j;
  float delta_1, delta_2, delta_3, i_thres;
  int bufferMove = bufferLength / 2;

  COUNT_sub_start("extractTransientCandidates");

  FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(bufferMove);
  memmove(transients, transients + noCols, bufferMove * sizeof (float));

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(bufferLength - bufferMove);
  memset (transients + bufferMove, 0, (bufferLength-bufferMove) * sizeof (float));


  PTR_INIT(1); /* pointer for thresholds[]
               */
  SHIFT(1); ADD(2); LOOP(1);
  for (i = start_band; i < stop_band; i++) {

    DIV(1);
    i_thres = (float)1.0f / thresholds[i];

    PTR_INIT(2); /* pointer for Energies[],
                                transients[]
                  */
    LOOP(1);
    for (j = 0; j < noCols+noCols/2 - 3; j++) {

      ADD(1);
      delta_1 = Energies[(j + noCols/2 + 1)/2][i] - Energies[(j + noCols/2 - 1)/2][i];

      ADD(1); BRANCH(1);
      if(delta_1 > thresholds[i]) {
        ADD(1); MAC(1);
        transients[j + bufferMove] += delta_1 * i_thres - (float)1.0f;
      }

      ADD(2);
      delta_2 = Energies[(j + noCols/2 + 2)/2][i] - Energies[(j + noCols/2 - 2)/2][i] + delta_1;

      ADD(1); BRANCH(1);
      if(delta_2 > thresholds[i]) {

        ADD(1); MAC(1);
        transients[j + bufferMove] += delta_2 * i_thres - (float)1.0f;
      }

      ADD(2);
      delta_3 = Energies[(j + noCols/2 + 3)/2][i] - Energies[(j + noCols/2 - 3)/2][i] + delta_2;

      ADD(1); BRANCH(1);
      if(delta_3 > thresholds[i]) {

        ADD(1); MAC(1);
        transients[j + bufferMove] += delta_3 * i_thres - (float)1.0f;
      }


    }
  }

  COUNT_sub_end();
}


void
transientDetect (float **Energies,
                 HANDLE_SBR_TRANSIENT_DETECTOR h_sbrTran,
                 int *tran_vector,
                 int timeStep
                 )
{

  int i;
  int cond;
  int no_cols = h_sbrTran->no_cols;
  int qmfStartSample = no_cols + timeStep * 4;
  float int_thres = h_sbrTran->tran_thr / (float) h_sbrTran->no_rows;

  COUNT_sub_start("transientDetect");

  INDIRECT(3); MULT(1); ADD(1); DIV(1); /* counting previous operations */

  INDIRECT(3); FUNC(4);
  calculateThresholds (Energies,
                       h_sbrTran->no_cols,
                       h_sbrTran->no_rows,
                       h_sbrTran->thresholds);


  INDIRECT(5); FUNC(7);
  extractTransientCandidates (Energies,
                              h_sbrTran->no_cols,
                              0,
                              h_sbrTran->no_rows,
                              h_sbrTran->thresholds,
                              h_sbrTran->transients,
                              h_sbrTran->buffer_length);


  MOVE(2);
  tran_vector[0] = 0;
  tran_vector[1] = 0;


  LOOP(1);
  for (i = qmfStartSample; i < qmfStartSample + no_cols; i++){

    MULT(1); ADD(2); LOGIC(1);
      cond = (h_sbrTran->transients[i] < (float)0.9f * h_sbrTran->transients[i - 1]) &&
           (h_sbrTran->transients[i - 1] > int_thres);


      BRANCH(1);
      if (cond) {

        ADD(1); DIV(1); MISC(1); STORE(1);
	      tran_vector[0] =
	        (int) floor ( (i - qmfStartSample) / timeStep );

        MOVE(1);
	      tran_vector[1] = 1;
        break;
      }
  }

  COUNT_sub_end();
}





int
CreateSbrTransientDetector (int chan,
                            HANDLE_SBR_TRANSIENT_DETECTOR h_sbrTransientDetector,

                            int   sampleFreq,
                            int   totalBitrate,
                            int   codecBitrate,
                            int   tran_thr,
                            int   mode,
                            int   tran_fc


                            )
{

  float bitrateFactor;
  float frame_dur = (float) 2048 / (float) sampleFreq;
  float temp = frame_dur - (float)0.010f;

  COUNT_sub_start("CreateSbrTransientDetector");

  DIV(1); ADD(1); /* counting previous operations */

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(SBR_TRANSIENT_DETECTOR));
  memset(h_sbrTransientDetector,0,sizeof(SBR_TRANSIENT_DETECTOR));

  BRANCH(1);
  if(codecBitrate)
  {
    DIV(1);
    bitrateFactor = (float)totalBitrate / (float)codecBitrate;
  }
  else
  {
    MOVE(1);
    bitrateFactor = (float)1.0f;
  }

  ADD(1); BRANCH(1);
  if (temp < (float)0.0001f)
  {
    MOVE(1);
    temp = (float)0.0001f;
  }

  MULT(1); DIV(1);
  temp = (float)0.000075f / (temp * temp);





  INDIRECT(3); MOVE(3);
  h_sbrTransientDetector->no_cols = 32;
  h_sbrTransientDetector->tran_thr = (float) tran_thr;
  h_sbrTransientDetector->tran_fc = tran_fc;

  INDIRECT(2); MULT(1); STORE(2);
  h_sbrTransientDetector->split_thr = temp * bitrateFactor;
  h_sbrTransientDetector->buffer_length = 96;

  INDIRECT(3); MOVE(3);
  h_sbrTransientDetector->no_rows = 64;
  h_sbrTransientDetector->mode = mode;
  h_sbrTransientDetector->prevLowBandEnergy = 0;

  INDIRECT(2); PTR_INIT(1);
  h_sbrTransientDetector->thresholds = &sbr_thresholds[chan*QMF_CHANNELS];

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(QMF_CHANNELS);
  memset(h_sbrTransientDetector->thresholds,0,sizeof(float)*QMF_CHANNELS);

  MULT(1); INDIRECT(2); PTR_INIT(1);
  h_sbrTransientDetector->transients =  &sbr_transients[chan*h_sbrTransientDetector->buffer_length];

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(h_sbrTransientDetector->buffer_length);
  memset(h_sbrTransientDetector->transients,0,sizeof(float)*h_sbrTransientDetector->buffer_length);

  COUNT_sub_end();

  return 0;
}



void
deleteSbrTransientDetector (HANDLE_SBR_TRANSIENT_DETECTOR h_sbrTransientDetector)
{
  COUNT_sub_start("deleteSbrTransientDetector");

  BRANCH(1);
  if(h_sbrTransientDetector){
    /*
      nothing to do
    */

  }

  COUNT_sub_end();

}
