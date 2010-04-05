/*
  Static bit counter $Revision
*/
#include "stat_bits.h"
#include "bitenc.h"
#include "tns.h"

#include "counters.h" /* the 3GPP instrumenting tool */

typedef enum{
  SI_ID_BITS                =(3),
  SI_FILL_COUNT_BITS        =(4),
  SI_FILL_ESC_COUNT_BITS    =(8),
  SI_FILL_EXTENTION_BITS    =(4),
  SI_FILL_NIBBLE_BITS       =(4),
  SI_SCE_BITS               =(4),
  SI_CPE_BITS               =(5),
  SI_CPE_MS_MASK_BITS       =(2) ,
  SI_ICS_INFO_BITS_LONG     =(1+2+1+6+1),
  SI_ICS_INFO_BITS_SHORT    =(1+2+1+4+7),
  SI_ICS_BITS               =(8+1+1+1),
}SI_BITS;



static int countMsMaskBits(int   sfbCnt,
                           int   sfbPerGroup,
                           int   maxSfbPerGroup,
                           struct TOOLSINFO *toolsInfo)
{

  int msBits=0,sfbOff,sfb;

  COUNT_sub_start("countMsMaskBits");

  MOVE(1); /* counting previous operation */

  INDIRECT(1); BRANCH(2);
  switch(toolsInfo->msDigest)
  {
    case MS_NONE:
    case MS_ALL:
    break;

    case MS_SOME:

      LOOP(1);
      for(sfbOff = 0; sfbOff < sfbCnt; sfbOff+=sfbPerGroup)
      {
        LOOP(1);
        for(sfb=0; sfb<maxSfbPerGroup; sfb++)
        {
        ADD(1);
        msBits++;
        }
      }
    break;
  }

  COUNT_sub_end();

  return(msBits);

}


static int tnsCount(TNS_INFO *tnsInfo, int blockType)
{

  int i,k;
  int tnsPresent;
  int numOfWindows;
  int count;
  int coefBits;

  COUNT_sub_start("tnsCount");

  MOVE(1);
  count = 0;

  ADD(1); BRANCH(1); MOVE(1);
  numOfWindows=(blockType==2?8:1);

  MOVE(1);
  tnsPresent=0;

  PTR_INIT(1); /* tnsInfo->numOfFilters[] */
  LOOP(1);
  for (i=0; i<numOfWindows; i++) {

    ADD(1); BRANCH(1);
    if (tnsInfo->tnsActive[i]==1) {

      MOVE(1);
      tnsPresent=1;
    }
  }

  BRANCH(1);
  if (tnsPresent==0) {
    /* count+=1; */
  }
  else{ /* there is data to be written*/
    /*count += 1; */

    PTR_INIT(2); /* tnsInfo->tnsActive[]
                    tnsInfo->coefRes[]
                 */
    LOOP(1);
    for (i=0; i<numOfWindows; i++) {

      ADD(1); BRANCH(1); /* .. == .. ? */ ADD(1);
      count +=(blockType==SHORT_WINDOW?1:2);

      BRANCH(1);
      if (tnsInfo->tnsActive[i]) {

        ADD(1);
        count += 1;

        ADD(1); BRANCH(1); /* .. == .. ? */ ADD(2);
        count +=(blockType==SHORT_WINDOW?4:6);
        count +=(blockType==SHORT_WINDOW?3:5);
        
        BRANCH(1);
        if (tnsInfo->order[i]){

          ADD(2);
          count +=1; /*direction*/
          count +=1; /*coef_compression */
          
          ADD(1); BRANCH(1);
          if(tnsInfo->coefRes[i] == 4) {

            MOVE(1);
            coefBits=3;
            
            PTR_INIT(1); /* tnsInfo->coef[]*/
            LOOP(1);
            for(k=0; k<tnsInfo->order[i]; k++) {
              
              ADD(2); LOGIC(1); BRANCH(1);
              if (tnsInfo->coef[i*TNS_MAX_ORDER_SHORT+k]> 3 ||
                  tnsInfo->coef[i*TNS_MAX_ORDER_SHORT+k]< -4) {
                
                MOVE(1);
                coefBits = 4;
                break;
              }
            }
          }
          else {
            
            MOVE(1);
            coefBits = 2;
            
            PTR_INIT(1); /* tnsInfo->coef[]*/
            LOOP(1);
            for(k=0; k<tnsInfo->order[i]; k++) {
              
              ADD(2); LOGIC(1); BRANCH(1);
              if (tnsInfo->coef[i*TNS_MAX_ORDER_SHORT+k]> 1 ||
                  tnsInfo->coef[i*TNS_MAX_ORDER_SHORT+k]< -2) {
                
                MOVE(1);
                coefBits = 3;
                break;
              }
            }
          }
          
          LOOP(1);
          for (k=0; k<tnsInfo->order[i]; k++ ) {
            
            ADD(1);
            count +=coefBits;
          }
        }
      }
    }
  }

  COUNT_sub_end();

  return count;
}


