/*
  Noiseless coder module
*/
#include <stdlib.h>
#include <limits.h>
#include "dyn_bits.h"
#include "bit_cnt.h"
#include "psy_const.h"
#include "minmax.h"
#include "aac_ram.h"

#include "counters.h" /* the 3GPP instrumenting tool */


static int
calcSideInfoBits(int sfbCnt,
                 int blockType)
{
  int seg_len_bits = (blockType == SHORT_WINDOW ? 3 : 5);
  int escape_val = (blockType == SHORT_WINDOW ? 7 : 31);
  int sideInfoBits, tmp;

  COUNT_sub_start("calcSideInfoBits");

  ADD(2); BRANCH(2); MOVE(2); /* counting previous operations */

  MOVE(2);
  sideInfoBits = CODE_BOOK_BITS;
  tmp = sfbCnt;

  LOOP(1);
  while (tmp >= 0)
  {
    ADD(2);
    sideInfoBits += seg_len_bits;
    tmp -= escape_val;
  }

  COUNT_sub_end();

  return (sideInfoBits);
}



/*
   count bits using all possible tables
 */

static void
buildBitLookUp(const short *quantSpectrum,
               const int maxSfb,
               const int *sfbOffset,
               const unsigned short *sfbMax,
               int bitLookUp[MAX_SFB_LONG][CODE_BOOK_ESC_NDX + 1],
               SECTION_INFO * section)
{
  int i;

  COUNT_sub_start("buildBitLookUp");

  PTR_INIT(4); /* pointers for section[],
                               sfbOffset[],
                               sfbMax[],
                               bitLookUp[]
               */
  LOOP(1);
  for (i = 0; i < maxSfb; i++)
  {
    int sfbWidth, maxVal;

    MOVE(4);
    section[i].sfbCnt = 1;
    section[i].sfbStart = i;
    section[i].sectionBits = INVALID_BITCOUNT;
    section[i].codeBook = -1;

    ADD(1);
    sfbWidth = sfbOffset[i + 1] - sfbOffset[i];

    MOVE(1);
    maxVal = sfbMax[i];

    PTR_INIT(1); FUNC(4);
    bitCount(quantSpectrum + sfbOffset[i], sfbWidth, maxVal, bitLookUp[i]);
  }

  COUNT_sub_end();
}

/*
   essential helper functions
 */

static int
findBestBook(const int *bc, int *book)
{
  int minBits = INVALID_BITCOUNT, j;

  COUNT_sub_start("findBestBook");

  MOVE(1); /* counting previous operations */

  PTR_INIT(1); /* pointer for bc[] */
  LOOP(1);
  for (j = 0; j <= CODE_BOOK_ESC_NDX; j++)
  {
    ADD(1); BRANCH(1);
    if (bc[j] < minBits)
    {

      MOVE(2);
      minBits = bc[j];
      *book = j;
    }
  }

  COUNT_sub_end();

  return (minBits);
}

static int
findMinMergeBits(const int *bc1, const int *bc2)
{
  int minBits = INVALID_BITCOUNT, j;

  COUNT_sub_start("findMinMergeBits");

  MOVE(1); /* counting previous operations */

  PTR_INIT(2); /* pointers for bc1[],
                               bc2[]
               */
  LOOP(1);
  for (j = 0; j <= CODE_BOOK_ESC_NDX; j++)
  {
    ADD(2); BRANCH(1);
    if (bc1[j] + bc2[j] < minBits)
    {
      MOVE(1);
      minBits = bc1[j] + bc2[j];
    }
  }

  COUNT_sub_end();

  return (minBits);
}

static void
mergeBitLookUp(int *bc1, const int *bc2)
{
  int j;

  COUNT_sub_start("mergeBitLookUp");

  PTR_INIT(2); /* pointers for bc1[],
                               bc2[]
               */
  LOOP(1);
  for (j = 0; j <= CODE_BOOK_ESC_NDX; j++)
  {
    ADD(2); BRANCH(1); MOVE(1);
    bc1[j] = min(bc1[j] + bc2[j], INVALID_BITCOUNT);
  }

  COUNT_sub_end();
}

static int
findMaxMerge(const int mergeGainLookUp[MAX_SFB_LONG],
             const SECTION_INFO *section,
             const int maxSfb, int *maxNdx)
{
  int i, maxMergeGain = 0;

  COUNT_sub_start("findMaxMerge");

  MOVE(1); /* counting previous operations */

  PTR_INIT(2); /* pointers for section[],
                               mergeGainLookUp[]
               */
  LOOP(1);
  for (i = 0; i + section[i].sfbCnt < maxSfb; i += section[i].sfbCnt)
  {
    ADD(1); BRANCH(1);
    if (mergeGainLookUp[i] > maxMergeGain)
    {
      MOVE(2);
      maxMergeGain = mergeGainLookUp[i];
      *maxNdx = i;
    }
  }

  COUNT_sub_end();

  return (maxMergeGain);
}



