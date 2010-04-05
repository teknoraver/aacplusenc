/*
  Scalefactor estimation
*/
#ifndef __SF_ESTIM_H__
#define __SF_ESTIM_H__

#include "psy_const.h"
#include "interface.h"
#include "qc_data.h"

void
CalcFormFactor(float sfbFormFactor[MAX_CHANNELS][MAX_GROUPED_SFB],
               float sfbNRelevantLines[MAX_CHANNELS][MAX_GROUPED_SFB],
               PSY_OUT_CHANNEL  psyOutChannel[MAX_CHANNELS],
               const int nChannels);

void
EstimateScaleFactors(PSY_OUT_CHANNEL psyOutChannel[MAX_CHANNELS],
                     QC_OUT_CHANNEL   qcOutChannel[MAX_CHANNELS],
                     float sfbFormFactor[MAX_CHANNELS][MAX_GROUPED_SFB],
                     float sfbNRelevantLines[MAX_CHANNELS][MAX_GROUPED_SFB],
                     const int nChannels);

#endif
