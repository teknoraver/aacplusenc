/*
  Inverse Filtering detection prototypes
*/
#ifndef _INV_FILT_DET_H
#define _INV_FILT_DET_H

#include "sbr_main.h"
#include "sbr_def.h"


#define INVF_SMOOTHING_LENGTH 2

typedef struct
{
  float *quantStepsSbr;
  float *quantStepsOrig;
  float *nrgBorders;
  int   numRegionsSbr;
  int   numRegionsOrig;
  int   numRegionsNrg;
  INVF_MODE regionSpace[5][5];
  INVF_MODE regionSpaceTransient[5][5];
  int EnergyCompFactor[5];

}DETECTOR_PARAMETERS;



typedef struct
{
  float  origQuotaMean[INVF_SMOOTHING_LENGTH+1];
  float  sbrQuotaMean[INVF_SMOOTHING_LENGTH+1];
  float origQuotaMeanFilt;
  float sbrQuotaMeanFilt;
  float avgNrg;

}DETECTOR_VALUES;



typedef struct
{

  int prevRegionSbr[MAX_NUM_NOISE_VALUES];
  int prevRegionOrig[MAX_NUM_NOISE_VALUES];

  float nrgAvg;

  int freqBandTableInvFilt[MAX_NUM_NOISE_VALUES];
  int noDetectorBands;
  int noDetectorBandsMax;

  DETECTOR_PARAMETERS *detectorParams;
  INVF_MODE prevInvfMode[MAX_NUM_NOISE_VALUES];
  DETECTOR_VALUES detectorValues[MAX_NUM_NOISE_VALUES];


  float wmQmf[MAX_NUM_NOISE_VALUES];
}
SBR_INV_FILT_EST;

typedef SBR_INV_FILT_EST *HANDLE_SBR_INV_FILT_EST;


void
qmfInverseFilteringDetector (HANDLE_SBR_INV_FILT_EST hInvFilt,
                             float ** quotaMatrix,
                             float *nrgVector,
                             char * indexVector,
                             int startIndex,
                             int stopIndex,
                             int transientFlag,
                             INVF_MODE* infVec);



int
createInvFiltDetector (HANDLE_SBR_INV_FILT_EST hInvFilt,
                       int* freqBandTableDetector,
                       int numDetectorBands,
                       int numberOfEstimatesPerFrame,
                       unsigned int useSpeechConfig);

void
deleteInvFiltDetector (HANDLE_SBR_INV_FILT_EST hInvFilt);


int
resetInvFiltDetector(HANDLE_SBR_INV_FILT_EST hInvFilt,
                     int* freqBandTableDetector,
                     int numDetectorBands);



#endif /* _QMF_INV_FILT_H */

