/*
  SBR encoder top level processing
*/
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sbr_main.h"
#include "sbr.h"
#include "sbr_ram.h"
#include "freq_sca.h"
#include "ps_enc.h"
#include "qmf_enc.h"
#include "env_est.h"
#include "env_bit.h"
#include "cmondata.h"
#include "FloatFR.h"

#include "counters.h" /* the 3GPP instrumenting tool */

#define INVALID_TABLE_IDX -1

/*
   tuningTable
*/
static const struct
{
  unsigned int    bitrateFrom ;
  unsigned int    bitrateTo ;

  unsigned int    sampleRate ;
  unsigned int    numChannels ;

  unsigned int    startFreq ;
  unsigned int    stopFreq ;

  int numNoiseBands ;
  int noiseFloorOffset ;
  int noiseMaxLevel ;
  SBR_STEREO_MODE stereoMode ;
  int freqScale ;

} tuningTable[] = {


  /*** mono ***/
  { 10000, 12000,  16000, 1,  1,  3, 1, 0, 6, SBR_MONO, 3 }, /* nominal: 10 kbit/s */
  { 12000, 16000,  16000, 1,  2,  0, 1, 0, 6, SBR_MONO, 3 }, /* nominal: 12 kbit/s */
  { 16000, 20000,  16000, 1,  2,  3, 1, 0, 6, SBR_MONO, 3 }, /* nominal: 18 kbit/s */

  { 12000, 18000,  22050, 1,  1,  4, 1, 0, 6, SBR_MONO, 3 }, /* nominal: 14 kbit/s */
  { 18000, 22000,  22050, 1,  3,  5, 2, 0, 6, SBR_MONO, 2 }, /* nominal: 20 kbit/s */
  { 22000, 28000,  22050, 1,  7,  8, 2, 0, 6, SBR_MONO, 2 }, /* nominal: 24 kbit/s */
  { 28000, 36000,  22050, 1, 11,  9, 2, 0, 3, SBR_MONO, 2 }, /* nominal: 32 kbit/s */
  { 36000, 44001,  22050, 1, 11,  9, 2, 0, 3, SBR_MONO, 1 }, /* nominal: 40 kbit/s */

  { 12000, 18000,  24000, 1,  1,  4, 1, 0, 6, SBR_MONO, 3 }, /* nominal: 14 kbit/s */
  { 18000, 22000,  24000, 1,  3,  5, 2, 0, 6, SBR_MONO, 2 }, /* nominal: 20 kbit/s */
  { 22000, 28000,  24000, 1,  7,  8, 2, 0, 6, SBR_MONO, 2 }, /* nominal: 24 kbit/s */
  { 28000, 36000,  24000, 1, 10,  9, 2, 0, 3, SBR_MONO, 2 }, /* nominal: 32 kbit/s */
  { 36000, 44001,  24000, 1, 11,  9, 2, 0, 3, SBR_MONO, 1 }, /* nominal: 40 kbit/s */

  /*** stereo ***/
  { 18000, 24000,  16000, 2,  4,  2, 1, 0, -3, SBR_SWITCH_LRC, 3 }, /* nominal: 18 kbit/s */

  { 24000, 28000,  22050, 2,  5,  6, 1, 0, -3, SBR_SWITCH_LRC, 3 }, /* nominal: 24 kbit/s */
  { 28000, 36000,  22050, 2,  7,  8, 2, 0, -3, SBR_SWITCH_LRC, 2 }, /* nominal: 32 kbit/s */
#ifdef __LP64__
  { 36000, 44000,  22050, 2, 10,  8, 2, 0, -3, SBR_SWITCH_LRC, 2 }, /* nominal: 40 kbit/s */
#else
  { 36000, 44000,  22050, 2, 10,  9, 2, 0, -3, SBR_SWITCH_LRC, 2 }, /* nominal: 40 kbit/s */
#endif

  { 44000, 64001,  22050, 2, 12,  9, 3, 0, -3, SBR_SWITCH_LRC, 2 }, /* nominal: 48 kbit/s */

  { 24000, 28000,  24000, 2,  5,  6, 1, 0, -3, SBR_SWITCH_LRC, 3 }, /* nominal: 24 kbit/s */
  { 28000, 36000,  24000, 2,  7,  8, 2, 0, -3, SBR_SWITCH_LRC, 2 }, /* nominal: 32 kbit/s */
  { 36000, 44000,  24000, 2, 10,  9, 2, 0, -3, SBR_SWITCH_LRC, 2 }, /* nominal: 40 kbit/s */
  { 44000, 64000,  24000, 2, 12,  9, 3, 0, -3, SBR_SWITCH_LRC, 2 }, /* nominal: 48 kbit/s */

};


struct SBR_ENCODER
{

