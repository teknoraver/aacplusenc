/*
  adjust thresholds
*/
#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "adj_thr_data.h"
#include "adj_thr.h"
#include "qc_data.h"
#include "line_pe.h"
#include "minmax.h"

#include "counters.h" /* the 3GPP instrumenting tool */

#define redExp       0.25f
#define invRedExp    (1.0f/redExp)
#define minSnrLimit  0.8f /* 1dB */

/* values for avoid hole flag */
enum _avoid_hole_state {
  NO_AH              =0,
  AH_INACTIVE        =1,
  AH_ACTIVE          =2
};


typedef struct {
   PE_CHANNEL_DATA peChannelData[MAX_CHANNELS];
   float pe;
   float constPart;
   float nActiveLines;
   float offset;
} PE_DATA;


/* convert from bits to pe */
float bits2pe(const float bits) {

   COUNT_sub_start("bits2pe");
   MULT(1); /* counting post-operation */
   COUNT_sub_end();

   return (bits * 1.18f);
}

/* loudness calculation (threshold to the power of redExp) */
static void calcThreshExp(float thrExp[MAX_CHANNELS][MAX_GROUPED_SFB],
                          PSY_OUT_CHANNEL  psyOutChannel[MAX_CHANNELS],
                          const int nChannels)
{
   int ch, sfb,sfbGrp;

   COUNT_sub_start("calcThreshExp");

   LOOP(1);
   for (ch=0; ch<nChannels; ch++) {
    PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];

    PTR_INIT(1); /* counting operation above */

    INDIRECT(2); LOOP(1);
    for(sfbGrp = 0;sfbGrp < psyOutChan->sfbCnt;sfbGrp+= psyOutChan->sfbPerGroup) {

      PTR_INIT(2); /* pointer for thrExp[][],
                                  psyOutChan->sfbThreshold[]
                   */
      INDIRECT(1); LOOP(1);
      for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {

        TRANS(1); STORE(1);
        thrExp[ch][sfbGrp+sfb] = (float) pow(psyOutChan->sfbThreshold[sfbGrp+sfb], redExp);
      }
    }
   }

   COUNT_sub_end();
}

/* reduce minSnr requirements for bands with relative low energies */
static void adaptMinSnr(PSY_OUT_CHANNEL     psyOutChannel[MAX_CHANNELS],
                        MINSNR_ADAPT_PARAM *msaParam,
                        const int           nChannels)
{
  int ch, sfb, sfbOffs, nSfb;
  float avgEn, dbRatio, minSnrRed;

  COUNT_sub_start("adaptMinSnr");

  LOOP(1);
  for (ch=0; ch<nChannels; ch++) {
    PSY_OUT_CHANNEL* psyOutChan = &psyOutChannel[ch];

    PTR_INIT(1); /* counting operation above */

    /* calc average energy per scalefactor band */
    MOVE(2);
    avgEn = 0.0f;
    nSfb = 0;

    INDIRECT(2); LOOP(1);
    for (sfbOffs=0; 
         sfbOffs<psyOutChan->sfbCnt;
         sfbOffs+=psyOutChan->sfbPerGroup) {

      PTR_INIT(1); /* pointer for psyOutChan->sfbEnergy[] */
      INDIRECT(1); LOOP(1);
      for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {

        ADD(1);
        avgEn += psyOutChan->sfbEnergy[sfbOffs+sfb];

        ADD(1);
        nSfb++;
      }
    }

    BRANCH(1);
    if (nSfb > 0) {

      DIV(1);
      avgEn /= nSfb;
    }

    /* reduce minSnr requirement by minSnr^minSnrRed dependent on avgEn/sfbEn */
    INDIRECT(2); LOOP(1);
    for (sfbOffs=0; 
         sfbOffs<psyOutChan->sfbCnt;
         sfbOffs+=psyOutChan->sfbPerGroup) {

      PTR_INIT(2); /* pointer for psyOutChan->sfbEnergy[],
                                  psyOutChan->sfbMinSnr[]
                   */
      INDIRECT(1); LOOP(1);
      for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {

        INDIRECT(1); MULT(1); ADD(1); BRANCH(1);
        if (msaParam->startRatio*psyOutChan->sfbEnergy[sfbOffs+sfb] < avgEn) {

          ADD(2); DIV(1); TRANS(1); MULT(1);
          dbRatio = (float) (10.0*log10((FLT_MIN+avgEn) /
                                        (FLT_MIN+psyOutChan->sfbEnergy[sfbOffs+sfb])));

          INDIRECT(1); MULT(1); ADD(1);
          minSnrRed = msaParam->redOffs + msaParam->redRatioFac * dbRatio;

          INDIRECT(1); ADD(1); BRANCH(1); MOVE(1);
          minSnrRed = max(minSnrRed, msaParam->maxRed);

          TRANS(1); STORE(1);
          psyOutChan->sfbMinSnr[sfbOffs+sfb] =
            (float)pow(psyOutChan->sfbMinSnr[sfbOffs+sfb], minSnrRed);

          ADD(1); BRANCH(1); MOVE(1);
          psyOutChan->sfbMinSnr[sfbOffs+sfb] =
            min(minSnrLimit, psyOutChan->sfbMinSnr[sfbOffs+sfb]);
        }
      }
    }

  }

  COUNT_sub_end();
}

