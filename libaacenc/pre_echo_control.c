/*
  Pre echo control
*/
#include <math.h>
#include "pre_echo_control.h"

#include "counters.h" /* the 3GPP instrumenting tool */

void InitPreEchoControl(float *pbThresholdNm1,
                        int numPb,
                        float *pbThresholdQuiet)
{
  int pb;

  COUNT_sub_start("InitPreEchoControl");

  PTR_INIT(2); /* pbThresholdNm1[]
                  pbThresholdQuiet[]
               */
  LOOP(1);
  for(pb=0;pb<numPb;pb++)
  {
    MOVE(1);
    pbThresholdNm1[pb]=pbThresholdQuiet[pb];
  }

  COUNT_sub_end();
}



void PreEchoControl(float *pbThresholdNm1,
                    int     numPb,
                    float  maxAllowedIncreaseFactor,
                    float  minRemainingThresholdFactor,
                    float  *pbThreshold)
{
  int i;
  float tmpThreshold1, tmpThreshold2;

  COUNT_sub_start("PreEchoControl");

    PTR_INIT(2); /* pbThresholdNm1[]
                    pbThreshold[]
                 */
    LOOP(1);
    for(i = 0; i < numPb; i++) {
      
      MULT(2);
      tmpThreshold1 = maxAllowedIncreaseFactor * (pbThresholdNm1[i]);
      tmpThreshold2 = minRemainingThresholdFactor * pbThreshold[i];
      
      /* copy thresholds to internal memory */
      MOVE(1);
      pbThresholdNm1[i] = pbThreshold[i];
      
      ADD(1); BRANCH(1);
      if(pbThreshold[i] > tmpThreshold1) {

        MOVE(1);
        pbThreshold[i] = tmpThreshold1;
      }

      ADD(1); BRANCH(1);
      if(tmpThreshold2 > pbThreshold[i]) {

        MOVE(1);
        pbThreshold[i] = tmpThreshold2;
      }
      
    }
  
  COUNT_sub_end();
}
