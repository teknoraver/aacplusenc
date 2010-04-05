/*
  missing harmonics detection header file
*/

#ifndef __MH_DETECT_H
#define __MH_DETECT_H


#include "sbr_main.h"
#include "fram_gen.h"



typedef struct
{
  float* guideVectorDiff;
  float* guideVectorOrig;
  unsigned char* guideVectorDetected;
}GUIDE_VECTORS;





typedef struct
{
  int qmfNoChannels;
  int nSfb;
  int sampleFreq;
  int previousTransientFlag;
  int previousTransientFrame;
  int previousTransientPos;

  int noVecPerFrame;
  int transientPosOffset;

  int move;
  int totNoEst;
  int noEstPerFrame;
  int timeSlots;

  unsigned char* guideScfb;
  char *prevEnvelopeCompensation;
  unsigned char* detectionVectors[NO_OF_ESTIMATES];
  float* tonalityDiff[NO_OF_ESTIMATES];
  float* sfmOrig[NO_OF_ESTIMATES];
  float* sfmSbr[NO_OF_ESTIMATES];
  GUIDE_VECTORS guideVectors[NO_OF_ESTIMATES];
}
SBR_MISSING_HARMONICS_DETECTOR;

typedef SBR_MISSING_HARMONICS_DETECTOR *HANDLE_SBR_MISSING_HARMONICS_DETECTOR;



void
SbrMissingHarmonicsDetectorQmf(HANDLE_SBR_MISSING_HARMONICS_DETECTOR h_sbrMissingHarmonicsDetector,
                               float ** pQuotaBuffer,
                               char *indexVector,
                               const SBR_FRAME_INFO *pFrameInfo,
                               const int* pTranInfo,
                               int* pAddHarmonicsFlag,
                               unsigned char* pAddHarmonicsScaleFactorBands,
                               const unsigned char* freqBandTable,
                               int nSfb,
                               char * envelopeCompensation);


int
CreateSbrMissingHarmonicsDetector(int chan,
                                  HANDLE_SBR_MISSING_HARMONICS_DETECTOR h_sbrMissingHarmonicsDetector,
                                  int sampleFreq,
                                  unsigned char* freqBandTable,
                                  int nSfb,
                                  int qmfNoChannels,
                                  int totNoEst,
                                  int move,
                                  int noEstPerFrame);

#endif
