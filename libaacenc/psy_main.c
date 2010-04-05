/*
  Psychoaccoustic main module
*/
#include <stdlib.h>
#include <string.h>
#include "psy_const.h"
#include "block_switch.h"
#include "transform.h"
#include "spreading.h"
#include "pre_echo_control.h"
#include "band_nrg.h"
#include "psy_configuration.h"
#include "psy_data.h"
#include "ms_stereo.h"
#include "interface.h"
#include "psy_main.h"
#include "grp_data.h"
#include "tns_func.h"
#include "minmax.h"
#include "aac_ram.h"
#include "sbr_ram.h"

#include "counters.h" /* the 3GPP instrumenting tool */

/*                                    long       start       short       stop */
static int blockType2windowShape[] = {KBD_WINDOW,SINE_WINDOW,SINE_WINDOW,KBD_WINDOW};

/*
   forward definitions
*/
static int advancePsychLong(PSY_DATA* psyData,
                            TNS_DATA* tnsData,
                            PSY_CONFIGURATION_LONG *psyConfLong,
                            PSY_OUT_CHANNEL* psyOutChannel,
                            float *pScratchTns,
                            const TNS_DATA *tnsData2,
                            const int ch);

static int advancePsychLongMS (PSY_DATA  psyData[MAX_CHANNELS],
                               PSY_CONFIGURATION_LONG *psyConfLong);

static int advancePsychShort(PSY_DATA* psyData,
                             TNS_DATA* tnsData,
                             PSY_CONFIGURATION_SHORT *psyConfShort,
                             PSY_OUT_CHANNEL* psyOutChannel,
                             float *pScratchTns,
                             const TNS_DATA *tnsData2,
                             const int ch);

static int advancePsychShortMS (PSY_DATA  psyData[MAX_CHANNELS],
                                PSY_CONFIGURATION_SHORT *psyConfShort);


/*****************************************************************************

    functionname: PsyNew
    description:  allocates memory for psychoacoustic
    returns:      an error code
    input:        pointer to a psych handle

*****************************************************************************/
int PsyNew(PSY_KERNEL  *hPsy, int nChan)
{
    int i;

    COUNT_sub_start("PsyNew");

    PTR_INIT(3); /* hPsy->psyData[]
                    mdctDelayBuffer[]
                    mdctSpectrum[]
                 */
    LOOP(1);
    for (i=0; i<nChan; i++){
      /*
        reserve memory for mdct delay buffer
      */
      PTR_INIT(1);
      hPsy->psyData[i].mdctDelayBuffer = &mdctDelayBuffer[i*BLOCK_SWITCHING_OFFSET];

      
      /*
        reserve memory for mdct spectum
      */
      
      PTR_INIT(1);
      hPsy->psyData[i].mdctSpectrum = &sbr_envRBuffer[i*FRAME_LEN_LONG];

    }
  
    INDIRECT(1); PTR_INIT(1);
    hPsy->pScratchTns = sbr_envIBuffer;

    COUNT_sub_end();
 
    return 0;
}

/*****************************************************************************

    functionname: PsyDelete
    description:  allocates memory for psychoacoustic
    returns:      an error code

*****************************************************************************/
int PsyDelete(PSY_KERNEL  *hPsy)
{
  COUNT_sub_start("PsyDelete");

  /*
    nothing to do
  */
  PTR_INIT(1);
  hPsy=NULL;

  COUNT_sub_end();
  return 0;
}


/*****************************************************************************

    functionname: PsyOutNew
    description:  allocates memory for psyOut struc
    returns:      an error code
    input:        pointer to a psych handle

*****************************************************************************/
int PsyOutNew(PSY_OUT *hPsyOut)
{
  
  COUNT_sub_start("PsyOutNew");

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(PSY_OUT));
  memset(hPsyOut,0,sizeof(PSY_OUT));
 
  /*
    alloc some more stuff, tbd
  */

  COUNT_sub_end();
  return 0;
}

/*****************************************************************************

    functionname: PsyOutDelete
    description:  allocates memory for psychoacoustic
    returns:      an error code

*****************************************************************************/
int PsyOutDelete(PSY_OUT *hPsyOut)
{
  COUNT_sub_start("PsyOutDelete");

  PTR_INIT(1);
  hPsyOut=NULL;

  COUNT_sub_end();
  return 0;
}


