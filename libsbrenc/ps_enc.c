/*
  parametric stereo encoding
*/
#include <math.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "ps_enc.h"
#include "sbr_def.h"

#include "sbr_rom.h"
#include "sbr_ram.h"

#include "counters.h" /* the 3GPP instrumenting tool */


#define FLOAT_EPSILON                   0.0001f
#define INV_LOG2                        1.4427f
#define NO_QMFSB_IN_SUBQMF              ( 3 )
#define SUBQMF_BINS_ENERGY              ( 8 )
#define SUBQMF_GROUPS_MIX               ( 16 )

#define MAX_MAGNITUDE_CORR              ( 2.0f )

#define PS_MODE_LOW_FREQ_RES_IID_ICC    ( 0x00020000 )

static void downmixToMono( HANDLE_PS_ENC   pms,
                           float            **qmfLeftReal,
                           float            **qmfLeftImag,
                           float            **qmfRightReal,
                           float            **qmfRightImag );

int GetPsMode(int bitRate)
{
  int psMode = 0;

  COUNT_sub_start("GetPsMode");

  MOVE(1); /* counting previous operation */

  ADD(1); BRANCH(1);
  if(bitRate < 21000)
  {
    MOVE(1);
    psMode =
      PS_MODE_LOW_FREQ_RES_IID_ICC;
  }
  else {
  ADD(1); BRANCH(1);
  if(bitRate < 128000)
  {
    MOVE(1);
    psMode = 0;
  }
  }

  COUNT_sub_end();

  return psMode;
}

