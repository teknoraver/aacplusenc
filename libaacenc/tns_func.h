/*
  Temporal Noise Shaping
*/
#ifndef _TNS_FUNC_H
#define _TNS_FUNC_H
#include "psy_configuration.h"

int InitTnsConfiguration(int bitrate,
                         long samplerate,
                         int channels,
                         TNS_CONFIG *tnsConfig,
                         PSY_CONFIGURATION_LONG psyConfig,
                         int active);

int InitTnsConfigurationShort(int bitrate,
                              long samplerate,
                              int channels,
                              TNS_CONFIG *tnsConfig,
                              PSY_CONFIGURATION_SHORT psyConfig,
                              int active);

int TnsDetect(TNS_DATA* tnsData,
              TNS_CONFIG tC,
              float* pScratchTns,
              const int sfbOffset[],
              float* spectrum,
              int subBlockNumber,
              int blockType,
              float * sfbEnergy);

void TnsSync(TNS_DATA *tnsDataDest,
             const TNS_DATA *tnsDataSrc,
             const TNS_CONFIG tC,
             const int subBlockNumber,
             const int blockType);

int TnsEncode(TNS_INFO* tnsInfo,
              TNS_DATA* tnsData,
              int numOfSfb,
              TNS_CONFIG tC,
              int lowPassLine,
              float* spectrum,
              int subBlockNumber,
              int blockType);

void ApplyTnsMultTableToRatios(int startCb,
                               int stopCb,
                               float *thresholds);


#endif /* _TNS_FUNC_H */
