/*
  Parametric stereo bitstream encoder
*/

#include "sbr_def.h"
#include "ps_bitenc.h"
#include "ps_enc.h"
#include "env_bit.h"
#include "sbr_ram.h"
#include "sbr_rom.h"

#include "counters.h" /* the 3GPP instrumenting tool */

static int
appendBitstream( HANDLE_BIT_BUF hBitBufWrite,
                 HANDLE_BIT_BUF hBitBufRead,
                 int nBits )
{
  int i;
  unsigned b;

  COUNT_sub_start("appendBitstream");

  LOOP(1);
  for (i=0; i< nBits; i++)
  {
    FUNC(2);
    b = ReadBits (hBitBufRead, 1);

    FUNC(3);
    WriteBits (hBitBufWrite, b, 1);
  }

  COUNT_sub_end();

  return nBits;
}

int
WritePsData (HANDLE_PS_ENC h_ps_e,
             int           bHeaderActive)
{
  int temp, gr;
  const int *aaHuffBookIidC;
  const short *aaHuffBookIccC;
  const char *aaHuffBookIidL;
  const char *aaHuffBookIccL;
  int *aaDeltaIid;
  int *aaDeltaIcc;

  int index, lastIndex;
  int noBitsF = 0;
  int noBitsT = 0;

  int aaDeltaIidT[NO_IID_BINS];
  int aaDeltaIccT[NO_ICC_BINS];

  int aaDeltaIidF[NO_IID_BINS];
  int aaDeltaIccF[NO_ICC_BINS];

  int abDtFlagIid;
  int abDtFlagIcc;

  int bSendHeader;

  unsigned int bZeroIid = 1;
  unsigned int bZeroIcc = 1;
  unsigned int bKeepParams = 1;

  HANDLE_BIT_BUF bb = &h_ps_e->psBitBuf;

  COUNT_sub_start("WritePsData");

  INDIRECT(1); MOVE(6); PTR_INIT(1); /* counting previous operations */

  FUNC(1);
  temp = GetBitsAvail(bb);

  /* bit buffer shall be empty */
  BRANCH(1);
  if (temp != 0)
  {
    COUNT_sub_end();
    return -1;
  }

  BRANCH(1);
  if (bHeaderActive) {

    MOVE(1);
    bKeepParams = 0;
  }

  MOVE(1);
  lastIndex = 0;

  PTR_INIT(3); /* h_ps_e->aaaIIDDataBuffer[][]
                  h_ps_e->aaaICCDataBuffer[][]
                  h_ps_e->aLastIidIndex[]
               */
  INDIRECT(1); LOOP(1);
  for (gr = 0; gr < h_ps_e->iidIccBins; gr++) {
    float panValue = h_ps_e->aaaIIDDataBuffer[gr][SYSTEMLOOKAHEAD];

    MOVE(1); /* counting previous operation */

    ADD(2); BRANCH(1);
    if (panValue >= -panClass[0] &&
        panValue <=  panClass[0]) {

      MOVE(1);
      index = 0;
    }
    else  {

      BRANCH(1);
      if (panValue < 0) {

        PTR_INIT(1); /* panClass[] */
        LOOP(1);
        for (index = NO_IID_STEPS-1; panValue > -panClass[index]; index--) {
          ADD(1); /* for() condition */
        }

        ADD(1);
        index = -index - 1;
      }
      else  {
        PTR_INIT(1); /* panClass[] */
        LOOP(1);
        for (index = NO_IID_STEPS-1; panValue <  panClass[index]; index--) {
          ADD(1); /* for() condition */
        }

        ADD(1);
        index++;
      }

      MOVE(1);
      bZeroIid = 0;
    }

    BRANCH(1);
    if (gr == 0) {

      MOVE(2);
      aaDeltaIidF[gr] = index;
      noBitsT = 0;

      INDIRECT(1); MOVE(1);
      noBitsF = aBookPsIidFreqLength[index + CODE_BOOK_LAV_IID];
    }
    else {

      ADD(1); STORE(1);
      aaDeltaIidF[gr] = index - lastIndex;

      INDIRECT(1); ADD(1);
      noBitsF += aBookPsIidFreqLength[aaDeltaIidF[gr] + CODE_BOOK_LAV_IID];
    }

    MOVE(1);
    lastIndex = index;

    ADD(1); STORE(1);
    aaDeltaIidT[gr] = index - h_ps_e->aLastIidIndex[gr];

    MOVE(1);
    h_ps_e->aLastIidIndex[gr] = index;

    INDIRECT(1); ADD(1);
    noBitsT += aBookPsIidTimeLength[aaDeltaIidT[gr] + CODE_BOOK_LAV_IID];

    BRANCH(1);
    if (aaDeltaIidT[gr] != 0) {

      MOVE(1);
      bKeepParams = 0;
    }
  } /* gr */

  ADD(1); LOGIC(1); BRANCH(1);
  if (noBitsT < noBitsF && !bHeaderActive) {

    MOVE(4);
    abDtFlagIid    = 1;
    aaDeltaIid     = aaDeltaIidT;
    aaHuffBookIidC = aBookPsIidTimeCode;
    aaHuffBookIidL = aBookPsIidTimeLength;
  }
  else  {

    MOVE(4);
    abDtFlagIid    = 0;
    aaDeltaIid     = aaDeltaIidF;
    aaHuffBookIidC = aBookPsIidFreqCode;
    aaHuffBookIidL = aBookPsIidFreqLength;
  }

  MOVE(1);
  lastIndex = 0;

  PTR_INIT(4); /* h_ps_e->aaaICCDataBuffer[][]
                  h_ps_e->aLastIccIndex[]
                  aaDeltaIccF[]
                  aaDeltaIccT[]
               */
  INDIRECT(1); LOOP(1);
  for (gr = 0; gr < h_ps_e->iidIccBins; gr++) {

    float saValue = h_ps_e->aaaICCDataBuffer[gr][SYSTEMLOOKAHEAD];

    MOVE(1); /* counting previous operation */

    ADD(1); BRANCH(1);
    if (saValue <= saClass[0]) {

      MOVE(1);
      index = 0;
    }
    else  {
      PTR_INIT(1); /* saClass[] */
      LOOP(1);
      for (index = NO_ICC_STEPS-2;  saValue < saClass[index]; index--) {
        ADD(1); /* for() condition */
      }

      ADD(1);
      index++;

      MOVE(1);
      bZeroIcc = 0;
    }

    BRANCH(1);
    if (gr == 0) {

      MOVE(1);
      aaDeltaIccF[gr] = index;

      INDIRECT(1); MOVE(1);
      noBitsF = aBookPsIccFreqLength[index + CODE_BOOK_LAV_ICC];

      MOVE(1);
      noBitsT = 0;
    }
    else  {
      ADD(1); STORE(1);
      aaDeltaIccF[gr] = index - lastIndex;

      INDIRECT(1); ADD(1);
      noBitsF += aBookPsIccFreqLength[aaDeltaIccF[gr] + CODE_BOOK_LAV_ICC];
    }

    MOVE(1);
    lastIndex = index;

    ADD(1); STORE(1);
    aaDeltaIccT[gr] = index - h_ps_e->aLastIccIndex[gr];

    MOVE(1);
    h_ps_e->aLastIccIndex[gr] = index;

    INDIRECT(1); ADD(1);
    noBitsT += aBookPsIccTimeLength[aaDeltaIccT[gr] + CODE_BOOK_LAV_ICC];

    BRANCH(1);
    if (aaDeltaIccT[gr] != 0) {

      MOVE(1);
      bKeepParams = 0;
    }
  } /* gr */

  ADD(1); LOGIC(1); BRANCH(1);
  if (noBitsT < noBitsF && !bHeaderActive) {

    MOVE(4);
    abDtFlagIcc    = 1;
    aaDeltaIcc     = aaDeltaIccT;
    aaHuffBookIccC = aBookPsIccTimeCode;
    aaHuffBookIccL = aBookPsIccTimeLength;
  }
  else {

    MOVE(4);
    abDtFlagIcc    = 0;
    aaDeltaIcc     = aaDeltaIccF;
    aaHuffBookIccC = aBookPsIccFreqCode;
    aaHuffBookIccL = aBookPsIccFreqLength;
  }

  {
    static int initheader = 0;

    MOVE(1); /* counting previous operation */

    LOGIC(1); BRANCH(1);
    if (!initheader || bHeaderActive) {

      INDIRECT(1); MOVE(2);
      initheader = 1;
      h_ps_e->bEnableHeader = 1;
    }
    else {

      INDIRECT(1); MOVE(1);
      h_ps_e->bEnableHeader = 0;
    }
  }

  INDIRECT(3); ADD(2); LOGIC(2);
  bSendHeader = h_ps_e->bEnableHeader            ||
                h_ps_e->bPrevZeroIid != bZeroIid ||
                h_ps_e->bPrevZeroIcc != bZeroIcc;

  FUNC(3);
  WriteBits (bb, bSendHeader, 1);

  BRANCH(1);
  if (bSendHeader) {

    LOGIC(1); FUNC(3);
    WriteBits (bb, !bZeroIid, 1);

    BRANCH(1);
    if (!bZeroIid)
    {
      BRANCH(1); FUNC(3);
      WriteBits (bb, (h_ps_e->bHiFreqResIidIcc)?1:0, 3);
    }

    LOGIC(1); FUNC(3);
    WriteBits (bb, !bZeroIcc, 1);

    BRANCH(1);
    if (!bZeroIcc)
    {
      BRANCH(1); FUNC(3);
      WriteBits (bb, (h_ps_e->bHiFreqResIidIcc)?1:0, 3);
    }

    FUNC(3);
    WriteBits (bb, 0, 1);
  }

  FUNC(3);
  WriteBits (bb, 0, 1);

  ADD(1); FUNC(3);
  WriteBits (bb, 1-bKeepParams, 2);

  PTR_INIT(1); /* h_ps_e->iidIccBins
                */
  BRANCH(1);
  if (!bKeepParams) {

    LOGIC(1); BRANCH(1);
    if (!bZeroIid) {

      FUNC(3);
      WriteBits (bb, abDtFlagIid, 1);

      LOOP(1);
      for (gr = 0; gr < h_ps_e->iidIccBins; gr++) {

        INDIRECT(2); FUNC(3);
        WriteBits (bb,
                   aaHuffBookIidC[aaDeltaIid[gr] + CODE_BOOK_LAV_IID],
                   aaHuffBookIidL[aaDeltaIid[gr] + CODE_BOOK_LAV_IID]);
      } /* gr */
    }  /* if (!bZeroIid) */
  }

  PTR_INIT(1); /* h_ps_e->iidIccBins
               */
  BRANCH(1);
  if (!bKeepParams) {

    LOGIC(1); BRANCH(1);
    if (!bZeroIcc) {

      FUNC(3);
      WriteBits (bb, abDtFlagIcc, 1);

      LOOP(1);
      for (gr = 0; gr < h_ps_e->iidIccBins; gr++) {

        INDIRECT(2); FUNC(3);
        WriteBits (bb,
                   aaHuffBookIccC[aaDeltaIcc[gr] + CODE_BOOK_LAV_ICC],
                   aaHuffBookIccL[aaDeltaIcc[gr] + CODE_BOOK_LAV_ICC]);
      } /* gr */
    }  /* if (!bZeroIcc) */
  }

  INDIRECT(2); MOVE(2);
  h_ps_e->bPrevZeroIid = bZeroIid;
  h_ps_e->bPrevZeroIcc = bZeroIcc;

  FUNC(1); /* counting post-operation */

  COUNT_sub_end();

  return  GetBitsAvail(bb);

} /* writePsData */


