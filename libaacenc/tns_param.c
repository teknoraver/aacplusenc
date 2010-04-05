/*
  Temporal Noise Shaping parameters
*/
#include <stdio.h>
#include "aac_rom.h"

#include "counters.h" /* the 3GPP instrumenting tool */


/*****************************************************************************

    functionname: GetTnsParam
    description:  Get threshold calculation parameter set that best matches
                  the bit rate
    returns:      the parameter set
    input:        blockType, bitRate
    output:       the parameter set

*****************************************************************************/
int GetTnsParam(TNS_CONFIG_TABULATED *tnsConfigTab, int bitRate, int channels, int blockType) {

  unsigned int i;

  COUNT_sub_start("GetTnsParam");

  PTR_INIT(1); /* tnsInfoTab[] */
  
  BRANCH(1); LOGIC(1);
  if (tnsConfigTab == NULL)
    return 1;

  MOVE(1);
  tnsConfigTab->threshOn = -1.0f;

  LOOP(1);
  for(i = 0; i < sizeof(tnsInfoTab)/sizeof(TNS_INFO_TAB); i++) {

    ADD(2); LOGIC(1); BRANCH(1);
    if((bitRate >= tnsInfoTab[i].bitRateFrom) && bitRate <= tnsInfoTab[i].bitRateTo) {

      BRANCH(2);
      switch(blockType) {

      case LONG_WINDOW :
        BRANCH(2);
        switch(channels) {
        case 1 :
          MOVE(1);
          *tnsConfigTab=*tnsInfoTab[i].paramMono_Long;
          break;
        case 2 :
          MOVE(1);
          *tnsConfigTab=*tnsInfoTab[i].paramStereo_Long;
          break;
        }
      break;

      case SHORT_WINDOW :
        BRANCH(2);
        switch(channels) {
        case 1 :
          BRANCH(1);
          MOVE(1);
          *tnsConfigTab=*tnsInfoTab[i].paramMono_Short;
          break;
        case 2 :
          MOVE(1);
          *tnsConfigTab=*tnsInfoTab[i].paramStereo_Short;
          break;
        }

        break;
      }
    }
  }

  BRANCH(1); LOGIC(1);
  if (tnsConfigTab->threshOn == -1.0f) {
    return 1;
  }

  COUNT_sub_end();
  return 0;
}

/*****************************************************************************

    functionname: GetTnsMaxBands
    description:  sets tnsMaxSfbLong, tnsMaxSfbShort according to sampling rate
    returns:
    input:        samplingRate, profile, granuleLen
    output:       tnsMaxSfbLong, tnsMaxSfbShort

*****************************************************************************/
void GetTnsMaxBands(int samplingRate, int blockType, int* tnsMaxSfb){
  unsigned int i;

  COUNT_sub_start("GetTnsMaxBands");

  MULT(1); STORE(1);
  *tnsMaxSfb=-1;

  PTR_INIT(1); /* tnsMaxBandsTab[] */
  LOOP(1);
  for(i=0;i<sizeof(tnsMaxBandsTab)/sizeof(TNS_MAX_TAB_ENTRY);i++){

    ADD(1); BRANCH(1);
    if(samplingRate == tnsMaxBandsTab[i].samplingRate){

        ADD(1); BRANCH(1); MOVE(1);
        *tnsMaxSfb=(blockType==2)?tnsMaxBandsTab[i].maxBandShort:
                                  tnsMaxBandsTab[i].maxBandLong;
        break;
    }
  }

  COUNT_sub_end();
}
