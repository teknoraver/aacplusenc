/*
  Short block grouping
*/
#ifndef __GRP_DATA_H__
#define __GRP_DATA_H__
#include "psy_data.h"

void
groupShortData(float         *mdctSpectrum,
               float         *tmpSpectrum,
               SFB_THRESHOLD *sfbThreshold,
               SFB_ENERGY    *sfbEnergy,
               SFB_ENERGY    *sfbEnergyMS,
               SFB_ENERGY    *sfbSpreadedEnergy,
               const int      sfbCnt,
               const int     *sfbOffset,
               const float   *sfbMinSnr,
               int           *groupedSfbOffset,
               int           *maxSfbPerGroup,
               float         *groupedSfbMinSnr,
               const int      noOfGroups,
               const int     *groupLen);

#endif /* __GRP_DATA_H__ */
