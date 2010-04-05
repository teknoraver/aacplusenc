/*
  MDCT transform
*/
#include "psy_const.h"
#include "transform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "aac_rom.h"
#include "cfftn.h"

#include "counters.h" /* the 3GPP instrumenting tool */

#define LS_TRANS ((FRAME_LEN_LONG-FRAME_LEN_SHORT)/2) /* 448 */



static void preModulationDCT(float *x,int m,const float *sineWindow)
{
  int i;
  float wre, wim, re1, re2, im1, im2;

  COUNT_sub_start("preModulationDCT");

  PTR_INIT(4); /* pointers for x[2*i],
                               x[m-1-2*i],
                               sineWindow[i*2],
                               sineWindow[m-1-2*i]
               */
  LOOP(1);
  for(i=0;i<m/4;i++){

    MOVE(4);
    re1 = x[2*i];
    im2 = x[2*i+1];
    re2 = x[m-2-2*i];
    im1 = x[m-1-2*i];

    MOVE(2);
    wim = sineWindow[i*2];
    wre = sineWindow[m-1-2*i];

    MULT(1); MAC(1); STORE(1);
    x[2*i]   = im1*wim + re1* wre;

    MULT(2); ADD(1); STORE(1);
    x[2*i+1] = im1*wre - re1* wim;

    MOVE(2);
    wim = sineWindow[m-2-2*i];
    wre = sineWindow[2*i+1];

    MULT(1); MAC(1); STORE(1);
    x[m-2-2*i] = im2*wim + re2*wre;

    MULT(2); ADD(1); STORE(1);
    x[m-1-2*i] = im2*wre - re2*wim;
  }

  COUNT_sub_end();
}


static void postModulationDCT(float *x,int m, const float *trigData, int step,int trigDataSize)
{
  int i;
  float wre, wim, re1, re2, im1, im2;
  const float *sinPtr = trigData;
  const float *cosPtr = trigData+trigDataSize;

  COUNT_sub_start("postModulationDCT");

  PTR_INIT(2); /* counting previous operations */

  MOVE(2);
  wim = *sinPtr;
	wre = *cosPtr;

  PTR_INIT(2); /* pointers for x[2*i],
                               x[m-1-2*i]
               */
  LOOP(1);
  for(i=0;i<m/4;i++){

    MOVE(4);
    re1=x[2*i];
    im1=x[2*i+1];
    re2=x[m-2-2*i];
    im2=x[m-1-2*i];

    MULT(1); MAC(1); STORE(1);
    x[2*i]=(re1*wre + im1*wim);

    MULT(2); ADD(1); STORE(1);
    x[m-1-2*i]=(re1*wim - im1*wre);

    sinPtr+=step;
    cosPtr-=step;

    MOVE(2);
    wim=*sinPtr;
    wre=*cosPtr;

    MULT(1); MAC(1); STORE(1);
    x[m-2-2*i] = (re2*wim + im2* wre);

    MULT(2); ADD(1); STORE(1);
    x[2*i+1]   = (re2*wre - im2* wim);
  }

  COUNT_sub_end();
}


static void mdct(float *dctdata,
                 const float *trigData,
                 const float *sineWindow,
                 int n,
                 int ld_n)
{
  
  COUNT_sub_start("mdct");

  FUNC(3);
  preModulationDCT(dctdata,n,sineWindow);

  SHIFT(1); FUNC(3);
  CFFTN(dctdata,n/2,-1);

  assert (LD_FFT_TWIDDLE_TABLE_SIZE >= ld_n-1);

  ADD(1); SHIFT(1); FUNC(5);
  postModulationDCT(dctdata, n, trigData, 1 << (LD_FFT_TWIDDLE_TABLE_SIZE - (ld_n - 1)), FFT_TWIDDLE_TABLE_SIZE);

  COUNT_sub_end();
}




/*!
    \brief 
    the mdct delay buffer has a size of 1600,
    so the calculation of LONG,START,STOP must be  spilt in two 
    passes with 1024 samples and a mid shift,
    the SHORT transforms can be completed in the delay buffer,
    and afterwards a shift
*/

