/*
  stereo pre-processing
*/
#include <string.h>
#include <stdlib.h>
#include "stprepro.h"

#include "counters.h" /* the 3GPP instrumenting tool */


#ifndef min
#define min(a,b) ( a < b ? a:b)
#endif

#ifndef max
#define max(a,b) ( a > b ? a:b)
#endif



/***************************************************************************/
/*!
 
  \brief  create and initialize a handle for stereo preprocessing
 
  \return an error state
 
****************************************************************************/
int InitStereoPreProcessing(HANDLE_STEREO_PREPRO hStPrePro, /*! handle (modified) */
                            int nChannels,    /*! number of channels */ 
                            int bitRate,      /*! the bit rate */
                            int sampleRate,   /*! the sample rate */
                            float usedScfRatio /*! the amount of scalefactors used (0-1.0) */
                            )
{
  float bpf = bitRate*1024.0f/sampleRate;
  float tmp;
  
  COUNT_sub_start("InitStereoPreProcessing");

  MULT(1); DIV(1); /* counting previous operation */

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(struct STEREO_PREPRO));
  memset(hStPrePro,0,sizeof(struct STEREO_PREPRO));
  
  ADD(1); BRANCH(1);
  if(nChannels == 2) {

    INDIRECT(1); MOVE(1);
    (hStPrePro)->stereoAttenuationFlag = 1;


    INDIRECT(1); MULT(1); DIV(1); STORE(1);
    (hStPrePro)->normPeFac    = 230.0f * usedScfRatio / bpf;

    INDIRECT(1); DIV(2); MULT(1); ADD(2); BRANCH(1); MOVE(1);
    (hStPrePro)->ImpactFactor = max (1, 400000.0f / (float) (bitRate - sampleRate*sampleRate/72000.0f) );

    INDIRECT(2); DIV(3); MULT(2); STORE(2);
    (hStPrePro)->stereoAttenuationInc = 22050.0f / sampleRate * 400.0f / bpf;
    (hStPrePro)->stereoAttenuationDec = 22050.0f / sampleRate * 200.0f / bpf;

    INDIRECT(2); MOVE(2);
    (hStPrePro)->ConstAtt     = 0.0f;
    (hStPrePro)->stereoAttMax = 12.0f;

    /* energy ratio thresholds (dB) */
    INDIRECT(4); MOVE(4);
    (hStPrePro)->SMMin = 0.0f;
    (hStPrePro)->SMMax = 15.0f;
    (hStPrePro)->LRMin = 10.0f;
    (hStPrePro)->LRMax = 30.0f;

    /* pe thresholds */
    INDIRECT(3); MOVE(3);
    (hStPrePro)->PeCrit =  1200.0f; 
    (hStPrePro)->PeMin  =  700.0f;
    (hStPrePro)->PeImpactMax = 100.0f;  

    /* init start values */
    INDIRECT(8); MOVE(8);
    (hStPrePro)->avrgFreqEnergyL  = 0.0f;
    (hStPrePro)->avrgFreqEnergyR  = 0.0f;
    (hStPrePro)->avrgFreqEnergyS  = 0.0f;
    (hStPrePro)->avrgFreqEnergyM  = 0.0f;
    (hStPrePro)->smoothedPeSumSum = 7000.0f; /* typical start value */
    (hStPrePro)->avgStoM          = -10.0f;  /* typical start value */
    (hStPrePro)->lastLtoR         = 0.0f;
    (hStPrePro)->lastNrgLR        = 0.0f;

    DIV(1); ADD(1);
    tmp = 1.0f - (bpf / 2600.0f);

    BRANCH(1); MOVE(1);
    tmp = max (tmp, 0.0f);
    
    INDIRECT(2); MULT(1); STORE(1);
    (hStPrePro)->stereoAttenuation =  tmp * (hStPrePro)->stereoAttMax;
  }

  COUNT_sub_end();

  return 0;
}


