/*
  memory requirement in dynamic and static RAM
*/

#ifndef AAC_RAM_H
#define AAC_RAM_H

#include "psy_const.h"

extern float mdctDelayBuffer[MAX_CHANNELS*BLOCK_SWITCHING_OFFSET];

extern int sideInfoTabLong[MAX_SFB_LONG + 1];
extern int sideInfoTabShort[MAX_SFB_SHORT + 1];

extern short *quantSpec;
extern float *expSpec;
extern short *quantSpecTmp;

extern short *scf;
extern unsigned short *maxValueInSfb;

/* dynamic buffers of SBR that can be reused by the core */
extern float sbr_envRBuffer[];
extern float sbr_envIBuffer[];

#endif
