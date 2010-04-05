/*
  DPCM Envelope coding
*/

#ifndef __CODE_ENV_H
#define __CODE_ENV_H

#include "sbr_main.h"
#include "sbr_def.h"
#include "fram_gen.h"

typedef struct
{
  int offset;
  int upDate;
  int nSfb[2];
  int sfb_nrg_prev[MAX_FREQ_COEFFS];
  int deltaTAcrossFrames;
  float dF_edge_1stEnv;
  float dF_edge_incr;
  int dF_edge_incr_fac;

  int codeBookScfLavTime;
  int codeBookScfLavFreq;

  int codeBookScfLavLevelTime;
  int codeBookScfLavLevelFreq;
  int codeBookScfLavBalanceTime;
  int codeBookScfLavBalanceFreq;

  int start_bits;
  int start_bits_balance;


  const unsigned char *hufftableTimeL;
  const unsigned char *hufftableFreqL;

  const unsigned char *hufftableLevelTimeL;
  const unsigned char *hufftableBalanceTimeL;
  const unsigned char *hufftableLevelFreqL;
  const unsigned char *hufftableBalanceFreqL;
}
SBR_CODE_ENVELOPE;
typedef SBR_CODE_ENVELOPE *HANDLE_SBR_CODE_ENVELOPE;



void
codeEnvelope (int *sfb_nrg,
              const FREQ_RES *freq_res,
              SBR_CODE_ENVELOPE * h_sbrCodeEnvelope,
              int *directionVec, int coupling, int nEnvelopes, int channel,
              int headerActive);

int
CreateSbrCodeEnvelope (HANDLE_SBR_CODE_ENVELOPE h_sbrCodeEnvelope,
                       int *nSfb,
                       int deltaTAcrossFrames,
                       float dF_edge_1stEnv,
                       float dF_edge_incr);

void deleteSbrCodeEnvelope (HANDLE_SBR_CODE_ENVELOPE h_sbrCodeEnvelope);

struct SBR_ENV_DATA;

int
InitSbrHuffmanTables (struct SBR_ENV_DATA*      sbrEnvData,
                      HANDLE_SBR_CODE_ENVELOPE  henv,
                      HANDLE_SBR_CODE_ENVELOPE  hnoise,
                      AMP_RES                   amp_res);

#endif
