/*
  Hybrid Filter Bank
*/
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "hybrid.h"
#include "sbr_ram.h"
#include "sbr_rom.h"
#include "sbr_def.h"
#include "cfftn.h"

#include "counters.h" /* the 3GPP instrumenting tool */

static void fourChannelFiltering( const float *pQmfReal,
                                  const float *pQmfImag,
                                  float **mHybridReal,
                                  float **mHybridImag,
                                  int chOffset)
{
  int i, k, n;
  int midTap = HYBRID_FILTER_DELAY;
  float real, imag;
  float cum[8];

  COUNT_sub_start("fourChannelFiltering");

  MOVE(1);
  LOOP(1); PTR_INIT(2);
  for(i = 0; i < QMF_TIME_SLOTS; i++) {

    MOVE(2);
    cum[5] = cum[4] = 0;

    LOOP(1); PTR_INIT(3);
    for(k = 0; k < 4; k++) {

      MAC(1);
      cum[5] -= p4_13[4*k] * pQmfReal[i+4*k];

      MAC(1);
      cum[4] += p4_13[4*k] * pQmfImag[i+4*k];
    }
    ADD(1);
    MOVE(2);

    MOVE(2);
    real = imag = 0;

    LOOP(1); PTR_INIT(3);
    for(k = 0; k < 3; k++) {

      MAC(2);
      real += p4_13[4*k+3] * pQmfReal[i+4*k+3];
      imag += p4_13[4*k+3] * pQmfImag[i+4*k+3];
    }

    ADD(2); MULT(2); STORE(2);
    cum[6] = (imag + real) * 0.70710678118655f;
    cum[7] = (imag - real) * 0.70710678118655f;

    MULT(2); STORE(2);
    cum[0] = p4_13[midTap] * pQmfReal[i+midTap];
    cum[1] = p4_13[midTap] * pQmfImag[i+midTap];

    MOVE(2);
    real = imag = 0;

    LOOP(1); PTR_INIT(3);
    for(k = 0; k < 3; k++) {

      MAC(2);
      real += p4_13[4*k+1] * pQmfReal[i+4*k+1];
      imag += p4_13[4*k+1] * pQmfImag[i+4*k+1];
    }

    ADD(2); MULT(2); STORE(2);
    cum[2] = (real - imag ) * 0.70710678118655f;
    cum[3] = (real + imag ) * 0.70710678118655f;

    FUNC(3);
    CFFTN(cum, 4, 1);

    LOOP(1); PTR_INIT(3);
    for(n = 0; n < 4; n++) {

      MOVE(2);
      mHybridReal[i][n + chOffset] = cum[2*n];
      mHybridImag[i][n + chOffset] = cum[2*n+1];
    }
  }
  COUNT_sub_end();
}

