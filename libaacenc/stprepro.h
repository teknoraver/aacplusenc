/*
  stereo preprocessing struct and prototypes
*/
#ifndef __STPREPRO_H
#define __STPREPRO_H

struct STEREO_PREPRO;
typedef struct STEREO_PREPRO *HANDLE_STEREO_PREPRO;

#include "interface.h"
#include "channel_map.h"

struct STEREO_PREPRO {

  float   normPeFac;              /*! factor to normalize input PE, depends on bitrate and bandwidth */
  float   stereoAttenuationInc;   /*! att. increment parameter */
  float   stereoAttenuationDec;   /*! att. decrement parameter */

  float   avrgFreqEnergyL;        /*! energy left */
  float   avrgFreqEnergyR;        /*! energy right */
  float   avrgFreqEnergyM;        /*! energy mid */
  float   avrgFreqEnergyS;        /*! energy side */
  float   smoothedPeSumSum;       /*! time-smoothed PE */
  float   avgStoM;                /*! time-smoothed energy ratio S/M [dB] */
  float   lastLtoR;               /*! previous frame energy ratio L/R [dB] */
  float   lastNrgLR;              /*! previous frame energy L+R */

  float   ImpactFactor;           /*! bitrate dependent parameter */
  float   stereoAttenuation;      /*! the actual attenuation of this frame */
  float   stereoAttFac;           /*! the actual attenuation factor of this frame */

  /* tuning parameters that are not varied from frame to frame but initialized at init */
  int     stereoAttenuationFlag;  /*! flag to indicate usage */
  float   ConstAtt;               /*! if not zero, a constant att. will be applied [dB]*/
  float   stereoAttMax;           /*! the max. attenuation [dB]*/

  float   LRMin;                  /*! tuning parameter [dB] */
  float   LRMax;                  /*! tuning parameter [dB] */
  float   SMMin;                  /*! tuning parameter [dB] */
  float   SMMid;                  /*! tuning parameter [dB] */
  float   SMMax;                  /*! tuning parameter [dB] */
 
  float   PeMin;                  /*! tuning parameter */
  float   PeCrit;                 /*! tuning parameter */
  float   PeImpactMax;            /*! tuning parameter */
};

int InitStereoPreProcessing(HANDLE_STEREO_PREPRO hStPrePro, 
                            int nChannels, 
                            int bitRate, 
                            int sampleRate,
                            float usedScfRatio);

void ApplyStereoPreProcess(HANDLE_STEREO_PREPRO hStPrePro,
                           int                 nChannels, /*! total number of channels */              
                           ELEMENT_INFO        *elemInfo,
                           float *timeData,
                           int granuleLen);

void UpdateStereoPreProcess(PSY_OUT_CHANNEL  psyOutChan[MAX_CHANNELS], 
                            QC_OUT_ELEMENT* qcOutElement,
                            HANDLE_STEREO_PREPRO hStPrePro,
                            float weightPeFac);


#endif
