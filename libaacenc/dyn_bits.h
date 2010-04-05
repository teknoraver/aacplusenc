/*
  Noiseless coder module
*/
#ifndef __DYN_BITS_H
#define __DYN_BITS_H

#include "psy_const.h"
#include "tns.h"
#include "bit_cnt.h"

#define MAX_SECTIONS          MAX_GROUPED_SFB
#define SECT_ESC_VAL_LONG    31
#define SECT_ESC_VAL_SHORT    7
#define CODE_BOOK_BITS        4
#define SECT_BITS_LONG        5
#define SECT_BITS_SHORT       3

typedef struct
{
  int codeBook;
  int sfbStart;
  int sfbCnt;
  int sectionBits;              /* huff + si ! */
}
SECTION_INFO;

typedef struct
{
  int blockType;
  int noOfGroups;
  int sfbCnt;
  int maxSfbPerGroup;
  int sfbPerGroup;
  int noOfSections;
  SECTION_INFO section[MAX_SECTIONS];
  int sideInfoBits;             /* sectioning bits       */
  int huffmanBits;              /* huffman    coded bits */
  int scalefacBits;             /* scalefac   coded bits */
  int firstScf;                 /* first scf to be coded */
}
SECTION_DATA;


int  BCInit(void);

int  dynBitCount(const short *quantSpectrum,
                 const unsigned short *maxValueInSfb,
                 const signed  short  *scalefac,
                 const int blockType,
                 const int sfbCnt,
                 const int maxSfbPerGroup,
                 const int sfbPerGroup,
                 const int *sfbOffset,
                 SECTION_DATA * sectionData);

int  RecalcSectionInfo(SECTION_DATA * sectionData,
                       const unsigned short *maxValueInSfb,
                       const signed   short *scalefac,
                       const int blockType);

#endif
