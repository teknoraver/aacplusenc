/*
  IIR resampler tool box
*/

#include <memory.h>
#include <assert.h>
#include <stdio.h>
#include "resampler.h"

#include "counters.h" /* the 3GPP instrumenting tool */

#define MAX_COEFF 32


struct IIR_PARAM{
  const float *coeffIIRa;
  const float *coeffIIRb;
  int noOffCoeffs;
  int transitionFactor;
  int delay;
};

static const float set1_a[] = {
  0.004959855f,   0.025814206f,   0.080964205f,   0.182462303f,
  0.322109621f,   0.462709927f,   0.552160404f,   0.552160404f,
  0.462709927f,   0.322109621f,   0.182462303f,   0.080964205f,
  0.025814206f,   0.004959855f
};

static const float set1_b[] = {
  0.0f,          -1.038537170f,   2.627279635f,  -1.609574122f,
  2.205922661f,  -0.751928739f,   0.787128253f,  -0.105573173f,
  0.131638380f,   0.003884641f,   0.010544805f,   0.001232040f,
  0.000320798f,   0.000023031f
};
  
  
static struct IIR_PARAM const set1 = {
  set1_a,
  set1_b,
  14,
  218,
  6
};



/*
  Reset downsampler instance and clear delay lines     
  
  returns status
*/
int 
InitIIR21_Resampler(IIR21_RESAMPLER *ReSampler)
     
{
  COUNT_sub_start("InitDownsampler");

  INDIRECT(1); MOVE(1);
  ReSampler->iirFilter.ptr   =   0;
 
  INDIRECT(8); MOVE(4);
  ReSampler->iirFilter.coeffIIRa = set1.coeffIIRa;
  ReSampler->iirFilter.coeffIIRb = set1.coeffIIRb;
  ReSampler->iirFilter.noOffCoeffs = set1.noOffCoeffs;
  ReSampler->delay=set1.delay;

  assert(ReSampler->iirFilter.noOffCoeffs <= BUFFER_SIZE);

  INDIRECT(1); MOVE(1);
  ReSampler->ratio = 2;

  INDIRECT(1); STORE(1);
  ReSampler->pending = 1;

  COUNT_sub_end();

  return 1;
}



/* 
   NOTE: enabling NEWIIR would save us some wMOPS, but result in 1 LSB diffs, it is worth a CR
*/

#ifdef NEWIIR
static float
AdvanceMAFilter( IIR_FILTER *iirFilter
                 )
{
  float y;
  int j;
  int ptr = iirFilter->ptr;
  int i = ptr + (BUFFER_SIZE-1);

  COUNT_sub_start("AdvanceMAFilter");

  INDIRECT(1); /* MOVE(1); --> ptr isn't needed */ ADD(1); /* counting previous operations */

  INDIRECT(2); MULT(1);
  y = (iirFilter->coeffIIRa[0] * iirFilter->ring_buf_2[i & (BUFFER_SIZE-1)]);

  PTR_INIT(3); /* iirFilter->noOffCoeffs
                  iirFilter->coeffIIRa[]
                  iirFilter->ring_buf_2[]
               */
  LOOP(1);
  for (j=1; j<iirFilter->noOffCoeffs; j++) {
    i--;

    MAC(1);
    y += (iirFilter->coeffIIRa[j] * iirFilter->ring_buf_2[i & (BUFFER_SIZE-1)]);
  }

  COUNT_sub_end();

  return y;
}


static void
AdvanceARFilter( IIR_FILTER *iirFilter,
                 float input
                 )

{
  int j;
  float y;
  int ptr = iirFilter->ptr;
  int i = ptr + (BUFFER_SIZE-1);

  COUNT_sub_start("AdvanceARFilter");

  INDIRECT(1); MOVE(1); ADD(1); /* counting previous operations */

  INDIRECT(2); MULT(1); ADD(1);
  y = input + (iirFilter->coeffIIRb[1] * (-iirFilter->ring_buf_2[i & (BUFFER_SIZE-1)]));

  PTR_INIT(4); /* iirFilter->noOffCoeffs
                  iirFilter->coeffIIRb[]
                  iirFilter->ring_buf_2[i]
                  iirFilter->ring_buf_2[ptr]
               */

  LOOP(1);
  for (j=2; j<iirFilter->noOffCoeffs; j++) {
    i--;

    MULT(1); MAC(1);
    y += (iirFilter->coeffIIRb[j] * (-iirFilter->ring_buf_2[i & (BUFFER_SIZE-1)]));
  }

  MOVE(1);
  iirFilter->ring_buf_2[ptr] = y;

  /* pointer update */
  iirFilter->ptr = (ptr+1) & (BUFFER_SIZE-1);

  COUNT_sub_end();

}
#else  //NEWIIR



