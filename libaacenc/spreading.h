/*
  Spreading of energy and weighted tonality
*/
#ifndef _SPREADING_H
#define _SPREADING_H

void SpreadingMax(const int    pbCnt,
                  const float *maskLowFactor,
                  const float *maskHighFactor,
                  float       *pbSpreadedEnergy);

#endif /* #ifndef _SPREADING_H */