  struct SBR_CONFIG_DATA    sbrConfigData;
  struct SBR_HEADER_DATA    sbrHeaderData;
  struct SBR_BITSTREAM_DATA sbrBitstreamData;
  struct ENV_CHANNEL*       hEnvChannel[MAX_CHANNELS];
  struct COMMON_DATA        CmonData;
  struct PS_ENC             *hPsEnc;
  SBR_QMF_FILTER_BANK       *hSynthesisQmfBank;
  unsigned int              sbrPayloadPrevious[MAX_PAYLOAD_SIZE/(sizeof(int))];
  unsigned int              sbrPayload[MAX_PAYLOAD_SIZE/(sizeof(int))];
  int                       sbrPayloadSize;
} ;


static struct ENV_CHANNEL EnvChannel[MAX_CHANNELS];
static struct SBR_ENCODER sbrEncoder;
static SBR_QMF_FILTER_BANK SynthesisQmfBank;
static struct PS_ENC psEncoder;


/***************************************************************************/
/*!

  \brief  Selects the SBR tuning.
  \return Index

****************************************************************************/
static int
getSbrTuningTableIndex(unsigned int bitrate,
                       unsigned int numChannels,
                       unsigned int sampleRate
                       )
{
  int i, paramSets = sizeof (tuningTable) / sizeof (tuningTable [0]);

  COUNT_sub_start("getSbrTuningTableIndex");

  MOVE(1); /* counting previous operation */

  PTR_INIT(1); /* tuningTable[] */
  LOOP(1);
  for (i = 0 ; i < paramSets ; i++)  {
    ADD(1); BRANCH(1);
    if (numChannels == tuningTable [i].numChannels) {

      ADD(3); LOGIC(2); BRANCH(1);
      if ((sampleRate == tuningTable [i].sampleRate) &&
          (bitrate >= tuningTable [i].bitrateFrom) &&
          (bitrate < tuningTable [i].bitrateTo)) {
        COUNT_sub_end();
        return i ;
      }
    }
  }

  COUNT_sub_end();

  return INVALID_TABLE_IDX;
}

/***************************************************************************/
/*!

  \brief  Is SBR setting available.
  \return a flag indicating success: yes (1) or no (0)

****************************************************************************/
unsigned int
IsSbrSettingAvail (unsigned int bitrate,
                   unsigned int numOutputChannels,
                   unsigned int sampleRateInput,
                   unsigned int *sampleRateCore
                   )
{
  int idx = INVALID_TABLE_IDX;

  COUNT_sub_start("IsSbrSettingAvail");

  MOVE(1); /* counting previous operation */

  ADD(1); BRANCH(1);
  if (sampleRateInput < 32000)
  {
    COUNT_sub_end();
    return 0;
  }

  DIV(1); STORE(1);
  *sampleRateCore = sampleRateInput/2;

  FUNC(3);
  idx = getSbrTuningTableIndex(bitrate,numOutputChannels, *sampleRateCore);

  ADD(1); BRANCH(1); /* counting post-operation */

  COUNT_sub_end();

  return (idx == INVALID_TABLE_IDX) ? 0 : 1;
}


/***************************************************************************/
/*!

  \brief  Adjusts the SBR settings.
  \return success:

****************************************************************************/
unsigned int
AdjustSbrSettings (const sbrConfigurationPtr config,
                   unsigned int bitRate,
                   unsigned int numChannels,
                   unsigned int fsCore,
                   unsigned int transFac,
                   unsigned int standardBitrate
                   )
{
  int idx = INVALID_TABLE_IDX;
  unsigned int sampleRate;

  COUNT_sub_start("AdjustSbrSettings");

  MOVE(1); /* counting previous operation */

  /* set the codec settings */
  INDIRECT(5); MOVE(5);
  config->codecSettings.bitRate         = bitRate;
  config->codecSettings.nChannels       = numChannels;
  config->codecSettings.sampleFreq      = fsCore;
  config->codecSettings.transFac        = transFac;
  config->codecSettings.standardBitrate = standardBitrate;

  MULT(1);
  sampleRate  = fsCore * 2;

  FUNC(3);
  idx = getSbrTuningTableIndex(bitRate,numChannels,fsCore);

  ADD(1); BRANCH(1);
  if (idx != INVALID_TABLE_IDX) {

    INDIRECT(4); MOVE(3);
    config->startFreq       = tuningTable [idx].startFreq ;
    config->stopFreq        = tuningTable [idx].stopFreq ;
    config->sbr_noise_bands = tuningTable [idx].numNoiseBands ;

    INDIRECT(1); MOVE(1);
    config->noiseFloorOffset= tuningTable[idx].noiseFloorOffset;

    INDIRECT(3); MOVE(3);
    config->ana_max_level   = tuningTable [idx].noiseMaxLevel ;
    config->stereoMode      = tuningTable[idx].stereoMode ;
    config->freqScale       = tuningTable[idx].freqScale ;

    ADD(1); BRANCH(1);
    if (bitRate <= 20000) {

      INDIRECT(2); MOVE(2);
      config->parametricCoding = 0;
      config->useSpeechConfig  = 1;
    }

#ifndef MONO_ONLY
    INDIRECT(1); BRANCH(1);
    if(config->usePs)
    {
      INDIRECT(1); FUNC(1); STORE(1);
      config->psMode = GetPsMode(bitRate);
    }
#endif

    COUNT_sub_end();

    return 1 ;
  }

  COUNT_sub_end();

  return 0 ;
}


