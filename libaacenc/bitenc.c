/*
  Bitstream encoder
*/
#include <stdio.h>
#include <stdlib.h>

#include "bitenc.h"
#include "bit_cnt.h"
#include "dyn_bits.h"
#include "qc_data.h"
#include "interface.h"
#include "minmax.h"

#include "counters.h" /* the 3GPP instrumenting tool */

#define globalGainOffset 100
#define icsReservedBit     0


/*****************************************************************************

    functionname: encodeSpectralData
    description:  encode spectral data
    returns:      none
    input:
    output:

*****************************************************************************/
static int encodeSpectralData(int                 *sfbOffset,
                              SECTION_DATA        *sectionData,
                              short               *quantSpectrum,
                              HANDLE_BIT_BUF      hBitStream)
{
  int i,sfb;
  int dbgVal;

  COUNT_sub_start("encodeSpectralData");

  INDIRECT(1); MOVE(1); /* GetBitsAvail() */
  dbgVal = GetBitsAvail(hBitStream);

  PTR_INIT(1); /* sectionData->section[] */
  INDIRECT(1); LOOP(1);
  for(i=0;i<sectionData->noOfSections;i++)
  {
  /*
   huffencode spectral data for this section
   */
    PTR_INIT(1); /* sfbOffset[] */
    LOOP(1);
    for(sfb=sectionData->section[i].sfbStart;
        sfb<sectionData->section[i].sfbStart+sectionData->section[i].sfbCnt;
        sfb++)
    {

      ADD(1); PTR_INIT(1); FUNC(4);
      codeValues(quantSpectrum+sfbOffset[sfb],
                 sfbOffset[sfb+1]-sfbOffset[sfb],
                 sectionData->section[i].codeBook,
                 hBitStream);

    }
  }

  INDIRECT(1); ADD(1); /* counting post-operation */

  COUNT_sub_end();

  return(GetBitsAvail(hBitStream)-dbgVal);

}

/*****************************************************************************

    functionname:encodeGlobalGain
    description: encodes Global Gain (common scale factor)
    returns:     none
    input:
    output:

*****************************************************************************/
static void encodeGlobalGain(int globalGain,
                             int logNorm,
                             int scalefac,
                             HANDLE_BIT_BUF hBitStream)
{
  COUNT_sub_start("encodeGlobalGain");

  ADD(3); MULT(1); FUNC(3);
  WriteBits(hBitStream,globalGain - scalefac + globalGainOffset-4*logNorm,8);

  COUNT_sub_end();
}


/*****************************************************************************

    functionname:encodeIcsInfo
    description: encodes Ics Info
    returns:     none
    input:
    output:

*****************************************************************************/

static void encodeIcsInfo(int blockType,
                          int windowShape,
                          int groupingMask,
                          SECTION_DATA *sectionData,
                          HANDLE_BIT_BUF  hBitStream)
{
  COUNT_sub_start("encodeIcsInfo");

  FUNC(3);
  WriteBits(hBitStream,icsReservedBit,1);

  FUNC(3);
  WriteBits(hBitStream,blockType,2);

  FUNC(3);
  WriteBits(hBitStream,windowShape,1);

  BRANCH(2);
  switch(blockType){
  case LONG_WINDOW:
  case START_WINDOW:
  case STOP_WINDOW:
    INDIRECT(1); FUNC(3);
    WriteBits(hBitStream,sectionData->maxSfbPerGroup,6);

    /* No predictor data present */
    FUNC(3);
    WriteBits(hBitStream, 0, 1);
    break;

  case SHORT_WINDOW:
    INDIRECT(1); FUNC(3);
    WriteBits(hBitStream,sectionData->maxSfbPerGroup,4);

    /*
     Write grouping bits
    */
    FUNC(3);
    WriteBits(hBitStream,groupingMask,TRANS_FAC-1);
    break;
  }

  COUNT_sub_end();
}

