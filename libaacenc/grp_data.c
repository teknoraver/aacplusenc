/*
  Short block grouping
*/
#include "psy_const.h"
#include "interface.h"
#include "minmax.h"
#include "grp_data.h"

#include "counters.h" /* the 3GPP instrumenting tool */

/*
* this routine does not work in-place
*/

void
groupShortData(float         *mdctSpectrum,
               float         *tmpSpectrum,
               SFB_THRESHOLD *sfbThreshold,
               SFB_ENERGY    *sfbEnergy,
               SFB_ENERGY    *sfbEnergyMS,
               SFB_ENERGY    *sfbSpreadedEnergy,
               const int      sfbCnt,
               const int     *sfbOffset,
               const float *sfbMinSnr,
               int           *groupedSfbOffset,
               int           *maxSfbPerGroup,
               float       *groupedSfbMinSnr,
               const int     noOfGroups,
               const int     *groupLen)
{
  int i,j;
  int line;
  int sfb;
  int grp;
  int wnd;
  int offset;
  int highestSfb;

  COUNT_sub_start("groupShortData");



  /* for short: regroup and  */
  /* cumulate energies und thresholds group-wise . */
 
  
    /* calculate sfbCnt */
    MOVE(1);
    highestSfb = 0;

    LOOP(1);
    for (wnd = 0; wnd < TRANS_FAC; wnd++)
    {
      PTR_INIT(1); /* sfbOffset[] */
      LOOP(1);
      for (sfb = sfbCnt-1; sfb >= highestSfb; sfb--)
      {
        PTR_INIT(1); /* mdctSpectrum[] */
        LOOP(1);
        for (line = sfbOffset[sfb+1]-1; line >= sfbOffset[sfb]; line--)
        {
          BRANCH(1);
          if (mdctSpectrum[wnd*FRAME_LEN_SHORT+line] != 0.0) break;  // this band is not completely zero
        }
        ADD(1); BRANCH(1);
        if (line >= sfbOffset[sfb]) break; // this band was not completely zero
      }
      ADD(1); BRANCH(1); MOVE(1);
      highestSfb = max(highestSfb, sfb);
    }
    BRANCH(1); MOVE(1);
    highestSfb = highestSfb > 0 ? highestSfb : 0;

    ADD(1); STORE(1);
    *maxSfbPerGroup = highestSfb+1;

  /* calculate sfbOffset */
  MOVE(2);
  i = 0;
  offset = 0;

  PTR_INIT(2); /* groupedSfbOffset[]
                  groupLen[]
               */
  LOOP(1);
  for (grp = 0; grp < noOfGroups; grp++)
  {
    PTR_INIT(1); /* sfbOffset[] */
    LOOP(1);
    for (sfb = 0; sfb < sfbCnt; sfb++)
    {
      MULT(1); ADD(1); STORE(1);
      groupedSfbOffset[i++] = offset + sfbOffset[sfb] * groupLen[grp];
    }

    MAC(1);
    offset += groupLen[grp] * FRAME_LEN_SHORT;
  }
  MOVE(1);
  groupedSfbOffset[i++] = FRAME_LEN_LONG;

   /* calculate minSnr */

  MOVE(2);
  i = 0;
  offset = 0;

  PTR_INIT(1); /* groupedSfbMinSnr[] */
  for (grp = 0; grp < noOfGroups; grp++)
  {
    PTR_INIT(1); /* sfbMinSnr[] */
    LOOP(1);
    for (sfb = 0; sfb < sfbCnt; sfb++)
    {
      MOVE(1);
      groupedSfbMinSnr[i++] = sfbMinSnr[sfb];
    }

    MAC(1);
    offset += groupLen[grp] * FRAME_LEN_SHORT;
  }



  /* sum up sfbThresholds */
  MOVE(2);
  wnd = 0;
  i = 0;

  PTR_INIT(2); /* groupLen[]
                  sfbThreshold->Long[]
               */
  LOOP(1);
  for (grp = 0; grp < noOfGroups; grp++)
  {
    PTR_INIT(1); /* sfbThreshold->Short[][] */
    LOOP(1);
    for (sfb = 0; sfb < sfbCnt; sfb++)
    {
      float thresh = sfbThreshold->Short[wnd][sfb];

      MOVE(1); /* counting previous operation */

      LOOP(1);
      for (j=1; j<groupLen[grp]; j++)
      {
        ADD(1);
        thresh += sfbThreshold->Short[wnd+j][sfb];
      }

      MOVE(1);
      sfbThreshold->Long[i++] = thresh;
    }
    wnd += groupLen[grp];
  }

  /* sum up sfbEnergies left/right */
  MOVE(2);
  wnd = 0;
  i = 0;

  PTR_INIT(2); /* groupLen[]
                  sfbEnergy->Long[]
               */
  LOOP(1);
  for (grp = 0; grp < noOfGroups; grp++)
  {
    PTR_INIT(1); /* sfbEnergy->Short[][] */
    LOOP(1);
    for (sfb = 0; sfb < sfbCnt; sfb++)
    {
      float energy = sfbEnergy->Short[wnd][sfb];

      MOVE(1); /* counting previous operation */

      LOOP(1);
      for (j=1; j<groupLen[grp]; j++)
      {
        ADD(1);
        energy += sfbEnergy->Short[wnd+j][sfb];
      }
      MOVE(1);
      sfbEnergy->Long[i++] = energy;
    }
    wnd += groupLen[grp];
  }

  /* sum up sfbEnergies mid/side */
  MOVE(1);
  wnd = 0;
  i = 0;

  PTR_INIT(2); /* groupLen[]
                  sfbEnergy->Long[]
               */
  LOOP(1);
  for (grp = 0; grp < noOfGroups; grp++)
  {
    PTR_INIT(1); /* sfbEnergy->Short[][] */
    LOOP(1);
    for (sfb = 0; sfb < sfbCnt; sfb++)
    {
      float energy = sfbEnergyMS->Short[wnd][sfb];

      MOVE(1); /* counting previous operation */

      LOOP(1);
      for (j=1; j<groupLen[grp]; j++)
      {
        ADD(1);
        energy += sfbEnergyMS->Short[wnd+j][sfb];
      }
      MOVE(1);
      sfbEnergyMS->Long[i++] = energy;
    }
    wnd += groupLen[grp];
  }

  /* sum up sfbSpreadedEnergies */
  MOVE(2);
  wnd = 0;
  i = 0;

  PTR_INIT(2); /* groupLen[]
                  sfbEnergy->Long[]
               */
  LOOP(1);
  for (grp = 0; grp < noOfGroups; grp++)
  {
     PTR_INIT(1); /* sfbEnergy->Short[][] */
     LOOP(1);
     for (sfb = 0; sfb < sfbCnt; sfb++)
     {
        float energy = sfbSpreadedEnergy->Short[wnd][sfb];

        MOVE(1); /* counting previous operation */

        LOOP(1);
        for (j=1; j<groupLen[grp]; j++)
        {
           ADD(1);
           energy += sfbSpreadedEnergy->Short[wnd+j][sfb];
        }
        MOVE(1);
        sfbSpreadedEnergy->Long[i++] = energy;
     }
     wnd += groupLen[grp];
  }

  /* re-group spectrum */
    MOVE(2);
    wnd = 0;
    i = 0;

    PTR_INIT(2); /* groupLen[]
                    tmpSpectrum[]
                 */
    LOOP(1);
    for (grp = 0; grp < noOfGroups; grp++)
    {
      PTR_INIT(1); /* sfbOffset[] */
      LOOP(1);
      for (sfb = 0; sfb < sfbCnt; sfb++)
      {
        LOOP(1);
        for (j = 0; j < groupLen[grp]; j++)
        {
          PTR_INIT(1); /* mdctSpectrum[] */
          LOOP(1);
          for (line = sfbOffset[sfb]; line < sfbOffset[sfb+1]; line++)
          {
            MOVE(1);
            tmpSpectrum[i++] = mdctSpectrum[(wnd+j)*FRAME_LEN_SHORT+line];
          }
        }
      }
     wnd += groupLen[grp];
  }

  PTR_INIT(2); /* mdctSpectrum[]
                  tmpSpectrum[]
               */
  LOOP(1);
  for(i=0;i<FRAME_LEN_LONG;i++)
  {
    MOVE(1);
    mdctSpectrum[i]=tmpSpectrum[i];
  }

  COUNT_sub_end();
}