/* determine bands where avoid hole is not necessary resp. possible */
static void initAvoidHoleFlag(int ahFlag[MAX_CHANNELS][MAX_GROUPED_SFB],
                              PSY_OUT_CHANNEL  psyOutChannel[MAX_CHANNELS],
                              PSY_OUT_ELEMENT* psyOutElement,
                              const int nChannels,
                              AH_PARAM *ahParam)
{
  int ch, sfb,sfbGrp;
  float sfbEn;
  float scaleSprEn;

  COUNT_sub_start("initAvoidHoleFlag");
  
  /* decrease spreaded energy by 3dB for long blocks, resp. 2dB for shorts
     (avoid more holes in long blocks) */
  LOOP(1);
  for (ch=0; ch<nChannels; ch++) {
    PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];
    
    PTR_INIT(1); /* counting operation above */
    
    INDIRECT(1); ADD(1); BRANCH(1);
    if (psyOutChan->windowSequence != SHORT_WINDOW) {
      
      MOVE(1);
      scaleSprEn = 0.5f;
    }
    else {
      
      MOVE(1);
      scaleSprEn = 0.63f;
    }
    
    INDIRECT(2); LOOP(1);
    for(sfbGrp = 0;sfbGrp < psyOutChan->sfbCnt;sfbGrp+= psyOutChan->sfbPerGroup){
      
      PTR_INIT(1); /* pointer for psyOutChan->sfbSpreadedEnergy[] */
      LOOP(1);
      for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {

        MULT(1); STORE(1);
        psyOutChan->sfbSpreadedEnergy[sfbGrp+sfb] *= scaleSprEn;
      }
    }
  }


  /* increase minSnr for local peaks, decrease it for valleys */
  
  INDIRECT(1); BRANCH(1);
  if (ahParam->modifyMinSnr) {
    
    LOOP(1);
    for(ch=0; ch<nChannels; ch++) {
      PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];
      
      PTR_INIT(1); /* counting operation above */
      
      INDIRECT(2); LOOP(1);
      for(sfbGrp = 0;sfbGrp < psyOutChan->sfbCnt;sfbGrp+= psyOutChan->sfbPerGroup){
        
        PTR_INIT(3); /* pointers for psyOutChan->sfbEnergy[sfbGrp],
                        psyOutChan->sfbEnergy[sfbGrp+sfb],
                        psyOutChan->sfbMinSnr[]
                     */
        INDIRECT(1); LOOP(1);
        for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {
          float sfbEnm1, sfbEnp1, avgEn;
          
          BRANCH(1); MOVE(1);
          if (sfb > 0)
            sfbEnm1 = psyOutChan->sfbEnergy[sfbGrp+sfb-1];
          else
            sfbEnm1 = psyOutChan->sfbEnergy[sfbGrp];
          
          INDIRECT(1); ADD(2); BRANCH(1); MOVE(1);
          if (sfb < psyOutChan->maxSfbPerGroup-1)
            sfbEnp1 = psyOutChan->sfbEnergy[sfbGrp+sfb+1];
          else
            sfbEnp1 = psyOutChan->sfbEnergy[sfbGrp+sfb];
          
          ADD(1); MULT(1);
          avgEn = (sfbEnm1 + sfbEnp1)/(float)2.0f;
          
          MOVE(1);
          sfbEn = psyOutChan->sfbEnergy[sfbGrp+sfb];
          
          /* peak ? */
          ADD(1); BRANCH(1);
          if (sfbEn > avgEn) {
            float tmpMinSnr = max((float)0.8f*avgEn/sfbEn,
                                  (float)0.316f);
            
            MULT(1); DIV(1); ADD(1); BRANCH(1); MOVE(1); /* counting previous operation */
            
            INDIRECT(1); ADD(1); BRANCH(1); /* if() */ ADD(1); BRANCH(1); MOVE(1); /* max() */
            if (psyOutChan->windowSequence!=SHORT_WINDOW)
              tmpMinSnr = max(tmpMinSnr, (float)0.316f);
            else
              tmpMinSnr = max(tmpMinSnr, (float)0.5f);
            
            ADD(1); BRANCH(1); MOVE(1);
            psyOutChan->sfbMinSnr[sfbGrp+sfb] =
              min(psyOutChan->sfbMinSnr[sfbGrp+sfb], tmpMinSnr);
          }
          
          /* valley ? */
          MULT(1); ADD(1); LOGIC(1); BRANCH(1);
          if (((float)2.0f*sfbEn < avgEn) && (sfbEn > (float)0.0f)) {
            float tmpMinSnr = avgEn/((float)2.0f*sfbEn) *
              psyOutChan->sfbMinSnr[sfbGrp+sfb];
            
            MULT(2); DIV(1); /* counting previous operation */
            
            ADD(1); BRANCH(1); MOVE(1);
            tmpMinSnr = min((float)0.8f, tmpMinSnr);
            
            MULT(1); ADD(1); BRANCH(1); MOVE(1);
            psyOutChan->sfbMinSnr[sfbGrp+sfb] = min(tmpMinSnr,
                                                    psyOutChan->sfbMinSnr[sfbGrp+sfb]*(float)3.16f);
          }
        }
      }
    }
  }
  
  ADD(1); BRANCH(1);
  if (nChannels == 2) {
    PSY_OUT_CHANNEL *psyOutChanM = &psyOutChannel[0];
    PSY_OUT_CHANNEL *psyOutChanS = &psyOutChannel[1];
    
    PTR_INIT(2); /* counting previous operation */
    
    PTR_INIT(7); /* pointers for psyOutElement->toolsInfo.msMask[],
                    psyOutChanM->sfbEnergy[],
                    psyOutChanS->sfbEnergy[],
                    psyOutChanM->sfbMinSnr[],
                    psyOutChanS->sfbMinSnr[],
                    psyOutChanM->sfbSpreadedEnergy[],
                    psyOutChanS->sfbSpreadedEnergy[]
                 */
    INDIRECT(1); LOOP(1);
    for (sfb=0; sfb<psyOutChanM->sfbCnt; sfb++) {
      
      BRANCH(1);
      if (psyOutElement->toolsInfo.msMask[sfb]) {
        float sfbEnM = psyOutChanM->sfbEnergy[sfb];
        float sfbEnS = psyOutChanS->sfbEnergy[sfb];
        float maxSfbEn = max(sfbEnM, sfbEnS);
        float maxThr = 0.25f * psyOutChanM->sfbMinSnr[sfb] * maxSfbEn;
        
        ADD(1); BRANCH(1); MOVE(1); /* max() */ MOVE(2); MULT(2); /* counting previous operations */
        
        ADD(1); DIV(1); ADD(1); BRANCH(1); MOVE(1); /* min() */ ADD(1); BRANCH(1); MOVE(1); /* max() */
        psyOutChanM->sfbMinSnr[sfb] = (float)max(psyOutChanM->sfbMinSnr[sfb],
                                                 min ( FLT_MAX, ((double)maxThr/(FLT_MIN+(double)sfbEnM))));
        
        ADD(1); BRANCH(1);
        if (psyOutChanM->sfbMinSnr[sfb] <= 1.0f) {
          
          ADD(1); BRANCH(1); MOVE(1);
          psyOutChanM->sfbMinSnr[sfb] = min(psyOutChanM->sfbMinSnr[sfb], 0.8f);
        }
        
        ADD(1); DIV(1); ADD(1); BRANCH(1); MOVE(1); /* min() */ ADD(1); BRANCH(1); MOVE(1); /* max() */       
        psyOutChanS->sfbMinSnr[sfb] = (float)max(psyOutChanS->sfbMinSnr[sfb],
                                                 min ( FLT_MAX, ((double)maxThr/(FLT_MIN+(double)sfbEnS))));
        
        ADD(1); BRANCH(1);
        if (psyOutChanS->sfbMinSnr[sfb] <= 1.0f) {
          
          ADD(1); BRANCH(1); MOVE(1);
          psyOutChanS->sfbMinSnr[sfb] = min(psyOutChanS->sfbMinSnr[sfb], 0.8f);
        }
        
        ADD(1); BRANCH(1);
        if (sfbEnM > psyOutChanM->sfbSpreadedEnergy[sfb]) {
          
          MULT(1); STORE(1);
          psyOutChanS->sfbSpreadedEnergy[sfb] = 0.9f * sfbEnS;
        }
        
        ADD(1); BRANCH(1);
        if (sfbEnS > psyOutChanS->sfbSpreadedEnergy[sfb]) {
          
          MULT(1); STORE(1);
          psyOutChanM->sfbSpreadedEnergy[sfb] = 0.9f * sfbEnM;
        }
      }
    }
  }
  
  
  /* init ahFlag (0: no ah necessary, 1: ah possible, 2: ah active */
  LOOP(1);
  for(ch=0; ch<nChannels; ch++) {
    PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];
    
    PTR_INIT(1); /* counting previous operation */
    
    INDIRECT(2); LOOP(1);
    for(sfbGrp = 0;sfbGrp < psyOutChan->sfbCnt;sfbGrp+= psyOutChan->sfbPerGroup){
      
      PTR_INIT(4); /* pointers for psyOutChan->sfbEnergy[],
                      psyOutChan->sfbMinSnr[],
                      psyOutChan->sfbSpreadedEnergy[],
                      ahFlag[][]
                   */
      INDIRECT(1); LOOP(1);
      for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {
        
        ADD(2); LOGIC(1); BRANCH(1);
        if (psyOutChan->sfbSpreadedEnergy[sfbGrp+sfb] > psyOutChan->sfbEnergy[sfbGrp+sfb] ||
            psyOutChan->sfbMinSnr[sfbGrp+sfb] > (float)1.0) {
          
          MOVE(1);
          ahFlag[ch][sfbGrp+sfb] = NO_AH;
        }
        else {
          
          MOVE(1);
          ahFlag[ch][sfbGrp+sfb] = AH_INACTIVE;
        }
      }
      
      PTR_INIT(1); /* pointer for ahFlag[][] */
      INDIRECT(2); LOOP(1);
      for (sfb=psyOutChan->maxSfbPerGroup; sfb<psyOutChan->sfbPerGroup; sfb++) {
        
        MOVE(1);
        ahFlag[ch][sfbGrp+sfb] = NO_AH;
      }
    }
  }
  
  COUNT_sub_end();
}




