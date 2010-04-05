/*
  Perceptual entropie module
*/
#ifndef __LINE_PE_H
#define __LINE_PE_H


#include "psy_const.h" 


typedef struct {
   /* these two are calculated by prepareSfbPe */
   float sfbLdEnergy[MAX_GROUPED_SFB];    /* log(sfbEnergy)/log(2) */
   float  sfbNLines[MAX_GROUPED_SFB];         /* number of relevant lines in sfb */
   /* the rest is calculated by calcSfbPe */
   float sfbPe[MAX_GROUPED_SFB];           /* pe for each sfb */
   float sfbConstPart[MAX_GROUPED_SFB];    /* constant part for each sfb */
   float     sfbNActiveLines[MAX_GROUPED_SFB];   /* number of active lines in sfb */
   float pe;                               /* sum of sfbPe */
   float constPart;                        /* sum of sfbConstPart */
   float nActiveLines;                         /* sum of sfbNActiveLines */
} PE_CHANNEL_DATA;


void prepareSfbPe(PE_CHANNEL_DATA *peChanData,
                  const float *sfbEnergy,
                  const float *sfbThreshold,
                  const float  *sfbFormFactor,
                  const int     *sfbOffset,
                  const int     sfbCnt,
                  const int     sfbPerGroup,
                  const int     maxSfbPerGroup);

void calcSfbPe(PE_CHANNEL_DATA *peChanData,
               const float *sfbEnergy,
               const float *sfbThreshold,
               const int    sfbCnt,
               const int    sfbPerGroup,
               const int    maxSfbPerGroup);

#endif 