/***************************************************************************/
/*!
 
  \brief  do an appropriate attenuation on the side channel of a stereo
          signal
 
  \return nothing
 
****************************************************************************/
void ApplyStereoPreProcess(HANDLE_STEREO_PREPRO hStPrePro, /*!st.-preproc handle */
                           int                 nChannels, /*! total number of channels */              
                           ELEMENT_INFO        *elemInfo,
                           float *timeData,     /*! lr time data (modified) */
                           int granuleLen) /*! num. samples to be processed */
{
  /* inplace operation on inData ! */

  float SMRatio, StoM;
  float LRRatio, LtoR, deltaLtoR, deltaNrg;
  float EnImpact, PeImpact, PeNorm;
  float Att, AttAimed;
  float maxInc, maxDec, swiftfactor;
  float DELTA=0.1f;
  
  float fac = hStPrePro->stereoAttFac;
  float mPart, upper, div;
  float lFac,rFac;

  int i;

  COUNT_sub_start("ApplyStereoPreProcess");

  INDIRECT(1); MOVE(2); /* counting previous operations */

  INDIRECT(1); BRANCH(1);
  if (!hStPrePro->stereoAttenuationFlag) {
    COUNT_sub_end();
    return;
  }

  
  /* calc L/R ratio */
  INDIRECT(1); MULT(3); ADD(1);
  mPart = 2.0f * hStPrePro->avrgFreqEnergyM * (1.0f - fac*fac);

  INDIRECT(2); ADD(4); MULT(2); MAC(2);
  upper = hStPrePro->avrgFreqEnergyL * (1.0f+fac) + hStPrePro->avrgFreqEnergyR * (1.0f-fac) - mPart;
  div   = hStPrePro->avrgFreqEnergyR * (1.0f+fac) + hStPrePro->avrgFreqEnergyL * (1.0f-fac) - mPart;
  
  LOGIC(1); BRANCH(1);
  if (div == 0.0f || upper == 0.0f) {

    INDIRECT(1); MOVE(1);
    LtoR = hStPrePro->LRMax;
  }
  else {

    DIV(1); MISC(1);
    LRRatio = (float) fabs ( upper / div ) ;

    TRANS(1); MULT(1); MISC(1);
    LtoR = (float) fabs(10.0 * log10(LRRatio));
  }
  

  /* calc delta energy to previous frame */
  INDIRECT(3); ADD(3); DIV(1);
  deltaNrg = ( hStPrePro->avrgFreqEnergyL + hStPrePro->avrgFreqEnergyR + 1.0f) / 
             ( hStPrePro->lastNrgLR + 1.0f );

  TRANS(1); MULT(1); MISC(1);
  deltaNrg = (float) (fabs(10.0 * log10(deltaNrg)));
  
  

  /* Smooth S/M over time */
  INDIRECT(2); ADD(2); DIV(1);
  SMRatio = (hStPrePro->avrgFreqEnergyS + 1.0f) / (hStPrePro->avrgFreqEnergyM + 1.0f);

  TRANS(1); MULT(1);
  StoM = (float) (10.0 * log10(SMRatio));

  INDIRECT(2); MULT(1); MAC(1); STORE(1);
  hStPrePro->avgStoM = DELTA * StoM + (1-DELTA) * hStPrePro->avgStoM;
  
  

  MOVE(1);
  EnImpact = 1.0f;
  
  INDIRECT(2); ADD(1); BRANCH(1);
  if (hStPrePro->avgStoM > hStPrePro->SMMin) {

    INDIRECT(1); ADD(1); BRANCH(1);
    if (hStPrePro->avgStoM > hStPrePro->SMMax) {

      MOVE(1);
      EnImpact = 0.0f;
    }
    else {

      ADD(2); DIV(1);
      EnImpact = (hStPrePro->SMMax - hStPrePro->avgStoM) / (hStPrePro->SMMax - hStPrePro->SMMin);
    }
  }
  
  INDIRECT(1); ADD(1); BRANCH(1);
  if (LtoR > hStPrePro->LRMin) {

    INDIRECT(1); ADD(1); BRANCH(1);
    if ( LtoR > hStPrePro->LRMax) {

      MOVE(1);
      EnImpact = 0.0f;
    }
    else {

      ADD(2); DIV(1); MULT(1);
      EnImpact *= (hStPrePro->LRMax - LtoR) / (hStPrePro->LRMax - hStPrePro->LRMin);
    }
  }
  
  
  MOVE(1);
  PeImpact = 0.0f;
  
  INDIRECT(2); MULT(1);
  PeNorm = hStPrePro->smoothedPeSumSum * hStPrePro->normPeFac;
  
  INDIRECT(1); ADD(1); BRANCH(1);
  if ( PeNorm > hStPrePro->PeMin )  {

    INDIRECT(1); ADD(2); DIV(1);
    PeImpact= ((PeNorm - hStPrePro->PeMin) / (hStPrePro->PeCrit - hStPrePro->PeMin));
  }
  
  INDIRECT(1); ADD(1); BRANCH(1);
  if (PeImpact > hStPrePro->PeImpactMax) {

    MOVE(1);
    PeImpact = hStPrePro->PeImpactMax;
  }
  
  
  INDIRECT(1); MULT(2);
  AttAimed = EnImpact * PeImpact * hStPrePro->ImpactFactor;
  
  
  INDIRECT(1); ADD(1); BRANCH(1);
  if (AttAimed > hStPrePro->stereoAttMax) {

    MOVE(1);
    AttAimed = hStPrePro->stereoAttMax;
  }
  
  /* only accept changes if they are large enough */

  INDIRECT(1); ADD(2); MISC(1); LOGIC(1); BRANCH(1);
  if ( fabs(AttAimed - hStPrePro->stereoAttenuation) < 1.0f && AttAimed != 0.0f) {

    MOVE(1);
    AttAimed = hStPrePro->stereoAttenuation;
  }
  
  MOVE(1);
  Att = AttAimed;


  INDIRECT(1); ADD(1); MULT(1); BRANCH(1); /* max() */ ADD(2); DIV(1); MULT(1);
  swiftfactor = (6.0f + hStPrePro->stereoAttenuation) / (10.0f + LtoR) * max(1.0f, 0.2f * deltaNrg );
  
  INDIRECT(1); ADD(2); BRANCH(1); MOVE(1);
  deltaLtoR = max(3.0f, LtoR - hStPrePro->lastLtoR );
  
  MULT(2); DIV(1);
  maxDec = deltaLtoR * deltaLtoR / 9.0f * swiftfactor;
  
  ADD(1); BRANCH(1); MOVE(1);
  maxDec = min( maxDec, 5.0f );
  
  INDIRECT(1); MULT(1);
  maxDec *= hStPrePro->stereoAttenuationDec;
  
  
  INDIRECT(1); MULT(1); ADD(1); BRANCH(1);
  if (maxDec > hStPrePro->stereoAttenuation * 0.8f) {
    
    MOVE(1);
    maxDec = hStPrePro->stereoAttenuation * 0.8f;
  }
  
  INDIRECT(1); ADD(2); BRANCH(1); MOVE(1);
  deltaLtoR = max(3.0f, hStPrePro->lastLtoR - LtoR );
  
  MULT(2); DIV(1);
  maxInc = deltaLtoR * deltaLtoR / 9.0f * swiftfactor;
  
  ADD(1); BRANCH(1); MOVE(1);
  maxInc = min( maxInc, 5.0f );
  
  INDIRECT(1); MULT(1);
  maxInc *= hStPrePro->stereoAttenuationInc;
  
  
  INDIRECT(1); MULT(1); ADD(1); BRANCH(1);
  if (maxDec > hStPrePro->stereoAttenuation * 0.8f) {
    
    MOVE(1);
    maxDec = hStPrePro->stereoAttenuation * 0.8f;
  }
  
  INDIRECT(1); ADD(2); BRANCH(1); MOVE(1);
  deltaLtoR = max(3.0f, hStPrePro->lastLtoR - LtoR );
  
  MULT(2); DIV(1);
  maxInc = deltaLtoR * deltaLtoR / 9.0f * swiftfactor;
  
  ADD(1); BRANCH(1); MOVE(1);
  maxInc = min( maxInc, 5.0f );
  
  INDIRECT(1); MULT(1);
  maxInc *= hStPrePro->stereoAttenuationInc;
  
  
  INDIRECT(1); ADD(2); BRANCH(1);
  if (Att > hStPrePro->stereoAttenuation + maxInc) {

    MOVE(1);
    Att = hStPrePro->stereoAttenuation + maxInc;
  }

  INDIRECT(1); ADD(2); BRANCH(1);
  if (Att < hStPrePro->stereoAttenuation - maxDec) {

    MOVE(1);
    Att = hStPrePro->stereoAttenuation - maxDec;
  }
  
  INDIRECT(2); BRANCH(1); MOVE(1);
  if (hStPrePro->ConstAtt == 0) hStPrePro->stereoAttenuation = Att;
  else                          hStPrePro->stereoAttenuation = hStPrePro->ConstAtt;
  

  /* perform attenuation of Side Channel */
  
  INDIRECT(2); MULT(1); TRANS(1); STORE(1);
  hStPrePro->stereoAttFac = (float) pow(10.0f, 0.05f*(-hStPrePro->stereoAttenuation ));
  
  INDIRECT(1); ADD(2); MULT(2);
  lFac = 0.5f * (1.0f + hStPrePro->stereoAttFac);
  rFac = 0.5f * (1.0f - hStPrePro->stereoAttFac);
  
  PTR_INIT(2); /* pointer for timeData[nChannels * i + elemInfo->ChannelIndex[0]], 
                              timeData[nChannels * i + elemInfo->ChannelIndex[1]]
               */
  LOOP(1);
  for (i = 0; i < granuleLen; i++){
    float L = lFac * timeData[nChannels * i+elemInfo->ChannelIndex[0]] + rFac * timeData[nChannels * i + elemInfo->ChannelIndex[1]];
    float R = rFac * timeData[nChannels * i+elemInfo->ChannelIndex[0]] + lFac * timeData[nChannels * i + elemInfo->ChannelIndex[1]];

    MULT(2); MAC(2); /* counting operations above */
    
    MOVE(2);
    timeData[nChannels * i + elemInfo->ChannelIndex[0]]= L;
    timeData[nChannels * i + elemInfo->ChannelIndex[1]]= R;
  }
  
  INDIRECT(1); MOVE(1);
  hStPrePro->lastLtoR = LtoR;

  INDIRECT(3); ADD(1); STORE(1);
  hStPrePro->lastNrgLR = hStPrePro->avrgFreqEnergyL + hStPrePro->avrgFreqEnergyR;
  
  COUNT_sub_end();
}


