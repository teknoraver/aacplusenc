/*
  parametric stereo encoding
*/
#ifndef __PS_ENC_H
#define __PS_ENC_H

#include "ps_bitenc.h"
#include "hybrid.h"

/*############################################################################*/
/* Constant definitions                                                       */
/*############################################################################*/

#define NO_BINS                         ( 20 )
#define NO_LOW_RES_BINS                 ( NO_IID_BINS / 2 )

#define NO_IID_BINS                     ( NO_BINS )
#define NO_ICC_BINS                     ( NO_BINS )
#define NO_IPD_BINS                     ( 11 )
#define NO_IPD_BINS_EST                 ( NO_BINS )

#define NO_LOW_RES_IID_BINS             ( NO_LOW_RES_BINS )
#define NO_LOW_RES_ICC_BINS             ( NO_LOW_RES_BINS )

#define NO_IPD_GROUPS                   ( NO_IPD_BINS_EST + 6 + 2 )
#define NEGATE_IPD_MASK                 ( 0x00001000 )

#define ENERGY_WINDOW_SLOPES            ( 2 )

#define NO_SUBSAMPLES                   ( 32 )
#define NO_QMF_BANDS                    ( 64 )
#define SYSTEMLOOKAHEAD                 ( 1 )

/*############################################################################*/
/* Type definitions                                                           */
/*############################################################################*/

struct PS_ENC{

  int   bEnableHeader;

  int   bHiFreqResIidIcc;
  int   iidIccBins;

  unsigned int bPrevZeroIid;
  unsigned int bPrevZeroIcc;

  int psMode;

  struct BIT_BUF psBitBuf;

  int hdrBitsPrevFrame;

  float **aaaIIDDataBuffer;
  float **aaaICCDataBuffer;

  int   aLastIidIndex[NO_IID_BINS];
  int   aLastIccIndex[NO_ICC_BINS];

  float *mHybridRealLeft[NO_SUBSAMPLES];
  float *mHybridImagLeft[NO_SUBSAMPLES];
  float *mHybridRealRight[NO_SUBSAMPLES];
  float *mHybridImagRight[NO_SUBSAMPLES];

  HYBRID hybridLeft;
  HYBRID hybridRight;
  HANDLE_HYBRID hHybridLeft;
  HANDLE_HYBRID hHybridRight;

  float   powerLeft    [NO_BINS];
  float   powerRight   [NO_BINS];
  float   powerCorrReal[NO_BINS];
  float   powerCorrImag[NO_BINS];

  float **tempQmfLeftReal;
  float **tempQmfLeftImag;
  float **histQmfLeftReal;
  float **histQmfLeftImag;
  float **histQmfRightReal;
  float **histQmfRightImag;
};

typedef struct PS_ENC *HANDLE_PS_ENC;

/*############################################################################*/
/* Extern function prototypes                                                 */
/*############################################################################*/
int GetPsMode(int bitRate);

int
CreatePsEnc(HANDLE_PS_ENC h_ps_enc,
            int psMode);

void
DeletePsEnc(HANDLE_PS_ENC *h_ps_e);


void
EncodePsFrame(HANDLE_PS_ENC h_ps_e,
              float **iBufferLeft,
              float **rBufferLeft,
              float **iBufferRight,
              float **rBufferRight);
#endif
