/*
  Static bit counter
*/
#ifndef __STAT_BITS_H
#define __STAT_BITS_H

#include "psy_const.h"
#include "interface.h"

int countStaticBitdemand(PSY_OUT_CHANNEL psyOutChannel[MAX_CHANNELS],
                         PSY_OUT_ELEMENT *psyOutElement,
                         int nChannels);


#endif /* __STAT_BITS_H */