/*****************************************************************************

    functionname: psyMainInit
    description:  initializes psychoacoustic
    returns:      an error code

*****************************************************************************/

int psyMainInit(PSY_KERNEL *hPsy,
                int sampleRate,
                int bitRate,
                int channels,
                int tnsMask,
                int bandwidth)
{
  int ch, err;

  COUNT_sub_start("psyMainInit");

  DIV(1); PTR_INIT(1); FUNC(4);
  err = InitPsyConfiguration(bitRate/channels, sampleRate, bandwidth, &(hPsy->psyConfLong));

  BRANCH(1);
  if (!err)
  {
    PTR_INIT(2); LOGIC(1); FUNC(6);
    err = InitTnsConfiguration(bitRate, sampleRate, channels,
                               &hPsy->psyConfLong.tnsConf, hPsy->psyConfLong, (int)(tnsMask&2));
  }

  BRANCH(1);
  if (!err)
  {
    PTR_INIT(1); FUNC(4);
    err = InitPsyConfigurationShort(bitRate/channels, sampleRate, bandwidth, &hPsy->psyConfShort);
  }

  BRANCH(1);
  if (!err)
  {
    PTR_INIT(2); LOGIC(1); FUNC(6);
    err =
      InitTnsConfigurationShort(bitRate, sampleRate, channels,
                                &hPsy->psyConfShort.tnsConf, hPsy->psyConfShort,(int)(tnsMask&1));
  }

  BRANCH(1);
  if (!err)
  {
    PTR_INIT(1); /* hPsy->psyData[] */
    LOOP(1);
    for(ch=0;ch < channels;ch++){
  
      PTR_INIT(1); FUNC(3);
      InitBlockSwitching(&hPsy->psyData[ch].blockSwitchingControl,
                         bitRate, channels);

      INDIRECT(1); FUNC(3);
      InitPreEchoControl(   hPsy->psyData[ch].sfbThresholdnm1,
                            hPsy->psyConfLong.sfbCnt,
                            hPsy->psyConfLong.sfbThresholdQuiet);
  }
  }
 
  COUNT_sub_end();

  return(err);
}







/*****************************************************************************

    functionname: psyMain
    description:  psychoacoustic
    returns:      an error code

        This function assumes that enough input data is in the modulo buffer.

*****************************************************************************/