/*****************************************************************************

    functionname: encodeSectionData
    description:  encode section data (common Huffman codebooks for adjacent
                  SFB's)
    returns:      none
    input:
    output:

*****************************************************************************/
static int encodeSectionData(SECTION_DATA *sectionData,
                             HANDLE_BIT_BUF hBitStream)
{
  int sectEscapeVal=0,sectLenBits=0;
  int sectLen;
  int i;
  int dbgVal=GetBitsAvail(hBitStream);

  COUNT_sub_start("encodeSectionData");

  INDIRECT(1); MOVE(1); /* GetBitsAvail() */ MOVE(2); /* counting previous operations */

  INDIRECT(1); BRANCH(2);
  switch(sectionData->blockType)
  {
  case LONG_WINDOW:
  case START_WINDOW:
  case STOP_WINDOW:

    MOVE(2);
    sectEscapeVal = SECT_ESC_VAL_LONG;
    sectLenBits   = SECT_BITS_LONG;
    break;

  case SHORT_WINDOW:

    MOVE(2);
    sectEscapeVal = SECT_ESC_VAL_SHORT;
    sectLenBits   = SECT_BITS_SHORT;
    break;
  }

  PTR_INIT(1); /* sectionData->section[] */
  INDIRECT(1); LOOP(1);
  for(i=0;i<sectionData->noOfSections;i++)
  {
    FUNC(3);
    WriteBits(hBitStream,sectionData->section[i].codeBook,4);

    MOVE(1);
    sectLen = sectionData->section[i].sfbCnt;

    while(sectLen >= sectEscapeVal)
    {
      ADD(1); /* while() condition */

      FUNC(3);
      WriteBits(hBitStream,sectEscapeVal,sectLenBits);

      ADD(1);
      sectLen-=sectEscapeVal;
    }

    FUNC(3);
    WriteBits(hBitStream,sectLen,sectLenBits);
  }

  INDIRECT(1); ADD(1); /* counting post-operation */

  COUNT_sub_end();

  return(GetBitsAvail(hBitStream)-dbgVal);
}

/*****************************************************************************

    functionname: encodeScaleFactorData
    description:  encode DPCM coded scale factors
    returns:      none
    input:
    output:

*****************************************************************************/
static int encodeScaleFactorData(unsigned short        *maxValueInSfb,
                                 SECTION_DATA        *sectionData,
                                 short                 *scalefac,
                                 HANDLE_BIT_BUF      hBitStream)
{
  int i,j,lastValScf,deltaScf;
  int dbgVal = GetBitsAvail(hBitStream);

  COUNT_sub_start("encodeScaleFactorData");

  INDIRECT(1); MOVE(1); /* counting previous operation */

  INDIRECT(1); MOVE(1);
  lastValScf=scalefac[sectionData->firstScf];

  PTR_INIT(1); /* sectionData->section[] */
  INDIRECT(1); LOOP(1);
  for(i=0;i<sectionData->noOfSections;i++){

    ADD(1); BRANCH(1);
    if(sectionData->section[i].codeBook != CODE_BOOK_ZERO_NO){

      PTR_INIT(2); /* maxValueInSfb[]
                      scalefac[]
                   */
      LOOP(1);
      for(j=sectionData->section[i].sfbStart;j<sectionData->section[i].sfbStart+sectionData->section[i].sfbCnt;j++){

        /*
          check if we can repeat the last value to save bits
        */
        BRANCH(1);
        if(maxValueInSfb[j] == 0)
        {
          MOVE(1);
          deltaScf = 0;
        }
        else{
          ADD(1);
          deltaScf = -(scalefac[j]-lastValScf);

          MOVE(1);
          lastValScf = scalefac[j];
        }
        FUNC(2); BRANCH(1);
        if(codeScalefactorDelta(deltaScf,hBitStream)){
#ifdef DEBUG
          fprintf(stderr,"\nLAV for scf violated\n");
#endif

          COUNT_sub_end();
          return(1);
        }

      }
    }

  }

  INDIRECT(1); ADD(1); /* counting post-operation */

  COUNT_sub_end();

  return(GetBitsAvail(hBitStream)-dbgVal);
}

