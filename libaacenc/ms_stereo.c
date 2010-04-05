/*
  MS stereo processing
*/
#include "psy_const.h"
#include "ms_stereo.h"
#include "minmax.h"
#include <math.h> /* for atan() */

#include "counters.h" /* the 3GPP instrumenting tool */

void MsStereoProcessing(float       *sfbEnergyLeft,
                        float       *sfbEnergyRight,
                        const float *sfbEnergyMid,
                        const float *sfbEnergySide,
                        float       *mdctSpectrumLeft,
                        float       *mdctSpectrumRight,
                        float       *sfbThresholdLeft,
                        float       *sfbThresholdRight,
                        float       *sfbSpreadedEnLeft,
                        float       *sfbSpreadedEnRight,
                        int         *msDigest,
                        int         *msMask,
                        const int   sfbCnt,
                        const int   sfbPerGroup,
                        const int   maxSfbPerGroup,
                        const int   *sfbOffset,
                        float       *weightMsLrPeRatio) 
{
  int sfb,sfboffs, j, cnt = 0;
  int msMaskTrueSomewhere = 0;
  int msMaskFalseSomewhere = 0;
  float sumMsLrPeRatio = 0;

  COUNT_sub_start("MsStereoProcessing");

  MOVE(4); /* counting previous operations */

  PTR_INIT(10); /* pointers for sfbThresholdLeft[sfb+sfboffs]
                                sfbThresholdRight[sfb+sfboffs]
                                sfbEnergyLeft[sfb+sfboffs]
                                sfbEnergyRight[sfb+sfboffs]
                                sfbEnergyMid[sfb+sfboffs]
                                sfbEnergySide[sfb+sfboffs]
                                sfbSpreadedEnLeft[sfb+sfboffs]
                                sfbSpreadedEnRight[sfb+sfboffs]
                                sfbOffset[sfb+sfboffs]
                                msMask[sfb+sfboffs]
               */
  LOOP(1);
  for(sfb=0; sfb<sfbCnt; sfb+=sfbPerGroup) {

    LOOP(1);
    for(sfboffs=0;sfboffs<maxSfbPerGroup;sfboffs++) {
      float pnlr,pnms,minThreshold;
      int useMS;
      
      ADD(1); BRANCH(1); MOVE(1);
      minThreshold=min(sfbThresholdLeft[sfb+sfboffs], sfbThresholdRight[sfb+sfboffs]);
      
      ADD(2); BRANCH(2); MOVE(2); /* max() */ DIV(2); MULT(1);
      pnlr = (sfbThresholdLeft[sfb+sfboffs]/
              max(sfbEnergyLeft[sfb+sfboffs],sfbThresholdLeft[sfb+sfboffs]))*
             (sfbThresholdRight[sfb+sfboffs]/
              max(sfbEnergyRight[sfb+sfboffs],sfbThresholdRight[sfb+sfboffs]));
      
      ADD(2); BRANCH(2); MOVE(2); /* max() */ DIV(2); MULT(1);
      pnms=  (minThreshold/max(sfbEnergyMid[sfb+sfboffs],minThreshold))*
             (minThreshold/max(sfbEnergySide[sfb+sfboffs],minThreshold));
      
      ADD(3); DIV(1);
      sumMsLrPeRatio += (pnlr + 1.0e-9f) / (pnms + 1.0e-9f) ;

      ADD(1);
      cnt++;

      ADD(1);
      useMS = (pnms >= pnlr);
      
      BRANCH(1);
      if(useMS){

        MOVE(2);
        msMask[sfb+sfboffs] = 1;
        msMaskTrueSomewhere = 1;

        PTR_INIT(2); /* pointers for mdctSpectrumLeft[],
                                     mdctSpectrumRight[]
                     */
        LOOP(1);
        for(j=sfbOffset[sfb+sfboffs]; j<sfbOffset[sfb+sfboffs+1]; j++) {
          float tmp = mdctSpectrumLeft[j];

          MOVE(1); /* counting operation above */

          ADD(1); MULT(1); STORE(1);
          mdctSpectrumLeft[j]  = 0.5f * (mdctSpectrumLeft[j] + mdctSpectrumRight[j]) ;

          ADD(1); MULT(1); STORE(1);
          mdctSpectrumRight[j] = 0.5f * (tmp - mdctSpectrumRight[j]) ;
        }

        MOVE(2);
        sfbThresholdLeft[sfb+sfboffs] = sfbThresholdRight[sfb+sfboffs] = minThreshold;

        MOVE(2);
        sfbEnergyLeft[sfb+sfboffs] = sfbEnergyMid[sfb+sfboffs];
        sfbEnergyRight[sfb+sfboffs] = sfbEnergySide[sfb+sfboffs];

        ADD(1); BRANCH(1); MOVE(1); /* min() */ MULT(1); STORE(2);
        sfbSpreadedEnLeft[sfb+sfboffs] = sfbSpreadedEnRight[sfb+sfboffs] =
          min(sfbSpreadedEnLeft[sfb+sfboffs], sfbSpreadedEnRight[sfb+sfboffs]) * 0.5f;

      }else {
        
        MOVE(2);
        msMask[sfb+sfboffs] = 0;
        msMaskFalseSomewhere = 1;
      }
    }
  }

  ADD(1); BRANCH(1);
  if(msMaskTrueSomewhere == 1) {

    ADD(1); BRANCH(1);
    if(msMaskFalseSomewhere == 1) {

      MOVE(1);
      *msDigest = SI_MS_MASK_SOME;
    } else {

      MOVE(1);
      *msDigest = SI_MS_MASK_ALL;
    }
  } else {

    MOVE(1);
    *msDigest = SI_MS_MASK_NONE;
  }
  
  ADD(1); BRANCH(1); MOVE(1);
  cnt = max (1,cnt);

  DIV(1); ADD(2); MULT(2); TRANS(1); STORE(1);
  *weightMsLrPeRatio = (float) (0.28 * atan( 0.37*(sumMsLrPeRatio/cnt-6.5) ) + 1.25);
  
  COUNT_sub_end();
}
