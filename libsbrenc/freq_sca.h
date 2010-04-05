/*
  frequency scale prototypes
*/
#ifndef __FREQ_SCA2_H
#define __FREQ_SCA2_H
#include "sbr_def.h"

#define MAX_OCTAVE        29
#define MAX_SECOND_REGION 50


int
UpdateFreqScale(unsigned char *v_k_master, int *h_num_bands,
                const int k0, const int k2,
                const int freq_scale,
                const int alter_scale);

int
UpdateHiRes(unsigned char *h_hires,
            int *num_hires,
            unsigned char *v_k_master,
            int num_master ,
            int *xover_band,
            SR_MODE drOrSr,
            int noQMFChannels);

void  UpdateLoRes(unsigned char * v_lores,
                  int *num_lores,
                  unsigned char * v_hires,
                  int num_hires);

int
FindStartAndStopBand(const int samplingFreq,
                     const int noChannels,
                     const int startFreq,
                     const int stop_freq,
                     const SR_MODE sampleRateMode,
                     int *k0,
                     int *k2);

int getSbrStartFreqRAW (int startFreq, int QMFbands, int fs );
int getSbrStopFreqRAW  (int stopFreq, int QMFbands, int fs);
#endif
