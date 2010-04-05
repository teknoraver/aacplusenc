/*
  CFFTN header file
*/

#ifndef __cfftn_h
#define __cfftn_h

#include "../config.h"

#ifdef _FFTW3

void init_plans();
void destroy_plans();

#else

#define init_plans()
#define destroy_plans()

int cfftn(float Re[], float Im[], int  nTotal, int  nPass, int  nSpan, int  iSign);

#endif

int CFFTN(float *afftData, int len, int isign);

#endif
