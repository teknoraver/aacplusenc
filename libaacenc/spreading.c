/*
  Spreading of energy and weighted tonality
*/
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "spreading.h"
#include "minmax.h"

#include "counters.h" /* the 3GPP instrumenting tool */



void SpreadingMax(const int    pbCnt,
                  const float *maskLowFactor,
                  const float *maskHighFactor,
                  float       *pbSpreadedEnergy)
{
   int i;

   COUNT_sub_start("SpreadingMax");

   /* slope to higher frequencies */
   PTR_INIT(2); /* pointers for pbSpreadedEnergy[],
                                maskHighFactor[]
                */
   LOOP(1);
   for (i=1; i<pbCnt; i++) {

      MULT(1); ADD(1); BRANCH(1); MOVE(1);
      pbSpreadedEnergy[i] = max(pbSpreadedEnergy[i],
                                maskHighFactor[i] * pbSpreadedEnergy[i-1]);
   }

   /* slope to lower frequencies */
   PTR_INIT(2); /* pointers for pbSpreadedEnergy[],
                                maskLowFactor[]
                */
   LOOP(1);
   for (i=pbCnt-2; i>=0; i--) {

      MULT(1); ADD(1); BRANCH(1); MOVE(1);
      pbSpreadedEnergy[i] = max(pbSpreadedEnergy[i],
                                maskLowFactor[i] * pbSpreadedEnergy[i+1]);
   }

   COUNT_sub_end();

}
