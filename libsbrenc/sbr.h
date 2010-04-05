/*
  Main SBR structs definitions
*/

#ifndef __SBR_H
#define __SBR_H

#include "qmf_enc.h"
#include "tran_det.h"
#include "fram_gen.h"
#include "nf_est.h"
#include "mh_det.h"
#include "invf_est.h"
#include "env_est.h"
#include "code_env.h"
#include "sbr_main.h"
#include "ton_corr.h"

struct SBR_BITSTREAM_DATA
{
  int TotalBits;
  int PayloadBits;
  int FillBits;
  int HeaderActive;
  int CRCActive;
  int NrSendHeaderData;
  int CountSendHeaderData;

};

typedef struct SBR_BITSTREAM_DATA  *HANDLE_SBR_BITSTREAM_DATA;

struct SBR_HEADER_DATA
{

  int protocol_version;
  AMP_RES sbr_amp_res;
  int sbr_start_frequency;
  int sbr_stop_frequency;
  int sbr_xover_band;
  int sbr_noise_bands;
  int sbr_data_extra;
  int header_extra_1;
  int header_extra_2;
  int sbr_limiter_bands;
  int sbr_limiter_gains;
  int sbr_interpol_freq;
  int sbr_smoothing_length;
  int alterScale;
  int freqScale;

  SR_MODE sampleRateMode;

  int coupling;
  int prev_coupling;

};

typedef struct SBR_HEADER_DATA *HANDLE_SBR_HEADER_DATA;

struct SBR_CONFIG_DATA
{
  int nChannels;

  int nSfb[2];
  int num_Master;
  int sampleFreq;

  int xOverFreq;

  unsigned char *freqBandTable[2];
  unsigned char *v_k_master;


  SBR_STEREO_MODE stereoMode;

  int detectMissingHarmonics;
  int useParametricCoding;
  int xposCtrlSwitch;
};

typedef struct SBR_CONFIG_DATA *HANDLE_SBR_CONFIG_DATA;

struct SBR_ENV_DATA
{

  int sbr_xpos_ctrl;
  FREQ_RES freq_res_fixfix;


  INVF_MODE sbr_invf_mode;
  INVF_MODE sbr_invf_mode_vec[MAX_NUM_NOISE_VALUES];

  XPOS_MODE sbr_xpos_mode;

  int ienvelope[MAX_ENVELOPES][MAX_FREQ_COEFFS];

  int codeBookScfLavBalance;
  int codeBookScfLav;
  const int *hufftableTimeC;
  const int *hufftableFreqC;
  const unsigned char *hufftableTimeL;
  const unsigned char *hufftableFreqL;

  const int *hufftableLevelTimeC;
  const int *hufftableBalanceTimeC;
  const int *hufftableLevelFreqC;
  const int *hufftableBalanceFreqC;
  const unsigned char *hufftableLevelTimeL;
  const unsigned char *hufftableBalanceTimeL;
  const unsigned char *hufftableLevelFreqL;
  const unsigned char *hufftableBalanceFreqL;


  const unsigned char *hufftableNoiseTimeL;
  const int *hufftableNoiseTimeC;
  const unsigned char *hufftableNoiseFreqL;
  const int *hufftableNoiseFreqC;

  const unsigned char *hufftableNoiseLevelTimeL;
  const int *hufftableNoiseLevelTimeC;
  const unsigned char *hufftableNoiseBalanceTimeL;
  const int *hufftableNoiseBalanceTimeC;
  const unsigned char *hufftableNoiseLevelFreqL;
  const int *hufftableNoiseLevelFreqC;
  const unsigned char *hufftableNoiseBalanceFreqL;
  const int *hufftableNoiseBalanceFreqC;

  HANDLE_SBR_GRID hSbrBSGrid;


  int syntheticCoding;
  int noHarmonics;
  int addHarmonicFlag;
  unsigned char addHarmonic[MAX_FREQ_COEFFS];

  int si_sbr_start_env_bits_balance;
  int si_sbr_start_env_bits;
  int si_sbr_start_noise_bits_balance;
  int si_sbr_start_noise_bits;

  int noOfEnvelopes;
  int noScfBands[MAX_ENVELOPES];
  int domain_vec[MAX_ENVELOPES];
  int domain_vec_noise[MAX_ENVELOPES];
  int sbr_noise_levels[MAX_NUM_NOISE_VALUES];
  int noOfnoisebands;

  int balance;
  int init_sbr_amp_res;
};


typedef struct SBR_ENV_DATA *HANDLE_SBR_ENV_DATA;

struct ENV_CHANNEL
{

  SBR_TRANSIENT_DETECTOR sbrTransientDetector;
  SBR_CODE_ENVELOPE sbrCodeEnvelope;
  SBR_CODE_ENVELOPE sbrCodeNoiseFloor;
  SBR_EXTRACT_ENVELOPE sbrExtractEnvelope;
  SBR_QMF_FILTER_BANK sbrQmf;
  SBR_ENVELOPE_FRAME SbrEnvFrame;
  SBR_TON_CORR_EST   TonCorr;

  struct SBR_ENV_DATA encEnvData;
};

typedef struct ENV_CHANNEL *HANDLE_ENV_CHANNEL;


#endif /* __SBR_H */
