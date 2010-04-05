/*
  Block Switching
*/
#include "psy_const.h"
#include "block_switch.h"
#include "minmax.h"
#include <stdio.h>
#include <math.h>

#include "counters.h" /* the 3GPP instrumenting tool */


static float
IIRFilter(const float in, const float coeff[], float states[]);

static float
SrchMaxWithIndex(const float *in, int *index, int n);

static int
CalcWindowEnergy(BLOCK_SWITCHING_CONTROL *blockSwitchingControl,
                 float *timeSignal,
                 int chIncrement,
                 int windowLen);

/*
  IIR high pass coeffs
*/
static const float hiPassCoeff[BLOCK_SWITCHING_IIR_LEN]=
{
  -0.5095f, 0.7548f
};

#define accWindowNrgFac         0.3f
#define oneMinusAccWindowNrgFac 0.7f
#define invAttackRatioHighBr    0.1f
#define invAttackRatioLowBr     0.056f;
#define minAttackNrg (1e+6*NORM_PCM_ENERGY)


int InitBlockSwitching(BLOCK_SWITCHING_CONTROL *blockSwitchingControl,
                       const int bitRate, const int nChannels)
{
  COUNT_sub_start("InitBlockSwitching");
  
  /* select attackRatio */
  ADD(4); DIV(1); LOGIC(3); BRANCH(1);
  if ((nChannels==1 && bitRate > 24000) || (nChannels>1 && bitRate/nChannels > 16000)) {
    INDIRECT(1); MOVE(1);
    blockSwitchingControl->invAttackRatio = invAttackRatioHighBr;
  }
  else  {
    INDIRECT(1); MOVE(1);
    blockSwitchingControl->invAttackRatio = invAttackRatioLowBr;
  }
  COUNT_sub_end();
  
  return TRUE;
}




static const int suggestedGroupingTable[TRANS_FAC][MAX_NO_OF_GROUPS] =
{
    /* Attack in Window 0 */ {1,  3,  3,  1},
    /* Attack in Window 1 */ {1,  1,  3,  3},
    /* Attack in Window 2 */ {2,  1,  3,  2},
    /* Attack in Window 3 */ {3,  1,  3,  1},
    /* Attack in Window 4 */ {3,  1,  1,  3},
    /* Attack in Window 5 */ {3,  2,  1,  2},
    /* Attack in Window 6 */ {3,  3,  1,  1},
    /* Attack in Window 7 */ {3,  3,  1,  1}
};