/* constants that do not change during successive pe calculations */
static void preparePe(PE_DATA *peData,
                      PSY_OUT_CHANNEL  psyOutChannel[MAX_CHANNELS],
                      float sfbFormFactor[MAX_CHANNELS][MAX_GROUPED_SFB],
                      const int nChannels,
                      const float peOffset)
{
  int ch;
  
  COUNT_sub_start("preparePe");
  
  LOOP(1);
  for(ch=0; ch<nChannels; ch++) {
    PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];
    
    PTR_INIT(1); /* counting previous operation */
    
    INDIRECT(8); PTR_INIT(1); FUNC(8);
    prepareSfbPe(&peData->peChannelData[ch],
                 psyOutChan->sfbEnergy,
                 psyOutChan->sfbThreshold,
                 sfbFormFactor[ch],
                 psyOutChan->sfbOffsets,
                 psyOutChan->sfbCnt,
                 psyOutChan->sfbPerGroup,
                 psyOutChan->maxSfbPerGroup);
  }
  
  INDIRECT(1); MOVE(1);
  peData->offset = peOffset;
  
  COUNT_sub_end();
}



/* calculate pe for both channels */
static void calcPe(PE_DATA *peData,
                   PSY_OUT_CHANNEL  psyOutChannel[MAX_CHANNELS],
                   const int nChannels)
{
  int ch;

  COUNT_sub_start("calcPe");
  
  INDIRECT(4); MOVE(3);
  peData->pe = peData->offset;
  peData->constPart = 0.0f;
  peData->nActiveLines = 0.0f;
  
  LOOP(1);
  for(ch=0; ch<nChannels; ch++) {
    PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];
    PE_CHANNEL_DATA *peChanData = &peData->peChannelData[ch];
    
    INDIRECT(1); PTR_INIT(2); /* counting previous operations */
    
    INDIRECT(6); PTR_INIT(1); FUNC(6);
    calcSfbPe(&peData->peChannelData[ch],
              psyOutChan->sfbEnergy,
              psyOutChan->sfbThreshold,
              psyOutChan->sfbCnt,
              psyOutChan->sfbPerGroup,
              psyOutChan->maxSfbPerGroup);
    
    INDIRECT(3); ADD(3); STORE(3);
    peData->pe += peChanData->pe;
    peData->constPart += peChanData->constPart;
    peData->nActiveLines += peChanData->nActiveLines;
    
    INDIRECT(1); MOVE(1);
    psyOutChannel[ch].pe = peData->pe;                 /* update pe for stereo preprocessing */
  } 
  
  COUNT_sub_end();
}



/* sum the pe data only for bands where avoid hole is inactive */
static void calcPeNoAH(float *pe,
                       float *constPart,
                       float  *nActiveLines,
                       PE_DATA *peData,
                       int ahFlag[MAX_CHANNELS][MAX_GROUPED_SFB],
                       PSY_OUT_CHANNEL  psyOutChannel[MAX_CHANNELS],
                       const int nChannels)
{
   int ch, sfb,sfbGrp;

   COUNT_sub_start("calcPeNoAH");

   MOVE(3);
   *pe = 0.0f;
   *constPart = 0.0f;
   *nActiveLines = 0;

   LOOP(1);
   for(ch=0; ch<nChannels; ch++) {
      PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];
      PE_CHANNEL_DATA *peChanData = &peData->peChannelData[ch];

      PTR_INIT(2); /* counting previous operations */

      INDIRECT(2); LOOP(1);
      for(sfbGrp = 0;sfbGrp < psyOutChan->sfbCnt;sfbGrp+= psyOutChan->sfbPerGroup){

        PTR_INIT(4); /* pointers for ahFlag[ch][sfbGrp+sfb],
                                     peChanData->sfbPe[sfbGrp+sfb],
                                     peChanData->sfbConstPart[sfbGrp+sfb],
                                     peChanData->sfbNActiveLines[sfbGrp+sfb]
                     */
        INDIRECT(1); LOOP(1);
        for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {

          ADD(1); BRANCH(1);
          if(ahFlag[ch][sfbGrp+sfb] < AH_ACTIVE) {

            ADD(3); STORE(3);
            *pe += peChanData->sfbPe[sfbGrp+sfb];
            *constPart += peChanData->sfbConstPart[sfbGrp+sfb];
            *nActiveLines += peChanData->sfbNActiveLines[sfbGrp+sfb];
          }
        }
      }
   }

   COUNT_sub_end();
}


/* apply reduction formula */
static void reduceThresholds(PSY_OUT_CHANNEL  psyOutChannel[MAX_CHANNELS],
                             int ahFlag[MAX_CHANNELS][MAX_GROUPED_SFB],
                             float thrExp[MAX_CHANNELS][MAX_GROUPED_SFB],
                             const int nChannels,
                             const float redVal)
{
   int ch, sfb,sfbGrp;
   float sfbEn, sfbThr,sfbThrReduced;

   COUNT_sub_start("reduceThresholds");

   LOOP(1);
   for(ch=0; ch<nChannels; ch++) {
      PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];

      PTR_INIT(1); /* counting previous operation */

      INDIRECT(2); LOOP(1);
      for(sfbGrp = 0;sfbGrp < psyOutChan->sfbCnt;sfbGrp+= psyOutChan->sfbPerGroup){

        PTR_INIT(5); /* pointers for psyOutChan->sfbMinSnr[sfbGrp+sfb],
                                     psyOutChan->sfbEnergy[sfbGrp+sfb],
                                     psyOutChan->sfbThreshold[sfbGrp+sfb],
                                     thrExp[ch][sfbGrp+sfb],
                                     ahFlag[ch][sfbGrp+sfb]
                     */
        INDIRECT(1); LOOP(1);
        for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {

         MOVE(2);
         sfbEn  = psyOutChan->sfbEnergy[sfbGrp+sfb];
         sfbThr = psyOutChan->sfbThreshold[sfbGrp+sfb];

         ADD(1); BRANCH(1);
         if (sfbEn > sfbThr) {
            /* threshold reduction formula */

            ADD(1); TRANS(1);
            sfbThrReduced = (float) pow(thrExp[ch][sfbGrp+sfb]+redVal, invRedExp);

            /* avoid holes */
            MULT(1); ADD(2); LOGIC(1); BRANCH(1);
            if ((sfbThrReduced > psyOutChan->sfbMinSnr[sfbGrp+sfb] * sfbEn) && (ahFlag[ch][sfbGrp+sfb] != NO_AH)){

              ADD(1); BRANCH(1); MOVE(1);
              sfbThrReduced = max(psyOutChan->sfbMinSnr[sfbGrp+sfb] * sfbEn, sfbThr);

              MOVE(1);
              ahFlag[ch][sfbGrp+sfb] = AH_ACTIVE;
            }

            MOVE(1);
            psyOutChan->sfbThreshold[sfbGrp+sfb] = sfbThrReduced;
         }
        }
      }
   }

   COUNT_sub_end();
}


