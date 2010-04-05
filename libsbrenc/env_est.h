/*
  Envelope estimation structs and prototypes
*/
#ifndef __ENV_EST_H
#define __ENV_EST_H

#include "sbr_def.h"
#include "aacenc.h"
#include "qmf_enc.h"

typedef struct
{
  int pre_transient_info[2];
  float *rBuffer[QMF_TIME_SLOTS];
  float *iBuffer[QMF_TIME_SLOTS];
  float *YBuffer[QMF_TIME_SLOTS*2];

  char envelopeCompensation[MAX_FREQ_COEFFS];

  int YBufferWriteOffset;

  int no_cols;
  int no_rows;
  int start_index;

  int time_slots;
  int time_step;
}
SBR_EXTRACT_ENVELOPE;
typedef SBR_EXTRACT_ENVELOPE *HANDLE_SBR_EXTRACT_ENVELOPE;


/************  Function Declarations ***************/

int
CreateExtractSbrEnvelope (int chan,
                          HANDLE_SBR_EXTRACT_ENVELOPE hSbr,


                          int start_index


                          );

void deleteExtractSbrEnvelope (HANDLE_SBR_EXTRACT_ENVELOPE hSbrCut);



struct SBR_CONFIG_DATA;
struct SBR_HEADER_DATA;
struct SBR_BITSTREAM_DATA;
struct ENV_CHANNEL;
struct COMMON_DATA;
struct PS_ENC;


void
extractSbrEnvelope(float *timeInPtr,
                   float *pCoreBuffer,
                   unsigned int timeInStride,
                   struct SBR_CONFIG_DATA *h_con,
                   struct SBR_HEADER_DATA  *sbrHeaderData,
                   struct SBR_BITSTREAM_DATA *sbrBitstreamData,
                   struct ENV_CHANNEL      *h_envChan[MAX_CHANNELS],
                   struct PS_ENC *hPsEnc,
                   HANDLE_SBR_QMF_FILTER_BANK hSynthesisQmfBank,
                   struct COMMON_DATA      *cmonData);

#endif