/*****************************************************************************

 description:  initializes the SBR confifuration
 returns:      error status

*****************************************************************************/
unsigned int
InitializeSbrDefaults (sbrConfigurationPtr config

                       )
{
  COUNT_sub_start("InitializeSbrDefaults");






  INDIRECT(5); MOVE(5);
  config->SendHeaderDataTime     = 500;
  config->crcSbr                 = 0;
  config->tran_thr               = 13000;
  config->detectMissingHarmonics = 1;
  config->parametricCoding       = 1;
  config->useSpeechConfig        = 0;



  INDIRECT(10); MOVE(10);
  config->sbr_data_extra         = 0;
  config->amp_res                = SBR_AMP_RES_3_0 ;
  config->tran_fc                = 0 ;
  config->tran_det_mode          = 1 ;
  config->spread                 = 1 ;
  config->stat                   = 0 ;
  config->e                      = 1 ;
  config->deltaTAcrossFrames     = 1 ;
  config->dF_edge_1stEnv         = 0.3f ;
  config->dF_edge_incr           = 0.3f ;

  INDIRECT(4); MOVE(4);
  config->sbr_invf_mode   = INVF_SWITCHED;
  config->sbr_xpos_mode   = XPOS_LC;
  config->sbr_xpos_ctrl   = SBR_XPOS_CTRL_DEFAULT;
  config->sbr_xpos_level  = 0;

  INDIRECT(2); MOVE(2);
  config->usePs           = 0;
  config->psMode          = -1;

  INDIRECT(5); MOVE(5);
  config->stereoMode             = SBR_SWITCH_LRC;
  config->ana_max_level          = 6;
  config->noiseFloorOffset       = 0;
  config->startFreq              = 5;
  config->stopFreq               = 9;

  INDIRECT(3); MOVE(3);
  config->freqScale       = SBR_FREQ_SCALE_DEFAULT;
  config->alterScale      = SBR_ALTER_SCALE_DEFAULT;
  config->sbr_noise_bands = SBR_NOISE_BANDS_DEFAULT;

  INDIRECT(4); MOVE(4);
  config->sbr_limiter_bands    = SBR_LIMITER_BANDS_DEFAULT;
  config->sbr_limiter_gains    = SBR_LIMITER_GAINS_DEFAULT;
  config->sbr_interpol_freq    = SBR_INTERPOL_FREQ_DEFAULT;
  config->sbr_smoothing_length = SBR_SMOOTHING_LENGTH_DEFAULT;

  COUNT_sub_end();

  return 1;
}


/*****************************************************************************

 description:  frees memory of one SBR channel
 returns:      void

*****************************************************************************/
static void
deleteEnvChannel (HANDLE_ENV_CHANNEL hEnvCut)
{

  COUNT_sub_start("deleteEnvChannel");

  BRANCH(1);
  if (hEnvCut) {

    INDIRECT(1); PTR_INIT(1); FUNC(1);
    deleteFrameInfoGenerator (&hEnvCut->SbrEnvFrame);

    INDIRECT(1); PTR_INIT(1); FUNC(1);
    deleteQmfBank (&hEnvCut->sbrQmf);

    INDIRECT(1); PTR_INIT(1); FUNC(1);
    deleteSbrCodeEnvelope (&hEnvCut->sbrCodeEnvelope);

    INDIRECT(1); PTR_INIT(1); FUNC(1);
    deleteSbrCodeEnvelope (&hEnvCut->sbrCodeNoiseFloor);


    INDIRECT(1); PTR_INIT(1); FUNC(1);
    deleteSbrTransientDetector (&hEnvCut->sbrTransientDetector);

    INDIRECT(1); PTR_INIT(1); FUNC(1);
    deleteExtractSbrEnvelope (&hEnvCut->sbrExtractEnvelope);


    INDIRECT(1); PTR_INIT(1); FUNC(1);
    DeleteTonCorrParamExtr(&hEnvCut->TonCorr);

  }

  COUNT_sub_end();
}


/*****************************************************************************

 description:  close the envelope coding handle
 returns:      void

*****************************************************************************/
void
EnvClose (HANDLE_SBR_ENCODER hEnvEnc)
{
  int i;

  COUNT_sub_start("EnvClose");

  BRANCH(1);
  if (hEnvEnc != NULL) {

    PTR_INIT(1); /* hEnvEnc->hEnvChannel[] */
    LOOP(1);
    for (i = 0; i < MAX_CHANNELS; i++) {

      BRANCH(1);
      if (hEnvEnc->hEnvChannel[i] != NULL) {

        FUNC(1);
        deleteEnvChannel (hEnvEnc->hEnvChannel[i]);

        PTR_INIT(1);
        hEnvEnc->hEnvChannel[i] = NULL;
      }
    }

    
#ifndef MONO_ONLY
    INDIRECT(1); BRANCH(1);
    if (hEnvEnc->hSynthesisQmfBank)
    {
      PTR_INIT(1); FUNC(1);
      DeleteSynthesisQmfBank ((HANDLE_SBR_QMF_FILTER_BANK*)&hEnvEnc->hSynthesisQmfBank);
    }

    INDIRECT(1); BRANCH(1);
    if (hEnvEnc->hPsEnc)
    {
      PTR_INIT(1); FUNC(1);
      DeletePsEnc(&hEnvEnc->hPsEnc);
    }
#endif /* #ifndef MONO_ONLY */

  }

  COUNT_sub_end();
}

