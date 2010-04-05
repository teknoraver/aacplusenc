/*
  Core Coder's and SBR's shared data structure definition
*/
#ifndef __SBR_CMONDATA_H
#define __SBR_CMONDATA_H

#include "FFR_bitbuffer.h"


struct COMMON_DATA {
  int                   sbrHdrBits;
  int                   sbrCrcLen;
  int                   sbrDataBits;
  int                   sbrFillBits;
  struct BIT_BUF        sbrBitbuf;
  struct BIT_BUF        tmpWriteBitbuf;
  int                   sbrNumChannels;
  struct BIT_BUF        sbrBitbufPrev;

};

typedef struct COMMON_DATA *HANDLE_COMMON_DATA;



#endif
