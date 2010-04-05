/*
  Psychoaccoustic configuration
*/
#ifndef _PSY_CONFIGURATION_H
#define _PSY_CONFIGURATION_H

#include "psy_const.h"
#include "tns.h"

typedef struct{

  int sfbCnt;
  int sfbActive;
  int sfbOffset[MAX_SFB_LONG+1];
  
  float sfbThresholdQuiet[MAX_SFB_LONG];

  float maxAllowedIncreaseFactor;   /* preecho control */
  float minRemainingThresholdFactor;

  int   lowpassLine;
  float clipEnergy;                 /* for level dependend tmn */

  float ratio;
  float sfbMaskLowFactor[MAX_SFB_LONG];
  float sfbMaskHighFactor[MAX_SFB_LONG];

  float sfbMaskLowFactorSprEn[MAX_SFB_LONG];
  float sfbMaskHighFactorSprEn[MAX_SFB_LONG];


  float sfbMinSnr[MAX_SFB_LONG];

  TNS_CONFIG tnsConf;

} PSY_CONFIGURATION_LONG;


typedef struct{

  int sfbCnt;
  int sfbActive;
  int sfbOffset[MAX_SFB_SHORT+1];

  float sfbThresholdQuiet[MAX_SFB_SHORT];

  float maxAllowedIncreaseFactor;   /* preecho control */
  float minRemainingThresholdFactor;

  int   lowpassLine;
  float clipEnergy;                 /* for level dependend tmn */

  float ratio;
  float sfbMaskLowFactor[MAX_SFB_SHORT];
  float sfbMaskHighFactor[MAX_SFB_SHORT];

  float sfbMaskLowFactorSprEn[MAX_SFB_SHORT];
  float sfbMaskHighFactorSprEn[MAX_SFB_SHORT];


  float sfbMinSnr[MAX_SFB_SHORT];

  TNS_CONFIG tnsConf;

} PSY_CONFIGURATION_SHORT;


int InitPsyConfiguration(long bitrate,
                         long samplerate,
                         int  bandwidth,
                         PSY_CONFIGURATION_LONG *psyConf);

int InitPsyConfigurationShort(long bitrate,
                              long samplerate,
                              int  bandwidth,
                              PSY_CONFIGURATION_SHORT *psyConf);

#endif /* _PSY_CONFIGURATION_H */



