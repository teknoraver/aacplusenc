/*
   Resampler Tool Box
*/

#ifndef __RESAMPLER_H
#define __RESAMPLER_H

#define BUFFER_SIZE 32  


typedef struct
{
  const float *coeffIIRa;
  const float *coeffIIRb;
  int noOffCoeffs;
  float ring_buf_1[BUFFER_SIZE];
  float ring_buf_2[BUFFER_SIZE];
  int ptr;
} IIR_FILTER;


typedef struct
{
  IIR_FILTER iirFilter;
  int ratio;
  int delay;
  int pending;
} IIR21_RESAMPLER;



int 
InitIIR21_Resampler(IIR21_RESAMPLER *ReSampler);

int 
IIR21_Downsample(IIR21_RESAMPLER *DownSampler,
                 float *inSamples,
                 int numInSamples,
                 int inStride,
                 float *outSamples,
                 int *numOutSamples,
                 int outstride);

int 
IIR21_Upsample( IIR21_RESAMPLER  *UpSampler,
                float        *inSamples,
                int           numInSamples,
                int           inStride,
                float        *outSamples,
                int          *numOutSamples,
                int           outstride);
#endif


