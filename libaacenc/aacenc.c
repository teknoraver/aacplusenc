/*
  fast aac coder interface library functions
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "aacenc.h"
#include "bitenc.h"

#include "psy_configuration.h"
#include "psy_main.h"
#include "qc_main.h"
#include "psy_main.h"
#include "channel_map.h"
#include "stprepro.h"
#include "minmax.h"

#include "counters.h" /* the 3GPP instrumenting tool */

struct AAC_ENCODER {

  AACENC_CONFIG config;

  ELEMENT_INFO elInfo;

  QC_STATE qcKernel;
  QC_OUT   qcOut;

  PSY_OUT    psyOut;
  PSY_KERNEL psyKernel;

  struct BITSTREAMENCODER_INIT bseInit;

  struct STEREO_PREPRO stereoPrePro;

  struct BIT_BUF  bitStream;
  HANDLE_BIT_BUF  hBitStream;

  /* lifetime vars */
  int downmix;
  int downmixFac;
  int dualMono;
  int bandwidth90dB;
};


/* 
  static AAC encoder instance for one encoder 
  all other major static and dynamic memory areas are located
  in module aac_ram.c and aac_rom.c
*/
static struct AAC_ENCODER aacEncoder;


/*-----------------------------------------------------------------------------

     functionname: AacInitDefaultConfig
     description:  gives reasonable default configuration
     returns:      ---

 ------------------------------------------------------------------------------*/
void AacInitDefaultConfig(AACENC_CONFIG *config)
{
  COUNT_sub_start("AacInitDefaultConfig");

  /* make the pre initialization of the structs flexible */
  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(AACENC_CONFIG));
  memset(config, 0, sizeof(AACENC_CONFIG));

  /* default configurations */
  INDIRECT(2); MOVE(2);
  config->bitRate         = 48000;
  config->bandWidth       = 0;

  COUNT_sub_end();
}

/*---------------------------------------------------------------------------

    functionname: AacEncOpen
    description:  allocate and initialize a new encoder instance
    returns:      0 if success

  ---------------------------------------------------------------------------*/