/*****************************************************************************

    functionname:encodeMsInfo
    description: encodes MS-Stereo Info
    returns:     none
    input:
    output:

*****************************************************************************/
static void encodeMSInfo(int            sfbCnt,
                         int            grpSfb,
                         int            maxSfb,
                         int            msDigest,
                         int           *jsFlags,
                         HANDLE_BIT_BUF hBitStream)
{
  int sfb, sfbOff;

  COUNT_sub_start("encodeMSInfo");

  BRANCH(2);
  switch(msDigest)
  {
  case MS_NONE:
    FUNC(3);
    WriteBits(hBitStream,SI_MS_MASK_NONE,2);
    break;

  case MS_ALL:
    FUNC(3);
    WriteBits(hBitStream,SI_MS_MASK_ALL,2);
    break;

  case MS_SOME:
    FUNC(3);
    WriteBits(hBitStream,SI_MS_MASK_SOME,2);

    LOOP(1);
    for(sfbOff = 0; sfbOff < sfbCnt; sfbOff+=grpSfb)
    {
      LOOP(1);
      for(sfb=0; sfb<maxSfb; sfb++)
      {
        LOGIC(1); BRANCH(1);
        if(jsFlags[sfbOff+sfb] & MS_ON){

          FUNC(3);
          WriteBits(hBitStream,1,1);

        }
        else{

          FUNC(3);
          WriteBits(hBitStream,0,1);

        }
      }
    }

    break;

  }

  COUNT_sub_end();
}

