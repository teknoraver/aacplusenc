/*
  Pre echo control
*/
#ifndef __PRE_ECHO_CONTROL_H
#define __PRE_ECHO_CONTROL_H

void InitPreEchoControl(float *pbThresholdnm1,
                        int     numPb,
                        float *pbThresholdQuiet);


void PreEchoControl(float *pbThresholdNm1,
                    int     numPb,
                    float  maxAllowedIncreaseFactor,
                    float  minRemainingThresholdFactor,
                    float *pbThreshold);

#endif

