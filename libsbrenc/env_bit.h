/*
  Remaining SBR Bit Writing Routines
*/

#ifndef BIT_ENV_H
#define BIT_ENV_H

#include "FFR_bitbuffer.h"

#define SBR_CRC_POLY     (0x0233)
#define SBR_CRC_MASK     (0x0200)
#define SBR_CRC_RANGE    (0x03FF)
#define SBR_CRC_MAXREGS     1
#define SBR_CRCINIT      (0x0)

#define AAC_SI_FIL_SBR             13
#define AAC_SI_FIL_SBR_CRC         14


#define SI_SBR_CRC_ENABLE_BITS                  0
#define SI_SBR_CRC_BITS                        10





#define  SI_ID_BITS_AAC             3
#define  SI_FILL_COUNT_BITS         4
#define  SI_FILL_ESC_COUNT_BITS     8
#define  SI_FILL_EXTENTION_BITS     4
#define  ID_FIL                     6






struct COMMON_DATA;

void InitSbrBitstream(struct COMMON_DATA  *hCmonData,
                      unsigned char *memoryBase,
                      int            memorySize,
                      int CRCActive);

void
AssembleSbrBitstream(struct COMMON_DATA  *hCmonData);





#endif /* #ifndef BIT_ENV_H */