/***************************************************************************/
/*!

  \brief  appends the parametric stereo bitstream portion to the output
          bitstream

  \return Number of bits needed for parametric stereo coding
          of 0 if no EXTENSION_ID_PS_CODING element should be transmitted

****************************************************************************/
int
AppendPsBS (HANDLE_PS_ENC h_ps_e,
            HANDLE_BIT_BUF   hBitStream,
            HANDLE_BIT_BUF   hBitStreamPrev,
            int*             sbrHdrBits)
{
  struct BIT_BUF bitbufTmp;
  unsigned char tmp[MAX_PAYLOAD_SIZE];

  COUNT_sub_start("AppendPsBS");

  BRANCH(1);
  if (!h_ps_e)
  {
    COUNT_sub_end();
    return 0;
  }

  BRANCH(1);
  if (!hBitStream) {

    INDIRECT(1); PTR_INIT(1); FUNC(1); /* counting post-operation */
    COUNT_sub_end();

    return GetBitsAvail (&h_ps_e->psBitBuf);
  }
  else {
    int writtenNoBits = 0;
    int maxExtSize = (1<<SI_SBR_EXTENSION_SIZE_BITS) - 1;
    int numBits = GetBitsAvail (&h_ps_e->psBitBuf);
    int extDataSize = (numBits+SI_SBR_EXTENSION_ID_BITS+7)>>3;

    MOVE(2); INDIRECT(1); PTR_INIT(1); FUNC(1); ADD(1); SHIFT(1); /* counting previous operations */

    FUNC(1); BRANCH(1);
    if ( GetBitsAvail(hBitStreamPrev) == 0) {

      INDIRECT(1); MOVE(1);
      h_ps_e->hdrBitsPrevFrame = *sbrHdrBits;

      FUNC(2);
      CopyBitBuf(hBitStream, hBitStreamPrev);
    }
    else {

      int tmpBits;

      PTR_INIT(1); FUNC(3);
      CreateBitBuffer (&bitbufTmp, tmp, sizeof(tmp));

      MOVE(1);
      tmpBits = *sbrHdrBits;

      INDIRECT(1); MOVE(2);
      *sbrHdrBits = h_ps_e->hdrBitsPrevFrame;
      h_ps_e->hdrBitsPrevFrame = tmpBits;

      FUNC(2);
      CopyBitBuf (hBitStreamPrev, &bitbufTmp);

      FUNC(2);
      CopyBitBuf (hBitStream,  hBitStreamPrev);

      FUNC(2);
      CopyBitBuf (&bitbufTmp,  hBitStream);
    }

    FUNC(3);
    WriteBits (hBitStream, 1, SI_SBR_EXTENDED_DATA_BITS);

    ADD(1); BRANCH(1);
    if (extDataSize < maxExtSize) {

      FUNC(3);
      WriteBits (hBitStream, extDataSize, SI_SBR_EXTENSION_SIZE_BITS);
    } else {
      FUNC(3);
      WriteBits (hBitStream, maxExtSize, SI_SBR_EXTENSION_SIZE_BITS);

      ADD(1); FUNC(3);
      WriteBits (hBitStream, extDataSize - maxExtSize, SI_SBR_EXTENSION_ESC_COUNT_BITS);
    }

    FUNC(3); ADD(1);
    writtenNoBits += WriteBits (hBitStream, EXTENSION_ID_PS_CODING, SI_SBR_EXTENSION_ID_BITS);

    INDIRECT(1); PTR_INIT(1); FUNC(3); ADD(1);
    writtenNoBits += appendBitstream( hBitStream, &h_ps_e->psBitBuf, numBits );

    LOGIC(1);
    writtenNoBits = writtenNoBits%8;

    BRANCH(1);
    if(writtenNoBits)
    {
      ADD(1); FUNC(3);
      WriteBits(hBitStream, 0, (8 - writtenNoBits));
    }

    FUNC(1); ADD(2); /* counting post-operation */
    COUNT_sub_end();

    return GetBitsAvail(hBitStream)-(*sbrHdrBits)-SI_FILL_EXTENTION_BITS;
  }

  COUNT_sub_end();
}