int
CreatePsEnc(HANDLE_PS_ENC h_ps_e,
            int psMode)
{
  int i;
  int err;

  float *ptr1, *ptr2, *ptr3, *ptr4;

  COUNT_sub_start("CreatePsEnc");

  PTR_INIT(4);
  ptr1 = &sbr_envYBuffer[QMF_TIME_SLOTS * QMF_CHANNELS];
  ptr2 = &PsBuf2[0];
  ptr3 = &PsBuf3[0];
  ptr4 = &PsBuf4[0];

  BRANCH(1);
  if (h_ps_e == NULL)
  {
    COUNT_sub_end();
    return 1;
  }

  INDIRECT(1); MOVE(1);
  h_ps_e->psMode = psMode;

  INDIRECT(2); MOVE(2);
  h_ps_e->bPrevZeroIid = 0;
  h_ps_e->bPrevZeroIcc = 0;

  INDIRECT(2); LOGIC(2); BRANCH(2); MOVE(2);
  h_ps_e->bHiFreqResIidIcc = ( ( psMode & PS_MODE_LOW_FREQ_RES_IID_ICC ) != 0 )? 0: 1;

  INDIRECT(2); BRANCH(1); MOVE(1);
  h_ps_e->iidIccBins = ( h_ps_e->bHiFreqResIidIcc )? NO_IID_BINS: NO_LOW_RES_IID_BINS;

  INDIRECT(1); PTR_INIT(1);
  h_ps_e->aaaICCDataBuffer = (float **)ptr1;

  ADD(1);
  ptr1+= NO_BINS * sizeof (float *)/sizeof(float);

  INDIRECT(1); PTR_INIT(1);
  h_ps_e->aaaIIDDataBuffer = (float **)ptr1;

  ADD(1);
  ptr1+= NO_BINS * sizeof (float *)/sizeof(float);

  PTR_INIT(2); /* h_ps_e->aaaICCDataBuffer[][]
                  h_ps_e->aaaIIDDataBuffer[][]
                 */
  LOOP(1);
  for (i=0 ; i < NO_BINS ; i++) {

    PTR_INIT(1);
    h_ps_e->aaaICCDataBuffer[i] = ptr1;

    ADD(1);
    ptr1+= SYSTEMLOOKAHEAD + 1;

    PTR_INIT(1);
    h_ps_e->aaaIIDDataBuffer[i] = ptr1;

    ADD(1);
    ptr1+= SYSTEMLOOKAHEAD + 1;
  }

  INDIRECT(4); PTR_INIT(2);
  h_ps_e->hHybridLeft = &h_ps_e->hybridLeft;
  h_ps_e->hHybridRight = &h_ps_e->hybridRight;


  FUNC(2);
  err = CreateHybridFilterBank ( h_ps_e->hHybridLeft,
                                 &ptr4);

  BRANCH(1);
  if(err != 0) {

      PTR_INIT(1); FUNC(1);
      DeletePsEnc (&h_ps_e);

      COUNT_sub_end();
      return 1; /* "Failed to create hybrid filterbank." */
  }

  FUNC(2);
  err = CreateHybridFilterBank ( h_ps_e->hHybridRight,
                                 &ptr4);

  BRANCH(1);
  if(err != 0) {
      PTR_INIT(1); FUNC(1);
      DeletePsEnc (&h_ps_e);

      COUNT_sub_end();
      return 1; /* "Failed to create hybrid filterbank." */
  }

  PTR_INIT(4); /* h_ps_e->mHybridRealLeft[]
                  h_ps_e->mHybridImagLeft[]
                  h_ps_e->mHybridRealRight[]
                  h_ps_e->mHybridImagRight[]
                */
  LOOP(1);
  for (i=0 ; i < NO_SUBSAMPLES ; i++) {
    PTR_INIT(1);
    h_ps_e->mHybridRealLeft[i] = ptr3;

    ADD(1);
    ptr3+=NO_HYBRID_BANDS;

    PTR_INIT(1);
    h_ps_e->mHybridImagLeft[i] = ptr3;

    ADD(1);
    ptr3+=NO_HYBRID_BANDS;

    PTR_INIT(1);
    h_ps_e->mHybridRealRight[i] = ptr1;

    ADD(1);
    ptr1+=NO_HYBRID_BANDS;

    PTR_INIT(1);
    h_ps_e->mHybridImagRight[i] = ptr1;

    ADD(1);
    ptr1+=NO_HYBRID_BANDS;
  }

  INDIRECT(1); PTR_INIT(1); ADD(1);
  h_ps_e->tempQmfLeftReal = (float **)ptr1;
  ptr1+= HYBRID_FILTER_DELAY * sizeof (float *)/sizeof(float);

  INDIRECT(1); PTR_INIT(1); ADD(1);
  h_ps_e->tempQmfLeftImag= (float **)ptr1;
  ptr1+= HYBRID_FILTER_DELAY * sizeof (float *)/sizeof(float);

  INDIRECT(1); PTR_INIT(1); ADD(1);
  h_ps_e->histQmfLeftReal = (float **)ptr2;
  ptr2+= HYBRID_FILTER_DELAY * sizeof (float *)/sizeof(float);

  INDIRECT(1); PTR_INIT(1); ADD(1);
  h_ps_e->histQmfLeftImag= (float **)ptr2;
  ptr2+= HYBRID_FILTER_DELAY * sizeof (float *)/sizeof(float);

  INDIRECT(1); PTR_INIT(1); ADD(1);
  h_ps_e->histQmfRightReal = (float **)ptr2;
  ptr2+= HYBRID_FILTER_DELAY * sizeof (float *)/sizeof(float);

  INDIRECT(1); PTR_INIT(1); ADD(1);
  h_ps_e->histQmfRightImag= (float **)ptr2;
  ptr2+= HYBRID_FILTER_DELAY * sizeof (float *)/sizeof(float);

  PTR_INIT(6); /* h_ps_e->tempQmfLeftReal[]
                  h_ps_e->tempQmfLeftImag[]
                  h_ps_e->histQmfLeftReal[]
                  h_ps_e->histQmfLeftImag[]
                  h_ps_e->histQmfRightReal[]
                  h_ps_e->histQmfRightImag[]
               */
  LOOP(1);
  for (i=0 ; i < HYBRID_FILTER_DELAY ; i++) {
    PTR_INIT(1);
    h_ps_e->tempQmfLeftReal[i] = ptr1;

    ADD(1);
    ptr1+=NO_QMF_BANDS;

    PTR_INIT(1);
    h_ps_e->tempQmfLeftImag[i] = ptr1;

    ADD(1);
    ptr1+=NO_QMF_BANDS;

    PTR_INIT(1);
    h_ps_e->histQmfLeftReal[i] = ptr2;

    ADD(1);
    ptr2+=NO_QMF_BANDS;

    PTR_INIT(1);
    h_ps_e->histQmfLeftImag[i] = ptr2;

    ADD(1);
    ptr2+=NO_QMF_BANDS;

    PTR_INIT(1);
    h_ps_e->histQmfRightReal[i] = ptr2;

    ADD(1);
    ptr2+=NO_QMF_BANDS;

    PTR_INIT(1);
    h_ps_e->histQmfRightImag[i] = ptr2;

    ADD(1);
    ptr2+=NO_QMF_BANDS;

  }

  LOGIC(3); BRANCH(1);
  if ( ( h_ps_e->histQmfLeftReal     == NULL ) || ( h_ps_e->histQmfLeftImag     == NULL ) ||
       ( h_ps_e->histQmfRightReal    == NULL ) || ( h_ps_e->histQmfRightImag    == NULL ) )
  {
    PTR_INIT(1); FUNC(1);
    DeletePsEnc( &h_ps_e );

    COUNT_sub_end();
    return 1; /* "Memory allocation in function CreatePsEnc() failed." */
  }

  INDIRECT(2); LOGIC(1);
  if (h_ps_e->psBitBuf.isValid == 0) {
    INDIRECT(1); PTR_INIT(1); FUNC(3);
    CreateBitBuffer(&h_ps_e->psBitBuf,
                    (unsigned char*)ptr1,
                    255+15);

    LOOP(1);PTR_INIT(1); /* h_ps_e->aaaICCDataBuffer[][] */
    for (i=0; i<h_ps_e->iidIccBins; i++) {
      h_ps_e->aaaICCDataBuffer[i][0] = -1.0f; MOVE(1);
    }
  }
  COUNT_sub_end();

  return 0;
}

