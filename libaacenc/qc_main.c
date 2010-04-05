/*
  Quantizing & coding main module
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qc_main.h"
#include "quantize.h"
#include "interface.h"
#include "adj_thr.h"
#include "sf_estim.h"
#include "stat_bits.h"
#include "bit_cnt.h"
#include "dyn_bits.h"
#include "minmax.h"
#include "channel_map.h"
#include "aac_ram.h"

#include <math.h>

#include "counters.h" /* the 3GPP instrumenting tool */

typedef enum{
    FRAME_LEN_BYTES_MODULO =  1,
    FRAME_LEN_BYTES_INT    =  2
}FRAME_LEN_RESULT_MODE;

static const int maxFillElemBits = 7 + 270*8;

/* forward declarations */

static int calcMaxValueInSfb(int sfbCnt,
                             int maxSfbPerGroup,
                             int sfbPerGroup,
                             int sfbOffset[MAX_GROUPED_SFB],
                             short quantSpectrum[FRAME_LEN_LONG],
                             unsigned short maxValue[MAX_GROUPED_SFB]);


/*****************************************************************************

    functionname: calcFrameLen
    description:
    returns:
    input:
    output:

*****************************************************************************/
static int calcFrameLen(int bitRate,
                        int sampleRate,
                        FRAME_LEN_RESULT_MODE mode)
{

   int result;

   COUNT_sub_start("calcFrameLen");

   MULT(1);
   result = ((FRAME_LEN_LONG)>>3)*(bitRate);

   BRANCH(2);
   switch(mode) {
     case FRAME_LEN_BYTES_MODULO:
         DIV(1);
         result %= sampleRate;
     break;
     case FRAME_LEN_BYTES_INT:
         DIV(1);
         result /= sampleRate;
     break;
   }

   COUNT_sub_end();

   return(result);
}

/*****************************************************************************

    functionname:framePadding
    description: Calculates if padding is needed for actual frame
    returns:
    input:
    output:

*****************************************************************************/
static int framePadding(int bitRate,
                        int sampleRate,
                        int *paddingRest)
{
  int paddingOn;
  int difference;

  COUNT_sub_start("framePadding");

  MOVE(1);
  paddingOn = 0;

  FUNC(3);
  difference = calcFrameLen( bitRate,
                             sampleRate,
                             FRAME_LEN_BYTES_MODULO );

  ADD(1); STORE(1);
  *paddingRest-=difference;

  BRANCH(1);
  if (*paddingRest <= 0 ) {

    MOVE(1);
    paddingOn = 1;

    ADD(1); STORE(1);
    *paddingRest += sampleRate;
  }

  COUNT_sub_end();

  return( paddingOn );
}


/*********************************************************************************

         functionname: QCOutNew
         description:
         return:

**********************************************************************************/

int QCOutNew(QC_OUT *hQC, int nChannels)
{
  int error=0;
  int i;

  COUNT_sub_start("QCOutNew");

  MOVE(1); /* counting previous operation */

  PTR_INIT(4); /* hQC->qcChannel[]
                  quantSpec[]
                  maxValueInSfb[]
                  scf[]
               */
  LOOP(1);
  for (i=0; i<nChannels; i++) {
    PTR_INIT(1);
    hQC->qcChannel[i].quantSpec = &quantSpec[i*FRAME_LEN_LONG];

    PTR_INIT(1);
    hQC->qcChannel[i].maxValueInSfb = &maxValueInSfb[i*MAX_GROUPED_SFB];

    PTR_INIT(1);
    hQC->qcChannel[i].scf = &scf[i*MAX_GROUPED_SFB];
  }
 
  
  BRANCH(1);
  if (error){
    FUNC(1);
    QCOutDelete(hQC);

    MOVE(1);
    hQC = 0;
    }

  LOGIC(1); /* counting post-operation */

  COUNT_sub_end();

  return (hQC == 0);
}


/*********************************************************************************

         functionname: QCOutDelete
         description:
         return:

**********************************************************************************/
void QCOutDelete(QC_OUT* hQC)
{
  COUNT_sub_start("QCOutDelete");

  /* 
    nothing to do
  */
 
  MOVE(1);
  hQC=NULL;

  COUNT_sub_end();
}

/*********************************************************************************

         functionname: QCNew
         description:
         return:

**********************************************************************************/
int QCNew(QC_STATE *hQC)
{
  
  COUNT_sub_start("QCNew");

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(QC_STATE));
  memset(hQC,0,sizeof(QC_STATE));

  COUNT_sub_end();

  return (0);
}