/*****************************************************************************

    functionname: encodeTnsData
    description:  encode TNS data (filter order, coeffs, ..)
    returns:      none
    input:
    output:

*****************************************************************************/
static void encodeTnsData(TNS_INFO tnsInfo,
                          int blockType,
                          HANDLE_BIT_BUF hBitStream) 
{
  int i,k;
  int tnsPresent;
  int numOfWindows;
  int coefBits;

  COUNT_sub_start("encodeTnsData");

  ADD(1); BRANCH(1); MOVE(1);
  numOfWindows=(blockType==2?TRANS_FAC:1);

  MOVE(1);
  tnsPresent=0;

  PTR_INIT(1); /* tnsInfo.numOfFilters[] */
  LOOP(1);
  for (i=0; i<numOfWindows; i++) {
    
    ADD(1); BRANCH(1);
    if (tnsInfo.tnsActive[i]==1) {

      MOVE(1);
      tnsPresent=1;
    }
  }

  BRANCH(1);
  if (tnsPresent==0) {

    FUNC(3);
    WriteBits(hBitStream,0,1);
  }
  else { /* there is data to be written*/
    
    FUNC(3);
    WriteBits(hBitStream,1,1); /*data_present */

    PTR_INIT(2); /* tnsInfo.tnsActive[]
                    tnsInfo.coefRes[]
                 */
    LOOP(1);
    for (i=0; i<numOfWindows; i++) {

      ADD(1); BRANCH(1); /* .. == .. ? */ FUNC(3);
      WriteBits(hBitStream,tnsInfo.tnsActive[i],(blockType==2?1:2));
      
      BRANCH(1);
      if (tnsInfo.tnsActive[i]) {
        
        PTR_INIT(2); /* tnsInfo.length[]
                        tnsInfo.order[] */

        ADD(1); BRANCH(1); /* .. == .. ? */ FUNC(3);
        WriteBits(hBitStream,(tnsInfo.coefRes[i]==4?1:0),1);
        
        ADD(1); BRANCH(1); /* .. == .. ? */ FUNC(3);
        WriteBits(hBitStream,tnsInfo.length[i],(blockType==2?4:6));
        
        ADD(1); BRANCH(1); /* .. == .. ? */ FUNC(3);
        WriteBits(hBitStream,tnsInfo.order[i],(blockType==2?3:5));
        
        BRANCH(1);
        if (tnsInfo.order[i]){
          
          FUNC(3);
          WriteBits(hBitStream,FILTER_DIRECTION,1);
          
          ADD(1); BRANCH(1);
          if(tnsInfo.coefRes[i] == 4) {
            
            MOVE(1);
            coefBits = 3;
            
            PTR_INIT(1); /* tnsInfo.coef[][] */
            LOOP(1);
            for(k=0; k<tnsInfo.order[i]; k++) {
              
              ADD(2); LOGIC(1); BRANCH(1);
              if (tnsInfo.coef[i*TNS_MAX_ORDER_SHORT+k]> 3 ||
                  tnsInfo.coef[i*TNS_MAX_ORDER_SHORT+k]< -4) {
                
                MOVE(1);
                coefBits = 4;
                break;
              }
            }
          }
          else {
            
            MOVE(1);
            coefBits = 2;
            
            PTR_INIT(1); /* tnsInfo.coef[][] */
            LOOP(1);               
            for(k=0; k<tnsInfo.order[i]; k++) {
              
              ADD(2); LOGIC(1); BRANCH(1);
              if (tnsInfo.coef[i*TNS_MAX_ORDER_SHORT+k]> 1 ||
                  tnsInfo.coef[i*TNS_MAX_ORDER_SHORT+k]< -2) {
                
                MOVE(1);
                coefBits = 3;
                break;
              }
            }
          }
          
          ADD(1); FUNC(3);
          WriteBits(hBitStream,-(coefBits - tnsInfo.coefRes[i]),1); /*coef_compres*/
          
          PTR_INIT(1); /* tnsInfo.coef[] */
          INDIRECT(1); /* rmask[] */ LOOP(1);
          for (k=0; k<tnsInfo.order[i]; k++ ) {
            static const int rmask[] = {0,1,3,7,15};
            
            LOGIC(1); FUNC(3);
            WriteBits(hBitStream,tnsInfo.coef[i*TNS_MAX_ORDER_SHORT+k] & rmask[coefBits],coefBits);
          }
        }
      }
    }
  }
  
  COUNT_sub_end();
}
/*****************************************************************************

    functionname: encodeGainControlData
    description:  unsupported
    returns:      none
    input:
    output:

*****************************************************************************/
static void encodeGainControlData(HANDLE_BIT_BUF hBitStream)
{
  COUNT_sub_start("encodeGainControlData");

  FUNC(3);
  WriteBits(hBitStream,0,1);

  COUNT_sub_end();
}

/*****************************************************************************

    functionname: encodePulseData
    description:  not supported yet (dummy)
    returns:      none
    input:
    output:

*****************************************************************************/
static void encodePulseData(HANDLE_BIT_BUF hBitStream)
{
  COUNT_sub_start("encodePulseData");

  FUNC(3);
  WriteBits(hBitStream,0,1);

  COUNT_sub_end();
}