int psyMain(int   timeInStride,
            ELEMENT_INFO *elemInfo,
            float *timeSignal, 
            PSY_DATA  psyData[MAX_CHANNELS],
            TNS_DATA  tnsData[MAX_CHANNELS],
            PSY_CONFIGURATION_LONG  *psyConfLong,
            PSY_CONFIGURATION_SHORT *psyConfShort,
            PSY_OUT_CHANNEL  psyOutChannel[MAX_CHANNELS],
            PSY_OUT_ELEMENT* psyOutElement,
            float* pScratchTns)
{
  int maxSfbPerGroup[MAX_CHANNELS];
  int groupedSfbOffset[MAX_CHANNELS][MAX_GROUPED_SFB+1];  /* plus one for last dummy offset ! */
  float groupedSfbMinSnr[MAX_CHANNELS][MAX_GROUPED_SFB];
  
  int ch;   /* counts through channels          */
  int sfb;  /* counts through scalefactor bands */
  int line; /* counts through lines             */
  int channels = elemInfo->nChannelsInEl;
  
  COUNT_sub_start("psyMain");
  
  
  /* block switching */
  
  PTR_INIT(2); /* pointers for psyData[],
                  elemInfo->ChannelIndex[]
               */
  LOOP(1);
  for(ch = 0; ch < channels; ch++) {
    
    PTR_INIT(2); FUNC(3);
    BlockSwitching (&psyData[ch].blockSwitchingControl,
                    timeSignal+elemInfo->ChannelIndex[ch],
                    timeInStride);
  }
  
  /* synch left and right block type */
  PTR_INIT(2); FUNC(4);
  SyncBlockSwitching( &psyData[0].blockSwitchingControl,
                      &psyData[1].blockSwitchingControl,
                      channels);
  
  
  /* transform */
  PTR_INIT(2); /* pointers for psyData[],
                  elemInfo->ChannelIndex[]
               */
  LOOP(1);
  for(ch = 0; ch < channels; ch++) {

    ADD(1); BRANCH(1);
    if(psyData[ch].blockSwitchingControl.windowSequence != SHORT_WINDOW){
      
      PTR_INIT(1); FUNC(5);
      Transform_Real( psyData[ch].mdctDelayBuffer,
                      timeSignal+elemInfo->ChannelIndex[ch],
                      timeInStride,
                      psyData[ch].mdctSpectrum,
                      psyData[ch].blockSwitchingControl.windowSequence);
    }
    else {
      
      PTR_INIT(1); FUNC(5);
      Transform_Real( psyData[ch].mdctDelayBuffer,
                      timeSignal+elemInfo->ChannelIndex[ch],
                      timeInStride,
                      psyData[ch].mdctSpectrum,
                      SHORT_WINDOW);
    }
  }
  
  PTR_INIT(4); /* pointers for psyData[],
                  tnsData[],
                  psyOutChannel[],
                  maxSfbPerGroup[]
               */
  LOOP(1);
  for(ch = 0; ch < channels; ch++) {
    
    ADD(1); BRANCH(1);
    if(psyData[ch].blockSwitchingControl.windowSequence != SHORT_WINDOW) {

      PTR_INIT(1); FUNC(7);
      advancePsychLong(   &psyData[ch],
                          &tnsData[ch],
                          psyConfLong,
                          &psyOutChannel[ch],
                          pScratchTns,
                          &tnsData[1-ch],
                          ch);
      
      /* determine maxSfb */
      PTR_INIT(1); /* pointers for psyConfLong->sfbOffset[] */
      LOOP(1);
      for (sfb = psyConfLong->sfbCnt-1; sfb >= 0; sfb--) {
        
        PTR_INIT(1); /* pointers for psyData[ch].mdctSpectrum[] */
        LOOP(1);
        for (line = psyConfLong->sfbOffset[sfb+1]-1; line >= psyConfLong->sfbOffset[sfb]; line--) {
          
          BRANCH(1);
          if (psyData[ch].mdctSpectrum[line] != 0) break;
        }
        ADD(1); BRANCH(1);
        if (line >= psyConfLong->sfbOffset[sfb]) break;
      }
      
      ADD(1); STORE(1);
      maxSfbPerGroup[ch] = sfb + 1;
      
      /* Calc bandwise energies for mid and side channel
         Do it only if 2 channels exist */
      ADD(1); BRANCH(1);
      if (ch==1) {

        FUNC(2);
        advancePsychLongMS(psyData, psyConfLong);
      }
      
    }
    else {

      PTR_INIT(1); FUNC(7);
      advancePsychShort(  &psyData[ch],
                          &tnsData[ch],
                          psyConfShort,
                          &psyOutChannel[ch],
                          pScratchTns,
                          &tnsData[1-ch],
                          ch);
      
      /* Calc bandwise energies for mid and side channel
         Do it only if 2 channels exist */
      ADD(1); BRANCH(1);
      if (ch==1) {

        FUNC(2);
        advancePsychShortMS (psyData, psyConfShort);
      }
      
    }
  }
  
  /* group short data (maxSfb for short blocks is determined here) */
  PTR_INIT(5); /* pointers for psyData[],
                  tnsData[],
                  psyOutChannel[],
                  maxSfbPerGroup[],
                  groupedSfbOffset[]
               */
  LOOP(1);
  for(ch=0;ch<channels;ch++) {

    ADD(1); BRANCH(1);
    if(psyData[ch].blockSwitchingControl.windowSequence == SHORT_WINDOW) {
        
      INDIRECT(3); PTR_INIT(4); FUNC(14);
      groupShortData( psyData[ch].mdctSpectrum,
                      pScratchTns,
                      &psyData[ch].sfbThreshold,
                      &psyData[ch].sfbEnergy,
                      &psyData[ch].sfbEnergyMS,
                      &psyData[ch].sfbSpreadedEnergy,
                      psyConfShort->sfbCnt,
                      psyConfShort->sfbOffset,
                      psyConfShort->sfbMinSnr,
                      groupedSfbOffset[ch],
                      &maxSfbPerGroup[ch],
                      groupedSfbMinSnr[ch],
                      psyData[ch].blockSwitchingControl.noOfGroups,
                      psyData[ch].blockSwitchingControl.groupLen);
    }
    
  }
  
#if (MAX_CHANNELS>1)
  
  /*
    stereo Processing
  */
  ADD(1); BRANCH(1);
  if(channels == 2) {

    INDIRECT(2); MOVE(2); 
    psyOutElement->toolsInfo.msDigest = MS_NONE;
    
    ADD(1); BRANCH(1); MOVE(2);
    maxSfbPerGroup[0] = maxSfbPerGroup[1] = max(maxSfbPerGroup[0], maxSfbPerGroup[1]);

    ADD(1); BRANCH(1);
    if(psyData[0].blockSwitchingControl.windowSequence != SHORT_WINDOW) {
      
      INDIRECT(6); PTR_INIT(2); FUNC(17);
      MsStereoProcessing( psyData[0].sfbEnergy.Long,
                          psyData[1].sfbEnergy.Long,
                          psyData[0].sfbEnergyMS.Long,
                          psyData[1].sfbEnergyMS.Long,
                          psyData[0].mdctSpectrum,
                          psyData[1].mdctSpectrum,
                          psyData[0].sfbThreshold.Long,
                          psyData[1].sfbThreshold.Long,
                          psyData[0].sfbSpreadedEnergy.Long,
                          psyData[1].sfbSpreadedEnergy.Long,
                          &psyOutElement->toolsInfo.msDigest,
                          psyOutElement->toolsInfo.msMask,
                          psyConfLong->sfbCnt,
                          psyConfLong->sfbCnt,
                          maxSfbPerGroup[0],
                          psyConfLong->sfbOffset,
                          &psyOutElement->weightMsLrPeRatio);
    }
    else {
      
      INDIRECT(5); MULT(1); PTR_INIT(2); FUNC(17);
      MsStereoProcessing( psyData[0].sfbEnergy.Long,
                          psyData[1].sfbEnergy.Long,
                          psyData[0].sfbEnergyMS.Long,
                          psyData[1].sfbEnergyMS.Long,
                          psyData[0].mdctSpectrum,
                          psyData[1].mdctSpectrum,
                          psyData[0].sfbThreshold.Long,
                          psyData[1].sfbThreshold.Long,
                          psyData[0].sfbSpreadedEnergy.Long,
                          psyData[1].sfbSpreadedEnergy.Long,
                          &psyOutElement->toolsInfo.msDigest,
                          psyOutElement->toolsInfo.msMask,
                          psyData[0].blockSwitchingControl.noOfGroups*psyConfShort->sfbCnt,
                          psyConfShort->sfbCnt,
                          maxSfbPerGroup[0],
                          groupedSfbOffset[0],
                          &psyOutElement->weightMsLrPeRatio);
    }
  }
  
#endif /* (MAX_CHANNELS>1) */
  
  /*
    build output
  */
  PTR_INIT(5); /* pointers for psyData[],
                  psyOutChannel[],
                  maxSfbPerGroup[],
                  groupedSfbOffset[],
                  groupedSfbMinSnr[]
               */
  LOOP(1);
  for(ch=0;ch<channels;ch++) {

    ADD(1); BRANCH(1);
    if(psyData[ch].blockSwitchingControl.windowSequence != SHORT_WINDOW) {

      INDIRECT(3); PTR_INIT(4); INDIRECT(1); FUNC(15);
      BuildInterface( psyData[ch].mdctSpectrum,
                      &psyData[ch].sfbThreshold,
                      &psyData[ch].sfbEnergy,
                      &psyData[ch].sfbSpreadedEnergy,
                      psyData[ch].sfbEnergySum,
                      psyData[ch].sfbEnergySumMS,
                      psyData[ch].blockSwitchingControl.windowSequence,
                      blockType2windowShape[psyData[ch].blockSwitchingControl.windowSequence],
                      psyConfLong->sfbCnt,
                      psyConfLong->sfbOffset,
                      maxSfbPerGroup[ch],
                      psyConfLong->sfbMinSnr,
                      psyData[ch].blockSwitchingControl.noOfGroups,
                      psyData[ch].blockSwitchingControl.groupLen,
                      &psyOutChannel[ch]);
    }
    else {

      INDIRECT(1); PTR_INIT(4); MULT(1); FUNC(15);
      BuildInterface( psyData[ch].mdctSpectrum,
                      &psyData[ch].sfbThreshold,
                      &psyData[ch].sfbEnergy,
                      &psyData[ch].sfbSpreadedEnergy,
                      psyData[ch].sfbEnergySum,
                      psyData[ch].sfbEnergySumMS,
                      SHORT_WINDOW,
                      SINE_WINDOW,
                      psyData[0].blockSwitchingControl.noOfGroups*psyConfShort->sfbCnt,
                      groupedSfbOffset[ch],
                      maxSfbPerGroup[ch],
                      groupedSfbMinSnr[ch],
                      psyData[ch].blockSwitchingControl.noOfGroups,
                      psyData[ch].blockSwitchingControl.groupLen,
                      &psyOutChannel[ch]);
    }
  }
  
  COUNT_sub_end();
  
  return 0; /* no error */
}


