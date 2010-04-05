/*
  Block Switching
*/
#ifndef _BLOCK_SWITCH_H
#define _BLOCK_SWITCH_H

#define BLOCK_SWITCHING_IIR_LEN 2    
#define BLOCK_SWITCH_WINDOWS    TRANS_FAC               /* number of windows for energy calculation */
#define BLOCK_SWITCH_WINDOW_LEN FRAME_LEN_SHORT         /* minimal granularity of energy calculation */


typedef struct{
  float invAttackRatio;
  int   windowSequence;
  int   nextwindowSequence;
  int   attack;
  int   lastattack;
  int   attackIndex;
  int   lastAttackIndex;
  int   noOfGroups;
  int   groupLen[TRANS_FAC];
  float windowNrg[2][BLOCK_SWITCH_WINDOWS];     /* time signal energy in Subwindows (last and current) */
  float windowNrgF[2][BLOCK_SWITCH_WINDOWS];    /* filtered time signal energy in segments (last and current) */
  float iirStates[BLOCK_SWITCHING_IIR_LEN];     /* filter delay-line */
  float maxWindowNrg;                           /* max energy in subwindows */
  float accWindowNrg;                           /* recursively accumulated windowNrgF */
}BLOCK_SWITCHING_CONTROL;

int   InitBlockSwitching(BLOCK_SWITCHING_CONTROL *blockSwitchingControl,
                         const int bitRate, const int nChannels);

int   BlockSwitching(BLOCK_SWITCHING_CONTROL *blockSwitchingControl,
                     float *timeSignal,
                     int chIncrement);

int   SyncBlockSwitching(BLOCK_SWITCHING_CONTROL *blockSwitchingControlLeft,
                         BLOCK_SWITCHING_CONTROL *blockSwitchingControlRight,
                         const int noOfChannels);

#endif  /* #ifndef _BLOCK_SWITCH_H */
