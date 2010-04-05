/*
  Memory layout
*/
#ifndef __SBR_RAM_H
#define __SBR_RAM_H


extern float *PsBuf2;
extern float PsBuf3[];
extern float *PsBuf4;
extern float *PsBuf5;

extern  float sbr_envYBuffer[];
extern  float sbr_QmfStatesAnalysis[];
extern  float sbr_envRBuffer[];
extern  float sbr_envIBuffer[];
extern  float sbr_quotaMatrix[];
extern  float sbr_thresholds[];
extern  float sbr_transients[];
extern  unsigned char sbr_freqBandTableLO[];
extern  unsigned char sbr_freqBandTableHI[];
extern  unsigned char sbr_v_k_master[];

extern unsigned char  sbr_detectionVectors[];
extern char           sbr_prevEnvelopeCompensation[];
extern unsigned char  sbr_guideScfb[];
extern unsigned char  sbr_guideVectorDetected[];
extern float          sbr_toncorrBuff[];

#endif