/*
  faster simple folding operation    

  returns filtered value
*/


static float
AdvanceIIRFilter(IIR_FILTER *iirFilter,
                 float input
                 )

{
  float y = 0.0f;
  int j = 0;
  int i;

  COUNT_sub_start("AdvanceIIRFilter");

  MOVE(2); /* counting previous operations */

  INDIRECT(1); MOVE(1);
  iirFilter->ring_buf_1[iirFilter->ptr] = input;

  PTR_INIT(4); /* pointer for iirFilter->ring_buf_1,
                              iirFilter->ring_buf_2,
                              iirFilter->coeffIIRa,
                              iirFilter->coeffIIRb
               */
  ADD(1); LOOP(1);
  for (i = iirFilter->ptr; i > iirFilter->ptr - iirFilter->noOffCoeffs; i--, j++) {
    MULT(2); ADD(1);
    y += iirFilter->coeffIIRa[j] * iirFilter->ring_buf_1[i & (BUFFER_SIZE - 1)] - iirFilter->coeffIIRb[j] * iirFilter->ring_buf_2[i & (BUFFER_SIZE - 1)];
  }

  MOVE(1);
  iirFilter->ring_buf_2[(iirFilter->ptr) & (BUFFER_SIZE - 1)] = y;

  iirFilter->ptr = ++iirFilter->ptr & (BUFFER_SIZE - 1);

  COUNT_sub_end();

  return y;
}
#endif  //NEWIIR






/*
  Downsample numInSamples of type short

  returns  success of operation
*/

int 
IIR21_Downsample(IIR21_RESAMPLER *DownSampler,
                 float *inSamples,
                 int numInSamples,
                 int inStride,
                 float *outSamples,
                 int *numOutSamples,
                 int outStride
               )
{
  int i;
  *numOutSamples=0;

  COUNT_sub_start("Downsample");

  MOVE(1); /* counting previous operations */

  PTR_INIT(2); /* pointer for inSamples[],
                              outSamples[]
               */
  LOOP(1); 
  for(i=0;i<numInSamples;i++){
    float iirOut;

#ifdef NEWIIR
    FUNC(2);
    AdvanceARFilter(&(DownSampler->iirFilter), inSamples[i*inStride]);
#else
    FUNC(2);
    iirOut = AdvanceIIRFilter(&(DownSampler->iirFilter), inSamples[i*inStride]);
#endif

    ADD(1);
    DownSampler->pending++;

    ADD(1); BRANCH(1);
    if(DownSampler->pending == DownSampler->ratio){

#ifdef NEWIIR
      FUNC(1);
      outSamples[(*numOutSamples)*outStride] = AdvanceMAFilter(&(DownSampler->iirFilter));;
#else
      MOVE(1);
      outSamples[(*numOutSamples)*outStride] = iirOut;
#endif

      (*numOutSamples)++;

      MOVE(1);
      DownSampler->pending=0;
    }
  }

  COUNT_sub_end();

  return 1;
}


int 
IIR21_Upsample(IIR21_RESAMPLER *UpSampler,
               float *inSamples,
               int numInSamples,
               int inStride,
               float *outSamples,
               int *numOutSamples,
               int outStride
               )
{
  int i,k;
  int idxOut=0;

  COUNT_sub_start("Upsample");

  MOVE(1); /* counting previous operations */

  PTR_INIT(2); /* pointer for inSamples[],
                              outSamples[]
               */
  LOOP(1); 
  for(i=0;i<numInSamples;i++){

    MULT(1); FUNC(2); STORE(1);
    outSamples[idxOut] = AdvanceIIRFilter(&(UpSampler->iirFilter), inSamples[i*inStride] * UpSampler->ratio);

    idxOut += outStride;

    LOOP(1);
    for (k=0; k<UpSampler->ratio-1; k++) {

      FUNC(2); STORE(1);
      outSamples[idxOut] = AdvanceIIRFilter(&(UpSampler->iirFilter), 0.0f);

      idxOut += outStride;
    }      
  }

  MULT(1); STORE(1);
  *numOutSamples=numInSamples*UpSampler->ratio;

  COUNT_sub_end();

  return 1;
}