static void correctThresh(PSY_OUT_CHANNEL  psyOutChannel[MAX_CHANNELS],
                          int ahFlag[MAX_CHANNELS][MAX_GROUPED_SFB],
                          PE_DATA *peData,
                          float thrExp[MAX_CHANNELS][MAX_GROUPED_SFB],
                          const float redVal,
                          const int nChannels,
                          const float deltaPe)
{
   int ch, sfb,sfbGrp;
   PSY_OUT_CHANNEL *psyOutChan;
   PE_CHANNEL_DATA *peChanData;
   float deltaSfbPe;
   float thrFactor;
   float sfbPeFactors[MAX_CHANNELS][MAX_GROUPED_SFB], normFactor;
   float sfbEn, sfbThr, sfbThrReduced;

   COUNT_sub_start("correctThresh");

   /* for each sfb calc relative factors for pe changes */
   MOVE(1);
   normFactor = FLT_MIN; /* to avoid division by zero */

   LOOP(1);
   for(ch=0; ch<nChannels; ch++) {
      psyOutChan = &psyOutChannel[ch];
      peChanData = &peData->peChannelData[ch];

      INDIRECT(1); PTR_INIT(2); /* counting previous operations */

      INDIRECT(2); LOOP(1);
      for(sfbGrp = 0;sfbGrp < psyOutChan->sfbCnt;sfbGrp+= psyOutChan->sfbPerGroup){

        PTR_INIT(4); /* pointers for ahFlag[ch][sfbGrp+sfb],
                                     thrExp[ch][sfbGrp+sfb],
                                     sfbPeFactors[ch][sfbGrp+sfb],
                                     peChanData->sfbNActiveLines[sfbGrp+sfb]
                     */
        INDIRECT(1); LOOP(1);
        for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {

         ADD(1); LOGIC(1); BRANCH(1);
         if ((ahFlag[ch][sfbGrp+sfb] < AH_ACTIVE) || (deltaPe > 0)) {

            ADD(1); DIV(1); STORE(1);
            sfbPeFactors[ch][sfbGrp+sfb] = peChanData->sfbNActiveLines[sfbGrp+sfb] /
                                              (thrExp[ch][sfbGrp+sfb] + redVal);

            ADD(1);
            normFactor += sfbPeFactors[ch][sfbGrp+sfb];
         }
         else {
            MOVE(1);
            sfbPeFactors[ch][sfbGrp+sfb] = 0.0f;
         }
        }
      }
   }

   DIV(1);
   normFactor = 1.0f/normFactor;

   LOOP(1);
   for(ch=0; ch<nChannels; ch++) {
      psyOutChan = &psyOutChannel[ch];
      peChanData = &peData->peChannelData[ch];

      INDIRECT(1); PTR_INIT(2); /* counting previous operations */

      INDIRECT(2); LOOP(1);
      for(sfbGrp = 0;sfbGrp < psyOutChan->sfbCnt;sfbGrp+= psyOutChan->sfbPerGroup){

        PTR_INIT(6); /* pointers for ahFlag[ch][sfbGrp+sfb],
                                     sfbPeFactors[ch][sfbGrp+sfb],
                                     psyOutChan->sfbMinSnr[sfbGrp+sfb],
                                     peChanData->sfbNActiveLines[sfbGrp+sfb],
                                     psyOutChan->sfbEnergy[sfbGrp+sfb],
                                     psyOutChan->sfbThreshold[sfbGrp+sfb]
                     */
        INDIRECT(1); LOOP(1);
        for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {

         /* pe difference for this sfb */
         MULT(2);
         deltaSfbPe = sfbPeFactors[ch][sfbGrp+sfb] * normFactor * deltaPe;

         ADD(1); BRANCH(1);
         if (peChanData->sfbNActiveLines[sfbGrp+sfb] > (float)0.5f) {

            /* new threshold */
            MOVE(2);
            sfbEn  = psyOutChan->sfbEnergy[sfbGrp+sfb];
            sfbThr = psyOutChan->sfbThreshold[sfbGrp+sfb];

            MULT(1); DIV(1); ADD(1); BRANCH(1); MOVE(1);
            thrFactor = min(-deltaSfbPe/peChanData->sfbNActiveLines[sfbGrp+sfb],20.f);

            TRANS(1);
            thrFactor = (float) pow(2.0f,thrFactor);

            MULT(1);
            sfbThrReduced = sfbThr * thrFactor;
            /* avoid hole */

            MULT(1); ADD(2); BRANCH(1);
            if ((sfbThrReduced > psyOutChan->sfbMinSnr[sfbGrp+sfb] * sfbEn) &&
                (ahFlag[ch][sfbGrp+sfb] == AH_INACTIVE)) {

               ADD(1); BRANCH(1); MOVE(1);
               sfbThrReduced = max(psyOutChan->sfbMinSnr[sfbGrp+sfb] * sfbEn, sfbThr);

               MOVE(1);
               ahFlag[ch][sfbGrp+sfb] = AH_ACTIVE;
            }

            MOVE(1);
            psyOutChan->sfbThreshold[sfbGrp+sfb] = sfbThrReduced;
         }
        }
      }
   }

   COUNT_sub_end();
}

static void reduceMinSnr(PSY_OUT_CHANNEL  psyOutChannel[MAX_CHANNELS],
                         PE_DATA *peData, 
                         int ahFlag[MAX_CHANNELS][MAX_GROUPED_SFB],
                         const int nChannels,
                         const float desiredPe)
{
  int ch, sfb, sfbSubWin;
  float deltaPe;

  COUNT_sub_start("reduceMinSnr");

  /* start at highest freq down to 0 */
  MOVE(1);
  sfbSubWin = psyOutChannel[0].maxSfbPerGroup;
  
  PTR_INIT(7); /* pointer for ahFlag[ch][sfb],
                  psyOutChannel[ch].sfbMinSnr[sfb],
                  psyOutChannel[ch].sfbThreshold[sfb],
                  psyOutChannel[ch].sfbEnergy[sfb],
                  peData->peChannelData[ch].sfbNLines[sfb],
                  peData->peChannelData[ch].sfbPe[sfb],
                  peData->peChannelData[ch].pe
               */
  INDIRECT(1); LOOP(1);
  while (peData->pe > desiredPe && sfbSubWin > 0) {
    ADD(1); LOGIC(1); /* while() condition */
    
    ADD(1);
    sfbSubWin--;
    
    /* loop over all subwindows */
    LOOP(1);
    for (sfb=sfbSubWin; sfb<psyOutChannel[0].sfbCnt;
         sfb+=psyOutChannel[0].sfbPerGroup) {
      
      /* loop over all channels */
      LOOP(1);
      for (ch=0; ch<nChannels; ch++) {
        
        ADD(2); BRANCH(1);
        if (ahFlag[ch][sfb] != NO_AH &&
            psyOutChannel[ch].sfbMinSnr[sfb] < minSnrLimit) {
          
          MOVE(1);
          psyOutChannel[ch].sfbMinSnr[sfb] = minSnrLimit;
          
          MULT(1); STORE(1);
          psyOutChannel[ch].sfbThreshold[sfb] =
            psyOutChannel[ch].sfbEnergy[sfb] * psyOutChannel[ch].sfbMinSnr[sfb];
          
          MULT(1); ADD(1);
          deltaPe = peData->peChannelData[ch].sfbNLines[sfb] * 1.5f -
            peData->peChannelData[ch].sfbPe[sfb];
          
          INDIRECT(1); ADD(1); STORE(1);
          peData->pe += deltaPe;
          
          ADD(1); STORE(1);
          peData->peChannelData[ch].pe += deltaPe;
        }
      }
      
      INDIRECT(1); ADD(1); BRANCH(1);
      if (peData->pe <= desiredPe)
        break;
    }
  }
  
  COUNT_sub_end();
}