int BlockSwitching(BLOCK_SWITCHING_CONTROL *blockSwitchingControl,
                   float *timeSignal,
                   int chIncrement)
{
    int i,w;
    float enM1, enMax;

    COUNT_sub_start("BlockSwitching");

    PTR_INIT(1); /* blockSwitchingControl->groupLen[] */
    LOOP(1);
    for (i = 0; i < TRANS_FAC; i++)
    {
        MOVE(1);
        blockSwitchingControl->groupLen[i] = 0;
    }


    INDIRECT(2); PTR_INIT(2); FUNC(3); STORE(1);
    blockSwitchingControl->maxWindowNrg = SrchMaxWithIndex( &blockSwitchingControl->windowNrg[0][BLOCK_SWITCH_WINDOWS-1],
                                                            &blockSwitchingControl->attackIndex,
                                                            BLOCK_SWITCH_WINDOWS);

    INDIRECT(2); MOVE(1);
    blockSwitchingControl->attackIndex = blockSwitchingControl->lastAttackIndex;

    INDIRECT(1); MOVE(1);
    blockSwitchingControl->noOfGroups = MAX_NO_OF_GROUPS;

    INDIRECT(1);
    PTR_INIT(2); /* blockSwitchingControl->groupLen[]
                    suggestedGroupingTable[][]
                 */
    LOOP(1);
    for (i = 0; i < MAX_NO_OF_GROUPS; i++)
    {
        MOVE(1);
        blockSwitchingControl->groupLen[i] = suggestedGroupingTable[blockSwitchingControl->attackIndex][i];
    }


    PTR_INIT(4); /* blockSwitchingControl->windowNrg[0][]
                    blockSwitchingControl->windowNrg[1][]
                    blockSwitchingControl->windowNrgF[0][]
                    blockSwitchingControl->windowNrgF[1][]
                 */
    LOOP(1);
    for (w = 0; w < BLOCK_SWITCH_WINDOWS;w++) {

        MOVE(2);
        blockSwitchingControl->windowNrg[0][w] = blockSwitchingControl->windowNrg[1][w];
        blockSwitchingControl->windowNrgF[0][w] = blockSwitchingControl->windowNrgF[1][w];
    }


    FUNC(4);
    CalcWindowEnergy(blockSwitchingControl, timeSignal,chIncrement,BLOCK_SWITCH_WINDOW_LEN);


    INDIRECT(1); MOVE(1);
    blockSwitchingControl->attack = FALSE;

    MOVE(2);
    enMax = 0.0;
    enM1 = blockSwitchingControl->windowNrgF[0][BLOCK_SWITCH_WINDOWS-1];

    PTR_INIT(1); /* blockSwitchingControl->windowNrgF[1][] */
    LOOP(1);
    for (w=0; w<BLOCK_SWITCH_WINDOWS; w++) {
      
      INDIRECT(1); MULT(1); MAC(1); STORE(1);
      blockSwitchingControl->accWindowNrg = (oneMinusAccWindowNrgFac * blockSwitchingControl->accWindowNrg) + 
                                            (accWindowNrgFac * enM1);

      
      INDIRECT(2); MULT(1); ADD(1); BRANCH(1);
      if (blockSwitchingControl->windowNrgF[1][w] * blockSwitchingControl->invAttackRatio > blockSwitchingControl->accWindowNrg ) {

        INDIRECT(2); MOVE(2);
        blockSwitchingControl->attack = TRUE;
        blockSwitchingControl->lastAttackIndex = w;
      }

      MOVE(1);
      enM1 = blockSwitchingControl->windowNrgF[1][w];

      ADD(1); BRANCH(1); MOVE(1);
      enMax = max(enMax, enM1);
    }

    ADD(1); BRANCH(1);
    if (enMax < minAttackNrg) {

      INDIRECT(1); MOVE(1);
      blockSwitchingControl->attack = FALSE;
    }

    INDIRECT(2); LOGIC(1); BRANCH(1);
    if((!blockSwitchingControl->attack) &&
       (blockSwitchingControl->lastattack))
    {
        INDIRECT(1); ADD(1); BRANCH(1);
        if (blockSwitchingControl->attackIndex == TRANS_FAC-1)
        {
          INDIRECT(1); MOVE(1);
          blockSwitchingControl->attack = TRUE;
        }

        INDIRECT(1); MOVE(1);
        blockSwitchingControl->lastattack = FALSE;
    }
    else
    {
        INDIRECT(2); MOVE(1);
        blockSwitchingControl->lastattack = blockSwitchingControl->attack;
    }


  INDIRECT(2); MOVE(1);
  blockSwitchingControl->windowSequence =  blockSwitchingControl->nextwindowSequence;

  INDIRECT(1); BRANCH(1);
  if(blockSwitchingControl->attack){

    INDIRECT(1); MOVE(1);
    blockSwitchingControl->nextwindowSequence = SHORT_WINDOW;
  }
  else{

    INDIRECT(1); MOVE(1);
    blockSwitchingControl->nextwindowSequence=LONG_WINDOW;
  }

  INDIRECT(1); ADD(1); BRANCH(1);
  if(blockSwitchingControl->nextwindowSequence == SHORT_WINDOW){

    ADD(1); BRANCH(1);
    if(blockSwitchingControl->windowSequence == LONG_WINDOW)
    {
      INDIRECT(1); MOVE(1);
      blockSwitchingControl->windowSequence=START_WINDOW;
    }

    INDIRECT(1); ADD(1); BRANCH(1);
    if(blockSwitchingControl->windowSequence == STOP_WINDOW) {

      INDIRECT(1); MOVE(1);
      blockSwitchingControl->windowSequence = SHORT_WINDOW;

      INDIRECT(2); MOVE(4);
      blockSwitchingControl->noOfGroups = 3;
      blockSwitchingControl->groupLen[0] = 3;
      blockSwitchingControl->groupLen[1] = 3;
      blockSwitchingControl->groupLen[2] = 2;
    }
  }

  INDIRECT(1); ADD(1); BRANCH(1);
  if(blockSwitchingControl->nextwindowSequence == LONG_WINDOW){

    INDIRECT(1); ADD(1); BRANCH(1);
    if(blockSwitchingControl->windowSequence == SHORT_WINDOW)
    {
      INDIRECT(1); MOVE(1);
      blockSwitchingControl->nextwindowSequence = STOP_WINDOW;
    }
  }

  COUNT_sub_end();

  return TRUE;
}