/*****************************************************************************

    functionname: WriteIndividualChannelStream
    description:  management of write process of individual channel stream
    returns:      none
    input:
    output:

*****************************************************************************/
static void
writeIndividualChannelStream(int commonWindow,
                             int windowShape,
                             int groupingMask,
                             int *sfbOffset,
                             short scf[],
                             unsigned short *maxValueInSfb,
                             int globalGain,
                             short quantSpec[],
                             SECTION_DATA *sectionData,
                             HANDLE_BIT_BUF hBitStream,
                             TNS_INFO tnsInfo)
{
  int logNorm = -1;

  COUNT_sub_start("writeIndividualChannelStream");

  MOVE(1); /* counting previous operation */

  INDIRECT(1); FUNC(4);
  encodeGlobalGain(globalGain, logNorm,scf[sectionData->firstScf], hBitStream);

  BRANCH(1);
  if(!commonWindow)
  {
    INDIRECT(1); FUNC(5);
    encodeIcsInfo(sectionData->blockType, windowShape, groupingMask, sectionData, hBitStream);
  }

  INDIRECT(1); FUNC(2); ADD(1); BRANCH(1);
  if(encodeSectionData(sectionData, hBitStream) != sectionData->sideInfoBits)
  {
#ifdef DEBUG
    fprintf(stderr,"ERROR writing sectionData\n");
#endif
  }

  INDIRECT(1); FUNC(4); ADD(1); BRANCH(1);
  if(encodeScaleFactorData( maxValueInSfb,
                            sectionData,
                            scf,
                            hBitStream) != sectionData->scalefacBits)
  {
#ifdef DEBUG
    fprintf(stderr,"ERROR writing scalefacData\n");
#endif
  }



  FUNC(1);
  encodePulseData(hBitStream);

  INDIRECT(1); FUNC(3);
  encodeTnsData(tnsInfo, sectionData->blockType, hBitStream);

  FUNC(1);
  encodeGainControlData(hBitStream);

  INDIRECT(1); FUNC(4); ADD(1); BRANCH(1);
  if(encodeSpectralData(sfbOffset,
                     sectionData,
                     quantSpec,
                     hBitStream) != sectionData->huffmanBits)
  {
#ifdef DEBUG
   fprintf(stderr,"ERROR writing spectralData\n");
#endif
  }


  COUNT_sub_end();
}

/*****************************************************************************

    functionname: writeSingleChannelElement
    description:  write single channel element to bitstream
    returns:      none
    input:
    output:

*****************************************************************************/
static int writeSingleChannelElement(int instanceTag,
                                     int *sfbOffset,
                                     QC_OUT_CHANNEL* qcOutChannel,
                                     HANDLE_BIT_BUF hBitStream,
                                     TNS_INFO tnsInfo)
{
  COUNT_sub_start("writeSingleChannelElement");

  FUNC(3);
  WriteBits(hBitStream,ID_SCE,3);

  FUNC(3);
  WriteBits(hBitStream,instanceTag,4);

  INDIRECT(7); PTR_INIT(1); FUNC(11);
  writeIndividualChannelStream(0,
                               qcOutChannel->windowShape,
                               qcOutChannel->groupingMask,
                               sfbOffset,
                               qcOutChannel->scf,
                               qcOutChannel->maxValueInSfb,
                               qcOutChannel->globalGain,
                               qcOutChannel->quantSpec,
                               &(qcOutChannel->sectionData),
                               hBitStream,
                               tnsInfo
                               );

  COUNT_sub_end();
  return 0;
}