/*********************************************************************************

         functionname: QCDelete
         description:
         return:

**********************************************************************************/
void QCDelete(QC_STATE *hQC)
{
 
  COUNT_sub_start("QCDelete");

  /* 
    nothing to do
  */
  PTR_INIT(1);
  hQC=NULL;

  COUNT_sub_end();
}

/*********************************************************************************

         functionname: QCInit
         description:
         return:

**********************************************************************************/
int QCInit(QC_STATE *hQC,
           struct QC_INIT *init)
{
  COUNT_sub_start("QCInit");

  INDIRECT(6); MOVE(3);
  hQC->nChannels       = init->elInfo->nChannelsInEl;
  hQC->maxBitsTot      = init->maxBits;

  INDIRECT(3); ADD(1); STORE(1);
  hQC->bitResTot       = init->bitRes - init->averageBits;

  INDIRECT(6); MOVE(3);
  hQC->averageBitsTot  = init->averageBits;
  hQC->maxBitFac       = init->maxBitFac;
  hQC->padding.paddingRest = init->padding.paddingRest;

  INDIRECT(1); MOVE(1);
  hQC->globStatBits    = 3;                                  /* for ID_END */

  INDIRECT(5); FUNC(5);
  InitElementBits(&hQC->elementBits,
                  *init->elInfo,
                  init->bitrate,
                  init->averageBits,
                  hQC->globStatBits);

  INDIRECT(3); FUNC(3);
  AdjThrInit(&hQC->adjThr,
             init->meanPe,
             hQC->elementBits.chBitrate);

  FUNC(0);
  BCInit();

  COUNT_sub_end();

  return 0;
}


