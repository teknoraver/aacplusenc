/*
  Noise floor estimation structs and prototypes
*/

#ifndef __NF_EST_H
#define __NF_EST_H

#include "sbr_main.h"
#include "sbr_def.h"
#include "fram_gen.h"


#define NF_SMOOTHING_LENGTH 4


typedef struct
{
  float prevNoiseLevels[NF_SMOOTHING_LENGTH][MAX_NUM_NOISE_VALUES];
  int freqBandTableQmf[MAX_NUM_NOISE_VALUES + 1];
  float ana_max_level;
  float weightFac;
  int noNoiseBands;
  int noiseBands;
  float noiseFloorOffset[MAX_NUM_NOISE_VALUES];
  const float* smoothFilter;
  INVF_MODE diffThres;
}
SBR_NOISE_FLOOR_ESTIMATE;

typedef SBR_NOISE_FLOOR_ESTIMATE *HANDLE_SBR_NOISE_FLOOR_ESTIMATE;

/*************/
/* Functions */
/*************/

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
                          );


int
CreateSbrNoiseFloorEstimate (HANDLE_SBR_NOISE_FLOOR_ESTIMATE  h_sbrNoiseFloorEstimate,
                             int ana_max_level,
                             const unsigned char *freqBandTable,
                             int nSfb,
                             int noiseBands,
                             int noiseFloorOffset,
                             unsigned int useSpeechConfig
                             );



int
resetSbrNoiseFloorEstimate (HANDLE_SBR_NOISE_FLOOR_ESTIMATE h_sbrNoiseFloorEstimate,
                            const unsigned char *freqBandTable,
                            int nSfb);




void deleteSbrNoiseFloorEstimate (HANDLE_SBR_NOISE_FLOOR_ESTIMATE h_sbrNoiseFloorEstimate);

#endif
