/*
  SBR encoder top level processing prototypes
*/

#ifndef __SBR_MAIN_H
#define __SBR_MAIN_H

#define MAX_TRANS_FAC         8
#define MAX_CODEC_FRAME_RATIO 2
#define MAX_PAYLOAD_SIZE    256

typedef struct
{
  int bitRate;
  int nChannels;
  int sampleFreq;
  int transFac;
  int standardBitrate;
} CODEC_PARAM;

typedef enum
{
  SBR_MONO,
  SBR_LEFT_RIGHT,
  SBR_COUPLING,
  SBR_SWITCH_LRC
}
SBR_STEREO_MODE;

typedef struct sbrConfiguration
{
  CODEC_PARAM codecSettings;
  int SendHeaderDataTime;
  int crcSbr;
  int detectMissingHarmonics;
  int parametricCoding;


  int tran_thr;
  int noiseFloorOffset;
  unsigned int useSpeechConfig;


  int sbr_data_extra;
  int amp_res;
  int ana_max_level;
  int tran_fc;
  int tran_det_mode;
  int spread;
  int stat;
  int e;
  SBR_STEREO_MODE stereoMode;
  int deltaTAcrossFrames;
  float dF_edge_1stEnv;
  float dF_edge_incr;
  int sbr_invf_mode;
  int sbr_xpos_mode;
  int sbr_xpos_ctrl;
  int sbr_xpos_level;
  int startFreq;
  int stopFreq;

  int usePs;
  int psMode;

  int freqScale;
  int alterScale;
  int sbr_noise_bands;

  int sbr_limiter_bands;
  int sbr_limiter_gains;
  int sbr_interpol_freq;
  int sbr_smoothing_length;

} sbrConfiguration, *sbrConfigurationPtr ;




unsigned int
IsSbrSettingAvail (unsigned int bitrate,
                   unsigned int numOutputChannels,
                   unsigned int sampleRateInput,
                   unsigned int *sampleRateCore);

unsigned int
AdjustSbrSettings (const sbrConfigurationPtr config,
                   unsigned int bitRate,
                   unsigned int numChannels,
                   unsigned int fsCore,
                   unsigned int transFac,
                   unsigned int standardBitrate);

unsigned int
InitializeSbrDefaults (sbrConfigurationPtr config

                       );


typedef struct SBR_ENCODER *HANDLE_SBR_ENCODER;



int
EnvOpen (HANDLE_SBR_ENCODER*      hEnvEncoder,
         float *pCoreBuffer,
         sbrConfigurationPtr      params,
         int                      *coreBandWith
         );

void
EnvClose (HANDLE_SBR_ENCODER hEnvEnc);

int
SbrGetXOverFreq(HANDLE_SBR_ENCODER hEnv,
                int         xoverFreq );

int
SbrGetStopFreqRaw(HANDLE_SBR_ENCODER hEnv);

int
EnvEncodeFrame (HANDLE_SBR_ENCODER hEnvEncoder,
                float *samples,
                float *pCoreBuffer,
                unsigned int  timeInStride,
                unsigned int  *numAncBytes,
                unsigned char *ancData);

#endif /* ifndef __SBR_MAIN_H */
