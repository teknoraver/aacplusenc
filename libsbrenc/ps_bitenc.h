/*
  Parametric stereo bitstream encoder
*/
#ifndef __PS_BITENC_H
#define __PS_BITENC_H

#include "FFR_bitbuffer.h"
#include "sbr_main.h"

#define CODE_BOOK_LAV_IID 14
#define CODE_BOOK_LAV_ICC 7
#define NO_IID_STEPS                    ( 7 )
#define NO_ICC_STEPS                    ( 8 )


struct PS_ENC;

int WritePsData (struct PS_ENC*  h_ps_e,
                 int                bHeaderActive);

int AppendPsBS (struct PS_ENC*  h_ps_e,
                HANDLE_BIT_BUF     hBitstream,
                HANDLE_BIT_BUF     hBitstreamPrev,
                int                *sbrHdrBits);

#endif
