/*
  Psychoaccoustic data
*/
#ifndef _PSY_DATA_H
#define _PSY_DATA_H

#include "block_switch.h"
#include "tns.h"

/*
  the structs can be implemented as unions
*/

typedef struct{
  float Long[MAX_GROUPED_SFB];
  float Short[TRANS_FAC][MAX_SFB_SHORT];
}SFB_THRESHOLD;

typedef struct{
  float Long[MAX_GROUPED_SFB];
  float Short[TRANS_FAC][MAX_SFB_SHORT];
}SFB_ENERGY;

typedef struct{
  float Long;
  float Short[TRANS_FAC];
}SFB_ENERGY_SUM;

typedef struct{
  BLOCK_SWITCHING_CONTROL   blockSwitchingControl;          /* block switching */
  float                     *mdctDelayBuffer;               /* mdct delay buffer [BLOCK_SWITCHING_OFFSET]*/
  float                     sfbThresholdnm1[MAX_SFB];       /* PreEchoControl */

  SFB_THRESHOLD             sfbThreshold;                   /* adapt           */
  SFB_ENERGY                sfbEnergy;                      /* sfb Energy      */
  SFB_ENERGY                sfbEnergyMS;
  SFB_ENERGY_SUM            sfbEnergySum;
  SFB_ENERGY_SUM            sfbEnergySumMS;
  SFB_ENERGY                sfbSpreadedEnergy;

  float                     *mdctSpectrum;                  /* mdct spectrum [FRAME_LEN_LONG] */
}PSY_DATA;

#endif /* _PSY_DATA_H */