static void eightChannelFiltering( const float *pQmfReal,
                                   const float *pQmfImag,
                                   float **mHybridReal,
                                   float **mHybridImag)
{
  int i, n;
  float real, imag;
  int midTap = HYBRID_FILTER_DELAY;
  float cum[16];

  COUNT_sub_start("eightChannelFiltering");


  MOVE(1);
  LOOP(1); PTR_INIT(3);
  for(i = 0; i < QMF_TIME_SLOTS; i++) {

    MULT(1); MAC(1);
    real = p8_13[4]  * pQmfReal[4+i] +
           p8_13[12] * pQmfReal[12+i];

    MULT(1); MAC(1);
    imag = p8_13[4]  * pQmfImag[4+i] +
           p8_13[12] * pQmfImag[12+i];

    ADD(1); MULT(1); STORE(1);
    cum[4] =  (imag - real) * 0.70710678118655f;

    ADD(1); MULT(1); STORE(1);
    cum[5] = -(imag + real) * 0.70710678118655f;

    MULT(1); MAC(1);
    real = p8_13[3]  * pQmfReal[3+i] +
           p8_13[11] * pQmfReal[11+i];

    MULT(1); MAC(1);
    imag = p8_13[3]  * pQmfImag[3+i] +
           p8_13[11] * pQmfImag[11+i];

    MULT(1); MAC(1); STORE(1);
    cum[6] =   imag * 0.92387953251129f - real * 0.38268343236509f;

    MULT(1); MAC(1); STORE(1);
    cum[7] = -(imag * 0.38268343236509f + real * 0.92387953251129f);

    MULT(1); MAC(1); ADD(1); STORE(1);
    cum[9] = -( p8_13[2]  * pQmfReal[2+i] +
                p8_13[10] * pQmfReal[10+i] );

    MULT(1); MAC(1); STORE(1);
    cum[8] =    p8_13[2]  * pQmfImag[2+i] +
                p8_13[10] * pQmfImag[10+i];

    MULT(1); MAC(1);
    real = p8_13[1]  * pQmfReal[1+i] +
           p8_13[9] * pQmfReal[9+i];

    MULT(1); MAC(1);
    imag = p8_13[1]  * pQmfImag[1+i] +
           p8_13[9] * pQmfImag[9+i];

    MULT(1); MAC(1); STORE(1);
    cum[10] = imag * 0.92387953251129f + real * 0.38268343236509f;

    MULT(1); MAC(1); STORE(1);
    cum[11] = imag * 0.38268343236509f - real * 0.92387953251129f;

    MULT(1); MAC(1);
    real = p8_13[0]  * pQmfReal[i] +
           p8_13[8] * pQmfReal[8+i];

    MULT(1); MAC(1);
    imag = p8_13[0]  * pQmfImag[i] +
           p8_13[8] * pQmfImag[8+i];

    ADD(1); MULT(1); STORE(1);
    cum[12] = (imag + real) * 0.70710678118655f;

    ADD(1); MULT(1); STORE(1);
    cum[13] = (imag - real) * 0.70710678118655f;

    MULT(2);
    real = p8_13[7]  * pQmfReal[7+i];
    imag = p8_13[7]  * pQmfImag[7+i];

    MULT(1); MAC(1); STORE(1);
    cum[14] = imag * 0.38268343236509f + real * 0.92387953251129f;

    MULT(1); MAC(1); STORE(1);
    cum[15] = imag * 0.92387953251129f - real * 0.38268343236509f;

    MULT(2); STORE(2);
    cum[0] = p8_13[midTap]  * pQmfReal[midTap+i];
    cum[1] = p8_13[midTap]  * pQmfImag[midTap+i];

    MULT(2);
    real = p8_13[5]  * pQmfReal[5+i];
    imag = p8_13[5]  * pQmfImag[5+i];

    MULT(1); MAC(1); STORE(1);
    cum[2] = real * 0.92387953251129f - imag * 0.38268343236509f;

    MULT(1); MAC(1); STORE(1);
    cum[3] = real * 0.38268343236509f + imag * 0.92387953251129f;

    FUNC(3);
    CFFTN(cum, 8, 1);

    LOOP(1); PTR_INIT(3);
    for(n = 0; n < 8; n++) {

      MOVE(2);
      mHybridReal[i][n] = cum[2*n];
      mHybridImag[i][n] = cum[2*n+1];
    }
  }
  COUNT_sub_end();
}


/**************************************************************************/
/*!
  \brief   HybridAnalysis

  \return none.

*/
/**************************************************************************/
void
HybridAnalysis ( const float **mQmfReal,
                 const float **mQmfImag,
                 float **mHybridReal,
                 float **mHybridImag,
                 HANDLE_HYBRID hHybrid )   /*!< Handle to HYBRID struct. */

{
  int  n, band;
  HYBRID_RES hybridRes;
  int  chOffset = 0;

  COUNT_sub_start("HybridAnalysis");

  MOVE(1);

  LOOP(1);
  for(band = 0; band < NO_QMF_BANDS_IN_HYBRID; band++) {

    INDIRECT(1); MOVE(1);
    hybridRes = (HYBRID_RES)aHybridResolution[band];

    INDIRECT(1); MULT(1);
    FUNC(3); LOOP(1); INDIRECT(2); PTR_INIT(2); MOVE(1); STORE(QMF_BUFFER_MOVE - 1);
    memcpy(hHybrid->pWorkReal, hHybrid->mQmfBufferReal[band],
           QMF_BUFFER_MOVE * sizeof(float));

    FUNC(3); LOOP(1); INDIRECT(2); PTR_INIT(2); MOVE(1); STORE(QMF_BUFFER_MOVE - 1);
    memcpy(hHybrid->pWorkImag, hHybrid->mQmfBufferImag[band],
           QMF_BUFFER_MOVE * sizeof(float));

    PTR_INIT(4); LOOP(1);
    for(n = 0; n < QMF_TIME_SLOTS; n++) {
      MOVE(2);
      hHybrid->pWorkReal [QMF_BUFFER_MOVE + n] = mQmfReal [n] [band];
      hHybrid->pWorkImag [QMF_BUFFER_MOVE + n] = mQmfImag [n] [band];
    }

    FUNC(3); LOOP(1); INDIRECT(2); PTR_INIT(2); MOVE(1); STORE(QMF_BUFFER_MOVE - 1);
    memcpy(hHybrid->mQmfBufferReal[band], hHybrid->pWorkReal + QMF_TIME_SLOTS,
           QMF_BUFFER_MOVE * sizeof(float));

    FUNC(3); LOOP(1); INDIRECT(2); PTR_INIT(2); MOVE(1); STORE(QMF_BUFFER_MOVE - 1);
    memcpy(hHybrid->mQmfBufferImag[band], hHybrid->pWorkImag + QMF_TIME_SLOTS,
           QMF_BUFFER_MOVE * sizeof(float));

    switch(hybridRes) {
    case HYBRID_4_CPLX:
      ADD(1); BRANCH(1);

      /* filtering. */
      FUNC(5);
      fourChannelFiltering( hHybrid->pWorkReal,
                            hHybrid->pWorkImag,
                            mHybridReal,
                            mHybridImag,
                            chOffset );

     break;
    case HYBRID_8_CPLX:
      ADD(1); BRANCH(1);

      /* filtering. */
      FUNC(4);
      eightChannelFiltering( hHybrid->pWorkReal,
                             hHybrid->pWorkImag,
                             mHybridReal,
                             mHybridImag);
      break;
    default:
      assert(0);
    }

    ADD(1);
    chOffset += hybridRes;
  }
  COUNT_sub_end();
}