int  
AacEncOpen ( struct AAC_ENCODER**     phAacEnc,       /* pointer to an encoder handle, initialized on return */
             const  AACENC_CONFIG     config          /* pre-initialized config struct */
             )
{
  int error = 0;
  int profile = 1;
  ELEMENT_INFO* elInfo = NULL;
  struct AAC_ENCODER  *hAacEnc ;
  
  COUNT_sub_start("AacEncOpen");
  
  MOVE(2); /* counting previous operations */
  
  PTR_INIT(1);
  hAacEnc = &aacEncoder;
  
  BRANCH(1);
  if (phAacEnc==0)  {
    MOVE(1);
    error=1;
  }
  
  BRANCH(1);
  if (!error) {
    /* sanity checks on config structure */
    PTR_INIT(1); ADD(7); LOGIC(9); DIV(1);
    error = (&config == 0            || phAacEnc == 0             ||
             config.nChannelsIn  < 1 || config.nChannelsIn  > MAX_CHANNELS ||
             config.nChannelsOut < 1 || config.nChannelsOut > MAX_CHANNELS ||
             config.nChannelsIn  < config.nChannelsOut            ||
             (config.bitRate!=0 && (config.bitRate / config.nChannelsOut < 8000      ||
                                    config.bitRate / config.nChannelsOut > 160000)));
  }
  
  /* check sample rate */
  BRANCH(1);
  if (!error)  {

    BRANCH(2);
    switch (config.sampleRate) {
    case  8000: case 11025: case 12000:
    case 16000: case 22050: case 24000:
    case 32000: case 44100: case 48000:
      break;

    default:
      MOVE(1);
      error = 1; break;
    }
  }

  
  /* check if bit rate is not too high for sample rate */
  BRANCH(1);
  if (!error) {

    MULT(2); ADD(1); BRANCH(1);
    if (config.bitRate > ((float)(MAX_CHANNEL_BITS-744)/FRAME_LEN_LONG*
                          config.sampleRate*config.nChannelsOut))
      {
        MOVE(1);
        error=1;
      }
  }
  
  
  BRANCH(1);
  if (!error) {
    INDIRECT(1); MOVE(1);
    hAacEnc->config = config;
  }
  
  
  BRANCH(1);
  if (!error) {
    INDIRECT(2); PTR_INIT(1); FUNC(2);
    error = InitElementInfo (config.nChannelsOut,
                             &hAacEnc->elInfo);
  }
  
  BRANCH(1);
  if (!error) {
    INDIRECT(1); PTR_INIT(1);
    elInfo = &hAacEnc->elInfo;
  }
  
  /* allocate the Psy aud Psy Out structure */
  BRANCH(1);
  if (!error) {

    INDIRECT(3); PTR_INIT(2); FUNC(2); FUNC(1); LOGIC(1);
    error = (PsyNew(&hAacEnc->psyKernel, elInfo->nChannelsInEl) ||
             PsyOutNew(&hAacEnc->psyOut));
  }
  
  BRANCH(1);
  if (!error) {
    int tnsMask=3;
    
    MOVE(1); /* counting previous operation */
    
    INDIRECT(2); MOVE(1);
    hAacEnc->bandwidth90dB = (int)hAacEnc->config.bandWidth;
    
    INDIRECT(4); PTR_INIT(1); FUNC(6);
    error = psyMainInit(&hAacEnc->psyKernel,
                        config.sampleRate,
                        config.bitRate,
                        elInfo->nChannelsInEl,
                        tnsMask,
                        hAacEnc->bandwidth90dB);
  }
  
  
  /* allocate the Q&C Out structure */
  BRANCH(1);
  if (!error) {
    INDIRECT(2); PTR_INIT(1); FUNC(2);
    error = QCOutNew(&hAacEnc->qcOut,
                     elInfo->nChannelsInEl);
  }
  
  /* allocate the Q&C kernel */
  BRANCH(1);
  if (!error) {
    INDIRECT(1); PTR_INIT(1); FUNC(1);
    error = QCNew(&hAacEnc->qcKernel);
  }
  
  BRANCH(1);
  if (!error) {
    struct QC_INIT qcInit;
    
    INDIRECT(1); PTR_INIT(1); MOVE(1);
    qcInit.elInfo = &hAacEnc->elInfo;
    
    INDIRECT(1); MULT(1); STORE(1);
    qcInit.maxBits = MAX_CHANNEL_BITS * elInfo->nChannelsInEl;
    
    MOVE(1);
    qcInit.bitRes = qcInit.maxBits;
    
    MULT(1); DIV(1); STORE(1);
    qcInit.averageBits = (config.bitRate * FRAME_LEN_LONG) / config.sampleRate;
    
    MOVE(1);
    qcInit.padding.paddingRest = config.sampleRate;
    
    INDIRECT(1); MULT(2); DIV(1);
    qcInit.meanPe = 10.0f * FRAME_LEN_LONG * hAacEnc->bandwidth90dB/(config.sampleRate/2.0f);
    
    MULT(1); DIV(1); BRANCH(1); STORE(1);
    qcInit.maxBitFac = (float)((MAX_CHANNEL_BITS-744)*elInfo->nChannelsInEl) /
      (float)(qcInit.averageBits?qcInit.averageBits:1);
    
    MOVE(1);
    qcInit.bitrate = config.bitRate;
    
    INDIRECT(1); PTR_INIT(2); FUNC(2);
    error = QCInit(&hAacEnc->qcKernel, &qcInit);
  }
  
  /* init bitstream encoder */
  BRANCH(1);
  if (!error) {

    INDIRECT(5); MOVE(4);
      hAacEnc->bseInit.nChannels   = elInfo->nChannelsInEl;
      hAacEnc->bseInit.bitrate     = config.bitRate;
      hAacEnc->bseInit.sampleRate  = config.sampleRate;
      hAacEnc->bseInit.profile     = profile;
  }
  
  
  /* common things */
  BRANCH(1);
  if (!error) {
    
    INDIRECT(1); ADD(2); LOGIC(1); STORE(1);
    hAacEnc->downmix = (config.nChannelsIn==2 && config.nChannelsOut==1);
    
    INDIRECT(1); BRANCH(1); MOVE(1);
    hAacEnc->downmixFac = (hAacEnc->downmix) ? config.nChannelsIn : 1;
  }
  
  
  
  BRANCH(1);
  if(!error) {

    /*
      decide if stereo preprocessing should be activated 
    */
    
    ADD(3); DIV(1); MULT(1); LOGIC(2); BRANCH(1);
    if ( elInfo->elType == ID_CPE &&
         (config.sampleRate <= 24000 && (config.bitRate/elInfo->nChannelsInEl*2) < 60000) ) {

      float scfUsedRatio = (float) hAacEnc->psyKernel.psyConfLong.sfbActive / hAacEnc->psyKernel.psyConfLong.sfbCnt ;
      
      DIV(1); /* counting previous operation */
      
      FUNC(5);
      error = InitStereoPreProcessing(&(hAacEnc->stereoPrePro),
                                      elInfo->nChannelsInEl, 
                                      config.bitRate, 
                                      config.sampleRate,
                                      scfUsedRatio);
    }
  }
  
  BRANCH(1);
  if (error) {
    
    FUNC(1);
    AacEncClose(hAacEnc);
    
    MOVE(1);
    hAacEnc=0;
  }
  
  MOVE(1);
  *phAacEnc = hAacEnc;
  
  COUNT_sub_end();
  
  return error;
}