static float SrchMaxWithIndex(const float in[], int *index, int n)
{
  float max;
  int i,idx;
  
  COUNT_sub_start("SrchMaxWithIndex");
  
  /* Search maximum value in array and return index and value */
  MOVE(2);
  max = 0.0f;
  idx = 0;
  
  PTR_INIT(1); /* in[] */
  LOOP(1);
  for (i = 0; i < n; i++) {

    ADD(1); BRANCH(1);
    if (in[i+1] > max) {
      MOVE(2);
      max = in[i+1];
      idx = i;
    }
  }
  MOVE(1);
  *index = idx;
  
  COUNT_sub_end();
  
  return max;
}



static int CalcWindowEnergy(BLOCK_SWITCHING_CONTROL *blockSwitchingControl,
                            float *timeSignal,
                            int chIncrement,
                            int windowLen)
{
    int w, i;
    float accuUE,accuFE;
    float tempUnfiltered, tempFiltered;

    COUNT_sub_start("CalcWindowEnergy");

    PTR_INIT(2); /* pointers for blockSwitchingControl->windowNrg[1][w],
                                 blockSwitchingControl->windowNrgF[1][w]
                 */
    LOOP(1);
    for (w=0; w < BLOCK_SWITCH_WINDOWS; w++) {

        MOVE(2);
        accuUE = 0.0;
        accuFE = 0.0;

        PTR_INIT(1); /* pointer for timeSignal[] */
        LOOP(1);
        for(i=0;i<windowLen;i++) {

          MOVE(1);
          tempUnfiltered = timeSignal[(windowLen * w + i) * chIncrement] ;

          INDIRECT(1); FUNC(3);
          tempFiltered   = IIRFilter(tempUnfiltered,hiPassCoeff,blockSwitchingControl->iirStates);
          
          MAC(2);
          accuUE += (tempUnfiltered * tempUnfiltered);
          accuFE += (tempFiltered   * tempFiltered);
        }

        MOVE(2);
        blockSwitchingControl->windowNrg[1][w] = accuUE;
        blockSwitchingControl->windowNrgF[1][w] =accuFE;
  }

  COUNT_sub_end();

  return(TRUE);
}


static float IIRFilter(const float in, const float coeff[], float states[])
{
  float accu1, accu2;
  float out;

  COUNT_sub_start("IIRFilter");

  MULT(2);
  accu1 = coeff[1]*in;
  accu2 = coeff[1]*states[0];

  ADD(1);
  accu1 = accu1 - accu2;

  MULT(1);
  accu2 = coeff[0]*states[1];

  ADD(1);
  accu1 = accu1 - accu2;

  MOVE(3);
  out = accu1;
  states[0] = in;
  states[1] = out;

  COUNT_sub_end();

  return out;
}


static const int synchronizedBlockTypeTable[4][4] =
{
  /*                 LONG_WINDOW   START_WINDOW   SHORT_WINDOW   STOP_WINDOW */
  /* LONG_WINDOW  */{LONG_WINDOW,  START_WINDOW,  SHORT_WINDOW,  STOP_WINDOW},
  /* START_WINDOW */{START_WINDOW, START_WINDOW,  SHORT_WINDOW,  SHORT_WINDOW},
  /* SHORT_WINDOW */{SHORT_WINDOW, SHORT_WINDOW,  SHORT_WINDOW,  SHORT_WINDOW},
  /* STOP_WINDOW  */{STOP_WINDOW,  SHORT_WINDOW,  SHORT_WINDOW,  STOP_WINDOW}
};