static int
CalcMergeGain(const SECTION_INFO * section,
              int bitLookUp[MAX_SFB_LONG][CODE_BOOK_ESC_NDX + 1],
              const int *sideInfoTab,
              const int ndx1,
              const int ndx2)
{
  int SplitBits;
  int MergeBits;
  int MergeGain;

  COUNT_sub_start("CalcMergeGain");

  PTR_INIT(4); /* pointers for section[ndx1],
                               section[ndx2],
                               bitLookUp[ndx1],
                               bitLookUp[ndx2]
               */

  /*
     Bit amount for splitted sections
   */
  ADD(1);
  SplitBits = section[ndx1].sectionBits + section[ndx2].sectionBits;

  ADD(2); INDIRECT(1); FUNC(2);
  MergeBits = sideInfoTab[section[ndx1].sfbCnt + section[ndx2].sfbCnt] + findMinMergeBits(bitLookUp[ndx1], bitLookUp[ndx2]);

  ADD(1);
  MergeGain = SplitBits - MergeBits;

  COUNT_sub_end();

  return (MergeGain);
}


static void
gmStage0(SECTION_INFO * section,
         int bitLookUp[MAX_SFB_LONG][CODE_BOOK_ESC_NDX + 1],
         const int maxSfb)
{
  int i;

  COUNT_sub_start("gmStage0");

  PTR_INIT(2); /* pointers for section[],
                               bitLookUp[]
               */
  LOOP(1);
  for (i = 0; i < maxSfb; i++)
  {
    ADD(1); BRANCH(1);
    if (section[i].sectionBits == INVALID_BITCOUNT)
    {
      PTR_INIT(1); FUNC(2); STORE(1);
      section[i].sectionBits = findBestBook(bitLookUp[i], &(section[i].codeBook));

    }
  }

  COUNT_sub_end();
}


static void
gmStage1(SECTION_INFO * section,
         int bitLookUp[MAX_SFB_LONG][CODE_BOOK_ESC_NDX + 1],
         const int maxSfb,
         const int *sideInfoTab)
{
  int mergeStart = 0, mergeEnd;

  COUNT_sub_start("gmStage1");

  MOVE(1); /* counting previous operations */

  LOOP(1);
  do
  {

    PTR_INIT(4); /* pointers for section[mergeStart]
                                 section[mergeEnd]
                                 bitLookUp[mergeStart]
                                 bitLookUp[mergeEnd]
                 */
    ADD(1); LOOP(1);
    for (mergeEnd = mergeStart + 1; mergeEnd < maxSfb; mergeEnd++)
    {
      ADD(1); BRANCH(1);
      if (section[mergeStart].codeBook != section[mergeEnd].codeBook)
        break;

      ADD(1); STORE(1);
      section[mergeStart].sfbCnt++;

      ADD(1); STORE(1);
      section[mergeStart].sectionBits += section[mergeEnd].sectionBits;

      FUNC(2);
      mergeBitLookUp(bitLookUp[mergeStart], bitLookUp[mergeEnd]);
    }

    INDIRECT(1); ADD(1); STORE(1);
    section[mergeStart].sectionBits += sideInfoTab[section[mergeStart].sfbCnt];

    MOVE(1);
    section[mergeEnd - 1].sfbStart = section[mergeStart].sfbStart;

    MOVE(1);
    mergeStart = mergeEnd;

  } while (mergeStart < maxSfb);

  COUNT_sub_end();
}