int AacEncEncode(struct AAC_ENCODER *aacEnc,          /*!< an encoder handle */
                 float  *timeSignal,                  /*!< BLOCKSIZE*nChannels audio samples */
                 unsigned int timeInStride,
                 const unsigned char *ancBytes,       /*!< pointer to ancillary data bytes */
                 unsigned int        *numAncBytes,    /*!< number of ancillary Data Bytes */
                 unsigned int        *outBytes,       /*!< pointer to output buffer            */
                 int                 *numOutBytes     /*!< number of bytes in output buffer */
                 )
{
  ELEMENT_INFO* elInfo = &aacEnc->elInfo;
  int globUsedBits;
  int ancDataBytes, ancDataBytesLeft;
  
  COUNT_sub_start("AacEncEncode");

  INDIRECT(2); PTR_INIT(1); FUNC(3);
  aacEnc->hBitStream = CreateBitBuffer(&aacEnc->bitStream, 
                                       (unsigned char*) outBytes, 
                                       (MAX_CHANNEL_BITS/8)*MAX_CHANNELS);

  INDIRECT(1); PTR_INIT(1); /* counting previous operation */

  MOVE(2);
  ancDataBytes = ancDataBytesLeft =  *numAncBytes;


  /* advance psychoacoustic */

  LOGIC(1); BRANCH(1);
  if (elInfo->elType == ID_CPE) {

    PTR_INIT(1); FUNC(5); 
    ApplyStereoPreProcess(&aacEnc->stereoPrePro,
                          timeInStride,
                          elInfo,
                          timeSignal,
                          FRAME_LEN_LONG);
  }

  INDIRECT(5); PTR_INIT(4); FUNC(9);
  psyMain(timeInStride,
          elInfo,
          timeSignal,
          &aacEnc->psyKernel.psyData[elInfo->ChannelIndex[0]],
          &aacEnc->psyKernel.tnsData[elInfo->ChannelIndex[0]],
          &aacEnc->psyKernel.psyConfLong,
          &aacEnc->psyKernel.psyConfShort,
          &aacEnc->psyOut.psyOutChannel[elInfo->ChannelIndex[0]],
          &aacEnc->psyOut.psyOutElement,
          aacEnc->psyKernel.pScratchTns);
  

  INDIRECT(3); PTR_INIT(1); FUNC(3);
  AdjustBitrate(&aacEnc->qcKernel,
                aacEnc->config.bitRate,
                aacEnc->config.sampleRate);

  PTR_INIT(4); /* 
                  aacEnc->qcKernel.elementBits
                  aacEnc->qcKernel.adjThr.adjThrStateElem,
                  aacEnc->psyOut.psyOutElement,
                  aacEnc->qcOut.qcElement
               */

  ADD(1); BRANCH(1); MOVE(1); /* min() */ INDIRECT(2); PTR_INIT(3); FUNC(9);
  QCMain( &aacEnc->qcKernel,
          elInfo->nChannelsInEl,
          &aacEnc->qcKernel.elementBits,
          &aacEnc->qcKernel.adjThr.adjThrStateElem,
          &aacEnc->psyOut.psyOutChannel[elInfo->ChannelIndex[0]],
          &aacEnc->psyOut.psyOutElement,
          &aacEnc->qcOut.qcChannel[elInfo->ChannelIndex[0]],
          &aacEnc->qcOut.qcElement,
          min(ancDataBytesLeft,ancDataBytes));

    
  LOGIC(1); BRANCH(1);
  if ( elInfo->elType == ID_CPE ) {

    INDIRECT(1); PTR_INIT(2); FUNC(4);
    UpdateStereoPreProcess(&aacEnc->psyOut.psyOutChannel[elInfo->ChannelIndex[0]],
                           &aacEnc->qcOut.qcElement,
                           &aacEnc->stereoPrePro,
                           aacEnc->psyOut.psyOutElement.weightMsLrPeRatio);
  }

  ADD(1);
  ancDataBytesLeft-=ancDataBytes;
   
  
  INDIRECT(2); PTR_INIT(2); FUNC(2);
  FinalizeBitConsumption( &aacEnc->qcKernel,
                          &aacEnc->qcOut);

  INDIRECT(3); PTR_INIT(3); FUNC(7);
  WriteBitstream( aacEnc->hBitStream,
                  *elInfo,
                  &aacEnc->qcOut,
                  &aacEnc->psyOut,
                  &globUsedBits,
                  ancBytes);

  INDIRECT(2); PTR_INIT(2); FUNC(2);
  UpdateBitres(&aacEnc->qcKernel,
               &aacEnc->qcOut);

  /* write out the bitstream */
  INDIRECT(1); FUNC(1); DIV(1); STORE(1);
  *numOutBytes = GetBitsAvail(aacEnc->hBitStream)/8;
  
  /* assert this frame is not too large */
  assert(*numOutBytes*8 <= MAX_CHANNEL_BITS * elInfo->nChannelsInEl);
  
  COUNT_sub_end();
  
  return 0;
}


/*---------------------------------------------------------------------------

    functionname:AacEncClose
    description: deallocate an encoder instance

  ---------------------------------------------------------------------------*/

void AacEncClose (struct AAC_ENCODER* hAacEnc)
{
  int error=0;

  COUNT_sub_start("AacEncClose");

  MOVE(1); /* counting previous operation */

  BRANCH(1);
  if (hAacEnc) {
    
    INDIRECT(1); PTR_INIT(1); FUNC(1);
    QCDelete(&hAacEnc->qcKernel);
    
    INDIRECT(1); PTR_INIT(1); FUNC(1);
    QCOutDelete(&hAacEnc->qcOut);
    
    INDIRECT(1); PTR_INIT(1); FUNC(1);
    error = PsyDelete(&hAacEnc->psyKernel);
    
    INDIRECT(1); PTR_INIT(1); FUNC(1);
    error = PsyOutDelete(&hAacEnc->psyOut);
    
    INDIRECT(1); PTR_INIT(1); FUNC(1);
    DeleteBitBuffer(&hAacEnc->hBitStream);
    
    PTR_INIT(1);
    hAacEnc=0;
  }
  
  COUNT_sub_end();
}