/*****************************************************************************

 description:  updates vk_master
 returns:      void

*****************************************************************************/
static int updateFreqBandTable(HANDLE_SBR_CONFIG_DATA  sbrConfigData,
                               HANDLE_SBR_HEADER_DATA  sbrHeaderData,
                               int noQmfChannels)
{


  int k0, k2;

  COUNT_sub_start("updateFreqBandTable");

  INDIRECT(4); PTR_INIT(2); FUNC(7); BRANCH(1);
  if(FindStartAndStopBand(sbrConfigData->sampleFreq,
                          noQmfChannels,
                          sbrHeaderData->sbr_start_frequency,
                          sbrHeaderData->sbr_stop_frequency,
                          sbrHeaderData->sampleRateMode,
                          &k0, &k2))
  {
    COUNT_sub_end();
    return(1);
  }



  INDIRECT(4); PTR_INIT(1); FUNC(6); BRANCH(1);
  if(UpdateFreqScale(sbrConfigData->v_k_master, &sbrConfigData->num_Master,
                     k0, k2, sbrHeaderData->freqScale,
                     sbrHeaderData->alterScale))
  {
    COUNT_sub_end();
    return(1);
  }


  INDIRECT(1); MOVE(1);
  sbrHeaderData->sbr_xover_band=0;


  INDIRECT(6); PTR_INIT(2); FUNC(7); BRANCH(1);
  if(UpdateHiRes(sbrConfigData->freqBandTable[HI],
                 &sbrConfigData->nSfb[HI],
                 sbrConfigData->v_k_master,
                 sbrConfigData->num_Master ,
                 &sbrHeaderData->sbr_xover_band,
                 sbrHeaderData->sampleRateMode,
                 noQmfChannels))
  {
    COUNT_sub_end();
    return(1);
  }


  INDIRECT(4); PTR_INIT(1); FUNC(4);
  UpdateLoRes(sbrConfigData->freqBandTable[LO],
              &sbrConfigData->nSfb[LO],
              sbrConfigData->freqBandTable[HI],
              sbrConfigData->nSfb[HI]);

  INDIRECT(3); MULT(1); DIV(1); ADD(1); SHIFT(1); STORE(1);
  sbrConfigData->xOverFreq = (sbrConfigData->freqBandTable[LOW_RES][0] * sbrConfigData->sampleFreq / noQmfChannels+1)>>1;


  COUNT_sub_end();

  return (0);
}

/*****************************************************************************

 description: performs the sbr envelope calculation
 returns:

*****************************************************************************/
int
EnvEncodeFrame (HANDLE_SBR_ENCODER hEnvEncoder,
                float *samples,
                float *pCoreBuffer,
                unsigned int timeInStride,
                unsigned int  *numAncBytes,
                unsigned char *ancData)
{
  COUNT_sub_start("EnvEncodeFrame");

  BRANCH(1);
  if (hEnvEncoder != NULL) {
    /* header bitstream handling */

    HANDLE_SBR_BITSTREAM_DATA sbrBitstreamData = &hEnvEncoder->sbrBitstreamData;

    PTR_INIT(1); /* counting previous operation */

    INDIRECT(1); MOVE(1);
    sbrBitstreamData->HeaderActive = 0;

    INDIRECT(1); BRANCH(1);
    if (sbrBitstreamData->CountSendHeaderData == 0)
    {
      INDIRECT(1); MOVE(1);
      sbrBitstreamData->HeaderActive = 1;
    }

    INDIRECT(1); BRANCH(1);
    if (sbrBitstreamData->NrSendHeaderData == 0) {
      INDIRECT(1); MOVE(1);
      sbrBitstreamData->CountSendHeaderData = 1;
    }
    else {
      INDIRECT(1); ADD(1); STORE(1);
      sbrBitstreamData->CountSendHeaderData++;

      INDIRECT(2); DIV(1); STORE(1);
      sbrBitstreamData->CountSendHeaderData %= sbrBitstreamData->NrSendHeaderData;
    }



     INDIRECT(3); PTR_INIT(1); FUNC(4);
     InitSbrBitstream(&hEnvEncoder->CmonData,
                      (unsigned char*) hEnvEncoder->sbrPayload,
                      sizeof(hEnvEncoder->sbrPayload),
                      hEnvEncoder->sbrBitstreamData.CRCActive);


     INDIRECT(7); PTR_INIT(4); FUNC(10);
     extractSbrEnvelope(samples,
                        pCoreBuffer,
                        timeInStride,
                        &hEnvEncoder->sbrConfigData,
                        &hEnvEncoder->sbrHeaderData,
                        &hEnvEncoder->sbrBitstreamData,
                        hEnvEncoder->hEnvChannel,
                        hEnvEncoder->hPsEnc,
                        hEnvEncoder->hSynthesisQmfBank,
                        &hEnvEncoder->CmonData);


    INDIRECT(1); PTR_INIT(1); FUNC(1);
    AssembleSbrBitstream(&hEnvEncoder->CmonData);


    assert(GetBitsAvail(&hEnvEncoder->CmonData.sbrBitbuf) % 8 == 0);


    INDIRECT(2); PTR_INIT(1); FUNC(1); MULT(1); STORE(1);
    hEnvEncoder->sbrPayloadSize = GetBitsAvail(&hEnvEncoder->CmonData.sbrBitbuf) / 8;

    INDIRECT(1); ADD(1); BRANCH(1);
    if(hEnvEncoder->sbrPayloadSize > MAX_PAYLOAD_SIZE)
    {
      MOVE(1);
      hEnvEncoder->sbrPayloadSize=0;
    }

   BRANCH(1);
   if(ancData){

      INDIRECT(1); MOVE(1);
      *numAncBytes = hEnvEncoder->sbrPayloadSize;

      FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(hEnvEncoder->sbrPayloadSize);
      memcpy(ancData,hEnvEncoder->sbrPayload,hEnvEncoder->sbrPayloadSize);
    }
  }

 COUNT_sub_end();
 return (0);
}

