/*
  Temporal Noise Shaping parameters
*/
#ifndef _TNS_PARAM_H
#define _TNS_PARAM_H

#include "tns.h"

typedef struct{
  int samplingRate;
  int maxBandLong;
  int maxBandShort;
}TNS_MAX_TAB_ENTRY;

typedef struct{
  int bitRateFrom;
  int bitRateTo;
  const TNS_CONFIG_TABULATED *paramMono_Long;  /* contains TNS parameters */
  const TNS_CONFIG_TABULATED *paramMono_Short;
  const TNS_CONFIG_TABULATED *paramStereo_Long;
  const TNS_CONFIG_TABULATED *paramStereo_Short;
}TNS_INFO_TAB;

int GetTnsParam(TNS_CONFIG_TABULATED *tnsConfigTab, 
                int bitRate, int channels, int blockType);

void GetTnsMaxBands(int samplingRate, int blockType, int* tnsMaxSfb);

#endif /* _TNS_PARAM_H */
