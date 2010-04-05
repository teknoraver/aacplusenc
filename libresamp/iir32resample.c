#include <assert.h>
#include <stdio.h>
#include <string.h>  /* memmove() */
#include "FloatFR.h"
#include "iir32resample.h"

#include "counters.h" /* the 3GPP instrumenting tool */

#define IIR_UPSAMPLE_FAC  2


#define IIR_32_ORDER  8

static const float coeffNum[IIR_32_ORDER] = 
  {2.129719423e-03f*IIR_UPSAMPLE_FAC, -9.219380130e-04f*IIR_UPSAMPLE_FAC, 3.859704087e-03f*IIR_UPSAMPLE_FAC, 1.218339222e-03f*IIR_UPSAMPLE_FAC, 
   1.218339222e-03f*IIR_UPSAMPLE_FAC, 3.859704087e-03f*IIR_UPSAMPLE_FAC, -9.219380130e-04f*IIR_UPSAMPLE_FAC, 2.129719423e-03f*IIR_UPSAMPLE_FAC};
static const float coeffDen[IIR_32_ORDER] = 
  {-1.000000000e+00f, 4.917738074e+00f, -1.129019179e+01f, 1.541498076e+01f, 
   -1.342576947e+01f, 7.432055685e+00f, -2.419025499e+00f, 3.576405931e-01f};



#define IIR_CHANNELS 2
#define IIR_DOWNSAMPLE_FAC  3
#define IIR_INTERNAL_BUFSIZE (IIR_UPSAMPLE_FAC*IIR_DOWNSAMPLE_FAC + IIR_32_ORDER)

static float statesIIR[IIR_32_ORDER * IIR_CHANNELS];


int
IIR32Resample( float *inbuf,
               float *outbuf,
               int    inSamples,
               int    outSamples,
               int    stride)
{
  int i, k, s, ch, r;
  double accu;
  float  scratch[IIR_INTERNAL_BUFSIZE];
  int nProcessRuns  = outSamples  >> 1;

  COUNT_sub_start("IIR32Resample");

  SHIFT(1); /* counting previous operation */

  assert( stride <= IIR_CHANNELS);

  LOOP(1);
  for (ch=0; ch<stride; ch++) {
    int idxIn  = ch;
    int idxOut = ch;

    MOVE(2); /* counting previous operations */

    PTR_INIT(2); /* scratch[s]
                    statesIIR[s*stride+ch]
                 */
    LOOP(1);
    for (s=0; s<IIR_32_ORDER; s++) {

      MOVE(1);
      scratch[s] = statesIIR[s*stride+ch];
    }

    LOOP(1);
    for (r=0; r<nProcessRuns; r++) {

      PTR_INIT(1); /* scratch[s] */
      MOVE(1);
      s=IIR_32_ORDER;

      PTR_INIT(2); /* inbuf[idxIn] 
                      outbuf[idxOut]
                   */
      LOOP(1);
      for (i=0; i<IIR_DOWNSAMPLE_FAC; i++) {

        MOVE(1);
        accu = inbuf[idxIn];

        PTR_INIT(2); /* coeffDen[k]
                        scratch[s-k]
                     */
        LOOP(1);
        for (k=1; k<IIR_32_ORDER; k++) {
          MAC(1);
          accu += coeffDen[k] * scratch[s-k];
        }

        MOVE(1);
        scratch[s] = (float) accu;

        s++;

        assert( s<=IIR_INTERNAL_BUFSIZE);

        MOVE(1);
        accu = 0.0;

        PTR_INIT(2); /* coeffDen[k]
                        scratch[s-k]
                     */
        LOOP(1);
        for (k=1; k<IIR_32_ORDER; k++) {

          MAC(1);
          accu += coeffDen[k] * scratch[s-k];
        }

        MOVE(1);
        scratch[s] = (float) accu;

        s++;

        idxIn += stride;

      }

      PTR_INIT(1); /* scratch[s] */
      MOVE(1);
      s = IIR_32_ORDER;

      LOOP(1);
      for (i=0; i<IIR_UPSAMPLE_FAC; i++) {

        MULT(1);
        accu = coeffNum[0] * scratch[s];

        PTR_INIT(2); /* coeffNum[k]
                        scratch[s-k]
                     */
        LOOP(1);
        for (k=1; k<IIR_32_ORDER; k++) {

          MAC(1);
          accu += coeffNum[k] * scratch[s-k];
        }

        MOVE(1);
        outbuf[idxOut] = (float) accu;

        assert( s<=IIR_INTERNAL_BUFSIZE);

        s += IIR_DOWNSAMPLE_FAC;

        idxOut += stride;

      }

      FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(IIR_32_ORDER);
      memmove( &scratch[0], &scratch[IIR_UPSAMPLE_FAC*IIR_DOWNSAMPLE_FAC], IIR_32_ORDER*sizeof(float));

    }

    PTR_INIT(2); /* statesIIR[s*stride+ch]
                    scratch[s]
                 */
    LOOP(1);
    for (s=0; s<IIR_32_ORDER; s++) {

      MOVE(1);
      statesIIR[s*stride+ch] = scratch[s];
    }

    assert(idxIn/stride <= inSamples);

  } /* ch */

  MULT(1); /* counting post-operation */

  COUNT_sub_end();
  
  return outSamples * stride;
}

int
IIR32GetResamplerFeed( int blockSizeOut)
{
  int size;

  COUNT_sub_start("IIR32GetResamplerFeed");

  MULT(1);
  size  = blockSizeOut * 3;

  DIV(1);
  size /= 2;

  COUNT_sub_end();

  return size;
}



void
IIR32Init( void)
{
  unsigned int s;

  COUNT_sub_start("IIR32Init");

  PTR_INIT(1); /* statesIIR[] */
  LOOP(1);
  for (s=0; s<sizeof(statesIIR)/sizeof(float); s++) {

    MOVE(1);
    statesIIR[s] = 0.0f;
  }

  COUNT_sub_end();
}
