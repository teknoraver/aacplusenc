/*
  MDCT transform
*/
#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__

int Transform_Real(float *mdctDelayBuffer,
                   float *timeSignal,
                   int chIncrement,
                   float *realOut,
                   int windowSequence);
#endif