/***************************************************************************/
/*!
 
  \brief  calc attenuation parameters - this has to take place after
          the 'QCMain' call because PeSum of the merged l/r-m/s signal
          is relevant
 
  \return nothing
 
****************************************************************************/
void UpdateStereoPreProcess(PSY_OUT_CHANNEL psyOutChannel[MAX_CHANNELS],
                            QC_OUT_ELEMENT* qcOutElement,   /*! provides access to PE */
                            HANDLE_STEREO_PREPRO hStPrePro,
                            float weightPeFac               /*! ratio of ms PE vs. lr PE */
                            )
{
  COUNT_sub_start("UpdateStereoPreProcess");

  INDIRECT(1); BRANCH(1);
  if (hStPrePro->stereoAttenuationFlag)
  { 
    float DELTA = 0.1f;

    INDIRECT(4); MOVE(4);
    hStPrePro->avrgFreqEnergyL = psyOutChannel[0].sfbEnSumLR;
    hStPrePro->avrgFreqEnergyR = psyOutChannel[1].sfbEnSumLR;
    hStPrePro->avrgFreqEnergyM = psyOutChannel[0].sfbEnSumMS;
    hStPrePro->avrgFreqEnergyS = psyOutChannel[1].sfbEnSumMS;


    INDIRECT(3); MULT(1); MAC(1); STORE(1);
    hStPrePro->smoothedPeSumSum = 
      DELTA * qcOutElement->pe * weightPeFac + (1 - DELTA) * hStPrePro->smoothedPeSumSum;


  }

  COUNT_sub_end();
}