/*****************************************************************************

 description:  initializes parameters and allocates memory
 returns:      error status

*****************************************************************************/

static int
createEnvChannel (int chan,
                  HANDLE_SBR_CONFIG_DATA sbrConfigData,
                  HANDLE_SBR_HEADER_DATA sbrHeaderData,
                  HANDLE_ENV_CHANNEL     hEnv,
                  sbrConfigurationPtr params)
{

  int e;
  int tran_fc;

  int startIndex;
  int noiseBands[2] = { 3, 3 };

  COUNT_sub_start("createEnvChannel");

  MOVE(2); /* counting previous operations */

  INDIRECT(1); SHIFT(1);
  e = 1 << params->e;

  INDIRECT(1); MOVE(1);
  hEnv->encEnvData.freq_res_fixfix = FREQ_RES_HIGH;

  INDIRECT(4); MOVE(2);
  hEnv->encEnvData.sbr_xpos_mode = (XPOS_MODE)params->sbr_xpos_mode;
  hEnv->encEnvData.sbr_xpos_ctrl = params->sbr_xpos_ctrl;


  INDIRECT(1); PTR_INIT(1); FUNC(2); BRANCH(1);
  if(createQmfBank (chan, &hEnv->sbrQmf)){

    COUNT_sub_end();
    return (1); /* initialisation failed */
  }

  INDIRECT(2); ADD(1);
  startIndex = 576;


  INDIRECT(14); PTR_INIT(1); FUNC(16); BRANCH(1);
  if(CreateTonCorrParamExtr (chan,
                             &hEnv->TonCorr,



                             sbrConfigData->sampleFreq,
                             sbrConfigData->freqBandTable[LOW_RES][sbrConfigData->nSfb[LO]],
                             64,
                             params->sbr_xpos_ctrl,
                             sbrConfigData->freqBandTable[LOW_RES][0],
                             0,
                             sbrConfigData->v_k_master,
                             sbrConfigData->num_Master,
                             params->ana_max_level,
                             sbrConfigData->freqBandTable,
                             sbrConfigData->nSfb,
                             sbrHeaderData->sbr_noise_bands,
                             params->noiseFloorOffset,
                             params->useSpeechConfig))
  {
    COUNT_sub_end();
    return(1);
  }

  INDIRECT(2); MOVE(1);
  hEnv->encEnvData.noOfnoisebands = hEnv->TonCorr.sbrNoiseFloorEstimate.noNoiseBands;

  INDIRECT(1); MOVE(2);
  noiseBands[0] = hEnv->encEnvData.noOfnoisebands;
  noiseBands[1] = hEnv->encEnvData.noOfnoisebands;

  INDIRECT(2); MOVE(1);
  hEnv->encEnvData.sbr_invf_mode = (INVF_MODE)params->sbr_invf_mode;

  ADD(1); BRANCH(1);
  if (hEnv->encEnvData.sbr_invf_mode == INVF_SWITCHED) {

    INDIRECT(1); MOVE(2);
    hEnv->encEnvData.sbr_invf_mode = INVF_MID_LEVEL;
    hEnv->TonCorr.switchInverseFilt = TRUE;
  }
  else {

    INDIRECT(1); MOVE(1);
    hEnv->TonCorr.switchInverseFilt = FALSE;
  }


  INDIRECT(1); MOVE(1);
  tran_fc  = params->tran_fc;

  BRANCH(1);
  if (tran_fc == 0)
  {
    INDIRECT(2); FUNC(3); ADD(1); BRANCH(1); MOVE(1);
    tran_fc = min (5000, getSbrStartFreqRAW (sbrHeaderData->sbr_start_frequency,64,sbrConfigData->sampleFreq));
  }


  INDIRECT(2); MULT(2); DIV(1); ADD(1); SHIFT(1);
  tran_fc = (tran_fc*4*64/sbrConfigData->sampleFreq + 1)>>1;


  INDIRECT(1); PTR_INIT(1); FUNC(3); BRANCH(1);
  if(CreateExtractSbrEnvelope (chan,
                               &hEnv->sbrExtractEnvelope,

                               startIndex

                               ))
  {
    COUNT_sub_end();
    return(1);
  }

  INDIRECT(5); PTR_INIT(1); FUNC(5); BRANCH(1);
  if(CreateSbrCodeEnvelope (&hEnv->sbrCodeEnvelope,
                            sbrConfigData->nSfb,
                            params->deltaTAcrossFrames,
                            params->dF_edge_1stEnv,
                            params->dF_edge_incr))
  {
    COUNT_sub_end();
    return(1);
  }

  INDIRECT(2); PTR_INIT(1); FUNC(5); BRANCH(1);
  if(CreateSbrCodeEnvelope (&hEnv->sbrCodeNoiseFloor,
                            noiseBands,
                            params->deltaTAcrossFrames,
                            0, 0))
  {
    COUNT_sub_end();
    return(1);
  }

  INDIRECT(4); PTR_INIT(3); FUNC(4); BRANCH(1);
  if(InitSbrHuffmanTables (&hEnv->encEnvData,
                           &hEnv->sbrCodeEnvelope,
                           &hEnv->sbrCodeNoiseFloor,
                           sbrHeaderData->sbr_amp_res))
  {
   COUNT_sub_end();
   return(1);
  }

  INDIRECT(4); PTR_INIT(1); FUNC(5);
  CreateFrameInfoGenerator (&hEnv->SbrEnvFrame,
                            params->spread,
                            e,
                            params->stat,

                            hEnv->encEnvData.freq_res_fixfix);

  INDIRECT(7); PTR_INIT(1); FUNC(9); BRANCH(1);
  if(CreateSbrTransientDetector (chan,
                                 &hEnv->sbrTransientDetector,

                                 sbrConfigData->sampleFreq,
                                 params->codecSettings.standardBitrate *
                                 params->codecSettings.nChannels,
                                 params->codecSettings.bitRate,
                                 params->tran_thr,
                                 params->tran_det_mode,
                                 tran_fc


                                 ))
  {
    COUNT_sub_end();
    return(1);
  }

  INDIRECT(2); MOVE(1);
  sbrConfigData->xposCtrlSwitch = params->sbr_xpos_ctrl;


    INDIRECT(5); MOVE(3);
    hEnv->encEnvData.noHarmonics = sbrConfigData->nSfb[HI];
    hEnv->encEnvData.syntheticCoding = sbrConfigData->detectMissingHarmonics;
    hEnv->encEnvData.addHarmonicFlag = 0;

  COUNT_sub_end();

  return (0);
}


