#ifndef __BITCOUNT_H
#define __BITCOUNT_H

#include <limits.h>
#include "FFR_bitbuffer.h"
#define INVALID_BITCOUNT (INT_MAX/4)

/*
  code book number table
*/

enum codeBookNo{
 CODE_BOOK_ZERO_NO=               0,
 CODE_BOOK_1_NO=                  1,
 CODE_BOOK_2_NO=                  2,
 CODE_BOOK_3_NO=                  3,
 CODE_BOOK_4_NO=                  4,
 CODE_BOOK_5_NO=                  5,
 CODE_BOOK_6_NO=                  6,
 CODE_BOOK_7_NO=                  7,
 CODE_BOOK_8_NO=                  8,
 CODE_BOOK_9_NO=                  9,
 CODE_BOOK_10_NO=                10,
 CODE_BOOK_ESC_NO=               11,
 CODE_BOOK_RES_NO=               12,
 CODE_BOOK_PNS_NO=               13
};

/*
  code book index table
*/

enum codeBookNdx{
 CODE_BOOK_ZERO_NDX,
 CODE_BOOK_1_NDX,
 CODE_BOOK_2_NDX,
 CODE_BOOK_3_NDX,
 CODE_BOOK_4_NDX,
 CODE_BOOK_5_NDX,
 CODE_BOOK_6_NDX,
 CODE_BOOK_7_NDX,
 CODE_BOOK_8_NDX,
 CODE_BOOK_9_NDX,
 CODE_BOOK_10_NDX,
 CODE_BOOK_ESC_NDX,
 CODE_BOOK_RES_NDX,
 CODE_BOOK_PNS_NDX,
 NUMBER_OF_CODE_BOOKS
};

/*
  code book lav table
*/

enum codeBookLav{
 CODE_BOOK_ZERO_LAV=0,
 CODE_BOOK_1_LAV=1,
 CODE_BOOK_2_LAV=1,
 CODE_BOOK_3_LAV=2,
 CODE_BOOK_4_LAV=2,
 CODE_BOOK_5_LAV=4,
 CODE_BOOK_6_LAV=4,
 CODE_BOOK_7_LAV=7,
 CODE_BOOK_8_LAV=7,
 CODE_BOOK_9_LAV=12,
 CODE_BOOK_10_LAV=12,
 CODE_BOOK_ESC_LAV=16,
 CODE_BOOK_SCF_LAV=60,
 CODE_BOOK_PNS_LAV=60
 };

int    bitCount(const short       *aQuantSpectrum,
                const int         noOfSpecLines,
                int               maxVal,
                int               *bitCountLut);

int    codeValues(short *values, int width, int codeBook, HANDLE_BIT_BUF hBitstream);

int    bitCountScalefactorDelta(signed int delta);
int    codeScalefactorDelta(signed int scalefactor, HANDLE_BIT_BUF hBitstream);



#endif