static void
gmStage2(SECTION_INFO * section,
         int mergeGainLookUp[MAX_SFB_LONG],
         int bitLookUp[MAX_SFB_LONG][CODE_BOOK_ESC_NDX + 1],
         const int maxSfb,
         const int *sideInfoTab)
{
  int i;

  COUNT_sub_start("gmStage2");

  PTR_INIT(2); /* pointers for section[],
                               mergeGainLookUp[]
               */
  LOOP(1);
  for (i = 0; i + section[i].sfbCnt < maxSfb; i += section[i].sfbCnt)
  {
    ADD(1); FUNC(5); STORE(1);
    mergeGainLookUp[i] = CalcMergeGain(section,
                                       bitLookUp,
                                       sideInfoTab,
                                       i,
                                       i + section[i].sfbCnt);

  }

  LOOP(1);
  while (TRUE)
  {
    int maxMergeGain, maxNdx, maxNdxNext, maxNdxLast;

    PTR_INIT(1); FUNC(4);
    maxMergeGain = findMaxMerge(mergeGainLookUp, section, maxSfb, &maxNdx);

    /*
       exit while loop if no more gain is possible
     */
    BRANCH(1);
    if (maxMergeGain <= 0)
      break;


    PTR_INIT(3); /* pointers for section[maxNdx],
                                 bitLookUp[maxNdx],
                                 mergeGainLookUp[maxNdx]
                 */
    ADD(1);
    maxNdxNext = maxNdx + section[maxNdx].sfbCnt;

    PTR_INIT(2); /* pointers for section[maxNdxNext],
                                 bitLookUp[maxNdxNext]
                 */

    ADD(1);
    section[maxNdx].sfbCnt += section[maxNdxNext].sfbCnt;

    ADD(2);
    section[maxNdx].sectionBits += section[maxNdxNext].sectionBits - maxMergeGain;

    FUNC(2);
    mergeBitLookUp(bitLookUp[maxNdx], bitLookUp[maxNdxNext]);

    BRANCH(1);
    if (maxNdx != 0)
    {
      MOVE(1);
      maxNdxLast = section[maxNdx - 1].sfbStart;

      FUNC(5); INDIRECT(1); STORE(1);
      mergeGainLookUp[maxNdxLast] = CalcMergeGain(section,
                                                  bitLookUp,
                                                  sideInfoTab,
                                                  maxNdxLast,
                                                  maxNdx);
    }

    ADD(1);
    maxNdxNext = maxNdx + section[maxNdx].sfbCnt;

    PTR_INIT(1); /* pointers for section[maxNdxNext] */

    MOVE(1);
    section[maxNdxNext - 1].sfbStart = section[maxNdx].sfbStart;

    ADD(1); BRANCH(1);
    if (maxNdxNext < maxSfb)
    {
      FUNC(5); STORE(1);
      mergeGainLookUp[maxNdx] = CalcMergeGain(section,
                                              bitLookUp,
                                              sideInfoTab,
                                              maxNdx,
                                              maxNdxNext);
    }
  }

  COUNT_sub_end();
}

/*
   count bits used by the noiseless coder
 */
static void
noiselessCounter(SECTION_DATA * sectionData,
                 int mergeGainLookUp[MAX_SFB_LONG],
                 int bitLookUp[MAX_SFB_LONG][CODE_BOOK_ESC_NDX + 1],
                 const short *quantSpectrum,
                 const unsigned short *maxValueInSfb,
                 const int *sfbOffset,
                 const int blockType)
{
  int grpNdx, i;
  int *sideInfoTab = NULL;
  SECTION_INFO *section;

  COUNT_sub_start("noiselessCounter");

  PTR_INIT(1); /* counting previous operations */

  BRANCH(2);
  switch (blockType)
  {
  case LONG_WINDOW:
  case START_WINDOW:
  case STOP_WINDOW:

    MOVE(1);
    sideInfoTab = sideInfoTabLong;
    break;
  case SHORT_WINDOW:

    MOVE(1);
    sideInfoTab = sideInfoTabShort;
    break;
  }


  INDIRECT(3); MOVE(3);
  sectionData->noOfSections = 0;
  sectionData->huffmanBits = 0;
  sectionData->sideInfoBits = 0;



  INDIRECT(1); BRANCH(1);
  if (sectionData->maxSfbPerGroup == 0) {

    COUNT_sub_end();
    return;
  }

  INDIRECT(2); LOOP(1);
  for (grpNdx = 0; grpNdx < sectionData->sfbCnt; grpNdx += sectionData->sfbPerGroup)
  {

    INDIRECT(2); ADD(1);
    section = sectionData->section + sectionData->noOfSections;

    INDIRECT(1); ADD(2); FUNC(6);
    buildBitLookUp(quantSpectrum,
                   sectionData->maxSfbPerGroup,
                   sfbOffset + grpNdx,
                   maxValueInSfb + grpNdx,
                   bitLookUp,
                   section);

    INDIRECT(1); FUNC(3);
    gmStage0(section, bitLookUp, sectionData->maxSfbPerGroup);

    FUNC(4);
    gmStage1(section, bitLookUp, sectionData->maxSfbPerGroup, sideInfoTab);

    INDIRECT(1); FUNC(5);
    gmStage2(section,
             mergeGainLookUp,
             bitLookUp,
             sectionData->maxSfbPerGroup,
             sideInfoTab);


    PTR_INIT(2); /* pointers for section[],
                                 bitLookUp[]
                 */
    INDIRECT(1); LOOP(1);
    for (i = 0; i < sectionData->maxSfbPerGroup; i += section[i].sfbCnt)
    {
      PTR_INIT(1); FUNC(2);
      findBestBook(bitLookUp[i], &(section[i].codeBook));

      ADD(1); STORE(1);
      section[i].sfbStart += grpNdx;

      INDIRECT(2); ADD(2); STORE(1);
      sectionData->huffmanBits += section[i].sectionBits - sideInfoTab[section[i].sfbCnt];

      INDIRECT(2); ADD(1); STORE(1);
      sectionData->sideInfoBits += sideInfoTab[section[i].sfbCnt];

      ADD(1); INDIRECT(1); MOVE(1);
      sectionData->section[sectionData->noOfSections++] = section[i];
    }
  }
  COUNT_sub_end();
}