/*****************************************************************************

 description:  initializes parameters and allocates memory
 returns:      error status

*****************************************************************************/
int
EnvOpen (HANDLE_SBR_ENCODER * hEnvEncoder,
         float *pCoreBuffer,
         sbrConfigurationPtr params,
         int      *coreBandWith)
{
  HANDLE_SBR_ENCODER hEnvEnc ;
  int ch;

  COUNT_sub_start("EnvOpen");

  MOVE(1);
  *hEnvEncoder=0;

  PTR_INIT(1);
  hEnvEnc = &sbrEncoder;

  PTR_INIT(2); /* hEnvEnc->hEnvChannel[]
                  EnvChannel[]
               */
  LOOP(1);
  for (ch=0; ch<MAX_CHANNELS; ch++)
  {
    PTR_INIT(1);
    hEnvEnc->hEnvChannel[ch] = &EnvChannel[ch];
  }

  INDIRECT(2); ADD(2); LOGIC(1); BRANCH(1);
  if ((params->codecSettings.nChannels < 1) || (params->codecSettings.nChannels > MAX_CHANNELS)){

    FUNC(1);
    EnvClose (hEnvEnc);

    COUNT_sub_end();
    return(1);
  }


  INDIRECT(1); PTR_INIT(1);
  hEnvEnc->sbrConfigData.freqBandTable[LO] =  sbr_freqBandTableLO;

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(MAX_FREQ_COEFFS/2+1);
  memset(hEnvEnc->sbrConfigData.freqBandTable[LO],0,sizeof(unsigned char)*MAX_FREQ_COEFFS/2+1);

  INDIRECT(1); PTR_INIT(1);
  hEnvEnc->sbrConfigData.freqBandTable[HI] =  sbr_freqBandTableHI;

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(MAX_FREQ_COEFFS+1);
  memset(hEnvEnc->sbrConfigData.freqBandTable[HI],0,sizeof(unsigned char)*MAX_FREQ_COEFFS+1);

  INDIRECT(1); PTR_INIT(1);
  hEnvEnc->sbrConfigData.v_k_master =  sbr_v_k_master;

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(MAX_FREQ_COEFFS+1);
  memset(hEnvEnc->sbrConfigData.v_k_master,0,sizeof(unsigned char)*MAX_FREQ_COEFFS+1);

  INDIRECT(1); BRANCH(1);
  if (hEnvEnc->CmonData.sbrBitbuf.isValid == 0) {

    INDIRECT(2); PTR_INIT(1); FUNC(3);
    CreateBitBuffer(&hEnvEnc->CmonData.sbrBitbuf,
                    (unsigned char*) hEnvEnc->sbrPayload,
                    sizeof(hEnvEnc->sbrPayload));
  }

  INDIRECT(1); BRANCH(1);
  if (hEnvEnc->CmonData.sbrBitbufPrev.isValid == 0) {

    INDIRECT(2); PTR_INIT(1); FUNC(3);
    CreateBitBuffer(&hEnvEnc->CmonData.sbrBitbufPrev,
                    (unsigned char*) hEnvEnc->sbrPayloadPrevious,
                    sizeof(hEnvEnc->sbrPayload));
  }

  INDIRECT(2); MOVE(1);
  hEnvEnc->sbrConfigData.nChannels = params->codecSettings.nChannels;

  INDIRECT(1); ADD(1); BRANCH(1);
  if(params->codecSettings.nChannels == 2)
  {
     INDIRECT(2); MOVE(1);
     hEnvEnc->sbrConfigData.stereoMode = params->stereoMode;
  }
  else
  {
     INDIRECT(1); MOVE(1);
     hEnvEnc->sbrConfigData.stereoMode = SBR_MONO;
  }






  INDIRECT(1); ADD(1); BRANCH(1);
  if (params->codecSettings.sampleFreq <= 24000) {

    INDIRECT(1); MOVE(1);
    hEnvEnc->sbrHeaderData.sampleRateMode = DUAL_RATE;

    INDIRECT(1); MULT(1); STORE(1);
    hEnvEnc->sbrConfigData.sampleFreq = 2 * params->codecSettings.sampleFreq;
  }
  else {

    INDIRECT(2); MOVE(2);
    hEnvEnc->sbrHeaderData.sampleRateMode = SINGLE_RATE;
    hEnvEnc->sbrConfigData.sampleFreq = params->codecSettings.sampleFreq;
  }


  INDIRECT(1); MOVE(1);
  hEnvEnc->sbrBitstreamData.CountSendHeaderData = 0;

  INDIRECT(1); BRANCH(1);
  if (params->SendHeaderDataTime > 0 ) {

    INDIRECT(2); MULT(2); DIV(1); STORE(1);
    hEnvEnc->sbrBitstreamData.NrSendHeaderData = (int)(params->SendHeaderDataTime * 0.001*
                                             hEnvEnc->sbrConfigData.sampleFreq / 2048
                                             );

    ADD(1); BRANCH(1); MOVE(1);
    hEnvEnc->sbrBitstreamData.NrSendHeaderData = max(hEnvEnc->sbrBitstreamData.NrSendHeaderData,1);
  }
  else {

   INDIRECT(1); MOVE(1);
   hEnvEnc->sbrBitstreamData.NrSendHeaderData = 0;
  }

  INDIRECT(10); MOVE(6);
  hEnvEnc->sbrHeaderData.sbr_data_extra = params->sbr_data_extra;
  hEnvEnc->sbrBitstreamData.CRCActive = params->crcSbr;
  hEnvEnc->sbrBitstreamData.HeaderActive = 0;
  hEnvEnc->sbrHeaderData.sbr_start_frequency = params->startFreq;
  hEnvEnc->sbrHeaderData.sbr_stop_frequency  = params->stopFreq;
  hEnvEnc->sbrHeaderData.sbr_xover_band = 0;

    INDIRECT(1); ADD(1); BRANCH(1);
    if (params->sbr_xpos_ctrl!= SBR_XPOS_CTRL_DEFAULT)
    {
       INDIRECT(1); MOVE(1);
       hEnvEnc->sbrHeaderData.sbr_data_extra = 1;
    }

   INDIRECT(1); MOVE(1);
   hEnvEnc->sbrHeaderData.protocol_version = SI_SBR_PROTOCOL_VERSION_ID;

   INDIRECT(2); MOVE(2);
   hEnvEnc->sbrHeaderData.sbr_amp_res = (AMP_RES)params->amp_res;



  INDIRECT(7); MOVE(4);
  hEnvEnc->sbrHeaderData.freqScale  = params->freqScale;
  hEnvEnc->sbrHeaderData.alterScale = params->alterScale;
  hEnvEnc->sbrHeaderData.sbr_noise_bands = params->sbr_noise_bands;
  hEnvEnc->sbrHeaderData.header_extra_1 = 0;

  ADD(3); LOGIC(2); BRANCH(1);
  if ((params->freqScale != SBR_FREQ_SCALE_DEFAULT) ||
      (params->alterScale != SBR_ALTER_SCALE_DEFAULT) ||
      (params->sbr_noise_bands != SBR_NOISE_BANDS_DEFAULT)) {

   MOVE(1);
   hEnvEnc->sbrHeaderData.header_extra_1 = 1;
  }

  INDIRECT(9); MOVE(5);
  hEnvEnc->sbrHeaderData.sbr_limiter_bands = params->sbr_limiter_bands;
  hEnvEnc->sbrHeaderData.sbr_limiter_gains = params->sbr_limiter_gains;
  hEnvEnc->sbrHeaderData.sbr_interpol_freq = params->sbr_interpol_freq;
  hEnvEnc->sbrHeaderData.sbr_smoothing_length = params->sbr_smoothing_length;
  hEnvEnc->sbrHeaderData.header_extra_2 = 0;

  ADD(4); LOGIC(3); BRANCH(1);
  if ((params->sbr_limiter_bands != SBR_LIMITER_BANDS_DEFAULT) ||
      (params->sbr_limiter_gains != SBR_LIMITER_GAINS_DEFAULT) ||
      (params->sbr_interpol_freq != SBR_INTERPOL_FREQ_DEFAULT) ||
      (params->sbr_smoothing_length != SBR_SMOOTHING_LENGTH_DEFAULT)) {

     MOVE(1);
     hEnvEnc->sbrHeaderData.header_extra_2 = 1;
  }

  INDIRECT(4); MOVE(2);
  hEnvEnc->sbrConfigData.detectMissingHarmonics    = params->detectMissingHarmonics;
  hEnvEnc->sbrConfigData.useParametricCoding       = params->parametricCoding;


  INDIRECT(2); PTR_INIT(2); FUNC(3); BRANCH(1);
  if(updateFreqBandTable(&hEnvEnc->sbrConfigData,
                         &hEnvEnc->sbrHeaderData,
                         QMF_CHANNELS)){
    FUNC(1);
    EnvClose (hEnvEnc);

    COUNT_sub_end();
    return(1);
  }


  PTR_INIT(1); /* hEnvEnc->hEnvChannel[] */
  INDIRECT(3); PTR_INIT(2); LOOP(1);
  for ( ch = 0; ch < hEnvEnc->sbrConfigData.nChannels; ch++ ) {

    FUNC(5); BRANCH(1);
    if(createEnvChannel(ch,
                        &hEnvEnc->sbrConfigData,
                        &hEnvEnc->sbrHeaderData,
                        hEnvEnc->hEnvChannel[ch],
                        params)){

      FUNC(1);
      EnvClose (hEnvEnc);

      COUNT_sub_end();
      return(1);
    }
  }


  INDIRECT(1); PTR_INIT(1);
  hEnvEnc->hPsEnc = NULL;


#ifndef MONO_ONLY

  INDIRECT(1); BRANCH(1);
  if (params->usePs)
  {
    INDIRECT(2); PTR_INIT(1); FUNC(2); BRANCH(1);
    if(createQmfBank (1, &hEnvEnc->hEnvChannel[1]->sbrQmf)){
      COUNT_sub_end();
      return (1);
    }
    INDIRECT(1); PTR_INIT(1); FUNC(3); BRANCH(1);
    if(CreateExtractSbrEnvelope (1,
                                 &hEnvEnc->hEnvChannel[1]->sbrExtractEnvelope,
                                 576
                                 )) {
      COUNT_sub_end();
      return(1);
    }

    INDIRECT(1); MOVE(1);
    hEnvEnc->hSynthesisQmfBank = &SynthesisQmfBank;

    INDIRECT(1); PTR_INIT(1); FUNC(1); BRANCH(1);
    if(CreateSynthesisQmfBank (hEnvEnc->hSynthesisQmfBank)){

      FUNC(1);
      DeleteSynthesisQmfBank ((HANDLE_SBR_QMF_FILTER_BANK*)&hEnvEnc->hSynthesisQmfBank);

      COUNT_sub_end();
      return 1;
    }

    INDIRECT(1); MOVE(1);
    hEnvEnc->hPsEnc = &psEncoder;

    INDIRECT(2); PTR_INIT(1); FUNC(2); BRANCH(1);
    if(CreatePsEnc (hEnvEnc->hPsEnc,
                    params->psMode)){

      FUNC(1);
      DeletePsEnc(&hEnvEnc->hPsEnc);

      COUNT_sub_end();
      return 1;
    }
  }
#endif /* #ifndef MONO_ONLY */

  INDIRECT(2); MOVE(1);
  hEnvEnc->CmonData.sbrNumChannels  = hEnvEnc->sbrConfigData.nChannels;

  INDIRECT(1); MOVE(1);
  hEnvEnc->sbrPayloadSize = 0;

  INDIRECT(1); MOVE(2);
  *hEnvEncoder = hEnvEnc;
  *coreBandWith = hEnvEnc->sbrConfigData.xOverFreq;

  COUNT_sub_end();

  return 0;
}

