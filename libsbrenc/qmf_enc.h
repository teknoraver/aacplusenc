/*
  QMF analysis structs and prototypes
*/
#ifndef __QMF_ENC_H
#define __QMF_ENC_H

typedef struct
{
  const float *p_filter;

  const float *cos_twiddle;
  const float *sin_twiddle;
  const float *alt_sin_twiddle;

  float *timeBuffer;
  float *workBuffer;

  float *qmf_states_buffer;
}
SBR_QMF_FILTER_BANK;

typedef SBR_QMF_FILTER_BANK *HANDLE_SBR_QMF_FILTER_BANK;


void sbrAnalysisFiltering (const float *timeIn,
                           int   timeInStride,
                           float **rAnalysis,
                           float **iAnalysis,
                           HANDLE_SBR_QMF_FILTER_BANK qmfBank);


int createQmfBank (int chan,HANDLE_SBR_QMF_FILTER_BANK h_sbrQmf);


void deleteQmfBank (HANDLE_SBR_QMF_FILTER_BANK  h_sbrQmf);


void
getEnergyFromCplxQmfData (float **energyValues,
                          float **realValues,
                          float **imagValues

                          );

void
SynthesisQmfFiltering (float **sbrReal,
                       float **sbrImag,
                       float *timeOut,
                       HANDLE_SBR_QMF_FILTER_BANK qmfBank);


int
CreateSynthesisQmfBank (HANDLE_SBR_QMF_FILTER_BANK h_sbrQmf);




void DeleteSynthesisQmfBank (HANDLE_SBR_QMF_FILTER_BANK *h_sbrQmf);

#endif
