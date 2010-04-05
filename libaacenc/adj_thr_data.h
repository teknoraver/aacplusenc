/*
  threshold calculations
*/
#ifndef __ADJ_THR_DATA_H
#define __ADJ_THR_DATA_H

#include "psy_const.h"

typedef struct {
   float clipSaveLow, clipSaveHigh;
   float minBitSave, maxBitSave;
   float clipSpendLow, clipSpendHigh;
   float minBitSpend, maxBitSpend;
} BRES_PARAM;

typedef struct {
   unsigned char modifyMinSnr;
   int startSfbL, startSfbS;
} AH_PARAM;

typedef struct {
  float maxRed;
  float startRatio, maxRatio;
  float redRatioFac, redOffs;
} MINSNR_ADAPT_PARAM;

typedef struct {
  float peMin, peMax;
  float peOffset;
  AH_PARAM ahParam;
  MINSNR_ADAPT_PARAM minSnrAdaptParam;
  float peLast;
  int dynBitsLast;
  float peCorrectionFactor;
} ATS_ELEMENT;

typedef struct {
  BRES_PARAM   bresParamLong, bresParamShort;
  ATS_ELEMENT  adjThrStateElem;
} ADJ_THR_STATE;

#endif