void
DeletePsEnc(HANDLE_PS_ENC *h_ps_e)
{
  COUNT_sub_start("DeletePsEnc");
  COUNT_sub_end();

  return;
}


/***************************************************************************/
/*!

  \brief  Extracts the parameters (IID, ICC) of the current frame.

****************************************************************************/
void
EncodePsFrame(HANDLE_PS_ENC pms,
              float **iBufferLeft,
              float **rBufferLeft,
              float **iBufferRight,
              float **rBufferRight)
{
  int     env;
  int     i;
  int     bin;
  int     subband, maxSubband;
  int     startSample, stopSample;
  float **hybrLeftImag;
  float **hybrLeftReal;
  float **hybrRightImag;
  float **hybrRightReal;

  COUNT_sub_start("EncodePsFrame");

  INDIRECT(3); FUNC(5);
  HybridAnalysis ( (const float**) rBufferLeft,
                   (const float**) iBufferLeft,
                   pms->mHybridRealLeft,
                   pms->mHybridImagLeft,
                   pms->hHybridLeft);

  INDIRECT(3); FUNC(5);
  HybridAnalysis ( (const float**) rBufferRight,
                   (const float**) iBufferRight,
                   pms->mHybridRealRight,
                   pms->mHybridImagRight,
                   pms->hHybridRight);

  PTR_INIT(4); /*  pms->iidIccBins
                   pms->bHiFreqResIidIcc
                   pms->aaaIIDDataBuffer[][]
                   pms->aaaICCDataBuffer[][]
                */

  LOOP(1);
  for ( bin = 0; bin < pms->iidIccBins; bin++ ) {
    MOVE(2);
    pms->aaaIIDDataBuffer[bin][1] = pms->aaaIIDDataBuffer[bin][0];
    pms->aaaICCDataBuffer[bin][1] = pms->aaaICCDataBuffer[bin][0];
  }

  LOOP(1);
  for ( env = 0; env < 2; env++ ) {

    INDIRECT(4); MOVE(4);
    hybrLeftReal  = pms->mHybridRealLeft;
    hybrLeftImag  = pms->mHybridImagLeft;
    hybrRightReal = pms->mHybridRealRight;
    hybrRightImag = pms->mHybridImagRight;

    BRANCH(1);
    if ( env == 0  ) {

      MOVE(1);
      startSample   = 0;

      MOVE(1);
      stopSample    = NO_SUBSAMPLES/2;
    }
    else {
      MOVE(1);
      startSample   = NO_SUBSAMPLES/2;

      MOVE(1);
      stopSample    = NO_SUBSAMPLES;
    }

    PTR_INIT(9); /* hiResBandBorders[]
                    pms->powerLeft[]
                    pms->powerRight[]
                    pms->powerCorrReal[]
                    pms->powerCorrImag[]
                    pms->histQmfLeftReal;
                    pms->histQmfLeftImag;
                    pms->histQmfRightReal;
                    pms->histQmfRightImag;
                 */
    LOOP(1);
    for ( bin = 0; bin < NO_BINS; bin++ ) {

      ADD(1); BRANCH(1);
      if ( bin >= SUBQMF_BINS_ENERGY ) {

        BRANCH(1);
        if ( env == 0 ) {

          MOVE(4);
          hybrLeftReal  = pms->histQmfLeftReal;
          hybrLeftImag  = pms->histQmfLeftImag;
          hybrRightReal = pms->histQmfRightReal;
          hybrRightImag = pms->histQmfRightImag;
        }
        else {

          ADD(4);
          hybrLeftReal  = rBufferLeft -HYBRID_FILTER_DELAY;
          hybrLeftImag  = iBufferLeft -HYBRID_FILTER_DELAY;
          hybrRightReal = rBufferRight-HYBRID_FILTER_DELAY;
          hybrRightImag = iBufferRight-HYBRID_FILTER_DELAY;
        }
      }

      ADD(1); BRANCH(1); ADD(1);
      maxSubband = ( bin < SUBQMF_BINS_ENERGY )? hiResBandBorders[bin] + 1: hiResBandBorders[bin + 1];

      LOOP(1);
      for ( i = startSample; i < stopSample; i++ ) {

        ADD(2); LOGIC(1); BRANCH(1);
        if ( bin >= SUBQMF_BINS_ENERGY && i == HYBRID_FILTER_DELAY ) {

          ADD(4);
          hybrLeftReal  = rBufferLeft -HYBRID_FILTER_DELAY;
          hybrLeftImag  = iBufferLeft -HYBRID_FILTER_DELAY;
          hybrRightReal = rBufferRight-HYBRID_FILTER_DELAY;
          hybrRightImag = iBufferRight-HYBRID_FILTER_DELAY;
        }

        PTR_INIT(4); /* hybrLeftReal[][]
                        hybrLeftImag[][]
                        hybrRightReal[][]
                        hybrRightImag[][]
                     */
        LOOP(1);
        for ( subband = hiResBandBorders[bin]; subband < maxSubband; subband++ ) {

          MAC(2); STORE(1);
          pms->powerLeft    [bin] += hybrLeftReal [i][subband] * hybrLeftReal [i][subband] +
                                     hybrLeftImag [i][subband] * hybrLeftImag [i][subband];

          MAC(2); STORE(1);
          pms->powerRight   [bin] += hybrRightReal[i][subband] * hybrRightReal[i][subband] +
                                     hybrRightImag[i][subband] * hybrRightImag[i][subband];

          MAC(2); STORE(1);
          pms->powerCorrReal[bin] += hybrLeftReal [i][subband] * hybrRightReal[i][subband] +
                                     hybrLeftImag [i][subband] * hybrRightImag[i][subband];

          MAC(1); MULT(1); ADD(1); STORE(1);
          pms->powerCorrImag[bin] += hybrLeftImag [i][subband] * hybrRightReal[i][subband] -
                                     hybrLeftReal [i][subband] * hybrRightImag[i][subband];
        }
      }   /* for (i) */

      BRANCH(1);
      if ( env == 0 ) {

        ADD(4); STORE(4);
        pms->powerLeft    [bin] += FLOAT_EPSILON;
        pms->powerRight   [bin] += FLOAT_EPSILON;
        pms->powerCorrReal[bin] += FLOAT_EPSILON;
        pms->powerCorrImag[bin] += FLOAT_EPSILON;
      }
    } /* bins loop */

    BRANCH(1);
    if (env == 0) {
      float  tempLeft;
      float  tempRight;
      float  tempCorrR;
      float  tempCorrI;

      PTR_INIT(5); /* pms->powerLeft[]
                      pms->powerRight[]
                      pms->powerCorrReal[]
                      pms->powerCorrImag[]
                      pms->aaaICCDataBuffer[][][]
                   */
      LOOP(1);
      for ( bin = 0; bin < pms->iidIccBins; bin++ ) {

        BRANCH(1);
        if ( pms->bHiFreqResIidIcc ) {
          MOVE(4);
          tempLeft  = pms->powerLeft    [bin];
          tempRight = pms->powerRight   [bin];
          tempCorrR = pms->powerCorrReal[bin];
          tempCorrI = pms->powerCorrImag[bin];
        }
        else {
          INDIRECT(4); ADD(4);
          tempLeft  = pms->powerLeft    [2 * bin] + pms->powerLeft    [2 * bin + 1];
          tempRight = pms->powerRight   [2 * bin] + pms->powerRight   [2 * bin + 1];
          tempCorrR = pms->powerCorrReal[2 * bin] + pms->powerCorrReal[2 * bin + 1];
          tempCorrI = pms->powerCorrImag[2 * bin] + pms->powerCorrImag[2 * bin + 1];
        }

        ADD(1); BRANCH(1);
        if (bin > NO_IPD_BINS) {
          MULT(1); MAC(1);
          tempCorrR = tempCorrR * tempCorrR + tempCorrI * tempCorrI;

          MULT(1); TRANS(1); DIV(1); STORE(1);
          pms->aaaICCDataBuffer[bin][0] = ( float )sqrt(tempCorrR / ( tempLeft * tempRight ));
        }
        else
        {
          MULT(1); TRANS(1); DIV(1); STORE(1);
          pms->aaaICCDataBuffer[bin][0] = tempCorrR / ( float )sqrt( tempLeft * tempRight );
        }

        ADD(1); BRANCH(1);
        if ( pms->aaaICCDataBuffer[bin][0] > 1.0f ) {

          MOVE(1);
          pms->aaaICCDataBuffer[bin][0] = 0;
        }
        else
        {
          ADD(1); MULT(1); TRANS(1); STORE(1);
          pms->aaaICCDataBuffer[bin][0] = ( float )sqrt( ( 1.0f - pms->aaaICCDataBuffer[bin][0] ) * 0.5f );
        }

        DIV(1); TRANS(2); MULT(1); STORE(1);
        pms->aaaIIDDataBuffer[bin][0] = INV_LOG2 * ( float )log( sqrt( tempLeft / tempRight ) );
      }

      FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(pms->powerLeft));
      memset( pms->powerLeft    , 0, sizeof( pms->powerLeft     ) );

      FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(pms->powerRight));
      memset( pms->powerRight   , 0, sizeof( pms->powerRight    ) );

      FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(pms->powerCorrReal));
      memset( pms->powerCorrReal, 0, sizeof( pms->powerCorrReal ) );

      FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(pms->powerCorrImag));
      memset( pms->powerCorrImag, 0, sizeof( pms->powerCorrImag ) );
    } /* if (env==0) */
  } /* envelopes loop */

  FUNC(5);
  downmixToMono( pms, rBufferLeft, iBufferLeft, rBufferRight, iBufferRight );

  COUNT_sub_end();
}

