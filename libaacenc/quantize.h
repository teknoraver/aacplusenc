/*
  Quantization submodule
*/
#ifndef _QUANTIZE_H_
#define _QUANTIZE_H_

#define MAX_QUANT 8191

void initQuantTables(void);

void QuantizeSpectrum(int sfbCnt, 
                      int maxSfbPerGroup,
                      int sfbPerGroup,
                      int *sfbOffset, float *mdctSpectrum,
                      int globalGain, short *scalefactors,
                      short *quantizedSpectrum);

void calcExpSpec(float *expSpec, float *mdctSpectrum, int noOfLines);

float calcSfbDist(const float *spec,
                  const float *expSpec,
                  short *quantSpec,
                   int  sfbWidth,
                   int  gain);

#endif /* _QUANTIZE_H_ */