/*****************************************************************************

    functionname: advancePsychLong
    description:  psychoacoustic for long blocks

*****************************************************************************/

static int advancePsychLong(PSY_DATA* psyData,
                            TNS_DATA* tnsData,
                            PSY_CONFIGURATION_LONG *psyConfLong,
                            PSY_OUT_CHANNEL* psyOutChannel,
                            float *pScratchTns,
                            const TNS_DATA* tnsData2,
                            const int ch)
{
    int i;
  
    /* the energy Shift is actually needed since we do not multiply the spectrum by two after the cmdct */
    float energyShift = 0.25f;

    float clipEnergy = psyConfLong->clipEnergy * energyShift;

    COUNT_sub_start("advancePsychLong");

    INDIRECT(1); MOVE(1); MULT(1); /* counting previous operations */
 
    /* low pass */
    PTR_INIT(1); /* pointers for psyData->mdctSpectrum[] */
    INDIRECT(1); LOOP(1);
    for(i=psyConfLong->lowpassLine; i<FRAME_LEN_LONG; i++){
        MOVE(1);
        psyData->mdctSpectrum[i] = 0.0f;
    }

    /* Calc sfb-bandwise mdct-energies for left and right channel */
    INDIRECT(5); PTR_INIT(1); FUNC(5);
    CalcBandEnergy( psyData->mdctSpectrum,
                    psyConfLong->sfbOffset,
                    psyConfLong->sfbActive,
                    psyData->sfbEnergy.Long,
                    &psyData->sfbEnergySum.Long);


    /*
        TNS
    */
    INDIRECT(5); FUNC(8);
    TnsDetect(  tnsData,
                psyConfLong->tnsConf,
                pScratchTns,
                psyConfLong->sfbOffset,
                psyData->mdctSpectrum,
                0,
                (int)psyData->blockSwitchingControl.windowSequence,
                psyData->sfbEnergy.Long);

    /*  TnsSync */
    ADD(1); BRANCH(1);
    if (ch==1) {
       INDIRECT(2); FUNC(5);
       TnsSync(tnsData,
               tnsData2,
               psyConfLong->tnsConf,
               0,
               (int)psyData->blockSwitchingControl.windowSequence);
    }

    INDIRECT(6); PTR_INIT(1); FUNC(8);
    TnsEncode(  &psyOutChannel->tnsInfo,
                tnsData,
                psyConfLong->sfbCnt,
                psyConfLong->tnsConf,
                psyConfLong->lowpassLine,
                psyData->mdctSpectrum,
                0,
                (int)psyData->blockSwitchingControl.windowSequence);

    /* first part of threshold calculation */
    PTR_INIT(2); /* pointers for psyData->sfbThreshold.Long[],
                                 psyData->sfbEnergy.Long[]
                 */
    INDIRECT(1); LOOP(1);
    for (i=0; i<psyConfLong->sfbCnt; i++) {

        /* multiply sfbEnergy by ratio */
        INDIRECT(1); MULT(1); STORE(1);
        psyData->sfbThreshold.Long[i] = psyData->sfbEnergy.Long[i] * psyConfLong->ratio;

        /* limit threshold to avoid clipping */
        ADD(1); BRANCH(1); MOVE(1);
        psyData->sfbThreshold.Long[i] = min(psyData->sfbThreshold.Long[i], clipEnergy);
    }

    /* Calc sfb-bandwise mdct-energies for left and right channel again,
       if tns has modified the spectrum */
    INDIRECT(1); ADD(1); BRANCH(1);
    if (psyOutChannel->tnsInfo.tnsActive[0]==1) {

        INDIRECT(5); PTR_INIT(1); FUNC(5);
        CalcBandEnergy( psyData->mdctSpectrum,
                        psyConfLong->sfbOffset,
                        psyConfLong->sfbActive,
                        psyData->sfbEnergy.Long,
                        &psyData->sfbEnergySum.Long);
    }


    /* spreading */
    INDIRECT(4); FUNC(4);
    SpreadingMax(   psyConfLong->sfbCnt,
                    psyConfLong->sfbMaskLowFactor,
                    psyConfLong->sfbMaskHighFactor,
                    psyData->sfbThreshold.Long);

    /* threshold in quiet */
    PTR_INIT(2); /* pointers for psyData->sfbThreshold.Long[],
                                 psyConfLong.sfbThresholdQuiet[]
                 */
    INDIRECT(1); LOOP(1);
    for (i=0; i<psyConfLong->sfbCnt;i++)
    {
        MULT(1); ADD(1); BRANCH(1); MOVE(1);
        psyData->sfbThreshold.Long[i] = max(psyData->sfbThreshold.Long[i],
                                            (psyConfLong->sfbThresholdQuiet[i] * energyShift));
    }


    /* preecho control */
    INDIRECT(1); ADD(1); BRANCH(1);
    if(psyData->blockSwitchingControl.windowSequence == STOP_WINDOW)
    {
        /* prevent PreEchoControl from comparing stop
           thresholds with short thresholds */
        PTR_INIT(1); /* pointer for psyData->sfbThresholdnm1[] */
        INDIRECT(1); LOOP(1);
        for (i=0; i<psyConfLong->sfbCnt;i++)
        {
            MOVE(1);
            psyData->sfbThresholdnm1[i] = 1.0e20f;
        }
    }

    INDIRECT(5); FUNC(5);
    PreEchoControl( psyData->sfbThresholdnm1,
                    psyConfLong->sfbCnt,
                    psyConfLong->maxAllowedIncreaseFactor,
                    psyConfLong->minRemainingThresholdFactor,
                    psyData->sfbThreshold.Long);

    INDIRECT(1); ADD(1); BRANCH(1);
    if(psyData->blockSwitchingControl.windowSequence == START_WINDOW)
    {
        /* prevent PreEchoControl in next frame to compare start
           thresholds with short thresholds */
        PTR_INIT(1); /* pointer for psyData->sfbThresholdnm1[] */
        INDIRECT(1); LOOP(1);
        for (i=0; i<psyConfLong->sfbCnt;i++)
        {
            MOVE(1);
            psyData->sfbThresholdnm1[i] = 1.0e20f;
        }
    }

    /* apply tns mult table on cb thresholds */
    BRANCH(1);
    if (psyOutChannel->tnsInfo.tnsActive[0]) {
      INDIRECT(3); FUNC(3);
      ApplyTnsMultTableToRatios(  psyConfLong->tnsConf.tnsRatioPatchLowestCb,
                                  psyConfLong->tnsConf.tnsStartBand,
                                  psyData->sfbThreshold.Long);
    }

    /* spreaded energy for avoid hole detection */
    PTR_INIT(2); /* pointers for psyData->sfbSpreadedEnergy.Long[],
                                 psyData->sfbEnergy.Long[]
                 */
    INDIRECT(1); LOOP(1);
    for (i=0; i<psyConfLong->sfbCnt; i++)
    {
      MOVE(1);
      psyData->sfbSpreadedEnergy.Long[i] = psyData->sfbEnergy.Long[i];
    }

    INDIRECT(4); FUNC(4);
    SpreadingMax(psyConfLong->sfbCnt,
                 psyConfLong->sfbMaskLowFactorSprEn, 
                 psyConfLong->sfbMaskHighFactorSprEn,
                 psyData->sfbSpreadedEnergy.Long);

    COUNT_sub_end();

    return 0;
}


