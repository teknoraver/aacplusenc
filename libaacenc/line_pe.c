/*
  Perceptual entropie module
*/
#include <math.h>
#include "float.h"
#include "line_pe.h"

#include "counters.h" /* the 3GPP instrumenting tool */

#define LOG2_1    1.442695041f
#define C1        3.0f        /* log(8.0)/log(2) */
#define C2        1.3219281f  /* log(2.5)/log(2) */
#define C3        0.5593573f  /* 1-C2/C1 */


/* constants that do not change during successive pe calculations */
void prepareSfbPe(PE_CHANNEL_DATA *peChanData,
                  const float *sfbEnergy,
                  const float *sfbThreshold,
                  const float *sfbFormFactor,
                  const int     *sfbOffset,
                  const int     sfbCnt,
                  const int     sfbPerGroup,
                  const int     maxSfbPerGroup)
{
   int sfbGrp,sfb;
   int sfbWidth;
   float avgFormFactor;

   COUNT_sub_start("prepareSfbPe");

   LOOP(1);
   for(sfbGrp = 0;sfbGrp < sfbCnt;sfbGrp+=sfbPerGroup){

    PTR_INIT(6); /* pointers for sfbEnergy[],
                                 sfbThreshold[],
                                 sfbOffset[],
                                 sfbFormFactor[],
                                 peChanData->sfbNLines[],
                                 peChanData->sfbLdEnergy[]
                 */
    LOOP(1);
    for (sfb=0; sfb<maxSfbPerGroup; sfb++) {

      ADD(1); BRANCH(1);
      if (sfbEnergy[sfbGrp+sfb] > sfbThreshold[sfbGrp+sfb]) {

         ADD(1);
         sfbWidth = sfbOffset[sfbGrp+sfb+1] - sfbOffset[sfbGrp+sfb];

         /* estimate number of active lines */
         DIV(1); TRANS(1);
         avgFormFactor = (float) pow(sfbEnergy[sfbGrp+sfb]/(float)sfbWidth, 0.25f);

         DIV(1); STORE(1);
         peChanData->sfbNLines[sfbGrp+sfb] =

         sfbFormFactor[sfbGrp+sfb]/avgFormFactor;

          /* ld(sfbEn) */
         TRANS(1); MULT(1); STORE(1);
         peChanData->sfbLdEnergy[sfbGrp+sfb] = (float) (log(sfbEnergy[sfbGrp+sfb]) * LOG2_1);
      }
      else {

         MOVE(2);
         peChanData->sfbNLines[sfbGrp+sfb] = 0.0f;
         peChanData->sfbLdEnergy[sfbGrp+sfb] = 0.0f;
      }
    }
   }

   COUNT_sub_end();
}


void calcSfbPe(PE_CHANNEL_DATA *peChanData,
               const float *sfbEnergy,
               const float *sfbThreshold,
               const int    sfbCnt,
               const int    sfbPerGroup,
               const int    maxSfbPerGroup)
{
   int sfbGrp,sfb;
   float nLines;
   float ldThr, ldRatio;

   COUNT_sub_start("calcSfbPe");

   INDIRECT(3); MOVE(3);
   peChanData->pe = 0.0f;
   peChanData->constPart = 0.0f;
   peChanData->nActiveLines = 0.0f;

   LOOP(1);
   for(sfbGrp = 0;sfbGrp < sfbCnt;sfbGrp+=sfbPerGroup){

    PTR_INIT(8); /* pointers for sfbEnergy[]
                                 sfbThreshold[]
                                 peChanData->sfbLdEnergy[]
                                 peChanData->sfbNLines[]
                                 peChanData->sfbPe[]
                                 peChanData->sfbConstPart[]
                                 peChanData->sfbLdEnergy[]
                                 peChanData->sfbNActiveLines[]
                 */
    LOOP(1);
    for (sfb=0; sfb<maxSfbPerGroup; sfb++) {

      ADD(1); BRANCH(1);
      if (sfbEnergy[sfbGrp+sfb] > sfbThreshold[sfbGrp+sfb]) {

         TRANS(1); MULT(1);
         ldThr = (float)log(sfbThreshold[sfbGrp+sfb]) * LOG2_1;

         ADD(1);
         ldRatio = peChanData->sfbLdEnergy[sfbGrp+sfb] - ldThr;

         MOVE(1);
         nLines = peChanData->sfbNLines[sfbGrp+sfb];

         ADD(1); BRANCH(1);
         if (ldRatio >= C1) {

            MULT(1); STORE(1);
            peChanData->sfbPe[sfbGrp+sfb] = nLines * ldRatio;

            MULT(1); STORE(1);
            peChanData->sfbConstPart[sfbGrp+sfb] = nLines*peChanData->sfbLdEnergy[sfbGrp+sfb];
         }
         else {

            MULT(2); ADD(1); STORE(1);
            peChanData->sfbPe[sfbGrp+sfb] = nLines * (C2 + C3 * ldRatio);

            MULT(2); ADD(1); STORE(1);
            peChanData->sfbConstPart[sfbGrp+sfb] = nLines *
               (C2 + C3 * peChanData->sfbLdEnergy[sfbGrp+sfb]);

            MULT(1);
            nLines = nLines * C3;
         }

         MOVE(1);
         peChanData->sfbNActiveLines[sfbGrp+sfb] = nLines;
      }
      else {

         MOVE(3);
         peChanData->sfbPe[sfbGrp+sfb] = 0.0f;
         peChanData->sfbConstPart[sfbGrp+sfb] = 0.0f;
         peChanData->sfbNActiveLines[sfbGrp+sfb] = 0.0;
      }

      INDIRECT(3); ADD(3); STORE(3);
      peChanData->pe += peChanData->sfbPe[sfbGrp+sfb];
      peChanData->constPart += peChanData->sfbConstPart[sfbGrp+sfb];
      peChanData->nActiveLines += peChanData->sfbNActiveLines[sfbGrp+sfb];
    }
   }

   COUNT_sub_end();
}
