/*
  Interface psychoaccoustic/quantizer
*/
#include "minmax.h"
#include "psy_const.h"
#include "interface.h"

#include "counters.h" /* the 3GPP instrumenting tool */

void BuildInterface(float                  *groupedMdctSpectrum,
                    SFB_THRESHOLD           *groupedSfbThreshold,
                    SFB_ENERGY              *groupedSfbEnergy,
                    SFB_ENERGY              *groupedSfbSpreadedEnergy,
                    const SFB_ENERGY_SUM    sfbEnergySumLR,
                    const SFB_ENERGY_SUM    sfbEnergySumMS,
                    const int               windowSequence,
                    const int               windowShape,
                    const int               groupedSfbCnt,
                    const int              *groupedSfbOffset,
                    const int               maxSfbPerGroup,
                    const float          *groupedSfbMinSnr,
                    const int               noOfGroups,
                    const int              *groupLen,
                    PSY_OUT_CHANNEL        *psyOutCh) // output
{
  int j;
  int grp;
  int mask;

  COUNT_sub_start("BuildInterface");

  /*
    copy values to psyOut
  */

  INDIRECT(11); MOVE(8); DIV(1); STORE(1);
  psyOutCh->maxSfbPerGroup = maxSfbPerGroup;
  psyOutCh->sfbCnt         = groupedSfbCnt;
  psyOutCh->sfbPerGroup    = groupedSfbCnt / noOfGroups;
  psyOutCh->windowSequence = windowSequence;
  psyOutCh->windowShape    = windowShape;
  psyOutCh->mdctSpectrum   = groupedMdctSpectrum;
  psyOutCh->sfbEnergy      = groupedSfbEnergy->Long;
  psyOutCh->sfbThreshold   = groupedSfbThreshold->Long;
  psyOutCh->sfbSpreadedEnergy = groupedSfbSpreadedEnergy->Long;
  

  PTR_INIT(2); /* psyOutCh->sfbOffsets[]
                  groupedSfbOffset[]
               */
  LOOP(1);
  for(j=0; j<groupedSfbCnt+1; j++)
  {
    MOVE(1);
    psyOutCh->sfbOffsets[j]=groupedSfbOffset[j];
  }

  PTR_INIT(2); /* psyOutCh->sfbMinSnr[]
                  groupedSfbMinSnr[]
               */
  for(j=0;j<groupedSfbCnt; j++){

    MOVE(1);
    psyOutCh->sfbMinSnr[j] = groupedSfbMinSnr[j];
  }

  /* generate grouping mask */
  MOVE(1);
  mask = 0;

  PTR_INIT(1); /* groupLen[grp] */
  LOOP(1);
  for (grp = 0; grp < noOfGroups; grp++)
  {
    SHIFT(1);
    mask <<= 1;

    LOOP(1);
    for (j=1; j<groupLen[grp]; j++) {

      SHIFT(1); LOGIC(1);
      mask<<=1; 
      mask|=1;
    }
  }
  /* copy energy sums to psy Out for stereo preprocessing */
  
  ADD(1); BRANCH(1);
  if (windowSequence != SHORT_WINDOW) {

    INDIRECT(2); MOVE(2);
    psyOutCh->sfbEnSumLR =  sfbEnergySumLR.Long;
    psyOutCh->sfbEnSumMS =  sfbEnergySumMS.Long;
  }
  else {
    int i;

    INDIRECT(2); MOVE(2);
    psyOutCh->sfbEnSumMS=0;
    psyOutCh->sfbEnSumLR=0;

    PTR_INIT(2); /* sfbEnergySumLR.Short[]
                    sfbEnergySumMS.Short[]
                 */
    LOOP(1);
    for (i=0;i< TRANS_FAC; i++) {

      ADD(2);
      psyOutCh->sfbEnSumLR += sfbEnergySumLR.Short[i];
      psyOutCh->sfbEnSumMS += sfbEnergySumMS.Short[i];
    }
    INDIRECT(2); STORE(2);
  }

  INDIRECT(1); MOVE(1);
  psyOutCh->groupingMask = mask;

  COUNT_sub_end();
}