static int advancePsychLongMS (PSY_DATA psyData[MAX_CHANNELS],
                               PSY_CONFIGURATION_LONG *psyConfLong)
{
    COUNT_sub_start("advancePsychLongMS");

    INDIRECT(2); PTR_INIT(2); FUNC(8);
    CalcBandEnergyMS(psyData[0].mdctSpectrum,
                     psyData[1].mdctSpectrum,
                     psyConfLong->sfbOffset,
                     psyConfLong->sfbActive,
                     psyData[0].sfbEnergyMS.Long,
                     &psyData[0].sfbEnergySumMS.Long,
                     psyData[1].sfbEnergyMS.Long,
                     &psyData[1].sfbEnergySumMS.Long);

    COUNT_sub_end();

    return 0;
}


/*****************************************************************************

    functionname: advancePsychShort
    description:  psychoacoustic for short blocks

*****************************************************************************/

static int advancePsychShort(PSY_DATA* psyData,
                             TNS_DATA* tnsData,
                             PSY_CONFIGURATION_SHORT *psyConfShort,
                             PSY_OUT_CHANNEL* psyOutChannel,
                             float *pScratchTns,
                             const TNS_DATA *tnsData2,
                             const int ch)
{
    int w;

    /* the energy Shift is actually needed since we do not multiply the spectrum by two after the cmdct */
    float energyShift = 0.25f;
    float clipEnergy = psyConfShort->clipEnergy *energyShift;

    COUNT_sub_start("advancePsychShort");

    INDIRECT(1); MOVE(1); MULT(1); /* counting previous operations */

    PTR_INIT(6); /* pointers for psyData->sfbEnergy.Short[w],
                                 psyData->sfbEnergySum.Short[w],
                                 psyData->sfbThreshold.Short[w],
                                 psyData->sfbSpreadedEnergy.Short[w],
                                 tnsData->dataRaw.Short.subBlockInfo[w],
                                 tnsData->dataRaw.Short.ratioMultTable[w]
                 */
    LOOP(1);
    for(w = 0; w < TRANS_FAC; w++)
    {
        int wOffset=w*FRAME_LEN_SHORT;
        int i;

        MULT(1); /* counting previous operation */

        /* low pass */
        PTR_INIT(1); /* pointers for psyData->mdctSpectrum[] */
        INDIRECT(1); LOOP(1);
        for(i=psyConfShort->lowpassLine; i<FRAME_LEN_SHORT; i++){
            MOVE(1);
            psyData->mdctSpectrum[i+wOffset] = 0.0f;
        }

        /* Calc sfb-bandwise mdct-energies for left and right channel */
        INDIRECT(5); FUNC(5);
        CalcBandEnergy( psyData->mdctSpectrum+wOffset,
                        psyConfShort->sfbOffset,
                        psyConfShort->sfbActive,
                        psyData->sfbEnergy.Short[w],
                        &psyData->sfbEnergySum.Short[w]);

        /*
        TNS
        */
        INDIRECT(4); FUNC(8);
        TnsDetect(  tnsData,
                    psyConfShort->tnsConf,
                    pScratchTns,
                    psyConfShort->sfbOffset,
                    psyData->mdctSpectrum+wOffset,
                    w,
                    (int)psyData->blockSwitchingControl.windowSequence,
                    psyData->sfbEnergy.Short[w]);

        /*  TnsSync */
        ADD(1); BRANCH(1);
        if (ch==1) {
            INDIRECT(2); FUNC(5);
            TnsSync(tnsData,
                    tnsData2,
                    psyConfShort->tnsConf,
                    w,
                    (int)psyData->blockSwitchingControl.windowSequence);
        }

        INDIRECT(6); PTR_INIT(1); FUNC(8);
        TnsEncode(  &psyOutChannel->tnsInfo,
                    tnsData,
                    psyConfShort->sfbCnt,
                    psyConfShort->tnsConf,
                    psyConfShort->lowpassLine,
                    psyData->mdctSpectrum+wOffset,
                    w,
                    (int)psyData->blockSwitchingControl.windowSequence);

        /* first part of threshold calculation */
        PTR_INIT(2); /* pointers for psyData->sfbThreshold.Short[][],
                                     psyData->sfbEnergy.Short[][]
                     */
        INDIRECT(1); LOOP(1);
        for (i=0; i<psyConfShort->sfbCnt; i++) {

            /* multiply sfbEnergy by ratio */
            INDIRECT(1); MULT(1); STORE(1);
            psyData->sfbThreshold.Short[w][i] = psyData->sfbEnergy.Short[w][i] * psyConfShort->ratio;

            /* limit threshold to avoid clipping */
            ADD(1); BRANCH(1); MOVE(1);
            psyData->sfbThreshold.Short[w][i] = min(psyData->sfbThreshold.Short[w][i], clipEnergy);
        }

        /* Calc sfb-bandwise mdct-energies for left and right channel again,
           if tns has modified the spectrum */

        ADD(1); BRANCH(1);
        if (psyOutChannel->tnsInfo.tnsActive[w]) {
          INDIRECT(3); FUNC(5);
          CalcBandEnergy( psyData->mdctSpectrum+wOffset,
                          psyConfShort->sfbOffset,
                          psyConfShort->sfbActive,
                          psyData->sfbEnergy.Short[w],
                          &psyData->sfbEnergySum.Short[w]);

        }

        /* spreading */
        INDIRECT(3); FUNC(4);
        SpreadingMax(   psyConfShort->sfbCnt,
                        psyConfShort->sfbMaskLowFactor,
                        psyConfShort->sfbMaskHighFactor,
                        psyData->sfbThreshold.Short[w]);


        /* threshold in quiet */
        PTR_INIT(2); /* pointers for psyData->sfbThreshold.Short[w][i] 
                                     psyConfShort.sfbThresholdQuiet[i]
                     */
        INDIRECT(1); LOOP(1);
        for(i=0;i<psyConfShort->sfbCnt;i++)
        {
            MULT(1); ADD(1); BRANCH(1); MOVE(1);
            psyData->sfbThreshold.Short[w][i] = max(psyData->sfbThreshold.Short[w][i],
                                                    (psyConfShort->sfbThresholdQuiet[i] * energyShift));
        }


        /* preecho */
        INDIRECT(4); FUNC(5);
        PreEchoControl( psyData->sfbThresholdnm1,
                        psyConfShort->sfbCnt,
                        psyConfShort->maxAllowedIncreaseFactor,
                        psyConfShort->minRemainingThresholdFactor,
                        psyData->sfbThreshold.Short[w]);

        /* apply tns mult table on cb thresholds */
        BRANCH(1);
        if (psyOutChannel->tnsInfo.tnsActive[w]) {
          INDIRECT(3); FUNC(3);
          ApplyTnsMultTableToRatios( psyConfShort->tnsConf.tnsRatioPatchLowestCb,
                                     psyConfShort->tnsConf.tnsStartBand,
                                     psyData->sfbThreshold.Short[w]);
        }
        
        /* spreaded energy for avoid hole detection */
        PTR_INIT(2); /* pointers for psyData->sfbSpreadedEnergy.Short[][],
                                     psyData->sfbEnergy.Short[][]
                     */
        INDIRECT(1); LOOP(1);
        for (i=0; i<psyConfShort->sfbCnt; i++)
        {
          MOVE(1);
          psyData->sfbSpreadedEnergy.Short[w][i] = psyData->sfbEnergy.Short[w][i];
        }

        INDIRECT(3); FUNC(4);
        SpreadingMax(psyConfShort->sfbCnt,
                     psyConfShort->sfbMaskLowFactorSprEn, 
                     psyConfShort->sfbMaskHighFactorSprEn,
                     psyData->sfbSpreadedEnergy.Short[w]);

    } /* for TRANS_FAC */

    COUNT_sub_end();

    return 0;
}