/***************************************************************************/
/*!
  \brief  generates mono downmix

****************************************************************************/
static void
downmixToMono( HANDLE_PS_ENC pms,
               float **qmfLeftReal,
               float **qmfLeftImag,
               float **qmfRightReal,
               float **qmfRightImag )
{
  int     i;
  int     group;
  int     subband;
  int     maxSubband;
  int     bin;
  float   temp;
  float   temp2;
  float   tempLeftReal;
  float   tempLeftImag;
  float   tempRightReal;
  float   tempRightImag;
  float **hybrLeftReal;
  float **hybrLeftImag;
  float **hybrRightReal;
  float **hybrRightImag;

  COUNT_sub_start("downmixToMono");

  LOOP(1);
  for ( i = 0; i < HYBRID_FILTER_DELAY; i++ ) {
    FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(NO_QMF_BANDS * sizeof(**qmfLeftReal));
    memcpy( pms->tempQmfLeftReal [i], qmfLeftReal [NO_SUBSAMPLES-HYBRID_FILTER_DELAY+i], NO_QMF_BANDS * sizeof( **qmfLeftReal  ) );

    FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(NO_QMF_BANDS * sizeof(**qmfLeftImag));
    memcpy( pms->tempQmfLeftImag [i], qmfLeftImag [NO_SUBSAMPLES-HYBRID_FILTER_DELAY+i], NO_QMF_BANDS * sizeof( **qmfLeftImag  ) );
  }

  INDIRECT(4); MOVE(4);
  hybrLeftReal  = pms->mHybridRealLeft;
  hybrLeftImag  = pms->mHybridImagLeft;
  hybrRightReal = pms->mHybridRealRight;
  hybrRightImag = pms->mHybridImagRight;

  PTR_INIT(2); /* bins2groupMap[group]
                  groupBordersMix[group]
               */
  LOOP(1);
  for ( group = 0; group < NO_IPD_GROUPS; group++ ) {

    LOGIC(1);
    bin = ( ~NEGATE_IPD_MASK ) & bins2groupMap[group];

    ADD(1); BRANCH(1);
    if ( group >= SUBQMF_GROUPS_MIX ) {

      MOVE(4);
      hybrLeftReal  = qmfLeftReal -HYBRID_FILTER_DELAY;
      hybrLeftImag  = qmfLeftImag -HYBRID_FILTER_DELAY;
      hybrRightReal = qmfRightReal-HYBRID_FILTER_DELAY;
      hybrRightImag = qmfRightImag-HYBRID_FILTER_DELAY;
    }

    ADD(1); BRANCH(1); /* ... ? */ ADD(1);
    maxSubband = ( group < SUBQMF_GROUPS_MIX )? groupBordersMix[group] + 1: groupBordersMix[group + 1];

    LOOP(1);
    for ( i = NO_SUBSAMPLES-1; i >= 0; i-- ) {

      ADD(2); LOGIC(1); BRANCH(1);
      if ( group >= SUBQMF_GROUPS_MIX && i == HYBRID_FILTER_DELAY-1 ) {

        MOVE(4);
        hybrLeftReal  = pms->histQmfLeftReal;
        hybrLeftImag  = pms->histQmfLeftImag;
        hybrRightReal = pms->histQmfRightReal;
        hybrRightImag = pms->histQmfRightImag;
      }

      PTR_INIT(4); /* hybrLeftReal [i][subband]
                      hybrLeftImag [i][subband]
                      hybrRightReal[i][subband]
                      hybrRightImag[i][subband]
                   */
      LOOP(1);
      for ( subband = groupBordersMix[group]; subband < maxSubband; subband++ ) {

        MOVE(4);
        tempLeftReal  = hybrLeftReal [i][subband];
        tempLeftImag  = hybrLeftImag [i][subband];
        tempRightReal = hybrRightReal[i][subband];
        tempRightImag = hybrRightImag[i][subband];

        MULT(2); MAC(3); ADD(1);
        temp = ( tempLeftReal  * tempLeftReal  + tempLeftImag  * tempLeftImag  +
                 tempRightReal * tempRightReal + tempRightImag * tempRightImag ) * 0.5f + FLOAT_EPSILON;

        MULT(1); MAC(1); ADD(1);
        temp2 = temp + ( tempLeftReal * tempRightReal +
                         tempLeftImag * tempRightImag );

        MULT(1); ADD(1); BRANCH(1);
        if (temp > temp2 * 2.0f * MAX_MAGNITUDE_CORR * MAX_MAGNITUDE_CORR) {
          MOVE(1);
          temp = MAX_MAGNITUDE_CORR;
        }
        else {
          MULT(1); DIV(1); TRANS(1);
          temp = ( float )sqrt( 0.5f * temp / temp2 );
        }

        ADD(2); MULT(2);
        tempLeftReal = ( tempLeftReal + tempRightReal ) * temp;
        tempLeftImag = ( tempLeftImag + tempRightImag ) * temp;

        ADD(1); BRANCH(1);
        if ( group < SUBQMF_GROUPS_MIX ) {
          MOVE(2);
          hybrLeftReal[i][subband] = tempLeftReal;
          hybrLeftImag[i][subband] = tempLeftImag;
        }
        else {

          MOVE(2);
          qmfLeftReal [i][subband] = tempLeftReal;
          qmfLeftImag [i][subband] = tempLeftImag;
        }
      } /* subbands loop */
    } /* loop (i) */
  } /* groups loop */

  LOOP(1);
  for ( i = 0; i < HYBRID_FILTER_DELAY; i++ ) {
    FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(NO_QMF_BANDS * sizeof(**qmfLeftReal));
    memcpy( pms->histQmfLeftReal [i], pms->tempQmfLeftReal [i], NO_QMF_BANDS * sizeof( **qmfLeftReal  ) );

    FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(NO_QMF_BANDS * sizeof(**qmfLeftImag));
    memcpy( pms->histQmfLeftImag [i], pms->tempQmfLeftImag [i], NO_QMF_BANDS * sizeof( **qmfLeftImag  ) );

    FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(NO_QMF_BANDS * sizeof(**qmfRightReal));
    memcpy( pms->histQmfRightReal[i], qmfRightReal[NO_SUBSAMPLES-HYBRID_FILTER_DELAY+i], NO_QMF_BANDS * sizeof( **qmfRightReal ) );

    FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(NO_QMF_BANDS * sizeof(**qmfRightImag));
    memcpy( pms->histQmfRightImag[i], qmfRightImag[NO_SUBSAMPLES-HYBRID_FILTER_DELAY+i], NO_QMF_BANDS * sizeof( **qmfRightImag ) );
  }

  INDIRECT(3); FUNC(5);
  HybridSynthesis( ( const float** )pms->mHybridRealLeft,
                   ( const float** )pms->mHybridImagLeft,
                   qmfLeftReal,
                   qmfLeftImag,
                   pms->hHybridLeft );

  COUNT_sub_end();
}

