#ifndef _IIR_32_RESAMPLE_H_
#define _IIR_32_RESAMPLE_H_


int
IIR32Resample( float *inbuf,
               float *outbuf,
               int    inSamples,
               int    outSamples,
               int    stride);

int
IIR32GetResamplerFeed( int blockSizeOut);


void
IIR32Init( void);

#endif