static void allowMoreHoles(PSY_OUT_CHANNEL  psyOutChannel[MAX_CHANNELS], 
                           PSY_OUT_ELEMENT *psyOutElement,
                           PE_DATA *peData, 
                           int ahFlag[MAX_CHANNELS][MAX_GROUPED_SFB],
                           const AH_PARAM *ahParam,
                           const int nChannels,
                           const float desiredPe)
{
  int ch, sfb;
  float actPe;

  COUNT_sub_start("allowMoreHoles");

  INDIRECT(1); MOVE(1);
  actPe = peData->pe;

  ADD(2); LOGIC(1); BRANCH(1);
  if (nChannels==2 &&
      psyOutChannel[0].windowSequence==psyOutChannel[1].windowSequence) {
    PSY_OUT_CHANNEL *psyOutChanL = &psyOutChannel[0];
    PSY_OUT_CHANNEL *psyOutChanR = &psyOutChannel[1];

    PTR_INIT(2); /* counting previous operations */

    PTR_INIT(11); /* pointers for ahFlag[0][sfb],
                                  ahFlag[1][sfb],
                                  psyOutElement->toolsInfo.msMask[sfb],
                                  psyOutChanL->sfbMinSnr[sfb],
                                  psyOutChanR->sfbMinSnr[sfb],
                                  psyOutChanL->sfbEnergy[sfb],
                                  psyOutChanR->sfbEnergy[sfb],
                                  psyOutChanL->sfbThreshold[sfb],
                                  psyOutChanR->sfbThreshold[sfb],
                                  peData->peChannelData[0].sfbPe[sfb],
                                  peData->peChannelData[1].sfbPe[sfb]
                  */
    INDIRECT(1); LOOP(1);
    for (sfb=0; sfb<psyOutChanL->sfbCnt; sfb++) {

      BRANCH(1);
      if (psyOutElement->toolsInfo.msMask[sfb]) {

        ADD(2); MULT(2); LOGIC(1); BRANCH(1);
        if (ahFlag[1][sfb] != NO_AH &&
            0.4f*psyOutChanL->sfbMinSnr[sfb]*psyOutChanL->sfbEnergy[sfb] >
            psyOutChanR->sfbEnergy[sfb]) {

          MOVE(1);
          ahFlag[1][sfb] = NO_AH;

          MULT(1); STORE(1);
          psyOutChanR->sfbThreshold[sfb] = 2.0f * psyOutChanR->sfbEnergy[sfb];

          ADD(1);
          actPe -= peData->peChannelData[1].sfbPe[sfb];
        }
        else {
          ADD(2); MULT(2); LOGIC(1); BRANCH(1);
          if (ahFlag[0][sfb] != NO_AH &&
            0.4f*psyOutChanR->sfbMinSnr[sfb]*psyOutChanR->sfbEnergy[sfb] >
            psyOutChanL->sfbEnergy[sfb]) {

          MOVE(1);
          ahFlag[0][sfb] = NO_AH;

          MULT(1); STORE(1);
          psyOutChanL->sfbThreshold[sfb] = 2.0f * psyOutChanL->sfbEnergy[sfb];

          ADD(1);
          actPe -= peData->peChannelData[0].sfbPe[sfb];
          }
        }
        ADD(1); BRANCH(1);
        if (actPe < desiredPe)
          break;
      }
    }
  }

  ADD(1); BRANCH(1);
  if (actPe > desiredPe) {
    int startSfb[2];
    float avgEn, minEn;
    int ahCnt;
    int enIdx;
    float en[4];
    int minSfb, maxSfb;
    int done;

    PTR_INIT(2); /* pointers for psyOutChannel[ch].windowSequence,
                                 startSfb[ch]
                 */
    LOOP(1);
    for (ch=0; ch<nChannels; ch++) {

      ADD(1); BRANCH(1); INDIRECT(1); MOVE(1);
      if (psyOutChannel[ch].windowSequence != SHORT_WINDOW)
        startSfb[ch] = ahParam->startSfbL;
      else
        startSfb[ch] = ahParam->startSfbS;
    }

    MOVE(3);
    avgEn = 0.0f;
    minEn = FLT_MAX;
    ahCnt = 0;

    PTR_INIT(1); /* pointers for startSfb[ch] */
    LOOP(1);
    for (ch=0; ch<nChannels; ch++) {
      PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];

      PTR_INIT(1); /* counting previous operation */

      PTR_INIT(4); /* pointers for ahFlag[ch][sfb],
                                   psyOutChan->sfbMinSnr[sfb],
                                   psyOutChan->sfbEnergy[sfb],
                                   psyOutChan->sfbThreshold[sfb],
                   */
      INDIRECT(1); LOOP(1);
      for (sfb=startSfb[ch]; sfb<psyOutChan->sfbCnt; sfb++){

        ADD(2); LOGIC(1); BRANCH(1);
        if ((ahFlag[ch][sfb]!=NO_AH) &&
            (psyOutChan->sfbEnergy[sfb] > psyOutChan->sfbThreshold[sfb])){

          ADD(1); BRANCH(1); MOVE(1);
          minEn = min(minEn, psyOutChan->sfbEnergy[sfb]);

          ADD(1);
          avgEn += psyOutChan->sfbEnergy[sfb];

          ADD(1);
          ahCnt++;
        }
      }
    }
    ADD(1); BRANCH(1); MOVE(1); /* min() */ ADD(1); DIV(1);
    avgEn = min ( FLT_MAX , avgEn /(ahCnt+FLT_MIN));

    /* calc some energy borders between minEn and avgEn */
    PTR_INIT(1); /* pointers for en[enIdx] */
    LOOP(1);
    for (enIdx=0; enIdx<4; enIdx++) {
      ADD(2); MULT(2); DIV(2); TRANS(1); STORE(1);
      en[enIdx] = minEn * (float)pow(avgEn/(minEn+FLT_MIN), (2*enIdx+1)/7.0f);
    }

    ADD(1);
    maxSfb = psyOutChannel[0].sfbCnt - 1;

    MOVE(1);
    minSfb = startSfb[0];

    ADD(1); BRANCH(1);
    if (nChannels==2) {

      ADD(2); BRANCH(1); MOVE(1);
      maxSfb = max(maxSfb, psyOutChannel[1].sfbCnt-1);

      ADD(1); BRANCH(1); MOVE(1);
      minSfb = min(minSfb, startSfb[1]);
    }

    MOVE(3);
    sfb = maxSfb;
    enIdx = 0;
    done = 0;

    LOOP(1);
    while (!done) {

      PTR_INIT(1); /* pointer for en[enIdx] */

      LOOP(1);
      for (ch=0; ch<nChannels; ch++) {
        PSY_OUT_CHANNEL *psyOutChan = &psyOutChannel[ch];

        PTR_INIT(1); /* counting previous operation */

        PTR_INIT(4); /* pointers for startSfb[ch]
                                     ahFlag[ch][sfb],
                                     psyOutChan->sfbEnergy[sfb],
                                     psyOutChan->sfbThreshold[sfb]
                     */

        INDIRECT(1); ADD(2); LOGIC(1); BRANCH(1);
        if (sfb>=startSfb[ch] && sfb < psyOutChan->sfbCnt) {

          ADD(2); LOGIC(1); BRANCH(1);
          if (ahFlag[ch][sfb] != NO_AH && psyOutChan->sfbEnergy[sfb] < en[enIdx]){
            
            MOVE(1);
            ahFlag[ch][sfb] = NO_AH;
            
            MULT(1); STORE(1);
            psyOutChan->sfbThreshold[sfb] = 2.0f * psyOutChan->sfbEnergy[sfb];
            
            ADD(1);
            actPe -= peData->peChannelData[ch].sfbPe[sfb];
          }

          ADD(1); BRANCH(1);
          if (actPe < desiredPe) {
            
            MOVE(1);
            done = 1;
            break;
          }
        }
      }
      ADD(1);
      sfb--;

      ADD(1); BRANCH(1);
      if (sfb < minSfb) {
        MOVE(1);
        sfb = maxSfb;

        ADD(1);
        enIdx++;

        ADD(1); BRANCH(1);
        if (enIdx >= 4) {

          MOVE(1);
          done = 1;
        }
      }
    }
  }

  COUNT_sub_end();
}