/*****************************************************************************

    functionname: writeChannelPairElement
    description:
    returns:      none
    input:
    output:

*****************************************************************************/
static int writeChannelPairElement(int instanceTag,
                                   int msDigest,
                                   int msFlags[MAX_GROUPED_SFB],
                                   int *sfbOffset[2],
                                   QC_OUT_CHANNEL qcOutChannel[2],
                                   HANDLE_BIT_BUF hBitStream,
                                   TNS_INFO tnsInfo[2]
                                   )
{
  COUNT_sub_start("writeChannelPairElement");

  FUNC(3);
  WriteBits(hBitStream,ID_CPE,3);

  FUNC(3);
  WriteBits(hBitStream,instanceTag,4);

  FUNC(3);
  WriteBits(hBitStream,1,1); /* common window */

  PTR_INIT(1); FUNC(5);
  encodeIcsInfo(qcOutChannel[0].sectionData.blockType,
                qcOutChannel[0].windowShape,
                qcOutChannel[0].groupingMask,
                &(qcOutChannel[0].sectionData),
                hBitStream);
  
  FUNC(6);
  encodeMSInfo(qcOutChannel[0].sectionData.sfbCnt,
               qcOutChannel[0].sectionData.sfbPerGroup,
               qcOutChannel[0].sectionData.maxSfbPerGroup,
               msDigest,
               msFlags,
               hBitStream);

  PTR_INIT(1); FUNC(11);
  writeIndividualChannelStream(1,
                               qcOutChannel[0].windowShape,
                               qcOutChannel[0].groupingMask,
                               sfbOffset[0],
                               qcOutChannel[0].scf,
                               qcOutChannel[0].maxValueInSfb,
                               qcOutChannel[0].globalGain,
                               qcOutChannel[0].quantSpec,
                               &(qcOutChannel[0].sectionData),
                               hBitStream,
                               tnsInfo[0]);
  
  PTR_INIT(1); FUNC(11);
  writeIndividualChannelStream(1,
                               qcOutChannel[1].windowShape,
                               qcOutChannel[1].groupingMask,
                               sfbOffset[1],
                               qcOutChannel[1].scf,
                               qcOutChannel[1].maxValueInSfb,
                               qcOutChannel[1].globalGain,
                               qcOutChannel[1].quantSpec,
                               &(qcOutChannel[1].sectionData),
                               hBitStream,
                               tnsInfo[1]);

  COUNT_sub_end();

  return 0;
}



/*****************************************************************************

    functionname: writeFillElement
    description:  write fill elements to bitstream
    returns:      none
    input:
    output:

*****************************************************************************/
static void writeFillElement( const unsigned char *ancBytes,
                             int totFillBits,
                             HANDLE_BIT_BUF hBitStream)
{
  int i, cnt, esc_count;
  
  COUNT_sub_start("writeFillElement");
  
  /*
    Write fill Element(s):
    amount of a fill element can be 7+X*8 Bits, X element of [0..270]
  */
  
  LOOP(1);
  while(totFillBits>=(3+4)) {

    ADD(2); MULT(1); BRANCH(1); MOVE(1);
    cnt = min((totFillBits-(3+4))/8, ((1<<4)-1));
    
    FUNC(3);
    WriteBits(hBitStream,ID_FIL,3);
    
    FUNC(3);
    WriteBits(hBitStream,cnt,4);
    
    ADD(1);
    totFillBits-=(3+4);
    
    ADD(1); BRANCH(1);
    if(cnt == (1<<4)-1){
      
      MULT(1); ADD(2); BRANCH(1); MOVE(1);
      esc_count = min( (totFillBits/8)- ((1<<4)-1), (1<<8)-1);
      
      FUNC(3);
      WriteBits(hBitStream,esc_count,8);
      
      ADD(1);
      totFillBits-=8;
      
      ADD(2);
      cnt+=esc_count-1;
    }
    
    LOOP(1);
    for(i=0;i<cnt;i++){
      
      BRANCH(1); FUNC(3);
      if(ancBytes)
        WriteBits(hBitStream,*ancBytes++,8);
      else
        WriteBits(hBitStream,0,8);
      
      ADD(1);
      totFillBits-=8;
    }
    
  }
  COUNT_sub_end();
}


