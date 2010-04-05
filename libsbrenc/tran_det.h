/*
  Transient detector prototypes
*/
#ifndef __TRAN_DET_H
#define __TRAN_DET_H

typedef struct
{
  float *transients;
  float *thresholds;

  float  tran_thr;
  float  split_thr;
  int    tran_fc;
  int    buffer_length;
  int    no_cols;
  int    no_rows;
  int    mode;

  float  prevLowBandEnergy;
  float  totalHighBandEnergy;
}
SBR_TRANSIENT_DETECTOR;


typedef SBR_TRANSIENT_DETECTOR *HANDLE_SBR_TRANSIENT_DETECTOR;

/*************/
/* Functions */
/*************/

void transientDetect (float **Energies,
                      HANDLE_SBR_TRANSIENT_DETECTOR h_sbrTransientDetector,
                      int *tran_vector,
                      int timeStep
                      );

int
CreateSbrTransientDetector (int chan,
                            HANDLE_SBR_TRANSIENT_DETECTOR h_sbrTransientDetector,

                            int   sampleFreq,
                            int   totalBitrate,
                            int   codecBitrate,
                            int   tran_thr,
                            int   mode,
                            int   tran_fc


                            );

void deleteSbrTransientDetector (HANDLE_SBR_TRANSIENT_DETECTOR h_sbrTransientDetector);

void
frameSplitter(float **Energies,
              HANDLE_SBR_TRANSIENT_DETECTOR h_sbrTransientDetector,
              unsigned char * freqBandTable,
              int nSfb,
              int timeStep,
              int no_cols,
              int *tran_vector);

#endif