static int advancePsychShortMS (PSY_DATA psyData[MAX_CHANNELS],
                                PSY_CONFIGURATION_SHORT *psyConfShort)
{
    int w;

    COUNT_sub_start("advancePsychShortMS");

    PTR_INIT(4); /* pointers for psyData[0].sfbEnergyMS.Short[w],
                                 psyData[0].sfbEnergySumMS.Short[w],
                                 psyData[1].sfbEnergyMS.Short[w],
                                 psyData[1].sfbEnergySumMS.Short[w]
                 */
    LOOP(1);
    for(w = 0; w < TRANS_FAC; w++) {
        int   wOffset=w*FRAME_LEN_SHORT;

        MULT(1); /* counting previous operation */

        INDIRECT(2); FUNC(8);
        CalcBandEnergyMS(   psyData[0].mdctSpectrum+wOffset,
                            psyData[1].mdctSpectrum+wOffset,
                            psyConfShort->sfbOffset,
                            psyConfShort->sfbActive,
                            psyData[0].sfbEnergyMS.Short[w],
                            &psyData[0].sfbEnergySumMS.Short[w],
                            psyData[1].sfbEnergyMS.Short[w],
                            &psyData[1].sfbEnergySumMS.Short[w]);
    }

    COUNT_sub_end();

    return 0;
}
