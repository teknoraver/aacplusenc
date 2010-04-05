/*
  General tonality correction detector module.
*/
#ifndef _TON_CORR_EST_H
#define _TON_CORR_EST_H

#include "sbr.h"
#include "sbr_main.h"
#include "sbr_def.h"
#include "mh_det.h"
#include "invf_est.h"
#include "nf_est.h"


#define MAX_NUM_PATCHES 6

/** parameter set for one single patch */
typedef struct {
  int    sourceStartBand;
  int    sourceStopBand;
  int    guardStartBand;

  int    targetStartBand;
  int    targetBandOffs;
  int    numBandsInPatch;
} PATCH_PARAM;




typedef struct
{
  int switchInverseFilt;
  int noQmfChannels;
  int numberOfEstimates;
  int numberOfEstimatesPerFrame;
  int move;
  int frameStartIndex;
  int startIndexMatrix;
  int frameStartIndexInvfEst;

  int prevTransientFlag;
  int transientNextFrame;
  int transientPosOffset;

  float *quotaMatrix[NO_OF_ESTIMATES];
  float nrgVector[NO_OF_ESTIMATES];
  char indexVector[QMF_CHANNELS];

  PATCH_PARAM  patchParam[MAX_NUM_PATCHES];
  int    guard;
  int    shiftStartSb;
  int    noOfPatches;

  SBR_MISSING_HARMONICS_DETECTOR sbrMissingHarmonicsDetector;
  SBR_NOISE_FLOOR_ESTIMATE sbrNoiseFloorEstimate;
  SBR_INV_FILT_EST sbrInvFilt;
}
SBR_TON_CORR_EST;

typedef SBR_TON_CORR_EST *HANDLE_SBR_TON_CORR_EST;


/* Functions */

void
TonCorrParamExtr(HANDLE_SBR_TON_CORR_EST hTonCorr,
                 INVF_MODE* infVec,
                 float* noiseLevels,
                 int* missingHarmonicFlag,
                 unsigned char* missingHarmonicsIndex,
                 char* envelopeCompensation,
                 const SBR_FRAME_INFO *frameInfo,
                 int* transientInfo,
                 unsigned char * freqBandTable,
                 int nSfb,
                 XPOS_MODE xposType
                 );



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
                        );


void
DeleteTonCorrParamExtr(HANDLE_SBR_TON_CORR_EST hTonCorr); /*!< Handle to SBR_TON_CORR struct. */


void
CalculateTonalityQuotas(HANDLE_SBR_TON_CORR_EST hTonCorr,
                        float **sourceBufferReal,
                        float **sourceBufferImag,
                        int usb);
#endif