static int countTnsBits(TNS_INFO *tnsInfo,int blockType)
{
  COUNT_sub_start("countTnsBits");
  FUNC(2);
  COUNT_sub_end();

  return(tnsCount(tnsInfo, blockType));
}





int countStaticBitdemand(PSY_OUT_CHANNEL psyOutChannel[MAX_CHANNELS],
                         PSY_OUT_ELEMENT *psyOutElement,
                         int channels)
{

  int statBits=0;
  int ch;

  COUNT_sub_start("countStaticBitdemand");

  MOVE(1); /* counting previous operation */

  BRANCH(2);
  switch(channels) {

  case 1:

    ADD(1);
    statBits+=SI_ID_BITS+SI_SCE_BITS+SI_ICS_BITS;

    PTR_INIT(1); FUNC(2); ADD(1);
    statBits+=countTnsBits(&(psyOutChannel[0].tnsInfo),
                           psyOutChannel[0].windowSequence);

    BRANCH(2);
    switch(psyOutChannel[0].windowSequence){
      case LONG_WINDOW:
      case START_WINDOW:
      case STOP_WINDOW:

        ADD(1);
        statBits+=SI_ICS_INFO_BITS_LONG;
      break;
      case SHORT_WINDOW:

      ADD(1);
      statBits+=SI_ICS_INFO_BITS_SHORT;
      break;
    }
    break;

  case 2:
    ADD(1);
    statBits+=SI_ID_BITS+SI_CPE_BITS+2*SI_ICS_BITS;
    
    ADD(1);
    statBits+=SI_CPE_MS_MASK_BITS;
    
    INDIRECT(1); PTR_INIT(1); FUNC(4); ADD(1);
    statBits+=countMsMaskBits(psyOutChannel[0].sfbCnt,
                              psyOutChannel[0].sfbPerGroup,
                              psyOutChannel[0].maxSfbPerGroup,
                              &psyOutElement->toolsInfo);
    
    PTR_INIT(1); /* psyOutChannel[] */
    
    switch(psyOutChannel[0].windowSequence) {
      
    case LONG_WINDOW:
    case START_WINDOW:
    case STOP_WINDOW:
      
      ADD(1);
      statBits+=SI_ICS_INFO_BITS_LONG;
      break;
      
    case SHORT_WINDOW:
      
      ADD(1);
      statBits+=SI_ICS_INFO_BITS_SHORT;
      break;
    }
    
    PTR_INIT(1); /* psyOutChannel[ch] */
    LOOP(1);
    for(ch=0;ch<2;ch++) {
      
      PTR_INIT(1); FUNC(2); ADD(1);
      statBits+=countTnsBits(&(psyOutChannel[ch].tnsInfo),
                             psyOutChannel[ch].windowSequence);
    }
    
    break;
  }

  COUNT_sub_end();
  
  return statBits;
}