static void adaptThresholdsToPe(PSY_OUT_CHANNEL  psyOutChannel[MAX_CHANNELS],
                                PSY_OUT_ELEMENT* psyOutElement,
                                PE_DATA *peData,
                                const int nChannels,
                                const float desiredPe,
                                AH_PARAM *ahParam,
                                MINSNR_ADAPT_PARAM *msaParam)
{
   float noRedPe, redPe, redPeNoAH;
   float constPart, constPartNoAH;
   float nActiveLines, nActiveLinesNoAH;
   float desiredPeNoAH;
   float avgThrExp, redVal;
   int   ahFlag[MAX_CHANNELS][MAX_GROUPED_SFB];
   float thrExp[MAX_CHANNELS][MAX_GROUPED_SFB];
   int iter;

   COUNT_sub_start("adaptThresholdsToPe");

   FUNC(3);
   calcThreshExp(thrExp, psyOutChannel, nChannels);

   FUNC(3);
   adaptMinSnr(psyOutChannel, msaParam, nChannels);

   FUNC(5);
   initAvoidHoleFlag(ahFlag, psyOutChannel, psyOutElement, nChannels, ahParam);

   INDIRECT(3); MOVE(3);
   noRedPe = peData->pe;
   constPart = peData->constPart;
   nActiveLines = peData->nActiveLines;

   MULT(1); ADD(1); DIV(1); TRANS(1);
   avgThrExp = (float)pow(2.0f, (constPart - noRedPe)/(invRedExp*nActiveLines));

   /* MULT(1); calculated above */ ADD(2); DIV(1); TRANS(1);
   redVal = (float)pow(2.0f, (constPart - desiredPe)/(invRedExp*nActiveLines)) -
            avgThrExp;

   BRANCH(1); MOVE(1);
   redVal = max(0.0f, redVal);

   FUNC(5);
   reduceThresholds(psyOutChannel, ahFlag, thrExp, nChannels, redVal);

   FUNC(3);
   calcPe(peData, psyOutChannel, nChannels);

   INDIRECT(1); MOVE(1);
   redPe = peData->pe;

   MOVE(1);
   iter = 0;

   LOOP(1);
   do {

     PTR_INIT(3); FUNC(7);
     calcPeNoAH(&redPeNoAH, &constPartNoAH, &nActiveLinesNoAH,
                peData, ahFlag, psyOutChannel, nChannels);

     ADD(2); BRANCH(1); MOVE(1);
     desiredPeNoAH = max(desiredPe - (redPe - redPeNoAH), 0);

     BRANCH(1);
     if (nActiveLinesNoAH > 0) {

       MULT(1); ADD(1); DIV(1); TRANS(1);
       avgThrExp = (float)pow(2.0f, (constPartNoAH - redPeNoAH) / (invRedExp*nActiveLinesNoAH));

       /* MULT(1); calculated above */ ADD(3); DIV(1); TRANS(1);
       redVal   += (float)pow(2.0f, (constPartNoAH - desiredPeNoAH) / (invRedExp*nActiveLinesNoAH))
                   - avgThrExp;

       BRANCH(1); MOVE(1);
       redVal = max(0.0f, redVal);

       FUNC(5);
       reduceThresholds(psyOutChannel, ahFlag, thrExp, nChannels, redVal);
     }

     FUNC(3);
     calcPe(peData, psyOutChannel, nChannels);

     INDIRECT(1); MOVE(1);
     redPe = peData->pe;

     ADD(1);
     iter++;

     ADD(3); MISC(1); MULT(1); LOGIC(1); /* while() condition */
   } while ((fabs(redPe - desiredPe) > (0.05f)*desiredPe) && (iter < 2));


   MULT(1); ADD(1); BRANCH(1);
   if (redPe < 1.15f*desiredPe) {

     ADD(1); FUNC(7);
     correctThresh(psyOutChannel, ahFlag, peData, thrExp, redVal,
                   nChannels, desiredPe - redPe);
   }
   else {

     MULT(1); FUNC(5);
     reduceMinSnr(psyOutChannel, peData, ahFlag, nChannels, 1.05f*desiredPe);

     MULT(1); FUNC(7);
     allowMoreHoles(psyOutChannel, psyOutElement, peData, ahFlag,
                    ahParam, nChannels, 1.05f*desiredPe);
   }

   COUNT_sub_end();
}


static float calcBitSave(float fillLevel,
                         const float clipLow,
                         const float clipHigh,
                         const float minBitSave,
                         const float maxBitSave)
{
   float bitsave;

   COUNT_sub_start("calcBitSave");

   ADD(2); BRANCH(2); MOVE(2);
   fillLevel = max(fillLevel, clipLow);
   fillLevel = min(fillLevel, clipHigh);

   ADD(4); DIV(1); MULT(1);
   bitsave = maxBitSave - ((maxBitSave-minBitSave) / (clipHigh-clipLow)) *
                          (fillLevel-clipLow);

   COUNT_sub_end();

   return (bitsave);
}

static float calcBitSpend(float fillLevel,
                          const float clipLow,
                          const float clipHigh,
                          const float minBitSpend,
                          const float maxBitSpend)
{
   float bitspend;

   COUNT_sub_start("calcBitSpend");

   ADD(2); BRANCH(2); MOVE(2);
   fillLevel = max(fillLevel, clipLow);
   fillLevel = min(fillLevel, clipHigh);

   ADD(4); DIV(1); MULT(1);
   bitspend = minBitSpend + ((maxBitSpend-minBitSpend) / (clipHigh-clipLow)) *
                            (fillLevel-clipLow);

   COUNT_sub_end();

   return (bitspend);
}