/*********************************************************************************

         functionname: QCMain
         description:
         return:

**********************************************************************************/
int QCMain(QC_STATE* hQC,
           int nChannels,
           ELEMENT_BITS* elBits,
           ATS_ELEMENT* adjThrStateElement,
           PSY_OUT_CHANNEL  psyOutChannel[MAX_CHANNELS],  /* may be modified in-place */
           PSY_OUT_ELEMENT* psyOutElement,
           QC_OUT_CHANNEL  qcOutChannel[MAX_CHANNELS],    /* out                      */
           QC_OUT_ELEMENT* qcOutElement,
           int ancillaryDataBytes)      
{
  int ch;
  float sfbFormFactor[MAX_CHANNELS][MAX_GROUPED_SFB];
  float sfbNRelevantLines[MAX_CHANNELS][MAX_GROUPED_SFB];
  int maxChDynBits[MAX_CHANNELS];
  float chBitDistribution[MAX_CHANNELS];  
  int loopCnt=0;

  COUNT_sub_start("QCMain");

  INDIRECT(1); BRANCH(1);
  if (elBits->bitResLevel < 0) {
#ifdef DEBUG
     fprintf(stderr, "\ntoo little bits in bitres\n");
#endif
     return -1;
  }

  INDIRECT(2); ADD(1); BRANCH(1);
  if (elBits->bitResLevel > elBits->maxBitResBits) {
#ifdef DEBUG
     fprintf(stderr, "\ntoo many bits in bitres\n");
#endif
     return -1;
  }

  INDIRECT(1); FUNC(3); STORE(1);
  qcOutElement->staticBitsUsed = countStaticBitdemand(psyOutChannel,
                                                      psyOutElement,
                                                      nChannels);

  
  BRANCH(1);
  if (ancillaryDataBytes) {

    INDIRECT(1); LOGIC(1); ADD(2); MULT(1); STORE(1);
    qcOutElement->ancBitsUsed = 7+8*(ancillaryDataBytes + (ancillaryDataBytes >=15));
  } else {

    INDIRECT(1); MOVE(1);
    qcOutElement->ancBitsUsed = 0;
  }

  FUNC(4);
  CalcFormFactor(sfbFormFactor,sfbNRelevantLines, psyOutChannel, nChannels);

  INDIRECT(9); PTR_INIT(1); ADD(3); FUNC(13);
  AdjustThresholds(&hQC->adjThr,
                  adjThrStateElement,
                  psyOutChannel,
                  psyOutElement,
                  chBitDistribution,
                  sfbFormFactor,
                  nChannels,
                  qcOutElement,
                  elBits->averageBits-qcOutElement->staticBitsUsed - qcOutElement->ancBitsUsed,
                  elBits->bitResLevel,
                  elBits->maxBits,
                  hQC->maxBitFac,
                  qcOutElement->staticBitsUsed+qcOutElement->ancBitsUsed);

  FUNC(5);
  EstimateScaleFactors(psyOutChannel,
                       qcOutChannel,
                       sfbFormFactor,
                       sfbNRelevantLines,
                       nChannels);


  /* condition to prevent empty bitreservoir */
  PTR_INIT(2); /* pointers for maxChDynBits[],
                               chBitDistribution[]
               */
  LOOP(1);
  for (ch = 0; ch < nChannels; ch++) {

      INDIRECT(4); MULT(1); ADD(4); MISC(1); STORE(1);
      maxChDynBits[ch] = (int)floor(chBitDistribution[ch] * (float)
                                    (elBits->averageBits + elBits->bitResLevel - 7 -
                                     qcOutElement->staticBitsUsed - qcOutElement->ancBitsUsed));
                                 
      /* -7 bec. of align bits */

  }

  INDIRECT(1); MOVE(1);
  qcOutElement->dynBitsUsed = 0;

  PTR_INIT(3); /* pointers for psyOutChannel[],
                               qcOutChannel[],
                               maxChDynBits[]
               */
  LOOP(1);
  for (ch = 0; ch < nChannels; ch++)
  {
    /* now loop until bitstream constraints (chDynBits < maxChDynBits)
       are fulfilled */
    int chDynBits;
    int constraintsFulfilled;
    int iter=0;

    MOVE(1); /* counting previous operation */

    do
    {
        MOVE(1);
        constraintsFulfilled = 1;

        BRANCH(1);
        if (iter>0) {

          FUNC(8);
          QuantizeSpectrum(psyOutChannel[ch].sfbCnt,
                           psyOutChannel[ch].maxSfbPerGroup,
                           psyOutChannel[ch].sfbPerGroup,
                           psyOutChannel[ch].sfbOffsets,
                           psyOutChannel[ch].mdctSpectrum,
                           qcOutChannel[ch].globalGain,
                           qcOutChannel[ch].scf,
                           qcOutChannel[ch].quantSpec);
        }

        FUNC(6); ADD(1); BRANCH(1);
        if (calcMaxValueInSfb(psyOutChannel[ch].sfbCnt,
                                psyOutChannel[ch].maxSfbPerGroup,
                                psyOutChannel[ch].sfbPerGroup,
                                psyOutChannel[ch].sfbOffsets,
                                qcOutChannel[ch].quantSpec,
                                qcOutChannel[ch].maxValueInSfb) > MAX_QUANT)
        {
#ifdef DEBUG
            fprintf(stderr,"\nMAXQUANT violated\n");
#endif
            MOVE(1);
            constraintsFulfilled=0;
        }

        PTR_INIT(1); FUNC(9);
        chDynBits =
            dynBitCount(qcOutChannel[ch].quantSpec,
                        qcOutChannel[ch].maxValueInSfb,
                        qcOutChannel[ch].scf,
                        psyOutChannel[ch].windowSequence,
                        psyOutChannel[ch].sfbCnt,
                        psyOutChannel[ch].maxSfbPerGroup,
                        psyOutChannel[ch].sfbPerGroup,
                        psyOutChannel[ch].sfbOffsets,
                        &qcOutChannel[ch].sectionData);

        ADD(1); BRANCH(1);
        if (chDynBits >= maxChDynBits[ch])
        {
            MOVE(1);
            constraintsFulfilled = 0;
#ifdef DEBUG
            fprintf(stderr, "\nWARNING: too many dynBits, extra quantization necessary\n");
#endif
        }

        BRANCH(1);
        if (!constraintsFulfilled)
        {
          ADD(1); STORE(1);
          qcOutChannel[ch].globalGain++;
        }

        ADD(2);
        iter++;
        loopCnt+=100;


    } while(!constraintsFulfilled);

    INDIRECT(1); ADD(1); STORE(1);
    qcOutElement->dynBitsUsed += chDynBits;
    

    MOVE(2);
    qcOutChannel[ch].groupingMask = psyOutChannel[ch].groupingMask;
    qcOutChannel[ch].windowShape  = psyOutChannel[ch].windowShape;
  }

  /* save dynBitsUsed for correction of bits2pe relation */
  INDIRECT(1); FUNC(2);
  AdjThrUpdate(adjThrStateElement, qcOutElement->dynBitsUsed);

  {
    int bitResSpace = elBits->maxBitResBits - elBits->bitResLevel;
    int deltaBitRes = elBits->averageBits - (qcOutElement->staticBitsUsed
                                             + qcOutElement->dynBitsUsed + qcOutElement->ancBitsUsed);

    INDIRECT(6); ADD(4); /* counting previous operations */

    INDIRECT(1); ADD(2); BRANCH(1); MOVE(1);
    qcOutElement->fillBits = max(0,(deltaBitRes - bitResSpace));
  }

  COUNT_sub_end();

  return 0; /* OK */
}


