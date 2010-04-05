/*
  Psychoaccoustic main module
*/
#ifndef _PSYMAIN_H
#define _PSYMAIN_H

#include "psy_configuration.h"
#include "qc_data.h"


/*
  psy kernel
*/
typedef struct  {
  PSY_CONFIGURATION_LONG  psyConfLong;
  PSY_CONFIGURATION_SHORT psyConfShort;
  PSY_DATA           psyData[MAX_CHANNELS];
  TNS_DATA           tnsData[MAX_CHANNELS];
  float*             pScratchTns;
}PSY_KERNEL;


int PsyNew( PSY_KERNEL  *hPsy, int nChan);
int PsyDelete( PSY_KERNEL  *hPsy);

int PsyOutNew( PSY_OUT *hPsyOut);
int PsyOutDelete( PSY_OUT *hPsyOut);

int psyMainInit( PSY_KERNEL *hPsy,
                 int sampleRate,
                 int bitRate,
                 int channels,
                 int tnsMask,
                 int bandwidth);


int psyMain(int                     nChannels, /*! total number of channels */              
            ELEMENT_INFO            *elemInfo,
            float                   *timeSignal, /*! interleaved time signal */ 
            PSY_DATA                 psyData[MAX_CHANNELS],
            TNS_DATA                 tnsData[MAX_CHANNELS],
            PSY_CONFIGURATION_LONG  *psyConfLong,
            PSY_CONFIGURATION_SHORT *psyConfShort,
            PSY_OUT_CHANNEL          psyOutChannel[MAX_CHANNELS],
            PSY_OUT_ELEMENT         *psyOutElement,
            float                   *pScratchTns);

#endif /* _PSYMAIN_H */

