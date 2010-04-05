/*
  Interface psychoaccoustic/quantizer
*/
#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "psy_const.h"
#include "psy_data.h"

enum
{
  MS_NONE = 0,
  MS_SOME = 1,
  MS_ALL  = 2
};

enum
{
  MS_ON = 1
};

struct TOOLSINFO {
  int msDigest; /* 0 = no MS; 1 = some MS, 2 = all MS */
  int msMask[MAX_GROUPED_SFB];
};


typedef struct  {
  int   sfbCnt;
  int   sfbPerGroup;
  int   maxSfbPerGroup;
  int   windowSequence;
  int   windowShape;
  int   groupingMask;
  int   sfbOffsets[MAX_GROUPED_SFB+1];
  float *sfbEnergy;
  float *sfbSpreadedEnergy;
  float *sfbThreshold;
  float *mdctSpectrum;
  
  float sfbEnSumLR;
  float sfbEnSumMS;
  float pe;   /* pe sum for stereo preprocessing */
  
  float sfbMinSnr[MAX_GROUPED_SFB];
  TNS_INFO tnsInfo;
 }PSY_OUT_CHANNEL;

typedef struct {
  struct TOOLSINFO toolsInfo;
  float weightMsLrPeRatio;      /*! factor to weight PE dependent on ms vs. lr PE ratio */
} PSY_OUT_ELEMENT;

typedef struct {
  /* information shared by both channels  */
  PSY_OUT_ELEMENT  psyOutElement;
  /* information specific to each channel */
  PSY_OUT_CHANNEL  psyOutChannel[MAX_CHANNELS];
}PSY_OUT;

void BuildInterface(float                  *mdctSpectrum,
                    SFB_THRESHOLD          *sfbThreshold,
                    SFB_ENERGY             *sfbEnergy,
                    SFB_ENERGY             *sfbSpreadedEnergy,
                    const SFB_ENERGY_SUM    sfbEnergySumLR,
                    const SFB_ENERGY_SUM    sfbEnergySumMS,
                    const int               windowSequence,
                    const int               windowShape,
                    const int               sfbCnt,
                    const int              *sfbOffset,
                    const int               maxSfbPerGroup,
                    const float           *groupedSfbMinSnr,
                    const int               noOfGroups,
                    const int              *groupLen,
                    PSY_OUT_CHANNEL        *psyOutCh);

#endif /* _INTERFACE_H */
