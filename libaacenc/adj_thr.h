#ifndef __ADJ_THR_H
#define __ADJ_THR_H

#include "adj_thr_data.h"
#include "qc_data.h"
#include "interface.h"

float bits2pe(const float bits);

int  AdjThrNew(ADJ_THR_STATE** phAdjThr,
               int nElements);

void AdjThrDelete(ADJ_THR_STATE *hAdjThr);

void AdjThrInit(ADJ_THR_STATE *hAdjThr,
                const float peMean,
                int chBitrate);

void AdjustThresholds(ADJ_THR_STATE *adjThrState,
                      ATS_ELEMENT* AdjThrStateElement,
                      PSY_OUT_CHANNEL psyOutChannel[MAX_CHANNELS],
                      PSY_OUT_ELEMENT* psyOutElement,
                      float *chBitDistribution,
                      float sfbFormFactor[MAX_CHANNELS][MAX_GROUPED_SFB],
                      const int nChannels,
                      QC_OUT_ELEMENT* qcOE,
                      const int avgBits,
                      const int bitresBits,
                      const int maxBitresBits,
                      const float maxBitFac,
                      const int sideInfoBits);

void AdjThrUpdate(ATS_ELEMENT *AdjThrStateElement,
                  const int dynBitsUsed);

#endif
