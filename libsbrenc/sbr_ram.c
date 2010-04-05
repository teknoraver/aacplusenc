/*
  Memory layout
  This module declares all static and dynamic memory spaces
*/
#include <stdio.h>
#include <assert.h>
#include "aacenc.h"
#include "sbr_ram.h"
#include "sbr_def.h"
#include "sbr_main.h"
#include "aac_ram.h"
#include "hybrid.h"
#include "FloatFR.h"

#define PS_BUF4_SIZE (4*(QMF_TIME_SLOTS + QMF_BUFFER_MOVE) + 4*(NO_QMF_BANDS_IN_HYBRID + NO_QMF_BANDS_IN_HYBRID*QMF_BUFFER_MOVE))
#define PS_BUF5_SIZE (QMF_FILTER_LENGTH/2 + QMF_CHANNELS)

/* Overlay with mdctDelayBuffer of 2nd channel since AAC only works in mono */
float *PsBuf2= &mdctDelayBuffer[BLOCK_SWITCHING_OFFSET];

/* Overlay PsBuf4 and PsBuf5 with sbr_toncorrBuff of 2nd channel, since SBR only works in mono */
float *PsBuf4 = &sbr_toncorrBuff[5*NO_OF_ESTIMATES*MAX_FREQ_COEFFS];
float *PsBuf5 = &sbr_toncorrBuff[5*NO_OF_ESTIMATES*MAX_FREQ_COEFFS + PS_BUF4_SIZE];

float PsBuf3[MAX_CHANNELS*FRAME_LEN_LONG*sizeof(short)/sizeof(float)];

/*!
  \name StaticSbrData

  Static memory areas, must not be overwritten in other sections of the encoder
*/

/*! Filter states for QMF-analysis. <br>
  Dimension: #MAXNRSBRCHANNELS * #SBR_QMF_FILTER_LENGTH */
float sbr_QmfStatesAnalysis[MAX_CHANNELS  * QMF_FILTER_LENGTH];

/*! Energy buffer for envelope extraction <br>
  Dimension #MAXNRSBRCHANNELS * +#SBR_QMF_SLOTS*2 *  #SBR_QMF_CHANNELS
*/

float sbr_envYBuffer[MAX_CHANNELS  * QMF_TIME_SLOTS * QMF_CHANNELS];

/*! Matrix holding the quota values for all estimates, all channels
  Dimension #MAXNRSBRCHANNELS * +#SBR_QMF_CHANNELS* #NO_OF_ESTIMATES
*/

float sbr_quotaMatrix[MAX_CHANNELS  * NO_OF_ESTIMATES*QMF_CHANNELS];


/*! Thresholds for transient detection <br>
  Dimension #MAXNRSBRCHANNELS * #SBR_QMF_CHANNELS
*/
float sbr_thresholds[MAX_CHANNELS *QMF_CHANNELS];





/*! Frequency band table (low res) <br>
  Dimension #MAX_FREQ_COEFFS/2+1
*/

unsigned char    sbr_freqBandTableLO[MAX_FREQ_COEFFS/2+1];

/*! Frequency band table (high res) <br>
  Dimension #MAX_FREQ_COEFFS +1
*/

unsigned char    sbr_freqBandTableHI[MAX_FREQ_COEFFS+1];

/*! vk matser table <br>
  Dimension #MAX_FREQ_COEFFS +1
*/


unsigned char    sbr_v_k_master[MAX_FREQ_COEFFS +1];



/*
  Missing harmonics detection
*/

/*! sbr_detectionVectors <br>
  Dimension #MAX_CHANNELS*#NO_OF_ESTIMATES*#MAX_FREQ_COEFFS]
*/
unsigned char   sbr_detectionVectors[MAX_CHANNELS*NO_OF_ESTIMATES*MAX_FREQ_COEFFS];

/*!
  The following tonality correclation buffers are allocated in
  one non-reusable buffer
  sbr_tonalityDiff <br>
  Dimension #MAX_CHANNELS*#NO_OF_ESTIMATES*#MAX_FREQ_COEFFS]
  sbr_sfmOrig <br>
  Dimension #MAX_CHANNELS*#NO_OF_ESTIMATES*#MAX_FREQ_COEFFS]
  sbr_sfmSbr <br>
  Dimension #MAX_CHANNELS*#NO_OF_ESTIMATES*#MAX_FREQ_COEFFS]
  sbr_guideVectorDiff <br>
  Dimension #MAX_CHANNELS*#NO_OF_ESTIMATES*#MAX_FREQ_COEFFS]
  sbr_guideVectorOrig <br>
  Dimension #MAX_CHANNELS*#NO_OF_ESTIMATES*#MAX_FREQ_COEFFS]
*/

/* To overlay 2nd half of sbr_toncorrBuff with PS-Buffers, the 2nd half
   must not fall below a minium size */
float  sbr_toncorrBuff[max( /* two channels or... */
                           (MAX_CHANNELS*5*NO_OF_ESTIMATES*MAX_FREQ_COEFFS),
                           (
                            /* 1st half */
                            (5*NO_OF_ESTIMATES*MAX_FREQ_COEFFS)+
                            PS_BUF4_SIZE + PS_BUF5_SIZE
                            )
                           )];

/*! sbr_prevCompVec[ <br>
  Dimension #MAX_CHANNELS*#MAX_FREQ_COEFFS]
*/
char     sbr_prevEnvelopeCompensation[MAX_CHANNELS*MAX_FREQ_COEFFS];
/*! sbr_guideScfb[ <br>
  Dimension #MAX_CHANNELS*#MAX_FREQ_COEFFS]
*/
unsigned char  sbr_guideScfb[MAX_CHANNELS*MAX_FREQ_COEFFS];
/*! sbr_guideVectorDiff <br>
  Dimension #MAX_CHANNELS*#NO_OF_ESTIMATES*#MAX_FREQ_COEFFS]
*/
/*! sbr_guideVectorDetected <br>
  Dimension #MAX_CHANNELS*#NO_OF_ESTIMATES*#MAX_FREQ_COEFFS]
*/
unsigned char   sbr_guideVectorDetected[MAX_CHANNELS*NO_OF_ESTIMATES*MAX_FREQ_COEFFS];



/*!
  \name DynamicSbrData

  Dynamic memory areas, might be reused in other algorithm sections,
  e.g. the core decoder
*/


/*! Real buffer for envelope extraction <br>
  Dimension #SBR_QMF_SLOTS *  #SBR_QMF_CHANNELS
*/


float  sbr_envRBuffer [MAX_CHANNELS * QMF_TIME_SLOTS * QMF_CHANNELS];

/*! Imag buffer for envelope extraction <br>
  Dimension #SBR_QMF_SLOTS *  #SBR_QMF_CHANNELS
*/

float  sbr_envIBuffer [MAX_CHANNELS * QMF_TIME_SLOTS * QMF_CHANNELS];


/*! Transients for transient detection <br>
  Dimension MAX_CHANNELS*3* #QMF_TIME_SLOTS
*/
float sbr_transients[MAX_CHANNELS*3*QMF_TIME_SLOTS];

