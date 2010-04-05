/*
  Channel Mapping
*/
#include "channel_map.h"
#include "bitenc.h"
#include "psy_const.h"
#include "qc_data.h"
#include <string.h>

#include "counters.h" /* the 3GPP instrumenting tool */


static int initElement (ELEMENT_INFO* elInfo, ELEMENT_TYPE elType) {

  int error=0;

  COUNT_sub_start("initElement");

  MOVE(1); /* counting previous operations */

  INDIRECT(1); MOVE(1);
  elInfo->elType=elType;

  BRANCH(2);
  switch(elInfo->elType) {

  case ID_SCE:
    INDIRECT(1); MOVE(1);
    elInfo->nChannelsInEl=1;

    INDIRECT(1); MOVE(1);
    elInfo->ChannelIndex[0]=0;

    INDIRECT(2); STORE(1);
    elInfo->instanceTag=0;
    break;

  case ID_CPE:

    INDIRECT(1); MOVE(1);
    elInfo->nChannelsInEl=2;

    INDIRECT(2); MOVE(2);
    elInfo->ChannelIndex[0]=0;
    elInfo->ChannelIndex[1]=1;

    INDIRECT(1); STORE(1);
    elInfo->instanceTag=0;
    break;

  default:
    MOVE(1);
    error=1;
  }

  COUNT_sub_end();

  return error;
}

int InitElementInfo (int nChannels, ELEMENT_INFO* elInfo) {

  int error=0;

  COUNT_sub_start("InitChannelMapping");

  MOVE(1); /* counting previous operation */


  BRANCH(2);
  switch(nChannels) {

  case 1: 
    FUNC(2);
    initElement(elInfo, ID_SCE);
    break;

  case 2:
    FUNC(5);
    initElement(elInfo, ID_CPE);
    break;

  default:
    MOVE(1);
    error=1;

    COUNT_sub_end();
    return error;
  }

  COUNT_sub_end();

  return 0;
}


int InitElementBits(ELEMENT_BITS*   elementBits,
                    ELEMENT_INFO    elInfo,
                    int bitrateTot,
                    int averageBitsTot,
                    int staticBitsTot)
{
  int error=0;

  COUNT_sub_start("InitElementBits");

  MOVE(1); /* counting previous operation */

  INDIRECT(1); BRANCH(2);
  switch(elInfo.nChannelsInEl) {

  case 1:
    MOVE(1); PTR_INIT(1);
    elementBits->chBitrate = bitrateTot;

    ADD(1); STORE(1);
    elementBits->averageBits = (averageBitsTot - staticBitsTot);

    MOVE(1);
    elementBits->maxBits = MAX_CHANNEL_BITS;

    ADD(1); STORE(1);
    elementBits->maxBitResBits = MAX_CHANNEL_BITS - averageBitsTot;

    LOGIC(1); ADD(1); STORE(1);
    elementBits->maxBitResBits -= (elementBits[0].maxBitResBits%8);

    MOVE(2);
    elementBits->bitResLevel = elementBits[0].maxBitResBits;
    elementBits->relativeBits  = 1.0f;
    break;

  case 2:
    MULT(1); STORE(1);
    elementBits->chBitrate   = (int)(bitrateTot*0.5f);

    ADD(1); STORE(1);
    elementBits->averageBits = (averageBitsTot - staticBitsTot);

    MULT(1); STORE(1);
    elementBits->maxBits     = 2*MAX_CHANNEL_BITS;

    MULT(1); ADD(1); STORE(1);
    elementBits->maxBitResBits = 2*MAX_CHANNEL_BITS - averageBitsTot;

    LOGIC(1); ADD(1); STORE(1);
    elementBits->maxBitResBits -= (elementBits[0].maxBitResBits%8);

    MOVE(2);
    elementBits->bitResLevel = elementBits[0].maxBitResBits;
    elementBits->relativeBits = 1.0f;
    break;

  default:
    MOVE(2);
    error=1;
  }

  COUNT_sub_end();

  return error;
}