static void shiftMdctDelayBuffer(
                                 float *mdctDelayBuffer, /*! start of mdct delay buffer */
                                 float *timeSignal,      /*! pointer to new time signal samples, interleaved */
                                 int chIncrement         /*! number of channels */
                                 )
{
  int i;

  COUNT_sub_start("shiftMdctDelayBuffer");

  FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(BLOCK_SWITCHING_OFFSET-FRAME_LEN_LONG);
  memmove(mdctDelayBuffer,mdctDelayBuffer+FRAME_LEN_LONG,(BLOCK_SWITCHING_OFFSET-FRAME_LEN_LONG)*sizeof(float));

  PTR_INIT(2); /* pointers for mdctDelayBuffer[],
                               timeSignal[]
               */
  LOOP(1);
  for(i=0;i<FRAME_LEN_LONG ;i++) {

    MOVE(1);
    mdctDelayBuffer[BLOCK_SWITCHING_OFFSET-FRAME_LEN_LONG+i] = timeSignal[i*chIncrement];
  }

  COUNT_sub_end();
}

int Transform_Real(float *mdctDelayBuffer,float *timeSignal,int chIncrement,float *realOut,int blockType)
{
  int i,w;
  float timeSignalSample;
  float ws1, ws2;

  float *dctIn;

  COUNT_sub_start("Transform_Real");

  BRANCH(2);
  switch(blockType){


    case LONG_WINDOW:
      PTR_INIT(1);
      dctIn = realOut;

      PTR_INIT(5); /* pointer for mdctDelayBuffer[i],
                                  mdctDelayBuffer[FRAME_LEN_LONG-i-1],
                                  LongWindowKBD[i],
                                  LongWindowKBD[FRAME_LEN_LONG-i-1],
                                  dctIn[FRAME_LEN_LONG/2+i] 
                   */
      LOOP(1);
      for(i=0;i<FRAME_LEN_LONG/2;i++){

        MOVE(1);
        timeSignalSample = mdctDelayBuffer[i];

        MULT(1);
        ws1 = timeSignalSample * LongWindowKBD[i];

        MOVE(1);
        timeSignalSample = mdctDelayBuffer[(FRAME_LEN_LONG-i-1)];

        MULT(1);
        ws2 = timeSignalSample * LongWindowKBD[FRAME_LEN_LONG-i-1];

        ADD(1); STORE(1);
        dctIn[FRAME_LEN_LONG/2+i] = ws1 - ws2;
      }

      FUNC(3);
      shiftMdctDelayBuffer(mdctDelayBuffer,timeSignal,chIncrement);

      PTR_INIT(5); /* pointer for mdctDelayBuffer[i],
                                  mdctDelayBuffer[FRAME_LEN_LONG-i-1],
                                  LongWindowKBD[i],
                                  LongWindowKBD[FRAME_LEN_LONG-i-1],
                                  dctIn[FRAME_LEN_LONG/2+i] 
                   */
      LOOP(1);
      for(i=0;i<FRAME_LEN_LONG/2;i++){
      
        MOVE(1);
        timeSignalSample = mdctDelayBuffer[i];

        MULT(1);
        ws1 = timeSignalSample * LongWindowKBD[FRAME_LEN_LONG-i-1];

        MOVE(1);
        timeSignalSample = mdctDelayBuffer[(FRAME_LEN_LONG-i-1)];

        MULT(1);
        ws2 = timeSignalSample * LongWindowKBD[i];

        ADD(1); MULT(1); STORE(1);
        dctIn[FRAME_LEN_LONG/2-i-1] = -(ws1 + ws2);

      }

      FUNC(5);
      mdct(dctIn, fftTwiddleTab, LongWindowSine, FRAME_LEN_LONG, 10);

    break;

    case START_WINDOW:
      PTR_INIT(1);
      dctIn = realOut;

    PTR_INIT(5); /* pointer for mdctDelayBuffer[i],
                                mdctDelayBuffer[FRAME_LEN_LONG-i-1],
                                LongWindowKBD[i],
                                LongWindowKBD[FRAME_LEN_LONG-i-1],
                                dctIn[FRAME_LEN_LONG/2+i] 
                   */
    LOOP(1);
    for(i=0;i<FRAME_LEN_LONG/2;i++){

      MOVE(1);
      timeSignalSample = mdctDelayBuffer[i];

      MULT(1);
      ws1 = timeSignalSample * LongWindowKBD[i];

      MOVE(1);
      timeSignalSample = mdctDelayBuffer[(FRAME_LEN_LONG-i-1)];

      MULT(1);
      ws2 = timeSignalSample * LongWindowKBD[FRAME_LEN_LONG-i-1];

      ADD(1); STORE(1);
      dctIn[FRAME_LEN_LONG/2+i] = ws1 - ws2;
    }

    FUNC(3);
    shiftMdctDelayBuffer(mdctDelayBuffer,timeSignal,chIncrement);
 

    PTR_INIT(2); /* pointer for mdctDelayBuffer[i],
                                dctIn[FRAME_LEN_LONG/2+i] 
                 */
    LOOP(1);
    for(i=0;i<LS_TRANS;i++)
    {
      MOVE(3);
      timeSignalSample = mdctDelayBuffer[i];
      ws1 = timeSignalSample;
      ws2 = 0.0f;

      ADD(1); MULT(1); STORE(1);
      dctIn[FRAME_LEN_LONG/2-i-1] = -(ws1 + ws2);
    }
    
    PTR_INIT(5); /* pointer for mdctDelayBuffer[i+LS_TRANS],
                                mdctDelayBuffer[FRAME_LEN_LONG-i-1-LS_TRANS],
                                ShortWindowSine[i],
                                ShortWindowSine[FRAME_LEN_SHORT-i-1],
                                dctIn[FRAME_LEN_LONG/2-i-1-LS_TRANS]
                 */
    LOOP(1);
    for(i=0;i<FRAME_LEN_SHORT/2;i++){

      MOVE(1);
      timeSignalSample= mdctDelayBuffer[i+LS_TRANS];

      MULT(1);
      ws1 = timeSignalSample * ShortWindowSine[FRAME_LEN_SHORT-i-1];

      MOVE(1);
      timeSignalSample= mdctDelayBuffer[(FRAME_LEN_LONG-i-1-LS_TRANS)];

      MULT(1);
      ws2 = timeSignalSample * ShortWindowSine[i];

      ADD(1); MULT(1); STORE(1);
      dctIn[FRAME_LEN_LONG/2-i-1-LS_TRANS] = -(ws1 + ws2);
    }

    FUNC(5);
    mdct(dctIn, fftTwiddleTab, LongWindowSine, FRAME_LEN_LONG, 10);

    break;

    case STOP_WINDOW:

    PTR_INIT(1);
    dctIn = realOut;

    PTR_INIT(2); /* pointer for mdctDelayBuffer[FRAME_LEN_LONG-i-1],
                                dctIn[FRAME_LEN_LONG/2+i] 
                   */
    LOOP(1);
    for(i=0;i<LS_TRANS;i++){

      MOVE(3);
      ws1 = 0.0f;
      timeSignalSample= mdctDelayBuffer[(FRAME_LEN_LONG-i-1)];
      ws2 = timeSignalSample;

      ADD(1); STORE(1);
      dctIn[FRAME_LEN_LONG/2+i] = ws1 - ws2;
    }
    
    PTR_INIT(5); /* pointer for mdctDelayBuffer[i+LS_TRANS],
                                mdctDelayBuffer[FRAME_LEN_LONG-i-1-LS_TRANS],
                                ShortWindowSine[i],
                                ShortWindowSine[FRAME_LEN_SHORT-i-1],
                                dctIn[FRAME_LEN_LONG/2-i-1-LS_TRANS]
                 */
    LOOP(1);
    for(i=0;i<FRAME_LEN_SHORT/2;i++){

      MOVE(1);
      timeSignalSample= mdctDelayBuffer[(i+LS_TRANS)];

      MULT(1);
      ws1 = timeSignalSample * ShortWindowSine[i];

      MOVE(1);
      timeSignalSample= mdctDelayBuffer[(FRAME_LEN_LONG-LS_TRANS-i-1)];

      MULT(1);
      ws2 = timeSignalSample * ShortWindowSine[FRAME_LEN_SHORT-i-1];

      ADD(1); STORE(1);
      dctIn[FRAME_LEN_LONG/2+i+LS_TRANS] = ws1 - ws2;
    }

    FUNC(3);
    shiftMdctDelayBuffer(mdctDelayBuffer,timeSignal,chIncrement);
 

    PTR_INIT(5); /* pointer for mdctDelayBuffer[i],
                                mdctDelayBuffer[FRAME_LEN_LONG-i-1],
                                LongWindowKBD[i],
                                LongWindowKBD[FRAME_LEN_LONG-i-1],
                                dctIn[FRAME_LEN_LONG/2+i] 
                 */
    LOOP(1);
    for(i=0;i<FRAME_LEN_LONG/2;i++){

      MOVE(1);
      timeSignalSample= mdctDelayBuffer[i];

      MULT(1);
      ws1 = timeSignalSample * LongWindowKBD[FRAME_LEN_LONG-i-1];

      MOVE(1);
      timeSignalSample= mdctDelayBuffer[(FRAME_LEN_LONG-i-1)];

      MULT(1);
      ws2 = timeSignalSample * LongWindowKBD[i];

      ADD(1); MULT(1); STORE(1);
      dctIn[FRAME_LEN_LONG/2-i-1] = -(ws1 + ws2);
    }

    FUNC(5);
    mdct(dctIn, fftTwiddleTab, LongWindowSine, FRAME_LEN_LONG, 10);

    break;

    case SHORT_WINDOW:

    LOOP(1);
    for(w=0;w<TRANS_FAC;w++){

      PTR_INIT(1);
      dctIn = realOut+w*FRAME_LEN_SHORT;

      PTR_INIT(8); /* pointer for mdctDelayBuffer[TRANSFORM_OFFSET_SHORT+w*FRAME_LEN_SHORT+i],
                                  mdctDelayBuffer[TRANSFORM_OFFSET_SHORT+w*FRAME_LEN_SHORT+FRAME_LEN_SHORT-i-1],
                                  mdctDelayBuffer[TRANSFORM_OFFSET_SHORT+w*FRAME_LEN_SHORT+FRAME_LEN_SHORT+i],
                                  mdctDelayBuffer[TRANSFORM_OFFSET_SHORT+w*FRAME_LEN_SHORT+FRAME_LEN_SHORT*2-i-1],
                                  ShortWindowSine[i],
                                  ShortWindowSine[FRAME_LEN_SHORT-i-1],
                                  dctIn[FRAME_LEN_SHORT/2+i],
                                  dctIn[FRAME_LEN_SHORT/2-i-1]
                   */
      LOOP(1);
      for(i=0;i<FRAME_LEN_SHORT/2;i++){

        MOVE(1);
        timeSignalSample= mdctDelayBuffer[TRANSFORM_OFFSET_SHORT+w*FRAME_LEN_SHORT+i];

        MULT(1);
        ws1 = timeSignalSample * ShortWindowSine[i];

        MOVE(1);
        timeSignalSample= mdctDelayBuffer[TRANSFORM_OFFSET_SHORT+w*FRAME_LEN_SHORT+FRAME_LEN_SHORT-i-1];

        MULT(1);
        ws2 = timeSignalSample * ShortWindowSine[FRAME_LEN_SHORT-i-1];

        ADD(1); STORE(1);
        dctIn[FRAME_LEN_SHORT/2+i] = ws1 - ws2;
        
        MOVE(1);
        timeSignalSample= mdctDelayBuffer[TRANSFORM_OFFSET_SHORT+w*FRAME_LEN_SHORT+FRAME_LEN_SHORT+i];

        MULT(1);
        ws1 = timeSignalSample * ShortWindowSine[FRAME_LEN_SHORT-i-1];

        MOVE(1);
        timeSignalSample= mdctDelayBuffer[TRANSFORM_OFFSET_SHORT+w*FRAME_LEN_SHORT+FRAME_LEN_SHORT*2-i-1];

        MULT(1);
        ws2 = timeSignalSample * ShortWindowSine[i];

        ADD(1); MULT(1); STORE(1);
        dctIn[FRAME_LEN_SHORT/2-i-1] = -(ws1 + ws2);
      }

      FUNC(5);
      mdct(dctIn, fftTwiddleTab, ShortWindowSine, FRAME_LEN_SHORT, 7);
    }

    FUNC(3);
    shiftMdctDelayBuffer(mdctDelayBuffer,timeSignal,chIncrement);
    break;

  }
  COUNT_sub_end();

  return 0;
}