/**************************************************************************/
/*!
  \brief  HybridSynthesis

  \return none.

*/
/**************************************************************************/
void
HybridSynthesis ( const float **mHybridReal,
                  const float **mHybridImag,
                  float **mQmfReal,
                  float **mQmfImag,
                  HANDLE_HYBRID hHybrid )      /*!< Handle to HYBRID struct. */
{
  int  k, n, band;
  HYBRID_RES hybridRes;
  int  chOffset = 0;

  COUNT_sub_start("HybridSynthesis");
  MOVE(1);

  LOOP(1);
  for(band = 0; band < NO_QMF_BANDS_IN_HYBRID; band++) {

    INDIRECT(1); MOVE(1);
    hybridRes = (HYBRID_RES)aHybridResolution[band];

    LOOP(1);
    for(n = 0; n < QMF_TIME_SLOTS; n++) {

      MOVE(2);
      mQmfReal [n] [band] = mQmfImag [n] [band] = 0;

      PTR_INIT(2); LOOP(1);
      for(k = 0; k < hybridRes; k++) {

        ADD(2);
        mQmfReal [n] [band] += mHybridReal [n] [chOffset + k];
        mQmfImag [n] [band] += mHybridImag [n] [chOffset + k];
      }
      MOVE(2);
    }

    ADD(1);
    chOffset += hybridRes;
  }
  COUNT_sub_end();
}


/**************************************************************************/
/*!
  \brief    CreateHybridFilterBank


  \return   errorCode, noError if successful.

*/
/**************************************************************************/
int
CreateHybridFilterBank ( HANDLE_HYBRID hs,         /*!< Handle to HYBRID struct. */
                         float **pPtr)
{
  int i;
  float *ptr = *pPtr;

  COUNT_sub_start("CreateHybridFilterBank");

  MOVE(1); PTR_INIT(1); /* counting previous operations */

  INDIRECT(2); PTR_INIT(2); ADD(2);
  hs->pWorkReal = ptr;ptr+= QMF_TIME_SLOTS + QMF_BUFFER_MOVE;
  hs->pWorkImag = ptr;ptr+= QMF_TIME_SLOTS + QMF_BUFFER_MOVE;

  INDIRECT(1); PTR_INIT(1);
  hs->mQmfBufferReal = (float **)ptr;

  ADD(1);
  ptr+= NO_QMF_BANDS_IN_HYBRID * sizeof (float *)/sizeof(float);

  INDIRECT(1); PTR_INIT(1);
  hs->mQmfBufferImag = (float **)ptr;

  ADD(1);
  ptr+= NO_QMF_BANDS_IN_HYBRID * sizeof (float *)/sizeof(float);

  PTR_INIT(2); /* hs->mQmfBufferReal[]
                  hs->mQmfBufferImag[]
               */
  LOOP(1);
  for (i = 0; i < NO_QMF_BANDS_IN_HYBRID; i++) {

    ADD(1); STORE(1);
    hs->mQmfBufferReal[i] = ptr;ptr+= QMF_BUFFER_MOVE;

    ADD(1); STORE(1);
    hs->mQmfBufferImag[i] = ptr;ptr+= QMF_BUFFER_MOVE;

  }

  PTR_INIT(1);
  *pPtr=ptr;

  COUNT_sub_end();

  return 0;
}


/**************************************************************************/
/*!
  \brief

  \return   none

*/
/**************************************************************************/
void
DeleteHybridFilterBank ( HANDLE_HYBRID *phHybrid ) /*!< Pointer to handle to HYBRID struct. */
{
  COUNT_sub_start("DeleteHybridFilterBank");
  COUNT_sub_end();
  return;
}