/*****************************************************************************

    functionname: WriteBitStream
    description:  main function of write process
    returns:
    input:
    output:

*****************************************************************************/
int WriteBitstream (HANDLE_BIT_BUF hBitStream,
                    ELEMENT_INFO elInfo,
                    QC_OUT *qcOut,
                    PSY_OUT* psyOut,
                    int* globUsedBits,
                    const unsigned char *ancBytes
                    ) /* returns error code */
{
  int bitMarkUp, elementUsedBits, frameBits;

  COUNT_sub_start("WriteBitstream");

  INDIRECT(1); MOVE(1); /* GetBitsAvail() */
  bitMarkUp = GetBitsAvail(hBitStream);

  MOVE(1);
  *globUsedBits = 0;

  {
    int* sfbOffset[2];
    TNS_INFO tnsInfo[2];
    
    MOVE(1);
    elementUsedBits =0;

    INDIRECT(1); BRANCH(2);
    switch (elInfo.elType) {

    case ID_SCE:      /* single channel */
      
      INDIRECT(1); MOVE(2);
      sfbOffset[0] =
        psyOut->psyOutChannel[elInfo.ChannelIndex[0]].sfbOffsets;
      tnsInfo[0]=psyOut->psyOutChannel[elInfo.ChannelIndex[0]].tnsInfo;
      
      PTR_INIT(1); FUNC(5);
      writeSingleChannelElement(  elInfo.instanceTag,
                                  sfbOffset[0],
                                  &qcOut->qcChannel[elInfo.ChannelIndex[0]],
                                  hBitStream,
                                  tnsInfo[0] );
      break;
      
    case ID_CPE:     /* channel pair */
      {
        int msDigest = psyOut->psyOutElement.toolsInfo.msDigest;
        int* msFlags = psyOut->psyOutElement.toolsInfo.msMask;
        
        MOVE(2); PTR_INIT(1); /* counting previous operation */
        
        INDIRECT(2); MOVE(2);
        sfbOffset[0] = psyOut->psyOutChannel[elInfo.ChannelIndex[0]].sfbOffsets;
        sfbOffset[1] = psyOut->psyOutChannel[elInfo.ChannelIndex[1]].sfbOffsets;
        
        MOVE(2);
        tnsInfo[0] = psyOut->psyOutChannel[elInfo.ChannelIndex[0]].tnsInfo;
        tnsInfo[1] = psyOut->psyOutChannel[elInfo.ChannelIndex[1]].tnsInfo;
        
        INDIRECT(2); PTR_INIT(1); FUNC(7);
        writeChannelPairElement(elInfo.instanceTag,
                                msDigest,
                                msFlags,
                                sfbOffset,
                                &qcOut->qcChannel[elInfo.ChannelIndex[0]],
                                hBitStream,
                                tnsInfo);
      }
      break;
      
    default:
      COUNT_sub_end();
      return 1;

    }   /* switch */

    ADD(1); STORE(1);
    elementUsedBits -= bitMarkUp;

    INDIRECT(1); MOVE(1); /* GetBitsAvail() */
    bitMarkUp = GetBitsAvail(hBitStream);

    ADD(1); STORE(1);
    frameBits = elementUsedBits + bitMarkUp;
    
  }

  /*
    ancillary data
  */
  INDIRECT(1); FUNC(3);
  writeFillElement(ancBytes,
                   qcOut->totAncBitsUsed, 
                   hBitStream);
  

  INDIRECT(1); FUNC(3);
  writeFillElement(NULL,
                   qcOut->totFillBits, 
                   hBitStream);

  FUNC(3);
  WriteBits(hBitStream,ID_END,3);

  /* byte alignement */

  INDIRECT(1); ADD(1); LOGIC(2); FUNC(3);
  WriteBits(hBitStream,0,(8-(hBitStream->cntBits % 8)) % 8);
  
  ADD(1); STORE(1);
  *globUsedBits -= bitMarkUp;

  INDIRECT(1); MOVE(1); /* GetBitsAvail() */
  bitMarkUp = GetBitsAvail(hBitStream);

  ADD(1); STORE(1);
  *globUsedBits+=bitMarkUp;

  ADD(1);
  frameBits+= *globUsedBits;

  INDIRECT(5); ADD(5); BRANCH(1);
  if(frameBits != qcOut->totStaticBitsUsed+qcOut->totDynBitsUsed + qcOut->totAncBitsUsed + 
     + qcOut->totFillBits+qcOut->alignBits){
#ifdef DEBUG
    fprintf(stderr,"\nNo of written frame bits != expected Bits !!!\n");
#endif
    COUNT_sub_end();
    return -1;
  }

  COUNT_sub_end();
  return 0;
}