static void adjustPeMinMax(const float currPe,
                           float      *peMin,
                           float      *peMax)
{
  float minFacHi = 0.3f, maxFacHi = 1.0f, minFacLo = 0.14f, maxFacLo = 0.07f;
  float diff;
  float minDiff = currPe * (float)0.1666666667f;

  COUNT_sub_start("adjustPeMinMax");

  MOVE(4); MULT(1); /* counting previous operations */

  ADD(1); BRANCH(1);
  if (currPe > *peMax) {

     ADD(1);
     diff = (currPe-*peMax) ;

     MAC(2); STORE(2);
     *peMin += diff * minFacHi;
     *peMax += diff * maxFacHi;

  } else {

    ADD(1); BRANCH(1);
    if (currPe < *peMin) {

     ADD(1);
     diff = (*peMin-currPe) ;

     MAC(2); STORE(2);
     *peMin -= diff * minFacLo;
     *peMax -= diff * maxFacLo;
  } else {

     ADD(1); MAC(1); STORE(1);
     *peMin += (currPe - *peMin) * minFacHi;

     ADD(2); MULT(1); STORE(1);
     *peMax -= (*peMax - currPe) * maxFacLo;
  }
  }

  ADD(2); BRANCH(1);
  if ((*peMax - *peMin) < minDiff) {
     float partLo, partHi;

     ADD(2); BRANCH(1); MOVE(1);
     partLo = max((float)0.0f, currPe - *peMin);

     ADD(2); BRANCH(1); MOVE(1);
     partHi = max((float)0.0f, *peMax - currPe);

     ADD(2 * 2); DIV(2); MULT(2); STORE(2);
     *peMax = currPe + partHi/(partLo+partHi) * minDiff;
     *peMin = currPe - partLo/(partLo+partHi) * minDiff;

     ADD(1); BRANCH(1); MOVE(1);
     *peMin = max((float)0.0f, *peMin);
  }

  COUNT_sub_end();
}



static float bitresCalcBitFac( const int       bitresBits,
                                const int       maxBitresBits,
                                const float    pe,
                                const int       windowSequence,
                                const int       avgBits,
                                const float    maxBitFac,
                                ADJ_THR_STATE   *AdjThr,
                                ATS_ELEMENT     *adjThrChan)
{
   BRES_PARAM *bresParam;
   float pex;
   float fillLevel;
   float bitSave, bitSpend, bitresFac;

   COUNT_sub_start("bitresCalcBitFac");

   DIV(1);
   fillLevel = (float)bitresBits / (float)maxBitresBits;


   ADD(1); BRANCH(1); INDIRECT(1); PTR_INIT(1);
   if (windowSequence != SHORT_WINDOW)
      bresParam = &(AdjThr->bresParamLong);
   else
      bresParam = &(AdjThr->bresParamShort);

   INDIRECT(1); ADD(1); BRANCH(1); MOVE(1);
   pex = max(pe, adjThrChan->peMin);

   INDIRECT(1); ADD(1); BRANCH(1); MOVE(1);
   pex = min(pex,adjThrChan->peMax);

   INDIRECT(4); FUNC(5);
   bitSave = calcBitSave(fillLevel,
                         bresParam->clipSaveLow, bresParam->clipSaveHigh,
                         bresParam->minBitSave, bresParam->maxBitSave);

   INDIRECT(4); FUNC(5);
   bitSpend = calcBitSpend(fillLevel,
                           bresParam->clipSpendLow, bresParam->clipSpendHigh,
                           bresParam->minBitSpend, bresParam->maxBitSpend);

   INDIRECT(2); ADD(5); MULT(1); DIV(1);
   bitresFac = (float)1.0f - bitSave +
               ((bitSpend + bitSave) / (adjThrChan->peMax - adjThrChan->peMin)) *
               (pex - adjThrChan->peMin);

   /* limit bitresFac for small bitreservoir */
   ADD(3); DIV(1); BRANCH(1); MOVE(1);
   bitresFac = min(bitresFac, (float)1.0f - (float)0.3f + (float)bitresBits/(float)avgBits);

   /* limit bitresFac for high bitrates */
   ADD(1); BRANCH(1); MOVE(1);
   bitresFac = min(bitresFac, maxBitFac);

   INDIRECT(2); PTR_INIT(2); FUNC(3);
   adjustPeMinMax(pe, &adjThrChan->peMin, &adjThrChan->peMax);

   COUNT_sub_end();

   return bitresFac;
}



void AdjThrInit(ADJ_THR_STATE  *hAdjThr,
                const float    meanPe,
                int            chBitrate)
{
  ATS_ELEMENT* atsElem = &hAdjThr->adjThrStateElem;
  MINSNR_ADAPT_PARAM *msaParam = &atsElem->minSnrAdaptParam;

  COUNT_sub_start("AdjThrInit");

  MOVE(1); PTR_INIT(2); /* counting previous operations */

  /* common for all elements: */
  /* parameters for bitres control */
  INDIRECT(8); MOVE(8);
  hAdjThr->bresParamLong.clipSaveLow   =  0.2f;
  hAdjThr->bresParamLong.clipSaveHigh  =  0.95f;
  hAdjThr->bresParamLong.minBitSave    = -0.05f;
  hAdjThr->bresParamLong.maxBitSave    =  0.3f;
  hAdjThr->bresParamLong.clipSpendLow  =  0.2f;
  hAdjThr->bresParamLong.clipSpendHigh =  0.95f;
  hAdjThr->bresParamLong.minBitSpend   = -0.10f;
  hAdjThr->bresParamLong.maxBitSpend   =  0.4f;
  
  INDIRECT(8); MOVE(8);
  hAdjThr->bresParamShort.clipSaveLow   =  0.2f;
  hAdjThr->bresParamShort.clipSaveHigh  =  0.75f;
  hAdjThr->bresParamShort.minBitSave    = 0.0f;
  hAdjThr->bresParamShort.maxBitSave    =  0.2f;
  hAdjThr->bresParamShort.clipSpendLow  =  0.2f;
  hAdjThr->bresParamShort.clipSpendHigh =  0.75f;
  hAdjThr->bresParamShort.minBitSpend   = -0.05f;
  hAdjThr->bresParamShort.maxBitSpend   =  0.5f ;
  
  INDIRECT(2); MULT(2); STORE(2);
  atsElem->peMin = (float)0.8f * meanPe;
  atsElem->peMax = (float)1.2f * meanPe;
  
  INDIRECT(1); MOVE(1);
  atsElem->peOffset = 0.0f;
  
  ADD(1); BRANCH(1);
  if (chBitrate < 32000) {
    INDIRECT(1); MULT(1); ADD(2); BRANCH(1); MOVE(1);
    atsElem->peOffset = max((float)50.0f, (float)(100.0f)-(float)(100.0f/32000)*(float)chBitrate);
  }
  
  /* avoid hole parameters */
  ADD(1); BRANCH(1);
  if (chBitrate > 20000) {
    INDIRECT(3); MOVE(3);
    atsElem->ahParam.modifyMinSnr = TRUE;
    atsElem->ahParam.startSfbL = 15;
    atsElem->ahParam.startSfbS = 3;
  }
  else {
    INDIRECT(3); MOVE(3);
    atsElem->ahParam.modifyMinSnr = FALSE;
    atsElem->ahParam.startSfbL = 0;
    atsElem->ahParam.startSfbS = 0;
  }
  
  /* minSnr adaptation */
  /* maximum reduction of minSnr goes down to minSnr^maxRed */
  INDIRECT(1); MOVE(1);
  msaParam->maxRed = (float)0.25f;
  
  /* start adaptation of minSnr for avgEn/sfbEn > startRatio */
  INDIRECT(1); MOVE(1);
  msaParam->startRatio = (float)1.e1f;
  
  /* maximum minSnr reduction to minSnr^maxRed is reached for
     avgEn/sfbEn >= maxRatio */
  INDIRECT(1); MOVE(1);
  msaParam->maxRatio = (float)1.e3f;
  
  /* helper variables to interpolate minSnr reduction for
     avgEn/sfbEn between startRatio and maxRatio */
  INDIRECT(4); ADD(1); MULT(1); DIV(1); TRANS(1); STORE(1);
  msaParam->redRatioFac = (1.0f - msaParam->maxRed) /
    (10.0f*(float)log10(msaParam->startRatio/msaParam->maxRatio));
  
  INDIRECT(3); MULT(2); TRANS(1); ADD(1); STORE(1);
  msaParam->redOffs = 1.0f - msaParam->redRatioFac *
    10.0f * (float) log10(msaParam->startRatio);
  
  /* pe correction */
  INDIRECT(3); MOVE(3);
  atsElem->peLast = (float)0.0f;
  atsElem->dynBitsLast = 0;
  atsElem->peCorrectionFactor = (float)1.0f;

  COUNT_sub_end();
}