/*********************************************************************************

         functionname: calcMaxValueInSfb
         description:
         return:

**********************************************************************************/

static int calcMaxValueInSfb(int sfbCnt,
                             int maxSfbPerGroup,
                             int sfbPerGroup,
                             int sfbOffset[MAX_GROUPED_SFB],
                             short quantSpectrum[FRAME_LEN_LONG],
                             unsigned short maxValue[MAX_GROUPED_SFB])
{
  int sfbOffs,sfb;
  int maxValueAll = 0;

  COUNT_sub_start("calcMaxValueInSfb");

  MOVE(1); /* counting previous operation */

  LOOP(1);
  for(sfbOffs=0;sfbOffs<sfbCnt;sfbOffs+=sfbPerGroup)
  {

  PTR_INIT(2); /* pointers for sfbOffset[],
                               maxValue[]
               */
  LOOP(1);
  for (sfb = 0; sfb < maxSfbPerGroup; sfb++)
  {
    int line;
    int maxThisSfb = 0;

    MOVE(1); /* counting previous operation */
 
    PTR_INIT(1); /* pointers for quantSpectrum[] */
    LOOP(1);
    for (line = sfbOffset[sfbOffs+sfb]; line < sfbOffset[sfbOffs+sfb+1]; line++)
    {
      MISC(1); ADD(1); BRANCH(1);
      if (abs(quantSpectrum[line]) > maxThisSfb)
      {
        MOVE(1);
        maxThisSfb = abs(quantSpectrum[line]);
      }
    }

    MOVE(1);
    maxValue[sfbOffs+sfb] = maxThisSfb;

    ADD(1); BRANCH(1);
    if (maxThisSfb > maxValueAll)
    {
      MOVE(1);
      maxValueAll = maxThisSfb;
    }
  }
  }

  COUNT_sub_end();

  return maxValueAll;
}


/*********************************************************************************

         functionname: updateBitres
         description:
         return:

**********************************************************************************/
void UpdateBitres(QC_STATE* qcKernel,
                  QC_OUT*   qcOut)
                  
{
  ELEMENT_BITS* elBits;

  COUNT_sub_start("updateBitres");
 
  INDIRECT(1); MOVE(1);
  qcKernel->bitResTot=0;

  INDIRECT(1); PTR_INIT(1);
  elBits = &qcKernel->elementBits;
  
  INDIRECT(1); BRANCH(1);
  if (elBits->averageBits > 0) {
    
    /* constant bitrate */
    INDIRECT(2); ADD(5); STORE(1);
    elBits->bitResLevel += 
      elBits->averageBits - (qcOut->qcElement.staticBitsUsed
                             + qcOut->qcElement.dynBitsUsed
                             + qcOut->qcElement.ancBitsUsed
                             + qcOut->qcElement.fillBits);
    
    INDIRECT(2); ADD(1); STORE(1);
    qcKernel->bitResTot += elBits->bitResLevel;
  }
  else {
    
    /* variable bitrate */
    INDIRECT(4); MOVE(2);
    elBits->bitResLevel = elBits->maxBits;
    qcKernel->bitResTot = qcKernel->maxBitsTot;
  }

  COUNT_sub_end();
}