int SyncBlockSwitching(BLOCK_SWITCHING_CONTROL *blockSwitchingControlLeft,
                       BLOCK_SWITCHING_CONTROL *blockSwitchingControlRight,
                       const int nChannels)
{
  int i;
  int patchType = LONG_WINDOW;
  
  COUNT_sub_start("SyncBlockSwitching");
  
  MOVE(1); /* counting previous operations */
  
  ADD(1); BRANCH(1);
  if( nChannels == 1) { 
    /* Mono */
    INDIRECT(1); ADD(1); BRANCH(1);
    if (blockSwitchingControlLeft->windowSequence!=SHORT_WINDOW){
      
      INDIRECT(2); MOVE(2);
      blockSwitchingControlLeft->noOfGroups = 1;
      blockSwitchingControlLeft->groupLen[0] = 1;
      
      PTR_INIT(1); /* blockSwitchingControlLeft->groupLen[] */
      LOOP(1);
      for (i = 1; i < TRANS_FAC; i++) {
        MOVE(1);
        blockSwitchingControlLeft->groupLen[i] = 0;
      }
    }
  }
  else {
    /* Stereo */
          
    INDIRECT(2); MOVE(2);
    patchType = synchronizedBlockTypeTable[patchType][blockSwitchingControlLeft->windowSequence];
    patchType = synchronizedBlockTypeTable[patchType][blockSwitchingControlRight->windowSequence];
    
    INDIRECT(2); MOVE(2);
    blockSwitchingControlLeft->windowSequence = patchType;
    blockSwitchingControlRight->windowSequence = patchType;
    
    ADD(1); BRANCH(1);
    if(patchType != SHORT_WINDOW) {     /* Long Blocks */

      INDIRECT(4); MOVE(4);
      blockSwitchingControlLeft->noOfGroups   = 1;
      blockSwitchingControlRight->noOfGroups  = 1;
      blockSwitchingControlLeft->groupLen[0]  = 1;
      blockSwitchingControlRight->groupLen[0] = 1;
      
      PTR_INIT(2); /* blockSwitchingControlLeft->groupLen[]
                      blockSwitchingControlRight->groupLen[]
                   */
      LOOP(1);
      for (i = 1; i < TRANS_FAC; i++) {
        MOVE(2);
        blockSwitchingControlLeft->groupLen[i] = 0;
        blockSwitchingControlRight->groupLen[i] = 0;
      }
    }
    else  {     /* Short Blocks */

      INDIRECT(2); ADD(1); BRANCH(1);
      if(blockSwitchingControlLeft->maxWindowNrg > blockSwitchingControlRight->maxWindowNrg) {  
        INDIRECT(2); MOVE(1);
        blockSwitchingControlRight->noOfGroups = blockSwitchingControlLeft->noOfGroups;
        
        PTR_INIT(2); /* blockSwitchingControlRight->groupLen[]
                        blockSwitchingControlLeft->groupLen[]
                     */
        LOOP(1);
        for (i = 0; i < TRANS_FAC; i++) {
          MOVE(1);
          blockSwitchingControlRight->groupLen[i] = blockSwitchingControlLeft->groupLen[i];
        }
      }
      else  {
        INDIRECT(2); MOVE(1);
        blockSwitchingControlLeft->noOfGroups = blockSwitchingControlRight->noOfGroups;
        
        PTR_INIT(2); /* blockSwitchingControlRight->groupLen[]
                        blockSwitchingControlLeft->groupLen[]
                     */
        LOOP(1);
        for (i = 0; i < TRANS_FAC; i++) {
          MOVE(1);
          blockSwitchingControlLeft->groupLen[i] = blockSwitchingControlRight->groupLen[i];
        }
      }

    }

  } /*endif Mono or Stereo */
  
  COUNT_sub_end();
  
  return TRUE;
}