static void calcPeCorrection(float *correctionFac,
                             const float peAct,
                             const float peLast, 
                             const int bitsLast) 
{
  COUNT_sub_start("calcPeCorrection");

  ADD(4); FUNC(1); FUNC(1); MULT(4); LOGIC(4); BRANCH(1);
  if ((bitsLast > 0) && (peAct < (float)1.5f*peLast) && (peAct > (float)0.7f*peLast) &&
      ((float)1.2f*bits2pe((float)bitsLast) > peLast) && 
      ((float)0.65f*bits2pe((float)bitsLast) < peLast))
  {
    float newFac = peLast / bits2pe((float)bitsLast);

    FUNC(1); DIV(1); /* counting previous operation */

    /* dead zone */
    ADD(1); BRANCH(1);
    if (newFac < (float)1.0f) {

      MULT(1); ADD(1); BRANCH(1); MOVE(1);
      newFac = min((float)1.1f*newFac, (float)1.0f);

      ADD(1); BRANCH(1); MOVE(1);
      newFac = max(newFac, (float)0.85f);
    }
    else {
      MULT(1); ADD(1); BRANCH(1); MOVE(1);
      newFac = max((float)0.9f*newFac, (float)1.0f);

      ADD(1); BRANCH(1); MOVE(1);
      newFac = min(newFac, (float)1.15f);
    }
    ADD(4); LOGIC(3); BRANCH(1);
    if (((newFac > (float)1.0f) && (*correctionFac < (float)1.0f)) ||
        ((newFac < (float)1.0f) && (*correctionFac > (float)1.0f))) {

      MOVE(1);
      *correctionFac = (float)1.0f;
    }

    /* faster adaptation towards 1.0, slower in the other direction */
    ADD(3); LOGIC(3); BRANCH(1); /* if() */ MULT(1); MAC(1); STORE(1);
    if ((*correctionFac < (float)1.0f && newFac < *correctionFac) ||
        (*correctionFac > (float)1.0f && newFac > *correctionFac))
      *correctionFac = (float)0.85f * (*correctionFac) + (float)0.15f * newFac;
    else
      *correctionFac = (float)0.7f * (*correctionFac) + (float)0.3f * newFac;

    ADD(2); BRANCH(2); MOVE(2);
    *correctionFac = min(*correctionFac, (float)1.15f);
    *correctionFac = max(*correctionFac, (float)0.85f);
  }
  else {
    MOVE(1);
    *correctionFac = (float)1.0f;
  }

  COUNT_sub_end();
}


void AdjustThresholds(ADJ_THR_STATE     *adjThrState,
                      ATS_ELEMENT       *AdjThrStateElement,
                      PSY_OUT_CHANNEL   psyOutChannel[MAX_CHANNELS],
                      PSY_OUT_ELEMENT   *psyOutElement,
                      float             *chBitDistribution,
                      float            sfbFormFactor[MAX_CHANNELS][MAX_GROUPED_SFB],
                      const int         nChannels,
                      QC_OUT_ELEMENT    *qcOE,
                      const int         avgBits,
                      const int         bitresBits,
                      const int         maxBitresBits,
                      const float       maxBitFac,
                      const int         sideInfoBits)
{
   float noRedPe, grantedPe, grantedPeCorr;
   int curWindowSequence;
   PE_DATA peData;
   float bitFactor;
   int ch;

   COUNT_sub_start("AdjustThresholds");

   INDIRECT(1); PTR_INIT(1); FUNC(5);
   preparePe(&peData, psyOutChannel, sfbFormFactor, nChannels, AdjThrStateElement->peOffset);

   PTR_INIT(1); FUNC(3);
   calcPe(&peData, psyOutChannel, nChannels);

   MOVE(1);
   noRedPe = peData.pe;

   MOVE(1);
   curWindowSequence = LONG_WINDOW;

   ADD(1); BRANCH(1);
   if (nChannels==2) {

     ADD(2); LOGIC(1);
     if ((psyOutChannel[0].windowSequence == SHORT_WINDOW) ||
         (psyOutChannel[1].windowSequence == SHORT_WINDOW)) {

       MOVE(1);
       curWindowSequence = SHORT_WINDOW;
     }
   }
   else {

     MOVE(1);
     curWindowSequence = psyOutChannel[0].windowSequence;
   }


   MULT(1); ADD(1); FUNC(8);
   bitFactor = bitresCalcBitFac(bitresBits, maxBitresBits,
                                noRedPe+5.0f*sideInfoBits,
                                curWindowSequence, avgBits, maxBitFac,
                                adjThrState,
                                AdjThrStateElement);

   FUNC(1); MULT(1);
   grantedPe = bitFactor * bits2pe((float)avgBits);

   INDIRECT(3); ADD(1); BRANCH(1); MOVE(1); /* min() */ PTR_INIT(1); FUNC(4);
   calcPeCorrection(&(AdjThrStateElement->peCorrectionFactor), 
                    min(grantedPe, noRedPe),
                    AdjThrStateElement->peLast, 
                    AdjThrStateElement->dynBitsLast);

   INDIRECT(1); MULT(1);
   grantedPeCorr = grantedPe * AdjThrStateElement->peCorrectionFactor;

   ADD(1); BRANCH(1);
   if (grantedPeCorr < noRedPe) {

      INDIRECT(2); PTR_INIT(3); FUNC(7);
      adaptThresholdsToPe(psyOutChannel,
                          psyOutElement,
                          &peData,
                          nChannels,
                          grantedPeCorr,
                          &AdjThrStateElement->ahParam,
                          &AdjThrStateElement->minSnrAdaptParam);
   }

   PTR_INIT(2); /* pointers for chBitDistribution[],
                                peData.peChannelData[]
                */
   LOOP(1);
   for (ch=0; ch<nChannels; ch++) {

     BRANCH(1);
     if (peData.pe) {

       DIV(1); MULT(2); ADD(2); STORE(1);
       chBitDistribution[ch] = 0.2f + (float)(1.0f-nChannels*0.2f) * (peData.peChannelData[ch].pe/peData.pe);
     } else {

       MOVE(1);
       chBitDistribution[ch] = 0.2f;
     }
   }

   INDIRECT(1); MOVE(1);
   qcOE->pe= noRedPe;

   INDIRECT(1); MOVE(1);
   AdjThrStateElement->peLast = grantedPe;

   COUNT_sub_end();
}

void AdjThrUpdate(ATS_ELEMENT *AdjThrStateElement,
                  const int dynBitsUsed)
{
   COUNT_sub_start("AdjThrUpdate");

   INDIRECT(1); MOVE(1);
   AdjThrStateElement->dynBitsLast = dynBitsUsed;

   COUNT_sub_end();
}