/*********************************************************************************

         functionname: FinalizeBitConsumption
         description:
         return:

**********************************************************************************/
int FinalizeBitConsumption( QC_STATE *qcKernel,
                            QC_OUT* qcOut)
{
  int nFullFillElem, diffBits;
  int totFillBits = 0;

  COUNT_sub_start("FinalizeBitConsumption");

  MOVE(2); /* prev. instructions */

  INDIRECT(5); MOVE(4);
  qcOut->totStaticBitsUsed = qcKernel->globStatBits;
  qcOut->totDynBitsUsed = 0;
  qcOut->totAncBitsUsed = 0;
  qcOut->totFillBits=0;

  PTR_INIT(1); /* qcOut->qcElement[] */
  {
    INDIRECT(4); ADD(4); STORE(4);
    qcOut->totStaticBitsUsed += qcOut->qcElement.staticBitsUsed;
    qcOut->totDynBitsUsed    += qcOut->qcElement.dynBitsUsed;
    qcOut->totAncBitsUsed    += qcOut->qcElement.ancBitsUsed;
    qcOut->totFillBits       += qcOut->qcElement.fillBits;

    BRANCH(1);
    if (qcOut->qcElement.fillBits) {

      ADD(1); STORE(1);
      totFillBits += qcOut->qcElement.fillBits;
    }
  }

  INDIRECT(1); ADD(1); DIV(1);
  nFullFillElem = (qcOut->totFillBits-1) / maxFillElemBits;

  BRANCH(1);
  if (nFullFillElem) {

    INDIRECT(1); MULT(1); ADD(1); STORE(1);
    qcOut->totFillBits -= nFullFillElem * maxFillElemBits;
  }

  /* check fill elements */
  INDIRECT(1); BRANCH(1);
  if (qcOut->totFillBits > 0) {

    /* minimum Fillelement contains 7 (TAG + byte cnt) bits */
    ADD(1); BRANCH(1); MOVE(1);
    qcOut->totFillBits = max(7, qcOut->totFillBits);

    /* fill element size equals n*8 + 7 */
    ADD(2); LOGIC(2); /* .. % 8 */ STORE(1);
    qcOut->totFillBits += ((8-(qcOut->totFillBits-7)%8)%8);
  }

  MAC(1); STORE(1);
  qcOut->totFillBits += nFullFillElem * maxFillElemBits;

  /* now distribute extra fillbits and alignbits over channel elements */
  INDIRECT(5); ADD(5); LOGIC(1); /* .. % 8 */
  qcOut->alignBits = 7 - (qcOut->totDynBitsUsed + qcOut->totStaticBitsUsed + qcOut->totAncBitsUsed + 
                          + qcOut->totFillBits -1)%8;

  INDIRECT(3); ADD(4); LOGIC(1); BRANCH(1);
  if( ((qcOut->alignBits + qcOut->totFillBits - totFillBits)==8) && (qcOut->totFillBits>8) ) {
    INDIRECT(1); ADD(1); STORE(1);
    qcOut->totFillBits -= 8;
  }

  STORE(1); INDIRECT(2); ADD(2);
  diffBits = (qcOut->alignBits + qcOut->totFillBits) - totFillBits;
  
  BRANCH(1);
  if (diffBits) {

    BRANCH(1);
    if(diffBits<0) {
#ifdef DEBUG
      fprintf(stderr,"\nbuffer fullness underflow \n");
#endif
      COUNT_sub_end();
      return -1;
    }
    else {

      ADD(1); STORE(1); INDIRECT(1);
      qcOut->qcElement.fillBits += diffBits;

    }
  }

  
  INDIRECT(6); ADD(5); BRANCH(1);
  if ((qcOut->totDynBitsUsed + qcOut->totStaticBitsUsed + qcOut->totAncBitsUsed + 
       qcOut->totFillBits +  qcOut->alignBits) > qcKernel->maxBitsTot) {
#ifdef DEBUG
    fprintf(stderr,"\ntoo many bits used in Q&C\n");
#endif

    COUNT_sub_end();
    return -1;
  }

  COUNT_sub_end();
  return 0;
}


/*********************************************************************************

         functionname: AdjustBitrate
         description:  adjusts framelength via padding on a frame to frame basis,
                       to achieve a bitrate that demands a non byte aligned
                       framelength
         return:       errorcode

**********************************************************************************/
int AdjustBitrate(QC_STATE* hQC,
                  int       bitRate,    /* total bitrate */
                  int       sampleRate  /* output sampling rate */
                  )
{
  int paddingOn;
  int frameLen;
  int codeBits;
  int codeBitsLast;

  COUNT_sub_start("AdjustBitrate");

  /* Do we need a extra padding byte? */
  INDIRECT(1); PTR_INIT(1); FUNC(3);
  paddingOn = framePadding(bitRate,
                           sampleRate,
                           &hQC->padding.paddingRest);

  FUNC(3); ADD(1);
  frameLen = paddingOn + calcFrameLen(bitRate,
                                      sampleRate,
                                      FRAME_LEN_BYTES_INT);

  SHIFT(1);
  frameLen <<= 3;

  INDIRECT(2); ADD(1);
  codeBitsLast = hQC->averageBitsTot - hQC->globStatBits;

  INDIRECT(1); ADD(1);
  codeBits     = frameLen - hQC->globStatBits;


  /* calculate bits for every channel element */
  ADD(1); BRANCH(1);
  if (codeBits != codeBitsLast) {
    int totalBits = 0;

    MOVE(1); /* counting previous operation */

    MULT(1); STORE(1);
    hQC->elementBits.averageBits =  (int)(hQC->elementBits.relativeBits * codeBits);
    
    ADD(1);
    totalBits += hQC->elementBits.averageBits;

    ADD(2); STORE(1);
    hQC->elementBits.averageBits += codeBits - totalBits;
  }

  INDIRECT(1); MOVE(2);
  hQC->averageBitsTot = frameLen;

  COUNT_sub_end();

  return 0;
}