static void scfCount(const short *scalefacGain,
                     const unsigned short *maxValueInSfb,
                     SECTION_DATA * sectionData)

{
  /* counter */
  int i = 0; /* section counter */
  int j = 0; /* sfb counter */
  int k = 0; /* current section auxiliary counter */
  int m = 0; /* other section auxiliary counter */
  int n = 0; /* other sfb auxiliary counter */

  /* further variables */
  int lastValScf     = 0;
  int deltaScf       = 0;
  int found          = 0;
  int scfSkipCounter = 0;

  COUNT_sub_start("scfCount");

  MOVE(9); /* counting previous operations */

  INDIRECT(1); MOVE(1);
  sectionData->scalefacBits = 0;
  
  BRANCH(1);
  if (scalefacGain == NULL) {
    COUNT_sub_end();
    return;
  }

  INDIRECT(1); MOVE(2);
  lastValScf = 0;
  sectionData->firstScf = 0;
  
  PTR_INIT(1); /* sectionData->section[] */
  INDIRECT(1); LOOP(1);
  for (i = 0; i < sectionData->noOfSections; i++) {

    ADD(1); BRANCH(1);
    if (sectionData->section[i].codeBook != CODE_BOOK_ZERO_NO) {

      INDIRECT(1); MOVE(1);
      sectionData->firstScf = sectionData->section[i].sfbStart;
      
      INDIRECT(1); MOVE(1);
      lastValScf = scalefacGain[sectionData->firstScf];
      break;
    }
  }
  
  PTR_INIT(1); /* sectionData->section[] */
  INDIRECT(1); LOOP(1);
  for (i = 0; i < sectionData->noOfSections; i++) {

    ADD(2); LOGIC(1); BRANCH(1);
    if ((sectionData->section[i].codeBook != CODE_BOOK_ZERO_NO) &&
        (sectionData->section[i].codeBook != CODE_BOOK_PNS_NO)) {

      PTR_INIT(2); /* maxValueInSfb[]
                      scalefacGain[]
                   */
      ADD(1); LOOP(1);
      for (j = sectionData->section[i].sfbStart;
           j < sectionData->section[i].sfbStart + sectionData->section[i].sfbCnt;
           j++) {
        
        BRANCH(1);
        if (maxValueInSfb[j] == 0) {

          MOVE(1);
          found = 0;
          
          BRANCH(1);
          if (scfSkipCounter == 0) {
            
            ADD(3); BRANCH(1);
            if (j == (sectionData->section[i].sfbStart + sectionData->section[i].sfbCnt - 1) ) {
              
              MOVE(1);
              found = 0;
            }
            else {
              
              PTR_INIT(2); /* maxValueInSfb[]
                              scalefacGain[]
                           */
              LOOP(1);
              for (k = (j+1); k < sectionData->section[i].sfbStart + sectionData->section[i].sfbCnt; k++) {
                BRANCH(1);
                if (maxValueInSfb[k] != 0) {
                  MOVE(1);
                  found = 1;
                  
                  ADD(2); MISC(1); BRANCH(1);
                  if ( (abs(scalefacGain[k] - lastValScf)) < CODE_BOOK_SCF_LAV) {
                    MOVE(1);
                    deltaScf = 0;
                  }
                  else {
                    ADD(1); MULT(1);
                    deltaScf = -(scalefacGain[j] - lastValScf);
                    
                    MOVE(2);
                    lastValScf = scalefacGain[j];
                    scfSkipCounter = 0;
                  }
                  break;
                }
                /* count scalefactor skip */
                ADD(1);
                scfSkipCounter = scfSkipCounter + 1;
              }
            }
            
            /* search for the next maxValueInSfb[] != 0 in all other sections */
            PTR_INIT(1); /* sectionData->section[] */
            INDIRECT(1); LOOP(1);
            for (m = (i+1); (m < sectionData->noOfSections) && (found == 0); m++) {

              ADD(2); LOGIC(1); BRANCH(1);
              if ((sectionData->section[m].codeBook != CODE_BOOK_ZERO_NO) &&
                  (sectionData->section[m].codeBook != CODE_BOOK_PNS_NO)) {
                PTR_INIT(2); /* maxValueInSfb[]
                                scalefacGain[]
                             */
                LOOP(1);
                for (n = sectionData->section[m].sfbStart;
                     n < sectionData->section[m].sfbStart + sectionData->section[m].sfbCnt;
                     n++) {
                  BRANCH(1);
                  if (maxValueInSfb[n] != 0) {
                    MOVE(1);
                    found = 1;
                    
                    ADD(2); MISC(1); BRANCH(1);
                    if ( (abs(scalefacGain[n] - lastValScf)) < CODE_BOOK_SCF_LAV) {
                      MOVE(1);
                      deltaScf = 0;
                    }
                    else {
                      
                      ADD(1); MULT(1);
                      deltaScf = -(scalefacGain[j] - lastValScf);
                      
                      MOVE(2);
                      lastValScf = scalefacGain[j];
                      scfSkipCounter = 0;
                    }
                    break;
                  }
                  
                  ADD(1);
                  scfSkipCounter = scfSkipCounter + 1;
                }
              }
            }
            
            BRANCH(1);
            if (found == 0)   {
              MOVE(2);
              deltaScf = 0;
              scfSkipCounter = 0;
            }
          }
          else {

            MOVE(1);
            deltaScf = 0;
            
            ADD(1);
            scfSkipCounter = scfSkipCounter - 1;
          }
        }
        else {
          ADD(1); MULT(1);
          deltaScf = -(scalefacGain[j] - lastValScf);
          
          MOVE(1);
          lastValScf = scalefacGain[j];
        }
        
        INDIRECT(1); FUNC(1); ADD(1); STORE(1);
        sectionData->scalefacBits += bitCountScalefactorDelta(deltaScf);
      }
    }
  }
  
  COUNT_sub_end();
}


typedef int (*lookUpTable)[CODE_BOOK_ESC_NDX + 1];

int
dynBitCount(const short          *quantSpectrum,
            const unsigned short   *maxValueInSfb,
            const signed short     *scalefac,
            const int             blockType,
            const int             sfbCnt,
            const int             maxSfbPerGroup,
            const int             sfbPerGroup,
            const int            *sfbOffset,
            SECTION_DATA         *sectionData)
{
  int bitLookUp[MAX_SFB_LONG*(CODE_BOOK_ESC_NDX+1)];
  int mergeGainLookUp[MAX_SFB_LONG];

  COUNT_sub_start("dynBitCount");

  INDIRECT(3); MOVE(3);
  sectionData->blockType      = blockType;
  sectionData->sfbCnt         = sfbCnt;
  sectionData->sfbPerGroup    = sfbPerGroup;

  INDIRECT(1); DIV(1); STORE(1);
  sectionData->noOfGroups     = sfbCnt / sfbPerGroup;

  MOVE(1);
  sectionData->maxSfbPerGroup = maxSfbPerGroup;

  FUNC(7);
  noiselessCounter(sectionData,
                   mergeGainLookUp,
                   (lookUpTable)bitLookUp,
                   quantSpectrum,
                   maxValueInSfb,
                   sfbOffset,
                   blockType);

  FUNC(3);
  scfCount(scalefac,
           maxValueInSfb,
           sectionData);

  INDIRECT(3); ADD(2); /* counting post-operations */

  COUNT_sub_end();

  return (sectionData->huffmanBits +
          sectionData->sideInfoBits +
          sectionData->scalefacBits);
}


int
BCInit(void)
{
  int i;

  COUNT_sub_start("BCInit");

  PTR_INIT(1); /* sideInfoTabLong[] */
  LOOP(1);
  for (i = 0; i <= MAX_SFB_LONG; i++)
  {
    FUNC(2); STORE(1);
    sideInfoTabLong[i] = calcSideInfoBits(i, LONG_WINDOW);
  }

  PTR_INIT(1); /* sideInfoTabShort[] */
  for (i = 0; i <= MAX_SFB_SHORT; i++)
  {
    FUNC(2); STORE(1);
    sideInfoTabShort[i] = calcSideInfoBits(i, SHORT_WINDOW);
  }

  COUNT_sub_end();

  return 0;
}
